#!/usr/bin/env python3
"""Construct, consume, qualify, export and CPU-display actual watch-study parts."""
import argparse
from fractions import Fraction as F
import hashlib
from pathlib import Path
import sys
import time

from brepgpu.extrusion_meshing import tessellate as mesh_extrusion
from brepgpu.extrusion_meshing_oracles import qualify_mesh as qualify_extrusion_mesh
from brepgpu.feature_bore_meshing import tessellate as mesh_bore
from brepgpu.feature_bore_meshing_oracles import qualify_mesh as qualify_bore_mesh
from mesh_body import read_mesh,tsv,ratio,write_source
from mesh_extrusion import tree,read_history as read_extrusion_history
from watch_scene import recipes,construct
from watch_display import render,MATERIALS

ROOT=Path(__file__).resolve().parents[1]
TOLERANCE=F(1,256)
TABLES={'vertex':'vertices','edge':'edges','coedge':'coedges','loop':'loops','face':'faces','chart':'charts'}


def pins():
    paths=tuple(sorted((ROOT/'python/brepgpu').glob('*.py')))+tuple(
        ROOT/'examples'/name for name in ('mesh_body.py','mesh_extrusion.py','watch_scene.py','watch_display.py','render_watch.py'))
    return tuple((str(path.relative_to(ROOT)),hashlib.sha256(path.read_bytes()).hexdigest()) for path in paths)


def require(ok,message):
    if not ok:raise AssertionError(message)


def read_bore_history(result):
    """Consume actual root records and ordered map steps, not merely key counts."""
    checksum=0;paths=0;steps=0;roots=0
    actual_roots=(result.feature.profile,result.cutter)
    immediate_sources=(result.feature.solid,result.cutter)
    for record in result.history.records:
        require(result.history.from_input(record.source) is record,'immediate forward lookup lost actual record')
        source=next((body for body in immediate_sources if
                     (body.body_id,body.revision)==(record.source.body_id,record.source.revision)),None)
        require(source is not None,'immediate disposition escaped original operands')
        original=next(item for item in getattr(source,TABLES[record.source.kind]) if item.id==record.source.entity_id)
        for label,value in (('immediate-source',original),('immediate-record',record)):
            for name,kind,data in tree(label,value):checksum+=sum(map(ord,name+kind+str(data)))
        for target in (*record.images,*record.generated):
            require(record.source in result.history.origin(target).parents,'actual immediate forward source lost')
    for origin in result.composed_history.origins:
        require(result.composed_history.origin(origin.target) is origin,'composed lookup lost object')
        require((origin.target.body_id,origin.target.revision)==(result.solid.body_id,result.solid.revision),
                'composed target escaped actual result')
        immediate=result.history.origin(origin.target)
        for path in origin.paths:
            root=next((source for source in actual_roots if
                       (source.body_id,source.revision)==(path.root.body_id,path.root.revision)),None)
            require(root is not None,'history escaped actual original profile/cutter')
            original=next(item for item in getattr(root,TABLES[path.root.kind]) if item.id==path.root.entity_id)
            require(path.steps and path.steps[0] is immediate,'path replaced actual immediate map')
            if root is result.feature.profile:
                require(len(path.steps)==2 and path.steps[1] is result.feature.history.origin(path.steps[1].target),
                        'path replaced original extrusion map')
            else:require(len(path.steps)==1,'cutter path has fabricated extrusion step')
            record=result.composed_history.from_input(path.root)
            require(origin.target in (*record.images,*record.generated),'root-to-result consumer differs')
            for label,value in (('source',original),('path',path)):
                for name,kind,data in tree(label,value):checksum+=sum(map(ord,name+kind+str(data)))
            paths+=1;steps+=len(path.steps)
    for record in result.composed_history.records:
        require(result.composed_history.from_input(record.source) is record,'root record lookup lost object')
        roots+=1
        for name,kind,data in tree('record',record):checksum+=sum(map(ord,name+kind+str(data)))
    for origin in result.history.origins:
        require(result.history.origin(origin.target) is origin,'immediate lookup lost object')
        for name,kind,data in tree('immediate',origin):checksum+=sum(map(ord,name+kind+str(data)))
    return (('immediate_input_dispositions',len(result.history.records)),
            ('original_root_records',roots),('result_origins',len(result.composed_history.origins)),
            ('ordered_paths_read',paths),('actual_origin_steps_read',steps),('history_checksum',checksum))


def ply(path,parts):
    vertices=sum(len(mesh.vertices) for _,mesh,_ in parts)
    triangles=sum(len(mesh.triangles) for _,mesh,_ in parts)
    with path.open('w') as out:
        out.write('ply\nformat ascii 1.0\ncomment actual BRepGPU rounded indexed assembly; separate solids\n')
        out.write(f'element vertex {vertices}\nproperty double x\nproperty double y\nproperty double z\n')
        out.write(f'element face {triangles}\nproperty list uchar uint vertex_indices\n')
        out.write('property uint part_id\nproperty uint source_face_id\nproperty uchar red\nproperty uchar green\nproperty uchar blue\nend_header\n')
        for _,_,reader in parts:
            for i in range(0,len(reader.xyz),3):out.write(' '.join(format(x,'.17g') for x in reader.xyz[i:i+3])+'\n')
        offset=0
        for part_id,(recipe,mesh,reader) in enumerate(parts):
            color=tuple(round(255*x) for x in MATERIALS[recipe.material])
            for i,face in enumerate(reader.face_ids):
                ids=reader.indices[3*i:3*i+3]
                out.write('3 '+' '.join(map(str,(*(offset+x for x in ids),part_id,face,*color)))+'\n')
            offset+=len(mesh.vertices)


def export_part(directory,recipe,profile,feature,bore,mesh,reader,history,quality):
    directory.mkdir()
    tsv(directory/'recipe.tsv',('path','type','value'),tree('recipe',recipe))
    tsv(directory/'original-profile.tsv',('path','type','value'),tree('profile',profile))
    tsv(directory/'extrusion-history.tsv',('path','type','value'),
        (*tree('records',feature.history.records),*tree('origins',feature.history.origins)))
    names=('body_id','revision','frame','height','vertices','curves','edges','coedges','loops','faces','charts','patches','volume')
    tsv(directory/'actual-extrusion.tsv',('path','type','value'),
        (row for name in names for row in tree('extrusion/'+name,getattr(feature.solid,name))))
    if bore is not None:names+=('center','radius','wall_band')
    tsv(directory/'actual-result.tsv',('path','type','value'),
        (row for name in names for row in tree('result/'+name,getattr(mesh.body,name))))
    if bore is not None:
        cutter_directory=directory/'original-cutter';cutter_directory.mkdir();write_source(cutter_directory,bore.cutter)
        tsv(directory/'immediate-history.tsv',('path','type','value'),
            (*tree('records',bore.history.records),*tree('origins',bore.history.origins)))
        tsv(directory/'composed-history.tsv',('path','type','value'),
            (*tree('records',bore.composed_history.records),*tree('origins',bore.composed_history.origins)))
    tsv(directory/'history-reader.tsv',('metric','value'),history)
    tsv(directory/'mesh.tsv',('path','type','value'),
        (row for name in ('vertices','triangles','edge_polylines','boundary_uses','tolerance',
                          'quarter_segments','curve_bound','rounding_bound','error_bound')
         for row in tree('mesh/'+name,getattr(mesh,name))))
    tsv(directory/'indexed-reader.tsv',('metric','value'),reader.metrics)
    tsv(directory/'qualification.tsv',('field','value'),(('result','PASS'),*((key,value) for key,value in quality.items())))
    ply(directory/'mesh.ply',((recipe,mesh,reader),))


def main(args):
    require(sys.platform=='linux','product/mesh/display callers execute on the fleet')
    full_begin=time.perf_counter_ns();source_pins=pins()
    args.output_directory.mkdir(parents=True,exist_ok=False)
    (args.output_directory/'parts').mkdir()
    source_recipes=recipes();require(len({r.name for r in source_recipes})==len(source_recipes),'duplicate part name')
    parts=[];inventory=[];timings=[];triangle_map=[]
    for part_id,recipe in enumerate(source_recipes):
        start=time.perf_counter_ns();profile,feature,bore=construct(recipe);constructed=time.perf_counter_ns()
        if bore is None:mesh=mesh_extrusion(feature,tolerance=TOLERANCE)
        else:mesh=mesh_bore(bore,tolerance=TOLERANCE,max_quarter_segments=64)
        meshed=time.perf_counter_ns();reader=read_mesh(mesh)
        history=read_extrusion_history(feature) if bore is None else read_bore_history(bore)
        consumed=time.perf_counter_ns()
        require(profile is feature.profile,'construction replaced original source')
        if bore is None:
            require(mesh.feature_result is feature and mesh.body is feature.solid,'mesh replaced actual feature')
            quality=qualify_extrusion_mesh(mesh)
        else:
            require(bore.feature is feature and mesh.bore_result is bore and mesh.body is bore.solid,
                    'mesh replaced actual feature/cut result')
            require(mesh.history is bore.history and mesh.composed_history is bore.composed_history,
                    'mesh replaced actual ordered histories')
            quality=qualify_bore_mesh(mesh)
        checked=time.perf_counter_ns()
        export_part(args.output_directory/'parts'/recipe.name,recipe,profile,feature,bore,mesh,reader,history,quality)
        exported=time.perf_counter_ns()
        timings.append((recipe.name,(constructed-start)/1e6,(meshed-constructed)/1e6,
                        (consumed-meshed)/1e6,(consumed-start)/1e6,(checked-consumed)/1e6,(exported-checked)/1e6))
        inventory.append((part_id,recipe.name,recipe.role,recipe.material,profile.body_id,profile.revision,
                          feature.solid.body_id,feature.solid.revision,
                          '' if bore is None else bore.cutter.body_id,'' if bore is None else bore.cutter.revision,
                          mesh.body.body_id,mesh.body.revision,len(mesh.vertices),len(mesh.triangles),
                          ratio(mesh.error_bound),ratio(TOLERANCE),*recipe.origin,recipe.scale,
                          ratio(recipe.height),'actual source Frame; no instance transform'))
        triangle_map.extend((part_id,recipe.name,t.id,t.face_id,*t.vertices) for t in mesh.triangles)
        parts.append((recipe,mesh,reader))
        print(f'{part_id+1}/{len(source_recipes)} {recipe.name}: {len(mesh.triangles)} triangles PASS',flush=True)
    assembly_start=time.perf_counter_ns();ply(args.output_directory/'assembly.ply',parts)
    tsv(args.output_directory/'parts.tsv',('part_id','part','role','material','profile_body','profile_revision',
        'extrusion_body','extrusion_revision','cutter_body','cutter_revision','result_body','result_revision',
        'vertices','triangles','error_bound','world_tolerance','origin_x','origin_y','origin_z','scale','local_height','placement'),inventory)
    tsv(args.output_directory/'triangle-parts.tsv',('part_id','part','triangle_id','source_face_id','v0','v1','v2'),triangle_map)
    tsv(args.output_directory/'materials.tsv',('material','red_linear_display','green_linear_display','blue_linear_display'),
        ((name,*color) for name,color in MATERIALS.items()))
    tsv(args.output_directory/'timings-parts.tsv',('part','source_and_construction_ms','mesh_ms','indexed_and_history_reader_ms',
        'construct_to_consumed_ms','independent_qualification_ms','part_filesystem_export_ms'),timings)
    assembly_end=time.perf_counter_ns();render_times=[]
    for name,az,el,display_scale in (('hero',-16.,57.,410.),('top',0.,90.,385.)):
        start=time.perf_counter_ns();scene=render(args.output_directory,parts,name=name,azimuth=az,elevation=el,scale=display_scale)
        render_times.append((name,(time.perf_counter_ns()-start)/1e6))
        tsv(args.output_directory/(name+'-scene.tsv'),('field','value'),scene)
    require(pins()==source_pins,'caller/product source changed during actual work')
    tsv(args.output_directory/'sources.tsv',('path','sha256'),source_pins)
    full_end=time.perf_counter_ns()
    totals=[('sum_source_and_construction',sum(r[1] for r in timings)),('sum_mesh',sum(r[2] for r in timings)),
            ('sum_indexed_and_history_reader',sum(r[3] for r in timings)),
            ('sum_construct_to_consumed',sum(r[4] for r in timings)),
            ('sum_independent_qualification',sum(r[5] for r in timings)),
            ('sum_part_filesystem_export',sum(r[6] for r in timings)),
            ('assembly_filesystem_export',(assembly_end-assembly_start)/1e6),
            *((name+'_cpu_display_including_png_svg_writes',elapsed) for name,elapsed in render_times),
            ('full_caller_through_final_source_check',(full_end-full_begin)/1e6)]
    tsv(args.output_directory/'timings.tsv',('scope','milliseconds'),totals)
    (args.output_directory/'SCOPE.md').write_text(
        '# Actual openwork movement study 01\n\n'
        f'{len(parts)} separately modeled solids. Every pierced part consumes its actual '
        'profile → extrusion → bore → mesh chain and retains immediate and ordered '
        'root histories. Every unpierced part is an actual convex extrusion mesh. '
        f'All distinct actual meshes were independently qualified; world tolerance {ratio(TOLERANCE)}. '
        'Placement is in original source Frames, with no display geometry transform.\n\n'
        'This is an original schematic, nonfunctional watch-inspired assembly. '
        'Radial lugs, spokes, posts and raised head stripes are separate solids; '
        'they are not fused gears or machined slots. Gear contact, ratios, '
        'interference clearance, escapement, springs, kinematics and manufacturing '
        'dimensions are not established. No source watch CAD/image was copied.\n\n'
        'All construction, meshing, qualification and depth-buffer display run on CPU. '
        'No native/CUDA calls, device transfers or GPU rendering. Part timings '
        'separate construction, mesh, actual indexed/source-map consumption, independent '
        'qualification and export. Construct-to-consumed overlaps its three stages. '
        'Render includes PNG encoding and surface/SVG writes; scene-table writes follow '
        'that scope. Full wall starts after imports/argument parsing and includes all '
        'part work, qualification, files, display and final source-hash check; it excludes '
        'final timings/SCOPE writes, process shutdown and standalone SVG composition. '
        'One actual assembly run; no speedup or variability claim. External alpha '
        'qualification remains incomplete.\n')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output-directory',required=True,type=Path)
    main(parser.parse_args())
