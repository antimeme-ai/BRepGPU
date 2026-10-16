#!/usr/bin/env python3
"""Qualify one fresh whole OM10 model on CPU and both P100s without rasterizing.

Uses existing independent source/mesh/native checks. These are qualification
walls, not fresh render benchmarks. Product execution is fleet-only.
"""
import argparse
import hashlib
from pathlib import Path
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[1]
sys.path[:0] = [str(ROOT/'python'), str(ROOT/'examples')]

from render_om10 import OM10_SHA256, pin_files, require, source_tables, tsv
from om10_scene import (PALETTE, authored_layout, authored_material, camera_view,
                        pack_views, placed_corners)
from brepgpu.imported_source import load_source
from brepgpu.imported_meshing import MeshQuality, SceneInstance, SceneMaterial, mesh_imported
from brepgpu.imported_assembly import assemble
from brepgpu.imported_oracles import (qualify_mesh, qualify_original_geometry,
                                      qualify_original_assembly, qualify_native_scene)


def main(args):
    require(sys.platform == 'linux', 'whole-source qualification executes on the Linux fleet')
    begin = time.perf_counter()
    own_path = Path(__file__).resolve()
    pins = (*pin_files(), (str(own_path.relative_to(ROOT)), hashlib.sha256(own_path.read_bytes()).hexdigest()))
    binaries = (('step_source_adapter', args.adapter), ('native_executable', args.native), ('cuda_plugin', args.plugin))
    if args.domain_backend == 'native':
        from brepgpu.face_domain import library_path
        binaries += (('native_face_domain', library_path()),)
    def binary_hashes():
        return tuple((role, hashlib.sha256(path.read_bytes()).hexdigest()) for role, path in binaries)
    executable_pins = binary_hashes()
    require(hashlib.sha256(args.step.read_bytes()).hexdigest() == OM10_SHA256, 'supplied source hash differs')
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    times, metrics = [], []
    def measure(name, call):
        start = time.perf_counter()
        result = call()
        times.append((name, time.perf_counter()-start))
        tsv(out/'timings.tsv', ('stage', 'seconds', 'scope'),
            ((stage, seconds, 'qualification wall; nested stages are not additive; no raster') for stage, seconds in times))
        return result
    def record(prefix, values):
        metrics.extend((prefix+'/'+key, value) for key, value in values.items())
        tsv(out/'qualification.tsv', ('field', 'value'), metrics)
    with (out/'adapter.tsv').open('w') as log:
        measure('foreign_cpu_step_translation', lambda: subprocess.run(
            (str(args.adapter.resolve()), str(args.step.resolve()), str(out/'source.bgsource'),
             '--query-brep-dir',str(out/'query-source'),'--source-residual-mm',str(args.source_residual),
             '--tolerance-mm',str(args.tolerance)),
            stdout=log, stderr=subprocess.STDOUT, check=True))
    source = measure('owned_source_decode', lambda: load_source(out/'source.bgsource'))
    require(source.source_sha256 == OM10_SHA256 and
            (len(source.definitions), len(source.faces), len(source.products), len(source.usages)) == (107,9162,138,196),
            'complete original source inventory differs')
    query_paths=tuple((d.id,out/'query-source'/f'definition-{d.id}.brep') for d in source.definitions)
    require({p.name for p in (out/'query-source').iterdir()}=={p.name for _,p in query_paths},
            'complete prepared query source inventory differs')
    def source_hashes():
        return (hashlib.sha256((out/'source.bgsource').read_bytes()).hexdigest(),
                *(hashlib.sha256(path.read_bytes()).hexdigest() for _,path in query_paths))
    query_pins=source_hashes()
    quality = MeshQuality(args.tolerance, args.source_residual, args.max_refinement, args.max_edge_samples)
    assembly = measure('owned_complete_assembly', lambda: assemble(source))
    require(len(assembly.occurrences) == 166 and len({o.definition_id for o in assembly.occurrences}) == 107,
            'complete expanded assembly differs')
    record('original_geometry', measure('qualify_original_geometry', lambda: qualify_original_geometry(source,args.step)))
    record('original_assembly', measure('qualify_original_assembly', lambda: qualify_original_assembly(source,args.step,assembly)))
    mesh = measure('owned_complete_mesh', lambda: mesh_imported(source, quality, domain_backend=args.domain_backend,
        progress=lambda *row: print(*row, flush=True)))
    record('mesh', measure('qualify_mesh', lambda: qualify_mesh(mesh,original_path=args.step)))
    bounds = mesh.definition_bounds
    layout, records, axes = authored_layout(assembly,bounds)
    scene, views = out/'scene.bgmesh', out/'views.bgview'
    instances = tuple(SceneInstance(o.id,o.definition_id,o.transform,layout.offsets[o.id-1]) for o in assembly.occurrences)
    materials = tuple(SceneMaterial(rgb,roughness) for _,rgb,roughness in PALETTE)
    mesh.write_scene(scene,instances,materials,definition_materials=tuple(
        (d.id,authored_material(source.product(d.product_id).name)) for d in source.definitions))
    source_tables(out,source,assembly,records,mesh)
    views.write_bytes(pack_views(tuple(camera_view(placed_corners(assembly,bounds,layout,alpha),
        width=args.width,height=args.height,supersample=args.supersample,alpha=alpha,
        azimuth=args.azimuth,elevation=args.elevation,padding=(360,360,210,240),axes=axes,flags=3)
        for alpha in (0.,1.))))
    def input_hashes():
        return tuple(hashlib.sha256(p.read_bytes()).hexdigest() for p in (scene,views))
    inputs = input_hashes()
    tsv(out/'inputs.tsv',('field','value'), (
        ('source_sha256',OM10_SHA256),('scene_sha256',inputs[0]),('views_sha256',inputs[1]),
        ('source_snapshot_path','source.bgsource'),('source_snapshot_sha256',query_pins[0]),
        ('query_source_directory','query-source'),('query_definition_count',len(query_paths)),
        *((key,value) for index,(id,path) in enumerate(query_paths) for key,value in (
            (f'query_definition_{id}_path',str(path.relative_to(out))),
            (f'query_definition_{id}_sha256',query_pins[index+1]))),
        ('tolerance_mm',args.tolerance),('source_residual_mm',args.source_residual),
        ('domain_backend',args.domain_backend),
        ('max_refinement',args.max_refinement),('max_edge_samples',args.max_edge_samples),
        ('width',args.width),('height',args.height),('supersample',args.supersample),
        ('azimuth',args.azimuth),('elevation',args.elevation),('view_flags',3)))
    for label,backend,device in (('cpu24','cpu',0),('p100-0','cuda',0),('p100-1','cuda',1)):
        native_out = out/label
        command = [str(args.native.resolve()),'--scene',str(scene),'--views',str(views),
                   '--output-directory',str(native_out),'--backend',backend,'--threads','24','--dump-source-jets']
        if backend == 'cuda':
            command += ['--device',str(device),'--plugin',str(args.plugin.resolve())]
        with (out/(label+'-process.tsv')).open('w') as log:
            measure(label+'/native_process_including_exit', lambda: subprocess.run(
                command,stdout=log,stderr=subprocess.STDOUT,check=True))
        require(not any(native_out.glob('*.rgba')), 'nonvisual qualification unexpectedly emitted raster images')
        record(label,measure(label+'/qualify_actual_native', lambda: qualify_native_scene(
            mesh,assembly,scene,native_out/'source-jets.bin',
            tuple(native_out/f'frame-{i:06d}.projected' for i in range(2)),views_path=views,explosion=layout)))
        require(input_hashes() == inputs,'actual native scene/view inputs changed across backends')
        require(source_hashes()==query_pins,'source snapshot/prepared query bytes changed during qualification')
        print(label,'complete original-source native geometry PASS',flush=True)
    require((*pin_files(),(str(own_path.relative_to(ROOT)),hashlib.sha256(own_path.read_bytes()).hexdigest())) == pins,
            'qualification/product source changed during operation')
    require(binary_hashes() == executable_pins,'selected binaries changed during qualification')
    tsv(out/'sources.tsv',('path','sha256'),pins)
    tsv(out/'binaries.tsv',('role','sha256'),executable_pins)
    tsv(out/'mesh-errors.tsv',('metric','value'),mesh.error_metrics)
    times.append(('complete_nonvisual_qualification',time.perf_counter()-begin))
    tsv(out/'timings.tsv',('stage','seconds','scope'),
        ((stage,seconds,'qualification wall; no raster; nested stages not additive') for stage,seconds in times))


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--step',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True)
    p.add_argument('--adapter',type=Path,default=ROOT/'build/om10-step-source/brepgpu-step-source')
    p.add_argument('--native',type=Path,default=ROOT/'build/om10-release/brepgpu-om10-native')
    p.add_argument('--plugin',type=Path,default=ROOT/'build/om10-release/libbrepgpu_om10_cuda.so')
    p.add_argument('--domain-backend',choices=('native','python'),default='native')
    p.add_argument('--width',type=int,default=2400)
    p.add_argument('--height',type=int,default=2000)
    p.add_argument('--supersample',type=int,choices=(1,2),default=2)
    p.add_argument('--azimuth',type=float,default=-58.)
    p.add_argument('--elevation',type=float,default=52.)
    p.add_argument('--tolerance',type=float,default=.005)
    p.add_argument('--source-residual',type=float,default=.001)
    p.add_argument('--max-refinement',type=int,default=20)
    p.add_argument('--max-edge-samples',type=int,default=16384)
    main(p.parse_args())
