#!/usr/bin/env python3
"""Four serial exact CPU mesh construction plus indexed-reader cost cases."""
import argparse
from fractions import Fraction as F
from pathlib import Path
import platform
import statistics
import sys
import time

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'examples'))
from mesh_body import (Frame, make_body, tessellate, read_mesh, check_binding,
                       qualify_mesh, same_output, export_mesh, source_pins, tsv, ratio)

CASES = (('box','box',(0.,0.,0.),1.,F(1,256)),
         ('cylinder','cylinder',(0.,0.,0.),1.,F(1,1024)),
         ('plate','plate',(0.,0.,0.),1.,F(1,1024)),
         ('plate-fine-frame','plate',(3.,-2.,1.),4.,F(1,16384)))


def main(args):
    if sys.platform != 'linux': raise RuntimeError('product benchmarks run on the fleet')
    output = args.output_directory
    output.mkdir(parents=True,exist_ok=False)
    pins = source_pins()
    tsv(output/'sources.tsv',('path','sha256'),pins)
    tsv(output/'environment.tsv',('key','value'),
        (('placement','exact CPU, one process, serial cases'),('python',platform.python_version()),
         ('system',platform.system()),('release',platform.release()),('architecture',platform.machine()),
         ('cpu',next((s.split(':',1)[1].strip() for s in Path('/proc/cpuinfo').read_text().splitlines()
                      if s.startswith('model name')),'unknown')),
         ('first','after imports; original body prepared once per case'),('warmup',1),('repeats',5),
         ('qualification','first output only, outside complete wall'),
         ('repeat_correspondence','full frozen output compared to first qualified output, outside wall'),
         ('export','first independently qualified full output, after repeated measurements'),
         ('native_calls',0),('host_device_transfer_bytes',0)))
    raw, single, counts, inputs = [], [], [], []
    for name,shape,origin,scale,tolerance in CASES:
        start = time.perf_counter()
        body = make_body(shape,Frame(origin,scale))
        prepared = time.perf_counter()
        inputs.append((name,shape,body.body_id,body.revision,*(float(x).hex() for x in origin),
                       scale.hex(),ratio(tolerance),256))
        first = first_reader = quality = None
        for phase,sample in (('first',0),('warmup',0),*(('repeat',i) for i in range(5))):
            start_call = time.perf_counter()
            mesh = tessellate(body,tolerance=tolerance,max_quarter_segments=256)
            built = time.perf_counter()
            reader = read_mesh(mesh)
            consumed = time.perf_counter()
            check_start = time.perf_counter()
            check_binding(mesh,body,tolerance)
            if first is None:
                first,first_reader = mesh,reader
                quality = qualify_mesh(mesh)
                quality_ms = 1000*(time.perf_counter()-check_start)
                correspondence_ms = 0.
            else:
                same_output(mesh,first)
                if reader.metrics != first_reader.metrics or any(
                    getattr(reader,key).tobytes() != getattr(first_reader,key).tobytes()
                    for key in ('xyz','indices','face_ids','normals')):
                    raise AssertionError('actual indexed-reader output changed')
                correspondence_ms = 1000*(time.perf_counter()-check_start)
                quality_ms = 0.
            raw.append((name,phase,sample,1000*(built-start_call),1000*(consumed-built),
                        1000*(consumed-start_call),quality_ms,correspondence_ms,'PASS'))
        export_start = time.perf_counter()
        export_mesh(output/name,first,first_reader,quality)
        export_ms = 1000*(time.perf_counter()-export_start)
        single.append((name,1000*(prepared-start),export_ms))
        counts.append((name,first.quarter_segments,len(first.vertices),len(first.triangles),
                       len(first.edge_polylines),len(first.boundary_uses),
                       ratio(first.curve_bound),ratio(first.rounding_bound),ratio(first.error_bound),
                       dict(first_reader.metrics)['indexed_buffer_bytes']))
        print(f'{name}: n={first.quarter_segments} vertices={len(first.vertices)} '
              f'triangles={len(first.triangles)}; first qualified, six full-output correspondences',flush=True)
    tsv(output/'input.tsv',('case','shape','body_id','revision','origin_x_hex','origin_y_hex','origin_z_hex',
                          'scale_hex','tolerance','max_quarter_segments'),inputs)
    tsv(output/'single-phases.tsv',('case','body_prepare_ms','filesystem_export_ms'),single)
    tsv(output/'counts.tsv',('case','quarter_segments','vertices','triangles','edge_polylines','boundary_uses',
                           'curve_bound','rounding_bound','error_bound','indexed_buffer_bytes'),counts)
    tsv(output/'raw.tsv',('case','phase','sample','construct_ms','indexed_reader_ms','complete_ms',
                        'first_qualification_ms','repeat_correspondence_ms','result'),raw)
    summary = []
    for name,*_ in CASES:
        for index,scope in ((3,'construct'),(4,'indexed_reader'),(5,'complete'),(7,'repeat_correspondence')):
            values = [r[index] for r in raw if r[0]==name and r[1]=='repeat']
            summary.append((name,scope,statistics.median(values),min(values),max(values)))
    tsv(output/'summary.tsv',('case','scope','median_ms','min_ms','max_ms'),summary)
    if source_pins() != pins: raise AssertionError('measured source changed during serial campaign')
    (output/'SCOPE.md').write_text(
        '# Exact CPU mesh caller scopes\n\n'
        'Four cases each prepare one original snapshot, then construct seven fresh meshes: first '
        'after imports/preparation, one excluded warmup, five measured repeats. Complete wall '
        'directly wraps construction plus the actual full indexed consumer. Reader buffers pack '
        'shared rounded XYZ, triangle indices, face IDs and geometric facet normals; it also '
        'reads every exact corner/source/polyline/coedge occurrence and error bound.\n\n'
        'Only each first output receives independent whole-domain qualification, outside complete '
        'wall. All six later complete frozen outputs and indexed buffers are compared directly '
        'with that first qualified output outside wall; correspondence cost is named separately. '
        'Full graph/source/PLY/SVG exports retain that FIRST qualified output, after measurements. '
        'Body preparation and filesystem export are separate single observations. No native '
        'library calls, CPU24 comparison, CUDA operation or host/device/network transfer is timed. '
        'Imports, final interpreter shutdown and final timing/source-file writes are outside all '
        'measured calls. Separate medians need not sum to a complete median.\n')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output-directory',type=Path,required=True)
    main(parser.parse_args())
