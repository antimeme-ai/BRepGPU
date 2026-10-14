"""Independent public-caller geometry and snapshot conformance for projection."""
from array import array
from concurrent.futures import ThreadPoolExecutor
import ctypes
from dataclasses import FrozenInstanceError, replace
import itertools
import math
import os
import unittest
from unittest.mock import patch as replace_attribute

from brepgpu.projection_fixtures import (
    ProjectionCase, orthogonal_plane, projection_cases, normal_parabolic_cases, qualify_row)


def options_for(case):
    from brepgpu import ProjectionOptions
    return ProjectionOptions(case.max_iterations,case.max_backtracks,case.tolerance)


def values_at(batch,index):
    return tuple(batch.data[field*batch.count+index] for field in range(7))


def assert_row(test,case,batch,index):
    failures,_=qualify_row(case,values_at(batch,index),int(batch.statuses[index]),
                          int(batch.iterations[index]),int(batch.evaluation_counts[index]),int(batch.flags[index]))
    test.assertFalse(failures,case.name+": "+"; ".join(failures))


class ProjectionTests(unittest.TestCase):
    def setUp(self):
        from brepgpu import Engine
        self.engine=Engine(backend=os.environ.get("BREPGPU_TEST_BACKEND","cpu"),
                           device=int(os.environ.get("BREPGPU_TEST_DEVICE","0")))

    def tearDown(self):self.engine.close()

    def evaluate(self,cases,engine=None):
        from brepgpu import Patch
        engine=engine or self.engine
        with engine.create_patch_set([Patch(c.fixture.degree_u,c.fixture.degree_v,c.fixture.controls) for c in cases]) as patches:
            slots=array("I",(0xFFFFFFFF if c.status==1 else i for i,c in enumerate(cases)))
            targets=array("d",(x for c in cases for x in c.target))
            seeds=array("d",(x for c in cases for x in c.seed))
            with engine.create_projection_query_set(patches,slots,targets,seeds) as queries:
                with engine.project(queries,options_for(cases[0]) if cases else None) as result:
                    return result.download()

    def test_named_public_accessors_against_literal_plane_answer(self):
        from brepgpu import Engine, Patch, ProjectionOptions
        controls=(0.,0.,0.,1., 0.,1.,0.,1., 1.,0.,0.,1., 1.,1.,0.,1.)
        with Engine(backend=os.environ.get("BREPGPU_TEST_BACKEND","cpu"),
                    device=int(os.environ.get("BREPGPU_TEST_DEVICE","0"))) as engine:
            with engine.create_patch_set([Patch(1,1,controls)]) as patches:
                with engine.create_projection_query_set(patches,array("I",[0]),
                        array("d",[.25,.75,.5]),array("d",[.75,.25])) as queries:
                    with engine.project(queries,ProjectionOptions()) as result:
                        batch=result.download()
            batch.require_stationary()
            self.assertEqual(list(batch.statuses),[0])
            self.assertLessEqual(abs(batch.uv(0)[0]-.25),2.0**-20)
            self.assertLessEqual(abs(batch.uv(0)[1]-.75),2.0**-20)
            self.assertLessEqual(abs(batch.distance_squared(0)-.25),2.0**-40)
            expected=(.25,.75,.25,.75,0.,.25,0.)
            for field,name in enumerate(("s","t","x","y","z","distance_squared","residual")):
                self.assertEqual(batch.field(name,0),expected[field])
                self.assertEqual(batch.field(field,0),expected[field])
            self.assertEqual(batch.point(0),(.25,.75,0.))
            self.assertEqual(batch.residual(0),0.)
            self.assertEqual((batch.info.count,batch.info.patch_count,batch.info.field_count),(1,1,7))
            self.assertEqual(batch.info.backend,1 if os.environ.get("BREPGPU_TEST_BACKEND","cpu")=="cuda" else 0)
            self.assertEqual(batch.info.flags,0);self.assertTrue(math.isnan(batch.info.kernel_ms))
            for field,index in ((-1,0),(7,0),(0,-1),(0,1)):
                with self.assertRaises(IndexError):batch.field(field,index)

    def test_complete_independent_geometry_status_and_curvature_corpus(self):
        groups={}
        for case in projection_cases():
            groups.setdefault((case.max_iterations,case.max_backtracks,case.tolerance),[]).append(case)
        for key,cases in groups.items():
            cases=cases[::2]+list(reversed(cases[1::2]))
            with self.subTest(options=key):
                batch=self.evaluate(cases)
                self.assertEqual(batch.count,len(cases));self.assertEqual(len(batch.data),7*len(cases))
                for index,case in enumerate(cases):
                    with self.subTest(case=case.name):assert_row(self,case,batch,index)

    def test_stationarity_guard_accepts_saddle_and_rejects_unresolved_rows(self):
        from brepgpu import QueryError
        cases={c.name:c for c in projection_cases()}
        for name in ("parabolic_stationary_0.5","quartic_flat"):
            batch=self.evaluate([cases[name]])
            self.assertIs(batch.require_stationary(),batch)
            self.assertEqual(batch.flags[0]&112,32 if name.endswith("0.5") else 64)
        batch=self.evaluate([cases["zero_updates_unresolved"]])
        with self.assertRaises(QueryError):batch.require_stationary()

    def test_mixed_invalid_precedence_counters_and_nan_fields(self):
        from brepgpu import Patch, QueryError
        base=ProjectionCase("mixed_good",orthogonal_plane(),(.25,.75,.5),(.75,.25),answer=(.25,.75),distance_answer=.25)
        specs=[(0,base.target,base.seed,0),
               (0xFFFFFFFF,(math.nan,0.,0.),(math.nan,.5),1),
               (0,base.target,(math.nan,.5),2),
               (0,base.target,(math.nextafter(0.,-math.inf),.5),2),
               (0,base.target,(.5,math.nextafter(1.,math.inf)),2),
               (0,(math.inf,0.,0.),base.seed,2),
               (0,(math.nextafter(4.,math.inf),0.,0.),base.seed,2),
               (0,base.target,base.seed,0)]
        f=base.fixture
        with self.engine.create_patch_set([Patch(f.degree_u,f.degree_v,f.controls)]) as patches:
            with self.engine.create_projection_query_set(patches,array("I",(x[0] for x in specs)),
                    array("d",(v for x in specs for v in x[1])),array("d",(v for x in specs for v in x[2]))) as queries:
                with self.engine.project(queries) as result:batch=result.download()
        self.assertEqual(list(batch.statuses),[x[3] for x in specs])
        with self.assertRaises(QueryError):batch.require_stationary()
        for index,(_,target,seed,status) in enumerate(specs):
            case=replace(base,name="mixed_"+str(index),target=target,seed=seed,status=status)
            assert_row(self,case,batch,index)

    def test_empty_singleton_tail_and_full_stride_batches(self):
        cases=normal_parabolic_cases()
        for count in (0,1,31,33,257):
            with self.subTest(count=count):
                requests=[cases[(i*5)%len(cases)] for i in range(count)]
                batch=self.evaluate(requests)
                self.assertEqual(batch.count,count);self.assertEqual(len(batch.data),7*count)
                self.assertEqual(len(batch.statuses),count)
                for index,case in enumerate(requests):assert_row(self,case,batch,index)
        if os.environ.get("BREPGPU_TEST_BACKEND","cpu")=="cpu":
            from brepgpu import Engine
            requests=[cases[(i*5)%len(cases)] for i in range(33)]
            with Engine(backend="cpu",threads=4) as parallel:
                batch=self.evaluate(requests,parallel)
            for index,case in enumerate(requests):assert_row(self,case,batch,index)

    def test_input_copies_frozen_options_and_retained_provenance(self):
        from brepgpu import BgError, Lineage, Patch, ProjectionOptions
        base=normal_parabolic_cases()[0];f=base.fixture
        lineage=Lineage(101,5,303,7,11)
        controls=array("d",f.controls)
        patches=self.engine.create_patch_set([Patch(f.degree_u,f.degree_v,controls,lineage=lineage)])
        targets=array("d",base.target);seeds=array("d",base.seed);slots=array("I",[0])
        queries=self.engine.create_projection_query_set(patches,slots,targets,seeds)
        options=ProjectionOptions()
        with self.assertRaises((FrozenInstanceError,AttributeError)):options.max_iterations=0
        controls[:]=array("d",[math.nan])*len(controls)
        targets[:]=array("d",[math.nan])*3;seeds[:]=array("d",[math.nan])*2;slots[0]=0xFFFFFFFF
        patches.close()
        result=self.engine.project(queries,options)
        queries.close()
        try:
            batch=result.download();assert_row(self,base,batch,0)
            self.assertEqual(batch.lineage,(lineage,));self.assertEqual(batch.options,options)
            self.assertEqual(tuple(batch.targets),base.target);self.assertEqual(tuple(batch.seeds),base.seed)
            saved=tuple(batch.data)
            batch.data[0]=-999.;batch.targets[0]=-999.;batch.seeds[0]=-999.
            again=result.download()
            self.assertEqual(tuple(again.data),saved)
            self.assertEqual(tuple(again.targets),base.target);self.assertEqual(tuple(again.seeds),base.seed)
            with self.assertRaises(BgError):self.engine.project(queries)
        finally:result.close()
        self.assertEqual(batch.lineage,(lineage,))

    def test_query_binding_distinct_results_and_per_result_options(self):
        from brepgpu import Patch, ProjectionOptions
        cases=[c for c in projection_cases() if c.name in ("radial_cylinder_(0.125, 0.875)","plane_0.25_0.75")]
        resources=[]
        try:
            for case in cases:
                f=case.fixture
                patches=self.engine.create_patch_set([Patch(f.degree_u,f.degree_v,f.controls)]);resources.append(patches)
                query=self.engine.create_projection_query_set(patches,[0],case.target,case.seed);resources.append(query)
                result=self.engine.project(query);resources.append(result)
                assert_row(self,case,result.download(),0)
            first=resources[2];saved=tuple(first.download().data)
            query=resources[1]
            with self.engine.project(query,ProjectionOptions(max_iterations=0)) as limited:
                limited_batch=limited.download()
                self.assertEqual(limited_batch.statuses[0],5)
                self.assertEqual(limited_batch.options.max_iterations,0)
            self.assertEqual(tuple(first.download().data),saved)
            self.assertEqual(first.download().options.max_iterations,64)
        finally:
            for resource in reversed(resources):resource.close()

    def test_structural_rejection_foreign_kind_stale_and_context_close(self):
        from brepgpu import BgError, Engine, Patch, ProjectionOptions
        case=normal_parabolic_cases()[0];f=case.fixture
        with self.engine.create_patch_set([Patch(f.degree_u,f.degree_v,f.controls)]) as patches:
            with self.engine.create_projection_query_set(patches,[0],case.target,case.seed) as queries:
                for targets,seeds in ((case.target[:2],case.seed),(case.target,case.seed[:1])):
                    with self.assertRaises((BgError,ValueError)):
                        self.engine.create_projection_query_set(patches,[0],targets,seeds)
                for options in (ProjectionOptions(max_iterations=129),ProjectionOptions(max_backtracks=25),
                                ProjectionOptions(gradient_tolerance=math.nan),ProjectionOptions(gradient_tolerance=2.**-33)):
                    with self.assertRaises(BgError):self.engine.project(queries,options)
                with Engine(backend="cpu") as foreign:
                    with self.assertRaises(BgError):foreign.project(queries)
                with self.assertRaises(BgError):self.engine.evaluate(queries)
                with self.engine.create_query_set(patches,[0],case.seed) as ordinary:
                    with self.assertRaises(BgError):self.engine.project(ordinary)
                result=self.engine.project(queries)
                self.engine.close()
                with self.assertRaises(BgError):result.download()
                result.close()

    def test_readonly_strided_inputs_and_host_threads(self):
        from brepgpu import Patch
        case=normal_parabolic_cases()[0];f=case.fixture
        target_storage=array("d",(v for x in case.target for v in (x,-999.)))
        targets=memoryview(target_storage)[::2].toreadonly()
        seeds=memoryview(array("d",case.seed)).toreadonly()
        slots=memoryview(array("I",[0])).toreadonly()
        with self.engine.create_patch_set([Patch(f.degree_u,f.degree_v,memoryview(array("d",f.controls)).toreadonly())]) as patches:
            with self.engine.create_projection_query_set(patches,slots,targets,seeds) as queries:
                target_storage[0]=math.nan
                def run(_):
                    with self.engine.project(queries) as result:return result.download()
                with ThreadPoolExecutor(max_workers=3) as pool:batches=list(pool.map(run,range(6)))
                for batch in batches:assert_row(self,case,batch,0)

    def test_raw_options_wrong_kind_and_stale_handle_leave_no_publication(self):
        from brepgpu import Diagnostic, Patch, ProjectionOptionsDesc
        case=normal_parabolic_cases()[0];f=case.fixture
        with self.engine.create_patch_set([Patch(f.degree_u,f.degree_v,f.controls)]) as patches:
            query=self.engine.create_projection_query_set(patches,[0],case.target,case.seed)
            stale=query.handle
            for fault in ("version","size","bytes","flags","reserved"):
                options=ProjectionOptionsDesc();options.max_iterations=64;options.max_backtracks=24;options.gradient_tolerance=2.**-24
                size=ctypes.sizeof(options)
                if fault=="version":options.abi_version=1
                elif fault=="size":options.struct_size-=1
                elif fault=="bytes":size-=1
                elif fault=="flags":options.flags=1
                else:options.reserved=1
                diagnostic=Diagnostic();out=ctypes.c_uint64(12345)
                code=self.engine._lib.bg2_project(query.handle,ctypes.byref(options),size,ctypes.byref(out),
                                                 ctypes.byref(diagnostic),ctypes.sizeof(diagnostic))
                self.assertEqual(code,1);self.assertEqual(out.value,0)
            query.close()
            options=ProjectionOptionsDesc();options.max_iterations=64;options.max_backtracks=24;options.gradient_tolerance=2.**-24
            for handle in (patches.handle,stale,0):
                diagnostic=Diagnostic();out=ctypes.c_uint64(12345)
                code=self.engine._lib.bg2_project(handle,ctypes.byref(options),ctypes.sizeof(options),ctypes.byref(out),
                                                 ctypes.byref(diagnostic),ctypes.sizeof(diagnostic))
                self.assertEqual(code,3);self.assertEqual(out.value,0)

    def test_raw_download_all_overlap_pairs_and_extents_write_no_sentinels(self):
        from brepgpu import Diagnostic
        case=normal_parabolic_cases()[0]
        from brepgpu import Patch
        f=case.fixture
        with self.engine.create_patch_set([Patch(f.degree_u,f.degree_v,f.controls)]) as patches:
            with self.engine.create_projection_query_set(patches,[0],case.target,case.seed) as query:
                with self.engine.project(query) as result:
                    for pair in list(itertools.combinations(range(5),2))+[None]:
                        owners=[(ctypes.c_double*8)(*([123456.125]*8))]
                        owners += [(ctypes.c_uint32*1)(0xA5A5A5A5) for _ in range(4)]
                        pointers=[ctypes.cast(owner,ctypes.POINTER(ctypes.c_double if i==0 else ctypes.c_uint32)) for i,owner in enumerate(owners)]
                        if pair is not None:pointers[pair[1]]=ctypes.cast(owners[pair[0]],ctypes.POINTER(ctypes.c_uint32))
                        before=[bytes(owner) for owner in owners]
                        diagnostic=Diagnostic()
                        code=self.engine._lib.bg2_projection_download(result.handle,pointers[0],7 if pair else 8,
                            pointers[1],1,pointers[2],1,pointers[3],1,pointers[4],1,
                            ctypes.byref(diagnostic),ctypes.sizeof(diagnostic))
                        self.assertEqual(code,1);self.assertEqual([bytes(owner) for owner in owners],before)
                    assert_row(self,case,result.download(),0)

    def test_raw_diagnostic_alias_download_and_sources_leave_all_storage_unchanged(self):
        from brepgpu import Diagnostic,Patch,PatchDesc,ProjectionOptionsDesc
        case=normal_parabolic_cases()[0];f=case.fixture
        with self.engine.create_patch_set([Patch(f.degree_u,f.degree_v,f.controls)]) as patches:
            with self.engine.create_projection_query_set(patches,[0],case.target,case.seed) as query:
                with self.engine.project(query) as result:
                    diagnostic=Diagnostic()
                    fields=ctypes.cast(ctypes.pointer(diagnostic),ctypes.POINTER(ctypes.c_double))
                    integers=[(ctypes.c_uint32*1)(0xA5A5A5A5) for _ in range(4)]
                    before=(bytes(diagnostic),[bytes(x) for x in integers])
                    code=self.engine._lib.bg2_projection_download(result.handle,fields,7,
                        integers[0],1,integers[1],1,integers[2],1,integers[3],1,
                        ctypes.byref(diagnostic),ctypes.sizeof(diagnostic))
                    self.assertEqual(code,1);self.assertEqual(diagnostic.code,0)
                    self.assertEqual((bytes(diagnostic),[bytes(x) for x in integers]),before)
                    descriptors=(PatchDesc*1)();slots=(ctypes.c_uint32*1)(0xA5A5A5A5)
                    seeds=(ctypes.c_double*2)(-99.,-99.);options=ProjectionOptionsDesc()
                    targets=ctypes.cast(ctypes.pointer(diagnostic),ctypes.POINTER(ctypes.c_double))
                    before=tuple(bytes(x) for x in (diagnostic,descriptors,slots,seeds,options))
                    code=self.engine._lib.bg2_projection_sources(result.handle,descriptors,1,ctypes.sizeof(descriptors),
                        slots,1,targets,3,seeds,2,ctypes.byref(options),ctypes.sizeof(options),
                        ctypes.byref(diagnostic),ctypes.sizeof(diagnostic))
                    self.assertEqual(code,1);self.assertEqual(diagnostic.code,0)
                    self.assertEqual(tuple(bytes(x) for x in (diagnostic,descriptors,slots,seeds,options)),before)
                    assert_row(self,case,result.download(),0)

    def test_projection_constructor_failure_closes_native_token_and_preserves_exception(self):
        import brepgpu
        case=normal_parabolic_cases()[0];f=case.fixture
        with self.engine.create_patch_set([brepgpu.Patch(f.degree_u,f.degree_v,f.controls)]) as patches:
            captured=[];chosen=MemoryError("projection query wrapper allocation witness")
            def reject_query(engine,handle):
                captured.append(handle);raise chosen
            with replace_attribute.object(brepgpu,"ProjectionQueries",reject_query):
                with self.assertRaises(MemoryError) as caught:
                    self.engine.create_projection_query_set(patches,[0],case.target,case.seed)
            self.assertIs(caught.exception,chosen);self.assertEqual(len(captured),1)
            diagnostic=brepgpu.Diagnostic()
            code=self.engine._lib.bg1_close(captured[0],ctypes.byref(diagnostic),ctypes.sizeof(diagnostic))
            with self.subTest(constructor="ProjectionQueries"):
                self.assertEqual(code,3,"native query survived failed public wrapper construction")
            with self.engine.create_projection_query_set(patches,[0],case.target,case.seed) as query:
                captured=[];chosen=MemoryError("projection result wrapper allocation witness")
                def reject_result(engine,handle):
                    captured.append(handle);raise chosen
                with replace_attribute.object(brepgpu,"ProjectionResult",reject_result):
                    with self.assertRaises(MemoryError) as caught:self.engine.project(query)
                self.assertIs(caught.exception,chosen);self.assertEqual(len(captured),1)
                diagnostic=brepgpu.Diagnostic()
                code=self.engine._lib.bg1_close(captured[0],ctypes.byref(diagnostic),ctypes.sizeof(diagnostic))
                with self.subTest(constructor="ProjectionResult"):
                    self.assertEqual(code,3,"native result survived failed public wrapper construction")
                with self.engine.project(query) as result:assert_row(self,case,result.download(),0)

    def test_engine_constructor_failure_closes_native_context_and_preserves_exception(self):
        import brepgpu
        captured=[];chosen=MemoryError("engine base-wrapper allocation witness")
        def reject_resource(instance,engine,handle):
            captured.append((instance,instance._lib,handle));raise chosen
        with replace_attribute.object(brepgpu._Resource,"__init__",reject_resource):
            with self.assertRaises(MemoryError) as caught:
                brepgpu.Engine(backend=os.environ.get("BREPGPU_TEST_BACKEND","cpu"),
                               device=int(os.environ.get("BREPGPU_TEST_DEVICE","0")))
        self.assertIs(caught.exception,chosen);self.assertEqual(len(captured),1)
        instance,lib,handle=captured[0]
        diagnostic=brepgpu.Diagnostic()
        code=lib.bg1_close(handle,ctypes.byref(diagnostic),ctypes.sizeof(diagnostic))
        self.assertEqual(code,3,"native context survived partial engine initialization")

    def test_exception_after_native_query_return_closes_token_and_preserves_interrupt(self):
        import brepgpu
        case=normal_parabolic_cases()[0];f=case.fixture
        captured=[];chosen=KeyboardInterrupt("after native projection query publication")
        original=brepgpu._invoke
        def after_success(lib,name,*arguments):
            original(lib,name,*arguments)
            if name=="bg2_create_projection_query_set":
                captured.append(ctypes.cast(arguments[-1],ctypes.POINTER(ctypes.c_uint64)).contents.value)
                raise chosen
        with self.engine.create_patch_set([brepgpu.Patch(f.degree_u,f.degree_v,f.controls)]) as patches:
            with replace_attribute.object(brepgpu,"_invoke",after_success):
                with self.assertRaises(KeyboardInterrupt) as caught:
                    self.engine.create_projection_query_set(patches,[0],case.target,case.seed)
            self.assertIs(caught.exception,chosen);self.assertEqual(len(captured),1)
            diagnostic=brepgpu.Diagnostic()
            code=self.engine._lib.bg1_close(captured[0],ctypes.byref(diagnostic),ctypes.sizeof(diagnostic))
            self.assertEqual(code,3,"native query survived exception immediately after native return")
            with self.engine.create_projection_query_set(patches,[0],case.target,case.seed) as query:
                with self.engine.project(query) as result:assert_row(self,case,result.download(),0)

    def test_cuda_projection_callbacks_restore_each_calling_threads_prior_device(self):
        if os.environ.get("BREPGPU_TEST_BACKEND","cpu")!="cuda":
            self.skipTest("CUDA device selection is outside the explicitly selected CPU backend")
        from brepgpu import Engine,Patch
        runtime=ctypes.CDLL(os.environ.get("BREPGPU_CUDART","libcudart.so.12"))
        runtime.cudaGetDevice.argtypes=[ctypes.POINTER(ctypes.c_int)]
        runtime.cudaGetDevice.restype=ctypes.c_int
        runtime.cudaSetDevice.argtypes=[ctypes.c_int];runtime.cudaSetDevice.restype=ctypes.c_int
        runtime.cudaGetDeviceCount.argtypes=[ctypes.POINTER(ctypes.c_int)]
        runtime.cudaGetDeviceCount.restype=ctypes.c_int
        def current():
            value=ctypes.c_int(-1)
            self.assertEqual(runtime.cudaGetDevice(ctypes.byref(value)),0)
            return value.value
        count=ctypes.c_int(0)
        self.assertEqual(runtime.cudaGetDeviceCount(ctypes.byref(count)),0)
        self.assertGreaterEqual(count.value,2,"both-device qualification requires two CUDA devices")
        selected=int(os.environ.get("BREPGPU_TEST_DEVICE","0"));other=(selected+1)%count.value
        case=normal_parabolic_cases()[0];f=case.fixture;saved=current()
        self.assertEqual(runtime.cudaSetDevice(other),0)
        resources=[]
        try:
            engine=Engine(backend="cuda",device=selected);resources.append(engine)
            self.assertEqual(current(),other,"context creation changed caller device")
            patches=engine.create_patch_set([Patch(f.degree_u,f.degree_v,f.controls)]);resources.append(patches)
            self.assertEqual(current(),other,"patch upload changed caller device")
            query=engine.create_projection_query_set(patches,[0],case.target,case.seed);resources.append(query)
            self.assertEqual(current(),other,"projection query upload changed caller device")
            result=engine.project(query);resources.append(result)
            self.assertEqual(current(),other,"resident projection changed caller device")
            batch=result.download()
            self.assertEqual(current(),other,"projection download/provenance changed caller device")
            assert_row(self,case,batch,0)
            self.assertEqual(tuple(batch.targets),case.target);self.assertEqual(tuple(batch.seeds),case.seed)
            def worker(_):
                prior=current();self.assertEqual(runtime.cudaSetDevice(other),0)
                try:
                    with engine.project(query) as threaded:
                        self.assertEqual(current(),other,"worker project changed caller device")
                        observed=threaded.download()
                        self.assertEqual(current(),other,"worker download/provenance changed caller device")
                        assert_row(self,case,observed,0)
                    self.assertEqual(current(),other,"worker result cleanup changed caller device")
                finally:self.assertEqual(runtime.cudaSetDevice(prior),0)
            with ThreadPoolExecutor(max_workers=2) as pool:list(pool.map(worker,range(3)))
            for resource in reversed(resources):
                resource.close()
                self.assertEqual(current(),other,"projection/context cleanup changed caller device")
            resources.clear()
        finally:
            try:
                for resource in reversed(resources):resource.close()
            finally:self.assertEqual(runtime.cudaSetDevice(saved),0)


if __name__=="__main__":unittest.main()
