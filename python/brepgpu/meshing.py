"""Complete shared-edge canonical meshes with explicit world approximation bounds.

CPU exact construction. Circle chords approximate trims; this is not a general
triangulator. Corner UV is chart-local; BoundaryUse.uv is the original lifted map.
"""
from dataclasses import dataclass
from fractions import Fraction as F
import math
import operator
from .body import BodySnapshot


class MeshBudgetError(ValueError):
    """The complete requested mesh cannot meet its bounded resource/error budget."""


@dataclass(frozen=True, slots=True)
class MeshVertex:
    id: int
    position: tuple
    rounded_position: tuple
    source: tuple


@dataclass(frozen=True, slots=True)
class MeshCorner:
    chart_id: int
    uv: tuple


@dataclass(frozen=True, slots=True)
class MeshTriangle:
    id: int
    vertices: tuple
    face_id: int
    corners: tuple


@dataclass(frozen=True, slots=True)
class EdgePolyline:
    edge_id: int
    parameters: tuple
    vertices: tuple


@dataclass(frozen=True, slots=True)
class BoundaryUse:
    coedge_id: int
    face_id: int
    edge_id: int
    orientation: int
    parameters: tuple
    vertices: tuple
    uv: tuple


@dataclass(frozen=True, slots=True, init=False)
class Mesh:
    body: BodySnapshot
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
        raise TypeError('Mesh construction is factory-only; use tessellate')


def _quarter(x, y, q):
    return ((x,y),(-y,x),(-x,-y),(y,-x))[q]


def _circle(radius, t):
    q=min(int(4*t),3);s=4*t-q;d=1+s*s
    return _quarter(radius*(1-s*s)/d, radius*2*s/d, q)


def _curve(curve, t):
    values=tuple(F(x) for x in curve.data)
    if curve.kind=='line':
        return tuple((1-t)*values[a]+t*values[a+3] for a in range(3))
    if curve.kind=='circle':
        return (*_circle(values[0],t),values[1])
    raise ValueError('unsupported source curve')


def _uv(mapping,t):
    d=tuple(F(x) for x in mapping.data)
    if mapping.kind=='line':return tuple((1-t)*d[a]+t*d[a+2] for a in range(2))
    if mapping.kind=='circle_plane':
        return tuple((x+d[1])/(2*d[1]) for x in _circle(d[0],t))
    if mapping.kind=='circle_wall':return 4*t,d[0]
    if mapping.kind=='seam_wall':return d[0],t
    raise ValueError('unsupported source UV map')


def _plane_basis(chart):
    c=chart.patch.controls
    p=tuple(F(c[a])/F(c[3]) for a in range(3))
    u=tuple(F(c[8+a])/F(c[11])-p[a] for a in range(3))
    v=tuple(F(c[4+a])/F(c[7])-p[a] for a in range(3))
    return p,u,v


def _plane_uv(chart,point):
    origin,u,v=_plane_basis(chart)
    uu=sum(x*x for x in u);vv=sum(x*x for x in v)
    return (sum((x-o)*a for x,o,a in zip(point,origin,u))/uu,
            sum((x-o)*a for x,o,a in zip(point,origin,v))/vv)


def _cross(a,b):return a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]


def _det(a,b,c):return (b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0])


def _rounded_guard(body, vertices, triangles, polylines):
    """Cheap canonical embedding prerequisites, in exact ratios of output floats."""
    xyz=tuple(tuple(F(x) for x in v.rounded_position) for v in vertices)
    center=tuple(F(x) for x in body.frame.origin)
    rings=[]
    for edge,poly in zip(body.edges,polylines):
        if body.curves[edge.curve_slot].kind!='circle':continue
        ring=tuple(xyz[i] for i in poly.vertices[:-1]);N=len(ring);n=N//4
        if len({p[2] for p in ring})!=1:raise MeshBudgetError('rounded ring is not planar')
        for k,p in enumerate(ring):
            if _det(p,ring[(k+1)%N],ring[(k+2)%N])<=0:
                raise MeshBudgetError('rounded ring lost strict convexity')
        # Monotone single traversal of the four prescribed quadrants excludes
        # winding/multi-turn stars; local turns alone would not establish this.
        for q in range(4):
            xy=[]
            for j in range(n+1):
                p=ring[(q*n+j)%N]
                x,y=_quarter(p[0]-center[0],p[1]-center[1],(-q)%4)
                if x<0 or y<0:raise MeshBudgetError('rounded circle left source quadrant')
                xy.append((x,y))
            if xy[0][1]!=0 or xy[-1][0]!=0 or any(a[0]<=b[0] or a[1]>=b[1] for a,b in zip(xy,xy[1:])):
                raise MeshBudgetError('rounded ring lost angular order')
        rings.append(ring)
    if rings:
        if tuple(p[:2] for p in rings[0])!=tuple(p[:2] for p in rings[1]):
            raise MeshBudgetError('rounded top/bottom rings disagree')
        if not rings[0][0][2]<rings[1][0][2]:raise MeshBudgetError('rounded body height collapsed')
        if body.shape=='plate':
            corners=xyz[:8]
            xmin,xmax=min(p[0] for p in corners),max(p[0] for p in corners)
            ymin,ymax=min(p[1] for p in corners),max(p[1] for p in corners)
            if not all(xmin<p[0]<xmax and ymin<p[1]<ymax for p in rings[0]):
                raise MeshBudgetError('rounded hole left outer boundary')
    for triangle in triangles:
        a,b,c=(xyz[i] for i in triangle.vertices)
        normal=_cross(tuple(y-x for x,y in zip(a,b)),tuple(y-x for x,y in zip(a,c)))
        face=body.faces[triangle.face_id-1]
        if face.kind=='cylinder':
            sign=sum(normal[k]*(sum(p[k] for p in (a,b,c))/3-center[k]) for k in (0,1))*face.orientation
        else:
            _,u,v=_plane_basis(body.charts[face.chart_slots[0]])
            reference=_cross(u,v)
            sign=sum(x*y for x,y in zip(normal,reference))*face.orientation
            axes=[axis for axis in range(3) if reference[axis]]
            if len(axes)!=1 or len({p[axes[0]] for p in (a,b,c)})!=1:
                raise MeshBudgetError('rounded face left its source plane')
        if sign<=0:raise MeshBudgetError('rounded triangle collapsed or reversed')


def tessellate(body, *, tolerance, max_quarter_segments=256):
    """Return one complete immutable canonical shell; raise on an unmet budget.

    tolerance bounds the analytic-face/chord approximation plus the actual
    shared binary64 output perturbation, in world units. See docs/MESHING.md.
    """
    if not isinstance(body,BodySnapshot):raise TypeError('expected factory BodySnapshot')
    if isinstance(tolerance,bool) or not isinstance(tolerance,(int,float,F)):
        raise TypeError('tolerance must be int, Fraction or finite binary64')
    if isinstance(tolerance,float) and not math.isfinite(tolerance):raise ValueError('tolerance must be positive and finite')
    tolerance=F(tolerance)
    if tolerance<=0:raise ValueError('tolerance must be positive')
    if isinstance(max_quarter_segments,bool):raise TypeError('segment budget must be integer')
    maximum=operator.index(max_quarter_segments)
    if not 1<=maximum<=1024 or maximum&(maximum-1):raise ValueError('segment budget must be a power of two in 1..1024')
    circles=tuple(c for c in body.curves if c.kind=='circle')
    n=0;curve_bound=F(0)
    if circles:
        radius=max(F(c.data[0])*F(body.frame.scale) for c in circles)
        n=1
        while radius/(2*n*n)>tolerance/2:
            n*=2
            if n>maximum:raise MeshBudgetError('requested tolerance exceeds circle subdivision budget')
        curve_bound=radius/(2*n*n)
    # Resource counts are bounded before any mesh allocation (N<=4096).
    N=4*n
    expected_vertices=8 if body.shape=='box' else 2*N+(2 if body.shape=='cylinder' else 8)
    expected_triangles=12 if body.shape=='box' else 4*N+(0 if body.shape=='cylinder' else 16)
    origin=tuple(F(x) for x in body.frame.origin);scale=F(body.frame.scale)
    vertices=[];local=[];triangles=[];polylines=[];uses=[]
    def vertex(point,source):
        point=tuple(F(x) for x in point)
        world=tuple(o+scale*x for o,x in zip(origin,point))
        slot=len(vertices)
        vertices.append(MeshVertex(slot,world,tuple(float(x) for x in world),source));local.append(point)
        return slot
    for v in body.vertices:vertex(v.position,('vertex',v.id))
    for edge in body.edges:
        curve=body.curves[edge.curve_slot]
        if curve.kind=='line':
            parameters=(F(0),F(1));indices=(edge.start,edge.end)
        else:
            parameters=tuple(F(k,N) for k in range(N+1))
            indices=(edge.start,)+tuple(vertex(_curve(curve,t),('edge',edge.id,t)) for t in parameters[1:-1])+(edge.end,)
        polylines.append(EdgePolyline(edge.id,parameters,indices))
    for coedge in body.coedges:
        poly=polylines[coedge.edge_slot]
        params,indices=poly.parameters,poly.vertices
        if coedge.orientation<0:params,indices=params[::-1],indices[::-1]
        uses.append(BoundaryUse(coedge.id,body.faces[coedge.face_slot].id,poly.edge_id,
                               coedge.orientation,params,indices,tuple(_uv(coedge.uv_map,t) for t in params)))
    def emit(face,indices,uvs,chart):
        indices=tuple(indices);uvs=tuple(uvs)
        if face.orientation<0:indices=(indices[0],indices[2],indices[1]);uvs=(uvs[0],uvs[2],uvs[1])
        a,b,c=(local[i] for i in indices)
        if len(set(indices))!=3 or _cross(tuple(y-x for x,y in zip(a,b)),tuple(y-x for x,y in zip(a,c)))==(0,0,0):
            raise MeshBudgetError('degenerate source triangle')
        triangles.append(MeshTriangle(len(triangles),indices,face.id,tuple(MeshCorner(chart.id,uv) for uv in uvs)))
    def planar(face,ids):
        chart=body.charts[face.chart_slots[0]]
        emit(face,ids,tuple(_plane_uv(chart,local[i]) for i in ids),chart)
    def corners(face):
        # Resolve corners through original endpoint topology and its original UV
        # occurrence; UV lookup selects a role, never welds a physical position.
        result={}
        for lid in face.loop_slots:
            loop=body.loops[lid]
            if loop.hole:continue
            for cid in loop.coedge_slots:
                coedge=body.coedges[cid];edge=body.edges[coedge.edge_slot]
                for t,vid in ((F(0),edge.start),(F(1),edge.end)):
                    result[_uv(coedge.uv_map,t)]=vid
        return result
    rings={F(body.curves[e.curve_slot].data[1]):polylines[i] for i,e in enumerate(body.edges) if body.curves[e.curve_slot].kind=='circle'}
    for face in body.faces:
        chart=body.charts[face.chart_slots[0]]
        if face.kind=='rectangle':
            c=corners(face);a,b,c1,d=(c[uv] for uv in ((0,0),(1,0),(1,1),(0,1)))
            planar(face,(a,b,c1));planar(face,(a,c1,d))
        elif face.kind in ('disk','rectangle_hole'):
            z=F(chart.patch.controls[2])/F(chart.patch.controls[3]);poly=rings[z];ring=poly.vertices
            if face.kind=='disk':
                p,u,v=_plane_basis(chart)
                center=vertex(tuple(p[a]+(u[a]+v[a])/2 for a in range(3)),('face',face.id,'center'))
                for i in range(N):planar(face,(center,ring[i],ring[i+1]))
            else:
                c=corners(face)
                outer=tuple(c[uv] for uv in ((1,1),(0,1),(0,0),(1,0)))
                for q in range(4):
                    for k in range(q*n,(q+1)*n):planar(face,(outer[q],ring[k+1],ring[k]))
                    planar(face,(outer[q],outer[(q+1)%4],ring[((q+1)*n)%N]))
        elif face.kind=='cylinder':
            lower,upper=(rings[z].vertices for z in sorted(rings))
            for q,cslot in enumerate(face.chart_slots):
                wall_chart=body.charts[cslot]
                for j in range(n):
                    k=q*n+j;s0,s1=F(j,n),F(j+1,n)
                    emit(face,(lower[k],lower[k+1],upper[k+1]),((s0,F(0)),(s1,F(0)),(s1,F(1))),wall_chart)
                    emit(face,(lower[k],upper[k+1],upper[k]),((s0,F(0)),(s1,F(1)),(s0,F(1))),wall_chart)
        else:raise ValueError('unsupported source face')
    if len(vertices)!=expected_vertices or len(triangles)!=expected_triangles:
        raise RuntimeError('incomplete canonical mesh construction')
    rounding_bound=max(sum(abs(F(b)-a) for a,b in zip(v.position,v.rounded_position)) for v in vertices)
    if curve_bound+rounding_bound>tolerance:raise MeshBudgetError('rounded output exceeds requested tolerance')
    _rounded_guard(body,vertices,triangles,polylines)
    result=object.__new__(Mesh)
    fields=(body,tolerance,n,curve_bound,rounding_bound,curve_bound+rounding_bound,
            tuple(vertices),tuple(triangles),tuple(polylines),tuple(uses))
    for name,value in zip(Mesh.__dataclass_fields__,fields):object.__setattr__(result,name,value)
    return result
