"""Independent, bounded exact domain families chosen before CDT tuning."""
from fractions import Fraction as F

from face_domain_external_cases import ExternalCase


def _case(name, loops):
    nodes, words = [], []
    for loop in loops:
        word = []
        for point in loop:
            identifier = len(nodes)
            nodes.append((identifier, tuple(F(value) for value in point)))
            word.append(identifier)
        words.append(tuple(word))
    return ExternalCase(name, tuple(words), tuple(nodes))


def authored_cases():
    ring = []
    for quadrant in range(4):
        for k in range(4):
            t = F(k, 4)
            x, y = (1-t*t)/(1+t*t), 2*t/(1+t*t)
            for _ in range(quadrant):
                x, y = -y, x
            ring.append((x, y))
    stations = tuple(F(i) + F(i*i, 1024) for i in range(9))
    strip = tuple((x, 0) for x in stations) + tuple((x, F(1, 16)) for x in reversed(stations))
    comb = ((0, 0), (12, 0), (12, 5), (10, 5), (10, 1), (9, 1), (9, 5),
            (7, 5), (7, 1), (6, 1), (6, 5), (4, 5), (4, 1), (3, 1), (3, 5), (0, 5))
    corridor = ((0, 0), (10, 0), (10, 10), (0, 10), (0, 8), (8, 8), (8, 2), (0, 2))
    outer = ((0, 0), (20, 0), (20, 12), (0, 12))
    holes = (((2, 2), (2, 5), (5, 5), (5, 2)),
             ((8, 6), (8, 10), (11, 10), (11, 6)), ((14, 3), (14, 5), (16, 3)))
    epsilon = F(1, 2**180)
    near = ((0, 0), (1, epsilon), (2, 0), (3, 1), (0, 1))
    almost_circle = ((0, 0), (1, 0), (1, 1), (0, 1+epsilon))
    offset = (F(2**260, 3), F(2**220, 7))
    wide = tuple((x+offset[0], y+offset[1]) for x, y in almost_circle)
    return (_case('authored/strict-convex-rational-circle', (ring,)),
            _case('authored/protected-nonuniform-thin-strip', (strip,)),
            _case('authored/concave-comb', (comb,)),
            _case('authored/non-star-corridor', (corridor,)),
            _case('authored/staggered-three-holes', (outer, *holes)),
            _case('authored/exact-nearly-collinear', (near,)),
            _case('authored/wide-nearly-cocircular', (wide,)))


def transformed(case):
    """One exact geometry transform plus independent input-order/identity changes."""
    count = len(case.nodes)
    ids = {old: (0 if index == count-1 else 2**63 + 97*(count-index))
           for index, (old, _) in enumerate(case.nodes)}
    loops = []
    for index, loop in enumerate(case.loops):
        word = tuple(ids[i] for i in loop)
        rotation = (index+1) % len(word)
        loops.append(word[rotation:] + word[:rotation])
    loops = loops[:1] + list(reversed(loops[1:]))
    nodes = tuple((ids[i], (-x+F(1, 3), -y+F(1, 7)))
                  for i, (x, y) in reversed(case.nodes))
    return ExternalCase(case.name + '/transformed-remapped-reordered', tuple(loops), nodes)
