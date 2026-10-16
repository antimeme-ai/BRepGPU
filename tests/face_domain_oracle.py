"""Independent exact source-domain reader shared by bounded geometry tests."""
from collections import Counter, defaultdict

from brepgpu.imported_oracles import qualify_domain, turn, _simple_domains


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

