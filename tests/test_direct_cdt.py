"""Actual public direct constructor outputs against independent CDT semantics."""
from dataclasses import asdict
from fractions import Fraction as F
import unittest

from brepgpu.face_domain import DirectCDT, DomainError, ResourceError, constrained_delaunay
from direct_cdt_cases import authored_cases, scaling_case, transformed, validate_scaling_input
from direct_cdt_oracle import qualify_cdt
from face_domain_external_cases import ExternalCase, external_cases


class DirectCDTTests(unittest.TestCase):
    def test_authored_independent_families_metrics_and_seeds(self):
        for case in authored_cases():
            for metric in ((1., 1.), (16., .25)):
                for seed in (0, 1, 2):
                    with self.subTest(case=case.name, metric=metric, seed=seed):
                        output = constrained_delaunay(case.loops, case.points, metric=metric, seed=seed)
                        qualify_cdt(case, output, metric=metric)

    def test_five_original_external_domains(self):
        for case in external_cases():
            for seed in (0, 1, 2):
                with self.subTest(case=case.name, seed=seed):
                    qualify_cdt(case, constrained_delaunay(case.loops, case.points, seed=seed))

    def test_original_source_ids_loop_order_and_node_insertion_order(self):
        for original in (*authored_cases(), *external_cases()):
            case = transformed(original)
            with self.subTest(case=case.name):
                output = constrained_delaunay(case.loops, case.points, seed=1)
                qualify_cdt(case, output)

    def test_anisotropic_legal_diagonal_and_exact_zero(self):
        nodes = tuple(enumerate(((0, 0), (2, 0), (2, 1), (F(1, 2), 2))))
        case = ExternalCase('metric-reversal', ((0, 1, 2, 3),), nodes)
        for metric, diagonal in (((1., 1.), (0, 2)), ((2., 1.), (1, 3)),
                                 ((F(2, 3), F(1, 3)), (1, 3))):
            output = constrained_delaunay(case.loops, case.points, metric=metric)
            qualify_cdt(case, output, metric=metric)
            self.assertTrue(all(set(diagonal) <= set(triangle) for triangle in output))
        square = ExternalCase('cocircular-square', case.loops,
                              tuple(enumerate(((0, 0), (1, 0), (1, 1), (0, 1)))))
        for seed in (0, 1, 2):
            stats = qualify_cdt(square, constrained_delaunay(square.loops, square.points, seed=seed))
            self.assertEqual(stats.cocircular_edges, 1)

    def test_owner_repeat_copies_cached_result_and_close_is_terminal(self):
        case = authored_cases()[1]
        points = case.points
        owner = DirectCDT(case.loops, points, seed=2, token=2**64-1)
        try:
            points[next(iter(points))] = (999, 999)
            output = owner.triangulate()
            qualify_cdt(case, output)
            stats = asdict(owner.stats)
            self.assertEqual(owner.triangulate(), output)
            repeated_stats = asdict(owner.stats)
            # Repeated publication is real work; native output copy time grows.
            stats.pop('copy_ns', None)
            repeated_stats.pop('copy_ns', None)
            self.assertEqual(repeated_stats, stats)
            self.assertGreater(owner.encoded_bytes, 0)
            self.assertGreaterEqual(owner.normalize_ns, 0)
        finally:
            owner.close()
        owner.close()
        with self.assertRaises(DomainError):
            owner.triangulate()

    def test_invalid_metric_and_scalar_aliases_rejected_before_native_use(self):
        case = authored_cases()[0]
        for metric in ((0., 1.), (-1., 1.), (float('inf'), 1.),
                       (1., float('nan')), (1.,), (1., 1., 1.)):
            with self.subTest(metric=metric), self.assertRaises(ValueError):
                constrained_delaunay(case.loops, case.points, metric=metric)
        for kwargs in ({'seed': True}, {'seed': -1}, {'seed': 2**64},
                       {'token': False}, {'token': 2**64}, {'work_limit': True}):
            with self.subTest(kwargs=kwargs), self.assertRaises(ValueError):
                constrained_delaunay(case.loops, case.points, **kwargs)

    def test_input_native_memory_and_work_budgets_are_explicit_failures(self):
        case = authored_cases()[0]
        for kwargs in ({'input_limit': 1}, {'memory_limit': 1}, {'work_limit': 0}):
            with self.subTest(kwargs=kwargs), self.assertRaises(ResourceError):
                constrained_delaunay(case.loops, case.points, **kwargs)
        # One failed independent owner must not poison a later valid owner.
        qualify_cdt(case, constrained_delaunay(case.loops, case.points))

    def test_failed_attempt_is_terminal_and_does_not_reset_work(self):
        case = authored_cases()[1]
        with DirectCDT(case.loops, case.points) as prepared:
            preparation_work = prepared.stats.work
        # Give the second owner exactly the measured deterministic preparation
        # effort: creation succeeds, then construction has no work remaining.
        with DirectCDT(case.loops, case.points, work_limit=preparation_work) as owner:
            with self.assertRaises(ResourceError):
                owner.triangulate()
            failed_work = owner.stats.work
            with self.assertRaises(DomainError):
                owner.triangulate()
            self.assertEqual(owner.stats.work, failed_work)

    def test_source_identity_and_invalid_domains_are_not_silently_repaired(self):
        points = {0: (0, 0), 1: (2, 0), 2: (2, 2), 3: (0, 2)}
        invalid = (
            (((0, 1, 2, 9),), points),
            (((0, 2, 1, 3),), points),
            (((0, 1, 2, 3, 1),), points),
            (((0, 1, 2, 3),), points | {4: (1, 1)}),
            (((0., 1, 2, 3),), points),
            (((False, 1, 2, 3),), points),
            (((0, 1, 2, 3),), {0.: (0, 0), 1: (2, 0), 2: (2, 2), 3: (0, 2)}),
        )
        for loops, coordinates in invalid:
            with self.subTest(loops=loops, coordinates=coordinates), self.assertRaises(ValueError):
                constrained_delaunay(loops, coordinates)

    def test_long_protected_constraint_really_crosses_initial_dt(self):
        for count in (32, 64):
            case = scaling_case('long-recovery', count)
            for seed in (0, 1, 2):
                with self.subTest(sites=count, seed=seed):
                    with DirectCDT(case.loops, case.points, seed=seed) as owner:
                        output = owner.triangulate()
                        stats = owner.stats
                    full = qualify_cdt(case, output)
                    chain = qualify_cdt(case, output, input_validator=lambda actual:
                                        validate_scaling_input('long-recovery', actual))
                    self.assertEqual(full, chain)
                    self.assertGreater(stats.crossed_edges, 0)
                    self.assertGreater(stats.cavity_occurrences, 0)
