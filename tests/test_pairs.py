"""Direct pair/bounds truth; actual native execution belongs to the fleet."""
import math
import ctypes
from dataclasses import FrozenInstanceError
from concurrent.futures import ThreadPoolExecutor
from itertools import repeat
from unittest.mock import patch
import os
import struct
import unittest


def engine_arguments():
    return dict(backend=os.environ.get("BREPGPU_TEST_BACKEND", "cpu"),
                device=int(os.environ.get("BREPGPU_TEST_DEVICE", "0")),
                threads=int(os.environ.get("BREPGPU_TEST_THREADS", "1")),
                plugin_path=os.environ.get("BREPGPU_TEST_PLUGIN"))


class PairTests(unittest.TestCase):
    def test_closed_box_contact_is_not_lost(self):
        from brepgpu import Engine
        # A healthy existing Engine precedes the absent-operation direct red.
        with Engine(**engine_arguments()) as engine:
            point = (0., 0., 0., 0., 0., 0.)
            separated = (1., 0., 0., 1., 0., 0.)
            with engine.create_pair_index((point,)) as left, \
                    engine.create_pair_index((point, separated)) as right:
                with engine.discover_pairs(left, right, start=0, max_pairs=2) as result:
                    batch = result.download()
            self.assertEqual(tuple(batch.words), (1,))
            info = batch.info
            self.assertEqual((info.total_pairs, info.start, info.count, info.next_start,
                              info.left_count, info.right_count, info.word_count),
                             (2, 0, 2, 2, 1, 2, 1))
            self.assertEqual(info.flags & 3, 3)
            self.assertFalse(info.flags & 4)
            self.assertTrue(math.isnan(info.kernel_ms))


    def test_exact_extreme_subnormal_and_closed_touch_bit_truth(self):
        from brepgpu import Engine
        from brepgpu.pair_oracles import qualify_page, same_boxes
        tiny = math.nextafter(0., 1.)
        maximum = float.fromhex("0x1.fffffffffffffp+1023")
        boxes = ((-0., -0., -0., 0., 0., 0.), (tiny, 0., 0., tiny, 0., 0.),
                 (-tiny, 0., 0., -tiny, 0., 0.), (0., 0., 0., 1., 1., 1.),
                 (1., 0., 0., 2., 1., 1.), (1., 1., 0., 2., 2., 1.),
                 (1., 1., 1., 2., 2., 2.), (-maximum, 0., 0., -maximum, 0., 0.),
                 (maximum, 0., 0., maximum, 0., 0.))
        with Engine(**engine_arguments()) as engine:
            with engine.create_pair_index(boxes) as left, engine.create_pair_index(tuple(reversed(boxes))) as right:
                self.assertEqual(left.info().count, len(boxes))
                self.assertEqual(left.info().backend, int(engine.backend == "cuda"))
                same_boxes(left.download(), boxes)
                with engine.discover_pairs(left, right) as result:
                    batch = result.download()
                self.assertEqual(batch.info.backend, int(engine.backend == "cuda"))
                stats = qualify_page(batch, left.download(), right.download(), start=0, max_pairs=1048576, profiled=False)
                self.assertEqual(stats["count"], 81)

    def test_asymmetric_offsets_tails_and_complete_page_walk(self):
        from brepgpu import Engine
        from brepgpu.pair_oracles import qualify_page, expected_words
        left_boxes = tuple((float(i), 0., 0., float(i+1), 1., 1.) for i in range(3))
        right_boxes = tuple((float(i)/2, 0., 0., float(i)/2, 0., 0.) for i in range(11))
        with Engine(**engine_arguments()) as engine:
            with engine.create_pair_index(left_boxes) as left, engine.create_pair_index(right_boxes) as right:
                for start in (0, 1, 31, 32, 33):
                    for budget in (1, 31, 32, 33, 1048576):
                        with self.subTest(start=start, budget=budget):
                            with engine.discover_pairs(left, right, start=start, max_pairs=budget) as result:
                                qualify_page(result.download(), left_boxes, right_boxes, start=start,
                                             max_pairs=budget, profiled=False)
                examined = []
                cursor = 0
                while cursor < 33:
                    with engine.discover_pairs(left, right, start=cursor, max_pairs=7) as result:
                        batch = result.download()
                    qualify_page(batch, left_boxes, right_boxes, start=cursor, max_pairs=7)
                    examined.extend((cursor+k, bool(batch.words[k//32] & (1 << (k%32))))
                                    for k in range(batch.info.count))
                    cursor = batch.info.next_start
                whole = expected_words(left_boxes, right_boxes)
                self.assertEqual(examined, [(k, bool(whole[k//32] & (1 << (k%32)))) for k in range(33)])

    def test_empty_sources_profile_and_admission_rejections(self):
        from brepgpu import Engine, BgError
        from brepgpu.pair_oracles import qualify_page
        point = (0.,)*6
        with Engine(**engine_arguments(), profile=True) as engine:
            for left_boxes, right_boxes in (((), (point,)), ((point,), ()), ((), ()), ((point,), (point,))):
                with engine.create_pair_index(left_boxes) as left, engine.create_pair_index(right_boxes) as right:
                    self.assertEqual(left.info().backend, int(engine.backend == "cuda"))
                    with engine.discover_pairs(left, right) as result:
                        batch = result.download()
                        self.assertEqual(batch.info.backend, int(engine.backend == "cuda"))
                        qualify_page(batch, left_boxes, right_boxes, start=0,
                                     max_pairs=1048576, profiled=True)
                    for start, budget in ((-1, 1), (len(left_boxes)*len(right_boxes)+1, 1), (0, 0), (0, 1048577)):
                        with self.assertRaises((BgError, ValueError, OverflowError)):
                            engine.discover_pairs(left, right, start=start, max_pairs=budget)
            for box in ((0.,)*5, (0.,)*7, (1., 0., 0., 0., 0., 0.), (math.inf, 0., 0., math.inf, 0., 0.),
                        (math.nan, 0., 0., 0., 0., 0.)):
                with self.assertRaises((BgError, ValueError, TypeError)):
                    engine.create_pair_index((box,))
            with self.assertRaises((BgError, ValueError)):
                engine.create_pair_index(repeat(point, 65537))

    def test_uint64_total_last_pair_terminal_and_maximum_page(self):
        from brepgpu import Engine
        point = (0.,)*6
        with Engine(**engine_arguments()) as engine:
            with engine.create_pair_index(repeat(point, 65536)) as left, \
                    engine.create_pair_index(repeat(point, 65536)) as right:
                for start, count in (((1 << 32)-1, 1), (1 << 32, 0)):
                    with engine.discover_pairs(left, right, start=start, max_pairs=33) as result:
                        batch = result.download()
                    self.assertEqual((batch.info.total_pairs, batch.info.start, batch.info.count,
                                      batch.info.next_start, batch.info.left_count, batch.info.right_count),
                                     (1 << 32, start, count, 1 << 32, 65536, 65536))
                    self.assertEqual(batch.info.flags, 1)
                    self.assertEqual(tuple(batch.words), (1,) if count else ())
                with engine.discover_pairs(left, right, max_pairs=1 << 20) as result:
                    batch = result.download()
                self.assertEqual((batch.info.count, batch.info.word_count, batch.info.next_start),
                                 (1 << 20, 1 << 15, 1 << 20))
                self.assertEqual(batch.info.flags, 0)
                # All identical zero-width points touch: literal all-one truth
                # proves every actual capped bit without a 2^32 allocation.
                self.assertTrue(all(word == 0xffffffff for word in batch.words))

    def test_native_snapshots_retained_parents_foreign_and_closed_context(self):
        from brepgpu import Engine, BgError
        from brepgpu.pair_oracles import qualify_page
        engine = Engine(**engine_arguments())
        try:
            source = [[-0., 0., 0., 0., 0., 0.]]
            left = engine.create_pair_index(source)
            right = engine.create_pair_index(((0.,)*6,))
            original = ((-0., 0., 0., 0., 0., 0.),)
            source[0][0] = 99.
            self.assertEqual(struct.pack('<6d', *left.download()[0]), struct.pack('<6d', *original[0]))
            with Engine(backend="cpu") as foreign:
                with foreign.create_pair_index(((0.,)*6,)) as other:
                    with self.assertRaises((BgError, ValueError)):
                        engine.discover_pairs(left, other)
            result = engine.discover_pairs(left, right)
            left.close(); right.close()
            with self.assertRaises(BgError):
                engine.discover_pairs(left, right)
            batch = result.download()
            qualify_page(batch, original, ((0.,)*6,))
            engine.close()
            with self.assertRaises(BgError):
                result.download()
            result.close()
            self.assertEqual(tuple(batch.words), (1,))
        finally:
            engine.close()

    def test_actual_raw_extents_alignment_alias_and_descriptor_rejection(self):
        from brepgpu import Engine, Diagnostic, PairBox, PairOptionsDesc, PairIndexInfo, PairPageInfo
        point = (0.,)*6
        with Engine(**engine_arguments()) as engine:
            with engine.create_pair_index((point,)) as index:
                with engine.discover_pairs(index, index) as result:
                    lib = engine._lib
                    for fault in ("short", "long", "bytes", "unaligned", "diagnostic"):
                        backing = (ctypes.c_uint64*128)(*([0x1234567812345678]*128))
                        address = ctypes.addressof(backing)
                        diag = Diagnostic()
                        dp = ctypes.pointer(diag)
                        if fault == "diagnostic":
                            dp = ctypes.cast(address, ctypes.POINTER(Diagnostic))
                            ctypes.memmove(dp, ctypes.byref(diag), ctypes.sizeof(diag))
                        before = (bytes(backing), bytes(diag))
                        words = ctypes.cast(address+(1 if fault == "unaligned" else 0), ctypes.POINTER(ctypes.c_uint32))
                        count = 0 if fault == "short" else 2 if fault == "long" else 1
                        size = 3 if fault == "bytes" else 4*count
                        self.assertEqual(lib.bg4_pair_page_download(result.handle, words, count, size, dp, ctypes.sizeof(diag)), 1)
                        self.assertEqual(bytes(backing), before[0])
                        if fault == "diagnostic": self.assertEqual(bytes(diag), before[1])
                    for count, size in ((0, 0), (2, 96), (1, 47), (1, 49)):
                        backing = (ctypes.c_uint64*128)(*([0x1234567812345678]*128));before = bytes(backing)
                        diag = Diagnostic()
                        self.assertEqual(lib.bg4_pair_index_download(index.handle, ctypes.cast(backing, ctypes.POINTER(PairBox)),
                                        count, size, ctypes.byref(diag), ctypes.sizeof(diag)), 1)
                        self.assertEqual(bytes(backing), before)
                    backing=(ctypes.c_uint64*128)();diag=Diagnostic()
                    dp=ctypes.cast(backing,ctypes.POINTER(Diagnostic))
                    ctypes.memmove(dp,ctypes.byref(diag),ctypes.sizeof(diag));before=bytes(backing)
                    self.assertEqual(lib.bg4_pair_index_download(index.handle,ctypes.cast(backing,ctypes.POINTER(PairBox)),
                                     1,48,dp,ctypes.sizeof(diag)),1)
                    self.assertEqual(bytes(backing),before)
                    borrowed=(PairBox*1)();borrowed[0].lower[:]=(123.,)*3;borrowed[0].upper[:]=(123.,)*3
                    diag=Diagnostic();before=(bytes(borrowed),bytes(diag))
                    self.assertEqual(lib.bg4_create_pair_index(engine.handle,borrowed,1,48,
                                     ctypes.cast(borrowed,ctypes.POINTER(ctypes.c_uint64)),ctypes.byref(diag),ctypes.sizeof(diag)),1)
                    self.assertEqual((bytes(borrowed),bytes(diag)),before)
                    for handle, name, record in ((index.handle, "bg4_pair_index_info", PairIndexInfo),
                                                (result.handle, "bg4_pair_page_info", PairPageInfo)):
                        backing = (ctypes.c_uint64*128)();address = ctypes.addressof(backing)
                        output = record.from_buffer(backing);output.abi_version = 4;output.struct_size = ctypes.sizeof(record)
                        diag = Diagnostic();dp = ctypes.cast(address+ctypes.sizeof(record)-8, ctypes.POINTER(Diagnostic))
                        ctypes.memmove(dp, ctypes.byref(diag), ctypes.sizeof(diag));before = bytes(backing)
                        self.assertEqual(getattr(lib, name)(handle, ctypes.byref(output), ctypes.sizeof(output), dp, ctypes.sizeof(diag)), 1)
                        self.assertEqual(bytes(backing), before)
                    for fault in ("version", "size", "bytes", "flags"):
                        options = PairOptionsDesc();options.max_pairs = 1
                        size = ctypes.sizeof(options)
                        if fault == "version": options.abi_version = 1
                        elif fault == "size": options.struct_size -= 1
                        elif fault == "bytes": size -= 1
                        else: options.flags = 1
                        out = ctypes.c_uint64(99);diag = Diagnostic()
                        self.assertEqual(lib.bg4_discover_pairs(index.handle, index.handle, ctypes.byref(options), size,
                                         ctypes.byref(out), ctypes.byref(diag), ctypes.sizeof(diag)), 1)
                        self.assertEqual(out.value, 0)
                    backing = (ctypes.c_uint64*128)();options = PairOptionsDesc.from_buffer(backing)
                    options.abi_version=4;options.struct_size=24;options.max_pairs=1
                    diag=Diagnostic();before=(bytes(backing),bytes(diag))
                    self.assertEqual(lib.bg4_discover_pairs(index.handle,index.handle,ctypes.byref(options),24,
                                     ctypes.cast(backing,ctypes.POINTER(ctypes.c_uint64)),ctypes.byref(diag),ctypes.sizeof(diag)),1)
                    self.assertEqual((bytes(backing),bytes(diag)),before)
                    stale = index.handle
                index.close()
                for handle in (0, stale, engine.handle):
                    options=PairOptionsDesc();options.max_pairs=1;out=ctypes.c_uint64(99);diag=Diagnostic()
                    self.assertEqual(lib.bg4_discover_pairs(handle,handle,ctypes.byref(options),24,
                                     ctypes.byref(out),ctypes.byref(diag),ctypes.sizeof(diag)),3)
                    self.assertEqual(out.value,0)

    def test_native_python_publication_failure_closes_new_handle(self):
        import brepgpu
        from brepgpu import Engine, Diagnostic
        point = (0.,)*6
        with Engine(**engine_arguments()) as engine:
            for constructor in ("PairIndex", "PairPage"):
                captured = [];primary = MemoryError("chosen pair wrapper allocation failure")
                def reject(engine_arg, handle):
                    captured.append(int(handle));raise primary
                with engine.create_pair_index((point,)) as left:
                    with patch.object(brepgpu, constructor, reject):
                        with self.assertRaises(MemoryError) as caught:
                            if constructor == "PairIndex": engine.create_pair_index((point,))
                            else: engine.discover_pairs(left,left)
                    self.assertIs(caught.exception,primary);self.assertEqual(len(captured),1)
                    diag=Diagnostic()
                    self.assertEqual(engine._lib.bg1_close(captured[0],ctypes.byref(diag),ctypes.sizeof(diag)),3)
            original = brepgpu._invoke;captured=[];primary=KeyboardInterrupt("chosen post-native pair interruption")
            def interrupted(lib,name,*args):
                value=original(lib,name,*args)
                if name=="bg4_create_pair_index":
                    captured.append(ctypes.cast(args[-1],ctypes.POINTER(ctypes.c_uint64))[0]);raise primary
                return value
            with patch.object(brepgpu,"_invoke",interrupted):
                with self.assertRaises(KeyboardInterrupt) as caught:engine.create_pair_index((point,))
            self.assertIs(caught.exception,primary);self.assertEqual(len(captured),1)
            diag=Diagnostic()
            self.assertEqual(engine._lib.bg1_close(captured[0],ctypes.byref(diag),ctypes.sizeof(diag)),3)

    def test_scene_selection_exact_world_bounds_and_frame_relations(self):
        from brepgpu import Engine
        from brepgpu.body import make_body, Frame
        from brepgpu.pair_scene import prepare_scene, PairDiscovery
        from brepgpu.pair_oracles import qualify_scene, qualify_page
        first=make_body("box");second=make_body("cylinder")
        with Engine(backend="cpu") as prep, Engine(**engine_arguments()) as engine:
            third=make_body("plate")
            whole=prepare_scene(prep,(first,second,third))
            expected=[(body,slot,body.faces[chart.face_slot].id,chart.id)
                      for body in (first,second,third) for slot,chart in enumerate(body.charts)]
            self.assertEqual(len(whole.sources),len(expected))
            for source,(body,slot,face_id,chart_id) in zip(whole.sources,expected):
                self.assertIs(source.snapshot,body)
                self.assertEqual((source.patch_slot,source.face_id,source.chart_id),(slot,face_id,chart_id))
            qualify_scene(whole)
            keys=((second.body_id,3),(second.body_id,1),(first.body_id,1))
            subset=prepare_scene(prep,(first,second),keys)
            expected=[(body,slot,body.faces[chart.face_slot].id,chart.id)
                      for body in (first,second) for slot,chart in enumerate(body.charts)
                      if (body.body_id,body.faces[chart.face_slot].id) in keys]
            self.assertEqual(len(subset.sources),len(expected))
            for source,(body,slot,face_id,chart_id) in zip(subset.sources,expected):
                self.assertIs(source.snapshot,body)
                self.assertEqual((source.patch_slot,source.face_id,source.chart_id),(slot,face_id,chart_id))
            qualify_scene(subset)
            selected=prepare_scene(prep,(first,second),((second.body_id,1),))
            self.assertEqual(len(selected.sources),1);self.assertIs(selected.sources[0].snapshot,second)
            qualify_scene(selected)
            for selection in ((1,),((first.body_id,1),)*2,((999999,1),),((first.body_id,999),)):
                with self.assertRaises((TypeError,ValueError)):prepare_scene(prep,(first,second),selection)
            with self.assertRaises(ValueError):prepare_scene(prep,(first,first))
            with self.assertRaises((FrozenInstanceError,AttributeError)):selected.boxes=()
            # Right/left local planes differ, but these two frames map both to x=.75.
            shifted=make_body("box",Frame((2.25,0.,0.),2.))
            left=prepare_scene(prep,(first,),((first.body_id,5),))
            right=prepare_scene(prep,(shifted,),((shifted.body_id,6),))
            with PairDiscovery(engine,left,right) as consumer:
                actual=consumer.download_boxes();qualify_scene(left,actual[0]);qualify_scene(right,actual[1])
                page=consumer.page();qualify_page(page,*actual,start=0,max_pairs=1048576)
                self.assertEqual(page.words,(1,))
                self.assertIs(page.left,left);self.assertIs(page.right,right)
            separated=make_body("box",Frame((4.,0.,0.),1.))
            a=prepare_scene(prep,(first,));b=prepare_scene(prep,(separated,))
            with PairDiscovery(engine,a,b) as consumer:
                page=consumer.page();stats=qualify_page(page,*consumer.download_boxes())
                self.assertEqual(stats["survivors"],0)
            empty=prepare_scene(prep,(first,),())
            with PairDiscovery(engine,empty,a) as consumer:
                page=consumer.page();self.assertEqual(qualify_page(page)["count"],0)

    def test_transverse_coplanar_and_parallel_carrier_cases(self):
        from brepgpu import Engine
        from brepgpu.body import make_body, Frame
        from brepgpu.pair_scene import prepare_scene, PairDiscovery
        from brepgpu.pair_oracles import qualify_scene, qualify_page
        with Engine(backend="cpu") as prep, Engine(**engine_arguments()) as engine:
            xy=make_body("box",Frame((0.,0.,-.5),1.))
            a=prepare_scene(prep,(xy,),((xy.body_id,1),))
            for body, face_id, expected in (
                    (make_body("box",Frame((-.75,0.,0.),1.)),5,1),
                    (make_body("box",Frame((.5,0.,-.5),1.)),1,1),
                    (make_body("box",Frame((0.,0.,-.5+2.**-10),1.)),1,0)):
                b=prepare_scene(prep,(body,),((body.body_id,face_id),))
                with PairDiscovery(engine,a,b) as consumer:
                    page=consumer.page();qualify_page(page,*consumer.download_boxes())
                    self.assertEqual(page.words,(expected,))
            cylinder=make_body("cylinder")
            wall=prepare_scene(prep,(cylinder,),((cylinder.body_id,3),))
            with PairDiscovery(engine,wall,a) as consumer:
                page=consumer.page();qualify_page(page,*consumer.download_boxes())
                self.assertEqual(page.words,(15,))
            for scale in (.25,4.):
                shifted=make_body("plate",Frame((16.,-16.,.125),scale))
                scene=prepare_scene(prep,(shifted,))
                qualify_scene(scene)
                with PairDiscovery(engine,scene,scene) as consumer:
                    actual=consumer.download_boxes();qualify_scene(scene,actual[0])
                    qualify_page(consumer.page(),*actual)

    def test_known_tangent_carriers_and_explicit_trim_false_positives(self):
        from brepgpu import Engine
        from brepgpu.body import make_body, Frame
        from brepgpu.pair_scene import prepare_scene, PairDiscovery
        from brepgpu.pair_oracles import qualify_page
        with Engine(backend="cpu") as prep, Engine(**engine_arguments()) as engine:
            cylinder=make_body("cylinder")
            wall=prepare_scene(prep,(cylinder,),((cylinder.body_id,3),))
            tangent=make_body("box",Frame((-.5,0.,0.),1.))
            tangent_plane=prepare_scene(prep,(tangent,),((tangent.body_id,5),))
            other=make_body("cylinder",Frame((.5,0.,0.),1.))
            other_wall=prepare_scene(prep,(other,),((other.body_id,3),))
            larger=make_body("cylinder",Frame(scale=2.))
            larger_wall=prepare_scene(prep,(larger,),((larger.body_id,3),))
            for right, required in ((tangent_plane,(0,3)),(other_wall,(1,2,13,14)),(larger_wall,(0,5,10,15))):
                with PairDiscovery(engine,wall,right) as consumer:
                    page=consumer.page();qualify_page(page,*consumer.download_boxes())
                    survivors={flat for flat,_,_ in page.survivors()}
                    self.assertTrue(set(required)<=survivors)
            plate=make_body("plate")
            cap=prepare_scene(prep,(plate,),((plate.body_id,1),))
            small=make_body("cylinder",Frame((0.,0.,.375),.25))
            inside_hole=prepare_scene(prep,(small,),((small.body_id,1),))
            with PairDiscovery(engine,cap,inside_hole) as consumer:
                page=consumer.page();qualify_page(page,*consumer.download_boxes())
                self.assertEqual(page.words,(1,))
            # The small cap's entire rectangular carrier is within radius .25
            # and z=.5: retaining it is a carrier-box false positive for trims.
            self.assertLess(2*(.25*.25)**2,.25**2)

    def test_scene_page_lifetime_and_primary_cleanup_failure(self):
        from brepgpu import Engine, BgError
        from brepgpu.body import make_body
        from brepgpu.pair_scene import prepare_scene, PairDiscovery
        from brepgpu.pair_oracles import qualify_page
        original=Engine.discover_pairs;children=[];primary=ArithmeticError("chosen pair download failure")
        def injected(engine,*args,**kwargs):
            result=original(engine,*args,**kwargs);children.append(result);close=result.close
            def fail_download():raise primary
            def fail_close():
                result.close=close;close();raise KeyboardInterrupt("chosen page cleanup failure")
            result.download=fail_download;result.close=fail_close
            return result
        with Engine(backend="cpu") as prep:
            scene=prepare_scene(prep,(make_body("box"),))
        engine=Engine(**engine_arguments())
        try:
            consumer=PairDiscovery(engine,scene,scene)
            try:
                with patch.object(Engine,"discover_pairs",injected):
                    with self.assertRaises(ArithmeticError) as caught:consumer.page()
                self.assertIs(caught.exception,primary)
                self.assertEqual(len(children),1);self.assertFalse(children[0].handle)
                self.assertTrue(getattr(primary,"brepgpu_cleanup_error",None) or getattr(primary,"__notes__",None))
                page=consumer.page(max_pairs=7);qualify_page(page,start=0,max_pairs=7)
                with self.assertRaises((FrozenInstanceError,AttributeError)):page.info.start=99
                consumer.close();consumer.close()
                with self.assertRaises(BgError):consumer.page()
                engine.close()
                qualify_page(page,start=0,max_pairs=7)
            finally:consumer.close()
        finally:engine.close()


    def test_cuda_pair_callbacks_restore_each_host_threads_device(self):
        if engine_arguments()["backend"] != "cuda":
            self.skipTest("CUDA device behavior is outside the explicit CPU backend")
        from brepgpu import Engine
        from brepgpu.pair_oracles import qualify_page, same_boxes
        runtime=ctypes.CDLL(os.environ.get("BREPGPU_CUDART","libcudart.so.12"))
        runtime.cudaGetDevice.argtypes=[ctypes.POINTER(ctypes.c_int)];runtime.cudaGetDevice.restype=ctypes.c_int
        runtime.cudaSetDevice.argtypes=[ctypes.c_int];runtime.cudaSetDevice.restype=ctypes.c_int
        runtime.cudaGetDeviceCount.argtypes=[ctypes.POINTER(ctypes.c_int)];runtime.cudaGetDeviceCount.restype=ctypes.c_int
        def current():
            value=ctypes.c_int(-1)
            self.assertEqual(runtime.cudaGetDevice(ctypes.byref(value)),0)
            return value.value
        count=ctypes.c_int();self.assertEqual(runtime.cudaGetDeviceCount(ctypes.byref(count)),0)
        self.assertGreaterEqual(count.value,2,"qualification requires both CUDA devices")
        selected=engine_arguments()["device"];other=(selected+1)%count.value;saved=current();resources=[]
        left_boxes=((0.,)*6,);right_boxes=((0.,)*6,(1.,0.,0.,1.,0.,0.))
        self.assertEqual(runtime.cudaSetDevice(other),0)
        try:
            engine=Engine(**engine_arguments(),profile=True);resources.append(engine);self.assertEqual(current(),other)
            left=engine.create_pair_index(left_boxes);resources.append(left);self.assertEqual(current(),other)
            right=engine.create_pair_index(right_boxes);resources.append(right);self.assertEqual(current(),other)
            same_boxes(left.download(),left_boxes);self.assertEqual(current(),other)
            same_boxes(right.download(),right_boxes);self.assertEqual(current(),other)
            result=engine.discover_pairs(left,right);resources.append(result);self.assertEqual(current(),other)
            batch=result.download();self.assertEqual(current(),other)
            qualify_page(batch,left_boxes,right_boxes,profiled=True)
            def worker(_):
                prior=current();self.assertEqual(runtime.cudaSetDevice(other),0)
                try:
                    with engine.create_pair_index(left_boxes) as local:
                        self.assertEqual(current(),other)
                        same_boxes(local.download(),left_boxes);self.assertEqual(current(),other)
                        with engine.discover_pairs(local,right) as child:
                            self.assertEqual(current(),other)
                            observed=child.download();self.assertEqual(current(),other)
                            qualify_page(observed,left_boxes,right_boxes,profiled=True)
                        self.assertEqual(current(),other)
                    self.assertEqual(current(),other)
                finally:self.assertEqual(runtime.cudaSetDevice(prior),0)
            with ThreadPoolExecutor(max_workers=2) as pool:list(pool.map(worker,range(3)))
            for resource in reversed(resources):
                resource.close();self.assertEqual(current(),other)
            resources.clear()
        finally:
            try:
                for resource in reversed(resources):resource.close()
            finally:self.assertEqual(runtime.cudaSetDevice(saved),0)


if __name__ == "__main__":
    unittest.main()
