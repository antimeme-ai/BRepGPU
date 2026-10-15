"""Exact planar triangulation of the actual owned convex ExtrusionResult."""
from dataclasses import dataclass
from fractions import Fraction as F
import math

from .extrusion import ExtrusionResult
from .meshing import (MeshBudgetError,MeshVertex,MeshCorner,MeshTriangle,
                      EdgePolyline,BoundaryUse,_plane_basis,_cross,_det)


@dataclass(frozen=True,slots=True,init=False)
class ExtrusionMesh:
    feature_result: ExtrusionResult
    body: object
    tolerance: F
    rounding_bound: F
    error_bound: F
    vertices: tuple
    triangles: tuple
    edge_polylines: tuple
    boundary_uses: tuple

    def __init__(self,*args,**kwargs):
        raise TypeError('ExtrusionMesh is factory-only; use tessellate')

    @property
    def history(self):return self.feature_result.history

    @property
    def curve_bound(self):return F(0)

    @property
    def quarter_segments(self):return 0


def _uv(chart,world):
    """Solve the actual affine chart exactly, including its retained Frame."""
    origin,a,b=_plane_basis(chart);patch=chart.patch
    p=tuple((x-F(o))/F(patch.scale)-c for x,o,c in zip(world,patch.origin,origin))
    aa,ab,bb=(sum(x*y for x,y in zip(u,v)) for u,v in ((a,a),(a,b),(b,b)))
    pa,pb=(sum(x*y for x,y in zip(p,d)) for d in (a,b))
    det=aa*bb-ab*ab
    if det<=0:raise ArithmeticError('source affine chart is degenerate')
    u,v=(pa*bb-pb*ab)/det,(pb*aa-pa*ab)/det
    if (not 0<=u<=1 or not 0<=v<=1 or
            tuple(u*x+v*y for x,y in zip(a,b))!=p):
        raise ArithmeticError('vertex left actual affine source chart')
    return u,v


def _rounded_guard(body,vertices,triangles):
    xyz=tuple(tuple(F(x) for x in v.rounded_position) for v in vertices)
    n=len(body.profile.vertices);lower,upper=xyz[:n],xyz[n:]
    if (tuple(p[:2] for p in lower)!=tuple(p[:2] for p in upper) or
            len({p[2] for p in lower})!=1 or len({p[2] for p in upper})!=1 or
            not lower[0][2]<upper[0][2]):
        raise MeshBudgetError('rounded prism layers collapsed or lost correspondence')
    for i,a in enumerate(lower):
        b=lower[(i+1)%n]
        if any(_det(a,b,p)<=0 for j,p in enumerate(lower) if j not in (i,(i+1)%n)):
            raise MeshBudgetError('rounded profile lost global strict convexity')
    for triangle in triangles:
        points=tuple(xyz[i] for i in triangle.vertices);a,b,c=points
        normal=_cross(tuple(y-x for x,y in zip(a,b)),tuple(y-x for x,y in zip(a,c)))
        face=body.faces[triangle.face_id-1]
        if face.kind=='convex_polygon':
            z=lower[0][2] if face.orientation<0 else upper[0][2]
            if any(p[2]!=z for p in points):raise MeshBudgetError('rounded cap left its plane')
            sign=normal[2]*face.orientation
        else:
            i=face.id-3;a,b=lower[i],lower[(i+1)%n]
            if any(_det(a,b,p)!=0 or not lower[0][2]<=p[2]<=upper[0][2] for p in points):
                raise MeshBudgetError('rounded side left its actual extrusion plane')
            sign=normal[0]*(b[1]-a[1])-normal[1]*(b[0]-a[0])
        if sign<=0:raise MeshBudgetError('rounded triangle collapsed or reversed')


def tessellate(feature_result,*,tolerance):
    """Return the complete actual planar boundary, bounded after float rounding."""
    if type(feature_result) is not ExtrusionResult:
        raise TypeError('expected actual owned ExtrusionResult')
    if type(tolerance) not in (int,float,F):
        raise TypeError('tolerance requires int, Fraction or finite binary64')
    if type(tolerance) is float and not math.isfinite(tolerance):
        raise ValueError('tolerance must be positive and finite')
    tolerance=F(tolerance)
    if tolerance<=0:raise ValueError('tolerance must be positive')
    body=feature_result.solid;vertices=[]
    for record in body.vertices:
        point=tuple(F(o)+F(body.frame.scale)*x for o,x in zip(body.frame.origin,record.position))
        vertices.append(MeshVertex(len(vertices),point,tuple(float(x) for x in point),('vertex',record.id)))
    polylines=tuple(EdgePolyline(e.id,(F(0),F(1)),(e.start,e.end)) for e in body.edges)
    uses=[]
    for c in body.coedges:
        poly=polylines[c.edge_slot];params,ids=poly.parameters,poly.vertices
        if c.orientation<0:params,ids=params[::-1],ids[::-1]
        uses.append(BoundaryUse(c.id,body.faces[c.face_slot].id,poly.edge_id,c.orientation,
                                params,ids,tuple(body.coedge_uv(c.id,t) for t in params)))
    triangles=[]
    for face in body.faces:
        if len(face.loop_slots)!=1:raise ArithmeticError('source face needs one convex loop')
        loop=body.loops[face.loop_slots[0]];word=[]
        for cs in loop.coedge_slots:
            use=uses[cs]
            if word and word[-1]!=use.vertices[0]:raise ArithmeticError('source boundary does not join')
            word.extend(use.vertices if not word else use.vertices[1:])
        if len(word)<4 or word[0]!=word[-1]:raise ArithmeticError('source face loop is not closed')
        word=word[:-1];chart=body.charts[face.chart_slots[0]]
        _,u,v=_plane_basis(chart);reference=_cross(u,v)
        for j in range(1,len(word)-1):
            ids=(word[0],word[j],word[j+1]);a,b,c=(vertices[i].position for i in ids)
            normal=_cross(tuple(y-x for x,y in zip(a,b)),tuple(y-x for x,y in zip(a,c)))
            if sum(x*y for x,y in zip(normal,reference))*face.orientation<=0:
                raise ArithmeticError('exact source triangle collapsed or reversed')
            corners=tuple(MeshCorner(chart.id,_uv(chart,vertices[i].position)) for i in ids)
            triangles.append(MeshTriangle(len(triangles),ids,face.id,corners))
    n=len(body.profile.vertices)
    if len(vertices)!=2*n or len(triangles)!=4*n-4:
        raise ArithmeticError('incomplete extrusion boundary mesh')
    rounding=max(sum(abs(F(b)-a) for a,b in zip(v.position,v.rounded_position)) for v in vertices)
    if rounding>tolerance:raise MeshBudgetError('actual rounding exceeds requested tolerance')
    _rounded_guard(body,vertices,triangles)
    mesh=object.__new__(ExtrusionMesh)
    values=(feature_result,body,tolerance,rounding,rounding,tuple(vertices),tuple(triangles),polylines,tuple(uses))
    for name,value in zip(ExtrusionMesh.__dataclass_fields__,values):object.__setattr__(mesh,name,value)
    return mesh
