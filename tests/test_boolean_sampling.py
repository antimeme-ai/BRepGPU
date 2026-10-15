"""Direct Boolean sample request, mixed-frame and native lifetime contracts."""
import math
import os
from fractions import Fraction as F
from dataclasses import FrozenInstanceError
import unittest
from unittest.mock import patch


class CutSamplingTests(unittest.TestCase):
    def api(self):
        from brepgpu import boolean_sampling
        return boolean_sampling

    def result(self, scale=1., origin=(0.,0.,0.)):
        from brepgpu.body import make_body, Frame
        from brepgpu.boolean import difference
        return difference(make_body('box',Frame(origin,scale)),
                          make_body('cylinder',Frame(origin,2*scale)))

    def engine(self):
        from brepgpu import Engine
        return Engine(backend=os.environ.get('BREPGPU_TEST_BACKEND','cpu'),
            device=int(os.environ.get('BREPGPU_TEST_DEVICE','0')),
            threads=int(os.environ.get('BREPGPU_TEST_THREADS','1')))

    def test_entry(self):
        from brepgpu import Patch
        self.assertEqual(Patch(0,0,[(0,0,0,1)]).controls,(0.,0.,0.,1.))
        self.assertTrue(callable(self.api().sample))

    def test_request_actual_bits_and_immutability(self):
        from brepgpu.body_sampling import Candidate
        from brepgpu.body import INTERIOR, BOUNDARY, REJECTED, INVALID
        p=self.api();result=self.result()
        submitted=[Candidate(0,.5,.5),Candidate(0,0.,0.),Candidate(6,0.,.5),
            Candidate(7,0.,.5),Candidate(6,.5,.125),Candidate(6,.5,float('nan'))]
        request=p.CutRequest(result,submitted)
        submitted.clear()
        self.assertIs(request.result,result)
        self.assertEqual(request.labels,(REJECTED,BOUNDARY,BOUNDARY,INTERIOR,REJECTED,INVALID))
        self.assertEqual(request.admitted_indices,(1,2,3))
        self.assertEqual(len(request.candidates),6)
        with self.assertRaises(FrozenInstanceError):request.result=None
        with self.assertRaises(FrozenInstanceError):request.candidates[0].u=0.
        # Exact classify can name a rim; native requests label their rounded bits.
        exact=F(5,6)
        candidate=Candidate(0,exact,F(1,2))
        rounded=p.CutRequest(result,[candidate])
        self.assertEqual(rounded.labels,(result.solid.classify(0,float(exact),.5),))

    def test_request_and_grid_admission(self):
        from brepgpu.body_sampling import Candidate
        p=self.api();result=self.result()
        for value in (object(),result.solid):
            with self.assertRaises(TypeError):p.CutRequest(value,[])
        with self.assertRaises(TypeError):p.CutRequest(result,[object()])
        with self.assertRaises(ValueError):p.CutRequest(result,[Candidate(10,0,0)])
        for n in (True,1,258,2.5):
            with self.subTest(n=n):
                with self.assertRaises((TypeError,ValueError)):p.prepare_grid(result,n)
        request=p.prepare_grid(result,3)
        self.assertEqual(len(request.candidates),90)
        for c in request.candidates:
            if c.patch_slot>=6:self.assertIn(c.v,(.25,.5,.75))
        seen=[]
        def infinite():
            while True:
                seen.append(1);yield Candidate(0,0,0)
        with patch.object(p,'MAX_CANDIDATES',2):
            with self.assertRaises(ValueError):p.CutRequest(result,infinite())
        self.assertEqual(len(seen),3)

    def test_mixed_frames_and_literal_native_rows(self):
        from brepgpu.body_sampling import Candidate
        from brepgpu.body import BOUNDARY, INTERIOR, REJECTED, INVALID
        p=self.api();result=self.result(origin=(3.,-2.,1.))
        request=p.CutRequest(result,[Candidate(0,0.,0.),Candidate(0,.5,.5),
            Candidate(6,0.,.5),Candidate(6,0.,.75),Candidate(2,.5,.5),
            Candidate(6,.5,.125),Candidate(6,.5,float('inf'))])
        with self.engine() as engine:
            batch=p.sample(engine,request)
            self.assertEqual(batch.backend,engine.backend);self.assertEqual(batch.device,engine.device)
        self.assertIs(batch.result,result);self.assertIs(batch.request,request)
        self.assertEqual((batch.count,batch.evaluated_count),(7,4))
        expected={0:((2.25,-2.75,1.5),(0.,0.,1.),BOUNDARY),
            2:((3.5,-2.,1.),(-1.,0.,0.),BOUNDARY),
            3:((3.5,-2.,1.5),(-1.,0.,0.),BOUNDARY),
            4:((3.,-2.75,1.),(0.,-1.,0.),INTERIOR)}
        for index,row in enumerate(batch.rows):
            self.assertEqual(row.index,index)
            self.assertEqual(row.patch_slot,request.candidates[index].patch_slot)
            self.assertEqual((row.face_id,row.chart_id),
                ((1,1) if index<2 else (3,3) if index==4 else (7,7)))
            if index in expected:
                point,normal,label=expected[index];self.assertEqual(row.trim,label)
                self.assertEqual(row.native_status,0)
                for a,b in zip(row.position,point):self.assertLessEqual(abs(a-b),2**-36)
                for a,b in zip(row.normal,normal):self.assertLessEqual(abs(a-b),2**-30)
            else:
                self.assertEqual(row.trim,INVALID if index==6 else REJECTED)
                self.assertEqual(row.native_status,-1)
                self.assertTrue(all(math.isnan(x) for x in (*row.position,*row.normal)))
        self.assertTrue(all(math.isfinite(t) and t>=0 for _,t in batch.timings))
        with self.assertRaises((TypeError,FrozenInstanceError)):batch.rows=()

    def test_empty_and_all_rejected_requests(self):
        from brepgpu.body_sampling import Candidate
        p=self.api();result=self.result()
        with self.engine() as engine:
            for candidates in ([],[Candidate(0,.5,.5)]):
                batch=p.sample(engine,p.CutRequest(result,candidates))
                self.assertEqual(batch.count,len(candidates));self.assertEqual(batch.evaluated_count,0)

    def test_late_host_failure_closes_children_and_healthy_retry(self):
        p=self.api();result=self.result();request=p.prepare_grid(result,3)
        held=[];failure=RuntimeError('actual late host consumer failure')
        with self.engine() as engine:
            original={name:getattr(engine,name) for name in ('create_patch_set','create_query_set','evaluate')}
            def capture(name):
                def call(*a,**kw):
                    child=original[name](*a,**kw);held.append(child);return child
                return call
            with patch.object(engine,'create_patch_set',capture('create_patch_set')), \
                 patch.object(engine,'create_query_set',capture('create_query_set')), \
                 patch.object(engine,'evaluate',capture('evaluate')), \
                 patch.object(p,'_rows',side_effect=failure):
                with self.assertRaises(RuntimeError) as caught:p.sample(engine,request)
            self.assertIs(caught.exception,failure)
            self.assertEqual(len(held),3);self.assertTrue(all(c.handle==0 for c in held))
            healthy=p.sample(engine,request)
            self.assertIs(healthy.result,result);self.assertGreater(healthy.evaluated_count,0)

    def test_actual_downloaded_descriptor_failure_closes_children(self):
        from brepgpu import Result
        p=self.api();request=p.prepare_grid(self.result(),3)
        original=Result.download;held=[]
        def corrupt(resource):
            held.append(resource);batch=original(resource)
            batch._descriptors[6].scale*=.5
            return batch
        with self.engine() as engine:
            with patch.object(Result,'download',corrupt):
                with self.assertRaises(RuntimeError):p.sample(engine,request)
            self.assertTrue(held and all(not child.handle for child in held))
            self.assertGreater(p.sample(engine,request).evaluated_count,0)

    def test_closed_engine_and_wrong_request(self):
        from brepgpu import BgError
        p=self.api();request=p.prepare_grid(self.result(),2)
        with self.engine() as engine:
            with self.assertRaises(TypeError):p.sample(engine,object())
            with self.assertRaises(TypeError):p.sample(object(),request)
        with self.assertRaises(BgError):p.sample(engine,request)


if __name__=='__main__':unittest.main()
