"""Complete exact grid/disjoint-circle arrangements on owned rectangular faces.

CPU construction only. A circle is one physical closed edge with retained
four-piece section geometry; chart joins never manufacture feature edges.
"""
from bisect import bisect_left, bisect_right
from dataclasses import dataclass, field
from fractions import Fraction
from itertools import islice
import operator

from .classification import _rational, _rectangle
from .intersections import (FaceRef, IntersectionSet, ExactMap, Scalar,
                            _circle as _circle_map, _plane_map)


class UnsupportedArrangement(ValueError):
    pass


@dataclass(frozen=True, slots=True)
class Area:
    rational: Fraction
    pi: Fraction

    def __post_init__(self):
        object.__setattr__(self, "rational", _rational(self.rational))
        object.__setattr__(self, "pi", _rational(self.pi))


@dataclass(frozen=True, slots=True)
class Vertex:
    id: int
    uv: tuple
    position: tuple
    source: tuple


@dataclass(frozen=True, slots=True)
class Edge:
    id: int
    kind: str
    start: int
    end: int
    source: tuple
    piece_ids: tuple
    event_ids: tuple


@dataclass(frozen=True, slots=True)
class Use:
    id: int
    edge: int
    orientation: int
    region: int
    twin: int
    next: int
    previous: int
    loop: int


@dataclass(frozen=True, slots=True)
class Loop:
    id: int
    region: int
    hole: bool
    uses: tuple


@dataclass(frozen=True, slots=True)
class Region:
    id: int
    kind: str
    cell: object
    section: object
    outer_loop: object
    holes: tuple
    witness_uv: tuple
    area: object


@dataclass(frozen=True, slots=True)
class _Circle:
    center: tuple
    radius: Fraction
    cell: tuple
    side: int
    component: int
    piece_ids: tuple
    event_ids: tuple
    anchor_event: int
    orientation: int


def _id(value, count):
    if isinstance(value, bool):
        raise TypeError("record ID excludes bool")
    value = operator.index(value)
    if not 0 <= value < count:
        raise IndexError("record ID outside arrangement")
    return value


def _parameter(value):
    value = _rational(value)
    if not 0 <= value <= 1:
        raise ValueError("edge parameter outside [0,1]")
    return value


def _fractions(values):
    if any(type(value) is not Scalar or value.b for value in values):
        raise UnsupportedArrangement("source circle map is not rational")
    return tuple(value.a for value in values)


def _world(rectangle, uv):
    return tuple(rectangle.origin[axis]+rectangle.u[axis]*uv[0]+rectangle.v[axis]*uv[1]
                 for axis in range(3))


@dataclass(frozen=True, slots=True, init=False)
class Arrangement:
    face: FaceRef
    sections: tuple
    u_cuts: tuple
    v_cuts: tuple
    vertices: tuple
    edges: tuple
    uses: tuple
    loops: tuple
    regions: tuple
    outside_id: int
    _rectangle: object = field(repr=False)
    _circles: tuple = field(repr=False)
    _cell_sections: tuple = field(repr=False)
    _axes: tuple = field(repr=False)

    def __init__(self, *args, **kwargs):
        raise TypeError("Arrangement construction is factory-only")

    def _piece(self, edge, t):
        index = min(int(4*t), 3)
        return self.sections[edge.source[1]].pieces[edge.piece_ids[index]], 4*t-index

    def edge_uv(self, edge_id, t):
        edge, t = self.edges[_id(edge_id, len(self.edges))], _parameter(t)
        if edge.kind == "line":
            start, end = self.vertices[edge.start].uv, self.vertices[edge.end].uv
            return tuple((1-t)*a+t*b for a, b in zip(start, end))
        piece, parameter = self._piece(edge, t)
        side = self._circles[edge.source[1]].side
        return _fractions((piece.left_map if side == 0 else piece.right_map).evaluate(parameter))

    def edge_point(self, edge_id, t):
        edge, t = self.edges[_id(edge_id, len(self.edges))], _parameter(t)
        if edge.kind == "line":
            start, end = self.vertices[edge.start].position, self.vertices[edge.end].position
            return tuple((1-t)*a+t*b for a, b in zip(start, end))
        piece, parameter = self._piece(edge, t)
        return _fractions(piece.position(parameter))

    def locate_uv(self, u, v):
        u, v = _rational(u), _rational(v)
        if not 0 <= u <= 1 or not 0 <= v <= 1:
            return (self.outside_id,)
        found = {self.outside_id} if u in (0, 1) or v in (0, 1) else set()
        indices = []
        for axis, value in zip(self._axes, (u, v)):
            at = bisect_left(axis, value)
            indices.append(tuple(index for index in ((at-1, at) if at < len(axis) and axis[at] == value
                                 else (at-1,)) if 0 <= index < len(axis)-1))
        width = len(self._axes[0])-1
        for j in indices[1]:
            for i in indices[0]:
                ordinal = j*width+i
                inside = False
                for section in self._cell_sections[ordinal]:
                    circle = self._circles[section]
                    delta = (u-circle.center[0])**2+(v-circle.center[1])**2-circle.radius**2
                    if delta <= 0:
                        found.add(self.regions[1+width*(len(self._axes[1])-1)+section].id)
                    if delta < 0:
                        inside = True
                if not inside:
                    found.add(self.regions[1+ordinal].id)
        return tuple(sorted(found))


def _bounded(values, limit, name):
    result = tuple(islice(iter(values), limit+1))
    if len(result) > limit:
        raise ValueError(name+" exceeds supported count")
    return result


def _cuts(values):
    values = tuple(_rational(value) for value in _bounded(values, 64, "cuts"))
    if any(not 0 < value < 1 for value in values) or len(set(values)) != len(values):
        raise ValueError("cuts must be distinct exact values strictly inside (0,1)")
    return tuple(sorted(values))


def _boundary(rectangle):
    ref, face = rectangle.ref, rectangle.ref.face
    if len(face.loop_slots) != 1:
        raise UnsupportedArrangement("rectangle needs one original outer loop")
    loop = ref.snapshot.loops[face.loop_slots[0]]
    if loop.hole or len(loop.coedge_slots) != 4:
        raise UnsupportedArrangement("rectangle boundary needs four original line uses")
    result = {}
    scale = _rational(ref.snapshot.frame.scale)
    origin = tuple(_rational(x) for x in ref.snapshot.frame.origin)
    for slot in loop.coedge_slots:
        coedge = ref.snapshot.coedges[slot]
        edge = ref.snapshot.edges[coedge.edge_slot]
        curve = ref.snapshot.curves[edge.curve_slot]
        if coedge.uv_map.kind != "line" or curve.kind != "line" or coedge.face_slot != ref.face_id-1:
            raise UnsupportedArrangement("source boundary is not an original face line use")
        data = tuple(_rational(x) for x in coedge.uv_map.data)
        start, end = data[:2], data[2:]
        if len(start) != 2 or len(end) != 2:
            raise UnsupportedArrangement("invalid source boundary UV data")
        varying = tuple(axis for axis in range(2) if start[axis] != end[axis])
        if len(varying) != 1:
            raise UnsupportedArrangement("source boundary is not axial")
        axis = varying[0]
        if {start[axis], end[axis]} != {0, 1} or start[1-axis] != end[1-axis] or start[1-axis] not in (0, 1):
            raise UnsupportedArrangement("source boundary does not span rectangle side")
        key = (1-axis, start[1-axis])
        if key in result:
            raise UnsupportedArrangement("duplicate source boundary side")
        for uv, offset in ((start, 0), (end, 3)):
            position = tuple(origin[a]+scale*_rational(curve.data[offset+a]) for a in range(3))
            if position != _world(rectangle, uv):
                raise UnsupportedArrangement("original edge and face UV map disagree")
        result[key] = (coedge.id, edge.id, axis, start, end)
    if len(result) != 4:
        raise UnsupportedArrangement("incomplete rectangle source boundary")
    return result


def _section(section, face, rectangle, axes):
    if type(section) is not IntersectionSet:
        raise TypeError("sections require owned IntersectionSet values")
    matches = tuple(index for index, ref in enumerate((section.left, section.right))
                    if ref.snapshot is face.snapshot and ref.face_id == face.face_id)
    if len(matches) != 1:
        raise UnsupportedArrangement("section must retain the actual target snapshot and face")
    side = matches[0]
    wall = (section.right, section.left)[side]
    if wall.face.kind != "cylinder" or rectangle.fixed_axis != 2:
        raise UnsupportedArrangement("circle sections require a horizontal face and original cylindrical wall")
    if len(section.components) != 1 or len(section.pieces) != 4 or len(section.events) != 4:
        raise UnsupportedArrangement("section must be one complete four-piece circle")
    component = section.components[0]
    if component.kind != "circle" or not component.closed or component.tangent or len(set(component.piece_ids)) != 4:
        raise UnsupportedArrangement("clipped/open/tangent circle section is unsupported")
    if set(component.piece_ids) != set(range(4)) or len({piece.span_id for piece in section.pieces}) != 4:
        raise UnsupportedArrangement("source circle needs four distinct ordered physical spans")
    frame = wall.snapshot.frame
    center_world = tuple(_rational(x) for x in frame.origin)
    scale = _rational(frame.scale)
    radii = tuple(_rational(curve.data[0])*scale for curve in wall.snapshot.curves if curve.kind == "circle")
    if not radii or any(radius != radii[0] for radius in radii) or radii[0] <= 0:
        raise UnsupportedArrangement("invalid original circle radius")
    radius_world = radii[0]
    if abs(rectangle.u[rectangle.u_axis]) != abs(rectangle.v[rectangle.v_axis]):
        raise UnsupportedArrangement("circle-bearing face must have equal absolute world U/V spans")
    radius = radius_world/abs(rectangle.u[rectangle.u_axis])
    if radius != radius_world/abs(rectangle.v[rectangle.v_axis]):
        raise UnsupportedArrangement("derived UV radii disagree")
    height = rectangle.origin[2]
    center = rectangle.uv((center_world[0], center_world[1], height))
    v = (height-center_world[2])/scale+Fraction(1, 2)
    if not 0 <= v <= 1:
        raise UnsupportedArrangement("section is outside finite wall height")
    geometry = (rectangle.origin, rectangle.u, rectangle.v,
                rectangle.u_axis, rectangle.v_axis, rectangle.fixed_axis)
    endpoints = {}
    event_ids = []
    for event in section.events:
        if event.id in event_ids or len(event.incidences) != 2:
            raise UnsupportedArrangement("source events need distinct IDs and two incidences each")
        event_ids.append(event.id)
        for incidence in event.incidences:
            if incidence in endpoints:
                raise UnsupportedArrangement("duplicate source endpoint incidence")
            endpoints[incidence] = event.id
    previous = None
    for quadrant, pid in enumerate(component.piece_ids):
        piece = section.pieces[pid]
        wall_slot = piece.right_slot if side == 0 else piece.left_slot
        plane_slot = piece.left_slot if side == 0 else piece.right_slot
        if (piece.id != pid or piece.component_id != component.id or piece.kind != "arc"
                or piece.lo != 0 or piece.hi != 1 or plane_slot != face.face.chart_slots[0]
                or not 0 <= wall_slot < len(wall.snapshot.charts)
                or wall.snapshot.charts[wall_slot].face_slot != wall.face_id-1
                or wall.snapshot.charts[wall_slot].quadrant != quadrant):
            raise UnsupportedArrangement("source circle piece/chart domain is incomplete")
        spatial = _circle_map(quadrant, center_world[0], center_world[1], height, radius_world)
        plane_map = _plane_map(spatial, geometry)
        # Piece UVs are native-normalized within the original quadrant chart;
        # the chart ID carries the separate atlas lift.
        wall_map = ExactMap(((0, 1), (v,)))
        expected_left, expected_right = (plane_map, wall_map) if side == 0 else (wall_map, plane_map)
        if piece.spatial_map != spatial or piece.left_map != expected_left or piece.right_map != expected_right:
            raise UnsupportedArrangement("source circle maps do not match original carriers")
        if (pid, "lo") not in endpoints or (pid, "hi") not in endpoints:
            raise UnsupportedArrangement("source circle endpoint event missing")
        source_event = next(event for event in section.events if event.id == endpoints[(pid, "lo")])
        wall_side = "right" if side == 0 else "left"
        join = "0/4" if quadrant == 0 else str(quadrant)
        if source_event.joins != (wall_side+":atlas:"+join,):
            raise UnsupportedArrangement("source circle named atlas join is invalid")
        if previous is not None and endpoints[(previous, "hi")] != endpoints[(pid, "lo")]:
            raise UnsupportedArrangement("source circle event order is disconnected")
        previous = pid
    first = component.piece_ids[0]
    if endpoints[(previous, "hi")] != endpoints[(first, "lo")] or len(endpoints) != 8:
        raise UnsupportedArrangement("source circle events do not close completely")
    cell = []
    for values, coordinate in zip(axes, center):
        index = bisect_right(values, coordinate)-1
        if not 0 <= index < len(values)-1 or not values[index] < coordinate-radius < coordinate+radius < values[index+1]:
            raise UnsupportedArrangement("closed circle disk must lie strictly inside one open cell")
        cell.append(index)
    cross = rectangle.u[0]*rectangle.v[1]-rectangle.u[1]*rectangle.v[0]
    return _Circle(center, radius, tuple(cell), side, component.id,
                   component.piece_ids, tuple(event_ids), endpoints[(first, "lo")], 1 if cross > 0 else -1)


def arrange_face(face, sections=(), *, u_cuts=(), v_cuts=()):
    if type(face) is not FaceRef:
        raise TypeError("arrangement requires an owned FaceRef")
    try:
        rectangle = _rectangle(face)
    except ValueError as error:
        raise UnsupportedArrangement(str(error)) from error
    u_cuts, v_cuts = _cuts(u_cuts), _cuts(v_cuts)
    sections = _bounded(sections, 256, "sections")
    axes = ((Fraction(0),)+u_cuts+(Fraction(1),), (Fraction(0),)+v_cuts+(Fraction(1),))
    circles = tuple(_section(section, face, rectangle, axes) for section in sections)
    for index, circle in enumerate(circles):
        for previous in circles[:index]:
            if sum((a-b)**2 for a, b in zip(circle.center, previous.center)) <= (circle.radius+previous.radius)**2:
                raise UnsupportedArrangement("circle disks must be strictly disjoint, without tangency or nesting")
    boundary = _boundary(rectangle)
    width, height = len(axes[0])-1, len(axes[1])-1
    cell_sections = [[] for _ in range(width*height)]
    for index, circle in enumerate(circles):
        cell_sections[circle.cell[1]*width+circle.cell[0]].append(index)
    vertices, edges, drafts, loops = [], [], [], []
    for j, v in enumerate(axes[1]):
        for i, u in enumerate(axes[0]):
            uv = (u, v)
            vertices.append(Vertex(len(vertices), uv, _world(rectangle, uv), ("grid", i, j)))

    def vertex(i, j):
        return j*(width+1)+i

    def edge(kind, start, end, source, piece_ids=(), event_ids=()):
        eid = len(edges)
        edges.append(Edge(eid, kind, start, end, source, piece_ids, event_ids))
        drafts.extend([dict(id=2*eid, edge=eid, orientation=1, twin=2*eid+1),
                       dict(id=2*eid+1, edge=eid, orientation=-1, twin=2*eid)])
        return eid

    def line(start, end, fixed_axis, fixed_value, source):
        if fixed_value in (0, 1):
            coedge, original, axis, uv0, uv1 = boundary[(fixed_axis, fixed_value)]
            t0 = (vertices[start].uv[axis]-uv0[axis])/(uv1[axis]-uv0[axis])
            t1 = (vertices[end].uv[axis]-uv0[axis])/(uv1[axis]-uv0[axis])
            source = ("boundary", coedge, original, t0, t1)
        return edge("line", start, end, source)

    horizontal = [[line(vertex(i,j), vertex(i+1,j), 1, axes[1][j], ("v_cut",j-1))
                   for i in range(width)] for j in range(height+1)]
    vertical = [[line(vertex(i,j), vertex(i,j+1), 0, axes[0][i], ("u_cut",i-1))
                 for i in range(width+1)] for j in range(height)]

    def use(eid, orientation):
        return 2*eid+(orientation < 0)

    def loop(region, hole, word):
        lid = len(loops)
        word = tuple(word)
        loops.append(Loop(lid, region, hole, word))
        for index, uid in enumerate(word):
            if "loop" in drafts[uid]:
                raise ArithmeticError("arrangement use assigned to more than one boundary loop")
            drafts[uid].update(region=region, loop=lid, next=word[(index+1)%len(word)], previous=word[index-1])
        return lid

    perimeter = ([use(vertical[j][0],1) for j in range(height)]
                 +[use(horizontal[height][i],1) for i in range(width)]
                 +[use(vertical[j][width],-1) for j in reversed(range(height))]
                 +[use(horizontal[0][i],-1) for i in reversed(range(width))])
    outside_loop = loop(0, True, perimeter)
    outer = []
    for j in range(height):
        for i in range(width):
            outer.append(loop(1+j*width+i, False,
                (use(horizontal[j][i],1),use(vertical[j][i+1],1),
                 use(horizontal[j+1][i],-1),use(vertical[j][i],-1))))
    holes = [[] for _ in cell_sections]
    disks = []
    for index, circle in enumerate(circles):
        section = sections[index]
        anchor = _fractions(section.pieces[circle.piece_ids[0]].position(0))
        anchor_uv = rectangle.uv(anchor)
        vid = len(vertices)
        vertices.append(Vertex(vid, anchor_uv, anchor, ("section",index,circle.anchor_event)))
        eid = edge("circle", vid, vid, ("section",index,circle.component),circle.piece_ids,circle.event_ids)
        disk_id = 1+width*height+index
        ordinal = circle.cell[1]*width+circle.cell[0]
        disk_loop = loop(disk_id, False, (use(eid,circle.orientation),))
        holes[ordinal].append(loop(1+ordinal, True, (use(eid,-circle.orientation),)))
        disks.append(Region(disk_id,"disk",circle.cell,index,disk_loop,(),circle.center,Area(Fraction(0),circle.radius**2)))
    regions = [Region(0,"outside",None,None,None,(outside_loop,),(Fraction(-1),Fraction(-1)),None)]
    for j in range(height):
        for i in range(width):
            ordinal = j*width+i
            local = tuple(circles[index] for index in cell_sections[ordinal])
            right = min(circle.center[0]-circle.radius for circle in local) if local else axes[0][i+1]
            witness = ((axes[0][i]+right)/2,(axes[1][j]+axes[1][j+1])/2)
            area = Area((axes[0][i+1]-axes[0][i])*(axes[1][j+1]-axes[1][j]),
                        -sum((circle.radius**2 for circle in local),Fraction(0)))
            regions.append(Region(1+ordinal,"cell",(i,j),None,outer[ordinal],tuple(holes[ordinal]),witness,area))
    regions.extend(disks)
    if any("loop" not in draft for draft in drafts):
        raise ArithmeticError("arrangement contains an unassigned halfedge use")
    result = object.__new__(Arrangement)
    values = dict(face=face,sections=sections,u_cuts=u_cuts,v_cuts=v_cuts,
                  vertices=tuple(vertices),edges=tuple(edges),uses=tuple(Use(**draft) for draft in drafts),
                  loops=tuple(loops),regions=tuple(regions),outside_id=0,_rectangle=rectangle,
                  _circles=circles,_cell_sections=tuple(tuple(indices) for indices in cell_sections),_axes=axes)
    for name, value in values.items():
        object.__setattr__(result,name,value)
    return result
