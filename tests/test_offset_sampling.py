"""Actual signed-offset carriers, native provenance and child lifetime."""
from dataclasses import FrozenInstanceError
from fractions import Fraction as F
import math
import os
import unittest
from unittest.mock import patch


class OffsetSamplingTests(unittest.TestCase):
    def api(self):
        from brepgpu import offset_sampling
        return offset_sampling

    def result(self,distance=F(1,8),origin=(0.,0.,0.),scale=1.):
        from brepgpu.body import Frame,make_body
        from brepgpu.offset import offset_solid
        source=make_body('cylinder',Frame(origin,scale))
        return offset_solid(source,distance)

    def engine(self):
        from brepgpu import Engine
        return Engine(backend=os.environ.get('BREPGPU_TEST_BACKEND','cpu'),
            device=int(os.environ.get('BREPGPU_TEST_DEVICE','0')),
            threads=int(os.environ.get('BREPGPU_TEST_THREADS','1')))

    def test_entry(self):
        from brepgpu import Patch
        self.assertEqual(Patch(0,0,[(0,0,0,1)]).controls,(0.,0.,0.,1.))
        self.assertTrue(callable(self.api().sample))

    def test_request_copy_labels_and_rounded_exact_circle_boundary(self):
        from brepgpu.body_sampling import Candidate
        from brepgpu.body import INTERIOR,BOUNDARY,REJECTED,INVALID
        p=self.api();result=self.result()
        submitted=[Candidate(0,.5,.5),Candidate(0,1,.5),Candidate(0,0,0),
            Candidate(2,0,.5),Candidate(3,0,.5),Candidate(5,1,.5),
            Candidate(2,.5,0),Candidate(3,.5,float('nan')),Candidate(0,-.25,.5)]
        request=p.OffsetRequest(result,submitted);submitted.clear()
        self.assertIs(request.result,result)
        self.assertEqual(request.labels,(INTERIOR,BOUNDARY,REJECTED,BOUNDARY,
                                         INTERIOR,BOUNDARY,BOUNDARY,INVALID,INVALID))
        self.assertEqual(request.admitted_indices,(0,1,3,4,5,6))
        self.assertEqual(len(request.candidates),9)
        with self.assertRaises(FrozenInstanceError):request.labels=()
        with self.assertRaises(FrozenInstanceError):request.candidates[0].u=0.
        u,v=F(4,5),F(9,10)
        self.assertEqual(result.solid.classify(0,u,v),BOUNDARY)
        copied=p.OffsetRequest(result,[Candidate(0,u,v)])
        self.assertIs(type(copied.candidates[0].u),float)
        actual=(2*F(float(u))-1)**2+(2*F(float(v))-1)**2
        expected=REJECTED if actual>1 else BOUNDARY if actual==1 else INTERIOR
        self.assertNotEqual(actual,1)
        self.assertEqual((copied.candidates[0].u,copied.candidates[0].v),(float(u),float(v)))
        self.assertEqual(copied.labels,(expected,))

    def test_complete_six_chart_grid_budget_and_operation_guards(self):
        from brepgpu.body_sampling import Candidate
        p=self.api();result=self.result()
        for value in (result.source,result.solid,object()):
            with self.assertRaises(TypeError):p.OffsetRequest(value,[])
        with self.assertRaises(TypeError):p.OffsetRequest(result,[object()])
        with self.assertRaises(ValueError):p.OffsetRequest(result,[Candidate(6,0,0)])
        for value in (True,1,258,2.5):
            with self.assertRaises((TypeError,ValueError)):p.prepare_grid(result,value)
        request=p.prepare_grid(result)
        self.assertEqual(len(request.candidates),1734)
        self.assertEqual({c.patch_slot for c in request.candidates},set(range(6)))
        self.assertGreater(len(request.admitted_indices),0)
        self.assertLess(len(request.admitted_indices),len(request.candidates))
        seen=[]
        def infinite():
            while True:
                seen.append(1);yield Candidate(2,0,0)
        with patch.object(p,'MAX_CANDIDATES',2):
            with self.assertRaises(ValueError):p.OffsetRequest(result,infinite())
            with self.assertRaises(ValueError):p.prepare_grid(result,2)
        self.assertEqual(len(seen),3)

    def test_actual_nonaxis_native_rows_signed_sizes_source_frame(self):
        from brepgpu.body_sampling import Candidate
        from brepgpu.body import INTERIOR,REJECTED,INVALID
        p=self.api()
        with self.engine() as engine:
            for distance,radius,halfheight in ((F(1,8),F(3,8),F(5,8)),
                                              (-F(1,8),F(1,8),F(3,8))):
                result=self.result(distance,origin=(3.,-2.,1.),scale=2.)
                request=p.OffsetRequest(result,[Candidate(0,.5,.5),Candidate(1,.5,.5),
                    Candidate(2,.5,.5),Candidate(0,0,0),Candidate(2,.5,float('inf'))])
                batch=p.sample(engine,request)
                self.assertEqual((batch.backend,batch.device),(engine.backend,engine.device))
                self.assertIs(batch.result,result);self.assertIs(batch.request,request)
                self.assertEqual((batch.count,batch.evaluated_count),(5,3))
                expected={0:((3.,-2.,float(1+2*halfheight)),(0.,0.,1.)),
                          1:((3.,-2.,float(1-2*halfheight)),(0.,0.,-1.)),
                          2:((float(3+2*radius*F(3,5)),float(-2+2*radius*F(4,5)),1.),(.6,.8,0.))}
                for index,row in enumerate(batch.rows):
                    slot=request.candidates[index].patch_slot
                    self.assertEqual((row.index,row.patch_slot,row.face_id,row.chart_id),
                                     (index,slot,slot+1 if slot<2 else 3,slot+1))
                    if index in expected:
                        point,normal=expected[index]
                        self.assertEqual((row.trim,row.native_status),(INTERIOR,0))
                        for a,b in zip(row.position,point):self.assertLessEqual(abs(a-b),2**-36)
                        for a,b in zip(row.normal,normal):self.assertLessEqual(abs(a-b),2**-30)
                    else:
                        self.assertEqual((row.trim,row.native_status),(INVALID if index==4 else REJECTED,-1))
                        self.assertTrue(all(math.isnan(x) for x in (*row.position,*row.normal)))
                self.assertTrue(all(math.isfinite(x) and x>=0 for _,x in batch.timings))
                with self.assertRaises((TypeError,FrozenInstanceError)):batch.rows=()

    def test_actual_seam_vs_internal_quarter_join_geometry(self):
        from brepgpu.body_sampling import Candidate
        from brepgpu.body import INTERIOR,BOUNDARY
        p=self.api();result=self.result()
        candidates=[Candidate(2,0,F(1,3)),Candidate(5,1,F(1,3))]
        for q in range(3):candidates.extend((Candidate(2+q,1,F(1,3)),Candidate(3+q,0,F(1,3))))
        with self.engine() as engine:batch=p.sample(engine,p.OffsetRequest(result,candidates))
        for index in range(0,len(batch.rows),2):
            a,b=batch.rows[index:index+2]
            self.assertEqual((a.trim,b.trim),(BOUNDARY,BOUNDARY) if index==0 else (INTERIOR,INTERIOR))
            for x,y in zip(a.position,b.position):self.assertLessEqual(abs(x-y),2**-36)
            for x,y in zip(a.normal,b.normal):self.assertLessEqual(abs(x-y),2**-30)
        # Literal cardinal unit normals in every wall quarter, not parity alone.
        with self.engine() as engine:
            cardinal=p.sample(engine,p.OffsetRequest(result,[Candidate(slot,0,.5) for slot in range(2,6)]))
        for row,point,normal in zip(cardinal.rows,((.375,0.,0.),(0.,.375,0.),(-.375,0.,0.),(0.,-.375,0.)),
                                    ((1.,0.,0.),(0.,1.,0.),(-1.,0.,0.),(0.,-1.,0.))):
            for a,b in zip(row.position,point):self.assertLessEqual(abs(a-b),2**-36)
            for a,b in zip(row.normal,normal):self.assertLessEqual(abs(a-b),2**-30)

    def test_empty_all_nonadmitted_and_engine_guards(self):
        from brepgpu.body_sampling import Candidate
        from brepgpu import BgError
        p=self.api();result=self.result()
        with self.engine() as engine:
            for candidates in ([],[Candidate(0,0,0),Candidate(2,.5,float('nan'))]):
                batch=p.sample(engine,p.OffsetRequest(result,candidates))
                self.assertEqual((batch.count,batch.evaluated_count),(len(candidates),0))
                self.assertTrue(all(row.native_status==-1 and all(math.isnan(x) for x in
                    (*row.position,*row.normal)) for row in batch.rows))
            with self.assertRaises(TypeError):p.sample(engine,object())
            with self.assertRaises(TypeError):p.sample(object(),p.prepare_grid(result,2))
        with self.assertRaises(BgError):p.sample(engine,p.prepare_grid(result,2))

    def test_late_host_failure_closes_three_children_and_same_engine_retry(self):
        p=self.api();result=self.result();request=p.prepare_grid(result,3)
        held=[];failure=RuntimeError('actual late offset host failure')
        with self.engine() as engine:
            original={name:getattr(engine,name) for name in ('create_patch_set','create_query_set','evaluate')}
            def capture(name):
                def call(*args,**kwargs):
                    child=original[name](*args,**kwargs);held.append(child);return child
                return call
            with patch.object(engine,'create_patch_set',capture('create_patch_set')), \
                 patch.object(engine,'create_query_set',capture('create_query_set')), \
                 patch.object(engine,'evaluate',capture('evaluate')), \
                 patch.object(p,'_rows',side_effect=failure):
                with self.assertRaises(RuntimeError) as caught:p.sample(engine,request)
            self.assertIs(caught.exception,failure)
            self.assertEqual(len(held),3);self.assertTrue(all(not child.handle for child in held))
            healthy=p.sample(engine,request)
            self.assertIs(healthy.result,result)
            self.assertEqual(healthy.evaluated_count,len(request.admitted_indices))

    def test_actual_download_descriptor_status_rejection_then_retry(self):
        from brepgpu import Result
        p=self.api();result=self.result();request=p.prepare_grid(result,3)
        original=Result.download
        for kind in ('descriptor','status'):
            held=[]
            def corrupt(resource):
                held.append(resource);jets=original(resource)
                if kind=='descriptor':jets._descriptors[2].origin[0]+=.25
                else:jets.statuses[0]=1
                return jets
            with self.engine() as engine:
                with patch.object(Result,'download',corrupt):
                    with self.assertRaises(RuntimeError):p.sample(engine,request)
                self.assertTrue(held and all(not child.handle for child in held))
                self.assertEqual(p.sample(engine,request).evaluated_count,len(request.admitted_indices))


if __name__=='__main__':unittest.main()
