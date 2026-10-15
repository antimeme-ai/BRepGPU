#!/usr/bin/env python3
"""Read the declared ASCII USD BRep subset and consume owned carrier first jets."""
import argparse
from array import array
import csv
from fractions import Fraction as F
import hashlib
import math
from pathlib import Path
import sys
import time

from brepgpu import Engine
from brepgpu.interchange import load
from brepgpu.interchange_oracles import qualify

ROOT=Path(__file__).resolve().parents[1]
SOURCES=('examples/import_carriers.py','python/brepgpu/usda_subset.py',
         'python/brepgpu/interchange.py','python/brepgpu/interchange_oracles.py',
         'python/brepgpu/__init__.py','python/brepgpu/body.py')


def tsv(path,header,rows):
    with path.open('w',newline='') as stream:
        writer=csv.writer(stream,delimiter='\t',lineterminator='\n')
        writer.writerow(header);writer.writerows(rows)


def require(condition,message):
    if not condition:raise AssertionError(message)


def execute(args):
    require(sys.platform=='linux','product geometry executes on the fleet')
    require(2<=args.grid<=129 and (args.grid-1)&(args.grid-2)==0,'grid must be power-of-two intervals plus one in 2..129')
    pins=tuple((name,hashlib.sha256((ROOT/name).read_bytes()).hexdigest()) for name in SOURCES)
    started=time.perf_counter_ns();stages=[]
    previous=started
    def phase(name):
        nonlocal previous
        now=time.perf_counter_ns();stages.append((name,(now-previous)/1e6));previous=now
    conversion=load(args.input)
    require(conversion.patches,'sampling request has no supported converted carriers')
    phase('read_parse_convert')
    require(len(conversion.patches)*args.grid**2<=1_048_576,'complete query request exceeds 1048576-sample budget')
    slots=array('I',(slot for slot in range(len(conversion.patches)) for _ in range(args.grid**2)))
    uv=array('d',(value for _ in conversion.patches for i in range(args.grid) for j in range(args.grid)
                  for value in (i/(args.grid-1),j/(args.grid-1))))
    phase('prepare_queries')
    with Engine(backend=args.backend,device=args.device,threads=args.threads,
                library_path=args.library_path) as engine:
        core=Path(engine._lib._name).resolve()
        native_paths=(core,)+( (core.with_name('libbrepgpu_cuda.so'),) if args.backend=='cuda' else () )
        phase('context_setup')
        with engine.create_patch_set(conversion.patches) as patches:
            phase('patch_snapshot_upload')
            with engine.create_query_set(patches,slots,uv) as queries:
                phase('query_snapshot_upload')
                with engine.evaluate(queries,order=1) as result:
                    phase('evaluate')
                    local=result.download()
                    phase('download')
        phase('query_patch_result_cleanup')
        require(local.count==len(slots) and local.order==1 and local.info.terms==3,'native output extent/order differs')
        require(local.info.backend==int(args.backend=='cuda') and local.info.patch_count==len(conversion.patches),'native placement/patch count differs')
        require(local._slots==slots and local._uv.tobytes()==uv.tobytes(),'native source queries differ')
        local.require_ok()
        world=local.convert();world.require_ok()
        phase('validate_convert')
        require(all(math.isfinite(x) for x in world.data),'nonfinite consumed world first jets')
        checksum=math.fsum(world.data)
        require(math.isfinite(checksum),'nonfinite checksum')
        phase('consume')
    phase('context_cleanup')
    complete_ms=(time.perf_counter_ns()-started)/1e6
    qualification_start=time.perf_counter_ns()
    metrics=qualify(conversion,local,world)
    qualification_ms=(time.perf_counter_ns()-qualification_start)/1e6
    native_pins=tuple((path.name,hashlib.sha256(path.read_bytes()).hexdigest()) for path in native_paths)
    out=args.output_directory;out.mkdir(parents=True,exist_ok=False)
    (out/'source.usda').write_bytes(conversion.source_bytes)
    tsv(out/'sources.tsv',('path','sha256'),pins)
    tsv(out/'native-libraries.tsv',('file','sha256'),native_pins)
    tsv(out/'costs.tsv',('phase','wall_ms'),(*stages,('complete_file_to_consumed_cleanup',complete_ms),('independent_qualification',qualification_ms)))
    tsv(out/'metrics.tsv',('field','value'),sorted(metrics.items()))
    tsv(out/'recipe.tsv',('field','value'),(('source_sha256',conversion.sha256),('backend',args.backend),
         ('observed_backend',local.info.backend),('device',args.device),('threads',args.threads),
         ('grid',args.grid),('faces',len(conversion.faces)),('patches',len(conversion.patches)),('queries',local.count),
         ('native_components',len(local.data)),('checksum',checksum),('trims','uninterpreted'),
         ('coordinates','source local; source unit/up-axis metadata retained; no unit conversion'),
         ('jets_layout','little-endian f64; component-major Sxyz,Suxyz,Svxyz'),
         ('qualification_scope','exact source Cox-de Boor and actual-control Bernstein; coefficient/frame rounding budgets')))
    tsv(out/'faces.tsv',('sha256','prim','face_index','ordinal','surface_type','trim_type','loop_count','status','reason','patch_slot','range_umin','range_umax','range_vmin','range_vmax','position_bound','du_bound','dv_bound'),
        ((*face.key,face.ordinal,face.surface_type,face.trim_type,face.loop_count,face.status,face.reason,
          '' if face.patch_slot is None else face.patch_slot,*face.face_range,
          face.position_bound,face.du_bound,face.dv_bound) for face in conversion.faces))
    tsv(out/'faceuses.tsv',('prim','face_index','faceuse_index','orientation'),
        ((face.key[1],face.key[2],index,orientation) for face in conversion.faces for index,orientation in face.faceuses))
    tsv(out/'source-controls.tsv',('prim','face_index','control','x_hex','y_hex','z_hex','weight_hex'),
        ((face.key[1],face.key[2],k,*(v.hex() for v in point),weight.hex())
         for face in conversion.faces if face.nurbs for k,(point,weight) in enumerate(zip(face.nurbs.points,face.nurbs.weights,strict=True))))
    tsv(out/'source-knots.tsv',('prim','face_index','axis','index','value_hex'),
        ((face.key[1],face.key[2],axis,index,value.hex()) for face in conversion.faces if face.nurbs
         for axis,values in (('u',face.nurbs.knots_u),('v',face.nurbs.knots_v)) for index,value in enumerate(values)))
    tsv(out/'patches.tsv',('slot','degree_u','degree_v','umin_hex','umax_hex','vmin_hex','vmax_hex','origin_x_hex','origin_y_hex','origin_z_hex','scale_hex','body_id','entity_kind','entity_id','revision','chart_id'),
        ((slot,p.degree_u,p.degree_v,*(v.hex() for v in (*p.uv,*p.origin,p.scale)),p.lineage.body_id,p.lineage.entity_kind,p.lineage.entity_id,p.lineage.revision,p.lineage.chart_id) for slot,p in enumerate(conversion.patches)))
    tsv(out/'controls.tsv',('slot','control','X_hex','Y_hex','Z_hex','W_hex'),
        ((slot,k,*(v.hex() for v in p.controls[4*k:4*k+4])) for slot,p in enumerate(conversion.patches) for k in range(len(p.controls)//4)))
    by_patch={face.patch_slot:face for face in conversion.faces if face.patch_slot is not None}
    def query_rows():
        for index,slot in enumerate(slots):
            face=by_patch[slot];p=conversion.patches[slot];s,t=uv[2*index:2*index+2]
            U=F(p.uv[0])+(F(p.uv[1])-F(p.uv[0]))*F(s)
            V=F(p.uv[2])+(F(p.uv[3])-F(p.uv[2]))*F(t)
            yield (index,slot,face.key[1],face.key[2],s.hex(),t.hex(),str(U),str(V),float(U).hex(),float(V).hex())
    tsv(out/'queries.tsv',('query','slot','prim','face_index','s_hex','t_hex','exact_u','exact_v','reference_u_hex','reference_v_hex'),query_rows())
    require(sys.byteorder=='little','binary artifact layout requires little endian')
    (out/'local-jets.f64').write_bytes(local.data.tobytes())
    (out/'source-jets.f64').write_bytes(world.data.tobytes())
    tsv(out/'consumed.tsv',('query','slot','x','y','z','du_x','du_y','du_z','dv_x','dv_y','dv_z'),
        ((index,int(slots[index]),*(x for term in range(3) for x in world.vector(term,index))) for index in range(world.count)))
    require(pins==tuple((name,hashlib.sha256((ROOT/name).read_bytes()).hexdigest()) for name in SOURCES),'owned source changed during caller')
    print(f'faces={len(conversion.faces)} converted={len(conversion.patches)} queries={local.count} complete_ms={complete_ms:.6f} qualification_ms={qualification_ms:.6f}',flush=True)


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input',type=Path,required=True)
    parser.add_argument('--backend',choices=('cpu','cuda'),default='cpu')
    parser.add_argument('--device',type=int,default=0)
    parser.add_argument('--threads',type=int,default=1)
    parser.add_argument('--library-path',type=Path,default=Path('build/pair-release/libbrepgpu.so'))
    parser.add_argument('--grid',type=int,default=17)
    parser.add_argument('--output-directory',type=Path,required=True)
    execute(parser.parse_args())
