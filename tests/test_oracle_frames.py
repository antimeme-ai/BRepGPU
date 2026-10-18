"""The oracle reads axis2_placement_3d defaults as ISO 10303-42 states them; fleet only.

first_proj_axis without a ref_direction takes (1,0,0), or (0,1,0) when the axis is
+-(1,0,0) (the adapter review's attack, B2-reader). original_frame refused the second
case as a degenerate direction. Fixture: source-uv.step, usage #386 placed by
ITEM_DEFINED_TRANSFORMATION(#11, #19) with #11 the identity, so the usage's matrix is
frame #19's own: columns x, y = z x x, z and the origin (4,-3,2), derived by hand.
"""
import unittest

from tests.test_placement_oracles import ROTATED_BOX, edited

F19 = "#19 = AXIS2_PLACEMENT_3D('',#20,#21,#22);"


def frame_without_reference(axis):
    return edited((F19, "#19 = AXIS2_PLACEMENT_3D('',#20,#700,$);\n#700 = DIRECTION('',(" + axis + "));"))


class Part42FrameDefaultTests(unittest.TestCase):

    def test_first_proj_axis_defaults(self):
        import tempfile
        from pathlib import Path
        from brepgpu.imported_oracles import _original_usage_transforms, original_records
        cases = {'0.,1.,0.': (1., 0., 0., 4., 0., 0., 1., -3., 0., -1., 0., 2.),
                 '1.,0.,0.': (0., 0., 1., 4., 1., 0., 0., -3., 0., 1., 0., 2.),
                 '-1.,0.,0.': (0., 0., -1., 4., 1., 0., 0., -3., 0., -1., 0., 2.)}
        with tempfile.TemporaryDirectory() as directory:
            for axis, expected in cases.items():
                with self.subTest(axis=axis):
                    step = Path(directory)/'frame.step'
                    step.write_text(frame_without_reference(axis))
                    matrix = _original_usage_transforms(original_records(step))[ROTATED_BOX]
                    for a, b in zip(matrix, expected):
                        self.assertEqual(a, b, matrix)


if __name__ == '__main__':
    unittest.main()
