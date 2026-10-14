"""Direct complete analytic intersections; actual native tests run on the fleet."""
from fractions import Fraction
import os
import unittest


def engine_arguments():
    return dict(backend=os.environ.get("BREPGPU_TEST_BACKEND", "cpu"),
                device=int(os.environ.get("BREPGPU_TEST_DEVICE", "0")),
                threads=int(os.environ.get("BREPGPU_TEST_THREADS", "1")),
                plugin_path=os.environ.get("BREPGPU_TEST_PLUGIN"))


class IntersectionTests(unittest.TestCase):
    def assert_rational_values(self, actual, expected):
        self.assertEqual(len(actual), len(expected))
        for value, reference in zip(actual, expected):
            # Inspect raw exact output; do not use production equality/rounding
            # as the expected answer in this literal first contract.
            self.assertEqual(value.b, 0)
            self.assertEqual(value.a, Fraction(reference))

    def test_body_entry_point_complete_circle_and_coupled_maps(self):
        from brepgpu import Engine
        from brepgpu.body import Frame, make_body
        from brepgpu.body_sampling import Candidate, SampleRequest, BodySampler
        plane = make_body("box", Frame((0., 0., -.5), 1.))
        wall = make_body("cylinder")
        # Qualified existing body/native setup precedes the intended absent
        # BodySnapshot.intersect_faces red; no new-module import gates it.
        with Engine(**engine_arguments()) as engine:
            for snapshot, slot, expected in ((plane, 0, (0., 0., 0.)),
                                              (wall, 2, (.25, 0., 0.))):
                with BodySampler(engine, snapshot) as sampler:
                    request = SampleRequest(snapshot, (Candidate(slot, .5 if slot == 0 else 0., .5),))
                    batch = sampler.sample(request)
                self.assertEqual(batch.rows[0].native_status, 0)
                self.assertEqual(batch.rows[0].position, expected)
            result = plane.intersect_faces(1, wall, 3)
        self.assertIs(result.left.snapshot, plane)
        self.assertIs(result.right.snapshot, wall)
        self.assertEqual(len(result.components), 1)
        self.assertTrue(result.components[0].closed)
        self.assertFalse(result.components[0].tangent)
        self.assertEqual(len(result.components[0].piece_ids), 4)
        self.assertEqual(len(result.pieces), 4)
        self.assertEqual(len({piece.span_id for piece in result.pieces}), 4)
        self.assertEqual({(piece.left_slot, piece.right_slot) for piece in result.pieces},
                         {(0, 2), (0, 3), (0, 4), (0, 5)})
        piece = next(piece for piece in result.pieces if piece.right_slot == 2)
        self.assert_rational_values((piece.lo, piece.hi), (0, 1))
        self.assertEqual(len(piece.spatial_map.numerators), 3)
        self.assertEqual(len(piece.left_map.numerators), 2)
        self.assertEqual(len(piece.right_map.numerators), 2)
        self.assertIsInstance(piece.spatial_map.denominator, tuple)
        for parameter, position, left_uv, right_uv in (
                (Fraction(0), (Fraction(1,4), 0, 0), (Fraction(2,3), Fraction(1,2)), (0, Fraction(1,2))),
                (Fraction(1,2), (Fraction(3,20), Fraction(1,5), 0), (Fraction(3,5), Fraction(19,30)), (Fraction(1,2), Fraction(1,2))),
                (Fraction(1), (0, Fraction(1,4), 0), (Fraction(1,2), Fraction(2,3)), (1, Fraction(1,2)))):
            self.assert_rational_values(piece.position(parameter), position)
            self.assert_rational_values(piece.left_uv(parameter), left_uv)
            self.assert_rational_values(piece.right_uv(parameter), right_uv)


if __name__ == "__main__":
    unittest.main()
