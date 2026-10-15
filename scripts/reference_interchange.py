#!/usr/bin/env python3
"""Isolated optional USD BRep reference acquisition and scalar query experiment."""
import argparse
import csv
import hashlib
import importlib.metadata
import math
from pathlib import Path
import sys
import time


def tsv(path,header,rows):
    with path.open('w',newline='') as f:
        writer=csv.writer(f,delimiter='\t',lineterminator='\n');writer.writerow(header);writer.writerows(rows)


def execute(args):
    if sys.platform!='linux':raise RuntimeError('reference execution belongs on fleet')
    start=time.perf_counter_ns()
    import usd_brep as sm
    import_ms=(time.perf_counter_ns()-start)/1e6
    out=args.output_directory;out.mkdir(parents=True,exist_ok=False)
    if args.action=='create-sheet':
        started=time.perf_counter_ns()
        surface=sm.create_surface_from_points([(i/3,j/3,.25 if i in (1,2) and j in (1,2) else 0.) for i in range(4) for j in range(4)],4,4)
        corners=((0.,0.,0.),(1.,0.,0.),(1.,1.,0.),(0.,1.,0.))
        curves=[sm.create_line_segment(a,b) for a,b in zip(corners,corners[1:]+corners[:1])]
        body=sm.trim_surface_with_3d_curves(surface,[4],curves)
        create_ms=(time.perf_counter_ns()-started)/1e6
        started=time.perf_counter_ns();sm.usd.export_brep(body.copy(),str(out/'sheet.usda'),shrink_face_domains=False)
        export_ms=(time.perf_counter_ns()-started)/1e6
        tsv(out/'costs.tsv',('phase','wall_ms'),(('reference_module_import',import_ms),('reference_construct_sheet',create_ms),('reference_copy_export_io',export_ms)))
    else:
        case=args.case_directory
        if case is None:raise ValueError('--case-directory required for query')
        recipe=dict(csv.reader((case/'recipe.tsv').open(),delimiter='\t'))
        data=(case/'source.usda').read_bytes()
        if hashlib.sha256(data).hexdigest()!=recipe['source_sha256']:raise AssertionError('actual reference source hash differs')
        started=time.perf_counter_ns();bodies=sm.usd.import_breps(str(case/'source.usda'));load_ms=(time.perf_counter_ns()-started)/1e6
        if len(bodies)!=1:raise AssertionError('reference correspondence currently requires one actual packed body')
        faces=tuple(bodies[0].faces())
        inventory=list(csv.DictReader((case/'faces.tsv').open(),delimiter='\t'))
        if len(faces)!=len(inventory) or len({row['prim'] for row in inventory})!=1:raise AssertionError('reference/source face inventory differs')
        # Explicitly establish ordinal/domain correspondence for this observed fixture.
        domains=[]
        for index,face in enumerate(faces):
            lower,upper=face.surface().uv_domain()
            if len(lower)!=2 or len(upper)!=2:raise AssertionError('unexpected reference domain layout')
            domains.append((float(lower[0]),float(upper[0]),float(lower[1]),float(upper[1])))
        query_rows=list(csv.DictReader((case/'queries.tsv').open(),delimiter='\t'))
        actual=[];started=time.perf_counter_ns()
        for row in query_rows:
            index=int(row['face_index']);u=float.fromhex(row['reference_u_hex']);v=float.fromhex(row['reference_v_hex'])
            jet=faces[index].surface().derivatives(u,v)
            values=tuple(float(x) for vector in jet for x in vector)
            if len(values)!=9 or not all(math.isfinite(x) for x in values):raise AssertionError('invalid reference first jet')
            actual.append((row['query'],row['prim'],index,u.hex(),v.hex(),*values))
        scalar_ms=(time.perf_counter_ns()-started)/1e6
        tsv(out/'reference-jets.tsv',('query','prim','face_index','u_hex','v_hex','x','y','z','du_x','du_y','du_z','dv_x','dv_y','dv_z'),actual)
        tsv(out/'domains.tsv',('face_index','umin','umax','vmin','vmax'),((i,*domain) for i,domain in enumerate(domains)))
        tsv(out/'costs.tsv',('phase','wall_ms'),(('reference_module_import',import_ms),('reference_import_source',load_ms),('reference_scalar_query_loop',scalar_ms)))
        tsv(out/'source.tsv',('field','value'),(('sha256',recipe['source_sha256']),('faces',len(faces)),('queries',len(actual)),('correspondence','observed original face ordinal; domain and first jets checked separately')))
    tsv(out/'environment.tsv',('field','value'),(('reference','usd-brep'),('version',importlib.metadata.version('usd-brep')),
        ('python',sys.version.split()[0]),('script_sha256',hashlib.sha256(Path(__file__).read_bytes()).hexdigest()),
        ('reference_dependency','isolated optional experiment; not imported by owned converter')))
    print('reference experiment:',args.action,'complete',flush=True)


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('action',choices=('create-sheet','query'))
    p.add_argument('--case-directory',type=Path)
    p.add_argument('--output-directory',type=Path,required=True)
    execute(p.parse_args())
