"""Healthy existing geometry before the complete trim-aware mesh entry."""
from fractions import Fraction as F
import unittest


class MeshingEntryTests(unittest.TestCase):
    def test_existing_body_then_complete_plate_mesh(self):
        from brepgpu import Engine
        from brepgpu.body import make_body
        from brepgpu.body_sampling import BodySampler,Candidate,SampleRequest
        cylinder = make_body('cylinder')
        with Engine(backend='cpu',threads=1) as engine:
            with BodySampler(engine,cylinder) as sampler:
                batch = sampler.sample(SampleRequest(cylinder,(Candidate(2,0.,.5),)))
            self.assertEqual(batch.rows[0].native_status,0)
            self.assertEqual(batch.rows[0].position,(.25,0.,0.))
        plate = make_body('plate')
        self.assertEqual(plate.classify_point((0,0,.5)).label,'exterior')
        tolerance = F(1,4)
        mesh = plate.tessellate(tolerance=tolerance,max_quarter_segments=1)
        self.assertIs(mesh.body,plate)
        self.assertEqual(mesh.tolerance,tolerance)
        self.assertEqual(mesh.quarter_segments,1)
        self.assertEqual((len(mesh.vertices),len(mesh.triangles)),(16,32))
        self.assertEqual(len(mesh.boundary_uses),len(plate.coedges))
        self.assertLessEqual(mesh.error_bound,tolerance)


if __name__=='__main__':
    unittest.main()
