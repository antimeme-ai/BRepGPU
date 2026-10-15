"""Independent complete rounded-box material, source maps and native truth.

Original canonical source/key/radius/Frame determine all expected geometry and
ancestry. No production fillet/trim/material/Map2/validation helper is an oracle.
"""
from collections import Counter,defaultdict
from fractions import Fraction as F
import math
import struct
from .body_oracles import rational,add,mul,factor,radd,rscale,same,value

POSITION_LIMIT=F(1,1<<36)
NORMAL_LIMIT=F(1,1<<30)
_KINDS=('vertex','edge','coedge','loop','face','chart')
_TABLES=('vertices','edges','coedges','loops','faces','charts')
_NORMALS=((0,0,1),(0,0,-1),(0,-1,0),(0,1,0),(1,0,0),(-1,0,0))
_A=F(3,4);_W=F(3,2)
_ID_U=(F(0),F(1),F(0));_ID_V=(F(0),F(0),F(1))
_ID_UV=('affine',(F(0),F(1),F(0),F(0),F(0),F(1)))


def _need(ok,message):
    if not ok:raise AssertionError(message)


def _f(v):
    if type(v) is F:return v
    if type(v) is int:return F(v)
    if type(v) is float and math.isfinite(v):return F(v)
    raise ValueError('expected finite exact numeric value')


def _bits(v):return struct.pack('<d',v)


def _exact(values,n,label):
    _need(type(values) is tuple and len(values)==n and all(type(v) is F for v in values),label+' exact Fraction tuple')


def _ints(values,label):_need(type(values) is tuple and all(type(v) is int for v in values),label+' typed integer tuple')


def _key(key):
    from .boolean import EntityKey
    _need(type(key) is EntityKey and all(type(v) is int and 0<v<1<<64 for v in (key.body_id,key.revision,key.entity_id)) and type(key.kind) is str and key.kind in _KINDS,'typed actual entity key')
    return key.body_id,key.revision,key.kind,key.entity_id


def _k(obj,kind,id):return obj.body_id,obj.revision,kind,id


def _rotate(x,y,q):return ((x,y),(-y,x),(-x,-y),(y,-x))[q]


def _circle(q,u):return _rotate((1-u*u)/(1+u*u),2*u/(1+u*u),q)


def _blend(a,b,t):return tuple(x+t*(y-x) for x,y in zip(a,b))


def _world(frame,p):return tuple(F(o)+F(frame.scale)*v for o,v in zip(frame.origin,p))


def _plane(slot,u,v):
    x,y,z=_W*u-_A,_W*v-_A,v-F(1,2)
    return ((x,y,F(1,2)),(x,y,-F(1,2)),(x,-_A,z),(x,_A,z),(_A,x,z),(-_A,x,z))[slot]


def _plane_uv(slot,p):
    return ((p[0]+_A)/_W,(p[1]+_A)/_W) if slot<2 else (((p[0] if slot<4 else p[1])+_A)/_W,p[2]+F(1,2))


def _surface(m,slot,u,v):
    if slot<6:return _plane(slot,u,v)
    x,y=_circle(m['q'],u)
    return (m['center'][0]+m['r']*x,m['center'][1]+m['r']*y,v-F(1,2))


def _section(m,x,y):
    p,q=m['sx']*x,m['sy']*y;c=m['c'];r=m['r']
    return p<=c or q<=c or (p-c)**2+(q-c)**2<=r*r


def _on_band(m,x,y):
    p,q=m['sx']*x,m['sy']*y;c=m['c']
    return p>=c and q>=c and (p-c)**2+(q-c)**2==m['r']**2


def _material(m,point):
    x,y,z=((_f(p)-F(o))/F(m['source'].frame.scale) for p,o in zip(point,m['source'].frame.origin))
    if abs(x)>_A or abs(y)>_A or abs(z)>F(1,2) or not _section(m,x,y):return 'exterior',()
    supports=((1,z==F(1,2)),(2,z==-F(1,2)),(3,y==-_A),(4,y==_A),(5,x==_A),(6,x==-_A),(7,_on_band(m,x,y)))
    faces=tuple(id for id,on in supports if on)
    return ('boundary' if faces else 'interior'),faces


def _trim(m,slot,u,v):
    if type(u) is float and not math.isfinite(u) or type(v) is float and not math.isfinite(v):return 3
    u,v=_f(u),_f(v)
    if not 0<=u<=1 or not 0<=v<=1:return 3
    if slot==6:return 2 if u in (0,1) or v in (0,1) else 1
    x,y,z=_plane(slot,u,v)
    if not _section(m,x,y):return 0
    return 2 if u in (0,1) or v in (0,1) or _on_band(m,x,y) else 1


def _model(result):
    source=result.source;r=result.radius;k=result.edge.entity_id-9;q=(k+2)%4;c=_A-r;center=_rotate(c,c,q)
    sx,sy=((-1,-1),(1,-1),(1,1),(-1,1))[k]
    incoming_normal=_rotate(1,0,q);outgoing_normal=_rotate(0,1,q)
    support=lambda n: { (0,-1):2,(0,1):3,(1,0):4,(-1,0):5}[n]
    sidefaces=(support(incoming_normal),support(outgoing_normal))
    deleted_vertices=(k,k+4);selected=k+8
    vmap={old:i for i,old in enumerate(v for v in range(8) if v not in deleted_vertices)}
    vertices=[tuple(map(_f,source.vertices[old].position)) for old in vmap]
    contacts=tuple((center[0]+r*n[0],center[1]+r*n[1]) for n in (incoming_normal,outgoing_normal))
    vertices.extend((*p,z) for z in (-F(1,2),F(1,2)) for p in contacts)
    emap={old:i for i,old in enumerate(es for es in range(12) if es!=selected)};ranges={};edges=[];curves=[];erel={}
    for old,out in emap.items():
        edge=source.edges[old];t0=F(2,3)*r if edge.start in deleted_vertices else F(0);t1=1-F(2,3)*r if edge.end in deleted_vertices else F(1)
        ranges[old]=(t0,t1);erel[old]='retained' if (t0,t1)==(0,1) else 'modified'
        start=vmap[edge.start] if edge.start not in deleted_vertices else 6+2*(edge.start//4)+1
        end=vmap[edge.end] if edge.end not in deleted_vertices else 6+2*(edge.end//4)
        edges.append((start,end));curves.append(('line',vertices[start]+vertices[end]))
    edges.extend(((6,7),(8,9),(6,8),(7,9)))
    curves.extend((('quarter_arc',(center[0],center[1],r,q,-F(1,2))),('quarter_arc',(center[0],center[1],r,q,F(1,2))),('line',vertices[6]+vertices[8]),('line',vertices[7]+vertices[9])))
    deleted_uses=tuple(i for i,u in enumerate(source.coedges) if u.edge_slot==selected)
    cmap={old:i for i,old in enumerate(cs for cs in range(24) if cs not in deleted_uses)};uses=[];crel={}
    for old,out in cmap.items():
        use=source.coedges[old];t0,t1=ranges[use.edge_slot];d=tuple(map(_f,use.uv_map.data));uv=_blend(d[:2],d[2:],t0)+_blend(d[:2],d[2:],t1)
        crel[old]=erel[use.edge_slot];uses.append((emap[use.edge_slot],use.face_slot,use.orientation,'line',uv))
    arcuv=((center[0]+_A)/_W,(center[1]+_A)/_W,r/_W,q)
    uses.extend(((11,1,-1,'arc',arcuv),(12,0,1,'arc',arcuv)))
    sideuses=[]
    for contact,face in enumerate(sidefaces):
        old=next(cs for cs in deleted_uses if source.coedges[cs].face_slot==face);sideuses.append(old)
        uv=_plane_uv(face,vertices[6+contact])+_plane_uv(face,vertices[8+contact])
        uses.append((13+contact,face,source.coedges[old].orientation,'line',uv))
    uses.extend(((11,6,1,'line',tuple(map(F,(0,0,1,0)))),(14,6,1,'line',tuple(map(F,(1,0,1,1)))),(12,6,-1,'line',tuple(map(F,(0,1,1,1)))),(13,6,-1,'line',tuple(map(F,(0,0,0,1))))))
    words=[]
    for fs in range(6):
        word=[]
        for old in source.loops[fs].coedge_slots:
            use=source.coedges[old]
            if old in deleted_uses:word.append(24+sideuses.index(old));continue
            word.append(cmap[old])
            edge=source.edges[use.edge_slot];end=edge.end if use.orientation==1 else edge.start
            if fs<2 and end in deleted_vertices:word.append(23 if fs==0 else 22)
        words.append(tuple(word))
    words.append((26,27,28,29))
    return dict(source=source,r=r,k=k,q=q,c=c,center=center,sx=sx,sy=sy,sidefaces=sidefaces,sideuses=tuple(sideuses),normals=(incoming_normal,outgoing_normal),vmap=vmap,emap=emap,cmap=cmap,ranges=ranges,erel=erel,crel=crel,vertices=tuple(vertices),edges=tuple(edges),curves=tuple(curves),uses=tuple(uses),words=tuple(words),changedfaces={0,1,*sidefaces})


def _inputs(result):
    from .body import BodySnapshot,Frame
    from .fillet import FilletResult,FilletedSolid
    _need(type(result) is FilletResult and type(result.solid) is FilletedSolid,'actual factory fillet result/solid')
    source=result.source;_need(type(source) is BodySnapshot and source.shape=='box' and type(source.frame) is Frame,'actual canonical original source/frame')
    _need(type(source.shape) is str and type(source.body_id) is int and 0<source.body_id<1<<64 and type(source.revision) is int and source.revision==1,'strict original namespace/revision')
    _float_tuple(source.frame.origin,source.frame.origin,'original Frame origin')
    _need(len(source.frame.origin)==3 and all(math.isfinite(v) and abs(v)<=16 for v in source.frame.origin) and type(source.frame.scale) is float and source.frame.scale in (.25,.5,1.,2.,4.),'strict original Frame bounds/types')
    for table in _TABLES:
        records=getattr(source,table);_need(type(records) is tuple and all(type(v.id) is int for v in records),'strict original typed entity tables')
    for vertex in source.vertices:_float_tuple(vertex.position,vertex.position,'original source vertex')
    _need(type(source.curves) is tuple and type(source.patches) is tuple,'original immutable curve/carrier tables')
    for curve in source.curves:_float_tuple(curve.data,curve.data,'original source curve')
    for edge in source.edges:_need(all(type(v) is int for v in (edge.start,edge.end,edge.curve_slot)),'strict original edge slots')
    for use in source.coedges:
        _need(all(type(v) is int for v in (use.edge_slot,use.face_slot,use.loop_slot,use.orientation)),'strict original use ownership')
        _float_tuple(use.uv_map.data,use.uv_map.data,'original source use UV')
    for loop in source.loops:
        _need(type(loop.face_slot) is int and type(loop.hole) is bool,'strict original loop metadata');_ints(loop.coedge_slots,'original source word')
    for face in source.faces:
        _need(type(face.orientation) is int,'strict original face sense');_ints(face.loop_slots,'original source face loops');_ints(face.chart_slots,'original source face charts')
    for chart in source.charts:_need(all(type(v) is int for v in (chart.face_slot,chart.quadrant)),'strict original chart metadata')
    for patch in source.patches:
        _need(type(patch.degree_u) is int and type(patch.degree_v) is int and type(patch.scale) is float,'strict original native scalars')
        for name in ('controls','uv','origin'):_float_tuple(getattr(patch,name),getattr(patch,name),'original native '+name)
        lin=patch.lineage;_need(all(type(v) is int for v in (lin.body_id,lin.revision,lin.entity_kind,lin.entity_id,lin.chart_id)),'strict original native lineage')
    _need(_key(result.edge)==(source.body_id,source.revision,'edge',result.edge.entity_id) and 9<=result.edge.entity_id<=12,'actual selected original vertical edge')
    _need(type(result.radius) is F and F(1,16)<=result.radius<=F(1,2) and (result.radius*256).denominator==1,'exact radius family')
    _need(result.solid.source is source and result.solid.edge is result.edge and type(result.solid.radius) is F and result.solid.radius==result.radius and result.solid.frame is source.frame,'retained actual source/key/radius/frame binding')
    _need(type(result.solid.body_id) is int and 0<result.solid.body_id<1<<64 and result.solid.body_id!=source.body_id and type(result.solid.revision) is int and result.solid.revision==1,'fresh typed output namespace')
    from .body_oracles import qualify_snapshot
    qualify_snapshot(source) # The unchanged canonical source claim, once.
    return _model(result)


def _float_tuple(actual,expected,label):
    _need(type(actual) is tuple and len(actual)==len(expected) and all(type(a) is float and _bits(a)==_bits(b) for a,b in zip(actual,expected)),label+' retained original float bits')


def _patches(result,m):
    solid,source=result.solid,result.source;_need(type(solid.patches) is tuple and len(solid.patches)==7,'whole carrier inventory')
    for slot,(patch,chart) in enumerate(zip(solid.patches,solid.charts)):
        if slot<6:expected=source.patches[slot].controls
        else:
            controls=[]
            for x,y,w in ((1,0,1),(1,1,1),(0,2,2)):
                x,y=_rotate(x,y,m['q'])
                for z in (-F(1,2),F(1,2)):controls.extend((float(m['center'][0]*w+m['r']*x),float(m['center'][1]*w+m['r']*y),float(z*w),float(w)))
            expected=tuple(controls)
        _need(all(type(v) is int for v in (patch.degree_u,patch.degree_v)) and (patch.degree_u,patch.degree_v)==((1,1) if slot<6 else (2,1)),'actual native degrees')
        _float_tuple(patch.controls,expected,'carrier controls');_float_tuple(patch.origin,source.frame.origin,'carrier origin');_float_tuple(patch.uv,(0.,1.,0.,1.),'original raw UV')
        _need(type(patch.scale) is float and _bits(patch.scale)==_bits(source.frame.scale),'actual per-Patch source scale')
        lin=patch.lineage;_need(all(type(v) is int for v in (lin.body_id,lin.revision,lin.entity_kind,lin.entity_id,lin.chart_id)) and (lin.body_id,lin.revision,lin.entity_kind,lin.entity_id,lin.chart_id)==(solid.body_id,1,5,slot+1,slot+1),'fresh full native Lineage')
        _need(all(type(v) is int for v in (chart.face_slot,chart.quadrant)) and (chart.face_slot,chart.quadrant)==(slot,-1 if slot<6 else m['q']) and chart.patch is patch,'actual carrier chart occurrence')


def _graph(result,m):
    solid=result.solid;source=result.source
    for table,size in zip(_TABLES,(10,15,30,7,7,7)):
        records=getattr(solid,table);_need(type(records) is tuple and len(records)==size and all(type(r.id) is int and r.id==i+1 for i,r in enumerate(records)),'complete dense '+table)
    oldvertex={out:old for old,out in m['vmap'].items()}
    for i,(vertex,expected) in enumerate(zip(solid.vertices,m['vertices'])):
        _need(tuple(map(_f,vertex.position))==expected,'source/tangent vertex exact position')
        if i in oldvertex:_float_tuple(vertex.position,source.vertices[oldvertex[i]].position,'original vertex')
        else:_exact(vertex.position,3,'generated tangent vertex')
    _need(type(solid.curves) is tuple and len(solid.curves)==15,'whole physical curve inventory')
    oldedge={out:old for old,out in m['emap'].items()}
    for i,(edge,expected,curvewanted) in enumerate(zip(solid.edges,m['edges'],m['curves'])):
        _need(all(type(v) is int for v in (edge.start,edge.end,edge.curve_slot)) and (edge.start,edge.end)==expected and 0<=edge.curve_slot<15,'physical source/created edge endpoints')
        curve=solid.curves[edge.curve_slot];kind,data=curvewanted
        _need(curve.kind==kind and type(curve.data) is tuple and len(curve.data)==len(data) and tuple(map(_f,curve.data))==tuple(map(_f,data)),'whole exact physical curve coefficients')
        if i in oldedge and m['erel'][oldedge[i]]=='retained':_float_tuple(curve.data,source.curves[source.edges[oldedge[i]].curve_slot].data,'original curve')
        elif kind=='line':_exact(curve.data,6,'modified/new line')
        else:_need(all(type(curve.data[j]) is F for j in (0,1,2,4)) and type(curve.data[3]) is int,'typed exact quarter arc')
    _need(Counter(e.curve_slot for e in solid.edges)==Counter(range(15)),'complete unique curve ownership')
    oldcoedge={out:old for old,out in m['cmap'].items()};incidence=defaultdict(list);links=defaultdict(list);neighbors=defaultdict(set)
    for i,(use,wanted) in enumerate(zip(solid.coedges,m['uses'])):
        edge,face,sign,kind,data=wanted
        _need(all(type(v) is int for v in (use.edge_slot,use.face_slot,use.loop_slot,use.orientation)) and (use.edge_slot,use.face_slot,use.loop_slot,use.orientation)==(edge,face,face,sign),'complete literal source/new coedge word')
        actual=use.uv_map;_need(actual.kind==kind and type(actual.data) is tuple and len(actual.data)==len(data) and tuple(map(_f,actual.data))==tuple(map(_f,data)),'whole intrinsic incident UV coefficients')
        if i in oldcoedge and m['crel'][oldcoedge[i]]=='retained':_float_tuple(actual.data,source.coedges[oldcoedge[i]].uv_map.data,'original UV')
        elif kind=='line':_exact(actual.data,4,'modified/new line UV')
        else:_need(all(type(actual.data[j]) is F for j in (0,1,2)) and type(actual.data[3]) is int,'typed exact arc UV')
        incidence[edge].append(use)
    for fs,(face,loop,word) in enumerate(zip(solid.faces,solid.loops,m['words'])):
        _ints(face.loop_slots,'face loops');_ints(face.chart_slots,'face charts');_ints(loop.coedge_slots,'loop coedges')
        sense=source.faces[fs].orientation if fs<6 else 1;kind='fillet_cap' if fs<2 else 'rectangle' if fs<6 else 'fillet_band';name=source.faces[fs].name if fs<6 else 'fillet'
        _need(type(face.orientation) is int and (face.orientation,face.kind,face.name,face.loop_slots,face.chart_slots)==(sense,kind,name,(fs,),(fs,)),'complete face source domain/absolute sense')
        _need(type(loop.face_slot) is int and loop.face_slot==fs and type(loop.hole) is bool and not loop.hole and loop.coedge_slots==word,'whole rounded cap/side/band loop domain')
        for previous,current in zip(word[-1:]+word[:-1],word):
            p=solid.coedges[previous];c=solid.coedges[current];pe=solid.edges[p.edge_slot];ce=solid.edges[c.edge_slot];pend=1 if p.orientation==1 else 0;cstart=0 if c.orientation==1 else 1
            vertex=(pe.start,pe.end)[pend];_need(vertex==(ce.start,ce.end)[cstart],'oriented source-ID loop closure')
            links[vertex].append(((p.edge_slot,pend),(c.edge_slot,cstart)))
    for members in incidence.values():
        _need(len(members)==2 and sum(c.orientation for c in members)==0 and members[0].face_slot!=members[1].face_slot,'opposed ordinary physical edge uses')
        a,b=(c.face_slot for c in members);neighbors[a].add(b);neighbors[b].add(a)
    found={0};todo=[0]
    while todo:
        for other in neighbors[todo.pop()]-found:found.add(other);todo.append(other)
    _need(found==set(range(7)),'single connected shell')
    for vs in range(10):
        ports={(i,end) for i,e in enumerate(solid.edges) for end,v in enumerate((e.start,e.end)) if v==vs};adj=defaultdict(list)
        for a,b in links[vs]:adj[a].append(b);adj[b].append(a)
        _need(len(ports)==3 and set(adj)==ports and all(len(v)==2 for v in adj.values()),'full three-port vertex link')
        found={next(iter(ports))};todo=list(found)
        while todo:
            for other in set(adj[todo.pop()])-found:found.add(other);todo.append(other)
        _need(found==ports,'single cyclic vertex link')
    _need(10-15+7==2,'supporting genus-zero Euler check');_patches(result,m)


def _map_value(spec,u,v):
    kind,d=spec
    if kind=='affine':return d[0]+d[1]*u+d[2]*v,d[3]+d[4]*u+d[5]*v
    c,s=_circle(d[3],u);return d[0]+d[2]*c,d[1]+d[2]*s


def _check_map(actual,spec,label):
    from .fillet import Map2
    if spec is None:_need(actual is None,label+' absent map');return
    kind,data=spec
    _need(type(actual) is Map2 and actual.kind==kind and type(actual.data) is tuple and len(actual.data)==len(data) and actual.data==data,label+' complete map descriptor')
    if kind=='affine':_exact(actual.data,6,label)
    else:_need(all(type(actual.data[j]) is F for j in (0,1,2)) and type(actual.data[3]) is int and 0<=actual.data[3]<4,label+' exact arc metadata')
    for u,v in ((F(0),F(0)),(F(1,7),F(2,3)),(F(1),F(1))):
        observed=actual.evaluate(u,v);_exact(observed,2,label+' actual public map read')
        _need(observed==_map_value(spec,u,v),label+' actual map value at original exact arguments')


def _affine_uv(endpoints):
    a,b=endpoints[:2],endpoints[2:]
    return ('affine',(a[0],b[0]-a[0],F(0),a[1],b[1]-a[1],F(0)))


def _constant_uv(point):return ('affine',(point[0],F(0),F(0),point[1],F(0),F(0)))


def _history(result,m):
    source,solid=result.source,result.solid;expected={}
    def put(kind,id,relation,parents,role,towner=None,trelation=None,t=None,uvowner=None,uv=None,orientation=1):
        target=_k(solid,kind,id);_need(target not in expected,'unique independent origin role')
        expected[target]=(relation,tuple(_k(source,k,i) for k,i in parents),role,None if towner is None else _k(source,*towner),trelation,t,None if uvowner is None else _k(source,*uvowner),uv,orientation)
    for old,out in m['vmap'].items():put('vertex',out+1,'retained',(('vertex',old+1),),'source_vertex')
    for old,out in m['emap'].items():
        t0,t1=m['ranges'][old]
        put('edge',out+1,m['erel'][old],(('edge',old+1),),'source_edge',('edge',old+1),'coincident',(t0,t1-t0,F(0)))
    for old,out in m['cmap'].items():
        use=source.coedges[old];t0,t1=m['ranges'][use.edge_slot];uv=m['uses'][out][4]
        put('coedge',out+1,m['crel'][old],(('coedge',old+1),),'source_coedge',('coedge',old+1),'coincident',(t0,t1-t0,F(0)),('face',use.face_slot+1),_affine_uv(uv),use.orientation)
    for kind in ('loop','face','chart'):
        for fs in range(6):
            put(kind,fs+1,'modified' if fs in m['changedfaces'] else 'retained',((kind,fs+1),),'source_'+kind,uvowner=(('face',fs+1) if kind=='loop' else (kind,fs+1)),uv=_ID_UV,orientation=source.faces[fs].orientation)
    selected=('edge',result.edge.entity_id)
    for level in (0,1):
        endpoint=m['k']+4*level+1;cap=2 if level==0 else 1;prefix='lower' if level==0 else 'upper'
        for contact,name in enumerate(('incoming','outgoing')):
            face=m['sidefaces'][contact]+1;vertex=6+2*level+contact
            put('vertex',vertex+1,'generated',(('vertex',endpoint),selected,('face',face)),prefix+'_'+name+'_vertex',selected,'spine',(F(level),F(0),F(0)),('face',face),_constant_uv(_plane_uv(face-1,m['vertices'][vertex])))
        arcuv=('arc',m['uses'][22][4]);parents=(selected,('vertex',endpoint),('face',cap));t=(F(level),F(0),F(0))
        put('edge',12+level,'generated',parents,prefix+'_arc',selected,'spine',t,('face',cap),arcuv)
        put('coedge',23+level,'generated',parents,prefix+'_cap_arc',selected,'spine',t,('face',cap),arcuv,-1 if level==0 else 1)
        put('coedge',27+2*level,'generated',parents,'band_'+prefix+'_arc',selected,'spine',t,('face',cap),arcuv,1 if level==0 else -1)
    for contact,name in enumerate(('incoming','outgoing')):
        face=m['sidefaces'][contact]+1;oldcoedge=m['sideuses'][contact]+1;uv=_affine_uv(m['uses'][24+contact][4])
        put('edge',14+contact,'generated',(selected,('face',face)),name+'_rail',selected,'spine',_ID_U,('face',face),uv)
        parents=(selected,('coedge',oldcoedge),('face',face))
        put('coedge',25+contact,'generated',parents,name+'_side_rail',selected,'spine',_ID_U,('face',face),uv,source.coedges[oldcoedge-1].orientation)
        put('coedge',30 if contact==0 else 28,'generated',parents,'band_'+name+'_rail',selected,'spine',_ID_U,('face',face),uv,-1 if contact==0 else 1)
    for kind in ('loop','face','chart'):
        put(kind,7,'generated',(selected,('face',m['sidefaces'][0]+1),('face',m['sidefaces'][1]+1)),'band_'+kind,selected,'spine',_ID_V)
    history=result.history;_need(type(history.records) is tuple and len(history.records)==62 and type(history.origins) is tuple and len(history.origins)==76,'whole six-kind history inventory')
    outputkeys=tuple(_k(solid,kind,i+1) for kind,size in zip(_KINDS,(10,15,30,7,7,7)) for i in range(size))
    _need(tuple(_key(o.target) for o in history.origins)==outputkeys,'whole six-kind ordered output origins')
    forward=defaultdict(lambda:[[],[]])
    for origin in history.origins:
        target=_key(origin.target);_need(type(origin.parents) is tuple,'owned history parents');parents=tuple(_key(p) for p in origin.parents)
        _need(type(origin.orientation) is int and type(origin.role) is str and type(origin.relation) is str,'typed absolute target history orientation/role')
        if origin.source_t is not None:_exact(origin.source_t,3,'history intrinsic source t')
        t_owner=None if origin.source_t_owner is None else _key(origin.source_t_owner);uv_owner=None if origin.source_uv_owner is None else _key(origin.source_uv_owner)
        wanted=expected[target]
        _need((origin.relation,parents,origin.role,t_owner,origin.source_t_relation,origin.source_t,uv_owner,origin.orientation)==wanted[:7]+(wanted[8],),'precise original causal/map-owner/role/parameter/sense')
        _check_map(origin.source_uv,wanted[7],'history original source UV')
        _need(history.origin(origin.target) is origin,'actual reverse history lookup identity')
        for parent in parents:forward[parent][1 if origin.relation=='generated' else 0].append(target)
    inputkeys=tuple(_k(source,kind,r.id) for kind,table in zip(_KINDS,_TABLES) for r in getattr(source,table))
    _need(tuple(_key(r.source) for r in history.records)==inputkeys,'complete original source disposition/order')
    statuses=Counter()
    for record in history.records:
        key=_key(record.source);images=tuple(forward[key][0]);generated=tuple(forward[key][1])
        status=expected[images[0]][0] if images else 'deleted';statuses[status]+=1
        _need(type(record.status) is str and record.status==status and type(record.images) is tuple and type(record.generated) is tuple,'typed original source fate')
        _need(tuple(_key(v) for v in record.images)==images and tuple(_key(v) for v in record.generated)==generated,'complete ordered reciprocal images/generated')
        _need(history.from_input(record.source) is record,'actual source history lookup identity')
    _need(statuses==Counter(retained=33,modified=24,deleted=5),'all six-kind known source fates')
    return dict(history_inputs=62,history_outputs=76,retained=33,modified=24,deleted=5,generated=19)


def _contacts(result,m):
    _need(type(result.contacts) is tuple and len(result.contacts)==2,'two complete ordered contact rails')
    for contact,rail in enumerate(result.contacts):
        face=m['sidefaces'][contact]+1;old=m['sideuses'][contact]+1
        _need((_key(rail.edge),_key(rail.source_face),_key(rail.source_coedge),_key(rail.blend_face))==(_k(result.solid,'edge',14+contact),_k(result.source,'face',face),_k(result.source,'coedge',old),_k(result.solid,'face',7)),'contact actual selected source/target incidences')
        _need(type(rail.blend_u) is int and rail.blend_u==contact,'distinct incoming/outgoing blend occurrences')
        _exact(rail.source_t,3,'contact spine map');_need(rail.source_t==_ID_U,'whole selected source spine parameter')
        _exact(rail.normal,3,'exact contact unit normal');_need(rail.normal==(*map(F,m['normals'][contact]),F(0)),'whole original-side/blend tangent-normal identity')
        _check_map(rail.source_uv,_affine_uv(m['uses'][24+contact][4]),'contact original incident UV')
        blend=('affine',(F(contact),F(0),F(0),F(0),F(1),F(0)));_check_map(rail.blend_uv,blend,'contact new band UV')
        # Both maps are affine in the intrinsic spine parameter. Source planes
        # and a fixed band-u are affine as well: endpoint coefficients prove
        # whole-rail position equality, with analytic constant normal identity.
        for t in (F(0),F(1)):
            su,sv=_map_value(_affine_uv(m['uses'][24+contact][4]),t,F(0));bu,bv=_map_value(blend,t,F(0))
            _need(_plane(face-1,su,sv)==_surface(m,6,bu,bv)==m['vertices'][6+2*int(t)+contact],'whole contact affine coefficient identity')
    return dict(contacts=2)


def _circle_expr(q,s):
    ss=mul(s[0],s[0]);dd=mul(s[1],s[1]);den=add(dd,ss);x=(add(dd,factor(ss,-1)),den);y=(factor(mul(s[0],s[1]),2),den)
    return ((x,y),(rscale(y,-1),x),(rscale(x,-1),rscale(y,-1)),(y,rscale(x,-1)))[q]


def _curve_expr(spec):
    kind,d=spec
    if kind=='line':return tuple(rational((d[k],d[k+3]-d[k])) for k in range(3))
    x,y=_circle_expr(d[3],rational((0,1)))
    return radd(rscale(x,d[2]),rational((d[0],))),radd(rscale(y,d[2]),rational((d[1],))),rational((d[4],))


def _uv_expr(spec):
    kind,d=spec
    if kind=='line':return tuple(rational((d[k],d[k+2]-d[k])) for k in range(2))
    x,y=_circle_expr(d[3],rational((0,1)))
    return radd(rscale(x,d[2]),rational((d[0],))),radd(rscale(y,d[2]),rational((d[1],)))


def _surface_expr(m,slot,uv):
    u,v=uv;c=lambda x:rational((x,))
    if slot==6:
        x,y=_circle_expr(m['q'],u)
        return radd(rscale(x,m['r']),c(m['center'][0])),radd(rscale(y,m['r']),c(m['center'][1])),radd(v,c(-F(1,2)))
    x=radd(rscale(u,_W),c(-_A));y=radd(rscale(v,_W),c(-_A));z=radd(v,c(-F(1,2)))
    return ((x,y,c(F(1,2))),(x,y,c(-F(1,2))),(x,c(-_A),z),(x,c(_A),z),(c(_A),x,z),(c(-_A),x,z))[slot]


def _maps(result,m):
    solid=result.solid;checks=0
    for use,wanted in zip(solid.coedges,m['uses']):
        edge,face,sign,kind,data=wanted;spatial=_curve_expr(m['curves'][edge]);uv=_uv_expr((kind,data));mapped=_surface_expr(m,face,uv)
        _need(all(same(a,b) for a,b in zip(spatial,mapped)),'whole rational source/new edge and both incident UV/carrier identities')
        for t in (F(0),F(1,7),F(1,2),F(1)):
            actual=solid.coedge_uv(use.id,t);_exact(actual,2,'actual exact incident UV reader')
            _need(actual==tuple(value(v,t) for v in uv),'actual non-dyadic intrinsic coedge UV');checks+=1
    for i,spec in enumerate(m['curves']):
        expr=_curve_expr(spec)
        for t in (F(0),F(1,7),F(1,2),F(1)):
            actual=solid.edge_point(i+1,t);_exact(actual,3,'actual exact physical edge reader')
            _need(actual==_world(result.source.frame,tuple(value(v,t) for v in expr)),'actual non-dyadic retained/cropped/new curve reader');checks+=1
    return checks


def _readers(result,m):
    probes=[(F(0),F(0),F(0)),(F(2),F(0),F(0)),(F(0),F(0),F(1)),(m['sx']*_A,m['sy']*_A,F(0))]
    probes.extend(tuple(map(_f,v.position)) for v in result.source.vertices);probes.extend(m['vertices'])
    for spec in m['curves']:
        expr=_curve_expr(spec);probes.append(tuple(value(c,F(1,2)) for c in expr))
    # The unused half of the full circle and artificial section split lines
    # must not be advertised as physical supports.
    for p,q in ((m['c'],m['c']),(m['c']-m['r'],m['c']),(m['c'],m['c']-m['r']),(m['c'],m['c']+m['r']/2)):
        probes.append((m['sx']*p,m['sy']*q,F(0)))
    tiny=F(1,1<<1074)
    for u in (F(0),F(1,7),F(1,2),F(1)):
        for v in (F(0),F(1,2),F(1)):probes.append(_surface(m,6,u,v))
        p=_surface(m,6,u,F(1,2));nx,ny=_circle(m['q'],u)
        for delta in (-tiny,tiny,-F(1,1<<28),F(1,1<<28)):probes.append((p[0]+delta*nx,p[1]+delta*ny,F(0)))
    for z in (-.5,.5):
        for direction in (-math.inf,math.inf):probes.append((F(0),F(0),F(math.nextafter(z,direction))))
    for local in probes:
        query=_world(result.source.frame,local);answer=result.solid.classify_point(query)
        _need(answer.solid is result.solid and answer.point==query,'actual material owner/query binding');_exact(answer.point,3,'actual exact material point');_ints(answer.boundary_faces,'all bounded material supports')
        _need((answer.label,answer.boundary_faces)==_material(m,query),'complete original rounded-section material/support truth')
    d=1-F(2,3)*m['r'];uv=[(F(0),F(0)),(F(1),F(1)),(F(1,2),F(1,2)),(F(0),F(1,2)),(F(1),F(1,2)),(d,F(1,2)),(1-d,F(1,2)),(float(d),.5),(float(1-d),.5),(math.nextafter(float(d),math.inf),.5),(math.nextafter(float(d),-math.inf),.5),(-1,.5),(math.nan,.5),(math.inf,.5)]
    trimqueries=0
    for slot in range(7):
        probesuv=list(uv)
        if slot<2:
            spec=('arc',m['uses'][22][4])
            for t in (F(0),F(1,7),F(1,2),F(1)):
                point=_map_value(spec,t,F(0));probesuv.extend((point,tuple(float(c) for c in point)))
                probesuv.extend((math.nextafter(float(point[0]),direction),float(point[1])) for direction in (-math.inf,math.inf))
        for u,v in probesuv:
            actual=result.solid.classify(slot,u,v);_need(type(actual) is int and actual==_trim(m,slot,u,v),'actual original-UV cap/side/quarter trim');trimqueries+=1
    return dict(material_queries=len(probes),trim_queries=trimqueries)


def qualify_result(result):
    m=_inputs(result);_graph(result,m);stats=_history(result,m);stats.update(_contacts(result,m));stats.update(_readers(result,m));stats['exact_reader_queries']=_maps(result,m)
    scale=F(result.source.frame.scale);r=result.radius;volume=result.solid.volume
    _need(type(volume.rational) is F and type(volume.pi) is F and (volume.rational,volume.pi)==((F(9,4)-r*r)*scale**3,r*r*scale**3/4),'exact rounded-section world volume coefficients')
    _need(type(result.timings) is tuple and len({name for name,_ in result.timings})==len(result.timings) and all(type(name) is str and math.isfinite(ms) and ms>=0 for name,ms in result.timings),'construction timings')
    stats.update(vertices=10,edges=15,coedges=30,loops=7,faces=7,charts=7,genus=0,volume_rational=volume.rational,volume_pi=volume.pi)
    return stats


def qualify_samples(batch):
    result=batch.result;m=_model(result)
    _need(batch.request.result is result and type(batch.rows) is tuple and type(batch.request.candidates) is tuple,'actual sample request/result ownership')
    _need(type(batch.backend) is str and batch.backend in ('cpu','cuda') and type(batch.device) is int and batch.device>=0,'observed native placement')
    candidates=batch.request.candidates;_need(type(batch.count) is int and batch.count==len(candidates)==len(batch.rows),'complete scatter count')
    stats=dict(count=len(candidates),accepted=0,rejected=0,invalid=0,boundary=0,max_position_error=F(0),max_normal_error=F(0));labels=[]
    for i,(candidate,row) in enumerate(zip(candidates,batch.rows)):
        slot=candidate.patch_slot;_need(type(slot) is int and 0<=slot<7 and type(candidate.u) is float and type(candidate.v) is float,'actual copied native query types')
        _need(all(type(v) is int for v in (row.index,row.patch_slot,row.face_id,row.chart_id,row.trim,row.native_status)) and (row.index,row.patch_slot,row.face_id,row.chart_id)==(i,slot,slot+1,slot+1),'actual source/scatter identity')
        _need(type(row.u) is float and type(row.v) is float and _bits(row.u)==_bits(candidate.u) and _bits(row.v)==_bits(candidate.v),'actual normalized native UV bits')
        label=_trim(m,slot,candidate.u,candidate.v);labels.append(label);_need(row.trim==label,'independent actual-float trim admission')
        _need(type(row.position) is tuple and type(row.normal) is tuple and len(row.position)==len(row.normal)==3,'immutable sample vectors')
        if label in (0,3):
            _need(row.native_status==-1 and all(type(v) is float and math.isnan(v) for v in (*row.position,*row.normal)),'explicit rejected/invalid scatter');stats['rejected' if label==0 else 'invalid']+=1;continue
        _need(row.native_status==0 and all(type(v) is float and math.isfinite(v) for v in (*row.position,*row.normal)),'finite admitted native vectors')
        expected=_world(result.source.frame,_surface(m,slot,F(candidate.u),F(candidate.v)))
        normal=tuple(map(F,_NORMALS[slot])) if slot<6 else (*_circle(m['q'],F(candidate.u)),F(0))
        for name,actual,wanted,limit in (('position',row.position,expected,POSITION_LIMIT),('normal',row.normal,normal,NORMAL_LIMIT)):
            for a,b in zip(actual,wanted):
                error=abs(F(a)-b);_need(error<=limit,'direct original-source world '+name+' component budget');stats['max_'+name+'_error']=max(stats['max_'+name+'_error'],error)
        stats['accepted']+=1;stats['boundary']+=label==2
    _ints(batch.request.labels,'actual request labels');_ints(batch.request.admitted_indices,'complete gather')
    _need(batch.request.labels==tuple(labels) and batch.request.admitted_indices==tuple(i for i,v in enumerate(labels) if v in (1,2)),'actual pre-native float admission/gather')
    _need(type(batch.evaluated_count) is int and batch.evaluated_count==stats['accepted'],'actual native evaluated count')
    _need(type(batch.timings) is tuple and len({name for name,_ in batch.timings})==len(batch.timings) and all(type(name) is str and math.isfinite(ms) and ms>=0 for name,ms in batch.timings),'sample timings')
    return stats


def qualify_topology(result,actual):
    solid=result.solid;source=actual.input
    _need(source.snapshots==() and (source.edge_count,source.face_count,source.coedge_count)==(15,7,30),'actual standalone fillet topology counts')
    def key(s):
        _need(all(type(v) is int for v in (s.instance,s.body_id,s.revision,s.entity_id)) and (s.instance,s.body_id,s.revision)==(0,solid.body_id,1),'typed actual topology namespace');return s.entity_id
    edgeids=tuple(map(key,source.edge_sources));faceids=tuple(map(key,source.face_sources));useids=tuple(map(key,source.coedge_sources))
    _need(set(edgeids)==set(range(1,16)) and set(faceids)==set(range(1,8)) and set(useids)==set(range(1,31)),'complete typed topology identity cover')
    edges=struct.unpack('<30I',source.coedge_edges);faces=struct.unpack('<30I',source.coedge_faces);signs=struct.unpack('<30i',source.orientations);groups=defaultdict(list)
    for slot,id in enumerate(useids):
        use=solid.coedges[id-1];_need((edgeids[edges[slot]],faceids[faces[slot]],signs[slot])==(use.edge_slot+1,use.face_slot+1,use.orientation),'actual ABI source incidence');groups[use.edge_slot+1].append(use)
    from .adjacency_oracles import qualify_result as qualify_adjacency
    stats=qualify_adjacency(actual)
    _need(stats['paired']==15 and not any(stats[k] for k in ('unused','boundary','same_face_pair','orientation_conflict','multiple_uses')),'complete ordinary closed fillet pairs')
    for slot,id in enumerate(edgeids):
        _need(Counter(key(v) for v in actual.edge_uses(slot))==Counter(v.id for v in groups[id]) and actual.edge_kind(slot)==2,'actual typed paired edge-use reader')
    for slot,id in enumerate(useids):
        current=solid.coedges[id-1];mate=next(v for v in groups[current.edge_slot+1] if v.id!=id)
        _need(key(actual.mate(slot))==mate.id and key(actual.neighbor(slot))==mate.face_slot+1,'actual typed mate/neighbor reader')
    return stats
