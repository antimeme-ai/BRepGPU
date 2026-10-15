"""Complete bounded CPU mesh of the actual strict through-bore CutResult.

Physical curves use the result frame; retained charts keep their own frames and
raw cutter V band. No canonical body is reconstructed or reparameterized.
"""
from dataclasses import dataclass
from fractions import Fraction as F
import math
import operator

from .boolean import CutResult
from .meshing import (MeshBudgetError, MeshVertex, MeshCorner, MeshTriangle,
                      EdgePolyline, BoundaryUse, _plane_basis, _cross, _det,
                      _quarter)


@dataclass(frozen=True, slots=True, init=False)
class CutMesh:
    cut_result: CutResult
    body: object
    tolerance: F
    quarter_segments: int
    curve_bound: F
    rounding_bound: F
    error_bound: F
    vertices: tuple
    triangles: tuple
    edge_polylines: tuple
    boundary_uses: tuple

    def __init__(self, *args, **kwargs):
        raise TypeError('CutMesh construction is factory-only; use tessellate')

    @property
    def history(self):
        return self.cut_result.history


def _plane_uv(chart, world):
    """Invert the actual retained affine patch and its own world frame."""
    p, u, v = _plane_basis(chart)
    patch = chart.patch
    local = tuple((x-F(o))/F(patch.scale) for x,o in zip(world,patch.origin))
    return tuple(sum((x-a)*b for x,a,b in zip(local,p,d))/sum(b*b for b in d)
                 for d in (u,v))


def _rounded_guard(body, vertices, triangles, polylines):
    """Exact ratios of saved floats must preserve this result's extrusion."""
    xyz = tuple(tuple(F(x) for x in v.rounded_position) for v in vertices)
    center = tuple(F(x) for x in body.frame.origin)
    rings = []
    for edge, poly in zip(body.edges,polylines):
        if body.curves[edge.curve_slot].kind != 'circle':
            continue
        ring = tuple(xyz[i] for i in poly.vertices[:-1])
        count = len(ring); n = count//4
        if len({p[2] for p in ring}) != 1:
            raise MeshBudgetError('rounded result ring is not planar')
        for k,p in enumerate(ring):
            if _det(p,ring[(k+1)%count],ring[(k+2)%count]) <= 0:
                raise MeshBudgetError('rounded result ring lost strict convexity')
        for q in range(4):
            xy = []
            for j in range(n+1):
                p = ring[(q*n+j)%count]
                x,y = _quarter(p[0]-center[0],p[1]-center[1],(-q)%4)
                if x < 0 or y < 0:
                    raise MeshBudgetError('rounded result ring left source quadrant')
                xy.append((x,y))
            if (xy[0][1] != 0 or xy[-1][0] != 0 or
                    any(a[0] <= b[0] or a[1] >= b[1] for a,b in zip(xy,xy[1:]))):
                raise MeshBudgetError('rounded result ring lost angular order')
        rings.append(ring)
    rings.sort(key=lambda ring:ring[0][2])
    if len(rings) != 2 or tuple(p[:2] for p in rings[0]) != tuple(p[:2] for p in rings[1]):
        raise MeshBudgetError('rounded result rim correspondence differs')
    if not rings[0][0][2] < rings[1][0][2]:
        raise MeshBudgetError('rounded result cap heights collapsed')
    outer = xyz[:8]
    lo = tuple(min(p[k] for p in outer) for k in range(2))
    hi = tuple(max(p[k] for p in outer) for k in range(2))
    if not all(lo[k] < p[k] < hi[k] for p in rings[0] for k in range(2)):
        raise MeshBudgetError('rounded result hole left outer boundary')
    for triangle in triangles:
        a,b,c = (xyz[i] for i in triangle.vertices)
        normal = _cross(tuple(y-x for x,y in zip(a,b)),tuple(y-x for x,y in zip(a,c)))
        face = body.faces[triangle.face_id-1]
        if face.kind == 'cylinder':
            sign = sum(normal[k]*(sum(p[k] for p in (a,b,c))/3-center[k])
                       for k in (0,1))*face.orientation
        else:
            _,u,v = _plane_basis(body.charts[face.chart_slots[0]])
            reference = _cross(u,v)
            axes = [k for k in range(3) if reference[k]]
            if len(axes) != 1 or len({p[axes[0]] for p in (a,b,c)}) != 1:
                raise MeshBudgetError('rounded result face left source plane')
            sign = sum(x*y for x,y in zip(normal,reference))*face.orientation
        if sign <= 0:
            raise MeshBudgetError('rounded result triangle collapsed or reversed')


def tessellate(cut_result, *, tolerance, max_quarter_segments=256):
    """Mesh an actual CutResult, retaining original domains and source history."""
    if type(cut_result) is not CutResult:
        raise TypeError('expected actual owned CutResult')
    if isinstance(tolerance,bool) or type(tolerance) not in (int,float,F):
        raise TypeError('tolerance requires int, Fraction or finite binary64')
    if type(tolerance) is float and not math.isfinite(tolerance):
        raise ValueError('tolerance must be positive and finite')
    tolerance = F(tolerance)
    if tolerance <= 0:
        raise ValueError('tolerance must be positive')
    if isinstance(max_quarter_segments,bool):
        raise TypeError('segment budget must be an integer')
    maximum = operator.index(max_quarter_segments)
    if not 1 <= maximum <= 1024 or maximum & (maximum-1):
        raise ValueError('segment budget must be a power of two in 1..1024')
    body = cut_result.solid
    radius = max(F(body.curves[e.curve_slot].data[0])*F(body.frame.scale)
                 for e in body.edges if body.curves[e.curve_slot].kind == 'circle')
    n = 1
    while radius/(2*n*n) > tolerance/2:
        n *= 2
        if n > maximum:
            raise MeshBudgetError('requested tolerance exceeds circle subdivision budget')
    curve_bound = radius/(2*n*n); count = 4*n
    vertices, triangles, polylines, uses = [],[],[],[]

    def vertex(world, source):
        slot = len(vertices)
        vertices.append(MeshVertex(slot,world,tuple(float(x) for x in world),source))
        return slot

    for v in body.vertices:
        vertex(tuple(F(o)+F(body.frame.scale)*F(x) for o,x in zip(body.frame.origin,v.position)),
               ('vertex',v.id))
    for edge in body.edges:
        curve = body.curves[edge.curve_slot]
        if curve.kind == 'line':
            params, ids = (F(0),F(1)),(edge.start,edge.end)
        else:
            params = tuple(F(k,count) for k in range(count+1))
            ids = ((edge.start,)+tuple(vertex(body.edge_point(edge.id,t),('edge',edge.id,t))
                                      for t in params[1:-1])+(edge.end,))
        polylines.append(EdgePolyline(edge.id,params,ids))
    for c in body.coedges:
        poly = polylines[c.edge_slot]
        params,ids = poly.parameters,poly.vertices
        if c.orientation < 0:
            params,ids = params[::-1],ids[::-1]
        uses.append(BoundaryUse(c.id,body.faces[c.face_slot].id,poly.edge_id,c.orientation,
                                params,ids,tuple(body.coedge_uv(c.id,t) for t in params)))

    def emit(face, ids, uvs, chart):
        if face.orientation < 0:
            ids,uvs = (ids[0],ids[2],ids[1]),(uvs[0],uvs[2],uvs[1])
        a,b,c = (vertices[i].position for i in ids)
        if len(set(ids)) != 3 or _cross(tuple(y-x for x,y in zip(a,b)),tuple(y-x for x,y in zip(a,c))) == (0,0,0):
            raise MeshBudgetError('degenerate exact result triangle')
        triangles.append(MeshTriangle(len(triangles),ids,face.id,
                                     tuple(MeshCorner(chart.id,uv) for uv in uvs)))

    def planar(face, ids):
        chart = body.charts[face.chart_slots[0]]
        emit(face,ids,tuple(_plane_uv(chart,vertices[i].position) for i in ids),chart)

    def corners(face):
        found = {}
        for ls in face.loop_slots:
            loop = body.loops[ls]
            if loop.hole:
                continue
            for cs in loop.coedge_slots:
                c = body.coedges[cs]; edge = body.edges[c.edge_slot]
                for t,vid in ((F(0),edge.start),(F(1),edge.end)):
                    found[body.coedge_uv(c.id,t)] = vid
        return found

    wall = next(f for f in body.faces if f.kind == 'cylinder')
    ring_uses = tuple(body.coedges[cs] for ls in wall.loop_slots
                      for cs in body.loops[ls].coedge_slots
                      if body.coedges[cs].uv_map.kind == 'circle_wall')
    ring_uses = tuple(sorted(ring_uses,key=lambda c:body.coedge_uv(c.id,F(0))[1]))
    if len(ring_uses) != 2:
        raise ArithmeticError('actual wall needs two complete source rim uses')
    lower,upper = (polylines[c.edge_slot].vertices for c in ring_uses)
    vlo,vhi = (body.coedge_uv(c.id,F(0))[1] for c in ring_uses)
    for face in body.faces:
        if face.kind == 'rectangle':
            c = corners(face)
            a,b,c1,d = (c[uv] for uv in ((0,0),(1,0),(1,1),(0,1)))
            planar(face,(a,b,c1)); planar(face,(a,c1,d))
        elif face.kind == 'rectangle_hole':
            holes = tuple(body.loops[ls] for ls in face.loop_slots if body.loops[ls].hole)
            if len(holes) != 1 or len(holes[0].coedge_slots) != 1:
                raise ArithmeticError('actual cap needs one complete source hole')
            hole = body.coedges[holes[0].coedge_slots[0]]
            ring = polylines[hole.edge_slot].vertices
            c = corners(face)
            outer = tuple(c[uv] for uv in ((1,1),(0,1),(0,0),(1,0)))
            for q in range(4):
                for k in range(q*n,(q+1)*n):
                    planar(face,(outer[q],ring[k+1],ring[k]))
                planar(face,(outer[q],outer[(q+1)%4],ring[((q+1)*n)%count]))
        elif face is wall:
            for cs in face.chart_slots:
                chart = body.charts[cs]; q = chart.quadrant
                for j in range(n):
                    k = q*n+j; s0,s1 = F(j,n),F(j+1,n)
                    emit(face,(lower[k],lower[k+1],upper[k+1]),((s0,vlo),(s1,vlo),(s1,vhi)),chart)
                    emit(face,(lower[k],upper[k+1],upper[k]),((s0,vlo),(s1,vhi),(s0,vhi)),chart)
        else:
            raise ArithmeticError('unsupported actual result face')
    if len(vertices) != 2*count+8 or len(triangles) != 4*count+16:
        raise ArithmeticError('incomplete actual result mesh')
    rounding = max(sum(abs(F(b)-a) for a,b in zip(v.position,v.rounded_position)) for v in vertices)
    if curve_bound+rounding > tolerance:
        raise MeshBudgetError('rounded output exceeds requested tolerance')
    _rounded_guard(body,vertices,triangles,polylines)
    mesh = object.__new__(CutMesh)
    values = (cut_result,body,tolerance,n,curve_bound,rounding,curve_bound+rounding,
              tuple(vertices),tuple(triangles),tuple(polylines),tuple(uses))
    for name,value in zip(CutMesh.__dataclass_fields__,values):
        object.__setattr__(mesh,name,value)
    return mesh
