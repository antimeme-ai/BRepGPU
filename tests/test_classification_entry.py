"""Direct healthy existing body caller before the new classification entry point."""
import unittest


class ClassificationEntryTests(unittest.TestCase):
    def test_existing_body_then_exact_material_entry(self):
        from brepgpu import Engine
        from brepgpu.body import make_body
        from brepgpu.body_sampling import BodySampler, Candidate, SampleRequest
        cylinder = make_body("cylinder")
        with Engine(backend="cpu", threads=1) as engine:
            with BodySampler(engine, cylinder) as sampler:
                batch = sampler.sample(SampleRequest(cylinder, (Candidate(2, 0., .5),)))
            self.assertEqual(batch.rows[0].native_status, 0)
            self.assertEqual(batch.rows[0].position, (.25, 0., 0.))
        result = cylinder.classify_point((.25, 0., 0.))
        self.assertIs(result.snapshot, cylinder)
        self.assertEqual(result.point, (.25, 0., 0.))
        self.assertEqual(result.label, "boundary")
        self.assertEqual(result.boundary_faces, (3,))


if __name__ == "__main__":
    unittest.main()
