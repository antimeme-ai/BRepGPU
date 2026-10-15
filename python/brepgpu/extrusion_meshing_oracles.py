"""Independent actual extrusion mesh domains, source occurrences and float truth.

No product mesher, fan, inverse-map or rounded-guard helper supplies answers.
"""
from collections import Counter,defaultdict
from fractions import Fraction as F
import math
from .meshing_oracles import (require,point,sub,dot,turn,original_uv,
                              patch_point,chain_add,normalized,expected_chain)


def qualify_mesh(mesh):
    from .extrusion import ExtrusionResult,ExtrudedSolid
    from .extrusion_oracles import qualify_result
    result=mesh.feature_result;body=mesh.body
    require(type(result) is ExtrusionResult and type(body) is ExtrudedSolid,'actual result/body type differs')
    require(body is result.solid and mesh.history is result.history,'retained actual result/body/history differs')
    qualify_result(result)
    require(type(mesh.tolerance) is F and mesh.tolerance>0,'retained exact tolerance differs')
    require(type(mesh.quarter_segments) is int and mesh.quarter_segments==0 and
            type(mesh.curve_bound) is F and mesh.curve_bound==0,'exact polygonal source approximation differs')
    for name in ('vertices','triangles','edge_polylines','boundary_uses'):
        require(type(getattr(mesh,name)) is tuple,'mutable mesh table')
    profile=result.profile;n=len(profile.vertices);h=result.height
    source=tuple(tuple(F(o)+F(body.frame.scale)*x for o,x in zip(body.frame.origin,(*v.position[:2],z)))
                 for z in (-h/2,h/2) for v in profile.vertices)
    require(len(mesh.vertices)==2*n,'complete original-source vertex generators differ')
    positions=[];rounded=[];rho=F(0)
    for i,(v,expected) in enumerate(zip(mesh.vertices,source)):
        require(type(v.id) is int and v.id==i,'dense mesh vertex identity differs')
        require(type(v.source) is tuple and len(v.source)==2 and v.source[0]=='vertex' and
                type(v.source[1]) is int and v.source[1]==body.vertices[i].id,'original vertex source identity differs')
        point(v.position,3,'world vertex');require(v.position==expected,'vertex differs from original exact profile/height')
        require(type(v.rounded_position) is tuple and len(v.rounded_position)==3 and
                all(type(x) is float and math.isfinite(x) for x in v.rounded_position),
                'rounded binary64 vertex differs')
        require(v.rounded_position==tuple(float(x) for x in expected),'actual rounded coordinates differ')
        actual=tuple(F.from_float(x) for x in v.rounded_position)
        rho=max(rho,sum((abs(a-b) for a,b in zip(actual,expected)),F(0)))
        positions.append(expected);rounded.append(actual)
    require(len(set(positions))==len(set(rounded))==2*n,'distinct physical vertices collide')
    require(type(mesh.rounding_bound) is F and mesh.rounding_bound==rho,'actual rounded displacement bound differs')
    require(type(mesh.error_bound) is F and mesh.error_bound==rho and rho<=mesh.tolerance,
            'actual whole-triangle error exceeds submitted tolerance')
    # Every actual rounded shell is the vertical extrusion of one strict polygon.
    for data in (positions,rounded):
        require(all(data[i][:2]==data[n+i][:2] for i in range(n)),'paired layers lost actual shared XY')
        require(len({p[2] for p in data[:n]})==len({p[2] for p in data[n:]})==1 and
                data[0][2]<data[n][2],'actual cap planes collapsed/reversed')
        for i,a in enumerate(data[:n]):
            b=data[(i+1)%n]
            require(all(turn(a[:2],b[:2],p[:2])>0 for j,p in enumerate(data[:n]) if j not in (i,(i+1)%n)),
                    'actual profile lost global strict convex support')
    require(len(mesh.edge_polylines)==3*n,'complete original edge polyline inventory differs')
    polylines={}
    for p in mesh.edge_polylines:
        require(type(p.edge_id) is int and 1<=p.edge_id<=3*n and p.edge_id not in polylines,
                'original edge identity differs')
        edge=body.edges[p.edge_id-1]
        require(type(p.parameters) is tuple and all(type(t) is F for t in p.parameters) and
                p.parameters==(F(0),F(1)),'intrinsic source parameter chain differs')
        require(type(p.vertices) is tuple and all(type(i) is int for i in p.vertices) and
                p.vertices==(edge.start,edge.end),'shared original edge endpoints differ')
        polylines[p.edge_id]=p
    require(len(mesh.boundary_uses)==6*n,'complete original coedge occurrence inventory differs')
    uses={}
    for u in mesh.boundary_uses:
        require(type(u.coedge_id) is int and 1<=u.coedge_id<=6*n and u.coedge_id not in uses,
                'original coedge identity differs')
        c=body.coedges[u.coedge_id-1];e=body.edges[c.edge_slot]
        require(all(type(i) is int for i in (u.face_id,u.edge_id,u.orientation)) and
                (u.face_id,u.edge_id,u.orientation)==(body.faces[c.face_slot].id,e.id,c.orientation),
                'original coedge typed ownership/sign differs')
        ts=(F(0),F(1));ids=(e.start,e.end)
        if c.orientation<0:ts,ids=ts[::-1],ids[::-1]
        require(type(u.parameters) is tuple and all(type(t) is F for t in u.parameters) and u.parameters==ts and
                type(u.vertices) is tuple and all(type(i) is int for i in u.vertices) and u.vertices==ids,
                'directed coedge parameter/identity chain differs')
        require(type(u.uv) is tuple and len(u.uv)==2,'original UV occurrence inventory differs')
        for uv,t in zip(u.uv,ts):
            point(uv,2,'original coedge UV');require(uv==original_uv(c,t),'original directed UV occurrence differs')
        uses[c.id]=u
    words={}
    for face in body.faces:
        word=[]
        require(len(face.loop_slots)==1,'expected one original convex loop')
        for cs in body.loops[face.loop_slots[0]].coedge_slots:
            c=body.coedges[cs];e=body.edges[c.edge_slot];ids=(e.start,e.end)
            if c.orientation<0:ids=ids[::-1]
            if word:require(word[-1]==ids[0],'original directed loop does not join')
            word.extend(ids if not word else ids[1:])
        require(word[0]==word[-1],'original directed loop does not close')
        words[face.id]=tuple(word[:-1])
    expected={fid:expected_chain((word,)) for fid,word in words.items()}
    chains={};rounded_chains={};incidence=defaultdict(list);links=defaultdict(list);face_counts=Counter()
    for index,t in enumerate(mesh.triangles):
        require(type(t.id) is int and t.id==index and type(t.vertices) is tuple and len(t.vertices)==3 and
                all(type(i) is int and 0<=i<2*n for i in t.vertices) and len(set(t.vertices))==3,
                'triangle dense identity/index/nondegeneracy differs')
        require(type(t.face_id) is int and 1<=t.face_id<=n+2,'foreign triangle original face')
        f=body.faces[t.face_id-1];face_counts[f.id]+=1
        require(type(t.corners) is tuple and len(t.corners)==3,'triangle corner occurrences differ')
        for vid,c in zip(t.vertices,t.corners):
            require(type(c.chart_id) is int and c.chart_id==body.charts[f.chart_slots[0]].id,
                    'corner chart/face ownership differs')
            point(c.uv,2,'original chart-local corner UV')
            require(all(0<=x<=1 for x in c.uv),'corner source parameter domain differs')
            require(patch_point(body.charts[c.chart_id-1].patch,c.uv)==positions[vid],
                    'corner original patch/UV does not reproduce shared vertex')
        for data,collection in ((positions,chains),(rounded,rounded_chains)):
            if f.id<=2:
                require(all(data[i][2]==data[0 if f.id==1 else n][2] for i in t.vertices),
                        'triangle left its actual cap plane')
                projected=tuple(data[i][:2] for i in t.vertices)
                sign=f.orientation
            else:
                k=f.id-3;a=data[k];b=data[(k+1)%n];d=sub(b[:2],a[:2]);den=dot(d,d)
                require(den>0,'actual side edge collapsed')
                projected=[]
                for vid in t.vertices:
                    p=data[vid];lam=dot(sub(p[:2],a[:2]),d)/den
                    require(tuple(a[j]+lam*d[j] for j in range(2))==p[:2] and 0<=lam<=1 and
                            data[0][2]<=p[2]<=data[n][2],'triangle left its actual side rectangle')
                    projected.append((lam,p[2]))
                sign=1
            require(turn(*projected)*sign>0,'actual triangle orientation/collapse differs')
            chain=collection.setdefault(f.id,Counter())
            for a,b in zip(t.vertices,t.vertices[1:]+t.vertices[:1]):chain_add(chain,a,b)
        for a,b,c in ((t.vertices[0],t.vertices[1],t.vertices[2]),(t.vertices[1],t.vertices[2],t.vertices[0]),(t.vertices[2],t.vertices[0],t.vertices[1])):
            incidence[min(a,b),max(a,b)].append((a,b,index));links[a].append((b,c))
    for collection in (chains,rounded_chains):
        require(set(collection)==set(expected),'missing original whole face')
        require(all(normalized(collection[fid])==word for fid,word in expected.items()),
                'full positive triangle boundary chain does not cover original domain')
    neighbors=defaultdict(set)
    for records in incidence.values():
        require(len(records)==2 and records[0][:2]==records[1][:2][::-1] and records[0][2]!=records[1][2],
                'mesh edge does not have two opposite distinct triangle uses')
        a,b=records[0][2],records[1][2];neighbors[a].add(b);neighbors[b].add(a)
    require(set(links)==set(range(2*n)),'unused physical mesh vertex')
    for members in links.values():
        nexts={};indegree=Counter()
        for a,b in members:
            require(a not in nexts,'vertex link branches');nexts[a]=b;indegree[b]+=1
        require(set(nexts)==set(indegree) and all(v==1 for v in indegree.values()),'vertex link is not cyclic')
        start=next(iter(nexts));seen=set();cursor=start
        while cursor not in seen:seen.add(cursor);cursor=nexts[cursor]
        require(cursor==start and len(seen)==len(nexts),'vertex link has multiple pinched cycles')
    seen=set();todo=[0]
    while todo:
        cursor=todo.pop()
        if cursor in seen:continue
        seen.add(cursor);todo.extend(neighbors[cursor]-seen)
    require(len(seen)==len(mesh.triangles),'mesh shell is disconnected')
    euler=2*n-len(incidence)+len(mesh.triangles)
    require(euler==2,'actual extrusion shell genus differs')
    require(len(mesh.triangles)==4*n-4,'complete convex triangulation count differs')
    return dict(vertices=2*n,triangles=len(mesh.triangles),mesh_edges=len(incidence),
                boundary_uses=len(uses),faces=len(face_counts),genus=0)
