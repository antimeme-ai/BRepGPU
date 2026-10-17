#!/usr/bin/env python3
"""Original STEP -> OCCT reference tessellation -> shared native CPU display.

Requested OCCT deflection is not an owned mesher accuracy certificate. Every
original definition/face/placement is required; failure never gets a substitute.
"""
import argparse
import csv
import hashlib
import math
import os
from pathlib import Path
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[1]
sys.path[:0] = [str(ROOT/'python'),str(ROOT/'examples')]
from brepgpu.reference_source import load_reference_source
from brepgpu.imported_assembly import assemble
from brepgpu.nvidia_reference import load_definition, write_assembly
from om10_scene import (PALETTE, authored_material, authored_layout, camera_view,
                        placed_corners, pack_views, visible_anchors, write_png, labeled_svg)
from render_om10 import callouts


def require(value, message):
    if not value:
        raise ValueError(message)


def digest(path):
    result = hashlib.sha256()
    with Path(path).open('rb') as stream:
        while block := stream.read(1 << 20):
            result.update(block)
    return result.hexdigest()


def tsv(path, header, rows):
    with path.open('w', newline='', encoding='utf-8') as stream:
        writer = csv.writer(stream, delimiter='\t', lineterminator='\n')
        writer.writerow(header)
        writer.writerows(rows)


def read_rows(path):
    with path.open(newline='', encoding='utf-8') as stream:
        return tuple(csv.DictReader(stream, delimiter='\t'))


def main(args):
    require(sys.platform == 'linux', 'native reference execution belongs on the fleet')
    require(math.isfinite(args.deflection) and args.deflection > 0, 'positive finite requested deflection required')
    require(1 <= args.display_threads <= 256, 'invalid display thread width')
    require(not args.output.exists(), 'output directory already exists')
    require(args.step.is_file(), 'original source STEP does not exist')
    args.output.mkdir(parents=True)
    started = time.perf_counter()
    stages = []
    source_hash = digest(args.step)
    pins = tuple((name, digest(path)) for name,path in (
        ('source_exporter',args.exporter),('occt_reference_mesher',args.producer),
        ('shared_cpu_display',args.display)))
    environment = dict(os.environ, CUDA_VISIBLE_DEVICES='', OMP_NUM_THREADS='1',
                       OPENBLAS_NUM_THREADS='1', MKL_NUM_THREADS='1')
    def run(command, log):
        with log.open('wb') as stream:
            return subprocess.run(tuple(map(str,command)), env=environment, stdout=stream,
                                  stderr=subprocess.STDOUT)
    def stage(name, start):
        stages.append((name,time.perf_counter()-start))
    start = time.perf_counter()
    exported = run((args.exporter,args.step,args.output/'source'),args.output/'source-export.log')
    require(exported.returncode == 0, 'original source exporter failed; see source-export.log')
    source = load_reference_source(args.output/'source/source.bgocct')
    assembly = assemble(source)
    require(source.source_sha256 == source_hash, 'original source digest differs')
    scope = (len(source.definitions),len(source.faces),len(assembly.occurrences))
    if not args.allow_fixture:
        require(scope == (107,9162,166), 'complete original watch inventory differs from107/9162/166')
    stage('fresh_original_step_export_and_manifest',start)
    source_seconds=stages[-1][1]
    expected = tuple((d.id,f.id) for d in source.definitions for f in sorted(d.faces,key=lambda f:f.id))
    tsv(args.output/'jobs.tsv',('definition','brep','face','storage','orientation'),
        ((d.id,str((args.output/'source'/d.filename).resolve()),f.id,
          f.serialized_storage_index,f.orientation)
         for d in source.definitions for f in sorted(d.faces,key=lambda f:f.id)))
    start = time.perf_counter()
    meshed = run((args.producer,'--jobs',args.output/'jobs.tsv','--deflection',repr(args.deflection),
                  '--output-directory',args.output/'meshes'),args.output/'meshing.log')
    stage('occt_reference_serial_meshing_process',start)
    meshing_seconds=stages[-1][1]
    require(meshed.returncode == 0, 'OCCT reference meshing failed; no complete image claim; see meshing.log')
    faces = read_rows(args.output/'meshes/faces.tsv')
    require(tuple((int(r['definition']),int(r['face'])) for r in faces) == expected,
            'OCCT reference omitted/duplicated an original face')
    mapping = {(d.id,f.id):f for d in source.definitions for f in d.faces}
    require(all(int(r['triangles']) > 0 and int(r['storage']) == mapping[int(r['definition']),int(r['face'])].serialized_storage_index
                and int(r['orientation']) == mapping[int(r['definition']),int(r['face'])].orientation for r in faces),
            'OCCT reference original face map/triangulation differs')
    definitions = read_rows(args.output/'meshes/definitions.tsv')
    require(tuple(int(r['definition']) for r in definitions) == tuple(d.id for d in source.definitions)
            and all(r['status'] == 'PASS' for r in definitions), 'incomplete original definition output')
    start = time.perf_counter()
    meshes = tuple(load_definition(args.output/f'meshes/definition-{d.id}.bgrefmesh',expected_id=d.id)
                   for d in source.definitions)
    for d,mesh in zip(source.definitions,meshes):
        require(set(mesh.face_ids) == set(d.face_ids), 'actual mesh face IDs differ from original source')
    bounds = tuple((m.id,tuple(min(p[a] for p in m.points) for a in range(3))+
                        tuple(max(p[a] for p in m.points) for a in range(3))) for m in meshes)
    layout,records,axes = authored_layout(assembly,bounds)
    products = {p.id:p for p in source.products}
    materials = tuple((rgb,roughness) for _,rgb,roughness in PALETTE)
    material_map = {d.id:authored_material(products[d.product_id].name) for d in source.definitions}
    write_assembly(args.output/'scene.bgrefmesh',meshes,assembly,layout.offsets,materials,material_map)
    views = tuple(camera_view(placed_corners(assembly,bounds,layout,float(exploded)),
        width=args.width,height=args.height,alpha=float(exploded),axes=axes,
        padding=(max(80,args.width//7),max(80,args.width//7),80,100)) for exploded in (False,True))
    request = pack_views(views)
    (args.output/'views.bgview').write_bytes(request)
    tsv(args.output/'bom.tsv',('occurrence_id','source_usage_path','product_id','solid_id',
        *(f'matrix_{i}' for i in range(12))),
        ((o.id,'/'.join(map(str,o.path)),o.product_id,o.definition_id,*o.transform)
         for o in assembly.occurrences))
    tsv(args.output/'explosion.tsv',('occurrence_id','world_offset_x_mm','world_offset_y_mm','world_offset_z_mm'),
        ((o.id,*offset) for o,offset in zip(assembly.occurrences,layout.offsets)))
    tsv(args.output/'labels.tsv',('label','original_occurrence','source_usage_path','definition','name'),
        ((i+1,o.id,'/'.join(map(str,o.path)),o.definition_id,o.name) for i,o in enumerate(assembly.occurrences)))
    tsv(args.output/'layout.tsv',('occurrence_id','tier','reason'),
        ((r.occurrence_id,r.tier,r.reason) for r in records))
    stage('reference_decode_original_assembly_and_authored_views',start)
    start = time.perf_counter()
    displayed = run((args.display,'--mesh',args.output/'scene.bgrefmesh','--views',args.output/'views.bgview',
                     '--threads',args.display_threads,'--output-directory',args.output/'native'),args.output/'display.log')
    stage('shared_native_cpu_display_process_both_views',start)
    display_seconds=stages[-1][1]
    require(displayed.returncode == 0, 'shared native CPU display failed; see display.log')
    for i,(name,view) in enumerate(zip(('assembled','exploded'),views)):
        start = time.perf_counter()
        prefix = args.output/'native'/f'frame-{i:06d}'
        png = args.output/f'{i:03d}-{name}.png'
        write_png(png,view.width,view.height,prefix.with_suffix('.rgba').read_bytes())
        anchors = visible_anchors(prefix.with_suffix('.instance'),view.width,view.height,
                                  valid_ids=tuple(o.id for o in assembly.occurrences))
        labels,rows = callouts(assembly,anchors,view.width,view.height)
        tsv(args.output/f'{i:03d}-{name}-callouts.tsv',('label','visible_anchor_occurrence_id',
            'source_group_occurrence_ids','anchor_x','anchor_y','label_x','label_y'),rows)
        svg = args.output/f'{i:03d}-{name}.svg'
        labeled_svg(svg,png,view,title='OM10 / '+name.upper()+' / OCCT REFERENCE',
            subtitle=f'OCCT CPU reference · source-linked QUERY copy · requested deflection {args.deflection:g} mm (not certified) · CPU{args.display_threads} display',
            counts=(('original definitions',scope[0]),('original faces',scope[1]),('placed parts',scope[2])),
            timings=(('Fresh STEP import',f'{source_seconds:.3f} s'),
                     ('OCCT mesh + UV preparation',f'{meshing_seconds:.3f} s'),
                     ('CPU display, both views',f'{display_seconds:.3f} s')),callouts=labels,
            attribution='Association openmovement · OM10 MAN VA01.3 · CC BY-SA 3.0 · OCCT reference mesh; BRepGPU display, palette and labels')
        old = 'Owned tessellation, illustrative palette, lighting, authored explosion and labels by BRepGPU.'
        data = svg.read_text()
        require(data.count(old) == 1, 'shared SVG attribution changed')
        svg.write_text(data.replace(old,'OCCT REFERENCE CPU tessellation; owned display, illustrative palette, lighting, explosion and labels.'))
        stage(name+'_png_svg_and_callouts',start)
    require(digest(args.step) == source_hash, 'original source changed during reference operation')
    require(tuple((name,digest(path)) for (name,_),path in zip(pins,(args.exporter,args.producer,args.display))) == pins,
            'reference executable changed during operation')
    tsv(args.output/'runtime-pins.tsv',('component','sha256'),pins)
    tsv(args.output/'metadata.tsv',('field','value'),(
        ('status','COMPLETE'),('mesher','OCCT REFERENCE CPU BRepMesh_IncrementalMesh'),
        ('source_sha256',source_hash),('source_definitions',scope[0]),('source_faces',scope[1]),('occurrences',scope[2]),
        ('requested_deflection_mm',args.deflection),('tolerance_mm',args.deflection),('angular_deflection_radians',.5),
        ('relative_deflection',False),('parallel_meshing',False),('display_threads',args.display_threads),
        ('geometry_qualification','not performed; requested deflection is not a certified whole-surface bound'),
        ('normals','OCCT ComputeNormals: original UV surface estimate with OCCT triangulation fallback'),
        ('uv_preparation','existing prepare_source_uv RAW plus source-linked QUERY deep copy; query_mapped original faces; original surfaces/3D curves/vertices guarded'),
        ('uv_policy','projection precision1e-7mm; finite source residual cap.001mm; guarded seam/period translations; copied columns/joints/domain/pole connectors; cumulative pcurve metric perturbation <= requested deflection/8; not exact trim preservation'),
        ('physical_keys','face-local triangulation transport keys; not source topology qualification'),
        ('views_sha256',hashlib.sha256(request).hexdigest()),('views_scope','independently authored; unmatched'),
        ('palette','illustrative authored presentation, not guaranteed original face styles')))
    stages.append(('complete_reference_caller_before_final_report_close',time.perf_counter()-started))
    tsv(args.output/'timings.tsv',('stage','seconds','scope'),
        ((name,seconds,'actual nested process/caller wall; not additive') for name,seconds in stages))
    (args.output/'COMPLETE').write_text('OCCT REFERENCE; complete original inventory; geometric error not independently certified\n')
    print(f'OCCT REFERENCE COMPLETE: {scope[0]} definitions, {scope[1]} faces, {scope[2]} placements')


def parser():
    result = argparse.ArgumentParser(description=__doc__)
    for name in ('step','output','producer','exporter','display'):
        result.add_argument('--'+name,type=Path,required=True)
    result.add_argument('--deflection',type=float,default=.005)
    result.add_argument('--display-threads',type=int,default=12)
    result.add_argument('--width',type=int,default=2048)
    result.add_argument('--height',type=int,default=1536)
    result.add_argument('--allow-fixture',action='store_true')
    return result


if __name__ == '__main__':
    try:
        main(parser().parse_args())
    except (ValueError,OSError) as error:
        print(f'OCCT REFERENCE FAILED: {error}',file=sys.stderr)
        raise SystemExit(1)
