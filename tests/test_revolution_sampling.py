"""Actual owned periodic-surface sampling and native resource lifetime."""
from dataclasses import FrozenInstanceError
from fractions import Fraction as F
import math
import os
import unittest
from unittest.mock import patch

TRIANGLE=((F(1,4),-F(1,2)),(F(3,4),-F(1,4)),(F(1,2),F(1,2)))
# Integer strictly convex32-gon, centered radially after division by256.
MANY=((64,0),(63,13),(59,25),(53,36),(46,46),(36,53),(25,59),(13,63),
      (0,64),(-13,63),(-25,59),(-36,53),(-46,46),(-53,36),(-59,25),(-63,13),
      (-64,0),(-63,-13),(-59,-25),(-53,-36),(-46,-46),(-36,-53),(-25,-59),(-13,-63),
      (0,-64),(13,-63),(25,-59),(36,-53),(46,-46),(53,-36),(59,-25),(63,-13))


class RevolutionSamplingTests(unittest.TestCase):
    def api(self):
        from brepgpu import revolution_sampling
        return revolution_sampling

    def result(self,points=TRIANGLE,origin=(0.,0.,0.),scale=1.):
        from brepgpu.body import Frame
        from brepgpu.extrusion import make_profile
        from brepgpu.revolution import revolve
        return revolve(make_profile(points,Frame(origin,scale)))

    def engine(self):
        from brepgpu import Engine
        return Engine(backend=os.environ.get('BREPGPU_TEST_BACKEND','cpu'),
            device=int(os.environ.get('BREPGPU_TEST_DEVICE','0')),
            threads=int(os.environ.get('BREPGPU_TEST_THREADS','1')))

    def test_entry(self):
        from brepgpu import Patch
        self.assertEqual(Patch(0,0,[(0,0,0,1)]).controls,(0.,0.,0.,1.))
        self.assertTrue(callable(self.api().sample))

    def test_request_copy_labels_and_immutable_binding(self):
        from brepgpu.body_sampling import Candidate
        from brepgpu.body import INTERIOR,BOUNDARY,INVALID
        p=self.api();result=self.result()
        submitted=[Candidate(0,.5,.5),Candidate(0,0,.5),Candidate(0,1,.5),
                   Candidate(1,0,.5),Candidate(3,1,.5),Candidate(2,.5,0),
                   Candidate(2,.5,float('nan')),Candidate(0,-.25,.5)]
        request=p.RevolutionRequest(result,submitted);submitted.clear()
        self.assertIs(request.result,result)
        self.assertEqual(request.labels,(INTERIOR,BOUNDARY,INTERIOR,INTERIOR,
                                         BOUNDARY,BOUNDARY,INVALID,INVALID))
        self.assertEqual(request.admitted_indices,(0,1,2,3,4,5))
        self.assertEqual(len(request.candidates),8)
        with self.assertRaises(FrozenInstanceError):request.labels=()
        with self.assertRaises(FrozenInstanceError):request.candidates[0].u=0.
        copied=p.RevolutionRequest(result,[Candidate(0,F(1,3),F(2,3))])
        self.assertIs(type(copied.candidates[0].u),float)
        self.assertEqual((copied.candidates[0].u,copied.candidates[0].v),
                         (float(F(1,3)),float(F(2,3))))
        self.assertEqual(copied.labels,(result.solid.classify(0,float(F(1,3)),float(F(2,3))),))

    def test_budget_complete_128_chart_grid_and_operation_guards(self):
        from brepgpu.body_sampling import Candidate
        p=self.api();result=self.result()
        for value in (result.profile,result.solid,object()):
            with self.assertRaises(TypeError):p.RevolutionRequest(value,[])
        with self.assertRaises(TypeError):p.RevolutionRequest(result,[object()])
        with self.assertRaises(ValueError):p.RevolutionRequest(result,[Candidate(12,0,0)])
        for value in (True,1,258,2.5):
            with self.assertRaises((TypeError,ValueError)):p.prepare_grid(result,value)
        self.assertEqual(len(p.prepare_grid(result).candidates),972)
        many=self.result(tuple((F(128+x,256),F(y,256)) for x,y in MANY))
        grid=p.prepare_grid(many,2)
        self.assertEqual((len(grid.candidates),len(grid.admitted_indices)),(128*4,128*4))
        self.assertEqual({c.patch_slot for c in grid.candidates},set(range(128)))
        with self.assertRaises(ValueError):p.prepare_grid(many,257)
        seen=[]
        def infinite():
            while True:
                seen.append(1);yield Candidate(0,0,0)
        with patch.object(p,'MAX_CANDIDATES',2):
            with self.assertRaises(ValueError):p.RevolutionRequest(result,infinite())
            with self.assertRaises(ValueError):p.prepare_grid(result,2)
        self.assertEqual(len(seen),3)

    def test_actual_nonaxis_native_rows_and_source_frame(self):
        from brepgpu.body_sampling import Candidate
        from brepgpu.body import INTERIOR,BOUNDARY,INVALID
        p=self.api();result=self.result(origin=(3.,-2.,1.),scale=2.)
        request=p.RevolutionRequest(result,[Candidate(0,.5,.5),Candidate(1,0,.5),
            Candidate(3,1,.5),Candidate(4,0,.5),Candidate(0,.5,float('inf'))])
        with self.engine() as engine:
            batch=p.sample(engine,request)
            self.assertEqual((batch.backend,batch.device),(engine.backend,engine.device))
        self.assertIs(batch.result,result);self.assertIs(batch.request,request)
        self.assertEqual((batch.count,batch.evaluated_count),(5,4))
        root5=math.sqrt(5);root10=math.sqrt(10)
        expected={0:((3.6,-2.75,.2),(3/(5*root5),-2/root5,-4/(5*root5)),INTERIOR),
                  1:((3.,-2.75,0.),(0.,-2/root5,-1/root5),INTERIOR),
                  2:((4.,-2.75,1.),(1/root5,-2/root5,0.),BOUNDARY),
                  3:((4.25,-1.75,1.),(3/root10,1/root10,0.),BOUNDARY)}
        for index,row in enumerate(batch.rows):
            slot=request.candidates[index].patch_slot
            self.assertEqual((row.index,row.patch_slot,row.face_id,row.chart_id),
                             (index,slot,slot//4+1,slot+1))
            if index in expected:
                point,normal,label=expected[index]
                self.assertEqual((row.trim,row.native_status),(label,0))
                for a,b in zip(row.position,point):self.assertLessEqual(abs(a-b),2**-36)
                for a,b in zip(row.normal,normal):self.assertLessEqual(abs(a-b),2**-30)
            else:
                self.assertEqual((row.trim,row.native_status),(INVALID,-1))
                self.assertTrue(all(math.isnan(x) for x in (*row.position,*row.normal)))
        self.assertTrue(all(math.isfinite(value) and value>=0 for _,value in batch.timings))
        with self.assertRaises((TypeError,FrozenInstanceError)):batch.rows=()

    def test_native_periodic_seam_and_quarter_join_rows(self):
        from brepgpu.body_sampling import Candidate
        from brepgpu.body import INTERIOR,BOUNDARY
        p=self.api();result=self.result()
        candidates=[Candidate(0,0,F(1,3)),Candidate(3,1,F(1,3))]
        for q in range(3):candidates.extend((Candidate(q,1,F(1,3)),Candidate(q+1,0,F(1,3))))
        with self.engine() as engine:batch=p.sample(engine,p.RevolutionRequest(result,candidates))
        for index in range(0,len(batch.rows),2):
            a,b=batch.rows[index:index+2]
            self.assertEqual((a.trim,b.trim),(BOUNDARY,BOUNDARY) if index==0 else (INTERIOR,INTERIOR))
            for x,y in zip(a.position,b.position):self.assertLessEqual(abs(x-y),2**-36)
            for x,y in zip(a.normal,b.normal):self.assertLessEqual(abs(x-y),2**-30)

    def test_empty_all_invalid_and_engine_guards(self):
        from brepgpu.body_sampling import Candidate
        from brepgpu import BgError
        p=self.api();result=self.result()
        with self.engine() as engine:
            for candidates in ([],[Candidate(0,-1,.5),Candidate(0,.5,float('nan'))]):
                batch=p.sample(engine,p.RevolutionRequest(result,candidates))
                self.assertEqual((batch.count,batch.evaluated_count),(len(candidates),0))
                self.assertTrue(all(row.native_status==-1 and all(math.isnan(x) for x in
                    (*row.position,*row.normal)) for row in batch.rows))
            with self.assertRaises(TypeError):p.sample(engine,object())
            with self.assertRaises(TypeError):p.sample(object(),p.prepare_grid(result,2))
        with self.assertRaises(BgError):p.sample(engine,p.prepare_grid(result,2))

    def test_late_host_failure_closes_three_children_and_same_engine_retry(self):
        p=self.api();result=self.result();request=p.prepare_grid(result,3)
        held=[];failure=RuntimeError('actual late revolution host failure')
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
            self.assertEqual(healthy.evaluated_count,len(request.candidates))

    def test_actual_download_descriptor_and_status_rejection_then_retry(self):
        from brepgpu import Result
        p=self.api();result=self.result();request=p.prepare_grid(result,3)
        original=Result.download
        for kind in ('descriptor','status'):
            held=[]
            def corrupt(resource):
                held.append(resource);jets=original(resource)
                if kind=='descriptor':jets._descriptors[3].origin[0]+=.25
                else:jets.statuses[0]=1
                return jets
            with self.engine() as engine:
                with patch.object(Result,'download',corrupt):
                    with self.assertRaises(RuntimeError):p.sample(engine,request)
                self.assertTrue(held and all(not child.handle for child in held))
                self.assertEqual(p.sample(engine,request).evaluated_count,len(request.candidates))


if __name__=='__main__':unittest.main()
