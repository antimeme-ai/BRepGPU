"""Direct canonical-body sampling contracts; native calls run on the fleet."""
from dataclasses import FrozenInstanceError
import math
import os
import unittest


def engine_arguments():
    return dict(backend=os.environ.get("BREPGPU_TEST_BACKEND", "cpu"),
                device=int(os.environ.get("BREPGPU_TEST_DEVICE", "0")),
                threads=int(os.environ.get("BREPGPU_TEST_THREADS", "1")),
                plugin_path=os.environ.get("BREPGPU_TEST_PLUGIN"))


class BodyTests(unittest.TestCase):
    def test_box_samples_preserve_exact_source_and_invalid_rows(self):
        # Existing native engine must work; absent new body API is the direct red.
        from brepgpu import Engine
        with Engine(**engine_arguments()) as engine:
            from brepgpu.body import make_body
            from brepgpu.body_sampling import Candidate, SampleRequest, BodySampler
            from brepgpu.body_oracles import qualify_batch
            snapshot = make_body("box")
            top = next(i for i, chart in enumerate(snapshot.charts)
                       if snapshot.faces[chart.face_slot].name == "top")
            request = SampleRequest(snapshot, (Candidate(top, .25, .75),
                                                Candidate(top, math.nan, .5),
                                                Candidate(top, -1., .5)))
            with BodySampler(engine, snapshot) as sampler:
                batch = sampler.sample(request)
            metrics = qualify_batch(batch)
            self.assertEqual((batch.count, batch.evaluated_count), (3, 1))
            self.assertEqual((metrics["accepted"], metrics["invalid"]), (1, 2))
            for actual, expected in zip(batch.rows[0].position, (-.375, .375, .5)):
                self.assertLessEqual(abs(actual-expected), 2.**-36)
            self.assertEqual(batch.rows[0].normal, (0., 0., 1.))


if __name__ == "__main__":
    unittest.main()
