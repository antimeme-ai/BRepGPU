#!/usr/bin/env python3
"""Optional isolated reference Boolean on the identical recorded operand frames."""
import argparse
import csv
import hashlib
import importlib.metadata
import math
from pathlib import Path
import sys
import time


def tsv(path,header,rows):
    with path.open('w',newline='') as stream:
        writer=csv.writer(stream,delimiter='\t',lineterminator='\n');writer.writerow(header);writer.writerows(rows)


def require(value,message):
    if not value:raise AssertionError(message)


def execute(args):
    require(sys.platform=='linux','reference experiment executes on fleet')
    case=args.case_directory
    recipe=dict(csv.reader((case/'recipe.tsv').open(),delimiter='\t'))
    require((case/'quality.tsv').is_file(),'owned qualified result required')
    origin=tuple(float.fromhex(recipe[f'origin_{axis}_hex']) for axis in 'xyz')
    scale=float.fromhex(recipe['scale_hex'])
    start=time.perf_counter_ns()
    import usd_brep as sm
    import_ms=(time.perf_counter_ns()-start)/1e6
    out=args.output_directory;out.mkdir(parents=True,exist_ok=False)
    start=time.perf_counter_ns()
    box=sm.create_box(tuple(o-d*scale for o,d in zip(origin,(.75,.75,.5))),1.5*scale,1.5*scale,scale)
    cutter=sm.create_cylinder((origin[0],origin[1],origin[2]-scale),.5*scale,2*scale)
    create_ms=(time.perf_counter_ns()-start)/1e6
    before=(box.volume(relative_accuracy=1e-8),cutter.volume(relative_accuracy=1e-8))
    start=time.perf_counter_ns()
    result=sm.boolean_difference(box.copy(),cutter.copy())
    difference_ms=(time.perf_counter_ns()-start)/1e6
    start=time.perf_counter_ns()
    volume=result.volume(relative_accuracy=1e-8)
    manifold=result.is_manifold_solid()
    bounds=tuple(tuple(float(x) for x in p) for p in result.bounding_box(tight=True))
    after=(box.volume(relative_accuracy=1e-8),cutter.volume(relative_accuracy=1e-8))
    require(before==after,'reference copy inputs changed')
    expected=(9-math.pi)/4*scale**3
    from fractions import Fraction
    owned=float(Fraction(recipe['volume_rational']))+math.pi*float(Fraction(recipe['volume_pi']))
    require(math.isfinite(volume) and abs(volume-expected)<=1e-8*abs(expected),'reference volume disagrees with analytic family')
    require(abs(owned-expected)<=1e-14*abs(expected),'owned measure disagrees with analytic family')
    require(manifold,'reference result is not a manifold solid')
    expected_bounds=(tuple(o-d*scale for o,d in zip(origin,(.75,.75,.5))),
                     tuple(o+d*scale for o,d in zip(origin,(.75,.75,.5))))
    require(max(abs(a-b) for p,q in zip(bounds,expected_bounds) for a,b in zip(p,q))<=1e-8*scale,'reference result bounds disagree')
    measure_ms=(time.perf_counter_ns()-start)/1e6
    points=[]
    for row in csv.DictReader((case/'edge-points.tsv').open(),delimiter='\t'):
        point=tuple(float(Fraction(row[axis])) for axis in 'xyz')
        points.append(('owned_exact_edge',row['edge_id']+':'+row['t'],point))
    # Also compare actual admitted carrier samples from all result faces.
    face_counts={}
    for row in csv.DictReader((case/'samples.tsv').open(),delimiter='\t'):
        if row['native_status']=='-1':continue
        face=row['face'];count=face_counts.get(face,0)
        if count<8:
            points.append(('owned_native_sample',row['index'],tuple(float(row[axis]) for axis in 'xyz')))
            face_counts[face]=count+1
    start=time.perf_counter_ns();queries=[]
    for kind,key,point in points:
        closest,distance=sm.get_closest_point(result,point)
        closest=tuple(float(x) for x in closest);distance=float(distance)
        residual=math.dist(point,closest)
        require(all(math.isfinite(x) for x in (*closest,distance,residual)) and distance>=0,'invalid reference closest-point result')
        require(max(distance,residual)<=1e-7*scale,'owned boundary witness is away from reference boundary')
        queries.append((kind,key,*point,*closest,distance,residual))
    closest_ms=(time.perf_counter_ns()-start)/1e6
    tsv(out/'comparison.tsv',('field','value'),(('recipe',recipe['recipe']),('reference','usd-brep'),
        ('version',importlib.metadata.version('usd-brep')),('manifold',manifold),('reference_volume',volume),
        ('analytic_volume',expected),('owned_volume',owned),('reference_volume_absolute_error',abs(volume-expected)),
        ('boundary_witnesses',len(queries)),('max_boundary_distance',max(q[-2] for q in queries)),
        ('max_boundary_residual',max(q[-1] for q in queries)),('volume_relative_limit',1e-8),('boundary_world_limit',1e-7*scale),
        ('input_preservation','original reference inputs unchanged; copies consumed'),
        ('material_membership','not compared; no public material classifier established'),
        ('source_history','not compared; owned full history has independent qualification'),
        ('alpha','not external corpus qualification')))
    tsv(out/'bounds.tsv',('corner','x','y','z','expected_x','expected_y','expected_z'),((i,*p,*q) for i,(p,q) in enumerate(zip(bounds,expected_bounds))))
    tsv(out/'closest-points.tsv',('kind','key','x','y','z','reference_x','reference_y','reference_z','distance','residual'),queries)
    tsv(out/'costs.tsv',('phase','wall_ms'),(('reference_import',import_ms),('reference_construct_operands',create_ms),
        ('reference_copy_and_difference',difference_ms),('reference_measures_and_input_preservation',measure_ms),('reference_scalar_boundary_queries',closest_ms)))
    tsv(out/'sources.tsv',('path','sha256'),(('scripts/reference_boolean.py',hashlib.sha256(Path(__file__).read_bytes()).hexdigest()),
        *((name,hashlib.sha256((case/name).read_bytes()).hexdigest()) for name in ('recipe.tsv','quality.tsv','edge-points.tsv','samples.tsv'))))
    print(f'reference {recipe["recipe"]}: volume_error={abs(volume-expected):.3g} boundary_max={max(q[-1] for q in queries):.3g}',flush=True)


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--case-directory',type=Path,required=True)
    p.add_argument('--output-directory',type=Path,required=True)
    execute(p.parse_args())
