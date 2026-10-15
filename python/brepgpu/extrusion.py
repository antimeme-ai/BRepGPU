"""Owned exact convex-profile extrusion; independent of canonical body recipes."""
from collections import defaultdict
from dataclasses import dataclass
from fractions import Fraction as F
from itertools import islice
import math
import operator
import time

from . import Lineage, Patch
from .body import (Frame, Vertex, Curve, Edge, Coedge, Loop, Face, Chart, UVMap,
                   _namespace, INVALID, REJECTED, BOUNDARY, INTERIOR)
from .boolean import EntityKey


class UnsupportedExtrusion(ValueError):
    """Input lies outside the complete convex lattice-profile family."""


def _check(condition,message):
    if not condition:raise ArithmeticError(message)


def _rational(value):
    if type(value) in (int,F):return F(value)
    if type(value) is float:
        if not math.isfinite(value):raise UnsupportedExtrusion('geometry requires finite coordinates')
        return F(value)
    raise TypeError('geometry requires int, Fraction or finite binary64')


def _slot(value,count,base=0):
    if isinstance(value,bool):raise TypeError('indices cannot be bool')
    index=operator.index(value)
    if not base<=index<count+base:raise IndexError('index outside extrusion table')
    return index-base


def _publish(cls,**values):
    result=object.__new__(cls)
    for name,value in values.items():object.__setattr__(result,name,value)
    return result


def _triple(values):
    values=tuple(islice(iter(values),4))
    if len(values)!=3:raise ValueError('expected exactly three coordinates')
    return tuple(_rational(x) for x in values)


def _parameter(t):
    t=_rational(t)
    if not 0<=t<=1:raise ValueError('parameter outside [0,1]')
    return t


def _uv(u,v):
    for x in (u,v):
        if type(x) not in (int,float,F):raise TypeError('trim coordinates require exact numeric values')
    if any(type(x) is float and not math.isfinite(x) for x in (u,v)):return None
    u,v=_rational(u),_rational(v)
    return (u,v) if 0<=u<=1 and 0<=v<=1 else None


def _supports(profile,x,y):
    points=tuple(v.position for v in profile.vertices)
    return tuple((b[0]-a[0])*(y-a[1])-(b[1]-a[1])*(x-a[0])
                 for a,b in zip(points,points[1:]+points[:1]))


def _profile_label(profile,u,v):
    uv=_uv(u,v)
    if uv is None:return INVALID
    u,v=uv;values=_supports(profile,2*u-1,2*v-1)
    if any(x<0 for x in values):return REJECTED
    return BOUNDARY if any(x==0 for x in values) else INTERIOR


@dataclass(frozen=True,slots=True,init=False)
class PlanarProfile:
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
    area:F

    def __init__(self,*args,**kwargs):raise TypeError('PlanarProfile is factory-only; use make_profile')

    def classify(self,u,v):return _profile_label(self,u,v)


@dataclass(frozen=True,slots=True)
class Origin:
    target:EntityKey
    relation:str
    parents:tuple
    role:str
    local_translation:object=None
    source_t:object=None
    source_uv:object=None
    axial:object=None
    orientation:int=1

    def __post_init__(self):
        if type(self.target) is not EntityKey:raise TypeError('target requires EntityKey')
        if self.relation not in ('modified','generated'):raise ValueError('invalid extrusion relation')
        parents=tuple(self.parents)
        if not parents or any(type(x) is not EntityKey for x in parents) or len(set(parents))!=len(parents):
            raise ValueError('origins require distinct typed source parents')
        object.__setattr__(self,'parents',parents)
        if self.role not in ('lower_cap','upper_cap','side','swept_vertex','side_lower','side_upper','side_start','side_end'):
            raise ValueError('unknown feature role')
        for name in ('local_translation','source_t','axial'):
            value=getattr(self,name)
            if value is not None:object.__setattr__(self,name,_triple(value))
        if self.source_uv is not None:
            rows=tuple(islice(iter(self.source_uv),3))
            if len(rows)!=2:raise ValueError('source UV requires two affine rows')
            object.__setattr__(self,'source_uv',tuple(_triple(x) for x in rows))
        if type(self.orientation) is not int or self.orientation not in (-1,1):raise ValueError('orientation requires integer sign')


@dataclass(frozen=True,slots=True)
class HistoryRecord:
    source:EntityKey
    status:str
    images:tuple
    generated:tuple

    def __post_init__(self):
        if type(self.source) is not EntityKey or self.status!='modified':raise ValueError('source must be modified')
        for name in ('images','generated'):
            values=tuple(getattr(self,name))
            if any(type(x) is not EntityKey for x in values) or len(set(values))!=len(values):raise ValueError('invalid history targets')
            object.__setattr__(self,name,values)
        if len(self.images)!=2 or any(x.kind!=self.source.kind for x in self.images):raise ValueError('exactly two same-kind cap images required')


@dataclass(frozen=True,slots=True,init=False)
class History:
    records:tuple
    origins:tuple

    def __init__(self,*args,**kwargs):raise TypeError('History is factory-only')

    def from_input(self,key):
        if type(key) is EntityKey:
            for r in self.records:
                if r.source==key:return r
        raise KeyError(key)

    def origin(self,key):
        if type(key) is EntityKey:
            for o in self.origins:
                if o.target==key:return o
        raise KeyError(key)


@dataclass(frozen=True,slots=True,init=False)
class PointClassification:
    solid:object
    point:tuple
    label:str
    boundary_faces:tuple

    def __init__(self,*args,**kwargs):raise TypeError('PointClassification is factory-only')


@dataclass(frozen=True,slots=True,init=False)
class ExtrudedSolid:
    profile:PlanarProfile
    body_id:int
    revision:int
    frame:Frame
    height:F
    vertices:tuple
    curves:tuple
    edges:tuple
    coedges:tuple
    loops:tuple
    faces:tuple
    charts:tuple
    patches:tuple
    volume:F

    def __init__(self,*args,**kwargs):raise TypeError('ExtrudedSolid is factory-only; use extrude')

    def classify(self,slot,u,v):
        slot=_slot(slot,len(self.charts))
        if slot<2:return _profile_label(self.profile,u,v)
        uv=_uv(u,v)
        if uv is None:return INVALID
        return BOUNDARY if any(x in (0,1) for x in uv) else INTERIOR

    def classify_point(self,point):
        point=_triple(point)
        x,y,z=tuple((p-F(o))/F(self.frame.scale) for p,o in zip(point,self.frame.origin))
        values=(z+self.height/2,self.height/2-z)+_supports(self.profile,x,y)
        if any(v<0 for v in values):label,faces='exterior',()
        else:
            faces=tuple(i+1 for i,v in enumerate(values) if v==0)
            label='boundary' if faces else 'interior'
        return _publish(PointClassification,solid=self,point=point,label=label,boundary_faces=faces)

    def edge_point(self,edge_id,t):
        e=self.edges[_slot(edge_id,len(self.edges),1)];t=_parameter(t)
        data=self.curves[e.curve_slot].data
        local=tuple((1-t)*data[i]+t*data[i+3] for i in range(3))
        return tuple(F(o)+F(self.frame.scale)*x for o,x in zip(self.frame.origin,local))

    def coedge_uv(self,coedge_id,t):
        c=self.coedges[_slot(coedge_id,len(self.coedges),1)];t=_parameter(t);d=c.uv_map.data
        return tuple((1-t)*d[i]+t*d[i+2] for i in range(2))

    def topology_input(self):
        from .adjacency import Source,TopologyInput
        def sources(records):return tuple(Source(0,self.body_id,self.revision,x.id) for x in records)
        return TopologyInput(sources(self.edges),sources(self.faces),sources(self.coedges),
                             tuple(c.edge_slot for c in self.coedges),tuple(c.face_slot for c in self.coedges),
                             tuple(c.orientation for c in self.coedges),snapshots=())


@dataclass(frozen=True,slots=True,init=False)
class ExtrusionResult:
    profile:PlanarProfile
    height:F
    solid:ExtrudedSolid
    history:History
    timings:tuple

    def __init__(self,*args,**kwargs):raise TypeError('ExtrusionResult is factory-only; use extrude')


_TABLES=(('vertex','vertices'),('edge','edges'),('coedge','coedges'),('loop','loops'),('face','faces'),('chart','charts'))
_T=(F(0),F(1),F(0))
_UV=(_T,(F(0),F(0),F(1)))


def _key(body,kind,identifier):return EntityKey(body.body_id,body.revision,kind,identifier)


def _keys(body):return tuple(_key(body,k,r.id) for k,table in _TABLES for r in getattr(body,table))


def _patch(body_id,face_id,frame,points):
    return Patch(1,1,tuple(tuple(float(x) for x in (*p,F(1))) for p in points),
                 Lineage(body_id,5,face_id,1,face_id),origin=frame.origin,scale=frame.scale)


def _plane(body_id,face_id,frame,z):
    return _patch(body_id,face_id,frame,tuple((F(x),F(y),z) for x in (-1,1) for y in (-1,1)))


def _admit_points(points):
    raw=tuple(islice(iter(points),33))
    if not 3<=len(raw)<=32:raise UnsupportedExtrusion('profile requires 3..32 vertices')
    vertices=[]
    for p in raw:
        values=tuple(islice(iter(p),3))
        if len(values)!=2:raise UnsupportedExtrusion('profile vertices require two coordinates')
        xy=tuple(_rational(x) for x in values)
        if any(abs(x)>F(3,4) or (x*256).denominator!=1 for x in xy):
            raise UnsupportedExtrusion('profile coordinates require bounded 1/256 grid')
        vertices.append((*xy,F(0)))
    n=len(vertices)
    for i,a in enumerate(vertices):
        b=vertices[(i+1)%n]
        for j,p in enumerate(vertices):
            if j not in (i,(i+1)%n) and (b[0]-a[0])*(p[1]-a[1])-(b[1]-a[1])*(p[0]-a[0])<=0:
                raise UnsupportedExtrusion('profile requires strictly convex CCW simple boundary')
    return tuple(vertices)


def make_profile(points,frame=Frame()):
    if type(frame) is not Frame:raise TypeError('profile requires exact owned Frame')
    points=_admit_points(points);n=len(points);body_id=_namespace()
    vertices=tuple(Vertex(i+1,p) for i,p in enumerate(points))
    curves=tuple(Curve('line',a+b) for a,b in zip(points,points[1:]+points[:1]))
    edges=tuple(Edge(i+1,i,(i+1)%n,i) for i in range(n))
    coedges=tuple(Coedge(i+1,i,0,0,1,UVMap('line',tuple((x+1)/2 for x in a[:2]+b[:2])))
                  for i,(a,b) in enumerate(zip(points,points[1:]+points[:1])))
    cp=_plane(body_id,1,frame,F(0))
    area=sum((a[0]*b[1]-b[0]*a[1] for a,b in zip(points,points[1:]+points[:1])),F(0))/2
    result=_publish(PlanarProfile,body_id=body_id,revision=1,frame=frame,vertices=vertices,curves=curves,
                    edges=edges,coedges=coedges,loops=(Loop(1,0,False,tuple(range(n))),),
                    faces=(Face(1,'profile','convex_polygon',1,(0,),(0,)),),charts=(Chart(1,0,-1,cp),),patches=(cp,),area=area)
    _validate_profile(result)
    return result


def _validate_profile(profile):
    points=tuple(v.position for v in profile.vertices);n=len(points)
    _check(type(profile.frame) is Frame and type(profile.body_id) is int and 0<profile.body_id<1<<64
           and type(profile.revision) is int and profile.revision==1,'source namespace/frame differs')
    _check(_admit_points(tuple(p[:2] for p in points))==points,'source geometry outside admitted family')
    _check(all(type(x) is F for p in points for x in p),'source coordinates must be Fractions')
    for _,table in _TABLES:
        _check(all(type(r.id) is int and r.id==i+1 for i,r in enumerate(getattr(profile,table))),'source IDs differ')
    _check(tuple(len(getattr(profile,t)) for _,t in _TABLES)==(n,n,n,1,1,1),'source coverage differs')
    _check(profile.patches==(_plane(profile.body_id,1,profile.frame,F(0)),),'source carrier differs')
    _check(profile.charts==(Chart(1,0,-1,profile.patches[0]),) and profile.faces==(Face(1,'profile','convex_polygon',1,(0,),(0,)),),
           'source face/chart ownership differs')
    _check(profile.loops==(Loop(1,0,False,tuple(range(n))),),'source loop word differs')
    for i,(a,b) in enumerate(zip(points,points[1:]+points[:1])):
        _check(profile.edges[i]==Edge(i+1,i,(i+1)%n,i) and profile.curves[i]==Curve('line',a+b),'source edge differs')
        expected=Coedge(i+1,i,0,0,1,UVMap('line',tuple((x+1)/2 for x in a[:2]+b[:2])))
        _check(profile.coedges[i]==expected,'source occurrence differs')
    _check(profile.area==sum((a[0]*b[1]-b[0]*a[1] for a,b in zip(points,points[1:]+points[:1])),F(0))/2,'source area differs')


def _assemble(profile,height):
    n=len(profile.vertices);body_id=_namespace();points=tuple(v.position for v in profile.vertices)
    vertices=tuple(Vertex(level*n+i+1,(p[0],p[1],z)) for level,z in enumerate((-height/2,height/2)) for i,p in enumerate(points))
    edges=[];curves=[]
    for level in range(2):
        for i in range(n):
            edges.append(Edge(len(edges)+1,level*n+i,level*n+(i+1)%n,len(curves)))
            curves.append(Curve('line',vertices[edges[-1].start].position+vertices[edges[-1].end].position))
    for i in range(n):
        edges.append(Edge(len(edges)+1,i,n+i,len(curves)));curves.append(Curve('line',vertices[i].position+vertices[n+i].position))
    coedges=[];loops=[];faces=[];charts=[];origins=[]
    def origin(kind,identifier,relation,parents,role,**maps):
        origins.append(Origin(EntityKey(body_id,1,kind,identifier),relation,tuple(parents),role,**maps))
    def cap_origin(kind,identifier,source_id,level):
        z=(-height/2,height/2)[level]
        origin(kind,identifier,'modified',(_key(profile,kind,source_id),),('lower_cap','upper_cap')[level],
               local_translation=(F(0),F(0),z),source_t=_T if kind in ('edge','coedge') else None,
               source_uv=_UV if kind in ('face','chart') else None,axial=(z,F(0),F(0)),
               orientation=-1 if level==0 and kind in ('coedge','loop','face','chart') else 1)
    for level in range(2):
        for kind in ('vertex','edge'):
            for i in range(n):cap_origin(kind,level*n+i+1,i+1,level)
    for i in range(n):origin('edge',2*n+i+1,'generated',(_key(profile,'vertex',i+1),),'swept_vertex',axial=(-height/2,F(1)*height,F(0)))
    for level,z in enumerate((-height/2,height/2)):
        fs=len(faces);word=[]
        for i in (range(n-1,-1,-1) if level==0 else range(n)):
            cs=len(coedges);word.append(cs)
            coedges.append(Coedge(cs+1,level*n+i,fs,fs,-1 if level==0 else 1,profile.coedges[i].uv_map))
            cap_origin('coedge',cs+1,i+1,level)
        loops.append(Loop(fs+1,fs,False,tuple(word)))
        cp=_plane(body_id,fs+1,profile.frame,z)
        faces.append(Face(fs+1,('bottom','top')[level],'convex_polygon',-1 if level==0 else 1,(fs,),(fs,)))
        charts.append(Chart(fs+1,fs,-1,cp))
        for kind in ('loop','face','chart'):cap_origin(kind,fs+1,1,level)
    for i,(a,b) in enumerate(zip(points,points[1:]+points[:1])):
        fs=len(faces);j=(i+1)%n;word=[]
        specs=((i,1,(0,0,1,0),'side_lower'),(2*n+j,1,(1,0,1,1),'side_end'),
               (n+i,-1,(0,1,1,1),'side_upper'),(2*n+i,-1,(0,0,0,1),'side_start'))
        parents=(_key(profile,'edge',i+1),_key(profile,'coedge',i+1))
        for es,sign,uv,role in specs:
            cs=len(coedges);word.append(cs);coedges.append(Coedge(cs+1,es,fs,fs,sign,UVMap('line',tuple(F(x) for x in uv))))
            if role in ('side_lower','side_upper'):
                origin('coedge',cs+1,'generated',parents,role,source_t=_T,
                       axial=((-height/2 if role=='side_lower' else height/2),F(0),F(0)),orientation=sign)
            else:
                vertex_id=(i if role=='side_start' else j)+1
                origin('coedge',cs+1,'generated',(_key(profile,'vertex',vertex_id),_key(profile,'edge',i+1)),role,
                       axial=(-height/2,height,F(0)),orientation=sign)
        loops.append(Loop(fs+1,fs,False,tuple(word)))
        cp=_patch(body_id,fs+1,profile.frame,tuple((p[0],p[1],z) for p in (a,b) for z in (-height/2,height/2)))
        faces.append(Face(fs+1,'side_'+str(i+1),'rectangle',1,(fs,),(fs,)));charts.append(Chart(fs+1,fs,-1,cp))
        for kind in ('loop','face','chart'):
            origin(kind,fs+1,'generated',parents,'side',source_t=_T,axial=(-height/2,F(0),height))
    solid=_publish(ExtrudedSolid,profile=profile,body_id=body_id,revision=1,frame=profile.frame,height=height,
                   vertices=vertices,curves=tuple(curves),edges=tuple(edges),coedges=tuple(coedges),loops=tuple(loops),
                   faces=tuple(faces),charts=tuple(charts),patches=tuple(c.patch for c in charts),volume=profile.area*height*F(profile.frame.scale)**3)
    records=[]
    for source in _keys(profile):
        images=tuple(o.target for o in origins if o.relation=='modified' and source in o.parents)
        generated=tuple(o.target for o in origins if o.relation=='generated' and source in o.parents)
        records.append(HistoryRecord(source,'modified',images,generated))
    return solid,_publish(History,records=tuple(records),origins=tuple(origins))


def _local_patch(patch,u,v):
    return tuple(sum(((1-u if i==0 else u)*(1-v if j==0 else v)*F(patch.controls[4*(2*i+j)+axis])
                      for i in range(2) for j in range(2)),F(0)) for axis in range(3))


def _validate_candidate(solid,history,profile,height):
    """Final private boundary before publication; no public failure switches."""
    n=len(profile.vertices)
    _check(solid.profile is profile and solid.frame is profile.frame and solid.height==height,'source binding differs')
    _check(type(solid.body_id) is int and 0<solid.body_id<1<<64 and solid.body_id!=profile.body_id
           and type(solid.revision) is int and solid.revision==1,'output namespace differs')
    _check(tuple(len(getattr(solid,t)) for _,t in _TABLES)==(2*n,3*n,6*n,n+2,n+2,n+2),'entity coverage differs')
    for _,table in _TABLES:
        _check(all(type(r.id) is int and r.id==i+1 for i,r in enumerate(getattr(solid,table))),'output IDs differ')
    seen_c=[];seen_l=[];seen_ch=[];uses=defaultdict(list);links=defaultdict(lambda:defaultdict(list));neighbors=defaultdict(set)
    for fs,face in enumerate(solid.faces):
        _check(face.orientation==(-1 if fs==0 else 1) and face.loop_slots==(fs,) and face.chart_slots==(fs,),'face ownership/sense differs')
        _check(face.kind==('convex_polygon' if fs<2 else 'rectangle'),'face domain differs')
        chart=solid.charts[fs];seen_ch.append(fs)
        _check(chart.face_slot==fs and chart.patch==solid.patches[fs] and chart.quadrant==-1,'chart differs')
        cp=chart.patch
        _check(cp.degree_u==cp.degree_v==1 and cp.uv==(0.,1.,0.,1.) and cp.origin==profile.frame.origin
               and cp.scale==profile.frame.scale and cp.lineage==Lineage(solid.body_id,5,fs+1,1,fs+1),'carrier metadata differs')
        if fs<2:expected=_plane(solid.body_id,fs+1,profile.frame,(-height/2,height/2)[fs])
        else:
            a=profile.vertices[fs-2].position;b=profile.vertices[(fs-1)%n].position
            expected=_patch(solid.body_id,fs+1,profile.frame,tuple((p[0],p[1],z) for p in (a,b) for z in (-height/2,height/2)))
        _check(cp.controls==expected.controls,'carrier controls differ')
        loop=solid.loops[fs];seen_l.append(fs)
        _check(loop.face_slot==fs and not loop.hole,'loop domain differs')
        expected_edges=(tuple(range(n-1,-1,-1)) if fs==0 else tuple(range(n,2*n)) if fs==1
                        else (fs-2,2*n+(fs-1)%n,n+fs-2,2*n+fs-2))
        _check(tuple(solid.coedges[s].edge_slot for s in loop.coedge_slots)==expected_edges,'complete boundary word differs')
        for prevslot,slot in zip(loop.coedge_slots[-1:]+loop.coedge_slots[:-1],loop.coedge_slots):
            c,prev=solid.coedges[slot],solid.coedges[prevslot];e,pe=solid.edges[c.edge_slot],solid.edges[prev.edge_slot]
            _check(c.face_slot==fs and c.loop_slot==fs and type(c.orientation) is int and c.orientation in (-1,1),'use ownership/sense differs')
            start=e.start if c.orientation==1 else e.end;endprev=pe.end if prev.orientation==1 else pe.start
            _check(start==endprev,'loop does not close by vertex identity')
            a=(prev.edge_slot,1 if prev.orientation==1 else 0);b=(c.edge_slot,0 if c.orientation==1 else 1)
            links[start][a].append(b);links[start][b].append(a);uses[c.edge_slot].append(c);seen_c.append(slot)
            _check(c.uv_map.kind=='line' and all(type(x) is F for x in c.uv_map.data),'exact line UV required')
            # Composition of bilinear carrier and affine line is degree<=2;
            # three exact values establish the whole spatial/UV polynomial.
            for t in (F(0),F(1,2),F(1)):
                u,v=solid.coedge_uv(c.id,t);local=_local_patch(cp,u,v)
                world=tuple(F(o)+F(profile.frame.scale)*x for o,x in zip(profile.frame.origin,local))
                _check(world==solid.edge_point(e.id,t),'whole incident UV/curve polynomial differs')
    for seen,count in ((seen_c,6*n),(seen_l,n+2),(seen_ch,n+2)):_check(sorted(seen)==list(range(count)),'duplicate/missing ownership')
    for es,e in enumerate(solid.edges):
        _check(len(uses[es])==2 and sum(c.orientation for c in uses[es])==0,'edge needs two opposed uses')
        a,b=(c.face_slot for c in uses[es]);neighbors[a].add(b);neighbors[b].add(a)
        _check(type(e.start) is int and type(e.end) is int and type(e.curve_slot) is int
               and 0<=e.start<2*n and 0<=e.end<2*n and 0<=e.curve_slot<len(solid.curves),'edge ownership differs')
        curve=solid.curves[e.curve_slot]
        _check(curve.kind=='line' and curve.data==solid.vertices[e.start].position+solid.vertices[e.end].position
               and all(type(x) is F for x in curve.data),'exact endpoint curve differs')
    def connected(graph,start):
        seen=set();todo=[start]
        while todo:
            v=todo.pop()
            if v not in seen:seen.add(v);todo.extend(graph[v])
        return seen
    _check(connected(neighbors,0)==set(range(n+2)),'shell disconnected')
    _check(set(links)==set(range(2*n)),'missing vertex links')
    for graph in links.values():
        _check(len(graph)==3 and all(len(v)==2 for v in graph.values()) and connected(graph,next(iter(graph)))==set(graph),'vertex link differs')
    _check(2*n-3*n+n+2==2 and solid.volume==profile.area*height*F(profile.frame.scale)**3,'Euler/volume differs')
    source_keys=set(_keys(profile));output_keys=set(_keys(solid));origin_by_key={o.target:o for o in history.origins}
    _check(len(history.records)==len(source_keys)==3*n+3 and len(history.origins)==len(origin_by_key)==14*n+6 and set(origin_by_key)==output_keys,'history coverage differs')
    _check({r.source for r in history.records}==source_keys,'source histories differ')
    _check(sum(o.relation=='generated' for o in history.origins)==8*n,'birth coverage differs')
    for record in history.records:
        _check(record.status=='modified' and len(record.images)==2,'source disposition differs')
        for target in record.images+record.generated:
            o=origin_by_key[target]
            _check(record.source in o.parents and (o.relation=='generated')==(target in record.generated),'forward history differs')
    for o in history.origins:
        _check(set(o.parents)<=source_keys and o.parents,'origin has foreign parents')
        # Bind ancestry to the actual target role, not merely parent kinds or
        # reciprocal tables which could consistently name the wrong source.
        kind,identifier=o.target.kind,o.target.entity_id
        cap_count=2*n if kind in ('vertex','edge','coedge') else 2
        if identifier<=cap_count:
            if kind in ('vertex','edge'):
                level=(identifier-1)//n;source_id=(identifier-1)%n+1
            elif kind=='coedge':
                level=(identifier-1)//n
                source_id=n-identifier+1 if level==0 else identifier-n
            else:level=identifier-1;source_id=1
            expected=('modified',('lower_cap','upper_cap')[level],(_key(profile,kind,source_id),))
        elif kind=='edge':
            expected=('generated','swept_vertex',(_key(profile,'vertex',identifier-2*n),))
        elif kind in ('loop','face','chart'):
            source_id=identifier-2
            expected=('generated','side',(_key(profile,'edge',source_id),_key(profile,'coedge',source_id)))
        else:
            i,position=divmod(identifier-2*n-1,4)
            role=('side_lower','side_end','side_upper','side_start')[position]
            if position in (0,2):parents=(_key(profile,'edge',i+1),_key(profile,'coedge',i+1))
            else:parents=(_key(profile,'vertex',(i+1)%n+1 if position==1 else i+1),_key(profile,'edge',i+1))
            expected=('generated',role,parents)
        _check((o.relation,o.role,o.parents)==expected,'origin does not name actual target/source role')
        for parent in o.parents:
            r=history.from_input(parent)
            _check(o.target in (r.generated if o.relation=='generated' else r.images),'reverse history differs')
        if o.relation=='modified':
            _check(len(o.parents)==1 and o.parents[0].kind==o.target.kind and o.role in ('lower_cap','upper_cap'),'modified cap origin differs')
            z=-height/2 if o.role=='lower_cap' else height/2;kind=o.target.kind
            _check(o.local_translation==(F(0),F(0),z) and o.axial==(z,F(0),F(0))
                   and o.source_t==(_T if kind in ('edge','coedge') else None)
                   and o.source_uv==(_UV if kind in ('face','chart') else None)
                   and o.orientation==(-1 if z<0 and kind in ('coedge','loop','face','chart') else 1),'cap history map differs')
        else:
            _check(o.local_translation is None and o.source_uv is None,'generated origin has cap map')
            if o.role=='swept_vertex':
                _check(o.target.kind=='edge' and len(o.parents)==1 and o.parents[0].kind=='vertex'
                       and o.source_t is None and o.axial==(-height/2,height,F(0)) and o.orientation==1,'vertical birth map differs')
            elif o.role=='side':
                _check(o.target.kind in ('loop','face','chart') and tuple(p.kind for p in o.parents)==('edge','coedge')
                       and o.source_t==_T and o.axial==(-height/2,F(0),height) and o.orientation==1,'side map differs')
            elif o.role in ('side_lower','side_upper'):
                lower=o.role=='side_lower'
                _check(o.target.kind=='coedge' and tuple(p.kind for p in o.parents)==('edge','coedge') and o.source_t==_T
                       and o.axial==((-height/2 if lower else height/2),F(0),F(0)) and o.orientation==(1 if lower else -1),'horizontal birth differs')
            else:
                _check(o.role in ('side_start','side_end') and o.target.kind=='coedge' and tuple(p.kind for p in o.parents)==('vertex','edge')
                       and o.source_t is None and o.axial==(-height/2,height,F(0)) and o.orientation==(-1 if o.role=='side_start' else 1),'vertical use map differs')


def extrude(profile,height):
    started=time.perf_counter_ns();previous=started;timings=[]
    def phase(name):
        nonlocal previous
        now=time.perf_counter_ns();timings.append((name,(now-previous)/1e6));previous=now
    if type(profile) is not PlanarProfile:raise TypeError('extrude requires exact owned PlanarProfile')
    height=_rational(height)
    if not F(1,4)<=height<=2 or (height*256).denominator!=1:raise UnsupportedExtrusion('height requires 1/256 grid in[1/4,2]')
    _validate_profile(profile);phase('admission_ms')
    solid,history=_assemble(profile,height);phase('assembly_history_ms')
    _validate_candidate(solid,history,profile,height);phase('validation_ms')
    timings.append(('total_ms',(time.perf_counter_ns()-started)/1e6))
    return _publish(ExtrusionResult,profile=profile,height=height,solid=solid,history=history,timings=tuple(timings))
