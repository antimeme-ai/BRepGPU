"""Direct owned extrusion sampling and native-resource lifetime contracts."""
from dataclasses import FrozenInstanceError
from fractions import Fraction as F
import math
import os
import unittest
from unittest.mock import patch

TRIANGLE=((-F(1,2),-F(1,2)),(F(1,2),-F(1,2)),(F(0),F(1,2)))
# Strict-convex integer circle: 32 lattice points; no trigonometric source recipe.
MANY=((128,0),(126,25),(118,49),(106,71),(91,91),(71,106),(49,118),(25,126),
      (0,128),(-25,126),(-49,118),(-71,106),(-91,91),(-106,71),(-118,49),(-126,25),
      (-128,0),(-126,-25),(-118,-49),(-106,-71),(-91,-91),(-71,-106),(-49,-118),(-25,-126),
      (0,-128),(25,-126),(49,-118),(71,-106),(91,-91),(106,-71),(118,-49),(126,-25))


class ExtrusionSamplingTests(unittest.TestCase):
    def api(self):
        from brepgpu import extrusion_sampling
        return extrusion_sampling

    def result(self,points=TRIANGLE,origin=(0.,0.,0.),scale=1.,height=1):
        from brepgpu.body import Frame
        from brepgpu.extrusion import make_profile,extrude
        return extrude(make_profile(points,Frame(origin,scale)),height)

    def engine(self):
        from brepgpu import Engine
        return Engine(backend=os.environ.get('BREPGPU_TEST_BACKEND','cpu'),
            device=int(os.environ.get('BREPGPU_TEST_DEVICE','0')),
            threads=int(os.environ.get('BREPGPU_TEST_THREADS','1')))

    def test_entry(self):
        from brepgpu import Patch
        self.assertEqual(Patch(0,0,[(0,0,0,1)]).controls,(0.,0.,0.,1.))
        self.assertTrue(callable(self.api().sample))

    def test_request_bits_labels_and_owned_rows(self):
        from brepgpu.body_sampling import Candidate
        from brepgpu.body import REJECTED,INTERIOR,BOUNDARY,INVALID
        p=self.api();result=self.result()
        source=[Candidate(0,.5,.5),Candidate(0,0.,0.),Candidate(0,.5,.25),
                Candidate(3,.5,.5),Candidate(3,.5,float('nan'))]
        request=p.ExtrusionRequest(result,source);source.clear()
        self.assertIs(request.result,result)
        self.assertEqual(request.labels,(INTERIOR,REJECTED,BOUNDARY,INTERIOR,INVALID))
        self.assertEqual(request.admitted_indices,(0,2,3))
        self.assertEqual(len(request.candidates),5)
        with self.assertRaises(FrozenInstanceError):request.labels=()
        with self.assertRaises(FrozenInstanceError):request.candidates[0].u=0.
        rounded=p.ExtrusionRequest(result,[Candidate(0,F(1,3),F(1,3))])
        self.assertEqual(rounded.labels,(result.solid.classify(0,float(F(1,3)),float(F(1,3))),))

    def test_budget_and_34_chart_grid(self):
        from brepgpu.body_sampling import Candidate
        p=self.api();result=self.result()
        for value in (result.profile,result.solid,object()):
            with self.assertRaises(TypeError):p.ExtrusionRequest(value,[])
        with self.assertRaises(TypeError):p.ExtrusionRequest(result,[object()])
        with self.assertRaises(ValueError):p.ExtrusionRequest(result,[Candidate(5,0,0)])
        for value in (True,1,258,2.5):
            with self.assertRaises((TypeError,ValueError)):p.prepare_grid(result,value)
        many=self.result(tuple((F(x,256),F(y,256)) for x,y in MANY))
        grid=p.prepare_grid(many,2)
        self.assertEqual(len(grid.candidates),34*4)
        self.assertEqual({c.patch_slot for c in grid.candidates},set(range(34)))
        with self.assertRaises(ValueError):p.prepare_grid(many,257)
        seen=[]
        def infinite():
            while True:
                seen.append(1);yield Candidate(0,0,0)
        with patch.object(p,'MAX_CANDIDATES',2):
            with self.assertRaises(ValueError):p.ExtrusionRequest(result,infinite())
            with self.assertRaises(ValueError):p.prepare_grid(result,2)
        self.assertEqual(len(seen),3)

    def test_actual_nonaxis_native_rows_and_frames(self):
        from brepgpu.body_sampling import Candidate
        from brepgpu.body import REJECTED,INTERIOR
        p=self.api();result=self.result(origin=(3.,-2.,1.),scale=2.,height=F(3,4))
        request=p.ExtrusionRequest(result,[Candidate(0,.5,.5),Candidate(1,.5,.5),
            Candidate(3,.5,.5),Candidate(0,0,0),Candidate(3,.5,float('inf'))])
        with self.engine() as engine:
            batch=p.sample(engine,request)
            self.assertEqual(batch.backend,engine.backend);self.assertEqual(batch.device,engine.device)
        self.assertIs(batch.result,result);self.assertIs(batch.request,request)
        self.assertEqual((batch.count,batch.evaluated_count),(5,3))
        expected={0:((3.,-2.,.25),(0.,0.,-1.)),1:((3.,-2.,1.75),(0.,0.,1.)),
                  2:((3.5,-2.,1.),(2/math.sqrt(5),1/math.sqrt(5),0.))}
        for index,row in enumerate(batch.rows):
            self.assertEqual(row.index,index);self.assertEqual(row.patch_slot,request.candidates[index].patch_slot)
            self.assertEqual(row.face_id,request.candidates[index].patch_slot+1)
            self.assertEqual(row.chart_id,row.face_id)
            if index in expected:
                point,normal=expected[index];self.assertEqual(row.trim,INTERIOR);self.assertEqual(row.native_status,0)
                for a,b in zip(row.position,point):self.assertLessEqual(abs(a-b),2**-36)
                for a,b in zip(row.normal,normal):self.assertLessEqual(abs(a-b),2**-30)
            else:
                self.assertEqual(row.native_status,-1)
                self.assertTrue(all(math.isnan(x) for x in (*row.position,*row.normal)))
        self.assertEqual(batch.rows[3].trim,REJECTED)
        self.assertTrue(all(math.isfinite(x) and x>=0 for _,x in batch.timings))
        with self.assertRaises((TypeError,FrozenInstanceError)):batch.rows=()

    def test_empty_all_rejected_and_type_guards(self):
        from brepgpu.body_sampling import Candidate
        from brepgpu import BgError
        p=self.api();result=self.result()
        with self.engine() as engine:
            for candidates in ([],[Candidate(0,0,0)]):
                batch=p.sample(engine,p.ExtrusionRequest(result,candidates))
                self.assertEqual((batch.count,batch.evaluated_count),(len(candidates),0))
            with self.assertRaises(TypeError):p.sample(engine,object())
            with self.assertRaises(TypeError):p.sample(object(),p.prepare_grid(result,2))
        with self.assertRaises(BgError):p.sample(engine,p.prepare_grid(result,2))

    def test_late_host_failure_cleanup_and_same_engine_retry(self):
        p=self.api();result=self.result();request=p.prepare_grid(result,3)
        held=[];failure=RuntimeError('actual late extrusion host failure')
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

    def test_real_download_descriptor_rejection_and_retry(self):
        from brepgpu import Result
        p=self.api();result=self.result();request=p.prepare_grid(result,3)
        original=Result.download;held=[]
        def corrupt(resource):
            held.append(resource);jets=original(resource);jets._descriptors[3].origin[0]+=.25;return jets
        with self.engine() as engine:
            with patch.object(Result,'download',corrupt):
                with self.assertRaises(RuntimeError):p.sample(engine,request)
            self.assertTrue(held and all(not c.handle for c in held))
            self.assertGreater(p.sample(engine,request).evaluated_count,0)


if __name__=='__main__':unittest.main()
