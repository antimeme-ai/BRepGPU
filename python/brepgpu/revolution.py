"""Exact full positive-Y revolution of an owned off-axis convex XY profile.

Construction owns physical rings, same-face seam occurrences and source history.
Native consumers use the existing patch and topology interfaces separately.
"""
from collections import defaultdict
from dataclasses import dataclass
from fractions import Fraction as F
from itertools import islice
import math
import operator
import struct
import time

from . import Lineage,Patch
from .body import (Vertex,Curve,Edge,Coedge,Loop,Face,Chart,UVMap,_namespace,
                   INVALID,INTERIOR,BOUNDARY)
from .boolean import EntityKey,Measure
from .extrusion import PlanarProfile,_validate_profile


class UnsupportedRevolution(ValueError):
    """The profile is outside the full off-axis lattice family."""


def _check(condition,message):
    if not condition:raise ArithmeticError(message)


def _rational(value):
    if type(value) in (int,F):return F(value)
    if type(value) is float:
        if not math.isfinite(value):raise ValueError('exact geometry requires finite values')
        return F(value)
    raise TypeError('geometry requires int, Fraction or finite binary64')


def _tuple(values,width):
    values=tuple(islice(iter(values),width+1))
    if len(values)!=width:raise ValueError('wrong exact coordinate count')
    return tuple(_rational(x) for x in values)


def _slot(value,count,base=0):
    if isinstance(value,bool):raise TypeError('indices cannot be bool')
    value=operator.index(value)
    if not base<=value<count+base:raise IndexError('index outside revolution table')
    return value-base


def _parameter(value):
    value=_rational(value)
    if not 0<=value<=1:raise ValueError('edge parameter outside [0,1]')
    return value


def _publish(cls,**values):
    result=object.__new__(cls)
    for name,value in values.items():object.__setattr__(result,name,value)
    return result


def _sign(value):return (value>0)-(value<0)


def _radical_sign(a,b,q):
    """Exact sign of a+b*sqrt(q), q>=0, with signs retained before squaring."""
    if not q or not b:return _sign(a)
    if not a:return _sign(b)
    if a*b>0:return _sign(a)
    return _sign(a)*_sign(a*a-b*b*q)


def _circle(tau):
    if tau==4:q,s=3,F(1)
    else:q=int(tau);s=tau-q
    x,y=(1-s*s)/(1+s*s),2*s/(1+s*s)
    return ((x,y),(-y,x),(-x,-y),(y,-x))[q]


@dataclass(frozen=True,slots=True)
class Origin:
    target:EntityKey
    relation:str
    parents:tuple
    role:str
    source_t:object=None
    source_uv:object=None
    periodic:object=None
    orientation:int=1

    def __post_init__(self):
        if type(self.target) is not EntityKey:raise TypeError('target requires EntityKey')
        if self.relation not in ('modified','generated'):raise ValueError('invalid revolution origin relation')
        parents=tuple(self.parents)
        if not parents or any(type(p) is not EntityKey for p in parents) or len(set(parents))!=len(parents):raise ValueError('origin requires distinct original typed parents')
        object.__setattr__(self,'parents',parents)
        if self.role not in ('anchor','seam','seam_end','seam_start','ring','ring_start','ring_end','swept_loop','swept_face','swept_chart'):raise ValueError('unknown revolution origin role')
        for name in ('source_t','periodic'):
            value=getattr(self,name)
            if value is not None:object.__setattr__(self,name,_tuple(value,3))
        if self.source_uv is not None:
            values=tuple(islice(iter(self.source_uv),3))
            if len(values)!=2:raise ValueError('source UV requires two affine rows')
            object.__setattr__(self,'source_uv',tuple(_tuple(v,3) for v in values))
        if type(self.orientation) is not int or self.orientation not in (-1,1):raise ValueError('orientation requires an integer sign')


@dataclass(frozen=True,slots=True)
class HistoryRecord:
    source:EntityKey
    status:str
    images:tuple
    generated:tuple

    def __post_init__(self):
        if type(self.source) is not EntityKey:raise TypeError('history source requires EntityKey')
        if self.status not in ('modified','consumed'):raise ValueError('invalid source disposition')
        for name in ('images','generated'):
            values=tuple(getattr(self,name))
            if any(type(x) is not EntityKey for x in values) or len(set(values))!=len(values):raise ValueError('invalid history targets')
            object.__setattr__(self,name,values)
        if (not self.images)!=(self.status=='consumed'):raise ValueError('consumed iff no same-kind boundary image')
        expected=2 if self.source.kind=='coedge' else 1 if self.source.kind in ('vertex','edge') else 0
        if len(self.images)!=expected or any(x.kind!=self.source.kind for x in self.images):raise ValueError('source kind has wrong modified image count')


@dataclass(frozen=True,slots=True,init=False)
class History:
    records:tuple
    origins:tuple

    def __init__(self,*args,**kwargs):raise TypeError('History is factory-only')

    def from_input(self,key):
        if type(key) is EntityKey:
            for record in self.records:
                if record.source==key:return record
        raise KeyError(key)

    def origin(self,key):
        if type(key) is EntityKey:
            for origin in self.origins:
                if origin.target==key:return origin
        raise KeyError(key)


@dataclass(frozen=True,slots=True,init=False)
class PointClassification:
    solid:object
    point:tuple
    label:str
    boundary_faces:tuple

    def __init__(self,*args,**kwargs):raise TypeError('PointClassification is factory-only')


@dataclass(frozen=True,slots=True,init=False)
class RevolvedSolid:
    profile:PlanarProfile
    body_id:int
    revision:int
    frame:object
    vertices:tuple
    curves:tuple
    edges:tuple
    coedges:tuple
    loops:tuple
    faces:tuple
    charts:tuple
    patches:tuple
    volume:Measure

    def __init__(self,*args,**kwargs):raise TypeError('RevolvedSolid is factory-only; use revolve')

    def classify(self,chart_slot,u,v):
        slot=_slot(chart_slot,len(self.charts))
        for x in (u,v):
            if type(x) not in (int,F,float):raise TypeError('trim coordinates require exact numeric values')
        if any(type(x) is float and not math.isfinite(x) for x in (u,v)):return INVALID
        u,v=_rational(u),_rational(v)
        if not 0<=u<=1 or not 0<=v<=1:return INVALID
        q=self.charts[slot].quadrant
        return BOUNDARY if v in (0,1) or q==0 and u==0 or q==3 and u==1 else INTERIOR

    def classify_point(self,point):
        point=_tuple(point,3)
        x,y,z=tuple((p-F(o))/F(self.frame.scale) for p,o in zip(point,self.frame.origin));q=x*x+z*z
        points=tuple(v.position for v in self.profile.vertices);signs=[]
        for a,b in zip(points,points[1:]+points[:1]):
            dr,da=b[0]-a[0],b[1]-a[1]
            signs.append(_radical_sign(dr*(y-a[1])+da*a[0],-da,q))
        if min(signs)<0:label,faces='exterior',()
        else:
            faces=tuple(i+1 for i,v in enumerate(signs) if v==0)
            label='boundary' if faces else 'interior'
        return _publish(PointClassification,solid=self,point=point,label=label,boundary_faces=faces)

    def edge_point(self,edge_id,t):
        edge=self.edges[_slot(edge_id,len(self.edges),1)];t=_parameter(t);curve=self.curves[edge.curve_slot]
        if curve.kind=='circle_y':
            radius,axial=curve.data;c,s=_circle(4*t);local=(radius*c,axial,-radius*s)
        else:
            _check(curve.kind=='line','unsupported revolution curve')
            d=curve.data;local=tuple((1-t)*d[k]+t*d[k+3] for k in range(3))
        return tuple(F(o)+F(self.frame.scale)*p for o,p in zip(self.frame.origin,local))

    def coedge_uv(self,coedge_id,t):
        use=self.coedges[_slot(coedge_id,len(self.coedges),1)];t=_parameter(t);d=use.uv_map.data
        return tuple((1-t)*d[k]+t*d[k+2] for k in range(2))

    def topology_input(self):
        from .adjacency import Source,TopologyInput
        def sources(records):return tuple(Source(0,self.body_id,self.revision,r.id) for r in records)
        return TopologyInput(sources(self.edges),sources(self.faces),sources(self.coedges),
                             tuple(c.edge_slot for c in self.coedges),tuple(c.face_slot for c in self.coedges),
                             tuple(c.orientation for c in self.coedges),snapshots=())


@dataclass(frozen=True,slots=True,init=False)
class RevolutionResult:
    profile:PlanarProfile
    solid:RevolvedSolid
    history:History
    timings:tuple

    def __init__(self,*args,**kwargs):raise TypeError('RevolutionResult is factory-only; use revolve')


_TABLES=(('vertex','vertices'),('edge','edges'),('coedge','coedges'),('loop','loops'),('face','faces'),('chart','charts'))
_ZERO=(F(0),F(0),F(0));_T=(F(0),F(1),F(0));_V=(F(0),F(0),F(1))


def _key(body,kind,identifier):return EntityKey(body.body_id,body.revision,kind,identifier)


def _keys(body):return tuple(_key(body,kind,r.id) for kind,table in _TABLES for r in getattr(body,table))


def _source_uv(point,delta=None,parameter=0):
    return tuple(((point[k]+1)/2,F(0),F(0)) if delta is None else
                 ((point[k]+1)/2,delta[k]/2 if parameter==0 else F(0),delta[k]/2 if parameter==1 else F(0)) for k in range(2))


def _carrier(body_id,face,chart,q,frame,a,b):
    controls=[]
    for x,y,w in ((1,0,1),(1,1,1),(0,2,2)):
        x,y=((x,y),(-y,x),(-x,-y),(y,-x))[q]
        for r,axial,_ in (a,b):controls.append((float(r*x),float(axial*w),float(-r*y),float(w)))
    return Patch(2,1,tuple(controls),Lineage(body_id,5,face,1,chart),
                 uv=(float(q),float(q+1),0.,1.),origin=frame.origin,scale=frame.scale)


def _assemble(profile):
    n=len(profile.vertices);points=tuple(v.position for v in profile.vertices);body_id=_namespace()
    curves=tuple(Curve('circle_y',p[:2]) for p in points)+tuple(profile.curves[e.curve_slot] for e in profile.edges)
    edges=tuple(Edge(i+1,i,i,i) for i in range(n))+tuple(Edge(n+i+1,i,(i+1)%n,n+i) for i in range(n))
    coedges=[];loops=[];faces=[];charts=[]
    for i,(a,b) in enumerate(zip(points,points[1:]+points[:1])):
        specs=((i,1,(0,0,4,0)),(n+i,1,(4,0,4,1)),((i+1)%n,-1,(0,1,4,1)),(n+i,-1,(0,0,0,1)))
        for k,(es,sign,uv) in enumerate(specs):coedges.append(Coedge(4*i+k+1,es,i,i,sign,UVMap('line',tuple(F(x) for x in uv))))
        loops.append(Loop(i+1,i,False,tuple(range(4*i,4*i+4))))
        faces.append(Face(i+1,'revolved_'+str(i+1),'revolved_line',1,(i,),tuple(range(4*i,4*i+4))))
        for q in range(4):
            cs=4*i+q;cp=_carrier(body_id,i+1,cs+1,q,profile.frame,a,b);charts.append(Chart(cs+1,i,q,cp))
    coefficient=sum(((a[0]+b[0])*(a[0]*b[1]-b[0]*a[1]) for a,b in zip(points,points[1:]+points[:1])),F(0))*F(profile.frame.scale)**3/3
    solid=_publish(RevolvedSolid,profile=profile,body_id=body_id,revision=1,frame=profile.frame,
                   vertices=profile.vertices,curves=curves,edges=edges,coedges=tuple(coedges),loops=tuple(loops),
                   faces=tuple(faces),charts=tuple(charts),patches=tuple(c.patch for c in charts),volume=Measure(F(0),coefficient))
    origins=[]
    def add(kind,identifier,relation,parents,role,**maps):
        origins.append(Origin(_key(solid,kind,identifier),relation,tuple(parents),role,**maps))
    for i,p in enumerate(points):add('vertex',i+1,'modified',(_key(profile,'vertex',i+1),),'anchor',source_uv=_source_uv(p),periodic=_ZERO)
    for i,p in enumerate(points):add('edge',i+1,'generated',(_key(profile,'vertex',i+1),),'ring',source_uv=_source_uv(p),periodic=(F(0),F(4),F(0)))
    for i,(a,b) in enumerate(zip(points,points[1:]+points[:1])):
        delta=tuple(y-x for x,y in zip(a,b))
        add('edge',n+i+1,'modified',(_key(profile,'edge',i+1),),'seam',source_t=_T,source_uv=_source_uv(a,delta),periodic=_ZERO)
    for i,(a,b) in enumerate(zip(points,points[1:]+points[:1])):
        j=(i+1)%n;delta=tuple(y-x for x,y in zip(a,b));own=(_key(profile,'edge',i+1),_key(profile,'coedge',i+1))
        add('coedge',4*i+1,'generated',(_key(profile,'vertex',i+1),)+own,'ring_start',source_t=_ZERO,source_uv=_source_uv(a),periodic=(F(0),F(4),F(0)))
        add('coedge',4*i+2,'modified',(_key(profile,'coedge',i+1),),'seam_end',source_t=_T,source_uv=_source_uv(a,delta),periodic=(F(4),F(0),F(0)))
        add('coedge',4*i+3,'generated',(_key(profile,'vertex',j+1),)+own,'ring_end',source_t=(F(1),F(0),F(0)),source_uv=_source_uv(b),periodic=(F(0),F(4),F(0)),orientation=-1)
        add('coedge',4*i+4,'modified',(_key(profile,'coedge',i+1),),'seam_start',source_t=_T,source_uv=_source_uv(a,delta),periodic=_ZERO,orientation=-1)
    for kind,role,parent in (('loop','swept_loop','loop'),('face','swept_face','face'),('chart','swept_chart','chart')):
        for i,(a,b) in enumerate(zip(points,points[1:]+points[:1])):
            own=(_key(profile,'edge',i+1),_key(profile,'coedge',i+1),_key(profile,parent,1));delta=tuple(y-x for x,y in zip(a,b))
            for q in (range(4) if kind=='chart' else (0,)):
                add(kind,4*i+q+1 if kind=='chart' else i+1,'generated',own,role,source_t=_V,source_uv=_source_uv(a,delta,1),periodic=(F(q),F(1),F(0)))
    records=[]
    for source in _keys(profile):
        images=tuple(o.target for o in origins if o.relation=='modified' and source in o.parents)
        generated=tuple(o.target for o in origins if o.relation=='generated' and source in o.parents)
        records.append(HistoryRecord(source,'modified' if images else 'consumed',images,generated))
    return solid,_publish(History,records=tuple(records),origins=tuple(origins))


def _same_float_tuple(actual,expected):
    return (type(actual) is tuple and len(actual)==len(expected) and
            all(type(a) is float and struct.pack('<d',a)==struct.pack('<d',b) for a,b in zip(actual,expected)))


def _admit_profile(profile):
    _validate_profile(profile)
    _check(type(profile.area) is F,'source area must remain exact Fraction')
    _check(type(profile.curves) is tuple and len(profile.curves)==len(profile.vertices),'complete original line table required')
    _check(type(profile.frame.origin) is tuple and len(profile.frame.origin)==3 and
           all(type(x) is float and math.isfinite(x) and abs(x)<=16 for x in profile.frame.origin)
           and type(profile.frame.scale) is float and profile.frame.scale in (.25,.5,1.,2.,4.),'typed source Frame differs')
    for _,table in _TABLES:_check(type(getattr(profile,table)) is tuple,'source entity tables must remain tuples')
    for curve in profile.curves:_check(type(curve.data) is tuple and all(type(x) is F for x in curve.data),'exact original line coefficients required')
    for e in profile.edges:_check(all(type(x) is int for x in (e.start,e.end,e.curve_slot)),'typed source edge slots required')
    for c in profile.coedges:
        _check(all(type(x) is int for x in (c.edge_slot,c.face_slot,c.loop_slot,c.orientation))
               and type(c.uv_map.data) is tuple and all(type(x) is F for x in c.uv_map.data),'typed exact source occurrence required')
    for loop in profile.loops:_check(type(loop.face_slot) is int and type(loop.hole) is bool and type(loop.coedge_slots) is tuple
                                     and all(type(x) is int for x in loop.coedge_slots),'typed source loop required')
    for face in profile.faces:_check(type(face.orientation) is int and all(type(word) is tuple and all(type(x) is int for x in word)
                                     for word in (face.loop_slots,face.chart_slots)),'typed source face required')
    cp=profile.patches[0];chart=profile.charts[0]
    _check(type(profile.patches) is tuple and type(chart.face_slot) is int and type(chart.quadrant) is int and chart.patch is cp,'actual source carrier ownership differs')
    expected=tuple(float(v) for x in (-1,1) for y in (-1,1) for v in (x,y,0,1))
    _check(type(cp.degree_u) is int and type(cp.degree_v) is int and _same_float_tuple(cp.controls,expected)
           and _same_float_tuple(cp.uv,(0.,1.,0.,1.)) and _same_float_tuple(cp.origin,profile.frame.origin)
           and type(cp.scale) is float and struct.pack('<d',cp.scale)==struct.pack('<d',profile.frame.scale),'source native bits/types differ')
    lin=cp.lineage
    _check(type(lin) is Lineage and all(type(x) is int for x in (lin.body_id,lin.entity_kind,lin.entity_id,lin.revision,lin.chart_id)),'typed source lineage required')
    if any(v.position[0]<F(1,4) for v in profile.vertices):raise UnsupportedRevolution('profile radius must remain in[1/4,3/4]')


def _validate_candidate(solid,history,profile):
    """Last private complete-geometry/history boundary before result publication."""
    n=len(profile.vertices);points=tuple(v.position for v in profile.vertices)
    _check(solid.profile is profile and solid.frame is profile.frame,'result replaced actual source')
    _check(type(solid.body_id) is int and 0<solid.body_id<1<<64 and solid.body_id!=profile.body_id
           and type(solid.revision) is int and solid.revision==1,'fresh output namespace differs')
    _check(tuple(len(getattr(solid,t)) for _,t in _TABLES)==(n,2*n,4*n,n,n,4*n),'complete output entity coverage differs')
    for _,table in _TABLES:
        _check(type(getattr(solid,table)) is tuple and all(type(r.id) is int and r.id==i+1 for i,r in enumerate(getattr(solid,table))),'dense typed output IDs differ')
    _check(solid.vertices==profile.vertices and all(type(x) is F for v in solid.vertices for x in v.position),'theta0 anchor geometry differs')
    _check(type(solid.curves) is tuple and len(solid.curves)==2*n,'curve coverage differs')
    for es,e in enumerate(solid.edges):
        i=es%n;expected=(i,i,es) if es<n else (i,(i+1)%n,es)
        _check(all(type(x) is int for x in (e.start,e.end,e.curve_slot)) and (e.start,e.end,e.curve_slot)==expected,'physical ring/seam ownership differs')
        curve=solid.curves[es]
        wanted=Curve('circle_y',points[i][:2]) if es<n else profile.curves[profile.edges[i].curve_slot]
        _check(curve==wanted and all(type(v) is F for v in curve.data),'whole ring/seam curve differs')
    seen_c=[];seen_l=[];seen_ch=[];uses=defaultdict(list);links=defaultdict(lambda:defaultdict(list));neighbors=defaultdict(set)
    for fs,face in enumerate(solid.faces):
        _check(face==Face(fs+1,'revolved_'+str(fs+1),'revolved_line',1,(fs,),tuple(range(4*fs,4*fs+4)))
               and type(face.orientation) is int and all(type(word) is tuple and all(type(x) is int for x in word)
                    for word in (face.loop_slots,face.chart_slots)),'face source domain/sense/ownership differs')
        a,b=points[fs],points[(fs+1)%n]
        for q,cs in enumerate(face.chart_slots):
            chart=solid.charts[cs];cp=solid.patches[cs];seen_ch.append(cs)
            _check(type(chart.face_slot) is int and type(chart.quadrant) is int and (chart.face_slot,chart.quadrant)==(fs,q) and chart.patch is cp,'chart owner/quarter differs')
            expected=_carrier(solid.body_id,fs+1,cs+1,q,profile.frame,a,b)
            _check(type(cp.degree_u) is int and type(cp.degree_v) is int and (cp.degree_u,cp.degree_v)==(2,1),'rational degree differs')
            _check(_same_float_tuple(cp.controls,expected.controls) and _same_float_tuple(cp.uv,expected.uv)
                   and _same_float_tuple(cp.origin,profile.frame.origin) and type(cp.scale) is float
                   and struct.pack('<d',cp.scale)==struct.pack('<d',profile.frame.scale),'complete exact native carrier/frame bits differ')
            lin=cp.lineage
            _check(type(lin) is Lineage and all(type(x) is int for x in (lin.body_id,lin.entity_kind,lin.entity_id,lin.revision,lin.chart_id))
                   and lin==expected.lineage,'actual native lineage differs')
        loop=solid.loops[fs];seen_l.append(fs)
        _check(type(loop.face_slot) is int and loop.face_slot==fs and type(loop.hole) is bool and not loop.hole
               and type(loop.coedge_slots) is tuple and loop.coedge_slots==tuple(range(4*fs,4*fs+4))
               and all(type(v) is int for v in loop.coedge_slots),'complete periodic loop differs')
        specs=((fs,1,(0,0,4,0)),(n+fs,1,(4,0,4,1)),((fs+1)%n,-1,(0,1,4,1)),(n+fs,-1,(0,0,0,1)))
        for slot,(es,sign,uv) in zip(loop.coedge_slots,specs):
            c=solid.coedges[slot];seen_c.append(slot)
            _check(all(type(x) is int for x in (c.edge_slot,c.face_slot,c.loop_slot,c.orientation)) and
                   (c.edge_slot,c.face_slot,c.loop_slot,c.orientation)==(es,fs,fs,sign),'physical occurrence owner/word differs')
            _check(type(c.uv_map.data) is tuple and all(type(x) is F for x in c.uv_map.data)
                   and c.uv_map==UVMap('line',tuple(F(x) for x in uv)),'whole intrinsic raw UV map differs')
            # Literal complete curve kind/radius/axial records plus the raw UV
            # words and exact carrier nets establish every rational edge map,
            # including all four ring spans, without finite sample inference.
            uses[es].append(c)
        word=loop.coedge_slots
        for previous,current in zip(word[-1:]+word[:-1],word):
            p,c=solid.coedges[previous],solid.coedges[current];pe,ce=solid.edges[p.edge_slot],solid.edges[c.edge_slot]
            pend=1 if p.orientation==1 else 0;cstart=0 if c.orientation==1 else 1
            vertex=(pe.start,pe.end)[pend];_check(vertex==(ce.start,ce.end)[cstart],'loop does not close by identity')
            x,y=(p.edge_slot,pend),(c.edge_slot,cstart);links[vertex][x].append(y);links[vertex][y].append(x)
    for seen,count in ((seen_c,4*n),(seen_l,n),(seen_ch,4*n)):_check(sorted(seen)==list(range(count)),'missing/duplicate ownership')
    for es in range(2*n):
        _check(len(uses[es])==2 and sum(c.orientation for c in uses[es])==0,'physical edge needs two opposed occurrences')
        a,b=(c.face_slot for c in uses[es]);_check((a==b)==(es>=n),'ring/seam incidence differs');neighbors[a].add(b);neighbors[b].add(a)
    def visit(graph,start):
        found=set();todo=[start]
        while todo:
            v=todo.pop()
            if v not in found:found.add(v);todo.extend(graph[v])
        return found
    _check(visit(neighbors,0)==set(range(n)) and set(links)==set(range(n)),'shell/link ownership differs')
    for vertex,graph in links.items():
        expected={(es,end) for es,e in enumerate(solid.edges) for end,v in enumerate((e.start,e.end)) if v==vertex}
        _check(len(expected)==4 and set(graph)==expected and all(len(v)==2 for v in graph.values()) and visit(graph,next(iter(graph)))==expected,'anchor endpoint-germ link differs')
    coefficient=sum(((a[0]+b[0])*(a[0]*b[1]-b[0]*a[1]) for a,b in zip(points,points[1:]+points[:1])),F(0))*F(profile.frame.scale)**3/3
    _check(n-2*n+n==0 and type(solid.volume) is Measure and type(solid.volume.rational) is F
           and type(solid.volume.pi) is F and solid.volume.rational==0 and solid.volume.pi==coefficient>0,'genus/positive exact radial moment differs')
    source_keys=_keys(profile);target_keys=_keys(solid)
    _check(type(history.records) is tuple and type(history.origins) is tuple and len(history.records)==3*n+3
           and tuple(o.target for o in history.origins)==target_keys,'complete ordered history coverage differs')
    _check(tuple(r.source for r in history.records)==source_keys,'complete original-source record order differs')
    origins={o.target:o for o in history.origins};_check(len(origins)==13*n,'duplicate history origin')
    for o in history.origins:
        for key in (o.target,)+o.parents:
            _check(type(key) is EntityKey and type(key.kind) is str and key.kind in dict(_TABLES)
                   and all(type(x) is int and 0<x<1<<64 for x in (key.body_id,key.revision,key.entity_id)),'strict typed original/output history key differs')
        kind,identifier=o.target.kind,o.target.entity_id
        if kind=='vertex':
            p=points[identifier-1];expected=('modified',(_key(profile,'vertex',identifier),),'anchor',None,_source_uv(p),_ZERO,1)
        elif kind=='edge':
            i=(identifier-1)%n;a,b=points[i],points[(i+1)%n];delta=tuple(y-x for x,y in zip(a,b))
            expected=(('generated',(_key(profile,'vertex',i+1),),'ring',None,_source_uv(a),(F(0),F(4),F(0)),1) if identifier<=n else
                      ('modified',(_key(profile,'edge',i+1),),'seam',_T,_source_uv(a,delta),_ZERO,1))
        elif kind=='coedge':
            i,k=divmod(identifier-1,4);j=(i+1)%n;a,b=points[i],points[j];delta=tuple(y-x for x,y in zip(a,b))
            if k in (0,2):
                vertex=i if k==0 else j;parents=(_key(profile,'vertex',vertex+1),_key(profile,'edge',i+1),_key(profile,'coedge',i+1))
                expected=('generated',parents,'ring_start' if k==0 else 'ring_end',(F(k//2),F(0),F(0)),_source_uv(points[vertex]),(F(0),F(4),F(0)),1 if k==0 else -1)
            else:
                expected=('modified',(_key(profile,'coedge',i+1),),'seam_end' if k==1 else 'seam_start',_T,
                          _source_uv(a,delta),(F(4 if k==1 else 0),F(0),F(0)),1 if k==1 else -1)
        else:
            i,q=divmod(identifier-1,4) if kind=='chart' else (identifier-1,0)
            a,b=points[i],points[(i+1)%n];delta=tuple(y-x for x,y in zip(a,b))
            expected=('generated',(_key(profile,'edge',i+1),_key(profile,'coedge',i+1),_key(profile,kind,1)),
                      'swept_'+kind,_V,_source_uv(a,delta,1),(F(q),F(1),F(0)),1)
        _check(type(o) is Origin and type(o.target) is EntityKey and type(o.parents) is tuple
               and all(type(p) is EntityKey for p in o.parents),'typed history origin differs')
        for name in ('source_t','periodic'):
            value=getattr(o,name)
            _check(value is None or type(value) is tuple and len(value)==3 and all(type(v) is F for v in value),'exact affine history map differs')
        if o.source_uv is not None:_check(type(o.source_uv) is tuple and len(o.source_uv)==2 and all(type(row) is tuple and len(row)==3 and all(type(v) is F for v in row) for row in o.source_uv),'exact source UV map differs')
        _check(type(o.orientation) is int and (o.relation,o.parents,o.role,o.source_t,o.source_uv,o.periodic,o.orientation)==expected,'origin does not name actual target/source role/maps')
    for rec in history.records:
        images=tuple(o.target for o in history.origins if o.relation=='modified' and rec.source in o.parents)
        generated=tuple(o.target for o in history.origins if o.relation=='generated' and rec.source in o.parents)
        _check(type(rec) is HistoryRecord and type(rec.images) is tuple and type(rec.generated) is tuple and rec.images==images
               and rec.generated==generated and rec.status==('modified' if images else 'consumed'),'complete forward/reverse source history differs')


def revolve(profile):
    started=time.perf_counter_ns();previous=started;timings=[]
    def phase(name):
        nonlocal previous
        now=time.perf_counter_ns();timings.append((name,(now-previous)/1e6));previous=now
    if type(profile) is not PlanarProfile:raise TypeError('revolve requires exact factory PlanarProfile')
    _admit_profile(profile)
    phase('admission_ms');solid,history=_assemble(profile);phase('assembly_history_ms')
    _validate_candidate(solid,history,profile);phase('validation_ms')
    timings.append(('total_ms',(time.perf_counter_ns()-started)/1e6))
    return _publish(RevolutionResult,profile=profile,solid=solid,history=history,timings=tuple(timings))
