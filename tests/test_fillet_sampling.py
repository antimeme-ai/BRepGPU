"""Actual trimmed box fillet jets, contact tangency and child lifetime."""
from dataclasses import FrozenInstanceError
from fractions import Fraction as F
import math
import os
import unittest
from unittest.mock import patch


class FilletSamplingTests(unittest.TestCase):
    def api(self):
        from brepgpu import fillet_sampling
        return fillet_sampling

    def result(self,edge=11,radius=F(3,8),origin=(0.,0.,0.),scale=1.):
        from brepgpu.body import Frame,make_body
        from brepgpu.boolean import EntityKey
        from brepgpu.fillet import fillet_edge
        source=make_body('box',Frame(origin,scale))
        return fillet_edge(source,EntityKey(source.body_id,source.revision,'edge',edge),radius)

    def engine(self):
        from brepgpu import Engine
        return Engine(backend=os.environ.get('BREPGPU_TEST_BACKEND','cpu'),
            device=int(os.environ.get('BREPGPU_TEST_DEVICE','0')),
            threads=int(os.environ.get('BREPGPU_TEST_THREADS','1')))

    def test_entry(self):
        from brepgpu import Patch
        self.assertEqual(Patch(0,0,[(0,0,0,1)]).controls,(0.,0.,0.,1.))
        self.assertTrue(callable(self.api().sample))

    def test_request_copies_labels_and_nonbinary_join_bits(self):
        from brepgpu.body_sampling import Candidate
        from brepgpu.body import INTERIOR,BOUNDARY,REJECTED,INVALID
        p=self.api();result=self.result()
        submitted=[Candidate(6,.5,.5),Candidate(6,0,.5),Candidate(0,1,1),
                   Candidate(4,.75,.5),Candidate(4,.9,.5),Candidate(2,.5,.5),
                   Candidate(6,.5,float('nan')),Candidate(0,-.25,.5)]
        request=p.FilletRequest(result,submitted);submitted.clear()
        self.assertIs(request.result,result)
        self.assertEqual(request.labels,(INTERIOR,BOUNDARY,REJECTED,BOUNDARY,
                                         REJECTED,INTERIOR,INVALID,INVALID))
        self.assertEqual(request.admitted_indices,(0,1,3,5))
        self.assertEqual(len(request.candidates),8)
        with self.assertRaises(FrozenInstanceError):request.labels=()
        with self.assertRaises(FrozenInstanceError):request.candidates[0].u=0.
        odd=self.result(radius=F(65,256));join=F(319,384)
        self.assertEqual(odd.solid.classify(4,join,F(1,2)),BOUNDARY)
        rounded=p.FilletRequest(odd,[Candidate(4,join,F(1,2))])
        actual=F(float(join))
        expected=BOUNDARY if actual==join else INTERIOR if actual<join else REJECTED
        self.assertIs(type(rounded.candidates[0].u),float)
        self.assertEqual(rounded.candidates[0].u,float(join))
        self.assertEqual(rounded.labels,(expected,))
        self.assertNotEqual(actual,join)

    def test_seven_chart_grid_budget_and_operation_guards(self):
        from brepgpu.body_sampling import Candidate
        p=self.api();result=self.result()
        for value in (result.source,result.solid,object()):
            with self.assertRaises(TypeError):p.FilletRequest(value,[])
        with self.assertRaises(TypeError):p.FilletRequest(result,[object()])
        with self.assertRaises(ValueError):p.FilletRequest(result,[Candidate(7,0,0)])
        for value in (True,1,258,2.5):
            with self.assertRaises((TypeError,ValueError)):p.prepare_grid(result,value)
        request=p.prepare_grid(result)
        self.assertEqual(len(request.candidates),2023)
        self.assertEqual({c.patch_slot for c in request.candidates},set(range(7)))
        self.assertGreater(len(request.admitted_indices),0)
        self.assertLess(len(request.admitted_indices),len(request.candidates))
        seen=[]
        def infinite():
            while True:
                seen.append(1);yield Candidate(6,0,0)
        with patch.object(p,'MAX_CANDIDATES',2):
            with self.assertRaises(ValueError):p.FilletRequest(result,infinite())
            with self.assertRaises(ValueError):p.prepare_grid(result,2)
        self.assertEqual(len(seen),3)

    def test_actual_nonaxis_native_rows_source_frame_and_rejections(self):
        from brepgpu.body_sampling import Candidate
        from brepgpu.body import INTERIOR,REJECTED,INVALID
        p=self.api();result=self.result(origin=(3.,-2.,1.),scale=2.)
        request=p.FilletRequest(result,[Candidate(6,.5,.5),Candidate(0,.9,.9),
            Candidate(2,.5,.5),Candidate(0,1,1),Candidate(4,.9,.5),
            Candidate(6,.5,float('inf'))])
        with self.engine() as engine:
            batch=p.sample(engine,request)
            self.assertEqual((batch.backend,batch.device),(engine.backend,engine.device))
        self.assertIs(batch.result,result);self.assertIs(batch.request,request)
        self.assertEqual((batch.count,batch.evaluated_count),(6,3))
        expected={0:((4.2,-.65,1.),(.6,.8,0.)),
                  1:((4.2,-.8,2.),(0.,0.,1.)),
                  2:((3.,-3.5,1.),(0.,-1.,0.))}
        for index,row in enumerate(batch.rows):
            slot=request.candidates[index].patch_slot
            self.assertEqual((row.index,row.patch_slot,row.face_id,row.chart_id),
                             (index,slot,slot+1,slot+1))
            if index in expected:
                point,normal=expected[index]
                self.assertEqual((row.trim,row.native_status),(INTERIOR,0))
                for a,b in zip(row.position,point):self.assertLessEqual(abs(a-b),2**-36)
                for a,b in zip(row.normal,normal):self.assertLessEqual(abs(a-b),2**-30)
            else:
                self.assertEqual((row.trim,row.native_status),(INVALID if index==5 else REJECTED,-1))
                self.assertTrue(all(math.isnan(x) for x in (*row.position,*row.normal)))
        self.assertTrue(all(math.isfinite(x) and x>=0 for _,x in batch.timings))
        with self.assertRaises((TypeError,FrozenInstanceError)):batch.rows=()

    def test_actual_dual_chart_contacts_and_all_quarter_orientations(self):
        from brepgpu.body_sampling import Candidate
        from brepgpu.body import BOUNDARY
        p=self.api();result=self.result()
        request=p.FilletRequest(result,[Candidate(6,0,.5),Candidate(4,.75,.5),
            Candidate(6,1,.5),Candidate(3,.75,.5)])
        with self.engine() as engine:
            batch=p.sample(engine,request)
            for offset,point,normal in ((0,(.75,.375,0.),(1.,0.,0.)),
                                        (2,(.375,.75,0.),(0.,1.,0.))):
                for row in batch.rows[offset:offset+2]:
                    self.assertEqual((row.trim,row.native_status),(BOUNDARY,0))
                    for a,b in zip(row.position,point):self.assertLessEqual(abs(a-b),2**-36)
                    for a,b in zip(row.normal,normal):self.assertLessEqual(abs(a-b),2**-30)
            for edge,point,normal in ((9,(-.6,-.675,0.),(-.6,-.8,0.)),
                (10,(.675,-.6,0.),(.8,-.6,0.)),(11,(.6,.675,0.),(.6,.8,0.)),
                (12,(-.675,.6,0.),(-.8,.6,0.))):
                single=p.sample(engine,p.FilletRequest(self.result(edge=edge),[Candidate(6,.5,.5)])).rows[0]
                for a,b in zip(single.position,point):self.assertLessEqual(abs(a-b),2**-36)
                for a,b in zip(single.normal,normal):self.assertLessEqual(abs(a-b),2**-30)

    def test_empty_all_nonadmitted_and_engine_guards(self):
        from brepgpu.body_sampling import Candidate
        from brepgpu import BgError
        p=self.api();result=self.result()
        with self.engine() as engine:
            for candidates in ([],[Candidate(0,1,1),Candidate(6,.5,float('nan'))]):
                batch=p.sample(engine,p.FilletRequest(result,candidates))
                self.assertEqual((batch.count,batch.evaluated_count),(len(candidates),0))
                self.assertTrue(all(row.native_status==-1 and all(math.isnan(x) for x in
                    (*row.position,*row.normal)) for row in batch.rows))
            with self.assertRaises(TypeError):p.sample(engine,object())
            with self.assertRaises(TypeError):p.sample(object(),p.prepare_grid(result,2))
        with self.assertRaises(BgError):p.sample(engine,p.prepare_grid(result,2))

    def test_late_host_failure_closes_three_children_and_same_engine_retry(self):
        p=self.api();result=self.result();request=p.prepare_grid(result,3)
        held=[];failure=RuntimeError('actual late fillet host failure')
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
                if kind=='descriptor':jets._descriptors[6].origin[0]+=.25
                else:jets.statuses[0]=1
                return jets
            with self.engine() as engine:
                with patch.object(Result,'download',corrupt):
                    with self.assertRaises(RuntimeError):p.sample(engine,request)
                self.assertTrue(held and all(not child.handle for child in held))
                self.assertEqual(p.sample(engine,request).evaluated_count,len(request.admitted_indices))


if __name__=='__main__':unittest.main()
