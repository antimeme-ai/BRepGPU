"""Independent original-profile/cutter truth for one composed axial bore.

No product difference, trim, history-builder or source-map helper computes an
expected answer. Actual source expressions determine geometry and exact maps.
"""
from collections import Counter,defaultdict
from fractions import Fraction as F
import math
import struct

from .meshing_oracles import require,point,sub,dot,turn,patch_point,rotate

KINDS=('vertex','edge','coedge','loop','face','chart')
TABLES=('vertices','edges','coedges','loops','faces','charts')


def key(obj,kind,identifier):return obj.body_id,obj.revision,kind,identifier


def typed_key(value):
    from .boolean import EntityKey
    require(type(value) is EntityKey and type(value.kind) is str and value.kind in KINDS and
            all(type(v) is int and 0<v<1<<64 for v in (value.body_id,value.revision,value.entity_id)),
            'typed source/history key differs')
    return value.body_id,value.revision,value.kind,value.entity_id


def keys(obj):return tuple(key(obj,k,r.id) for k,t in zip(KINDS,TABLES) for r in getattr(obj,t))


def source_data(result):
    feature,cutter=result.feature,result.cutter;frame=feature.profile.frame
    points=tuple(v.position[:2] for v in feature.profile.vertices);h=feature.height
    center=tuple((F(cutter.frame.origin[k])-F(frame.origin[k]))/F(frame.scale) for k in (0,1))
    radius=F(cutter.frame.scale)/(4*F(frame.scale))
    band=(F(1,2)-h*F(frame.scale)/(2*F(cutter.frame.scale)),
          F(1,2)+h*F(frame.scale)/(2*F(cutter.frame.scale)))
    return points,h,center,radius,band


def circle(center,radius,t):
    q,s=(3,F(1)) if t==1 else (int(4*t),4*t-int(4*t))
    x,y=rotate(radius*(1-s*s)/(1+s*s),2*radius*s/(1+s*s),q)
    return center[0]+x,center[1]+y


def world(frame,local):return tuple(F(o)+F(frame.scale)*x for o,x in zip(frame.origin,local))


def expected_edge(result,identifier,t):
    points,h,c,r,band=source_data(result);n=len(points);source=result.feature.solid
    if identifier<=3*n:
        edge=source.edges[identifier-1];a=source.vertices[edge.start].position;b=source.vertices[edge.end].position
        local=tuple((1-t)*x+t*y for x,y in zip(a,b))
    elif identifier<=3*n+2:local=(*circle(c,r,t),(-h/2,h/2)[identifier-3*n-1])
    else:local=(c[0]+r,c[1],-h/2+h*t)
    return world(result.feature.profile.frame,local)


def expected_uv(result,identifier,t):
    points,h,c,r,band=source_data(result);n=len(points)
    if identifier<=6*n:
        data=result.feature.solid.coedges[identifier-1].uv_map.data
        return tuple((1-t)*F(data[k])+t*F(data[k+2]) for k in range(2))
    if identifier<=6*n+2:return tuple((x+1)/2 for x in circle(c,r,t))
    at=identifier-6*n-3
    if at in (0,2):return F(0 if at==0 else 4),band[0]+t*(band[1]-band[0])
    return 4*t,band[1 if at==1 else 0]


def material(result,query):
    points,h,c,r,band=source_data(result);frame=result.feature.profile.frame
    p=tuple((F(x)-F(o))/F(frame.scale) for x,o in zip(query,frame.origin))
    values=((1,p[2]+h/2),(2,h/2-p[2]))+tuple((i+3,turn(a,b,p[:2])) for i,(a,b) in enumerate(zip(points,points[1:]+points[:1])))
    values+=((len(points)+3,(p[0]-c[0])**2+(p[1]-c[1])**2-r*r),)
    if any(v<0 for _,v in values):return 'exterior',()
    faces=tuple(i for i,v in values if v==0)
    return ('boundary' if faces else 'interior'),faces


def trim(result,slot,u,v):
    if any(type(x) is float and not math.isfinite(x) for x in (u,v)):return 3
    u,v=F(u),F(v)
    if not 0<=u<=1 or not 0<=v<=1:return 3
    points,h,c,r,band=source_data(result);n=len(points)
    if slot<2:
        p=(2*u-1,2*v-1);supports=tuple(turn(a,b,p) for a,b in zip(points,points[1:]+points[:1]))
        radial=(p[0]-c[0])**2+(p[1]-c[1])**2-r*r
        if any(s<0 for s in supports) or radial<0:return 0
        return 2 if any(s==0 for s in supports) or radial==0 else 1
    if slot<n+2:return 2 if u in (0,1) or v in (0,1) else 1
    if not band[0]<=v<=band[1]:return 0
    q=slot-n-2
    return 2 if v in band or q==0 and u==0 or q==3 and u==1 else 1


def _expected_origins(result):
    """Literal target-role contract, independent of produced reciprocal tables."""
    feature=result.feature.solid;cutter=result.cutter;body=result.solid
    points,h,c,r,band=source_data(result);n=len(points);answer={}
    def put(kind,i,relation,parents,ran=None,uv=None,orientation=1):
        target=key(body,kind,i)
        require(target not in answer,'independent target role duplicated')
        answer[target]=(relation,tuple(parents),ran,uv,orientation)
    for kind,table in zip(KINDS,TABLES):
        for record in getattr(feature,table):
            i=record.id
            put(kind,i,'modified' if kind in ('face','chart') and i<=2 else 'retained',
                (key(feature,kind,i),), (F(0),F(1)) if kind in ('edge','coedge') else None,
                (F(0),F(1),F(0),F(1)) if kind in ('face','chart') else None)
    for level in range(2):
        parents=(key(feature,'face',level+1),key(cutter,'face',3))
        put('vertex',2*n+level+1,'generated',parents+(key(cutter,'edge',3),))
        put('edge',3*n+level+1,'generated',parents)
        put('loop',n+level+3,'generated',parents,orientation=1 if level==0 else -1)
        put('coedge',6*n+level+1,'generated',parents,orientation=1 if level==0 else -1)
    put('edge',3*n+3,'modified',(key(cutter,'edge',3),),band)
    put('coedge',6*n+3,'modified',(key(cutter,'coedge',6),),band,orientation=-1)
    put('coedge',6*n+4,'generated',(key(feature,'face',2),key(cutter,'face',3)))
    put('coedge',6*n+5,'modified',(key(cutter,'coedge',4),),band,orientation=-1)
    put('coedge',6*n+6,'generated',(key(feature,'face',1),key(cutter,'face',3)),orientation=-1)
    for kind,i,parent in (('loop',n+5,3),('face',n+3,3)):
        put(kind,i,'modified',(key(cutter,kind,parent),),uv=(F(0),F(4),*band),orientation=-1)
    for q in range(4):
        put('chart',n+3+q,'modified',(key(cutter,'chart',q+3),),uv=(F(0),F(1),*band),orientation=-1)
    return answer


def _history(result):
    from .boolean import EntityKey
    expected=_expected_origins(result);history=result.history;feature=result.feature
    require(type(history.origins) is tuple and type(history.records) is tuple,'immutable immediate history differs')
    actual={};forward=defaultdict(lambda:[[],[]]);images_modified={}
    for origin in history.origins:
        target=typed_key(origin.target)
        require(target in expected and target not in actual,'complete unique immediate output origin differs')
        parents=tuple(typed_key(p) for p in origin.parents)
        require(type(origin.parents) is tuple and type(origin.orientation) is int,'typed immediate origin differs')
        value=(origin.relation,parents,origin.source_range,origin.source_uv,origin.orientation)
        require(value==expected[target],'independent original source role/maps differ')
        for name,width in (('source_range',2),('source_uv',4)):
            v=getattr(origin,name)
            require(v is None or type(v) is tuple and len(v)==width and all(type(x) is F for x in v),'exact immediate source map types differ')
        require(origin.section is None and not origin.piece_ids and not origin.event_ids and not origin.arrangement,
                'specialized bore invented general section provenance')
        require(history.origin(origin.target) is origin,'actual immediate origin lookup identity differs')
        actual[target]=origin
        for parent in parents:
            forward[parent][origin.relation=='generated'].append(target)
            if origin.relation=='modified':images_modified[parent]=True
    require(set(actual)==set(expected)==set(keys(result.solid)),'complete actual immediate output inventory differs')
    _records(history,keys(feature.solid)+keys(result.cutter),forward,images_modified,'immediate')
    # First operation's source roles/maps were independently qualified already.
    # Join the independently expected cut roles to those ACTUAL origin objects.
    composed=result.composed_history;seen={};root_forward=defaultdict(lambda:[[],[]]);root_modified={}
    for origin in composed.origins:
        target=typed_key(origin.target)
        require(target in expected and target not in seen,'complete unique composed origin differs')
        immediate=actual[target];paths=[]
        for parent in expected[target][1]:
            if parent[:2]==(feature.solid.body_id,feature.solid.revision):
                previous=feature.history.origin(EntityKey(*parent))
                for root in previous.parents:paths.append((typed_key(root),(immediate,previous)))
            else:paths.append((parent,(immediate,)))
        require(type(origin.paths) is tuple and len(origin.paths)==len(paths),'complete composed source path count differs')
        actual_paths=[]
        for path,(root,steps) in zip(origin.paths,paths):
            require(typed_key(path.root)==root and type(path.steps) is tuple and len(path.steps)==len(steps)
                    and all(a is b for a,b in zip(path.steps,steps)),
                    'composed path lost actual original source/map owner')
            generated=any(o.relation=='generated' for o in steps)
            root_forward[root][generated].append(target)
            if not generated and any(o.relation=='modified' for o in steps):root_modified[root]=True
            actual_paths.append((root,tuple(id(step) for step in steps)))
        require(len(set(actual_paths))==len(actual_paths),'duplicate composed source path differs')
        require(composed.origin(origin.target) is origin,'actual composed lookup identity differs')
        seen[target]=origin
    require(set(seen)==set(expected),'complete final composed history differs')
    _records(composed,keys(feature.profile)+keys(result.cutter),root_forward,root_modified,'composed')
    return dict(history_inputs=len(history.records),history_outputs=len(actual),
                composed_inputs=len(composed.records),composed_paths=sum(len(o.paths) for o in composed.origins))


def _records(history,inputs,forward,modified,label):
    expected=set(inputs);seen=set()
    for record in history.records:
        source=typed_key(record.source)
        require(source in expected and source not in seen,'complete '+label+' input inventory differs');seen.add(source)
        wanted=forward[source];images=set(wanted[0]);births=set(wanted[1])
        require(type(record.images) is tuple and type(record.generated) is tuple,
                'immutable '+label+' history targets differ')
        ai=tuple(typed_key(v) for v in record.images);ag=tuple(typed_key(v) for v in record.generated)
        require(len(set(ai))==len(ai) and len(set(ag))==len(ag) and set(ai)==images and set(ag)==births,
                'whole reciprocal '+label+' history differs')
        status='deleted' if not images else 'modified' if modified.get(source) else 'retained'
        require(record.status==status,'source '+label+' disposition differs')
        require(history.from_input(record.source) is record,'actual '+label+' input lookup identity differs')
    require(seen==expected,'missing original '+label+' input disposition')


def _topology(result):
    body=result.solid;feature=result.feature.solid;points,h,c,r,band=source_data(result);n=len(points)
    sizes=(2*n+2,3*n+3,6*n+6,n+5,n+3,n+6)
    for table,size in zip(TABLES,sizes):
        records=getattr(body,table)
        require(type(records) is tuple and len(records)==size and
                all(type(v.id) is int and v.id==i+1 for i,v in enumerate(records)),'complete dense bore '+table+' differ')
    require(type(body.body_id) is int and 0<body.body_id<1<<64 and body.body_id not in
            (feature.body_id,result.cutter.body_id,result.feature.profile.body_id) and
            type(body.revision) is int and body.revision==1,'fresh typed bore namespace differs')
    require(body.frame is feature.frame and body.center==c and body.radius==r and body.wall_band==band,
            'actual source frame/circle/raw wall band differs')
    for name in ('center','wall_band'):point(getattr(body,name),2,'actual '+name)
    require(type(body.radius) is F,'exact retained radius differs')
    for i,v in enumerate(body.vertices):
        expected=feature.vertices[i].position if i<2*n else (c[0]+r,c[1],(-h/2,h/2)[i-2*n])
        point(v.position,3,'body-local vertex');require(v.position==expected,'original/section vertex differs')
    require(type(body.curves) is tuple and len(body.curves)==3*n+3,'complete original/section curves differ')
    for i,e in enumerate(body.edges):
        if i<3*n:
            require(e==feature.edges[i] and body.curves[e.curve_slot]==feature.curves[feature.edges[i].curve_slot],
                    'retained actual outer edge/curve differs')
        else:
            a,b=(2*n+i-3*n,)*2 if i<3*n+2 else (2*n,2*n+1)
            require((e.start,e.end,e.curve_slot)==(a,b,i) and
                    all(type(x) is int for x in (e.start,e.end,e.curve_slot)),'new rim/seam topology differs')
            curve=body.curves[i]
            expected=(c[0],c[1],r,(-h/2,h/2)[i-3*n]) if i<3*n+2 else body.vertices[a].position+body.vertices[b].position
            require(curve.kind==('circle_center' if i<3*n+2 else 'line') and curve.data==expected and
                    all(type(x) is F for x in curve.data),'actual complete circle/seam expression differs')
    for i,use in enumerate(body.coedges):
        if i<6*n:require(use==feature.coedges[i],'retained original coedge differs')
        else:
            expected=((3*n,0,n+2,1),(3*n+1,1,n+3,-1),
                      (3*n+2,n+2,n+4,1),(3*n+1,n+2,n+4,1),
                      (3*n+2,n+2,n+4,-1),(3*n,n+2,n+4,-1))[i-6*n]
            require((use.edge_slot,use.face_slot,use.loop_slot,use.orientation)==expected and
                    all(type(x) is int for x in (use.edge_slot,use.face_slot,use.loop_slot,use.orientation)),
                    'complete original seam/hole occurrence differs')
    for i,loop in enumerate(body.loops):
        if i<n+2:require(loop==feature.loops[i],'retained original loop differs')
        else:
            expected=((0,True,(6*n,)),(1,True,(6*n+1,)),(n+2,False,tuple(range(6*n+2,6*n+6))))[i-n-2]
            require((loop.face_slot,loop.hole,loop.coedge_slots)==expected and type(loop.hole) is bool and
                    type(loop.face_slot) is int and all(type(s) is int for s in loop.coedge_slots),'complete hole/wall loop differs')
    for i,face in enumerate(body.faces):
        require(type(face.orientation) is int and type(face.loop_slots) is tuple and type(face.chart_slots) is tuple,
                'typed face ownership differs')
        if i<2:
            require(face.orientation==(-1,1)[i] and face.loop_slots==(i,n+2+i) and face.chart_slots==(i,),
                    'actual cap with original outer and section hole differs')
        elif i<n+2:require(face==feature.faces[i],'actual original side face differs')
        else:require(face.orientation==-1 and face.loop_slots==(n+4,) and face.chart_slots==tuple(range(n+2,n+6)),
                     'inward wall face/charts differ')
    for i,chart in enumerate(body.charts):
        expected=feature.charts[i] if i<n+2 else result.cutter.charts[i-n]
        require(type(chart.face_slot) is int and chart.face_slot==(i if i<n+2 else n+2) and
                type(chart.quadrant) is int and chart.quadrant==expected.quadrant,
                'actual original carrier/frame/quarter owner differs')
        patch,source=chart.patch,expected.patch
        require(type(patch.degree_u) is int and type(patch.degree_v) is int and
                (patch.degree_u,patch.degree_v)==(source.degree_u,source.degree_v),
                'actual source carrier degree differs')
        for attr in ('controls','origin','uv'):
            actual,wanted=getattr(patch,attr),getattr(source,attr)
            require(type(actual) is tuple and len(actual)==len(wanted) and
                    all(type(a) is float and struct.pack('<d',a)==struct.pack('<d',b) for a,b in zip(actual,wanted)),
                    'actual original carrier '+attr+' bits differ')
        require(type(patch.scale) is float and struct.pack('<d',patch.scale)==struct.pack('<d',source.scale),
                'actual original carrier scale bits differ')
        lin=patch.lineage
        require((lin.body_id,lin.revision,lin.entity_kind,lin.entity_id,lin.chart_id)==
                (body.body_id,1,5,chart.face_slot+1,i+1) and
                all(type(v) is int for v in (lin.body_id,lin.revision,lin.entity_kind,lin.entity_id,lin.chart_id)),
                'fresh actual carrier lineage differs')
        require(body.patches[i] is chart.patch,'original patch/chart identity differs')
    # Closed incidence and cyclic vertex links, including self-loop ports.
    uses=defaultdict(list);links=defaultdict(list);neighbors=defaultdict(set);seen_uses=[]
    for loop in body.loops:
        for previous,current in zip(loop.coedge_slots[-1:]+loop.coedge_slots[:-1],loop.coedge_slots):
            pc,cc=body.coedges[previous],body.coedges[current];pe,ce=body.edges[pc.edge_slot],body.edges[cc.edge_slot]
            a,b=(pc.edge_slot,1 if pc.orientation==1 else 0),(cc.edge_slot,0 if cc.orientation==1 else 1)
            require((pe.start,pe.end)[a[1]]==(ce.start,ce.end)[b[1]],'closed loop physical identity differs')
            links[(ce.start,ce.end)[b[1]]].append((a,b));uses[cc.edge_slot].append(cc);seen_uses.append(current)
            require(cc.face_slot==loop.face_slot and cc.loop_slot==loop.id-1,'coedge loop/face owner differs')
    require(sorted(seen_uses)==list(range(6*n+6)),'whole occurrence ownership differs')
    for members in uses.values():
        require(len(members)==2 and sum(c.orientation for c in members)==0,'two opposed bore edge uses differ')
        a,b=(v.face_slot for v in members);neighbors[a].add(b);neighbors[b].add(a)
    _connected(neighbors,0,set(range(n+3)),'single connected bore shell differs')
    require(set(links)==set(range(2*n+2)),'complete bore vertex links differ')
    for v,arcs in links.items():
        graph=defaultdict(list)
        for a,b in arcs:graph[a].append(b);graph[b].append(a)
        ports={(i,k) for i,e in enumerate(body.edges) for k,x in enumerate((e.start,e.end)) if x==v}
        require(set(graph)==ports and all(len(v)==2 for v in graph.values()),'bore vertex-link degrees differ')
        _connected(graph,next(iter(graph)),ports,'bore vertex link has multiple cycles')
    require(sizes[0]-sizes[1]+sizes[4]-2==0,'bore genus-one topology differs')


def _connected(graph,start,expected,message):
    seen=set();todo=[start]
    while todo:
        v=todo.pop()
        if v not in seen:seen.add(v);todo.extend(graph[v])
    require(seen==expected,message)


def _readers(result):
    body=result.solid;points,h,c,r,band=source_data(result);n=len(points);probes=[];tiny=F(1,1<<1074)
    for t in (F(0),F(1,7),F(1,4),F(1,2),F(1)):
        xy=circle(c,r,t)
        for z in (-h/2,F(0),h/2):probes.append((*xy,z))
    for a,b in zip(points,points[1:]+points[:1]):
        mid=tuple((x+y)/2 for x,y in zip(a,b))
        for xy in (a,mid):
            probes.extend((*xy,z) for z in (-h/2,F(0),h/2))
    probes.extend((c[0]+r+d,c[1],z) for d in (-tiny,F(0),tiny) for z in (-h/2,F(0),h/2))
    probes.extend((c[0],c[1],z) for z in (-h/2-tiny,-h/2,F(0),h/2,h/2+tiny))
    for p in probes:
        q=world(body.frame,p);actual=body.classify_point(q)
        require(actual.solid is body and actual.point==q and (actual.label,actual.boundary_faces)==material(result,q),
                'actual original-value material/all-support reader differs')
    trim_queries=0
    general=((F(0),F(0)),(F(1,2),F(1,2)),(F(1),F(1)),(-F(1,256),F(1,2)),(math.nan,.5))
    cap=general+tuple(tuple((v+1)/2 for v in xy) for xy in points+(c,circle(c,r,F(0)),circle(c,r,F(1,7))))
    for slot in range(n+6):
        uv=cap if slot<2 else general if slot<n+2 else general+tuple((u,v) for u in (F(0),F(1,7),F(1)) for v in (*band,sum(band)/2))
        for u,v in uv:
            require(type(body.classify(slot,u,v)) is int and body.classify(slot,u,v)==trim(result,slot,u,v),
                    'actual exact cap/side/raw-wall trim differs');trim_queries+=1
    count=0
    for e in body.edges:
        for t in (F(0),F(1,7),F(2,3),F(1)):
            answer=body.edge_point(e.id,t);point(answer,3,'exact edge reader')
            require(answer==expected_edge(result,e.id,t),'actual original edge/circle expression differs');count+=1
    for coedge in body.coedges:
        for t in (F(0),F(1,7),F(2,3),F(1)):
            uv=body.coedge_uv(coedge.id,t);point(uv,2,'actual incident UV')
            require(uv==expected_uv(result,coedge.id,t),'actual original cap/side/cutter UV differs');count+=1
            face=body.faces[coedge.face_slot];chart=body.charts[face.chart_slots[0]]
            if face.id==n+3:
                q=min(3,int(uv[0]));chart=body.charts[n+2+q];uv=(uv[0]-q,uv[1])
            require(patch_point(chart.patch,uv)==expected_edge(result,body.edges[coedge.edge_slot].id,t),
                    'actual coupled edge/original incident patch maps differ')
    return dict(material_queries=len(probes),trim_queries=trim_queries,exact_reader_queries=count)


def qualify_result(result):
    from .extrusion_oracles import qualify_result as qualify_extrusion
    from .body_oracles import qualify_snapshot
    from .feature_bore import BoreResult,BoreSolid
    require(type(result) is BoreResult and type(result.solid) is BoreSolid,'actual bore result/solid types differ')
    qualify_extrusion(result.feature);qualify_snapshot(result.cutter)
    points,h,c,r,band=source_data(result);n=len(points)
    require(result.cutter.shape=='cylinder' and result.cutter.frame.origin[2]==result.feature.profile.frame.origin[2],
            'actual selected axial cutter source differs')
    for a,b in zip(points,points[1:]+points[:1]):
        s=turn(a,b,c);d=sub(b,a)
        require(s>0 and s*s>r*r*dot(d,d),'strict whole circle/source support admission differs')
    require(F(0)<band[0]<band[1]<1,'strict original cutter span differs')
    scale=F(result.feature.profile.frame.scale)
    require(type(result.solid.volume.rational) is F and type(result.solid.volume.pi) is F and
            result.solid.volume.rational==result.feature.profile.area*h*scale**3 and
            result.solid.volume.pi==-r*r*h*scale**3,'exact original material volume expression differs')
    _topology(result);stats=_history(result);stats.update(_readers(result))
    return dict(profile_vertices=n,vertices=2*n+2,edges=3*n+3,coedges=6*n+6,
                loops=n+5,faces=n+3,charts=n+6,genus=1,**stats)
