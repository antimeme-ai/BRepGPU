#!/usr/bin/env python3
"""Replay the completed OM10 as a source-linked exploded assembly diagram.

Fleet only. All166 original instances and rotations remain. Only authored
presentation translations and cameras change; this is not disassembly planning.
"""
import argparse
from array import array
import base64
from collections import Counter
import csv
from dataclasses import replace
import hashlib
import html
import math
from pathlib import Path
import shutil
import subprocess
import sys
import time

ROOT=Path(__file__).resolve().parents[1]
sys.path[:0]=[str(ROOT/'python'),str(ROOT/'examples')]
from brepgpu.nvidia_reference import load_native_model_match
from om10_explosion import (MODULE_NAMES,make_layout,translated_scene,diagram_views,
                            require,scene_header)
from om10_scene import pack_views,project_point,visible_anchors,write_png

CRITICAL_NAMES = {
    'Platine brute', 'Pont de rouage brut', 'Pont de barillet brut',
    'Tambour de barillet manuel', 'Ressort de barillet manuel', 'Couvercle de barillet',
    'Planche de grande moyenne', 'Planche de moyenne', 'Planche de seconde',
    "Planche d'échappement 20.5", 'Platine de mécanisme de tige brute',
    'Pont de mécanisme de tige', 'Couvre mécanisme de tige (manuel)',
    "Corps d'ancre", "Pont d'ancre brut", 'Coq brut', 'Serge de balancier',
    'Spiral', 'Disque de quantième brut', 'Plaque de quantième brute',
}


def rows(path):
    with Path(path).open(newline='',encoding='utf-8') as f:
        return tuple(csv.DictReader(f,delimiter='\t'))


def tsv(path,fields,word):
    with Path(path).open('w',newline='',encoding='utf-8') as f:
        writer=csv.writer(f,delimiter='\t',lineterminator='\n')
        writer.writerow(fields);writer.writerows(word)


def digest(path):
    value=hashlib.sha256()
    with Path(path).open('rb') as f:
        while chunk:=f.read(1<<20):value.update(chunk)
    return value.hexdigest()


def embedded(path):
    return 'data:image/png;base64,'+base64.b64encode(Path(path).read_bytes()).decode('ascii')


def pixel_inventory(path,width,height,identifiers,mask_path):
    ids=array('Q');ids.frombytes(Path(path).read_bytes())
    if sys.byteorder!='little':ids.byteswap()
    require(len(ids)==width*height,'actual ID buffer dimensions differ')
    count=Counter(ids);background=(1<<64)-1
    require(set(count)-set(identifiers)-{background}==set(),'unknown actual source occurrence pixel')
    # Mask guides onto real background only. Geometry pixels keep their native
    # RGB; source-ID visibility, not a cosmetic center, drives visible claims.
    mask=bytearray(width*height*4)
    for i,identifier in enumerate(ids):
        c=255 if identifier==background else 0
        at=4*i;mask[at:at+4]=bytes((c,c,c,255))
    write_png(mask_path,width,height,mask)
    return count


def world_center(points):
    return tuple((min(p[k] for p in points)+max(p[k] for p in points))/2 for k in range(3))


def make_annotation(records,labels,boxes,view,id_path,counts):
    visible=visible_anchors(id_path,view.width,view.height,step=2,
                            valid_ids=tuple(r.occurrence_id for r in records))
    by_id={r.occurrence_id:r for r in records}
    label_by_id={int(r['occurrence']):r for r in labels}
    modules=[];guides=[];parts=[]
    for module,(letter,title) in MODULE_NAMES.items():
        selected=[r for r in records if r.module==module]
        points=[tuple(p[k]+r.offset[k] for k in range(3)) for r in selected for p in boxes[r.occurrence_id]]
        projected=[project_point(view,p) for p in points]
        x0,x1=min(p[0] for p in projected),max(p[0] for p in projected)
        y0,y1=min(p[1] for p in projected),max(p[1] for p in projected)
        shown=[r for r in selected if counts.get(r.occurrence_id,0)>0]
        modules.append((letter,title,module,len(selected),len(shown),x0,y0,x1,y1))
        # One source-placement/translation guide per significant role. These
        # show authored alignment, not inferred contacts or feasible paths.
        roles={}
        for r in selected:
            name=label_by_id[r.occurrence_id]['product_name']
            if name in CRITICAL_NAMES or ('mobile' in r.role and 'pinion' not in name.casefold()):
                roles.setdefault(r.role,r)
        for r in roles.values():
            origin=world_center(boxes[r.occurrence_id])
            target=tuple(origin[k]+r.offset[k] for k in range(3))
            p,q=project_point(view,origin),project_point(view,target)
            if math.hypot(q[0]-p[0],q[1]-p[1])>8:
                guides.append((r.occurrence_id,*p[:2],*q[:2],r.role))
    for label in labels:
        identifier=int(label['occurrence']);name=label['product_name']
        if name not in CRITICAL_NAMES:continue
        anchor=visible.get(identifier)
        parts.append((identifier,int(label['label']),name,by_id[identifier].module,
                      counts.get(identifier,0),anchor))
    return modules,guides,parts,visible


def poster(path,raw,mask,view,records,labels,annotations,backend,costs):
    esc=html.escape;modules,guides,parts,_=annotations
    w=view.width;factor=w/3840.;image_y=240*factor
    # Complete part names live outside the native raster. No panel covers CAD.
    legend_y=image_y+view.height+220*factor
    ordered=[]
    for module,(letter,title) in MODULE_NAMES.items():
        ordered.append(('module',letter+' · '+title,None))
        ordered.extend(('part',f"{r.display_label:03d}  "+next(x['product_name'] for x in labels if int(x['occurrence'])==r.occurrence_id),r)
                       for r in records if r.module==module)
    lines_per_column=math.ceil(len(ordered)/4)
    height=int(legend_y+(lines_per_column*29+135)*factor)
    gray='#506773';ink='#102c3b';accent='#087d83'
    a=[f'<svg xmlns="http://www.w3.org/2000/svg" width="{w}" height="{height}" viewBox="0 0 {w} {height}">',
       '<title>OM10 — source-linked exploded assembly illustration</title>',
       '<desc>All166 original occurrences; source geometry, rotations and materials retained. Authored presentation translations and source-linked module grouping. Alignment guides are explanatory, not validated disassembly or fastening instructions. All original names retained in legend and labels.tsv.</desc>',
       '<metadata xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#" xmlns:dc="http://purl.org/dc/elements/1.1/" xmlns:cc="http://creativecommons.org/ns#"><rdf:RDF><cc:Work rdf:about=""><dc:source rdf:resource="https://openmovement.org/project/om10/"/><dc:creator>Association openmovement (CAD); BRepGPU (mesh and authored illustration)</dc:creator><cc:license rdf:resource="https://creativecommons.org/licenses/by-sa/3.0/"/></cc:Work></rdf:RDF></metadata>',
       f'<rect width="{w}" height="{height}" fill="#edf2f4"/>',
       f'<image y="{image_y}" width="{w}" height="{view.height}" href="{embedded(raw)}"/>',
       f'<defs><mask id="background" maskUnits="userSpaceOnUse" x="0" y="{image_y}" width="{w}" height="{view.height}"><image y="{image_y}" width="{w}" height="{view.height}" href="{embedded(mask)}"/></mask></defs>',
       f'<g font-family="DejaVu Sans,Arial,sans-serif" fill="{ink}">',
       f'<rect width="{w}" height="{240*factor}" fill="#112d3b"/>',
       f'<text x="{85*factor}" y="{62*factor}" font-size="{21*factor}" letter-spacing="{6*factor}" fill="#77d8d0">BRepGPU / ASSOCIATION OPENMOVEMENT</text>',
       f'<text x="{80*factor}" y="{145*factor}" font-size="{74*factor}" font-weight="700" fill="#f1f7fa">OM10 · EXPLODED ASSEMBLY</text>',
       f'<text x="{86*factor}" y="{199*factor}" font-size="{25*factor}" fill="#c8dbe2">107 definitions · 9,162 original faces · 166 occurrences · original orientations</text>',
       f'<text x="{w-85*factor}" y="{64*factor}" text-anchor="end" font-size="{29*factor}" fill="#77d8d0">{esc(backend)}</text>',
       f'<g mask="url(#background)" stroke="{accent}" fill="none" stroke-width="{1.7*factor}" opacity=".35" stroke-dasharray="{8*factor} {8*factor}">']
    for _,x,y,xx,yy,_ in guides:
        a.append(f'<path d="M {x:.2f} {y+image_y:.2f} L {xx:.2f} {yy+image_y:.2f}"/>')
    a.append('</g>')
    # Module headings occupy gaps above their actual projected bounds.
    for letter,title,module,total,shown,x0,y0,x1,y1 in modules:
        x=max(70*factor,min(w-650*factor,x0-(180*factor if module=='train' else 0)))
        y=max(80*factor,y0-(65*factor if module=='train' else 50*factor))+image_y
        a.extend((f'<path d="M {x:.1f} {y+13*factor:.1f} h {210*factor}" stroke="{accent}" stroke-width="{2*factor}"/>',
                  f'<text x="{x:.1f}" y="{y:.1f}" font-size="{28*factor}" font-weight="700" fill="{accent}">{letter} · {esc(title)}</text>',
                  f'<text x="{x:.1f}" y="{y+43*factor:.1f}" font-size="{19*factor}" fill="{gray}">{total} original occurrences · {shown} with visible pixels</text>'))
    # Significant source parts receive compact numeric visible-pixel markers;
    # full names are in the grouped legend rather than166 crossing leaders.
    for identifier,label,name,module,count,anchor in parts:
        if anchor is None:continue
        x,y,_=anchor;y+=image_y
        a.extend((f'<circle cx="{x}" cy="{y}" r="{14*factor}" fill="#f7fbfc" fill-opacity=".9" stroke="{accent}" stroke-width="{1.3*factor}"/>',
                  f'<text x="{x+20*factor}" y="{y-15*factor}" font-size="{23*factor}" fill="{ink}" font-weight="700">{label:03d}<title>{esc(name)} · {count} actual visible pixels</title></text>'))
    panel_y=image_y+view.height
    a.extend((f'<rect x="0" y="{panel_y}" width="{w}" height="{185*factor}" fill="#dce7eb"/>',
       f'<text x="{85*factor}" y="{panel_y+46*factor}" font-size="{24*factor}" fill="{accent}" font-weight="700">AUTHORED MODULE EXPLOSION · SAME COMPLETED MESH</text>',
       f'<text x="{85*factor}" y="{panel_y+88*factor}" font-size="{22*factor}">This exploded view raster + download: {costs["raster"]:.3f}s · Both-view replay process: {costs["display"]:.3f}s</text>',
       f'<text x="{85*factor}" y="{panel_y+126*factor}" font-size="{21*factor}" fill="{gray}">Original CPU12 cold source → mesh → 1920 both views: {costs["cold"]:.2f}s · Meshing: {costs["mesh"]:.2f}s · no remeshing in this replay</text>',
       f'<text x="{85*factor}" y="{panel_y+164*factor}" font-size="{19*factor}" fill="{gray}">Guides show authored source alignment; not validated removal paths. Full native stages and PNG/SVG/export costs accompany this image.</text>'))
    column=(w-160*factor)/4
    for i,(kind,text,record) in enumerate(ordered):
        col,line=divmod(i,lines_per_column);x=80*factor+col*column;y=legend_y+(line*29+32)*factor
        a.append(f'<text x="{x}" y="{y}" font-size="{19*factor if kind=="part" else 20*factor}" fill="{gray if kind=="part" else accent}" font-weight="{"400" if kind=="part" else "700"}">{esc(text)}</text>')
    a.extend((f'<text x="{85*factor}" y="{height-52*factor}" font-size="{19*factor}" fill="{gray}">Association openmovement · OM10 MAN VA01.3 · CC BY-SA 3.0 · Original CAD / BRepGPU mesh, display and authored layout</text>',
              f'<text x="{85*factor}" y="{height-22*factor}" font-size="{18*factor}" fill="{gray}">All166 occurrences retained; small attached source jewels/posts may remain occluded by their own parent. Full source usage identity in explosion.tsv.</text>',
              '</g></svg>'))
    Path(path).write_text('\n'.join(a)+'\n',encoding='utf-8')
    return height


def main(args):
    require(sys.platform=='linux','geometry/display execution belongs on the fleet')
    require(not args.output.exists(),'new output directory required')
    started=time.perf_counter();match=load_native_model_match(args.model,args.query,.005)
    labels=rows(match.labels_path);data=match.scene_path.read_bytes()
    require(len(match.source.definitions)==107 and len(match.source.faces)==9162,'complete original watch required')
    records,boxes,axes=make_layout(match,labels,data);offsets={r.occurrence_id:r.offset for r in records}
    views=diagram_views(boxes,offsets,axes)
    if args.preview:
        views=tuple(replace(v,width=v.width//2,height=v.height//2,supersample=1,
                            pixel_center=tuple(x/2 for x in v.pixel_center),pixels_per_unit=v.pixels_per_unit/2)
                    for v in views)
    pins=[(name,digest(path)) for name,path in [('input_scene',match.scene_path),('input_labels',match.labels_path),('native_display',args.native),('display_module',args.plugin),('svg_exporter',args.rsvg)]]
    args.output.mkdir(parents=True);shutil.copyfile(match.labels_path,args.output/'labels.tsv')
    shutil.copyfile(args.model/'timings.tsv',args.output/'original-native-timings.tsv')
    shutil.copyfile(args.process_report,args.output/'original-process.tsv')
    (args.output/'diagram.bgscene').write_bytes(translated_scene(data,offsets))
    (args.output/'views.bgview').write_bytes(pack_views(views))
    tsv(args.output/'explosion.tsv',('occurrence_id','display_label','source_usage_path','product_id','definition_id','source_group','module','role','world_offset_x_mm','world_offset_y_mm','world_offset_z_mm','reason'),
        ((r.occurrence_id,r.display_label,r.source_usage_path,r.product_id,r.definition_id,r.source_group,r.module,r.role,*r.offset,r.reason) for r in records))
    command=[str(args.native),'--scene',str(args.output/'diagram.bgscene'),'--views',str(args.output/'views.bgview'),'--output-directory',str(args.output/'native'),'--backend',args.backend,'--threads','12','--device','1']
    if args.backend=='cuda':command+=['--plugin',str(args.plugin)]
    (args.output/'command.txt').write_text(repr(tuple(command))+'\n')
    stages=[('source_scene_admission_layout_and_copy',time.perf_counter()-started)]
    begin=time.perf_counter()
    with (args.output/'display.log').open('wb') as f:
        process=subprocess.run(['/usr/bin/time','-f','exit=%x wall=%e user=%U system=%S peak_kib=%M','-o',str(args.output/'display-process.tsv'),*command],stdout=f,stderr=subprocess.STDOUT)
    display=time.perf_counter()-begin;stages.append(('both_view_display_process',display));require(process.returncode==0,'native replay failed; see display.log')
    native=rows(args.output/'native/timings.tsv')
    raster_names={'instance_transform_project','triangle_setup','tile_count_scan_allocate_fill','visibility_shading_resolve','result_download'}
    raster=[math.fsum(float(r['milliseconds'])/1000 for r in native if int(r['frame'])==i and r['stage'] in raster_names) for i in (0,1)]
    for i in (0,1):require({r['stage'] for r in native if int(r['frame'])==i and r['stage'] in raster_names}==raster_names,'incomplete native raster stages')
    begin=time.perf_counter()
    for i in (0,1):
        rgba=(args.output/f'native/frame-{i:06d}.rgba').read_bytes()
        write_png(args.output/('assembled-raw.png' if i==0 else 'exploded-raw.png'),views[i].width,views[i].height,rgba)
    stages.append(('encode_actual_native_pngs',time.perf_counter()-begin))
    begin=time.perf_counter();mask=args.output/'background-mask.png';counts=pixel_inventory(args.output/'native/frame-000001.instance',views[1].width,views[1].height,offsets,mask)
    annotation=make_annotation(records,labels,boxes,views[1],args.output/'native/frame-000001.instance',counts)
    visibility=[]
    for row,r in zip(labels,records):
        visibility.append((r.display_label,r.occurrence_id,row['product_name'],r.module,counts.get(r.occurrence_id,0),row['product_name'] in CRITICAL_NAMES))
    tsv(args.output/'visibility.tsv',('display_label','occurrence_id','original_product_name','module','actual_visible_pixels','major_part'),visibility)
    tsv(args.output/'guides.tsv',('occurrence_id','source_x','source_y','exploded_x','exploded_y','role'),annotation[1])
    stages.append(('actual_visibility_mask_and_guides',time.perf_counter()-begin))
    original_fields=dict(item.split('=',1) for item in args.process_report.read_text().splitlines()[-1].split())
    old={r['scope']:float(r['seconds']) for r in rows(args.model/'timings.tsv')}
    costs={'raster':raster[1],'display':display,'cold':float(original_fields['wall']),'mesh':old[f'query{args.query}.complete_meshing']}
    backend='CPU12' if args.backend=='cpu' else 'P1001 DISPLAY / CPU12 HOST'
    begin=time.perf_counter();height=poster(args.output/'exploded-assembly.svg',args.output/'exploded-raw.png',mask,views[1],records,labels,annotation,backend,costs)
    stages.append(('compose_exploded_svg_and_complete_legend',time.perf_counter()-begin))
    begin=time.perf_counter()
    with (args.output/'svg-export.log').open('wb') as f:p=subprocess.run([str(args.rsvg),'--output',str(args.output/'exploded-assembly.png'),str(args.output/'exploded-assembly.svg')],stdout=f,stderr=subprocess.STDOUT)
    require(p.returncode==0,'SVG rasterization failed');stages.append(('rasterize_svg_poster',time.perf_counter()-begin))
    missing=[name for _,_,name,_,count,major in visibility if major and count==0]
    tsv(args.output/'metadata.tsv',('key','value'),(('backend',backend),('source_sha256',match.source.source_sha256),('original_definitions',107),('original_faces',9162),('occurrences',166),('native_width',views[1].width),('native_height',views[1].height),('supersample',views[1].supersample),('poster_width',views[1].width),('poster_height',height),('layout','authored semantic modules; translations only; no validated disassembly'),('major_parts_without_visible_pixels',','.join(missing)),('actual_visible_occurrences',sum(counts.get(r.occurrence_id,0)>0 for r in records)),('new_exploded_raster_and_download_seconds',raster[1]),('new_both_view_display_process_seconds',display),('original_cold_process_seconds',original_fields['wall'])))
    current=[(name,digest(path)) for (name,_),path in zip(pins,(match.scene_path,match.labels_path,args.native,args.plugin,args.rsvg))]
    require(current==pins,'immutable inputs or display tools changed')
    tsv(args.output/'pins.tsv',('component','sha256'),[*pins,('diagram_scene',digest(args.output/'diagram.bgscene')),('views',digest(args.output/'views.bgview')),('explosion_manifest',digest(args.output/'explosion.tsv'))])
    stages.append(('complete_new_diagram_replay_export',time.perf_counter()-started));tsv(args.output/'timings.tsv',('stage','seconds'),stages)
    if missing and not args.preview:raise ValueError('major parts have no visible pixels: '+','.join(missing))
    (args.output/'COMPLETE').write_text('Preview with visibility findings.\n' if args.preview else 'Completed authored exploded assembly illustration; no new meshing.\n')
    print(backend,'PREVIEW' if args.preview else 'COMPLETE','major hidden',missing,'visible occurrences',sum(counts.get(r.occurrence_id,0)>0 for r in records),'display',display,'raster',raster[1],flush=True)

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('model','process-report','output','native','plugin','rsvg'):parser.add_argument('--'+name,type=Path,required=True)
    parser.add_argument('--query',type=int,default=0);parser.add_argument('--backend',choices=('cpu','cuda'),default='cpu');parser.add_argument('--preview',action='store_true')
    try:main(parser.parse_args())
    except (ValueError,OSError,KeyError,StopIteration) as error:
        print('exploded assembly FAILED:',error,file=sys.stderr);raise SystemExit(1)
