"""Complete original definition -> native component -> existing face oracle."""
from collections import Counter
from dataclasses import replace
from fractions import Fraction as F
from pathlib import Path
import argparse
import csv
import math
import subprocess
import tempfile

from native_adaptive_curves_caller import (
    source as plane_source, read as read_curves, check as check_curves,
    check_seam, curve_h, curve_local, whole_chord)
from native_adaptive_trimmed_caller import (
    read_face, qualify_face, original_boundary, cylinder_source, cylinder_trig,
    observed_features, face_semantics)
from brepgpu.imported_source import Vertex, Edge, Coedge, Loop, Face, Curve, Carrier, ZERO_FRAME, write_source


def box_source():
    base = plane_source('plain')
    xyz = tuple((float(i & 1), float((i >> 1) & 1), float((i >> 2) & 1)) for i in range(8))
    pairs = sorted((i, i ^ (1 << axis)) for i in range(8) for axis in range(3)
                   if not i & (1 << axis))
    vertices = tuple(Vertex(300+i, point, 0.) for i, point in enumerate(xyz))
    curves, edges, uses, loops, faces, carriers = ([] for _ in range(6))
    for i, (a, b) in enumerate(pairs):
        curves.append(Curve(2000+i, 3, 4, ZERO_FRAME, (0., 0.), (0., 1.), 1,
                            controls=(*xyz[a], 1., *xyz[b], 1.)))
        edges.append(Edge(400+i, (300+a, 300+b), 2000+i, (0., 1.), True, True, False, 0.))
    frames = (((0,0,0),(0,0,1),(0,1,0)), ((1,0,0),(0,1,0),(0,0,1)),
              ((0,0,0),(1,0,0),(0,0,1)), ((0,1,0),(0,0,1),(1,0,0)),
              ((0,0,0),(0,1,0),(1,0,0)), ((0,0,1),(0,1,0),(1,0,0)))
    for face, (origin, x, y) in enumerate(frames):
        # The last chart is mirrored. Its original physical word stays outward;
        # normalized chart traversal reverses and face orientation flips once.
        uv = ((0.,0.), (1.,0.), (1.,1.), (0.,1.))
        if face == 5:
            uv = tuple(reversed(uv))
        ids = []
        for u, v in uv:
            point = tuple(origin[a]+u*x[a]+v*y[a] for a in range(3))
            ids.append(xyz.index(point))
        for side, a in enumerate(ids):
            b = ids[(side+1) % 4]
            edge = pairs.index(tuple(sorted((a,b))))
            sense = 1 if (a,b) == pairs[edge] else -1
            # Pcurve parameter follows ORIGINAL spatial t, independently of use sense.
            first, last = (uv[side], uv[(side+1) % 4]) if sense == 1 else (
                uv[(side+1) % 4], uv[side])
            curve = 3000+4*face+side
            curves.append(Curve(curve, 2, 4, ZERO_FRAME, (0.,0.), (0.,1.), 1,
                                controls=(*first,1.,*last,1.)))
            uses.append(Coedge(500+4*face+side, 400+edge, 800+face, sense, curve,
                               (0.,1.), reconstructed=True))
        normal = (x[1]*y[2]-x[2]*y[1], x[2]*y[0]-x[0]*y[2], x[0]*y[1]-x[1]*y[0])
        carriers.append(Carrier(9+face, 1, tuple(map(float, (*origin,*x,*y,*normal))),
                                (0.,0.), (0.,1.,0.,1.)))
        loops.append(Loop(800+face, 900+face, True, tuple(500+4*face+i for i in range(4))))
        faces.append(Face(900+face, 200, 9+face, -1 if face == 5 else 1, (800+face,), 0.))
    definition = replace(base.definitions[0], face_ids=tuple(f.id for f in faces),
                         edge_ids=tuple(e.id for e in edges), vertex_ids=tuple(v.id for v in vertices))
    return replace(base, source_sha256='b6'*32, definitions=(definition,), vertices=vertices,
                   edges=tuple(edges), curves=tuple(curves), coedges=tuple(uses),
                   loops=tuple(loops), faces=tuple(faces), carriers=tuple(carriers))


def boundary_source():
    """An unaffected square and a real curved closed edge, with no sample count hint."""
    base = plane_source('plain')
    curves = []
    for curve in base.curves:
        h = list(curve.controls)
        for i in range(0, len(h), curve.dimension+1):
            h[i] += 3*h[i+curve.dimension]
        curves.append(replace(curve, controls=tuple(h)))
    vertices = tuple(replace(v, xyz=(v.xyz[0]+3,*v.xyz[1:])) for v in base.vertices)
    unit = base.carriers[0].frame
    curves.extend((Curve(99001,3,2,unit,(1.,1.),(0.,math.tau)),
                   Curve(99002,2,2,unit,(1.,1.),(0.,math.tau))))
    edges = (*base.edges, Edge(99003,(99000,99000),99001,(0.,math.tau),True,True,False,0.))
    vertices = (*vertices, Vertex(99000,(1.,0.,0.),0.))
    return replace(base, source_sha256='bc'*32, curves=tuple(curves), edges=edges, vertices=vertices,
        coedges=(*base.coedges,Coedge(99004,99003,99005,1,99002,(0.,math.tau),reconstructed=True)),
        loops=(*base.loops,Loop(99005,901,True,(99004,))),
        faces=(*base.faces,Face(901,200,10,1,(99005,),0.)),
        carriers=(replace(base.carriers[0],uv_bounds=(3.,4.,0.,1.)),
                  Carrier(10,1,unit,(0.,0.),(-2.,2.,-2.,2.))),
        definitions=(replace(base.definitions[0],face_ids=(900,901),
            edge_ids=tuple(e.id for e in edges),vertex_ids=tuple(v.id for v in vertices)),))


def coincident_source():
    base = plane_source('plain')
    # Distinct source identities at equal coordinates must never become a shared simplex.
    offset = 10000
    return replace(base, source_sha256='bf'*32,
        vertices=(*base.vertices,*(replace(v,id=v.id+offset) for v in base.vertices)),
        curves=(*base.curves,*(replace(c,id=c.id+offset) for c in base.curves)),
        edges=(*base.edges,*(replace(e,id=e.id+offset,curve_id=e.curve_id+offset,
             vertex_ids=tuple(v+offset for v in e.vertex_ids)) for e in base.edges)),
        coedges=(*base.coedges,*(replace(u,id=u.id+offset,edge_id=u.edge_id+offset,
             curve_id=u.curve_id+offset,loop_id=u.loop_id+offset) for u in base.coedges)),
        loops=(*base.loops,Loop(10800,901,True,tuple(10500+i for i in range(4)))),
        faces=(*base.faces,Face(901,200,9,1,(10800,),0.)),
        definitions=(replace(base.definitions[0],face_ids=(900,901),
             edge_ids=tuple(e.id+d for d in (0,offset) for e in base.edges),
             vertex_ids=tuple(v.id+d for d in (0,offset) for v in base.vertices)),))


def plane_value(src, use, uv):
    loop = next(loop for loop in src.loops if loop.id == use.loop_id)
    face = next(face for face in src.faces if face.id == loop.face_id)
    carrier = next(c for c in src.carriers if c.id == face.carrier_id)
    return tuple(F(carrier.frame[a])+uv[0]*F(carrier.frame[3+a])+uv[1]*F(carrier.frame[6+a])
                 for a in range(3))


def analytic_point_box(curve, t):
    if curve.kind in (4,5):
        h = curve_h(curve,t)
        return tuple((h[a]/h[3],)*2 for a in range(3))
    if curve.kind == 1:
        return tuple((F(curve.frame[a])+t*F(curve.frame[3+a]),)*2 for a in range(3))
    assert curve.kind == 2 and curve.radii == (1.,1.)
    sine, cosine = cylinder_trig(t)
    result = []
    for a in range(3):
        lo = hi = F(curve.frame[a])
        for scale, interval in ((F(curve.frame[3+a]),cosine),(F(curve.frame[6+a]),sine)):
            ends = tuple(scale*x for x in interval)
            lo += min(ends); hi += max(ends)
        result.append((lo,hi))
    return tuple(result)


def departure_squared(box, point):
    return sum(max(abs(lo-q),abs(hi-q))**2 for (lo,hi),q in zip(box,point))


def check_closed_plane_boundary(boundary, src, positional, node_cap):
    """Same original chord formula as the seam reader, using its rigorous Taylor helper."""
    assert boundary['digest'] == bytes.fromhex(src.source_sha256)
    assert boundary['definition'] == src.definitions[0].id
    assert [e['id'] for e in boundary['edges']] == [e.id for e in src.edges]
    assert {u['ids'][0] for u in boundary['uses']} == {u.id for u in src.coedges}
    nodes = boundary['nodes']
    originals = {e.id:e for e in src.edges}
    curves = {c.id:c for c in src.curves}
    vertices = {v.id:tuple(map(F,v.xyz)) for v in src.vertices}
    for edge in boundary['edges']:
        original = originals[edge['id']]
        assert edge['curve'] == original.curve_id and edge['vertices'] == original.vertex_ids
        assert edge['t'] == tuple(map(F,original.parameter_range))
        first,count = edge['nodes']; pfirst,pcount = edge['intervals']
        chain = nodes[first:first+count]
        assert count >= 2 and pcount == count-1 and len(chain) == count
        assert (chain[0]['t'],chain[-1]['t']) == edge['t']
        assert (chain[0]['xyz'],chain[-1]['xyz']) == tuple(vertices[v] for v in edge['vertices'])
        assert all(a['t'] < b['t'] for a,b in zip(chain,chain[1:]))
        curve = curves[original.curve_id]
        for row in chain:
            assert departure_squared(analytic_point_box(curve,row['t']),row['xyz']) <= positional**2
        for interval,a,b in zip(boundary['physical'][pfirst:pfirst+pcount],chain,chain[1:]):
            assert interval['t'] == (a['t'],b['t']) and interval['key'][0] == edge['id']
            assert interval['parameter'] <= positional
            if curve.kind in (4,5):
                whole_chord(curve_local(curve,a['t'],b['t']),F(0),F(1),a['xyz'],b['xyz'],
                            interval['parameter'],positional)
            else:
                # This authored circle has unit orthogonal axes, hence ||C''||=1.
                assert curve.frame == (0.,0.,0.,1.,0.,0.,0.,1.,0.,0.,0.,1.)
                curvature = (b['t']-a['t'])**2/8
                for cap in (positional,interval['parameter']):
                    assert cap >= curvature
                    assert max(departure_squared(analytic_point_box(curve,p['t']),p['xyz'])
                               for p in (a,b)) <= (cap-curvature)**2
    for row in boundary['uses']:
        use = next(u for u in src.coedges if u.id == row['ids'][0])
        edge = next(e for e in boundary['edges'] if e['id'] == use.edge_id)
        first,count = row['sites']; pfirst,pcount = row['intervals']
        ef,en = edge['nodes']; ei,ec = edge['intervals']
        order = list(range(ef,ef+en)); paired = boundary['physical'][ei:ei+ec]
        if use.sense < 0:
            order.reverse(); paired = list(reversed(paired))
        sites = boundary['sites'][first:first+count]
        assert [s['node'] for s in sites] == order and count == en and pcount == ec
        for site in sites:
            assert site['loop'] == use.loop_id and site['t'] == site['s']
            assert site['guard'] <= node_cap
            value = plane_value(src,use,site['uv'])
            assert departure_squared(analytic_point_box(curves[edge['curve']],site['t']),value) <= site['guard']**2
            for endpoint,t in enumerate(edge['t']):
                if site['t'] == t:
                    original_vertex = vertices[edge['vertices'][endpoint]]
                    assert sum((a-b)**2 for a,b in zip(original_vertex,value)) <= site['guard']**2
                    assert departure_squared(analytic_point_box(curves[edge['curve']],t),
                                             original_vertex) <= site['guard']**2
        for j,interval in enumerate(boundary['intervals'][pfirst:pfirst+pcount]):
            assert interval['sites'] == (first+j,first+j+1) and interval['key'] == paired[j]['key']
            assert interval['trace'] <= positional
            for site in sites[j:j+2]:
                value = plane_value(src,use,site['uv'])
                assert sum((a-b)**2 for a,b in zip(value,nodes[site['node']]['xyz'])) <= interval['trace']**2


def incident_chains(src, face, captured, final):
    loops = {loop.id for loop in src.loops if loop.face_id == face.id}
    edges = {use.edge_id for use in src.coedges if use.loop_id in loops}
    for edge_id in edges:
        chains = []
        for boundary in (captured,final):
            edge = next(e for e in boundary['edges'] if e['id'] == edge_id)
            first,count = edge['nodes']
            chains.append([(n['key'],n['revision'],n['t'],n['xyz'])
                           for n in boundary['nodes'][first:first+count]])
        assert chains[0] == chains[1], 'retained face uses stale INCIDENT physical chain'


def check_output(src, family, output, positional, lines=None, final_path=None):
    if lines is None:
        lines = [line.split('\t') for line in (output/'faces.tsv').read_text().splitlines()]
    definition = src.definitions[0]
    assert [int(row[0]) for row in lines] == sorted(definition.face_ids)
    assert all(len(row) == 3 for row in lines)
    final = read_curves(final_path or output/'final.bgacrv')
    if family == 'cylinder':
        check_seam(final,src,positional,F(1e-8))
    elif family == 'needs_boundary':
        check_closed_plane_boundary(final,src,positional,F(1e-8))
    else:
        check_curves(final,src,positional,F(1e-8),'plain',lambda use,uv:plane_value(src,use,uv))
    frontier = Counter()
    total = 0
    for identifier,curve_path,face_path in lines:
        face = next(face for face in src.faces if face.id == int(identifier))
        boundary,mesh = read_curves(output/curve_path),read_face(output/face_path)
        kind = 'cylinder' if family == 'cylinder' else 'plane'
        qualify_face(src,boundary,mesh,positional,F(1e-8),kind,face=face)
        assert {r['carrier'] for r in mesh['roots']} == {face.carrier_id}, 'other carrier leaked into face roots'
        assert len(mesh['roots']) == 1, 'incomplete/duplicate original analytic root closure'
        incident_chains(src,face,boundary,final)
        total += len(mesh['facets'])
        cycles,_ = original_boundary(src,boundary,dict(mesh['bindings']),[s['uv'] for s in mesh['sites']],face)
        for cycle in cycles:
            for a,b in zip(cycle,cycle[1:]+cycle[:1]):
                def physical(site):
                    row = mesh['sites'][site]
                    assert row['kind'] == 1
                    return boundary['nodes'][row['source'][0]]['key']
                pair = physical(a),physical(b)
                if face.orientation < 0:
                    pair = pair[::-1]
                frontier[pair] += 1
        if family == 'cylinder':
            observed_features('cylinder_seam',src,boundary,mesh)
    def signed(rows):
        result = Counter()
        for (a,b),count in rows.items():
            assert a != b, 'physical boundary atom collapsed'
            result[min(a,b),max(a,b)] += count if a < b else -count
        return +result, +Counter({key:-value for key,value in result.items()})
    expected_frontier = Counter()
    incidence = Counter(use.edge_id for use in src.coedges)
    if family == 'box':
        assert set(incidence.values()) == {2} and len(incidence) == 12
        assert any(f.orientation == -1 for f in src.faces)
    else:
        assert all(face.orientation == 1 for face in src.faces)
        for use in src.coedges:
            if incidence[use.edge_id] != 1:
                continue
            edge = next(edge for edge in final['edges'] if edge['id'] == use.edge_id)
            first,count = edge['nodes']
            chain = [node['key'] for node in final['nodes'][first:first+count]]
            if use.sense < 0:
                chain.reverse()
            expected_frontier.update(zip(chain,chain[1:]))
    assert signed(frontier) == signed(expected_frontier), 'wrong complete physical source frontier'
    return total


def read_tsv(path):
    with path.open(newline='') as stream:
        return list(csv.DictReader(stream, delimiter='\t'))


def exact_metadata(text):
    assert text.startswith('r:')
    numerator,denominator = text[2:].split('/')
    return F(int(numerator,16),int(denominator,16))


def run_caller(options, root, src, source, positional):
    output = root/'native'
    qualities = (positional, positional*2, positional)
    args = [str(options.executable.resolve()), '--source', str(source),
            '--definition',str(src.definitions[0].id),'--output',str(output),
            '--node-source-mm','1e-8','--threads',str(options.threads), '--face-owners',str(min(2,options.threads))]
    for quality in qualities:
        args.extend(('--positional-mm',str(quality)))
    if options.module:
        args.extend(('--backend','p100','--cuda-module',str(options.module.resolve()),
                     '--device',str(options.device)))
    proc = subprocess.run(args,text=True,capture_output=True,timeout=120)
    (root/'stdout.txt').write_text(proc.stdout); (root/'stderr.txt').write_text(proc.stderr)
    if options.family == 'coincident':
        assert proc.returncode == 1 and 'contact' in proc.stderr.lower(), proc.stderr
        assert not output.exists() and not list(root.glob('native.tmp-*'))
        print('PASS\tcomponent CLI contact refusal without publication')
        return
    assert proc.returncode == 0, (proc.returncode,proc.stdout,proc.stderr)
    assert (output/'source.bgsource').read_bytes() == source.read_bytes()
    index = read_tsv(output/'faces.tsv')
    queries = read_tsv(output/'queries.tsv')
    assert len(queries) == len(qualities)
    geometry = []
    for q,quality in enumerate(qualities):
        row = queries[q]
        assert int(row['query']) == q and float(row['requested_positional_mm']) == quality
        local = exact_metadata(row['local_positional_exact'])
        node = exact_metadata(row['local_node_exact'])
        # These literal fixtures have no placements; exact assembly transport is identity.
        assert local == F(quality) and node == F(1e-8)
        selected = [r for r in index if int(r['query']) == q]
        assert len(selected) == len(src.faces)
        paths = []
        for face in selected:
            assert int(face['definition']) == src.definitions[0].id
            curve,mesh = read_curves(output/face['boundary']),read_face(output/face['mesh'])
            # Forest epoch and per-face generation are separate counters. BGACRV
            # carries the actual chains, not an epoch field. incident_chains below
            # checks those retained chains against the final boundary.
            assert int(face['boundary_epoch']) >= 0
            assert int(face['face_boundary_generation']) == mesh['key'][2]
            assert mesh['upper'] == local and mesh['node_source'] == node
            paths.append((face['face'],face['boundary'],face['mesh']))
        final = read_tsv(output/f'query-{q}'/'final-boundary.tsv')
        assert len(final) == 1
        final_path = output/f'query-{q}'/final[0]['snapshot']
        total = check_output(src,options.family,output,local,paths,final_path)
        assert int(row['faces']) == len(src.faces) and int(row['facets']) == total
        assert int(row['contact_epochs']) == 1 and int(row['contact_facets']) == total
        assert int(row['contact_forbidden_pairs']) == 0
        if options.family == 'needs_boundary':
            assert int(row['boundary_rounds']) > 0 and int(row['requested_intervals']) > 0
            assert int(row['retained_faces']) > 0
        geometry.append([face_semantics(read_face(output/r['mesh'])) for r in selected])
    assert geometry[0] == geometry[2], 'same-quality complete component history differs'
    scopes = {r['scope'] for r in read_tsv(output/'timings.tsv')}
    for q in range(len(qualities)):
        assert {f'query{q}.assembly_transport',f'query{q}.complete_component',f'query{q}.output'} <= scopes
    if options.family != 'box' or options.threads != 1 or options.module:
        print(f'component CLI original-source PASS\t{options.family}\t{len(qualities)} queries')
        return
    # CLI-specific transaction ports reuse the already qualified authored source.
    refusals = [('definition', ['--definition','18446744073709551615'],
                 ('source.decode','0','unknown source ID 18446744073709551615')),
                ('memory', ['--memory-mib','1'],
                 ('service.construct','2','run memory reservation exhausted')),
                ('work', ['--construction-work','1'],
                 ('query.complete_component','exact','exact work budget exhausted'))]
    malformed = root/'truncated.bgsource'; malformed.write_bytes(source.read_bytes()[:31])
    refusals.append(('malformed',['--source',str(malformed)],
                     ('source.decode','0','truncated source binary')))
    for name,override,expected in refusals:
        target = root/f'refused-{name}'
        failed = subprocess.run(args + ['--output',str(target)] + override,
                                text=True,capture_output=True,timeout=120)
        assert failed.returncode == 1 and failed.stderr.startswith('error\t'), (name,failed.stderr)
        fields = failed.stderr.strip().split('\t')
        assert tuple(fields[1:]) == expected, (name,failed.stderr)
        assert not target.exists() and not list(root.glob(f'{target.name}.tmp-*')), name
        (root/f'{name}-stderr.txt').write_text(failed.stderr)
    sentinel = root/'existing'; sentinel.mkdir(); (sentinel/'sentinel').write_bytes(b'unchanged\x00')
    failed = subprocess.run(args + ['--output',str(sentinel)],text=True,capture_output=True,timeout=120)
    assert failed.returncode == 1 and 'already exists' in failed.stderr, failed.stderr
    assert list(sentinel.iterdir()) == [sentinel/'sentinel']
    assert (sentinel/'sentinel').read_bytes() == b'unchanged\x00'
    assert not list(root.glob('existing.tmp-*'))
    print(f'component CLI original-source PASS\t{options.family}\t{len(qualities)} queries')


def run(options, root):
    src = (box_source() if options.family == 'box' else cylinder_source() if options.family == 'cylinder'
           else boundary_source() if options.family == 'needs_boundary' else coincident_source())
    source = root/'original.bgsource'
    write_source(src,source)
    positional = 16. if options.family == 'needs_boundary' else .01
    if options.caller:
        return run_caller(options,root,src,source,positional)
    output = root/'native'; output.mkdir()
    args = [str(options.executable.resolve()),str(source),str(output),str(options.threads)]
    if options.module:
        args.extend((str(options.module.resolve()),str(options.device)))
    args.append(str(positional))
    proc = subprocess.run(args,text=True,capture_output=True,timeout=120)
    (root/'stdout.txt').write_text(proc.stdout); (root/'stderr.txt').write_text(proc.stderr)
    if options.family == 'coincident':
        assert proc.returncode != 0 and 'contact' in proc.stderr.lower(), proc.stderr
        assert not list(output.iterdir()), 'refused component published a partial result'
        print('PASS\tcoincident original physical identities refused without output')
        return
    assert proc.returncode == 0, (proc.returncode,proc.stdout,proc.stderr)
    fields = proc.stdout.strip().split('\t')
    assert fields[0] == 'PASS' and len(fields)%2 == 1
    stats = {fields[i]:int(fields[i+1]) for i in range(1,len(fields),2)}
    total = check_output(src,options.family,output,F(positional))
    assert stats['faces'] == len(src.faces) and stats['facets'] == total
    assert stats['contact_epochs'] == 1 and stats['contact_facets'] == total and stats['forbidden_pairs'] == 0
    if options.family == 'needs_boundary':
        assert stats['boundary_rounds'] > 0 and stats['requested_intervals'] > 0
        assert stats['retained_faces'] > 0 and stats['assignments'] > len(src.faces)
    print(proc.stdout.strip())
    print(f'original-source component PASS\t{options.family}\t{total} facets')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--executable',type=Path,required=True)
    parser.add_argument('--caller',action='store_true',help='invoke complete component CLI and transaction ports')
    parser.add_argument('--family',choices=('box','cylinder','needs_boundary','coincident'),required=True)
    parser.add_argument('--threads',type=int,choices=(1,4),default=1)
    parser.add_argument('--module',type=Path)
    parser.add_argument('--device',type=int,choices=(0,1),default=0)
    parser.add_argument('--workspace',type=Path)
    options = parser.parse_args()
    if options.workspace:
        options.workspace.mkdir(parents=True,exist_ok=False)
        run(options,options.workspace.resolve())
    else:
        with tempfile.TemporaryDirectory(prefix='adaptive-component-') as temporary:
            run(options,Path(temporary))


if __name__ == '__main__':
    main()
