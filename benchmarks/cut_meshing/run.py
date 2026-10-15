#!/usr/bin/env python3
"""Four serial actual Boolean-result mesh plus indexed/history consumer cases."""
import argparse
from pathlib import Path
import platform
import statistics
import sys
import time

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'examples'))
from mesh_cut import (CASES,Frame,make_body,difference,tessellate,read_mesh,read_history,
                      check_binding,qualify_mesh,same_output,export_cut,source_pins,tsv,ratio,require)


def main(args):
    require(sys.platform=='linux','product benchmarks run on the fleet')
    output=args.output_directory
    output.mkdir(parents=True,exist_ok=False)
    pins=source_pins()
    tsv(output/'sources.tsv',('path','sha256'),pins)
    tsv(output/'environment.tsv',('key','value'),
        (('placement','exact CPU, one process, serial cases'),('python',platform.python_version()),
         ('system',platform.system()),('release',platform.release()),('architecture',platform.machine()),
         ('cpu',next((s.split(':',1)[1].strip() for s in Path('/proc/cpuinfo').read_text().splitlines()
                      if s.startswith('model name')),'unknown')),
         ('first','after imports; original operands and CutResult prepared once per case'),
         ('warmup',1),('repeats',5),('qualification','first output only, outside complete wall'),
         ('repeat_correspondence','full frozen mesh and actual consumer outputs, outside wall'),
         ('export','first independently qualified output after repeated measurements'),
         ('native_calls',0),('host_device_transfer_bytes',0)))
    raw,single,counts,inputs=[],[],[],[]
    for name,origin,scale,tolerance in CASES:
        start=time.perf_counter_ns()
        left=make_body('box',Frame(origin,scale));right=make_body('cylinder',Frame(origin,2*scale))
        prepared=time.perf_counter_ns()
        cut=difference(left,right)
        constructed=time.perf_counter_ns()
        require(cut.left is left and cut.right is right,'cut replaced original operands')
        inputs.append((name,left.body_id,left.revision,right.body_id,right.revision,
                       cut.solid.body_id,cut.solid.revision,*(x.hex() for x in origin),
                       scale.hex(),(2*scale).hex(),ratio(tolerance),256))
        first=first_reader=first_history=quality=None
        for phase,sample in (('first',0),('warmup',0),*(('repeat',i) for i in range(5))):
            start_call=time.perf_counter_ns()
            mesh=tessellate(cut,tolerance=tolerance,max_quarter_segments=256)
            built=time.perf_counter_ns()
            reader=read_mesh(mesh)
            indexed=time.perf_counter_ns()
            history_read=read_history(cut)
            consumed=time.perf_counter_ns()
            check_start=time.perf_counter_ns()
            check_binding(mesh,cut,tolerance)
            if first is None:
                first,first_reader,first_history=mesh,reader,history_read
                quality=qualify_mesh(mesh)
                quality_ms=(time.perf_counter_ns()-check_start)/1e6
                correspondence_ms=0.
            else:
                same_output(mesh,first)
                require(reader.metrics==first_reader.metrics and history_read==first_history,
                        'actual mesh/history consumer output changed')
                require(all(getattr(reader,key).tobytes()==getattr(first_reader,key).tobytes()
                            for key in ('xyz','indices','face_ids','normals')),
                        'actual indexed buffers changed')
                correspondence_ms=(time.perf_counter_ns()-check_start)/1e6
                quality_ms=0.
            raw.append((name,phase,sample,(built-start_call)/1e6,(indexed-built)/1e6,
                        (consumed-indexed)/1e6,(consumed-start_call)/1e6,quality_ms,correspondence_ms,'PASS'))
        export_start=time.perf_counter_ns()
        export_cut(output/name,first,first_reader,first_history,quality)
        export_ms=(time.perf_counter_ns()-export_start)/1e6
        single.append((name,(prepared-start)/1e6,(constructed-prepared)/1e6,export_ms))
        counts.append((name,first.quarter_segments,len(first.vertices),len(first.triangles),
                       len(first.edge_polylines),len(first.boundary_uses),
                       ratio(first.curve_bound),ratio(first.rounding_bound),ratio(first.error_bound),
                       dict(first_reader.metrics)['indexed_buffer_bytes']))
        print(f'{name}: n={first.quarter_segments} vertices={len(first.vertices)} '
              f'triangles={len(first.triangles)}; first qualified, six full-output correspondences',flush=True)
    tsv(output/'input.tsv',('case','left_body_id','left_revision','right_body_id','right_revision',
                          'result_body_id','result_revision','origin_x_hex','origin_y_hex','origin_z_hex',
                          'box_scale_hex','cutter_scale_hex','tolerance','max_quarter_segments'),inputs)
    tsv(output/'single-phases.tsv',('case','operand_prepare_ms','cpu_boolean_ms','filesystem_export_ms'),single)
    tsv(output/'counts.tsv',('case','quarter_segments','vertices','triangles','edge_polylines','boundary_uses',
                           'curve_bound','rounding_bound','error_bound','indexed_buffer_bytes'),counts)
    tsv(output/'raw.tsv',('case','phase','sample','construct_ms','indexed_reader_ms','history_reader_ms',
                        'complete_ms','first_qualification_ms','repeat_correspondence_ms','result'),raw)
    summary=[]
    for name,*_ in CASES:
        for index,scope in ((3,'construct'),(4,'indexed_reader'),(5,'history_reader'),
                            (6,'complete'),(8,'repeat_correspondence')):
            values=[r[index] for r in raw if r[0]==name and r[1]=='repeat']
            summary.append((name,scope,statistics.median(values),min(values),max(values)))
    tsv(output/'summary.tsv',('case','scope','median_ms','min_ms','max_ms'),summary)
    require(source_pins()==pins,'measured source changed during serial campaign')
    (output/'SCOPE.md').write_text(
        '# Actual Boolean-result CPU mesh caller scopes\n\n'
        'Four cases prepare original box/cylinder operands and one actual CutResult each. '
        'Each constructs seven fresh meshes: first after imports/preparation, one excluded '
        'warmup and five measured repeats. Complete wall starts immediately before tessellate '
        'and ends after the actual full indexed reader and typed history lookups. The reader '
        'packs shared rounded XYZ, triangle indices, result face IDs and geometric facet normals '
        'while consuming every exact source/corner/edge/coedge record and error bound. History '
        'reading follows every forward/reverse origin to original operand entities and reads '
        'source maps, ranges and event fields.\n\n'
        'Only first outputs receive independent whole-domain/source/history qualification, '
        'outside complete wall. Six later full immutable meshes, retained input/history identity '
        'and indexed/history consumer outputs correspond directly with their first qualified '
        'outputs outside wall; those costs are reported separately. Exports retain that FIRST '
        'qualified output. Operands, Boolean construction and filesystem exports are separate '
        'single observations. Phase medians need not sum to the direct complete median.\n\n'
        'These vary resolution and frames, not Boolean complexity. Exact CPU construction; '
        'no native/CUDA mesher, multithread comparison or host/device/network transfer. Imports, '
        'final timing/source-file writes and interpreter shutdown lie outside measured calls. '
        'No GPU or whole-CAD speedup is inferred.\n')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output-directory',type=Path,required=True)
    main(parser.parse_args())
