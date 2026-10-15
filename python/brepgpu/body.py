"""Canonical immutable bodies, exact trim labels and shared boundary records.

Creation is factory-only. This CPU body layer uses the existing patch ABI;
packed tables are owned exports, not a GPU topology ABI or general file loader.

All table fields are little-endian, tightly packed, without implicit padding:
header <8sIIQQII4d: magic,version,flags,body,revision,shape,shells,origin3,scale.
vertices <Q3d: id,position3. curves <II6d: kind,length,data padded to6.
edges <QIII: id,start,end,curve. uv_maps <II4d: kind,length,data padded to4.
coedges <QIIIIi: id,edge,face,loop,uv-map,orientation.
loops <QIIII: id,face,hole,coedge-offset,count.
faces <Q12sIiIIII: id,nameASCII,kind,orientation,loop-offset,count,
                       chart-offset,count.
charts <QIiI: id,face,quadrant,patch. patches <IIII4d: degreeU,degreeV,
                 control-offset,control-count,sourceUV4.
controls <4d: Ax,Ay,Az,W. loop_coedges/face_loops/face_charts/shell_faces:
uint32 slots. Offsets/counts address records, not bytes. Vertex/edge/coedge/
loop/face/chart IDs are slot+1 within their own kind. Charts and patches share
the same slot order. Shape tags: box1,cylinder2,plate3; curve tags line1,circle2;
UV-map tags line1,circle_plane2,circle_wall3,seam_wall4; face tags rectangle1,
disk2,rectangle_hole3,cylinder4. The single outward shell is implicit.
"""
from dataclasses import dataclass
import math
import operator
import struct
import threading

from . import Lineage, Patch


REJECTED, INTERIOR, BOUNDARY, INVALID = 0, 1, 2, 3
FACE_KIND = 5
_SHAPES = ("box", "cylinder", "plate")
_FACE_KINDS = ("rectangle", "disk", "rectangle_hole", "cylinder")
_CURVE_KINDS = ("line", "circle")
_UV_KINDS = ("line", "circle_plane", "circle_wall", "seam_wall")
_namespace_lock = threading.Lock()
_next_body_id = 1

TABLE_LAYOUTS = (
    ("header", "<8sIIQQII4d"), ("vertices", "<Q3d"),
    ("curves", "<II6d"), ("edges", "<QIII"), ("uv_maps", "<II4d"),
    ("coedges", "<QIIIIi"), ("loops", "<QIIII"),
    ("faces", "<Q12sIiIIII"), ("charts", "<QIiI"),
    ("patches", "<IIII4d"), ("controls", "<4d"),
    ("loop_coedges", "<I"), ("face_loops", "<I"),
    ("face_charts", "<I"), ("shell_faces", "<I"))


@dataclass(frozen=True, slots=True)
class Frame:
    origin: tuple = (0., 0., 0.)
    scale: float = 1.

    def __post_init__(self):
        origin = tuple(float(x) for x in self.origin)
        scale = float(self.scale)
        if len(origin) != 3 or any(not math.isfinite(x) or abs(x) > 16 for x in origin):
            raise ValueError("frame origin needs three finite components in [-16,16]")
        if not math.isfinite(scale) or not .25 <= scale <= 4 or math.frexp(scale)[0] != .5:
            raise ValueError("frame scale must be a power of two in [.25,4]")
        object.__setattr__(self, "origin", origin)
        object.__setattr__(self, "scale", scale)


def _parameter(value):
    t = float(value)
    if not math.isfinite(t) or not 0 <= t <= 1:
        raise ValueError("curve parameter must be finite in [0,1]")
    return t


def _circle_xy(radius, t):
    a = 4*t
    quadrant = min(int(a), 3)
    s = a-quadrant
    denominator = 1+s*s
    x, y = radius*(1-s*s)/denominator, radius*(2*s)/denominator
    if quadrant == 0: return x, y
    if quadrant == 1: return -y, x
    if quadrant == 2: return -x, -y
    return y, -x


@dataclass(frozen=True, slots=True)
class Vertex:
    id: int
    position: tuple


@dataclass(frozen=True, slots=True)
class Curve:
    kind: str
    data: tuple

    def evaluate(self, t):
        t = _parameter(t)
        if self.kind == "line":
            if t == 0: return self.data[:3]
            if t == 1: return self.data[3:]
            return tuple((1-t)*a+t*b for a, b in zip(self.data[:3], self.data[3:]))
        if self.kind == "circle":
            radius, z = self.data
            return (*_circle_xy(radius, t), z)
        raise ValueError("unsupported curve kind")


@dataclass(frozen=True, slots=True)
class UVMap:
    kind: str
    data: tuple

    def evaluate(self, t):
        t = _parameter(t)
        if self.kind == "line":
            u0, v0, u1, v1 = self.data
            return (1-t)*u0+t*u1, (1-t)*v0+t*v1
        if self.kind == "circle_plane":
            radius, half_width = self.data
            x, y = _circle_xy(radius, t)
            return (x+half_width)/(2*half_width), (y+half_width)/(2*half_width)
        if self.kind == "circle_wall": return 4*t, self.data[0]
        if self.kind == "seam_wall": return self.data[0], t
        raise ValueError("unsupported UV map kind")


@dataclass(frozen=True, slots=True)
class Edge:
    id: int
    start: int
    end: int
    curve_slot: int


@dataclass(frozen=True, slots=True)
class Coedge:
    id: int
    edge_slot: int
    face_slot: int
    loop_slot: int
    orientation: int
    uv_map: UVMap


@dataclass(frozen=True, slots=True)
class Loop:
    id: int
    face_slot: int
    hole: bool
    coedge_slots: tuple


@dataclass(frozen=True, slots=True)
class Face:
    id: int
    name: str
    kind: str
    orientation: int
    loop_slots: tuple
    chart_slots: tuple


@dataclass(frozen=True, slots=True)
class Chart:
    id: int
    face_slot: int
    quadrant: int
    patch: Patch


def _slot(value, count):
    if isinstance(value, bool): raise TypeError("slot must be an integer")
    slot = operator.index(value)
    if not 0 <= slot < count: raise IndexError("slot outside snapshot")
    return slot


@dataclass(frozen=True, slots=True, init=False)
class BodySnapshot:
    shape: str
    body_id: int
    revision: int
    frame: Frame
    vertices: tuple
    curves: tuple
    edges: tuple
    coedges: tuple
    loops: tuple
    faces: tuple
    charts: tuple
    patches: tuple
    packed_tables: tuple

    def __init__(self, *args, **kwargs):
        raise TypeError("BodySnapshot construction is factory-only; use make_body")

    def classify(self, patch_slot, u, v):
        chart = self.charts[_slot(patch_slot, len(self.charts))]
        try: u, v = float(u), float(v)
        except (TypeError, ValueError, OverflowError): return INVALID
        if not math.isfinite(u) or not math.isfinite(v) or not 0 <= u <= 1 or not 0 <= v <= 1:
            return INVALID
        kind = self.faces[chart.face_slot].kind
        if kind == "cylinder":
            return BOUNDARY if (v == 0 or v == 1 or
                (chart.quadrant == 0 and u == 0) or (chart.quadrant == 3 and u == 1)) else INTERIOR
        if kind == "rectangle":
            return BOUNDARY if u == 0 or u == 1 or v == 0 or v == 1 else INTERIOR
        un, ud = u.as_integer_ratio(); vn, vd = v.as_integer_ratio()
        ratio = 1 if kind == "disk" else 3
        sign = ratio*ratio*((2*un-ud)**2*vd*vd+(2*vn-vd)**2*ud*ud)-ud*ud*vd*vd
        if kind == "disk": return REJECTED if sign > 0 else BOUNDARY if sign == 0 else INTERIOR
        if kind == "rectangle_hole":
            if sign < 0: return REJECTED
            return BOUNDARY if sign == 0 or u == 0 or u == 1 or v == 0 or v == 1 else INTERIOR
        raise ValueError("unsupported face kind")

    def edge_point(self, edge_slot, t):
        edge = self.edges[_slot(edge_slot, len(self.edges))]
        return self.curves[edge.curve_slot].evaluate(t)

    def intersect_faces(self, face_id, other, other_face_id):
        """Construct a complete supported closed-face section, on CPU."""
        from .intersections import FaceRef, intersect_faces
        return intersect_faces(FaceRef(self, face_id), FaceRef(other, other_face_id))

    def classify_point(self, point):
        """Classify exact submitted world coordinates against closed material."""
        from .classification import classify_point
        return classify_point(self, point)

    def contact_faces(self, face_id, other, other_face_id):
        """Construct the complete supported closed rectangular-face contact."""
        from .classification import contact_faces
        from .intersections import FaceRef
        return contact_faces(FaceRef(self, face_id), FaceRef(other, other_face_id))

    def arrange_face(self, face_id, sections=(), *, u_cuts=(), v_cuts=()):
        """Build supported exact regions and shared uses on one rectangular face."""
        from .arrangements import arrange_face
        from .intersections import FaceRef
        return arrange_face(FaceRef(self, face_id), sections, u_cuts=u_cuts, v_cuts=v_cuts)


def _namespace():
    global _next_body_id
    with _namespace_lock:
        if _next_body_id >= 1 << 64: raise OverflowError("body namespace exhausted")
        body_id = _next_body_id
        _next_body_id += 1
        return body_id


def _rotate(x, y, quadrant):
    if quadrant == 0: return x, y
    if quadrant == 1: return -y, x
    if quadrant == 2: return -x, -y
    return y, -x


def _packed(snapshot):
    def records(fmt, values): return b"".join(struct.pack(fmt, *v) for v in values)
    loop_slots = tuple(c for loop in snapshot.loops for c in loop.coedge_slots)
    face_loops = tuple(l for face in snapshot.faces for l in face.loop_slots)
    face_charts = tuple(c for face in snapshot.faces for c in face.chart_slots)
    controls = []; patch_rows = []
    for patch in snapshot.patches:
        offset = len(controls)//4
        controls.extend(patch.controls)
        patch_rows.append((patch.degree_u, patch.degree_v, offset, len(patch.controls)//4, *patch.uv))
    loops = []; offset = 0
    for loop in snapshot.loops:
        loops.append((loop.id, loop.face_slot, int(loop.hole), offset, len(loop.coedge_slots)))
        offset += len(loop.coedge_slots)
    faces = []; loop_offset = chart_offset = 0
    for face in snapshot.faces:
        faces.append((face.id, face.name.encode("ascii"), _FACE_KINDS.index(face.kind)+1,
                      face.orientation, loop_offset, len(face.loop_slots), chart_offset, len(face.chart_slots)))
        loop_offset += len(face.loop_slots); chart_offset += len(face.chart_slots)
    result = (
        ("header", struct.pack("<8sIIQQII4d", b"BGbody01", 1, 0, snapshot.body_id,
            snapshot.revision, _SHAPES.index(snapshot.shape)+1, 1, *snapshot.frame.origin, snapshot.frame.scale)),
        ("vertices", records("<Q3d", ((v.id, *v.position) for v in snapshot.vertices))),
        ("curves", records("<II6d", ((_CURVE_KINDS.index(c.kind)+1, len(c.data),
            *c.data, *((0.,)*(6-len(c.data)))) for c in snapshot.curves))),
        ("edges", records("<QIII", ((e.id, e.start, e.end, e.curve_slot) for e in snapshot.edges))),
        ("uv_maps", records("<II4d", ((_UV_KINDS.index(c.uv_map.kind)+1, len(c.uv_map.data),
            *c.uv_map.data, *((0.,)*(4-len(c.uv_map.data)))) for c in snapshot.coedges))),
        ("coedges", records("<QIIIIi", ((c.id, c.edge_slot, c.face_slot, c.loop_slot, i, c.orientation)
            for i, c in enumerate(snapshot.coedges)))),
        ("loops", records("<QIIII", loops)), ("faces", records("<Q12sIiIIII", faces)),
        ("charts", records("<QIiI", ((c.id, c.face_slot, c.quadrant, i) for i, c in enumerate(snapshot.charts)))),
        ("patches", records("<IIII4d", patch_rows)),
        ("controls", records("<4d", (controls[i:i+4] for i in range(0, len(controls), 4)))),
        ("loop_coedges", records("<I", ((i,) for i in loop_slots))),
        ("face_loops", records("<I", ((i,) for i in face_loops))),
        ("face_charts", records("<I", ((i,) for i in face_charts))),
        ("shell_faces", records("<I", ((i,) for i in range(len(snapshot.faces))))))
    return result


def make_body(shape, frame=Frame()):
    """Construct only the canonical analytic box/cylinder/one-hole plate."""
    if shape not in _SHAPES: raise ValueError("shape must be box, cylinder or plate")
    if not isinstance(frame, Frame): raise TypeError("frame must be Frame")
    body_id = _namespace()
    vertices = []; curves = []; edges = []; coedges = []; loops = []; faces = []; charts = []
    a, z0, z1, radius = .75, -.5, .5, .25

    def vertex(point):
        slot = len(vertices); vertices.append(Vertex(slot+1, tuple(point))); return slot

    def edge(start, end, curve):
        slot = len(edges); curves.append(curve); edges.append(Edge(slot+1, start, end, len(curves)-1)); return slot

    def loop(face_slot, hole, uses):
        slot = len(loops); uses_slots = []
        for edge_slot, orientation, uv_map in uses:
            cslot = len(coedges); uses_slots.append(cslot)
            coedges.append(Coedge(cslot+1, edge_slot, face_slot, slot, orientation, uv_map))
        loops.append(Loop(slot+1, face_slot, hole, tuple(uses_slots))); return slot

    def chart(face_slot, quadrant, p, q, controls, uv=(0., 1., 0., 1.)):
        slot = len(charts)
        patch = Patch(p, q, tuple(controls), Lineage(body_id, FACE_KIND, face_slot+1, 1, slot+1),
                      uv, frame.origin, frame.scale)
        charts.append(Chart(slot+1, face_slot, quadrant, patch)); return slot

    ring_bottom = ring_top = seam = None
    if shape in ("box", "plate"):
        for z in (z0, z1):
            for x, y in ((-a, -a), (a, -a), (a, a), (-a, a)): vertex((x, y, z))
        for start, end in ((0,1),(1,2),(2,3),(3,0),(4,5),(5,6),(6,7),(7,4),
                           (0,4),(1,5),(2,6),(3,7)):
            edge(start, end, Curve("line", vertices[start].position+vertices[end].position))
    if shape != "box":
        bottom = vertex((radius, 0., z0)); top = vertex((radius, 0., z1))
        ring_bottom = edge(bottom, bottom, Curve("circle", (radius, z0)))
        ring_top = edge(top, top, Curve("circle", (radius, z1)))
        seam = edge(bottom, top, Curve("line", vertices[bottom].position+vertices[top].position))

    # Plane chart tuples: name, origin, U vector, V vector, oriented-normal sign.
    width = radius if shape == "cylinder" else a
    plane_specs = [
        ("top", (-width,-width,z1), (2*width,0.,0.), (0.,2*width,0.), 1),
        ("bottom", (-width,-width,z0), (2*width,0.,0.), (0.,2*width,0.), -1)]
    if shape != "cylinder":
        plane_specs += [
            ("front", (-a,-a,z0), (2*a,0.,0.), (0.,0.,z1-z0), 1),
            ("back", (-a,a,z0), (2*a,0.,0.), (0.,0.,z1-z0), -1),
            ("right", (a,-a,z0), (0.,2*a,0.), (0.,0.,z1-z0), 1),
            ("left", (-a,-a,z0), (0.,2*a,0.), (0.,0.,z1-z0), -1)]
    edge_lookup = {frozenset((e.start,e.end)): i for i,e in enumerate(edges) if e.start != e.end}
    position_lookup = {v.position: i for i,v in enumerate(vertices)}
    for name, origin, U, V, orientation in plane_specs:
        face_slot = len(faces)
        controls = tuple(value for u in (0,1) for v in (0,1)
                         for value in (*tuple(origin[k]+u*U[k]+v*V[k] for k in range(3)), 1.))
        cslot = chart(face_slot, -1, 1, 1, controls)
        face_loops = []
        is_cap = name in ("top", "bottom")
        if shape == "cylinder":
            rim = ring_top if name == "top" else ring_bottom
            face_loops.append(loop(face_slot, False, ((rim, orientation, UVMap("circle_plane", (radius,width))),)))
            kind = "disk"
        else:
            corners = ((0.,0.),(1.,0.),(1.,1.),(0.,1.)) if orientation == 1 else \
                      ((0.,0.),(0.,1.),(1.,1.),(1.,0.))
            uses = []
            for start_uv, end_uv in zip(corners, corners[1:]+corners[:1]):
                points = [tuple(origin[k]+uv[0]*U[k]+uv[1]*V[k] for k in range(3)) for uv in (start_uv,end_uv)]
                start, end = (position_lookup[p] for p in points)
                eslot = edge_lookup[frozenset((start,end))]; e = edges[eslot]
                sign = 1 if e.start == start else -1
                uvdata = start_uv+end_uv if sign == 1 else end_uv+start_uv
                uses.append((eslot, sign, UVMap("line", uvdata)))
            face_loops.append(loop(face_slot, False, uses))
            kind = "rectangle_hole" if shape == "plate" and is_cap else "rectangle"
            if kind == "rectangle_hole":
                rim = ring_top if name == "top" else ring_bottom
                face_loops.append(loop(face_slot, True, ((rim, -orientation, UVMap("circle_plane", (radius,width))),)))
        faces.append(Face(face_slot+1, name, kind, orientation, tuple(face_loops), (cslot,)))

    if shape != "box":
        face_slot = len(faces); orientation = -1 if shape == "plate" else 1
        chart_slots = []
        for quadrant in range(4):
            controls = []
            for Ax, Ay, W in ((radius,0.,1.), (radius,radius,1.), (0.,2*radius,2.)):
                x, y = _rotate(Ax, Ay, quadrant)
                for z in (z0,z1): controls.extend((x,y,z*W,W))
            chart_slots.append(chart(face_slot, quadrant, 2, 1, controls,
                                     (float(quadrant),float(quadrant+1),0.,1.)))
        uses = ((ring_bottom,1,UVMap("circle_wall",(0.,))),
                (seam,1,UVMap("seam_wall",(4.,))),
                (ring_top,-1,UVMap("circle_wall",(1.,))),
                (seam,-1,UVMap("seam_wall",(0.,))))
        if orientation == -1: uses = tuple((e,-sign,m) for e,sign,m in reversed(uses))
        lslot = loop(face_slot, False, uses)
        faces.append(Face(face_slot+1, "wall", "cylinder", orientation, (lslot,), tuple(chart_slots)))

    snapshot = object.__new__(BodySnapshot)
    for name, value in (("shape",shape),("body_id",body_id),("revision",1),("frame",frame),
        ("vertices",tuple(vertices)),("curves",tuple(curves)),("edges",tuple(edges)),
        ("coedges",tuple(coedges)),("loops",tuple(loops)),("faces",tuple(faces)),
        ("charts",tuple(charts)),("patches",tuple(c.patch for c in charts)),("packed_tables",())):
        object.__setattr__(snapshot, name, value)
    object.__setattr__(snapshot, "packed_tables", _packed(snapshot))
    return snapshot
