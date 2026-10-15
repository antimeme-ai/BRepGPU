#!/usr/bin/env python3
"""Actual exact region/curve/topology readers and serial CPU caller costs."""
import argparse
import csv
from fractions import Fraction as F
import hashlib
from html import escape
from pathlib import Path
import platform
import statistics
import sys
import time

from brepgpu.body import Frame, make_body
from brepgpu.intersections import FaceRef
from brepgpu.arrangements import arrange_face
from brepgpu.arrangement_oracles import qualify_arrangement, qualify_locations

ROOT = Path(__file__).resolve().parents[1]
SOURCES = ('python/brepgpu/__init__.py', 'python/brepgpu/body.py',
           'python/brepgpu/intersections.py', 'python/brepgpu/classification.py',
           'python/brepgpu/intersection_oracles.py', 'python/brepgpu/body_oracles.py',
           'python/brepgpu/arrangements.py', 'python/brepgpu/arrangement_oracles.py',
           'tests/test_arrangement_entry.py', 'tests/test_arrangements.py',
           'examples/arrange_face.py')
RECIPES = ('bare', 'single', 'mixed', 'many-holes', 'grid32')


def tsv(path, header, rows):
    with path.open('w', newline='') as stream:
        writer = csv.writer(stream, delimiter='\t', lineterminator='\n')
        writer.writerow(header)
        writer.writerows(rows)


def ratio(value):
    return str(value.numerator)+'/'+str(value.denominator)


def value_text(value):
    if isinstance(value, F): return ratio(value)
    if value is None: return ''
    return str(value)


def prepare(recipe):
    started = time.perf_counter()
    scale = 1. if recipe in ('bare','single') else 4.
    body = make_body('box', Frame((0.,0.,-.5*scale), scale))
    face = FaceRef(body, 1)
    if recipe == 'bare': origins = ()
    elif recipe == 'single': origins = ((0.,0.),)
    elif recipe == 'mixed': origins = ((-2.,-1.5),(-1.,-1.5),(1.5,1.5))
    else:
        origins = tuple((float(-3+6*F(8*i+1,64)), float(-3+6*F(8*j+1,64)))
                        for j in range(8) for i in range(8))
    wall_scale = .25 if recipe in ('many-holes','grid32') else 1.
    walls = tuple(make_body('cylinder', Frame((x,y,0.), wall_scale)) for x,y in origins)
    prepared_bodies = time.perf_counter()
    sections = tuple(face.snapshot.intersect_faces(face.face_id, wall, 3) for wall in walls)
    section_end = time.perf_counter()
    cuts = (F(1,2),) if recipe == 'mixed' else tuple(F(i,32) for i in range(1,32)) if recipe == 'grid32' else ()
    return face, sections, cuts, cuts, (
        1000*(prepared_bodies-started), 1000*(section_end-prepared_bodies),
        1000*(time.perf_counter()-started))


def weight(value):
    if isinstance(value, F): return value.numerator.bit_length()+value.denominator.bit_length()
    if hasattr(value, 'a') and hasattr(value, 'b'):
        return weight(value.a)+weight(value.b)+value.d.bit_length()
    return int(value)


def graph_reader(arr):
    """Follow every actual next/twin and read all region/source metadata."""
    checksum = 0
    visited = 0
    for loop in arr.loops:
        cursor = loop.uses[0]
        for _ in loop.uses:
            use = arr.uses[cursor]
            checksum += sum((use.id,use.edge,use.orientation,use.region,use.twin,use.next,use.previous,use.loop))
            checksum += arr.uses[use.twin].region
            cursor = use.next
            visited += 1
        if cursor != loop.uses[0]:
            raise AssertionError('actual loop reader did not close')
        checksum += loop.id+loop.region+int(loop.hole)
    for vertex in arr.vertices:
        checksum += vertex.id+sum(weight(v) for v in vertex.uv+vertex.position)
        checksum += len(str(vertex.source))
    for edge in arr.edges:
        checksum += edge.id+edge.start+edge.end+len(edge.kind)+len(str(edge.source))
        checksum += sum(edge.piece_ids)+sum(edge.event_ids)
    for region in arr.regions:
        checksum += region.id+len(region.kind)+sum(region.holes)
        checksum += 0 if region.outer_loop is None else region.outer_loop
        checksum += sum(region.cell) if region.cell is not None else 0
        checksum += region.section if region.section is not None else 0
        checksum += sum(weight(v) for v in region.witness_uv)
        if region.area is not None: checksum += weight(region.area.rational)+weight(region.area.pi)
    return visited, checksum


def geometry_reader(arr):
    checksum = queries = 0
    body = arr.face.snapshot
    coedges = {coedge.id:coedge for coedge in body.coedges}
    edges = {edge.id:edge for edge in body.edges}
    origin, scale = tuple(F(x) for x in body.frame.origin), F(body.frame.scale)
    for edge in arr.edges:
        for t in ((F(0),F(1,2),F(1)) if edge.kind == 'line' else tuple(F(i,8) for i in range(9))):
            uv, position = arr.edge_uv(edge.id,t), arr.edge_point(edge.id,t)
            checksum += sum(weight(x) for x in uv+position)
            if edge.source[0] == 'boundary':
                _, coedge_id, edge_id, t0, t1 = edge.source
                parameter = (1-t)*t0+t*t1
                original_uv = tuple(F(x) for x in coedges[coedge_id].uv_map.data)
                curve = body.curves[edges[edge_id].curve_slot]
                original_xyz = tuple(F(x) for x in curve.data)
                # Source records use the original canonical curve parameter.
                # Their coedge traversal sign must not reverse it again.
                source_uv = tuple((1-parameter)*original_uv[a]+parameter*original_uv[a+2]
                                  for a in range(2))
                source_world = tuple(origin[a]+scale*((1-parameter)*original_xyz[a]
                                      +parameter*original_xyz[a+3]) for a in range(3))
                if uv != source_uv or position != source_world:
                    raise AssertionError('actual boundary reader lost original directed parameter correspondence')
                checksum += sum(weight(x) for x in source_uv+source_world)
            queries += 1
        if edge.kind == 'circle':
            section = arr.sections[edge.source[1]]
            for pid in edge.piece_ids:
                piece = section.pieces[pid]
                checksum += piece.id+piece.span_id+piece.left_slot+piece.right_slot
                for t in (piece.lo,(piece.lo+piece.hi)/2,piece.hi):
                    checksum += sum(weight(x) for x in piece.position(t)+piece.left_uv(t)+piece.right_uv(t))
    return queries, checksum


def location_reader(arr):
    queries = tuple(region.witness_uv for region in arr.regions)+tuple(v.uv for v in arr.vertices)+((F(-1),F(-1)),(F(1,2),F(0)),(F(1,2),F(1,2)))
    outputs = tuple(arr.locate_uv(*point) for point in queries)
    digest = (len(queries),sum(len(row) for row in outputs),sum(sum(row) for row in outputs))
    return queries, outputs, digest


def source_tables(directory, arr):
    target = arr.face
    tsv(directory/'face.tsv',('body_id','revision','face_id','chart_id','orientation','origin_x','origin_y','origin_z','scale'),
        ((target.snapshot.body_id,target.snapshot.revision,target.face_id,
          target.snapshot.charts[target.face.chart_slots[0]].id,target.face.orientation,
          *(ratio(F(x)) for x in target.snapshot.frame.origin),ratio(F(target.snapshot.frame.scale))),))
    tsv(directory/'cuts.tsv',('axis','index','coordinate'),
        ((axis,i,ratio(x)) for axis,values in (('u',arr.u_cuts),('v',arr.v_cuts)) for i,x in enumerate(values)))
    tsv(directory/'section-sources.tsv',('section','side','body_id','revision','face_id','shape','origin_x','origin_y','origin_z','scale'),
        ((i,side,ref.snapshot.body_id,ref.snapshot.revision,ref.face_id,ref.snapshot.shape,
          *(ratio(F(x)) for x in ref.snapshot.frame.origin),ratio(F(ref.snapshot.frame.scale)))
         for i,s in enumerate(arr.sections) for side,ref in (('left',s.left),('right',s.right))))
    tsv(directory/'source-components.tsv',('section','component','kind','closed','tangent','pieces'),
        ((i,c.id,c.kind,int(c.closed),int(c.tangent),','.join(map(str,c.piece_ids)))
         for i,s in enumerate(arr.sections) for c in s.components))
    tsv(directory/'source-pieces.tsv',('section','piece','component','span','kind','left_slot','right_slot','lo','hi','boundary_constraints'),
        ((i,p.id,p.component_id,p.span_id,p.kind,p.left_slot,p.right_slot,ratio(p.lo.a),ratio(p.hi.a),repr(p.boundary_constraints))
         for i,s in enumerate(arr.sections) for p in s.pieces))
    tsv(directory/'source-coefficients.tsv',('section','piece','map','row','term','a','b','d'),
        ((i,p.id,name,row,term,ratio(value.a),ratio(value.b),value.d)
         for i,s in enumerate(arr.sections) for p in s.pieces
         for name,mapping in (('spatial',p.spatial_map),('left_uv',p.left_map),('right_uv',p.right_map))
         for row,values in ((-1,mapping.denominator),*enumerate(mapping.numerators))
         for term,value in enumerate(values)))
    tsv(directory/'source-events.tsv',('section','event','constraints','joins','incidences'),
        ((i,e.id,repr(e.constraints),repr(e.joins),repr(e.incidences))
         for i,s in enumerate(arr.sections) for e in s.events))
    references = ((-1,'target',target),)+tuple((i,side,ref) for i,s in enumerate(arr.sections)
                    for side,ref in (('left',s.left),('right',s.right)))
    tsv(directory/'source-charts.tsv',('section','side','body_id','revision','face_id','face_orientation',
        'chart_slot','chart_id','quadrant','degree_u','degree_v','u0','u1','v0','v1',
        'origin_x','origin_y','origin_z','scale'),
        ((i,side,ref.snapshot.body_id,ref.snapshot.revision,ref.face_id,ref.face.orientation,
          slot,chart.id,chart.quadrant,chart.patch.degree_u,chart.patch.degree_v,
          *(ratio(F(x)) for x in chart.patch.uv+chart.patch.origin),ratio(F(chart.patch.scale)))
         for i,side,ref in references for slot in ref.face.chart_slots
         for chart in (ref.snapshot.charts[slot],)))
    tsv(directory/'source-controls.tsv',('section','side','chart_slot','control','ax','ay','az','w'),
        ((i,side,slot,control//4,*(ratio(F(x)) for x in patch.controls[control:control+4]))
         for i,side,ref in references for slot in ref.face.chart_slots
         for patch in (ref.snapshot.charts[slot].patch,)
         for control in range(0,len(patch.controls),4)))
    body = target.snapshot
    tsv(directory/'original-boundary.tsv',('coedge_id','edge_id','orientation','uv_map_kind','u0','v0','u1','v1','x0','y0','z0','x1','y1','z1'),
        ((c.id,body.edges[c.edge_slot].id,c.orientation,c.uv_map.kind,
          *(ratio(F(x)) for x in c.uv_map.data),
          *(ratio(F(x)) for x in body.curves[body.edges[c.edge_slot].curve_slot].data))
         for c in body.coedges if c.face_slot == target.face_id-1))


def export(directory, arr, queries, outputs):
    directory.mkdir()
    source_tables(directory,arr)
    tsv(directory/'vertices.tsv',('id','u','v','x','y','z','source_kind','source_a','source_b'),
        ((v.id,*(ratio(x) for x in v.uv+v.position),*v.source) for v in arr.vertices))
    tsv(directory/'edges.tsv',('id','kind','start','end','source_kind','source_a','source_b','source_t0','source_t1','pieces','events'),
        ((e.id,e.kind,e.start,e.end,*(tuple(value_text(x) for x in e.source)+('',)*(5-len(e.source))),
          ','.join(map(str,e.piece_ids)),','.join(map(str,e.event_ids))) for e in arr.edges))
    tsv(directory/'uses.tsv',('id','edge','orientation','region','twin','next','previous','loop'),
        ((u.id,u.edge,u.orientation,u.region,u.twin,u.next,u.previous,u.loop) for u in arr.uses))
    tsv(directory/'loops.tsv',('id','region','hole','uses'),
        ((l.id,l.region,int(l.hole),','.join(map(str,l.uses))) for l in arr.loops))
    tsv(directory/'regions.tsv',('id','kind','cell_u','cell_v','section','outer_loop','holes','witness_u','witness_v','area_rational','area_pi'),
        ((r.id,r.kind,*(r.cell if r.cell is not None else ('','')),value_text(r.section),value_text(r.outer_loop),
          ','.join(map(str,r.holes)),*(ratio(x) for x in r.witness_uv),
          ratio(r.area.rational) if r.area is not None else '',ratio(r.area.pi) if r.area is not None else '') for r in arr.regions))
    tsv(directory/'locations.tsv',('query','u','v','regions'),
        ((i,ratio(q[0]),ratio(q[1]),','.join(map(str,row))) for i,(q,row) in enumerate(zip(queries,outputs))))


def svg(path, outputs):
    out=['<svg xmlns="http://www.w3.org/2000/svg" width="1200" height="900" viewBox="0 0 1200 900">',
         '<rect width="1200" height="900" rx="18" fill="#101923"/>',
         '<g font-family="monospace" fill="#d7e5ef">',
         '<text x="30" y="38" font-size="24">Exact planar arrangements: regions, holes, shared uses</text>',
         '<text x="30" y="66" font-size="13">One physical circle edge retains four source pieces and both UV maps. Geometry drawn from actual readers.</text>']
    for panel,(name,arr) in enumerate(outputs):
        x=40+(panel%3)*390
        y=110+(panel//3)*390
        size=300
        def point(uv): return x+size*float(uv[0]), y+size*(1-float(uv[1]))
        def loop_path(loop_id):
            result=[]
            for uid in arr.loops[loop_id].uses:
                use=arr.uses[uid]
                edge=arr.edges[use.edge]
                ts=(F(0),F(1)) if edge.kind=='line' else tuple(F(i,64) for i in range(65))
                if use.orientation<0: ts=tuple(reversed(ts))
                for t in ts:
                    px,py=point(arr.edge_uv(edge.id,t))
                    result.append(('M' if not result else 'L')+f'{px:.3f},{py:.3f}')
            return ' '.join(result)+' Z'
        out.append(f'<text x="{x}" y="{y-19}" font-size="18">{escape(name)}</text>')
        for r in arr.regions[1:]:
            paths=loop_path(r.outer_loop)+' '+' '.join(loop_path(h) for h in r.holes)
            fill='#274459' if r.kind=='disk' else ('#17383c' if sum(r.cell)%2 else '#1d303e')
            out.append(f'<path d="{paths}" fill="{fill}" fill-rule="evenodd"/>')
        for e in arr.edges:
            ts=(F(0),F(1)) if e.kind=='line' else tuple(F(i,64) for i in range(65))
            pixels=tuple(point(arr.edge_uv(e.id,t)) for t in ts)
            coords=' '.join(f'{px:.3f},{py:.3f}' for px,py in pixels)
            color='#ffd166' if e.kind=='circle' else '#37d1b0'
            out.append(f'<polyline points="{coords}" fill="none" stroke="{color}" stroke-width="{1.5 if e.kind=="circle" else .7}"/>')
        if name in ('single','mixed'):
            for v in arr.vertices:
                px,py=point(v.uv)
                out.append(f'<circle cx="{px:.3f}" cy="{py:.3f}" r="3" fill="#d7e5ef"/>')
            for r in arr.regions[1:]:
                px,py=point(r.witness_uv)
                out.append(f'<text x="{px+3:.3f}" y="{py-3:.3f}" font-size="11">r{r.id}</text>')
        cells=sum(r.kind=='cell' for r in arr.regions)
        disks=sum(r.kind=='disk' for r in arr.regions)
        out.append(f'<text x="{x}" y="{y+size+24}" font-size="12">{cells} cell regions + {disks} disk regions</text>')
        out.append(f'<text x="{x}" y="{y+size+43}" font-size="12">{len(arr.edges)} edges / {len(arr.uses)} opposed uses</text>')
    out.extend(('<text x="820" y="625" font-size="16">Exact topology first</text>',
                '<text x="820" y="658" font-size="13">CCW outer / CW holes</text>',
                '<text x="820" y="682" font-size="13">Shared events and twins</text>',
                '<text x="820" y="706" font-size="13">Exterior region retained</text>',
                '<text x="820" y="730" font-size="13">CPU construction and readers</text>',
                '<text x="820" y="770" font-size="11">Display samples are not a mesh bound.</text>',
                '</g></svg>'))
    path.write_text('\n'.join(out)+'\n')


def main(args):
    if sys.platform!='linux': raise RuntimeError('product callers and benchmarks run on the fleet')
    args.output.mkdir(parents=True,exist_ok=False)
    pins=tuple((name,hashlib.sha256((ROOT/name).read_bytes()).hexdigest()) for name in SOURCES)
    tsv(args.output/'sources.tsv',('path','sha256'),pins)
    tsv(args.output/'environment.tsv',('key','value'),
        (('placement','exact CPU single process'),('python',platform.python_version()),('system',platform.system()),
         ('release',platform.release()),('architecture',platform.machine()),
         ('cpu',next((line.split(':',1)[1].strip() for line in Path('/proc/cpuinfo').read_text().splitlines() if line.startswith('model name')),'unknown')),
         ('warmup',1),('repeats',5),('qualification','after complete caller; measured separately')))
    raw=[]; prep=[]; counts=[]; diagram=[]
    for name in RECIPES:
        face,sections,ucuts,vcuts,phases=prepare(name)
        prep.append((name,*phases))
        expected=None
        for phase,sample in (('first',0),('warmup',0),*(('repeat',i) for i in range(5))):
            start=time.perf_counter()
            arr=arrange_face(face,sections,u_cuts=ucuts,v_cuts=vcuts)
            built=time.perf_counter()
            gd=graph_reader(arr)
            graph_end=time.perf_counter()
            geometry=geometry_reader(arr)
            geometry_end=time.perf_counter()
            queries,locations,ld=location_reader(arr)
            end=time.perf_counter()
            check=time.perf_counter()
            if arr.face is not face or len(arr.sections)!=len(sections) or any(a is not b for a,b in zip(arr.sections,sections)):
                raise AssertionError('arrangement lost actual submitted source identity')
            if arr.u_cuts!=tuple(sorted(ucuts)) or arr.v_cuts!=tuple(sorted(vcuts)):
                raise AssertionError('arrangement lost exact submitted cuts')
            qualify_arrangement(arr)
            qualify_locations(arr,queries,locations)
            checked=time.perf_counter()
            digest=gd+geometry+ld
            if expected is None: expected=digest
            elif expected!=digest: raise AssertionError('actual retained-input reader output changed')
            raw.append((name,phase,sample,1000*(built-start),1000*(graph_end-built),
                        1000*(geometry_end-graph_end),1000*(end-geometry_end),1000*(end-start),
                        1000*(checked-check),','.join(map(str,digest)),'PASS'))
        counts.append((name,len(arr.vertices),len(arr.edges),len(arr.uses),len(arr.loops),len(arr.regions),
                       len(arr.sections),len(queries),geometry[0]))
        # Full graphs/source maps are retained even for the larger deterministic recipes.
        export(args.output/name,arr,queries,locations)
        diagram.append((name,arr))
        print(name,'qualified 7 arrangements',len(arr.regions)-1,'bounded regions',flush=True)
    tsv(args.output/'preparation.tsv',('case','body_frame_ms','exact_sections_ms','prepare_complete_ms'),prep)
    tsv(args.output/'counts.tsv',('case','vertices','edges','uses','loops','regions','circles','location_queries','edge_reader_queries'),counts)
    tsv(args.output/'raw.tsv',('case','phase','sample','construct_ms','graph_reader_ms','geometry_reader_ms',
                            'location_reader_ms','complete_ms','qualification_ms','reader_digest','result'),raw)
    summaries=[]
    for name in RECIPES:
        for index,scope in ((3,'construct'),(4,'graph_reader'),(5,'geometry_reader'),(6,'location_reader'),(7,'complete'),(8,'qualification')):
            values=[r[index] for r in raw if r[0]==name and r[1]=='repeat']
            summaries.append((name,scope,statistics.median(values),min(values),max(values)))
    tsv(args.output/'summary.tsv',('case','scope','median_ms','min_ms','max_ms'),summaries)
    svg(args.output/'arrangements.svg',diagram)
    if pins!=tuple((name,hashlib.sha256((ROOT/name).read_bytes()).hexdigest()) for name in SOURCES):
        raise AssertionError('measured source changed during serial campaign')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    main(parser.parse_args())
