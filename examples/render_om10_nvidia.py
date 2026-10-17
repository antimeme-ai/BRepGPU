#!/usr/bin/env python3
"""Fresh STEP→NVIDIA CPU tessellation→both shared CPU displays.

Reference only. Run on the fleet after the owned complete-source rendering gate.
Qualification is separate; similarly named tolerance settings do not establish
equal quality. Actual failures keep their reports and never receive a substitute.
"""
import argparse
import csv
import hashlib
import os
from pathlib import Path
import shutil
import signal
import struct
import subprocess
import sys
import time

ROOT=Path(__file__).resolve().parents[1]
sys.path[:0]=[str(ROOT/'python'),str(ROOT/'examples'),str(ROOT/'scripts')]
from brepgpu.imported_assembly import assemble
from brepgpu.nvidia_reference import (load_definition, write_assembly, load_native_model_match,
                                     qualify_physical_definition)
from om10_scene import PALETTE, authored_material, labeled_svg, visible_anchors, write_png
from render_om10 import callouts


def require(value,message):
    if not value:raise ValueError(message)


def digest(path):
    h=hashlib.sha256()
    with Path(path).open('rb') as f:
        while block:=f.read(1<<20):h.update(block)
    return h.hexdigest()


def tsv(path,header,rows):
    with Path(path).open('w',newline='',encoding='utf-8') as f:
        writer=csv.writer(f,delimiter='\t');writer.writerow(header);writer.writerows(rows)


def read_tsv(path):
    with Path(path).open(newline='',encoding='utf-8') as f:
        return tuple(csv.DictReader(f,delimiter='\t'))


def matched_views(path):
    data=path.read_bytes();row=struct.Struct('<4I19dII2d')
    require(len(data)==16+2*row.size and data[:16]==struct.pack('<8sII',b'BGVIEW01',1,2),
            'matched source must contain exactly two actual views')
    from om10_scene import View
    views=[]
    for i in range(2):
        r=row.unpack_from(data,16+i*row.size)
        require(r[4]==float(i) and r[23]==0,'matched views must render alpha0 thenalpha1')
        views.append(View(r[0],r[1],r[2],r[4],r[5:14],r[14:17],r[17:19],r[19],r[20:23],r[23]))
    return data,tuple(views)


def matched_view_path(path,query=0):
    return path/f'query-{query}/views.bgview' if (path/'model.tsv').is_file() else path/'views.bgview'


def matched_inputs(path,assembly,source_sha256,chord_height,query=0):
    """Actual shared authored inputs; never recompute cameras from foreign bounds."""
    if (path/'model.tsv').is_file():
        match=load_native_model_match(path,query,chord_height,source_sha256=source_sha256)
        require(tuple((o.id,o.path,o.product_id,o.definition_id) for o in match.assembly.occurrences)==
                tuple((o.id,o.path,o.product_id,o.definition_id) for o in assembly.occurrences),
                'fresh original occurrence identity differs from native model')
        data,views=matched_views(match.views_path)
        return data,views,match.offsets
    metadata={row['field']:row['value'] for row in read_tsv(path/'metadata.tsv')}
    require(metadata.get('source_sha256')==source_sha256,
            'matched original source SHA differs from fresh reference source')
    require(float(metadata['tolerance_mm'])==chord_height,
            'requested reference chord height differs from owned mesh target')
    data,views=matched_views(path/'views.bgview')
    expected=read_tsv(path/'bom.tsv')
    require(len(expected)==len(assembly.occurrences),'fresh reference occurrence count differs')
    for row,occurrence in zip(expected,assembly.occurrences):
        require((int(row['occurrence_id']),row['source_usage_path'],int(row['product_id']),int(row['solid_id']))==
                (occurrence.id,'/'.join(map(str,occurrence.path)),occurrence.product_id,occurrence.definition_id),
                'fresh original occurrence identity differs from matched source')
        require(tuple(float(row[f'matrix_{i}']) for i in range(12))==occurrence.transform,
                'fresh original occurrence placement differs from matched source')
    offsets={}
    for row in read_tsv(path/'explosion.tsv'):
        id=int(row['occurrence_id']);require(id not in offsets,'duplicate authored explosion')
        offsets[id]=tuple(float(row[f'world_offset_{axis}_mm']) for axis in 'xyz')
    require(set(offsets)=={o.id for o in assembly.occurrences},'authored explosion incomplete')
    return data,tuple(views),tuple(offsets[o.id] for o in assembly.occurrences)


def reference_description(path):
    """Override only the pinned owned-only helper's fixed exported SVG desc."""
    old='<desc>Source: Association openmovement, OM10 MAN VA01.3. Owned tessellation, illustrative palette, lighting, authored explosion and labels by BRepGPU.</desc>'
    new='<desc>Source: Association openmovement, OM10 MAN VA01.3. Genuine NVIDIA USD/BRep CPU tessellation; shared owned CPU display, illustrative palette, lighting, authored explosion and labels by BRepGPU. Actual geometry admission is reported separately.</desc>'
    data=path.read_text(encoding='utf-8')
    require(data.count(old)==1,'shared presentation description changed; reference override requires review')
    path.write_text(data.replace(old,new),encoding='utf-8')


def assignments(definitions,count):
    """Predeclared deterministic largest-first source-face balancing."""
    bins=[[] for _ in range(min(count,len(definitions)))];loads=[0]*len(bins)
    for d in sorted(definitions,key=lambda d:(-len(d.faces),d.id)):
        i=min(range(len(bins)),key=lambda i:(loads[i],i))
        bins[i].append(d);loads[i]+=len(d.faces)
    return tuple(tuple(group) for group in bins)


def run_main(args):
    require(sys.platform=='linux','reference product execution belongs on the fleet')
    from brepgpu.reference_source import load_reference_source
    require(1<=args.workers<=24,'reference worker count must be1..24')
    require(1<=args.display_threads<=256,'reference display thread count must be1..256')
    require(args.chord_height>0 and args.chord_height<float('inf'),'finite positive reference chord required')
    prepared_policy=args.reference_preparation=='periodic-plane'
    curve_angle=(25. if prepared_policy else 0.) if args.curve_angle is None else args.curve_angle
    surface_angle=(25. if prepared_policy else 0.) if args.surface_angle is None else args.surface_angle
    require(all(0<=x<float('inf') for x in (curve_angle,surface_angle)),
            'finite nonnegative reference angles required')
    args.output.mkdir(parents=True,exist_ok=False)
    begin=time.perf_counter();times=[];source_hash=digest(args.step)
    pin_paths=(("reference_source_executable",args.exporter),("nvidia_producer_executable",args.producer),
        ("shared_cpu_display_executable",args.display),
        *((f'package/lib/{name}',args.package/'lib'/name) for name in
          ('libOCCT_BREP_IMPORT.a','libBREP_STAGING.a','libBREP_USD_DATA.so','libSM_API_USD.so',
           'libBREP_SM_USD.so','libUSD_BREP_UTILS.so','libSM_API.so','libSMLib.so')),
        *((f'owned/{name}',ROOT/name) for name in ('examples/render_om10_nvidia.py',
           'examples/om10_scene.py','examples/render_om10.py',
           'python/brepgpu/imported_assembly.py','python/brepgpu/imported_source.py',
           'python/brepgpu/nvidia_reference.py','python/brepgpu/reference_source.py',
           'native/om10_nvidia/main.cpp','native/om10_nvidia/raw_mesh.hpp',
           'native/om10_nvidia/raw_mesh.cpp','native/reference_source/main.cpp',
           'native/om10_reference/raw.hpp','native/om10_reference/main.cpp','native/om10_reference/cpu.cpp')))
    if prepared_policy:
        pin_paths=(*pin_paths,('nvidia_converter_executable',args.converter),
            ('periodic_preparation_executable',args.periodic_preparer),
            ('plane_envelope_executable',args.plane_helper),
            *((f'owned/{name}',ROOT/name) for name in (
                'scripts/prepare_nvidia_reference_worker.py','scripts/prepare_nvidia_plane_envelope.py',
                'native/om10_nvidia/convert.cpp','native/om10_nvidia/plane_envelope.cpp',
                'native/reference_periodic/prepare.cpp','native/reference_periodic/main.cpp')))
    pins=tuple((name,digest(path)) for name,path in pin_paths)
    tsv(args.output/'runtime-pins.tsv',('component','sha256'),pins)
    base_env=dict(os.environ,CUDA_VISIBLE_DEVICES='',PXR_WORK_THREAD_LIMIT='1',OMP_NUM_THREADS='1',
             OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1')
    env=dict(base_env,
             OMNISOLID_PLUGIN_PATH=str(args.package.resolve()/'omniSolid/resources'))
    # Package RPATH and its matching transitive dependencies stay isolated from
    # the separate installed Python wheel. No inherited wheel library override.
    env['LD_LIBRARY_PATH']=os.pathsep.join(str(args.package.resolve()/p) for p in ('lib','extraLibs'))
    if prepared_policy:
        env['PYTHONPATH']=str(args.package.resolve()/'usdpy')
    def run(command,log,environment):
        with log.open('wb') as stream:
            child=subprocess.Popen(tuple(map(str,command)),env=environment,stdout=stream,
                                   stderr=subprocess.STDOUT,start_new_session=True)
            try:
                child.wait()
            finally:
                if child.poll() is None:
                    try:os.killpg(child.pid,signal.SIGTERM)
                    except ProcessLookupError:pass
                    child.wait()
            return child
    start=time.perf_counter()
    result=run((args.exporter,args.step,args.output/'source'),args.output/'source-export.log',base_env)
    times.append(('fresh_step_source_export_process_wall',time.perf_counter()-start))
    require(result.returncode==0,'fresh original STEP export failed; see source-export.log')
    source=load_reference_source(args.output/'source/source.bgocct');assembly=assemble(source)
    require(source.source_sha256==source_hash and digest(args.step)==source_hash,
            'source bytes changed during fresh original transfer')
    if not args.allow_fixture:
        require((len(source.definitions),len(source.faces),len(assembly.occurrences))==(107,9162,166),
                'reference must consume complete original107/9162/166 movement')
    native_match=None
    if (args.matched/'model.tsv').is_file():
        native_match=load_native_model_match(args.matched,args.matched_query,args.chord_height,
            source_sha256=source_hash,reference_source=source)
        assembly=native_match.assembly
        request,views=matched_views(native_match.views_path)
        offsets=native_match.offsets
        shutil.copyfile(native_match.labels_path,args.output/'labels.tsv')
        tsv(args.output/'bom.tsv',('occurrence_id','source_usage_path','product_id','solid_id',
            *(f'matrix_{i}' for i in range(12))),
            ((o.id,'/'.join(map(str,o.path)),o.product_id,o.definition_id,*o.transform)
             for o in assembly.occurrences))
        tsv(args.output/'explosion.tsv',('occurrence_id','world_offset_x_mm','world_offset_y_mm','world_offset_z_mm'),
            ((o.id,*offset) for o,offset in zip(assembly.occurrences,offsets)))
    else:
        request,views,offsets=matched_inputs(args.matched,assembly,source_hash,args.chord_height)
        for name in ('bom.tsv','explosion.tsv'):
            shutil.copyfile(args.matched/name,args.output/name)
    (args.output/'views.bgview').write_bytes(request)
    # Retain fresh complete input identity even when an official worker refuses.
    tsv(args.output/'metadata.tsv',('field','value'),
        (('source_sha256',source_hash),('views_sha256',hashlib.sha256(request).hexdigest()),
         ('chord_height_mm',args.chord_height),('source_definitions',len(source.definitions)),
         ('source_faces',len(source.faces)),('occurrences',len(assembly.occurrences)),
         ('input_admission','complete_native_model_inventory' if native_match else 'legacy_matched_inputs'),
         ('reference_preparation',args.reference_preparation),
         ('curve_angle_degrees',curve_angle),('surface_angle_degrees',surface_angle),
         ('scope','fresh reference preparation; producer completion not yet established')))
    groups=assignments(source.definitions,args.workers)
    tsv(args.output/'assignments.tsv',('worker','source_definition_id','source_face_count'),
        ((i,d.id,len(d.faces)) for i,group in enumerate(groups) for d in group))
    start=time.perf_counter();children=[]
    try:
        for i,group in enumerate(groups):
            if prepared_policy:
                command=[sys.executable,str(ROOT/'scripts/prepare_nvidia_reference_worker.py'),
                    '--preparation-directory',str(args.output/f'preparation-worker-{i:02d}'),
                    '--output-directory',str(args.output/f'worker-{i:02d}'),
                    '--producer',str(args.producer.resolve()),'--converter',str(args.converter.resolve()),
                    '--periodic-preparer',str(args.periodic_preparer.resolve()),
                    '--plane-helper',str(args.plane_helper.resolve()),
                    '--heal',str(args.heal),'--chord-height',str(args.chord_height),
                    '--curve-angle',str(curve_angle),'--surface-angle',str(surface_angle)]
                for d in group:
                    command.extend(('--definition',str(d.id),str((args.output/'source'/d.filename).resolve()),
                                    str(len(d.faces))))
            else:
                command=[str(args.producer),'--heal',str(args.heal),'--chord-height',str(args.chord_height),
                         '--output-directory',str(args.output/f'worker-{i:02d}')]
                if args.curve_angle is not None or args.surface_angle is not None:
                    command.extend(('--curve-angle',str(curve_angle),'--surface-angle',str(surface_angle)))
                for d in group:command.extend(('--definition',str(d.id),str(args.output/'source'/d.filename)))
            log=(args.output/f'worker-{i:02d}.log').open('wb')
            try:process=subprocess.Popen(command,env=env,stdout=log,stderr=subprocess.STDOUT,start_new_session=True)
            except BaseException:log.close();raise
            children.append((i,process,log))
        statuses=[]
        for i,process,log in children:
            statuses.append((i,process.wait()));log.close()
    finally:
        for _,process,log in children:
            if process.poll() is None:
                try:os.killpg(process.pid,signal.SIGTERM)
                except ProcessLookupError:pass
        for _,process,log in children:
            if process.poll() is None:process.wait()
            if not log.closed:log.close()
    times.append(('nvidia_batch_through_all_worker_exits_wall',time.perf_counter()-start))
    tsv(args.output/'worker-exits.tsv',('worker','exit_code'),statuses)
    reports=[]
    for i,_,_ in children:
        path=args.output/f'worker-{i:02d}/definitions.tsv'
        if path.is_file():reports.extend(read_tsv(path))
    if reports:tsv(args.output/'definitions.tsv',tuple(reports[0]),(tuple(r[k] for k in reports[0]) for r in reports))
    if prepared_policy:
        prep_reports=[]
        for i,_,_ in children:
            path=args.output/f'preparation-worker-{i:02d}/definitions.tsv'
            if path.is_file():prep_reports.extend(read_tsv(path))
        if prep_reports:
            tsv(args.output/'preparation-definitions.tsv',tuple(prep_reports[0]),
                (tuple(r[k] for k in prep_reports[0]) for r in prep_reports))
        require(len(prep_reports)==len(source.definitions) and
                {int(r['definition_id']) for r in prep_reports}=={d.id for d in source.definitions}
                and all(r['status']=='PASS' for r in prep_reports),'reference preparation incomplete')
    require(all(code==0 for _,code in statuses),'NVIDIA strict worker failure; complete definition reports retained')
    require(len(reports)==len(source.definitions) and {int(r['definition_id']) for r in reports}==
            {d.id for d in source.definitions} and all(r['status']=='PASS' for r in reports),
            'NVIDIA batch omitted/duplicated/failed original definition')
    start=time.perf_counter();locations={d.id:args.output/f'worker-{i:02d}'/f'definition-{d.id}.bgrefmesh'
                                      for i,group in enumerate(groups) for d in group}
    meshes=tuple(load_definition(locations[d.id],expected_id=d.id) for d in source.definitions)
    if prepared_policy:
        require(native_match is not None or args.allow_fixture,
                'prepared watch reference requires the original native source for source-Euler admission')
        start_physical=time.perf_counter();physical=[]
        for mesh in meshes:
            try:
                if native_match is not None:
                    stats=qualify_physical_definition(mesh,native_match.source,
                                                      native_match.source.definition(mesh.id))
                else:
                    from brepgpu.imported_oracles import qualify_physical_solid
                    stats=qualify_physical_solid(dict(zip(mesh.physical_keys,mesh.points)),
                        tuple(tuple(mesh.physical_keys[i] for i in t) for t in mesh.triangles))
                physical.append((mesh.id,'PASS',stats['vertices'],stats['edges'],stats['triangles'],stats['euler'],''))
            except Exception as error:
                physical.append((mesh.id,'FAIL',0,0,0,0,str(error)))
        tsv(args.output/'physical.tsv',('definition_id','status','vertices','edges','triangles','euler','message'),physical)
        require(all(r[1]=='PASS' for r in physical),'prepared reference physical shell/source-Euler admission failed')
        times.append(('all_definition_physical_shell_source_euler_admission',time.perf_counter()-start_physical))
    tsv(args.output/'geometry-pins.tsv',('source_definition_id','sha256'),
        ((d.id,digest(locations[d.id])) for d in source.definitions))
    products={p.id:p for p in source.products}
    material_map={d.id:authored_material(products[d.product_id].name) for d in source.definitions}
    materials=tuple((rgb,roughness) for _,rgb,roughness in PALETTE)
    write_assembly(args.output/'scene.bgrefmesh',meshes,assembly,offsets,materials,material_map)
    times.append(('genuine_mesh_decode_scene_pack_write',time.perf_counter()-start))
    native_seconds=None
    if args.mode=='render':
        start=time.perf_counter()
        result=run((args.display,'--mesh',args.output/'scene.bgrefmesh','--views',args.output/'views.bgview',
                    '--threads',str(args.display_threads),'--output-directory',args.output/'native'),args.output/'display.log',base_env)
        native_seconds=time.perf_counter()-start;times.append(('shared_owned_cpu_display_process_wall',native_seconds))
        require(result.returncode==0,'shared owned CPU display failed; see display.log')
        for i,(name,view) in enumerate(zip(('assembled','exploded'),views)):
            start=time.perf_counter();prefix=args.output/'native'/f'frame-{i:06d}'
            png=args.output/f'{i:03d}-{name}.png'
            write_png(png,view.width,view.height,prefix.with_suffix('.rgba').read_bytes())
            anchors=visible_anchors(prefix.with_suffix('.instance'),view.width,view.height,
                                    valid_ids=tuple(o.id for o in assembly.occurrences))
            labels,rows=callouts(assembly,anchors,view.width,view.height)
            tsv(args.output/f'{i:03d}-{name}-callouts.tsv',('label','visible_anchor_occurrence_id',
                'source_group_occurrence_ids','anchor_x','anchor_y','label_x','label_y'),rows)
            labeled_svg(args.output/f'{i:03d}-{name}.svg',png,view,title='OM10 / '+name.upper(),
                subtitle=f'NVIDIA USD/BRep CPU import+tessellation · heal={args.heal} · shared owned CPU{args.display_threads} display',
                counts=(('source definitions',len(source.definitions)),('source faces',len(source.faces)),
                        ('placed parts',len(assembly.occurrences))),
                timings=(('CPU display process, both views',f'{native_seconds:.3f} s'),
                         ('requested chord height',f'{args.chord_height:g} mm'),
                         ('actual geometry admission','reported separately')),
                callouts=labels,
                attribution='Association openmovement · OM10 MAN VA01.3 · CC BY-SA 3.0 · NVIDIA reference mesh; BRepGPU display and labels')
            reference_description(args.output/f'{i:03d}-{name}.svg')
            times.append((f'{name}_png_svg_labels_write',time.perf_counter()-start))
    require(digest(args.step)==source_hash,'original source changed during reference caller')
    require(tuple((name,digest(path)) for name,path in pin_paths)==pins,
            'reference executable/library/source changed during caller')
    metadata=(('source_sha256',source_hash),('views_sha256',hashlib.sha256(request).hexdigest()),
        ('geometry','genuine NVIDIA definition-local f64 tessellation, official rectangle polygon triangulation and retained original oriented per-corner normals'),
        ('polygon_triangulation','official SDK TriangulatePlanar for exact axis-extruded rectangles; per-definition counts in worker polygon-export.tsv'),
        ('scope','fresh STEP through '+('both labeled views' if args.mode=='render' else 'NVIDIA mesh only')),
        ('heal',args.heal),('chord_height_mm',args.chord_height),('curve_angle_degrees',curve_angle),
        ('surface_angle_degrees',surface_angle),('reference_preparation',args.reference_preparation),
        ('physical_shell_source_euler','PASS' if prepared_policy else 'not_performed_in_caller'),
        ('independent_distance_qualification','not_performed_in_caller'),
        ('allow_partial',False),('workers',len(groups)),
        ('display_threads',args.display_threads),('source_definitions',len(source.definitions)),('source_faces',len(source.faces)),
        ('occurrences',len(assembly.occurrences)),('requested_quality_is_measured_admission',False),
        ('input_admission','complete_native_model_inventory' if native_match else 'legacy_matched_inputs'),
        ('available_cpu_affinity',','.join(map(str,sorted(os.sched_getaffinity(0))))),
        *((key,env[key]) for key in ('PXR_WORK_THREAD_LIMIT','OMP_NUM_THREADS','OPENBLAS_NUM_THREADS','MKL_NUM_THREADS')))
    tsv(args.output/'metadata.tsv',('field','value'),metadata)
    times.append(('caller_through_outputs_before_timing_write_and_interpreter_exit',time.perf_counter()-begin))
    tsv(args.output/'timings.tsv',('stage','seconds','scope'),
        ((name,seconds,'actual nested wall; not additive; outer driver includes metadata/timing writes and exit') for name,seconds in times))
    print('NVIDIA reference '+args.mode+' complete; geometry admission remains separate')


def main(args):
    # TERM must enter the same child/process-group cleanup as ordinary errors.
    # This also covers source export and display subprocesses outside the batch.
    terminating=False
    def terminate(signum,frame):
        nonlocal terminating
        if terminating:return
        terminating=True
        raise InterruptedError('reference caller terminated')
    previous=signal.signal(signal.SIGTERM,terminate)
    try:
        return run_main(args)
    finally:
        signal.signal(signal.SIGTERM,previous)


def parser():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--step',type=Path,required=True);p.add_argument('--matched',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True);p.add_argument('--heal',type=int,choices=(0,1),required=True)
    p.add_argument('--mode',choices=('prepare','render'),default='prepare')
    p.add_argument('--matched-query',type=int,default=0);p.add_argument('--display-threads',type=int,default=24);p.add_argument('--workers',type=int,default=24);p.add_argument('--chord-height',type=float,default=.005)
    p.add_argument('--package',type=Path,default=ROOT/'quarantine/nvidia-usd-brep-1.0.0-linux')
    p.add_argument('--exporter',type=Path,default=ROOT/'build/om10-reference-source/brepgpu-reference-source')
    p.add_argument('--producer',type=Path,default=ROOT/'build/om10-nvidia/brepgpu-om10-nvidia')
    p.add_argument('--reference-preparation',choices=('none','periodic-plane'),default='none')
    p.add_argument('--curve-angle',type=float);p.add_argument('--surface-angle',type=float)
    p.add_argument('--converter',type=Path,default=ROOT/'build/nvidia-reference-recovery/brepgpu-nvidia-convert')
    p.add_argument('--periodic-preparer',type=Path,default=ROOT/'build/reference-periodic/brepgpu-reference-periodic')
    p.add_argument('--plane-helper',type=Path,default=ROOT/'build/nvidia-reference-recovery/brepgpu-nvidia-plane-envelope')
    p.add_argument('--display',type=Path,default=ROOT/'build/om10-reference/brepgpu-om10-reference-display')
    p.add_argument('--allow-fixture',action='store_true',help='explicit small authored integration fixture; never actualwatch benchmark')
    return p


if __name__=='__main__':
    try:main(parser().parse_args())
    except Exception as error:
        print(f'NVIDIA reference failed: {error}',file=sys.stderr);sys.exit(1)
