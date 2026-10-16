"""Actual native outputs on independently authored, pinned external domains."""
from fractions import Fraction
import unittest

from brepgpu.face_domain import first_crossing, triangulate
from brepgpu.imported_oracles import turn
from face_domain_oracle import qualify_external
from face_domain_external_cases import external_cases, nvidia_case


class ExternalFaceDomainTests(unittest.TestCase):
    def test_complete_original_external_domains(self):
        for case in external_cases():
            with self.subTest(case=case.name):
                points = case.points
                self.assertIsNone(first_crossing(case.loops, points))
                triangles = triangulate(case.loops, points)
                qualify_external(case, triangles)

    def test_external_domains_under_exact_rotation_translation_and_source_id_remap(self):
        # A separately named metamorphic derivative, preserving all source slots.
        # Holes may choose a different valid exact-distance bridge after rotation;
        # demand source geometry/embedding rather than a copied triangle sequence.
        for case in external_cases():
            with self.subTest(case=case.name):
                ids = {i: 2**63 + 17*i for i, _ in case.nodes}
                loops = tuple(tuple(ids[i] for i in loop) for loop in case.loops)
                nodes = tuple((ids[i], (-p[0]+Fraction(1, 3), -p[1]+Fraction(1, 7)))
                              for i, p in case.nodes)
                transformed = type(case)(case.name + '/rotated-rational-remapped', loops, nodes)
                self.assertIsNone(first_crossing(loops, transformed.points))
                qualify_external(transformed, triangulate(loops, transformed.points))

    def test_nvidia_complete_circle_schedule_retains_both_original_holes(self):
        case = nvidia_case()
        self.assertEqual(tuple(map(len, case.loops)), (6, 3, 32))
        points = case.points
        for identifier in case.loops[2]:
            x, y = points[identifier]
            self.assertEqual((x-6)**2 + (y-Fraction(5, 2))**2, Fraction(9, 4))
        triangles = triangulate(case.loops, points)
        qualify_external(case, triangles)
        actual_area = sum(turn(*(points[i] for i in triangle)) / 2 for triangle in triangles)
        circle = tuple(points[i] for i in case.loops[2])
        circle_signed_area = sum(a[0]*b[1]-a[1]*b[0]
                                 for a, b in zip(circle, circle[1:]+circle[:1])) / 2
        self.assertLess(circle_signed_area, 0)
        self.assertEqual(actual_area, Fraction(141, 2) + circle_signed_area)


if __name__ == '__main__':
    unittest.main()
