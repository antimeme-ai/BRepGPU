"""Direct healthy exact sources before the new planar arrangement entry."""
from fractions import Fraction as F
import unittest


class ArrangementEntryTests(unittest.TestCase):
    def test_existing_exact_circle_then_perforated_cap_regions(self):
        from brepgpu.body import Frame, make_body
        plane = make_body("box", Frame((0., 0., -.5)))
        wall = make_body("cylinder")
        section = plane.intersect_faces(1, wall, 3)
        self.assertEqual(len(section.components), 1)
        self.assertTrue(section.components[0].closed)
        self.assertEqual(section.components[0].kind, "circle")
        self.assertEqual(len(section.pieces), 4)
        self.assertEqual(plane.classify_point((.5, 0, 0)).boundary_faces, (1,))
        arrangement = plane.arrange_face(1, (section,))
        self.assertIs(arrangement.face.snapshot, plane)
        self.assertIs(arrangement.sections[0], section)
        self.assertEqual((len(arrangement.vertices), len(arrangement.edges), len(arrangement.uses)), (5, 5, 10))
        self.assertEqual(tuple(r.kind for r in arrangement.regions), ("outside", "cell", "disk"))
        self.assertEqual(len(arrangement.regions[1].holes), 1)
        self.assertEqual((arrangement.regions[1].area.rational, arrangement.regions[1].area.pi), (F(1), -F(1,36)))
        self.assertEqual(arrangement.locate_uv(F(1,2), F(1,2)), (2,))
        self.assertEqual(arrangement.locate_uv(F(2,3), F(1,2)), (1,2))
        self.assertEqual(arrangement.locate_uv(0, F(1,2)), (0,1))
        circle = next(e for e in arrangement.edges if e.kind == "circle")
        self.assertEqual(circle.start, circle.end)
        self.assertEqual(len(circle.piece_ids), 4)


if __name__ == "__main__":
    unittest.main()
