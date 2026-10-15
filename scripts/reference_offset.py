#!/usr/bin/env python3
"""Optional pure same-world sharp-cylinder offset with unchanged criteria."""
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
    require(bool(rows),'reference original has no boundary edges')
    return volume,bounds,tuple(rows),bool(body.is_manifold_solid())


def source_artifacts(out,base,worldradius,worldheight,worlddistance,anchors,sourcebounds,epochs):
    tsv(out/'world-source.tsv',('field','exact','binary64_hex'),
        tuple((f'base_{i}',x,float(x).hex()) for i,x in enumerate(base))+tuple((name,x,float(x).hex()) for name,x in (('radius',worldradius),('height',worldheight),('distance',worlddistance)))+
        tuple((f'anchor_{i+1}_{j}',x,float(x).hex()) for i,p in enumerate(anchors) for j,x in enumerate(p))+
        tuple((f'bound_{i}_{j}',x,float(x).hex()) for i,p in enumerate(sourcebounds) for j,x in enumerate(p)))
    tsv(out/'original-body.tsv',('epoch','volume','manifold','lo_x','lo_y','lo_z','hi_x','hi_y','hi_z'),
        ((epoch,probes[0],probes[3],*probes[1][0],*probes[1][1]) for epoch,probes in epochs))
    tsv(out/'original-edge-probes.tsv',('epoch','edge_index','start_x','start_y','start_z','end_x','end_y','end_z','lo','hi','p0x','p0y','p0z','pmx','pmy','pmz','p1x','p1y','p1z'),
        ((epoch,i,*ends[0],*ends[1],lo,hi,*(v for p in samples for v in p)) for epoch,probes in epochs for i,(ends,lo,hi,samples) in enumerate(probes[2])))


def execute(args):
    require(sys.platform=='linux','reference geometry executes on fleet')
    case=args.case_directory;recipe=dict(csv.reader((case/'recipe.tsv').open(),delimiter='\t'))
    require((case/'quality.tsv').is_file(),'qualified actual owned output required')
    origin=tuple(F(float.fromhex(recipe[f'origin_{i}'])) for i in range(3));scale=F(float.fromhex(recipe['scale']))
    distance=F(recipe['distance']);require(recipe['join']=='intersection','reference requires sharp joins')
    require(distance!=0 and F(-1,8)<=distance<=F(1,8) and (distance*256).denominator==1,'unsupported signed offset')
    points={int(r['id']):tuple(F(r[axis]) for axis in 'xyz') for r in csv.DictReader((case/'source-vertices.tsv').open(),delimiter='\t')}
    require(points=={1:(F(1,4),F(0),F(-1,2)),2:(F(1,4),F(0),F(1,2))},'actual original cylinder anchors differ')
    R=F(1,4)+distance;H=F(1,2)+distance
    require((F(recipe['radius']),F(recipe['half_height']))==(R,H),'owned offset dimensions differ from request')
    base=(origin[0],origin[1],origin[2]-scale/2);worldradius=scale/4;worldheight=scale;worlddistance=distance*scale
    sourcebounds=tuple(tuple(o+sign*scale*d for o,d in zip(origin,(F(1,4),F(1,4),F(1,2)))) for sign in (-1,1))
    anchors=tuple(tuple(o+scale*x for o,x in zip(origin,points[id])) for id in (1,2))
    exact=(*base,worldradius,worldheight,worlddistance,*(v for p in (*sourcebounds,*anchors) for v in p))
    require(all(F(float(v))==v for v in exact),'world source input rounded at reference boundary')
    start=time.perf_counter_ns();import usd_brep as sm
    import_ms=(time.perf_counter_ns()-start)/1e6
    out=args.output_directory;out.mkdir(parents=True,exist_ok=False)
    start=time.perf_counter_ns();original=sm.create_cylinder(tuple(float(x) for x in base),radius=float(worldradius),height=float(worldheight))
    before=body_probes(original);create_ms=(time.perf_counter_ns()-start)/1e6
    sourceexpected=math.pi*float(scale**3/16)
    source_volume_error=abs(before[0]-sourceexpected);source_volume_limit=1e-8*sourceexpected
    source_bounds_error=max(abs(a-float(b)) for p,q in zip(before[1],sourcebounds) for a,b in zip(p,q));bounds_limit=1e-8*float(scale)
    preflight=(('source_manifold',before[3],before[3],True),
        ('source_volume',source_volume_error<=source_volume_limit,source_volume_error,source_volume_limit),
        ('source_bounds',source_bounds_error<=bounds_limit,source_bounds_error,bounds_limit))
    tsv(out/'source-preflight.tsv',('criterion','passed','observed','limit'),preflight)
    source_artifacts(out,base,worldradius,worldheight,worlddistance,anchors,sourcebounds,(('before',before),))
    pins=(('scripts/reference_offset.py',hashlib.sha256(Path(__file__).read_bytes()).hexdigest()),
        *((name,hashlib.sha256((case/name).read_bytes()).hexdigest()) for name in ('recipe.tsv','quality.tsv','source-vertices.tsv','edge-points.tsv','samples.tsv')))
    tsv(out/'sources.tsv',('path','sha256'),pins)
    if not all(row[1] for row in preflight):
        tsv(out/'comparison.tsv',('field','value'),(('recipe',recipe['recipe']),('reference','usd-brep'),('version',importlib.metadata.version('usd-brep')),
            ('overall_pass',False),('comparison_stage','source_factory'),('offset_attempted',False),
            ('source_manifold',before[3]),('source_reference_volume',before[0]),('source_analytic_volume_float',sourceexpected),
            ('source_volume_absolute_error',source_volume_error),('source_volume_relative_error',source_volume_error/sourceexpected),
            ('source_volume_limit',source_volume_limit),('source_bounds_absolute_error',source_bounds_error),('bounds_world_limit',bounds_limit),
            ('join','intersection'),('extended_offset',True),('self_intersection',True),('alpha','not external corpus qualification')))
        tsv(out/'outcomes.tsv',('criterion','passed','observed','limit'),
            (*preflight,*((name,'NOT_RUN','unqualified_original_source',limit) for name,limit in (
                ('original_source_preservation',True),('manifold',True),('volume',1e-8*math.pi*float(2*H*R*R*scale**3)),
                ('bounds',bounds_limit),('boundary',1e-7*float(scale))))))
        tsv(out/'costs.tsv',('phase','wall_ms'),(('reference_import',import_ms),('reference_source_and_before_probes',create_ms)))
        print(f'reference {recipe["recipe"]}: source_factory FAIL; offset not attempted; source_volume_error={source_volume_error:.9g} source_manifold={before[3]}',flush=True)
        raise AssertionError('reference original source preflight failed: '+', '.join(name for name,passed,_,_ in preflight if not passed))
    start=time.perf_counter_ns();work=sm.offset_brep(original,float(worlddistance),extended_offset=True,self_intersection=True)
    offset_ms=(time.perf_counter_ns()-start)/1e6;require(work is not original,'pure reference offset reused original object')
    start=time.perf_counter_ns();after=body_probes(original);preserved=before==after
    volume=float(work.volume(relative_accuracy=1e-8));require(math.isfinite(volume),'nonfinite result volume')
    bounds=tuple(finite_point(p) for p in work.bounding_box(tight=True))
    require(len(bounds)==2 and all(a<=b for a,b in zip(*bounds)),'invalid result bounds')
    manifold=bool(work.is_manifold_solid());pi=2*H*R*R*scale**3
    require((F(recipe['world_volume_rational']),F(recipe['world_volume_pi_coefficient']))==(F(0),pi),'owned volume differs from original exact offset formula')
    expected=math.pi*float(pi);volume_error=abs(volume-expected);volume_limit=1e-8*expected
    boundtruth=tuple(tuple(o+sign*scale*d for o,d in zip(origin,(R,R,H))) for sign in (-1,1))
    bounds_error=max(abs(a-float(b)) for p,q in zip(bounds,boundtruth) for a,b in zip(p,q));bounds_limit=1e-8*float(scale)
    measure_ms=(time.perf_counter_ns()-start)/1e6
    witnesses=[]
    for row in csv.DictReader((case/'edge-points.tsv').open(),delimiter='\t'):
        witnesses.append(('owned_exact_edge',row['edge_id']+':'+row['t'],tuple(float(F(row[axis])) for axis in 'xyz')))
    bychart={}
    for row in csv.DictReader((case/'samples.tsv').open(),delimiter='\t'):
        if row['native_status']!='-1' and row['trim']=='1':bychart.setdefault(row['chart'],[]).append(row)
    require(set(bychart)==set(map(str,range(1,7))),'reference witnesses miss output charts')
    for chart,rows in sorted(bychart.items(),key=lambda x:int(x[0])):
        require(len(rows)>=8,'insufficient native interior witnesses')
        for i in range(8):
            row=rows[(2*i+1)*len(rows)//16]
            witnesses.append(('owned_native_chart_interior',row['index'],tuple(float(row[axis]) for axis in 'xyz')))
    require(len(witnesses)==63,'reference witness budget differs')
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
        ('overall_pass',overall),('comparison_stage','offset_result'),('offset_attempted',True),('source_preserved',preserved),('manifold',manifold),('reference_volume',volume),('analytic_volume_float',expected),
        ('owned_exact_pi',pi),('volume_absolute_error',volume_error),('volume_relative_error',volume_error/expected),
        ('bounds_absolute_error',bounds_error),('boundary_witnesses',len(queries)),('max_boundary_distance',max(q[-2] for q in queries)),
        ('max_boundary_residual',max(q[-1] for q in queries)),('reference_volume_requested_accuracy',1e-8),('volume_relative_limit',1e-8),
        ('bounds_world_limit',bounds_limit),('boundary_world_limit',boundary_limit),('join','intersection'),
        ('extended_offset',True),('self_intersection',True),('world_input_rounding','exact binary64 cylinder inputs/bounds/anchors/distance'),
        ('ownership','pure new result; original retained and probed before/after; no copy/mutation'),
        ('source_preservation_scope','original volume/manifold/bounds/all edge endpoint ports/ranges/3point probes before and after'),
        ('material_history','not compared; no foreign material/history adapter'),('alpha','not external corpus qualification')))
    source_artifacts(out,base,worldradius,worldheight,worlddistance,anchors,sourcebounds,(('before',before),('after',after)))
    tsv(out/'bounds.tsv',('corner','x','y','z','expected_x','expected_y','expected_z'),((i,*p,*q) for i,(p,q) in enumerate(zip(bounds,boundtruth))))
    tsv(out/'closest-points.tsv',('kind','key','x','y','z','reference_x','reference_y','reference_z','distance','residual'),queries)
    tsv(out/'costs.tsv',('phase','wall_ms'),(('reference_import',import_ms),('reference_source_and_before_probes',create_ms),('reference_pure_offset',offset_ms),
        ('reference_original_preservation_and_measure',measure_ms),('reference_boundary_queries',query_ms)))
    print(f'reference {recipe["recipe"]}: overall_pass={overall} volume_error={volume_error:.3g} boundary_max={boundary_error:.3g} witnesses={len(queries)}',flush=True)
    require(overall,'reference offset comparison failed: '+', '.join(name for name,passed,_,_ in outcomes if not passed))


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--case-directory',type=Path,required=True)
    p.add_argument('--output-directory',type=Path,required=True);execute(p.parse_args())
