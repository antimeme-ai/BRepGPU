"""Independent, bounded exact domain families chosen before CDT tuning."""
from fractions import Fraction as F
import math

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


SCALING_FAMILIES = ('convex-parabola', 'protected-strip', 'concave-comb', 'separated-holes',
                    'long-recovery')


def scaling_case(family, count):
    """General exact families; count is a target and actual site count is explicit."""
    if type(count) is not int or count < 8:
        raise ValueError('scaling target must be an integer at least 8')
    if family == 'convex-parabola':
        coordinates = tuple((F(i, count), F(i*i, count*count)) for i in range(count))
        return _case(family, (coordinates,))
    if family == 'protected-strip':
        n = max(2, count//2)
        stations = tuple(F(i) + F(i*i, 64*n*n) for i in range(n))
        loop = tuple((x, 0) for x in stations) + tuple((x, F(1, 16)) for x in reversed(stations))
        return _case(family, (loop,))
    if family == 'concave-comb':
        notches = max(1, (count-4)//4)
        width = 3*notches+1
        loop = [(0, 0), (width, 0), (width, 4)]
        for notch in range(notches):
            x = width-3*notch-1
            loop.extend(((x, 4), (x, 1), (x-1, 1), (x-1, 4)))
        loop.append((0, 4))
        return _case(family, (loop,))
    if family == 'separated-holes':
        side = max(1, math.isqrt(max(1, (count-4)//4)))
        extent = 4*side+2
        loops = [[(0, 0), (extent, 0), (extent, extent), (0, extent)]]
        for i in range(side):
            for j in range(side):
                x, y = 4*i+1, 4*j+1
                loops.append(((x, y), (x, y+1), (x+1, y+1), (x+1, y)))
        return _case(family, loops)
    if family == 'long-recovery':
        n = max(2, (count-6)//2)
        stations = tuple(F(i, n-1) for i in range(n))
        epsilon = F(1, 1024)
        outer = ((F(-1), -epsilon), *tuple((x, -epsilon) for x in stations),
                 (F(2), -epsilon), (F(2), F(1)), (F(-1), F(1)))
        hole = ((F(0), F(0)), *tuple((x, epsilon) for x in stations), (F(1), F(0)))
        return _case(family, (outer, hole))
    raise ValueError('unknown independent scaling family')


def validate_scaling_input(family, case):
    """Prove input simplicity from actual exact coordinates, never a producer flag.

    Original unique-ID/UV/slot admission is also mandatory in qualify_cdt before
    this reader. The family restrictions are oracle preconditions, not product
    admission or native dispatch specializations.
    """
    points = case.points
    loops = tuple(tuple(points[i] for i in word) for word in case.loops)
    def require(condition):
        if not condition:
            raise AssertionError(f'actual source coordinates do not satisfy {family} input contract')
    if family == 'convex-parabola':
        require(len(loops) == 1)
        loop = loops[0]
        require(all(a[0] < b[0] for a, b in zip(loop, loop[1:])))
        require(all(y == x*x for x, y in loop))
    elif family == 'protected-strip':
        require(len(loops) == 1 and len(loops[0]) % 2 == 0)
        n = len(loops[0])//2
        bottom, top = loops[0][:n], loops[0][n:]
        require(bottom[0][1] < top[0][1])
        require(all(y == bottom[0][1] for _, y in bottom))
        require(all(y == top[0][1] for _, y in top))
        require(all(a[0] < b[0] for a, b in zip(bottom, bottom[1:])))
        require(tuple(x for x, _ in bottom) == tuple(x for x, _ in reversed(top)))
    elif family == 'concave-comb':
        require(len(loops) == 1)
        lower, upper = loops[0][:2], loops[0][2:]
        require(len(upper) >= 3)
        left, right = lower
        require(left[0] < right[0] and left[1] == right[1])
        require(upper[0][0] == right[0] and upper[-1][0] == left[0])
        require(upper[1][0] < right[0] and upper[-2][0] > left[0])
        require(all(y > left[1] for _, y in upper))
        require(all(a[0] >= b[0] for a, b in zip(upper, upper[1:])))
        require(all(a[0] > c[0] for a, _, c in zip(upper, upper[1:], upper[2:])))
    elif family == 'separated-holes':
        outer = loops[0]
        require(len(outer) == 4)
        x0, y0 = outer[0]
        x1, y1 = outer[2]
        require(x0 < x1 and y0 < y1)
        require(outer == ((x0, y0), (x1, y0), (x1, y1), (x0, y1)))
        cells = set()
        for hole in loops[1:]:
            require(len(hole) == 4)
            x, y = hole[0]
            require(hole == ((x, y), (x, y+1), (x+1, y+1), (x+1, y)))
            require(x0 < x and x+1 < x1 and y0 < y and y+1 < y1)
            cell = ((x-1)/4, (y-1)/4)
            require(all(value.denominator == 1 for value in cell))
            require(cell not in cells)
            cells.add(cell)
    elif family == 'long-recovery':
        require(len(loops) == 2)
        outer, hole = loops
        require(len(outer) >= 6 and len(hole) >= 4)
        bottom, top = outer[:-2], outer[-2:]
        require(all(a[0] < b[0] for a, b in zip(bottom, bottom[1:])))
        require(all(y == bottom[0][1] for _, y in bottom))
        require(top[0][0] == bottom[-1][0] and top[1][0] == bottom[0][0])
        require(top[0][1] == top[1][1])
        stations = tuple(x for x, _ in bottom[1:-1])
        a, b = hole[0], hole[-1]
        upper = hole[1:-1]
        require(a[1] == b[1] and a[0] == stations[0] and b[0] == stations[-1])
        require(tuple(x for x, _ in upper) == stations)
        require(all(y == upper[0][1] for _, y in upper))
        require(bottom[0][1] < a[1] < upper[0][1] < top[0][1])
    else:
        raise ValueError('unknown independently checked input family')
    return True
