"""Independent exact product-domain, source-history and native consumer truth.

Expected geometry is reconstructed from original lattice vertices and height;
no feature construction, trim, validation or history helper supplies an answer.
"""
from collections import Counter,defaultdict
from fractions import Fraction as F
import math
import struct

POSITION_LIMIT=F(1,1<<36)
NORMAL_LIMIT=F(1,1<<30)
_KINDS=('vertex','edge','coedge','loop','face','chart')
_TABLES=('vertices','edges','coedges','loops','faces','charts')
_ZERO=F(0);_ONE=F(1)
_ID=(_ZERO,_ONE,_ZERO)
_UV_ID=(_ID,(_ZERO,_ZERO,_ONE))


def _need(ok,message):
    if not ok:raise AssertionError(message)


def _f(x):
    if type(x) is F:return x
    if type(x) is int:return F(x)
    if type(x) is float and math.isfinite(x):return F(x)
    raise ValueError('expected finite exact numeric coordinate')


def _bits(x):return struct.pack('<d',x)


def _tuple(x,n,label):
    _need(type(x) is tuple and len(x)==n and all(type(v) is F for v in x),label+' exact Fraction tuple')


def _ints(x,label):_need(type(x) is tuple and all(type(v) is int for v in x),label+' typed slot tuple')


def _namespace(obj):
    _need(type(obj.body_id) is int and 0<obj.body_id<1<<64 and type(obj.revision) is int and obj.revision==1,'typed body namespace/revision')


def _points(profile):
    from .body import Frame
    from .extrusion import PlanarProfile
    _need(type(profile) is PlanarProfile,'actual factory profile type')
    _namespace(profile)
    _need(type(profile.frame) is Frame,'owned Frame type')
    _need(type(profile.frame.origin) is tuple and len(profile.frame.origin)==3 and all(type(v) is float and math.isfinite(v) and abs(v)<=16 for v in profile.frame.origin),'source frame origin')
    _need(type(profile.frame.scale) is float and profile.frame.scale in (.25,.5,1.,2.,4.),'source frame scale')
    _need(type(profile.vertices) is tuple and 3<=len(profile.vertices)<=32,'source polygon size')
    points=[]
    for vertex in profile.vertices:
        _tuple(vertex.position,3,'source vertex')
        x,y,z=vertex.position
        _need(z==0 and all(-F(3,4)<=v<=F(3,4) and (v*256).denominator==1 for v in (x,y)),'original source lattice coordinates')
        points.append((x,y))
    _need(len(set(points))==len(points),'distinct source vertices')
    for i,a in enumerate(points):
        b=points[(i+1)%len(points)]
        _need(all(_cross(a,b,p)>0 for j,p in enumerate(points) if j not in (i,(i+1)%len(points))),'global strict source support')
    return tuple(points)


def _cross(a,b,p):return (b[0]-a[0])*(p[1]-a[1])-(b[1]-a[1])*(p[0]-a[0])


def _area(points):return sum(a[0]*b[1]-a[1]*b[0] for a,b in zip(points,points[1:]+points[:1]))/2


def _world(frame,p):return tuple(F(o)+F(frame.scale)*x for o,x in zip(frame.origin,p))


def _local(frame,p):return tuple((_f(x)-F(o))/F(frame.scale) for x,o in zip(p,frame.origin))


def _blend(a,b,t):return tuple(x+t*(y-x) for x,y in zip(a,b))


def _surface(points,h,slot,u,v):
    if slot<2:return (2*u-1,2*v-1,(-h/2,h/2)[slot])
    a=points[slot-2];b=points[(slot-1)%len(points)]
    return (*_blend(a,b,u),(v-F(1,2))*h)


def _trim(points,slot,u,v):
    if type(u) is float and not math.isfinite(u) or type(v) is float and not math.isfinite(v):return 3
    u,v=_f(u),_f(v)
    if not 0<=u<=1 or not 0<=v<=1:return 3
    if slot>=2:return 2 if u in (0,1) or v in (0,1) else 1
    p=(2*u-1,2*v-1);signs=tuple(_cross(a,b,p) for a,b in zip(points,points[1:]+points[:1]))
    if any(s<0 for s in signs):return 0
    return 2 if any(s==0 for s in signs) else 1


def _material(points,h,frame,worldpoint):
    x,y,z=_local(frame,worldpoint)
    values=((1,z+h/2),(2,h/2-z))+tuple((i+3,_cross(a,b,(x,y))) for i,(a,b) in enumerate(zip(points,points[1:]+points[:1])))
    if any(v<0 for _,v in values):return 'exterior',()
    faces=tuple(i for i,v in values if v==0)
    return ('boundary' if faces else 'interior'),faces


def _model(points,h=None):
    """Literal source/output roles, independently enumerated from the contract."""
    n=len(points);source=h is None
    levels=(F(0),) if source else (-h/2,h/2)
    vertices=tuple((x,y,z) for z in levels for x,y in points)
    edges=[(i+k*n,(i+1)%n+k*n) for k in range(len(levels)) for i in range(n)]
    if not source:edges.extend((i,n+i) for i in range(n))
    uv=lambda i:tuple((v+1)/2 for v in points[i])
    uses=[];words=[]
    if source:
        for i in range(n):uses.append((i,0,1,uv(i)+uv((i+1)%n)))
        words.append(tuple(range(n)))
    else:
        for i in reversed(range(n)):uses.append((i,0,-1,uv(i)+uv((i+1)%n)))
        words.append(tuple(range(n)))
        for i in range(n):uses.append((n+i,1,1,uv(i)+uv((i+1)%n)))
        words.append(tuple(range(n,2*n)))
        for i in range(n):
            start=len(uses);j=(i+1)%n
            uses.extend(((i,i+2,1,(F(0),F(0),F(1),F(0))),
                         (2*n+j,i+2,1,(F(1),F(0),F(1),F(1))),
                         (n+i,i+2,-1,(F(0),F(1),F(1),F(1))),
                         (2*n+i,i+2,-1,(F(0),F(0),F(0),F(1)))))
            words.append(tuple(range(start,start+4)))
    return vertices,tuple(edges),tuple(uses),tuple(words)


def _patches(obj,points,h=None):
    source=h is None;count=1 if source else len(points)+2
    _need(type(obj.patches) is tuple and len(obj.patches)==len(obj.charts)==count,'complete native carriers')
    for slot,(patch,chart) in enumerate(zip(obj.patches,obj.charts)):
        if source or slot<2:
            z=F(0) if source else (-h/2,h/2)[slot]
            controls=tuple(float(v) for x in (-1,1) for y in (-1,1) for v in (x,y,z,1))
        else:
            controls=tuple(float(v) for p in (points[slot-2],points[(slot-1)%len(points)]) for z in (-h/2,h/2) for v in (*p,z,1))
        _need(all(type(x) is int for x in (patch.degree_u,patch.degree_v)) and (patch.degree_u,patch.degree_v)==(1,1),'bilinear native degrees')
        for label,actual,expected in (('controls',patch.controls,controls),('UV',patch.uv,(0.,1.,0.,1.)),('origin',patch.origin,obj.frame.origin)):
            _need(type(actual) is tuple and len(actual)==len(expected) and all(type(a) is float and _bits(a)==_bits(b) for a,b in zip(actual,expected)),'complete exact native '+label+' bits')
        _need(type(patch.scale) is float and _bits(patch.scale)==_bits(obj.frame.scale),'native actual source scale')
        lin=patch.lineage
        _need(all(type(v) is int for v in (lin.body_id,lin.revision,lin.entity_kind,lin.entity_id,lin.chart_id)) and
              (lin.body_id,lin.revision,lin.entity_kind,lin.entity_id,lin.chart_id)==(obj.body_id,1,5,slot+1,slot+1),'typed actual native lineage')
        _need(all(type(v) is int for v in (chart.face_slot,chart.quadrant)) and chart.face_slot==slot and chart.quadrant==-1 and chart.patch is patch,'actual chart carrier ownership')


def _graph(obj,points,h=None):
    vertices,edges,uses,words=_model(points,h);n=len(points);source=h is None
    sizes=(len(vertices),len(edges),len(uses),len(words),len(words),len(words))
    for name,size in zip(_TABLES,sizes):
        records=getattr(obj,name)
        _need(type(records) is tuple and len(records)==size and all(type(r.id) is int and r.id==i+1 for i,r in enumerate(records)),'complete dense '+name)
    for actual,expected in zip(obj.vertices,vertices):
        _tuple(actual.position,3,'actual local vertex');_need(actual.position==expected,'complete exact source/output vertices')
    _need(type(obj.curves) is tuple and len(obj.curves)==len(edges),'complete curve table')
    for i,(edge,(a,b)) in enumerate(zip(obj.edges,edges)):
        _need(all(type(v) is int for v in (edge.start,edge.end,edge.curve_slot)) and (edge.start,edge.end,edge.curve_slot)==(a,b,i),'physical edge ownership/direction')
        curve=obj.curves[i];_tuple(curve.data,6,'curve data')
        _need(curve.kind=='line' and curve.data==vertices[a]+vertices[b],'complete original affine curve')
    for i,(use,(edge,face,sign,uv)) in enumerate(zip(obj.coedges,uses)):
        _need(all(type(v) is int for v in (use.edge_slot,use.face_slot,use.loop_slot,use.orientation)) and
              (use.edge_slot,use.face_slot,use.loop_slot,use.orientation)==(edge,face,face,sign),'complete oriented use word')
        _tuple(use.uv_map.data,4,'incident UV data');_need(use.uv_map.kind=='line' and use.uv_map.data==uv,'complete intrinsic affine incident UV')
        # Affine in t on both sides: equality of constant/linear coefficients
        # establishes the whole edge and both incident surface maps.
        a,b=edges[edge]
        def mapped(u,v):
            return (2*u-1,2*v-1,F(0)) if source else _surface(points,h,face,u,v)
        _need(mapped(*uv[:2])==vertices[a] and mapped(*uv[2:])==vertices[b],'whole affine edge/surface composition identity')
    for i,(loop,face,word) in enumerate(zip(obj.loops,obj.faces,words)):
        _ints(loop.coedge_slots,'loop uses');_ints(face.loop_slots,'face loops');_ints(face.chart_slots,'face charts')
        _need(type(loop.face_slot) is int and loop.face_slot==i and type(loop.hole) is bool and not loop.hole and loop.coedge_slots==word,'complete simple loop domain')
        _need(type(face.orientation) is int and face.orientation==(-1 if not source and i==0 else 1) and
              face.kind==('convex_polygon' if source or i<2 else 'rectangle') and type(face.name) is str and face.loop_slots==(i,) and face.chart_slots==(i,),'complete face ownership/domain/sense')
    _patches(obj,points,h)
    if source:return
    incidence=defaultdict(list);neighbors=defaultdict(set);links=defaultdict(list)
    for i,(es,fs,sign,uv) in enumerate(uses):incidence[es].append((i,fs,sign))
    for members in incidence.values():
        _need(len(members)==2 and sum(v[2] for v in members)==0,'opposed degree-two edge uses')
        a,b=(v[1] for v in members);neighbors[a].add(b);neighbors[b].add(a)
    for word in words:
        for previous,current in zip(word[-1:]+word[:-1],word):
            pe,pf,ps,_=uses[previous];ce,cf,cs,_=uses[current]
            pend=1 if ps==1 else 0;cstart=0 if cs==1 else 1
            vertex=edges[pe][pend];_need(vertex==edges[ce][cstart],'oriented loop closure')
            links[vertex].append(((pe,pend),(ce,cstart)))
    found={0};todo=[0]
    while todo:
        for other in neighbors[todo.pop()]-found:found.add(other);todo.append(other)
    _need(found==set(range(n+2)),'single connected shell')
    for vertex in range(2*n):
        ports={(i,e) for i,edge in enumerate(edges) for e,v in enumerate(edge) if v==vertex};adj=defaultdict(list)
        for a,b in links[vertex]:adj[a].append(b);adj[b].append(a)
        _need(set(adj)==ports and all(len(v)==2 for v in adj.values()),'complete vertex link degree')
        seen={next(iter(ports))};todo=list(seen)
        while todo:
            for other in set(adj[todo.pop()])-seen:seen.add(other);todo.append(other)
        _need(seen==ports,'single cyclic vertex link')
    _need(2*n-3*n+n+2==2,'genus-zero supporting Euler check')


def _key(key):
    _need(all(type(v) is int and 0<v<1<<64 for v in (key.body_id,key.revision,key.entity_id)) and type(key.kind) is str and key.kind in _KINDS,'typed history key')
    return key.body_id,key.revision,key.kind,key.entity_id


def _k(obj,kind,id):return obj.body_id,obj.revision,kind,id


def _history(result,points):
    profile,solid,h=result.profile,result.solid,result.height;n=len(points);expected={}
    def put(kind,id,relation,parents,role,translation=None,st=None,suv=None,axial=None,orientation=1):
        key=_k(solid,kind,id);_need(key not in expected,'unique independent history role')
        expected[key]=(relation,tuple(_k(profile,k,i) for k,i in parents),role,translation,st,suv,axial,orientation)
    sizes=(n,n,n,1,1,1)
    for kind,size in zip(_KINDS,sizes):
        for i in range(size):
            for level in (0,1):
                z=(-h/2,h/2)[level]
                id=(i+level*n+1 if kind in ('vertex','edge') else
                    (n-i if level==0 else n+i+1) if kind=='coedge' else level+1)
                put(kind,id,'modified',((kind,i+1),),('lower_cap','upper_cap')[level],(F(0),F(0),z),
                    _ID if kind in ('edge','coedge') else None,_UV_ID if kind in ('face','chart') else None,
                    (z,F(0),F(0)),-1 if level==0 and kind in ('coedge','loop','face','chart') else 1)
    for i in range(n):
        j=(i+1)%n;parents=(('edge',i+1),('coedge',i+1))
        put('edge',2*n+i+1,'generated',(('vertex',i+1),),'swept_vertex',axial=(-h/2,h,F(0)))
        for kind in ('face','chart','loop'):
            put(kind,i+3,'generated',parents,'side',st=_ID,axial=(-h/2,F(0),h))
        base=2*n+4*i+1
        put('coedge',base,'generated',parents,'side_lower',st=_ID,axial=(-h/2,F(0),F(0)))
        put('coedge',base+2,'generated',parents,'side_upper',st=_ID,axial=(h/2,F(0),F(0)),orientation=-1)
        put('coedge',base+1,'generated',(('vertex',j+1),('edge',i+1)),'side_end',axial=(-h/2,h,F(0)))
        put('coedge',base+3,'generated',(('vertex',i+1),('edge',i+1)),'side_start',axial=(-h/2,h,F(0)),orientation=-1)
    history=result.history
    _need(type(history.origins) is tuple and len(history.origins)==14*n+6 and type(history.records) is tuple and len(history.records)==3*n+3,'complete history tables')
    actual={};forward=defaultdict(lambda: [[],[]])
    for origin in history.origins:
        target=_key(origin.target);_need(target in expected and target not in actual,'complete unique history targets')
        _need(type(origin.parents) is tuple,'owned history parents');parents=tuple(_key(p) for p in origin.parents)
        _need(type(origin.orientation) is int and origin.orientation in (-1,1),'typed history orientation')
        for label in ('local_translation','source_t','axial'):
            value=getattr(origin,label)
            if value is not None:_tuple(value,3,'history '+label)
        if origin.source_uv is not None:
            _need(type(origin.source_uv) is tuple and len(origin.source_uv)==2,'source UV affine rows')
            for row in origin.source_uv:_tuple(row,3,'source UV affine row')
        value=(origin.relation,parents,origin.role,origin.local_translation,origin.source_t,origin.source_uv,origin.axial,origin.orientation)
        _need(value==expected[target],'complete exact independent origin role/maps')
        _need(history.origin(origin.target) is origin,'actual origin lookup identity')
        actual[target]=origin
        for parent in parents:forward[parent][0 if origin.relation=='modified' else 1].append(target)
    inputs={_k(profile,kind,i+1) for kind,size in zip(_KINDS,sizes) for i in range(size)};seen=set()
    for record in history.records:
        source=_key(record.source);_need(source in inputs and source not in seen and record.status=='modified','complete original modified input roles');seen.add(source)
        _need(type(record.images) is tuple and type(record.generated) is tuple,'owned forward history links')
        images=tuple(_key(v) for v in record.images);generated=tuple(_key(v) for v in record.generated)
        _need(len(images)==2 and Counter(images)==Counter(forward[source][0]) and Counter(generated)==Counter(forward[source][1]),'whole reciprocal history images/generated')
        _need(history.from_input(record.source) is record,'actual source lookup identity')
    _need(seen==inputs,'all original six-kind entities dispositioned')
    return dict(history_inputs=3*n+3,history_outputs=14*n+6,modified=6*n+6,generated=8*n)


def _readers(result,points):
    solid,h=result.solid,result.height;n=len(points)
    center=tuple(sum(p[k] for p in points)/n for k in (0,1));tiny=F(1,1<<1074)
    probes=[(*center,F(0)),(*center,-h/2),(*center,h/2),(*center,-h/2-tiny),(*center,h/2+tiny),(*center,F(2))]
    for i,a in enumerate(points):
        b=points[(i+1)%n];mid=_blend(a,b,F(1,2));dx=b[0]-a[0];dy=b[1]-a[1]
        for xy in (a,mid):
            probes.extend((*xy,z) for z in (-h/2,F(0),h/2))
        for delta in (F(1,1<<30),tiny):
            probes.extend(((mid[0]-dy*delta,mid[1]+dx*delta,F(0)),(mid[0]+dy*delta,mid[1]-dx*delta,F(0))))
    for z in (-h/2,h/2):
        for direction in (-math.inf,math.inf):probes.append((*center,F(math.nextafter(float(z),direction))))
    for p in probes:
        q=_world(solid.frame,p);answer=solid.classify_point(q)
        _need(answer.solid is solid and type(answer.point) is tuple and answer.point==q and all(type(v) is F for v in answer.point),'actual exact point request binding')
        _need(type(answer.boundary_faces) is tuple and all(type(v) is int for v in answer.boundary_faces) and (answer.label,answer.boundary_faces)==_material(points,h,solid.frame,q),'complete actual material/support reader')
    uvs=[(F(0),F(0)),(F(1),F(1)),(F(1,2),F(1,2)),(-F(1,256),F(1,2)),(math.nan,.5),(math.inf,.5)]
    for i,a in enumerate(points):
        b=points[(i+1)%n]
        for t in (F(0),F(1,7),F(1,2)):
            uv=tuple((x+1)/2 for x in _blend(a,b,t));uvs.append(uv)
            uvs.append(tuple(float(x) for x in uv))
            uvs.extend((math.nextafter(float(uv[0]),d),float(uv[1])) for d in (-math.inf,math.inf))
    trimcount=0
    for slot in range(n+2):
        localuvs=uvs if slot<2 else ((0,0),(1,1),(F(1,7),F(2,3)),(0,F(1,3)),(1,F(1,3)),(F(1,3),0),(F(1,3),1),(-1,.5),(math.nan,.5))
        for u,v in localuvs:
            actual=solid.classify(slot,u,v);_need(type(actual) is int and actual==_trim(points,slot,u,v),'actual exact and actual-float trim reader');trimcount+=1
    vertices,edges,uses,words=_model(points,h);readerchecks=0
    for i,(a,b) in enumerate(edges):
        for t in (F(0),F(1,7),F(2,3),F(1)):
            actual=solid.edge_point(i+1,t);_tuple(actual,3,'exact public edge reader')
            _need(actual==_world(solid.frame,_blend(vertices[a],vertices[b],t)),'actual non-dyadic source edge reader');readerchecks+=1
    for i,(_,_,_,uv) in enumerate(uses):
        for t in (F(0),F(1,7),F(2,3),F(1)):
            actual=solid.coedge_uv(i+1,t);_tuple(actual,2,'exact public UV reader')
            _need(actual==_blend(uv[:2],uv[2:],t),'actual non-dyadic intrinsic coedge reader');readerchecks+=1
    return dict(material_queries=len(probes),trim_queries=trimcount,exact_reader_queries=readerchecks)


def qualify_profile(profile):
    points=_points(profile);_graph(profile,points)
    _need(type(profile.area) is F and profile.area==_area(points)>0,'exact original source area')
    probes=[(F(0),F(0)),(F(1),F(1)),(F(1,2),F(1,2)),(-1,.5),(math.nan,.5)]
    for i,a in enumerate(points):
        for t in (F(0),F(1,7),F(1,2)):
            uv=tuple((v+1)/2 for v in _blend(a,points[(i+1)%len(points)],t));probes.extend((uv,tuple(float(v) for v in uv)))
    for u,v in probes:
        actual=profile.classify(u,v);_need(type(actual) is int and actual==_trim(points,0,u,v),'actual original profile exact trim reader')
    return dict(profile_vertices=len(points),profile_area=profile.area,profile_trim_queries=len(probes))


def qualify_result(result):
    from .extrusion import ExtrusionResult,ExtrudedSolid
    _need(type(result) is ExtrusionResult and type(result.solid) is ExtrudedSolid,'actual factory result/solid')
    profile,solid,h=result.profile,result.solid,result.height;stats=qualify_profile(profile);points=_points(profile);n=len(points)
    _need(type(h) is F and F(1,4)<=h<=2 and (h*256).denominator==1,'exact selected height')
    _need(solid.profile is profile and type(solid.height) is F and solid.height==h and solid.frame is profile.frame,'actual retained profile/frame/height')
    _namespace(solid);_need(solid.body_id!=profile.body_id,'fresh output namespace')
    _graph(solid,points,h)
    _need(type(solid.volume) is F and solid.volume==_area(points)*h*F(profile.frame.scale)**3,'exact source product world volume')
    stats.update(_history(result,points));stats.update(_readers(result,points))
    _need(type(result.timings) is tuple and len({name for name,_ in result.timings})==len(result.timings) and all(type(name) is str and math.isfinite(ms) and ms>=0 for name,ms in result.timings),'construction phase timings')
    stats.update(vertices=2*n,edges=3*n,coedges=6*n,loops=n+2,faces=n+2,charts=n+2,genus=0,volume=solid.volume)
    return stats


def _normal_intervals(points,slot):
    if slot<2:
        return tuple((F(v),F(v)) for v in (0,0,-1 if slot==0 else 1))
    a=points[slot-2];b=points[(slot-1)%len(points)];dx=int((b[0]-a[0])*256);dy=int((b[1]-a[1])*256);m=dx*dx+dy*dy
    root=math.isqrt(m)
    if root*root==m:return tuple((F(v,root),F(v,root)) for v in (dy,-dx,0))
    k=math.isqrt(m<<128);low=F(k,1<<64);high=F(k+1,1<<64)
    return tuple(tuple(sorted((F(v)/low,F(v)/high))) for v in (dy,-dx,0))


def qualify_samples(batch):
    result=batch.result;profile=result.profile;points=_points(profile);solid=result.solid;h=result.height
    _need(batch.request.result is result and type(batch.rows) is tuple and type(batch.request.candidates) is tuple,'actual request/result ownership')
    _need(type(batch.backend) is str and batch.backend in ('cpu','cuda') and type(batch.device) is int and batch.device>=0,'observed sample placement')
    candidates=batch.request.candidates;_need(type(batch.count) is int and batch.count==len(candidates)==len(batch.rows),'complete native scatter rows')
    stats=dict(count=len(candidates),accepted=0,rejected=0,invalid=0,boundary=0,max_position_error=F(0),max_normal_error_bound=F(0));labels=[]
    for i,(candidate,row) in enumerate(zip(candidates,batch.rows)):
        slot=candidate.patch_slot
        _need(type(slot) is int and 0<=slot<len(points)+2 and type(candidate.u) is float and type(candidate.v) is float,'actual copied candidate types')
        _need(all(type(v) is int for v in (row.index,row.patch_slot,row.face_id,row.chart_id,row.trim,row.native_status)) and
              (row.index,row.patch_slot,row.face_id,row.chart_id)==(i,slot,slot+1,slot+1),'actual row source/scatter identity')
        _need(type(row.u) is float and type(row.v) is float and _bits(row.u)==_bits(candidate.u) and _bits(row.v)==_bits(candidate.v),'actual query UV bits')
        label=_trim(points,slot,candidate.u,candidate.v);labels.append(label);_need(row.trim==label,'independent exact actual-UV trim')
        _need(type(row.position) is tuple and type(row.normal) is tuple and len(row.position)==len(row.normal)==3,'immutable output vectors')
        if label in (0,3):
            _need(row.native_status==-1 and all(type(v) is float and math.isnan(v) for v in (*row.position,*row.normal)),'explicit rejected/invalid rows');stats['rejected' if label==0 else 'invalid']+=1;continue
        _need(row.native_status==0 and all(type(v) is float and math.isfinite(v) for v in (*row.position,*row.normal)),'finite admitted native output')
        expected=_world(profile.frame,_surface(points,h,slot,F(candidate.u),F(candidate.v)))
        for actual,wanted in zip(row.position,expected):
            error=abs(F(actual)-wanted);_need(error<=POSITION_LIMIT,'original source world position component limit');stats['max_position_error']=max(stats['max_position_error'],error)
        for actual,(low,high) in zip(row.normal,_normal_intervals(points,slot)):
            error=max(abs(F(actual)-low),abs(F(actual)-high));_need(error<=NORMAL_LIMIT,'independent exact sqrt interval unit-normal component limit');stats['max_normal_error_bound']=max(stats['max_normal_error_bound'],error)
        stats['accepted']+=1;stats['boundary']+=label==2
    _need(type(batch.request.labels) is tuple and all(type(v) is int for v in batch.request.labels) and batch.request.labels==tuple(labels),'actual complete labels')
    admitted=tuple(i for i,v in enumerate(labels) if v in (1,2));_ints(batch.request.admitted_indices,'actual gather')
    _need(batch.request.admitted_indices==admitted and type(batch.evaluated_count) is int and batch.evaluated_count==len(admitted),'actual gather/evaluation count')
    _need(type(batch.timings) is tuple and len({name for name,_ in batch.timings})==len(batch.timings) and all(type(name) is str and math.isfinite(ms) and ms>=0 for name,ms in batch.timings),'sample timings')
    return stats


def qualify_topology(result,actual):
    solid=result.solid;n=len(result.profile.vertices);source=actual.input
    _need(source.snapshots==() and (source.edge_count,source.face_count,source.coedge_count)==(3*n,n+2,6*n),'actual standalone source topology counts')
    def key(s):
        _need(all(type(v) is int for v in (s.instance,s.body_id,s.revision,s.entity_id)) and (s.instance,s.body_id,s.revision)==(0,solid.body_id,1),'typed actual topology lineage');return s.entity_id
    edgeids=tuple(map(key,source.edge_sources));faceids=tuple(map(key,source.face_sources));useids=tuple(map(key,source.coedge_sources))
    _need(set(edgeids)==set(range(1,3*n+1)) and set(faceids)==set(range(1,n+3)) and set(useids)==set(range(1,6*n+1)),'complete typed topology source cover')
    edges=struct.unpack('<'+str(6*n)+'I',source.coedge_edges);faces=struct.unpack('<'+str(6*n)+'I',source.coedge_faces);signs=struct.unpack('<'+str(6*n)+'i',source.orientations)
    groups=defaultdict(list)
    for slot,id in enumerate(useids):
        use=solid.coedges[id-1]
        _need((edgeids[edges[slot]],faceids[faces[slot]],signs[slot])==(use.edge_slot+1,use.face_slot+1,use.orientation),'actual ABI source incidence')
        groups[use.edge_slot+1].append(use)
    from .adjacency_oracles import qualify_result as qualify_adjacency
    stats=qualify_adjacency(actual)
    _need(stats['paired']==3*n and not any(stats[k] for k in ('unused','boundary','same_face_pair','orientation_conflict','multiple_uses')),'closed native topology kinds')
    for slot,id in enumerate(edgeids):
        _need(Counter(key(v) for v in actual.edge_uses(slot))==Counter(v.id for v in groups[id]) and actual.edge_kind(slot)==2,'actual typed edge-use/kind reader')
    for slot,id in enumerate(useids):
        current=solid.coedges[id-1];mate=next(v for v in groups[current.edge_slot+1] if v.id!=id)
        _need(key(actual.mate(slot))==mate.id and key(actual.neighbor(slot))==mate.face_slot+1,'actual mate/neighbor reader')
    return stats
