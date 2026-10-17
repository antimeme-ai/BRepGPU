#!/usr/bin/env python3
"""Replay a completed native watch scene into a 4K hero and labeled poster.

Fleet only. No meshing, source edits, camera refitting or bitmap enlargement.
The original complete process and this additional display/export have separate
reported costs. Requires the existing source-profile1 native display and rsvg.
"""
import argparse
import base64
import csv
from dataclasses import replace
import hashlib
import html
import math
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[1]
sys.path[:0] = [str(ROOT/'python'), str(ROOT/'examples')]
from brepgpu.nvidia_reference import load_native_model_match
from om10_scene import pack_views, project_point, visible_anchors, write_png
from render_om10_nvidia import matched_views


def require(condition, message):
    if not condition:
        raise ValueError(message)


def digest(path):
    result = hashlib.sha256()
    with Path(path).open('rb') as stream:
        while block := stream.read(1 << 20):
            result.update(block)
    return result.hexdigest()


def rows(path):
    with Path(path).open(newline='', encoding='utf-8') as stream:
        return tuple(csv.DictReader(stream, delimiter='\t'))


def tsv(path, fields, values):
    with Path(path).open('w', newline='', encoding='utf-8') as stream:
        writer = csv.writer(stream, delimiter='\t')
        writer.writerow(fields)
        writer.writerows(values)


def process_fields(path):
    fields = dict(item.split('=', 1) for item in Path(path).read_text().splitlines()[-1].split())
    require(fields['exit'] == '0', 'original complete process failed')
    require(math.isfinite(float(fields['wall'])) and float(fields['wall']) > 0,
            'original complete process wall invalid')
    return fields


def occurrence_centers(scene):
    """Projected fallback uses only the actual stored-mesh bounds and instances."""
    data = Path(scene).read_bytes()
    h = struct.unpack_from('<8sII8Q', data)
    require(h[:3] == (b'BGMESH01', 1, 1), 'source profile1 scene required')
    nd, ni, nv = h[3:6]
    definitions = tuple(struct.unpack_from('<5Q', data, 80+40*i) for i in range(nd))
    start = 80+40*nd+136*ni
    centers = []
    for _, first, count, _, _ in definitions:
        require(count > 0 and first+count <= nv, 'stored vertex range invalid')
        lower = [math.inf]*3
        upper = [-math.inf]*3
        for i in range(first, first+count):
            point = struct.unpack_from('<3d', data, start+56*i)
            for k in range(3):
                lower[k] = min(lower[k], point[k])
                upper[k] = max(upper[k], point[k])
        centers.append(tuple((a+b)/2 for a,b in zip(lower,upper)))
    result = {}
    for i in range(ni):
        row = struct.unpack_from('<QII15d', data, 80+40*nd+136*i)
        require(row[0] not in result and row[1] < nd, 'instance identity invalid')
        point = centers[row[1]]
        m = row[3:15]
        result[row[0]] = (tuple(math.fsum(m[4*r+k]*point[k] for k in range(3))+m[4*r+3]
                               for r in range(3)), row[15:18])
    return result


def callouts(labels, centers, view, id_path):
    visible = visible_anchors(id_path, view.width, view.height, step=3,
                              valid_ids=tuple(centers))
    anchors = []
    for row in labels:
        identifier = int(row['occurrence'])
        if identifier in visible:
            x, y, _ = visible[identifier]
            kind = 'visible ID sample'
        else:
            point, offset = centers[identifier]
            point = tuple(point[k]+view.alpha*offset[k] for k in range(3))
            x, y, _ = project_point(view, point)
            kind = 'projected stored-mesh center (no sampled visible anchor)'
        require(math.isfinite(x) and math.isfinite(y), 'callout anchor invalid')
        anchors.append((row, x, y, kind))
    anchors.sort(key=lambda item:(item[1], int(item[0]['label'])))
    halves = (anchors[:83], anchors[83:])
    result = []
    for side, half in enumerate(halves):
        half.sort(key=lambda item:(item[2], int(item[0]['label'])))
        for i,(row,x,y,kind) in enumerate(half):
            result.append((row, x, y, kind, side, 345+i*34))
    require(len(result) == 166 and {int(r[0]['label']) for r in result} == set(range(1,167)),
            'callout original label inventory differs')
    return result


def svg_gallery(path, png, view, name, backend, labels, original, display_seconds, original_times,
                mesh_counts, original_source_seconds, native_seconds, raster_seconds):
    esc = html.escape
    w, h = view.width, view.height+600
    image_y = 240
    accent = '#71e2d4' if backend == 'cpu12' else '#94bfff'
    encoded = base64.b64encode(Path(png).read_bytes()).decode('ascii')
    items = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{w}" height="{h}" viewBox="0 0 {w} {h}">',
        f'<title>OM10 — {name} — BRepGPU {backend.upper()}</title>',
        '<desc>Completed original OM10 source assembly. BRepGPU cached adaptive meshing; additional 4K replay of the actual stored mesh and unchanged camera. Illustrative source-linked presentation. Poster * marks a projected stored-mesh center where no visible ID sample was found; not a visibility claim.</desc>',
        '<metadata xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#" xmlns:dc="http://purl.org/dc/elements/1.1/" xmlns:cc="http://creativecommons.org/ns#"><rdf:RDF><cc:Work rdf:about=""><dc:source rdf:resource="https://openmovement.org/project/om10/"/><dc:creator>Association openmovement (CAD); BRepGPU (meshing and visualization)</dc:creator><cc:license rdf:resource="https://creativecommons.org/licenses/by-sa/3.0/"/></cc:Work></rdf:RDF></metadata>',
        f'<rect width="{w}" height="{h}" fill="#111b28"/>',
        f'<image y="{image_y}" width="{w}" height="{view.height}" href="data:image/png;base64,{encoded}"/>',
        '<defs><linearGradient id="header" x2="0" y2="1"><stop stop-color="#111d2b" stop-opacity=".98"/><stop offset="1" stop-color="#111d2b" stop-opacity=".78"/></linearGradient></defs>',
        '<g font-family="DejaVu Sans,Arial,sans-serif" fill="#edf4fa">',
        f'<rect width="{w}" height="240" fill="url(#header)"/>',
        f'<text x="108" y="72" font-size="23" letter-spacing="7" fill="{accent}">BRepGPU  /  OPENMOVEMENT</text>',
        f'<text x="102" y="163" font-size="82" font-weight="700" letter-spacing="-2">OM10 · {esc(name)}</text>',
        f'<text x="108" y="216" font-size="27" fill="#b2c3d5">107 original definitions · 9,162 faces · 166 placed parts · {mesh_counts:,} triangles</text>',
        f'<text x="{w-108}" y="93" text-anchor="end" font-size="43" font-weight="700" fill="{accent}">{esc(backend.upper())}</text>',
        f'<text x="{w-108}" y="137" text-anchor="end" font-size="23" fill="#b2c3d5">3840 × 3600 · 2× supersampling</text>',
        f'<text x="{w-108}" y="177" text-anchor="end" font-size="23" fill="#b2c3d5">0.005 mm positional · 0.001 mm source-node request</text>']
    if labels:
        for row,ax,ay,kind,side,y in labels:
            ay += image_y
            y += image_y
            left = side == 0
            text_x = 62 if left else w-62
            line_x = 515 if left else w-515
            elbow = 550 if left else w-550
            text_anchor = 'start' if left else 'end'
            fallback = kind != 'visible ID sample'
            mark = '*' if fallback else ''
            title = f"PART {int(row['label']):03d}{mark} · {row['product_name']}"
            short = row['product_name']
            if len(short) > 30:
                short = short[:29]+'…'
            label = f"{int(row['label']):03d}{mark}  {short}"
            style = ' stroke-dasharray="5 7"' if fallback else ''
            items.extend((f'<path d="M {ax:.2f} {ay:.2f} L {elbow} {y-8} L {line_x} {y-8}" stroke="{accent}" stroke-width="1.1" opacity=".22" fill="none"{style}/>',
                f'<circle cx="{ax:.2f}" cy="{ay:.2f}" r="3" fill="{accent}" opacity=".75"/>',
                f'<text x="{text_x}" y="{y}" font-size="21" text-anchor="{text_anchor}" stroke="#111b28" stroke-width="5" paint-order="stroke"><title>{esc(title)} · {esc(kind)}</title>{esc(label)}</text>'))
    panel_y = h-330
    items.extend((f'<rect x="72" y="{panel_y}" width="{w-144}" height="250" rx="20" fill="#0b1521" fill-opacity=".94" stroke="#304052"/>',
        f'<text x="108" y="{panel_y+47}" font-size="21" fill="{accent}" letter-spacing="3">ACTUAL COSTS · ONE OBSERVATION · NESTED STAGES ARE NOT ADDITIVE</text>'))
    columns = [
        ('ORIGINAL COMPLETE RUN · 1920 BOTH VIEWS', [
            ('Cold source → meshing → both images', f"{float(original['wall']):.2f} s"),
            ('Source decode + envelopes + banks', f'{original_source_seconds:.3f} s'),
            ('Complete cached adaptive meshing', f"{original_times['query0.complete_meshing']:.3f} s")]),
        ('ADDITIONAL 4K DISPLAY · SAME SCENE', [
            ('THIS view raster, including download', f'{raster_seconds:.3f} s'),
            ('Both-view process, including file output', f'{display_seconds:.3f} s'),
            ('Native session, including explicit close', f'{native_seconds:.3f} s')]),
        ('SOURCE & PRESENTATION', [
            ('Original 107 / 9,162 / 166 inventory', 'Complete'),
            ('Camera, placement and explosion', 'Unchanged'),
            ('4K PNG / SVG / raster export costs', 'timings.tsv')])]
    for col,(heading,entries) in enumerate(columns):
        x = 108+col*1210
        items.append(f'<text x="{x}" y="{panel_y+94}" font-size="20" font-weight="700" fill="#cad8e6">{esc(heading)}</text>')
        for j,(label,value) in enumerate(entries):
            y = panel_y+143+39*j
            items.append(f'<text x="{x}" y="{y}" font-size="22" fill="#91a7bc">{esc(label)}</text>')
            items.append(f'<text x="{x+1090}" y="{y}" text-anchor="end" font-size="24" font-weight="600">{esc(value)}</text>')
    if labels:
        items.append(f'<text x="108" y="{h-64}" font-size="18" fill="#91a7bc">All 166 PART IDs retained · * projected mesh-center anchor if no visible ID sample · complete original names in labels.tsv</text>')
    items.extend((f'<text x="108" y="{h-30}" font-size="18" fill="#91a7bc">Association openmovement · OM10 MAN VA01.3 · CC BY-SA 3.0 · CAD unchanged · BRepGPU meshing, display and labels</text>', '</g></svg>\n'))
    Path(path).write_text('\n'.join(items), encoding='utf-8')


def main(args):
    require(sys.platform == 'linux', 'gallery rendering belongs on the fleet')
    require(not args.output.exists(), 'output already exists')
    require(args.threads == 12 and args.device == 1, 'this matched export uses CPU12 / P100 device1')
    started = time.perf_counter()
    match = load_native_model_match(args.model, args.query, .005)
    require(len(match.source.definitions) == 107 and len(match.source.faces) == 9162 and
            len(match.assembly.occurrences) == 166, 'complete original watch inventory required')
    _, saved_views = matched_views(match.views_path)
    require(all((v.width,v.height,v.supersample) == (1920,1800,1) for v in saved_views),
            'expected exact saved 1920×1800 camera')
    views = tuple(replace(v,width=3840,height=3600,supersample=2,
                          pixel_center=tuple(2*x for x in v.pixel_center),
                          pixels_per_unit=2*v.pixels_per_unit) for v in saved_views)
    labels = rows(match.labels_path)
    require(len(labels) == 166 and [int(r['label']) for r in labels] == list(range(1,167)),
            'complete original label word required')
    original = process_fields(args.process_report)
    original_times = {r['scope']:float(r['seconds']) for r in rows(args.model/'timings.tsv')}
    original_source_seconds = sum(original_times[s] for s in
        ('source.decode','source.envelopes_and_initial_assembly','source.compile_banks'))
    label = 'cpu12' if args.backend == 'cpu' else 'p1001'
    pins = tuple((name,digest(path)) for name,path in (
        ('scene',match.scene_path),('saved_views',match.views_path),('labels',match.labels_path),
        ('native_display',args.native),('display_module',args.plugin),('rsvg_convert',args.rsvg)))
    args.output.mkdir(parents=True)
    shutil.copyfile(match.labels_path,args.output/'labels.tsv')
    shutil.copyfile(args.model/'timings.tsv',args.output/'original-native-timings.tsv')
    shutil.copyfile(args.model/'queries.tsv',args.output/'original-queries.tsv')
    shutil.copyfile(args.process_report,args.output/'original-process.tsv')
    (args.output/'views.bgview').write_bytes(pack_views(views))
    command = [str(args.native),'--scene',str(match.scene_path),'--views',str(args.output/'views.bgview'),
               '--output-directory',str(args.output/'native'),'--backend',args.backend,
               '--threads',str(args.threads),'--device',str(args.device)]
    if args.backend == 'cuda':
        command.extend(('--plugin',str(args.plugin)))
    (args.output/'command.txt').write_text(repr(tuple(command))+'\n')
    stages = [('admit_complete_scene_and_prepare',time.perf_counter()-started)]
    start = time.perf_counter()
    timed = ['/usr/bin/time','-f','exit=%x wall=%e user=%U system=%S peak_kib=%M',
             '-o',str(args.output/'display-process.tsv'),*command]
    with (args.output/'display.log').open('wb') as log:
        completed = subprocess.run(timed,stdout=log,stderr=subprocess.STDOUT)
    display_seconds = time.perf_counter()-start
    stages.append(('additional_4k_display_process_both_views',display_seconds))
    require(completed.returncode == 0,'4K source-profile1 display failed; see display.log')
    native_rows = rows(args.output/'native/timings.tsv')
    native_seconds = next(float(r['milliseconds'])/1000 for r in native_rows
                          if r['stage'] == 'complete_native_wall')
    raster_stages = {'instance_transform_project','triangle_setup','tile_count_scan_allocate_fill',
                     'visibility_shading_resolve','result_download'}
    raster_seconds = tuple(math.fsum(float(r['milliseconds'])/1000 for r in native_rows
                                    if int(r['frame']) == i and r['stage'] in raster_stages)
                           for i in range(2))
    require(all({r['stage'] for r in native_rows if int(r['frame']) == i and
                 r['stage'] in raster_stages} == raster_stages for i in range(2)),
            'native per-frame disjoint raster stages incomplete')
    centers = occurrence_centers(match.scene_path)
    mesh_counts = sum(int(r['facets']) for r in rows(args.model/'queries.tsv') if int(r['query']) == args.query)
    images = []
    callout_rows = []
    for i,(name,view) in enumerate(zip(('assembled-hero','exploded-poster'),views)):
        start = time.perf_counter()
        rgba = (args.output/f'native/frame-{i:06d}.rgba').read_bytes()
        raw_png = args.output/f'{name}-raw.png'
        write_png(raw_png,view.width,view.height,rgba)
        stages.append((name+'.png_encode',time.perf_counter()-start))
        start = time.perf_counter()
        points = callouts(labels,centers,view,args.output/f'native/frame-{i:06d}.instance') if i else ()
        for row,x,y,kind,side,ly in points:
            callout_rows.append((row['label'],row['occurrence'],row['definition'],row['product_name'],
                                 kind,x,y,side,ly))
        stages.append((name+'.callout_layout',time.perf_counter()-start))
        start = time.perf_counter()
        svg = args.output/f'{name}.svg'
        svg_gallery(svg,raw_png,view,name.replace('-',' ').upper(),label,points,original,
                    display_seconds,original_times,mesh_counts,original_source_seconds,native_seconds,
                    raster_seconds[i])
        stages.append((name+'.svg_compose',time.perf_counter()-start))
        start = time.perf_counter()
        png = args.output/f'{name}.png'
        with (args.output/f'{name}-rsvg.log').open('wb') as log:
            converted = subprocess.run((str(args.rsvg),'--output',str(png),str(svg)),
                                       stdout=log,stderr=subprocess.STDOUT)
        stages.append((name+'.svg_rasterize',time.perf_counter()-start))
        require(converted.returncode == 0,'SVG rasterization failed')
        header = png.read_bytes()[:24]
        require(header[:8] == b'\x89PNG\r\n\x1a\n' and struct.unpack_from('>II',header,16) == (3840,4200),
                'shareable output dimensions differ')
        images.append((name,png.name,digest(png),svg.name,digest(svg),3840,4200))
    require(tuple((name,digest(path)) for (name,_),path in zip(pins,
        (match.scene_path,match.views_path,match.labels_path,args.native,args.plugin,args.rsvg))) == pins,
        'retained scene/camera/labels or display tools changed')
    tsv(args.output/'pins.tsv',('component','sha256'),pins)
    tsv(args.output/'callouts.tsv',('label','original_occurrence','definition','original_product_name',
        'anchor_kind','anchor_x','anchor_y','side','label_y'),callout_rows)
    stages.append(('complete_gallery_replay_and_export',time.perf_counter()-started))
    tsv(args.output/'timings.tsv',('stage','seconds','scope'),
        ((name,seconds,'additional replay/export wall; nested, not additive') for name,seconds in stages))
    tsv(args.output/'images.tsv',('view','png','png_sha256','svg','svg_sha256','width','height'),images)
    tsv(args.output/'metadata.tsv',('key','value'),(
        ('backend',label),('source_profile',1),('definitions',107),('original_faces',9162),('placements',166),
        ('unique_facets',mesh_counts),('native_width',3840),('native_height',3600),('supersample',2),
        ('poster_width',3840),('poster_height',4200),('native_image_y_in_poster',240),
        ('internal_width',7680),('internal_height',7200),('camera','saved camera scaled exactly2; unchanged basis/center/alpha'),
        ('geometry','completed immutable scene; no remesh or bitmap enlargement'),
        ('original_complete_1920_process_seconds',original['wall']),
        ('additional_4k_display_process_seconds',display_seconds),
        ('assembled_4k_raster_including_download_seconds',raster_seconds[0]),
        ('exploded_4k_raster_including_download_seconds',raster_seconds[1]),
        ('callouts','all166 original IDs; visible ID sample or explicitly marked projected stored-mesh center'),
        ('timings','original-native-timings.tsv + native/timings.tsv + timings.tsv; nested stages not additive'),
        ('source_attribution','Association openmovement OM10 MAN VA01.3; CC BY-SA3.0; BRepGPU visualization')))
    (args.output/'COMPLETE').write_text('Completed native scene replay, not a new meshing/qualification run.\n')
    print(f'{label} gallery COMPLETE; {display_seconds:.3f}s additional 4K display, both views; all166 poster labels')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('model','process-report','output','native','plugin','rsvg'):
        parser.add_argument('--'+name,type=Path,required=True)
    parser.add_argument('--backend',choices=('cpu','cuda'),required=True)
    parser.add_argument('--query',type=int,default=0)
    parser.add_argument('--threads',type=int,default=12)
    parser.add_argument('--device',type=int,default=1)
    try:
        main(parser.parse_args())
    except (ValueError,OSError,StopIteration) as error:
        print(f'gallery FAILED: {error}',file=sys.stderr)
        raise SystemExit(1)
