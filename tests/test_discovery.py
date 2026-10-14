"""Direct public-caller qualification of conservative named-patch discovery."""
from array import array
from fractions import Fraction
from concurrent.futures import ThreadPoolExecutor
from dataclasses import FrozenInstanceError, replace
import ctypes
import itertools
from unittest.mock import patch as replace_attribute
import math
import os
import unittest


from brepgpu.discovery_fixtures import (
    DiscoveryCase, discovery_cases, degree_fixture, constant_zero,
    diagonal_plane, negative_coefficients, subnormal_line, rank_one, orthogonal_plane,
    qualify_hierarchy, qualify_row, COVERAGE, VISIT_LIMIT, GAP)


def values_at(batch,index):return tuple(batch.data[k*batch.count+index] for k in range(8))


def assert_row(test,case,batch,index,oracle,slot):
    failures,_=qualify_row(case,values_at(batch,index),int(batch.statuses[index]),
        int(batch.flags[index]),int(batch.visits[index]),int(batch.pruned[index]),
        int(batch.refinements[index]),int(batch.evaluation_counts[index]),oracle,slot)
    test.assertFalse(failures,case.name+": "+"; ".join(failures))


class DiscoveryTests(unittest.TestCase):
    def setUp(self):
        from brepgpu import Engine
        self.engine=Engine(backend=os.environ.get("BREPGPU_TEST_BACKEND","cpu"),
                           device=int(os.environ.get("BREPGPU_TEST_DEVICE","0")))

    def tearDown(self):self.engine.close()

    def evaluate(self,cases,mode=0,engine=None):
        from brepgpu import Patch,HierarchyOptions,DiscoveryOptions
        engine=engine or self.engine;first=cases[0]
        fixtures=tuple(c.fixture for c in cases)
        with engine.create_patch_set([Patch(f.degree_u,f.degree_v,f.controls) for f in fixtures]) as patches:
            with engine.create_hierarchy(patches,HierarchyOptions(first.depth,mode=mode)) as hierarchy:
                failures,oracle=qualify_hierarchy(fixtures,hierarchy.download(),first.depth,mode)
                self.assertFalse(failures,"; ".join(failures));self.assertIsNotNone(oracle)
                with engine.create_discovery_query_set(hierarchy,range(len(cases)),
                        array("d",(x for c in cases for x in c.target))) as queries:
                    options=DiscoveryOptions(first.max_visits,first.max_refinements,first.tolerance_squared)
                    with engine.discover(queries,options) as result:return result.download(),oracle

    def test_constant_third_original_uv_upper_certificate(self):
        # Imports deliberately use only the existing API: missing discovery must
        # fail at the actual new operation, not an eager new-class import.
        from brepgpu import Engine, Patch
        target = float(Fraction(1, 3))
        truth = (Fraction(1, 3) - Fraction.from_float(target)) ** 2
        self.assertGreater(truth, 0)
        with Engine(backend=os.environ.get("BREPGPU_TEST_BACKEND", "cpu"),
                    device=int(os.environ.get("BREPGPU_TEST_DEVICE", "0"))) as engine:
            with engine.create_patch_set([Patch(0, 0, (1., 0., 0., 3.))]) as patches:
                with engine.create_hierarchy(patches) as hierarchy:
                    with engine.create_discovery_query_set(
                            hierarchy, array("I", [0]), array("d", [target, 0., 0.])) as queries:
                        with engine.discover(queries) as result:
                            batch = result.download()
        self.assertEqual(batch.count, 1)
        self.assertEqual(len(batch.data), 8)
        self.assertTrue(all(math.isfinite(x) for x in batch.data))
        self.assertTrue(all(0 <= u <= 1 for u in batch.data[:2]))
        lower, upper = map(Fraction.from_float, batch.data[6:8])
        self.assertLessEqual(lower, truth)
        self.assertGreaterEqual(upper, truth)
        self.assertGreater(upper, 0)
        self.assertEqual(int(batch.statuses[0]), 0)

    def test_all_modes_complete_exact_cache_and_global_witness_corpus(self):
        groups={}
        for case in discovery_cases():
            groups.setdefault((case.depth,case.max_visits,case.max_refinements,case.tolerance_squared),[]).append(case)
        for mode in range(3):
            for key,cases in groups.items():
                cases=cases[::2]+list(reversed(cases[1::2]))
                with self.subTest(mode=mode,options=key):
                    batch,oracle=self.evaluate(cases,mode)
                    self.assertEqual(batch.count,len(cases));self.assertEqual(len(batch.data),8*len(cases))
                    for index,case in enumerate(cases):
                        with self.subTest(case=case.name):assert_row(self,case,batch,index,oracle,index)

    def test_all_degree_cache_levels_and_maximum_admitted_depth(self):
        from brepgpu import Patch,HierarchyOptions
        matrix=tuple(degree_fixture(p,q,pattern) for p in range(4) for q in range(4)
                     for pattern in ("constant","asymmetric","cancellation"))
        deep=(degree_fixture(3,3,"asymmetric"),degree_fixture(3,3,"cancellation"),
              degree_fixture(3,1,"asymmetric"),degree_fixture(1,3,"cancellation"),
              degree_fixture(3,0,"asymmetric"),degree_fixture(0,3,"cancellation"),
              subnormal_line(),negative_coefficients())
        for mode in range(3):
            for fixtures,depth in [(matrix,d) for d in (0,1,2,5)]+[(deep,8),((negative_coefficients(),),12)]:
                with self.subTest(mode=mode,depth=depth):
                    with self.engine.create_patch_set([Patch(f.degree_u,f.degree_v,f.controls) for f in fixtures]) as patches:
                        with self.engine.create_hierarchy(patches,HierarchyOptions(depth,mode=mode)) as hierarchy:
                            failure,oracle=qualify_hierarchy(fixtures,hierarchy.download(),depth,mode)
                            self.assertFalse(failure,"; ".join(failure));self.assertIsNotNone(oracle)

    def test_public_accessors_gap_guard_and_independent_flags(self):
        from brepgpu import QueryError
        cases={c.name:c for c in discovery_cases()}
        for name in ("cap_tight","cap_wide","complete_wide","complete_tight_root_leaf"):
            case=cases[name];batch,oracle=self.evaluate([case])
            assert_row(self,case,batch,0,oracle,0)
            self.assertEqual(batch.uv(0),tuple(batch.data[:2]));self.assertEqual(batch.point(0),tuple(batch.data[2:5]))
            self.assertEqual(batch.bracket(0),tuple(batch.data[6:8]))
            self.assertEqual(batch.approximate_distance_squared(0),batch.data[5])
            for k,name2 in enumerate(("s","t","x","y","z","approximate_distance_squared","lower_squared","upper_squared")):
                self.assertEqual(batch.field(k,0),batch.field(name2,0))
            for k,i in ((-1,0),(8,0),(0,-1),(0,1)):
                with self.assertRaises(IndexError):batch.field(k,i)
            self.assertEqual((batch.info.field_count,batch.info.count,batch.info.patch_count),(8,1,1))
            self.assertEqual(batch.info.backend,1 if self.engine.backend=="cuda" else 0)
            self.assertEqual(batch.info.flags,0);self.assertTrue(math.isnan(batch.info.kernel_ms))
            if case.status==0:self.assertIs(batch.require_gap(),batch)
            else:
                with self.assertRaises(QueryError):batch.require_gap()

    def test_empty_single_stride_tail_and_parallel_cpu_workers(self):
        from brepgpu import Engine,Patch,HierarchyOptions,DiscoveryOptions
        case=next(c for c in discovery_cases() if c.name=="diagonal_norm2");f=case.fixture
        with Engine(backend=self.engine.backend,device=self.engine.device,threads=4) as engine:
            with engine.create_patch_set([Patch(f.degree_u,f.degree_v,f.controls)]) as patches:
                with engine.create_hierarchy(patches,HierarchyOptions(4,mode=2)) as hierarchy:
                    failures,oracle=qualify_hierarchy((f,),hierarchy.download(),4,2)
                    self.assertFalse(failures)
                    for n in (0,1,32,257,4096):
                        with self.subTest(count=n):
                            with engine.create_discovery_query_set(hierarchy,array("I",[0])*n,
                                    array("d",case.target)*n) as queries:
                                with engine.discover(queries) as result:batch=result.download()
                            self.assertEqual(batch.count,n);self.assertEqual(len(batch.data),8*n)
                            for i in range(n):assert_row(self,case,batch,i,oracle,0)

    def test_invalid_rows_have_precedence_nan_payload_and_zero_work(self):
        from brepgpu import Patch,HierarchyOptions,QueryError
        f=diagonal_plane()
        with self.engine.create_patch_set([Patch(f.degree_u,f.degree_v,f.controls)]) as patches:
            with self.engine.create_hierarchy(patches,HierarchyOptions(2)) as hierarchy:
                targets=(math.nan,0,0, math.nan,0,0, 0,math.inf,0, 4.000000000000001,0,0, .75,.25,.5)
                with self.engine.create_discovery_query_set(hierarchy,[0xFFFFFFFF,0,0,0,0],targets) as queries:
                    with self.engine.discover(queries) as result:batch=result.download()
                self.assertEqual(list(batch.statuses),[2,3,3,3,int(batch.statuses[4])])
                for i in range(4):
                    self.assertTrue(all(math.isnan(v) for v in values_at(batch,i)))
                    self.assertEqual((batch.flags[i],batch.visits[i],batch.pruned[i],batch.refinements[i],batch.evaluation_counts[i]),(0,0,0,0,0))
                self.assertTrue(batch.flags[4]&1)
                with self.assertRaises(QueryError):batch.require_gap()

    def test_frozen_options_private_input_copies_and_local_frame_truth(self):
        from brepgpu import Patch,Lineage,HierarchyOptions,DiscoveryOptions
        f=diagonal_plane();lineage=Lineage(17,5,23,29,31)
        controls=array("d",f.controls);slots=array("I",[0]);targets=array("d",[.75,.25,.5])
        patch=Patch(1,1,controls,lineage=lineage,uv=(10.,20.,-3.,4.),origin=(1024.,-2048.,4096.),scale=16.)
        hop=HierarchyOptions(2,mode="dop26");options=DiscoveryOptions(1,0,1.)
        for value,attribute in ((hop,"depth"),(hop,"mode"),(options,"max_visits"),(patch,"controls")):
            with self.assertRaises(FrozenInstanceError):setattr(value,attribute,99)
        with self.engine.create_patch_set([patch]) as patches:
            with self.engine.create_hierarchy(patches,hop) as hierarchy:
                with self.engine.create_discovery_query_set(hierarchy,slots,targets) as query:
                    controls[0]=math.nan;slots[0]=0xFFFFFFFF;targets[0]=math.nan
                    with self.engine.discover(query,options) as result:
                        first=result.download();original=tuple(first.data)
                        self.assertEqual(first.lineage,(lineage,));self.assertEqual(tuple(first.targets),(.75,.25,.5))
                        self.assertEqual(first.hierarchy_options,hop);self.assertEqual(first.options,options)
                        self.assertEqual(first.point(0),(.5,.5,.5));self.assertGreaterEqual(first.bracket(0)[1],.125)
                        first.data[0]=99.;first.targets[0]=99.
                        second=result.download();self.assertEqual(tuple(second.data),original)
                        self.assertEqual(tuple(second.targets),(.75,.25,.5))

    def test_retained_source_handles_same_size_binding_and_host_threads(self):
        from brepgpu import Patch,HierarchyOptions,DiscoveryOptions,Lineage
        a,b=diagonal_plane(),orthogonal_plane();la,lb=Lineage(1,5,101,1,1),Lineage(1,5,102,2,1)
        patches=self.engine.create_patch_set([Patch(a.degree_u,a.degree_v,a.controls,lineage=la)])
        other=self.engine.create_patch_set([Patch(b.degree_u,b.degree_v,b.controls,lineage=lb)])
        hierarchy=self.engine.create_hierarchy(patches,HierarchyOptions(2));foreign=self.engine.create_hierarchy(other,HierarchyOptions(2))
        query=self.engine.create_discovery_query_set(hierarchy,[0],[.75,.25,.5]);result=self.engine.discover(query,DiscoveryOptions(1,0,1.))
        patches.close();other.close();hierarchy.close();foreign.close();query.close()
        try:
            batch=result.download();self.assertEqual(batch.lineage,(la,));self.assertEqual(tuple(batch.targets),(.75,.25,.5))
            self.assertEqual(batch.point(0),(.5,.5,.5));self.assertEqual(batch.options,DiscoveryOptions(1,0,1.))
        finally:result.close()
        with self.engine.create_patch_set([Patch(a.degree_u,a.degree_v,a.controls)]) as patches:
            with self.engine.create_hierarchy(patches,HierarchyOptions(2)) as hierarchy:
                backing=array("d",[.75,-999.,.25,-999.,.5,-999.])
                with self.engine.create_discovery_query_set(hierarchy,memoryview(array("I",[0])).toreadonly(),memoryview(backing)[::2].toreadonly()) as queries:
                    backing[0]=math.nan
                    def run(_):
                        with self.engine.discover(queries) as result:return result.download()
                    with ThreadPoolExecutor(max_workers=3) as pool:batches=list(pool.map(run,range(6)))
                    for batch in batches:
                        self.assertEqual(tuple(batch.targets),(.75,.25,.5));self.assertTrue(batch.flags[0]&1)

    def test_admission_options_shapes_wrong_kinds_and_context_close(self):
        from brepgpu import Engine,Patch,BgError,HierarchyOptions,DiscoveryOptions
        f=diagonal_plane()
        with self.engine.create_patch_set([Patch(1,1,f.controls)]) as patches:
            for hop in (HierarchyOptions(13),HierarchyOptions(2,max_nodes=6),HierarchyOptions(mode=3),HierarchyOptions(max_nodes=1048577)):
                with self.assertRaises(BgError):self.engine.create_hierarchy(patches,hop)
            with self.engine.create_hierarchy(patches,HierarchyOptions(2)) as hierarchy:
                for target in ((1.,2.),(1.,2.,3.,4.)):
                    with self.assertRaises((BgError,ValueError)):self.engine.create_discovery_query_set(hierarchy,[0],target)
                with self.engine.create_discovery_query_set(hierarchy,[0],[.75,.25,.5]) as query:
                    for options in (DiscoveryOptions(0),DiscoveryOptions(1048577),DiscoveryOptions(max_refinements=65),
                                    DiscoveryOptions(tolerance_squared=math.nan),DiscoveryOptions(tolerance_squared=-1.),DiscoveryOptions(tolerance_squared=1.0000000000000002)):
                        with self.assertRaises(BgError):self.engine.discover(query,options)
                    with Engine(backend="cpu") as foreign:
                        with self.assertRaises(BgError):foreign.discover(query)
                    with self.assertRaises(BgError):self.engine.evaluate(query)
                    with self.assertRaises(BgError):self.engine.project(query)
                    with self.assertRaises(BgError):self.engine.discover(patches)
                    result=self.engine.discover(query)
                    self.engine.close()
                    with self.assertRaises(BgError):result.download()
                    result.close()

    def test_raw_structural_options_stale_kinds_and_alias_creators(self):
        from brepgpu import Patch,HierarchyOptions,HierarchyOptionsDesc,DiscoveryOptionsDesc,Diagnostic
        f=diagonal_plane();lib=self.engine._lib
        with self.engine.create_patch_set([Patch(1,1,f.controls)]) as patches:
            with self.engine.create_hierarchy(patches,HierarchyOptions(2)) as hierarchy:
                query=self.engine.create_discovery_query_set(hierarchy,[0],[.75,.25,.5])
                try:
                    for kind,handle,name,record in (("hierarchy",patches.handle,"bg3_create_hierarchy",HierarchyOptionsDesc),
                                                    ("result",query.handle,"bg3_discover",DiscoveryOptionsDesc)):
                        for fault in ("version","size","bytes","flags","reserved"):
                            desc=record()
                            if kind=="hierarchy":desc.depth=2;desc.max_nodes=1048576
                            else:desc.max_visits=8191;desc.max_refinements=8;desc.tolerance_squared=2.**-24
                            size=ctypes.sizeof(desc)
                            if fault=="version":desc.abi_version=1
                            elif fault=="size":desc.struct_size-=1
                            elif fault=="bytes":size-=1
                            elif fault=="flags":desc.flags=3 if kind=="hierarchy" else 1
                            else:desc.reserved=1
                            out=ctypes.c_uint64(123);diag=Diagnostic()
                            with self.subTest(creator=kind,fault=fault):
                                code=getattr(lib,name)(handle,ctypes.byref(desc),size,ctypes.byref(out),ctypes.byref(diag),ctypes.sizeof(diag))
                                self.assertEqual(code,1);self.assertEqual(out.value,0)
                        desc=record()
                        if kind=="hierarchy":desc.depth=2;desc.max_nodes=1048576
                        else:desc.max_visits=8191;desc.max_refinements=8;desc.tolerance_squared=2.**-24
                        backing=(ctypes.c_double*64)();ctypes.memmove(backing,ctypes.byref(desc),ctypes.sizeof(desc))
                        borrowed=record.from_buffer(backing);out=ctypes.cast(backing,ctypes.POINTER(ctypes.c_uint64));diag=Diagnostic()
                        before=(bytes(backing),bytes(diag))
                        code=getattr(lib,name)(handle,ctypes.byref(borrowed),ctypes.sizeof(borrowed),out,ctypes.byref(diag),ctypes.sizeof(diag))
                        self.assertEqual(code,1);self.assertEqual((bytes(backing),bytes(diag)),before)
                    backing=(ctypes.c_double*64)(*([123.125]*64));slots=ctypes.cast(backing,ctypes.POINTER(ctypes.c_uint32));slots[0]=0
                    targets=(ctypes.c_double*3)(.75,.25,.5);out=ctypes.cast(backing,ctypes.POINTER(ctypes.c_uint64));diag=Diagnostic()
                    before=(bytes(backing),bytes(targets),bytes(diag))
                    code=lib.bg3_create_discovery_query_set(hierarchy.handle,slots,1,targets,3,out,ctypes.byref(diag),ctypes.sizeof(diag))
                    self.assertEqual(code,1);self.assertEqual((bytes(backing),bytes(targets),bytes(diag)),before)
                    stale=query.handle
                finally:query.close()
                desc=DiscoveryOptionsDesc();desc.max_visits=8191;desc.max_refinements=8;desc.tolerance_squared=2.**-24
                for handle in (0,stale,patches.handle,hierarchy.handle):
                    out=ctypes.c_uint64(123);diag=Diagnostic()
                    code=lib.bg3_discover(handle,ctypes.byref(desc),ctypes.sizeof(desc),ctypes.byref(out),ctypes.byref(diag),ctypes.sizeof(diag))
                    self.assertEqual(code,3);self.assertEqual(out.value,0)

    def test_raw_all_result_overlap_pairs_exact_extents_and_alignment(self):
        from brepgpu import Patch,HierarchyOptions,Diagnostic
        lib=self.engine._lib;f=diagonal_plane()
        with self.engine.create_patch_set([Patch(1,1,f.controls)]) as patches:
            with self.engine.create_hierarchy(patches,HierarchyOptions(2)) as hierarchy:
                with self.engine.create_discovery_query_set(hierarchy,[0,0],[.75,.25,.5,.75,.25,.5]) as queries:
                    with self.engine.discover(queries) as result:
                        for pair in list(itertools.combinations(range(7),2))+["extent","alignment"]:
                            backing=(ctypes.c_double*256)(*([123456.125]*256))
                            addresses=[ctypes.addressof(backing)+256*i for i in range(7)]
                            if isinstance(pair,tuple):addresses[pair[1]]=addresses[pair[0]]
                            elif pair=="alignment":addresses[0]+=1
                            pointers=[ctypes.cast(address,ctypes.POINTER(ctypes.c_double if i==0 else ctypes.c_uint32)) for i,address in enumerate(addresses)]
                            args=[result.handle,pointers[0],17 if pair=="extent" else 16]
                            for pointer in pointers[1:]:args.extend((pointer,2))
                            diag=Diagnostic();before=bytes(backing)
                            code=lib.bg3_discovery_download(*args,ctypes.byref(diag),ctypes.sizeof(diag))
                            with self.subTest(pair=pair):
                                self.assertEqual(code,1);self.assertEqual(bytes(backing),before)
                        backing=(ctypes.c_double*256)(*([123456.125]*256));base=ctypes.addressof(backing)
                        diag=Diagnostic.from_buffer(backing);initialized=Diagnostic()
                        ctypes.memmove(ctypes.byref(diag),ctypes.byref(initialized),ctypes.sizeof(diag))
                        args=[result.handle,ctypes.cast(base,ctypes.POINTER(ctypes.c_double)),16]
                        for i in range(1,7):args.extend((ctypes.cast(base+256*i,ctypes.POINTER(ctypes.c_uint32)),2))
                        before=bytes(backing)
                        self.assertEqual(lib.bg3_discovery_download(*args,ctypes.byref(diag),ctypes.sizeof(diag)),1)
                        self.assertEqual(bytes(backing),before)
                        self.assertTrue(all(math.isfinite(x) for x in result.download().data))

    def test_raw_hierarchy_bounds_info_and_sources_no_write_aliases(self):
        from brepgpu import Patch,HierarchyOptions,HierarchyNode,HierarchyInfo,PatchDesc,HierarchyOptionsDesc,DiscoveryOptionsDesc,Diagnostic
        f=constant_zero();lib=self.engine._lib
        with self.engine.create_patch_set([Patch(0,0,f.controls)]) as patches:
            with self.engine.create_hierarchy(patches,HierarchyOptions(0,mode=2)) as hierarchy:
                backing=(ctypes.c_double*640)(*([123456.125]*640));address=ctypes.addressof(backing)
                nodes=ctypes.cast(address,ctypes.POINTER(HierarchyNode));roots=ctypes.cast(address,ctypes.POINTER(ctypes.c_uint32))
                diag=Diagnostic();before=bytes(backing)
                self.assertEqual(lib.bg3_hierarchy_download(hierarchy.handle,nodes,1,80,roots,1,ctypes.byref(diag),ctypes.sizeof(diag)),1)
                self.assertEqual(bytes(backing),before)
                bounds=ctypes.cast(address,ctypes.POINTER(ctypes.c_double))
                diag=Diagnostic();diag_view=Diagnostic.from_buffer(backing)
                ctypes.memmove(ctypes.byref(diag_view),ctypes.byref(diag),ctypes.sizeof(diag));before=bytes(backing)
                self.assertEqual(lib.bg3_hierarchy_bound_download(hierarchy.handle,bounds,490,ctypes.byref(diag_view),ctypes.sizeof(diag_view)),1)
                self.assertEqual(bytes(backing),before)
                for extent in (489,491):
                    diag=Diagnostic();before=bytes(backing)
                    self.assertEqual(lib.bg3_hierarchy_bound_download(hierarchy.handle,bounds,extent,ctypes.byref(diag),ctypes.sizeof(diag)),1)
                    self.assertEqual(bytes(backing),before)
                info=HierarchyInfo.from_buffer(backing);info.abi_version=3;info.struct_size=40;before=bytes(backing)
                self.assertEqual(lib.bg3_hierarchy_info(hierarchy.handle,ctypes.byref(info),40,ctypes.byref(diag_view),ctypes.sizeof(diag_view)),1)
                self.assertEqual(bytes(backing),before)
                with self.engine.create_discovery_query_set(hierarchy,[0],[0.,0.,0.]) as query:
                    with self.engine.discover(query) as result:
                        for pair in list(itertools.combinations(range(5),2))+["diagnostic"]:
                            backing=(ctypes.c_double*640)(*([123456.125]*640));base=ctypes.addressof(backing)
                            addresses=[base+512*i for i in range(5)]
                            if isinstance(pair,tuple):addresses[pair[1]]=addresses[pair[0]]
                            descriptors=ctypes.cast(addresses[0],ctypes.POINTER(PatchDesc))
                            slots=ctypes.cast(addresses[1],ctypes.POINTER(ctypes.c_uint32))
                            targets=ctypes.cast(addresses[2],ctypes.POINTER(ctypes.c_double))
                            hop=ctypes.cast(addresses[3],ctypes.POINTER(HierarchyOptionsDesc))
                            options=ctypes.cast(addresses[4],ctypes.POINTER(DiscoveryOptionsDesc))
                            hop.contents.abi_version=3;hop.contents.struct_size=32
                            options.contents.abi_version=3;options.contents.struct_size=32
                            diag=Diagnostic()
                            dp=ctypes.cast(addresses[2],ctypes.POINTER(Diagnostic)) if pair=="diagnostic" else ctypes.pointer(diag)
                            before=(bytes(backing),bytes(diag))
                            code=lib.bg3_discovery_sources(result.handle,descriptors,1,ctypes.sizeof(PatchDesc),slots,1,targets,3,
                                hop,32,options,32,dp,ctypes.sizeof(Diagnostic))
                            with self.subTest(pair=pair):
                                self.assertEqual(code,1);self.assertEqual((bytes(backing),bytes(diag)),before)
                        self.assertTrue(result.download().flags[0]&1)

    def test_empty_patch_hierarchy_and_named_query_failures(self):
        from brepgpu import HierarchyOptions
        with self.engine.create_patch_set([]) as patches:
            for mode in range(3):
                with self.engine.create_hierarchy(patches,HierarchyOptions(12,mode=mode)) as hierarchy:
                    cache=hierarchy.download();failure,oracle=qualify_hierarchy((),cache,12,mode)
                    self.assertFalse(failure);self.assertEqual(len(cache.nodes),0);self.assertEqual(len(cache.bounds),0)
                    with self.engine.create_discovery_query_set(hierarchy,[],[]) as query:
                        with self.engine.discover(query) as result:
                            batch=result.download();self.assertEqual(batch.count,0);self.assertIs(batch.require_gap(),batch)
                    with self.engine.create_discovery_query_set(hierarchy,[0],[0.,0.,0.]) as query:
                        with self.engine.discover(query) as result:batch=result.download()
                    self.assertEqual(list(batch.statuses),[2]);self.assertEqual(list(batch.flags),[0])
                    self.assertEqual(list(batch.visits),[0]);self.assertTrue(all(math.isnan(x) for x in batch.data))

    def test_python_native_success_constructor_and_interrupt_cleanup(self):
        import brepgpu
        f=diagonal_plane();engine=self.engine
        with engine.create_patch_set([brepgpu.Patch(1,1,f.controls)]) as patches:
            with engine.create_hierarchy(patches,brepgpu.HierarchyOptions(2)) as hierarchy:
                with engine.create_discovery_query_set(hierarchy,[0],[.75,.25,.5]) as queries:
                    operations=(("Hierarchy","bg3_create_hierarchy",lambda:engine.create_hierarchy(patches,brepgpu.HierarchyOptions(2))),
                                ("DiscoveryQueries","bg3_create_discovery_query_set",lambda:engine.create_discovery_query_set(hierarchy,[0],[.75,.25,.5])),
                                ("DiscoveryResult","bg3_discover",lambda:engine.discover(queries)))
                    for constructor,function,invoke in operations:
                        captured=[];chosen=MemoryError("discovery wrapper allocation witness")
                        def reject(owner,handle):captured.append(handle);raise chosen
                        with replace_attribute.object(brepgpu,constructor,reject):
                            with self.assertRaises(MemoryError) as caught:invoke()
                        self.assertIs(caught.exception,chosen);self.assertEqual(len(captured),1)
                        diagnostic=brepgpu.Diagnostic()
                        self.assertEqual(engine._lib.bg1_close(captured[0],ctypes.byref(diagnostic),ctypes.sizeof(diagnostic)),3)
                        captured=[];chosen=KeyboardInterrupt("discovery post-native publication witness");original=brepgpu._invoke
                        def after_success(lib,name,*args):
                            original(lib,name,*args)
                            if name==function:
                                captured.append(ctypes.cast(args[-1],ctypes.POINTER(ctypes.c_uint64)).contents.value)
                                raise chosen
                        with replace_attribute.object(brepgpu,"_invoke",after_success):
                            with self.assertRaises(KeyboardInterrupt) as caught:invoke()
                        self.assertIs(caught.exception,chosen);self.assertEqual(len(captured),1)
                        diagnostic=brepgpu.Diagnostic()
                        self.assertEqual(engine._lib.bg1_close(captured[0],ctypes.byref(diagnostic),ctypes.sizeof(diagnostic)),3)
                    with engine.discover(queries) as result:self.assertTrue(result.download().flags[0]&1)

    def test_cuda_discovery_callbacks_restore_each_threads_previous_device(self):
        if self.engine.backend!="cuda":self.skipTest("CUDA device behavior is outside the explicit CPU backend")
        from brepgpu import Engine,Patch,HierarchyOptions
        runtime=ctypes.CDLL(os.environ.get("BREPGPU_CUDART","libcudart.so.12"))
        runtime.cudaGetDevice.argtypes=[ctypes.POINTER(ctypes.c_int)];runtime.cudaGetDevice.restype=ctypes.c_int
        runtime.cudaSetDevice.argtypes=[ctypes.c_int];runtime.cudaSetDevice.restype=ctypes.c_int
        runtime.cudaGetDeviceCount.argtypes=[ctypes.POINTER(ctypes.c_int)];runtime.cudaGetDeviceCount.restype=ctypes.c_int
        def current():
            value=ctypes.c_int(-1);self.assertEqual(runtime.cudaGetDevice(ctypes.byref(value)),0);return value.value
        count=ctypes.c_int();self.assertEqual(runtime.cudaGetDeviceCount(ctypes.byref(count)),0)
        self.assertGreaterEqual(count.value,2,"qualification explicitly requires both CUDA devices")
        selected=self.engine.device;other=(selected+1)%count.value;saved=current();resources=[]
        self.assertEqual(runtime.cudaSetDevice(other),0)
        try:
            engine=Engine(backend="cuda",device=selected);resources.append(engine);self.assertEqual(current(),other)
            f=diagonal_plane();patches=engine.create_patch_set([Patch(1,1,f.controls)]);resources.append(patches);self.assertEqual(current(),other)
            hierarchy=engine.create_hierarchy(patches,HierarchyOptions(2,mode=2));resources.append(hierarchy);self.assertEqual(current(),other)
            cache=hierarchy.download();self.assertEqual(current(),other)
            failures,oracle=qualify_hierarchy((f,),cache,2,2);self.assertFalse(failures)
            query=engine.create_discovery_query_set(hierarchy,[0],[.75,.25,.5]);resources.append(query);self.assertEqual(current(),other)
            result=engine.discover(query);resources.append(result);self.assertEqual(current(),other)
            case=replace(next(c for c in discovery_cases() if c.name=="diagonal_norm2"),depth=2)
            observed=result.download();self.assertEqual(current(),other);assert_row(self,case,observed,0,oracle,0)
            def worker(_):
                prior=current();self.assertEqual(runtime.cudaSetDevice(other),0)
                try:
                    with engine.discover(query) as child:
                        self.assertEqual(current(),other);batch=child.download();self.assertEqual(current(),other)
                        assert_row(self,case,batch,0,oracle,0)
                    self.assertEqual(current(),other)
                finally:self.assertEqual(runtime.cudaSetDevice(prior),0)
            with ThreadPoolExecutor(max_workers=2) as pool:list(pool.map(worker,range(3)))
            for resource in reversed(resources):resource.close();self.assertEqual(current(),other)
            resources.clear()
        finally:
            try:
                for resource in reversed(resources):resource.close()
            finally:self.assertEqual(runtime.cudaSetDevice(saved),0)


if __name__=="__main__":unittest.main()
