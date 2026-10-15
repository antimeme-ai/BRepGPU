"""Independent complete through-bore geometry/history/consumer qualification."""
from fractions import Fraction as F
import unittest


class BooleanTests(unittest.TestCase):
    def test_existing_healthy_body_then_actual_difference(self):
        from brepgpu import Engine
        from brepgpu.body import Frame,make_body
        from brepgpu.body_sampling import BodySampler,Candidate,SampleRequest
        box=make_body('box');cutter=make_body('cylinder',Frame(scale=2.))
        with Engine(backend='cpu',threads=1) as engine:
            with BodySampler(engine,box) as sampler:
                batch=sampler.sample(SampleRequest(box,(Candidate(0,0.,0.),)))
            self.assertEqual(batch.rows[0].native_status,0)
            self.assertEqual(batch.rows[0].position,(-.75,-.75,.5))
        self.assertEqual(cutter.classify_point((0,0,0)).label,'interior')
        from brepgpu.boolean import difference
        result=difference(box,cutter)
        self.assertIs(result.left,box);self.assertIs(result.right,cutter)
        self.assertEqual(result.solid.classify_point((0,0,0)).label,'exterior')
        self.assertEqual(result.solid.classify_point((F(1,2),0,0)).label,'boundary')


if __name__=='__main__':unittest.main()
