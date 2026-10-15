"""Exact complete one-vertical-edge fillet of an actual owned source box."""
from collections import defaultdict
from dataclasses import dataclass
from fractions import Fraction as F
from itertools import islice
from math import comb
import math
import operator
import struct
import time

from . import Lineage, Patch
from .body import (BodySnapshot,Frame,Vertex,Curve,Edge,Coedge,UVMap,Loop,Face,Chart,
                   _namespace,_packed,INVALID,REJECTED,INTERIOR,BOUNDARY)
from .boolean import EntityKey,Measure


class UnsupportedFillet(ValueError):
    """Source or selection is outside the complete canonical box-edge family."""


def _check(condition,message):
    if not condition:raise ArithmeticError(message)


def _rational(x):
    if type(x) in (F,int):return F(x)
    if type(x) is float:
        if not math.isfinite(x):raise ValueError('geometry must be finite')
        return F(x)
    raise TypeError('geometry requires int, Fraction or finite binary64')


def _tuple(values,width):
    values=tuple(islice(iter(values),width+1))
    if len(values)!=width:raise ValueError('wrong coordinate count')
    return tuple(_rational(x) for x in values)


def _parameter(x):
    x=_rational(x)
    if not 0<=x<=1:raise ValueError('parameter outside [0,1]')
    return x


def _slot(x,count,base=0):
    if isinstance(x,bool):raise TypeError('indices cannot be bool')
    x=operator.index(x)
    if not base<=x<count+base:raise IndexError('index outside fillet table')
    return x-base


def _publish(cls,**values):
    result=object.__new__(cls)
    for name,value in values.items():object.__setattr__(result,name,value)
    return result


def _rotate(x,y,q):return ((x,y),(-y,x),(-x,-y),(y,-x))[q]


def _circle(t,q):
    d=1+t*t
    return _rotate((1-t*t)/d,2*t/d,q)


@dataclass(frozen=True,slots=True)
class Map2:
    kind:str
    data:tuple

    def __post_init__(self):
        if self.kind=='affine':values=_tuple(self.data,6)
        elif self.kind=='arc':
            values=tuple(islice(iter(self.data),5))
            if len(values)!=4 or type(values[3]) is not int or not 0<=values[3]<4:raise ValueError('arc requires center,radius,integer quarter')
            values=(*_tuple(values[:3],3),values[3])
        else:raise ValueError('unknown exact map kind')
        object.__setattr__(self,'data',values)

    def evaluate(self,u,v):
        u,v=_parameter(u),_parameter(v);d=self.data
        if self.kind=='affine':return d[0]+d[1]*u+d[2]*v,d[3]+d[4]*u+d[5]*v
        x,y=_circle(u,d[3]);return d[0]+d[2]*x,d[1]+d[2]*y


@dataclass(frozen=True,slots=True)
class Origin:
    target:EntityKey
    relation:str
    parents:tuple
    role:str
    source_t_owner:object=None
    source_t_relation:object=None
    source_t:object=None
    source_uv_owner:object=None
    source_uv:object=None
    orientation:int=1

    def __post_init__(self):
        if type(self.target) is not EntityKey:raise TypeError('target requires EntityKey')
        if self.relation not in ('retained','modified','generated'):raise ValueError('invalid origin relation')
        parents=tuple(islice(iter(self.parents),4))
        if not 1<=len(parents)<=3 or any(type(p) is not EntityKey for p in parents) or len(set(parents))!=len(parents):raise ValueError('one to three distinct typed parents required')
        object.__setattr__(self,'parents',parents)
        if (self.source_t_owner is None)!=(self.source_t is None) or (self.source_t_relation is None)!=(self.source_t is None):raise ValueError('source parameter owner/relation/map must be paired')
        if self.source_t is not None:
            if type(self.source_t_owner) is not EntityKey or self.source_t_owner.kind not in ('edge','coedge') or self.source_t_relation not in ('coincident','spine'):raise ValueError('invalid source parameter owner/relation')
            object.__setattr__(self,'source_t',_tuple(self.source_t,3))
        if (self.source_uv_owner is None)!=(self.source_uv is None):raise ValueError('UV owner and map must be paired')
        if self.source_uv is not None and (type(self.source_uv_owner) is not EntityKey or self.source_uv_owner.kind not in ('face','chart') or type(self.source_uv) is not Map2):raise ValueError('UV requires one original face/chart owner')
        if type(self.orientation) is not int or self.orientation not in (-1,1):raise ValueError('integer orientation sign required')


@dataclass(frozen=True,slots=True)
class HistoryRecord:
    source:EntityKey
    status:str
    images:tuple
    generated:tuple

    def __post_init__(self):
        if type(self.source) is not EntityKey:raise TypeError('history source requires EntityKey')
        if self.status not in ('retained','modified','deleted'):raise ValueError('invalid source disposition')
        for name in ('images','generated'):
            values=tuple(getattr(self,name))
            if any(type(x) is not EntityKey for x in values) or len(set(values))!=len(values):raise ValueError('distinct typed targets required')
            object.__setattr__(self,name,values)
        if len(self.images)!=(0 if self.status=='deleted' else 1) or any(x.kind!=self.source.kind for x in self.images):raise ValueError('wrong source image disposition')


@dataclass(frozen=True,slots=True,init=False)
class History:
    records:tuple
    origins:tuple

    def __init__(self,*args,**kwargs):raise TypeError('History is factory-only')

    def from_input(self,key):
        if type(key) is EntityKey:
            for row in self.records:
                if row.source==key:return row
        raise KeyError(key)

    def origin(self,key):
        if type(key) is EntityKey:
            for row in self.origins:
                if row.target==key:return row
        raise KeyError(key)


@dataclass(frozen=True,slots=True)
class ContactRail:
    edge:EntityKey
    source_face:EntityKey
    source_coedge:EntityKey
    blend_face:EntityKey
    blend_u:int
    source_uv:Map2
    blend_uv:Map2
    source_t:tuple
    normal:tuple

    def __post_init__(self):
        for name,kind in (('edge','edge'),('source_face','face'),('source_coedge','coedge'),('blend_face','face')):
            key=getattr(self,name)
            if type(key) is not EntityKey or key.kind!=kind:raise TypeError('contact requires correctly typed keys')
        if type(self.blend_u) is not int or self.blend_u not in (0,1):raise ValueError('contact endpoint must be integer0/1')
        if type(self.source_uv) is not Map2 or type(self.blend_uv) is not Map2:raise TypeError('contact requires exact maps')
        object.__setattr__(self,'source_t',_tuple(self.source_t,3));object.__setattr__(self,'normal',_tuple(self.normal,3))


@dataclass(frozen=True,slots=True,init=False)
class PointClassification:
    solid:object
    point:tuple
    label:str
    boundary_faces:tuple
    def __init__(self,*args,**kwargs):raise TypeError('PointClassification is factory-only')


@dataclass(frozen=True,slots=True,init=False)
class FilletedSolid:
    source:BodySnapshot
    edge:EntityKey
    radius:F
    body_id:int
    revision:int
    frame:Frame
    vertices:tuple
    curves:tuple
    edges:tuple
    coedges:tuple
    loops:tuple
    faces:tuple
    charts:tuple
    patches:tuple
    volume:Measure
    def __init__(self,*args,**kwargs):raise TypeError('FilletedSolid is factory-only; use fillet_edge')

    def classify(self,chart_slot,u,v):
        fs=_slot(chart_slot,7)
        if any(type(x) not in (int,F,float) for x in (u,v)):raise TypeError('trim coordinates require exact numeric values')
        if any(type(x) is float and not math.isfinite(x) for x in (u,v)):return INVALID
        u,v=_rational(u),_rational(v)
        if not 0<=u<=1 or not 0<=v<=1:return INVALID
        if fs==6:return BOUNDARY if u in (0,1) or v in (0,1) else INTERIOR
        point=_plane_point(self.source.patches[fs],u,v)
        inside,band=_section(self,point[0],point[1])
        if not inside:return REJECTED
        return BOUNDARY if u in (0,1) or v in (0,1) or band else INTERIOR

    def classify_point(self,point):
        point=_tuple(point,3)
        x,y,z=tuple((p-F(o))/F(self.frame.scale) for p,o in zip(point,self.frame.origin))
        inside,band=_section(self,x,y)
        if not inside or not F(-1,2)<=z<=F(1,2):label,faces='exterior',()
        else:
            faces=tuple(i for i,yes in ((1,z==F(1,2)),(2,z==F(-1,2)),(3,y==F(-3,4)),(4,y==F(3,4)),(5,x==F(3,4)),(6,x==F(-3,4)),(7,band)) if yes)
            label='boundary' if faces else 'interior'
        return _publish(PointClassification,solid=self,point=point,label=label,boundary_faces=faces)

    def edge_point(self,edge_id,t):
        e=self.edges[_slot(edge_id,15,1)];t=_parameter(t)
        local=_curve_point(self.curves[e.curve_slot],t)
        return tuple(F(o)+F(self.frame.scale)*p for o,p in zip(self.frame.origin,local))

    def coedge_uv(self,coedge_id,t):
        c=self.coedges[_slot(coedge_id,30,1)];t=_parameter(t)
        return _map_from_uv(c.uv_map).evaluate(t,0)

    def topology_input(self):
        from .adjacency import Source,TopologyInput
        def sources(rows):return tuple(Source(0,self.body_id,self.revision,r.id) for r in rows)
        return TopologyInput(sources(self.edges),sources(self.faces),sources(self.coedges),tuple(c.edge_slot for c in self.coedges),tuple(c.face_slot for c in self.coedges),tuple(c.orientation for c in self.coedges),snapshots=())


@dataclass(frozen=True,slots=True,init=False)
class FilletResult:
    source:BodySnapshot
    edge:EntityKey
    radius:F
    solid:FilletedSolid
    history:History
    contacts:tuple
    timings:tuple
    def __init__(self,*args,**kwargs):raise TypeError('FilletResult is factory-only; use fillet_edge')


_TABLES=(('vertex','vertices'),('edge','edges'),('coedge','coedges'),('loop','loops'),('face','faces'),('chart','charts'))
_T=(F(0),F(1),F(0));_V=(F(0),F(0),F(1))
_IDENTITY=Map2('affine',(0,1,0,0,0,1))
_CORNERS=((-F(3,4),-F(3,4)),(F(3,4),-F(3,4)),(F(3,4),F(3,4)),(-F(3,4),F(3,4)))
_EDGE_ENDS=((0,1),(1,2),(2,3),(3,0),(4,5),(5,6),(6,7),(7,4),(0,4),(1,5),(2,6),(3,7))
_PLANES=(('top',(-.75,-.75,.5),(1.5,0.,0.),(0.,1.5,0.),1),('bottom',(-.75,-.75,-.5),(1.5,0.,0.),(0.,1.5,0.),-1),('front',(-.75,-.75,-.5),(1.5,0.,0.),(0.,0.,1.),1),('back',(-.75,.75,-.5),(1.5,0.,0.),(0.,0.,1.),-1),('right',(.75,-.75,-.5),(0.,1.5,0.),(0.,0.,1.),1),('left',(-.75,-.75,-.5),(0.,1.5,0.),(0.,0.,1.),-1))


def _key(body,kind,id):return EntityKey(body.body_id,body.revision,kind,id)
def _keys(body):return tuple(_key(body,kind,r.id) for kind,table in _TABLES for r in getattr(body,table))


def _float_bits(values,expected):
    return type(values) is tuple and len(values)==len(expected) and all(type(x) is float for x in values) and struct.pack('<'+'d'*len(values),*values)==struct.pack('<'+'d'*len(expected),*expected)


def _typed_key(key):
    return type(key) is EntityKey and type(key.kind) is str and key.kind in dict(_TABLES) and all(type(x) is int and 0<x<1<<64 for x in (key.body_id,key.revision,key.entity_id))


def _typed_map(m):
    return type(m) is Map2 and type(m.kind) is str and type(m.data) is tuple and ((m.kind=='affine' and len(m.data)==6 and all(type(x) is F for x in m.data)) or (m.kind=='arc' and len(m.data)==4 and all(type(x) is F for x in m.data[:3]) and type(m.data[3]) is int and 0<=m.data[3]<4))


def _admit(source):
    if type(source) is not BodySnapshot:raise TypeError('fillet requires exact factory BodySnapshot')
    if type(source.shape) is not str or source.shape!='box':raise UnsupportedFillet('only canonical source box is supported')
    _check(type(source.body_id) is int and 0<source.body_id<1<<64 and type(source.revision) is int and source.revision==1,'source namespace/revision differs')
    frame=source.frame
    _check(type(frame) is Frame and type(frame.origin) is tuple and len(frame.origin)==3 and all(type(x) is float and math.isfinite(x) and abs(x)<=16 for x in frame.origin) and type(frame.scale) is float and frame.scale in (.25,.5,1.,2.,4.),'typed source Frame differs')
    classes=(Vertex,Edge,Coedge,Loop,Face,Chart)
    for (_,table),count,cls in zip(_TABLES,(8,12,24,6,6,6),classes):
        rows=getattr(source,table)
        _check(type(rows) is tuple and len(rows)==count and all(type(r) is cls and type(r.id) is int and r.id==i+1 for i,r in enumerate(rows)),'source typed entity coverage differs')
    _check(type(source.curves) is tuple and len(source.curves)==12 and type(source.patches) is tuple and len(source.patches)==6,'source carrier coverage differs')
    positions=tuple((float(x),float(y),z) for z in (-.5,.5) for x,y in _CORNERS)
    for v,p in zip(source.vertices,positions):_check(_float_bits(v.position,p),'source original vertex bits differ')
    for es,(e,(a,b)) in enumerate(zip(source.edges,_EDGE_ENDS)):
        _check(all(type(x) is int for x in (e.start,e.end,e.curve_slot)) and (e.start,e.end,e.curve_slot)==(a,b,es),'source edge incidence differs')
        c=source.curves[es]
        _check(type(c) is Curve and c.kind=='line' and _float_bits(c.data,positions[a]+positions[b]),'source original curve bits differ')
    lookup={frozenset((a,b)):i for i,(a,b) in enumerate(_EDGE_ENDS)};vlookup={p:i for i,p in enumerate(positions)}
    for fs,(name,o,U,V,sign) in enumerate(_PLANES):
        face=source.faces[fs];loop=source.loops[fs];chart=source.charts[fs];cp=source.patches[fs]
        _check(face==Face(fs+1,name,'rectangle',sign,(fs,),(fs,)) and type(face.orientation) is int and all(type(w) is tuple and all(type(x) is int for x in w) for w in (face.loop_slots,face.chart_slots)),'source face ownership/sense differs')
        _check(type(loop.face_slot) is int and loop.face_slot==fs and type(loop.hole) is bool and not loop.hole and type(loop.coedge_slots) is tuple and loop.coedge_slots==tuple(range(4*fs,4*fs+4)) and all(type(x) is int for x in loop.coedge_slots),'source loop word differs')
        _check(type(chart.face_slot) is int and type(chart.quadrant) is int and (chart.face_slot,chart.quadrant)==(fs,-1) and chart.patch is cp,'source chart ownership differs')
        expected=tuple(float(x) for u in (0,1) for v in (0,1) for x in (*tuple(o[k]+u*U[k]+v*V[k] for k in range(3)),1.))
        _check(type(cp) is Patch and type(cp.degree_u) is int and type(cp.degree_v) is int and (cp.degree_u,cp.degree_v)==(1,1) and _float_bits(cp.controls,expected) and _float_bits(cp.uv,(0.,1.,0.,1.)) and _float_bits(cp.origin,frame.origin) and type(cp.scale) is float and struct.pack('<d',cp.scale)==struct.pack('<d',frame.scale),'source native control/frame bits differ')
        lin=cp.lineage
        _check(type(lin) is Lineage and all(type(x) is int for x in (lin.body_id,lin.entity_kind,lin.entity_id,lin.revision,lin.chart_id)) and lin==Lineage(source.body_id,5,fs+1,1,fs+1),'source native lineage differs')
        corners=((0.,0.),(1.,0.),(1.,1.),(0.,1.)) if sign==1 else ((0.,0.),(0.,1.),(1.,1.),(1.,0.))
        for cs,(a,b) in zip(loop.coedge_slots,zip(corners,corners[1:]+corners[:1])):
            points=tuple(tuple(o[k]+uv[0]*U[k]+uv[1]*V[k] for k in range(3)) for uv in (a,b));start,end=(vlookup[p] for p in points);es=lookup[frozenset((start,end))]
            s=1 if _EDGE_ENDS[es][0]==start else -1;c=source.coedges[cs]
            _check(all(type(x) is int for x in (c.edge_slot,c.face_slot,c.loop_slot,c.orientation)) and (c.edge_slot,c.face_slot,c.loop_slot,c.orientation)==(es,fs,fs,s) and type(c.uv_map) is UVMap and c.uv_map.kind=='line' and _float_bits(c.uv_map.data,a+b if s==1 else b+a),'source coedge incidence/UV bits differ')
    _check(type(source.packed_tables) is tuple and all(type(row) is tuple and len(row)==2 and type(row[0]) is str and type(row[1]) is bytes for row in source.packed_tables) and source.packed_tables==_packed(source),'source packed export differs')


def _geometry(solid):
    k=solid.edge.entity_id-9;q=(k+2)%4;r=solid.radius;c=F(3,4)-r
    cx,cy=_rotate(c,c,q)
    return k,q,c,cx,cy


def _section(solid,x,y):
    if abs(x)>F(3,4) or abs(y)>F(3,4):return False,False
    k,q,c,cx,cy=_geometry(solid);sx=1 if _CORNERS[k][0]>0 else -1;sy=1 if _CORNERS[k][1]>0 else -1
    X,Y=sx*x,sy*y;d=(X-c)**2+(Y-c)**2-solid.radius**2
    return X<=c or Y<=c or d<=0, X>=c and Y>=c and d==0


def _plane_point(cp,u,v):
    d=tuple(F(x) for x in cp.controls)
    return tuple(d[k]+u*(d[8+k]-d[k])+v*(d[4+k]-d[k]) for k in range(3))


def _plane_uv(cp,p):
    o=_plane_point(cp,0,0);a=_plane_point(cp,1,0);b=_plane_point(cp,0,1)
    i=next(k for k in range(3) if a[k]!=o[k]);j=next(k for k in range(3) if b[k]!=o[k])
    return (p[i]-o[i])/(a[i]-o[i]),(p[j]-o[j])/(b[j]-o[j])


def _curve_point(curve,t):
    if curve.kind=='line':return tuple((1-t)*F(a)+t*F(b) for a,b in zip(curve.data[:3],curve.data[3:]))
    _check(curve.kind=='quarter_arc','unsupported output curve')
    cx,cy,r,q,z=curve.data;x,y=_circle(t,q);return cx+r*x,cy+r*y,z


def _map_from_uv(uv):
    if uv.kind=='arc':return Map2('arc',uv.data)
    _check(uv.kind=='line','unsupported output UV kind')
    a,b,c,d=map(F,uv.data);return Map2('affine',(a,c-a,0,b,d-b,0))


def _line_map(a,b):return Map2('affine',(a[0],b[0]-a[0],0,a[1],b[1]-a[1],0))
def _uv_record(m):
    if m.kind=='arc':return UVMap('arc',m.data)
    a=m.evaluate(0,0);b=m.evaluate(1,0);return UVMap('line',a+b)


def _layout(source,edge,radius):
    k=edge.entity_id-9;selected=edge.entity_id-1;q=(k+2)%4;c=F(3,4)-radius;cx,cy=_rotate(c,c,q)
    oldv=tuple(i for i in range(8) if i not in (k,k+4));vm={old:new for new,old in enumerate(oldv)}
    olde=tuple(i for i in range(12) if i!=selected);em={old:new for new,old in enumerate(olde)}
    oldco=tuple(i for i,co in enumerate(source.coedges) if co.edge_slot!=selected);cm={old:new for new,old in enumerate(oldco)}
    rail_sources=[]
    for t in (F(0),F(1)):
        nx,ny=_circle(t,q)
        fs=4 if nx==1 else 5 if nx==-1 else 3 if ny==1 else 2
        cs=next(i for i,co in enumerate(source.coedges) if co.face_slot==fs and co.edge_slot==selected)
        rail_sources.append((fs,cs))
    return k,q,cx,cy,oldv,vm,olde,em,oldco,cm,tuple(rail_sources)


def _range(source,es,k,r):
    e=source.edges[es];delta=2*r/3
    if es<8 and e.start in (k,k+4):return delta,F(1)
    if es<8 and e.end in (k,k+4):return F(0),1-delta
    return F(0),F(1)


def _source_use_map(source,co,t0,t1):
    m=_map_from_uv(co.uv_map)
    return _line_map(m.evaluate(t0,0),m.evaluate(t1,0))


def _band_patch(solid):
    k,q,c,cx,cy=_geometry(solid);controls=[]
    for a,b,w in ((1,0,1),(1,1,1),(0,2,2)):
        a,b=_rotate(a,b,q)
        for z in (F(-1,2),F(1,2)):controls.extend(map(float,(cx*w+solid.radius*a,cy*w+solid.radius*b,z*w,w)))
    return Patch(2,1,tuple(controls),Lineage(solid.body_id,5,7,1,7),(0.,1.,0.,1.),solid.frame.origin,solid.frame.scale)


def _words(source,k,cm,new_side):
    words=[]
    for fs,loop in enumerate(source.loops):
        word=[]
        for cs in loop.coedge_slots:
            co=source.coedges[cs];e=source.edges[co.edge_slot]
            if cs in cm:
                word.append(cm[cs])
                if fs<2 and (e.end if co.orientation==1 else e.start) in (k,k+4):word.append(23 if fs==0 else 22)
            else:word.append(new_side[cs])
        words.append(tuple(word))
    return tuple(words)+((26,27,28,29),)


def _assemble(source,edge,radius):
    layout=_layout(source,edge,radius);k,q,cx,cy,oldv,vm,olde,em,oldco,cm,rails=layout
    body_id=_namespace();vertices=[Vertex(i+1,source.vertices[old].position) for i,old in enumerate(oldv)]
    for z in (F(-1,2),F(1,2)):
        for t in (F(0),F(1)):
            x,y=_circle(t,q);vertices.append(Vertex(len(vertices)+1,(cx+radius*x,cy+radius*y,z)))
    curves=[];edges=[]
    def add_edge(a,b,curve):
        es=len(edges);curves.append(curve);edges.append(Edge(es+1,a,b,es))
    for old in olde:
        e=source.edges[old];modified=old<8 and (e.start in (k,k+4) or e.end in (k,k+4))
        def endpoint(v):
            if v in vm:return vm[v]
            return 6+2*(v//4)+(0 if old%4==(k-1)%4 else 1)
        a,b=endpoint(e.start),endpoint(e.end)
        curve=Curve('line',tuple(F(x) for x in vertices[a].position+vertices[b].position)) if modified else source.curves[e.curve_slot]
        add_edge(a,b,curve)
    add_edge(6,7,Curve('quarter_arc',(cx,cy,radius,q,F(-1,2))))
    add_edge(8,9,Curve('quarter_arc',(cx,cy,radius,q,F(1,2))))
    add_edge(6,8,Curve('line',vertices[6].position+vertices[8].position))
    add_edge(7,9,Curve('line',vertices[7].position+vertices[9].position))
    coedges=[]
    for old in oldco:
        co=source.coedges[old];t0,t1=_range(source,co.edge_slot,k,radius)
        uv=co.uv_map if (t0,t1)==(0,1) else _uv_record(_source_use_map(source,co,t0,t1))
        coedges.append(Coedge(len(coedges)+1,em[co.edge_slot],co.face_slot,co.loop_slot,co.orientation,uv))
    capuv=Map2('arc',((cx+F(3,4))/F(3,2),(cy+F(3,4))/F(3,2),2*radius/3,q))
    def use(es,fs,sign,uv):coedges.append(Coedge(len(coedges)+1,es,fs,fs,sign,uv))
    use(11,1,-1,_uv_record(capuv));use(12,0,1,_uv_record(capuv))
    for j,(fs,cs) in enumerate(rails):
        a,b=(vertices[6+j].position,vertices[8+j].position)
        use(13+j,fs,source.coedges[cs].orientation,UVMap('line',_plane_uv(source.patches[fs],a)+_plane_uv(source.patches[fs],b)))
    for es,sign,uv in ((11,1,(0,0,1,0)),(14,1,(1,0,1,1)),(12,-1,(0,1,1,1)),(13,-1,(0,0,0,1))):use(es,6,sign,UVMap('line',tuple(F(x) for x in uv)))
    words=_words(source,k,cm,{rails[j][1]:24+j for j in range(2)})
    loops=tuple(Loop(i+1,i,False,word) for i,word in enumerate(words))
    faces=tuple(Face(i+1,old.name,'fillet_cap' if i<2 else 'rectangle',old.orientation,(i,),(i,)) for i,old in enumerate(source.faces))+(Face(7,'fillet','fillet_band',1,(6,),(6,)),)
    solid=_publish(FilletedSolid,source=source,edge=edge,radius=radius,body_id=body_id,revision=1,frame=source.frame,vertices=tuple(vertices),curves=tuple(curves),edges=tuple(edges),coedges=tuple(coedges),loops=loops,faces=faces,charts=(),patches=(),volume=Measure((F(9,4)-radius**2)*F(source.frame.scale)**3,radius**2*F(source.frame.scale)**3/4))
    patches=tuple(Patch(p.degree_u,p.degree_v,p.controls,Lineage(body_id,5,i+1,1,i+1),p.uv,p.origin,p.scale) for i,p in enumerate(source.patches))+(_band_patch(solid),)
    object.__setattr__(solid,'patches',patches);object.__setattr__(solid,'charts',tuple(Chart(i+1,i,-1 if i<6 else q,p) for i,p in enumerate(patches)))
    origins=_origins(solid,layout);records=[]
    for key in _keys(source):
        images=tuple(o.target for o in origins if o.relation!='generated' and key in o.parents);generated=tuple(o.target for o in origins if o.relation=='generated' and key in o.parents)
        relation=next((o.relation for o in origins if o.target in images),'deleted')
        records.append(HistoryRecord(key,relation,images,generated))
    history=_publish(History,records=tuple(records),origins=origins);contacts=[]
    for j,(fs,cs) in enumerate(rails):
        normal=(*_circle(F(j),q),F(0));uv=_line_map(_plane_uv(source.patches[fs],vertices[6+j].position),_plane_uv(source.patches[fs],vertices[8+j].position))
        contacts.append(ContactRail(_key(solid,'edge',14+j),_key(source,'face',fs+1),_key(source,'coedge',cs+1),_key(solid,'face',7),j,uv,Map2('affine',(j,0,0,0,1,0)),_T,normal))
    return solid,history,tuple(contacts)


def _origins(solid,layout):
    source=solid.source;r=solid.radius;S=solid.edge;k,q,cx,cy,oldv,vm,olde,em,oldco,cm,rails=layout
    origins=[];changedfaces={0,1,rails[0][0],rails[1][0]}
    arcmap=Map2('arc',((cx+F(3,4))/F(3,2),(cy+F(3,4))/F(3,2),2*r/3,q))
    def emit(kind,id,relation,parents,role,towner=None,trelation=None,t=None,uvowner=None,uv=None,sign=1):
        origins.append(Origin(_key(solid,kind,id),relation,parents,role,towner,trelation,t,uvowner,uv,sign))
    for kind,table in _TABLES:
        for id,row in enumerate(getattr(solid,table),1):
            original=oldv[id-1] if kind=='vertex' and id<=6 else olde[id-1] if kind=='edge' and id<=11 else oldco[id-1] if kind=='coedge' and id<=22 else id-1 if kind in ('loop','face','chart') and id<=6 else None
            if original is not None:
                parent=_key(source,kind,original+1);towner=trelation=t=uvowner=uv=None;sign=1
                if kind=='vertex':relation='retained'
                elif kind in ('edge','coedge'):
                    es=original if kind=='edge' else source.coedges[original].edge_slot;t0,t1=_range(source,es,k,r)
                    relation='retained' if (t0,t1)==(0,1) else 'modified';towner=parent;trelation='coincident';t=(t0,t1-t0,F(0))
                    if kind=='coedge':
                        co=source.coedges[original];uvowner=_key(source,'face',co.face_slot+1);uv=_source_use_map(source,co,t0,t1);sign=co.orientation
                else:
                    relation='modified' if original in changedfaces else 'retained';uvowner=_key(source,'chart' if kind=='chart' else 'face',original+1);uv=_IDENTITY;sign=source.faces[original].orientation
                emit(kind,id,relation,(parent,),'source_'+kind,towner,trelation,t,uvowner,uv,sign);continue
            if kind=='vertex':
                level,j=divmod(id-7,2);fs,cs=rails[j];endpoint=_key(source,'vertex',k+1+4*level);pos=solid.vertices[id-1].position;uvpoint=_plane_uv(source.patches[fs],pos)
                emit(kind,id,'generated',(endpoint,S,_key(source,'face',fs+1)),('lower_' if level==0 else 'upper_')+('incoming_vertex' if j==0 else 'outgoing_vertex'),S,'spine',(F(level),F(0),F(0)),_key(source,'face',fs+1),Map2('affine',(uvpoint[0],0,0,uvpoint[1],0,0)))
            elif kind in ('edge','coedge'):
                role=(('lower_arc','upper_arc','incoming_rail','outgoing_rail')[id-12] if kind=='edge' else ('lower_cap_arc','upper_cap_arc','incoming_side_rail','outgoing_side_rail','band_lower_arc','band_outgoing_rail','band_upper_arc','band_incoming_rail')[id-23])
                es=id-1 if kind=='edge' else solid.coedges[id-1].edge_slot;sign=1 if kind=='edge' else solid.coedges[id-1].orientation
                if es in (11,12):
                    level=es-11;cap=0 if level else 1;uvowner=_key(source,'face',cap+1);parents=(S,_key(source,'vertex',k+1+4*level),uvowner)
                    emit(kind,id,'generated',parents,role,S,'spine',(F(level),F(0),F(0)),uvowner,arcmap,sign)
                else:
                    j=es-13;fs,cs=rails[j];face=_key(source,'face',fs+1);parents=(S,face) if kind=='edge' else (S,_key(source,'coedge',cs+1),face)
                    uv=_line_map(_plane_uv(source.patches[fs],solid.vertices[6+j].position),_plane_uv(source.patches[fs],solid.vertices[8+j].position))
                    emit(kind,id,'generated',parents,role,S,'spine',_T,face,uv,sign)
            else:emit(kind,id,'generated',(S,_key(source,'face',rails[0][0]+1),_key(source,'face',rails[1][0]+1)),'band_'+kind,S,'spine',_V)
    return tuple(origins)


# Exact univariate rational identities for complete curve-to-carrier maps.
def _add(a,b):return _trim(tuple((a[i] if i<len(a) else 0)+(b[i] if i<len(b) else 0) for i in range(max(len(a),len(b)))))
def _trim(a):
    while len(a)>1 and a[-1]==0:a=a[:-1]
    return tuple(a)
def _scale(a,s):return _trim(tuple(x*s for x in a))
def _mul(a,b):
    c=[F(0)]*(len(a)+len(b)-1)
    for i,x in enumerate(a):
        for j,y in enumerate(b):c[i+j]+=x*y
    return _trim(c)
def _pow(a,n):
    out=(F(1),)
    for _ in range(n):out=_mul(out,a)
    return out
def _uv_polynomials(m):
    d=m.data
    if m.kind=='affine':return (d[0],d[1]),(d[3],d[4]),(F(1),)
    D=(F(1),F(0),F(1))
    # Tuple negation is not arithmetic: rotate polynomial coefficient vectors.
    C0=(F(1),F(0),F(-1));S0=(F(0),F(2));C,S=((C0,S0),(_scale(S0,-1),C0),(_scale(C0,-1),_scale(S0,-1)),(S0,_scale(C0,-1)))[d[3]]
    return _add(_scale(D,d[0]),_scale(C,d[2])),_add(_scale(D,d[1]),_scale(S,d[2])),D
def _curve_polynomials(curve):
    d=curve.data
    if curve.kind=='line':return tuple((F(d[k]),F(d[k+3])-F(d[k])) for k in range(3)),(F(1),)
    cx,cy,r,q,z=d;U,V,D=_uv_polynomials(Map2('arc',(cx,cy,r,q)));return (U,V,_scale(D,z)),D
def _carrier_polynomials(cp,m):
    U,V,D=_uv_polynomials(m);du=_add(D,_scale(U,-1));dv=_add(D,_scale(V,-1));out=[(F(0),)]*4
    for i in range(cp.degree_u+1):
        for j in range(cp.degree_v+1):
            b=_scale(_mul(_mul(_pow(U,i),_pow(du,cp.degree_u-i)),_mul(_pow(V,j),_pow(dv,cp.degree_v-j))),comb(cp.degree_u,i)*comb(cp.degree_v,j));off=4*(i*(cp.degree_v+1)+j)
            out=[_add(out[k],_scale(b,F(cp.controls[off+k]))) for k in range(4)]
    return out
def _curve_on_carrier(curve,cp,m):
    A,D=_curve_polynomials(curve);B=_carrier_polynomials(cp,m)
    return all(_mul(A[k],B[3])==_mul(B[k],D) for k in range(3))


def _validate_candidate(solid,history,contacts):
    source=solid.source;_admit(source)
    _check(_typed_key(solid.edge) and (solid.edge.body_id,solid.edge.revision,solid.edge.kind)==(source.body_id,source.revision,'edge') and 9<=solid.edge.entity_id<=12 and type(solid.radius) is F,'actual typed selection/radius differs')
    k,q,cx,cy,oldv,vm,olde,em,oldco,cm,rails=layout=_layout(source,solid.edge,solid.radius)
    _check(type(solid) is FilletedSolid and solid.frame is source.frame and type(solid.body_id) is int and 0<solid.body_id<1<<64 and solid.body_id!=source.body_id and type(solid.revision) is int and solid.revision==1,'fresh source-bound solid differs')
    for (kind,table),count,cls in zip(_TABLES,(10,15,30,7,7,7),(Vertex,Edge,Coedge,Loop,Face,Chart)):
        rows=getattr(solid,table);_check(type(rows) is tuple and len(rows)==count and all(type(row) is cls and type(row.id) is int and row.id==i+1 for i,row in enumerate(rows)),'complete typed output coverage differs')
    _check(type(solid.radius) is F and F(1,16)<=solid.radius<=F(1,2) and (solid.radius*256).denominator==1 and type(solid.edge) is EntityKey and solid.edge==_key(source,'edge',k+9),'actual selection/radius differs')
    _check(type(solid.curves) is tuple and len(solid.curves)==15 and type(solid.patches) is tuple and len(solid.patches)==7,'curve/carrier coverage differs')
    for i,old in enumerate(oldv):_check(_float_bits(solid.vertices[i].position,source.vertices[old].position),'retained original vertex bits differ')
    for level,z in enumerate((F(-1,2),F(1,2))):
        for j in range(2):
            x,y=_circle(F(j),q);point=(cx+solid.radius*x,cy+solid.radius*y,z);v=solid.vertices[6+2*level+j]
            _check(type(v.position) is tuple and all(type(x) is F for x in v.position) and v.position==point,'exact tangent endpoint differs')
    for es,e in enumerate(solid.edges):
        _check(all(type(x) is int for x in (e.start,e.end,e.curve_slot)) and 0<=e.start<10 and 0<=e.end<10 and e.curve_slot==es,'typed physical edge ownership differs')
        curve=solid.curves[es];_check(type(curve) is Curve and type(curve.data) is tuple,'typed curve differs')
        if es<11:
            old=olde[es];t0,t1=_range(source,old,k,solid.radius);original=source.curves[source.edges[old].curve_slot]
            if (t0,t1)==(0,1):_check(curve.kind==original.kind and _float_bits(curve.data,original.data),'retained original curve bits differ')
            else:_check(curve==Curve('line',_curve_point(original,t0)+_curve_point(original,t1)) and all(type(x) is F for x in curve.data),'complete restricted source curve differs')
        elif es<13:_check(curve==Curve('quarter_arc',(cx,cy,solid.radius,q,F(-1,2) if es==11 else F(1,2))) and all(type(x) is F for x in (curve.data[0],curve.data[1],curve.data[2],curve.data[4])) and type(curve.data[3]) is int,'complete cap quarter arc differs')
        else:_check(curve==Curve('line',solid.vertices[6+es-13].position+solid.vertices[8+es-13].position) and all(type(x) is F for x in curve.data),'complete upward contact rail differs')
        for t,vs in ((F(0),e.start),(F(1),e.end)):_check(_curve_point(curve,t)==tuple(F(x) for x in solid.vertices[vs].position),'physical edge endpoint identity differs')
    changedfaces={0,1,rails[0][0],rails[1][0]};words=_words(source,k,cm,{rails[j][1]:24+j for j in range(2)});uses=defaultdict(list);links=defaultdict(lambda:defaultdict(list));neighbors=defaultdict(set);seen=[]
    for fs,face in enumerate(solid.faces):
        expected=Face(fs+1,source.faces[fs].name if fs<6 else 'fillet','fillet_cap' if fs<2 else 'rectangle' if fs<6 else 'fillet_band',source.faces[fs].orientation if fs<6 else 1,(fs,),(fs,))
        _check(face==expected and type(face.orientation) is int and all(type(w) is tuple and all(type(x) is int for x in w) for w in (face.loop_slots,face.chart_slots)),'actual source face sense/domain ownership differs')
        loop=solid.loops[fs];chart=solid.charts[fs];cp=solid.patches[fs]
        _check(type(loop.face_slot) is int and loop.face_slot==fs and type(loop.hole) is bool and not loop.hole and type(loop.coedge_slots) is tuple and all(type(x) is int for x in loop.coedge_slots) and loop.coedge_slots==words[fs],'complete changed boundary word differs')
        _check(type(chart.face_slot) is int and type(chart.quadrant) is int and (chart.face_slot,chart.quadrant)==(fs,-1 if fs<6 else q) and chart.patch is cp,'chart source ownership differs')
        wanted=source.patches[fs] if fs<6 else _band_patch(solid)
        _check(type(cp) is Patch and type(cp.degree_u) is int and type(cp.degree_v) is int and (cp.degree_u,cp.degree_v)==(wanted.degree_u,wanted.degree_v) and _float_bits(cp.controls,wanted.controls) and _float_bits(cp.uv,wanted.uv) and _float_bits(cp.origin,source.frame.origin) and type(cp.scale) is float and struct.pack('<d',cp.scale)==struct.pack('<d',source.frame.scale),'original/new exact carrier bits differ')
        lin=cp.lineage;_check(type(lin) is Lineage and all(type(x) is int for x in (lin.body_id,lin.entity_kind,lin.entity_id,lin.revision,lin.chart_id)) and lin==Lineage(solid.body_id,5,fs+1,1,fs+1),'actual output lineage differs')
        for cs in loop.coedge_slots:
            co=solid.coedges[cs];seen.append(cs)
            _check(all(type(x) is int for x in (co.edge_slot,co.face_slot,co.loop_slot,co.orientation)) and 0<=co.edge_slot<15 and co.face_slot==fs and co.loop_slot==fs and co.orientation in (-1,1) and type(co.uv_map) is UVMap and type(co.uv_map.data) is tuple,'actual occurrence typed ownership differs')
            if cs<22:
                old=source.coedges[oldco[cs]];t0,t1=_range(source,old.edge_slot,k,solid.radius)
                expecteduv=old.uv_map if (t0,t1)==(0,1) else _uv_record(_source_use_map(source,old,t0,t1))
                _check(co.edge_slot==em[old.edge_slot] and co.orientation==old.orientation and co.uv_map==expecteduv,'restricted source occurrence differs')
                _check(_float_bits(co.uv_map.data,old.uv_map.data) if (t0,t1)==(0,1) else all(type(x) is F for x in co.uv_map.data),'original/new occurrence UV payload types differ')
            else:
                expectedes=(11,12,13,14,11,14,12,13)[cs-22];expectedsign=(-1,1,source.coedges[rails[0][1]].orientation,source.coedges[rails[1][1]].orientation,1,1,-1,-1)[cs-22]
                _check((co.edge_slot,co.orientation)==(expectedes,expectedsign) and all(type(x) is F for x in co.uv_map.data[:3]) and (type(co.uv_map.data[3]) is int if co.uv_map.kind=='arc' else type(co.uv_map.data[3]) is F),'new occurrence role/exact UV differs')
            _check(_curve_on_carrier(solid.curves[co.edge_slot],cp,_map_from_uv(co.uv_map)),'whole rational edge/incident carrier identity fails');uses[co.edge_slot].append(co)
        word=loop.coedge_slots
        for previous,current in zip(word[-1:]+word[:-1],word):
            a,b=solid.coedges[previous],solid.coedges[current];ea,eb=solid.edges[a.edge_slot],solid.edges[b.edge_slot];end=1 if a.orientation==1 else 0;start=0 if b.orientation==1 else 1;vertex=(ea.start,ea.end)[end]
            _check(vertex==(eb.start,eb.end)[start],'loop is not identity-closed');x,y=(a.edge_slot,end),(b.edge_slot,start);links[vertex][x].append(y);links[vertex][y].append(x)
    _check(sorted(seen)==list(range(30)),'missing/duplicate occurrence owner')
    for es in range(15):
        pair=uses[es];_check(len(pair)==2 and sum(c.orientation for c in pair)==0 and pair[0].face_slot!=pair[1].face_slot,'ordinary opposed pair differs');a,b=(c.face_slot for c in pair);neighbors[a].add(b);neighbors[b].add(a)
    def visit(graph,start):
        found=set();todo=[start]
        while todo:
            x=todo.pop()
            if x not in found:found.add(x);todo.extend(graph[x])
        return found
    _check(visit(neighbors,0)==set(range(7)) and set(links)==set(range(10)),'closed shell connectivity differs')
    for vertex,graph in links.items():
        germs={(es,end) for es,e in enumerate(solid.edges) for end,v in enumerate((e.start,e.end)) if v==vertex}
        _check(len(germs)==3 and set(graph)==germs and all(len(v)==2 for v in graph.values()) and visit(graph,next(iter(graph)))==germs,'vertex link is not one three-port cycle')
    expected_origins=_origins(solid,layout)
    _check(type(history) is History and type(history.origins) is tuple and history.origins==expected_origins and type(history.records) is tuple and tuple(row.source for row in history.records)==_keys(source),'whole precise ancestry differs')
    for o in history.origins:
        _check(type(o) is Origin and type(o.parents) is tuple and type(o.orientation) is int,'typed origin differs')
        for key in (o.target,)+o.parents+tuple(x for x in (o.source_t_owner,o.source_uv_owner) if x is not None):
            _check(_typed_key(key),'strict typed ancestry key differs')
        if o.source_t is not None:_check(type(o.source_t) is tuple and all(type(x) is F for x in o.source_t),'exact source parameter map differs')
        if o.source_uv is not None:
            m=o.source_uv;_check(_typed_map(m),'exact source UV map differs')
            if o.target.kind in ('vertex','edge','coedge'):
                curve=Curve('line',solid.vertices[o.target.entity_id-1].position*2) if o.target.kind=='vertex' else solid.curves[(o.target.entity_id-1 if o.target.kind=='edge' else solid.coedges[o.target.entity_id-1].edge_slot)]
                fs=o.source_uv_owner.entity_id-1;_check(_curve_on_carrier(curve,source.patches[fs],m),'whole actual source UV identity fails')
    for row in history.records:
        images=tuple(o.target for o in history.origins if o.relation!='generated' and row.source in o.parents);generated=tuple(o.target for o in history.origins if o.relation=='generated' and row.source in o.parents);status=next((o.relation for o in history.origins if o.target in images),'deleted')
        _check(type(row) is HistoryRecord and _typed_key(row.source) and type(row.images) is tuple and type(row.generated) is tuple and all(_typed_key(key) for key in row.images+row.generated) and (row.status,row.images,row.generated)==(status,images,generated),'whole inverse history differs')
    _check(type(contacts) is tuple and len(contacts)==2,'complete two-contact output required')
    for j,(contact,(fs,cs)) in enumerate(zip(contacts,rails)):
        curve=solid.curves[13+j];uv=_line_map(_plane_uv(source.patches[fs],solid.vertices[6+j].position),_plane_uv(source.patches[fs],solid.vertices[8+j].position));normal=(*_circle(F(j),q),F(0))
        wanted=ContactRail(_key(solid,'edge',14+j),_key(source,'face',fs+1),_key(source,'coedge',cs+1),_key(solid,'face',7),j,uv,Map2('affine',(j,0,0,0,1,0)),_T,normal)
        _check(type(contact) is ContactRail and contact==wanted and all(_typed_key(getattr(contact,name)) for name in ('edge','source_face','source_coedge','blend_face')) and _typed_map(contact.source_uv) and _typed_map(contact.blend_uv) and type(contact.blend_u) is int and type(contact.source_t) is tuple and type(contact.normal) is tuple and all(type(x) is F for x in contact.source_t+contact.normal),'complete precise contact maps/normal differs')
        _check(_curve_on_carrier(curve,source.patches[fs],contact.source_uv) and _curve_on_carrier(curve,solid.patches[6],contact.blend_uv),'whole two-sided contact identity fails')
        _,o,U,V,sign=_PLANES[fs];cross=(U[1]*V[2]-U[2]*V[1],U[2]*V[0]-U[0]*V[2],U[0]*V[1]-U[1]*V[0]);axis=tuple(F(sign*x)/sum(abs(F(z)) for z in cross) for x in cross)
        _check(normal==axis,'complete band/support normal identity fails')
    expected=Measure((F(9,4)-solid.radius**2)*F(source.frame.scale)**3,solid.radius**2*F(source.frame.scale)**3/4)
    _check(type(solid.volume) is Measure and type(solid.volume.rational) is F and type(solid.volume.pi) is F and solid.volume==expected and 10-15+7==2,'exact volume/genus differs')


def fillet_edge(source,edge_key,radius):
    started=time.perf_counter_ns();previous=started;timings=[]
    def phase(name):
        nonlocal previous
        now=time.perf_counter_ns();timings.append((name,(now-previous)/1e6));previous=now
    _admit(source)
    if type(edge_key) is not EntityKey:raise TypeError('selection requires actual typed EntityKey')
    if not _typed_key(edge_key):raise ValueError('selection requires strict typed source identity')
    if (edge_key.body_id,edge_key.revision,edge_key.kind)!=(source.body_id,source.revision,'edge'):raise KeyError(edge_key)
    if not 9<=edge_key.entity_id<=12:raise UnsupportedFillet('select one complete vertical source edge9..12')
    radius=_rational(radius)
    if not F(1,16)<=radius<=F(1,2) or (radius*256).denominator!=1:raise UnsupportedFillet('local radius requires1/256grid in[1/16,1/2]')
    phase('admission_ms');solid,history,contacts=_assemble(source,edge_key,radius);phase('assembly_history_ms')
    _validate_candidate(solid,history,contacts);phase('validation_ms')
    timings.append(('total_ms',(time.perf_counter_ns()-started)/1e6))
    return _publish(FilletResult,source=source,edge=edge_key,radius=radius,solid=solid,history=history,contacts=contacts,timings=tuple(timings))
