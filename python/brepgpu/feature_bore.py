"""Actual convex extrusion minus one strictly interior axial source cylinder.

This analytic family owns its trims and history; canonical readers stay narrow.
"""
from collections import defaultdict
from dataclasses import dataclass, replace
from fractions import Fraction as F
import time

from . import Lineage
from .body import (BodySnapshot, Vertex, Curve, Edge, Coedge, Loop, Face, Chart,
                   UVMap, _namespace, INVALID, REJECTED, BOUNDARY, INTERIOR)
from .boolean import (EntityKey, Origin, HistoryRecord, History, Measure, _circle)
from .extrusion import (ExtrusionResult, _rational, _parameter, _triple, _uv,
                        _supports, _publish, _validate_profile, _keys, _key)


class UnsupportedBore(ValueError):
    """Operands lie outside the complete interior axial-bore family."""


@dataclass(frozen=True, slots=True)
class HistoryPath:
    root: EntityKey
    steps: tuple


@dataclass(frozen=True, slots=True)
class ComposedOrigin:
    target: EntityKey
    paths: tuple

    @property
    def parents(self):
        return tuple(dict.fromkeys(p.root for p in self.paths))


@dataclass(frozen=True, slots=True, init=False)
class ComposedHistory:
    records: tuple
    origins: tuple

    def __init__(self,*args,**kwargs):
        raise TypeError('ComposedHistory is factory-only')

    def from_input(self,key):
        for record in self.records:
            if record.source==key:return record
        raise KeyError(key)

    def origin(self,key):
        for origin in self.origins:
            if origin.target==key:return origin
        raise KeyError(key)


@dataclass(frozen=True, slots=True, init=False)
class PointClassification:
    solid: object
    point: tuple
    label: str
    boundary_faces: tuple

    def __init__(self,*args,**kwargs):raise TypeError('factory-only classification')


@dataclass(frozen=True, slots=True, init=False)
class BoreSolid:
    feature: ExtrusionResult
    cutter: BodySnapshot
    body_id: int
    revision: int
    frame: object
    height: F
    center: tuple
    radius: F
    wall_band: tuple
    vertices: tuple
    curves: tuple
    edges: tuple
    coedges: tuple
    loops: tuple
    faces: tuple
    charts: tuple
    patches: tuple
    volume: Measure

    def __init__(self,*args,**kwargs):raise TypeError('BoreSolid is factory-only')

    @property
    def profile(self):return self.feature.profile

    def classify(self,slot,u,v):
        if isinstance(slot,bool):raise TypeError('chart index cannot be bool')
        import operator
        slot=operator.index(slot)
        if not 0<=slot<len(self.charts):raise IndexError('chart index outside result')
        uv=_uv(u,v)
        if uv is None:return INVALID
        u,v=uv;chart=self.charts[slot];face=self.faces[chart.face_slot]
        if face.kind=='cylinder':
            lo,hi=self.wall_band
            if not lo<=v<=hi:return REJECTED
            return BOUNDARY if (v in (lo,hi) or chart.quadrant==0 and u==0
                                or chart.quadrant==3 and u==1) else INTERIOR
        if face.kind=='convex_polygon_hole':
            x,y=2*u-1,2*v-1
            supports=_supports(self.profile,x,y)
            radial=(x-self.center[0])**2+(y-self.center[1])**2-self.radius**2
            if any(s<0 for s in supports) or radial<0:return REJECTED
            return BOUNDARY if radial==0 or any(s==0 for s in supports) else INTERIOR
        return BOUNDARY if u in (0,1) or v in (0,1) else INTERIOR

    def classify_point(self,point):
        point=_triple(point)
        x,y,z=((p-F(o))/F(self.frame.scale) for p,o in zip(point,self.frame.origin))
        values=(z+self.height/2,self.height/2-z)+_supports(self.profile,x,y)+(
            (x-self.center[0])**2+(y-self.center[1])**2-self.radius**2,)
        if any(v<0 for v in values):label,faces='exterior',()
        else:
            faces=tuple(i+1 for i,v in enumerate(values) if v==0)
            label='boundary' if faces else 'interior'
        return _publish(PointClassification,solid=self,point=point,label=label,boundary_faces=faces)

    def edge_point(self,edge_id,t):
        from .extrusion import _slot
        edge=self.edges[_slot(edge_id,len(self.edges),1)];t=_parameter(t)
        curve=self.curves[edge.curve_slot];d=curve.data
        if curve.kind=='line':local=tuple((1-t)*d[i]+t*d[i+3] for i in range(3))
        elif curve.kind=='circle_center':
            cx,cy,r,z=d;dx,dy=_circle(r,t);local=(cx+dx,cy+dy,z)
        else:raise ArithmeticError('unsupported actual bore curve')
        return tuple(F(o)+F(self.frame.scale)*p for o,p in zip(self.frame.origin,local))

    def coedge_uv(self,coedge_id,t):
        from .extrusion import _slot
        c=self.coedges[_slot(coedge_id,len(self.coedges),1)];t=_parameter(t);d=c.uv_map.data
        if c.uv_map.kind=='line':return tuple((1-t)*d[i]+t*d[i+2] for i in range(2))
        if c.uv_map.kind=='circle_wall':return 4*t,d[0]
        if c.uv_map.kind=='circle_bore_plane':
            cx,cy,r=d;x,y=_circle(r,t);return (cx+x+1)/2,(cy+y+1)/2
        raise ArithmeticError('unsupported actual bore UV map')

    def topology_input(self):
        from .adjacency import Source,TopologyInput
        def sources(rows):return tuple(Source(0,self.body_id,self.revision,r.id) for r in rows)
        return TopologyInput(sources(self.edges),sources(self.faces),sources(self.coedges),
                             tuple(c.edge_slot for c in self.coedges),tuple(c.face_slot for c in self.coedges),
                             tuple(c.orientation for c in self.coedges),snapshots=())


@dataclass(frozen=True, slots=True, init=False)
class BoreResult:
    feature: ExtrusionResult
    cutter: BodySnapshot
    solid: BoreSolid
    history: History
    composed_history: ComposedHistory
    timings: tuple

    def __init__(self,*args,**kwargs):raise TypeError('BoreResult is factory-only; use difference')

    def tessellate(self,*,tolerance,max_quarter_segments=256):
        from .feature_bore_meshing import tessellate
        return tessellate(self,tolerance=tolerance,max_quarter_segments=max_quarter_segments)


_TABLES=(('vertex','vertices'),('edge','edges'),('coedge','coedges'),
         ('loop','loops'),('face','faces'),('chart','charts'))
_FULL=(F(0),F(1));_SQUARE=(F(0),F(1),F(0),F(1))


def _require(condition,message):
    if not condition:raise ArithmeticError(message)


def _assemble(feature,cutter,center,radius,band):
    source=feature.solid;n=len(feature.profile.vertices);bid=_namespace()
    vertices=list(source.vertices);curves=list(source.curves);edges=list(source.edges)
    coedges=list(source.coedges);loops=list(source.loops);faces=[];charts=[];origins=[]
    wall=next(f for f in cutter.faces if f.kind=='cylinder')
    seam=next(e for e in cutter.edges if e.start!=e.end)
    cx,cy=center
    def origin(kind,id,relation,parents,**maps):
        origins.append(Origin(EntityKey(bid,1,kind,id),relation,tuple(parents),**maps))
    for kind,table in _TABLES:
        for row in getattr(source,table):
            maps={}
            if kind in ('edge','coedge'):maps['source_range']=_FULL
            if kind in ('face','chart'):maps['source_uv']=_SQUARE
            origin(kind,row.id,'modified' if kind in ('face','chart') and row.id<=2 else 'retained',
                   (_key(source,kind,row.id),),**maps)
    for level,z in enumerate((-feature.height/2,feature.height/2)):
        vi=len(vertices);vertices.append(Vertex(vi+1,(cx+radius,cy,z)))
        ei=len(edges);curves.append(Curve('circle_center',(cx,cy,radius,z)))
        edges.append(Edge(ei+1,vi,vi,len(curves)-1))
        parents=(_key(source,'face',level+1),_key(cutter,'face',wall.id))
        origin('vertex',vi+1,'generated',parents+(_key(cutter,'edge',seam.id),))
        origin('edge',ei+1,'generated',parents)
    se=len(edges);curves.append(Curve('line',vertices[2*n].position+vertices[2*n+1].position))
    edges.append(Edge(se+1,2*n,2*n+1,len(curves)-1))
    origin('edge',se+1,'modified',(_key(cutter,'edge',seam.id),),source_range=band)
    for level in range(2):
        fs=level;ls=len(loops);cs=len(coedges);sign=1 if level==0 else -1
        coedges.append(Coedge(cs+1,3*n+level,fs,ls,sign,UVMap('circle_bore_plane',(cx,cy,radius))))
        loops.append(Loop(ls+1,fs,True,(cs,)))
        parents=(_key(source,'face',fs+1),_key(cutter,'face',wall.id))
        origin('coedge',cs+1,'generated',parents,orientation=sign)
        origin('loop',ls+1,'generated',parents,orientation=sign)
    for old in source.faces:
        faces.append(replace(old,kind='convex_polygon_hole',loop_slots=old.loop_slots+(n+2+old.id-1,))
                     if old.id<=2 else old)
    for old in source.charts:
        patch=replace(old.patch,lineage=Lineage(bid,5,old.face_slot+1,1,old.id))
        charts.append(replace(old,patch=patch))
    fs=len(faces);ls=len(loops);word=[]
    specs=((se,1,UVMap('line',(F(0),band[0],F(0),band[1]))),
           (3*n+1,1,UVMap('circle_wall',(band[1],))),
           (se,-1,UVMap('line',(F(4),band[0],F(4),band[1]))),
           (3*n,-1,UVMap('circle_wall',(band[0],))))
    wallloop=cutter.loops[wall.loop_slots[0]]
    for i,(es,sign,mapping) in enumerate(specs):
        cs=len(coedges);word.append(cs);coedges.append(Coedge(cs+1,es,fs,ls,sign,mapping))
        if i in (0,2):
            atlas=0 if i==0 else 4
            old=next(cutter.coedges[s] for s in wallloop.coedge_slots
                     if cutter.coedges[s].uv_map.kind=='seam_wall' and cutter.coedges[s].uv_map.data==(atlas,))
            origin('coedge',cs+1,'modified',(_key(cutter,'coedge',old.id),),source_range=band,orientation=-1)
        else:
            level=1 if i==1 else 0
            origin('coedge',cs+1,'generated',(_key(source,'face',level+1),_key(cutter,'face',wall.id)),orientation=sign)
    loops.append(Loop(ls+1,fs,False,tuple(word)))
    slots=[]
    for oldslot in wall.chart_slots:
        old=cutter.charts[oldslot];cs=len(charts);slots.append(cs)
        patch=replace(old.patch,lineage=Lineage(bid,5,fs+1,1,cs+1))
        charts.append(Chart(cs+1,fs,old.quadrant,patch))
        origin('chart',cs+1,'modified',(_key(cutter,'chart',old.id),),source_uv=(*_FULL,*band),orientation=-1)
    faces.append(Face(fs+1,'wall','cylinder',-1,(ls,),tuple(slots)))
    for kind,id,parent in (('face',fs+1,wall.id),('loop',ls+1,wallloop.id)):
        origin(kind,id,'modified',(_key(cutter,kind,parent),),source_uv=(F(0),F(4),*band),orientation=-1)
    scale=F(source.frame.scale)
    solid=_publish(BoreSolid,feature=feature,cutter=cutter,body_id=bid,revision=1,frame=source.frame,
                   height=feature.height,center=center,radius=radius,wall_band=band,
                   vertices=tuple(vertices),curves=tuple(curves),edges=tuple(edges),coedges=tuple(coedges),
                   loops=tuple(loops),faces=tuple(faces),charts=tuple(charts),patches=tuple(c.patch for c in charts),
                   volume=Measure(feature.profile.area*feature.height*scale**3,-radius**2*feature.height*scale**3))
    records=[]
    for key in _keys(source)+_keys(cutter):
        images=tuple(o.target for o in origins if o.relation!='generated' and key in o.parents)
        generated=tuple(o.target for o in origins if o.relation=='generated' and key in o.parents)
        state=next((o.relation for o in origins if o.target in images),'deleted')
        records.append(HistoryRecord(key,state,images,generated))
    return solid,_publish(History,records=tuple(records),origins=tuple(origins))


def _compose(feature,cutter,history):
    middle=set(_keys(feature.solid));origins=[]
    for immediate in history.origins:
        paths=[]
        for parent in immediate.parents:
            if parent in middle:
                previous=feature.history.origin(parent)
                paths.extend(HistoryPath(root,(immediate,previous)) for root in previous.parents)
            else:paths.append(HistoryPath(parent,(immediate,)))
        origins.append(ComposedOrigin(immediate.target,tuple(paths)))
    records=[]
    for root in _keys(feature.profile)+_keys(cutter):
        images=[];births=[];modified=False
        for origin in origins:
            paths=tuple(p for p in origin.paths if p.root==root)
            if any(all(s.relation!='generated' for s in p.steps) for p in paths):
                images.append(origin.target)
                modified |= any(any(s.relation=='modified' for s in p.steps)
                                for p in paths if all(s.relation!='generated' for s in p.steps))
            if any(any(s.relation=='generated' for s in p.steps) for p in paths):births.append(origin.target)
        records.append(HistoryRecord(root,'modified' if images and modified else 'retained' if images else 'deleted',tuple(images),tuple(births)))
    return _publish(ComposedHistory,records=tuple(records),origins=tuple(origins))


def _validate(solid,history,composed):
    n=len(solid.profile.vertices);source=solid.feature.solid;cutter=solid.cutter
    _require(tuple(len(getattr(solid,t)) for _,t in _TABLES)==(2*n+2,3*n+3,6*n+6,n+5,n+3,n+6),'incomplete bore entity inventory')
    uses=defaultdict(list);seen=[];neighbors=defaultdict(set)
    for fs,face in enumerate(solid.faces):
        for ls in face.loop_slots:
            loop=solid.loops[ls];_require(loop.face_slot==fs,'loop has wrong face')
            word=loop.coedge_slots
            for before,cs in zip(word[-1:]+word[:-1],word):
                c=solid.coedges[cs];prev=solid.coedges[before];e=solid.edges[c.edge_slot];pe=solid.edges[prev.edge_slot]
                _require(c.face_slot==fs and c.loop_slot==ls,'coedge owner differs')
                _require((e.start if c.orientation>0 else e.end)==(pe.end if prev.orientation>0 else pe.start),'bore loop is open')
                uses[c.edge_slot].append(c);seen.append(cs)
        for chs in face.chart_slots:
            _require(solid.charts[chs].face_slot==fs,'chart owner differs')
    _require(sorted(seen)==list(range(6*n+6)),'coedge ownership incomplete')
    for es in range(len(solid.edges)):
        pair=uses[es];_require(len(pair)==2 and sum(c.orientation for c in pair)==0,'physical edge must have two opposed uses')
        a,b=(c.face_slot for c in pair);neighbors[a].add(b);neighbors[b].add(a)
    reached=set();todo=[0]
    while todo:
        v=todo.pop()
        if v not in reached:reached.add(v);todo.extend(neighbors[v])
    _require(reached==set(range(n+3)),'bore shell disconnected')
    outputs=set(_keys(solid));inputs=set(_keys(source)+_keys(cutter));roots=set(_keys(solid.profile)+_keys(cutter))
    _require(len(history.origins)==len(outputs) and {o.target for o in history.origins}==outputs,'immediate origins incomplete')
    _require(len(history.records)==len(inputs) and {r.source for r in history.records}==inputs,'input dispositions incomplete')
    for o in history.origins:
        _require(set(o.parents)<=inputs,'origin has foreign parent')
        for parent in o.parents:
            rec=history.from_input(parent)
            _require(o.target in (rec.generated if o.relation=='generated' else rec.images),'immediate reverse ancestry differs')
    _require(len(composed.origins)==len(outputs) and {o.target for o in composed.origins}==outputs,'composed outputs incomplete')
    _require(len(composed.records)==len(roots) and {r.source for r in composed.records}==roots,'composed roots incomplete')
    middle=set(_keys(source))
    for origin in composed.origins:
        immediate=history.origin(origin.target);expected=[]
        for parent in immediate.parents:
            if parent in middle:
                previous=solid.feature.history.origin(parent)
                expected.extend((r,(immediate,previous)) for r in previous.parents)
            else:expected.append((parent,(immediate,)))
        _require(len(origin.paths)==len(expected),'composition path missing')
        for path,(root,steps) in zip(origin.paths,expected):
            _require(path.root==root and root in roots and len(path.steps)==len(steps)
                     and all(a is b for a,b in zip(path.steps,steps)),'composition lost actual source/map step')


def difference(feature,cutter):
    """Cut a complete interior axial bore through the actual extrusion result."""
    started=time.perf_counter_ns();previous=started;timings=[]
    def phase(name):
        nonlocal previous
        now=time.perf_counter_ns();timings.append((name,(now-previous)/1e6));previous=now
    if type(feature) is not ExtrusionResult or type(cutter) is not BodySnapshot:
        raise TypeError('difference requires actual ExtrusionResult and cylinder BodySnapshot')
    if cutter.shape!='cylinder':raise UnsupportedBore('cutter must be the actual owned cylinder')
    _validate_profile(feature.profile)
    sf,sc=F(feature.solid.frame.scale),F(cutter.frame.scale)
    origin=tuple(F(x) for x in feature.solid.frame.origin);co=tuple(F(x) for x in cutter.frame.origin)
    h=feature.height
    if origin[2]!=co[2] or sc<=h*sf:
        raise UnsupportedBore('cutter must share Z origin and strictly exceed feature height')
    center=tuple((co[i]-origin[i])/sf for i in (0,1));radius=sc/(4*sf)
    points=tuple(v.position for v in feature.profile.vertices)
    for a,b in zip(points,points[1:]+points[:1]):
        dx,dy=b[0]-a[0],b[1]-a[1]
        support=dx*(center[1]-a[1])-dy*(center[0]-a[0])
        if support<=0 or support**2<=radius**2*(dx*dx+dy*dy):
            raise UnsupportedBore('whole cutter circle must lie strictly inside original profile')
    band=(F(1,2)-h*sf/(2*sc),F(1,2)+h*sf/(2*sc));phase('admission_ms')
    solid,history=_assemble(feature,cutter,center,radius,band);phase('assembly_history_ms')
    composed=_compose(feature,cutter,history);phase('composition_ms')
    _validate(solid,history,composed);phase('validation_ms')
    timings.append(('total_ms',(time.perf_counter_ns()-started)/1e6))
    return _publish(BoreResult,feature=feature,cutter=cutter,solid=solid,history=history,composed_history=composed,timings=tuple(timings))
