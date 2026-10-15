"""Independent complete material, boundary, history and consumer truth.

Expected geometry comes from original primitive equations and rational maps.
No Boolean construction, trim, selection or private validation helper is used.
"""
from collections import Counter,defaultdict
from fractions import Fraction as F
import math
import struct

from .body_oracles import (poly,add,mul,factor,rational,radd,rscale,same,value,
                           quarter_rational)

POSITION_LIMIT=F(1,1<<36)
NORMAL_LIMIT=F(1,1<<30)
_KINDS=('vertex','edge','coedge','loop','face','chart')
_TABLES=dict(zip(_KINDS,('vertices','edges','coedges','loops','faces','charts')))
_NAMES=('top','bottom','front','back','right','left','wall')
_NORMALS=((0,0,1),(0,0,-1),(0,-1,0),(0,1,0),(1,0,0),(-1,0,0))


def _need(ok,message):
    if not ok:raise AssertionError(message)


def _f(x):
    if type(x) is F:return x
    if type(x) is int:return F(x)
    if type(x) is float and math.isfinite(x):return F.from_float(x)
    raise ValueError('expected finite int/Fraction/float')


def _bits(x):return struct.pack('<d',x)


def _exact_tuple(x,n,label):
    _need(type(x) is tuple and len(x)==n and all(type(v) is F for v in x),label+' exact tuple')


def _key(key):
    values=(key.body_id,key.revision,key.kind,key.entity_id)
    _need(all(type(values[i]) is int and 0<values[i]<1<<64 for i in (0,1,3)) and
          type(values[2]) is str and values[2] in _KINDS,'typed entity key')
    return values


def _k(body,kind,id):return (body.body_id,body.revision,kind,id)


def _inputs(result):
    from .body import BodySnapshot
    a,b=result.left,result.right
    _need(type(a) is BodySnapshot and type(b) is BodySnapshot and a.shape=='box' and b.shape=='cylinder','actual input types/roles')
    _need(tuple(map(_f,a.frame.origin))==tuple(map(_f,b.frame.origin)) and _f(b.frame.scale)==2*_f(a.frame.scale)
          and _f(a.frame.scale) in (F(1,4),F(1,2),F(1),F(2)),'actual admitted original frames')
    _need(a.body_id!=b.body_id and a.revision==b.revision==1,'distinct source namespaces')
    return a,b


def _rotate(x,y,q):return ((x,y),(-y,x),(-x,-y),(y,-x))[q]


def _source_geometry(result,slot,u,v):
    u,v=_f(u),_f(v)
    if slot<6:
        x,y,z=F(3,2)*u-F(3,4),F(3,2)*v-F(3,4),v-F(1,2)
        point=((x,y,F(1,2)),(x,y,-F(1,2)),(x,-F(3,4),z),(x,F(3,4),z),
               (F(3,4),x,z),(-F(3,4),x,z))[slot]
        frame=result.left.frame;normal=tuple(map(F,_NORMALS[slot]))
    else:
        x,y=_rotate((1-u*u)/(1+u*u),2*u/(1+u*u),slot-6)
        point=(x/4,y/4,v-F(1,2));normal=(-x,-y,F(0));frame=result.right.frame
    return tuple(_f(o)+_f(frame.scale)*p for o,p in zip(frame.origin,point)),normal


def _trim(slot,u,v):
    if type(u) is float and not math.isfinite(u) or type(v) is float and not math.isfinite(v):return 3
    u,v=_f(u),_f(v)
    if not 0<=u<=1 or not 0<=v<=1:return 3
    if slot<2:
        delta=(u-F(1,2))**2+(v-F(1,2))**2-F(1,9)
        if delta<0:return 0
        return 2 if delta==0 or u in (0,1) or v in (0,1) else 1
    if slot<6:return 2 if u in (0,1) or v in (0,1) else 1
    if not F(1,4)<=v<=F(3,4):return 0
    return 2 if v in (F(1,4),F(3,4)) or slot==6 and u==0 or slot==9 and u==1 else 1


def _material(result,point):
    a=result.left;x,y,z=((_f(p)-_f(o))/_f(a.frame.scale) for p,o in zip(point,a.frame.origin))
    constraints=((1,F(1,2)-z),(2,z+F(1,2)),(3,y+F(3,4)),(4,F(3,4)-y),
                 (5,F(3,4)-x),(6,x+F(3,4)),(7,x*x+y*y-F(1,4)))
    if any(v<0 for _,v in constraints):return 'exterior',()
    faces=tuple(id for id,v in constraints if not v)
    return ('boundary' if faces else 'interior'),faces


def _curve(curve,q):
    data=tuple(map(_f,curve.data))
    if curve.kind=='line':
        _need(len(data)==6,'line data extent')
        return tuple(rational((data[k],data[k+3]-data[k])) for k in range(3))
    _need(curve.kind=='circle' and len(data)==2,'circle data extent')
    x,y=quarter_rational(q)
    return rscale(x,data[0]),rscale(y,data[0]),rational((data[1],))


def _uv(mapping,q):
    d=tuple(map(_f,mapping.data))
    if mapping.kind=='line':
        _need(len(d)==4,'UV line extent')
        return tuple(rational((d[k],d[k+2]-d[k])) for k in range(2))
    if mapping.kind=='circle_plane':
        _need(len(d)==2 and d==(F(1,2),F(3,4)),'actual cap UV circle')
        return tuple(radd(rscale(c,d[0]/(2*d[1])),rational((F(1,2),))) for c in quarter_rational(q))
    _need(mapping.kind=='circle_wall' and len(d)==1 and d[0] in (F(1,4),F(3,4)),'actual wall circle UV')
    return rational((q,1)),rational((d[0],))


def _source_map(result,face_slot,uv,q):
    u,v=uv
    if face_slot<6:
        x=radd(rscale(u,F(3,2)),rational((-F(3,4),)))
        y=radd(rscale(v,F(3,2)),rational((-F(3,4),)))
        z=radd(v,rational((-F(1,2),)));c=lambda x:rational((x,))
        xyz=((x,y,c(F(1,2))),(x,y,c(-F(1,2))),(x,c(-F(3,4)),z),(x,c(F(3,4)),z),
             (c(F(3,4)),x,z),(c(-F(3,4)),x,z))[face_slot]
        frame=result.left.frame
    else:
        s=radd(u,rational((-q,)));ss=mul(s[0],s[0]);dd=mul(s[1],s[1])
        x=(add(dd,factor(ss,-1)),add(dd,ss));y=(factor(mul(s[0],s[1]),2),add(dd,ss))
        if q==1:x,y=rscale(y,-1),x
        elif q==2:x,y=rscale(x,-1),rscale(y,-1)
        elif q==3:x,y=y,rscale(x,-1)
        xyz=rscale(x,F(1,4)),rscale(y,F(1,4)),radd(v,rational((-F(1,2),)))
        frame=result.right.frame
    return tuple(radd(rscale(c,_f(frame.scale)),rational((_f(o),))) for c,o in zip(xyz,frame.origin))


def _endpoint(coedge,end):
    circle=coedge.uv_map.kind.startswith('circle_')
    return tuple(value(c,F(end) if not circle else F(end)) for c in _uv(coedge.uv_map,3 if end and circle else 0))


def _patches(result):
    solid=result.solid
    _need(len(solid.charts)==len(solid.patches)==10,'complete carrier chart cover')
    original=result.left.patches+tuple(result.right.patches[c] for c in result.right.faces[2].chart_slots)
    for slot,(patch,source,chart) in enumerate(zip(solid.patches,original,solid.charts)):
        _need(all(type(v) is int for v in (chart.id,chart.face_slot,chart.quadrant)) and chart.patch is patch and chart.id==slot+1 and chart.face_slot==(slot if slot<6 else 6)
              and chart.quadrant==(-1 if slot<6 else slot-6),'output carrier chart identity')
        _need((patch.degree_u,patch.degree_v)==(source.degree_u,source.degree_v),'source degrees')
        for name in ('controls','uv','origin'):
            actual,expected=getattr(patch,name),getattr(source,name)
            _need(type(actual) is tuple and len(actual)==len(expected) and all(_bits(a)==_bits(b) for a,b in zip(actual,expected)),'unchanged source carrier '+name)
        _need(_bits(patch.scale)==_bits(source.scale),'each actual source Patch frame')
        lin=patch.lineage
        _need(all(type(v) is int for v in (lin.body_id,lin.revision,lin.entity_kind,lin.entity_id,lin.chart_id)),'typed native output lineage')
        _need((lin.body_id,lin.revision,lin.entity_kind,lin.entity_id,lin.chart_id)==
              (solid.body_id,1,5,slot+1 if slot<6 else 7,slot+1),'fresh output native lineage')


def _sections(result):
    from .arrangement_oracles import qualify_arrangement
    _need(type(result.sections) is tuple and len(result.sections)==2 and type(result.arrangements) is tuple and len(result.arrangements)==2,'complete actual sections/arrangements')
    info=[]
    for index,(section,arr) in enumerate(zip(result.sections,result.arrangements)):
        capid=(2,1)[index]
        _need(section.left.snapshot is result.left and section.left.face_id==capid and section.right.snapshot is result.right and section.right.face_id==3,'original two-sided section identity')
        _need(arr.face.snapshot is result.left and arr.face.face_id==capid and len(arr.sections)==1 and arr.sections[0] is section,'actual retained source arrangement/section')
        _need(arr.u_cuts==arr.v_cuts==(),'unclipped cap arrangement')
        qualify_arrangement(arr) # Existing independent source-map/arrangement mechanism, once.
        cells=[r for r in arr.regions if r.kind=='cell'];disks=[r for r in arr.regions if r.kind=='disk']
        _need(len(cells)==len(disks)==1 and len(arr.regions)==3,'complete annulus/disk/exterior')
        cell,disk=cells[0],disks[0]
        _need(len(cell.holes)==1 and cell.area.rational==1 and cell.area.pi==-F(1,9) and disk.area.rational==0 and disk.area.pi==F(1,9),'complete cap UV annulus')
        hole=arr.loops[cell.holes[0]];outer=arr.loops[cell.outer_loop]
        _need(len(hole.uses)==1,'whole-circle hole use')
        use=arr.uses[hole.uses[0]];edge=arr.edges[use.edge]
        _need(edge.kind=='circle' and edge.start==edge.end,'whole physical source circle')
        anchors=[e for e in section.events if e.joins==('right:atlas:0/4',)]
        _need(len(anchors)==1,'actual named source seam event')
        anchor=anchors[0]
        _need(arr.vertices[edge.start].source==('section',0,anchor.id),'arrangement anchor source event')
        info.append(dict(section=index,capid=capid,arr=arr,cell=cell,disk=disk,hole=hole,outer=outer,use=use,
                         pids=section.components[0].piece_ids,eids=tuple(e.id for e in section.events),anchor=anchor))
    return tuple(info)


def _graph(result):
    solid=result.solid
    for kind,size in zip(_KINDS,(10,15,30,9,7,10)):
        records=getattr(solid,_TABLES[kind])
        _need(type(records) is tuple and len(records)==size and all(type(x.id) is int for x in records)
              and tuple(x.id for x in records)==tuple(range(1,size+1)),'complete dense '+kind+' records')
    _need(type(solid.curves) is tuple and len(solid.curves)==15,'complete spatial curves')
    _need(solid.frame==result.left.frame and solid.body_id not in (result.left.body_id,result.right.body_id)
          and type(solid.body_id) is int and 0<solid.body_id<1<<64 and type(solid.revision) is int and solid.revision==1,'new result/topology frame identity')
    _need(solid.radius==F(1,2) and type(solid.radius) is F and solid.wall_band==(F(1,4),F(3,4)),'exact output hole/band')
    for i,vertex in enumerate(solid.vertices):
        expected=result.left.vertices[i].position if i<8 else (F(1,2),F(0),F(-1,2) if i==8 else F(1,2))
        _need(type(vertex.position) is tuple and len(vertex.position)==3 and tuple(map(_f,vertex.position))==tuple(map(_f,expected)),'source/section vertex positions')
        if i<8:_need(all(type(x) is float and _bits(x)==_bits(y) for x,y in zip(vertex.position,expected)),'retained original vertex bits/types')
        else:_exact_tuple(vertex.position,3,'generated exact section vertex')
    for i,edge in enumerate(solid.edges):
        _need(all(type(x) is int for x in (edge.start,edge.end,edge.curve_slot)) and 0<=edge.curve_slot<15,'typed edge slots')
        if i<12:
            old=result.left.edges[i]
            _need((edge.start,edge.end)==(old.start,old.end) and solid.curves[edge.curve_slot]==result.left.curves[old.curve_slot],'unchanged box edge')
            curve=solid.curves[edge.curve_slot];original=result.left.curves[old.curve_slot]
            _need(type(curve.data) is tuple and all(type(x) is float and _bits(x)==_bits(y) for x,y in zip(curve.data,original.data)),'retained spatial curve bits/types')
        else:
            expected=((8,8),(9,9),(8,9))[i-12]
            _need((edge.start,edge.end)==expected,'shared generated edge anchors')
            curve=solid.curves[edge.curve_slot]
            expected_kind='line' if i==14 else 'circle'
            expected_data=(F(1,2),F(0),-F(1,2),F(1,2),F(0),F(1,2)) if i==14 else (F(1,2),-F(1,2) if i==12 else F(1,2))
            _need(curve.kind==expected_kind and tuple(map(_f,curve.data))==expected_data,'exact section/spatial curve data')
    _need(Counter(e.curve_slot for e in solid.edges)==Counter(range(15)),'complete owned curve references')
    role={};owners=[];allloops=[];allcharts=[];incidence=defaultdict(list);links=defaultdict(list)
    for fs,face in enumerate(solid.faces):
        _need(face.name==_NAMES[fs] and face.kind==('rectangle_hole' if fs<2 else 'rectangle' if fs<6 else 'cylinder')
              and type(face.orientation) is int and face.orientation==(-1 if fs in (1,3,5,6) else 1),'complete oriented face roles')
        _need(type(face.loop_slots) is tuple and type(face.chart_slots) is tuple,'owned face references')
        _need(tuple(face.chart_slots)==((fs,) if fs<6 else (6,7,8,9)),'complete face charts')
        _need(all(type(i) is int and 0<=i<9 for i in face.loop_slots) and all(type(i) is int and 0<=i<10 for i in face.chart_slots),'typed face loop/chart slots')
        allcharts.extend(face.chart_slots);allloops.extend(face.loop_slots)
        loops=[solid.loops[l] for l in face.loop_slots]
        _need(len(loops)==(2 if fs<2 else 1) and sum(l.hole for l in loops)==(1 if fs<2 else 0),'complete outer/hole domains')
        for loop in loops:
            _need(type(loop.face_slot) is int and loop.face_slot==fs and type(loop.hole) is bool and type(loop.coedge_slots) is tuple,'loop ownership/types')
            _need(all(type(i) is int and 0<=i<30 for i in loop.coedge_slots),'typed loop coedge slots')
            uses=[solid.coedges[c] for c in loop.coedge_slots]
            _need(len(uses)==(1 if loop.hole else 4),'complete boundary word size')
            owners.extend(loop.coedge_slots)
            for use in uses:
                _need(all(type(x) is int for x in (use.edge_slot,use.face_slot,use.loop_slot,use.orientation))
                      and 0<=use.edge_slot<15 and use.face_slot==fs and use.loop_slot==loop.id-1 and use.orientation in (-1,1),'typed coedge ownership')
                incidence[use.edge_slot].append(use)
            if fs<6 and not loop.hole:
                oldface=result.left.faces[fs];oldloop=result.left.loops[oldface.loop_slots[0]]
                olduses=[result.left.coedges[c] for c in oldloop.coedge_slots]
                for use,old in zip(uses,olduses):
                    _need((use.edge_slot,use.orientation,use.uv_map)==(old.edge_slot,old.orientation,old.uv_map),'whole original outer word')
                    role[('coedge',use.id)]=('box',old.id)
                role[('loop',loop.id)]=('box',oldloop.id)
            elif loop.hole:
                _need(uses[0].edge_slot==(13 if fs==0 else 12) and uses[0].orientation==-face.orientation
                      and uses[0].uv_map.kind=='circle_plane' and tuple(map(_f,uses[0].uv_map.data))==(F(1,2),F(3,4)),'complete oriented cap hole word')
                role[('loop',loop.id)]=('hole',fs);role[('coedge',uses[0].id)]=('caprim',fs)
            else:
                expected=((14,1,'line',(0,F(1,4),0,F(3,4))),
                          (13,1,'circle_wall',(F(3,4),)),(14,-1,'line',(4,F(1,4),4,F(3,4))),
                          (12,-1,'circle_wall',(F(1,4),)))
                for use,(edge,sign,kind,data) in zip(uses,expected):
                    _need((use.edge_slot,use.orientation,use.uv_map.kind)==(edge,sign,kind)
                          and tuple(map(_f,use.uv_map.data))==data,'complete reversed/cropped wall word')
                    role[('coedge',use.id)]=('seam',0 if data[0]==0 else 4) if edge==14 else ('wallrim',0 if edge==13 else 1)
                role[('loop',loop.id)]=('wall',0)
            for previous,use in zip(uses[-1:]+uses[:-1],uses):
                pe=solid.edges[previous.edge_slot];ce=solid.edges[use.edge_slot]
                pend=1 if previous.orientation==1 else 0;cstart=0 if use.orientation==1 else 1
                pv=(pe.start,pe.end)[pend];cv=(ce.start,ce.end)[cstart]
                _need(pv==cv,'oriented spatial loop closure')
                links[cv].append(((previous.edge_slot,pend),(use.edge_slot,cstart)))
            ends=[_endpoint(c,1 if c.orientation==1 else 0) for c in uses]
            starts=[_endpoint(c,0 if c.orientation==1 else 1) for c in uses]
            _need(all(a==b for a,b in zip(ends,starts[1:]+starts[:1])),'oriented original-UV loop closure')
    _need(Counter(owners)==Counter(range(30)) and Counter(allloops)==Counter(range(9)) and Counter(allcharts)==Counter(range(10)),'complete unique record ownership')
    neighbors=defaultdict(set)
    for es,edge in enumerate(solid.edges):
        uses=incidence[es]
        _need(len(uses)==2 and sum(c.orientation for c in uses)==0,'two opposite physical edge uses')
        neighbors[uses[0].face_slot].add(uses[1].face_slot);neighbors[uses[1].face_slot].add(uses[0].face_slot)
        for endpoint,vs,q in ((0,edge.start,0),(1,edge.end,3)):
            point=tuple(value(x,F(endpoint)) for x in _curve(solid.curves[edge.curve_slot],q))
            _need(point==tuple(map(_f,solid.vertices[vs].position)),'curve/vertex complete endpoint identity')
    found={0};todo=[0]
    while todo:
        for other in neighbors[todo.pop()]-found:found.add(other);todo.append(other)
    _need(found==set(range(7)),'one connected shell')
    for vertex in range(10):
        ports={(i,e) for i,edge in enumerate(solid.edges) for e,v in enumerate((edge.start,edge.end)) if v==vertex}
        adjacency=defaultdict(list)
        for a,b in links[vertex]:adjacency[a].append(b);adjacency[b].append(a)
        _need(set(adjacency)==ports and all(len(n)==2 for n in adjacency.values()),'vertex link port degree')
        visited={next(iter(ports))};todo=list(visited)
        while todo:
            for other in set(adjacency[todo.pop()])-visited:visited.add(other);todo.append(other)
        _need(visited==ports,'single cyclic vertex link')
    _need(10-15+7-sum(l.hole for l in solid.loops)==0,'annular-hole corrected genus')
    return role,incidence


def _maps(result):
    solid=result.solid
    for edge in solid.edges:
        curve=solid.curves[edge.curve_slot]
        for q in range(4) if curve.kind=='circle' else (0,):
            spatial=tuple(radd(rscale(c,_f(solid.frame.scale)),rational((_f(o),)))
                          for c,o in zip(_curve(curve,q),solid.frame.origin))
            uses=[c for c in solid.coedges if c.edge_slot==edge.id-1]
            for use in uses:
                uv=_uv(use.uv_map,q)
                wallq=q if curve.kind=='circle' else (0 if _f(use.uv_map.data[0])==0 else 3)
                mapped=_source_map(result,use.face_slot,uv,wallq)
                _need(all(same(a,b) for a,b in zip(spatial,mapped)),'whole original edge/both-UV rational identity')
                # Actual exact public readers consume non-dyadic rationals;
                # polynomial identity above establishes the stored whole map.
                for t in (F(0),F(1,7),F(1,2),F(6,7),F(1)):
                    parameter=(q+t)/4 if curve.kind=='circle' else t
                    point=solid.edge_point(edge.id,parameter);actualuv=solid.coedge_uv(use.id,parameter)
                    _exact_tuple(point,3,'actual exact world edge');_exact_tuple(actualuv,2,'actual exact coedge UV')
                    _need(point==tuple(value(c,t) for c in spatial) and actualuv==tuple(value(c,t) for c in uv),'actual rational edge/UV reader')


def _operand_label(body,point):
    x,y,z=((_f(p)-_f(o))/_f(body.frame.scale) for p,o in zip(point,body.frame.origin))
    constraints=(z+F(1,2),F(1,2)-z)
    if body.shape=='box':constraints+=(x+F(3,4),F(3,4)-x,y+F(3,4),F(3,4)-y)
    else:constraints+=(F(1,16)-x*x-y*y,)
    return 'exterior' if any(v<0 for v in constraints) else 'boundary' if any(v==0 for v in constraints) else 'interior'


def _selections(result,sections):
    expected={}
    for info in sections:
        for region in (info['cell'],info['disk']):
            point,_=_source_geometry(result,info['capid']-1,*region.witness_uv)
            expected[('box',info['capid'],'cap_region',region.id,None)]=(point,region.kind=='cell',1)
    for fs in range(2,6):
        point,_=_source_geometry(result,fs,F(1,2),F(1,2))
        expected[('box',fs+1,'side',None,None)]=(point,True,1)
    for lo,hi in ((F(0),F(1,4)),(F(1,4),F(3,4)),(F(3,4),F(1))):
        point,_=_source_geometry(result,6,F(1,2),(lo+hi)/2)
        keep=lo==F(1,4)
        expected[('cutter',3,'wall_band',None,(lo,hi))]=(point,keep,-1 if keep else 1)
    for face,z in ((1,F(1,2)),(2,-F(1,2))):
        point=tuple(_f(o)+(z*_f(result.right.frame.scale) if i==2 else 0) for i,o in enumerate(result.right.frame.origin))
        expected[('cutter',face,'cutter_cap',None,None)]=(point,False,1)
    _need(type(result.selections) is tuple and len(result.selections)==13,'complete actual selections')
    seen=set()
    for row in result.selections:
        _need(type(row.face_id) is int and type(row.keep) is bool and type(row.orientation) is int,'typed selection metadata')
        _need(row.region is None or type(row.region) is int,'typed actual region identity')
        if row.domain is not None:_exact_tuple(row.domain,2,'selected source band')
        key=(row.side,row.face_id,row.kind,row.region,row.domain)
        _need(key in expected and key not in seen,'complete distinct selection roles');seen.add(key)
        point,keep,orientation=expected[key]
        _exact_tuple(row.witness,3,'actual selected world witness')
        _need(row.witness==point and row.keep is keep and row.orientation==orientation,'actual complete-cell witness/decision')
        opposite=result.right if row.side=='box' else result.left
        _need(row.label==_operand_label(opposite,point) and row.label in ('interior','exterior'),'independent opposite-operand material selection')


def _history(result,roles,sections):
    solid=result.solid;source={};expected={}
    for body in (result.left,result.right):
        for kind in _KINDS:
            for record in getattr(body,_TABLES[kind]):source[_k(body,kind,record.id)]=record
    _need(len(source)==85,'all original source subshapes')
    full=(F(0),F(1),F(0),F(1));band=(F(1,4),F(3,4));atlas=(F(0),F(4),*band)
    capinfo={info['capid']-1:info for info in sections}
    def define(kind,id,relation,parents,source_range=None,source_uv=None,orientation=1,info=None,arrangement=(),anchor=False):
        if info is None:section=None;pids=eids=()
        else:
            section=info['section']
            pids=tuple(pid for pid,_ in info['anchor'].incidences) if anchor else info['pids']
            eids=(info['anchor'].id,) if anchor else info['eids']
        expected[_k(solid,kind,id)]=(relation,set(parents),source_range,source_uv,orientation,section,pids,eids,arrangement)
    def attached(info,loop=None,use=None):return (info['section'],info['cell'].id,loop,use)
    def generated(kind,id,fs,orientation=1,use=False,anchor=False):
        info=capinfo[fs]
        parents=[_k(result.left,'face',fs+1),_k(result.right,'face',3)]
        if anchor:parents.append(_k(result.right,'edge',3))
        define(kind,id,'generated',parents,orientation=orientation,info=info,anchor=anchor,
               arrangement=attached(info,info['hole'].id,info['use'].id if use else None))
    for kind in ('vertex','edge'):
        for record in getattr(result.left,_TABLES[kind]):
            define(kind,record.id,'retained',(_k(result.left,kind,record.id),),source_range=(F(0),F(1)) if kind=='edge' else None)
    generated('vertex',9,1,anchor=True);generated('vertex',10,0,anchor=True)
    generated('edge',13,1);generated('edge',14,0)
    define('edge',15,'modified',(_k(result.right,'edge',3),),source_range=band)
    for fs in range(7):
        if fs<6:
            info=capinfo.get(fs)
            define('face',fs+1,'modified' if info else 'retained',(_k(result.left,'face',fs+1),),source_uv=full,
                   arrangement=attached(info) if info else ())
        else:define('face',7,'modified',(_k(result.right,'face',3),),source_uv=atlas,orientation=-1)
    for chart in solid.charts:
        fs=chart.face_slot
        if fs<6:
            info=capinfo.get(fs)
            old=result.left.charts[result.left.faces[fs].chart_slots[0]]
            define('chart',chart.id,'modified' if info else 'retained',(_k(result.left,'chart',old.id),),source_uv=full,
                   arrangement=attached(info) if info else ())
        else:
            old=result.right.charts[result.right.faces[2].chart_slots[chart.quadrant]]
            define('chart',chart.id,'modified',(_k(result.right,'chart',old.id),),source_uv=(F(0),F(1),*band),orientation=-1)
    for (kind,id),(role,argument) in roles.items():
        record=getattr(solid,_TABLES[kind])[id-1]
        if role=='box':
            old=source[_k(result.left,kind,argument)];fs=record.face_slot;info=capinfo.get(fs)
            arrangement=()
            if info:
                aid=None
                if kind=='coedge':
                    matches=[uid for uid in info['outer'].uses if info['arr'].edges[info['arr'].uses[uid].edge].source[1]==argument]
                    _need(len(matches)==1,'actual source outer arrangement use');aid=matches[0]
                arrangement=attached(info,info['outer'].id,aid)
            define(kind,id,'retained',(_k(result.left,kind,argument),),source_range=(F(0),F(1)) if kind=='coedge' else None,
                   source_uv=full if kind=='loop' else None,arrangement=arrangement)
        elif role=='hole':generated(kind,id,argument,orientation=solid.faces[argument].orientation)
        elif role=='caprim':generated(kind,id,argument,orientation=solid.faces[argument].orientation,use=True)
        elif role=='wallrim':generated(kind,id,argument,orientation=-solid.faces[argument].orientation,use=True)
        elif role=='wall':
            old=result.right.loops[result.right.faces[2].loop_slots[0]]
            define(kind,id,'modified',(_k(result.right,kind,old.id),),source_uv=atlas,orientation=-1)
        else:
            old=[c for c in result.right.coedges if c.uv_map.kind=='seam_wall' and _f(c.uv_map.data[0])==argument]
            _need(len(old)==1,'actual original seam occurrence')
            define(kind,id,'modified',(_k(result.right,kind,old[0].id),),source_range=band,orientation=-1)
    output={_k(solid,kind,record.id) for kind in _KINDS for record in getattr(solid,_TABLES[kind])}
    _need(set(expected)==output and len(output)==81,'complete independent origin model')
    origins=result.history.origins;records=result.history.records
    _need(type(origins) is tuple and len(origins)==81 and type(records) is tuple and len(records)==85,'complete immutable history tables')
    targets=set();images=defaultdict(set);births=defaultdict(set);relations=Counter()
    for origin in origins:
        target=_key(origin.target)
        _need(target in expected and target not in targets,'exact distinct output history target');targets.add(target)
        wanted=expected[target];relation,parents,rng,uv,orientation,section,pids,eids,arrangement=wanted
        _need(type(origin.parents) is tuple and len(origin.parents)==len(parents) and {_key(p) for p in origin.parents}==parents,'complete actual origin parent set')
        _need(origin.relation==relation and type(origin.orientation) is int and origin.orientation==orientation,'actual origin relation/sense')
        for actual,truth,width in ((origin.source_range,rng,2),(origin.source_uv,uv,4)):
            if truth is None:_need(actual is None,'absent intrinsic source metadata')
            else:_exact_tuple(actual,width,'origin parameter metadata');_need(actual==truth,'complete original source range/domain')
        _need((origin.section is None if section is None else type(origin.section) is int and origin.section==section),'actual source section identity')
        for actual,truth in ((origin.piece_ids,pids),(origin.event_ids,eids)):
            _need(type(actual) is tuple and all(type(v) is int for v in actual) and actual==truth,'complete ordered source piece/event keys')
        _need(type(origin.arrangement) is tuple and origin.arrangement==arrangement and
              all(v is None or type(v) is int for v in origin.arrangement),'actual source arrangement region/loop/use keys')
        _need(result.history.origin(origin.target)==origin,'public reverse history lookup')
        relations[relation]+=1
        for parent in parents:
            (births if relation=='generated' else images)[parent].add(target)
    seen=set();dispositions=Counter()
    for record in records:
        key=_key(record.source)
        _need(key in source and key not in seen,'complete original history source');seen.add(key)
        actualimages={_key(k) for k in record.images};actualbirths={_key(k) for k in record.generated}
        _need(type(record.images) is tuple and type(record.generated) is tuple and len(actualimages)==len(record.images)
              and len(actualbirths)==len(record.generated),'typed unique forward images/births')
        _need(actualimages==images[key] and actualbirths==births[key],'forward/reverse source history agreement')
        status=expected[next(iter(actualimages))][0] if actualimages else 'deleted'
        _need(record.status==status and all(expected[target][0]==status and target[2]==key[2] for target in actualimages),'actual retained/modified/deleted meaning')
        _need(result.history.from_input(record.source)==record,'public forward history lookup')
        dispositions[status]+=1
    _need(relations==Counter(retained=58,modified=13,generated=10) and dispositions==Counter(retained=58,modified=13,deleted=14),'all-kind literal history totals')
    return dict(history_inputs=85,history_outputs=81,retained=58,modified=13,deleted=14,generated=10)


def qualify_result(result):
    """Whole original-material/boundary/source/history qualification."""
    _inputs(result);roles,incidence=_graph(result);_patches(result)
    sections=_sections(result);_maps(result);_selections(result,sections)
    stats=_history(result,roles,sections)
    stats.update(_readers(result))
    scale=_f(result.left.frame.scale)
    _need(type(result.solid.volume.rational) is F and type(result.solid.volume.pi) is F and
          (result.solid.volume.rational,result.solid.volume.pi)==(F(9,4)*scale**3,-F(1,4)*scale**3),'exact selected-region volume coefficients')
    _need(type(result.timings) is tuple and all(type(name) is str and type(ms) is float and math.isfinite(ms) and ms>=0 for name,ms in result.timings),'owned finite construction timings')
    stats.update(vertices=10,edges=15,coedges=30,loops=9,faces=7,charts=10,selections=13,euler=0,genus=1)
    return stats


def qualify_samples(batch):
    """Direct actual-float query labels/world positions/oriented unit normals."""
    result=batch.result;_inputs(result)
    _need(batch.request.result is result and type(batch.rows) is tuple and type(batch.request.candidates) is tuple,'actual request/result sample ownership')
    _need(batch.backend in ('cpu','cuda') and type(batch.device) is int and batch.device>=0,'observed sample backend/device')
    candidates=batch.request.candidates
    _need(batch.count==len(candidates)==len(batch.rows),'complete request rows')
    labels=[];stats=dict(count=len(candidates),accepted=0,rejected=0,invalid=0,boundary=0,max_position_error=F(0),max_normal_error=F(0))
    for i,(row,candidate) in enumerate(zip(batch.rows,candidates)):
        slot=candidate.patch_slot
        _need(type(slot) is int and 0<=slot<10 and type(candidate.u) is float and type(candidate.v) is float,'actual candidate slot/float bits')
        chart=result.solid.charts[slot];face=result.solid.faces[chart.face_slot]
        _need(type(row.index) is int and row.index==i and row.patch_slot==slot and row.face_id==face.id and row.chart_id==chart.id,'actual sample source/scatter identity')
        _need(_bits(row.u)==_bits(candidate.u) and _bits(row.v)==_bits(candidate.v),'actual submitted sample UV bits')
        label=_trim(slot,candidate.u,candidate.v);labels.append(label)
        _need(type(row.trim) is int and row.trim==label,'exact actual-parameter trim label')
        _need(type(row.position) is tuple and type(row.normal) is tuple and len(row.position)==len(row.normal)==3,'immutable actual row vectors')
        if label in (0,3):
            _need(row.native_status==-1 and all(type(x) is float and math.isnan(x) for x in (*row.position,*row.normal)),'rejected/invalid row native scatter')
            stats['rejected' if label==0 else 'invalid']+=1;continue
        _need(row.native_status==0 and all(type(x) is float and math.isfinite(x) for x in (*row.position,*row.normal)),'all admitted native sample fields')
        point,normal=_source_geometry(result,slot,candidate.u,candidate.v)
        for name,actual,wanted,limit in (('position',row.position,point,POSITION_LIMIT),('normal',row.normal,normal,NORMAL_LIMIT)):
            for a,b in zip(actual,wanted):
                error=abs(_f(a)-b);_need(error<=limit,'original-source world '+name+' component limit')
                stats['max_'+name+'_error']=max(stats['max_'+name+'_error'],error)
        stats['accepted']+=1;stats['boundary']+=label==2
    _need(batch.request.labels==tuple(labels) and batch.request.admitted_indices==tuple(i for i,v in enumerate(labels) if v in (1,2)),'complete exact gather classification')
    _need(batch.evaluated_count==stats['accepted'],'actual evaluated sample count')
    _need(type(batch.timings) is tuple and len({name for name,_ in batch.timings})==len(batch.timings) and
          all(type(name) is str and math.isfinite(ms) and ms>=0 for name,ms in batch.timings),'sample phase timings')
    return stats


def qualify_topology(result,adjacency_result):
    """Actual typed ABI5 membership/mate/neighbor consumer, including permutations."""
    solid=result.solid;source=adjacency_result.input
    _need(source.snapshots==() and (source.edge_count,source.face_count,source.coedge_count)==(15,7,30),'actual standalone output topology input')
    def key(source):
        _need(all(type(v) is int for v in (source.instance,source.body_id,source.revision,source.entity_id)),'typed native topology lineage')
        _need((source.instance,source.body_id,source.revision)==(0,solid.body_id,solid.revision),'actual output topology namespace')
        return source.entity_id
    edgeids=tuple(map(key,source.edge_sources));faceids=tuple(map(key,source.face_sources));useids=tuple(map(key,source.coedge_sources))
    _need(set(edgeids)==set(range(1,16)) and set(faceids)==set(range(1,8)) and set(useids)==set(range(1,31)),'complete typed topology lineage cover')
    edges=struct.unpack('<30I',source.coedge_edges);faces=struct.unpack('<30I',source.coedge_faces);signs=struct.unpack('<30i',source.orientations)
    for slot,id in enumerate(useids):
        use=solid.coedges[id-1]
        _need((edgeids[edges[slot]],faceids[faces[slot]],signs[slot])==(use.edge_slot+1,use.face_slot+1,use.orientation),'actual topology source incidence')
    from .adjacency_oracles import qualify_result as qualify_adjacency
    stats=qualify_adjacency(adjacency_result)
    _need(stats['paired']==14 and stats['same_face_pair']==1 and not any(stats[name] for name in ('unused','boundary','orientation_conflict','multiple_uses')),'closed opposite-use native reader kinds')
    for slot,eid in enumerate(edgeids):
        expected=tuple(c.id for c in solid.coedges if c.edge_slot==eid-1)
        _need(Counter(key(v) for v in adjacency_result.edge_uses(slot))==Counter(expected),'actual typed edge-use reader')
        _need(adjacency_result.edge_kind(slot)==(3 if eid==15 else 2),'actual public closed edge-kind reader')
    for slot,id in enumerate(useids):
        current=solid.coedges[id-1]
        mate=next(c for c in solid.coedges if c.edge_slot==current.edge_slot and c.id!=id)
        _need(key(adjacency_result.mate(slot))==mate.id and key(adjacency_result.neighbor(slot))==mate.face_slot+1,'actual typed mate/neighbor reader')
    return stats


def _readers(result):
    """Bounded direct public material/trim checks, separate from stored-domain proof."""
    probes=[(F(0),F(0),F(0)),(F(5,8),F(0),F(0)),(-F(5,8),F(0),F(0)),
            (F(0),F(0),F(1,2)),(F(0),F(0),-F(1,2)),(F(0),F(0),F(1)),(F(0),F(0),-F(1))]
    probes.extend((x,y,z) for x in (-F(3,4),F(3,4)) for y in (-F(3,4),F(3,4)) for z in (-F(1,2),F(1,2)))
    probes.extend((x,y,z) for x,y in ((F(1,2),F(0)),(-F(1,2),F(0)),(F(0),F(1,2)),(F(0),-F(1,2))) for z in (-F(1,2),F(0),F(1,2)))
    probes.extend(((F(5,8),F(0),F(1,2)),(F(5,8),F(0),-F(1,2)),
                   (F(0),-F(3,4),F(0)),(F(0),F(3,4),F(0)),(F(3,4),F(0),F(0)),(-F(3,4),F(0),F(0))))
    # Each original outer edge midpoint, including the four vertical edges.
    probes.extend(tuple((F(a)+F(b))/2 for a,b in zip(result.left.vertices[e.start].position,result.left.vertices[e.end].position)) for e in result.left.edges)
    probes.extend(((F(math.nextafter(.5,0.)),F(0),F(0)),(F(math.nextafter(.5,1.)),F(0),F(0)),
                   (F(1,2)-F(1,1<<1074),F(0),F(0)),(F(1,2)+F(1,1<<1074),F(0),F(0))))
    for p in probes:
        point=tuple(_f(o)+_f(result.left.frame.scale)*v for o,v in zip(result.left.frame.origin,p))
        answer=result.solid.classify_point(point)
        _need(answer.solid is result.solid and answer.point==point,'actual point request/source binding')
        _need((answer.label,answer.boundary_faces)==_material(result,point),'actual complete regularized material/support reader')
    uv=((F(0),F(0)),(F(1),F(1)),(F(1,2),F(1,2)),(F(5,6),F(1,2)),
        (float(F(5,6)),.5),(math.nextafter(float(F(5,6)),0.),.5),
        (F(0),F(1,2)),(F(1),F(1,2)),(F(1,2),F(1,4)),(F(1,2),F(3,4)),
        (F(1,2),F(1,8)),(F(1,2),F(7,8)),(F(-1),F(0)),(math.nan,.5))
    for slot in range(10):
        for u,v in uv:_need(result.solid.classify(slot,u,v)==_trim(slot,u,v),'actual exact/source-float trim reader')
    return dict(material_queries=len(probes),trim_queries=10*len(uv))
