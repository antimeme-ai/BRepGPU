"""Direct public-caller qualification of conservative named-patch discovery."""
from array import array
from fractions import Fraction
import math
import os
import unittest


class DiscoveryTests(unittest.TestCase):
    def test_constant_third_original_uv_upper_certificate(self):
        # Imports deliberately use only the existing API: missing discovery must
        # fail at the actual new operation, not an eager new-class import.
        from brepgpu import Engine, Patch
        target = float(Fraction(1, 3))
        truth = (Fraction(1, 3) - Fraction.from_float(target)) ** 2
        self.assertGreater(truth, 0)
        with Engine(backend=os.environ.get("BREPGPU_TEST_BACKEND", "cpu"),
                    device=int(os.environ.get("BREPGPU_TEST_DEVICE", "0"))) as engine:
            with engine.create_patch_set([Patch(0, 0, (1., 0., 0., 3.))]) as patches:
                with engine.create_hierarchy(patches) as hierarchy:
                    with engine.create_discovery_query_set(
                            hierarchy, array("I", [0]), array("d", [target, 0., 0.])) as queries:
                        with engine.discover(queries) as result:
                            batch = result.download()
        self.assertEqual(batch.count, 1)
        self.assertEqual(len(batch.data), 8)
        self.assertTrue(all(math.isfinite(x) for x in batch.data))
        self.assertTrue(all(0 <= u <= 1 for u in batch.data[:2]))
        lower, upper = map(Fraction.from_float, batch.data[6:8])
        self.assertLessEqual(lower, truth)
        self.assertGreaterEqual(upper, truth)
        self.assertGreater(upper, 0)
        self.assertEqual(int(batch.statuses[0]), 0)


if __name__ == "__main__":
    unittest.main()
