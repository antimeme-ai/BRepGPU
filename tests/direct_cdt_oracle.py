"""Simple exact CDT output semantics, independent of native/legacy predicates."""
from collections import Counter, defaultdict
from dataclasses import dataclass
from fractions import Fraction

from brepgpu.imported_oracles import turn
from face_domain_external_cases import ExternalCase
from face_domain_oracle import qualify_chain, qualify_external


@dataclass(frozen=True)
class CDTStatistics:
    sites: int
    constraints: int
    triangles: int
    interior_edges: int
    cocircular_edges: int


def _determinant(matrix):
    """Generic rational elimination, with no incircle-specific expansion."""
    rows = [list(map(Fraction, row)) for row in matrix]
    determinant = Fraction(1)
    for column in range(len(rows)):
        pivot = next((i for i in range(column, len(rows)) if rows[i][column]), None)
        if pivot is None:
            return Fraction(0)
        if pivot != column:
            rows[column], rows[pivot] = rows[pivot], rows[column]
            determinant = -determinant
        value = rows[column][column]
        determinant *= value
        for i in range(column + 1, len(rows)):
            factor = rows[i][column] / value
            for j in range(column + 1, len(rows)):
                rows[i][j] -= factor * rows[column][j]
            rows[i][column] = Fraction(0)
    return determinant


def _admitted_case(case):
    """Validate original rows before a dictionary or set can hide information."""
    try:
        nodes = tuple((i, tuple(Fraction(x) for x in p)) for i, p in case.nodes)
        loops = tuple(tuple(word) for word in case.loops)
    except (TypeError, ValueError, OverflowError) as error:
        raise AssertionError('source requires finite exact 2D values and loop words') from error
    ids = tuple(i for i, _ in nodes)
    if any(type(i) is not int or not 0 <= i < 2**64 for i in ids):
        raise AssertionError('source node IDs must be unsigned64 integers')
    if len(set(ids)) != len(ids):
        raise AssertionError('source node IDs must be unique before dictionary construction')
    if not nodes or any(len(p) != 2 for _, p in nodes):
        raise AssertionError('source requires nonempty 2D node rows')
    if len({p for _, p in nodes}) != len(nodes):
        raise AssertionError('source nodes must have distinct exact UVs')
    if not loops or any(len(word) < 3 for word in loops):
        raise AssertionError('source requires loops of at least three sites')
    if any(type(i) is not int or not 0 <= i < 2**64 for word in loops for i in word):
        raise AssertionError('source loop node IDs must be unsigned64 integers')
    if Counter(i for word in loops for i in word) != Counter(ids):
        raise AssertionError('every original source node must occur exactly once in loop slots')
    return ExternalCase(case.name, loops, nodes)


def qualify_cdt(case, triangles, *, metric=(1., 1.), input_validator=None):
    """Require fixed-site source-domain coverage/embedding and local metric CDT."""
    try:
        scales = tuple(Fraction(value) for value in metric)
    except (TypeError, ValueError, OverflowError) as error:
        raise ValueError('metric requires two finite positive exact scales') from error
    if len(scales) != 2 or any(value <= 0 for value in scales):
        raise ValueError('metric requires two finite positive exact scales')
    original = _admitted_case(case)
    triangles = tuple(tuple(triangle) for triangle in triangles)
    if any(type(i) is not int or not 0 <= i < 2**64 for triangle in triangles for i in triangle):
        raise AssertionError('output source node IDs must be unsigned64 integers')
    if input_validator is None:
        qualify_external(original, triangles)
    else:
        if input_validator(original) is not True:
            raise AssertionError('independent original input validation did not pass')
        qualify_chain(original, triangles)
    points = original.points
    constraints = {tuple(sorted((a, b))) for loop in original.loops
                   for a, b in zip(loop, loop[1:] + loop[:1])}
    incidence = defaultdict(list)
    for triangle in triangles:
        for j in range(3):
            a, b, c = triangle[j], triangle[(j+1) % 3], triangle[(j+2) % 3]
            incidence[min(a, b), max(a, b)].append((a, b, c))
    checked = zeros = 0
    for edge, uses in incidence.items():
        if edge in constraints:
            continue
        if len(uses) != 2:
            raise AssertionError(f'interior edge {edge} must have exactly two owners')
        (a, b, c), (opposite_b, opposite_a, d) = uses
        if (opposite_a, opposite_b) != (a, b) or len({a, b, c, d}) != 4:
            raise AssertionError(f'interior edge {edge} has invalid source incidence')
        if turn(points[a], points[b], points[c]) <= 0 or turn(points[b], points[a], points[d]) <= 0:
            raise AssertionError(f'interior edge {edge} has invalid triangle orientations')
        # Build the complete unshifted rational matrix from original values.
        matrix = []
        for identifier in (a, b, c, d):
            x, y = (value * scale for value, scale in zip(points[identifier], scales))
            matrix.append((x, y, x*x + y*y, Fraction(1)))
        determinant = _determinant(matrix)
        if determinant > 0:
            raise AssertionError(f'unconstrained source edge {edge} is locally illegal; '
                                 f'opposite source nodes {(c, d)}')
        checked += 1
        zeros += determinant == 0
    return CDTStatistics(len(points), len(constraints), len(triangles), checked, zeros)
