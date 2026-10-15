#!/usr/bin/env python3
"""Run owned meshing on one explicit original-face diagnostic view."""
import argparse
import multiprocessing
import time
from dataclasses import replace
from pathlib import Path
import sys

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'python'))
from brepgpu.imported_source import load_source
from brepgpu.imported_meshing import mesh_imported,MeshQuality


def face_view(source,identifier):
    face=source.face(identifier);definition=source.definition(face.definition_id)
    loops=tuple(source.loop(i) for i in face.loop_ids)
    coedges=tuple(source.coedge(i) for loop in loops for i in loop.coedge_ids)
    edges=tuple(source.edge(i) for i in sorted({c.edge_id for c in coedges}))
    vertices=tuple(source.vertex(i) for i in sorted({v for e in edges for v in e.vertex_ids}))
    curves=tuple(source.curve(i) for i in sorted(({e.curve_id for e in edges if e.curve_id}
                                               |{c.curve_id for c in coedges})))
    return replace(source,root_product_id=definition.product_id,
        products=(source.product(definition.product_id),),
        definitions=(replace(definition,face_ids=(face.id,),edge_ids=tuple(e.id for e in edges),
                             vertex_ids=tuple(v.id for v in vertices)),),
        usages=(),faces=(face,),loops=loops,coedges=coedges,edges=edges,vertices=vertices,
        curves=curves,carriers=(source.carrier(face.carrier_id),))


_SCAN_SOURCE=None
_SCAN_QUALITY=None


def _scan_face(identifier):
    begin=time.perf_counter()
    try:
        mesh=mesh_imported(face_view(_SCAN_SOURCE,identifier),_SCAN_QUALITY)
        return identifier,None,len(mesh.vertices),len(mesh.triangles),time.perf_counter()-begin
    except Exception as exc:
        return identifier,f'{type(exc).__name__}: {exc}',0,0,time.perf_counter()-begin


def main():
    if sys.platform!='linux':raise SystemExit('CAD diagnostics run on the Linux fleet only')
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source',type=Path);parser.add_argument('face',type=int,nargs='?')
    parser.add_argument('--scan',action='store_true',help='diagnostic inventory only; emits no product mesh')
    parser.add_argument('--jobs',type=int,default=4)
    parser.add_argument('--tolerance',type=float,default=.005)
    args=parser.parse_args()
    if args.scan:
        if args.face is not None or not 1<=args.jobs<=8:parser.error('--scan requires no face and1..8 jobs')
        global _SCAN_SOURCE,_SCAN_QUALITY
        _SCAN_QUALITY=MeshQuality(tolerance_mm=args.tolerance)
        _SCAN_SOURCE=load_source(args.source);begin=time.perf_counter();failures=[];count=0
        print('DIAGNOSTIC_ONLY_ALL_FACE_SCAN',len(_SCAN_SOURCE.faces),'jobs',args.jobs,flush=True)
        with multiprocessing.get_context('fork').Pool(args.jobs) as pool:
            for identifier,error,vertices,triangles,seconds in pool.imap_unordered(_scan_face,(f.id for f in _SCAN_SOURCE.faces),chunksize=1):
                count+=1
                if error:
                    failures.append(identifier);print('FAIL',identifier,'seconds',seconds,error,flush=True)
                elif seconds>10:print('SLOW_PASS',identifier,vertices,triangles,'seconds',seconds,flush=True)
                if count%100==0:print('PROGRESS',count,'failures',len(failures),'elapsed',time.perf_counter()-begin,flush=True)
        print('DIAGNOSTIC_SUMMARY',count,'failures',len(failures),'ids',tuple(sorted(failures)),'seconds',time.perf_counter()-begin,flush=True)
        return bool(failures)
    if args.face is None:parser.error('provide a face ID or --scan')
    source=face_view(load_source(args.source),args.face)
    carrier=source.carriers[0]
    print('diagnostic_original_face',args.face,'of_source',source.source_sha256,flush=True)
    print('carrier',carrier.id,carrier.kind,carrier.degrees,carrier.uv_bounds,
          carrier.knot_u,carrier.mult_u,carrier.knot_v,carrier.mult_v,flush=True)
    mesh=mesh_imported(source,MeshQuality(tolerance_mm=args.tolerance))
    print('face_vertices',len(mesh.vertices),'face_triangles',len(mesh.triangles),
          'closures',len(mesh.faces[0].chart_closures),'metrics',mesh.error_metrics,
          'timings',mesh.timings,flush=True)


if __name__=='__main__':raise SystemExit(main())
