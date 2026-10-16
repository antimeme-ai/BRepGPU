"""Independent CDT semantics on actual existing native/Python consumer output."""
from fractions import Fraction as F
import unittest

from brepgpu.face_domain import triangulate
from brepgpu.imported_meshing import _improve_connectivity
from direct_cdt_oracle import qualify_cdt
from direct_cdt_cases import authored_cases, transformed
from face_domain_external_cases import ExternalCase, external_cases


def case_from(name, coordinates):
    return ExternalCase(name, (tuple(range(len(coordinates))),),
                        tuple(enumerate(coordinates)))


def improve(case, metric=(1., 1.)):
    points = case.points
    constraints = {tuple(sorted((a, b))) for loop in case.loops
                   for a, b in zip(loop, loop[1:] + loop[:1])}
    return _improve_connectivity(triangulate(case.loops, points), points,
                                 constraints, metric)


class DirectCDTOracleTests(unittest.TestCase):
    def test_actual_native_ear_can_be_domain_valid_and_cdt_illegal(self):
        case = case_from('strict-illegal-ear', ((0, 0), (1, 0), (1, 1), (0, 2)))
        ears = triangulate(case.loops, case.points)
        with self.assertRaisesRegex(AssertionError, 'locally illegal'):
            qualify_cdt(case, ears)
        result = improve(case)
        stats = qualify_cdt(case, result)
        self.assertEqual(stats.interior_edges, 1)
        self.assertEqual({tuple(sorted(t)) for t in result}, {(0, 1, 2), (0, 2, 3)})

    def test_cocircular_both_diagonals_are_semantically_valid(self):
        case = case_from('square', ((0, 0), (1, 0), (1, 1), (0, 1)))
        for triangles in (((0, 1, 2), (0, 2, 3)), ((3, 0, 1), (1, 2, 3))):
            stats = qualify_cdt(case, triangles)
            self.assertEqual(stats.cocircular_edges, 1)

    def test_anisotropic_metric_reverses_legal_diagonal(self):
        case = case_from('anisotropic', ((0, 0), (2, 0), (2, 1), (F(1, 2), 2)))
        diagonal02 = ((0, 1, 2), (0, 2, 3))
        diagonal13 = ((3, 0, 1), (1, 2, 3))
        qualify_cdt(case, diagonal02, metric=(1., 1.))
        qualify_cdt(case, diagonal13, metric=(2., 1.))
        for triangles, metric in ((diagonal02, (2., 1.)), (diagonal13, (1., 1.))):
            with self.assertRaisesRegex(AssertionError, 'locally illegal'):
                qualify_cdt(case, triangles, metric=metric)
        for metric in ((1., 1.), (2., 1.)):
            qualify_cdt(case, improve(case, metric), metric=metric)

    def test_authored_independent_families_on_existing_consumers(self):
        for case in authored_cases():
            for metric in ((1., 1.), (16., .25)):
                with self.subTest(case=case.name, metric=metric):
                    qualify_cdt(case, improve(case, metric), metric=metric)

    def test_five_original_external_domains_after_existing_lawson(self):
        for case in external_cases():
            with self.subTest(case=case.name):
                qualify_cdt(case, improve(case))

    def test_source_ids_insertion_order_loop_rotations_and_hole_order(self):
        for case in (*authored_cases(), *external_cases()):
            derived = transformed(case)
            with self.subTest(case=derived.name):
                qualify_cdt(derived, improve(derived))

    def test_bad_source_inventory_and_embedding_are_rejected(self):
        collinear = case_from('protected', ((0, 0), (1, 0), (2, 0), (2, 2), (0, 2)))
        with self.assertRaisesRegex(AssertionError, 'omitted a source node'):
            qualify_cdt(collinear, ((0, 2, 3), (0, 3, 4)))
        square = case_from('square', ((0, 0), (1, 0), (1, 1), (0, 1)))
        for triangles in (((0, 1, 2), (0, 1, 3)), ((0, 1, 9),), ((0, 1, 1),),
                          ((0., 1, 2), (0, 2, 3)), ((False, 1, 2), (0, 2, 3))):
            with self.assertRaises(AssertionError):
                qualify_cdt(square, triangles)

    def test_metric_must_be_exactly_finite_and_positive(self):
        case = case_from('triangle', ((0, 0), (1, 0), (0, 1)))
        for metric in ((0., 1.), (-1., 1.), (float('inf'), 1.),
                       (1., float('nan')), (1.,), (1., 1., 1.)):
            with self.assertRaisesRegex(ValueError, 'metric'):
                qualify_cdt(case, ((0, 1, 2),), metric=metric)

    def test_original_rows_are_checked_before_dictionary_or_set_collapse(self):
        case = case_from('triangle', ((0, 0), (1, 0), (0, 1)))
        malformed = (
            ExternalCase('duplicate-id', case.loops, (*case.nodes, (0, (2, 2)))),
            ExternalCase('duplicate-uv', ((0, 1, 2, 3),), (*case.nodes, (3, (0, 0)))),
            ExternalCase('unreferenced-site', case.loops, (*case.nodes, (3, (2, 2)))),
            ExternalCase('repeated-slot', ((0, 1, 2, 1),), case.nodes),
            ExternalCase('nonfinite', case.loops, ((0, (float('inf'), 0)), *case.nodes[1:])),
            ExternalCase('bool-id', ((False, 1, 2),), ((False, (0, 0)), *case.nodes[1:])),
        )
        for original in malformed:
            with self.subTest(case=original.name), self.assertRaises(AssertionError):
                qualify_cdt(original, ((0, 1, 2),))

    def test_exact_compensated_axis_scaling_preserves_metric_geometry(self):
        case = case_from('axis-scale', ((0, 0), (2, 0), (2, 1), (F(1, 2), 2)))
        metric = (2., 1.)
        triangles = improve(case, metric)
        scaled = ExternalCase('axis-scaled', case.loops,
                              tuple((i, (8*F(x), F(y)/4)) for i, (x, y) in case.nodes))
        stats = qualify_cdt(scaled, triangles, metric=(.25, 4.))
        self.assertEqual(stats, qualify_cdt(case, triangles, metric=metric))


if __name__ == '__main__':
    unittest.main()
