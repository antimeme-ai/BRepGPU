"""Exact CPU through-bore construction from actual owned operands and sections.

This separate solid type owns its new trim/topology/history. Original carriers
retain their own frames; canonical BodySnapshot readers are not widened.
"""
from collections import Counter, defaultdict
from dataclasses import dataclass, replace
from fractions import Fraction as F
from itertools import islice
import math
import operator
import time

from . import Lineage
from .body import (BodySnapshot, Vertex, Curve, Edge, Coedge, Loop, Face, Chart,
                   UVMap, _namespace, REJECTED, INTERIOR, BOUNDARY, INVALID)
from .classification import classify_point as _source_classify
from .intersections import FaceRef, ExactMap, intersect_faces
from .arrangements import arrange_face


class UnsupportedBoolean(ValueError):
    """The operands are outside the complete through-bore family."""


def _check(condition, message):
    if not condition:
        raise ArithmeticError(message)


def _integer(value, lower=1, upper=(1<<64)-1):
    if isinstance(value, bool):
        raise TypeError('Boolean IDs/indices cannot be bool')
    result = operator.index(value)
    if not lower<=result<=upper:
        raise ValueError('Boolean ID/index outside range')
    return result


def _rational(value):
    if type(value) is F:
        return value
    if type(value) is int:
        return F(value)
    if type(value) is float:
        if not math.isfinite(value):
            raise ValueError('exact geometry requires finite coordinates')
        return F(value)
    raise TypeError('exact geometry requires int, Fraction or finite float')


def _point(point):
    values = tuple(islice(iter(point),4))
    if len(values)!=3:
        raise ValueError('point requires exactly three coordinates')
    return tuple(_rational(x) for x in values)


def _publish(cls, **values):
    result = object.__new__(cls)
    for name,value in values.items():
        object.__setattr__(result,name,value)
    return result


@dataclass(frozen=True, slots=True)
class EntityKey:
    body_id: int
    revision: int
    kind: str
    entity_id: int

    def __post_init__(self):
        for name in ('body_id','revision','entity_id'):
            object.__setattr__(self,name,_integer(getattr(self,name)))
        if self.kind not in ('vertex','edge','coedge','loop','face','chart'):
            raise ValueError('unknown Boolean entity kind')


@dataclass(frozen=True, slots=True)
class Origin:
    target: EntityKey
    relation: str
    parents: tuple
    source_range: object = None
    source_uv: object = None
    orientation: int = 1
    section: object = None
    piece_ids: tuple = ()
    event_ids: tuple = ()
    arrangement: tuple = ()

    def __post_init__(self):
        if type(self.target) is not EntityKey:
            raise TypeError('origin target requires EntityKey')
        if self.relation not in ('retained','modified','generated'):
            raise ValueError('unknown origin relation')
        parents = tuple(self.parents)
        if not parents or any(type(p) is not EntityKey for p in parents) or len(set(parents))!=len(parents):
            raise ValueError('origin requires distinct typed source parents')
        object.__setattr__(self,'parents',parents)
        for name,width in (('source_range',2),('source_uv',4)):
            value = getattr(self,name)
            if value is not None:
                value = tuple(_rational(x) for x in value)
                if len(value)!=width or any(value[i]>value[i+1] for i in range(0,width,2)):
                    raise ValueError('invalid source range/domain')
                object.__setattr__(self,name,value)
        if type(self.orientation) is not int or self.orientation not in (-1,1):
            raise ValueError('orientation must be an integer sign')
        if self.section is not None:
            object.__setattr__(self,'section',_integer(self.section,0,1))
        for name in ('piece_ids','event_ids'):
            object.__setattr__(self,name,tuple(_integer(x,0) for x in getattr(self,name)))
        arrangement = tuple(self.arrangement)
        if arrangement:
            if len(arrangement)!=4:
                raise ValueError('arrangement provenance requires four fields')
            arrangement = tuple(None if x is None else _integer(x,0) for x in arrangement)
            if arrangement[0] not in (0,1) or arrangement[1] is None:
                raise ValueError('arrangement index/region missing')
        object.__setattr__(self,'arrangement',arrangement)


@dataclass(frozen=True, slots=True)
class HistoryRecord:
    source: EntityKey
    status: str
    images: tuple
    generated: tuple

    def __post_init__(self):
        if type(self.source) is not EntityKey or self.status not in ('retained','modified','deleted'):
            raise ValueError('invalid source history record')
        for name in ('images','generated'):
            values = tuple(getattr(self,name))
            if any(type(x) is not EntityKey for x in values) or len(set(values))!=len(values):
                raise ValueError('invalid/duplicate history image')
            object.__setattr__(self,name,values)
        if (not self.images)!=(self.status=='deleted'):
            raise ValueError('deleted history iff no retained/modified image')


@dataclass(frozen=True, slots=True, init=False)
class History:
    records: tuple
    origins: tuple

    def __init__(self,*args,**kwargs):
        raise TypeError('History construction is factory-only')

    def from_input(self,key):
        for record in self.records:
            if record.source==key:
                return record
        raise KeyError(key)

    def origin(self,key):
        for origin in self.origins:
            if origin.target==key:
                return origin
        raise KeyError(key)


@dataclass(frozen=True, slots=True)
class Selection:
    side: str
    face_id: int
    kind: str
    region: object
    domain: object
    witness: tuple
    label: str
    keep: bool
    orientation: int

    def __post_init__(self):
        object.__setattr__(self,'face_id',_integer(self.face_id))
        object.__setattr__(self,'witness',_point(self.witness))
        if self.domain is not None:
            object.__setattr__(self,'domain',tuple(_rational(x) for x in self.domain))
        if self.side not in ('box','cutter') or type(self.keep) is not bool:
            raise ValueError('invalid selection side/decision')
        if type(self.orientation) is not int or self.orientation not in (-1,1):
            raise ValueError('invalid selection orientation')


@dataclass(frozen=True, slots=True)
class Measure:
    rational: F
    pi: F

    def __post_init__(self):
        for name in ('rational','pi'):
            object.__setattr__(self,name,_rational(getattr(self,name)))


@dataclass(frozen=True, slots=True, init=False)
class PointClassification:
    solid: object
    point: tuple
    label: str
    boundary_faces: tuple

    def __init__(self,*args,**kwargs):
        raise TypeError('PointClassification construction is factory-only')


def _circle(radius,t):
    q,s = (3,F(1)) if t==1 else (int(4*t),4*t-int(4*t))
    x,y = radius*(1-s*s)/(1+s*s),radius*2*s/(1+s*s)
    return ((x,y),(-y,x),(-x,-y),(y,-x))[q]


def _curve_value(curve,t):
    data = tuple(_rational(x) for x in curve.data)
    if curve.kind=='line':
        return tuple((1-t)*data[i]+t*data[i+3] for i in range(3))
    _check(curve.kind=='circle','unsupported output curve')
    return (*_circle(data[0],t),data[1])


@dataclass(frozen=True, slots=True, init=False)
class CutSolid:
    body_id: int
    revision: int
    frame: object
    vertices: tuple
    curves: tuple
    edges: tuple
    coedges: tuple
    loops: tuple
    faces: tuple
    charts: tuple
    patches: tuple
    radius: F
    wall_band: tuple
    volume: Measure

    def __init__(self,*args,**kwargs):
        raise TypeError('CutSolid construction is factory-only; use difference')

    def classify(self,patch_slot,u,v):
        slot = _integer(patch_slot,0,len(self.charts)-1)
        for value in (u,v):
            if type(value) not in (int,float,F):
                raise TypeError('trim coordinates require exact numeric values')
        if any(type(x) is float and not math.isfinite(x) for x in (u,v)):
            return INVALID
        u,v = _rational(u),_rational(v)
        if not 0<=u<=1 or not 0<=v<=1:
            return INVALID
        chart = self.charts[slot]
        face = self.faces[chart.face_slot]
        if face.kind=='cylinder':
            lo,hi = self.wall_band
            if not lo<=v<=hi:
                return REJECTED
            return BOUNDARY if (v in (lo,hi) or chart.quadrant==0 and u==0
                                or chart.quadrant==3 and u==1) else INTERIOR
        if face.kind=='rectangle_hole':
            sign = (u-F(1,2))**2+(v-F(1,2))**2-(self.radius/F(3,2))**2
            if sign<0:
                return REJECTED
            if sign==0:
                return BOUNDARY
        return BOUNDARY if u in (0,1) or v in (0,1) else INTERIOR

    def classify_point(self,point):
        point = _point(point)
        local = tuple((x-_rational(o))/_rational(self.frame.scale) for x,o in zip(point,self.frame.origin))
        x,y,z = local
        names = {face.name:face.id for face in self.faces}
        constraints = (('bottom',z+F(1,2)),('top',F(1,2)-z),
                       ('left',x+F(3,4)),('right',F(3,4)-x),
                       ('front',y+F(3,4)),('back',F(3,4)-y),
                       ('wall',x*x+y*y-self.radius*self.radius))
        if any(value<0 for _,value in constraints):
            label,faces = 'exterior',()
        else:
            faces = tuple(sorted(names[name] for name,value in constraints if value==0))
            label = 'boundary' if faces else 'interior'
        return _publish(PointClassification,solid=self,point=point,label=label,boundary_faces=faces)

    def edge_point(self,edge_id,t):
        edge = self.edges[_integer(edge_id,1,len(self.edges))-1]
        t = _rational(t)
        if not 0<=t<=1:
            raise ValueError('edge parameter outside [0,1]')
        local = _curve_value(self.curves[edge.curve_slot],t)
        return tuple(_rational(o)+_rational(self.frame.scale)*x for o,x in zip(self.frame.origin,local))

    def coedge_uv(self,coedge_id,t):
        c = self.coedges[_integer(coedge_id,1,len(self.coedges))-1]
        t = _rational(t)
        if not 0<=t<=1:
            raise ValueError('coedge parameter outside [0,1]')
        mapping = c.uv_map
        data = tuple(_rational(x) for x in mapping.data)
        if mapping.kind=='line':
            return tuple((1-t)*data[i]+t*data[i+2] for i in range(2))
        if mapping.kind=='circle_wall':
            return 4*t,data[0]
        _check(mapping.kind=='circle_plane','unsupported output UV map')
        x,y = _circle(data[0],t)
        return (x+data[1])/(2*data[1]),(y+data[1])/(2*data[1])

    def topology_input(self):
        from .adjacency import Source,TopologyInput
        def sources(records):
            return tuple(Source(0,self.body_id,self.revision,r.id) for r in records)
        return TopologyInput(sources(self.edges),sources(self.faces),sources(self.coedges),
                             tuple(c.edge_slot for c in self.coedges),
                             tuple(c.face_slot for c in self.coedges),
                             tuple(c.orientation for c in self.coedges),snapshots=())


@dataclass(frozen=True, slots=True, init=False)
class CutResult:
    left: BodySnapshot
    right: BodySnapshot
    solid: CutSolid
    sections: tuple
    arrangements: tuple
    selections: tuple
    history: History
    timings: tuple

    def __init__(self,*args,**kwargs):
        raise TypeError('CutResult construction is factory-only; use difference')


_TABLES = (('vertex','vertices'),('edge','edges'),('coedge','coedges'),
           ('loop','loops'),('face','faces'),('chart','charts'))
_FULL = (F(0),F(1))
_UV = (F(0),F(1),F(0),F(1))
_BAND = (F(1,4),F(3,4))


def _key(body,kind,entity_id):
    return EntityKey(body.body_id,body.revision,kind,entity_id)


def _keys(body):
    return tuple(_key(body,kind,r.id) for kind,table in _TABLES for r in getattr(body,table))


def _patch_world(patch,u,v):
    bu = tuple(F(math.comb(patch.degree_u,i))*u**i*(1-u)**(patch.degree_u-i)
               for i in range(patch.degree_u+1))
    bv = tuple(F(math.comb(patch.degree_v,j))*v**j*(1-v)**(patch.degree_v-j)
               for j in range(patch.degree_v+1))
    h = tuple(sum((a*b*_rational(patch.controls[4*(i*len(bv)+j)+axis])
                   for i,a in enumerate(bu) for j,b in enumerate(bv)),F(0)) for axis in range(4))
    _check(h[3]>0,'nonpositive source denominator')
    return tuple(_rational(o)+_rational(patch.scale)*h[k]/h[3] for k,o in enumerate(patch.origin))


def _fraction_scalar(value):
    _check(not value.b,'admitted Boolean event must be rational')
    return value.a


def _choose(left,right,sections,arrangements):
    selections=[];cells=[]
    def decision(side,face,kind,region,domain,witness,wanted,keep,orientation=1):
        opposite = right if side=='box' else left
        label = _source_classify(opposite,witness).label
        _check(label==wanted,'unexpected whole-region material selection')
        selections.append(Selection(side,face.id,kind,region,domain,witness,label,keep,orientation))
    for section,arr in zip(sections,arrangements):
        face = section.left.face
        kept=[]
        for region in arr.regions:
            if region.kind=='outside':
                continue
            witness = _patch_world(left.patches[face.chart_slots[0]],*region.witness_uv)
            keep = region.kind=='cell'
            decision('box',face,'cap_region',region.id,None,witness,'exterior' if keep else 'interior',keep)
            if keep:
                kept.append(region)
        _check(len(kept)==1 and len(kept[0].holes)==1,'cap needs one complete outside region with one hole')
        cells.append(kept[0])
    for face in left.faces:
        if face.name not in ('bottom','top'):
            witness = _patch_world(left.patches[face.chart_slots[0]],F(1,2),F(1,2))
            decision('box',face,'side',None,None,witness,'exterior',True)
    wall = next(f for f in right.faces if f.kind=='cylinder')
    wall_patch = right.patches[wall.chart_slots[0]]
    for index,(lo,hi) in enumerate(((F(0),_BAND[0]),_BAND,(_BAND[1],F(1)))):
        keep = index==1
        witness = _patch_world(wall_patch,F(1,2),(lo+hi)/2)
        decision('cutter',wall,'wall_band',None,(lo,hi),witness,
                 'interior' if keep else 'exterior',keep,-1 if keep else 1)
    for face in right.faces:
        if face is not wall:
            witness = _patch_world(right.patches[face.chart_slots[0]],F(1,2),F(1,2))
            decision('cutter',face,'cutter_cap',None,None,witness,'exterior',False)
    return tuple(selections),tuple(cells)


def _assemble(left,right,sections,arrangements,cells):
    body_id = _namespace()
    wall = next(f for f in right.faces if f.kind=='cylinder')
    vertices,curves,edges = list(left.vertices),list(left.curves),list(left.edges)
    coedges,loops,faces,charts,origins = [],[],[],[],[]
    def origin(kind,entity_id,relation,parents,**extra):
        origins.append(Origin(EntityKey(body_id,1,kind,entity_id),relation,tuple(parents),**extra))
    for kind,records in (('vertex',vertices),('edge',edges)):
        for record in records:
            origin(kind,record.id,'retained',(_key(left,kind,record.id),),
                   source_range=_FULL if kind=='edge' else None)
    capinfo = {}
    for index,(section,arr,cell) in enumerate(zip(sections,arrangements,cells)):
        hole = arr.loops[cell.holes[0]]
        _check(len(hole.uses)==1,'circle hole must be one physical use')
        rawuse = arr.uses[hole.uses[0]]
        ring = arr.edges[rawuse.edge]
        _check(ring.kind=='circle' and ring.start==ring.end,'section ring must be one closed physical edge')
        anchor = arr.vertices[ring.start]
        _check(anchor.source[0]=='section','ring anchor lost section incidence')
        event_id = anchor.source[2]
        event = next(e for e in section.events if e.id==event_id)
        _check(event.joins==('right:atlas:0/4',),'ring anchor must be original wall seam join')
        point = tuple((x-_rational(o))/_rational(left.frame.scale) for x,o in zip(anchor.position,left.frame.origin))
        radius,z = point[0],point[2]
        _check(point[1]==0 and radius>0,'section anchor is not admitted radial origin')
        vid = len(vertices);vertices.append(Vertex(vid+1,point))
        eid = len(edges);curves.append(Curve('circle',(radius,z)))
        edges.append(Edge(eid+1,vid,vid,len(curves)-1))
        component = section.components[0]
        common = dict(section=index,piece_ids=tuple(component.piece_ids),
                      event_ids=tuple(e.id for e in section.events),
                      arrangement=(index,cell.id,hole.id,None))
        parents = (_key(left,'face',section.left.face_id),_key(right,'face',wall.id))
        origin('edge',eid+1,'generated',parents,**common)
        seam_source = next(e for e in right.edges if e.start!=e.end)
        anchor_common = dict(common,event_ids=(event_id,),piece_ids=tuple(i for i,_ in event.incidences))
        origin('vertex',vid+1,'generated',parents+(_key(right,'edge',seam_source.id),),**anchor_common)
        capinfo[section.left.face_id] = (index,arr,cell,hole,rawuse,eid,common,parents)
    _check(vertices[8].position[:2]==vertices[9].position[:2],'rim anchors do not share cutter seam')
    seam_slot = len(edges);curves.append(Curve('line',vertices[8].position+vertices[9].position))
    edges.append(Edge(seam_slot+1,8,9,len(curves)-1))
    seam_source = next(e for e in right.edges if e.start!=e.end)
    origin('edge',seam_slot+1,'modified',(_key(right,'edge',seam_source.id),),source_range=_BAND)
    # A loop builder publishes only immutable records after its complete word.
    def loop(face_slot,hole,specs):
        ls = len(loops);word=[]
        for edge_slot,sign,mapping in specs:
            cs = len(coedges);word.append(cs)
            coedges.append(Coedge(cs+1,edge_slot,face_slot,ls,sign,mapping))
        loops.append(Loop(ls+1,face_slot,hole,tuple(word)))
        return ls
    for source_face in left.faces:
        fs = len(faces);fl=[];fc=[]
        info = capinfo.get(source_face.id)
        for source_ls in source_face.loop_slots:
            oldloop = left.loops[source_ls]
            specs = tuple((left.coedges[cs].edge_slot,left.coedges[cs].orientation,left.coedges[cs].uv_map)
                          for cs in oldloop.coedge_slots)
            ls = loop(fs,False,specs);fl.append(ls)
            arrref=() if info is None else (info[0],info[2].id,info[2].outer_loop,None)
            origin('loop',ls+1,'retained',(_key(left,'loop',oldloop.id),),source_uv=_UV,arrangement=arrref)
            for oldcs,newcs in zip(oldloop.coedge_slots,loops[ls].coedge_slots):
                oldc = left.coedges[oldcs];arruse=None
                if info is not None:
                    _,arr,cell,*_ = info
                    rawword = arr.loops[cell.outer_loop].uses
                    matched = tuple(arr.uses[uid] for uid in rawword
                                    if arr.edges[arr.uses[uid].edge].source[:2]==('boundary',oldc.id))
                    _check(len(matched)==1,'cap outer arrangement lost an original use')
                    raw = matched[0];edge = arr.edges[raw.edge]
                    t0,t1 = edge.source[3:]
                    _check({t0,t1}=={F(0),F(1)} and
                           raw.orientation*(1 if t1>t0 else -1)*source_face.orientation==oldc.orientation,
                           'cap arrangement outer boundary is not the complete oriented original')
                    arruse=raw.id
                origin('coedge',newcs+1,'retained',(_key(left,'coedge',oldc.id),),source_range=_FULL,
                       arrangement=() if info is None else (info[0],info[2].id,info[2].outer_loop,arruse))
        if info is not None:
            ai,arr,cell,hole,rawuse,eid,common,parents = info
            # Raw arrangement words are UV-oriented; face sense supplies reversal.
            sign = rawuse.orientation*source_face.orientation
            ls = loop(fs,True,((eid,sign,UVMap('circle_plane',(curves[eid].data[0],F(3,4)))),));fl.append(ls)
            origin('loop',ls+1,'generated',parents,orientation=source_face.orientation,**common)
            origin('coedge',loops[ls].coedge_slots[0]+1,'generated',parents,
                   orientation=source_face.orientation,
                   **dict(common,arrangement=(ai,cell.id,hole.id,rawuse.id)))
        for oldslot in source_face.chart_slots:
            oldchart = left.charts[oldslot];cs=len(charts)
            p=replace(oldchart.patch,lineage=Lineage(body_id,5,fs+1,1,cs+1))
            charts.append(Chart(cs+1,fs,oldchart.quadrant,p));fc.append(cs)
            origin('chart',cs+1,'modified' if info else 'retained',(_key(left,'chart',oldchart.id),),
                   source_uv=_UV,arrangement=() if info is None else (info[0],info[2].id,None,None))
        faces.append(Face(fs+1,source_face.name,'rectangle_hole' if info else source_face.kind,
                          source_face.orientation,tuple(fl),tuple(fc)))
        origin('face',fs+1,'modified' if info else 'retained',(_key(left,'face',source_face.id),),
               source_uv=_UV,arrangement=() if info is None else (info[0],info[2].id,None,None))
    wall_fs=len(faces);wall_charts=[]
    for oldslot in wall.chart_slots:
        oldchart=right.charts[oldslot];cs=len(charts)
        p=replace(oldchart.patch,lineage=Lineage(body_id,5,wall_fs+1,1,cs+1))
        charts.append(Chart(cs+1,wall_fs,oldchart.quadrant,p));wall_charts.append(cs)
        origin('chart',cs+1,'modified',(_key(right,'chart',oldchart.id),),
               source_uv=(F(0),F(1),*_BAND),orientation=-1)
    lower,upper=(capinfo[s.left.face_id] for s in sections)
    wall_ls=loop(wall_fs,False,((seam_slot,1,UVMap('line',(F(0),_BAND[0],F(0),_BAND[1]))),
                 (upper[5],1,UVMap('circle_wall',(_BAND[1],))),
                 (seam_slot,-1,UVMap('line',(F(4),_BAND[0],F(4),_BAND[1]))),
                 (lower[5],-1,UVMap('circle_wall',(_BAND[0],)))))
    original_wall_loop=right.loops[wall.loop_slots[0]]
    for index,cs in enumerate(loops[wall_ls].coedge_slots):
        if index in (0,2):
            atlas=0 if index==0 else 4
            oldc=next(right.coedges[slot] for slot in original_wall_loop.coedge_slots
                      if right.coedges[slot].uv_map.kind=='seam_wall' and right.coedges[slot].uv_map.data==(atlas,))
            origin('coedge',cs+1,'modified',(_key(right,'coedge',oldc.id),),
                   source_range=_BAND,orientation=-1)
        else:
            info=upper if index==1 else lower
            ai,arr,cell,hole,rawuse,eid,common,parents=info
            cap=sections[ai].left.face
            origin('coedge',cs+1,'generated',parents,orientation=-cap.orientation,
                   **dict(common,arrangement=(ai,cell.id,hole.id,rawuse.id)))
    wall_uv=(F(0),F(4),*_BAND)
    origin('loop',wall_ls+1,'modified',(_key(right,'loop',original_wall_loop.id),),source_uv=wall_uv,orientation=-1)
    faces.append(Face(wall_fs+1,'wall','cylinder',-wall.orientation,(wall_ls,),tuple(wall_charts)))
    origin('face',wall_fs+1,'modified',(_key(right,'face',wall.id),),source_uv=wall_uv,orientation=-1)
    # Volume consumes the selected exact cap area and actual world spans/height.
    cap=sections[0].left.face;p=left.patches[cap.chart_slots[0]]
    p00,p10,p01=(_patch_world(p,*uv) for uv in ((F(0),F(0)),(F(1),F(0)),(F(0),F(1))))
    jacobian=abs((p10[0]-p00[0])*(p01[1]-p00[1])-(p10[1]-p00[1])*(p01[0]-p00[0]))
    height=arrangements[1].vertices[-1].position[2]-arrangements[0].vertices[-1].position[2]
    _check(cells[0].area==cells[1].area and height>0,'selected cap measures/heights disagree')
    volume=Measure(cells[0].area.rational*jacobian*height,cells[0].area.pi*jacobian*height)
    solid=_publish(CutSolid,body_id=body_id,revision=1,frame=left.frame,
                   vertices=tuple(vertices),curves=tuple(curves),edges=tuple(edges),
                   coedges=tuple(coedges),loops=tuple(loops),faces=tuple(faces),charts=tuple(charts),
                   patches=tuple(c.patch for c in charts),radius=_rational(curves[12].data[0]),
                   wall_band=_BAND,volume=volume)
    origins=tuple(origins);records=[]
    for source in _keys(left)+_keys(right):
        image_origins=tuple(o for o in origins if o.relation!='generated' and source in o.parents)
        images=tuple(o.target for o in image_origins)
        _check(len(image_origins)<=1,'admitted source has more than one retained image')
        generated=tuple(o.target for o in origins if o.relation=='generated' and source in o.parents)
        records.append(HistoryRecord(source,image_origins[0].relation if images else 'deleted',images,generated))
    history=_publish(History,records=tuple(records),origins=origins)
    return solid,history


def _circle_world_map(solid,curve,q):
    r,z=map(_rational,curve.data);o=tuple(map(_rational,solid.frame.origin));scale=_rational(solid.frame.scale)
    x,y=(1,0,-1),(0,2,0)
    x,y=((x,y),(tuple(-v for v in y),x),(tuple(-v for v in x),tuple(-v for v in y)),(y,tuple(-v for v in x)))[q]
    d=(1,0,1)
    return ExactMap((tuple(o[0]*d[i]+scale*r*x[i] for i in range(3)),
                     tuple(o[1]*d[i]+scale*r*y[i] for i in range(3)),
                     tuple((o[2]+scale*z)*v for v in d)),d)


def _validate_candidate(solid,history,left,right,sections,arrangements,selections):
    """Last private boundary before publication; no production failure flags."""
    _check(tuple(len(getattr(solid,n)) for _,n in _TABLES)==(10,15,30,9,7,10),'incomplete candidate entity tables')
    for kind,table in _TABLES:
        _check(all(type(r.id) is int and r.id==i+1 for i,r in enumerate(getattr(solid,table))),kind+' IDs not dense')
    seen_coedges=[];seen_loops=[];seen_charts=[];edge_uses=defaultdict(list);links=defaultdict(lambda:defaultdict(list))
    for fs,face in enumerate(solid.faces):
        for cs in face.chart_slots:
            _check(0<=cs<len(solid.charts) and solid.charts[cs].face_slot==fs,'chart ownership differs')
            seen_charts.append(cs)
        for ls in face.loop_slots:
            _check(0<=ls<len(solid.loops),'loop slot outside candidate')
            loop=solid.loops[ls];seen_loops.append(ls)
            _check(loop.face_slot==fs and loop.coedge_slots,'loop owner/word differs')
            for prevslot,slot in zip(loop.coedge_slots[-1:]+loop.coedge_slots[:-1],loop.coedge_slots):
                _check(0<=slot<len(solid.coedges) and 0<=prevslot<len(solid.coedges),'coedge slot outside candidate')
                c,prev=solid.coedges[slot],solid.coedges[prevslot]
                _check(c.face_slot==fs and c.loop_slot==ls and c.orientation in (-1,1),'coedge owner/sense differs')
                _check(0<=c.edge_slot<len(solid.edges),'edge slot outside candidate')
                e,pe=solid.edges[c.edge_slot],solid.edges[prev.edge_slot]
                _check(0<=e.start<len(solid.vertices) and 0<=e.end<len(solid.vertices)
                       and 0<=e.curve_slot<len(solid.curves),'edge endpoint/curve outside candidate')
                start=e.start if c.orientation==1 else e.end
                previous_end=pe.end if prev.orientation==1 else pe.start
                _check(start==previous_end,'oriented loop does not close by shared identity')
                a=(prev.edge_slot,1 if prev.orientation==1 else 0)
                b=(c.edge_slot,0 if c.orientation==1 else 1)
                links[start][a].append(b);links[start][b].append(a)
                edge_uses[c.edge_slot].append(c);seen_coedges.append(slot)
    for seen,count in ((seen_coedges,30),(seen_loops,9),(seen_charts,10)):
        _check(sorted(seen)==list(range(count)),'incomplete/duplicate candidate ownership')
    neighbors=defaultdict(set)
    for es,e in enumerate(solid.edges):
        uses=edge_uses[es]
        _check(len(uses)==2 and sum(c.orientation for c in uses)==0,'edge needs two opposed occurrences')
        a,b=(c.face_slot for c in uses);neighbors[a].add(b);neighbors[b].add(a)
        for t,vs in ((F(0),e.start),(F(1),e.end)):
            expected=tuple(_rational(o)+_rational(solid.frame.scale)*_rational(x)
                           for o,x in zip(solid.frame.origin,solid.vertices[vs].position))
            _check(solid.edge_point(e.id,t)==expected,'curve endpoint differs from shared vertex')
    visited=set();todo=[0]
    while todo:
        face=todo.pop()
        if face not in visited:
            visited.add(face);todo.extend(neighbors[face]-visited)
    _check(visited==set(range(7)),'shell is disconnected')
    _check(set(links)==set(range(10)),'vertex missing an incident link')
    for graph in links.values():
        _check(all(len(v)==2 for v in graph.values()),'vertex link degree differs from two')
        seen=set();todo=[next(iter(graph))]
        while todo:
            germ=todo.pop()
            if germ not in seen:
                seen.add(germ);todo.extend(graph[germ])
        _check(seen==set(graph),'vertex link has multiple cycles')
    _check(sum(loop.hole for loop in solid.loops)==2 and 10-15+7-2==0,'annular-domain Euler differs')
    _check(solid.radius==F(1,2) and solid.wall_band==_BAND,'actual result trim differs from admitted construction')
    original=left.patches+tuple(right.patches[i] for i in right.faces[2].chart_slots)
    for i,(patch,old) in enumerate(zip(solid.patches,original)):
        _check((patch.controls,patch.uv,patch.origin,patch.scale)==(old.controls,old.uv,old.origin,old.scale),
               'source carrier coefficients/frame were changed')
        _check(patch.lineage==Lineage(solid.body_id,5,solid.charts[i].face_slot+1,1,i+1),'new carrier lineage differs')
    for index,section in enumerate(sections):
        arr=arrangements[index]
        _check(section.left.snapshot is left and section.right.snapshot is right and len(arr.sections)==1 and arr.sections[0] is section,
               'actual section operands were replaced')
        _check(len(section.components)==1 and len(section.pieces)==4 and len(section.events)==4,
               'incomplete section')
        for q,pid in enumerate(section.components[0].piece_ids):
            _check(section.pieces[pid].spatial_map==_circle_world_map(solid,solid.curves[12+index],q),
                   'new physical rim differs over a complete source quarter')
    # Verify exact output trim/map records, not just their sampled endpoints.
    for c in solid.coedges:
        face=solid.faces[c.face_slot];curve=solid.curves[solid.edges[c.edge_slot].curve_slot]
        if c.edge_slot<12:
            old=history.origin(_key(solid,'coedge',c.id)).parents[0]
            oldc=left.coedges[old.entity_id-1]
            _check(c.uv_map==oldc.uv_map,'retained original UV map changed')
        elif c.edge_slot in (12,13):
            index=c.edge_slot-12
            expected=UVMap('circle_wall',(_BAND[index],)) if face.kind=='cylinder' else UVMap('circle_plane',(solid.radius,F(3,4)))
            _check(c.uv_map==expected,'new circle incident UV map differs')
        else:
            _check(face.kind=='cylinder' and c.uv_map.kind=='line' and
                   c.uv_map.data in ((F(0),_BAND[0],F(0),_BAND[1]),(F(4),_BAND[0],F(4),_BAND[1])),
                   'new seam source UV map differs')
            _check(curve.kind=='line' and curve.data==solid.vertices[8].position+solid.vertices[9].position,
                   'new seam geometry differs')
    source_keys=set(_keys(left)+_keys(right));target_keys=set(_keys(solid))
    _check(len(history.records)==85 and {r.source for r in history.records}==source_keys,'incomplete source dispositions')
    _check(len(history.origins)==81 and {o.target for o in history.origins}==target_keys,'incomplete output origins')
    for origin in history.origins:
        _check(set(origin.parents)<=source_keys,'foreign source parent')
        for parent in origin.parents:
            record=history.from_input(parent)
            _check(origin.target in (record.generated if origin.relation=='generated' else record.images),
                   'history forward/reverse differ')
            if origin.relation!='generated':
                _check(parent.kind==origin.target.kind and record.status==origin.relation,'same-kind history relation differs')
    _check(Counter(r.status for r in history.records)=={'retained':58,'modified':13,'deleted':14},'history dispositions differ')
    _check(sum(o.relation=='generated' for o in history.origins)==10,'generated origin coverage differs')
    _check(len(selections)==13 and sum(s.keep for s in selections)==7,'incomplete whole-region decisions')


def difference(box,cutter):
    started=time.perf_counter_ns();previous=started;timings=[]
    def phase(name):
        nonlocal previous
        now=time.perf_counter_ns();timings.append((name,(now-previous)/1e6));previous=now
    if type(box) is not BodySnapshot or type(cutter) is not BodySnapshot:
        raise TypeError('difference requires exact owned BodySnapshot operands')
    if (box.shape!='box' or cutter.shape!='cylinder'
            or tuple(map(_rational,box.frame.origin))!=tuple(map(_rational,cutter.frame.origin))
            or _rational(cutter.frame.scale)!=2*_rational(box.frame.scale)
            or box.frame.scale not in (.25,.5,1.,2.)):
        raise UnsupportedBoolean('requires same-origin canonical box and twice-scale through cylinder')
    caps=tuple(next(face for face in box.faces if face.name==name) for name in ('bottom','top'))
    wall=next(face for face in cutter.faces if face.kind=='cylinder')
    phase('admission_ms')
    sections=tuple(intersect_faces(FaceRef(box,cap.id),FaceRef(cutter,wall.id)) for cap in caps)
    phase('sections_ms')
    arrangements=tuple(arrange_face(FaceRef(box,cap.id),(section,)) for cap,section in zip(caps,sections))
    phase('arrangements_ms')
    selections,cells=_choose(box,cutter,sections,arrangements)
    phase('selection_ms')
    solid,history=_assemble(box,cutter,sections,arrangements,cells)
    phase('assembly_history_ms')
    _validate_candidate(solid,history,box,cutter,sections,arrangements,selections)
    phase('validation_ms')
    timings.append(('total_ms',(time.perf_counter_ns()-started)/1e6))
    return _publish(CutResult,left=box,right=cutter,solid=solid,sections=sections,
                    arrangements=arrangements,selections=selections,history=history,timings=tuple(timings))
