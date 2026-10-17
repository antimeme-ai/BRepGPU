"""Independent exact source-domain reader shared by bounded geometry tests."""
from collections import Counter, defaultdict

from brepgpu.imported_oracles import qualify_domain, turn, _simple_domains


def qualify_chain(case, triangles, *, required_sites=None, positive_cycles=1):
    """Exact positive chain/incidence on an independently admitted simple domain.

    Winding multiplicity proves complete nonoverlapping output coverage. Input
    simplicity must be established by the caller; arbitrary inputs use the
    complete bounded reader below instead of assuming that precondition.
    """
    points = case.points
    boundary = tuple(tuple(points[i] for i in loop) for loop in case.loops)
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
    expected_sites = ({i for loop in case.loops for i in loop}
                      if required_sites is None else set(required_sites))
    if used != expected_sites:
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
    boundary_count = sum(map(len, case.loops))
    negative_cycles = len(case.loops) - positive_cycles
    expected_count = 2*len(used) - boundary_count + 2*negative_cycles - 2*positive_cycles
    if len(triangles) != expected_count:
        raise AssertionError('actual external planar domain Euler triangle count differs')
    return converted, edges


def qualify_external(case, triangles):
    """Source-ID incidence + independent exact domain/embedding, not SDK counts."""
    points = case.points
    boundary = tuple(tuple(points[i] for i in loop) for loop in case.loops)
    _simple_domains(boundary, (True,) + (False,) * (len(boundary)-1))
    converted, edges = qualify_chain(case, triangles)
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

