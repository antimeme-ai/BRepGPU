"""Whole actual feature-bore mesh truth from original source domains.

Positive boundary chains check complete coverage, independently of the product
bridge/ear triangulator. Generic exact predicates contain no construction policy.
"""
from collections import Counter,defaultdict
from fractions import Fraction as F
import math

from .meshing_oracles import (require,point,sub,dot,turn,patch_point,chain_add,
                              normalized,expected_chain,ring_embedding,min_radius2)
from .feature_bore_oracles import (qualify_result,source_data,expected_edge,
                                   expected_uv,world,_connected)


def qualify_mesh(mesh):
    from .feature_bore import BoreResult,BoreSolid
    result=mesh.bore_result;body=mesh.body
    require(type(result) is BoreResult and type(body) is BoreSolid,'actual bore mesh result/body type differs')
    require(body is result.solid and mesh.history is result.history and
            mesh.composed_history is result.composed_history,'retained actual result/body/both histories differ')
    qualify_result(result)
    points,h,center,radius,band=source_data(result);count=len(points);n=mesh.quarter_segments;m=4*n
    require(type(n) is int and 1<=n<=1024 and not n&(n-1),'quarter subdivision contract differs')
    require(type(mesh.tolerance) is F and mesh.tolerance>0,'retained exact tolerance differs')
    world_radius=radius*F(body.frame.scale);curve=world_radius/(2*n*n)
    require(type(mesh.curve_bound) is F and mesh.curve_bound==curve and curve<=mesh.tolerance/2,
            'actual world source approximation bound differs')
    if n>1:require(4*curve>mesh.tolerance/2,'first admitted dyadic refinement differs')
    for name in ('vertices','triangles','edge_polylines','boundary_uses'):
        require(type(getattr(mesh,name)) is tuple,'mutable mesh table differs')
    parameters={e.id:tuple(F(k,m) for k in range(m+1)) if e.id in (3*count+1,3*count+2)
                else (F(0),F(1)) for e in body.edges}
    source_position={('vertex',v.id):world(body.frame,v.position) for v in body.vertices}
    for e in body.edges:
        for t in parameters[e.id][1:-1]:source_position['edge',e.id,t]=expected_edge(result,e.id,t)
    require(len(mesh.vertices)==len(source_position)==2*count+2*m,'complete original source generators differ')
    positions=[];rounded=[];by_source={};rho=F(0)
    for i,v in enumerate(mesh.vertices):
        require(type(v.id) is int and v.id==i and type(v.source) is tuple,'dense source vertex identity differs')
        s=v.source
        require(s and type(s[0]) is str,'source generator kind differs')
        if s[0]=='vertex':require(len(s)==2 and type(s[1]) is int,'original vertex source identity differs')
        elif s[0]=='edge':require(len(s)==3 and type(s[1]) is int and type(s[2]) is F,'original rim source parameter type differs')
        else:require(False,'unsupported original source generator')
        require(s in source_position and s not in by_source,'foreign/duplicate source generator differs')
        point(v.position,3,'exact world generator')
        require(v.position==source_position[s],'source generator differs from original expression')
        require(type(v.rounded_position) is tuple and len(v.rounded_position)==3 and
                all(type(x) is float and math.isfinite(x) for x in v.rounded_position),
                'actual rounded vertex types differ')
        require(v.rounded_position==tuple(float(x) for x in v.position),'actual rounded vertex coordinates differ')
        actual=tuple(F.from_float(x) for x in v.rounded_position)
        rho=max(rho,sum((abs(x-y) for x,y in zip(v.position,actual)),F(0)))
        positions.append(v.position);rounded.append(actual);by_source[s]=i
    require(set(by_source)==set(source_position),'missing original mesh generator')
    require(tuple(v.source for v in mesh.vertices[:len(body.vertices)])==tuple(('vertex',v.id) for v in body.vertices),
            'original physical vertex order differs')
    require(len(set(positions))==len(set(rounded))==len(positions),'physical mesh vertices collide')
    require(type(mesh.rounding_bound) is F and mesh.rounding_bound==rho,'actual rounded displacement bound differs')
    require(type(mesh.error_bound) is F and mesh.error_bound==curve+rho and mesh.error_bound<=mesh.tolerance,
            'whole actual output error bound differs')
    def vertex_at(e,t):
        source=('vertex',body.vertices[e.start if t==0 else e.end].id) if t in (0,1) else ('edge',e.id,t)
        return by_source[source]
    poly={}
    require(len(mesh.edge_polylines)==len(body.edges),'complete source edge inventory differs')
    for p in mesh.edge_polylines:
        require(type(p.edge_id) is int and 1<=p.edge_id<=len(body.edges) and p.edge_id not in poly,'source edge ID differs')
        e=body.edges[p.edge_id-1];ts=parameters[e.id];ids=tuple(vertex_at(e,t) for t in ts)
        require(type(p.parameters) is tuple and all(type(t) is F for t in p.parameters) and p.parameters==ts,
                'complete intrinsic source parameter chain differs')
        require(type(p.vertices) is tuple and all(type(i) is int for i in p.vertices) and p.vertices==ids,
                'complete shared source edge chain differs')
        poly[p.edge_id]=p.vertices
    uses={}
    require(len(mesh.boundary_uses)==len(body.coedges),'complete source occurrence inventory differs')
    for u in mesh.boundary_uses:
        require(type(u.coedge_id) is int and 1<=u.coedge_id<=len(body.coedges) and u.coedge_id not in uses,
                'source coedge occurrence ID differs')
        c=body.coedges[u.coedge_id-1];e=body.edges[c.edge_slot];ts=parameters[e.id];ids=poly[e.id]
        if c.orientation<0:ts,ids=ts[::-1],ids[::-1]
        require(all(type(v) is int for v in (u.face_id,u.edge_id,u.orientation)) and
                (u.face_id,u.edge_id,u.orientation)==(c.face_slot+1,e.id,c.orientation),'source coedge typed ownership differs')
        require(type(u.parameters) is tuple and all(type(t) is F for t in u.parameters) and u.parameters==ts and
                type(u.vertices) is tuple and all(type(i) is int for i in u.vertices) and u.vertices==ids,
                'directed source occurrence parameter/identity chain differs')
        require(type(u.uv) is tuple and len(u.uv)==len(ts),'source occurrence UV inventory differs')
        for uv,t in zip(u.uv,ts):
            point(uv,2,'exact original incident UV')
            require(uv==expected_uv(result,c.id,t),'original cap/side/cutter occurrence UV differs')
        uses[c.id]=u
    words={}
    for face in body.faces[:-1]:
        loops=[]
        for ls in face.loop_slots:
            word=[]
            for cs in body.loops[ls].coedge_slots:
                ids=uses[body.coedges[cs].id].vertices
                if word:require(word[-1]==ids[0],'actual source domain loop does not join')
                word.extend(ids if not word else ids[1:])
            require(word[0]==word[-1],'actual source domain loop does not close')
            loops.append(tuple(word[:-1]))
        words[face.id]=tuple(loops)
    bottom=poly[3*count+1];top=poly[3*count+2]
    wc=world(body.frame,(*center,F(0)))[:2]
    for data in (positions,rounded):
        lower=tuple(data[by_source['vertex',i+1]] for i in range(count))
        upper=tuple(data[by_source['vertex',count+i+1]] for i in range(count))
        require(all(a[:2]==b[:2] for a,b in zip(lower,upper)) and
                len({p[2] for p in lower})==len({p[2] for p in upper})==1 and lower[0][2]<upper[0][2],
                'actual paired outer layers collapsed/reversed')
        require(all(data[a][:2]==data[b][:2] for a,b in zip(bottom,top)) and
                all(data[i][2]==lower[0][2] for i in bottom) and all(data[i][2]==upper[0][2] for i in top),
                'actual ring layers lost original cap correspondence')
        for i,a in enumerate(lower):
            b=lower[(i+1)%count]
            require(all(turn(a[:2],b[:2],p[:2])>0 for j,p in enumerate(lower) if j not in (i,(i+1)%count)),
                    'actual rounded outer profile lost strict convex support')
            require(all(turn(a[:2],b[:2],data[j][:2])>0 for j in bottom[:-1]),
                    'actual rounded hole left strict convex outer domain')
        ring_embedding(tuple(data[i][:2] for i in bottom[:-1]),wc,n)
    for a,b in zip(bottom,bottom[1:]):
        aa,bb=sub(positions[a][:2],wc),sub(positions[b][:2],wc)
        require(dot(aa,aa)==dot(bb,bb)==world_radius**2 and turn(wc,positions[a][:2],positions[b][:2])>0,
                'original ring chord lost ordered exact source circle')
        require((world_radius**2+dot(aa,bb))/2>=(world_radius-curve)**2,'whole source arc/chord bound differs')
    expected={f.id:expected_chain(words[f.id]) for f in body.faces[:-1]}
    chains={};rounded_chains={};incidence=defaultdict(list);links=defaultdict(list);face_counts=Counter()
    require(len(mesh.triangles)==4*count+4*m,'complete source-domain triangulation count differs')
    for index,t in enumerate(mesh.triangles):
        require(type(t.id) is int and t.id==index and type(t.vertices) is tuple and len(t.vertices)==3 and
                all(type(i) is int and 0<=i<len(positions) for i in t.vertices) and len(set(t.vertices))==3,
                'triangle dense identity/index differs')
        require(type(t.face_id) is int and 1<=t.face_id<=count+3,'original triangle face differs')
        f=body.faces[t.face_id-1];face_counts[f.id]+=1
        require(type(t.corners) is tuple and len(t.corners)==3,'complete original corner occurrences differ')
        atlas=[];quadrants=[]
        for vid,c in zip(t.vertices,t.corners):
            require(type(c.chart_id) is int and 1<=c.chart_id<=len(body.charts),'typed original corner chart differs')
            chart=body.charts[c.chart_id-1];point(c.uv,2,'original corner UV')
            require(chart.face_slot==f.id-1 and all(0<=x<=1 for x in c.uv),'original corner chart/face/domain differs')
            require(patch_point(chart.patch,c.uv)==positions[vid],'original patch/corner UV does not reproduce shared vertex')
            if f.id==count+3:
                require(c.uv[1] in band,'wall corner lost original cutter V band')
                atlas.append((F(chart.quadrant)+c.uv[0])*n);quadrants.append(chart.quadrant)
        if f.id==count+3:
            require(len(set(quadrants))==1 and all(x.denominator==1 for x in atlas),
                    'wall triangle crosses original quarter/source grid')
            strip=int(min(atlas));require(0<=strip<m and max(atlas)==strip+1,'wall triangle skips original chord strip')
            group=(f.id,strip)
            target=expected_chain(((bottom[strip],bottom[strip+1],top[strip+1],top[strip])[::-1],))
        else:group=(f.id,None);target=expected[f.id]
        for data,collection in ((positions,chains),(rounded,rounded_chains)):
            if f.id<=2:
                z=data[bottom[0] if f.id==1 else top[0]][2]
                require(all(data[i][2]==z for i in t.vertices),'actual cap triangle left original plane')
                projected=tuple(data[i][:2] for i in t.vertices);sign=f.orientation
            else:
                if f.id==count+3:a,b=data[bottom[strip]],data[bottom[strip+1]];sign=-1
                else:
                    k=f.id-3;a=data[by_source['vertex',k+1]];b=data[by_source['vertex',(k+1)%count+1]];sign=1
                d=sub(b[:2],a[:2]);den=dot(d,d);require(den>0,'actual side/wall chord collapsed')
                projected=[]
                for vid in t.vertices:
                    p=data[vid];lam=dot(sub(p[:2],a[:2]),d)/den
                    require(tuple(a[k]+lam*d[k] for k in range(2))==p[:2] and 0<=lam<=1 and
                            data[bottom[0]][2]<=p[2]<=data[top[0]][2],'actual side/wall triangle left its full rectangle')
                    projected.append((lam,p[2]))
            require(turn(*projected)*sign>0,'actual triangle orientation/collapse differs')
            chain=collection.setdefault(group,(Counter(),target))
            for a,b in zip(t.vertices,t.vertices[1:]+t.vertices[:1]):chain_add(chain[0],a,b)
        if f.id<=2 or f.id==count+3:
            require(min_radius2(tuple(positions[i] for i in t.vertices),wc)>=(world_radius-curve)**2,
                    'whole source triangle enters unbounded analytic hole')
        for a,b,c in ((t.vertices[0],t.vertices[1],t.vertices[2]),(t.vertices[1],t.vertices[2],t.vertices[0]),(t.vertices[2],t.vertices[0],t.vertices[1])):
            incidence[min(a,b),max(a,b)].append((a,b,index));links[a].append((b,c))
    groups={(f.id,None) for f in body.faces[:-1]}|{(count+3,k) for k in range(m)}
    for collection in (chains,rounded_chains):
        require(set(collection)==groups,'missing original full face/chord strip')
        require(all(normalized(actual)==target for actual,target in collection.values()),
                'full positive triangle boundary chain does not cover original cap/hole/rectangle domain')
    require(set(face_counts)==set(range(1,count+4)),'missing original face triangles')
    neighbors=defaultdict(set)
    for records in incidence.values():
        require(len(records)==2 and records[0][:2]==records[1][:2][::-1] and records[0][2]!=records[1][2],
                'physical mesh edge needs two opposed distinct uses')
        a,b=(v[2] for v in records);neighbors[a].add(b);neighbors[b].add(a)
    require(set(links)==set(range(len(positions))),'unused physical mesh vertex')
    for members in links.values():
        nexts={};indegree=Counter()
        for a,b in members:
            require(a not in nexts,'physical vertex link branches');nexts[a]=b;indegree[b]+=1
        require(set(nexts)==set(indegree) and all(v==1 for v in indegree.values()),'physical vertex link is not cyclic')
        start=next(iter(nexts));seen=set();cursor=start
        while cursor not in seen:seen.add(cursor);cursor=nexts[cursor]
        require(cursor==start and len(seen)==len(nexts),'physical vertex has pinched multiple link cycles')
    _connected(neighbors,0,set(range(len(mesh.triangles))),'actual mesh shell is disconnected')
    euler=len(positions)-len(incidence)+len(mesh.triangles)
    require(euler==0,'actual through-hole mesh genus differs')
    return dict(vertices=len(positions),triangles=len(mesh.triangles),mesh_edges=len(incidence),
                boundary_uses=len(uses),faces=len(face_counts),genus=1,circle_segments=m)
