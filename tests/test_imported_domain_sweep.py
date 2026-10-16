"""Actual independent-domain answers and exact broadphase operation costs."""
from contextlib import ExitStack
from fractions import Fraction as F
import unittest
from unittest import mock

from brepgpu import imported_oracles as oracle
from direct_cdt_cases import SCALING_FAMILIES, authored_cases, scaling_case
from face_domain_external_cases import external_cases


def exact_loop(points):
    return tuple(tuple(map(F, point)) for point in points)


def square(x0, y0, x1, y1, *, hole=False):
    loop = exact_loop(((x0, y0), (x1, y0), (x1, y1), (x0, y1)))
    return loop[::-1] if hole else loop


def strip(count):
    n = count//2
    return tuple((F(i), F(0)) for i in range(n)) + tuple((F(i), F(1)) for i in reversed(range(n)))


class ImportedDomainSweepTests(unittest.TestCase):
    def test_fraction_comparison_work_does_not_enumerate_all_pairs(self):
        loop = strip(512)
        comparisons = 0
        def counting(original):
            def wrapped(left, right):
                nonlocal comparisons
                comparisons += 1
                return original(left, right)
            return wrapped
        # Real standard Fraction inputs retain their exact type and arithmetic;
        # scoped wrappers count comparisons, not min/max implementation syntax.
        with ExitStack() as stack:
            for name in ('__lt__', '__le__', '__gt__', '__ge__'):
                stack.enter_context(mock.patch.object(F, name, counting(getattr(F, name))))
            oracle._simple_domains((loop,), (True,))
        self.assertLess(comparisons, 64*len(loop), f'actual Fraction comparisons: {comparisons}')

    def test_exact_work_counts_and_generic_axis_choice(self):
        for count in (256, 1024):
            original = strip(count)
            for axis, loop in ((0, original), (1, tuple((-y, x) for x, y in original))):
                with self.subTest(count=count, axis=axis):
                    work = oracle._DomainSweepWork()
                    self.assertIsNone(oracle._simple_domains((loop,), (True,), work=work))
                    self.assertEqual(work.segments, len(loop))
                    self.assertEqual(work.sweep_axis, axis)
                    self.assertEqual(work.projected_pairs, work.active_visits)
                    self.assertLess(work.active_visits, 8*len(loop))
                    self.assertEqual(work.exact_tests, 0)
                    self.assertGreater(work.max_active, 0)

    def test_valid_neighbors_collinearity_and_original_source_order(self):
        loops = (
            square(0, 0, 4, 2),
            exact_loop(((1, 0), (3, 0), (3, 2), (0, 2))),
            exact_loop(((0, 0), (1, 0), (2, 0), (2, 1), (0, 1))),
            exact_loop(((0, 0), (2, 0), (0, 2))),
        )
        for loop in loops:
            for shift in range(len(loop)):
                word = loop[shift:]+loop[:shift]
                with self.subTest(loop=loop, shift=shift):
                    oracle._simple_domains((word,), (True,))

    def test_closed_bounds_verticals_proper_crossing_and_nonadjacent_touches(self):
        invalid = (
            exact_loop(((0, 0), (3, 2), (0, 2), (2, 0))),
            exact_loop(((0, 0), (2, 0), (2, 2), (1, 0), (0, 2))),
            exact_loop(((0, 0), (4, 0), (4, 4), (2, 4), (2, 0), (1, 2), (0, 2))),
            exact_loop(((0, 0), (3, 0), (1, 0), (1, 1), (0, 1))),
            exact_loop(((0, 0), (3, 0), (3, 2), (1, 2), (1, 0), (2, 0), (2, 1), (0, 1))),
        )
        for loop in invalid:
            for word in (loop, tuple((-y, x) for x, y in loop)):
                with self.subTest(word=word), self.assertRaisesRegex(AssertionError, 'crosses/touches'):
                    oracle._simple_domains((word,), (True,))

    def test_touching_distinct_loops_use_closed_candidate_bounds(self):
        outer = square(0, 0, 5, 5)
        for holes in ((square(3, 1, 5, 2, hole=True),),
                      (square(1, 1, 2, 2, hole=True), square(2, 2, 3, 3, hole=True))):
            with self.subTest(holes=holes), self.assertRaisesRegex(AssertionError, 'sampled trim domain crosses/touches'):
                oracle._simple_domains((outer, *holes), (True, *((False,)*len(holes))))

    def test_hole_order_containment_nested_holes_and_multiple_outer_owners(self):
        outer = square(0, 0, 10, 10)
        a, b = square(1, 1, 3, 3, hole=True), square(6, 6, 9, 9, hole=True)
        for holes in ((a, b), (b, a)):
            oracle._simple_domains((outer, *holes), (True, False, False))
        with self.assertRaisesRegex(AssertionError, 'holes overlap or nest'):
            oracle._simple_domains((outer, a, square(F(3, 2), F(3, 2), 2, 2, hole=True)),
                                   (True, False, False))
        with self.assertRaisesRegex(AssertionError, 'not inside exactly one outer'):
            oracle._simple_domains((outer, square(11, 11, 12, 12, hole=True)), (True, False))
        oracle._simple_domains((square(0, 0, 4, 4), square(10, 0, 14, 4), a), (True, True, False))
        # Preserve the existing reader's scope: it does not add an outer/outer
        # nesting rule. A hole inside both outer owners is rejected separately.
        oracle._simple_domains((outer, square(1, 1, 9, 9)), (True, True))
        with self.assertRaisesRegex(AssertionError, 'not inside exactly one outer'):
            oracle._simple_domains((outer, square(1, 1, 9, 9), square(6, 6, 8, 8, hole=True)),
                                   (True, True, False))

    def test_original_loop_area_and_collapsed_edge_errors_are_preserved(self):
        for loop, message in ((exact_loop(((0, 0), (1, 0))), 'too few'),
                              (square(0, 0, 1, 1)[::-1], 'winding differs'),
                              (exact_loop(((0, 0), (1, 0), (1, 0), (1, 1), (0, 1))), 'segment collapses')):
            with self.subTest(loop=loop), self.assertRaisesRegex(AssertionError, message):
                oracle._simple_domains((loop,), (True,))

    def test_actual_independent_and_original_external_domain_families(self):
        for case in (*authored_cases(), *external_cases(),
                     *(scaling_case(family, 64) for family in SCALING_FAMILIES)):
            points = case.points
            loops = tuple(tuple(points[i] for i in word) for word in case.loops)
            flags = (True, *((False,)*(len(loops)-1)))
            with self.subTest(case=case.name):
                oracle._simple_domains(loops, flags)
                moved = tuple(tuple((-y+F(2**180, 7), x+F(2**220, 3)) for x, y in loop) for loop in loops)
                oracle._simple_domains(moved, flags)
