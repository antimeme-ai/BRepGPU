"""Independent original-meridian revolution truth and actual native consumers.

No production construction, material, trim, source-map or validation helper
provides expected answers. Circle identities use exact rational polynomials.
"""
from collections import Counter,defaultdict
from fractions import Fraction as F
import math
import struct
from .body_oracles import rational,add,mul,factor,radd,rscale,same

POSITION_LIMIT=F(1,1<<36)
NORMAL_LIMIT=F(1,1<<30)
_KINDS=('vertex','edge','coedge','loop','face','chart')
_TABLES=('vertices','edges','coedges','loops','faces','charts')
_Z=F(0);_O=F(1)
_U=(_Z,_O,_Z);_V=(_Z,_Z,_O)


def _need(ok,message):
    if not ok:raise AssertionError(message)


def _f(v):
    if type(v) is F:return v
    if type(v) is int:return F(v)
    if type(v) is float and math.isfinite(v):return F(v)
    raise ValueError('expected finite int/Fraction/binary64')


def _bits(v):return struct.pack('<d',v)


def _exact(values,n,label):
    _need(type(values) is tuple and len(values)==n and all(type(v) is F for v in values),label+' exact Fraction tuple')


def _slots(values,label):_need(type(values) is tuple and all(type(v) is int for v in values),label+' typed slot tuple')


def _namespace(obj):
    _need(type(obj.body_id) is int and 0<obj.body_id<1<<64 and type(obj.revision) is int and obj.revision==1,'typed actual namespace/revision')


def _points(profile):return tuple((v.position[0],v.position[1]) for v in profile.vertices)


def _rotate(x,y,q):return ((x,y),(-y,x),(-x,-y),(y,-x))[q]


def _circle(q,u):return _rotate((1-u*u)/(1+u*u),2*u/(1+u*u),q)


def _blend(a,b,t):return tuple(x+t*(y-x) for x,y in zip(a,b))


def _world(frame,p):return tuple(F(o)+F(frame.scale)*x for o,x in zip(frame.origin,p))


def _surface(points,slot,u,v):
    face,q=divmod(slot,4);r,a=_blend(points[face],points[(face+1)%len(points)],v);c,s=_circle(q,u)
    return (r*c,a,-r*s)


def _trim(slot,u,v):
    if type(u) is float and not math.isfinite(u) or type(v) is float and not math.isfinite(v):return 3
    u,v=_f(u),_f(v)
    if not 0<=u<=1 or not 0<=v<=1:return 3
    return 2 if v in (0,1) or slot%4==0 and u==0 or slot%4==3 and u==1 else 1


def _sign(v):return (v>0)-(v<0)


def _radical_sign(a,b,q):
    """Exact sign(a+b*sqrt(q)); never discard sign information by squaring."""
    _need(q>=0,'nonnegative squared radial coordinate')
    if q==0 or b==0:return _sign(a)
    if a==0:return _sign(b)
    if _sign(a)==_sign(b):return _sign(a)
    return _sign(a)*_sign(a*a-b*b*q)


def _material(points,frame,point):
    x,y,z=((_f(p)-F(o))/F(frame.scale) for p,o in zip(point,frame.origin));q=x*x+z*z
    signs=[]
    for i,(a,b) in enumerate(zip(points,points[1:]+points[:1])):
        dr,da=b[0]-a[0],b[1]-a[1]
        signs.append(_radical_sign(dr*(y-a[1])+da*a[0],-da,q))
    if any(s<0 for s in signs):return 'exterior',()
    faces=tuple(i+1 for i,s in enumerate(signs) if s==0)
    return ('boundary' if faces else 'interior'),faces


def qualify_profile(profile):
    from .extrusion_oracles import qualify_profile as original_profile
    stats=original_profile(profile) # Same actual source-profile claim, once.
    _need(all(r>=F(1,4) for r,a in _points(profile)),'strict positive radial clearance')
    stats['minimum_radius']=min(p[0] for p in _points(profile))
    return stats


def _model(points):
    n=len(points);vertices=tuple((r,a,F(0)) for r,a in points)
    edges=tuple((i,i) for i in range(n))+tuple((i,(i+1)%n) for i in range(n))
    uses=[]
    for i in range(n):
        uses.extend(((i,i,1,tuple(map(F,(0,0,4,0)))),(n+i,i,1,tuple(map(F,(4,0,4,1)))),
                     ((i+1)%n,i,-1,tuple(map(F,(0,1,4,1)))),(n+i,i,-1,tuple(map(F,(0,0,0,1))))))
    return vertices,edges,tuple(uses)


def _patches(solid,points):
    n=len(points);_need(type(solid.patches) is tuple and len(solid.patches)==4*n,'complete four-chart bands')
    for slot,(patch,chart) in enumerate(zip(solid.patches,solid.charts)):
        face,q=divmod(slot,4);controls=[]
        for x,z,w in ((1,0,1),(1,1,1),(0,2,2)):
            x,z=_rotate(x,z,q)
            for r,a in (points[face],points[(face+1)%n]):controls.extend((float(r*x),float(a*w),float(-r*z),float(w)))
        _need(all(type(v) is int for v in (patch.degree_u,patch.degree_v)) and (patch.degree_u,patch.degree_v)==(2,1),'native angular/meridian degree order')
        for label,actual,expected in (('controls',patch.controls,tuple(controls)),('source UV',patch.uv,(float(q),float(q+1),0.,1.)),('origin',patch.origin,solid.frame.origin)):
            _need(type(actual) is tuple and len(actual)==len(expected) and all(type(a) is float and _bits(a)==_bits(b) for a,b in zip(actual,expected)),'complete original exact native '+label+' bits')
        _need(type(patch.scale) is float and _bits(patch.scale)==_bits(solid.frame.scale),'original per-Patch scale bits')
        lin=patch.lineage
        _need(all(type(v) is int for v in (lin.body_id,lin.revision,lin.entity_kind,lin.entity_id,lin.chart_id)) and
              (lin.body_id,lin.revision,lin.entity_kind,lin.entity_id,lin.chart_id)==(solid.body_id,1,5,face+1,slot+1),'full fresh native lineage')
        _need(all(type(v) is int for v in (chart.face_slot,chart.quadrant)) and (chart.face_slot,chart.quadrant)==(face,q) and chart.patch is patch,'actual chart occurrence identity')


def _graph(solid,points):
    n=len(points);vertices,edges,uses=_model(points)
    for table,size in zip(_TABLES,(n,2*n,4*n,n,n,4*n)):
        actual=getattr(solid,table)
        _need(type(actual) is tuple and len(actual)==size and all(type(r.id) is int and r.id==i+1 for i,r in enumerate(actual)),'complete dense '+table)
    for vertex,expected in zip(solid.vertices,vertices):
        _exact(vertex.position,3,'actual anchor');_need(vertex.position==expected,'original theta-zero anchor embedding')
    _need(type(solid.curves) is tuple and len(solid.curves)==2*n,'whole physical curve table')
    for i,(edge,(a,b)) in enumerate(zip(solid.edges,edges)):
        _need(all(type(v) is int for v in (edge.start,edge.end,edge.curve_slot)) and (edge.start,edge.end,edge.curve_slot)==(a,b,i),'physical edge ownership/endpoint germs')
        curve=solid.curves[i];_exact(curve.data,2 if i<n else 6,'physical curve data')
        _need(curve.kind==('circle_y' if i<n else 'line') and curve.data==(points[i] if i<n else vertices[a]+vertices[b]),'exact source ring/seam curve')
    incidence=defaultdict(list);neighbors=defaultdict(set);links=defaultdict(list)
    for i,(actual,(edge,face,sign,uv)) in enumerate(zip(solid.coedges,uses)):
        _need(all(type(v) is int for v in (actual.edge_slot,actual.face_slot,actual.loop_slot,actual.orientation)) and (actual.edge_slot,actual.face_slot,actual.loop_slot,actual.orientation)==(edge,face,face,sign),'literal periodic oriented-use word')
        _exact(actual.uv_map.data,4,'intrinsic raw UV coefficients');_need(actual.uv_map.kind=='line' and actual.uv_map.data==uv,'whole intrinsic raw tau/v map')
        incidence[edge].append(actual)
    for i,(face,loop) in enumerate(zip(solid.faces,solid.loops)):
        _slots(face.loop_slots,'face loops');_slots(face.chart_slots,'face charts');_slots(loop.coedge_slots,'loop coedges')
        _need(type(face.orientation) is int and face.orientation==1 and face.kind=='revolved_line' and face.name=='revolved_'+str(i+1) and face.loop_slots==(i,) and face.chart_slots==tuple(range(4*i,4*i+4)),'whole source-band domain/ownership/orientation')
        _need(type(loop.face_slot) is int and loop.face_slot==i and type(loop.hole) is bool and not loop.hole and loop.coedge_slots==tuple(range(4*i,4*i+4)),'complete periodic rectangular word')
        for previous,current in zip(loop.coedge_slots[-1:]+loop.coedge_slots[:-1],loop.coedge_slots):
            pe,pf,ps,_=uses[previous];ce,cf,cs,_=uses[current];pend=1 if ps==1 else 0;cstart=0 if cs==1 else 1
            vertex=edges[pe][pend];_need(vertex==edges[ce][cstart],'oriented spatial loop closure')
            links[vertex].append(((pe,pend),(ce,cstart)))
    for es,members in incidence.items():
        _need(len(members)==2 and sum(c.orientation for c in members)==0,'two opposed physical uses')
        a,b=(c.face_slot for c in members);neighbors[a].add(b);neighbors[b].add(a)
        _need((a==b)==(es>=n),'same-face seam versus ordinary ring adjacency')
    found={0};todo=[0]
    while todo:
        for other in neighbors[todo.pop()]-found:found.add(other);todo.append(other)
    _need(found==set(range(n)),'single connected shell')
    for vertex in range(n):
        ports={(i,e) for i,edge in enumerate(edges) for e,v in enumerate(edge) if v==vertex};adj=defaultdict(list)
        for a,b in links[vertex]:adj[a].append(b);adj[b].append(a)
        _need(len(ports)==4 and set(adj)==ports and all(len(v)==2 for v in adj.values()),'all four closed-ring/seam endpoint germs')
        visited={next(iter(ports))};todo=list(visited)
        while todo:
            for other in set(adj[todo.pop()])-visited:visited.add(other);todo.append(other)
        _need(visited==ports,'single cyclic four-port vertex link')
    _need(n-2*n+n==0,'supporting genus-one Euler check');_patches(solid,points)


def _rmul(a,b):return mul(a[0],b[0]),mul(a[1],b[1])


def _circle_expression(q,s):
    ss=mul(s[0],s[0]);dd=mul(s[1],s[1]);den=add(dd,ss)
    x=(add(dd,factor(ss,-1)),den);y=(factor(mul(s[0],s[1]),2),den)
    return ((x,y),(rscale(y,-1),x),(rscale(x,-1),rscale(y,-1)),(y,rscale(x,-1)))[q]


def _maps(solid,points):
    n=len(points);checks=0
    for edge in solid.edges:
        es=edge.id-1;ring=es<n;curve=solid.curves[es]
        for q in range(4) if ring else (0,):
            parameter=rational((F(q,4),F(1,4))) if ring else rational((0,1))
            if ring:
                c,s=_circle_expression(q,rational((0,1)));r,a=points[es]
                spatial=(rscale(c,r),rational((a,)),rscale(s,-r))
            else:
                a,b=points[es-n],points[(es-n+1)%n]
                spatial=(rational((a[0],b[0]-a[0])),rational((a[1],b[1]-a[1])),rational((0,)))
            for use in (v for v in solid.coedges if v.edge_slot==es):
                d=use.uv_map.data;tau=radd(rscale(parameter,d[2]-d[0]),rational((d[0],)));v=radd(rscale(parameter,d[3]-d[1]),rational((d[1],)))
                quadrant=q if ring else (0 if d[0]==0 else 3)
                c,s=_circle_expression(quadrant,radd(tau,rational((-quadrant,))))
                a,b=points[use.face_slot],points[(use.face_slot+1)%n]
                r=radd(rscale(v,b[0]-a[0]),rational((a[0],)));axial=radd(rscale(v,b[1]-a[1]),rational((a[1],)))
                mapped=(_rmul(r,c),axial,rscale(_rmul(r,s),-1))
                _need(all(same(x,y) for x,y in zip(spatial,mapped)),'whole quarterwise curve/incident-UV/carrier rational identity')
        for t in (F(0),F(1,8),F(1,7),F(1,2),F(6,7),F(1)):
            if ring:
                q=min(int(4*t),3);c,s=_circle(q,4*t-q);r,a=points[es];point=(r*c,a,-r*s)
            else:
                r,a=_blend(points[es-n],points[(es-n+1)%n],t);point=(r,a,F(0))
            actual=solid.edge_point(edge.id,t);_exact(actual,3,'exact public edge reader')
            _need(actual==_world(solid.frame,point),'actual non-dyadic whole source edge reader');checks+=1
    for use in solid.coedges:
        d=use.uv_map.data
        for t in (F(0),F(1,7),F(2,3),F(1)):
            actual=solid.coedge_uv(use.id,t);_exact(actual,2,'exact public intrinsic UV reader')
            _need(actual==_blend(d[:2],d[2:],t),'actual exact raw coedge parameter reader');checks+=1
    return checks


def _key(key):
    from .boolean import EntityKey
    _need(type(key) is EntityKey and all(type(v) is int and 0<v<1<<64 for v in (key.body_id,key.revision,key.entity_id)) and type(key.kind) is str and key.kind in _KINDS,'typed exact history key')
    return key.body_id,key.revision,key.kind,key.entity_id


def _k(obj,kind,id):return obj.body_id,obj.revision,kind,id


def _history(result,points):
    profile,solid=result.profile,result.solid;n=len(points);expected={}
    def constant_uv(p):return tuple(((v+1)/2,F(0),F(0)) for v in p)
    def along(a,b,axis):return tuple(((x+1)/2,(y-x)/2 if axis==0 else F(0),(y-x)/2 if axis==1 else F(0)) for x,y in zip(a,b))
    def put(kind,id,relation,parents,role,st=None,suv=None,periodic=None,orientation=1):
        key=_k(solid,kind,id);_need(key not in expected,'unique independent history role')
        expected[key]=(relation,tuple(_k(profile,k,i) for k,i in parents),role,st,suv,periodic,orientation)
    for i,a in enumerate(points):
        j=(i+1)%n;b=points[j];uv=constant_uv(a);parents=(('edge',i+1),('coedge',i+1))
        put('vertex',i+1,'modified',(('vertex',i+1),),'anchor',suv=uv,periodic=(_Z,_Z,_Z))
        put('edge',n+i+1,'modified',(('edge',i+1),),'seam',_U,along(a,b,0),(_Z,_Z,_Z))
        put('edge',i+1,'generated',(('vertex',i+1),),'ring',suv=uv,periodic=(_Z,F(4),_Z))
        put('coedge',4*i+1,'generated',(('vertex',i+1),)+parents,'ring_start',(_Z,_Z,_Z),uv,(_Z,F(4),_Z))
        put('coedge',4*i+2,'modified',(('coedge',i+1),),'seam_end',_U,along(a,b,0),(F(4),_Z,_Z))
        put('coedge',4*i+3,'generated',(('vertex',j+1),)+parents,'ring_end',(_O,_Z,_Z),constant_uv(b),(_Z,F(4),_Z),-1)
        put('coedge',4*i+4,'modified',(('coedge',i+1),),'seam_start',_U,along(a,b,0),(_Z,_Z,_Z),-1)
        for kind in ('loop','face'):
            put(kind,i+1,'generated',parents+((kind,1),),'swept_'+kind,_V,along(a,b,1),_U)
        for q in range(4):put('chart',4*i+q+1,'generated',parents+(('chart',1),),'swept_chart',_V,along(a,b,1),(F(q),_O,_Z))
    history=result.history;_need(type(history.origins) is tuple and len(history.origins)==13*n and type(history.records) is tuple and len(history.records)==3*n+3,'whole six-kind history inventory')
    keys=tuple(_k(solid,kind,i+1) for kind,size in zip(_KINDS,(n,2*n,4*n,n,n,4*n)) for i in range(size))
    _need(tuple(_key(o.target) for o in history.origins)==keys,'origin order follows full six-kind output tables')
    forward=defaultdict(lambda:[[],[]])
    for origin in history.origins:
        target=_key(origin.target);_need(type(origin.parents) is tuple,'owned ordered history parents');parents=tuple(_key(k) for k in origin.parents)
        _need(type(origin.orientation) is int and type(origin.role) is str and type(origin.relation) is str,'typed exact history role/orientation')
        for label in ('source_t','periodic'):
            value=getattr(origin,label)
            if value is not None:_exact(value,3,'history '+label)
        if origin.source_uv is not None:
            _need(type(origin.source_uv) is tuple and len(origin.source_uv)==2,'source UV two rows')
            for row in origin.source_uv:_exact(row,3,'source UV affine row')
        _need((origin.relation,parents,origin.role,origin.source_t,origin.source_uv,origin.periodic,origin.orientation)==expected[target],'precise original generating IDs/roles/maps')
        _need(history.origin(origin.target) is origin,'actual reverse origin lookup identity')
        for parent in parents:forward[parent][0 if origin.relation=='modified' else 1].append(target)
    inputs=tuple(_k(profile,kind,i+1) for kind,size in zip(_KINDS,(n,n,n,1,1,1)) for i in range(size))
    _need(tuple(_key(r.source) for r in history.records)==inputs,'complete ordered original source inventory')
    for record in history.records:
        source=_key(record.source);_need(type(record.status) is str and record.status==('modified' if source[2] in ('vertex','edge','coedge') else 'consumed'),'explicit modified/consumed input status')
        _need(type(record.images) is tuple and type(record.generated) is tuple,'owned forward history tables')
        _need(tuple(_key(k) for k in record.images)==tuple(forward[source][0]) and tuple(_key(k) for k in record.generated)==tuple(forward[source][1]),'whole ordered reciprocal images/generated')
        _need(history.from_input(record.source) is record,'actual source lookup identity')
    return dict(history_inputs=3*n+3,history_outputs=13*n,modified=4*n,generated=9*n,modified_inputs=3*n,consumed=3)


def _readers(result,points):
    solid=result.solid;n=len(points);center=tuple(sum(p[k] for p in points)/n for k in (0,1));tiny=F(1,1<<1074)
    probes=[(F(0),center[1],F(0)),(F(1,8),center[1],F(0)),(center[0],center[1],F(0)),(F(2),center[1],F(0)),(F(1,2),center[1],F(1,4)),(F(1,4),center[1],F(1,8))]
    circles=((F(1),F(0)),(F(0),F(1)),(-F(1),F(0)),(F(0),-F(1)),(F(3,5),F(4,5)))
    for i,a in enumerate(points):
        b=points[(i+1)%n]
        for t in (F(0),F(1,7),F(1,2)):
            r,y=_blend(a,b,t);probes.extend((r*c,y,-r*s) for c,s in circles)
        r,y=_blend(a,b,F(1,2));dr,da=b[0]-a[0],b[1]-a[1]
        for delta in (F(1,1<<24),tiny):probes.extend(((r-da*delta,y+dr*delta,F(0)),(r+da*delta,y-dr*delta,F(0))))
        probes.extend((F(math.nextafter(float(r),d)),y,F(0)) for d in (-math.inf,math.inf))
    for local in probes:
        query=_world(solid.frame,local);answer=solid.classify_point(query)
        _need(answer.solid is solid and answer.point==query,'actual material source/query binding');_exact(answer.point,3,'actual exact material query')
        _slots(answer.boundary_faces,'all material supports')
        _need((answer.label,answer.boundary_faces)==_material(points,solid.frame,query),'exact sign-aware original material/all supports')
    uv=((0,0),(1,1),(F(1,7),F(2,3)),(0,F(1,3)),(1,F(1,3)),(F(1,3),0),(F(1,3),1),(-F(1,256),.5),(math.nan,.5),(math.inf,.5),(math.nextafter(0.,-math.inf),.5),(math.nextafter(1.,math.inf),.5))
    for slot in range(4*n):
        for u,v in uv:
            value=solid.classify(slot,u,v);_need(type(value) is int and value==_trim(slot,u,v),'actual normalized chart trim/seam reader')
    return dict(material_queries=len(probes),trim_queries=4*n*len(uv))


def qualify_result(result):
    from .revolution import RevolutionResult,RevolvedSolid
    _need(type(result) is RevolutionResult and type(result.solid) is RevolvedSolid,'actual factory result/solid')
    profile,solid=result.profile,result.solid;stats=qualify_profile(profile);points=_points(profile);n=len(points)
    _need(solid.profile is profile and solid.frame is profile.frame,'actual retained original profile/frame');_namespace(solid)
    _need(solid.body_id!=profile.body_id,'fresh output namespace');_graph(solid,points)
    coefficient=F(profile.frame.scale)**3*sum((a[0]+b[0])*(a[0]*b[1]-b[0]*a[1]) for a,b in zip(points,points[1:]+points[:1]))/3
    _need(type(solid.volume.rational) is F and type(solid.volume.pi) is F and solid.volume.rational==0 and solid.volume.pi==coefficient>0,'exact positive radial-first-moment world pi-volume')
    stats.update(_history(result,points));stats.update(_readers(result,points));stats['exact_reader_queries']=_maps(solid,points)
    _need(type(result.timings) is tuple and len({name for name,_ in result.timings})==len(result.timings) and all(type(name) is str and math.isfinite(ms) and ms>=0 for name,ms in result.timings),'construction timings')
    stats.update(vertices=n,edges=2*n,coedges=4*n,loops=n,faces=n,charts=4*n,genus=1,volume_pi=coefficient)
    return stats


def _normal_intervals(points,slot,u):
    face,q=divmod(slot,4);a,b=points[face],points[(face+1)%len(points)];dr=int((b[0]-a[0])*256);da=int((b[1]-a[1])*256)
    m=dr*dr+da*da;c,s=_circle(q,u);numerators=(da*c,F(-dr),-da*s);root=math.isqrt(m)
    if root*root==m:return tuple((v/root,v/root) for v in numerators)
    k=math.isqrt(m<<128);low=F(k,1<<64);high=F(k+1,1<<64)
    return tuple(tuple(sorted((v/low,v/high))) for v in numerators)


def qualify_samples(batch):
    result=batch.result;points=_points(result.profile);n=len(points);solid=result.solid
    _need(batch.request.result is result and type(batch.rows) is tuple and type(batch.request.candidates) is tuple,'actual sample request/result ownership')
    _need(type(batch.backend) is str and batch.backend in ('cpu','cuda') and type(batch.device) is int and batch.device>=0,'observed sample placement')
    candidates=batch.request.candidates;_need(type(batch.count) is int and batch.count==len(candidates)==len(batch.rows),'complete scatter row count')
    stats=dict(count=len(candidates),accepted=0,rejected=0,invalid=0,boundary=0,max_position_error=F(0),max_normal_error_bound=F(0));labels=[]
    for i,(candidate,row) in enumerate(zip(candidates,batch.rows)):
        slot=candidate.patch_slot;_need(type(slot) is int and 0<=slot<4*n and type(candidate.u) is float and type(candidate.v) is float,'actual copied query types')
        _need(all(type(v) is int for v in (row.index,row.patch_slot,row.face_id,row.chart_id,row.trim,row.native_status)) and (row.index,row.patch_slot,row.face_id,row.chart_id)==(i,slot,slot//4+1,slot+1),'actual source/scatter identity')
        _need(type(row.u) is float and type(row.v) is float and _bits(row.u)==_bits(candidate.u) and _bits(row.v)==_bits(candidate.v),'actual submitted normalized UV bits')
        label=_trim(slot,candidate.u,candidate.v);labels.append(label);_need(row.trim==label,'independent actual-parameter seam/trim label')
        _need(type(row.position) is tuple and type(row.normal) is tuple and len(row.position)==len(row.normal)==3,'immutable sample vectors')
        if label==3:
            _need(row.native_status==-1 and all(type(v) is float and math.isnan(v) for v in (*row.position,*row.normal)),'explicit invalid scatter');stats['invalid']+=1;continue
        _need(row.native_status==0 and all(type(v) is float and math.isfinite(v) for v in (*row.position,*row.normal)),'finite admitted native vectors')
        expected=_world(result.profile.frame,_surface(points,slot,F(candidate.u),F(candidate.v)))
        for actual,wanted in zip(row.position,expected):
            error=abs(F(actual)-wanted);_need(error<=POSITION_LIMIT,'direct original-source world position component budget');stats['max_position_error']=max(stats['max_position_error'],error)
        for actual,(low,high) in zip(row.normal,_normal_intervals(points,slot,F(candidate.u))):
            error=max(abs(F(actual)-low),abs(F(actual)-high));_need(error<=NORMAL_LIMIT,'independent sqrt-interval oriented normal component budget');stats['max_normal_error_bound']=max(stats['max_normal_error_bound'],error)
        stats['accepted']+=1;stats['boundary']+=label==2
    _slots(batch.request.labels,'complete request labels');_slots(batch.request.admitted_indices,'actual gather indices')
    _need(batch.request.labels==tuple(labels) and batch.request.admitted_indices==tuple(i for i,v in enumerate(labels) if v in (1,2)),'complete actual seam classification/gather')
    _need(type(batch.evaluated_count) is int and batch.evaluated_count==stats['accepted'],'actual native evaluation count')
    _need(type(batch.timings) is tuple and len({name for name,_ in batch.timings})==len(batch.timings) and all(type(name) is str and math.isfinite(ms) and ms>=0 for name,ms in batch.timings),'sample timings')
    return stats


def qualify_topology(result,actual):
    solid=result.solid;n=len(result.profile.vertices);source=actual.input
    _need(source.snapshots==() and (source.edge_count,source.face_count,source.coedge_count)==(2*n,n,4*n),'actual standalone periodic topology counts')
    def key(s):
        _need(all(type(v) is int for v in (s.instance,s.body_id,s.revision,s.entity_id)) and (s.instance,s.body_id,s.revision)==(0,solid.body_id,1),'typed actual topology namespace');return s.entity_id
    edgeids=tuple(map(key,source.edge_sources));faceids=tuple(map(key,source.face_sources));useids=tuple(map(key,source.coedge_sources))
    _need(set(edgeids)==set(range(1,2*n+1)) and set(faceids)==set(range(1,n+1)) and set(useids)==set(range(1,4*n+1)),'complete original topology entity cover')
    edges=struct.unpack('<'+str(4*n)+'I',source.coedge_edges);faces=struct.unpack('<'+str(4*n)+'I',source.coedge_faces);signs=struct.unpack('<'+str(4*n)+'i',source.orientations);groups=defaultdict(list)
    for slot,id in enumerate(useids):
        use=solid.coedges[id-1]
        _need((edgeids[edges[slot]],faceids[faces[slot]],signs[slot])==(use.edge_slot+1,use.face_slot+1,use.orientation),'actual native source incidence')
        groups[use.edge_slot+1].append(use)
    from .adjacency_oracles import qualify_result as qualify_adjacency
    stats=qualify_adjacency(actual)
    _need(stats['paired']==n and stats['same_face_pair']==n and not any(stats[k] for k in ('unused','boundary','orientation_conflict','multiple_uses')),'closed ring pairs and same-face seam multiplicity')
    for slot,id in enumerate(edgeids):
        _need(Counter(key(v) for v in actual.edge_uses(slot))==Counter(v.id for v in groups[id]) and actual.edge_kind(slot)==(2 if id<=n else 3),'actual typed ring/seam edge readers')
    for slot,id in enumerate(useids):
        current=solid.coedges[id-1];mate=next(v for v in groups[current.edge_slot+1] if v.id!=id)
        _need(key(actual.mate(slot))==mate.id and key(actual.neighbor(slot))==mate.face_slot+1,'actual typed periodic mate/neighbor readers')
    return stats
