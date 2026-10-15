#!/usr/bin/env python3
"""Isolated optional reference sweep on exactly recorded world profile endpoints."""
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
    with path.open('w',newline='') as f:
        w=csv.writer(f,delimiter='\t',lineterminator='\n');w.writerow(header);w.writerows(rows)


def require(value,message):
    if not value:raise AssertionError(message)


def execute(args):
    require(sys.platform=='linux','reference executes on fleet')
    case=args.case_directory;recipe=dict(csv.reader((case/'recipe.tsv').open(),delimiter='\t'))
    require((case/'quality.tsv').is_file(),'qualified owned output required')
    origin=tuple(F(float.fromhex(recipe[f'origin_{axis}_hex'])) for axis in 'xyz')
    scale=F(float.fromhex(recipe['scale_hex']));height=F(recipe['height'])
    points=tuple((F(r['x']),F(r['y'])) for r in csv.DictReader((case/'profile-points.tsv').open(),delimiter='\t'))
    exact=tuple(tuple(o+scale*x for o,x in zip(origin,(*p,-height/2))) for p in points)
    native=tuple(tuple(float(x) for x in p) for p in exact)
    require(all(F(y)==x for p,q in zip(exact,native) for x,y in zip(p,q)),'world endpoint is not exactly binary64 representable')
    distance=float(scale*height);require(F(distance)==scale*height,'world sweep distance rounded')
    start=time.perf_counter_ns();import usd_brep as sm
    import_ms=(time.perf_counter_ns()-start)/1e6
    out=args.output_directory;out.mkdir(parents=True,exist_ok=False)
    start=time.perf_counter_ns()
    curves=tuple(sm.create_line_segment(a,b) for a,b in zip(native,native[1:]+native[:1]))
    def curve_records():
        rows=[]
        for curve in curves:
            lo,hi=(float(x) for x in curve.parameter_range())
            require(math.isfinite(lo) and math.isfinite(hi) and lo<hi,'invalid original curve domain')
            values=tuple(tuple(float(x) for x in curve.evaluate(lo+(hi-lo)*t)) for t in (0.,.5,1.))
            require(all(len(p)==3 and all(math.isfinite(x) for x in p) for p in values),'invalid original reference curve')
            rows.append((lo,hi,*values))
        return tuple(rows)
    before=curve_records();create_ms=(time.perf_counter_ns()-start)/1e6
    start=time.perf_counter_ns();body=sm.linear_sweep(list(curves),(0.,0.,1.),distance,cap_ends=True)
    sweep_ms=(time.perf_counter_ns()-start)/1e6
    start=time.perf_counter_ns()
    require(curve_records()==before,'pure reference sweep changed original source curves')
    volume=float(body.volume(relative_accuracy=1e-8))
    manifold=bool(body.is_manifold_solid());bounds=tuple(tuple(float(x) for x in p) for p in body.bounding_box(tight=True))
    require(len(bounds)==2 and all(len(p)==3 for p in bounds) and all(math.isfinite(x) for p in bounds for x in p),'invalid returned reference bounds')
    area=sum((a[0]*b[1]-a[1]*b[0] for a,b in zip(points,points[1:]+points[:1])),F(0))/2
    expected=area*height*scale**3;owned=F(recipe['world_volume'])
    require(owned==expected and area>0,'owned exact measure differs from original profile formula')
    require(manifold and math.isfinite(volume) and abs(volume-float(expected))<=1e-8*float(expected),'reference volume/manifold differs')
    boundtruth=(tuple(origin[k]+scale*(min(p[k] for p in points) if k<2 else -height/2) for k in range(3)),
                tuple(origin[k]+scale*(max(p[k] for p in points) if k<2 else height/2) for k in range(3)))
    require(max(abs(a-float(b)) for p,q in zip(bounds,boundtruth) for a,b in zip(p,q))<=1e-8*float(scale),'reference bounds differ')
    measure_ms=(time.perf_counter_ns()-start)/1e6
    witnesses=[]
    for row in csv.DictReader((case/'edge-points.tsv').open(),delimiter='\t'):
        witnesses.append(('owned_exact_edge',row['edge_id']+':'+row['t'],tuple(float(F(row[axis])) for axis in 'xyz')))
    byface={}
    for row in csv.DictReader((case/'samples.tsv').open(),delimiter='\t'):
        if row['native_status']=='-1' or row['trim']!='1':continue
        byface.setdefault(row['face'],[]).append(row)
    require(set(byface)=={str(i) for i in range(1,len(points)+3)},'native interior witnesses miss a face')
    for face,rows in sorted(byface.items()):
        count=min(8,len(rows))
        for i in range(count):
            row=rows[(2*i+1)*len(rows)//(2*count)]
            witnesses.append(('owned_native_face_interior',row['index'],tuple(float(row[axis]) for axis in 'xyz')))
    start=time.perf_counter_ns();queries=[]
    for kind,key,point in witnesses:
        closest,d=sm.get_closest_point(body,point);closest=tuple(float(x) for x in closest);d=float(d)
        require(len(closest)==3,'invalid reference closest point extent')
        residual=math.dist(point,closest)
        require(all(math.isfinite(x) for x in (*closest,d,residual)) and d>=0 and max(d,residual)<=1e-7*float(scale),'reference boundary differs')
        queries.append((kind,key,*point,*closest,d,residual))
    query_ms=(time.perf_counter_ns()-start)/1e6
    tsv(out/'comparison.tsv',('field','value'),(('recipe',recipe['recipe']),('reference','usd-brep'),('version',importlib.metadata.version('usd-brep')),
        ('manifold',manifold),('reference_volume',volume),('exact_analytic_volume',expected),('owned_exact_volume',owned),
        ('volume_absolute_error',abs(volume-float(expected))),('boundary_witnesses',len(queries)),
        ('max_boundary_distance',max(q[-2] for q in queries)),('max_boundary_residual',max(q[-1] for q in queries)),
        ('volume_relative_limit',1e-8),('boundary_world_limit',1e-7*float(scale)),('world_endpoint_rounding','exact binary64 endpoints and distance'),
        ('input_curves','original domains and3points each unchanged after pure sweep'),
        ('material_history','not compared; no public material/history reference adapter established'),('alpha','not external corpus qualification')))
    tsv(out/'world-endpoints.tsv',('vertex_id','x_exact','y_exact','z_exact','x_hex','y_hex','z_hex'),
        ((i+1,*p,*(x.hex() for x in q)) for i,(p,q) in enumerate(zip(exact,native))))
    tsv(out/'reference-curves.tsv',('edge_id','lo','hi','start_x','start_y','start_z','mid_x','mid_y','mid_z','end_x','end_y','end_z'),
        ((i+1,row[0],row[1],*(x for p in row[2:] for x in p)) for i,row in enumerate(before)))
    tsv(out/'bounds.tsv',('corner','x','y','z','expected_x','expected_y','expected_z'),((i,*p,*q) for i,(p,q) in enumerate(zip(bounds,boundtruth))))
    tsv(out/'closest-points.tsv',('kind','key','x','y','z','reference_x','reference_y','reference_z','distance','residual'),queries)
    tsv(out/'costs.tsv',('phase','wall_ms'),(('reference_import',import_ms),('reference_curves_and_before_probes',create_ms),
        ('reference_linear_sweep',sweep_ms),('reference_input_preservation_and_measure',measure_ms),('reference_boundary_queries',query_ms)))
    tsv(out/'sources.tsv',('path','sha256'),(('scripts/reference_extrusion.py',hashlib.sha256(Path(__file__).read_bytes()).hexdigest()),
        *((name,hashlib.sha256((case/name).read_bytes()).hexdigest()) for name in ('recipe.tsv','quality.tsv','profile-points.tsv','edge-points.tsv','samples.tsv'))))
    print(f'reference {recipe["recipe"]}: volume_error={abs(volume-float(expected)):.3g} boundary_max={max(q[-1] for q in queries):.3g} witnesses={len(queries)}',flush=True)


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--case-directory',type=Path,required=True)
    p.add_argument('--output-directory',type=Path,required=True);execute(p.parse_args())
