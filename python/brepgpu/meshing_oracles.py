"""Independent exact-source, winding-chain and rounded-embedding mesh truth.

No product mesher, refinement helper or geometry reader computes expected data.
Strict projected triangle signs plus the full integer boundary chain certify
complete polygonal-domain coverage, including holes, in linear output work.
"""
from collections import Counter,defaultdict
from fractions import Fraction as F
import math


def require(condition,message):
    if not condition:raise AssertionError(message)


def point(values,n,label):
    require(type(values) is tuple and len(values)==n and all(type(v) is F for v in values),label+' exact tuple differs')
    return values


def sub(a,b):return tuple(x-y for x,y in zip(a,b))
def dot(a,b):return sum((x*y for x,y in zip(a,b)),F(0))
def cross2(a,b):return a[0]*b[1]-a[1]*b[0]
def turn(a,b,c):return cross2(sub(b,a),sub(c,a))
def rotate(x,y,q):
    for _ in range(q%4):x,y=-y,x
    return x,y


def original_curve(body,edge,t):
    curve=body.curves[edge.curve_slot]
    data=tuple(F(v) for v in curve.data)
    if curve.kind=='line':local=tuple((1-t)*data[a]+t*data[a+3] for a in range(3))
    else:
        require(curve.kind=='circle','unknown original curve')
        q,s=(3,F(1)) if t==1 else (int(4*t),4*t-int(4*t))
        x,y=rotate(data[0]*(1-s*s)/(1+s*s),data[0]*2*s/(1+s*s),q)
        local=(x,y,data[1])
    return tuple(F(o)+F(body.frame.scale)*v for o,v in zip(body.frame.origin,local))


def original_uv(coedge,t):
    data=tuple(F(v) for v in coedge.uv_map.data);kind=coedge.uv_map.kind
    if kind=='line':return tuple((1-t)*data[a]+t*data[a+2] for a in range(2))
    if kind=='circle_wall':return 4*t,data[0]
    if kind=='seam_wall':return data[0],t
    require(kind=='circle_plane','unknown original coedge map')
    q,s=(3,F(1)) if t==1 else (int(4*t),4*t-int(4*t))
    x,y=rotate(data[0]*(1-s*s)/(1+s*s),data[0]*2*s/(1+s*s),q)
    return (x+data[1])/(2*data[1]),(y+data[1])/(2*data[1])


def patch_point(patch,uv):
    u,v=uv
    bu=(1-u,u) if patch.degree_u==1 else ((1-u)**2,2*u*(1-u),u*u)
    require(patch.degree_u in (1,2) and patch.degree_v==1,'unsupported original chart degree')
    bv=(1-v,v);h=[F(0)]*4
    for i,a in enumerate(bu):
        for j,b in enumerate(bv):
            for k in range(4):h[k]+=a*b*F(patch.controls[4*(2*i+j)+k])
    require(h[3]>0,'nonpositive original patch weight')
    return tuple(F(patch.origin[k])+F(patch.scale)*h[k]/h[3] for k in range(3))


def chain_add(chain,a,b):
    require(a!=b,'collapsed directed mesh edge')
    chain[(min(a,b),max(a,b))]+=1 if a<b else -1


def normalized(chain):return {key:value for key,value in chain.items() if value}


def expected_chain(words):
    answer=Counter()
    for word in words:
        require(len(word)>=3,'boundary word too short')
        for a,b in zip(word,word[1:]+word[:1]):chain_add(answer,a,b)
    return normalized(answer)


def min_radius2(triangle,center):
    pts=tuple(sub(p[:2],center) for p in triangle)
    area=turn(*pts)
    signs=tuple(turn(a,b,(F(0),F(0))) for a,b in zip(pts,pts[1:]+pts[:1]))
    if area and (all(s>=0 for s in signs) or all(s<=0 for s in signs)):return F(0)
    values=[]
    for a,b in zip(pts,pts[1:]+pts[:1]):
        d=sub(b,a);den=dot(d,d)
        t=max(F(0),min(F(1),-dot(a,d)/den)) if den else F(0)
        q=tuple(a[k]+t*d[k] for k in range(2));values.append(dot(q,q))
    return min(values)


def ring_embedding(points,center,n):
    """Quadrant monotonicity proves simplicity; positive turns then convexity."""
    count=4*n
    require(len(points)==count and len(set(points))==count,'rounded ring repeated/collapsed position')
    for q in range(4):
        previous=None
        for k in range(n+1):
            x,y=sub(points[(q*n+k)%count],center);x,y=rotate(x,y,-q)
            require(x>=0 and y>=0,'ring left its original angular quadrant')
            if k==0:require(x>0 and y==0,'ring cardinal source start differs')
            if k==n:require(x==0 and y>0,'ring cardinal source end differs')
            if previous is not None:
                require(x<=previous[0] and y>=previous[1] and (x,y)!=previous,'ring angular order reversed')
            previous=x,y
    require(all(turn(points[i-1],points[i],points[(i+1)%count])>0 for i in range(count)),
            'ring is not strictly convex CCW')


def qualify_mesh(mesh):
    from .body_oracles import qualify_snapshot
    body=mesh.body
    qualify_snapshot(body)
    require(type(mesh.tolerance) is F and mesh.tolerance>0,'retained exact tolerance differs')
    require(type(mesh.vertices) is tuple and type(mesh.triangles) is tuple and
            type(mesh.edge_polylines) is tuple and type(mesh.boundary_uses) is tuple,'mutable mesh tables')
    n=mesh.quarter_segments
    require(type(n) is int and (n==0 if body.shape=='box' else 1<=n<=1024 and not n&(n-1)),
            'quarter subdivision contract differs')
    radius=F(body.frame.scale)/4
    curve=F(0) if n==0 else radius/(2*n*n)
    require(type(mesh.curve_bound) is F and mesh.curve_bound==curve,'reported conservative curve bound differs')
    require(curve<=mesh.tolerance/2,'source approximation reserve exceeded')
    if n>1:require(4*curve>mesh.tolerance/2,'reported subdivision is not the first admitted dyadic level')
    parameters={e.id:((F(0),F(1)) if body.curves[e.curve_slot].kind=='line' else tuple(F(k,4*n) for k in range(4*n+1))) for e in body.edges}
    source_position={('vertex',v.id):tuple(F(o)+F(body.frame.scale)*F(x) for o,x in zip(body.frame.origin,v.position)) for v in body.vertices}
    for e in body.edges:
        for t in parameters[e.id][1:-1]:source_position['edge',e.id,t]=original_curve(body,e,t)
    for f in body.faces:
        if f.kind=='disk':source_position['face',f.id,'center']=patch_point(body.charts[f.chart_slots[0]].patch,(F(1,2),F(1,2)))
    require(len(mesh.vertices)==len(source_position),'complete original-source vertex generators differ')
    positions=[];rounded=[];by_source={};rho=F(0)
    for index,v in enumerate(mesh.vertices):
        require(type(v.id) is int and v.id==index and type(v.source) is tuple,'vertex identity/source tuple differs')
        source=v.source
        require(source and type(source[0]) is str,'source generator kind is not native text')
        if source[0]=='vertex':
            require(len(source)==2 and type(source[1]) is int,'original vertex source ID is not a native integer')
        elif source[0]=='edge':
            require(len(source)==3 and type(source[1]) is int and type(source[2]) is F,
                    'original edge source ID/parameter types differ')
        else:
            require(source[0]=='face' and len(source)==3 and type(source[1]) is int
                    and type(source[2]) is str and source[2]=='center','original face-center source types differ')
        require(source in source_position and source not in by_source,'foreign/duplicate vertex generator')
        point(v.position,3,'world vertex')
        require(v.position==source_position[v.source],'vertex differs from original exact source parameter')
        require(type(v.rounded_position) is tuple and len(v.rounded_position)==3 and
                all(type(x) is float and math.isfinite(x) for x in v.rounded_position),'rounded binary64 vertex differs')
        require(v.rounded_position==tuple(float(x) for x in v.position),'actual rounded coordinates differ from binary64 export')
        actual=tuple(F.from_float(x) for x in v.rounded_position)
        rho=max(rho,sum((abs(a-b) for a,b in zip(actual,v.position)),F(0)))
        positions.append(v.position);rounded.append(actual);by_source[v.source]=index
    require(set(by_source)==set(source_position),'missing original-source generator')
    require(tuple(v.source for v in mesh.vertices[:len(body.vertices)])==tuple(('vertex',v.id) for v in body.vertices),
            'original vertices did not retain original order')
    require(len(set(positions))==len(positions) and len(set(rounded))==len(rounded),'distinct physical vertices collide')
    require(type(mesh.rounding_bound) is F and mesh.rounding_bound==rho,'actual rounded displacement bound differs')
    require(type(mesh.error_bound) is F and mesh.error_bound==curve+rho and mesh.error_bound<=mesh.tolerance,
            'actual whole-triangle total error exceeds submitted tolerance')
    def vertex_at(e,t):
        key=('vertex',body.vertices[e.start if t==0 else e.end].id) if t in (0,1) else ('edge',e.id,t)
        return by_source[key]
    edge_ids={e.id:e for e in body.edges};poly={}
    require(len(mesh.edge_polylines)==len(body.edges),'source edge polyline count differs')
    for p in mesh.edge_polylines:
        require(type(p.edge_id) is int and p.edge_id in edge_ids and p.edge_id not in poly,'foreign/duplicate original edge polyline')
        e=edge_ids[p.edge_id];ts=parameters[e.id]
        require(type(p.parameters) is tuple and all(type(t) is F for t in p.parameters) and p.parameters==ts,
                'complete canonical source parameter chain differs')
        ids=tuple(vertex_at(e,t) for t in ts)
        require(type(p.vertices) is tuple and all(type(i) is int for i in p.vertices)
                and p.vertices==ids,'shared source edge vertex chain differs')
        poly[e.id]=ids
    require(len(mesh.boundary_uses)==len(body.coedges),'original coedge occurrence count differs')
    uses={}
    for u in mesh.boundary_uses:
        require(type(u.coedge_id) is int and 1<=u.coedge_id<=len(body.coedges) and u.coedge_id not in uses,
                'foreign/duplicate original coedge occurrence')
        c=body.coedges[u.coedge_id-1];e=body.edges[c.edge_slot];ts=parameters[e.id];ids=poly[e.id]
        if c.orientation<0:ts,ids=ts[::-1],ids[::-1]
        require(all(type(i) is int for i in (u.face_id,u.edge_id,u.orientation))
                and (u.face_id,u.edge_id,u.orientation)==(body.faces[c.face_slot].id,e.id,c.orientation),
                'original coedge typed ownership/sign differs')
        require(type(u.parameters) is tuple and u.parameters==ts and all(type(t) is F for t in u.parameters)
                and type(u.vertices) is tuple and all(type(i) is int for i in u.vertices)
                and u.vertices==ids,'directed original coedge parameter/identity chain differs')
        require(type(u.uv) is tuple and u.uv==tuple(original_uv(c,t) for t in ts)
                and all(type(uv) is tuple and all(type(x) is F for x in uv) for uv in u.uv),
                'complete original lifted boundary UV occurrence differs')
        uses[c.id]=u
    # Expected domains are independently reconstructed from original coedge loops.
    face_words={};wall=next((f for f in body.faces if f.kind=='cylinder'),None)
    for f in body.faces:
        if f is wall:continue
        words=[]
        for ls in f.loop_slots:
            word=[]
            for cs in body.loops[ls].coedge_slots:
                c=body.coedges[cs];e=body.edges[c.edge_slot];ids=poly[e.id]
                if c.orientation<0:ids=ids[::-1]
                if word:require(word[-1]==ids[0],'original source loop does not join')
                word.extend(ids if not word else ids[1:])
            require(word and word[0]==word[-1],'original source loop did not close')
            words.append(tuple(word[:-1]))
        face_words[f.id]=tuple(words)
    rings=[];center=tuple(F(o) for o in body.frame.origin[:2])
    for e in body.edges:
        if body.curves[e.curve_slot].kind!='circle':continue
        ids=poly[e.id];rings.append((e,ids))
        for a,b in zip(ids,ids[1:]):
            aa,bb=sub(positions[a][:2],center),sub(positions[b][:2],center)
            require(dot(aa,aa)==dot(bb,bb)==radius*radius and cross2(aa,bb)>0,'source circle interval is not an ordered minor arc')
            require((radius*radius+dot(aa,bb))/2>=(radius-curve)**2,'actual circle chord violates reported analytic bound')
        ring_embedding(tuple(positions[i][:2] for i in ids[:-1]),center,n)
        ring_embedding(tuple(rounded[i][:2] for i in ids[:-1]),tuple(F(float(o)) for o in body.frame.origin[:2]),n)
    if wall is not None:
        require(len(rings)==2,'wall needs both complete source rings')
        rings.sort(key=lambda row:positions[row[1][0]][2]);bottom,top=rings[0][1],rings[1][1]
        require(all(positions[a][:2]==positions[b][:2] and rounded[a][:2]==rounded[b][:2] for a,b in zip(bottom,top)),
                'actual top/bottom rings are not the same vertical extrusion')
        require(positions[bottom[0]][2]<positions[top[0]][2] and rounded[bottom[0]][2]<rounded[top[0]][2],
                'actual cap heights collapsed/reversed')
        if body.shape=='plate':
            outer=tuple(rounded[by_source['vertex',v.id]] for v in body.vertices[:8])
            lo=tuple(min(p[k] for p in outer) for k in range(2));hi=tuple(max(p[k] for p in outer) for k in range(2))
            require(all(lo[k]<rounded[a][k]<hi[k] for a in bottom[:-1] for k in range(2)),
                    'rounded hole no longer strictly inside original outer rectangle')
    charts={c.id:c for c in body.charts};faces={f.id:f for f in body.faces}
    plane_axes={}
    for f in body.faces:
        if f is wall:continue
        p=body.charts[f.chart_slots[0]].patch
        a,b,c=(patch_point(p,uv) for uv in ((F(0),F(0)),(F(1),F(0)),(F(0),F(1))))
        u,v=sub(b,a),sub(c,a)
        ua=tuple(i for i,x in enumerate(u) if x);va=tuple(i for i,x in enumerate(v) if x)
        require(len(ua)==len(va)==1 and ua!=va,'original plane axes differ')
        normal=next(i for i in range(3) if i not in ua+va)
        plane_axes[f.id]=(ua[0],va[0],normal,(1 if u[ua[0]]*v[va[0]]>0 else -1))
    face_expected={fid:expected_chain(words) for fid,words in face_words.items()}
    chains={};round_chains={};incidence=defaultdict(list);vertex_links=defaultdict(list);face_counts=Counter()
    require(len(mesh.triangles)>0,'empty whole mesh')
    for index,t in enumerate(mesh.triangles):
        require(type(t.id) is int and t.id==index and type(t.vertices) is tuple and len(t.vertices)==3 and
                all(type(i) is int and 0<=i<len(positions) for i in t.vertices) and len(set(t.vertices))==3,
                'triangle dense identity/index/nondegeneracy differs')
        require(type(t.face_id) is int and t.face_id in faces,'foreign triangle original face')
        f=faces[t.face_id];face_counts[f.id]+=1
        require(type(t.corners) is tuple and len(t.corners)==3,'triangle corner occurrences differ')
        qs=[];atlas=[]
        for vid,c in zip(t.vertices,t.corners):
            point(c.uv,2,'original chart-local corner UV')
            require(type(c.chart_id) is int and c.chart_id in charts,'foreign corner chart')
            ch=charts[c.chart_id]
            require(ch.face_slot==f.id-1 and all(0<=x<=1 for x in c.uv),'corner chart/face/domain differs')
            require(patch_point(ch.patch,c.uv)==positions[vid],'corner original patch/UV does not reproduce shared vertex')
            qs.append(ch.quadrant)
            if f is wall:atlas.append((F(ch.quadrant)+c.uv[0])*n)
        if f is wall:
            require(len(set(qs))==1 and all(a.denominator==1 for a in atlas),'wall triangle crosses original chart/source angular grid')
            strip=int(min(atlas));require(0<=strip<4*n and max(atlas)==strip+1,'wall triangle skips an original chord interval')
            require(all(c.uv[1] in (0,1) for c in t.corners),'unexpected wall height source')
            key=(f.id,strip)
            expected=((bottom[strip],bottom[strip+1],top[strip+1],top[strip]),)
            if f.orientation<0:expected=(expected[0][::-1],)
            for data,collection in ((positions,chains),(rounded,round_chains)):
                a,b=data[bottom[strip]],data[bottom[strip+1]];d=sub(b[:2],a[:2]);den=dot(d,d)
                require(den>0,'collapsed actual wall chord')
                projected=[]
                for vid in t.vertices:
                    p=data[vid];lam=dot(sub(p[:2],a[:2]),d)/den
                    require(tuple(a[k]+lam*d[k] for k in range(2))==p[:2] and 0<=lam<=1,'wall point left actual chord plane')
                    require(data[bottom[0]][2]<=p[2]<=data[top[0]][2],'wall point left cap height interval')
                    projected.append((lam,p[2]))
                require(turn(*projected)*f.orientation>0,'actual wall triangle orientation/collapse differs')
                state=collection.setdefault(key,(Counter(),expected_chain(expected)))
                for a,b in zip(t.vertices,t.vertices[1:]+t.vertices[:1]):chain_add(state[0],a,b)
        else:
            key=(f.id,None);ua,va,normal,basis_sign=plane_axes[f.id]
            expected=face_expected[f.id]
            for data,collection in ((positions,chains),(rounded,round_chains)):
                require(len({data[i][normal] for i in t.vertices})==1,'triangle left its actual plane')
                anchor=face_words[f.id][0][0]
                require(all(data[i][normal]==data[anchor][normal] for i in t.vertices),'face vertices are not coplanar with original boundary')
                projected=tuple((data[i][ua],data[i][va]) for i in t.vertices)
                require(turn(*projected)*f.orientation*basis_sign>0,'actual plane triangle orientation/collapse differs')
                state=collection.setdefault(key,(Counter(),expected))
                for a,b in zip(t.vertices,t.vertices[1:]+t.vertices[:1]):chain_add(state[0],a,b)
        if f is wall or f.kind=='rectangle_hole':
            require(min_radius2(tuple(positions[i] for i in t.vertices),center)>=(radius-curve)**2,
                    'whole triangle violates inward radial/trim-material bound')
        for a,b,c in ((t.vertices[0],t.vertices[1],t.vertices[2]),(t.vertices[1],t.vertices[2],t.vertices[0]),(t.vertices[2],t.vertices[0],t.vertices[1])):
            incidence[min(a,b),max(a,b)].append((a,b,index));vertex_links[a].append((b,c))
    expected_groups={(f.id,None) for f in body.faces if f is not wall}
    if wall is not None:expected_groups|={(wall.id,k) for k in range(4*n)}
    for collection in (chains,round_chains):
        require(set(collection)==expected_groups,'missing original whole face/chord strip')
        require(all(normalized(actual)==expected for actual,expected in collection.values()),
                'full positive triangle boundary chain does not cover original chordal domain')
    require(set(face_counts)==set(faces),'missing original source face triangles')
    neighbors=defaultdict(set)
    for records in incidence.values():
        require(len(records)==2 and records[0][:2]==records[1][:2][::-1] and records[0][2]!=records[1][2],
                'mesh edge does not have two opposite distinct triangle uses')
        a,b=records[0][2],records[1][2];neighbors[a].add(b);neighbors[b].add(a)
    require(set(vertex_links)==set(range(len(positions))),'unused physical mesh vertex')
    for links in vertex_links.values():
        nexts={};indegree=Counter()
        for a,b in links:
            require(a not in nexts,'vertex link branches');nexts[a]=b;indegree[b]+=1
        require(set(nexts)==set(indegree) and all(n==1 for n in indegree.values()),'vertex link is not cyclic')
        start=next(iter(nexts));seen=set();cursor=start
        while cursor not in seen:seen.add(cursor);cursor=nexts[cursor]
        require(cursor==start and len(seen)==len(nexts),'vertex link has multiple pinched cycles')
    todo=[0];seen=set()
    while todo:
        cursor=todo.pop()
        if cursor in seen:continue
        seen.add(cursor);todo.extend(neighbors[cursor]-seen)
    require(len(seen)==len(mesh.triangles),'mesh shell is disconnected')
    euler=len(positions)-len(incidence)+len(mesh.triangles)
    require(euler==(0 if body.shape=='plate' else 2),'canonical shell genus differs')
    return dict(vertices=len(positions),triangles=len(mesh.triangles),mesh_edges=len(incidence),
                boundary_uses=len(uses),faces=len(face_counts),genus=(2-euler)//2,circle_segments=4*n)
