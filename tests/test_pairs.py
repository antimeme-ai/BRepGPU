"""Direct pair/bounds truth; actual native execution belongs to the fleet."""
import math
import os
import unittest


def engine_arguments():
    return dict(backend=os.environ.get("BREPGPU_TEST_BACKEND", "cpu"),
                device=int(os.environ.get("BREPGPU_TEST_DEVICE", "0")),
                threads=int(os.environ.get("BREPGPU_TEST_THREADS", "1")),
                plugin_path=os.environ.get("BREPGPU_TEST_PLUGIN"))


class PairTests(unittest.TestCase):
    def test_closed_box_contact_is_not_lost(self):
        from brepgpu import Engine
        # A healthy existing Engine precedes the absent-operation direct red.
        with Engine(**engine_arguments()) as engine:
            point = (0., 0., 0., 0., 0., 0.)
            separated = (1., 0., 0., 1., 0., 0.)
            with engine.create_pair_index((point,)) as left, \
                    engine.create_pair_index((point, separated)) as right:
                with engine.discover_pairs(left, right, start=0, max_pairs=2) as result:
                    batch = result.download()
            self.assertEqual(tuple(batch.words), (1,))
            info = batch.info
            self.assertEqual((info.total_pairs, info.start, info.count, info.next_start,
                              info.left_count, info.right_count, info.word_count),
                             (2, 0, 2, 2, 1, 2, 1))
            self.assertEqual(info.flags & 3, 3)
            self.assertFalse(info.flags & 4)
            self.assertTrue(math.isnan(info.kernel_ms))


if __name__ == "__main__":
    unittest.main()
