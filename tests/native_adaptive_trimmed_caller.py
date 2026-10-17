"""Original source -> complete selected-face caller -> exact output oracle.

Python is the external test boundary only. Product geometry, construction and
GPU dispatch remain native. Expected boundary comes from the real BGACRV output
matched to original face/use inventory, not from a face artifact's own cycles.
"""
from collections import Counter
from dataclasses import replace
from fractions import Fraction as F
from pathlib import Path
import argparse
import math
import subprocess
import sys
import tempfile
import time
from types import SimpleNamespace

sys.path.insert(0, str(Path(__file__).resolve().parent))
from native_adaptive_curves_caller import source as curve_source, read as read_curves
from native_adaptive_curves_caller import write_source, Vertex, Curve, Edge, Coedge, Loop, ZERO_FRAME
from native_adaptive_curves_caller import Reader, check as check_plane_curves
from face_domain_oracle import qualify_chain
from brepgpu.imported_source import Carrier, SurfaceSpan


def plane_source(holes=(((F(251,512), F(251,512)),
                          (F(251,512), F(253,512)),
                          (F(253,512), F(253,512)),
                          (F(253,512), F(251,512))),)):
    """A tiny hole is inside the initial leaf; corner-only masking loses it."""
    src = curve_source('plain')
    vertices, curves, edges, uses, loops = (list(getattr(src, key)) for key in
        ('vertices', 'curves', 'edges', 'coedges', 'loops'))
    for h, polygon in enumerate(holes):
        n, base, loop = len(polygon), 10000+100*h, 1800+h
        for i, a in enumerate(polygon):
            b = polygon[(i+1) % n]
            a, b = tuple(map(float, a)), tuple(map(float, b))
            vertices.append(Vertex(base+i, (*a, 0.), 0.))
            curves.extend((
                Curve(base+20+i, 3, 4, ZERO_FRAME, (0.,0.), (0.,1.), 1,
                      controls=(*a,0.,1.,*b,0.,1.)),
                Curve(base+30+i, 2, 4, ZERO_FRAME, (0.,0.), (0.,1.), 1,
                      controls=(*a,1.,*b,1.))))
            edges.append(Edge(base+40+i, (base+i, base+(i+1)%n), base+20+i,
                              (0.,1.), True, True, False, 0.))
            uses.append(Coedge(base+50+i, base+40+i, loop, 1, base+30+i,
                                (0.,1.), reconstructed=True))
        loops.append(Loop(loop, 900, False, tuple(base+50+i for i in range(n))))
    definition = replace(src.definitions[0], edge_ids=tuple(e.id for e in edges),
                         vertex_ids=tuple(v.id for v in vertices))
    face = replace(src.faces[0], loop_ids=tuple(loop.id for loop in loops))
    return replace(src, definitions=(definition,), faces=(face,), vertices=tuple(vertices),
                   edges=tuple(edges), coedges=tuple(uses), loops=tuple(loops), curves=tuple(curves))


def source_case(family):
    if family == 'plane_authoritative':
        src = plane_source()
        curves = tuple(replace(curve,degree=3,controls=(0.,0.,0.,1.)*3+(1.,0.,0.,1.))
                       if curve.id == 2000 else
                       replace(curve,degree=2,controls=(0.,0.,1.,0.,0.,1.,1.,0.,1.))
                       if curve.id == 3000 else curve for curve in src.curves)
        return replace(src,curves=curves,coedges=tuple(
            replace(use,reconstructed=False) if use.id == 500 else use
            for use in src.coedges)), 'plane'
    if family == 'plane_many_holes':
        holes = []
        for row in range(4):
            for col in range(4):
                u,v = F(2+3*col,16),F(2+3*row,16)
                d = F(1,32)
                holes.append(((u,v),(u,v+d),(u+d,v+d),(u+d,v)))
        return plane_source(tuple(holes)), 'plane'
    if family == 'plane_reversed':
        src = plane_source()
        return replace(src,coedges=tuple(replace(use,sense=-use.sense) for use in src.coedges),
                       loops=tuple(replace(loop,coedge_ids=loop.coedge_ids[::-1])
                                   for loop in src.loops)), 'plane'
    return (plane_source() if family == 'plane' else curved_source(family)), family


def orient(a, b, c):
    return (b[0]-a[0])*(c[1]-a[1]) - (b[1]-a[1])*(c[0]-a[0])


def add(a,b):
    result = dict(a)
    for key,value in b.items():
        result[key] = result.get(key,F(0))+value
    return {key:value for key,value in result.items() if value}


def scale(a,b):
    return {key:value*b for key,value in a.items() if value*b}


def mul(a,b):
    result = {}
    for (i,j),x in a.items():
        for (k,l),y in b.items():
            key = i+k,j+l
            result[key] = result.get(key,F(0))+x*y
    return {key:value for key,value in result.items() if value}


def source_power(kind, u, v):
    """Authored ORIGINAL power model, independent of native restricted nets."""
    one = {(0,0):F(1)}
    if kind == 'weighted':
        w = add(one,v)
        return (mul(u,w),scale(v,2),scale(mul(u,v),2)),w
    if kind == 'bowl':
        return (u,v,add(add(scale(mul(u,u),F(3,4)),scale(mul(v,v),F(1,4))),mul(u,v))),one
    if kind == 'saddle':
        return (u,v,mul(u,v)),one
    return (u,v,{}),one


def source_value(kind,u,v):
    n,w = source_power(kind,{(0,0):u},{(0,0):v})
    return tuple(axis.get((0,0),F(0))/w[(0,0)] for axis in n)


def bernstein(poly, degree):
    # Independent monomial->simplex identity, no source blossom/restricted net.
    def coefficient(i,j):
        value = F(0)
        for (a,b),power in poly.items():
            if i >= a and j >= b:
                value += power*F(math.comb(i,a)*math.comb(j,b),
                                 math.comb(degree,a+b)*math.comb(a+b,a))
        return value
    return tuple(coefficient(i,j) for i in range(degree+1) for j in range(degree+1-i))


def prove_facet(kind, uv, xyz, upper, depth=0):
    u,v = ({(0,0):uv[0][axis], (1,0):uv[1][axis]-uv[0][axis],
            (0,1):uv[2][axis]-uv[0][axis]} for axis in range(2))
    n,w = source_power(kind,u,v)
    residual = []
    for axis in range(3):
        affine = {(0,0):xyz[0][axis],(1,0):xyz[1][axis]-xyz[0][axis],
                  (0,1):xyz[2][axis]-xyz[0][axis]}
        residual.append(add(n[axis],scale(mul(w,affine),-1)))
    degree = max((sum(key) for poly in (*residual,w) for key in poly),default=0)
    weights = bernstein(w,degree)
    assert min(weights) > 0, 'original source denominator not positive'
    nets = [bernstein(poly,degree) for poly in residual]
    if max(sum(axis[i]**2 for axis in nets) for i in range(len(weights))) <= upper**2*min(weights)**2:
        return
    # A source counterexample fails immediately; child L is restricted by
    # averaging ACTUAL XYZ, never replaced by a source sample.
    center = tuple(sum(p[a] for p in uv)/3 for a in range(2))
    q = tuple(sum(p[a] for p in xyz)/3 for a in range(3))
    truth = source_value(kind,*center)
    assert sum((truth[a]-q[a])**2 for a in range(3)) <= upper**2, 'original whole-facet counterexample'
    assert depth < 18, 'independent original residual bound unresolved'
    p = [*uv, tuple((uv[0][a]+uv[1][a])/2 for a in range(2)),
         tuple((uv[1][a]+uv[2][a])/2 for a in range(2)),
         tuple((uv[2][a]+uv[0][a])/2 for a in range(2))]
    q = [*xyz, tuple((xyz[0][a]+xyz[1][a])/2 for a in range(3)),
         tuple((xyz[1][a]+xyz[2][a])/2 for a in range(3)),
         tuple((xyz[2][a]+xyz[0][a])/2 for a in range(3))]
    for ids in ((0,3,5),(3,1,4),(5,4,2),(3,4,5)):
        prove_facet(kind,[p[i] for i in ids],[q[i] for i in ids],upper,depth+1)


def curved_source(kind='bowl', holes=()):
    src = plane_source(holes)
    degree = 1 if kind in ('weighted','saddle') else 2
    net = []
    for i in range(degree+1):
        for j in range(degree+1):
            u,v = F(i,degree),F(j,degree)
            if kind == 'weighted':
                xyz,w = (u,v,u*v),1+v
            elif kind == 'saddle':
                xyz,w = (u,v,u*v),1
            else:
                xyz,w = (u,v,F(3,4)*(i==2)+F(1,4)*(j==2)+u*v),1
            # Imported rational tensor controls are HOMOGENEOUS original values.
            net.extend(float(value) for value in (*[x*w for x in xyz],w))
    carrier = Carrier(9,6,ZERO_FRAME,(0.,0.),(0.,1.,0.,1.),(degree,degree),
        (0.,1.),(degree+1,degree+1),(0.,1.),(degree+1,degree+1),(degree+1,degree+1),
        tuple(net),(SurfaceSpan((0.,1.,0.,1.),(degree,degree),tuple(net)),))
    original = {c.id:c for c in src.curves}
    curves = []
    for edge in src.edges:
        guide = next(c for c in src.coedges if c.edge_id == edge.id)
        raw = original[guide.curve_id]
        a,b = tuple(map(F,raw.controls[:2])),tuple(map(F,raw.controls[3:5]))
        u,v = ({(0,0):a[axis],(1,0):b[axis]-a[axis]} for axis in range(2))
        n,w = source_power(kind,u,v)
        curve_degree = max(sum(key) for poly in (*n,w) for key in poly)
        controls = []
        for k in range(curve_degree+1):
            def control(poly):
                return sum(value*F(math.comb(k,i),math.comb(curve_degree,i))
                           for (i,j),value in poly.items() if i <= k and j == 0)
            # Original curves also store homogeneous controls, as the native
            # source protocol and existing source-reader fixtures specify.
            controls.extend(float(value) for value in (*[control(axis) for axis in n],control(w)))
        curves.append(replace(original[edge.curve_id],degree=curve_degree,controls=tuple(controls)))
    curves.extend(c for c in src.curves if c.dimension == 2)
    vertices = tuple(replace(vertex,xyz=tuple(map(float,source_value(kind,*map(F,vertex.xyz[:2])))))
                     for vertex in src.vertices)
    return replace(src,carriers=(carrier,),curves=tuple(curves),vertices=vertices)


def original_boundary(src, boundary, bindings, chart):
    """Expected directed source edges by ORIGINAL face loop/use order."""
    face = src.faces[0]
    source_uses = {use.id: use for use in src.coedges}
    source_loops = {loop.id: loop for loop in src.loops}
    actual_uses = {use['ids'][0]: use for use in boundary['uses']}
    assert set(actual_uses) == set(source_uses), 'incomplete original use inventory'
    expected, required = [], set()
    for loop_id in face.loop_ids:
        loop = source_loops[loop_id]
        cycle = []
        for use_id in loop.coedge_ids:
            use, actual = source_uses[use_id], actual_uses[use_id]
            first, count = actual['sites']
            row_sites = list(range(first, first+count))
            assert count >= 2
            # BGACRV sites already follow the original coedge traversal.
            chain = [bindings[i] for i in row_sites]
            if cycle:
                assert cycle[-1] == chain[0], 'original loop junction not unified'
                cycle.extend(chain[1:])
            else:
                cycle.extend(chain)
        assert cycle[0] == cycle[-1], 'source cycle not closed'
        cycle.pop()
        assert len(set(cycle)) == len(cycle), 'source cycle repeats nonjoint site'
        area = sum(orient((F(0), F(0)), chart[a], chart[b])
                   for a, b in zip(cycle, (*cycle[1:], cycle[0])))
        assert area != 0, 'original selected cycle has zero area'
        if (area > 0) != loop.outer:
            cycle.reverse()
        required.update(cycle)
        expected.append(tuple(cycle))
    return tuple(expected), required


def read_face(path):
    r = Reader(path.read_bytes())
    magic, wire, profile, digest = r.unpack('<8sII32s')
    assert (magic, wire) == (b'BGATRM01', 1), 'wrong complete-face wire'
    keys = r.unpack('<9Q')
    orientation, policy = r.unpack('<iI')
    assert orientation in (-1, 1) and policy in (1, 2)
    ns, nb, nls, ne, nl, nc, nf, nr, nu, np = r.unpack('<10Q')
    assert max(ns, nb, nls, ne, nl, nc, nf, nr, nu, np) <= len(r.raw)//4
    metric, upper, node_source = r.exacts(4), r.exact(), r.exact()
    assert metric[0]*metric[3]-metric[1]*metric[2] > 0
    assert upper > 0 and node_source > 0
    roots, sites, bindings, edges, loops, components, facets, uses = [], [], [], [], [], [], [], []
    for _ in range(nr):
        carrier, su, sv, family, pu, pv, *box = r.unpack('<Q5I4d')
        assert family <= 5 and pu <= 3 and pv <= 3 and all(map(math.isfinite, box))
        assert box[0] < box[1] and box[2] < box[3]
        roots.append(dict(carrier=carrier, span=(su,sv), family=family,
                          degrees=(pu,pv), box=tuple(map(F,box))))
    packets = {}
    for _ in range(np):
        owner, revision, original, su, sv, *tail = r.unpack('<QQ3I4d2I')
        prepared, depth, nw = tuple(map(F,tail[:4])), tail[4], tail[5]
        assert original < nr and depth <= 128 and nw == (depth+31)//32
        assert all(math.isfinite(float(x)) for x in prepared)
        assert prepared[0] < prepared[1] and prepared[2] < prepared[3]
        assert owner not in packets
        words = r.unpack(f'<{nw}Q')
        if depth % 32:
            assert words[-1] >> (2*(depth%32)) == 0
        box, axis_depth = list(prepared), [0,0]
        for i in range(depth):
            step = (words[i//32] >> (2*(i%32))) & 3
            axis, child = step//2, step & 1
            axis_depth[axis] += 1
            mid = (box[2*axis]+box[2*axis+1])/2
            box[2*axis+(1-child)] = mid
        assert max(axis_depth) <= 64 and tuple(box) == r.exacts(4)
        if roots[original]['family'] == 5:
            assert roots[original]['span'] == (su,sv)
        assert all(roots[original]['box'][2*a] <= prepared[2*a] < prepared[2*a+1]
                   <= roots[original]['box'][2*a+1] for a in (0,1))
        packets[owner] = dict(revision=revision, original=original, box=tuple(box))
    for _ in range(ns):
        generator, revision, *xyz = r.unpack('<IQ3d')
        assert generator in (1,2,3) and all(map(math.isfinite,xyz))
        uv = r.exacts(2)
        authority = r.unpack('<I') if generator == 1 else r.unpack('<IIQ')
        assert (authority[0] < nr and authority[1] <= 4) if generator != 1 else True
        sites.append(dict(kind=generator, revision=revision, xyz=tuple(map(F,xyz)),
                          uv=uv, source=authority))
    for _ in range(nb):
        a,b = r.unpack('<II')
        assert b < ns
        bindings.append((a,b))
    loop_sites = r.unpack(f'<{nls}I')
    assert all(i < ns for i in loop_sites)
    for _ in range(ne):
        a,b,kind,owner,interval,tag = r.unpack('<3IQQI')
        assert a < ns and b < ns and a != b and kind in (1,2)
        edges.append(dict(sites=(a,b), kind=kind, key=(owner,interval,tag)))
    def checked_range(first, count, total):
        assert first <= total and count <= total-first
        return first, count
    for _ in range(nl):
        positive, sf, sn, ef, en, source_loop, adapter = r.unpack('<I5Qi')
        assert positive in (0,1) and sn >= 3 and sn == en and adapter in (-1,1)
        loops.append(dict(positive=positive, sites=checked_range(sf,sn,nls),
                          edges=checked_range(ef,en,ne), source_loop=source_loop, adapter=adapter))
    for _ in range(nc):
        identifier, revision, lf, ln, ff, fn = r.unpack('<6Q')
        components.append(dict(id=identifier, revision=revision,
            loops=checked_range(lf,ln,nl), facets=checked_range(ff,fn,nf)))
    for _ in range(nf):
        generator, owner, revision, ordinal, a,b,c,component = r.unpack('<IQQI4I')
        assert generator in (1,2) and max(a,b,c) < ns
        assert (component == 2**32-1) == (generator == 1)
        assert component == 2**32-1 or component < nc
        if generator == 1:
            assert owner in packets and packets[owner]['revision'] == revision
            box = packets[owner]['box']
            assert all(all(box[2*j] <= sites[i]['uv'][j] <= box[2*j+1] for j in (0,1))
                       for i in (a,b,c)), 'packet facet escapes semantic source leaf'
        facets.append(dict(key=(generator,owner,revision,ordinal), sites=(a,b,c),
                           component=component))
    for _ in range(nu):
        use, use_policy, image = r.unpack('<QII')
        assert use_policy in (1,2) and image in (0,1)
        assert bool(image) == (use_policy == 2)
        payload = None
        if image:
            raw, traversal, first, count = r.unpack('<QiQQ')
            assert traversal in (-1,1) and count >= 2
            payload = dict(raw=raw, traversal=traversal, sites=(first,count),
                           endpoints=r.exacts(4), covector=r.exacts(2), scalar=r.exacts(2))
        uses.append(dict(use=use, policy=use_policy, image=payload))
    assert r.at == len(r.raw), 'trailing complete-face bytes'
    return dict(profile=profile, digest=digest, key=keys, orientation=orientation, policy=policy,
        metric=metric, upper=upper, node_source=node_source, roots=roots, sites=sites,
        bindings=bindings, loop_sites=loop_sites, edges=edges, loops=loops,
        components=components, facets=facets, uses=uses)


def qualify_face(src, boundary, mesh, positional, node_cap, kind='plane'):
    assert mesh['digest'] == bytes.fromhex(src.source_sha256) == boundary['digest']
    face = src.faces[0]
    assert mesh['key'][6:] == (face.definition_id, face.id, face.carrier_id)
    assert mesh['orientation'] == face.orientation and mesh['profile'] == 1
    assert mesh['upper'] == F(positional) and mesh['node_source'] == F(node_cap)
    sites, bindings = mesh['sites'], dict(mesh['bindings'])
    assert len(bindings) == len(mesh['bindings']), 'duplicate occurrence binding'
    selected = [u for u in boundary['uses'] if u['ids'][1] == face.id]
    expected_sites = {i for use in selected for i in range(use['sites'][0], sum(use['sites']))}
    assert set(bindings) == expected_sites, 'omitted original face occurrence'
    assert {row['use'] for row in mesh['uses']} == {u['ids'][0] for u in selected}
    assert len({s['uv'] for s in sites}) == len(sites), 'undeclared coincident chart sites'
    for shared, construction in bindings.items():
        use_site, site = boundary['sites'][shared], sites[construction]
        assert site['kind'] == 1 and site['uv'] == use_site['chart'] == use_site['uv']
        actual = boundary['nodes'][site['source'][0]]
        expected = boundary['nodes'][use_site['node']]
        assert actual['key'] == expected['key'] and site['xyz'] == actual['xyz'] == expected['xyz']
        assert site['revision'] == actual['revision'] == expected['revision']
    for i,site in enumerate(sites):
        if site['kind'] == 1:
            assert i in bindings.values(), 'physical site has no original occurrence'
        else:
            root = mesh['roots'][site['source'][0]]
            assert root['carrier'] == face.carrier_id
            assert all(root['box'][2*a] <= site['uv'][a] <= root['box'][2*a+1] for a in (0,1))
    cycles, required = original_boundary(src, boundary, bindings, [s['uv'] for s in sites])
    case = SimpleNamespace(points={i:s['uv'] for i,s in enumerate(sites)}, loops=cycles)
    triangles = [facet['sites'] for facet in mesh['facets']]
    positive = sum(loop.outer for loop in src.loops if loop.id in face.loop_ids)
    qualify_chain(case, triangles, required_sites=case.points, positive_cycles=positive)
    assert required <= set(case.points)
    assert len({f['key'] for f in mesh['facets']}) == len(mesh['facets']), 'duplicate facet identity'
    expected_edges = Counter((a,b) for loop in cycles for a,b in zip(loop,loop[1:]+loop[:1]))
    physical_edges = Counter(e['sites'] for e in mesh['edges'] if e['kind'] == 1)
    assert physical_edges == expected_edges, 'protected source edges differ from original chain'
    valid_keys = {row['key'] for row in boundary['physical']}
    assert all(e['key'] in valid_keys for e in mesh['edges'] if e['kind'] == 1)
    carrier = next(c for c in src.carriers if c.id == face.carrier_id)
    for facet in mesh['facets']:
        points = [sites[i] for i in facet['sites']]
        a = tuple(points[1]['xyz'][j]-points[0]['xyz'][j] for j in range(3))
        b = tuple(points[2]['xyz'][j]-points[0]['xyz'][j] for j in range(3))
        cross = (a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0])
        assert any(cross), 'actual physical facet collapsed'
        if kind == 'plane':
            for point in points:
                u,v = point['uv']
                truth = tuple(F(carrier.frame[j])+u*F(carrier.frame[3+j])+v*F(carrier.frame[6+j])
                              for j in range(3))
                assert sum((truth[j]-point['xyz'][j])**2 for j in range(3)) <= mesh['upper']**2
        else:
            prove_facet(kind, [p['uv'] for p in points], [p['xyz'] for p in points], mesh['upper'])
    return mesh


def invoke(executable, src, output, *, backend=(), qualities=(0.01,), timeout=120,
           threads=1, boundary_only=False):
    source_path = output.with_suffix('.bgsource')
    write_source(src, source_path)
    args = [str(executable), '--source', str(source_path), '--definition', '200',
            '--output', str(output), '--node-source-mm', '1e-10',
            '--threads', str(threads), *backend]
    if not boundary_only:
        args.extend(('--face','900'))
    for quality in qualities:
        args.extend(('--positional-mm', str(quality)))
    start = time.perf_counter()
    proc = subprocess.run(args, capture_output=True, text=True, timeout=timeout)
    elapsed = time.perf_counter()-start
    assert proc.returncode == 0, f'complete-face operation failed after {elapsed:.6f}s: {proc.stderr}'
    return elapsed


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('executable', type=Path)
    parser.add_argument('output', type=Path)
    parser.add_argument('--write-fixture', action='store_true')
    parser.add_argument('--temporary-output', action='store_true',
                        help='run in a fresh temporary child of OUTPUT for repeatable CTest use')
    parser.add_argument('--family', choices=('plane','plane_many_holes','plane_reversed','plane_authoritative',
                                            'bowl','weighted','saddle'), default='plane')
    parser.add_argument('--boundary-only', action='store_true')
    parser.add_argument('--threads', type=int, default=1)
    parser.add_argument('--cuda-module', type=Path)
    parser.add_argument('--device', type=int, default=0)
    parser.add_argument('--positional-mm', action='append', type=float)
    args = parser.parse_args()
    src, kind = source_case(args.family)
    if args.write_fixture:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        write_source(src, args.output)
        return
    temporary = None
    if args.temporary_output:
        args.output.mkdir(parents=True, exist_ok=True)
        temporary = tempfile.TemporaryDirectory(prefix='face-',dir=args.output)
        args.output = Path(temporary.name)/'result'
    args.output.parent.mkdir(parents=True, exist_ok=True)
    # Output must be new. The harness owns only this explicit test directory.
    assert not args.output.exists(), 'test output already exists'
    qualities = args.positional_mm or (.01,)
    backend = [] if args.cuda_module is None else ['--backend','p100','--cuda-module',
                                                   str(args.cuda_module),'--device',str(args.device)]
    elapsed = invoke(args.executable, src, args.output, backend=backend, qualities=qualities,
                     threads=args.threads, boundary_only=args.boundary_only)
    callback = None if kind == 'plane' else lambda use,uv: source_value(kind,*uv)
    for query,quality in enumerate(qualities):
        boundary = read_curves(args.output / f'query-{query}.bgacrv')
        check_plane_curves(boundary, src, F(quality), F(1e-10), 'plain', callback)
        if args.family == 'plane_authoritative':
            use = next(use for use in boundary['uses'] if use['ids'][0] == 500)
            assert use['sites'][1] == 2 and use['intervals'][1] == 1, 'clock-only subdivision'
            image = use['whole_image']
            assert image and image['raw'] == 3000 and image['traversal'] == 1
            assert image['endpoints'] == (F(0),F(0),F(1),F(0))
            assert image['scalar'] == (F(0),image['covector'][0])
            # Independently authored images are [0,1]x{0}: t^3 and s^2 are
            # continuous monotone surjections although their clocks differ.
            interval = boundary['intervals'][use['intervals'][0]]
            assert interval['domain'] is None, 'whole image relabeled as a per-parameter bound'
        if not args.boundary_only:
            qualify_face(src, boundary, read_face(args.output / f'query-{query}.bgatrm'),
                         quality, 1e-10, kind)
    scope = 'boundary_prerequisite' if args.boundary_only else 'complete_face'
    print(f'{scope}_seconds\t{elapsed:.17g}')


if __name__ == '__main__':
    main()
