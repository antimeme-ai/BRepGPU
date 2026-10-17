"""Original source -> native envelopes/assembly -> exact external source oracle."""
from dataclasses import replace
from fractions import Fraction as F
from itertools import product
from pathlib import Path
import argparse
import math
import subprocess
import sys
import tempfile

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'python'))
from brepgpu.imported_source import (ImportedSource, Product, SolidDefinition, Usage,
    Vertex, Edge, Coedge, Loop, Face, Carrier, Curve, SurfaceSpan, IDENTITY,
    ZERO_FRAME, write_source)


def authored():
    products = [Product(100, 'root', 'assembly')]
    definitions, vertices, edges, uses, loops, faces, carriers, curves, usages = ([] for _ in range(9))
    axes = (1., .5, 0., .25, 2., .5, .5, -.25, 1.5)
    # The last face shares the FIRST original carrier, with a larger guide box.
    for i, kind in enumerate((1, 2, 3, 4, 5, 6, 3, 1)):
        base, definition, pid = 10000 + i*100, 200+i, 101+i
        origin = (3.+30*i, -2.+20*i, 5.-10*i) if i != 7 else (3., -2., 5.)
        frame = (*origin, *axes)
        cid = 1000+i if i != 7 else 1000
        if i != 7:
            radius = (2., .5 if kind != 3 or i == 2 else 2.**-40)
            if kind == 6:
                points = [(origin[0]+u, origin[1]+v, origin[2]+z)
                          for u,v,z in ((0,0,0), (0,1,-2), (1,0,3), (1,1,1))]
                controls = tuple(x for point,w in zip(points,(1,2,4,8))
                                 for x in (*[float(F(p)*w) for p in point],float(w)))
                carriers.append(Carrier(cid,kind,frame,radius,(100.,101.,100.,101.),
                    (1,1),(-1.,0.,1.,2.),(1,1,1,1),(-1.,0.,1.,2.),(1,1,1,1),(2,2),
                    controls,(SurfaceSpan((0.,1.,0.,1.),(1,1),(999.,999.,999.,1.)*4),)))
            else:
                carriers.append(Carrier(cid,kind,frame,radius,(0.,1.,0.,1.)))
        uv = tuple((float(u+(2 if i == 7 else 0)),float(v))
                   for u,v in ((0,0),(1,0),(1,1),(0,1)))
        xyz = [tuple(origin[a]+axes[a]*u+axes[3+a]*v for a in range(3)) for u,v in uv]
        if kind == 6:
            xyz[0] = tuple(origin[a]-5-a for a in range(3))
        for j,p in enumerate(xyz):
            vertices.append(Vertex(base+j,p,0.))
        for j in range(4):
            a,b = xyz[j],xyz[(j+1)%4]
            ua,ub = uv[j],uv[(j+1)%4]
            ce, guide, eid, uid = base+10+j,base+20+j,base+30+j,base+40+j
            if j == 0 and i in (3,4):
                curves.append(Curve(ce,3,2 if i == 3 else 3,frame,(3.,.5),(0.,1.)))
            elif j == 1 and i == 0:
                direction3 = tuple(y-x for x,y in zip(a,b))
                curves.append(Curve(ce,3,1,(*a,*direction3,*(0.,)*6),(0.,0.),(-2.,3.)))
            else:
                controls = tuple(x for point,w in ((a,2.),(b,4.))
                                 for x in (*[p*w for p in point],w))
                if i == 5 and j == 0:
                    curves.append(Curve(ce,3,5,ZERO_FRAME,(0.,0.),(100.,101.),1,
                        (-1.,0.,1.,2.),(1,1,1,1),controls))
                else:
                    curves.append(Curve(ce,3,4,ZERO_FRAME,(0.,0.),(0.,1.),1,controls=controls))
            direction = tuple(ub[k]-ua[k] for k in range(2))
            curves.append(Curve(guide,2,1,(*ua,0.,*direction,0.,*(0.,)*6),
                                (0.,0.),(0.,1.)))
            # Both descending physical intervals and kind2 mapped s are covered.
            descending = j == 1
            domain = (1.,0.) if descending else (0.,1.)
            edges.append(Edge(eid,(base+j,base+(j+1)%4),ce,domain,False,True,False,0.))
            mapping = ((1.,0.),(.5,1.),(0.,0.)) if descending else ()
            uses.append(Coedge(uid,eid,base+50,1,guide,(0.,1.),
                               parameter_map_kind=2 if descending else 1,parameter_map=mapping))
        loops.append(Loop(base+50,base+60,True,tuple(base+40+j for j in range(4))))
        faces.append(Face(base+60,definition,cid,1,(base+50,),0.))
        definitions.append(SolidDefinition(definition,pid,(base+60,),
            tuple(base+30+j for j in range(4)),tuple(base+j for j in range(4))))
        products.append(Product(pid,f'family {kind}',f'family {i}',definition))
        if i != 6:
            usages.append(Usage(300+i,100,pid,'placement',IDENTITY))
    # Three tiny nonrigid perturbations are source-admitted, but compose to an
    # exact value that intermediate binary64 matrix multiplication would lose.
    products.extend(Product(500+i,f'level{i}','assembly') for i in range(3))
    first = (1.,0.,0.,13.,0.,1.,0.,-7.,0.,0.,1.,.5)
    scaled = list(IDENTITY)
    scaled[0] = 1.+2.**-40
    scaled[1] = 2.**-42
    usages.extend((Usage(401,100,500,'outer',first),
        Usage(402,500,501,'scale1',tuple(scaled)),Usage(403,501,502,'scale2',tuple(scaled)),
        Usage(404,502,101,'scale3',tuple(scaled))))
    return ImportedSource('a9'*32,1.,0.,100,'independent adaptive assembly',tuple(products),
        tuple(definitions),tuple(usages),tuple(vertices),tuple(edges),tuple(uses),tuple(loops),
        tuple(faces),tuple(carriers),tuple(curves))


def interval_union(boxes):
    return tuple(value for a in range(len(boxes[0])//2)
                 for value in (min(b[2*a] for b in boxes),max(b[2*a+1] for b in boxes)))


def point_box(point):
    return tuple(F(x) for x in point for _ in range(2))


def original_curve_box(curve,domain):
    if curve.kind == 1:
        return interval_union([point_box(tuple(F(curve.frame[a])+F(t)*F(curve.frame[3+a])
            for a in range(curve.dimension))) for t in domain])
    if curve.kind <= 3:
        return tuple(value for a in range(curve.dimension)
            for value in (F(curve.frame[a])-abs(F(curve.radii[0])*F(curve.frame[3+a]))-
                          abs(F(curve.radii[1])*F(curve.frame[6+a])),
                          F(curve.frame[a])+abs(F(curve.radii[0])*F(curve.frame[3+a]))+
                          abs(F(curve.radii[1])*F(curve.frame[6+a]))))
    stride = curve.dimension+1
    points = [tuple(F(curve.controls[i+a])/F(curve.controls[i+curve.dimension])
                    for a in range(curve.dimension)) for i in range(0,len(curve.controls),stride)]
    return interval_union([point_box(p) for p in points])


def dot(a,b):
    return sum(x*y for x,y in zip(a,b))


def expected(src):
    cs,vs,es = ({x.id:x for x in getattr(src,name)} for name in ('curves','vertices','edges'))
    physical = {e.id:interval_union([point_box(vs[v].xyz) for v in e.vertex_ids]+
        [original_curve_box(cs[e.curve_id],e.parameter_range)]) for e in src.edges}
    profiles = {}
    for face in src.faces:
        c = src.carrier(face.carrier_id)
        box = tuple(map(F,(0.,1.,0.,1.) if c.kind == 6 else c.uv_bounds))
        for loop_id in face.loop_ids:
            for uid in src.loop(loop_id).coedge_ids:
                u = src.coedge(uid)
                domain = tuple(pair[1] for pair in u.parameter_map) if u.parameter_map_kind == 2 else es[u.edge_id].parameter_range
                if c.kind != 6:
                    box = interval_union((box,original_curve_box(cs[u.curve_id],domain)))
                    if c.kind in (1,2):
                        x,y,z = (tuple(map(F,c.frame[i:i+3])) for i in (3,6,9))
                        duals = []
                        if c.kind == 1:
                            xx,xy,yy = dot(x,x),dot(x,y),dot(y,y)
                            det = xx*yy-xy*xy
                            duals = [(0,tuple((yy*a-xy*b)/det for a,b in zip(x,y))),
                                     (1,tuple((xx*b-xy*a)/det for a,b in zip(x,y)))]
                        else:
                            cross = (x[1]*y[2]-x[2]*y[1],x[2]*y[0]-x[0]*y[2],x[0]*y[1]-x[1]*y[0])
                            det = dot(cross,z)
                            duals = [(1,tuple(a/det for a in cross))]
                        box = list(box)
                        for axis,dual in duals:
                            corners = product(*(physical[u.edge_id][2*a:2*a+2] for a in range(3)))
                            values = [dot(tuple(p[a]-F(c.frame[a]) for a in range(3)),dual) for p in corners]
                            box[2*axis] = min(box[2*axis],min(values))
                            box[2*axis+1] = max(box[2*axis+1],max(values))
                        box = tuple(box)
        profiles[c.id] = interval_union((box,profiles[c.id])) if c.id in profiles else box
    def outward(x,upper):
        y = float(x)
        if (F(y)<x if upper else F(y)>x):
            y = math.nextafter(y,math.inf if upper else -math.inf)
        return F(y)
    profiles = {k:tuple(outward(x,i%2) for i,x in enumerate(v)) for k,v in profiles.items()}
    cb = {}
    for c in src.carriers:
        if c.kind == 6:
            cb[c.id] = original_curve_box(Curve(1,3,4,ZERO_FRAME,(0.,0.),(0.,1.),3,
                                               controls=c.controls),(0.,1.))
            continue
        u0,u1,v0,v1 = profiles[c.id]
        out = []
        for a in range(3):
            o,x,y,z = (F(c.frame[i+a]) for i in (0,3,6,9))
            r,minor = map(lambda v:abs(F(v)),c.radii)
            if c.kind == 1:
                vals = [o+x*u+y*v for u,v in product((u0,u1),(v0,v1))]
                out.extend((min(vals),max(vals)))
            else:
                av = max(abs(v0),abs(v1))
                extent = ({2:r*(abs(x)+abs(y))+av*abs(z),
                           3:(r+av)*(abs(x)+abs(y))+av*abs(z),
                           4:r*(abs(x)+abs(y)+abs(z)),
                           5:(r+minor)*(abs(x)+abs(y))+minor*abs(z)})[c.kind]
                out.extend((o-extent,o+extent))
        cb[c.id] = tuple(out)
    db = {d.id:interval_union([point_box(vs[v].xyz) for v in d.vertex_ids]+
        [physical[e] for e in d.edge_ids]+[cb[src.face(f).carrier_id] for f in d.face_ids]) for d in src.definitions}
    return db,profiles


def rational(word):
    sign,n,d = word.split(':')
    return int(sign)*F(int(n,16),int(d,16))


def compose(a,b):
    return tuple(sum(a[4*i+k]*b[4*k+j] for k in range(3))+(a[4*i+3] if j == 3 else 0)
                 for i in range(3) for j in range(4))


def check(src,text):
    boxes,profiles = expected(src)
    actual_boxes,actual_profiles,allowances,instances = {},{},{},[]
    for line in text.splitlines():
        row = line.split('\t')
        if row[0] == 'definition':
            actual_boxes[int(row[1])] = tuple(map(rational,row[2:]))
        elif row[0] == 'carrier':
            actual_profiles[int(row[1])] = tuple(map(rational,row[2:6]))
            assert actual_profiles[int(row[1])] == tuple(F(float.fromhex(v)) for v in row[6:])
        elif row[0] == 'allowance':
            allowances[int(row[1])] = tuple(map(rational,row[2:]))
        elif row[0] == 'instance':
            instances.append((int(row[1]),int(row[2]),tuple(F(float.fromhex(v)) for v in row[3:15]),tuple(map(int,row[15:]))))
        else:
            raise AssertionError('unknown native output row')
    assert boxes == actual_boxes, 'whole original source envelope differs from exact model'
    assert profiles == actual_profiles, 'original guide/natural-domain profile differs'
    assert set(allowances) == set(boxes)
    uses = {u.id:u for u in src.usages}
    epsilon,delta = F(.001),F(.0001)
    for pid,did,stored,path in instances:
        true = tuple(map(F,IDENTITY))
        for uid in path:
            true = compose(true,tuple(map(F,uses[uid].transform)))
        assert stored == tuple(F(float(v)) for v in true), 'matrix not rounded once'
        local,node = allowances[did]
        assert 0 < local <= epsilon and 0 < node <= delta
        # Independent row/column induced norm bound for these nearidentity authored
        # matrices (including translations): do not replay production Gram rows.
        columns = max(sum(abs(true[4*i+j]) for i in range(3)) for j in range(3))
        rows = max(sum(abs(true[4*i+j]) for j in range(3)) for i in range(3))
        assert node**2*columns*rows <= delta**2
        # These matrices have a small enough norm gap that the true largest
        # singular-value square is bounded tightly by a two-dimensional formula.
        aa = sum(true[4*i]**2 for i in range(3))
        bb = sum(true[4*i+1]**2 for i in range(3))
        ab = sum(true[4*i]*true[4*i+1] for i in range(3))
        # sqrt(lambda_max) <= max(column lengths)+small shear; choosing the
        # exact independent L1/Linf bound above can be looser than production.
        # Verify every principal direction by exact squared 2x2 PSD instead.
        for corner in product(*( (boxes[did][2*a]-epsilon,boxes[did][2*a+1]+epsilon) for a in range(3))):
            errors = [true[4*i+3]-stored[4*i+3]+sum((true[4*i+j]-stored[4*i+j])*corner[j] for j in range(3)) for i in range(3)]
            eta = sum(abs(v) for v in errors) # sufficient L1 storage departure
            remain = epsilon-eta
            assert remain > 0
            cap = (remain/local)**2
            assert cap >= 1 and cap >= aa and cap >= bb and (cap-aa)*(cap-bb) >= ab*ab
    assert len(instances) == len(src.usages)-3, 'missing reused placement or invented unplaced instance'
    assert allowances[206] == (epsilon,delta), 'unplaced definition changed quality'
    print(f'PASS\t{len(boxes)} original envelopes, {len(profiles)} carrier profiles, {len(instances)} placements')


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--fixture',required=True,type=Path)
    args = parser.parse_args()
    with tempfile.TemporaryDirectory() as tmp:
        path = Path(tmp)/'original.bgsource'
        src = authored()
        write_source(src,path)
        result = subprocess.run((str(args.fixture.resolve()),str(path)),capture_output=True,text=True,check=True)
        check(src,result.stdout)
        for kind in ('lift','map3'):
            use = src.coedges[0]
            use = replace(use,periodic_shift=(1,0)) if kind == 'lift' else replace(use,parameter_map_kind=3,parameter_map=((0.,0.),(1.,1.)))
            write_source(replace(src,coedges=(use,*src.coedges[1:])),path)
            refused = subprocess.run((str(args.fixture.resolve()),str(path)),capture_output=True,text=True)
            assert refused.returncode != 0 and 'FAIL\t' in refused.stderr, 'unsupported original profile admitted'
        raw = src.curves[1]
        huge = replace(raw,kind=4,degree=1,controls=(1.e308,0.,2.**-100)*2)
        write_source(replace(src,curves=(src.curves[0],huge,*src.curves[2:])),path)
        refused = subprocess.run((str(args.fixture.resolve()),str(path)),capture_output=True,text=True)
        assert refused.returncode != 0 and 'FAIL\tunresolved\tadaptive carrier profile has no finite binary64 enclosure' in refused.stderr, 'unrepresentable profile did not fail as unresolved'


if __name__ == '__main__':
    main()
