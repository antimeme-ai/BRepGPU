"""Complete exact sections of canonical closed planar/cylindrical faces.

Construction is CPU work. Immutable records expose spatial and both original
UV maps; representation occurrences are distinct from physical graph spans.
Python construction/conversion assumes round-to-nearest and gradual underflow.
"""
from dataclasses import dataclass
from fractions import Fraction
from functools import total_ordering
import math
import operator
import struct

from .body import BodySnapshot


class UnsupportedIntersection(ValueError):
    pass


def _sgn(value):
    return (value > 0) - (value < 0)


def _sign_one(a, b, d):
    if not b or not d:
        return _sgn(a)
    sa, sb = _sgn(a), _sgn(b)
    if not sa:
        return sb
    if sa == sb:
        return sa
    delta = a*a - b*b*d
    return sa if delta > 0 else sb if delta < 0 else 0


def _sign_two(k, u, d, v, f):
    z, w = _sign_one(k, u, d), _sgn(v) if f else 0
    if not z:
        return w
    if not w or z == w:
        return z
    magnitude = _sign_one(k*k + u*u*d - v*v*f, 2*k*u, d)
    return z if magnitude > 0 else w if magnitude < 0 else 0


@total_ordering
@dataclass(frozen=True, slots=True, eq=False)
class Scalar:
    """a+b*sqrt(d), exact and deliberately unhashable."""
    a: Fraction = Fraction(0)
    b: Fraction = Fraction(0)
    d: int = 0
    __hash__ = None

    def __post_init__(self):
        if isinstance(self.a, Scalar):
            if self.b or self.d:
                raise TypeError("Scalar coercion cannot add another radical")
            a, b, d = self.a.a, self.a.b, self.a.d
        else:
            a, b, d = Fraction(self.a), Fraction(self.b), operator.index(self.d)
        if d < 0:
            raise ValueError("negative scalar radicand")
        root = math.isqrt(d)
        if not b or root*root == d:
            a += b*root
            b, d = Fraction(0), 0
        object.__setattr__(self, "a", a)
        object.__setattr__(self, "b", b)
        object.__setattr__(self, "d", d)

    @staticmethod
    def sqrt(value):
        value = Fraction(value)
        if value < 0:
            raise ValueError("negative exact square root")
        return Scalar(0, Fraction(1, value.denominator), value.numerator*value.denominator)

    def _compare(self, other):
        other = Scalar(other)
        return _sign_two(self.a-other.a, self.b, self.d, -other.b, other.d)

    def __eq__(self, other):
        try:
            return self._compare(other) == 0
        except (TypeError, ValueError, OverflowError):
            return NotImplemented

    def __lt__(self, other):
        return self._compare(other) < 0

    def __bool__(self):
        return _sign_one(self.a, self.b, self.d) != 0

    def __neg__(self):
        return Scalar(-self.a, -self.b, self.d)

    def __abs__(self):
        return -self if self < 0 else self

    def _field(self, other):
        other = Scalar(other)
        if not self.b:
            return self.a, Fraction(0), other.a, other.b, other.d
        if not other.b:
            return self.a, self.b, other.a, Fraction(0), self.d
        if self.d == other.d:
            return self.a, self.b, other.a, other.b, self.d
        # Recognize rationally related radicands without factoring either.
        common = math.gcd(self.d, other.d)
        x, y = self.d//common, other.d//common
        sx, sy = math.isqrt(x), math.isqrt(y)
        if sx*sx != x or sy*sy != y:
            raise ValueError("mixed radical-field arithmetic is unsupported")
        return self.a, self.b, other.a, other.b*Fraction(sy, sx), self.d

    def __add__(self, other):
        a, b, c, e, d = self._field(other)
        return Scalar(a+c, b+e, d)

    __radd__ = __add__

    def __sub__(self, other):
        return self + -Scalar(other)

    def __rsub__(self, other):
        return Scalar(other) - self

    def __mul__(self, other):
        a, b, c, e, d = self._field(other)
        return Scalar(a*c+b*e*d, a*e+b*c, d)

    __rmul__ = __mul__

    def inverse(self):
        denominator = self.a*self.a - self.b*self.b*self.d
        if not denominator:
            raise ZeroDivisionError("zero exact scalar")
        return Scalar(self.a/denominator, -self.b/denominator, self.d)

    def __truediv__(self, other):
        return self * Scalar(other).inverse()

    def __rtruediv__(self, other):
        return Scalar(other) * self.inverse()

    def __pow__(self, exponent):
        exponent = operator.index(exponent)
        if exponent < 0:
            return self.inverse() ** -exponent
        answer, factor = Scalar(1), self
        while exponent:
            if exponent & 1:
                answer = answer*factor
            factor = factor*factor
            exponent >>= 1
        return answer

    def bracket(self):
        """Adjacent finite binary64 values enclosing this exact number."""
        if self < 0:
            lo, hi = (-self).bracket()
            return -hi, -lo
        maximum = 0x7fefffffffffffff
        def number(bits):
            return struct.unpack("<d", struct.pack("<Q", bits))[0]
        if self > Fraction.from_float(number(maximum)):
            raise OverflowError("exact scalar exceeds finite binary64")
        if not self.b:
            # Exact integer quantization, including gradual subnormal spacing.
            # Numeric quantization is integer; float decoding assumes the
            # declared round-to-nearest/gradual-underflow environment.
            numerator, denominator = self.a.numerator, self.a.denominator
            if not numerator:
                return 0., 0.
            exponent = numerator.bit_length()-denominator.bit_length()
            below_power = (numerator < (denominator << exponent)) if exponent >= 0 \
                          else ((numerator << -exponent) < denominator)
            if below_power:
                exponent -= 1
            shift = exponent-52 if exponent >= -1022 else -1074
            if shift < 0:
                scaled_numerator, scaled_denominator = numerator << -shift, denominator
            else:
                scaled_numerator, scaled_denominator = numerator, denominator << shift
            significand, remainder = divmod(scaled_numerator, scaled_denominator)
            bits = (((exponent+1023) << 52) + significand-(1 << 52)) \
                   if exponent >= -1022 else significand
            lower = number(bits)
            return (lower, number(bits+1)) if remainder else (lower, lower)
        lo, hi = 0, maximum
        while lo < hi:
            mid = (lo+hi+1)//2
            if self >= Fraction.from_float(number(mid)):
                lo = mid
            else:
                hi = mid-1
        lower = number(lo)
        if self == Fraction.from_float(lower):
            return lower, lower
        return lower, number(lo+1)

    def __float__(self):
        lo, hi = self.bracket()
        if lo == hi:
            return lo
        midpoint = (Fraction.from_float(lo)+Fraction.from_float(hi))/2
        sign = self._compare(midpoint)
        if sign < 0:
            return lo
        if sign > 0:
            return hi
        bits = struct.unpack("<Q", struct.pack("<d", lo))[0]
        return lo if not bits & 1 else hi


def _evaluate(coefficients, parameter):
    parameter = Scalar(parameter)
    answer = Scalar(coefficients[-1])
    for coefficient in reversed(coefficients[:-1]):
        answer = answer*parameter + coefficient
    return answer


@dataclass(frozen=True, slots=True)
class ExactMap:
    numerators: tuple
    denominator: tuple = (Scalar(1),)

    def __post_init__(self):
        numerators = tuple(tuple(Scalar(c) for c in row) for row in self.numerators)
        denominator = tuple(Scalar(c) for c in self.denominator)
        if not numerators or any(not row for row in numerators) or not denominator:
            raise ValueError("exact maps need nonempty polynomial records")
        object.__setattr__(self, "numerators", numerators)
        object.__setattr__(self, "denominator", denominator)

    def evaluate(self, parameter):
        denominator = _evaluate(self.denominator, parameter)
        if denominator <= 0:
            raise ArithmeticError("exact map denominator is not positive")
        return tuple(_evaluate(row, parameter)/denominator for row in self.numerators)


@dataclass(frozen=True, slots=True)
class FaceRef:
    snapshot: BodySnapshot
    face_id: int

    def __post_init__(self):
        if type(self.snapshot) is not BodySnapshot:
            raise TypeError("FaceRef needs an exact owned BodySnapshot")
        if isinstance(self.face_id, bool):
            raise TypeError("face ID must be an integer")
        face_id = operator.index(self.face_id)
        if not 1 <= face_id <= len(self.snapshot.faces) or self.snapshot.faces[face_id-1].id != face_id:
            raise ValueError("face ID outside snapshot")
        object.__setattr__(self, "face_id", face_id)

    @property
    def face(self):
        return self.snapshot.faces[self.face_id-1]


@dataclass(frozen=True, slots=True)
class Component:
    id: int
    kind: str
    closed: bool
    tangent: bool
    piece_ids: tuple


@dataclass(frozen=True, slots=True)
class Piece:
    id: int
    component_id: int
    span_id: int
    kind: str
    left_slot: int
    right_slot: int
    lo: Scalar
    hi: Scalar
    spatial_map: ExactMap
    left_map: ExactMap
    right_map: ExactMap
    boundary_constraints: tuple

    def _parameter(self, parameter):
        parameter = Scalar(parameter)
        if not self.lo <= parameter <= self.hi:
            raise ValueError("parameter outside exact piece")
        return parameter

    def position(self, parameter):
        return self.spatial_map.evaluate(self._parameter(parameter))

    def left_uv(self, parameter):
        return self.left_map.evaluate(self._parameter(parameter))

    def right_uv(self, parameter):
        return self.right_map.evaluate(self._parameter(parameter))


@dataclass(frozen=True, slots=True)
class Event:
    id: int
    constraints: tuple
    joins: tuple
    incidences: tuple


@dataclass(frozen=True, slots=True, init=False)
class IntersectionSet:
    left: FaceRef
    right: FaceRef
    components: tuple
    pieces: tuple
    events: tuple

    def __init__(self, *args, **kwargs):
        raise TypeError("IntersectionSet construction is factory-only")


def _fraction(value):
    return Fraction.from_float(float(value))


def _plane(ref):
    patch = ref.snapshot.patches[ref.face.chart_slots[0]]
    controls = tuple(tuple(_fraction(x) for x in patch.controls[i:i+4])
                     for i in range(0, len(patch.controls), 4))
    if (patch.degree_u, patch.degree_v) != (1, 1) or len(controls) != 4 or any(c[3] != 1 for c in controls):
        raise UnsupportedIntersection("unsupported canonical plane carrier")
    scale = _fraction(ref.snapshot.frame.scale)
    origin = tuple(_fraction(o)+scale*c for o, c in zip(ref.snapshot.frame.origin, controls[0][:3]))
    u = tuple(scale*(controls[2][a]-controls[0][a]) for a in range(3))
    v = tuple(scale*(controls[1][a]-controls[0][a]) for a in range(3))
    ua, va = tuple(i for i in range(3) if u[i]), tuple(i for i in range(3) if v[i])
    if len(ua) != 1 or len(va) != 1 or ua == va:
        raise UnsupportedIntersection("plane is not axis aligned")
    axis = next(i for i in range(3) if i not in (ua[0], va[0]))
    return origin, u, v, ua[0], va[0], axis


def _plane_map(spatial, geometry):
    origin, u, v, ua, va, _ = geometry
    rows = []
    for axis, span in ((ua, u[ua]), (va, v[va])):
        n, d = spatial.numerators[axis], spatial.denominator
        count = max(len(n), len(d))
        rows.append(tuple(((n[i] if i < len(n) else Scalar(0))
                          - origin[axis]*(d[i] if i < len(d) else Scalar(0)))/span
                          for i in range(count)))
    return ExactMap(tuple(rows), spatial.denominator)


def _rotate(x, y, q):
    return ((x, y), (tuple(-a for a in y), x),
            (tuple(-a for a in x), tuple(-a for a in y)),
            (y, tuple(-a for a in x)))[q]


def _circle(q, cx, cy, height, radius):
    x, y = _rotate((1, 0, -1), (0, 2, 0), q)
    d = (1, 0, 1)
    return ExactMap((tuple(cx*d[i]+radius*x[i] for i in range(3)),
                     tuple(cy*d[i]+radius*y[i] for i in range(3)),
                     tuple(height*a for a in d)), d)


def _rational_polynomial(values):
    values = tuple(Scalar(value) for value in values)
    if any(value.b for value in values):
        raise ArithmeticError("trim coefficients must be rational")
    return tuple(value.a for value in values)


def _roots(coefficients):
    c = list(map(Fraction, coefficients))
    while len(c) > 1 and not c[-1]:
        c.pop()
    if len(c) <= 1:
        return ()
    if len(c) == 2:
        return (Scalar(-c[0]/c[1]),)
    if len(c) != 3:
        raise ArithmeticError("unsupported trim polynomial degree")
    denominator = math.lcm(*(a.denominator for a in c))
    integers = [int(a*denominator) for a in c]
    divisor = math.gcd(*integers)
    cc, bb, aa = (a//divisor for a in integers)
    if aa < 0:
        cc, bb, aa = -cc, -bb, -aa
    discriminant = bb*bb-4*aa*cc
    if discriminant < 0:
        return ()
    if not discriminant:
        return (Scalar(Fraction(-bb, 2*aa)),)
    return (Scalar(Fraction(-bb, 2*aa), Fraction(-1, 2*aa), discriminant),
            Scalar(Fraction(-bb, 2*aa), Fraction(1, 2*aa), discriminant))


def _witness(lo, hi):
    lower, upper = Fraction(0), Fraction(1)
    while True:
        mid = (lower+upper)/2
        if lo < mid < hi:
            return mid
        if mid <= lo:
            lower = mid
        else:
            upper = mid


def _constraints(plane_ref, plane_uv, q, cx, cy, radius, side):
    kind = plane_ref.face.kind
    answer = []
    if kind in ("rectangle", "rectangle_hole"):
        d = _rational_polynomial(plane_uv.denominator)
        for coordinate, name in zip(plane_uv.numerators, ("u", "v")):
            n = _rational_polynomial(coordinate)
            count = max(len(n), len(d))
            n = n+(Fraction(0),)*(count-len(n));den = d+(Fraction(0),)*(count-len(d))
            answer.extend(((side+":"+name+"0", n),
                           (side+":"+name+"1", tuple(a-b for a, b in zip(den, n)))))
    if kind in ("disk", "rectangle_hole"):
        frame = plane_ref.snapshot.frame
        dx, dy = cx-_fraction(frame.origin[0]), cy-_fraction(frame.origin[1])
        trim_radius = _fraction(frame.scale)/4
        constant = dx*dx+dy*dy+radius*radius-trim_radius*trim_radius
        x, y = _rotate((1, 0, -1), (0, 2, 0), q)
        polynomial = tuple(constant*a + 2*radius*(dx*b+dy*c)
                           for a, b, c in zip((1, 0, 1), x, y))
        if kind == "disk":
            answer.append((side+":rim", tuple(-a for a in polynomial)))
        else:
            answer.append((side+":hole", polynomial))
    return tuple(answer)


class _Graph:
    def __init__(self, left, right, horizontal, tangent=False):
        self.left, self.right = left, right
        self.horizontal, self.tangent = horizontal, tangent
        self.nodes, self.pieces, self.next_span = {}, [], 0

    def node(self, key, constraints=(), joins=()):
        node = self.nodes.setdefault(key, dict(constraints=set(), joins=set(), incidences=[]))
        node["constraints"].update(constraints)
        node["joins"].update(joins)
        return key

    def span(self):
        span = self.next_span
        self.next_span += 1
        return span

    def piece(self, kind, span, lo, hi, spatial, plane_uv, wall_uv,
              plane_slot, wall_slot, plane_left, boundary, first, last):
        row = dict(id=len(self.pieces), span_id=span, kind=kind, lo=Scalar(lo), hi=Scalar(hi),
                   spatial_map=spatial, left_map=plane_uv if plane_left else wall_uv,
                   right_map=wall_uv if plane_left else plane_uv,
                   left_slot=plane_slot if plane_left else wall_slot,
                   right_slot=wall_slot if plane_left else plane_slot,
                   boundary_constraints=tuple(sorted(boundary)), first=first, last=last)
        self.pieces.append(row)
        if kind == "point":
            self.nodes[first]["incidences"].append((row["id"], "point"))
        else:
            self.nodes[first]["incidences"].append((row["id"], "lo"))
            self.nodes[last]["incidences"].append((row["id"], "hi"))

    def finish(self):
        active = {key: value for key, value in self.nodes.items() if value["incidences"]}
        event_ids = {key: i for i, key in enumerate(active)}
        spans = {}
        for piece in self.pieces:
            span = spans.setdefault(piece["span_id"], dict(first=piece["first"], last=piece["last"],
                        point=piece["kind"] == "point", pieces=[]))
            if (span["first"], span["last"], span["point"]) != \
                    (piece["first"], piece["last"], piece["kind"] == "point"):
                raise ArithmeticError("chart occurrences disagree on physical span")
            span["pieces"].append(piece["id"])
        adjacency = {key: [] for key in active}
        for sid, span in spans.items():
            adjacency[span["first"]].append(sid)
            if not span["point"]:
                adjacency[span["last"]].append(sid)
        todo, components, assigned = set(active), [], {}
        while todo:
            seed = min(todo, key=event_ids.get)
            nodes, physical = set(), set();pending = [seed]
            while pending:
                key = pending.pop()
                if key in nodes:
                    continue
                nodes.add(key);todo.discard(key)
                for sid in adjacency[key]:
                    physical.add(sid)
                    span = spans[sid]
                    if not span["point"]:
                        pending.extend((span["first"], span["last"]))
            curves = {sid for sid in physical if not spans[sid]["point"]}
            closed = bool(curves) and all(sum(not spans[sid]["point"] for sid in adjacency[key]) == 2
                                         for key in nodes)
            if curves:
                starts = [sid for sid in curves if not any(
                    spans[other]["last"] == spans[sid]["first"] for other in curves)]
                current = min(starts or curves)
                order, seen = [], set()
                while current not in seen:
                    seen.add(current);order.extend(spans[current]["pieces"])
                    ends = [sid for sid in curves if spans[sid]["first"] == spans[current]["last"]]
                    if not ends:
                        break
                    if len(ends) != 1:
                        raise ArithmeticError("supported intersection graph branches unexpectedly")
                    current = ends[0]
                if seen != curves:
                    raise ArithmeticError("supported graph traversal missed a span")
                kind = ("circle" if closed else "arc") if self.horizontal else "segment"
            else:
                order = [pid for sid in sorted(physical) for pid in spans[sid]["pieces"]]
                kind = "point"
            cid = len(components)
            for pid in order:
                assigned[pid] = cid
            components.append(Component(cid, kind, closed, self.tangent, tuple(order)))
        pieces = tuple(Piece(row["id"], assigned[row["id"]], row["span_id"], row["kind"],
                  row["left_slot"], row["right_slot"], row["lo"], row["hi"], row["spatial_map"],
                  row["left_map"], row["right_map"], row["boundary_constraints"]) for row in self.pieces)
        events = tuple(Event(event_ids[key], tuple(sorted(node["constraints"])),
                            tuple(sorted(node["joins"])), tuple(node["incidences"]))
                       for key, node in active.items())
        result = object.__new__(IntersectionSet)
        for name, value in (("left", self.left), ("right", self.right),
                            ("components", tuple(components)), ("pieces", pieces), ("events", events)):
            object.__setattr__(result, name, value)
        return result


def _horizontal(graph, plane_ref, wall_ref, geometry, plane_left, cx, cy, radius, z0, z1):
    height = geometry[0][2]
    if not z0 <= height <= z1:
        return
    plane_side, wall_side = ("left", "right") if plane_left else ("right", "left")
    axial = tuple(wall_side+":"+name for name, value in (("z0", z0), ("z1", z1)) if height == value)
    v = (height-z0)/(z1-z0)
    point_occurrences = {}
    for wall_slot in wall_ref.face.chart_slots:
        q = wall_ref.snapshot.charts[wall_slot].quadrant
        spatial = _circle(q, cx, cy, height, radius)
        plane_uv = _plane_map(spatial, geometry)
        wall_uv = ExactMap(((0, 1), (v,)))
        constraints = _constraints(plane_ref, plane_uv, q, cx, cy, radius, plane_side)
        values = [Scalar(0), Scalar(1)]
        for _, polynomial in constraints:
            values.extend(root for root in _roots(polynomial) if 0 <= root <= 1)
        values.sort();roots = []
        for value in values:
            if not roots or value != roots[-1]:
                roots.append(value)
        admitted = [all(_evaluate(polynomial, value) >= 0 for _, polynomial in constraints) for value in roots]
        keys = []
        for index, value in enumerate(roots):
            if value == 0 or value == 1:
                atlas = (q + int(value == 1)) % 4
                key = ("join", atlas)
                joins = (wall_side+":atlas:"+("0/4" if atlas == 0 else str(atlas)),)
            else:
                key, joins = ("root", q, index), ()
            keys.append(key)
            if admitted[index]:
                names = axial+tuple(name for name, polynomial in constraints if not _evaluate(polynomial, value))
                graph.node(key, names, joins)
                point_occurrences.setdefault(key, []).append((q, wall_slot, value, spatial, plane_uv, wall_uv))
        boundary = axial+tuple(name for name, polynomial in constraints if all(not a for a in polynomial))
        for index, (lo, hi) in enumerate(zip(roots, roots[1:])):
            witness = _witness(lo, hi)
            if all(_evaluate(polynomial, witness) >= 0 for _, polynomial in constraints):
                if not admitted[index] or not admitted[index+1]:
                    raise ArithmeticError("closed admitted interval lost an endpoint")
                graph.piece("arc", graph.span(), lo, hi, spatial, plane_uv, wall_uv,
                    plane_ref.face.chart_slots[0], wall_slot, plane_left, boundary, keys[index], keys[index+1])
    for key, occurrences in point_occurrences.items():
        if graph.nodes[key]["incidences"]:
            continue
        span = graph.span()
        for _, wall_slot, value, spatial, plane_uv, wall_uv in occurrences:
            graph.piece("point", span, value, value, spatial, plane_uv, wall_uv,
                plane_ref.face.chart_slots[0], wall_slot, plane_left,
                tuple(graph.nodes[key]["constraints"]), key, key)


def _vertical(graph, plane_ref, wall_ref, geometry, plane_left, cx, cy, radius, z0, z1):
    origin, u, v, ua, va, axis = geometry
    coordinate = origin[axis]
    center = cx if axis == 0 else cy
    delta = coordinate-center
    discriminant = radius*radius-delta*delta
    if discriminant < 0:
        return
    graph.tangent = not discriminant
    root = Scalar.sqrt(discriminant)
    branches = (root,) if not root else (-root, root)
    plane_side, wall_side = ("left", "right") if plane_left else ("right", "left")
    bounds = {}
    for a, span, name in ((ua, u[ua], "u"), (va, v[va], "v")):
        low, high = origin[a], origin[a]+span
        if low > high:
            low, high = high, low
            low_tag, high_tag = name+"1", name+"0"
        else:
            low_tag, high_tag = name+"0", name+"1"
        bounds[a] = (low, high, plane_side+":"+low_tag, plane_side+":"+high_tag)
    lo = max(z0, bounds[2][0]);hi = min(z1, bounds[2][1])
    if lo > hi:
        return
    lateral = 1-axis
    for branch, offset in enumerate(branches):
        x, y = (Scalar(coordinate), Scalar(cy)+offset) if axis == 0 else (Scalar(cx)+offset, Scalar(coordinate))
        fixed = y if lateral == 1 else x
        low, high, low_name, high_name = bounds[lateral]
        if not low <= fixed <= high:
            continue
        boundary = tuple(name for value, name in ((low, low_name), (high, high_name)) if fixed == value)
        names = []
        for height in (lo, hi):
            names.append(tuple(name for value, name in ((z0, wall_side+":z0"), (z1, wall_side+":z1"),
                (bounds[2][0], bounds[2][2]), (bounds[2][1], bounds[2][3])) if height == value)+boundary)
        first = graph.node(("line", branch, "point" if lo == hi else "lo"), names[0])
        last = first if lo == hi else graph.node(("line", branch, "hi"), names[1])
        if lo == hi:
            graph.node(first, names[1])
        spatial = ExactMap(((x,), (y,), (0, 1)))
        plane_uv = _plane_map(spatial, geometry)
        span = graph.span();occurrences = 0
        for wall_slot in wall_ref.face.chart_slots:
            q = wall_ref.snapshot.charts[wall_slot].quadrant
            # Rotate the generator point back to the nonnegative first quadrant.
            local_x, local_y = x-cx, y-cy
            px, py = ((local_x, local_y), (local_y, -local_x),
                      (-local_x, -local_y), (-local_y, local_x))[q]
            if px < 0 or py < 0:
                continue
            s = py/(radius+px)
            if not 0 <= s <= 1:
                continue
            joins = ()
            if s == 0 or s == 1:
                atlas = (q+int(s == 1))%4
                joins = (wall_side+":atlas:"+("0/4" if atlas == 0 else str(atlas)),)
            graph.node(first, joins=joins);graph.node(last, joins=joins)
            wall_uv = ExactMap(((s,), (-z0/(z1-z0), 1/(z1-z0))))
            graph.piece("point" if lo == hi else "segment", span, lo, hi, spatial, plane_uv, wall_uv,
                plane_ref.face.chart_slots[0], wall_slot, plane_left,
                tuple(graph.nodes[first]["constraints"]) if lo == hi else boundary, first, last)
            occurrences += 1
        if not occurrences:
            raise ArithmeticError("exact generator has no wall chart occurrence")


def intersect_faces(left, right):
    """Return every component of one supported plane/wall closed face pair."""
    if type(left) is not FaceRef or type(right) is not FaceRef:
        raise TypeError("intersect_faces requires FaceRef operands")
    planar = ("rectangle", "disk", "rectangle_hole")
    if left.face.kind in planar and right.face.kind == "cylinder":
        plane_ref, wall_ref, plane_left = left, right, True
    elif right.face.kind in planar and left.face.kind == "cylinder":
        plane_ref, wall_ref, plane_left = right, left, False
    else:
        raise UnsupportedIntersection("exactly one canonical planar face and one wall required")
    geometry = _plane(plane_ref)
    axis = geometry[-1]
    if axis != 2 and plane_ref.face.kind != "rectangle":
        raise UnsupportedIntersection("vertical supported planes must have rectangular trims")
    frame = wall_ref.snapshot.frame
    cx, cy, oz = map(_fraction, frame.origin)
    scale = _fraction(frame.scale)
    radius, z0, z1 = scale/4, oz-scale/2, oz+scale/2
    graph = _Graph(left, right, axis == 2)
    if axis == 2:
        _horizontal(graph, plane_ref, wall_ref, geometry, plane_left, cx, cy, radius, z0, z1)
    else:
        _vertical(graph, plane_ref, wall_ref, geometry, plane_left, cx, cy, radius, z0, z1)
    return graph.finish()
