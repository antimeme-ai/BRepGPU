"""Independent complete signed-cylinder geometry, source feet and native truth.

Expected answers derive only from actual original canonical source, distance and
Frame. No production offset/classification/map/validation helper is an oracle.
"""
from collections import Counter,defaultdict
from fractions import Fraction as F
import math
import struct
from .body_oracles import rational,radd,rscale,quarter_rational,same,add,mul,factor

POSITION_LIMIT=F(1,1<<36)
NORMAL_LIMIT=F(1,1<<30)
_KINDS=('vertex','edge','coedge','loop','face','chart')
_TABLES=('vertices','edges','coedges','loops','faces','charts')
_SIZES=(2,3,6,3,3,6)
_Z=F(0);_O=F(1)
_UNIT=(_Z,_O,_Z,_O)
_U=(_Z,_O,_Z)
_USES=((1,0,1,'circle_plane'),(0,1,-1,'circle_plane'),(0,2,1,'circle_wall'),(2,2,1,'seam_wall'),(1,2,-1,'circle_wall'),(2,2,-1,'seam_wall'))


def _need(ok,message):
    if not ok:raise AssertionError(message)


def _f(v):
    if type(v) is F:return v
    if type(v) is int:return F(v)
    if type(v) is float and math.isfinite(v):return F(v)
    raise ValueError('expected finite int/Fraction/binary64')


def _bits(v):return struct.pack('<d',v)


def _exact(values,n,label):_need(type(values) is tuple and len(values)==n and all(type(v) is F for v in values),label+' Fraction tuple')


def _ints(values,label):_need(type(values) is tuple and all(type(v) is int for v in values),label+' typed integer tuple')


def _floats(actual,expected,label):_need(type(actual) is tuple and len(actual)==len(expected) and all(type(a) is float and _bits(a)==_bits(b) for a,b in zip(actual,expected)),label+' binary64 bits')


def _key(key):
    from .boolean import EntityKey
    _need(type(key) is EntityKey and all(type(v) is int and 0<v<1<<64 for v in (key.body_id,key.revision,key.entity_id)) and type(key.kind) is str and key.kind in _KINDS,'strict actual entity key')
    return key.body_id,key.revision,key.kind,key.entity_id


def _k(obj,kind,id):return obj.body_id,obj.revision,kind,id


def _rotate(x,y,q):return ((x,y),(-y,x),(-x,-y),(y,-x))[q]


def _circle(q,u):return _rotate((1-u*u)/(1+u*u),2*u/(1+u*u),q)


def _full_circle(t):
    q=min(int(4*t),3)
    return _circle(q,4*t-q)


def _world(frame,p):return tuple(F(o)+F(frame.scale)*x for o,x in zip(frame.origin,p))


def _surface(R,H,slot,u,v):
    if slot<2:return (R*(2*u-1),R*(2*v-1),H if slot==0 else -H)
    c,s=_circle(slot-2,u)
    return R*c,R*s,H*(2*v-1)


def _normal(slot,u):return tuple(map(F,(0,0,1 if slot==0 else -1))) if slot<2 else (*_circle(slot-2,u),_Z)


def _trim(slot,u,v):
    if type(u) is float and not math.isfinite(u) or type(v) is float and not math.isfinite(v):return 3
    u,v=_f(u),_f(v)
    if not 0<=u<=1 or not 0<=v<=1:return 3
    if slot<2:
        radial=(2*u-1)**2+(2*v-1)**2
        return 0 if radial>1 else 2 if radial==1 else 1
    return 2 if v in (0,1) or slot==2 and u==0 or slot==5 and u==1 else 1


def _material(R,H,frame,point):
    x,y,z=((_f(p)-F(o))/F(frame.scale) for p,o in zip(point,frame.origin));rr=x*x+y*y
    if rr>R*R or abs(z)>H:return 'exterior',()
    faces=tuple(i for i,on in ((1,z==H),(2,z==-H),(3,rr==R*R)) if on)
    return ('boundary' if faces else 'interior'),faces


def _inputs(result):
    from .offset import OffsetResult,OffsetSolid
    from .body import BodySnapshot,Frame
    _need(type(result) is OffsetResult and type(result.solid) is OffsetSolid,'actual factory offset result/solid')
    source=result.source;solid=result.solid;d=result.distance
    _need(type(source) is BodySnapshot and type(source.shape) is str and source.shape=='cylinder' and type(source.frame) is Frame,'actual canonical cylinder source')
    _need(type(source.body_id) is int and 0<source.body_id<1<<64 and type(source.revision) is int and source.revision==1,'strict source namespace')
    _floats(source.frame.origin,source.frame.origin,'original Frame origin')
    _need(len(source.frame.origin)==3 and all(math.isfinite(v) and abs(v)<=16 for v in source.frame.origin) and type(source.frame.scale) is float and source.frame.scale in (.25,.5,1.,2.,4.),'strict admitted Frame')
    for table in _TABLES:
        values=getattr(source,table);_need(type(values) is tuple and all(type(v.id) is int for v in values),'strict original entity table')
    for i,vertex in enumerate(source.vertices):_floats(vertex.position,(.25,0.,-.5 if i==0 else .5),'original anchor')
    _need(type(source.curves) is tuple and type(source.patches) is tuple,'original immutable carrier tables')
    for i,curve in enumerate(source.curves):_floats(curve.data,(.25,-.5 if i==0 else .5) if i<2 else (.25,0.,-.5,.25,0.,.5),'original curve')
    for edge in source.edges:_need(all(type(v) is int for v in (edge.start,edge.end,edge.curve_slot)),'original edge slots')
    for use in source.coedges:
        _need(all(type(v) is int for v in (use.edge_slot,use.face_slot,use.loop_slot,use.orientation)),'original use slots/sense');_floats(use.uv_map.data,use.uv_map.data,'original raw UV')
    for loop in source.loops:
        _need(type(loop.face_slot) is int and type(loop.hole) is bool,'original loop metadata');_ints(loop.coedge_slots,'original loop word')
    for face in source.faces:
        _need(type(face.orientation) is int,'original face sense');_ints(face.loop_slots,'original face loops');_ints(face.chart_slots,'original face charts')
    for chart in source.charts:_need(all(type(v) is int for v in (chart.face_slot,chart.quadrant)),'original chart occurrence')
    for slot,p in enumerate(source.patches):
        _need(type(p.degree_u) is int and type(p.degree_v) is int and type(p.scale) is float,'original native scalars')
        for name in ('controls','uv','origin'):_floats(getattr(p,name),getattr(p,name),'original native '+name)
        l=p.lineage;_need(all(type(v) is int for v in (l.body_id,l.revision,l.entity_kind,l.entity_id,l.chart_id)),'original native identity')
        # Original factory rotates binary64 numerators, so signed zero bits
        # differ from the new exact-Fraction control construction at some q.
        if slot<2:
            expected=tuple(float(x) for u in (-.25,.25) for v in (-.25,.25) for x in (u,v,.5 if slot==0 else -.5,1.))
        else:
            expected=[]
            for x,y,w in ((.25,0.,1.),(.25,.25,1.),(0.,.5,2.)):
                x,y=_rotate(x,y,slot-2)
                for z in (-.5,.5):expected.extend((x,y,z*w,w))
            expected=tuple(expected)
        _floats(p.controls,expected,'canonical original control including signed zero')
    from .body_oracles import qualify_snapshot
    qualify_snapshot(source) # Same unchanged original-source claim, once.
    _need(type(d) is F and d!=0 and -F(1,8)<=d<=F(1,8) and (256*d).denominator==1,'exact nonzero signed distance')
    _need(solid.source is source and solid.frame is source.frame and type(solid.distance) is F and solid.distance==d and type(result.join) is str and type(solid.join) is str and result.join==solid.join=='intersection','actual source/distance/join binding')
    _need(type(solid.body_id) is int and 0<solid.body_id<1<<64 and solid.body_id!=source.body_id and type(solid.revision) is int and solid.revision==1,'fresh actual namespace')
    return F(1,4)+d,F(1,2)+d


def _patches(result,R,H):
    solid=result.solid
    _need(type(solid.patches) is tuple and len(solid.patches)==6,'complete actual carrier set')
    for slot,(p,chart) in enumerate(zip(solid.patches,solid.charts)):
        controls=[]
        if slot<2:
            for u in (0,1):
                for v in (0,1):controls.extend((*map(float,_surface(R,H,slot,F(u),F(v))),1.))
        else:
            for a,b,w in ((1,0,1),(1,1,1),(0,2,2)):
                a,b=_rotate(a,b,slot-2)
                for z in (-H,H):controls.extend((float(R*a),float(R*b),float(z*w),float(w)))
        face=slot if slot<2 else 2;q=-1 if slot<2 else slot-2
        _need(all(type(v) is int for v in (p.degree_u,p.degree_v)) and (p.degree_u,p.degree_v)==((1,1) if slot<2 else (2,1)),'native degree and ordering')
        _floats(p.controls,tuple(controls),'whole exact native control');_floats(p.uv,result.source.patches[slot].uv,'original raw domain');_floats(p.origin,result.source.frame.origin,'actual original origin')
        _need(type(p.scale) is float and _bits(p.scale)==_bits(result.source.frame.scale),'actual original scale bits')
        lin=p.lineage
        _need(all(type(v) is int for v in (lin.body_id,lin.revision,lin.entity_kind,lin.entity_id,lin.chart_id)) and (lin.body_id,lin.revision,lin.entity_kind,lin.entity_id,lin.chart_id)==(solid.body_id,1,5,face+1,slot+1),'complete fresh native source identity')
        _need(all(type(v) is int for v in (chart.face_slot,chart.quadrant)) and (chart.face_slot,chart.quadrant)==(face,q) and chart.patch is p,'actual chart carrier occurrence')


def _graph(result,R,H):
    solid=result.solid
    for table,size in zip(_TABLES,_SIZES):
        values=getattr(solid,table);_need(type(values) is tuple and len(values)==size and all(type(v.id) is int and v.id==i+1 for i,v in enumerate(values)),'complete dense '+table)
    vertices=((R,_Z,-H),(R,_Z,H));edges=((0,0),(1,1),(0,1));curves=(('circle',(R,-H)),('circle',(R,H)),('line',vertices[0]+vertices[1]))
    for v,p in zip(solid.vertices,vertices):_exact(v.position,3,'anchor');_need(v.position==p,'whole anchor embedding')
    _need(type(solid.curves) is tuple and len(solid.curves)==3,'complete physical curves')
    for i,(e,ends,spec) in enumerate(zip(solid.edges,edges,curves)):
        _need(all(type(v) is int for v in (e.start,e.end,e.curve_slot)) and (e.start,e.end,e.curve_slot)==(*ends,i),'closed-ring endpoint ports/physical curve owner')
        curve=solid.curves[i];_exact(curve.data,len(spec[1]),'physical curve');_need(type(curve.kind) is str and (curve.kind,curve.data)==spec,'whole physical circle/seam')
    uvdata=((R,R),(R,R),(_Z,),(F(4),),(_O,),(_Z,));groups=defaultdict(list)
    for i,(c,(edge,face,sign,kind),data) in enumerate(zip(solid.coedges,_USES,uvdata)):
        _need(all(type(v) is int for v in (c.edge_slot,c.face_slot,c.loop_slot,c.orientation)) and (c.edge_slot,c.face_slot,c.loop_slot,c.orientation)==(edge,face,face,sign),'literal complete use occurrence')
        _exact(c.uv_map.data,len(data),'intrinsic boundary UV');_need(type(c.uv_map.kind) is str and (c.uv_map.kind,c.uv_map.data)==(kind,data),'complete incident UV descriptor');groups[edge].append(c)
    words=((0,),(1,),(2,3,4,5));links=defaultdict(list)
    for i,(face,loop,word) in enumerate(zip(solid.faces,solid.loops,words)):
        _ints(face.loop_slots,'face loops');_ints(face.chart_slots,'face charts');_ints(loop.coedge_slots,'loop word')
        _need(type(face.orientation) is int and (face.name,face.kind,face.orientation,face.loop_slots,face.chart_slots)==(('top','bottom','wall')[i],('disk','disk','cylinder')[i],(1,-1,1)[i],(i,),(i,) if i<2 else (2,3,4,5)),'complete oriented face/domain coverage')
        _need(type(loop.face_slot) is int and type(loop.hole) is bool and (loop.face_slot,loop.hole,loop.coedge_slots)==(i,False,word),'whole owned boundary word')
        for previous,current in zip(word[-1:]+word[:-1],word):
            pe,pf,ps,_=_USES[previous];ce,cf,cs,_=_USES[current];pend=1 if ps==1 else 0;start=0 if cs==1 else 1
            vertex=edges[pe][pend];_need(vertex==edges[ce][start],'oriented loop closure');links[vertex].append(((pe,pend),(ce,start)))
    for edge,members in groups.items():_need(len(members)==2 and sum(v.orientation for v in members)==0 and (members[0].face_slot==members[1].face_slot)==(edge==2),'ordinary versus same-face opposed pair')
    # Each ring has two distinct germs even at its single physical anchor.
    for vertex in range(2):
        ports={(e,end) for e,pair in enumerate(edges) for end,v in enumerate(pair) if v==vertex};adj=defaultdict(list)
        for a,b in links[vertex]:adj[a].append(b);adj[b].append(a)
        _need(len(ports)==3 and set(adj)==ports and all(len(v)==2 for v in adj.values()),'complete three-port manifold anchor link')
        seen={next(iter(ports))};todo=list(seen)
        while todo:
            for other in set(adj[todo.pop()])-seen:seen.add(other);todo.append(other)
        _need(seen==ports,'single cyclic anchor link')
    # Literal graph connects top->wall->bottom; complete carrier domains prove embedding.
    _patches(result,R,H)


def _map_spec(result,kind,id):
    d=result.distance;scale=1+4*d;wallscale=1+2*d
    affine=lambda a,b,c,e,f,g:('affine',tuple(map(F,(a,b,c,e,f,g))),_UNIT)
    cap=affine(-2*d,scale,0,-2*d,0,scale)
    if kind=='vertex' or kind=='edge':return None,None
    if kind=='coedge':
        owner=('face',_USES[id-1][1]+1)
        if id<=2:return owner,('circle',(scale,),_UNIT)
        if id in (3,5):return owner,affine(0,4,0,-d if id==3 else 1+d,0,0)
        return owner,affine(4 if id==4 else 0,0,0,-d,wallscale,0)
    if kind=='chart':return ('chart',id),cap if id<=2 else affine(id-3,1,0,-d,0,wallscale)
    if id<=2:return ('face',id),cap
    return ('face',3),('affine',tuple(map(F,(0,1,0,-d,0,wallscale))),(_Z,F(4),_Z,_O))


def _map_value(spec,u,v):
    kind,data,domain=spec
    if kind=='affine':return data[0]+data[1]*u+data[2]*v,data[3]+data[4]*u+data[5]*v
    x,y=_full_circle(u);return (1+data[0]*x)/2,(1+data[0]*y)/2


def _check_map(mapping,spec):
    from .offset import UVMap2
    kind,data,domain=spec
    _need(type(mapping) is UVMap2 and type(mapping.kind) is str and mapping.kind==kind,'actual source map kind')
    _exact(mapping.data,len(data),'source map coefficients');_exact(mapping.domain,4,'source map input domain')
    _need((mapping.data,mapping.domain)==(data,domain),'whole normal-foot source map coefficient/domain identity')
    count=0
    for u,v in ((domain[0],domain[2]),(domain[1],domain[3]),((domain[0]+domain[1])/2,(domain[2]+domain[3])/2),((6*domain[0]+domain[1])/7,(domain[2]+2*domain[3])/3)):
        actual=mapping.evaluate(u,v);_exact(actual,2,'actual source map reader');_need(actual==_map_value(spec,u,v),'actual exact non-dyadic source map evaluation');count+=1
    return count


def _history(result):
    source,solid=result.source,result.solid;history=result.history;sourcekeys=tuple(_k(source,k,i+1) for k,size in zip(_KINDS,_SIZES) for i in range(size));targetkeys=tuple(_k(solid,k,i+1) for k,size in zip(_KINDS,_SIZES) for i in range(size))
    _need(type(history.records) is tuple and type(history.origins) is tuple and len(history.records)==len(history.origins)==23,'complete source/output history')
    _need(tuple(_key(r.source) for r in history.records)==sourcekeys and tuple(_key(o.target) for o in history.origins)==targetkeys,'ordered complete actual source inventories');mapchecks=0
    for record,origin,sk,tk in zip(history.records,history.origins,sourcekeys,targetkeys):
        kind,id=sk[2:];_need(type(record.status) is str and record.status=='modified' and type(record.images) is tuple and tuple(map(_key,record.images))==(tk,) and type(record.generated) is tuple and record.generated==(),'complete known modified source fate')
        _need(type(origin.parents) is tuple and tuple(map(_key,origin.parents))==(sk,) and type(origin.relation) is str and origin.relation=='modified' and type(origin.role) is str and origin.role=='source_'+kind,'precise same-kind causal parent')
        sense=1
        if kind=='coedge':sense=_USES[id-1][2]
        if kind in ('face','loop'):sense=-1 if id==2 else 1
        if kind=='chart':sense=-1 if id==2 else 1
        _need(type(origin.orientation) is int and origin.orientation==sense,'absolute target orientation, no duplicated traversal')
        if kind in ('edge','coedge'):
            _exact(origin.source_t,3,'intrinsic corresponding parameter');_need((_key(origin.source_t_owner),origin.source_t_relation,origin.source_t)==(sk,'boundary_parameter',_U),'physical corresponding parameter owner/relation')
        else:_need(origin.source_t_owner is None and origin.source_t_relation is None and origin.source_t is None,'no invented boundary parameter')
        owner,spec=_map_spec(result,kind,id)
        if owner is None:_need(origin.source_uv_owner is None and origin.source_uv is None and origin.source_uv_relation is None,'no invented normal-foot UV')
        else:
            _need(_key(origin.source_uv_owner)==_k(source,*owner) and type(origin.source_uv_relation) is str and origin.source_uv_relation=='normal_foot','actual contextual original carrier/UV owner');mapchecks+=_check_map(origin.source_uv,spec)
        _need(history.from_input(record.source) is record and history.origin(origin.target) is origin,'actual forward/reverse lookup identity')
    return dict(history_inputs=23,history_outputs=23,modified=23,retained=0,deleted=0,generated=0,history_map_queries=mapchecks)


def _edge(R,H,id,t):
    if id==3:return R,_Z,H*(2*t-1)
    c,s=_full_circle(t);return R*c,R*s,H if id==2 else -H


def _uv(id,t):
    if id<=2:
        c,s=_full_circle(t);return (1+c)/2,(1+s)/2
    if id in (3,5):return 4*t,F(id==5)
    return F(4 if id==4 else 0),t


def _maps(result,R,H):
    solid=result.solid;checks=0
    for id in range(1,4):
        for t in (F(0),F(1,8),F(1,7),F(1,2),F(6,7),F(1)):
            actual=solid.edge_point(id,t);_exact(actual,3,'actual exact curve reader');_need(actual==_world(solid.frame,_edge(R,H,id,t)),'whole physical edge reader');checks+=1
    for id in range(1,7):
        for t in (F(0),F(1,7),F(2,3),F(1)):
            actual=solid.coedge_uv(id,t);_exact(actual,2,'actual exact intrinsic UV reader');_need(actual==_uv(id,t),'actual raw coedge reader');checks+=1
        # Whole rational equality, using actual curve/map coefficients whose
        # schemas/payloads have already been bound to this original source/d.
        use=solid.coedges[id-1];edge=use.edge_slot
        for q in range(4) if edge<2 else (0,):
            c,s=quarter_rational(q);zero=rational((0,));one=rational((1,));t=rational((F(q,4),F(1,4))) if edge<2 else rational((0,1))
            spatial=(rscale(c,R),rscale(s,R),rational((H if edge==1 else -H,))) if edge<2 else (rational((R,)),zero,rational((-H,2*H)))
            if id<=2:
                u,v=(radd(rscale(a,F(1,2)),rational((F(1,2),))) for a in (c,s))
                mapped=(radd(rscale(u,2*R),rational((-R,))),radd(rscale(v,2*R),rational((-R,))),rational((H if id==1 else -H,)))
            else:
                x,y=(c,s) if edge<2 else (one,zero)
                axial=rational((H if id==5 else -H,)) if edge<2 else radd(rscale(t,2*H),rational((-H,)))
                mapped=rscale(x,R),rscale(y,R),axial
            _need(all(same(a,b) for a,b in zip(spatial,mapped)),'whole quarterwise curve/both-UV/carrier identity')
    return checks


def _foot_expected(result,R,H,slot,u,v):
    d=result.distance;p=_surface(R,H,slot,u,v);normal=_normal(slot,u)
    uv=(_Z,_Z)
    if slot<2:
        uv=(-2*d+(1+4*d)*u,-2*d+(1+4*d)*v);sourcepoint=(p[0],p[1],F(1,2) if slot==0 else -F(1,2))
        inside=all(0<=a<=1 for a in uv);label=_trim(slot,*uv)
    else:
        q=slot-2;uv=(F(q)+u,-d+(1+2*d)*v);c,s=_circle(q,u);sourcepoint=c/4,s/4,p[2]
        inside=F(q)<=uv[0]<=q+1 and 0<=uv[1]<=1;label=_trim(slot,u,uv[1])
    return uv,_world(result.source.frame,sourcepoint),_world(result.source.frame,p),normal,inside,label


def _feet(result,R,H):
    from .offset import Footpoint
    solid=result.solid;queries=((F(1,2),F(1,2)),(F(3,4),F(3,4)),(F(1),F(1,2)),(F(0),F(1,2)),(F(1,2),F(0)),(F(1,2),F(1)),(F(1,7),F(2,3)),(F(4,5),F(9,10)))
    count=0
    for slot in range(6):
        for u,v in queries:
            if _trim(slot,u,v) not in (1,2):continue
            foot=solid.source_foot(slot,u,v);uv,sp,p,n,inside,label=_foot_expected(result,R,H,slot,u,v)
            _need(type(foot) is Footpoint and foot.solid is solid and type(foot.chart_id) is int and foot.chart_id==slot+1,'actual foot owner/chart')
            _need(type(foot.u) is F and type(foot.v) is F and (foot.u,foot.v)==(u,v),'actual foot query binding')
            face=slot+1 if slot<2 else 3
            _need((_key(foot.source_face),_key(foot.source_chart))==(_k(result.source,'face',face),_k(result.source,'chart',slot+1)),'actual original foot face/chart ownership')
            for labelname,actual,expected in (('source UV',foot.source_uv,uv),('source carrier point',foot.source_point,sp),('target point',foot.point,p),('oriented unit normal',foot.normal,n)):
                _exact(actual,len(expected),'foot '+labelname);_need(actual==expected,'independent exact foot '+labelname)
            distance=result.distance*F(result.source.frame.scale)
            _need(type(foot.distance) is F and foot.distance==distance and type(foot.in_source_square) is bool and foot.in_source_square==inside and type(foot.source_trim) is int and foot.source_trim==label,'signed world foot distance and exact original effective domain')
            _need(sum(c*c for c in foot.normal)==1 and foot.point==tuple(a+distance*b for a,b in zip(foot.source_point,foot.normal)),'whole signed normal-foot identity');count+=1
    return count


def _readers(result,R,H):
    frame=result.source.frame;solid=result.solid;tiny=F(1,1<<1074);probes=[(_Z,_Z,_Z),(R,R,_Z),(_Z,_Z,H+F(1,256)),(F(1,4),_Z,F(1,2))]
    for c,s in ((_O,_Z),(_Z,_O),(-_O,_Z),(_Z,-_O),(F(3,5),F(4,5))):
        for z in (-H,_Z,H):probes.extend(((R*c,R*s,z),(R*c/2,R*s/2,z)))
        for delta in (-tiny,tiny,-F(1,1<<24),F(1,1<<24)):probes.append(((R+delta)*c,(R+delta)*s,_Z))
    for z in (-H,H):
        for direction in (-math.inf,math.inf):probes.append((_Z,_Z,F(math.nextafter(float(z),direction))))
    for local in probes:
        query=_world(frame,local);answer=solid.classify_point(query)
        _need(answer.solid is solid,'actual material owner');_exact(answer.point,3,'exact material query');_need(answer.point==query,'actual original material query binding');_ints(answer.boundary_faces,'complete support IDs')
        _need(type(answer.label) is str and (answer.label,answer.boundary_faces)==_material(R,H,frame,query),'complete material/support strata')
    uv=((0,0),(1,1),(.5,.5),(0,.5),(1,.5),(.5,0),(.5,1),(F(4,5),F(9,10)),(float(F(4,5)),float(F(9,10))),(F(1,7),F(2,3)),(math.nextafter(.8,0.),.9),(math.nextafter(.8,1.),.9),(-1,.5),(math.nan,.5),(math.inf,.5),(math.nextafter(0.,-math.inf),.5),(math.nextafter(1.,math.inf),.5))
    for slot in range(6):
        for u,v in uv:
            actual=solid.classify(slot,u,v);_need(type(actual) is int and actual==_trim(slot,u,v),'actual exact normalized disk/wall/rim/seam trim')
    return dict(material_queries=len(probes),trim_queries=6*len(uv))


def _timings(values,label):_need(type(values) is tuple and len({name for name,_ in values})==len(values) and all(type(name) is str and math.isfinite(ms) and ms>=0 for name,ms in values),label+' timings')


def qualify_result(result):
    R,H=_inputs(result);_graph(result,R,H);stats=_history(result);stats.update(_readers(result,R,H));stats['exact_reader_queries']=_maps(result,R,H);stats['source_foot_queries']=_feet(result,R,H)
    volume=result.solid.volume;coefficient=2*H*R*R*F(result.source.frame.scale)**3
    _need(type(volume.rational) is F and type(volume.pi) is F and (volume.rational,volume.pi)==(_Z,coefficient),'exact world signed-offset volume coefficient')
    _timings(result.timings,'construction');stats.update(vertices=2,edges=3,coedges=6,loops=3,faces=3,charts=6,genus=0,volume_rational=_Z,volume_pi=coefficient)
    return stats


def qualify_samples(batch):
    from .offset_sampling import OffsetSamples,OffsetRequest
    _need(type(batch) is OffsetSamples and type(batch.request) is OffsetRequest,'actual offset batch/request')
    result=batch.result;R,H=_inputs(result);_patches(result,R,H)
    _need(batch.request.result is result and type(batch.rows) is tuple and type(batch.request.candidates) is tuple,'actual request/source ownership')
    _need(type(batch.backend) is str and batch.backend in ('cpu','cuda') and type(batch.device) is int and batch.device>=0,'observed native placement')
    candidates=batch.request.candidates;_need(type(batch.count) is int and batch.count==len(candidates)==len(batch.rows),'complete scatter rows')
    stats=dict(count=len(candidates),accepted=0,rejected=0,invalid=0,boundary=0,max_position_error=_Z,max_normal_error=_Z);labels=[]
    for i,(candidate,row) in enumerate(zip(candidates,batch.rows)):
        slot=candidate.patch_slot;_need(type(slot) is int and 0<=slot<6 and type(candidate.u) is float and type(candidate.v) is float,'actual copied native query')
        face=slot+1 if slot<2 else 3
        _need(all(type(v) is int for v in (row.index,row.patch_slot,row.face_id,row.chart_id,row.trim,row.native_status)) and (row.index,row.patch_slot,row.face_id,row.chart_id)==(i,slot,face,slot+1),'actual scatter/typed source identity')
        _need(type(row.u) is float and type(row.v) is float and _bits(row.u)==_bits(candidate.u) and _bits(row.v)==_bits(candidate.v),'actual submitted binary64 UV bits')
        label=_trim(slot,candidate.u,candidate.v);labels.append(label);_need(row.trim==label,'independent copied-float trim admission')
        _need(type(row.position) is tuple and type(row.normal) is tuple and len(row.position)==len(row.normal)==3,'immutable vector rows')
        if label in (0,3):
            _need(row.native_status==-1 and all(type(v) is float and math.isnan(v) for v in (*row.position,*row.normal)),'explicit nonadmitted rows');stats['rejected' if label==0 else 'invalid']+=1;continue
        _need(row.native_status==0 and all(type(v) is float and math.isfinite(v) for v in (*row.position,*row.normal)),'finite successful native rows')
        u,v=F(candidate.u),F(candidate.v);point=_world(result.source.frame,_surface(R,H,slot,u,v));normal=_normal(slot,u)
        for name,actual,expected,limit in (('position',row.position,point,POSITION_LIMIT),('normal',row.normal,normal,NORMAL_LIMIT)):
            for a,b in zip(actual,expected):
                error=abs(F(a)-b);_need(error<=limit,'direct exact actual-UV world '+name+' component bound');stats['max_'+name+'_error']=max(stats['max_'+name+'_error'],error)
        stats['accepted']+=1;stats['boundary']+=label==2
    _ints(batch.request.labels,'copied labels');_ints(batch.request.admitted_indices,'actual gather')
    _need(batch.request.labels==tuple(labels) and batch.request.admitted_indices==tuple(i for i,v in enumerate(labels) if v in (1,2)),'complete actual-float admission/gather')
    _need(type(batch.evaluated_count) is int and batch.evaluated_count==stats['accepted'],'native evaluated count');_timings(batch.timings,'sample')
    return stats


def qualify_topology(result,actual):
    source=actual.input;solid=result.solid
    _need(source.snapshots==() and (source.edge_count,source.face_count,source.coedge_count)==(3,3,6),'actual standalone topology cover')
    def key(v):
        _need(all(type(c) is int for c in (v.instance,v.body_id,v.revision,v.entity_id)) and (v.instance,v.body_id,v.revision)==(0,solid.body_id,1),'actual typed topology identity');return v.entity_id
    edgeids=tuple(map(key,source.edge_sources));faceids=tuple(map(key,source.face_sources));useids=tuple(map(key,source.coedge_sources))
    _need(set(edgeids)=={1,2,3} and set(faceids)=={1,2,3} and set(useids)==set(range(1,7)),'full independent topology identity cover')
    edges=struct.unpack('<6I',source.coedge_edges);faces=struct.unpack('<6I',source.coedge_faces);signs=struct.unpack('<6i',source.orientations);groups=defaultdict(list)
    for slot,id in enumerate(useids):
        edge,face,sign,kind=_USES[id-1];_need((edgeids[edges[slot]],faceids[faces[slot]],signs[slot])==(edge+1,face+1,sign),'actual packed source incidence');groups[edge+1].append((id,face+1))
    from .adjacency_oracles import qualify_result as qualify_adjacency
    stats=qualify_adjacency(actual)
    _need((stats['paired'],stats['same_face_pair'])==(2,1) and not any(stats[k] for k in ('unused','boundary','orientation_conflict','multiple_uses')),'closed ordinary rings and same-face seam')
    for slot,id in enumerate(edgeids):_need(Counter(key(v) for v in actual.edge_uses(slot))==Counter(v[0] for v in groups[id]) and actual.edge_kind(slot)==(3 if id==3 else 2),'actual typed edge membership/kind')
    for slot,id in enumerate(useids):
        edge=_USES[id-1][0]+1;mate=next(v for v in groups[edge] if v[0]!=id)
        _need(key(actual.mate(slot))==mate[0] and key(actual.neighbor(slot))==mate[1],'actual typed opposed mate/neighbor readers')
    return stats
