"""Independent original-carrier and complete imported-domain answers.

Production triangulation, span extraction and native raster helpers are not
used here. Exact predicates use saved binary64 values and owned rational UV.
"""
from collections import Counter, defaultdict
from fractions import Fraction as F
from dataclasses import dataclass
from bisect import bisect_left, bisect_right
from heapq import heappop, heappush
import hashlib
import math
from pathlib import Path
import re
import struct

from .interchange_oracles import _basis, _bernstein, _quotient


class AdmissionVerdict(AssertionError):
    """A verdict of an independent check on the data it reads (never an error in the
    check's own code): physical admission scores it against the mesh's producer."""


def require(condition, message):
    if not condition:
        raise AdmissionVerdict(message)


def exact(value):
    if type(value) is F:
        return value
    if type(value) is int:
        return F(value)
    if type(value) is float and math.isfinite(value):
        return F.from_float(value)
    raise AssertionError('independent geometry requires finite numeric values')


def _expanded(knots, multiplicities):
    require(len(knots) == len(multiplicities), 'source knot/multiplicity extent')
    require(all(type(m) is int and m > 0 for m in multiplicities), 'source multiplicity')
    require(all(exact(a) < exact(b) for a, b in zip(knots, knots[1:])), 'source distinct knot order')
    return tuple(exact(k) for k, m in zip(knots, multiplicities) for _ in range(m))


def _frame_point(frame, coordinates):
    return tuple(frame[k] + sum(frame[3 + 3 * j + k] * coordinates[j] for j in range(3))
                 for k in range(3))


def _frame_vector(frame, coordinates):
    return tuple(sum(frame[3 + 3 * j + k] * coordinates[j] for j in range(3))
                 for k in range(3))


def carrier_jet(carrier, u, v):
    """Original-knot/original-axis S,SU,SV, independent of extracted spans."""
    kind = carrier.kind
    if kind == 6:
        nu, nv = carrier.control_shape
        pu, pv = carrier.degrees
        ku = _expanded(carrier.knot_u, carrier.mult_u)
        kv = _expanded(carrier.knot_v, carrier.mult_v)
        bu, du = _basis(ku, nu, pu, u)
        bv, dv = _basis(kv, nv, pv, v)
        controls = tuple(tuple(map(exact, carrier.controls[i:i + 4]))
                         for i in range(0, len(carrier.controls), 4))
        require(len(controls) == nu * nv, 'original source surface control extent')
        return _quotient(controls, bu, bv, du, dv)
    require(kind in (1, 2, 3, 4, 5), 'unsupported original source carrier')
    frame = carrier.frame
    require(len(frame) == 12 and all(math.isfinite(x) for x in frame), 'finite original carrier frame')
    u, v = float(u), float(v)
    r0, r1 = carrier.radii
    if kind == 1:
        position, du, dv = (u, v, 0.), (1., 0., 0.), (0., 1., 0.)
    else:
        c, s = math.cos(u), math.sin(u)
        if kind == 2:
            position, du, dv = (r0*c, r0*s, v), (-r0*s, r0*c, 0.), (0., 0., 1.)
        elif kind == 3:
            sa, ca = math.sin(r1), math.cos(r1)
            radius = r0 + v * sa
            position = (radius*c, radius*s, v*ca)
            du, dv = (-radius*s, radius*c, 0.), (sa*c, sa*s, ca)
        elif kind == 4:
            cv, sv = math.cos(v), math.sin(v)
            position = (r0*cv*c, r0*cv*s, r0*sv)
            du, dv = (-r0*cv*s, r0*cv*c, 0.), (-r0*sv*c, -r0*sv*s, r0*cv)
        else:
            cv, sv = math.cos(v), math.sin(v)
            radius = r0 + r1 * cv
            position = (radius*c, radius*s, r1*sv)
            du, dv = (-radius*s, radius*c, 0.), (-r1*sv*c, -r1*sv*s, r1*cv)
    return _frame_point(frame, position), _frame_vector(frame, du), _frame_vector(frame, dv)


_SPLINE_INPUTS = {}


def _float_inputs(record, surface):
    key = (id(record), surface)
    if key not in _SPLINE_INPUTS:
        if surface:
            ku = tuple(k for k, m in zip(record.knot_u, record.mult_u) for _ in range(m))
            kv = tuple(k for k, m in zip(record.knot_v, record.mult_v) for _ in range(m))
            value = (ku, kv)
        else:
            value = tuple(k for k, m in zip(record.knots, record.multiplicities) for _ in range(m))
        _SPLINE_INPUTS[key] = (record, value)
    return _SPLINE_INPUTS[key][1]


def _local_basis(knots, count, degree, parameter, side=None):
    """Independent local-support Cox recurrence; exact tests use _basis above."""
    t = float(parameter)
    require(knots[degree] <= t <= knots[count], 'original float source parameter outside domain')
    i = (bisect_left(knots, t) if side == 'left' else bisect_right(knots, t)) - 1
    i = min(count-1, max(degree, i))
    def values(p):
        result = [1.]
        for j in range(1, p+1):
            new, carried = [], 0.
            for r in range(j):
                left, right = t-knots[i+1-j+r], knots[i+r+1]-t
                denominator = right+left
                ratio = result[r]/denominator if denominator else 0.
                new.append(carried+right*ratio)
                carried = left*ratio
            result = new+[carried]
        return {i-p+j: value for j, value in enumerate(result)}
    b = values(degree)
    lower = values(degree-1) if degree else {}
    derivative = {}
    for index in b:
        a = knots[index+degree]-knots[index]
        z = knots[index+degree+1]-knots[index+1]
        derivative[index] = ((degree*lower.get(index, 0.)/a if a else 0.)-
                             (degree*lower.get(index+1, 0.)/z if z else 0.))
    return b, derivative


def carrier_jet_float(carrier, u, v, *, side_u=None, side_v=None):
    """Practical independent original-knot answers for all actual mesh corners."""
    if carrier.kind != 6:
        return carrier_jet(carrier, u, v)
    ku, kv = _float_inputs(carrier, True)
    nu, nv = carrier.control_shape
    pu, pv = carrier.degrees
    bu, du = _local_basis(ku, nu, pu, u, side_u)
    bv, dv = _local_basis(kv, nv, pv, v, side_v)
    sums = [[0.]*4 for _ in range(3)]
    for i in bu:
        for j in bv:
            offset = 4*(i*nv+j)
            for term, coefficient in enumerate((bu[i]*bv[j], du[i]*bv[j], bu[i]*dv[j])):
                for axis in range(4):
                    sums[term][axis] += coefficient*carrier.controls[offset+axis]
    a, au, av = sums
    w = a[3]
    require(math.isfinite(w) and w > 0, 'finite positive original source denominator')
    return (tuple(a[k]/w for k in range(3)),
            tuple((au[k]-a[k]*au[3]/w)/w for k in range(3)),
            tuple((av[k]-a[k]*av[3]/w)/w for k in range(3)))


def curve_point_float(curve, parameter):
    if curve.kind != 5:
        return tuple(map(float, curve_point(curve, parameter)))
    count = len(curve.controls)//(curve.dimension+1)
    basis, _ = _local_basis(_float_inputs(curve, False), count, curve.degree, parameter)
    values = tuple(sum(value*curve.controls[i*(curve.dimension+1)+axis] for i, value in basis.items())
                   for axis in range(curve.dimension+1))
    require(values[-1] > 0 and math.isfinite(values[-1]), 'positive actual source curve denominator')
    return tuple(x/values[-1] for x in values[:-1])


def native_uv_rounding_bound(carrier, uv):
    """Independent bound for explicitly rounding owned dyadic UV to binary64.

    The rare spline case compares exact original rational endpoint positions.
    Analytic cases use first-derivative norm bounds, including a conservative
    norm for the original frame. This says nothing about a rounded UV partition.
    """
    rounded = tuple(map(float, uv))
    gaps = tuple(abs(exact(a)-exact(b)) for a,b in zip(uv,rounded))
    if not any(gaps):
        return 0.
    if carrier.kind == 6:
        a = carrier_jet(carrier,*uv)[0]
        b = carrier_jet(carrier,*rounded)[0]
        square = sum(((exact(x)-exact(y))**2 for x,y in zip(a,b)),F(0))
        # Enclose both the rational-to-double and square-root roundings.
        return math.nextafter(math.sqrt(math.nextafter(float(square),math.inf)),math.inf)
    r0,r1 = carrier.radii
    if carrier.kind == 1:
        mu,mv = 1.,1.
    elif carrier.kind == 2:
        mu,mv = abs(r0),1.
    elif carrier.kind == 3:
        # |sin(angle)|<=1 avoids depending on a rounded transcendental bound.
        mu,mv = abs(exact(r0))+max(abs(exact(uv[1])),abs(exact(rounded[1]))),1.
    elif carrier.kind == 4:
        mu,mv = abs(r0),abs(r0)
    else:
        require(carrier.kind == 5, 'unsupported analytic native UV rounding')
        mu,mv = abs(exact(r0))+abs(exact(r1)),abs(r1)
    # Frobenius norm bounds the linear frame's operator norm. All calculations
    # up to sqrt are rational so a tiny UV displacement cannot disappear.
    frame_square = sum((exact(x)**2 for x in carrier.frame[3:]),F(0))
    local = exact(mu)*gaps[0]+exact(mv)*gaps[1]
    square = local*local*frame_square
    return math.nextafter(math.sqrt(math.nextafter(float(square),math.inf)),math.inf)


def curve_point(curve, parameter):
    """Original spatial/recovered curve evaluation with arbitrary source degree."""
    kind, dimension = curve.kind, curve.dimension
    require(dimension in (2, 3), 'original curve dimension')
    if kind in (4, 5):
        count = len(curve.controls) // (dimension + 1)
        require(count * (dimension + 1) == len(curve.controls), 'homogeneous curve extent')
        if kind == 5:
            basis, _ = _basis(_expanded(curve.knots, curve.multiplicities),
                              count, curve.degree, parameter)
        else:
            lo, hi = map(exact, curve.parameter_range)
            require((lo, hi) == (F(0),F(1)), 'original Bezier full carrier must retain natural [0,1] domain')
            basis, _ = _bernstein(curve.degree, (exact(parameter) - lo) / (hi - lo))
            require(len(basis) == count, 'Bezier curve degree/count')
        values = tuple(sum((basis[i] * exact(curve.controls[i * (dimension + 1) + k])
                            for i in range(count)), F(0)) for k in range(dimension + 1))
        require(values[-1] > 0, 'positive original curve weight')
        return tuple(x / values[-1] for x in values[:-1])
    require(kind in (1, 2, 3), 'unsupported original source curve')
    t = float(parameter)
    if kind == 1:
        local = (t, 0., 0.)
    else:
        r0, r1 = curve.radii
        local = (r0 * math.cos(t), (r0 if kind == 2 else r1) * math.sin(t), 0.)
    return _frame_point(curve.frame, local)[:dimension]


def cross(a, b):
    return (a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0])


def original_inventory(path):
    """Independent original entity-ID inventories; ignores private header fields."""
    data = Path(path).read_bytes()
    records = defaultdict(set)
    for match in re.finditer(rb'(?m)^\s*#(\d+)\s*=\s*([A-Z][A-Z0-9_]*)\s*\(', data):
        records[match[2].decode('ascii')].add(int(match[1]))
    return hashlib.sha256(data).hexdigest(), dict(records)


@dataclass(frozen=True)
class _Ref:
    id: int


@dataclass(frozen=True)
class _Call:
    kind: str
    args: tuple


def _step_text(text):
    """Decode the original file's explicit STEP string escape forms."""
    # Physical line wrapping is discarded; explicit \N\ and \T\ directives
    # retain their semantic control characters.
    text = text.replace('\r','').replace('\n','')
    def wide(match):
        width, digits = match[1], match[2]
        return bytes.fromhex(digits).decode('utf-16-be' if width == '2' else 'utf-32-be')
    text = re.sub(r'\\X([24])\\([0-9A-Fa-f]+)\\X0\\', wide, text)
    text = re.sub(r'\\X\\([0-9A-Fa-f]{2})', lambda m: chr(int(m[1],16)), text)
    text = re.sub(r'\\S\\(.)', lambda m: chr(ord(m[1])+128), text)
    text = text.replace('\\N\\','\n').replace('\\T\\','\t')
    return text.replace('\\\\','\\')


class _OriginalValues:
    """Small independent STEP value reader for retained original geometry only."""
    def __init__(self, text):
        self.text, self.at = text, 0

    def space(self):
        while self.at < len(self.text) and self.text[self.at].isspace():
            self.at += 1

    def value(self):
        self.space()
        require(self.at < len(self.text), 'truncated original geometry value')
        char = self.text[self.at]
        if char == '(':
            self.at += 1
            items = []
            while True:
                self.space()
                require(self.at < len(self.text), 'truncated original geometry list')
                if self.text[self.at] == ')':
                    self.at += 1
                    return tuple(items)
                if self.text[self.at] == ',':
                    self.at += 1
                    continue
                items.append(self.value())
        if char == "'":
            self.at += 1
            out = []
            while self.at < len(self.text):
                char = self.text[self.at]
                self.at += 1
                if char == "'":
                    if self.at < len(self.text) and self.text[self.at] == "'":
                        out.append("'")
                        self.at += 1
                    else:
                        return _step_text(''.join(out))
                else:
                    out.append(char)
            raise AssertionError('unterminated original geometry string')
        if char in '$*':
            self.at += 1
            return None
        if char == '#':
            self.at += 1
            match = re.match(r'\d+', self.text[self.at:])
            require(match is not None, 'invalid original geometry reference')
            self.at += len(match[0])
            return _Ref(int(match[0]))
        if char == '.' and self.at + 1 < len(self.text) and self.text[self.at+1].isalpha():
            end = self.text.find('.', self.at + 1)
            require(end > self.at, 'invalid original geometry enum')
            result = self.text[self.at:end+1]
            self.at = end + 1
            return result
        number = re.match(r'[-+]?(?:\d+(?:\.\d*)?|\.\d+)(?:[EeDd][-+]?\d+)?', self.text[self.at:])
        if number is not None:
            self.at += len(number[0])
            return F(number[0].replace('D', 'E').replace('d', 'e'))
        name = re.match(r'[A-Z_][A-Z0-9_]*', self.text[self.at:])
        require(name is not None, 'unsupported original geometry token')
        self.at += len(name[0])
        arguments = self.value()
        require(type(arguments) is tuple, 'original geometry entity arguments')
        return _Call(name[0], arguments)


def original_records(path):
    """Read the actual supplied family independently of the foreign adapter.

    Only original geometry/topology/placement records needed by direct source
    checks are retained. Header author/path fields are never returned.
    """
    text = Path(path).read_text(encoding='latin1')
    starts = tuple(re.finditer(r'(?m)^\s*#(\d+)\s*=\s*', text))
    wanted = {'CARTESIAN_POINT', 'DIRECTION', 'VECTOR', 'AXIS2_PLACEMENT_3D',
              'VERTEX_POINT', 'PLANE', 'CYLINDRICAL_SURFACE', 'CONICAL_SURFACE',
              'SPHERICAL_SURFACE', 'TOROIDAL_SURFACE', 'B_SPLINE_SURFACE_WITH_KNOTS',
              'LINE', 'CIRCLE', 'ELLIPSE', 'B_SPLINE_CURVE_WITH_KNOTS',
              'ADVANCED_FACE', 'FACE_OUTER_BOUND', 'FACE_BOUND', 'EDGE_LOOP', 'VERTEX_LOOP',
              'ORIENTED_EDGE', 'EDGE_CURVE', 'NEXT_ASSEMBLY_USAGE_OCCURRENCE',
              'PRODUCT_DEFINITION_SHAPE', 'CONTEXT_DEPENDENT_SHAPE_REPRESENTATION',
              'ITEM_DEFINED_TRANSFORMATION', 'CARTESIAN_TRANSFORMATION_OPERATOR_3D', 'SHAPE_REPRESENTATION',
              'ADVANCED_BREP_SHAPE_REPRESENTATION', 'SHAPE_DEFINITION_REPRESENTATION',
              'MANIFOLD_SOLID_BREP', 'MAPPED_ITEM', 'REPRESENTATION_MAP',
              'SHAPE_REPRESENTATION_RELATIONSHIP', 'UNCERTAINTY_MEASURE_WITH_UNIT',
              'BREP_WITH_VOIDS', 'CLOSED_SHELL', 'ORIENTED_CLOSED_SHELL',
              'PLANE_ANGLE_MEASURE_WITH_UNIT', 'LENGTH_MEASURE_WITH_UNIT', 'MEASURE_WITH_UNIT'}
    wanted.update(('PRODUCT_DEFINITION','PRODUCT_DEFINITION_FORMATION','PRODUCT'))
    records = {}
    for index, match in enumerate(starts):
        end = starts[index+1].start() if index+1 < len(starts) else len(text)
        rhs = text[match.end():end].strip()
        name = re.match(r'[A-Z_][A-Z0-9_]*', rhs)
        if not rhs.startswith('(') and (name is None or (name[0] not in wanted and
                                                         not _is_shape_representation_name(name[0]))):
            continue
        reader = _OriginalValues(rhs)
        result = reader.value()
        reader.space()
        require(reader.at < len(rhs) and rhs[reader.at] == ';', 'original geometry statement termination')
        records[int(match[1])] = result
    return records


def _refer(records, value, kind=None):
    require(type(value) is _Ref and value.id in records, 'original geometry reference missing')
    result = records[value.id]
    require(type(result) is _Call and (kind is None or result.kind == kind), 'original geometry reference type')
    return result


def _unit(vector):
    values = tuple(float(x) for x in vector)
    length = math.sqrt(sum(x*x for x in values))
    require(math.isfinite(length) and length > 0, 'original source direction degenerates')
    return tuple(x / length for x in values)


def original_frame(records, reference):
    axis = _refer(records, reference, 'AXIS2_PLACEMENT_3D').args
    origin = _refer(records, axis[1], 'CARTESIAN_POINT').args[1]
    z = _unit(_refer(records, axis[2], 'DIRECTION').args[1]) if axis[2] is not None else (0., 0., 1.)
    # ISO 10303-42 first_proj_axis: without a ref_direction, (1,0,0) unless the
    # axis is +-(1,0,0), then (0,1,0).
    x = (_unit(_refer(records, axis[3], 'DIRECTION').args[1]) if axis[3] is not None else
         (0., 1., 0.) if tuple(map(float, z)) in ((1., 0., 0.), (-1., 0., 0.)) else (1., 0., 0.))
    along = sum(a*b for a, b in zip(x, z))
    x = _unit(tuple(a-along*b for a, b in zip(x, z)))
    y = cross(z, x)
    return tuple(map(float, origin)) + x + y + z


def _original_direction(records, reference):
    """DIRECTION ratios as binary64 (WR1: not all zero), or None for $."""
    if reference is None:
        return None
    ratios = tuple(map(float, _refer(records, reference, 'DIRECTION').args[1]))
    require(len(ratios) == 3, 'original transformation operator direction is not 3D')
    require(all(math.isfinite(c) for c in ratios) and any(c != 0. for c in ratios),
            'original source direction degenerates')
    return ratios


def _dot3(a, b):
    return sum(x*y for x, y in zip(a, b))


def original_cartesian_operator(records, operator, inverse=False):
    """Row-major 3x4 rep_1-to-rep_2 map of a cartesian_transformation_operator_3d.

    ISO 10303-42 as carried by the AP242 MIM long form: scl = NVL(scale, 1)
    with WR1 scl > 0; u = base_axis(3, axis1, axis2, axis3), with
    first_proj_axis and second_proj_axis transcribed in their own operation
    order; the map is P -> local_origin + scl*(u[1] Px + u[2] Py + u[3] Pz).
    u[2] keeps the sense of axis2, so a mirroring operator has determinant
    -scl**3 here. u is orthonormal by construction, so inverse=True returns
    the rep_2-to-rep_1 map P -> u^T (P - local_origin) / scl. This deliberately does not follow OCCT 8.0.1
    StepToGeom::MakeTransformation3d, which inverts the scale, rebuilds a
    right-handed frame from axis1 and axis3 and defaults axis3 to axis1 x axis2.
    """
    require(type(operator) is _Call and operator.kind == 'CARTESIAN_TRANSFORMATION_OPERATOR_3D' and
            len(operator.args) == 8, 'original cartesian transformation operator arity')
    axis1, axis2, origin, scale, axis3 = operator.args[3:8]
    axis1, axis2, axis3 = (_original_direction(records, a) for a in (axis1, axis2, axis3))
    scl = 1. if scale is None else float(scale)
    require(math.isfinite(scl) and scl > 0., 'original transformation operator scale violates WR1')
    point = _refer(records, origin, 'CARTESIAN_POINT').args[1]
    require(len(point) == 3, 'original transformation operator origin is not 3D')
    point = tuple(map(float, point))
    # base_axis(3, ...): d1 := NVL(normalise(axis3), (0,0,1)).
    z = _unit(axis3) if axis3 is not None else (0., 0., 1.)
    # first_proj_axis(d1, axis1); its own normalise(z_axis) of a unit d1 is
    # omitted. An axis1 parallel to d1, or within rounding of it, has no
    # defined projection and refuses. The projection is applied twice: one
    # pass leaves x off orthogonal to z by about eps/sin(angle), a second pass
    # removes that residue, so u stays orthonormal to rounding.
    if axis1 is None:
        v = (1., 0., 0.) if z not in ((1., 0., 0.), (-1., 0., 0.)) else (0., 1., 0.)
    else:
        v = _unit(axis1)
        sine = cross(v, z)
        require(math.sqrt(_dot3(sine, sine)) > 8*2.**-52,
                'original transformation operator axis1 is parallel to axis3')
    for _ in range(2):
        along = _dot3(v, z)
        v = tuple(a-along*b for a, b in zip(v, z))
    x = _unit(v)
    # second_proj_axis(d1, d2, axis2) removes from arg (default (0,1,0)) its
    # z and x components and normalises. EXPRESS dot_product and
    # vector_difference work on normalised directions, so the standard's
    # intermediate vectors differ from these by a positive scale only, which
    # the final normalise removes. With x and z orthonormal the result equals
    # sign(arg . (z x x)) (z x x) in exact arithmetic; that form keeps u
    # orthonormal to rounding (classical Gram-Schmidt does not when arg is
    # near the x-z plane), so u^T is its inverse. An arg in that plane, or
    # within rounding of it, has no defined sense and refuses.
    v = axis2 if axis2 is not None else (0., 1., 0.)
    w = _unit(cross(z, x))
    along = _dot3(v, w)
    require(abs(along) > 8*2.**-52*math.sqrt(_dot3(v, v)),
            'original transformation operator axis2 lies in the axis1-axis3 plane')
    y = w if along > 0 else tuple(-c for c in w)
    u = (x, y, z)
    if inverse:
        return tuple(value for i in range(3) for value in
                     (u[i][0]/scl, u[i][1]/scl, u[i][2]/scl, -_dot3(u[i], point)/scl))
    return tuple(value for i in range(3) for value in
                 (scl*u[0][i], scl*u[1][i], scl*u[2][i], point[i]))


def _near(actual, expected, tolerance, description):
    require(len(actual) == len(expected), description + ' extent')
    error = max((abs(float(a)-float(b)) for a, b in zip(actual, expected)), default=0.)
    require(math.isfinite(error) and error <= tolerance, description)
    return error


_SI_PREFIX = {None: 1., '.EXA.': 1e18, '.PETA.': 1e15, '.TERA.': 1e12, '.GIGA.': 1e9,
              '.MEGA.': 1e6, '.KILO.': 1e3, '.HECTO.': 1e2, '.DECA.': 1e1, '.DECI.': 1e-1,
              '.CENTI.': 1e-2, '.MILLI.': 1e-3, '.MICRO.': 1e-6, '.NANO.': 1e-9,
              '.PICO.': 1e-12, '.FEMTO.': 1e-15, '.ATTO.': 1e-18}
# Every StepShape_ShapeRepresentation subtype OCCT 8.0.1 reads, as the adapter's
# IsKind(StepShape_ShapeRepresentation) admits them.
_SHAPE_REPRESENTATION_EXTRAS = frozenset(('SHAPE_REPRESENTATION_WITH_PARAMETERS', 'SHAPE_DIMENSION_REPRESENTATION',
                                          'POINT_REPRESENTATION',
                                          'TESSELLATED_SHAPE_REPRESENTATION_WITH_ACCURACY_PARAMETERS'))


def _is_shape_representation_name(name):
    return name.endswith('SHAPE_REPRESENTATION') or name in _SHAPE_REPRESENTATION_EXTRAS


def _shape_representation(row):
    """(name, items, context) of a shape representation record, else None."""
    if type(row) is _Call:
        return row.args if _is_shape_representation_name(row.kind) and len(row.args) >= 3 else None
    if type(row) is tuple and any(_is_shape_representation_name(part.kind) for part in row):
        for part in row:
            if part.kind == 'REPRESENTATION' and len(part.args) >= 3:
                return part.args
    return None


def _si_radian(records, reference):
    unit = records.get(reference.id)
    parts = {part.kind: part for part in unit} if type(unit) is tuple else {}
    if 'PLANE_ANGLE_UNIT' not in parts or 'SI_UNIT' not in parts:
        return None
    prefix, name = parts['SI_UNIT'].args
    return _SI_PREFIX[prefix] if name == '.RADIAN.' and prefix in _SI_PREFIX else None


def _plane_angle_unit(records, reference):
    """Radians per declared plane-angle unit; None when it is no plane-angle unit.

    Same rule as the step-source adapter: SI radian with its prefix, or a
    conversion-based unit whose positive measure is in SI radians. A conversion
    to anything else (including another conversion, so no cycle can recurse)
    is refused.
    """
    unit = records.get(reference.id)
    if type(unit) is not tuple:
        return None
    parts = {part.kind: part for part in unit}
    if 'PLANE_ANGLE_UNIT' not in parts:
        return None
    if 'SI_UNIT' in parts:
        factor = _si_radian(records, reference)
        require(factor is not None, 'unsupported original SI plane-angle unit')
        return factor
    require('CONVERSION_BASED_UNIT' in parts, 'uninterpretable original plane-angle unit')
    measure = _refer(records, parts['CONVERSION_BASED_UNIT'].args[1]).args
    value = measure[0]
    require(type(value) is _Call and value.kind == 'PLANE_ANGLE_MEASURE', 'original plane-angle conversion measure')
    target = _si_radian(records, measure[1])
    require(target is not None, 'uninterpretable original plane-angle unit: conversion is not to SI radians')
    require(math.isfinite(float(value.args[0])) and float(value.args[0]) > 0, 'nonpositive original plane-angle conversion')
    return target*float(value.args[0])


_SI_EXPONENT = {None: 0, '.EXA.': 18, '.PETA.': 15, '.TERA.': 12, '.GIGA.': 9, '.MEGA.': 6, '.KILO.': 3,
                '.HECTO.': 2, '.DECA.': 1, '.DECI.': -1, '.CENTI.': -2, '.MILLI.': -3, '.MICRO.': -6,
                '.NANO.': -9, '.PICO.': -12, '.FEMTO.': -15, '.ATTO.': -18}


def _length_unit(records, reference, chain=()):
    """Millimetres per length unit from its entities (G6 UR-2); None when no length unit.

    An SI metre gives 10^(prefix+3) mm; a conversion-based unit gives its
    positive length measure times its target's millimetres, to depth 8. A
    conversion target may be an SI metre written without LENGTH_UNIT (as the
    adapter accepts). The name never sets the factor.
    """
    unit = records.get(reference.id)
    if type(unit) is not tuple:
        return None
    parts = {part.kind: part for part in unit}
    plain_target = bool(chain) and 'SI_UNIT' in parts and parts['SI_UNIT'].args[1] == '.METRE.'
    if 'LENGTH_UNIT' not in parts and not plain_target:
        return None
    require(reference.id not in chain, 'original conversion-based length unit refers to itself (a cycle)')
    require(len(chain) < 8, 'original conversion-based length unit nested deeper than 8')
    if 'SI_UNIT' in parts:
        prefix, name = parts['SI_UNIT'].args
        require(name == '.METRE.' and prefix in _SI_EXPONENT, 'unsupported original SI length unit')
        return float('1e%d' % (_SI_EXPONENT[prefix]+3))
    require('CONVERSION_BASED_UNIT' in parts, 'uninterpretable original length unit')
    measure = _refer(records, parts['CONVERSION_BASED_UNIT'].args[1]).args
    value = measure[0]
    require(type(value) is _Call and value.kind == 'LENGTH_MEASURE', 'original length conversion measure')
    require(math.isfinite(float(value.args[0])) and float(value.args[0]) > 0, 'nonpositive original length conversion')
    target = _length_unit(records, measure[1], chain+(reference.id,))
    require(target is not None, 'original length conversion target is no length unit')
    return float(value.args[0])*target


class _RepresentationGraph:
    """Shape-representation links the adapter follows: SDR use, relationships, mapped items."""
    def __init__(self, records):
        self.records = records
        self.shape = {id: args for id, r in records.items() for args in (_shape_representation(r),) if args is not None}
        shape = self.shape
        self.used = {r.args[1].id for r in records.values()
                     if type(r) is _Call and r.kind == 'SHAPE_DEFINITION_REPRESENTATION'}
        self.holders, item_holders, self.callers = defaultdict(list), defaultdict(list), defaultdict(list)
        for id in shape:
            for item in shape[id][1]:
                if type(item) is _Ref:
                    item_holders[item.id].append(id)
        self.holders = item_holders
        for id, row in records.items():
            for part in (row if type(row) is tuple else (row,)):
                # A complex relationship's SHAPE_REPRESENTATION_RELATIONSHIP() part is empty.
                if part.kind in ('SHAPE_REPRESENTATION_RELATIONSHIP', 'REPRESENTATION_RELATIONSHIP') and len(part.args) >= 4:
                    a, b = part.args[2].id, part.args[3].id
                    if b in shape: self.callers[a].append(b)
                    if a in shape: self.callers[b].append(a)
            if type(row) is _Call and row.kind == 'MAPPED_ITEM':
                mapping = records.get(row.args[1].id)
                if type(mapping) is _Call and mapping.kind == 'REPRESENTATION_MAP':
                    self.callers[mapping.args[1].id].extend(item_holders.get(id, ()))

    def transferred(self, rep_id):
        """Breadth-first, each representation once and at any depth, as the adapter's
        transfer_path (native/step_units.hpp; bead .37.32): the visited set alone bounds
        the search, so no nesting depth is assumed."""
        seen, queue = {rep_id}, [rep_id]
        for rep in queue:
            if rep in self.used:
                return True
            for caller in self.callers.get(rep, ()):
                if caller not in seen:
                    seen.add(caller)
                    queue.append(caller)
        return False

    def plane_angle(self, rep_id):
        """('declared', factor) or ('absent', None) for one representation context."""
        records = self.records
        context = records.get(self.shape[rep_id][2].id)
        parts = [part for part in context if part.kind == 'GLOBAL_UNIT_ASSIGNED_CONTEXT'] if type(context) is tuple else []
        if parts:
            factors = [f for ref in parts[0].args[0] for f in (_plane_angle_unit(records, ref),) if f is not None]
            require(len(factors) <= 1, 'original context declares several plane-angle units')
            return ('declared', factors[0]) if factors else ('absent', None)
        # Like the adapter: a context without unit assignment declares no unit.
        return ('absent', None)

    def _context_parts(self, rep_id):
        context = self.records.get(self.shape[rep_id][2].id)
        return {part.kind: part for part in context} if type(context) is tuple else {}

    def length(self, rep_id):
        """Millimetres per length unit of one representation's context, or None."""
        parts = self._context_parts(rep_id).get('GLOBAL_UNIT_ASSIGNED_CONTEXT')
        factors = [f for ref in (parts.args[0] if parts else ()) for f in (_length_unit(self.records, ref),)
                   if f is not None]
        require(len(factors) <= 1, 'original context declares several length units')
        return factors[0] if factors else None

    def uncertainty(self, rep_id):
        """Governing uncertainty in mm of one context (G6 UR-7), or None when absent."""
        parts = self._context_parts(rep_id).get('GLOBAL_UNCERTAINTY_ASSIGNED_CONTEXT')
        measures = []
        for ref in (parts.args[0] if parts else ()):
            row = _refer(self.records, ref, 'UNCERTAINTY_MEASURE_WITH_UNIT').args
            factor = _length_unit(self.records, row[1])
            if factor is not None:
                named = (row[2] or '').lower() in ('closure', 'distance_accuracy_value', 'distance accuracy value',
                                                   'point coincidence tolerance')
                measures.append((named, float(row[0].args[0])*factor))
        if any(named for named, _ in measures):
            measures = [m for m in measures if m[0]]
        return max((value for _, value in measures), default=None)

    def solid_holders(self, solid_id):
        holders = [id for id in self.holders.get(solid_id, ()) if self.transferred(id)]
        require(holders, 'original solid is not held by a transferred shape representation')
        return holders

    def solid_plane_angle(self, solid_id):
        """Radians per plane-angle unit governing one solid, or None when absent."""
        holders = [id for id in self.holders.get(solid_id, ()) if self.transferred(id)]
        require(holders, 'original solid is not held by a transferred shape representation')
        found = {self.plane_angle(id) for id in holders}
        require(len(found) == 1, 'original solid representations declare different plane-angle units')
        return next(iter(found))[1]


def original_face_sides(original_path, face_ids):
    """{face: 'into_material' | 'outward' | 'undecidable: ...'} for the raw use of each face.

    Runs the reference exporter's --face-sides mode (native/reference_source;
    orientation_verdict.hpp raw_use_side): ray parity on OCCT's default read of
    the file against the raw surface, which shares no code with the STEP source
    adapter's orientation classifier. The executable is BREPGPU_REFERENCE_SOURCE
    or the repository build; without it the check refuses by name.
    """
    import os
    import subprocess
    executable = Path(os.environ.get('BREPGPU_REFERENCE_SOURCE',
                                     Path(__file__).resolve().parents[2]/'build/om10-reference-source/brepgpu-reference-source'))
    require(executable.is_file(), f'independent face-side check unavailable: {executable} is not built')
    completed = subprocess.run((str(executable), '--face-sides', str(original_path), *map(str, face_ids)),
                               capture_output=True, text=True)
    require(completed.returncode == 0, 'independent face-side check failed: '+completed.stderr.strip()[-200:])
    sides = {}
    for line in completed.stdout.splitlines():
        stage, face, side = line.split('\t', 2)
        if stage == 'face_side':
            sides[int(face)] = side
    require(set(sides) == set(face_ids), 'independent face-side check did not answer every face')
    return sides


def qualify_original_geometry(source, original_path, coordinate_tolerance=2.**-32):
    """Compare every actual source carrier/vertex with original STEP values.

    This closes the shared-adapter truth gap. The stated grade compares finite
    extracted numbers; it is not a global source or meshing error certificate.
    """
    counts = qualify_source(source, original_path)
    records = original_records(original_path)
    for product in source.products:
        raw = records[product.id]
        require(raw.kind == 'PRODUCT_DEFINITION', 'original product definition missing')
        formation = _refer(records, raw.args[2], 'PRODUCT_DEFINITION_FORMATION')
        item = _refer(records, formation.args[2], 'PRODUCT').args
        name = item[2] or item[1]
        require((product.name, product.reference) == (name, item[0]), 'original product part name/reference differs')
    for usage in source.usages:
        require(usage.name == records[usage.id].args[1], 'original source usage name differs')
    # Units and uncertainty per context (G6 UR-1, UR-7): each solid's holders
    # are in the source's millimetres, and the header carries the largest
    # governing uncertainty of the solids' own B-rep contexts that makes a usable
    # claim (0 when none). UR-7: a value below 1e-15 of the definition's diagonal,
    # 0 among them, makes no claim. The diagonal here is the box of the raw
    # vertex points, a lower bound of the adapter's shape box, so the two can
    # differ only for a value within that gap of the floor.
    representations = _RepresentationGraph(records)
    plane_angle = {definition.id: representations.solid_plane_angle(definition.id) for definition in source.definitions}
    governing = {}
    for definition in source.definitions:
        holders = representations.solid_holders(definition.id)
        require({representations.length(id) for id in holders} == {source.units_mm},
                'original solid context length unit differs from owned source')
        values = [v for v in map(representations.uncertainty, holders) if v is not None]
        points = [source.vertex(i).xyz for i in definition.vertex_ids]
        diagonal = math.dist(tuple(map(min, *points)), tuple(map(max, *points))) if len(points) > 1 else 0.
        value = max(values, default=None)
        governing[definition.id] = value if value is not None and value > 0 and value >= 1e-15*diagonal else None
    require(max((v for v in governing.values() if v is not None), default=0.) == source.source_uncertainty_mm,
            'original governing B-rep uncertainty differs from owned source')
    product_ids = {row.id for row in source.products}
    product_solids = {}
    # Plain relationships join one product's representations; a placing
    # relation (a context_dependent_shape_representation's) places a child in
    # its parent and joins nothing, as in _representation_products (bead .37.21).
    placing = _placing_relations(records)
    mapped_occurrences = original_mapped_occurrences(records)
    representation_links = defaultdict(set)
    for id, record in records.items():
        if type(record) is _Call and record.kind == 'SHAPE_REPRESENTATION_RELATIONSHIP' and id not in placing:
            a, b = record.args[2].id, record.args[3].id
            representation_links[a].add(b)
            representation_links[b].add(a)
    def representation_solids(id, active=()):
        require(id not in active, 'original mapped representation cycle')
        row = records[id]
        require(type(row) is _Call and row.kind in ('SHAPE_REPRESENTATION', 'ADVANCED_BREP_SHAPE_REPRESENTATION'),
                'original shape representation missing')
        result = set()
        for item in row.args[1]:
            record = records.get(item.id)
            if type(record) is not _Call:
                continue
            if record.kind in ('MANIFOLD_SOLID_BREP', 'BREP_WITH_VOIDS'):
                result.add(item.id)
            elif record.kind == 'MAPPED_ITEM':
                # A solid placed through a mapped item of a product's own
                # representation is an occurrence (original_mapped_occurrences;
                # bead .37.34), not the holder's own solid: it is not followed.
                if item.id in mapped_occurrences:
                    continue
                mapping = _refer(records, record.args[1], 'REPRESENTATION_MAP').args
                # Any other mapped item is followed only as a same-frame map.
                require(mapping[0] == record.args[2], 'unhandled nonidentity original representation map')
                result.update(representation_solids(mapping[1].id, active+(id,)))
        for neighbor in representation_links[id] - set(active):
            result.update(representation_solids(neighbor, active+(id,)))
        return result
    for row in records.values():
        if type(row) is _Call and row.kind == 'SHAPE_DEFINITION_REPRESENTATION':
            pds = _refer(records, row.args[0], 'PRODUCT_DEFINITION_SHAPE').args
            product_id = pds[2].id
            if product_id in product_ids:
                solids = representation_solids(row.args[1].id)
                require(len(solids) <= 1, 'original product has ambiguous solid definitions')
                product_solids[product_id] = next(iter(solids), 0)
    expected_products = {row.id: row.definition_id for row in source.products}
    differences = tuple((id, product_solids.get(id), expected_products.get(id))
                        for id in sorted(product_solids.keys() | expected_products.keys())
                        if product_solids.get(id) != expected_products.get(id))
    require(not differences, 'original product to solid representation correspondence differs: '+repr(differences[:8]))
    maximum = 0.
    for vertex in source.vertices:
        raw = records.get(vertex.id)
        require(type(raw) is _Call and raw.kind == 'VERTEX_POINT', 'original vertex record missing')
        point = _refer(records, raw.args[1], 'CARTESIAN_POINT').args[1]
        maximum = max(maximum, _near(vertex.xyz, point, coordinate_tolerance, 'original vertex coordinates differ'))
    tags = {'PLANE': 1, 'CYLINDRICAL_SURFACE': 2, 'CONICAL_SURFACE': 3,
            'SPHERICAL_SURFACE': 4, 'TOROIDAL_SURFACE': 5, 'B_SPLINE_SURFACE_WITH_KNOTS': 6}
    original_carriers = {id for id, row in records.items() if type(row) is _Call and row.kind in tags}
    require({row.id for row in source.carriers} == original_carriers, 'complete original surface carrier IDs differ')
    for carrier in source.carriers:
        row = records[carrier.id]
        args = row.args
        require(carrier.kind == tags[row.kind], 'original carrier type differs')
        if carrier.kind != 6:
            maximum = max(maximum, _near(carrier.frame, original_frame(records, args[1]),
                                         coordinate_tolerance, 'original carrier frame differs'))
            # The cone semi-angle is stated in the context's plane-angle unit.
            if carrier.kind == 3:
                owners = {plane_angle[face.definition_id] for face in source.faces if face.carrier_id == carrier.id}
                require(len(owners) == 1 and None not in owners, 'original cone has no single declared plane-angle unit')
            radii = (0., 0.) if carrier.kind == 1 else (
                (float(args[2]), float(args[3])*next(iter(owners))) if carrier.kind == 3 else
                (float(args[2]), float(args[3])) if carrier.kind == 5 else (float(args[2]), 0.))
            maximum = max(maximum, _near(carrier.radii, radii, coordinate_tolerance, 'original carrier parameters differ'))
        else:
            pu, pv = map(int, args[1:3])
            net = args[3]
            nu, nv = len(net), len(net[0])
            require(all(len(row) == nv for row in net), 'original surface control rows differ')
            require(carrier.degrees == (pu, pv) and carrier.control_shape == (nu, nv),
                    'original spline surface degrees/counts differ')
            require(tuple(map(exact, carrier.knot_u)) == tuple(map(exact, map(float, args[10]))) and
                    tuple(map(exact, carrier.knot_v)) == tuple(map(exact, map(float, args[11]))) and
                    carrier.mult_u == tuple(map(int, args[8])) and carrier.mult_v == tuple(map(int, args[9])),
                    'original spline surface knots/multiplicities differ')
            controls = tuple(component for point_row in net for reference in point_row
                             for component in (*_refer(records, reference, 'CARTESIAN_POINT').args[1], F(1)))
            maximum = max(maximum, _near(carrier.controls, controls, coordinate_tolerance,
                                         'original spline surface homogeneous controls differ'))
    # Shell of every original face: ordinal 0 outer, k for the k-th void, with
    # the void's oriented_closed_shell sense composed into the face use.
    face_shells = {}
    for id in {row.id for row in source.definitions}:
        raw = records[id]
        outer = raw.args[1]
        if records[outer.id].kind == 'ORIENTED_CLOSED_SHELL':
            # OCCT and the adapter take a .T. oriented outer shell as its
            # closed shell; ABSR WR3 forbids it and a .F. one refuses.
            oriented = records[outer.id].args
            require(oriented[3] == '.T.', 'original outer shell is a reversed oriented_closed_shell')
            outer = oriented[2]
        shells = [(0, True, outer)]
        if raw.kind == 'BREP_WITH_VOIDS':
            for k, ref in enumerate(raw.args[2], 1):
                oriented = _refer(records, ref, 'ORIENTED_CLOSED_SHELL').args
                shells.append((k, oriented[3] == '.T.', oriented[2]))
        for ordinal, forward, ref in shells:
            for face_ref in _refer(records, ref, 'CLOSED_SHELL').args[1]:
                require(face_ref.id not in face_shells, 'original face in two shells')
                face_shells[face_ref.id] = (ordinal, forward)
    require(set(face_shells) == {face.id for face in source.faces}, 'original shell face inventory differs')
    # A written side other than the raw use (same_sense composed with the shell)
    # is accepted only where the source records the correction
    # (source_face_orientation_raw, bead .37.36) and an independent check
    # confirms that the raw use's outward normal points into the material
    # (original_face_sides: ray parity, no code shared with the adapter).
    recorded = {d.source_ids[1] for d in source.diagnostics
                if d.stage == 'source_face_orientation_raw' and len(d.source_ids) >= 2}
    corrected = []
    for face in source.faces:
        raw = records[face.id]
        ordinal, forward = face_shells[face.id]
        raw_orientation = 1 if (raw.args[3] == '.T.') == forward else -1
        require(raw.kind == 'ADVANCED_FACE' and face.carrier_id == raw.args[2].id and
                face.orientation in (raw_orientation, -raw_orientation) and
                (face.orientation == raw_orientation or face.id in recorded) and
                face.shell == ordinal,
                'original face carrier/orientation/shell differs')
        if face.orientation == raw_orientation:
            require(face.id not in recorded, f'original face #{face.id} has an orientation record but is written as its raw use')
        else:
            corrected.append(face.id)
        loops = tuple(_refer(records, ref).args[1].id for ref in raw.args[1])
        require(set(face.loop_ids) == set(loops), 'original face bounds differ')
        for ref in raw.args[1]:
            bound = _refer(records, ref)
            loop = source.loop(bound.args[1].id)
            require(loop.outer == (bound.kind == 'FACE_OUTER_BOUND'), 'original outer/inner loop designation differs')
            if _refer(records, bound.args[1]).kind == 'VERTEX_LOOP':
                # AP242 advanced_face WR6, WR7: one point vertex, no uses.
                pole = _refer(records, bound.args[1]).args[1]
                require(_refer(records, pole, 'VERTEX_POINT') and loop.vertex_id == pole.id and not loop.coedge_ids,
                        'original vertex loop differs')
                continue
            require(loop.vertex_id == 0, 'original edge loop carries a vertex')
            raw_loop = _refer(records, bound.args[1], 'EDGE_LOOP')
            word = tuple(ref.id for ref in raw_loop.args[1])
            bound_forward = bound.args[2] == '.T.'
            require(loop.coedge_ids == (word if bound_forward else word[::-1]),
                    'original directed coedge loop word differs')
            for id in word:
                oe = records[id]
                expected_sense = 1 if (oe.args[4] == '.T.') == bound_forward else -1
                require(source.coedge(id).sense == expected_sense, 'original oriented-edge/bound traversal sense differs')
    require(recorded <= set(face_shells), 'original face orientation record names no face of the source')
    if corrected:
        sides = original_face_sides(original_path, corrected)
        unconfirmed = sorted(f for f in corrected if sides.get(f) != 'into_material')
        require(not unconfirmed, 'original face side correction not confirmed by the independent check (the raw '
                                 f'use is not shown to point into the material): faces {unconfirmed[:8]}')
    for edge in source.edges:
        raw = records[edge.id]
        require(raw.kind == 'EDGE_CURVE' and edge.vertex_ids == (raw.args[1].id, raw.args[2].id),
                'original edge endpoints differ')
        require(edge.curve_same_sense == (raw.args[4] == '.T.'), 'original edge curve sense differs')
        if not edge.degenerate:
            require(edge.curve_id == raw.args[3].id, 'original spatial curve identity differs')
    for coedge in source.coedges:
        raw = records[coedge.id]
        require(raw.kind == 'ORIENTED_EDGE' and coedge.edge_id == raw.args[3].id,
                'original coedge physical edge differs')
    physical_curves = tuple(curve for curve in source.curves if curve.dimension == 3)
    for curve in physical_curves:
        raw = records[curve.id]
        if type(raw) is tuple:
            parts = {part.kind: part for part in raw}
            require('B_SPLINE_CURVE' in parts and 'B_SPLINE_CURVE_WITH_KNOTS' in parts,
                    'unsupported original complex spatial curve')
            base, knot = parts['B_SPLINE_CURVE'].args, parts['B_SPLINE_CURVE_WITH_KNOTS'].args
            degree, references, mults, knots = int(base[0]), base[1], knot[0], knot[1]
            weights = parts['RATIONAL_B_SPLINE_CURVE'].args[0]
        elif raw.kind == 'B_SPLINE_CURVE_WITH_KNOTS':
            degree, references, mults, knots = int(raw.args[1]), raw.args[2], raw.args[6], raw.args[7]
            weights = (F(1),) * len(references)
        elif raw.kind == 'LINE':
            require(curve.kind == 1, 'original line kind differs')
            point = _refer(records, raw.args[1], 'CARTESIAN_POINT').args[1]
            vector = _refer(records, raw.args[2], 'VECTOR').args
            direction = _unit(_refer(records, vector[1], 'DIRECTION').args[1])
            maximum = max(maximum, _near(curve.frame[:6], tuple(point) + direction, coordinate_tolerance,
                                         'original line origin/direction differs'))
            continue
        elif raw.kind in ('CIRCLE', 'ELLIPSE'):
            require(curve.kind == (2 if raw.kind == 'CIRCLE' else 3), 'original conic curve kind differs')
            maximum = max(maximum, _near(curve.frame, original_frame(records, raw.args[1]),
                                         coordinate_tolerance, 'original conic curve frame differs'))
            radii = (float(raw.args[2]),)*2 if raw.kind == 'CIRCLE' else tuple(map(float, raw.args[2:4]))
            maximum = max(maximum, _near(curve.radii, radii, coordinate_tolerance,
                                         'original conic curve radius differs'))
            continue
        else:
            require(False, 'unsupported original spatial curve family')
        require(curve.kind == 5 and curve.degree == degree and
                curve.multiplicities == tuple(map(int, mults)) and
                tuple(curve.knots) == tuple(map(float, knots)), 'original spline curve degree/knots differ')
        controls = tuple(value for ref, weight in zip(references, weights)
                         for value in (*(x * weight for x in _refer(records, ref, 'CARTESIAN_POINT').args[1]), weight))
        maximum = max(maximum, _near(curve.controls, controls, coordinate_tolerance,
                                     'original spline spatial-curve homogeneous controls differ'))
    placement_error = qualify_original_placements(source, records, coordinate_tolerance)
    return dict(**counts, max_original_coordinate_error=maximum,
                original_spatial_curves=len(physical_curves), max_original_placement_error=placement_error)


def _placing_relations(records):
    """IDs of the representation relations that context_dependent_shape_representations place by."""
    return {row.args[0].id for row in records.values()
            if type(row) is _Call and row.kind == 'CONTEXT_DEPENDENT_SHAPE_REPRESENTATION'
            and type(row.args[0]) is _Ref}


def _representation_products(records):
    """Product definitions that each shape representation represents.

    A representation represents the definition of every
    shape_definition_representation that uses it, and of every representation
    joined to it by a plain (untransformed) shape_representation_relationship.
    A relationship that is the representation_relation of a
    context_dependent_shape_representation places a child in its parent and
    does not join one product's representations, so it is skipped. Only
    shape definitions of a product_definition count: one attached to a
    relationship's (for example a NAUO's) product_definition_shape names no
    product.
    """
    placing = _placing_relations(records)
    direct, links = defaultdict(set), defaultdict(set)
    for id, row in records.items():
        if type(row) is not _Call or id in placing:
            continue
        if row.kind == 'SHAPE_DEFINITION_REPRESENTATION' and all(type(a) is _Ref for a in row.args[:2]):
            shape = records.get(row.args[0].id)
            if type(shape) is _Call and shape.kind == 'PRODUCT_DEFINITION_SHAPE' and type(shape.args[2]) is _Ref:
                definition = records.get(shape.args[2].id)
                if type(definition) is _Call and definition.kind == 'PRODUCT_DEFINITION':
                    direct[row.args[1].id].add(shape.args[2].id)
        elif row.kind == 'SHAPE_REPRESENTATION_RELATIONSHIP' and all(type(a) is _Ref for a in row.args[2:4]):
            a, b = row.args[2].id, row.args[3].id
            links[a].add(b)
            links[b].add(a)
    def products(rep):
        seen, stack, found = {rep}, [rep], set()
        while stack:
            current = stack.pop()
            found |= direct[current]
            for neighbor in links[current] - seen:
                seen.add(neighbor)
                stack.append(neighbor)
        return found
    return products


def _original_relation_reversed(records, products, rep1, rep2, usage_id):
    """True when rep_1 is the parent's representation and rep_2 the child's.

    The transformation maps rep_1 into rep_2 and the NAUO names the parent
    (relating) and child (related) definitions; the representations' own
    products decide which way the usage reads, as OCCT 8.0.1
    STEPConstruct_Assembly::CheckSRRReversesNAUO does with direct SDRs. A
    reading needs at least one side with products, and every side with
    products must hold its expected definition: forward is rep_1 the child's
    and rep_2 the parent's, backward the reverse. Exactly one reading must
    hold. No evidence, evidence both ways, or a side holding an unrelated
    product refuses; OCCT falls through to forward in those cases, which
    places the child in coordinates that are not its parent's.
    """
    usage = records.get(usage_id)
    require(type(usage) is _Call and usage.kind == 'NEXT_ASSEMBLY_USAGE_OCCURRENCE' and
            len(usage.args) >= 5 and all(type(a) is _Ref for a in usage.args[3:5]),
            'original placed product relationship is not a NAUO')
    parent, child = usage.args[3].id, usage.args[4].id
    first, second = products(rep1), products(rep2)
    def reads(on_first, on_second):
        return bool(first or second) and (not first or on_first in first) and (not second or on_second in second)
    forward, backward = reads(child, parent), reads(parent, child)
    require(forward != backward, 'original representation relation does not orient its usage')
    return backward


def _original_relation_items(records, rep):
    shape = _shape_representation(records.get(rep))
    require(shape is not None, 'original related shape representation missing')
    return {item.id for item in shape[1] if type(item) is _Ref}


def _original_usage_transforms(records):
    """Child-to-parent 3x4 map of every usage placed by a transformed relation."""
    transforms = {}
    products = _representation_products(records)
    for row in records.values():
        if type(row) is not _Call or row.kind != 'CONTEXT_DEPENDENT_SHAPE_REPRESENTATION':
            continue
        relation = records.get(row.args[0].id) if type(row.args[0]) is _Ref else None
        # A placing relation without a transformation (simple or complex) has
        # no source that defines its placement; OCCT reads the identity, and
        # agreeing with that reading would not be independent.
        untransformed = type(relation) is _Call and relation.kind == 'SHAPE_REPRESENTATION_RELATIONSHIP'
        require(not untransformed, 'original placing relation carries no transformation')
        require(type(relation) is tuple, 'original transformed representation relation missing')
        parts = {part.kind: part for part in relation}
        transformation = parts.get('REPRESENTATION_RELATIONSHIP_WITH_TRANSFORMATION')
        require(transformation is not None, 'original placing relation carries no transformation')
        reps = parts.get('REPRESENTATION_RELATIONSHIP')
        require(reps is not None and len(reps.args) == 4 and all(type(a) is _Ref for a in reps.args[2:4]),
                'original representation relation representations missing')
        rep1, rep2 = reps.args[2].id, reps.args[3].id
        operator = transformation.args[0]
        operator = records.get(operator.id) if type(operator) is _Ref else None
        # transformation = SELECT(functionally_defined_transformation,
        # item_defined_transformation, set_item_defined_transformation); only
        # the two kinds below are read, every other value refuses here.
        require(type(operator) is _Call and
                operator.kind in ('ITEM_DEFINED_TRANSFORMATION', 'CARTESIAN_TRANSFORMATION_OPERATOR_3D'),
                'unsupported original representation transformation operator')
        product_shape = _refer(records, row.args[1], 'PRODUCT_DEFINITION_SHAPE').args
        usage_id = product_shape[2].id
        require(usage_id not in transforms, 'ambiguous original usage placement')
        reversed_ = _original_relation_reversed(records, products, rep1, rep2, usage_id)
        if operator.kind == 'CARTESIAN_TRANSFORMATION_OPERATOR_3D':
            # The operator maps rep_1 into rep_2; a reversed relation reads
            # child-to-parent through its inverse.
            transforms[usage_id] = original_cartesian_operator(records, operator, inverse=reversed_)
            continue
        item = operator.args
        require(all(type(a) is _Ref for a in item[2:4]), 'original item-defined transformation frames missing')
        first, second = item[2], item[3]
        # WR2: transform_item_1 belongs to rep_1, transform_item_2 to rep_2. A
        # pair placed the other way round is swapped, as OCCT's
        # ComputeTransformation does; a pair in neither order refuses.
        items1, items2 = _original_relation_items(records, rep1), _original_relation_items(records, rep2)
        if not (first.id in items1 and second.id in items2):
            require(first.id in items2 and second.id in items1,
                    'original item-defined transformation frames do not belong to its representations')
            first, second = second, first
        if reversed_:
            first, second = second, first
        transforms[usage_id] = _frames_matrix(records, first, second)
    return transforms


def _frames_matrix(records, first, second):
    """The child-side axis2_placement_3d A mapped onto the parent-side B: B * inverse(A)."""
    a, b = original_frame(records, first), original_frame(records, second)
    rotation = tuple(sum(b[3+3*k+i] * a[3+3*k+j] for k in range(3))
                     for i in range(3) for j in range(3))
    return tuple(value for i in range(3) for value in
                 (*rotation[3*i:3*i+3], b[i] - sum(rotation[3*i+j]*a[j] for j in range(3))))


_SOLID_KINDS = ('MANIFOLD_SOLID_BREP', 'BREP_WITH_VOIDS')
_SHAPE_REPRESENTATION_KINDS = ('SHAPE_REPRESENTATION', 'ADVANCED_BREP_SHAPE_REPRESENTATION')


def original_mapped_occurrences(records):
    """Solids placed through a MAPPED_ITEM in a product's own shape representation.

    Read from the STEP text, separately from the reference exporter
    (native/reference_source), as the occurrence it publishes (dispatcher
    decision, B2-reader round 2): the holder product is an assembly node, the
    mapped representation a part keyed by its own label owning its one solid,
    and the mapped item a usage keyed by its label. The placement maps the
    mapped representation's frame onto the holder's: mapping_target composed
    with the inverse of the representation_map's mapping_origin (ISO 10303-43;
    the PDM Usage Guide's C = B A^-1 for the same pair of frames).

    Returns {mapped item: (holder product, mapped representation, solid,
    matrix)}. A mapped item whose representation holds no solid is no
    occurrence. Refused by name: a mapped representation that a product owns
    (bead .37.26), nested mapped items, several solids, and a placement other
    than an axis2_placement_3d pair (a cartesian operator target is not read).
    """
    sdr_used, product_reps = set(), defaultdict(list)
    for row in records.values():
        if type(row) is _Call and row.kind == 'SHAPE_DEFINITION_REPRESENTATION' and all(type(a) is _Ref for a in row.args[:2]):
            sdr_used.add(row.args[1].id)
            shape = records.get(row.args[0].id)
            if type(shape) is _Call and shape.kind == 'PRODUCT_DEFINITION_SHAPE' and type(shape.args[2]) is _Ref:
                definition = records.get(shape.args[2].id)
                if type(definition) is _Call and definition.kind == 'PRODUCT_DEFINITION':
                    product_reps[shape.args[2].id].append(row.args[1].id)
    placing = _placing_relations(records)
    links = defaultdict(set)
    for id, row in records.items():
        if type(row) is _Call and row.kind == 'SHAPE_REPRESENTATION_RELATIONSHIP' and id not in placing \
                and all(type(a) is _Ref for a in row.args[2:4]):
            links[row.args[2].id].add(row.args[3].id)
            links[row.args[3].id].add(row.args[2].id)
    def reaches(start):
        seen, stack = {start}, [start]
        while stack:
            current = stack.pop()
            rep = records.get(current)
            if type(rep) is _Call and rep.kind in _SHAPE_REPRESENTATION_KINDS and any(
                    type(i) is _Ref and type(records.get(i.id)) is _Call and records[i.id].kind in _SOLID_KINDS
                    for i in rep.args[1]):
                return True
            for neighbor in links[current] - seen:
                seen.add(neighbor)
                stack.append(neighbor)
        return False
    out = {}
    for product, reps in product_reps.items():
        for rep_id in reps:
            rep = records.get(rep_id)
            if type(rep) is not _Call or rep.kind not in _SHAPE_REPRESENTATION_KINDS:
                continue
            for item in rep.args[1]:
                record = records.get(item.id) if type(item) is _Ref else None
                if type(record) is not _Call or record.kind != 'MAPPED_ITEM':
                    continue
                where = f'original mapped item #{item.id} of product #{product}'
                mapping = _refer(records, record.args[1], 'REPRESENTATION_MAP').args
                mapped = mapping[1].id
                mapped_rep = records.get(mapped)
                require(type(mapped_rep) is _Call and mapped_rep.kind in _SHAPE_REPRESENTATION_KINDS,
                        where+': mapped representation missing')
                held = [i.id for i in mapped_rep.args[1] if type(records.get(i.id)) is _Call and records[i.id].kind in _SOLID_KINDS]
                nested = [i.id for i in mapped_rep.args[1] if type(records.get(i.id)) is _Call and records[i.id].kind == 'MAPPED_ITEM']
                if not held and not nested:
                    # OCCT transfers the representations related to a mapped one
                    # with it (TransferRelatedSRR), so a solid reached that way
                    # is placed too; it is no occurrence this reading publishes.
                    require(not reaches(mapped), where+' maps a representation that reaches a solid through related '
                                                        'shape_representation_relationships, which is not read')
                    continue
                require(mapped not in sdr_used, where+' maps the representation of a product (bead .37.26)')
                require(not nested, where+': nested mapped items are not read')
                require(len(held) == 1, where+': its mapped representation holds several solids')
                origin, target = records.get(mapping[0].id), records.get(record.args[2].id)
                require(type(origin) is _Call and origin.kind == 'AXIS2_PLACEMENT_3D' and
                        type(target) is _Call and target.kind == 'AXIS2_PLACEMENT_3D',
                        where+': only an axis2_placement_3d origin and target are read')
                out[item.id] = (product, mapped, held[0], _frames_matrix(records, mapping[0], record.args[2]))
    return out


def qualify_original_reference(source, original_path, coordinate_tolerance=2.**-32):
    """The reference exporter's product, solid and usage graph against the STEP text.

    For a reference source (python/brepgpu/reference_source.py): every product
    is a PRODUCT_DEFINITION with its part name and reference, or a mapped
    representation of original_mapped_occurrences named as its holder; every
    usage is a NAUO placed as _original_usage_transforms reads it, or a mapped
    occurrence placed as original_mapped_occurrences reads it; each product's
    solid is the one its shape representation holds, not following its mapped
    occurrences. Returns the largest placement entry error.
    """
    records = original_records(original_path)
    mapped = original_mapped_occurrences(records)
    occurrence_reps = {m[1]: m for m in mapped.values()}
    names = {}
    for product in source.products:
        raw = records.get(product.id)
        if type(raw) is _Call and raw.kind == 'PRODUCT_DEFINITION':
            formation = _refer(records, raw.args[2], 'PRODUCT_DEFINITION_FORMATION')
            item = _refer(records, formation.args[2], 'PRODUCT').args
            names[product.id] = (item[2] or item[1], item[0])
    for product in source.products:
        if product.id in names:
            require((product.name, product.reference) == names[product.id], 'original product part name/reference differs')
        else:
            require(product.id in occurrence_reps, f'reference product #{product.id} is neither a product definition nor a mapped occurrence')
            require((product.name, product.reference) == names.get(occurrence_reps[product.id][0]),
                    'mapped occurrence product names differ from its holder')
            require(product.definition_id == occurrence_reps[product.id][2], 'mapped occurrence solid differs')
    representations = defaultdict(list)
    for row in records.values():
        if type(row) is _Call and row.kind == 'SHAPE_DEFINITION_REPRESENTATION':
            pds = _refer(records, row.args[0], 'PRODUCT_DEFINITION_SHAPE').args
            if type(pds[2]) is _Ref and pds[2].id in names:
                representations[pds[2].id].append(row.args[1].id)
    placing = _placing_relations(records)
    links = defaultdict(set)
    for id, row in records.items():
        if type(row) is _Call and row.kind == 'SHAPE_REPRESENTATION_RELATIONSHIP' and id not in placing:
            links[row.args[2].id].add(row.args[3].id)
            links[row.args[3].id].add(row.args[2].id)
    def solids(rep, seen):
        found = set()
        row = records.get(rep)
        if type(row) is _Call and row.kind in _SHAPE_REPRESENTATION_KINDS:
            for item in row.args[1]:
                record = records.get(item.id) if type(item) is _Ref else None
                if type(record) is _Call and record.kind in _SOLID_KINDS:
                    found.add(item.id)
        for neighbor in links[rep] - seen:
            found |= solids(neighbor, seen | {neighbor})
        return found
    for product in source.products:
        if product.id in names:
            held = set().union(*(solids(rep, {rep}) for rep in representations[product.id])) if representations[product.id] else set()
            require(len(held) <= 1, 'original product has ambiguous solid definitions')
            require(product.definition_id == next(iter(held), 0), 'original product to solid representation correspondence differs')
    transforms = _original_usage_transforms(records)
    require({u.id for u in source.usages} == set(transforms) | set(mapped), 'complete original usage and mapped occurrence inventory differs')
    maximum = 0.
    for usage in source.usages:
        if usage.id in mapped:
            holder, rep, _, matrix = mapped[usage.id]
            require((usage.parent_product_id, usage.child_product_id) == (holder, rep), 'original mapped occurrence parent/child differs')
        else:
            raw = records[usage.id]
            require((usage.parent_product_id, usage.child_product_id) == (raw.args[3].id, raw.args[4].id),
                    'original parent/child product reference differs')
            matrix = transforms[usage.id]
        maximum = max(maximum, _near(usage.transform, matrix, coordinate_tolerance, 'original usage placement differs'))
    return maximum


def qualify_original_placements(source, records_or_path, coordinate_tolerance=2.**-32):
    """Original two-frame placement and raw usage graph, independent of XDE."""
    records = original_records(records_or_path) if isinstance(records_or_path, (str, Path)) else records_or_path
    transforms = _original_usage_transforms(records)
    require(set(transforms) == {row.id for row in source.usages}, 'complete original usage transform inventory differs')
    maximum = 0.
    for usage in source.usages:
        raw = records[usage.id]
        require(raw.kind == 'NEXT_ASSEMBLY_USAGE_OCCURRENCE' and
                (usage.parent_product_id, usage.child_product_id) == (raw.args[3].id, raw.args[4].id),
                'original parent/child product reference differs')
        maximum = max(maximum, _near(usage.transform, transforms[usage.id], coordinate_tolerance,
                                     'original two-frame usage placement differs'))
    return maximum


def qualify_original_assembly(source, original_path, assembly, coordinate_tolerance=2.**-32):
    """All original root paths and composed frames, without calling assemble."""
    records = original_records(original_path)
    local = _original_usage_transforms(records)
    children, incoming = defaultdict(list), set()
    for id in local:
        raw = records[id]
        parent, child = raw.args[3].id, raw.args[4].id
        children[parent].append((id, child))
        incoming.add(child)
    roots = set(children) - incoming
    require(roots == {source.root_product_id} and assembly.root_product_id == source.root_product_id,
            'original source root assembly differs')
    for rows in children.values():
        rows.sort()
    products = {row.id: row for row in source.products}
    expected, active = [], set()
    identity = ((F(1),F(0),F(0),F(0)), (F(0),F(1),F(0),F(0)), (F(0),F(0),F(1),F(0)))
    def visit(product_id, path, parent):
        require(product_id not in active, 'original source assembly cycle')
        active.add(product_id)
        if product_id not in children:
            expected.append((path, product_id, products[product_id].definition_id, parent))
        else:
            for usage, child in children[product_id]:
                matrix = local[usage]
                # Exact rational composition of the independently reconstructed
                # original binary64 frames, using homogeneous coordinates.
                out = []
                for i in range(3):
                    row = []
                    for j in range(4):
                        value = sum(parent[i][k]*exact(matrix[4*k+j]) for k in range(3))
                        row.append(value + (parent[i][3] if j == 3 else 0))
                    out.append(tuple(row))
                visit(child, path+(usage,), tuple(out))
        active.remove(product_id)
    visit(source.root_product_id, (), identity)
    require(len(expected) == len(assembly.occurrences), 'complete original leaf occurrence inventory differs')
    maximum = 0.
    for index, (wanted, actual) in enumerate(zip(expected, assembly.occurrences), 1):
        path, product, definition, matrix = wanted
        require((actual.id, actual.path, actual.product_id, actual.definition_id) ==
                (index, path, product, definition), 'original occurrence full path/product/definition differs')
        maximum = max(maximum, _near(actual.transform, tuple(value for row in matrix for value in row),
                                     coordinate_tolerance, 'original composed occurrence placement differs'))
    require({row.definition_id for row in assembly.occurrences} == {row.id for row in source.definitions},
            'complete original solid definitions not present in assembly')
    return dict(occurrences=len(expected), definitions=len(source.definitions),
                max_original_composed_placement_error=maximum)


def qualify_source(source, original_path=None):
    """Check complete original identities and all typed topology references."""
    tables = {name: {row.id: row for row in getattr(source, name)}
              for name in ('products', 'definitions', 'usages', 'vertices', 'edges',
                           'coedges', 'loops', 'faces', 'carriers', 'curves')}
    for name, rows in tables.items():
        require(len(rows) == len(getattr(source, name)), 'duplicate source identity: ' + name)
        require(all(type(id) is int and id > 0 for id in rows), 'invalid source identity: ' + name)
    if original_path is not None:
        digest, original = original_inventory(original_path)
        require(source.source_sha256 == digest, 'original source bytes differ')
        kinds = {'products': ('PRODUCT_DEFINITION',),
                 'definitions': ('MANIFOLD_SOLID_BREP', 'BREP_WITH_VOIDS'),
                 'usages': ('NEXT_ASSEMBLY_USAGE_OCCURRENCE',), 'vertices': ('VERTEX_POINT',),
                 'edges': ('EDGE_CURVE',), 'coedges': ('ORIENTED_EDGE',), 'loops': ('EDGE_LOOP', 'VERTEX_LOOP'),
                 'faces': ('ADVANCED_FACE',)}
        for name, names in kinds.items():
            expected = set().union(*(original.get(kind, set()) for kind in names))
            require(set(tables[name]) == expected, 'complete original IDs: ' + name)
    require(source.root_product_id in tables['products'], 'original assembly root missing')
    for definition in source.definitions:
        require(definition.product_id in tables['products'], 'solid original product missing')
        for name, ids in (('faces', definition.face_ids), ('edges', definition.edge_ids),
                          ('vertices', definition.vertex_ids)):
            require(len(set(ids)) == len(ids) and set(ids) <= tables[name].keys(),
                    'definition source inventory: ' + name)
        require(all(tables['faces'][id].definition_id == definition.id for id in definition.face_ids),
                'original face definition owner differs')
    for face in source.faces:
        require(face.definition_id in tables['definitions'] and face.carrier_id in tables['carriers'],
                'original face carrier/definition missing')
        require(face.orientation in (-1, 1), 'original face orientation')
        require(face.loop_ids and len(set(face.loop_ids)) == len(face.loop_ids), 'complete distinct face loops')
        for id in face.loop_ids:
            require(id in tables['loops'] and tables['loops'][id].face_id == face.id,
                    'original loop face owner differs')
    coedge_owners = Counter()
    for loop in source.loops:
        require(loop.face_id in tables['faces'] and
                (bool(loop.coedge_ids) != bool(loop.vertex_id)) and
                (not loop.vertex_id or loop.vertex_id in tables['vertices']), 'original loop face/uses missing')
        for id in loop.coedge_ids:
            require(id in tables['coedges'] and tables['coedges'][id].loop_id == loop.id,
                    'original coedge loop owner differs')
            coedge_owners[id] += 1
    require(coedge_owners == Counter({id: 1 for id in tables['coedges']}),
            'every original coedge occurrence must belong once')
    for use in source.coedges:
        require(use.edge_id in tables['edges'] and use.curve_id in tables['curves'],
                'original coedge edge/pcurve missing')
        require(use.sense in (-1, 1), 'original coedge traversal sense')
        require(tables['curves'][use.curve_id].dimension == 2, 'original coedge requires 2D curve')
    for edge in source.edges:
        require((edge.degenerate and edge.curve_id == 0) or
                (edge.curve_id in tables['curves'] and tables['curves'][edge.curve_id].dimension == 3),
                'original edge spatial curve missing without declared degeneracy')
        require(len(edge.vertex_ids) == 2 and all(id in tables['vertices'] for id in edge.vertex_ids),
                'original edge endpoint missing')
        require(all(math.isfinite(x) for x in edge.parameter_range), 'finite original edge parameters')
        if not edge.degenerate:
            require((edge.parameter_range[1] > edge.parameter_range[0]) == edge.curve_same_sense,
                    'original directed edge range lost curve sense')
    return {name: len(rows) for name, rows in tables.items()}


def turn(a, b, c):
    return ((exact(b[0])-exact(a[0]))*(exact(c[1])-exact(a[1])) -
            (exact(b[1])-exact(a[1]))*(exact(c[0])-exact(a[0])))


def _chain_add(chain, a, b):
    if a == b:
        return
    if a < b:
        chain[a, b] += 1
    else:
        chain[b, a] -= 1


def qualify_domain(boundary_loops, triangles):
    """Full polygonal UV coverage by positive winding, independent of ear choices.

    Input loops and triangles contain actual 2D values in positively oriented
    source coordinates. Boundary loops must already be the source-derived valid
    simple outer/hole domain; this function checks its complete triangulation.
    """
    expected, observed = Counter(), Counter()
    for loop in boundary_loops:
        points = tuple(tuple(map(exact, p)) for p in loop)
        require(len(points) >= 3 and points[0] != points[-1], 'domain loop needs unclosed corner word')
        for a, b in zip(points, points[1:] + points[:1]):
            require(a != b, 'domain boundary UV edge collapses')
            _chain_add(expected, a, b)
    count = 0
    for triangle in triangles:
        points = tuple(tuple(map(exact, p)) for p in triangle)
        require(len(points) == 3 and turn(*points) > 0, 'triangle UV orientation/collapse differs')
        for a, b in zip(points, points[1:] + points[:1]):
            _chain_add(observed, a, b)
        count += 1
    require(count > 0, 'original polygonal domain has no triangles')
    require({edge: n for edge, n in observed.items() if n} ==
            {edge: n for edge, n in expected.items() if n},
            'positive complete triangle boundary chain differs from original domain')
    return count


def face_euler(source, face_id):
    """The Euler characteristic of one face's open region from its own topology
    (review round 4): chi = 2 - 2g - b - p over the face's closed region, with b its edge
    loops (boundary circles), p its vertex loops (points removed: each loop's vertex is
    counted with the definition's vertices) and g its genus: 1 for a face on a torus
    (carrier kind 5) whose edges include no seam, so it is the whole torus wrapped both
    ways (a natural torus, chi 0), else 0 (a band of two loops gives 0, a natural
    sphere 2). The disc formula 2 - loops is the g = 0 case."""
    face = source.face(face_id)
    loops = [source.loop(id) for id in face.loop_ids]
    edge_loops = sum(1 for loop in loops if loop.coedge_ids)
    vertex_loops = len(loops) - edge_loops
    edges = [source.coedge(id).edge_id for loop in loops for id in loop.coedge_ids]
    seam = len(edges) != len(set(edges))
    genus = 1 if source.carrier(face.carrier_id).kind == 5 and not seam else 0
    return 2 - 2*genus - edge_loops - vertex_loops


def _orient3d(a, b, c, d):
    """Sign of det[b-a, c-a, d-a], exact (rational) arithmetic."""
    bx, by, bz = b[0]-a[0], b[1]-a[1], b[2]-a[2]
    cx, cy, cz = c[0]-a[0], c[1]-a[1], c[2]-a[2]
    dx, dy, dz = d[0]-a[0], d[1]-a[1], d[2]-a[2]
    det = bx*(cy*dz-cz*dy) - by*(cx*dz-cz*dx) + bz*(cx*dy-cy*dx)
    return (det > 0) - (det < 0)


def _segment_meets_triangle(p, q, a, b, c):
    """Whether the closed segment pq meets the closed triangle abc (exact)."""
    sp, sq = _orient3d(a, b, c, p), _orient3d(a, b, c, q)
    if sp == sq != 0:
        return False
    if sp == sq == 0:
        # Coplanar: project out the dominant normal axis and test in the plane.
        n = cross(tuple(b[k]-a[k] for k in range(3)), tuple(c[k]-a[k] for k in range(3)))
        drop = max(range(3), key=lambda k: abs(n[k]))
        keep = [k for k in range(3) if k != drop]
        P, Q, A, B, C = (tuple(x[k] for k in keep) for x in (p, q, a, b, c))
        def turn2(o, u, v):
            d = (u[0]-o[0])*(v[1]-o[1]) - (u[1]-o[1])*(v[0]-o[0])
            return (d > 0) - (d < 0)
        def on(o, u, v):
            return turn2(o, u, v) == 0 and all(min(o[k], u[k]) <= v[k] <= max(o[k], u[k]) for k in range(2))
        def cross2(o, u, v, w):
            x, y, z, t = turn2(o, u, v), turn2(o, u, w), turn2(v, w, o), turn2(v, w, u)
            return (x*y < 0 and z*t < 0) or on(o, u, v) or on(o, u, w) or on(v, w, o) or on(v, w, u)
        def inside2(point):
            s1, s2, s3 = turn2(A, B, point), turn2(B, C, point), turn2(C, A, point)
            return (s1 >= 0 and s2 >= 0 and s3 >= 0) or (s1 <= 0 and s2 <= 0 and s3 <= 0)
        return (inside2(P) or inside2(Q) or cross2(P, Q, A, B) or cross2(P, Q, B, C) or cross2(P, Q, C, A))
    s1, s2, s3 = _orient3d(p, q, a, b), _orient3d(p, q, b, c), _orient3d(p, q, c, a)
    return (s1 >= 0 and s2 >= 0 and s3 >= 0) or (s1 <= 0 and s2 <= 0 and s3 <= 0)


def _triangles_meet(t, u):
    return (any(_segment_meets_triangle(t[i], t[(i+1) % 3], *u) for i in range(3)) or
            any(_segment_meets_triangle(u[i], u[(i+1) % 3], *t) for i in range(3)))


def _require_disjoint_shells(vertices, triangles, owner):
    """Every two triangles of different shells are disjoint: candidates share a cell of
    a uniform grid over their bounding boxes; each candidate pair is tested exactly."""
    boxes = []
    for triangle in triangles:
        ps = [tuple(map(float, vertices[key])) for key in triangle]
        boxes.append((tuple(min(p[k] for p in ps) for k in range(3)), tuple(max(p[k] for p in ps) for k in range(3))))
    lo = tuple(min(b[0][k] for b in boxes) for k in range(3))
    hi = tuple(max(b[1][k] for b in boxes) for k in range(3))
    span = max(hi[k]-lo[k] for k in range(3)) or 1.
    cells = max(1, min(256, round(len(triangles) ** (1/3))))
    size = span / cells * (1 + 1e-9)
    grid = defaultdict(list)
    for index, (a, b) in enumerate(boxes):
        r = [range(int((a[k]-lo[k])/size), int((b[k]-lo[k])/size)+1) for k in range(3)]
        for i in r[0]:
            for j in r[1]:
                for k in r[2]:
                    grid[i, j, k].append(index)
    seen = set()
    exact_points = {}
    def point(key):
        if key not in exact_points:
            exact_points[key] = tuple(map(exact, vertices[key]))
        return exact_points[key]
    for members in grid.values():
        for x in range(len(members)):
            for y in range(x+1, len(members)):
                i, j = members[x], members[y]
                if owner[i] == owner[j] or (i, j) in seen:
                    continue
                seen.add((i, j))
                a, b = boxes[i], boxes[j]
                if any(a[1][k] < b[0][k] or b[1][k] < a[0][k] for k in range(3)):
                    continue
                require(not _triangles_meet(tuple(point(key) for key in triangles[i]),
                                            tuple(point(key) for key in triangles[j])),
                        f'physical shells {owner[i]} and {owner[j]} intersect')


def qualify_physical_solid(vertices, triangles):
    """Closed orientable physical shells and vertex links, without product CSR: an outer
    shell enclosing a positive volume and any voids, negative and inside it."""
    incidence, links = defaultdict(list), defaultdict(list)
    for index, triangle in enumerate(triangles):
        require(len(triangle) == 3 and len(set(triangle)) == 3, 'physical triangle identity collapse')
        require(all(key in vertices for key in triangle), 'physical triangle references foreign vertex')
        a, b, c = (tuple(map(exact, vertices[key])) for key in triangle)
        normal = cross(tuple(b[k]-a[k] for k in range(3)), tuple(c[k]-a[k] for k in range(3)))
        require(any(normal), 'physical triangle area collapses')
        for j in range(3):
            a, b, c = triangle[j], triangle[(j+1) % 3], triangle[(j+2) % 3]
            incidence[min(a, b), max(a, b)].append((a, b, index))
            links[a].append((b, c))
    require(incidence, 'physical solid has no triangles')
    neighbors = defaultdict(set)
    for records in incidence.values():
        require(len(records) == 2 and records[0][:2] == records[1][:2][::-1] and
                records[0][2] != records[1][2], 'physical edge needs two opposite distinct uses')
        a, b = records[0][2], records[1][2]
        neighbors[a].add(b)
        neighbors[b].add(a)
    require(set(links) == set(vertices), 'physical solid has unused vertices')
    for members in links.values():
        nexts, indegree = {}, Counter()
        for a, b in members:
            require(a not in nexts, 'physical vertex link branches')
            nexts[a] = b
            indegree[b] += 1
        require(set(nexts) == set(indegree) and all(v == 1 for v in indegree.values()),
                'physical vertex link is not closed')
        start = next(iter(nexts))
        seen, cursor = set(), start
        while cursor not in seen:
            seen.add(cursor)
            cursor = nexts[cursor]
        require(cursor == start and len(seen) == len(nexts), 'physical vertex has pinched link cycles')
    # Review round 3 ruling, both arms and our owned meshes alike: the mesh is
    # split into its edge-connected shells, each closed and edge-consistent
    # (every edge used twice, once each way, checked above), and each shell's
    # signed volume has the sign its role requires: the outer shell (largest
    # by magnitude) positive, every other one a void, negative and inside the
    # outer shell by point containment. Euler is taken over all shells.
    shells, owner = [], {}
    for start in range(len(triangles)):
        if start in owner:
            continue
        members, pending = [], [start]
        owner[start] = len(shells)
        while pending:
            index = pending.pop()
            members.append(index)
            for other in neighbors[index]:
                if other not in owner:
                    owner[other] = len(shells)
                    pending.append(other)
        shells.append(members)
    points = {key: tuple(map(float, vertices[key])) for key in vertices}

    def volume(members):
        return math.fsum(
            a[0]*(b[1]*c[2]-b[2]*c[1]) - a[1]*(b[0]*c[2]-b[2]*c[0]) + a[2]*(b[0]*c[1]-b[1]*c[0])
            for a, b, c in (tuple(points[key] for key in triangles[i]) for i in members))/6
    volumes = [volume(members) for members in shells]
    require(all(math.isfinite(v) for v in volumes), 'physical shell mesh volume is not finite')
    outer = max(range(len(shells)), key=lambda k: abs(volumes[k]))
    require(volumes[outer] > 0,
            f'physical outer shell mesh volume {volumes[outer]:.6g} is not positive: the shell is inside out')

    def inside(point, members):
        # Ray parity against the outer shell's triangles, along three generic
        # directions; a strict majority decides (a ray grazing an edge counts twice).
        odd = 0
        for direction in ((0.5773502691896258, 0.4472135954999579, 0.6831300510639732),
                          (-0.3713906763541037, 0.7427813527082074, 0.5570860145311556),
                          (0.2672612419124244, -0.5345224838248488, 0.8017837257372732)):
            crossings = 0
            for i in members:
                a, b, c = (points[key] for key in triangles[i])
                e1 = tuple(b[k]-a[k] for k in range(3)); e2 = tuple(c[k]-a[k] for k in range(3))
                h = cross(direction, e2); det = sum(e1[k]*h[k] for k in range(3))
                if det == 0:
                    continue
                t0 = tuple(point[k]-a[k] for k in range(3))
                u = sum(t0[k]*h[k] for k in range(3))/det
                q = cross(t0, e1)
                v = sum(direction[k]*q[k] for k in range(3))/det
                t = sum(e2[k]*q[k] for k in range(3))/det
                if 0 <= u <= 1 and 0 <= v and u+v <= 1 and t > 0:
                    crossings += 1
            odd += crossings % 2
        return odd >= 2
    for k, members in enumerate(shells):
        if k != outer:
            require(volumes[k] < 0,
                    f'physical void shell mesh volume {volumes[k]:.6g} is not negative: the void is inside out')
    if len(shells) > 1:
        # Review round 4 ruling: shells are pairwise disjoint (an exact
        # triangle-triangle test on candidate pairs from a uniform grid), so one
        # vertex decides each containment: every void inside the outer shell,
        # and no void inside another void.
        _require_disjoint_shells(vertices, triangles, owner)
        for k, members in enumerate(shells):
            if k == outer:
                continue
            probe = points[triangles[members[0]][0]]
            require(inside(probe, shells[outer]), 'physical void shell is not inside the outer shell')
            for j, others in enumerate(shells):
                if j not in (k, outer):
                    require(not inside(probe, others), 'physical void shell lies inside another void shell')
    return dict(vertices=len(vertices), edges=len(incidence), triangles=len(triangles),
                euler=len(vertices)-len(incidence)+len(triangles), shells=len(shells))


def _on_segment(a, b, p):
    return turn(a, b, p) == 0 and all(min(a[k], b[k]) <= p[k] <= max(a[k], b[k]) for k in range(2))


def _intersect(a, b, c, d):
    x, y, z, w = turn(a, b, c), turn(a, b, d), turn(c, d, a), turn(c, d, b)
    return ((x*y < 0 and z*w < 0) or (x == 0 and _on_segment(a, b, c)) or
            (y == 0 and _on_segment(a, b, d)) or (z == 0 and _on_segment(c, d, a)) or
            (w == 0 and _on_segment(c, d, b)))


def _winding(loop, p):
    result = 0
    for a, b in zip(loop, loop[1:] + loop[:1]):
        require(not _on_segment(a, b, p), 'separate original trim loops touch')
        if a[1] <= p[1] < b[1] and turn(a, b, p) > 0:
            result += 1
        if b[1] <= p[1] < a[1] and turn(a, b, p) < 0:
            result -= 1
    return result


@dataclass
class _DomainSweepWork:
    segments: int = 0
    sweep_axis: int = 0
    projected_pairs: int = 0
    active_visits: int = 0
    box_candidates: int = 0
    exact_tests: int = 0
    max_active: int = 0


def _domain_box_candidates(segments, work):
    """Exact closed-box candidates; original endpoints remain the answer input."""
    ranks = tuple({value: rank for rank, value in enumerate(sorted(
        {point[axis] for _, _, a, b in segments for point in (a, b)}))}
        for axis in range(2))
    boxes = []
    for _, _, a, b in segments:
        x, u = ranks[0][a[0]], ranks[0][b[0]]
        y, v = ranks[1][a[1]], ranks[1][b[1]]
        boxes.append((min(x, u), max(x, u), min(y, v), max(y, v)))
    orders = tuple(sorted(range(len(boxes)), key=lambda i: (boxes[i][2*axis], i))
                   for axis in range(2))
    projected = []
    for axis, order in enumerate(orders):
        ends = []
        pairs = 0
        for index in order:
            low, high = boxes[index][2*axis:2*axis+2]
            while ends and ends[0] < low:
                heappop(ends)
            pairs += len(ends)
            heappush(ends, high)
        projected.append(pairs)
    axis = 0 if projected[0] <= projected[1] else 1
    if work is not None:
        work.segments = len(segments)
        work.sweep_axis = axis
        work.projected_pairs = projected[axis]
    active, ends = {}, []
    other = 2*(1-axis)
    for index in orders[axis]:
        low, high = boxes[index][2*axis:2*axis+2]
        while ends and ends[0][0] < low:
            _, expired = heappop(ends)
            del active[expired]
        ymin, ymax = boxes[index][other:other+2]
        for previous, box in active.items():
            if work is not None:
                work.active_visits += 1
            if ymax < box[other] or box[other+1] < ymin:
                continue
            if work is not None:
                work.box_candidates += 1
            # Sweep order can reverse source word order. Restore the original
            # ordering before applying the unchanged adjacent-edge exemption.
            yield (previous, index) if previous < index else (index, previous)
        active[index] = boxes[index]
        heappush(ends, (high, index))
        if work is not None:
            work.max_active = max(work.max_active, len(active))


def _simple_domains(loops, outer, *, work=None):
    if work is not None:
        work.segments = work.sweep_axis = work.projected_pairs = 0
        work.active_visits = work.box_candidates = work.exact_tests = work.max_active = 0
    segments = []
    for li, loop in enumerate(loops):
        require(len(loop) >= 3, 'original trim loop has too few UV corners')
        area2 = sum(a[0]*b[1]-a[1]*b[0] for a, b in zip(loop, loop[1:] + loop[:1]))
        require(area2 > 0 if outer[li] else area2 < 0, 'original outer/hole UV winding differs')
        for i, (a, b) in enumerate(zip(loop, loop[1:] + loop[:1])):
            require(a != b, 'original trim UV segment collapses')
            segments.append((li, i, a, b))
    for left, right in _domain_box_candidates(segments, work):
        li, i, a, b = segments[left]
        lj, j, c, d = segments[right]
        if li == lj and (j == i + 1 or (i == 0 and j == len(loops[li])-1)):
            continue
        if work is not None:
            work.exact_tests += 1
        require(not _intersect(a, b, c, d), 'original sampled trim domain crosses/touches')
    for li, loop in enumerate(loops):
        if not outer[li]:
            containers = sum(_winding(other, loop[0]) for lj, other in enumerate(loops) if outer[lj])
            require(containers == 1, 'original hole is not inside exactly one outer component')
            for lj, other in enumerate(loops):
                if li != lj and not outer[lj]:
                    require(_winding(other, loop[0]) == 0, 'original holes overlap or nest')


def _chart_closure(source, face, closure, coordinate_tolerance):
    require(closure.face_id == face.id and closure.loop_id in face.loop_ids,
            'chart closure original face/loop differs')
    loop = source.loop(closure.loop_id)
    word = loop.coedge_ids
    require(closure.incoming_coedge_id in word and closure.outgoing_coedge_id in word,
            'chart closure original incident coedge missing')
    index = word.index(closure.incoming_coedge_id)
    require(word[(index+1) % len(word)] == closure.outgoing_coedge_id,
            'chart closure must join adjacent original source uses')
    incoming, outgoing = source.coedge(closure.incoming_coedge_id), source.coedge(closure.outgoing_coedge_id)
    ie, oe = source.edge(incoming.edge_id), source.edge(outgoing.edge_id)
    require(ie.vertex_ids[1 if incoming.sense > 0 else 0] ==
            oe.vertex_ids[0 if outgoing.sense > 0 else 1] == closure.source_vertex_id,
            'chart closure changed original physical vertex identity')
    carrier = source.carrier(face.carrier_id)
    a, b = closure.uv_start, closure.uv_end
    require(len(a) == len(b) == 2 and all(math.isfinite(x) for x in (*a, *b)) and a != b,
            'finite nonzero chart closure UV span required')
    require(abs(a[0]-b[0]) <= 2*math.pi+32*math.ulp(2*math.pi), 'chart closure spans more than one period')
    if closure.kind == 'sphere_pole':
        require(carrier.kind == 4, 'sphere-pole closure needs original sphere')
        sign = 1 if a[1] >= 0 else -1
        target_v = sign*math.pi/2
        pole = _frame_point(carrier.frame, (0., 0., sign*carrier.radii[0]))
        forward = carrier.radii[0]*(max(abs(math.cos(a[1])), abs(math.cos(b[1]))) +
                                      max(abs(math.sin(a[1])-sign), abs(math.sin(b[1])-sign)))
    else:
        require(closure.kind == 'cone_apex' and carrier.kind == 3, 'unsupported analytic chart closure kind')
        target_v = -carrier.radii[0]/math.sin(carrier.radii[1])
        pole = _frame_point(carrier.frame, (0., 0., target_v*math.cos(carrier.radii[1])))
        forward = max(abs(v-target_v) for v in (a[1], b[1]))
    require(max(abs(v-target_v) for v in (a[1], b[1])) <= 2.**-44*max(1., abs(target_v)),
            'chart closure is not at the original analytic singular latitude')
    require(forward <= coordinate_tolerance and
            math.dist(source.vertex(closure.source_vertex_id).xyz, pole) <= coordinate_tolerance,
            'original physical vertex/whole UV closure is not the declared analytic pole')
    return (incoming.id, ie.parameter_range[1 if incoming.sense > 0 else 0],
            outgoing.id, oe.parameter_range[0 if outgoing.sense > 0 else 1])


_DUAL_FRAMES = {}


def _dual_frame(frame):
    """Exact Gauss-Jordan inverse of the saved axis columns."""
    if frame not in _DUAL_FRAMES:
        rows = [[exact(frame[3+3*j+i]) for j in range(3)]+[F(i==j) for j in range(3)] for i in range(3)]
        for column in range(3):
            pivot = next((i for i in range(column,3) if rows[i][column]),None)
            require(pivot is not None,'original analytic frame is singular')
            rows[column],rows[pivot] = rows[pivot],rows[column]
            divisor = rows[column][column]
            rows[column] = [x/divisor for x in rows[column]]
            for i in range(3):
                if i != column:
                    factor = rows[i][column]
                    rows[i] = [a-factor*b for a,b in zip(rows[i],rows[column])]
        _DUAL_FRAMES[frame] = tuple(tuple(row[3:]) for row in rows)
    return _DUAL_FRAMES[frame]


def _source_local(carrier,xyz,*,rational=False):
    dual = _dual_frame(carrier.frame)
    if rational:
        delta = tuple(exact(a)-exact(b) for a,b in zip(xyz,carrier.frame[:3]))
        return tuple(sum((a*b for a,b in zip(row,delta)),F(0)) for row in dual)
    delta = tuple(float(a)-b for a,b in zip(xyz,carrier.frame[:3]))
    return tuple(math.fsum(float(a)*b for a,b in zip(row,delta)) for row in dual)


def _analytic_inverse(carrier, xyz, branch_seed):
    require(carrier.kind in (1,2,3),'unsupported owned analytic trim inverse')
    if carrier.kind == 1:
        return _source_local(carrier,xyz,rational=True)[:2]
    require(carrier.radii[0] > 0 if carrier.kind == 2 else carrier.radii[0] >= 0,
            'owned angle inverse has invalid original reference radius')
    coordinates = _source_local(carrier,xyz)
    require(all(math.isfinite(x) for x in coordinates) and math.hypot(*coordinates[:2]) > 0,
            'owned angular inverse has invalid radial projection')
    v = coordinates[2]
    angle = math.atan2(coordinates[1],coordinates[0])
    if carrier.kind == 3:
        cosine = math.cos(carrier.radii[1])
        require(math.isfinite(cosine) and cosine != 0,'owned cone has singular axial conversion')
        v /= cosine
        radius = carrier.radii[0]+v*math.sin(carrier.radii[1])
        require(radius != 0,'owned regular cone inverse cannot invent apex branch')
        if radius < 0:
            angle += math.pi
    angle += 2*math.pi*round((float(branch_seed)-angle)/(2*math.pi))
    require(abs(angle-float(branch_seed)) < math.pi, 'owned angular inverse has ambiguous original branch')
    return angle,v


def _cone_generator_inverse(source,carrier,edge,physical,branch_seed,quality,coordinate_tolerance):
    require(carrier.kind == 3 and source.curve(edge.curve_id).kind == 1,
            'owned cone generator must retain an original LINE')
    curve = source.curve(edge.curve_id)
    midpoint = float(sum(map(exact,edge.parameter_range))/2)
    u,_ = _analytic_inverse(carrier,curve_point_float(curve,midpoint),branch_seed)
    cosine = math.cos(carrier.radii[1])
    for t in edge.parameter_range:
        xyz = curve_point_float(curve,t)
        v = _source_local(carrier,xyz)[2]/cosine
        require(math.dist(xyz,carrier_jet_float(carrier,u,v)[0]) <= quality.source_residual_mm,
                'original LINE whole-edge cone conversion exceeds endpoint residual bound')
    if physical.source_kind == 1:
        apex_v = -carrier.radii[0]/math.sin(carrier.radii[1])
        apex = _frame_point(carrier.frame,(0.,0.,apex_v*cosine))
        if math.dist(physical.xyz,apex) <= coordinate_tolerance:
            require(physical.source_id in edge.vertex_ids,'cone apex is not original incident source vertex')
            v = _source_local(carrier,physical.xyz)[2]/cosine
            require(abs(v-apex_v) <= 2.**-40*max(1.,abs(apex_v)),
                    'original cone apex fails explicit slant-coordinate grade')
            return u,apex_v
        raise AssertionError('cone generator endpoint kind requires verified original apex')
    return u,_source_local(carrier,physical.xyz)[2]/cosine


def _spline_column_uv(column,xyz):
    u,p,d,v0,dv = column
    delta = tuple(exact(a)-b for a,b in zip(xyz,p))
    scale = sum((a*b for a,b in zip(delta,d)),F(0))/sum((x*x for x in d),F(0))
    return u,v0+dv*scale


def _spline_boundary_profile(source,carrier,use,profiles):
    key = (carrier.id,use.id)
    if key in profiles:
        return profiles[key]
    edge = source.edge(use.edge_id)
    curve = source.curve(edge.curve_id)
    require(carrier.kind == 6 and carrier.degrees[1] == 1 and carrier.control_shape[1] == 2 and
            len(carrier.knot_v) == 2 and carrier.mult_v == (2,2) and
            carrier.mult_u[0] == carrier.mult_u[-1] == carrier.degrees[0]+1,
            'natural spline boundary requires original clamped U and linear clamped V')
    require(curve.kind == 1 and curve.dimension == 3 and use.periodic_shift == (0,0),
            'natural spline boundary must retain original nonperiodic spatial LINE')
    guide = source.curve(use.curve_id)
    if guide.kind == 1:
        values = tuple(exact(guide.frame[0])+exact(guide.frame[3])*exact(s) for s in use.parameter_range)
    else:
        require(guide.kind in (4,5) and guide.dimension == 2,'unsupported natural-boundary branch guide')
        homogeneous = tuple(tuple(map(exact,guide.controls[i:i+3])) for i in range(0,len(guide.controls),3))
        require(homogeneous and all(row[2] > 0 for row in homogeneous),'natural-boundary guide needs positive weights')
        values = tuple(row[0]/row[2] for row in homogeneous)
    hull = min(values),max(values)
    half = (exact(carrier.knot_u[-1])-exact(carrier.knot_u[0]))/2
    candidates = []
    for index,u in ((0,carrier.knot_u[0]),(carrier.control_shape[0]-1,carrier.knot_u[-1])):
        h0,h1 = (tuple(map(exact,carrier.controls[8*index+4*j:8*index+4*j+4])) for j in (0,1))
        if h0[3] <= 0 or h0[3] != h1[3]:
            continue
        p,q = tuple(x/h0[3] for x in h0[:3]),tuple(x/h1[3] for x in h1[:3])
        d = tuple(b-a for a,b in zip(p,q))
        if not any(d):
            continue
        origin = tuple(map(exact,curve.frame[:3]));direction = tuple(map(exact,curve.frame[3:6]))
        if not any(direction) or any(cross(tuple(a-b for a,b in zip(origin,p)),d)) or any(cross(direction,d)):
            continue
        column = exact(u),p,d,exact(carrier.knot_v[0]),exact(carrier.knot_v[-1])-exact(carrier.knot_v[0])
        points = tuple(tuple(origin[k]+direction[k]*exact(t) for k in range(3)) for t in edge.parameter_range)
        canonical = tuple(source.vertex(v).xyz for v in edge.vertex_ids)
        if any(any(cross(tuple(exact(a)-b for a,b in zip(xyz,p)),d)) for xyz in canonical):
            continue
        if any(not exact(carrier.knot_v[0]) <= _spline_column_uv(column,xyz)[1] <= exact(carrier.knot_v[-1])
               for xyz in (*points,*canonical)):
            continue
        if all(abs(bound-exact(u)) < half for bound in hull):
            candidates.append(column)
    require(len(candidates) == 1,'natural spline boundary lacks unique original LINE/column/whole-guide proof')
    profiles[key] = candidates[0]
    return candidates[0]


def _spline_vertex_uv(source,face,use,vertex_id,seed,profiles):
    loop = source.loop(use.loop_id)
    position = loop.coedge_ids.index(use.id)
    adjacent = (use,source.coedge(loop.coedge_ids[(position-1)%len(loop.coedge_ids)]),
                source.coedge(loop.coedge_ids[(position+1)%len(loop.coedge_ids)]))
    recovered = set()
    for candidate in adjacent:
        edge = source.edge(candidate.edge_id)
        if vertex_id not in edge.vertex_ids or source.curve(edge.curve_id).kind != 1:
            continue
        if candidate.id != use.id:
            # Membership must be this exact original traversal joint.
            if candidate.id == adjacent[1].id:
                incident = (edge.vertex_ids[1 if candidate.sense > 0 else 0],
                            source.edge(use.edge_id).vertex_ids[0 if use.sense > 0 else 1])
            else:
                incident = (source.edge(use.edge_id).vertex_ids[1 if use.sense > 0 else 0],
                            edge.vertex_ids[0 if candidate.sense > 0 else 1])
            if incident != (vertex_id,vertex_id):
                continue
        try:
            column = _spline_boundary_profile(source,source.carrier(face.carrier_id),candidate,profiles)
        except AssertionError:
            continue
        uv = _spline_column_uv(column,source.vertex(vertex_id).xyz)
        span = exact(source.carrier(face.carrier_id).knot_u[-1])-exact(source.carrier(face.carrier_id).knot_u[0])
        if abs(uv[0]-exact(seed)) < span/2:
            recovered.add(uv)
    require(len(recovered) == 1,'spline endpoint recovery lacks same-vertex adjacent original LINE proof')
    return recovered.pop()


def _spline_joint_candidate(source,face,recovery,quality,coordinate_tolerance):
    """Check the two retained source endpoints; never project or reuse product math."""
    carrier=source.carrier(face.carrier_id)
    require(carrier.kind==6,'finite spline endpoint candidate needs original spline carrier')
    for degree,multiplicities in zip(carrier.degrees,(carrier.mult_u,carrier.mult_v)):
        require(multiplicities[0]==multiplicities[-1]==degree+1,
                'spline endpoint candidate requires original clamped chart')
    occurrences=(recovery.incoming,recovery.outgoing)
    candidates=tuple(tuple(map(exact,o.uv)) for o in occurrences)
    for occurrence,uv in zip(occurrences,candidates):
        use=source.coedge(occurrence.coedge_id);edge=source.edge(use.edge_id)
        require(use.periodic_shift==(0,0) and use.parameter_map_kind==1 and edge.same_parameter,
                'spline endpoint candidate changed original chart/identity map')
        require(all(exact(knots[0])<=value<=exact(knots[-1]) for value,knots in
                    zip(uv,(carrier.knot_u,carrier.knot_v))),
                'spline endpoint candidate leaves original knot domain')
    require(any(exact(lo)<=candidates[0][0]<=exact(hi) and
                exact(lo)<=candidates[1][0]<=exact(hi)
                for lo,hi in zip(carrier.knot_u,carrier.knot_u[1:])),
            'spline endpoint candidates cross original U knot cells')
    grade=128*math.ulp(max(1.,abs(carrier.knot_v[0]),abs(carrier.knot_v[-1])))
    require(any(all(abs(uv[1]-exact(endpoint))<=exact(grade) for uv in candidates)
                for endpoint in (carrier.knot_v[0],carrier.knot_v[-1])),
            'spline endpoint candidates lack same original natural V boundary')
    require(math.dist(*[tuple(map(float,uv)) for uv in candidates])>
            2.**-32*max(1.,*(abs(x) for x in carrier.uv_bounds)),
            'spline endpoint candidate was not a non-small original joint')
    chosen=tuple(map(exact,recovery.uv))
    require(chosen in candidates,'spline endpoint choice invented a new UV')
    vertex=source.vertex(recovery.source_vertex_id)
    points=tuple(carrier_jet_float(carrier,*uv)[0] for uv in candidates)
    residuals=tuple(math.dist(p,vertex.xyz) for p in points)
    for occurrence,point in zip(occurrences,points):
        edge=source.edge(occurrence.edge_id)
        original=curve_point_float(source.curve(edge.curve_id),occurrence.source_parameter)
        require(math.isfinite(math.dist(point,original)) and
                math.dist(point,original)<=quality.source_residual_mm+coordinate_tolerance,
                'spline endpoint raw carrier/curve residual exceeds selected source gate')
    selected=residuals[candidates.index(chosen)]
    require(math.isfinite(selected) and selected<=min(residuals)+coordinate_tolerance,
            'spline endpoint choice did not select a minimum retained candidate')
    require(selected<=quality.source_residual_mm+coordinate_tolerance,
            'spline endpoint chosen carrier/vertex residual exceeds selected source gate')
    selected_point=points[candidates.index(chosen)]
    require(all(math.dist(p,selected_point)<=quality.tolerance_mm/8+coordinate_tolerance for p in points),
            'spline endpoint source displacement exceeds trim allocation')
    return chosen


def _source_joint_key(source,loop,use,parameter):
    edge=source.edge(use.edge_id);index=loop.coedge_ids.index(use.id)
    start=edge.parameter_range[0 if use.sense>0 else 1]
    if parameter==start:
        return loop.id,loop.coedge_ids[(index-1)%len(loop.coedge_ids)],use.id
    require(parameter==edge.parameter_range[1 if use.sense>0 else 0],
            'spline original vertex is not a directed source endpoint')
    return loop.id,use.id,loop.coedge_ids[(index+1)%len(loop.coedge_ids)]


def _joint_recovery(source,face,recovery,vertices,quality,coordinate_tolerance,boundary_profiles):
    inverse_kinds = {1:'plane_vertex_inverse',2:'cylinder_vertex_inverse',3:'cone_vertex_inverse',
                     6:'spline_boundary_vertex_inverse'}
    carrier = source.carrier(face.carrier_id)
    candidate_joint=carrier.kind==6 and recovery.kind=='spline_joint_endpoint_candidate'
    require((recovery.kind == inverse_kinds.get(carrier.kind) or candidate_joint) and recovery.face_id == face.id and
            recovery.loop_id in face.loop_ids, 'unsupported/foreign original-vertex joint recovery')
    loop = source.loop(recovery.loop_id)
    a,b = recovery.incoming,recovery.outgoing
    require(a.coedge_id in loop.coedge_ids and b.coedge_id in loop.coedge_ids and
            loop.coedge_ids[(loop.coedge_ids.index(a.coedge_id)+1)%len(loop.coedge_ids)] == b.coedge_id,
            'joint recovery changed original incident-use order')
    vertex = source.vertex(recovery.source_vertex_id)
    for occurrence,incoming in ((a,True),(b,False)):
        use = source.coedge(occurrence.coedge_id)
        edge = source.edge(use.edge_id)
        endpoint = (1 if use.sense > 0 else 0) if incoming else (0 if use.sense > 0 else 1)
        require(edge.vertex_ids[endpoint] == vertex.id and occurrence.edge_id == edge.id and
                occurrence.source_parameter == edge.parameter_range[endpoint],
                'joint recovery changed original endpoint identity or directed parameter')
        physical = vertices.get((face.definition_id,occurrence.sample_key))
        require(physical is not None and physical.source_kind == 1 and physical.source_id == vertex.id,
                'joint recovery is not the same original physical source vertex')
        require(getattr(occurrence,'recovery_kind','pcurve') == 'pcurve', 'joint recovery must retain pre-inverse occurrence')
        raw = curve_point_float(source.curve(use.curve_id),occurrence.pcurve_parameter)
        _near(occurrence.pcurve_uv,raw,coordinate_tolerance,'joint recovery lost raw recovered pcurve UV')
        shifted = (raw[0]+use.periodic_shift[0]*2*math.pi if carrier.kind in (2,3,4,5) else raw[0],raw[1])
        _near(occurrence.uv,shifted,coordinate_tolerance,'joint recovery changed pre-inverse branch')
        if use.parameter_map_kind == 1:
            require(occurrence.pcurve_parameter == occurrence.source_parameter,
                    'joint recovery changed identity parameter correspondence')
        endpoint_xyz = curve_point_float(source.curve(edge.curve_id),occurrence.source_parameter)
        require(math.dist(endpoint_xyz,vertex.xyz) <= quality.source_residual_mm,
                'original joint edge/vertex incidence exceeds selected source residual')
        if candidate_joint:
            continue
        expected = (_spline_vertex_uv(source,face,use,vertex.id,shifted[0],boundary_profiles) if carrier.kind == 6
                    else _analytic_inverse(carrier,vertex.xyz,shifted[0]))
        if carrier.kind in (1,6):
            require(tuple(map(exact,recovery.uv)) == expected,'plane joint lost exact source affine inverse')
        else:
            _near(recovery.uv,expected,coordinate_tolerance,'canonical joint is not original-vertex analytic inverse')
    require(a.sample_key == b.sample_key, 'joint recovery merged different physical source keys')
    if candidate_joint:
        _spline_joint_candidate(source,face,recovery,quality,coordinate_tolerance)
    corrected = carrier_jet_float(carrier,*recovery.uv)[0]
    require(math.dist(corrected,vertex.xyz) <= quality.source_residual_mm,
            'corrected original-vertex joint exceeds selected source residual')
    return recovery.loop_id,a.coedge_id,b.coedge_id


def qualify_mesh(mesh, *, original_path=None, coordinate_tolerance=2.**-32):
    """Actual full source→polygonal domains→closed solids, no triangulator reuse.

    Finite source probes are reported as measured residuals. They supplement
    source/control-hull bounds owned by the mesher; they are not a claimed global
    approximation certificate.
    """
    source = mesh.source
    source_counts = qualify_source(source, original_path)
    faces = {row.id: row for row in source.faces}
    definitions = {row.id: row for row in source.definitions}
    edge_by_id = {row.id: row for row in source.edges}
    uses = {row.id: row for row in source.coedges}
    vertices = {}
    max_vertex_error = max_uv_error = max_recovery_residual = max_probe_error = 0.
    max_original_edge_endpoint_departure=0.
    max_rounding_bound, rounding_bounds = 0., {}
    cylinder_recovery_count = joint_recovery_count = 0
    max_foreign_cylinder_residual = 0.
    owned_recovery_counts = Counter()
    boundary_profiles = {}
    spline_foreign_outside_count = 0
    for vertex in mesh.vertices:
        key = (vertex.definition_id, vertex.physical_key)
        require(key not in vertices and vertex.definition_id in definitions, 'duplicate/foreign physical source key')
        require(len(vertex.xyz) == 3 and all(math.isfinite(x) for x in vertex.xyz), 'finite actual physical XYZ')
        if vertex.source_kind == 1:
            require(vertex.source_id in definitions[vertex.definition_id].vertex_ids, 'foreign original vertex generator')
            expected = source.vertex(vertex.source_id).xyz
        elif vertex.source_kind == 2:
            require(vertex.source_id in definitions[vertex.definition_id].edge_ids, 'foreign original edge generator')
            edge = source.edge(vertex.source_id)
            require(not edge.degenerate and edge.curve_id, 'unsupported degenerate spatial-edge sample')
            expected = curve_point_float(source.curve(edge.curve_id), vertex.source_parameter)
        else:
            require(vertex.source_kind == 3 and vertex.source_id in definitions[vertex.definition_id].face_ids,
                    'foreign original face generator')
            face = source.face(vertex.source_id)
            expected = carrier_jet_float(source.carrier(face.carrier_id), *vertex.uv)[0]
        max_vertex_error = max(max_vertex_error, _near(vertex.xyz, expected, coordinate_tolerance,
                                                       'actual source generator coordinates differ'))
        vertices[key] = vertex
    samples = {row.edge_id: row for row in mesh.edge_samples}
    require(len(samples) == len(mesh.edge_samples) and set(samples) == set(edge_by_id),
            'complete original edge sample inventory differs')
    for edge in source.edges:
        sample = samples[edge.id]
        ts = sample.parameters
        require(len(ts) == len(sample.vertex_keys) == len(sample.positions) and len(ts) >= 2,
                'complete physical edge parameter/position chain differs')
        require(ts[0] == edge.parameter_range[0] and ts[-1] == edge.parameter_range[1],
                'original physical edge interval endpoints differ')
        direction = 1 if ts[-1] > ts[0] else -1
        require(all(direction*(b-a) > 0 for a, b in zip(ts, ts[1:])) or
                (edge.degenerate and all(t == ts[0] for t in ts)),
                'original physical edge parameter order differs')
        for index, (t, xyz) in enumerate(zip(ts, sample.positions)):
            if index in (0, len(ts)-1):
                expected = source.vertex(edge.vertex_ids[0 if index == 0 else 1]).xyz
                if edge.curve_id:
                    original=curve_point_float(source.curve(edge.curve_id),t)
                    departure=math.dist(xyz,original)
                    require(math.isfinite(departure) and departure<=
                            min(mesh.quality.tolerance_mm/8,mesh.quality.source_residual_mm)+coordinate_tolerance,
                            'actual original spatial endpoint departure exceeds source/chord allocation')
                    max_original_edge_endpoint_departure=max(max_original_edge_endpoint_departure,departure)
            elif edge.degenerate:
                expected = source.vertex(edge.vertex_ids[0]).xyz
            else:
                expected = curve_point_float(source.curve(edge.curve_id), t)
            max_vertex_error = max(max_vertex_error, _near(xyz, expected, coordinate_tolerance,
                                                           'original shared edge sample differs'))
    face_meshes = {row.face_id: row for row in mesh.faces}
    require(len(face_meshes) == len(mesh.faces) and set(face_meshes) == set(faces),
            'complete original face domain inventory differs')
    triangle_owners = Counter()
    physical_by_definition = defaultdict(list)
    collapsed_count = 0
    for face_id, face in faces.items():
        domain = face_meshes[face_id]
        carrier = source.carrier(face.carrier_id)
        require(len(domain.boundary_loops) == len(face.loop_ids), 'complete original face loop inventory differs')
        normalized_loops = []
        outer = []
        actual_loop_ids = []
        boundary_corners = set()
        closures = getattr(domain, 'chart_closures', ())
        closure_parameters = {}
        joint_recoveries = {}
        candidate_records_seen=set()
        for recovery in getattr(domain,'joint_recoveries',()):
            recovery_key = _joint_recovery(source,face,recovery,vertices,mesh.quality,coordinate_tolerance,boundary_profiles)
            require(recovery_key not in joint_recoveries, 'duplicate original-vertex joint recovery')
            joint_recoveries[recovery_key] = recovery
        joint_recovery_count += len(joint_recoveries)
        supported_poles = {v for loop_id in face.loop_ids for cid in source.loop(loop_id).coedge_ids
                           for edge in (source.edge(source.coedge(cid).edge_id),) if edge.degenerate
                           for v in edge.vertex_ids}
        for closure in closures:
            require((closure.loop_id, closure.incoming_coedge_id) not in closure_parameters,
                    'duplicate declared chart closure')
            closure_parameters[closure.loop_id, closure.incoming_coedge_id] = _chart_closure(
                source, face, closure, coordinate_tolerance)
            supported_poles.add(closure.source_vertex_id)
        for chain in domain.boundary_loops:
            require(chain and chain[0].coedge_id in uses, 'face boundary loop is empty/foreign')
            loop_id = uses[chain[0].coedge_id].loop_id
            require(loop_id in face.loop_ids, 'foreign original face loop')
            actual_loop_ids.append(loop_id)
            loop = source.loop(loop_id)
            seen_uses = []
            actual_occurrences = Counter()
            normalized = []
            for occurrence in chain:
                require(occurrence.coedge_id in loop.coedge_ids, 'foreign boundary occurrence')
                use = uses[occurrence.coedge_id]
                require(use.edge_id == occurrence.edge_id, 'boundary original physical edge differs')
                if not seen_uses or seen_uses[-1] != use.id:
                    seen_uses.append(use.id)
                edge = edge_by_id[use.edge_id]
                sample = samples[edge.id]
                require((occurrence.source_parameter, occurrence.sample_key) in
                        tuple(zip(sample.parameters, sample.vertex_keys)),
                        'boundary original parameter/physical key differs')
                expected_uv = curve_point_float(source.curve(use.curve_id), occurrence.pcurve_parameter)
                raw_uv = getattr(occurrence, 'pcurve_uv', ()) or occurrence.uv
                max_uv_error = max(max_uv_error, _near(raw_uv, expected_uv, coordinate_tolerance,
                                                       'original recovered pcurve UV differs'))
                periods = (2*math.pi if carrier.kind in (2,3,4,5) else 0.,
                           2*math.pi if carrier.kind == 5 else 0.)
                expected_uv = tuple(float(x) + shift*period for x, shift, period in
                                    zip(expected_uv, use.periodic_shift, periods))
                recovery_kind = getattr(occurrence,'recovery_kind','pcurve')
                inverse_kinds = {1:'plane_source_inverse',2:'cylinder_source_inverse',3:'cone_source_inverse'}
                require(recovery_kind == 'pcurve' or recovery_kind == inverse_kinds.get(carrier.kind) or
                        (carrier.kind == 3 and recovery_kind == 'cone_source_generator') or
                        (carrier.kind == 6 and recovery_kind in ('spline_boundary_generator','spline_boundary_vertex_inverse',
                                                                'spline_joint_endpoint_candidate')),
                        'unsupported owned trim recovery kind')
                if recovery_kind != 'pcurve':
                    physical = vertices.get((face.definition_id,occurrence.sample_key))
                    require(physical is not None and physical.source_kind in (1,2),
                            'owned trim inverse lost original spatial generator')
                    if recovery_kind == 'spline_joint_endpoint_candidate':
                        require(physical.source_kind==1 and physical.source_id in edge.vertex_ids,
                                'spline endpoint candidate is not an original vertex occurrence')
                        joint_key=_source_joint_key(source,loop,use,occurrence.source_parameter)
                        require(joint_key in joint_recoveries and
                                joint_recoveries[joint_key].kind==recovery_kind,
                                'spline endpoint candidate lost matching original joint record')
                        recovered=tuple(map(exact,joint_recoveries[joint_key].uv))
                        candidate_records_seen.add(joint_key)
                    elif recovery_kind == 'spline_boundary_generator':
                        column = _spline_boundary_profile(source,carrier,use,boundary_profiles)
                        recovered = _spline_column_uv(column,physical.xyz)
                        if physical.source_kind == 1:
                            require(physical.source_id in edge.vertex_ids,'spline generator lost original endpoint vertex')
                        else:
                            require(physical.source_id == edge.id and physical.source_parameter == occurrence.source_parameter,
                                    'spline generator changed original spatial LINE parameter identity')
                    elif recovery_kind == 'spline_boundary_vertex_inverse':
                        require(physical.source_kind == 1 and source.curve(edge.curve_id).kind != 1 and
                                physical.source_id in edge.vertex_ids,
                                'spline neighboring endpoint lost original non-LINE vertex incidence')
                        recovered = _spline_vertex_uv(source,face,use,physical.source_id,expected_uv[0],boundary_profiles)
                    elif recovery_kind == 'cone_source_generator':
                        recovered = _cone_generator_inverse(source,carrier,edge,physical,expected_uv[0],
                                                            mesh.quality,coordinate_tolerance)
                    else:
                        recovered = _analytic_inverse(carrier,physical.xyz,expected_uv[0])
                    if carrier.kind in (1,6):
                        require(tuple(map(exact,occurrence.uv)) == recovered,'owned affine trim lost exact source inverse')
                    else:
                        _near(occurrence.uv,recovered,coordinate_tolerance,
                              'owned trim is not original-source analytic inverse')
                    if recovery_kind == 'cone_source_generator' and physical.source_kind == 1:
                        require(float(occurrence.uv[1]) == recovered[1],
                                'verified original cone apex lost canonical analytic V/limiting-normal semantics')
                    owned_recovery_counts[recovery_kind] += 1
                    cylinder_recovery_count += carrier.kind == 2
                    if carrier.kind == 2:
                        raw_point = carrier_jet_float(carrier,*expected_uv)[0]
                        max_foreign_cylinder_residual = max(max_foreign_cylinder_residual,math.dist(raw_point,physical.xyz))
                    if carrier.kind == 6:
                        if any(not knots[0] <= value <= knots[-1] for value,knots in zip(expected_uv,(carrier.knot_u,carrier.knot_v))):
                            spline_foreign_outside_count += 1
                        if physical.source_kind == 1:
                            joint_key=_source_joint_key(source,loop,use,occurrence.source_parameter)
                            require(joint_key in joint_recoveries,'owned spline endpoint lost explicit original joint provenance')
                    # A non-small original-vertex joint must retain both raw
                    # endpoint occurrences in original traversal order.
                    start_index = 0 if use.sense > 0 else 1
                    if occurrence.source_parameter == edge.parameter_range[start_index] and physical.source_kind == 1:
                        previous = source.coedge(loop.coedge_ids[(loop.coedge_ids.index(use.id)-1)%len(loop.coedge_ids)])
                        previous_edge = source.edge(previous.edge_id)
                        end_index = 1 if previous.sense > 0 else 0
                        recovery_key = (loop.id,previous.id,use.id)
                        if previous.parameter_map_kind == 1:
                            previous_uv = curve_point_float(source.curve(previous.curve_id),previous_edge.parameter_range[end_index])
                            previous_uv = (previous_uv[0]+previous.periodic_shift[0]*periods[0],previous_uv[1])
                            joint_grade = coordinate_tolerance*max(1.,*(abs(x) for x in (*carrier.uv_bounds,*expected_uv)))
                            if (math.dist(previous_uv,expected_uv) > joint_grade and
                                    (loop.id,previous.id) not in closure_parameters):
                                require(recovery_key in joint_recoveries,'non-small source joint lost explicit inverse provenance')
                        if recovery_key in joint_recoveries:
                            _near(occurrence.uv,joint_recoveries[recovery_key].uv,coordinate_tolerance,
                                  'boundary joint differs from retained original-vertex recovery')
                if carrier.kind == 6:
                    # Source STEP supplies no pcurves. A reconstructed UV may
                    # overhang an ORIGINAL global knot endpoint by roundoff;
                    # only the reviewed fixed-ULP recovery is admitted.
                    for axis,knots in enumerate((carrier.knot_u,carrier.knot_v)):
                        lo,hi = knots[0],knots[-1]
                        recovered = expected_uv[axis]
                        require(lo <= exact(occurrence.uv[axis]) <= hi,
                                'actual original spline boundary leaves global source domain')
                        if not lo <= recovered <= hi and recovery_kind == 'pcurve':
                            endpoint = lo if recovered < lo else hi
                            grade = 128*math.ulp(max(1.,abs(lo),abs(hi)))
                            require(abs(recovered-endpoint) <= grade and
                                    abs(exact(occurrence.uv[axis])-exact(recovered)) <= exact(grade),
                                    'foreign recovered UV overhang exceeds explicit global-end grade')
                uv_grade = coordinate_tolerance*max(1., *(abs(x) for x in (*carrier.uv_bounds, *expected_uv)))
                if recovery_kind == 'pcurve':
                    require(max(abs(float(a)-float(b)) for a, b in zip(occurrence.uv, expected_uv)) <= uv_grade,
                            'canonical source joint changed more than its declared floating UV closure grade')
                # Canonical joint UV can close a measured foreign endpoint gap.
                # Preserve its declared periodic lift; never silently relabel a
                # seam occurrence by an entire revolution.
                branch_divisor = 2 if recovery_kind != 'pcurve' else 4
                require(all(not period or abs(float(actual)-float(wanted)) < period/branch_divisor
                            for actual, wanted, period in zip(occurrence.uv, expected_uv, periods)),
                        'canonical occurrence changed its original periodic branch')
                if use.parameter_map_kind == 1:
                    require(abs(occurrence.pcurve_parameter-occurrence.source_parameter) <= coordinate_tolerance,
                            'same-parameter occurrence changed original parameter')
                key = (face.definition_id, occurrence.sample_key)
                require(key in vertices, 'boundary shared physical key missing')
                xyz = vertices[key].xyz
                surface_point = carrier_jet_float(carrier, *occurrence.uv)[0]
                residual = math.dist(tuple(map(float, surface_point)), xyz)
                require(math.isfinite(residual) and residual <= mesh.quality.source_residual_mm,
                        'actual original spatial-edge/face residual exceeds selected budget')
                max_recovery_residual = max(max_recovery_residual, residual)
                actual_occurrences[use.id, occurrence.source_parameter, occurrence.sample_key] += 1
                boundary_corners.add((occurrence.sample_key, tuple(occurrence.uv)))
                normalized.append(tuple(map(exact, occurrence.uv)))
            expected_occurrences = Counter()
            for use_id in loop.coedge_ids:
                use = uses[use_id]
                sample = samples[use.edge_id]
                pairs = tuple(zip(sample.parameters, sample.vertex_keys))
                if use.sense < 0:
                    pairs = pairs[::-1]
                expected_occurrences.update((use_id, t, key) for t, key in pairs[:-1])
                if (loop_id, use_id) in closure_parameters:
                    expected_occurrences[use_id, pairs[-1][0], pairs[-1][1]] += 1
            require(actual_occurrences == expected_occurrences, 'complete original occurrence samples differ')
            for closure in closures:
                if closure.loop_id != loop_id:
                    continue
                incoming, it, outgoing, ot = closure_parameters[loop_id, closure.incoming_coedge_id]
                left = [o for o in chain if o.coedge_id == incoming and o.source_parameter == it]
                right = [o for o in chain if o.coedge_id == outgoing and o.source_parameter == ot]
                require(len(left) == len(right) == 1 and left[0].sample_key == right[0].sample_key and
                        left[0].uv == closure.uv_start and right[0].uv == closure.uv_end,
                        'chart closure original endpoint occurrences differ')
            if len(seen_uses) > 1 and seen_uses[0] == seen_uses[-1]:
                seen_uses.pop()
            word = tuple(seen_uses)
            expected_word = loop.coedge_ids
            allowed = tuple(expected_word[i:] + expected_word[:i] for i in range(len(expected_word)))
            reversed_word = expected_word[::-1]
            allowed += tuple(reversed_word[i:] + reversed_word[:i] for i in range(len(reversed_word)))
            require(word in allowed, 'complete original coedge cyclic word differs')
            normalized_loops.append(tuple(normalized))
            outer.append(loop.outer)
        require(len(set(actual_loop_ids)) == len(actual_loop_ids) and set(actual_loop_ids) == set(face.loop_ids),
                'complete original face loop owners differ')
        _simple_domains(normalized_loops, outer)
        triangle_uvs = []
        for index in domain.triangle_indices:
            require(type(index) is int and 0 <= index < len(mesh.triangles), 'foreign face triangle index')
            triangle_owners[index] += 1
            triangle = mesh.triangles[index]
            require(triangle.face_id == face_id and len(triangle.vertex_keys) == len(triangle.corners) == 3,
                    'actual triangle source face/corners differ')
            xyz = []
            for key, corner in zip(triangle.vertex_keys, triangle.corners):
                require((face.definition_id, key) in vertices, 'triangle shared physical key missing')
                require(0 <= corner.carrier_slot < len(source.carriers) and
                        source.carriers[corner.carrier_slot].id == face.carrier_id and
                        corner.face_orientation == face.orientation, 'triangle original carrier/orientation differs')
                require(len(corner.uv) == 2 and all(math.isfinite(x) for x in corner.uv), 'finite actual original UV')
                bound = 0.
                if any(type(x) is F for x in corner.uv):
                    rounding_key = (carrier.id,tuple(corner.uv))
                    if rounding_key not in rounding_bounds:
                        rounding_bounds[rounding_key] = native_uv_rounding_bound(carrier,corner.uv)
                    bound = rounding_bounds[rounding_key]
                require(math.isfinite(bound) and bound <= mesh.quality.tolerance_mm/16,
                        'actual owned UV rounding exceeds separate native quality budget')
                max_rounding_bound = max(max_rounding_bound,bound)
                vertex = vertices[face.definition_id, key]
                if vertex.source_kind == 3:
                    require(vertex.source_id == face_id and tuple(corner.uv) == vertex.uv,
                            'interior corner changed original face UV generator')
                else:
                    require((key, tuple(corner.uv)) in boundary_corners, 'boundary corner lost original UV occurrence')
                xyz.append(vertex.xyz)
            triangle_uvs.append(tuple(tuple(map(exact, c.uv)) for c in triangle.corners))
            collapsed = getattr(triangle, 'collapsed', False)
            require(collapsed == (len(set(triangle.vertex_keys)) < 3), 'physical quotient flag differs')
            if collapsed:
                repeated = {key for key, n in Counter(triangle.vertex_keys).items() if n > 1}
                require(supported_poles and all(vertices[face.definition_id, key].source_kind == 1 and
                                      vertices[face.definition_id, key].source_id in supported_poles for key in repeated),
                        'collapsed UV cell is not an actual declared source pole quotient')
                collapsed_count += 1
            else:
                keys = triangle.vertex_keys
                physical_by_definition[face.definition_id].append(keys if face.orientation > 0 else (keys[0], keys[2], keys[1]))
            # Direct finite interpolation probes include collapsed UV cells.
            for weights in ((F(1,3),)*3, (F(1,2),F(1,2),F(0)),
                            (F(0),F(1,2),F(1,2)), (F(1,2),F(0),F(1,2))):
                uv = tuple(sum(weights[j]*exact(triangle.corners[j].uv[k]) for j in range(3)) for k in range(2))
                wanted = tuple(map(float, carrier_jet_float(carrier, *uv)[0]))
                actual = tuple(float(sum(weights[j]*exact(xyz[j][k]) for j in range(3))) for k in range(3))
                error = math.dist(actual, wanted)
                require(math.isfinite(error) and error <= mesh.quality.tolerance_mm + coordinate_tolerance,
                        'actual physical interpolation probe exceeds requested tolerance plus numeric comparison grade')
                max_probe_error = max(max_probe_error, error)
        qualify_domain(tuple(normalized_loops), tuple(triangle_uvs))
        require(candidate_records_seen=={key for key,row in joint_recoveries.items()
                                         if row.kind=='spline_joint_endpoint_candidate'},
                'spline endpoint joint record lost typed original boundary occurrence')
    require(triangle_owners == Counter({i: 1 for i in range(len(mesh.triangles))}),
            'every actual UV cell must belong once to its complete face')
    solid_stats = []
    for definition in source.definitions:
        physical = {key: row.xyz for (owner, key), row in vertices.items() if owner == definition.id}
        stats = qualify_physical_solid(physical, physical_by_definition[definition.id])
        source_euler = (len(definition.vertex_ids)-len(definition.edge_ids)+
                        sum(face_euler(source, id) for id in definition.face_ids))
        require(stats['euler'] == source_euler, 'physical mesh changed original solid Euler characteristic')
        solid_stats.append(stats)
    return dict(source_counts, mesh_vertices=len(vertices), uv_triangles=len(mesh.triangles),
                collapsed_uv_cells=collapsed_count, physical_triangles=sum(s['triangles'] for s in solid_stats),
                solid_eulers=tuple(s['euler'] for s in solid_stats), max_source_vertex_error=max_vertex_error,
                max_pcurve_uv_error=max_uv_error, max_source_recovery_residual=max_recovery_residual,
                max_original_edge_endpoint_departure_mm=max_original_edge_endpoint_departure,
                max_finite_interpolation_probe_error=max_probe_error,
                max_independent_native_uv_rounding_bound_mm=max_rounding_bound,
                cylinder_source_inverse_occurrences=cylinder_recovery_count,
                original_vertex_joint_recoveries=joint_recovery_count,
                max_foreign_cylinder_pcurve_residual_mm=max_foreign_cylinder_residual,
                owned_source_recovery_counts=tuple(sorted(owned_recovery_counts.items())),
                owned_spline_foreign_outside_domain_occurrences=spline_foreign_outside_count)


def _normal(carrier, uv, jet, orientation):
    value = cross(jet[1], jet[2])
    if not any(value) and carrier.kind == 3:
        u, _ = uv
        frame = carrier.frame
        e = _frame_vector(frame, (math.cos(u), math.sin(u), 0.))
        ep = _frame_vector(frame, (-math.sin(u), math.cos(u), 0.))
        value = tuple(math.sin(carrier.radii[1])*a + math.cos(carrier.radii[1])*b
                      for a, b in zip(cross(ep, e), cross(ep, frame[9:12])))
    length = math.sqrt(sum(float(x)*float(x) for x in value))
    require(math.isfinite(length) and length > 0, 'independent original normal is singular')
    return tuple(orientation*float(x)/length for x in value)


def _rows(data, offset, count, fmt):
    width = struct.calcsize(fmt)
    require(offset + count*width <= len(data), 'truncated actual native output')
    return tuple(struct.unpack_from(fmt, data, offset + i*width) for i in range(count)), offset + count*width


def qualify_native_scene(mesh, assembly, scene_path, jets_path, projected_paths=(), *,
                         views_path=None, explosion=None, coordinate_tolerance=2.**-32,
                         normal_tolerance=2.**-32):
    """Read actual native inputs/outputs and check all original geometry/poses.

    Exact tiny fixtures establish scalar arithmetic. Actual full-corner source
    answers use an independently implemented local-support Cox recurrence and
    original analytic maps, with stated finite numeric comparison grades.
    """
    data = Path(scene_path).read_bytes()
    require(len(data) >= 80, 'native scene header truncated')
    header = struct.unpack_from('<8sII8Q', data)
    require(header[:3] == (b'BGMESH01', 1, 0) and header[-1] == 0, 'actual native scene header differs')
    nd, ni, nv, nt, nc, nm, nh = header[3:10]
    require(80+40*nd+136*ni+56*nv+24*nt+192*nc+32*nm+32*nh == len(data),
            'complete native scene byte extents differ')
    defs, at = _rows(data, 80, nd, '<5Q')
    instances, at = _rows(data, at, ni, '<QII15d')
    vertices, at = _rows(data, at, nv, '<5dIIQ')
    triangles, at = _rows(data, at, nt, '<4IQ')
    carriers, at = _rows(data, at, nc, '<QII18dQQIIQ')
    _, at = _rows(data, at, nm, '<4d')
    controls, at = _rows(data, at, nh, '<4d')
    require(at == len(data), 'actual native scene has trailing data')
    require(tuple(row[0] for row in defs) == tuple(row.id for row in mesh.source.definitions),
            'native complete original definition order differs')
    require(len(instances) == len(assembly.occurrences), 'native complete original occurrence inventory differs')
    for row, occurrence in zip(instances, assembly.occurrences):
        require(row[0] == occurrence.id and row[2] == 0 and row[1] < len(defs) and
                defs[row[1]][0] == occurrence.definition_id, 'native original full occurrence mapping differs')
        require(row[3:15] == occurrence.transform, 'native source assembled matrix changed')
        if explosion is not None:
            require(row[15:18] == explosion.offsets[occurrence.id-1], 'native authored world offset differs')
    original_carriers = {row.id: row for row in mesh.source.carriers}
    native_original = []
    native_sides = []
    for row in carriers:
        require(row[0] in original_carriers and row[2] == row[-1] == 0, 'native carrier original identity/flags differ')
        source = original_carriers[row[0]]
        require(row[1] == source.kind, 'native original carrier type differs')
        if source.kind == 6:
            matches = [span for span in source.spans if span.bounds == row[17:21]]
            require(len(matches) == 1 and matches[0].degrees == row[23:25], 'native original spline span differs')
            span = matches[0]
            require(row[21]+row[22] <= len(controls), 'native homogeneous span range differs')
            actual_controls = tuple(x for point in controls[row[21]:row[21]+row[22]] for x in point)
            require(actual_controls == span.controls, 'native submitted source-span controls differ')
            require(row[3:17] == (0.,)*14, 'native source-span unused frame/radii differ')
            native_sides.append(span.bounds)
        else:
            require(row[3:15] == source.frame and row[15:17] == source.radii,
                    'native original analytic frame/parameters differ')
            native_sides.append(None)
        native_original.append(source)
    mesh_vertices = {(row.definition_id, row.physical_key): row for row in mesh.vertices}
    expected_by_definition = defaultdict(list)
    face_by_id = {row.id: row for row in mesh.source.faces}
    original_cone_poles = {(row.face_id,closure.source_vertex_id) for row in mesh.faces
                           for closure in getattr(row,'chart_closures',()) if closure.kind == 'cone_apex'}
    limiting_cone_slots = set()
    for triangle in mesh.triangles:
        if not triangle.collapsed:
            expected_by_definition[face_by_id[triangle.face_id].definition_id].append(triangle)
    vertex_owners, seen_slots = {}, set()
    rounding_bounds = {}
    max_uv_rounding_bound = 0.
    for definition in defs:
        id, first_vertex, nvertex, first_triangle, ntriangle = definition
        expected = expected_by_definition[id]
        require(ntriangle == len(expected), 'native complete original physical triangle count differs')
        for original, actual in zip(expected, triangles[first_triangle:first_triangle+ntriangle]):
            keys, corners = original.vertex_keys, original.corners
            orientation = corners[0].face_orientation
            if orientation < 0:
                keys = (keys[0], keys[2], keys[1])
                corners = (corners[0], corners[2], corners[1])
            require(actual[4] == original.face_id, 'native original triangle face differs')
            for local, key, corner in zip(actual[:3], keys, corners):
                require(local < nvertex, 'native original triangle vertex range differs')
                slot = first_vertex+local
                seen_slots.add(slot)
                vertex_owners[slot] = id
                submitted = vertices[slot]
                require((id, key) in mesh_vertices, 'native physical source vertex missing')
                physical = mesh_vertices[id, key]
                flags = (1 if physical.source_kind != 3 else 0) | (2 if orientation < 0 else 0)
                rounded_uv = tuple(map(float,corner.uv))
                require(submitted[:3] == physical.xyz and submitted[3:5] == rounded_uv and
                        submitted[6:] == (flags, key), 'native actual original corner/physical identity differs')
                require(submitted[5] < len(carriers) and
                        carriers[submitted[5]][0] == mesh.source.carriers[corner.carrier_slot].id,
                        'native actual corner carrier correspondence differs')
                source_carrier = mesh.source.carriers[corner.carrier_slot]
                if physical.source_kind == 1 and (original.face_id,physical.source_id) in original_cone_poles:
                    require(source_carrier.kind == 3,'declared cone pole has wrong original carrier')
                    pole_v = -source_carrier.radii[0]/math.sin(source_carrier.radii[1])
                    pole_xyz = _frame_point(source_carrier.frame,(0.,0.,pole_v*math.cos(source_carrier.radii[1])))
                    require(math.dist(physical.xyz,pole_xyz) <= coordinate_tolerance and submitted[4] == pole_v,
                            'native original cone pole lost original vertex/canonical analytic V')
                    limiting_cone_slots.add(slot)
                rounding_bound = 0.
                if any(type(x) is F for x in corner.uv):
                    rounding_key = (source_carrier.id,tuple(corner.uv))
                    if rounding_key not in rounding_bounds:
                        rounding_bounds[rounding_key] = native_uv_rounding_bound(source_carrier,corner.uv)
                    rounding_bound = rounding_bounds[rounding_key]
                require(math.isfinite(rounding_bound) and rounding_bound <= mesh.quality.tolerance_mm/16,
                        'owned-to-native UV rounding displacement exceeds separate quality budget')
                max_uv_rounding_bound = max(max_uv_rounding_bound,rounding_bound)
    require(seen_slots == set(range(len(vertices))), 'native display mesh has unconsumed original corners')
    jets = Path(jets_path).read_bytes()
    require(jets[:16] == struct.pack('<8sQ', b'BGJETS01', len(vertices)) and
            len(jets) == 16+128*len(vertices), 'actual complete native jet output differs')
    max_position = max_partial = max_normal = 0.
    for index, vertex in enumerate(vertices):
        row = struct.unpack_from('<15dII', jets, 16+128*index)
        require(row[15] == 0 and not row[16] & ~1 and all(math.isfinite(x) for x in row[:15]),
                'actual native source jet status/finiteness differs')
        if index in limiting_cone_slots:
            require(row[16] & 1,'native declared original cone pole did not use limiting normal')
        carrier = native_original[vertex[5]]
        uv = vertex[3:5]
        sides = native_sides[vertex[5]]
        su = 'left' if sides is not None and uv[0] == sides[1] else None
        sv = 'left' if sides is not None and uv[1] == sides[3] else None
        expected = carrier_jet_float(carrier, *uv, side_u=su, side_v=sv)
        for term in range(3):
            for actual, wanted in zip(row[3*term:3*term+3], expected[term]):
                error = abs(actual-float(wanted))
                require(error <= coordinate_tolerance*max(1., abs(float(wanted))),
                        'actual original-source native first jet differs')
                if term:
                    max_partial = max(max_partial, error)
                else:
                    max_position = max(max_position, error)
        expected_normal = _normal(carrier, uv, expected, -1 if vertex[6] & 2 else 1)
        max_normal = max(max_normal, _near(row[9:12], expected_normal, normal_tolerance,
                                           'actual original-source native oriented normal differs'))
        used = vertex[:3] if vertex[6] & 1 else row[:3]
        require(struct.pack('<3d', *row[12:15]) == struct.pack('<3d', *used),
                'native canonical boundary/evaluated interior used XYZ differs')
    max_world_error = max_world_normal_error = max_depth_error = 0.
    projected_count = 0
    if projected_paths:
        require(views_path is not None, 'actual projected qualification needs original view requests')
        requests = Path(views_path).read_bytes()
        require(len(requests) >= 16 and requests[:12] == struct.pack('<8sI', b'BGVIEW01', 1),
                'actual native view request header differs')
        count = struct.unpack_from('<I', requests, 12)[0]
        require(len(requests) == 16+192*count and len(projected_paths) == count,
                'complete original projected/view request counts differ')
        for frame, path in enumerate(projected_paths):
            request = struct.unpack_from('<4I19dII2d', requests, 16+192*frame)
            alpha = request[4]
            basis, center, pixel, scale = request[5:14], request[14:17], request[17:19], request[19]
            supersample = request[2]
            projected = Path(path).read_bytes()
            nplaced = sum(defs[row[1]][2] for row in instances)
            require(projected[:16] == struct.pack('<8sQ', b'BGPROJ01', nplaced) and
                    len(projected) == 16+96*nplaced, 'complete actual projected output differs')
            placed = 0
            for instance in instances:
                definition = defs[instance[1]]
                matrix, offset = instance[3:15], instance[15:18]
                for slot in range(definition[1], definition[1]+definition[2]):
                    row = struct.unpack_from('<3Q2q7d', projected, 16+96*placed)
                    require(row[:3] == (instance[0], definition[0], slot),
                            'actual projected full occurrence/source-vertex mapping differs')
                    jet = struct.unpack_from('<15dII', jets, 16+128*slot)
                    world = tuple(math.fsum(matrix[4*i+j]*jet[12+j] for j in range(3))+
                                  matrix[4*i+3] + alpha*offset[i] for i in range(3))
                    max_world_error = max(max_world_error, _near(row[6:9], world, coordinate_tolerance,
                                                               'actual source instance world position differs'))
                    normal = tuple(sum(matrix[4*i+j]*jet[9+j] for j in range(3)) for i in range(3))
                    length = math.sqrt(sum(x*x for x in normal))
                    normal = tuple(x/length for x in normal)
                    max_world_normal_error = max(max_world_normal_error, _near(row[9:12], normal, normal_tolerance,
                                                                              'actual rigidly placed source normal differs'))
                    # Saved actual world coordinates are the raster input. Check
                    # projection/snap exactly on those values; source/placement
                    # agreement was established separately above.
                    delta = tuple(row[6+i]-center[i] for i in range(3))
                    camera = tuple(((basis[3*i]*delta[0]+basis[3*i+1]*delta[1])+basis[3*i+2]*delta[2]) for i in range(3))
                    xy = ((pixel[0]+scale*camera[0])*supersample,
                          (pixel[1]-scale*camera[1])*supersample)
                    snaps = tuple(math.floor(x*256+.5) if x >= 0 else math.ceil(x*256-.5) for x in xy)
                    require(row[3:5] == snaps, 'actual projected integer snap differs')
                    max_depth_error = max(max_depth_error, _near((row[5],), (camera[2],), coordinate_tolerance,
                                                                'actual projected camera depth differs'))
                    placed += 1
            projected_count += placed
    return dict(definitions=len(defs), instances=len(instances), display_corners=len(vertices),
                physical_triangles=len(triangles), source_jets=len(vertices), projected_vertices=projected_count,
                max_original_native_position_error=max_position, max_original_native_partial_error=max_partial,
                max_original_native_normal_error=max_normal, max_instance_world_error=max_world_error,
                max_instance_world_normal_error=max_world_normal_error, max_projected_depth_error=max_depth_error,
                max_independent_native_uv_rounding_bound_mm=max_uv_rounding_bound)
