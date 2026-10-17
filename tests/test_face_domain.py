"""Exact sampled-domain specification tests, executed on the fleet."""
from fractions import Fraction
import unittest
import ctypes as C
import os
from pathlib import Path
import random
import struct
import subprocess
import tempfile
from unittest.mock import patch

from brepgpu.face_domain import (FaceDomain, DomainError, ResourceError, first_crossing, triangulate,
                                 write_snapshot, read_snapshot, _exact_inputs, library_path, require_available)
from brepgpu.imported_meshing import _boundary_intersection, _triangulate
from brepgpu.imported_oracles import qualify_domain


def qualified(loops, points, triangles):
    return qualify_domain(tuple(tuple(points[i] for i in loop) for loop in loops),
                          tuple(tuple(points[i] for i in tri) for tri in triangles))


class FaceDomainSpecification(unittest.TestCase):
    def test_availability_checks_actual_dso_before_geometry(self):
        with tempfile.TemporaryDirectory() as directory:
            invalid = Path(directory)/'invalid.so'
            invalid.write_bytes(b'not a dynamic library')
            with patch.dict(os.environ, BREPGPU_FACE_DOMAIN_LIBRARY=str(invalid)):
                self.assertEqual(library_path(), invalid.resolve())
                with self.assertRaises(OSError):
                    require_available()

    def test_seeded_limb_operations_against_fraction_model(self):
        generator = random.Random(0xB0A7)
        pairs = [(0, 0), (2**64-1, 1), (2**128, -1), (2**513-1, 2**513-2)]
        for _ in range(80):
            widths = (1, 31, 32, 33, 63, 64, 65, 127, 129, 257, 513)
            a = generator.getrandbits(generator.choice(widths)) * generator.choice((-1, 1))
            b = generator.getrandbits(generator.choice(widths)) * generator.choice((-1, 1))
            pairs.append((a, b))
        data = bytearray(struct.pack('<I', 3*len(pairs)))
        def integer(value):
            sign = (value > 0)-(value < 0); value = abs(value)
            count = (value.bit_length()+31)//32
            return struct.pack('<II', sign+1, count)+value.to_bytes(4*count, 'little')
        for a, b in pairs:
            expected = (Fraction(a)+Fraction(b), Fraction(a)-Fraction(b), Fraction(a)*Fraction(b))
            for operation, result in enumerate(expected):
                self.assertEqual(result.denominator, 1)
                data.extend(struct.pack('<I', operation)+integer(a)+integer(b)+integer(result.numerator))
        library = Path(os.environ['BREPGPU_FACE_DOMAIN_LIBRARY'])
        executable = library.parent/'brepgpu_face_domain_arithmetic'
        result = subprocess.run([str(executable)], input=data, capture_output=True)
        self.assertEqual(result.returncode, 0, result.stderr.decode())
        self.assertIn(b'252 exact arithmetic vectors PASS', result.stdout)

    def test_snapshot_exact_roundtrip_and_malformed_input(self):
        points = {0: (Fraction(-1, 7), Fraction(1, 3)), 2**64-1: (2, Fraction(1, 3)),
                  2: (2, 3), 3: (Fraction(-1, 7), 3)}
        loops = ((0, 2**64-1, 2, 3),)
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory)/'face.bgface'
            write_snapshot(path, loops, points, token=17)
            self.assertEqual(read_snapshot(path), (loops, points, 17))
            original = path.read_bytes()
            for changed in (original[:-1], original+b'x', b'BADMAGIC'+original[8:]):
                path.write_bytes(changed)
                with self.assertRaises(DomainError):
                    read_snapshot(path)
        denominator = 1 << 8192
        many = {i: (Fraction(1, denominator) if i == 0 else Fraction(i), Fraction(i)) for i in range(100)}
        with self.assertRaises(ResourceError):
            _exact_inputs((tuple(many),), many, 16000)

    def test_exact_non_dyadic_quad_preserves_cursor_and_zero_id(self):
        points = {0: (Fraction(1, 3), Fraction(1, 7)),
                  1: (Fraction(4, 3), Fraction(1, 7)),
                  2: (Fraction(4, 3), Fraction(8, 7)),
                  3: (Fraction(1, 3), Fraction(8, 7))}
        loops = ((0, 1, 2, 3),)
        self.assertIsNone(first_crossing(loops, points))
        result = triangulate(loops, points)
        self.assertEqual(result, ((3, 0, 1), (1, 2, 3)))
        self.assertEqual(qualified(loops, points, result), 2)

    def test_first_crossing_is_original_lexicographic_pair(self):
        points = {0: (0, 0), 1: (3, 3), 2: (0, 3), 3: (3, 0)}
        loops = ((0, 1, 2, 3),)
        self.assertEqual(first_crossing(loops, points), (0, 1, 2, 3))
        with self.assertRaises(DomainError):
            triangulate(loops, points)

    def test_concave_collinear_source_samples_are_not_deleted(self):
        word = ((0, 0), (2, 0), (4, 0), (4, 3), (2, 1), (0, 3))
        points = dict(enumerate(word)); loops = (tuple(points),)
        result = triangulate(loops, points)
        self.assertEqual(result, tuple(_triangulate(loops, points)))
        qualified(loops, points, result)
        self.assertEqual(set(i for tri in result for i in tri), set(points))

    def test_hole_bridge_occurrences_preserve_source_boundary(self):
        points = dict(enumerate(((0, 0), (7, 0), (7, 7), (0, 7),
                                 (2, 2), (2, 4), (4, 2))))
        loops = ((0, 1, 2, 3), (4, 5, 6))
        result = triangulate(loops, points)
        self.assertEqual(len(result), 7)
        qualified(loops, points, result)
        self.assertEqual(set(i for tri in result for i in tri), set(points))

    def test_wide_cancellation_and_non_dyadic_affine_geometry(self):
        large = (1 << 513) - 1
        base = ((0, 0), (3, 0), (3, 2), (1, 1), (0, 2))
        points = {i: (Fraction(large + x, 7), Fraction(large * 3 + y, 11))
                  for i, (x, y) in enumerate(base)}
        loops = (tuple(points),)
        result = triangulate(loops, points)
        self.assertEqual(result, tuple(_triangulate(loops, points)))
        qualified(loops, points, result)

    def test_closed_boxes_preserve_nonadjacent_touch(self):
        points = dict(enumerate(((0, 0), (4, 0), (2, 0), (2, 2), (0, 2))))
        loops = (tuple(points),)
        self.assertEqual(first_crossing(loops, points),
                         _boundary_intersection(loops, points))
        self.assertIsNotNone(first_crossing(loops, points))

    def test_context_reuse_and_budget_failure_are_not_empty_answers(self):
        points = dict(enumerate(((0, 0), (2, 0), (2, 2), (0, 2))))
        loops = (tuple(points),)
        with FaceDomain(loops, points, token=2**64-1) as domain:
            self.assertEqual(domain.token, 2**64-1)
            self.assertIsNone(domain.first_crossing())
            self.assertEqual(domain.triangulate(), ((3, 0, 1), (1, 2, 3)))
            self.assertEqual(domain.triangulate(), ((3, 0, 1), (1, 2, 3)))
            self.assertEqual(domain.stats.ear_blocker_queries, 0)
        with self.assertRaises(ResourceError):
            first_crossing(loops, points, work_limit=1)
        with self.assertRaises(ResourceError):
            triangulate(loops, points, memory_limit=1)

    def test_malformed_domain_never_publishes_triangles(self):
        points = dict(enumerate(((0, 0), (2, 0), (2, 2), (0, 2))))
        for loops in ((), ((0, 1),), ((0, 1, 99),), ((0, 1, 2, 0),), ((3, 2, 1, 0),)):
            with self.subTest(loops=loops), self.assertRaises(DomainError):
                triangulate(loops, points)


if __name__ == '__main__':
    unittest.main()
