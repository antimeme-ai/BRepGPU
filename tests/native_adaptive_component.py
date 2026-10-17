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
    check_seam, curve_h, curve_local, whole_chord, check_line)
from native_adaptive_trimmed_caller import (
    read_face, qualify_face, original_boundary, cylinder_source, cylinder_trig,
    observed_features, face_semantics)
from brepgpu.imported_source import Vertex, Edge, Coedge, Loop, Face, Curve, Carrier, ZERO_FRAME, write_source, load_source


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
    return expression_box((1,curve_terms(curve,t)))


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


def check_output(src, family, output, positional, lines=None, final_path=None,
                 definition=None, node_cap=F(1e-8), boundary_check=None, facet_proof=None):
    if lines is None:
        lines = [line.split('\t') for line in (output/'faces.tsv').read_text().splitlines()]
    definition = src.definitions[0] if definition is None else definition
    assert [int(row[0]) for row in lines] == sorted(definition.face_ids)
    assert all(len(row) == 3 for row in lines)
    final = read_curves(final_path or output/'final.bgacrv')
    if boundary_check is not None:
        boundary_check(final)
    elif family == 'cylinder':
        check_seam(final,src,positional,F(1e-8))
    elif family == 'needs_boundary':
        check_closed_plane_boundary(final,src,positional,F(1e-8))
    else:
        check_curves(final,src,positional,node_cap,'plain',lambda use,uv:plane_value(src,use,uv),definition)
    frontier = Counter()
    total = 0
    checked_boundaries = {final_path}
    for identifier,curve_path,face_path in lines:
        face = next(face for face in src.faces if face.id == int(identifier))
        boundary,mesh = read_curves(output/curve_path),read_face(output/face_path)
        if boundary_check is not None and output/curve_path not in checked_boundaries:
            boundary_check(boundary)
            checked_boundaries.add(output/curve_path)
        kind = 'original' if facet_proof else 'cylinder' if family == 'cylinder' else 'plane'
        qualify_face(src,boundary,mesh,positional,node_cap,kind,face=face,facet_proof=facet_proof)
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
    face_ids = set(definition.face_ids)
    source_loops = {loop.id:loop for loop in src.loops if loop.face_id in face_ids}
    source_uses = {use.id:use for use in src.coedges if use.loop_id in source_loops}
    incidence = Counter(use.edge_id for use in source_uses.values())
    if family == 'box':
        assert set(incidence.values()) == {2} and len(incidence) == 12
        assert any(f.orientation == -1 for f in src.faces)
    else:
        if boundary_check is None:
            assert all(face.orientation == 1 for face in src.faces)
        directions = {}
        if boundary_check is not None:
            actual_uses = {row['ids'][0]:row for row in final['uses']}
            original_faces = {f.id:f for f in src.faces}
            for loop in source_loops.values():
                polygon = []
                for identifier in loop.coedge_ids:
                    first,count = actual_uses[identifier]['sites']
                    polygon.extend(s['uv'] for s in final['sites'][first:first+count-1])
                area = sum(a[0]*b[1]-a[1]*b[0] for a,b in zip(polygon,polygon[1:]+polygon[:1]))
                assert area != 0
                normalization = 1 if (area > 0) == loop.outer else -1
                for identifier in loop.coedge_ids:
                    directions[identifier] = normalization*original_faces[loop.face_id].orientation
        for use in source_uses.values():
            if boundary_check is None and incidence[use.edge_id] != 1:
                continue
            edge = next(edge for edge in final['edges'] if edge['id'] == use.edge_id)
            first,count = edge['nodes']
            chain = [node['key'] for node in final['nodes'][first:first+count]]
            if use.sense*directions.get(use.id,1) < 0:
                chain.reverse()
            expected_frontier.update(zip(chain,chain[1:]))
    assert signed(frontier) == signed(expected_frontier), 'wrong complete physical source frontier'
    return total


def original_terms(frame, dimension=3, constant=None, harmonics=()):
    return (tuple(F(frame[a]) if constant is None else constant[a] for a in range(dimension)),
            tuple(harmonics))


def curve_terms(curve, t):
    assert curve.dimension == 3 and curve.kind in (1,2,3), 'original curve oracle unsupported'
    frame = tuple(map(F,curve.frame))
    if curve.kind == 1:
        return original_terms(frame,constant=tuple(frame[a]+t*frame[3+a] for a in range(3)))
    return original_terms(frame,harmonics=(
        (t,0,tuple(F(curve.radii[0])*frame[3+a] for a in range(3))),
        (t,1,tuple(F(curve.radii[1])*frame[6+a] for a in range(3)))))


def carrier_terms(carrier, uv):
    frame = tuple(map(F,carrier.frame)); u,v = uv
    assert carrier.kind in (1,2), 'original carrier oracle unsupported'
    if carrier.kind == 1:
        return original_terms(frame,constant=tuple(frame[a]+u*frame[3+a]+v*frame[6+a] for a in range(3)))
    return original_terms(frame,constant=tuple(frame[a]+v*frame[9+a] for a in range(3)),harmonics=(
        (u,0,tuple(F(carrier.radii[0])*frame[3+a] for a in range(3))),
        (u,1,tuple(F(carrier.radii[0])*frame[6+a] for a in range(3)))))


def expression_box(*weighted):
    """Combine identical ORIGINAL arguments before enclosing their sin/cos."""
    constant = [F(0)]*3; terms = {}
    for sign,(base,harmonics) in weighted:
        for a in range(3):
            constant[a] += sign*base[a]
        for t,kind,coefficients in harmonics:
            row = terms.setdefault((t,kind),[F(0)]*3)
            for a in range(3):
                row[a] += sign*coefficients[a]
    lo,hi = list(constant),list(constant)
    for (t,kind),row in terms.items():
        if not any(row):
            continue
        if t == 0:
            interval = (F(1-kind),)*2
        else:
            sine,cosine = cylinder_trig(t)
            interval = cosine if kind == 0 else sine
        for a in range(3):
            ends = [row[a]*value for value in interval]
            lo[a] += min(ends); hi[a] += max(ends)
    return tuple(zip(lo,hi))


def exact_point(point):
    return tuple(point),()


def residual_upper(box):
    return sum(max(abs(a),abs(b))**2 for a,b in box)


def residual_lower(box):
    return sum((a if a>0 else b if b<0 else F(0))**2 for a,b in box)


def original_facet(carrier, uv, xyz, upper, depth=0):
    assert upper >= 0
    curvature = (F(0) if carrier.kind == 1 else
        abs(F(carrier.radii[0]))*sum(abs(F(x)) for x in carrier.frame[3:9])*
        (max(p[0] for p in uv)-min(p[0] for p in uv))**2/8)
    remainder = upper-curvature
    errors = [expression_box((1,carrier_terms(carrier,p)),(-1,exact_point(q)))
              for p,q in zip(uv,xyz)]
    if remainder >= 0 and max(map(residual_upper,errors)) <= remainder**2:
        return
    center = tuple(sum(p[a] for p in uv)/3 for a in range(2))
    actual = tuple(sum(p[a] for p in xyz)/3 for a in range(3))
    error = expression_box((1,carrier_terms(carrier,center)),(-1,exact_point(actual)))
    assert residual_lower(error) <= upper**2, 'original analytic facet counterexample'
    assert depth < 20, 'original analytic facet oracle unresolved at subdivision cap'
    p = [*uv,*(tuple((uv[i][a]+uv[j][a])/2 for a in range(2)) for i,j in ((0,1),(1,2),(2,0)))]
    q = [*xyz,*(tuple((xyz[i][a]+xyz[j][a])/2 for a in range(3)) for i,j in ((0,1),(1,2),(2,0)))]
    for indices in ((0,3,5),(3,1,4),(5,4,2),(3,4,5)):
        original_facet(carrier,[p[i] for i in indices],[q[i] for i in indices],upper,depth+1)


def original_chord(evaluate, curvature, ts, qa, qb, upper, depth=0):
    a,b = ts
    remaining = upper-curvature
    boxes = [expression_box((1,evaluate(t)),(-1,exact_point(q))) for t,q in ((a,qa),(b,qb))]
    if remaining >= 0 and max(map(residual_upper,boxes)) <= remaining**2:
        return
    middle = (a+b)/2; actual = tuple((x+y)/2 for x,y in zip(qa,qb))
    error = expression_box((1,evaluate(middle)),(-1,exact_point(actual)))
    assert residual_lower(error) <= upper**2, 'original analytic chord counterexample'
    assert depth < 20, 'original analytic chord oracle unresolved at subdivision cap'
    original_chord(evaluate,curvature/4,(a,middle),qa,actual,upper,depth+1)
    original_chord(evaluate,curvature/4,(middle,b),actual,qb,upper,depth+1)


def original_boundary_checker(src, definition, positional, node_cap):
    edges = {e.id:e for e in src.edges if e.id in definition.edge_ids}
    vertices = {v.id:tuple(map(F,v.xyz)) for v in src.vertices if v.id in definition.vertex_ids}
    curves = {c.id:c for c in src.curves}
    faces = {f.id:f for f in src.faces if f.id in definition.face_ids}
    loops = {l.id:l for l in src.loops if l.face_id in faces}
    uses = {u.id:u for u in src.coedges if u.loop_id in loops}
    carriers = {c.id:c for c in src.carriers}
    def check(boundary):
        assert boundary['definition'] == definition.id
        assert boundary['digest'] == bytes.fromhex(src.source_sha256)
        assert [e['id'] for e in boundary['edges']] == sorted(edges)
        assert {u['ids'][0] for u in boundary['uses']} == set(uses)
        assert not boundary['aliases'] and not any(e.degenerate for e in edges.values()), \
            'declared source quotient aliases are outside this analytic reader profile'
        key_values = {}; actual_edges = {e['id']:e for e in boundary['edges']}
        for node in boundary['nodes']:
            assert key_values.setdefault(node['key'],(node['revision'],node['xyz'])) == (node['revision'],node['xyz'])
        for identifier,edge in actual_edges.items():
            original = edges[identifier]; curve = curves[original.curve_id]
            assert edge['curve'] == original.curve_id and edge['vertices'] == original.vertex_ids
            assert edge['t'] == tuple(map(F,original.parameter_range))
            first,count = edge['nodes']; pfirst,pcount = edge['intervals']
            chain = boundary['nodes'][first:first+count]
            assert len(chain) == count >= 2 and pcount == count-1
            assert (chain[0]['t'],chain[-1]['t']) == edge['t']
            assert (chain[0]['xyz'],chain[-1]['xyz']) == tuple(vertices[v] for v in edge['vertices'])
            # Original V/Ce admission also applies to wire edges with no face use.
            for endpoint,t in enumerate(edge['t']):
                error = expression_box((1,curve_terms(curve,t)),
                                       (-1,exact_point(vertices[edge['vertices'][endpoint]])))
                assert residual_upper(error) <= node_cap**2
            direction = 1 if edge['t'][1]>edge['t'][0] else -1
            assert all(direction*(b['t']-a['t'])>0 for a,b in zip(chain,chain[1:]))
            # BoundaryForest roots are original spatial knots and original map-t
            # breakpoints. Exported paths locate MIDPOINT samples within a root;
            # initial-root endpoints legitimately have an empty path too.
            low,high = sorted(edge['t'])
            assert low < high, 'zero-range original edge oracle unsupported'
            cuts = {low,high}
            cuts.update(F(t) for t in curve.knots if low < F(t) < high)
            for use in uses.values():
                if use.edge_id == identifier and use.parameter_map_kind != 1:
                    cuts.update(F(t) for t,_ in use.parameter_map if low < F(t) < high)
            cuts = sorted(cuts,reverse=direction<0)
            assert len({node['key'] for node in chain[1:-1]}) == count-2
            for endpoint,node in ((0,chain[0]),(1,chain[-1])):
                assert node['key'] == (0,edge['vertices'][endpoint],0,0), \
                    'original endpoint key lost source VertexID authority'
                assert not node['path']
            for node in chain[1:-1]:
                generator,source_id,generation,ordinal = node['key']
                assert (generator,source_id,generation) == (1,identifier,0) and ordinal > 0, \
                    'interior key lost original EdgeID authority'
                if node['t'] in cuts:
                    assert not node['path']
                    continue
                for left,right in zip(cuts,cuts[1:]):
                    if min(left,right) < node['t'] < max(left,right):
                        break
                else:
                    raise AssertionError('original midpoint outside source roots')
                for child in node['path']:
                    middle = (left+right)/2
                    if child == 0:
                        right = middle
                    else:
                        left = middle
                assert node['t'] == (left+right)/2, 'source t disagrees with original midpoint path'
            for node in chain:
                error = expression_box((1,curve_terms(curve,node['t'])),(-1,exact_point(node['xyz'])))
                assert residual_upper(error) <= positional**2
            for offset,(interval,a,b) in enumerate(zip(boundary['physical'][pfirst:pfirst+pcount],chain,chain[1:])):
                assert interval['key'][0] == identifier and interval['t'] == (a['t'],b['t'])
                assert interval['nodes'] == (first+offset,first+offset+1)
                second = (F(0) if curve.kind == 1 else
                    sum(abs(F(curve.radii[0])*F(x)) for x in curve.frame[3:6])+
                    sum(abs(F(curve.radii[1])*F(x)) for x in curve.frame[6:9]))
                original_chord(lambda t:curve_terms(curve,t),second*(b['t']-a['t'])**2/8,
                               interval['t'],a['xyz'],b['xyz'],interval['parameter'])
                if interval['correspondence'] == 0:
                    assert interval['parameter'] <= positional
                else:
                    assert interval['correspondence'] == 1 and curve.kind == 1
                    assert interval['image'] is not None and interval['image'] <= positional
                    original_chord(lambda t:curve_terms(curve,t),F(0),interval['t'],
                                   a['xyz'],b['xyz'],interval['image'])
                    check_line(interval['line'],curve,interval['t'],
                               {a['t']:a['xyz'],b['t']:b['xyz']},direction,
                               lambda t:curve_terms(curve,t)[0])
        for row in boundary['uses']:
            use = uses[row['ids'][0]]; edge = actual_edges[use.edge_id]
            face = faces[loops[use.loop_id].face_id]; carrier = carriers[face.carrier_id]
            assert row['ids'] == (use.id,face.id,use.edge_id,face.carrier_id,use.loop_id)
            assert row['meta'][:5] == (use.sense,use.reconstructed,use.parameter_map_kind,*use.periodic_shift)
            first,count = row['sites']; pfirst,pcount = row['intervals']
            sites = boundary['sites'][first:first+count]
            ef,en = edge['nodes']; ei,ec = edge['intervals']
            order = list(range(ef,ef+en)); physical = boundary['physical'][ei:ei+ec]
            if use.sense < 0:
                order.reverse(); physical = list(reversed(physical))
            assert [site['node'] for site in sites] == order and count == en and pcount == ec
            curve = curves[edge['curve']]
            for site in sites:
                node = boundary['nodes'][site['node']]
                assert site['loop'] == use.loop_id and site['t'] == node['t']
                assert site['branches'][:2] == use.periodic_shift
                if use.parameter_map_kind == 1:
                    expected = site['t']
                else:
                    assert use.parameter_map_kind == 2, 'original parameter map oracle unsupported'
                    pairs = use.parameter_map
                    for (t0,s0),(t1,s1) in zip(pairs,pairs[1:]):
                        if min(F(t0),F(t1)) <= site['t'] <= max(F(t0),F(t1)):
                            expected = F(s0)+(site['t']-F(t0))*(F(s1)-F(s0))/(F(t1)-F(t0)); break
                    else:
                        raise AssertionError('source t outside original map')
                assert site['s'] == expected and site['guard'] <= node_cap
                source_point = curve_terms(curve,site['t']); surface_point = carrier_terms(carrier,site['uv'])
                assert residual_upper(expression_box((1,source_point),(-1,surface_point))) <= site['guard']**2
                for endpoint,t in enumerate(edge['t']):
                    if site['t'] == t:
                        vertex = exact_point(vertices[edge['vertices'][endpoint]])
                        assert residual_upper(expression_box((1,vertex),(-1,source_point))) <= site['guard']**2
                        assert residual_upper(expression_box((1,vertex),(-1,surface_point))) <= site['guard']**2
            for j,interval in enumerate(boundary['intervals'][pfirst:pfirst+pcount]):
                assert interval['sites'] == (first+j,first+j+1) and interval['key'] == physical[j]['key']
                assert interval['trace'] <= positional
                a,b = sites[j:j+2]; qa,qb = [boundary['nodes'][s['node']]['xyz'] for s in (a,b)]
                du = b['uv'][0]-a['uv'][0]
                curvature = (F(0) if carrier.kind == 1 else
                    abs(F(carrier.radii[0]))*sum(abs(F(x)) for x in carrier.frame[3:9])*du**2/8)
                def evaluate(t):
                    uv = tuple(a['uv'][axis]+t*(b['uv'][axis]-a['uv'][axis]) for axis in range(2))
                    return carrier_terms(carrier,uv)
                original_chord(evaluate,curvature,(F(0),F(1)),qa,qb,interval['trace'])
                assert interval['authority'] == 1 and use.reconstructed and interval['domain'] is None
    return check


def check_saved(options):
    output = options.saved_output.resolve()
    assert options.definition and options.source_file, 'saved reader requires original source file and definition'
    assert options.source_file.read_bytes() == (output/'source.bgsource').read_bytes(), 'retained original source bytes changed'
    src = load_source(options.source_file)
    definition = next(d for d in src.definitions if d.id == options.definition)
    rows = read_tsv(output/'queries.tsv'); index = read_tsv(output/'faces.tsv')
    for row in rows:
        q = int(row['query']); positional = exact_metadata(row['local_positional_exact'])
        node_cap = exact_metadata(row['local_node_exact'])
        selected = [r for r in index if int(r['query']) == q]
        for face in selected:
            assert int(face['definition']) == definition.id and int(face['boundary_epoch']) >= 0
            assert int(face['face_boundary_generation']) == read_face(output/face['mesh'])['key'][2]
        final = read_tsv(output/f'query-{q}'/'final-boundary.tsv')[0]
        count = check_output(src,'original',output,positional,
            [(r['face'],r['boundary'],r['mesh']) for r in selected],
            output/f'query-{q}'/final['snapshot'],definition,node_cap,
            original_boundary_checker(src,definition,positional,node_cap),original_facet)
        assert count == int(row['facets']) and len(selected) == int(row['faces'])
        assert int(row['contact_facets']) == count and int(row['contact_forbidden_pairs']) == 0
        print(f'original analytic component PASS\tfaces\t{len(selected)}\tfacets\t{count}')



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
    parser.add_argument('--executable',type=Path)
    parser.add_argument('--saved-output',type=Path,help='read existing complete output; never invoke native geometry')
    parser.add_argument('--source-file',type=Path,help='unchanged original source bytes for saved output')
    parser.add_argument('--definition',type=int)
    parser.add_argument('--caller',action='store_true',help='invoke complete component CLI and transaction ports')
    parser.add_argument('--family',choices=('box','cylinder','needs_boundary','coincident'))
    parser.add_argument('--threads',type=int,choices=(1,4),default=1)
    parser.add_argument('--module',type=Path)
    parser.add_argument('--device',type=int,choices=(0,1),default=0)
    parser.add_argument('--workspace',type=Path)
    options = parser.parse_args()
    if options.saved_output:
        check_saved(options)
        return
    if not options.executable or not options.family:
        parser.error('native mode requires --executable and --family')
    if options.workspace:
        options.workspace.mkdir(parents=True,exist_ok=False)
        run(options,options.workspace.resolve())
    else:
        with tempfile.TemporaryDirectory(prefix='adaptive-component-') as temporary:
            run(options,Path(temporary))


if __name__ == '__main__':
    main()
