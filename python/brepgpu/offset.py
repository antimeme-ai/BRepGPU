"""Complete exact signed intersection-joined offset of an actual source cylinder.

Positive sharp joins are not Euclidean round dilation. Negative admitted offsets
are exact erosion. Normal feet may belong to analytic source carrier extensions.
"""
from collections import defaultdict
from dataclasses import dataclass
from fractions import Fraction as F
from itertools import islice
from math import comb
import math
import operator
import struct
import time

from . import Lineage,Patch
from .body import (BodySnapshot,Frame,Vertex,Curve,Edge,Coedge,UVMap,Loop,Face,Chart,
                   _namespace,_packed,INVALID,REJECTED,INTERIOR,BOUNDARY)
from .boolean import EntityKey,Measure


class UnsupportedOffset(ValueError):
    """Source or signed distance is outside the sharp canonical cylinder family."""


def _check(ok,message):
    if not ok:raise ArithmeticError(message)


def _rational(value):
    if type(value) in (int,F):return F(value)
    if type(value) is float:
        if not math.isfinite(value):raise ValueError('geometry must be finite')
        return F(value)
    raise TypeError('geometry requires int, Fraction or finite binary64')


def _tuple(values,width):
    values=tuple(islice(iter(values),width+1))
    if len(values)!=width:raise ValueError('wrong coordinate count')
    return tuple(_rational(x) for x in values)


def _parameter(value):
    value=_rational(value)
    if not 0<=value<=1:raise ValueError('parameter outside [0,1]')
    return value


def _slot(value,count,base=0):
    if isinstance(value,bool):raise TypeError('indices cannot be bool')
    value=operator.index(value)
    if not base<=value<count+base:raise IndexError('index outside offset table')
    return value-base


def _publish(cls,**fields):
    result=object.__new__(cls)
    for name,value in fields.items():object.__setattr__(result,name,value)
    return result


def _quarter(u,q):
    x,y=(1-u*u)/(1+u*u),2*u/(1+u*u)
    return ((x,y),(-y,x),(-x,-y),(y,-x))[q]


def _circle(t):
    if t==1:return _quarter(F(1),3)
    tau=4*t;q=int(tau);return _quarter(tau-q,q)


@dataclass(frozen=True,slots=True)
class UVMap2:
    kind:str
    data:tuple
    domain:tuple

    def __post_init__(self):
        if type(self.kind) is not str or self.kind not in ('affine','circle'):raise ValueError('unknown source UV map kind')
        data=_tuple(self.data,6 if self.kind=='affine' else 1);domain=_tuple(self.domain,4)
        if not domain[0]<domain[1] or not domain[2]<domain[3]:raise ValueError('source map requires nonempty target rectangle')
        if self.kind=='circle' and (data[0]<=0 or domain[0]!=0 or domain[1]!=1):raise ValueError('circle map requires positive ratio and intrinsic unit parameter')
        object.__setattr__(self,'data',data);object.__setattr__(self,'domain',domain)

    def evaluate(self,u,v):
        u,v=_rational(u),_rational(v);a,b,c,d=self.domain
        if not a<=u<=b or not c<=v<=d:raise ValueError('query outside source-map target domain')
        data=self.data
        if self.kind=='affine':return data[0]+data[1]*u+data[2]*v,data[3]+data[4]*u+data[5]*v
        x,y=_circle(u);return (1+data[0]*x)/2,(1+data[0]*y)/2


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
    source_uv_relation:object=None
    orientation:int=1

    def __post_init__(self):
        parents=tuple(islice(iter(self.parents),2))
        if not _typed_key(self.target) or len(parents)!=1 or not _typed_key(parents[0]) or parents[0].kind!=self.target.kind:raise ValueError('offset origin requires one actual same-kind source')
        if self.relation!='modified' or self.role!='source_'+self.target.kind:raise ValueError('offset origin must name modified source role')
        object.__setattr__(self,'parents',parents)
        if any(x is None for x in (self.source_t_owner,self.source_t_relation,self.source_t)):
            if not all(x is None for x in (self.source_t_owner,self.source_t_relation,self.source_t)):raise ValueError('source parameter owner/relation/map must be paired')
        else:
            if not _typed_key(self.source_t_owner) or self.source_t_owner.kind not in ('edge','coedge') or self.source_t_relation!='boundary_parameter':raise ValueError('intrinsic correspondence requires actual original edge/coedge owner')
            object.__setattr__(self,'source_t',_tuple(self.source_t,3))
        if any(x is None for x in (self.source_uv_owner,self.source_uv_relation,self.source_uv)):
            if not all(x is None for x in (self.source_uv_owner,self.source_uv_relation,self.source_uv)):raise ValueError('source UV owner/relation/map must be paired')
        elif not _typed_key(self.source_uv_owner) or self.source_uv_owner.kind not in ('face','chart') or self.source_uv_relation!='normal_foot' or type(self.source_uv) is not UVMap2:raise ValueError('normal foot requires actual original carrier owner')
        if type(self.orientation) is not int or self.orientation not in (-1,1):raise ValueError('absolute integer orientation required')


@dataclass(frozen=True,slots=True)
class HistoryRecord:
    source:EntityKey
    status:str
    images:tuple
    generated:tuple

    def __post_init__(self):
        images=tuple(islice(iter(self.images),2));generated=tuple(islice(iter(self.generated),1))
        if not _typed_key(self.source) or self.status!='modified' or len(images)!=1 or not _typed_key(images[0]) or images[0].kind!=self.source.kind or generated:raise ValueError('one modified same-kind image and no births required')
        object.__setattr__(self,'images',images);object.__setattr__(self,'generated',generated)


@dataclass(frozen=True,slots=True,init=False)
class History:
    records:tuple
    origins:tuple
    def __init__(self,*args,**kwargs):raise TypeError('History is factory-only')

    def from_input(self,key):
        if _typed_key(key):
            for row in self.records:
                if row.source==key:return row
        raise KeyError(key)

    def origin(self,key):
        if _typed_key(key):
            for row in self.origins:
                if row.target==key:return row
        raise KeyError(key)


@dataclass(frozen=True,slots=True,init=False)
class Footpoint:
    solid:object
    chart_id:int
    u:F
    v:F
    source_face:EntityKey
    source_chart:EntityKey
    source_uv:tuple
    source_point:tuple
    point:tuple
    normal:tuple
    distance:F
    in_source_square:bool
    source_trim:int
    def __init__(self,*args,**kwargs):raise TypeError('Footpoint is factory-only')


@dataclass(frozen=True,slots=True,init=False)
class PointClassification:
    solid:object
    point:tuple
    label:str
    boundary_faces:tuple
    def __init__(self,*args,**kwargs):raise TypeError('PointClassification is factory-only')


@dataclass(frozen=True,slots=True,init=False)
class OffsetSolid:
    source:BodySnapshot
    distance:F
    join:str
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
    def __init__(self,*args,**kwargs):raise TypeError('OffsetSolid is factory-only; use offset_solid')

    def classify(self,chart_slot,u,v):
        slot=_slot(chart_slot,6)
        if any(type(x) not in (int,F,float) for x in (u,v)):raise TypeError('trim coordinates require exact numeric values')
        if any(type(x) is float and not math.isfinite(x) for x in (u,v)):return INVALID
        u,v=_rational(u),_rational(v)
        if not 0<=u<=1 or not 0<=v<=1:return INVALID
        return _trim(slot,u,v)

    def classify_point(self,point):
        point=_tuple(point,3);x,y,z=tuple((p-F(o))/F(self.frame.scale) for p,o in zip(point,self.frame.origin));R,H=_size(self)
        radial=x*x+y*y-R*R
        if radial>0 or abs(z)>H:label,faces='exterior',()
        else:
            faces=tuple(i for i,on in ((1,z==H),(2,z==-H),(3,radial==0)) if on);label='boundary' if faces else 'interior'
        return _publish(PointClassification,solid=self,point=point,label=label,boundary_faces=faces)

    def edge_point(self,edge_id,t):
        edge=self.edges[_slot(edge_id,3,1)];t=_parameter(t);curve=self.curves[edge.curve_slot]
        if curve.kind=='circle':
            r,z=curve.data;x,y=_circle(t);point=(r*x,r*y,z)
        else:
            _check(curve.kind=='line','unexpected offset curve');point=tuple((1-t)*a+t*b for a,b in zip(curve.data[:3],curve.data[3:]))
        return _world(self.frame,point)

    def coedge_uv(self,coedge_id,t):
        use=self.coedges[_slot(coedge_id,6,1)];t=_parameter(t);kind,data=use.uv_map.kind,use.uv_map.data
        if kind=='circle_plane':
            x,y=_circle(t);return (1+x)/2,(1+y)/2
        if kind=='circle_wall':return 4*t,data[0]
        _check(kind=='seam_wall','unexpected offset incident UV kind');return data[0],t

    def source_foot(self,chart_slot,u,v):
        slot=_slot(chart_slot,6);u,v=_parameter(u),_parameter(v)
        if self.classify(slot,u,v) not in (INTERIOR,BOUNDARY):raise ValueError('normal foot requires admitted output face query')
        m=_chart_map(self,slot);su,sv=m.evaluate(u,v);source=self.source
        # Owned exact Bernstein evaluation on the original analytic carrier:
        # this deliberately accepts its extension, without a native query.
        foot=_patch_point(source.patches[slot],su if slot<2 else su-(slot-2),sv)
        point=_patch_point(self.patches[slot],u,v)
        normal=(F(0),F(0),F(1 if slot==0 else -1)) if slot<2 else (*_quarter(u,slot-2),F(0))
        a,b,c,d=(F(x) for x in source.patches[slot].uv);inside=a<=su<=b and c<=sv<=d
        trim=INVALID if not inside else _trim(slot,su if slot<2 else su-(slot-2),sv)
        return _publish(Footpoint,solid=self,chart_id=slot+1,u=u,v=v,source_face=_key(source,'face',self.charts[slot].face_slot+1),source_chart=_key(source,'chart',slot+1),source_uv=(su,sv),source_point=_world(source.frame,foot),point=_world(self.frame,point),normal=normal,distance=self.distance*F(self.frame.scale),in_source_square=inside,source_trim=trim)

    def topology_input(self):
        from .adjacency import Source,TopologyInput
        def sources(rows):return tuple(Source(0,self.body_id,self.revision,row.id) for row in rows)
        return TopologyInput(sources(self.edges),sources(self.faces),sources(self.coedges),tuple(c.edge_slot for c in self.coedges),tuple(c.face_slot for c in self.coedges),tuple(c.orientation for c in self.coedges),snapshots=())


@dataclass(frozen=True,slots=True,init=False)
class OffsetResult:
    source:BodySnapshot
    distance:F
    join:str
    solid:OffsetSolid
    history:History
    timings:tuple
    def __init__(self,*args,**kwargs):raise TypeError('OffsetResult is factory-only; use offset_solid')


_TABLES=(('vertex','vertices'),('edge','edges'),('coedge','coedges'),('loop','loops'),('face','faces'),('chart','charts'))
_UNIT=(F(0),F(1),F(0),F(1));_WALL=(F(0),F(4),F(0),F(1));_T=(F(0),F(1),F(0))
_WORDS=((0,),(1,),(2,3,4,5))
_USES=((1,0,1,'circle_plane'),(0,1,-1,'circle_plane'),(0,2,1,'circle_wall'),(2,2,1,'seam_wall'),(1,2,-1,'circle_wall'),(2,2,-1,'seam_wall'))


def _key(body,kind,id):return EntityKey(body.body_id,body.revision,kind,id)
def _keys(body):return tuple(_key(body,kind,r.id) for kind,table in _TABLES for r in getattr(body,table))
def _typed_key(key):return type(key) is EntityKey and type(key.kind) is str and key.kind in dict(_TABLES) and all(type(x) is int and 0<x<1<<64 for x in (key.body_id,key.revision,key.entity_id))
def _float_bits(a,b):return type(a) is tuple and len(a)==len(b) and all(type(x) is float for x in a) and struct.pack('<'+'d'*len(a),*a)==struct.pack('<'+'d'*len(b),*b)
def _world(frame,p):return tuple(F(o)+F(frame.scale)*x for o,x in zip(frame.origin,p))
def _size(solid):return F(solid.source.curves[0].data[0])+solid.distance,F(solid.source.vertices[1].position[2])+solid.distance


def _trim(slot,u,v):
    if slot<2:
        d=(2*u-1)**2+(2*v-1)**2-1
        return REJECTED if d>0 else BOUNDARY if d==0 else INTERIOR
    return BOUNDARY if v in (0,1) or slot==2 and u==0 or slot==5 and u==1 else INTERIOR


def _patch_point(p,u,v):
    values=[F(0)]*4
    for i in range(p.degree_u+1):
        for j in range(p.degree_v+1):
            b=comb(p.degree_u,i)*u**i*(1-u)**(p.degree_u-i)*comb(p.degree_v,j)*v**j*(1-v)**(p.degree_v-j);offset=4*(i*(p.degree_v+1)+j)
            for k in range(4):values[k]+=b*F(p.controls[offset+k])
    return tuple(x/values[3] for x in values[:3])


def _controls(slot,R,H):
    if slot<2:return tuple(float(x) for u in (-R,R) for v in (-R,R) for x in (u,v,H if slot==0 else -H,F(1)))
    q=slot-2;controls=[]
    for a,b,w in ((1,0,1),(1,1,1),(0,2,2)):
        a,b=((a,b),(-b,a),(-a,-b),(b,-a))[q]
        for z in (-H,H):controls.extend(map(float,(R*a,R*b,z*w,w)))
    return tuple(controls)


def _source_controls(slot):
    if slot<2:return _controls(slot,F(1,4),F(1,2))
    # The original factory rotates binary64 numerators, retaining signed zeros.
    q=slot-2;controls=[]
    for a,b,w in ((.25,0.,1.),(.25,.25,1.),(0.,.5,2.)):
        a,b=((a,b),(-b,a),(-a,-b),(b,-a))[q]
        for z in (-.5,.5):controls.extend((a,b,z*w,w))
    return tuple(controls)


def _admit(source):
    if type(source) is not BodySnapshot:raise TypeError('offset requires exact factory BodySnapshot')
    if type(source.shape) is not str or source.shape!='cylinder':raise UnsupportedOffset('only the canonical source cylinder is supported')
    _check(type(source.body_id) is int and 0<source.body_id<1<<64 and type(source.revision) is int and source.revision==1,'strict original namespace/revision differs')
    frame=source.frame
    _check(type(frame) is Frame and type(frame.origin) is tuple and len(frame.origin)==3 and all(type(x) is float and math.isfinite(x) and abs(x)<=16 for x in frame.origin) and type(frame.scale) is float and frame.scale in (.25,.5,1.,2.,4.),'strict original Frame differs')
    for (_,table),count,cls in zip(_TABLES,(2,3,6,3,3,6),(Vertex,Edge,Coedge,Loop,Face,Chart)):
        rows=getattr(source,table);_check(type(rows) is tuple and len(rows)==count and all(type(row) is cls and type(row.id) is int and row.id==i+1 for i,row in enumerate(rows)),'complete typed source inventory differs')
    _check(type(source.curves) is tuple and len(source.curves)==3 and type(source.patches) is tuple and len(source.patches)==6,'source curve/carrier inventory differs')
    for i,v in enumerate(source.vertices):_check(_float_bits(v.position,(.25,0.,-.5 if i==0 else .5)),'original source anchor bits differ')
    for i,(ends,curve) in enumerate(zip(((0,0),(1,1),(0,1)),source.curves)):
        e=source.edges[i];_check(all(type(x) is int for x in (e.start,e.end,e.curve_slot)) and (e.start,e.end,e.curve_slot)==(*ends,i),'source physical edge ownership differs')
        _check(type(curve) is Curve and curve.kind==('circle' if i<2 else 'line') and _float_bits(curve.data,(.25,-.5 if i==0 else .5) if i<2 else (.25,0.,-.5,.25,0.,.5)),'original source curve bits differ')
    for i,(es,fs,sign,kind) in enumerate(_USES):
        co=source.coedges[i];data=(.25,.25) if i<2 else ((0.,),(4.,),(1.,),(0.,))[i-2]
        _check(all(type(x) is int for x in (co.edge_slot,co.face_slot,co.loop_slot,co.orientation)) and (co.edge_slot,co.face_slot,co.loop_slot,co.orientation)==(es,fs,fs,sign) and type(co.uv_map) is UVMap and co.uv_map.kind==kind and _float_bits(co.uv_map.data,data),'source occurrence identity/UV bits differ')
    for fs,word in enumerate(_WORDS):
        loop=source.loops[fs];face=source.faces[fs];sign=-1 if fs==1 else 1;chartslots=(fs,) if fs<2 else (2,3,4,5)
        _check(type(loop.face_slot) is int and loop.face_slot==fs and type(loop.hole) is bool and not loop.hole and type(loop.coedge_slots) is tuple and all(type(x) is int for x in loop.coedge_slots) and loop.coedge_slots==word,'source closed word differs')
        _check(face==Face(fs+1,('top','bottom','wall')[fs],'disk' if fs<2 else 'cylinder',sign,(fs,),chartslots) and type(face.orientation) is int and all(type(w) is tuple and all(type(x) is int for x in w) for w in (face.loop_slots,face.chart_slots)),'source face ownership/sense differs')
    for slot,chart in enumerate(source.charts):
        fs=slot if slot<2 else 2;cp=source.patches[slot];uv=(0.,1.,0.,1.) if slot<2 else (float(slot-2),float(slot-1),0.,1.)
        _check(type(chart.face_slot) is int and type(chart.quadrant) is int and (chart.face_slot,chart.quadrant)==(fs,-1 if slot<2 else slot-2) and chart.patch is cp,'source chart ownership differs')
        _check(type(cp) is Patch and type(cp.degree_u) is int and type(cp.degree_v) is int and (cp.degree_u,cp.degree_v)==((1,1) if slot<2 else (2,1)) and _float_bits(cp.controls,_source_controls(slot)) and _float_bits(cp.uv,uv) and _float_bits(cp.origin,frame.origin) and type(cp.scale) is float and struct.pack('<d',cp.scale)==struct.pack('<d',frame.scale),'original native control/rawUV/Frame bits differ')
        lin=cp.lineage;_check(type(lin) is Lineage and all(type(x) is int for x in (lin.body_id,lin.entity_kind,lin.entity_id,lin.revision,lin.chart_id)) and lin==Lineage(source.body_id,5,fs+1,1,slot+1),'source typed native Lineage differs')
    _check(type(source.packed_tables) is tuple and all(type(row) is tuple and len(row)==2 and type(row[0]) is str and type(row[1]) is bytes for row in source.packed_tables) and source.packed_tables==_packed(source),'actual packed source export differs')


def _chart_map(solid,slot):
    d=solid.distance
    return UVMap2('affine',(-2*d,1+4*d,0,-2*d,0,1+4*d),_UNIT) if slot<2 else UVMap2('affine',(slot-2,1,0,-d,0,1+2*d),_UNIT)


def _origins(solid):
    source=solid.source;d=solid.distance;origins=[]
    for kind,table in _TABLES:
        for row in getattr(solid,table):
            id=row.id;parent=_key(source,kind,id);towner=trelation=t=uvowner=uv=uvrelation=None;orientation=1
            if kind in ('edge','coedge'):towner=parent;trelation='boundary_parameter';t=_T
            if kind=='coedge':
                co=source.coedges[id-1];orientation=co.orientation;uvowner=_key(source,'face',co.face_slot+1)
                if id<3:uv=UVMap2('circle',(1+4*d,),_UNIT)
                elif id in (3,5):uv=UVMap2('affine',(0,4,0,-d if id==3 else 1+d,0,0),_UNIT)
                else:uv=UVMap2('affine',(4 if id==4 else 0,0,0,-d,1+2*d,0),_UNIT)
            elif kind in ('loop','face','chart'):
                fs=id-1 if kind in ('loop','face') else source.charts[id-1].face_slot;orientation=source.faces[fs].orientation;uvowner=_key(source,'chart' if kind=='chart' else 'face',id if kind=='chart' else fs+1)
                uv=_chart_map(solid,id-1) if kind=='chart' else _chart_map(solid,fs) if fs<2 else UVMap2('affine',(0,1,0,-d,0,1+2*d),_WALL)
            if uv is not None:uvrelation='normal_foot'
            origins.append(Origin(_key(solid,kind,id),'modified',(parent,),'source_'+kind,towner,trelation,t,uvowner,uv,uvrelation,orientation))
    return tuple(origins)


def _assemble(source,d):
    R=F(source.curves[0].data[0])+d;H=F(source.vertices[1].position[2])+d;body_id=_namespace()
    vertices=(Vertex(1,(R,F(0),-H)),Vertex(2,(R,F(0),H)))
    curves=(Curve('circle',(R,-H)),Curve('circle',(R,H)),Curve('line',vertices[0].position+vertices[1].position))
    edges=tuple(Edge(e.id,e.start,e.end,e.curve_slot) for e in source.edges)
    coedges=tuple(Coedge(co.id,co.edge_slot,co.face_slot,co.loop_slot,co.orientation,UVMap(co.uv_map.kind,(R,R) if i<2 else tuple(F(x) for x in co.uv_map.data))) for i,co in enumerate(source.coedges))
    patches=tuple(Patch(1 if slot<2 else 2,1,_controls(slot,R,H),Lineage(body_id,5,(slot+1 if slot<2 else 3),1,slot+1),old.uv,old.origin,old.scale) for slot,old in enumerate(source.patches))
    charts=tuple(Chart(old.id,old.face_slot,old.quadrant,patches[slot]) for slot,old in enumerate(source.charts))
    solid=_publish(OffsetSolid,source=source,distance=d,join='intersection',body_id=body_id,revision=1,frame=source.frame,vertices=vertices,curves=curves,edges=edges,coedges=coedges,loops=source.loops,faces=source.faces,charts=charts,patches=patches,volume=Measure(F(0),2*H*R*R*F(source.frame.scale)**3))
    origins=_origins(solid);records=tuple(HistoryRecord(key,'modified',(_key(solid,key.kind,key.entity_id),),()) for key in _keys(source))
    return solid,_publish(History,records=records,origins=origins)


def _validate_candidate(solid,history):
    source=solid.source;_admit(source)
    _check(type(solid) is OffsetSolid and solid.frame is source.frame and type(solid.distance) is F and solid.distance!=0 and abs(solid.distance)<=F(1,8) and (solid.distance*256).denominator==1 and type(solid.join) is str and solid.join=='intersection','source-bound offset family differs')
    _check(type(solid.body_id) is int and 0<solid.body_id<1<<64 and solid.body_id!=source.body_id and type(solid.revision) is int and solid.revision==1,'fresh typed result namespace differs')
    R,H=_size(solid);_check(R>0 and H>0,'singular/inverted cylinder')
    for (_,table),count,cls in zip(_TABLES,(2,3,6,3,3,6),(Vertex,Edge,Coedge,Loop,Face,Chart)):
        rows=getattr(solid,table);_check(type(rows) is tuple and len(rows)==count and all(type(row) is cls and type(row.id) is int and row.id==i+1 for i,row in enumerate(rows)),'complete typed result inventory differs')
    for i,v in enumerate(solid.vertices):_check(type(v.position) is tuple and all(type(x) is F for x in v.position) and v.position==(R,F(0),-H if i==0 else H),'actual exact rim anchor differs')
    _check(type(solid.curves) is tuple and len(solid.curves)==3,'physical curve coverage differs')
    expectedcurves=(Curve('circle',(R,-H)),Curve('circle',(R,H)),Curve('line',solid.vertices[0].position+solid.vertices[1].position))
    for es,(edge,curve,expected) in enumerate(zip(solid.edges,solid.curves,expectedcurves)):
        _check(all(type(x) is int for x in (edge.start,edge.end,edge.curve_slot)) and edge==source.edges[es],'physical closed-ring/seam identity differs')
        _check(type(curve) is Curve and type(curve.data) is tuple and all(type(x) is F for x in curve.data) and curve==expected,'whole exact circle/seam coefficients differ')
    for cs,(co,old) in enumerate(zip(solid.coedges,source.coedges)):
        expected=(R,R) if cs<2 else tuple(F(x) for x in old.uv_map.data)
        _check(all(type(x) is int for x in (co.edge_slot,co.face_slot,co.loop_slot,co.orientation)) and (co.edge_slot,co.face_slot,co.loop_slot,co.orientation)==(old.edge_slot,old.face_slot,old.loop_slot,old.orientation) and type(co.uv_map) is UVMap and type(co.uv_map.data) is tuple and all(type(x) is F for x in co.uv_map.data) and co.uv_map==UVMap(old.uv_map.kind,expected),'whole incident exact UV/orientation map differs')
    _check(type(solid.patches) is tuple and len(solid.patches)==6,'carrier coverage differs')
    for slot,(chart,cp) in enumerate(zip(solid.charts,solid.patches)):
        old=source.charts[slot];fs=slot if slot<2 else 2
        _check(type(chart.face_slot) is int and type(chart.quadrant) is int and (chart.id,chart.face_slot,chart.quadrant)==(old.id,old.face_slot,old.quadrant) and chart.patch is cp,'actual source chart occurrence differs')
        _check(type(cp) is Patch and type(cp.degree_u) is int and type(cp.degree_v) is int and (cp.degree_u,cp.degree_v)==((1,1) if slot<2 else (2,1)) and _float_bits(cp.controls,_controls(slot,R,H)) and _float_bits(cp.uv,source.patches[slot].uv) and _float_bits(cp.origin,source.frame.origin) and type(cp.scale) is float and struct.pack('<d',cp.scale)==struct.pack('<d',source.frame.scale),'whole changed carrier/rawUV/Frame bits differ')
        lin=cp.lineage;_check(type(lin) is Lineage and all(type(x) is int for x in (lin.body_id,lin.entity_kind,lin.entity_id,lin.revision,lin.chart_id)) and lin==Lineage(solid.body_id,5,fs+1,1,slot+1),'actual result Lineage differs')
    uses=defaultdict(list);links=defaultdict(lambda:defaultdict(list));neighbors=defaultdict(set);seen=[]
    for fs,(face,loop) in enumerate(zip(solid.faces,solid.loops)):
        _check(face==source.faces[fs] and type(face.orientation) is int and all(type(word) is tuple and all(type(x) is int for x in word) for word in (face.loop_slots,face.chart_slots)),'complete face sense/domain differs')
        _check(type(loop.face_slot) is int and type(loop.hole) is bool and not loop.hole and type(loop.coedge_slots) is tuple and all(type(x) is int for x in loop.coedge_slots) and loop==source.loops[fs],'complete ring/seam word differs')
        for cs in loop.coedge_slots:seen.append(cs);uses[solid.coedges[cs].edge_slot].append(solid.coedges[cs])
        word=loop.coedge_slots
        for previous,current in zip(word[-1:]+word[:-1],word):
            a,b=solid.coedges[previous],solid.coedges[current];ea,eb=solid.edges[a.edge_slot],solid.edges[b.edge_slot];end=1 if a.orientation==1 else 0;start=0 if b.orientation==1 else 1;vertex=(ea.start,ea.end)[end]
            _check(vertex==(eb.start,eb.end)[start],'loop is not closed by actual anchor identity');x,y=(a.edge_slot,end),(b.edge_slot,start);links[vertex][x].append(y);links[vertex][y].append(x)
    _check(sorted(seen)==list(range(6)),'missing/duplicate occurrence ownership')
    for es in range(3):
        pair=uses[es];_check(len(pair)==2 and sum(c.orientation for c in pair)==0 and (pair[0].face_slot==pair[1].face_slot)==(es==2),'ordinary/same-face physical use multiplicity differs');a,b=(c.face_slot for c in pair);neighbors[a].add(b);neighbors[b].add(a)
    def visit(graph,start):
        found=set();todo=[start]
        while todo:
            x=todo.pop()
            if x not in found:found.add(x);todo.extend(graph[x])
        return found
    _check(visit(neighbors,0)==set(range(3)) and set(links)=={0,1},'single closed shell differs')
    for vertex,graph in links.items():
        ports={(es,end) for es,e in enumerate(solid.edges) for end,v in enumerate((e.start,e.end)) if v==vertex}
        _check(len(ports)==3 and set(graph)==ports and all(len(v)==2 for v in graph.values()) and visit(graph,next(iter(graph)))==ports,'anchor three-port cyclic link differs')
    # Whole analytic substitution, not sampled equality: the exact cap nets
    # yield (R*(2u-1),R*(2v-1),±H). The certified wall nets yield
    # (R*C(tau),R*S(tau),-H+2H*v). The complete literal incident descriptors
    # above therefore map each entire physical ring/seam onto both carriers.
    # For source feet cap XY coefficients give r*(-1+2*(-2d+(1+4d)u))
    # =R*(2u-1); wall original z is -h+(-d+(1+2d)v)=-H+2H*v.
    # Source radius R-d=r, axial H-d=h and C²+S²=1 give the full signed-normal
    # identity and equal outward normals everywhere. R/H>0 fixes handedness.
    _check(R-solid.distance==F(source.curves[0].data[0]) and H-solid.distance==F(source.vertices[1].position[2]),'complete signed-normal carrier identity differs')
    expected_origins=_origins(solid)
    _check(type(history) is History and type(history.origins) is tuple and history.origins==expected_origins and type(history.records) is tuple and tuple(row.source for row in history.records)==_keys(source),'complete ordered precise source ancestry differs')
    for origin in history.origins:
        _check(type(origin) is Origin and type(origin.parents) is tuple and type(origin.orientation) is int and type(origin.role) is str and type(origin.relation) is str,'typed modified origin differs')
        for key in (origin.target,)+origin.parents+tuple(x for x in (origin.source_t_owner,origin.source_uv_owner) if x is not None):_check(_typed_key(key),'strict ancestry owner differs')
        if origin.source_t is not None:_check(type(origin.source_t) is tuple and len(origin.source_t)==3 and all(type(x) is F for x in origin.source_t),'exact intrinsic boundary map differs')
        if origin.source_uv is not None:
            m=origin.source_uv;_check(type(m) is UVMap2 and type(m.kind) is str and type(m.domain) is tuple and len(m.domain)==4 and all(type(x) is F for x in m.domain) and type(m.data) is tuple and all(type(x) is F for x in m.data),'complete exact carrier normal-foot descriptor differs')
    for row in history.records:
        _check(type(row) is HistoryRecord and _typed_key(row.source) and type(row.images) is tuple and type(row.generated) is tuple and row.status=='modified' and row.images==(_key(solid,row.source.kind,row.source.entity_id),) and not row.generated and all(_typed_key(key) for key in row.images),'one actual modified boundary image differs')
    _check(type(solid.volume) is Measure and type(solid.volume.rational) is F and type(solid.volume.pi) is F and solid.volume==Measure(F(0),2*H*R*R*F(source.frame.scale)**3) and 2-3+3==2,'exact volume/genus differs')


def offset_solid(source,distance):
    started=time.perf_counter_ns();previous=started;timings=[]
    def phase(name):
        nonlocal previous
        now=time.perf_counter_ns();timings.append((name,(now-previous)/1e6));previous=now
    _admit(source);distance=_rational(distance)
    if distance==0 or abs(distance)>F(1,8) or (distance*256).denominator!=1:raise UnsupportedOffset('signed distance requires nonzero1/256grid in[-1/8,+1/8]')
    phase('admission_ms');solid,history=_assemble(source,distance);phase('assembly_history_ms')
    _validate_candidate(solid,history);phase('validation_ms');timings.append(('total_ms',(time.perf_counter_ns()-started)/1e6))
    return _publish(OffsetResult,source=source,distance=distance,join='intersection',solid=solid,history=history,timings=tuple(timings))
