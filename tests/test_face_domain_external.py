"""Actual native outputs on independently authored, pinned external domains."""
from collections import Counter, defaultdict
from fractions import Fraction
import unittest

from brepgpu.face_domain import first_crossing, triangulate
from brepgpu.imported_oracles import qualify_domain, turn, _simple_domains
from face_domain_external_cases import external_cases, nvidia_case


def qualify_external(case, triangles):
    """Source-ID incidence + independent exact domain/embedding, not SDK counts."""
    points = case.points
    boundary = tuple(tuple(points[i] for i in loop) for loop in case.loops)
    _simple_domains(boundary, (True,) + (False,) * (len(boundary)-1))
    converted = []
    edges = defaultdict(list)
    used = set()
    for triangle in triangles:
        if len(triangle) != 3 or len(set(triangle)) != 3:
            raise AssertionError('actual external triangle has collapsed source IDs')
        if any(identifier not in points for identifier in triangle):
            raise AssertionError('actual external triangle invented a source node')
        used.update(triangle)
        converted.append(tuple(points[i] for i in triangle))
        for a, b in zip(triangle, triangle[1:] + triangle[:1]):
            edges[min(a, b), max(a, b)].append((a, b))
    if used != {i for loop in case.loops for i in loop}:
        raise AssertionError('actual external domain omitted a source node')
    qualify_domain(boundary, converted)
    original = Counter((a, b) for loop in case.loops
                       for a, b in zip(loop, loop[1:] + loop[:1]))
    observed_boundary = Counter()
    for occurrences in edges.values():
        if len(occurrences) == 1:
            observed_boundary[occurrences[0]] += 1
        elif len(occurrences) != 2 or occurrences[0] != occurrences[1][::-1]:
            raise AssertionError('actual external interior edge is not oppositely paired')
    if observed_boundary != original:
        raise AssertionError('actual external original directed source boundary differs')
    expected_count = len(used) + 2*(len(case.loops)-1) - 2
    if len(triangles) != expected_count:
        raise AssertionError('actual external planar domain Euler triangle count differs')
    # Bounded direct embedding checks attack crossings/T junctions independently
    # of the positive boundary-chain reader; no native predicate is reused here.
    all_edges = tuple(edges)
    for i, (a, b) in enumerate(all_edges):
        p, q = points[a], points[b]
        for c, d in all_edges[i+1:]:
            r, s = points[c], points[d]
            x, y, z, w = turn(p, q, r), turn(p, q, s), turn(r, s, p), turn(r, s, q)
            if x*y < 0 and z*w < 0:
                raise AssertionError('actual external triangle edges cross')
            for identifier, point, edge_a, edge_b in (
                    (c, r, a, b), (d, s, a, b), (a, p, c, d), (b, q, c, d)):
                if identifier in (edge_a, edge_b):
                    continue
                start, end = points[edge_a], points[edge_b]
                if (turn(start, end, point) == 0 and
                        all(min(start[k], end[k]) <= point[k] <= max(start[k], end[k])
                            for k in range(2))):
                    raise AssertionError('actual external triangle edges form a T junction/overlap')
    for i, triangle in enumerate(converted):
        for other in converted[i+1:]:
            if any(all(turn(other[j], other[(j+1) % 3], p) > 0 for j in range(3))
                   for p in triangle):
                raise AssertionError('actual external triangles overlap by strict containment')
            if any(all(turn(triangle[j], triangle[(j+1) % 3], p) > 0 for j in range(3))
                   for p in other):
                raise AssertionError('actual external triangles overlap by strict containment')


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
