"""Complete bounded mesh of actual convex-extrusion bore results."""
from dataclasses import dataclass
from fractions import Fraction as F
import math
import operator

from .feature_bore import BoreResult
from .extrusion_meshing import _uv
from .meshing import (MeshBudgetError, MeshVertex, MeshCorner, MeshTriangle,
                      EdgePolyline, BoundaryUse, _cross, _det, _quarter)


@dataclass(frozen=True, slots=True, init=False)
class BoreMesh:
    bore_result: BoreResult
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

    def __init__(self,*args,**kwargs):raise TypeError('BoreMesh is factory-only; use tessellate')

    @property
    def history(self):return self.bore_result.history

    @property
    def composed_history(self):return self.bore_result.composed_history


def _ears(outer,inner,vertices):
    """Triangulate one strictly nested convex pair using a visible X bridge.

    Endpoint occurrences are duplicated in the word, physical indices are not.
    Inclusive containment blocks a diagonal through another boundary vertex.
    """
    def xy(i):return vertices[i].position[:2]
    oi=max(range(len(outer)),key=lambda k:xy(outer[k])[0])
    outer=outer[oi:]+outer[:oi]
    # Circle's intrinsic parameter zero is its exact rightmost anchor.
    clockwise=(inner[0],)+inner[:0:-1]
    word=list(outer[:1]+clockwise+(inner[0],outer[0])+outer[1:])
    triangles=[];cursor=0
    while len(word)>3:
        found=False
        for offset in range(len(word)):
            k=(cursor+offset)%len(word);ids=(word[k-1],word[k],word[(k+1)%len(word)])
            a,b,c=map(xy,ids)
            if _det(a,b,c)<=0:continue
            blocked=False
            for other in word:
                p=xy(other)
                if p in (a,b,c):continue
                if _det(a,b,p)>=0 and _det(b,c,p)>=0 and _det(c,a,p)>=0:
                    blocked=True;break
            if blocked:continue
            triangles.append(ids);del word[k];cursor=(k-1)%len(word);found=True;break
        if not found:raise MeshBudgetError('exact cap bridge triangulation could not complete')
    if len(set(word))!=3 or _det(*(xy(i) for i in word))<=0:
        raise MeshBudgetError('exact cap bridge leaves a degenerate final triangle')
    triangles.append(tuple(word))
    if len(triangles)!=len(outer)+len(inner):
        raise ArithmeticError('incomplete cap-hole triangulation')
    return tuple(triangles)


def _rounded_guard(body,vertices,triangles,polylines):
    xyz=tuple(tuple(F(x) for x in v.rounded_position) for v in vertices)
    n=len(body.profile.vertices);lower,upper=xyz[:n],xyz[n:2*n]
    if (tuple(p[:2] for p in lower)!=tuple(p[:2] for p in upper) or
            len({p[2] for p in lower})!=1 or len({p[2] for p in upper})!=1 or
            not lower[0][2]<upper[0][2]):
        raise MeshBudgetError('rounded outer layers lost product embedding')
    for i,a in enumerate(lower):
        b=lower[(i+1)%n]
        if any(_det(a,b,p)<=0 for j,p in enumerate(lower) if j not in (i,(i+1)%n)):
            raise MeshBudgetError('rounded outer profile lost strict convexity')
    rings=tuple(tuple(xyz[i] for i in polylines[3*n+level].vertices[:-1]) for level in range(2))
    if (tuple(p[:2] for p in rings[0])!=tuple(p[:2] for p in rings[1]) or
            any(p[2]!=lower[0][2] for p in rings[0]) or any(p[2]!=upper[0][2] for p in rings[1])):
        raise MeshBudgetError('rounded rim layers lost correspondence')
    ring=rings[0];count=len(ring)
    center_xy=tuple(F(x) for x in body.cutter.frame.origin[:2]);q=count//4
    for quadrant in range(4):
        points=[]
        for j in range(q+1):
            p=ring[(quadrant*q+j)%count]
            xy=_quarter(p[0]-center_xy[0],p[1]-center_xy[1],(-quadrant)%4)
            if any(x<0 for x in xy):raise MeshBudgetError('rounded rim left source quadrant')
            points.append(xy)
        if (points[0][1]!=0 or points[-1][0]!=0 or
                any(a[0]<=b[0] or a[1]>=b[1] for a,b in zip(points,points[1:]))):
            raise MeshBudgetError('rounded rim lost source angular order')
    for j,p in enumerate(ring):
        if _det(p,ring[(j+1)%count],ring[(j+2)%count])<=0:
            raise MeshBudgetError('rounded inner ring lost strict convexity')
        if any(_det(a,lower[(i+1)%n],p)<=0 for i,a in enumerate(lower)):
            raise MeshBudgetError('rounded inner ring reached outer profile')
    center=tuple(sum(p[k] for p in ring)/count for k in (0,1))
    for triangle in triangles:
        points=tuple(xyz[i] for i in triangle.vertices);a,b,c=points
        normal=_cross(tuple(y-x for x,y in zip(a,b)),tuple(y-x for x,y in zip(a,c)))
        face=body.faces[triangle.face_id-1]
        if face.kind=='convex_polygon_hole':
            z=lower[0][2] if face.orientation<0 else upper[0][2]
            if any(p[2]!=z for p in points):raise MeshBudgetError('rounded cap left its plane')
            sign=normal[2]*face.orientation
        elif face.kind=='rectangle':
            i=face.id-3;a,b=lower[i],lower[(i+1)%n]
            if any(_det(a,b,p)!=0 or not lower[0][2]<=p[2]<=upper[0][2] for p in points):
                raise MeshBudgetError('rounded side lost its product plane')
            sign=normal[0]*(b[1]-a[1])-normal[1]*(b[0]-a[0])
        else:
            sign=-sum(normal[k]*(sum(p[k] for p in points)/3-center[k]) for k in (0,1))
        if sign<=0:raise MeshBudgetError('rounded facet collapsed or reversed')


def tessellate(bore_result,*,tolerance,max_quarter_segments=256):
    """Mesh actual trimmed caps, retained outer sides and cropped inward wall."""
    if type(bore_result) is not BoreResult:raise TypeError('expected actual owned BoreResult')
    if type(tolerance) not in (int,float,F):raise TypeError('tolerance requires exact numeric value')
    if type(tolerance) is float and not math.isfinite(tolerance):raise ValueError('tolerance must be finite')
    tolerance=F(tolerance)
    if tolerance<=0:raise ValueError('tolerance must be positive')
    if isinstance(max_quarter_segments,bool):raise TypeError('segment budget cannot be bool')
    maximum=operator.index(max_quarter_segments)
    if not 1<=maximum<=1024 or maximum&(maximum-1):
        raise ValueError('segment budget must be a power of two in1..1024')
    body=bore_result.solid;n=len(body.profile.vertices);radius=body.radius*F(body.frame.scale)
    q=1
    while radius/(2*q*q)>tolerance/2:
        q*=2
        if q>maximum:raise MeshBudgetError('requested tolerance exceeds circle subdivision budget')
    curve_bound=radius/(2*q*q);count=4*q;vertices=[];polylines=[];uses=[];triangles=[]
    def vertex(point,source):
        i=len(vertices);vertices.append(MeshVertex(i,point,tuple(float(x) for x in point),source));return i
    for v in body.vertices:
        point=tuple(F(o)+F(body.frame.scale)*x for o,x in zip(body.frame.origin,v.position))
        vertex(point,('vertex',v.id))
    for edge in body.edges:
        curve=body.curves[edge.curve_slot]
        if curve.kind=='line':params,ids=(F(0),F(1)),(edge.start,edge.end)
        else:
            params=tuple(F(k,count) for k in range(count+1))
            ids=(edge.start,)+tuple(vertex(body.edge_point(edge.id,t),('edge',edge.id,t)) for t in params[1:-1])+(edge.end,)
        polylines.append(EdgePolyline(edge.id,params,ids))
    for c in body.coedges:
        p=polylines[c.edge_slot];params,ids=p.parameters,p.vertices
        if c.orientation<0:params,ids=params[::-1],ids[::-1]
        uses.append(BoundaryUse(c.id,body.faces[c.face_slot].id,p.edge_id,c.orientation,
                                params,ids,tuple(body.coedge_uv(c.id,t) for t in params)))
    def emit(face,ids,uvs,chart):
        if face.orientation<0:ids,uvs=(ids[0],ids[2],ids[1]),(uvs[0],uvs[2],uvs[1])
        a,b,c=(vertices[i].position for i in ids)
        if len(set(ids))!=3 or _cross(tuple(y-x for x,y in zip(a,b)),tuple(y-x for x,y in zip(a,c)))==(0,0,0):
            raise MeshBudgetError('exact output facet is degenerate')
        triangles.append(MeshTriangle(len(triangles),tuple(ids),face.id,tuple(MeshCorner(chart.id,uv) for uv in uvs)))
    def planar(face,ids):
        chart=body.charts[face.chart_slots[0]]
        emit(face,ids,tuple(_uv(chart,vertices[i].position) for i in ids),chart)
    def outer_word(face):
        loop=next(body.loops[ls] for ls in face.loop_slots if not body.loops[ls].hole)
        word=tuple(uses[cs].vertices[0] for cs in loop.coedge_slots)
        if face.orientation<0:word=word[::-1]
        return word
    lower,upper=(polylines[3*n+level].vertices for level in range(2))
    for face in body.faces:
        if face.kind=='convex_polygon_hole':
            level=face.id-1;ring=(lower,upper)[level][:-1]
            for ids in _ears(outer_word(face),ring,vertices):planar(face,ids)
        elif face.kind=='rectangle':
            word=outer_word(face)
            planar(face,(word[0],word[1],word[2]));planar(face,(word[0],word[2],word[3]))
        else:
            for cs in face.chart_slots:
                chart=body.charts[cs];quad=chart.quadrant
                for j in range(q):
                    k=quad*q+j;s0,s1=F(j,q),F(j+1,q);lo,hi=body.wall_band
                    emit(face,(lower[k],lower[k+1],upper[k+1]),((s0,lo),(s1,lo),(s1,hi)),chart)
                    emit(face,(lower[k],upper[k+1],upper[k]),((s0,lo),(s1,hi),(s0,hi)),chart)
    if len(vertices)!=2*n+2*count or len(triangles)!=4*n+4*count:
        raise ArithmeticError('incomplete actual bore mesh')
    rounding=max(sum(abs(F(b)-a) for a,b in zip(v.position,v.rounded_position)) for v in vertices)
    if curve_bound+rounding>tolerance:raise MeshBudgetError('actual rounded output exceeds tolerance')
    _rounded_guard(body,vertices,triangles,polylines)
    mesh=object.__new__(BoreMesh)
    values=(bore_result,body,tolerance,q,curve_bound,rounding,curve_bound+rounding,
            tuple(vertices),tuple(triangles),tuple(polylines),tuple(uses))
    for name,value in zip(BoreMesh.__dataclass_fields__,values):object.__setattr__(mesh,name,value)
    return mesh
