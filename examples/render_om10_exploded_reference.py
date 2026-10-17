#!/usr/bin/env python3
"""Replay genuine complete NVIDIA geometry with the exact authored OM10 diagram.

No import, tessellation, repaired normals, geometry substitution or remeshing.
The source inventory/placement admission below is not numerical qualification.
"""
import argparse
import math
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import time

ROOT=Path(__file__).resolve().parents[1]
sys.path[:0]=[str(ROOT/'python'),str(ROOT/'examples')]
from brepgpu.nvidia_reference import load_native_model_match
from brepgpu.reference_source import load_reference_source
from om10_explosion import LayoutRecord,require,stored_geometry,translated_reference_scene
from om10_scene import write_png
from render_om10_nvidia import matched_views
from render_om10_exploded import (rows,tsv,digest,pixel_inventory,make_annotation,poster,
                                  CRITICAL_NAMES)


def reference_inventory(data,source,reports):
    """Check SDK ordinals; original coverage belongs to the retained source/pipeline history."""
    h=struct.unpack_from('<8sII6Q',data);nd,ni,nv,nt,nm=h[3:8]
    definitions=tuple(struct.unpack_from('<5Q',data,64+40*i) for i in range(nd))
    expected={int(r['definition_id']):int(r['input_faces']) for r in reports}
    require(len(expected)==len(reports)==len(source.definitions) and
            set(expected)=={d.id for d in source.definitions} and all(n>0 for n in expected.values()),
            'NVIDIA staged face inventory differs')
    require(len(definitions)==len(expected) and {d[0] for d in definitions}==set(expected),
            'NVIDIA original definition inventory differs')
    triangle_start=64+40*nd+136*ni+56*nv
    previous_v=previous_t=0
    for identifier,fv,vc,ft,tc in definitions:
        require((fv,ft)==(previous_v,previous_t) and vc>0 and tc>0 and
                fv+vc<=nv and ft+tc<=nt,'NVIDIA definition partition differs')
        faces=set()
        for i in range(ft,ft+tc):
            a,b,c,material,face=struct.unpack_from('<4IQ',data,triangle_start+24*i)
            require(len({a,b,c})==3 and max(a,b,c)<vc and material<nm,
                    'NVIDIA triangle range/material differs')
            faces.add(face)
        require(len(faces)==expected[identifier] and min(faces)==0 and
                max(faces)==expected[identifier]-1,'NVIDIA SDK face ordinal inventory differs')
        previous_v,previous_t=fv+vc,ft+tc
    require((previous_v,previous_t)==(nv,nt),'NVIDIA geometry ranges incomplete')


def main(args):
    require(sys.platform=='linux','reference display execution belongs on the fleet')
    require(not args.output.exists(),'new output directory required')
    started=time.perf_counter()
    source=load_reference_source(args.reference/'source/source.bgocct')
    match=load_native_model_match(args.model,args.query,.005,
        source_sha256=source.source_sha256,reference_source=source)
    require((len(source.definitions),len(source.faces),len(match.assembly.occurrences))==(107,9162,166),
            'complete original watch required')
    metadata={r['field']:r['value'] for r in rows(args.reference/'metadata.tsv')}
    chord=float(metadata['chord_height_mm']);curve_angle=float(metadata['curve_angle_degrees'])
    surface_angle=float(metadata['surface_angle_degrees']);heal=int(metadata['heal'])
    require(chord==.005 and all(math.isfinite(x) and x>=0 for x in (curve_angle,surface_angle)) and
            heal in (0,1) and args.config==f'heal{heal}','reference requested policy differs')
    require(metadata['source_sha256']==source.source_sha256 and
            (int(metadata['source_definitions']),int(metadata['source_faces']),int(metadata['occurrences']))==(107,9162,166),
            'reference source inventory metadata differs')
    reports=rows(args.reference/'definitions.tsv')
    require(len(reports)==107 and len({int(r['definition_id']) for r in reports})==107 and
            {int(r['definition_id']) for r in reports}=={d.id for d in source.definitions} and
            all(r['status']=='PASS' and int(r['heal'])==heal and float(r['chord_mm'])==chord for r in reports),
            'NVIDIA definition pipeline incomplete or requested policy differs')
    physical=rows(args.reference/'physical.tsv')
    require(len(physical)==107 and len({int(r['definition_id']) for r in physical})==107 and
            {int(r['definition_id']) for r in physical}=={d.id for d in source.definitions} and
            all(r['status']=='PASS' for r in physical) and metadata['physical_shell_source_euler']=='PASS',
            'complete existing NVIDIA physical/source-Euler admission required')
    expected={o.id:(o.definition_id,o.transform) for o in match.assembly.occurrences}
    bom=rows(args.reference/'bom.tsv');by_id={o.id:o for o in match.assembly.occurrences}
    require(len(bom)==166 and len({int(r['occurrence_id']) for r in bom})==166,
            'NVIDIA occurrence/path word incomplete')
    for r in bom:
        identifier=int(r['occurrence_id']);require(identifier in by_id,'unknown NVIDIA occurrence')
        o=by_id[identifier]
        require((r['source_usage_path'],int(r['product_id']),int(r['solid_id']))==
                ('/'.join(map(str,o.path)),o.product_id,o.definition_id) and
                struct.pack('<12d',*(float(r[f'matrix_{i}']) for i in range(12)))==struct.pack('<12d',*o.transform),
                'NVIDIA original path/product/placement differs')
    require((args.layout/'labels.tsv').read_bytes()==match.labels_path.read_bytes() and
            (args.reference/'labels.tsv').read_bytes()==match.labels_path.read_bytes(),
            'original166 label mapping differs')
    require((args.layout/'COMPLETE').is_file(),'final authored layout required')
    layout_metadata={r['key']:r['value'] for r in rows(args.layout/'metadata.tsv')}
    require(layout_metadata['source_sha256']==source.source_sha256,'layout original source differs')
    records=[]
    for r in rows(args.layout/'explosion.tsv'):
        identifier=int(r['occurrence_id']);require(identifier in by_id,'unknown layout occurrence')
        o=by_id[identifier]
        require((r['source_usage_path'],int(r['product_id']),int(r['definition_id']))==
                ('/'.join(map(str,o.path)),o.product_id,o.definition_id),'layout original usage differs')
        records.append(LayoutRecord(identifier,int(r['display_label']),r['source_usage_path'],o.product_id,
            o.definition_id,r['source_group'],r['module'],r['role'],
            tuple(float(r['world_offset_'+a+'_mm']) for a in 'xyz'),r['reason']))
    offsets={r.occurrence_id:r.offset for r in records}
    require(len(records)==len(offsets)==166 and set(offsets)==set(expected),'layout occurrence word incomplete')
    labels=rows(args.layout/'labels.tsv');label_by_id={int(r['occurrence']):r for r in labels}
    require(all(r.display_label==int(label_by_id[r.occurrence_id]['label']) for r in records),
            'layout display label differs')
    view_bytes,views=matched_views(args.layout/'views.bgview')
    outcome=[r for r in rows(args.outcomes) if r['config']==args.config and r['view']=='both']
    require(len(outcome)==1,'one selected actual reference outcome required')
    outcome=outcome[0];original_wall=float(outcome['complete_seconds'])
    require((outcome['scope'],outcome['status']) in
            (('fresh_step_both_labeled_views','PASS'),('failed_qualification','FAIL'),
             ('fresh_source_nominal_request_both_views','COMPLETE_UNQUALIFIED')) and
            math.isfinite(original_wall) and original_wall>0 and
            outcome['source_sha256']==source.source_sha256 and
            outcome['views_sha256']==digest(args.reference/'views.bgview'),
            'selected outcome does not describe complete reference images')
    paths=[('reference_scene',args.reference/'scene.bgrefmesh'),('reference_source',args.reference/'source/source.bgocct'),
           ('reference_metadata',args.reference/'metadata.tsv'),
           ('reference_outcomes',args.outcomes),('native_model_scene',match.scene_path),
           ('native_source',args.model/'source.bgsource'),('layout_scene',args.layout/'diagram.bgscene'),
           ('layout_views',args.layout/'views.bgview'),('layout_manifest',args.layout/'explosion.tsv'),
           ('labels',args.layout/'labels.tsv'),('reference_display',args.display),('svg_exporter',args.rsvg)]
    pins=[(name,digest(path)) for name,path in paths]
    data=(args.reference/'scene.bgrefmesh').read_bytes()
    changed=translated_reference_scene(data,offsets,expected);reference_inventory(data,source,reports)
    boxes=stored_geometry(match.scene_path.read_bytes())
    args.output.mkdir(parents=True)
    for name in ('labels.tsv','explosion.tsv','views.bgview'):shutil.copyfile(args.layout/name,args.output/name)
    (args.output/'diagram.bgrefmesh').write_bytes(changed)
    shutil.copyfile(args.outcomes,args.output/'original-reference-outcomes.tsv')
    shutil.copyfile(args.reference/'metadata.tsv',args.output/'original-reference-metadata.tsv')
    command=[str(args.display),'--mesh',str(args.output/'diagram.bgrefmesh'),'--views',str(args.output/'views.bgview'),
             '--threads','12','--output-directory',str(args.output/'native')]
    (args.output/'command.txt').write_text(repr(tuple(command))+'\n')
    stages=[('source_inventory_pose_layout_admission_and_copy',time.perf_counter()-started)]
    begin=time.perf_counter()
    with (args.output/'display.log').open('wb') as f:
        process=subprocess.run(['/usr/bin/time','-f','exit=%x wall=%e user=%U system=%S peak_kib=%M',
            '-o',str(args.output/'display-process.tsv'),*command],stdout=f,stderr=subprocess.STDOUT)
    display=time.perf_counter()-begin;stages.append(('both_view_reference_display_process',display))
    require(process.returncode==0,'reference display failed; see display.log')
    native=rows(args.output/'native/timings.tsv')
    raster_names={'raw_instance_transform_project','triangle_setup','tile_count_scan_allocate_fill',
                  'visibility_shading_resolve','diagnostic_host_copy'}
    raster=[]
    for i in (0,1):
        word=[r for r in native if int(r['frame'])==i and r['stage'] in raster_names]
        require(len(word)==5 and {r['stage'] for r in word}==raster_names,'reference raster stages incomplete')
        raster.append(math.fsum(float(r['milliseconds'])/1000 for r in word))
    begin=time.perf_counter()
    for i,name in enumerate(('assembled','exploded')):
        write_png(args.output/(name+'-raw.png'),views[i].width,views[i].height,
                  (args.output/f'native/frame-{i:06d}.rgba').read_bytes())
    stages.append(('encode_actual_reference_pngs',time.perf_counter()-begin));begin=time.perf_counter()
    mask=args.output/'background-mask.png'
    counts=pixel_inventory(args.output/'native/frame-000001.instance',views[1].width,views[1].height,offsets,mask)
    annotation=make_annotation(records,labels,boxes,views[1],args.output/'native/frame-000001.instance',counts)
    visibility=[(r.display_label,r.occurrence_id,label_by_id[r.occurrence_id]['product_name'],r.module,
                 counts.get(r.occurrence_id,0),label_by_id[r.occurrence_id]['product_name'] in CRITICAL_NAMES) for r in records]
    tsv(args.output/'visibility.tsv',('display_label','occurrence_id','original_product_name','module','actual_visible_pixels','major_part'),visibility)
    tsv(args.output/'guides.tsv',('occurrence_id','source_x','source_y','exploded_x','exploded_y','role'),annotation[1])
    stages.append(('actual_reference_visibility_mask_and_guides',time.perf_counter()-begin));begin=time.perf_counter()
    caption=(f'NVIDIA CPU reference mesh · Original source → both images: {original_wall:.2f}s · '
             f'heal{heal} · chord {chord:g}mm · curve/surface {curve_angle:g}°/{surface_angle:g}° · '
             f'{outcome["status"]} · no remeshing')
    height=poster(args.output/'exploded-assembly.svg',args.output/'exploded-raw.png',mask,views[1],records,labels,
        annotation,'NVIDIA CPU MESH / CPU12 DISPLAY',{'raster':raster[1],'display':display,'reference_caption':caption},reference=True)
    stages.append(('compose_reference_svg_and_complete_legend',time.perf_counter()-begin));begin=time.perf_counter()
    result=subprocess.run([str(args.rsvg),'--output',str(args.output/'exploded-assembly.png'),str(args.output/'exploded-assembly.svg')],capture_output=True)
    require(result.returncode==0,'reference SVG export failed');stages.append(('rasterize_reference_poster',time.perf_counter()-begin))
    require([(name,digest(path)) for name,path in paths]==pins,'reference inputs/tools changed')
    tsv(args.output/'pins.tsv',('component','sha256'),[*pins,('output_reference_scene',digest(args.output/'diagram.bgrefmesh'))])
    tsv(args.output/'metadata.tsv',('key','value'),(('backend','NVIDIA CPU reference mesh / BRepGPU CPU12 display'),
        ('source_sha256',source.source_sha256),('original_definitions',107),('original_faces',9162),('occurrences',166),
        ('native_width',views[1].width),('native_height',views[1].height),('supersample',views[1].supersample),
        ('poster_width',views[1].width),('poster_height',height),('reference_outcome_scope',outcome['scope']),
        ('heal',heal),('chord_height_mm',chord),('curve_angle_degrees',curve_angle),
        ('surface_angle_degrees',surface_angle),('independent_distance_qualification',metadata['independent_distance_qualification']),
        ('reference_outcome_status',outcome['status']),('original_source_to_both_images_seconds',original_wall),
        ('new_exploded_raster_and_host_copy_seconds',raster[1]),('new_both_view_display_process_seconds',display),
        ('actual_visible_occurrences',sum(counts.get(r.occurrence_id,0)>0 for r in records))))
    stages.append(('complete_reference_diagram_replay_export',time.perf_counter()-started))
    tsv(args.output/'timings.tsv',('stage','seconds'),stages)
    missing=[name for _,_,name,_,count,major in visibility if major and count==0]
    require(not missing,'major reference parts not visible: '+','.join(missing))
    (args.output/'COMPLETE').write_text('Genuine NVIDIA mesh diagram replay; geometry qualification remains separately reported.\n')
    print('NVIDIA CPU reference COMPLETE','visible',sum(counts.get(r.occurrence_id,0)>0 for r in records),
          'display',display,'exploded raster+hostcopy',raster[1],flush=True)


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('reference','model','layout','outcomes','output','display','rsvg'):parser.add_argument('--'+name,type=Path,required=True)
    parser.add_argument('--config',required=True);parser.add_argument('--query',type=int,default=0)
    try:main(parser.parse_args())
    except (ValueError,OSError,KeyError,StopIteration,struct.error) as error:
        print('NVIDIA exploded reference FAILED:',error,file=sys.stderr);raise SystemExit(1)
