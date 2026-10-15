#!/usr/bin/env python3
"""Suspended lattice artwork from one fully qualified exact planar arrangement.

Every height is a rendering displacement. This caller creates no fused solid,
extrusion, new intersection kind or certified trimmed mesh.
"""
import argparse
import csv
from fractions import Fraction as F
import hashlib
from html import escape
from pathlib import Path
import platform
import sys

from brepgpu.body import Frame, make_body
from brepgpu.intersections import FaceRef
from brepgpu.arrangements import arrange_face
from brepgpu.arrangement_oracles import qualify_arrangement, qualify_locations
from arrange_face import export, tsv, ratio, SOURCES

ROOT = Path(__file__).resolve().parents[1]
PINNED = SOURCES + ('examples/trophy_arrangement.py',)
N = 16
CELL_HEIGHTS = (F(0),F(0),F(0),F(1,3),F(2,3),F(1),F(2,3),F(1,3))


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def raw_map(mapping, parameter):
    """Read original rational coefficients, without its product evaluator."""
    def polynomial(values):
        answer = F(0)
        for value in reversed(values):
            require(not value.b and type(value.a) is F, 'nonrational source circle map')
            answer = answer*parameter + value.a
        return answer
    denominator = polynomial(mapping.denominator)
    require(denominator > 0, 'nonpositive original circle denominator')
    return tuple(polynomial(row)/denominator for row in mapping.numerators)


def prepare():
    body = make_body('box', Frame((0.,0.,-2.),4.))
    face = FaceRef(body,1)
    walls, recipes = [], []
    for j in range(N):
        for i in range(N):
            if (i+j)%2:
                continue
            x = F(-3)+F(3,8)*(F(i)+F(1,2))+F(1 if j%2==0 else -1,32)
            y = F(-3)+F(3,8)*(F(j)+F(1,2))+F(1 if i%2==0 else -1,32)
            scale = F(1,2) if (i//2+j//2)%2==0 else F(1,4)
            wall = make_body('cylinder',Frame((float(x),float(y),0.),float(scale)))
            require(tuple(F(v) for v in wall.frame.origin)==(x,y,F(0)), 'recipe frame rounded')
            require(F(wall.frame.scale)==scale, 'recipe scale rounded')
            walls.append(wall)
            recipes.append((i,j,x,y,scale,scale/4))
    sections = tuple(body.intersect_faces(1,wall,3) for wall in walls)
    cuts = tuple(F(i,N) for i in range(1,N))
    return face,sections,cuts,tuple(recipes)


def geometry_samples(arr):
    """Consume actual readers and bind every displayed sample to original maps."""
    coedges = {c.id:c for c in arr.face.snapshot.coedges}
    curves = {e.id:arr.face.snapshot.curves[e.curve_slot] for e in arr.face.snapshot.edges}
    body = arr.face.snapshot
    origin,scale = tuple(F(v) for v in body.frame.origin),F(body.frame.scale)
    samples = {}
    for edge in arr.edges:
        parameters = (F(0),F(1)) if edge.kind=='line' else tuple(F(i,64) for i in range(65))
        rows = []
        for t in parameters:
            uv,xyz = arr.edge_uv(edge.id,t),arr.edge_point(edge.id,t)
            # The original target top face is affine, with these literal source controls.
            target_xyz = (F(-3)+6*uv[0],F(-3)+6*uv[1],F(0))
            require(xyz==target_xyz, 'displayed reader left original target geometry')
            if edge.kind=='circle':
                section = arr.sections[edge.source[1]]
                q,local = (3,F(1)) if t==1 else (int(4*t),4*t-int(4*t))
                piece = section.pieces[edge.piece_ids[q]]
                require(uv==raw_map(piece.left_map,local) and xyz==raw_map(piece.spatial_map,local),
                        'displayed circle lost its original piece parameter/map')
            elif edge.source[0]=='boundary':
                _,cid,eid,t0,t1 = edge.source
                p = (1-t)*t0+t*t1
                source_uv = tuple(F(v) for v in coedges[cid].uv_map.data)
                source_xyz = tuple(F(v) for v in curves[eid].data)
                require(uv==tuple((1-p)*source_uv[a]+p*source_uv[a+2] for a in range(2)),
                        'displayed boundary lost directed original UV parameter')
                require(xyz==tuple(origin[a]+scale*((1-p)*source_xyz[a]+p*source_xyz[a+3]) for a in range(3)),
                        'displayed boundary lost original spatial edge parameter')
            rows.append((t,uv,xyz))
        samples[edge.id] = tuple(rows)
    return samples


def region_heights(arr):
    heights = {0:F(0)}
    for r in arr.regions[1:]:
        i,j = r.cell
        heights[r.id] = (F(2)+F((3*i+j)%5,8) if r.kind=='disk'
                         else CELL_HEIGHTS[(i+2*j)%len(CELL_HEIGHTS)])
    return heights


def queries_for(arr, recipes):
    queries = [r.witness_uv for r in arr.regions]+[v.uv for v in arr.vertices]
    for i,j,x,y,scale,radius in recipes:
        queries.extend(((x+dx+3)/6,(y+dy+3)/6) for dx,dy in ((radius,0),(0,radius),(-radius,0),(0,-radius)))
    queries.extend(((F(-1),F(-1)),(F(2),F(2)),(F(0),F(1,2)),(F(1),F(1,2)),
                    (F(1,2),F(0)),(F(1,2),F(1)),(F(1,2),F(1,2))))
    return tuple(queries)


def render(path, arr, samples, heights):
    """Paths follow actual loop words. Dotted lines denote actual twin identity."""
    def project(position, height):
        x,y,z = position
        return (900+100*float(x-y),1000+46*float(x+y)-190*float(z+height))
    def pixel(point):return f'{point[0]:.3f},{point[1]:.3f}'
    def loop_path(lid,height):
        loop = arr.loops[lid]
        cursor = loop.uses[0]
        commands = []
        for expected in loop.uses:
            require(cursor==expected, 'drawing next reader disagrees with loop word')
            use = arr.uses[cursor]
            points = tuple(row[2] for row in samples[use.edge])
            if use.orientation<0:points=points[::-1]
            for point in points:
                commands.append(('M' if not commands else 'L')+pixel(project(point,height)))
            cursor = use.next
        require(cursor==loop.uses[0], 'drawing loop did not close')
        return ' '.join(commands)+' Z'
    out = ['<svg xmlns="http://www.w3.org/2000/svg" width="1800" height="1600" viewBox="0 0 1800 1600">',
           '<title>Suspended lattice — exact planar arrangement sculpture</title>',
           '<desc>256 original cell regions and 128 original disk regions from one fully qualified planar arrangement. All vertical offsets are rendering displacements. Dotted links pair opposed uses of the same original graph edge; they are not modeled joins.</desc>',
           '<defs><radialGradient id="bg"><stop stop-color="#142333"/><stop offset="1" stop-color="#070c14"/></radialGradient></defs>',
           '<rect width="1800" height="1600" fill="url(#bg)"/>',
           '<g font-family="monospace" fill="#e6eef3">',
           '<text x="74" y="86" font-size="18" letter-spacing="5" fill="#8da6b8">BRepGPU / GEOMETRY STUDIES 02</text>',
           '<text x="70" y="150" font-size="53">SUSPENDED LATTICE</text>',
           '<text x="74" y="192" font-size="17" fill="#9bb0bf">One exact plane. 128 circles. Every region, hole and opposed use retained.</text>',
           '</g>']
    # Actual same-edge correspondence across differently displaced owning regions.
    links = []
    for use in arr.uses:
        if use.id>use.twin:continue
        twin = arr.uses[use.twin]
        if not use.region or not twin.region or heights[use.region]==heights[twin.region]:continue
        edge = arr.edges[use.edge]
        if edge.kind=='line' and edge.id%17!=0:continue
        if edge.kind=='circle' and edge.source[1]%8!=0:continue
        position = arr.edge_point(edge.id,F(1,2))
        a,b = project(position,heights[use.region]),project(position,heights[twin.region])
        links.append((edge.id,use.id,twin.id,a,b))
    out.append('<g data-role="twin-correspondence" fill="none" stroke="#88a6b5" stroke-width=".65" stroke-dasharray="2 6" opacity=".45">')
    for eid,uid,tid,a,b in links:
        out.append(f'<path data-edge="{eid}" data-use="{uid}" data-twin="{tid}" d="M{pixel(a)} L{pixel(b)}"/>')
    out.append('</g>')
    # Painter ordering is a declared display choice, independent of region identities.
    ordered = sorted(arr.regions[1:],key=lambda r:(heights[r.id],project(arr.face.snapshot.frame.origin,heights[r.id])[1],sum(r.witness_uv),r.id))
    cell_colors = {F(0):'#17495c',F(1,3):'#246d7c',F(2,3):'#4a949a',F(1):'#9abcb4'}
    disk_colors = ('#f5d894','#edba68','#d8974c','#e8c282')
    for r in ordered:
        h = heights[r.id]
        words = [loop_path(r.outer_loop,h)]+[loop_path(lid,h) for lid in r.holes]
        fill = disk_colors[r.section%len(disk_colors)] if r.kind=='disk' else cell_colors[h]
        stroke = '#fbe9bd' if r.kind=='disk' else '#91c1c9'
        title = escape(f'Original region {r.id}: {r.kind}; cell {r.cell}; exact UV area {r.area.rational} + ({r.area.pi})π; display offset {h}')
        out.append(f'<path data-role="region" data-region="{r.id}" data-kind="{r.kind}" data-height="{ratio(h)}" data-outer-loop="{r.outer_loop}" data-holes="{",".join(map(str,r.holes))}" d="{" ".join(words)}" fill="{fill}" fill-rule="evenodd" stroke="{stroke}" stroke-width=".65" stroke-linejoin="round"><title>{title}</title></path>')
    out.extend(('<g font-family="monospace" fill="#dce7ee">',
                '<text x="74" y="1370" font-size="19">256 cell regions + 128 suspended disks</text>',
                '<text x="74" y="1408" font-size="16" fill="#97afbf">417 vertices · 672 edges · 1,344 opposed uses · 385 regions including exterior</text>',
                '<text x="74" y="1453" font-size="15" fill="#97afbf">Closed circle edges preserve 512 original arc pieces, source events and both UV maps.</text>',
                '<text x="74" y="1490" font-size="15" fill="#97afbf">Dotted links pair actual twins at different display heights. They are correspondence annotations.</text>',
                '<text x="74" y="1532" font-size="15" fill="#d7b878">Heights are rendering offsets only. Planar regions; no fused solid or certified trimmed mesh.</text>',
                '</g></svg>'))
    path.write_text('\n'.join(out)+'\n')
    return len(links)


def main(args):
    if sys.platform!='linux':raise RuntimeError('product geometry callers run on the fleet')
    args.output.mkdir(parents=True,exist_ok=False)
    pins = tuple((name,hashlib.sha256((ROOT/name).read_bytes()).hexdigest()) for name in PINNED)
    face,sections,cuts,recipes = prepare()
    arr = arrange_face(face,sections,u_cuts=cuts,v_cuts=cuts)
    require(arr.face is face and len(arr.sections)==len(sections) and all(a is b for a,b in zip(arr.sections,sections)), 'lost submitted original source identity')
    require(arr.u_cuts==cuts and arr.v_cuts==cuts, 'lost submitted exact cuts')
    quality = qualify_arrangement(arr)
    expected = dict(vertices=417,edges=672,uses=1344,loops=513,regions=385,cells=256,disks=128,holes=128)
    require(quality==expected, 'literal complete source recipe counts differ')
    queries = queries_for(arr,recipes)
    outputs = tuple(arr.locate_uv(*q) for q in queries)
    location_quality = qualify_locations(arr,queries,outputs)
    samples = geometry_samples(arr)
    heights = region_heights(arr)
    export(args.output/'graph',arr,queries,outputs)
    tsv(args.output/'circle-inputs.tsv',('section','cell_u','cell_v','center_x','center_y','wall_scale','radius'),
        ((si,i,j,ratio(x),ratio(y),ratio(scale),ratio(radius)) for si,(i,j,x,y,scale,radius) in enumerate(recipes)))
    tsv(args.output/'render-heights.tsv',('region','kind','cell_u','cell_v','height_world_display_only'),
        ((r.id,r.kind,*(r.cell if r.cell is not None else ('','')),ratio(heights[r.id])) for r in arr.regions))
    tsv(args.output/'reader-samples.tsv',('edge','sample','t','u','v','x','y','z'),
        ((eid,i,ratio(t),*(ratio(v) for v in uv+xyz)) for eid,rows in samples.items() for i,(t,uv,xyz) in enumerate(rows)))
    links = render(args.output/'artwork.svg',arr,samples,heights)
    tsv(args.output/'sources.tsv',('path','sha256'),pins)
    tsv(args.output/'counts.tsv',('key','value'),tuple(quality.items())+(('location_queries',location_quality['queries']),('reader_samples',sum(map(len,samples.values()))),('twin_annotations',links)))
    tsv(args.output/'environment.tsv',('key','value'),(('purpose','artwork; no timing benchmark'),('placement','CPU exact geometry on Linux fleet'),('python',platform.python_version()),('system',platform.system()),('display','1800x1600; projection x=900+100*(world_x-world_y), y=1000+46*(world_x+world_y)-190*(world_z+display_height)')))
    require(pins==tuple((name,hashlib.sha256((ROOT/name).read_bytes()).hexdigest()) for name in PINNED), 'source changed while producing artwork')
    report = f'''# Suspended lattice: actual exact arrangement artwork

Generated on the CPU in the owned Linux fleet worktree, using existing exact
arrange_face geometry and independent full arrangement/location qualification.

The original source is one top face of a box with Frame scale4, origin(0,0,-2),
16-by-16 exact UV cells and128 strictly interior disjoint plane/wall circles.
Checkerboard input cells retain independently framed cylindrical walls of
radius1/16 or1/8. Every source body, original control, piece/map/event and all
submitted queries are exported under graph/ and circle-inputs.tsv.

Actual qualified graph:417 vertices,672 edges,1,344 opposed uses,513 loops,
385 regions including exterior,256 cells,128 disks and128 original holes.
All {len(queries)} closed queries and {sum(map(len,samples.values()))} displayed
curve samples are consumed and qualified; both original UV maps and512 original
arc pieces/events remain in the source records. Dotted annotations connect
{links} actual opposed-use pairs at their independently displaced display heights.

Every bounded region is drawn with its complete actual outer/hole use words.
Exact source-reader samples determine SVG paths. Integer/rational display offsets
and projection are saved in render-heights.tsv and environment.tsv. Colors and
painter ordering are display choices. Displaced regions do not form a new solid,
extruded surface, Boolean result or certified trimmed mesh. Finite circle display
samples are not a geometric error bound. No performance comparison is claimed.

The SVG title tooltips expose original region IDs, exact UV areas and display
heights. sources.tsv pins every consumed source, including the existing qualifier
and export helper; the independent source/end pins agree.
'''
    (args.output/'REPORT.md').write_text(report)
    print('Qualified artwork:',quality,'locations',len(queries),'samples',sum(map(len,samples.values())),'twin annotations',links,flush=True)


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    main(parser.parse_args())
