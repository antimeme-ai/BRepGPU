"""Public snapshot/lifetime/failure claims with direct geometric outcomes."""

from array import array
from concurrent.futures import ThreadPoolExecutor
import ctypes
from fractions import Fraction
import math
import os
import unittest

from brepgpu.fixtures import exact_component_error, exact_conversion, exact_jet, plane, weighted_bilinear
from test_qualification import assert_jet, patch_for, vector_at


class RuntimeTests(unittest.TestCase):
    def setUp(self):
        from brepgpu import Engine
        self.engine = Engine(backend=os.environ.get("BREPGPU_TEST_BACKEND", "cpu"),
                             device=int(os.environ.get("BREPGPU_TEST_DEVICE", "0")))

    def tearDown(self):
        self.engine.close()

    def make_queries(self, patches, uv=(0.25, 0.75), slots=(0,)):
        return self.engine.create_query_set(patches, patch_slots=array("I", slots), uv=array("d", uv))

    def test_caller_mutation_cannot_change_owned_patch_or_query_snapshot(self):
        from brepgpu import Patch
        f = weighted_bilinear()
        controls = array("d", f.controls)
        with self.engine.create_patch_set([Patch(f.degree_u, f.degree_v, controls)]) as patches:
            slots, uv = array("I", [0]), array("d", [0.25, 0.75])
            with self.engine.create_query_set(patches, patch_slots=slots, uv=uv) as queries:
                controls[:] = array("d", [math.nan] * len(controls))
                slots[0] = 0xFFFFFFFF
                uv[:] = array("d", [math.nan, math.inf])
                with self.engine.evaluate(queries, order=2) as result:
                    jets = result.download()
                jets.require_ok()
                assert_jet(self, jets, 0, exact_jet(f, 0.25, 0.75), label="snapshot")

    def test_result_and_query_keep_closed_parent_storage_alive(self):
        from brepgpu import BgError, Lineage, Patch
        f = weighted_bilinear()
        source = Lineage(body_id=101, entity_kind=5, entity_id=303, revision=7, chart_id=11)
        patches = self.engine.create_patch_set([Patch(f.degree_u, f.degree_v, f.controls, lineage=source)])
        queries = self.make_queries(patches)
        patches.close()
        with self.engine.evaluate(queries, order=2) as result:
            queries.close()
            jets = result.download()
            jets.require_ok()
            assert_jet(self, jets, 0, exact_jet(f, 0.25, 0.75), label="closed parents")
            self.assertEqual(jets.lineage[0], source)
            with self.assertRaises(BgError):
                self.engine.evaluate(queries, order=2)
        patches.close()
        queries.close()

    def test_same_sized_patch_sets_have_distinct_query_bindings(self):
        from brepgpu.fixtures import Fixture
        a = plane()
        b = Fixture("other plane", 1, 1, tuple(
            (-x if axis < 3 else x)
            for i in range(a.control_count) for axis,x in enumerate(a.controls[4*i:4*i+4])))
        with self.engine.create_patch_set([patch_for(a)]) as patches_a:
            with self.engine.create_patch_set([patch_for(b)]) as patches_b:
                with self.make_queries(patches_a) as query_a, self.make_queries(patches_b) as query_b:
                    for query, f in ((query_a,a), (query_b,b), (query_a,a)):
                        with self.engine.evaluate(query, order=2) as result:
                            jets = result.download()
                        jets.require_ok()
                        assert_jet(self, jets, 0, exact_jet(f,0.25,0.75), label="binding " + f.name)

    def test_later_evaluation_and_download_mutation_do_not_overwrite_result(self):
        f = weighted_bilinear()
        with self.engine.create_patch_set([patch_for(f)]) as patches, self.make_queries(patches) as query:
            with self.engine.evaluate(query, order=2) as first:
                original = first.download()
                saved = tuple(original.data)
                original.data[0] = -12345.0
                with self.engine.evaluate(query, order=0) as second:
                    second.download().require_ok()
                    recovered = first.download()
                    self.assertEqual(tuple(recovered.data), saved)
                    assert_jet(self, recovered, 0, exact_jet(f,0.25,0.75))

    def test_mixed_bad_queries_keep_status_order_and_fill_every_failed_field(self):
        from brepgpu import QueryError
        f = plane()
        uv = (0.25,0.75, 0.5,0.5, math.nan,0.5,
              math.nextafter(0.0,-math.inf),0.5, 0.5,math.nextafter(1.0,math.inf),
              0.5,math.inf, 1.0,0.0)
        with self.engine.create_patch_set([patch_for(f)]) as patches:
            with self.make_queries(patches, uv, (0,0xFFFFFFFF,0,0,0,0,0)) as query:
                for order, terms in ((0,1),(1,3),(2,6)):
                    with self.engine.evaluate(query,order=order) as result:
                        jets = result.download()
                    self.assertEqual(list(jets.statuses), [0,1,2,2,2,2,0])
                    with self.assertRaises(QueryError):
                        jets.require_ok()
                    for request in range(1,6):
                        for term in range(terms):
                            self.assertTrue(all(math.isnan(x) for x in vector_at(jets,request,term)))
                    assert_jet(self,jets,0,exact_jet(f,0.25,0.75,order))
                    assert_jet(self,jets,6,exact_jet(f,1.0,0.0,order))

    def test_structural_rejection_does_not_damage_existing_inputs(self):
        from brepgpu import BgError
        f = plane()
        with self.engine.create_patch_set([patch_for(f)]) as patches:
            with self.make_queries(patches) as query:
                with self.assertRaises((BgError,ValueError,TypeError)):
                    self.engine.create_query_set(patches,patch_slots=array("I",[0]),uv=array("d",[0.5]))
                with self.assertRaises((BgError,ValueError,TypeError)):
                    self.engine.evaluate(query,order=3)
                with self.engine.evaluate(query,order=2) as result:
                    jets=result.download()
                jets.require_ok()
                assert_jet(self,jets,0,exact_jet(f,0.25,0.75))

    def test_readonly_and_strided_inputs_are_copied_by_numeric_value(self):
        from brepgpu import Patch
        f=weighted_bilinear()
        controls=memoryview(array("d",f.controls)).toreadonly()
        slots=memoryview(array("I",[0])).toreadonly()
        spaced= array("d",[0.25,-999.0,0.75,-999.0])
        uv=memoryview(spaced)[::2]
        with self.engine.create_patch_set([Patch(f.degree_u,f.degree_v,controls)]) as patches:
            with self.engine.create_query_set(patches,patch_slots=slots,uv=uv) as query:
                spaced[0]=math.nan
                with self.engine.evaluate(query,order=2) as result:
                    jets=result.download()
        jets.require_ok()
        assert_jet(self,jets,0,exact_jet(f,0.25,0.75),label="strided logical values")

    def test_foreign_context_and_context_close_reject_stale_operations(self):
        from brepgpu import BgError, Engine
        f=plane()
        with self.engine.create_patch_set([patch_for(f)]) as patches, self.make_queries(patches) as query:
            with Engine(backend="cpu") as foreign:
                with self.assertRaises(BgError):
                    foreign.evaluate(query,order=2)
            result=self.engine.evaluate(query,order=2)
            self.engine.close()
            with self.assertRaises(BgError):
                result.download()
            result.close()

    def test_native_calls_from_other_threads_keep_geometric_results(self):
        f=weighted_bilinear()
        with self.engine.create_patch_set([patch_for(f)]) as patches, self.make_queries(patches) as query:
            def run(order):
                with self.engine.evaluate(query,order=order) as result:
                    return result.download()
            with ThreadPoolExecutor(max_workers=3) as pool:
                results=list(pool.map(run,(0,1,2,2,1,0)))
            for order,jets in zip((0,1,2,2,1,0),results):
                jets.require_ok()
                assert_jet(self,jets,0,exact_jet(f,0.25,0.75,order),label="host thread")

    def test_explicit_world_source_conversion_matches_exact_supplied_values(self):
        from brepgpu import Lineage, Patch
        f=weighted_bilinear()
        source=Lineage(body_id=12,entity_kind=5,entity_id=34,revision=56,chart_id=78)
        for origin,scale,du,dv in (((0.5,-0.25,1.0),8.0,0.25,2.0),
                                  ((2.0**40,-2.0**40,0.0),2.0**-20,2.0**20,2.0**-20),
                                  ((0.1,0.2,0.3),3.0,5.0,7.0)):
            with self.subTest(origin=origin,scale=scale,du=du,dv=dv):
                p=Patch(f.degree_u,f.degree_v,f.controls,lineage=source,
                        uv=(0.0,du,0.0,dv),origin=origin,scale=scale)
                with self.engine.create_patch_set([p]) as patches, self.make_queries(patches, (0.5,0.5)) as query:
                    with self.engine.evaluate(query,order=2) as result:
                        local=result.download()
                    converted=local.convert()
                converted.require_ok()
                self.assertEqual(list(converted.statuses),list(local.statuses))
                self.assertEqual(converted.lineage,local.lineage)
                local_vectors=tuple(vector_at(local,0,k) for k in range(6))
                expected=exact_conversion(local_vectors,origin,scale,du,dv)
                for term,vector in enumerate(expected):
                    for axis,truth in enumerate(vector):
                        value=vector_at(converted,0,term)[axis]
                        error=exact_component_error(value,truth)
                        self.assertIsNotNone(error)
                        budget=Fraction.from_float(32*math.ulp(float(truth)))
                        self.assertLessEqual(error,budget,f"conversion {term}/{axis}: {error} > {budget}")

    def test_conversion_refuses_wide_exponents_and_preserves_prior_query_failures(self):
        from brepgpu import Patch, QueryError
        f=plane()
        for scale,du,dv in ((2.0**600,1.0,1.0),(1.0,2.0**600,1.0),(1.0,1.0,2.0**-21)):
            with self.subTest(scale=scale,du=du,dv=dv):
                p=Patch(f.degree_u,f.degree_v,f.controls,scale=scale,uv=(0.0,du,0.0,dv))
                with self.engine.create_patch_set([p]) as patches:
                    with self.make_queries(patches,(0.5,0.5,math.nan,0.5),(0,0)) as query:
                        with self.engine.evaluate(query,order=2) as result:
                            local=result.download()
                converted=local.convert()
                self.assertEqual(list(converted.statuses),[4,2])
                with self.assertRaises(QueryError): converted.require_ok()
                self.assertTrue(all(math.isnan(x) for x in converted.data))

    def raw_diagnostic(self):
        from brepgpu import Diagnostic
        diagnostic=Diagnostic()
        diagnostic.abi_version=1
        diagnostic.struct_size=ctypes.sizeof(diagnostic)
        return diagnostic

    def test_raw_wrong_kind_and_stale_ids_reject_without_result_publication(self):
        f=plane()
        api=self.engine._lib
        with self.engine.create_patch_set([patch_for(f)]) as patches:
            query=self.make_queries(patches)
            stale=query.handle
            query.close()
            for handle in (patches.handle,stale,0):
                with self.subTest(handle=handle):
                    out=ctypes.c_uint64(0xDEADBEEF)
                    diagnostic=self.raw_diagnostic()
                    code=api.bg1_evaluate(handle,2,ctypes.byref(out),ctypes.byref(diagnostic),ctypes.sizeof(diagnostic))
                    self.assertEqual(code,3)  # BG_INVALID_HANDLE, canonical public header.
                    self.assertEqual(out.value,0)
                    self.assertEqual(diagnostic.code,code)
            with self.make_queries(patches) as valid:
                with self.engine.evaluate(valid,order=2) as result:
                    jets=result.download()
                jets.require_ok()
                assert_jet(self,jets,0,exact_jet(f,0.25,0.75))

    def test_raw_download_rejection_preserves_both_destination_sentinels(self):
        f=plane()
        api=self.engine._lib
        with self.engine.create_patch_set([patch_for(f)]) as patches, self.make_queries(patches) as query:
            with self.engine.evaluate(query,order=2) as result:
                for capacity in (0,17,19):
                    with self.subTest(capacity=capacity):
                        # Allocate the whole declared readable/writable capacity,
                        # including surplus capacity, rather than forge a pointer.
                        values=(ctypes.c_double*max(18,capacity))(*([1234567.125]*max(18,capacity)))
                        statuses=(ctypes.c_uint32*1)(0xA5A5A5A5)
                        before=(tuple(values),tuple(statuses))
                        diagnostic=self.raw_diagnostic()
                        code=api.bg1_download(result.handle,values,capacity,statuses,1,
                                              ctypes.byref(diagnostic),ctypes.sizeof(diagnostic))
                        self.assertEqual(code,1)
                        self.assertEqual((tuple(values),tuple(statuses)),before)
                memory=(ctypes.c_ubyte*(18*8+8))(*([0x6B]*(18*8+8)))
                misaligned=ctypes.cast(ctypes.addressof(memory)+1,ctypes.POINTER(ctypes.c_double))
                statuses=(ctypes.c_uint32*1)(0xA5A5A5A5)
                before=(bytes(memory),tuple(statuses))
                diagnostic=self.raw_diagnostic()
                code=api.bg1_download(result.handle,misaligned,18,statuses,1,
                                      ctypes.byref(diagnostic),ctypes.sizeof(diagnostic))
                self.assertEqual(code,1)
                self.assertEqual((bytes(memory),tuple(statuses)),before)
                jets=result.download()
                jets.require_ok()
                assert_jet(self,jets,0,exact_jet(f,0.25,0.75))

    def test_raw_version_size_and_control_rejections_leave_handles_unpublished(self):
        from brepgpu import PatchDesc
        api=self.engine._lib
        f=plane()
        def descriptor():
            d=PatchDesc()
            d.abi_version=1;d.struct_size=ctypes.sizeof(d)
            d.degree_u=d.degree_v=1;d.control_offset=0;d.control_count=4
            d.uv[:]=(0.0,1.0,0.0,1.0);d.origin[:]=(0.0,0.0,0.0);d.scale=1.0
            return d
        for fault in ("version","size","bytes","flags","zero-weight","wide-control","nan"):
            with self.subTest(fault=fault):
                d=descriptor()
                controls=(ctypes.c_double*len(f.controls))(*f.controls)
                size=ctypes.sizeof(d)
                if fault=="version":d.abi_version=999
                elif fault=="size":d.struct_size-=1
                elif fault=="bytes":size-=1
                elif fault=="flags":d.flags=0x80000000
                elif fault=="zero-weight":controls[3]=0.0
                elif fault=="wide-control":controls[0]=2.0
                elif fault=="nan":controls[0]=math.nan
                out=ctypes.c_uint64(0xDEADBEEF)
                diagnostic=self.raw_diagnostic()
                code=api.bg1_create_patch_set(self.engine.handle,ctypes.byref(d),1,size,
                                               controls,len(controls),ctypes.byref(out),
                                               ctypes.byref(diagnostic),ctypes.sizeof(diagnostic))
                self.assertIn(code,(1,2))
                self.assertEqual(out.value,0)
                self.assertEqual(diagnostic.code,code)

    def test_raw_partial_homogeneous_record_is_structurally_rejected(self):
        from brepgpu import PatchDesc
        # All five declared doubles are genuinely allocated. The valid first
        # record does not make a trailing incomplete record a valid snapshot.
        descriptor=PatchDesc()
        descriptor.degree_u=descriptor.degree_v=0
        descriptor.control_offset=0;descriptor.control_count=1
        descriptor.uv[:]=(0.0,1.0,0.0,1.0)
        descriptor.origin[:]=(0.0,0.0,0.0);descriptor.scale=1.0
        controls=(ctypes.c_double*5)(0.25,0.5,0.75,1.0,123.0)
        out=ctypes.c_uint64(0xDEADBEEF)
        diagnostic=self.raw_diagnostic()
        code=self.engine._lib.bg1_create_patch_set(
            self.engine.handle,ctypes.byref(descriptor),1,ctypes.sizeof(descriptor),
            controls,5,ctypes.byref(out),ctypes.byref(diagnostic),ctypes.sizeof(diagnostic))
        self.assertEqual(code,1)
        self.assertEqual(out.value,0)
        self.assertEqual(diagnostic.code,code)

    def test_raw_result_metadata_confirms_selected_backend_and_rejected_info_writes_nothing(self):
        from brepgpu import ResultInfo
        f=plane()
        api=self.engine._lib
        with self.engine.create_patch_set([patch_for(f)]) as patches, self.make_queries(patches) as query:
            with self.engine.evaluate(query,order=2) as result:
                info=ResultInfo()
                info.abi_version=1;info.struct_size=ctypes.sizeof(info)
                diagnostic=self.raw_diagnostic()
                code=api.bg1_result_info(result.handle,ctypes.byref(info),ctypes.sizeof(info),
                                         ctypes.byref(diagnostic),ctypes.sizeof(diagnostic))
                self.assertEqual(code,0)
                self.assertEqual((info.count,info.order,info.terms),(1,2,6))
                self.assertEqual(info.backend,1 if os.environ.get("BREPGPU_TEST_BACKEND","cpu")=="cuda" else 0)
                self.assertEqual(info.flags,0)
                self.assertTrue(math.isnan(info.kernel_ms))
                for short in (0,ctypes.sizeof(info)-1):
                    before=ctypes.string_at(ctypes.addressof(info),ctypes.sizeof(info))
                    code=api.bg1_result_info(result.handle,ctypes.byref(info),short,
                                             ctypes.byref(diagnostic),ctypes.sizeof(diagnostic))
                    self.assertEqual(code,1)
                    self.assertEqual(ctypes.string_at(ctypes.addressof(info),ctypes.sizeof(info)),before)

    def test_raw_retained_sources_survive_public_parent_closure(self):
        from brepgpu import Lineage,Patch,PatchDesc
        f=weighted_bilinear()
        source=Lineage(body_id=101,entity_kind=5,entity_id=303,revision=7,chart_id=11)
        patches=self.engine.create_patch_set([Patch(f.degree_u,f.degree_v,f.controls,lineage=source)])
        query=self.make_queries(patches)
        result=self.engine.evaluate(query,order=2)
        patches.close();query.close()
        try:
            descriptors=(PatchDesc*1)()
            slots=(ctypes.c_uint32*1)(0xFFFFFFFF)
            uv=(ctypes.c_double*2)(-99.0,-99.0)
            diagnostic=self.raw_diagnostic()
            code=self.engine._lib.bg1_result_sources(result.handle,descriptors,1,ctypes.sizeof(descriptors),
                                                      slots,1,uv,2,ctypes.byref(diagnostic),ctypes.sizeof(diagnostic))
            self.assertEqual(code,0)
            d=descriptors[0]
            self.assertEqual((d.body_id,d.entity_kind,d.entity_id,d.revision,d.chart_id),(101,5,303,7,11))
            self.assertEqual((list(slots),list(uv)),([0],[0.25,0.75]))
            self.assertEqual((d.degree_u,d.degree_v,d.control_count),(1,1,4))
            self.assertEqual(list(d.uv),[0.0,1.0,0.0,1.0])
        finally:
            result.close()

    def test_cuda_operations_restore_calling_threads_prior_device(self):
        if os.environ.get("BREPGPU_TEST_BACKEND","cpu")!="cuda":
            self.skipTest("CUDA device selection is outside the explicitly selected CPU backend")
        from brepgpu import Engine
        runtime=ctypes.CDLL(os.environ.get("BREPGPU_CUDART","libcudart.so.12"))
        runtime.cudaGetDevice.argtypes=[ctypes.POINTER(ctypes.c_int)]
        runtime.cudaGetDevice.restype=ctypes.c_int
        runtime.cudaSetDevice.argtypes=[ctypes.c_int]
        runtime.cudaSetDevice.restype=ctypes.c_int
        runtime.cudaGetDeviceCount.argtypes=[ctypes.POINTER(ctypes.c_int)]
        runtime.cudaGetDeviceCount.restype=ctypes.c_int
        def current():
            device=ctypes.c_int(-1)
            self.assertEqual(runtime.cudaGetDevice(ctypes.byref(device)),0)
            return device.value
        count=ctypes.c_int(0)
        self.assertEqual(runtime.cudaGetDeviceCount(ctypes.byref(count)),0)
        self.assertGreaterEqual(count.value,2,"both-device qualification requires two CUDA devices")
        selected=int(os.environ.get("BREPGPU_TEST_DEVICE","0"))
        other=(selected+1)%count.value
        f=weighted_bilinear()
        saved=current()
        self.assertEqual(runtime.cudaSetDevice(other),0)
        try:
            engine=Engine(backend="cuda",device=selected)
            self.assertEqual(current(),other,"context creation changed caller device")
            try:
                patches=engine.create_patch_set([patch_for(f)])
                self.assertEqual(current(),other,"patch upload changed caller device")
                query=engine.create_query_set(patches,patch_slots=array("I",[0]),uv=array("d",[0.25,0.75]))
                self.assertEqual(current(),other,"query upload changed caller device")
                result=engine.evaluate(query,order=2)
                self.assertEqual(current(),other,"evaluate changed caller device")
                jets=result.download()
                self.assertEqual(current(),other,"download changed caller device")
                jets.require_ok();assert_jet(self,jets,0,exact_jet(f,0.25,0.75))
                def worker():
                    prior=current()
                    self.assertEqual(runtime.cudaSetDevice(other),0)
                    try:
                        with engine.evaluate(query,order=2) as threaded_result:
                            self.assertEqual(current(),other,"worker evaluate changed device")
                            threaded_jets=threaded_result.download()
                            self.assertEqual(current(),other,"worker download changed device")
                        self.assertEqual(current(),other,"worker cleanup changed device")
                        threaded_jets.require_ok();assert_jet(self,threaded_jets,0,exact_jet(f,0.25,0.75))
                    finally:
                        self.assertEqual(runtime.cudaSetDevice(prior),0)
                with ThreadPoolExecutor(max_workers=2) as pool:
                    list(pool.map(lambda unused:worker(),range(2)))
                query.close();self.assertEqual(current(),other,"query close changed device")
                patches.close();self.assertEqual(current(),other,"patch close changed device")
                result.close();self.assertEqual(current(),other,"result/dependency cleanup changed device")
            finally:
                engine.close()
            self.assertEqual(current(),other,"context close changed caller device")
        finally:
            self.assertEqual(runtime.cudaSetDevice(saved),0)


if __name__=="__main__":
    unittest.main()
