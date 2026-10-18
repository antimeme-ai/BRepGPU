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


def require(condition, message):
    if not condition:
        raise AssertionError(message)


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
              'ADVANCED_FACE', 'FACE_OUTER_BOUND', 'FACE_BOUND', 'EDGE_LOOP',
              'ORIENTED_EDGE', 'EDGE_CURVE', 'NEXT_ASSEMBLY_USAGE_OCCURRENCE',
              'PRODUCT_DEFINITION_SHAPE', 'CONTEXT_DEPENDENT_SHAPE_REPRESENTATION',
              'ITEM_DEFINED_TRANSFORMATION', 'SHAPE_REPRESENTATION',
              'ADVANCED_BREP_SHAPE_REPRESENTATION', 'SHAPE_DEFINITION_REPRESENTATION',
              'MANIFOLD_SOLID_BREP', 'MAPPED_ITEM', 'REPRESENTATION_MAP',
              'SHAPE_REPRESENTATION_RELATIONSHIP', 'UNCERTAINTY_MEASURE_WITH_UNIT',
              'BREP_WITH_VOIDS', 'CLOSED_SHELL', 'ORIENTED_CLOSED_SHELL',
              'PLANE_ANGLE_MEASURE_WITH_UNIT', 'MEASURE_WITH_UNIT'}
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
    x = _unit(_refer(records, axis[3], 'DIRECTION').args[1]) if axis[3] is not None else (1., 0., 0.)
    along = sum(a*b for a, b in zip(x, z))
    x = _unit(tuple(a-along*b for a, b in zip(x, z)))
    y = cross(z, x)
    return tuple(map(float, origin)) + x + y + z


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
        """Breadth-first, each representation once, depth <= 8 (as the adapter)."""
        seen, queue = {rep_id}, [(rep_id, 0)]
        for rep, depth in queue:
            if rep in self.used:
                return True
            if depth < 8:
                for caller in self.callers.get(rep, ()):
                    if caller not in seen:
                        seen.add(caller)
                        queue.append((caller, depth+1))
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

    def solid_plane_angle(self, solid_id):
        """Radians per plane-angle unit governing one solid, or None when absent."""
        holders = [id for id in self.holders.get(solid_id, ()) if self.transferred(id)]
        require(holders, 'original solid is not held by a transferred shape representation')
        found = {self.plane_angle(id) for id in holders}
        require(len(found) == 1, 'original solid representations declare different plane-angle units')
        return next(iter(found))[1]


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
    def length_factor(reference):
        unit = records.get(reference.id)
        if type(unit) is not tuple:
            return None
        parts = {part.kind: part for part in unit}
        if 'LENGTH_UNIT' not in parts:
            return None
        require('SI_UNIT' in parts, 'unsupported original length unit')
        prefix, base = parts['SI_UNIT'].args
        require(base == '.METRE.' and prefix in (None,'.MILLI.','.CENTI.'),
                'unsupported original SI length prefix')
        return {None:1000.,'.MILLI.':1.,'.CENTI.':10.}[prefix]
    factors = set()
    for row in records.values():
        if type(row) is tuple:
            for part in row:
                if part.kind == 'GLOBAL_UNIT_ASSIGNED_CONTEXT':
                    factors.update(factor for ref in part.args[0] for factor in (length_factor(ref),)
                                   if factor is not None)
    require(factors == {source.units_mm}, 'original context length units differ from owned source')
    representations = _RepresentationGraph(records)
    plane_angle = {definition.id: representations.solid_plane_angle(definition.id) for definition in source.definitions}
    uncertainties = set()
    for row in records.values():
        if type(row) is _Call and row.kind == 'UNCERTAINTY_MEASURE_WITH_UNIT':
            value = row.args[0]
            require(type(value) is _Call and value.kind == 'LENGTH_MEASURE', 'original uncertainty measure kind')
            uncertainties.add(float(value.args[0])*length_factor(row.args[1]))
    require(uncertainties == {source.source_uncertainty_mm}, 'original source uncertainty differs')
    product_ids = {row.id for row in source.products}
    product_solids = {}
    representation_links = defaultdict(set)
    for record in records.values():
        if type(record) is _Call and record.kind == 'SHAPE_REPRESENTATION_RELATIONSHIP':
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
                mapping = _refer(records, record.args[1], 'REPRESENTATION_MAP').args
                # The supplied source has one same-frame map. A nonidentity map
                # needs explicit geometry-placement support instead of guessing.
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
    for face in source.faces:
        raw = records[face.id]
        ordinal, forward = face_shells[face.id]
        require(raw.kind == 'ADVANCED_FACE' and face.carrier_id == raw.args[2].id and
                face.orientation == (1 if (raw.args[3] == '.T.') == forward else -1) and
                face.shell == ordinal,
                'original face carrier/orientation/shell differs')
        loops = tuple(_refer(records, ref).args[1].id for ref in raw.args[1])
        require(set(face.loop_ids) == set(loops), 'original face bounds differ')
        for ref in raw.args[1]:
            bound = _refer(records, ref)
            loop = source.loop(bound.args[1].id)
            require(loop.outer == (bound.kind == 'FACE_OUTER_BOUND'), 'original outer/inner loop designation differs')
            raw_loop = _refer(records, bound.args[1], 'EDGE_LOOP')
            word = tuple(ref.id for ref in raw_loop.args[1])
            bound_forward = bound.args[2] == '.T.'
            require(loop.coedge_ids == (word if bound_forward else word[::-1]),
                    'original directed coedge loop word differs')
            for id in word:
                oe = records[id]
                expected_sense = 1 if (oe.args[4] == '.T.') == bound_forward else -1
                require(source.coedge(id).sense == expected_sense, 'original oriented-edge/bound traversal sense differs')
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


def _original_usage_transforms(records):
    transforms = {}
    for row in records.values():
        if type(row) is not _Call or row.kind != 'CONTEXT_DEPENDENT_SHAPE_REPRESENTATION':
            continue
        relation = records[row.args[0].id]
        require(type(relation) is tuple, 'original transformed representation relation missing')
        parts = {part.kind: part for part in relation}
        transformation = parts.get('REPRESENTATION_RELATIONSHIP_WITH_TRANSFORMATION')
        require(transformation is not None, 'original representation transformation missing')
        item = _refer(records, transformation.args[0], 'ITEM_DEFINED_TRANSFORMATION').args
        product_shape = _refer(records, row.args[1], 'PRODUCT_DEFINITION_SHAPE').args
        usage_id = product_shape[2].id
        require(usage_id not in transforms, 'ambiguous original usage placement')
        a, b = original_frame(records, item[2]), original_frame(records, item[3])
        # ITEM_DEFINED_TRANSFORMATION maps frame1 to frame2: B * inverse(A).
        rotation = tuple(sum(b[3+3*k+i] * a[3+3*k+j] for k in range(3))
                         for i in range(3) for j in range(3))
        matrix = tuple(value for i in range(3) for value in
                       (*rotation[3*i:3*i+3], b[i] - sum(rotation[3*i+j]*a[j] for j in range(3))))
        transforms[usage_id] = matrix
    return transforms


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
                 'edges': ('EDGE_CURVE',), 'coedges': ('ORIENTED_EDGE',), 'loops': ('EDGE_LOOP',),
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
        require(loop.face_id in tables['faces'] and loop.coedge_ids, 'original loop face/uses missing')
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


def qualify_physical_solid(vertices, triangles):
    """Closed orientable physical shell and vertex links, without product CSR."""
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
    seen, pending = set(), [0]
    while pending:
        index = pending.pop()
        if index not in seen:
            seen.add(index)
            pending.extend(neighbors[index] - seen)
    require(len(seen) == len(triangles), 'physical source solid shell is disconnected')
    return dict(vertices=len(vertices), edges=len(incidence), triangles=len(triangles),
                euler=len(vertices)-len(incidence)+len(triangles))


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
                        sum(2-len(source.face(id).loop_ids) for id in definition.face_ids))
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
