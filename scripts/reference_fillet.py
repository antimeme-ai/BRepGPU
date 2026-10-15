#!/usr/bin/env python3
"""Optional same-world reference fillet with isolated mutation and honest outcomes."""
import argparse
import csv
from fractions import Fraction as F
import hashlib
import importlib.metadata
import math
from pathlib import Path
import sys
import time


def tsv(path,header,rows):
    with path.open('w',newline='') as stream:
        w=csv.writer(stream,delimiter='\t',lineterminator='\n');w.writerow(header);w.writerows(rows)


def require(ok,message):
    if not ok:raise AssertionError(message)


def finite_point(values):
    values=tuple(float(v) for v in values)
    require(len(values)==3 and all(math.isfinite(v) for v in values),'invalid reference point')
    return values


def body_probes(body):
    volume=float(body.volume(relative_accuracy=1e-8));require(math.isfinite(volume),'nonfinite source volume')
    bounds=tuple(finite_point(p) for p in body.bounding_box(tight=True))
    require(len(bounds)==2 and all(a<=b for a,b in zip(*bounds)),'invalid source bounds')
    rows=[]
    for edge in body.edges():
        ends=tuple(finite_point(v.point()) for v in edge.vertices())
        require(len(ends)==2,'invalid source edge endpoints')
        lo,hi=(float(x) for x in edge.parameter_range())
        require(math.isfinite(lo) and math.isfinite(hi) and lo<hi,'invalid source edge range')
        points=tuple(finite_point(edge.point_at(lo+(hi-lo)*t)) for t in (0.,.5,1.))
        rows.append((ends,lo,hi,points))
    require(len(rows)==12,'reference original is not the complete source box')
    return volume,bounds,tuple(rows),bool(body.is_manifold_solid())


def execute(args):
    require(sys.platform=='linux','reference geometry executes on fleet')
    case=args.case_directory;recipe=dict(csv.reader((case/'recipe.tsv').open(),delimiter='\t'))
    require((case/'quality.tsv').is_file(),'qualified actual owned output required')
    origin=tuple(F(float.fromhex(recipe[f'origin_{axis}_hex'])) for axis in 'xyz')
    scale=F(float.fromhex(recipe['scale_hex']));radius=F(recipe['radius']);edgeid=int(recipe['source_edge']);k=edgeid-9
    require(0<=k<4,'reference requires selected original vertical edge')
    rows=list(csv.DictReader((case/'source-vertices.tsv').open(),delimiter='\t'))
    points={int(r['vertex_id']):tuple(F(r[axis]) for axis in 'xyz') for r in rows}
    require(set(points)==set(range(1,9)),'complete original source vertices required')
    selected_exact=tuple(tuple(o+scale*x for o,x in zip(origin,points[id])) for id in (k+1,k+5))
    lower=tuple(o-scale*d for o,d in zip(origin,(F(3,4),F(3,4),F(1,2))))
    dimensions=(F(3,2)*scale,F(3,2)*scale,scale);worldradius=radius*scale
    world_vertices=tuple(tuple(o+scale*x for o,x in zip(origin,points[id])) for id in range(1,9))
    values=(*lower,*dimensions,worldradius,*(x for p in world_vertices for x in p))
    require(all(F(float(x))==x for x in values),'world source input rounded at reference boundary')
    selected_native=tuple(tuple(float(x) for x in p) for p in selected_exact)
    start=time.perf_counter_ns();import usd_brep as sm
    import_ms=(time.perf_counter_ns()-start)/1e6
    out=args.output_directory;out.mkdir(parents=True,exist_ok=False)
    start=time.perf_counter_ns();original=sm.create_box(tuple(float(x) for x in lower),*(float(x) for x in dimensions))
    before=body_probes(original);create_ms=(time.perf_counter_ns()-start)/1e6
    require(before[3],'reference original source is not manifold')
    start=time.perf_counter_ns();work=original.copy()
    def select_copy_edge():
        matches=[]
        for edge in work.edges():
            ends=tuple(finite_point(v.point()) for v in edge.vertices())
            if len(ends)==2 and frozenset(ends)==frozenset(selected_native):matches.append(edge)
        require(len(matches)==1,'source endpoints do not select exactly one copied edge')
        edge=matches[0];lo,hi=(float(x) for x in edge.parameter_range())
        require(math.isfinite(lo) and math.isfinite(hi) and lo<hi,'invalid copied selected range')
        return edge,(lo,hi,tuple(finite_point(edge.point_at(lo+(hi-lo)*t)) for t in (0.,.5,1.)))
    selected,selected_probe=select_copy_edge();copy_ms=(time.perf_counter_ns()-start)/1e6
    start=time.perf_counter_ns();returned=sm.fillet_edges(work,[selected],radius=float(worldradius),xsect_type=1)
    require(returned is work,'installed reference fillet did not return its mutated body')
    del selected # Borrowed copy handles must not be queried after mutation.
    fillet_ms=(time.perf_counter_ns()-start)/1e6
    start=time.perf_counter_ns();after=body_probes(original);preserved=before==after
    volume=float(work.volume(relative_accuracy=1e-8));require(math.isfinite(volume),'nonfinite result volume')
    bounds=tuple(finite_point(p) for p in work.bounding_box(tight=True))
    require(len(bounds)==2 and all(a<=b for a,b in zip(*bounds)),'invalid result bounds')
    manifold=bool(work.is_manifold_solid())
    rational=(F(9,4)-radius*radius)*scale**3;pi=radius*radius*scale**3/4
    require((F(recipe['world_volume_rational']),F(recipe['world_volume_pi_coefficient']))==(rational,pi),'owned volume differs from original exact fillet formula')
    expected=float(rational)+math.pi*float(pi);volume_error=abs(volume-expected);volume_limit=1e-8*expected
    boundtruth=(lower,tuple(o+scale*d for o,d in zip(origin,(F(3,4),F(3,4),F(1,2)))))
    bounds_error=max(abs(a-float(b)) for p,q in zip(bounds,boundtruth) for a,b in zip(p,q));bounds_limit=1e-8*float(scale)
    measure_ms=(time.perf_counter_ns()-start)/1e6
    witnesses=[]
    for row in csv.DictReader((case/'edge-points.tsv').open(),delimiter='\t'):
        witnesses.append(('owned_exact_edge',row['edge_id']+':'+row['t'],tuple(float(F(row[axis])) for axis in 'xyz')))
    byface={}
    for row in csv.DictReader((case/'samples.tsv').open(),delimiter='\t'):
        if row['native_status']!='-1' and row['trim']=='1':byface.setdefault(row['face'],[]).append(row)
    require(set(byface)==set(map(str,range(1,8))),'reference witnesses miss a complete output face')
    for face,rows in sorted(byface.items(),key=lambda x:int(x[0])):
        require(len(rows)>=8,'insufficient spread native interior witnesses')
        for i in range(8):
            row=rows[(2*i+1)*len(rows)//16]
            witnesses.append(('owned_native_face_interior',row['index'],tuple(float(row[axis]) for axis in 'xyz')))
    start=time.perf_counter_ns();queries=[]
    for kind,key,point in witnesses:
        closest,d=sm.get_closest_point(work,point);closest=finite_point(closest);d=float(d);residual=math.dist(point,closest)
        require(math.isfinite(d) and d>=0 and math.isfinite(residual),'invalid boundary closest-point result')
        queries.append((kind,key,*point,*closest,d,residual))
    query_ms=(time.perf_counter_ns()-start)/1e6;boundary_error=max(max(q[-2:]) for q in queries);boundary_limit=1e-7*float(scale)
    outcomes=(('original_source_preservation',preserved,preserved,True),('manifold',manifold,manifold,True),
        ('volume',volume_error<=volume_limit,volume_error,volume_limit),('bounds',bounds_error<=bounds_limit,bounds_error,bounds_limit),
        ('boundary',boundary_error<=boundary_limit,boundary_error,boundary_limit))
    overall=all(r[1] for r in outcomes)
    tsv(out/'outcomes.tsv',('criterion','passed','observed','limit'),outcomes)
    tsv(out/'comparison.tsv',('field','value'),(('recipe',recipe['recipe']),('reference','usd-brep'),('version',importlib.metadata.version('usd-brep')),
        ('overall_pass',overall),('source_preserved',preserved),('manifold',manifold),('reference_volume',volume),('analytic_volume_float',expected),
        ('owned_exact_rational',rational),('owned_exact_pi',pi),('volume_absolute_error',volume_error),('volume_relative_error',volume_error/expected),
        ('bounds_absolute_error',bounds_error),('boundary_witnesses',len(queries)),('max_boundary_distance',max(q[-2] for q in queries)),
        ('max_boundary_residual',max(q[-1] for q in queries)),('reference_volume_requested_accuracy',1e-8),('volume_relative_limit',1e-8),
        ('bounds_world_limit',bounds_limit),('boundary_world_limit',boundary_limit),('world_input_rounding','exact binary64 box/selected endpoints/radius'),
        ('ownership','deep copy original; select copied edge by exact endpoints; mutate copy; discard copied child handles'),
        ('source_preservation_scope','original volume/manifold/bounds/all12edge endpoints/ranges/3point probes before and after'),
        ('material_history','not compared; no foreign material/history adapter established'),('alpha','not external corpus qualification')))
    tsv(out/'world-source.tsv',('field','exact','binary64_hex'),
        tuple((f'lower_{i}',x,float(x).hex()) for i,x in enumerate(lower))+tuple((f'dimension_{i}',x,float(x).hex()) for i,x in enumerate(dimensions))+
        (('radius',worldradius,float(worldradius).hex()),)+tuple((f'selected_{i}_{j}',x,float(x).hex()) for i,p in enumerate(selected_exact) for j,x in enumerate(p))+
        tuple((f'vertex_{i+1}_{j}',x,float(x).hex()) for i,p in enumerate(world_vertices) for j,x in enumerate(p)))
    tsv(out/'original-body.tsv',('epoch','volume','manifold','lo_x','lo_y','lo_z','hi_x','hi_y','hi_z'),
        ((epoch,probes[0],probes[3],*probes[1][0],*probes[1][1]) for epoch,probes in (('before',before),('after',after))))
    tsv(out/'original-edge-probes.tsv',('epoch','edge_index','start_x','start_y','start_z','end_x','end_y','end_z','lo','hi','p0x','p0y','p0z','pmx','pmy','pmz','p1x','p1y','p1z'),
        ((epoch,i,*ends[0],*ends[1],lo,hi,*(v for p in samples for v in p)) for epoch,probes in (('before',before),('after',after)) for i,(ends,lo,hi,samples) in enumerate(probes[2])))
    tsv(out/'selected-copy-edge.tsv',('lo','hi','p0x','p0y','p0z','pmx','pmy','pmz','p1x','p1y','p1z'),((selected_probe[0],selected_probe[1],*(v for p in selected_probe[2] for v in p)),))
    tsv(out/'bounds.tsv',('corner','x','y','z','expected_x','expected_y','expected_z'),((i,*p,*q) for i,(p,q) in enumerate(zip(bounds,boundtruth))))
    tsv(out/'closest-points.tsv',('kind','key','x','y','z','reference_x','reference_y','reference_z','distance','residual'),queries)
    tsv(out/'costs.tsv',('phase','wall_ms'),(('reference_import',import_ms),('reference_source_and_before_probes',create_ms),('reference_copy_select',copy_ms),
        ('reference_mutating_fillet',fillet_ms),('reference_original_preservation_and_measure',measure_ms),('reference_boundary_queries',query_ms)))
    tsv(out/'sources.tsv',('path','sha256'),(('scripts/reference_fillet.py',hashlib.sha256(Path(__file__).read_bytes()).hexdigest()),
        *((name,hashlib.sha256((case/name).read_bytes()).hexdigest()) for name in ('recipe.tsv','quality.tsv','source-vertices.tsv','edge-points.tsv','samples.tsv'))))
    print(f'reference {recipe["recipe"]}: overall_pass={overall} volume_error={volume_error:.3g} boundary_max={boundary_error:.3g} witnesses={len(queries)}',flush=True)
    require(overall,'reference fillet comparison failed: '+', '.join(name for name,passed,_,_ in outcomes if not passed))


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--case-directory',type=Path,required=True)
    p.add_argument('--output-directory',type=Path,required=True);execute(p.parse_args())
