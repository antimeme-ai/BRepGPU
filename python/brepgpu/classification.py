"""Exact closed material membership and finite rectangular-face contact.

CPU construction for immutable owned primitives. Inputs are exact int/Fraction/
finite float values; neither local-frame conversion nor output vertices round.
Contact dimensions describe finite faces and do not decide solid penetration.
"""
from dataclasses import dataclass
from fractions import Fraction
from itertools import islice, product
import math

from .body import BodySnapshot
from .intersections import FaceRef


class UnsupportedContact(ValueError):
    """The supplied face family is outside rectangular planar contact."""


@dataclass(frozen=True, slots=True, init=False)
class PointClassification:
    snapshot: BodySnapshot
    point: tuple
    label: str
    boundary_faces: tuple

    def __init__(self, *args, **kwargs):
        raise TypeError("PointClassification construction is factory-only")


@dataclass(frozen=True, slots=True, init=False)
class ContactVertex:
    position: tuple
    left_uv: tuple
    right_uv: tuple

    def __init__(self, *args, **kwargs):
        raise TypeError("ContactVertex construction is factory-only")


@dataclass(frozen=True, slots=True, init=False)
class FaceContact:
    left: FaceRef
    right: FaceRef
    carrier_relation: str
    dimension: int
    relation: str
    same_domain: bool
    normal_relation: str
    vertices: tuple

    def __init__(self, *args, **kwargs):
        raise TypeError("FaceContact construction is factory-only")


def _publish(cls, **fields):
    result = object.__new__(cls)
    for name, value in fields.items():
        object.__setattr__(result, name, value)
    return result


def _rational(value):
    if type(value) is Fraction:
        return value
    if type(value) is int:
        return Fraction(value)
    if type(value) is float:
        if not math.isfinite(value):
            raise ValueError("point and source values must be finite")
        return Fraction.from_float(value)
    raise TypeError("coordinates require int, Fraction or finite float; bool is excluded")


def _point(point):
    # Read at most four coordinates: an overlong (even infinite) point iterable
    # rejects its arity without collecting an unbounded coordinate sequence.
    values = tuple(islice(iter(point), 4))
    if len(values) != 3:
        raise ValueError("point needs exactly three coordinates")
    return tuple(_rational(value) for value in values)


def _body(snapshot):
    if type(snapshot) is not BodySnapshot:
        raise TypeError("material classification requires an owned BodySnapshot")
    if snapshot.shape not in ("box", "cylinder", "plate"):
        raise ValueError("unsupported body shape")
    return snapshot, tuple(_rational(value) for value in snapshot.frame.origin), \
        _rational(snapshot.frame.scale), {face.name: face.id for face in snapshot.faces}


def _classify(prepared, point):
    snapshot, origin, scale, faces = prepared
    world = _point(point)
    x, y, z = tuple((value-offset)/scale for value, offset in zip(world, origin))
    half = Fraction(1, 2)
    constraints = (("bottom", z+half), ("top", half-z))
    if snapshot.shape in ("box", "plate"):
        width = Fraction(3, 4)
        constraints += (("left", x+width), ("right", width-x),
                        ("front", y+width), ("back", width-y))
    if snapshot.shape != "box":
        radial = x*x+y*y-Fraction(1, 16)
        constraints += (("wall", radial if snapshot.shape == "plate" else -radial),)
    if any(value < 0 for _, value in constraints):
        label, boundary = "exterior", ()
    else:
        boundary = tuple(sorted(faces[name] for name, value in constraints if value == 0))
        label = "boundary" if boundary else "interior"
    return _publish(PointClassification, snapshot=snapshot, point=world,
                    label=label, boundary_faces=boundary)


def classify_point(snapshot, point):
    """Classify one exact submitted world point against closed material."""
    return _classify(_body(snapshot), point)


def classify_points(snapshot, iterable):
    """Return an owned ordered tuple; any invalid query aborts the whole batch."""
    prepared = _body(snapshot)
    return tuple(_classify(prepared, point) for point in iterable)


@dataclass(frozen=True, slots=True)
class _Rectangle:
    ref: FaceRef
    origin: tuple
    u: tuple
    v: tuple
    u_axis: int
    v_axis: int
    fixed_axis: int
    intervals: tuple
    normal_sign: int

    def uv(self, position):
        return ((position[self.u_axis]-self.origin[self.u_axis])/self.u[self.u_axis],
                (position[self.v_axis]-self.origin[self.v_axis])/self.v[self.v_axis])

    def interior(self, position):
        return all(self.intervals[axis][0] < position[axis] < self.intervals[axis][1]
                   for axis in (self.u_axis, self.v_axis))


def _rectangle(ref):
    if type(ref) is not FaceRef:
        raise TypeError("contact requires existing FaceRef sources")
    face = ref.face
    if face.kind != "rectangle" or len(face.chart_slots) != 1:
        raise UnsupportedContact("contact supports only single-chart rectangular planar faces")
    chart = ref.snapshot.charts[face.chart_slots[0]]
    patch = chart.patch
    if (patch.degree_u, patch.degree_v) != (1, 1) or len(patch.controls) != 16:
        raise UnsupportedContact("rectangle needs four bilinear homogeneous controls")
    controls = tuple(tuple(_rational(value) for value in patch.controls[index:index+4])
                     for index in range(0, 16, 4))
    if any(control[3] != 1 for control in controls):
        raise UnsupportedContact("rectangle needs unit homogeneous weights")
    local = tuple(control[:3] for control in controls)
    if any(local[3][axis] != local[2][axis]+local[1][axis]-local[0][axis]
           for axis in range(3)):
        raise UnsupportedContact("bilinear controls are not an affine rectangle")
    scale = _rational(ref.snapshot.frame.scale)
    frame_origin = tuple(_rational(value) for value in ref.snapshot.frame.origin)
    origin = tuple(frame_origin[axis]+scale*local[0][axis] for axis in range(3))
    u = tuple(scale*(local[2][axis]-local[0][axis]) for axis in range(3))
    v = tuple(scale*(local[1][axis]-local[0][axis]) for axis in range(3))
    ua, va = tuple(axis for axis in range(3) if u[axis]), tuple(axis for axis in range(3) if v[axis])
    if len(ua) != 1 or len(va) != 1 or ua == va:
        raise UnsupportedContact("rectangle must have two distinct axis-aligned affine directions")
    ua, va = ua[0], va[0]
    fixed = next(axis for axis in range(3) if axis not in (ua, va))
    end = tuple(origin[axis]+u[axis]+v[axis] for axis in range(3))
    intervals = tuple((min(origin[axis], end[axis]), max(origin[axis], end[axis]))
                      for axis in range(3))
    cross = u[(fixed+1)%3]*v[(fixed+2)%3]-u[(fixed+2)%3]*v[(fixed+1)%3]
    normal = ((cross > 0)-(cross < 0))*face.orientation
    return _Rectangle(ref, origin, u, v, ua, va, fixed, intervals, normal)


def contact_faces(left, right):
    """Construct the complete closed intersection of two supported finite faces."""
    a, b = _rectangle(left), _rectangle(right)
    if a.fixed_axis == b.fixed_axis:
        carrier = "coplanar" if a.origin[a.fixed_axis] == b.origin[b.fixed_axis] else "parallel_disjoint"
        normals = "same" if a.normal_sign == b.normal_sign else "opposite"
    else:
        carrier, normals = "transverse", "orthogonal"
    intervals = tuple((max(x[0], y[0]), min(x[1], y[1]))
                      for x, y in zip(a.intervals, b.intervals))
    same_domain = carrier == "coplanar" and a.intervals == b.intervals
    if any(lo > hi for lo, hi in intervals):
        dimension, relation, positions = -1, "disjoint", ()
        same_domain = False
    else:
        dimension = sum(lo < hi for lo, hi in intervals)
        if dimension == 2:
            relation = "coincident" if same_domain else "overlap"
            # Start at the left chart's lower U/V, then follow positive parameter
            # winding; source face orientation remains separate metadata.
            u_values = sorted(intervals[a.u_axis], key=lambda value: (value-a.origin[a.u_axis])/a.u[a.u_axis])
            v_values = sorted(intervals[a.v_axis], key=lambda value: (value-a.origin[a.v_axis])/a.v[a.v_axis])
            positions = []
            for uu, vv in ((u_values[0], v_values[0]), (u_values[1], v_values[0]),
                           (u_values[1], v_values[1]), (u_values[0], v_values[1])):
                position = [intervals[axis][0] for axis in range(3)]
                position[a.u_axis], position[a.v_axis] = uu, vv
                positions.append(tuple(position))
        else:
            positions = tuple(sorted(set(product(*(tuple(sorted(set(interval))) for interval in intervals)))))
            if carrier == "transverse" and dimension == 1:
                midpoint = tuple((lo+hi)/2 for lo, hi in intervals)
                relation = "crossing" if a.interior(midpoint) and b.interior(midpoint) else "touch"
            else:
                relation = "touch"
    vertices = tuple(_publish(ContactVertex, position=position,
                              left_uv=a.uv(position), right_uv=b.uv(position))
                     for position in positions)
    return _publish(FaceContact, left=left, right=right, carrier_relation=carrier,
                    dimension=dimension, relation=relation, same_domain=same_domain,
                    normal_relation=normals, vertices=vertices)
