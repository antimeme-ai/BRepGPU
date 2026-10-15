#!/usr/bin/env python3
"""Consume a complete exact CPU mesh and export its shared indexed geometry."""
import argparse
from array import array
from dataclasses import dataclass
import csv
from fractions import Fraction as F
import hashlib
from html import escape
import math
from pathlib import Path
import sys
import time

from brepgpu.body import Frame, make_body
from brepgpu.meshing import tessellate
from brepgpu.meshing_oracles import qualify_mesh

ROOT = Path(__file__).resolve().parents[1]
SOURCES = ('python/brepgpu/__init__.py', 'python/brepgpu/body.py',
           'python/brepgpu/body_oracles.py', 'python/brepgpu/meshing.py',
           'python/brepgpu/meshing_oracles.py', 'tests/test_meshing.py',
           'tests/test_meshing_entry.py', 'examples/mesh_body.py',
           'benchmarks/meshing/run.py')


def tsv(path, header, rows):
    with path.open('w', newline='') as stream:
        writer = csv.writer(stream, delimiter='\t', lineterminator='\n')
        writer.writerow(header)
        writer.writerows(rows)


def ratio(value):
    value = F(value)
    return str(value.numerator)+'/'+str(value.denominator)


def source_values(source):
    values = tuple(ratio(x) if type(x) is F else x for x in source)
    return values+('',)*(3-len(values))


def source_pins():
    return tuple((name, hashlib.sha256((ROOT/name).read_bytes()).hexdigest()) for name in SOURCES)


def check_binding(mesh, body, tolerance, max_quarter_segments=256):
    if mesh.body is not body or mesh.tolerance != F(tolerance) or mesh.quarter_segments > max_quarter_segments:
        raise AssertionError('mesh lost submitted snapshot or exact tolerance')


def same_output(mesh, first):
    """Direct full-value correspondence to one independently qualified output."""
    names = ('tolerance', 'quarter_segments', 'curve_bound', 'rounding_bound', 'error_bound',
             'vertices', 'triangles', 'edge_polylines', 'boundary_uses')
    if mesh.body is not first.body or any(getattr(mesh, name) != getattr(first, name) for name in names):
        raise AssertionError('retained-input complete mesh changed between calls')


def weight(value):
    if type(value) is F:
        return value.numerator.bit_length()+value.denominator.bit_length()
    if type(value) is tuple:
        return sum(weight(x) for x in value)
    if type(value) is str:
        return sum(ord(c) for c in value)
    return int(value)


@dataclass(frozen=True)
class IndexedRead:
    xyz: array
    indices: array
    face_ids: array
    normals: array
    metrics: tuple


def read_mesh(mesh):
    """Build actual indexed rendering/export buffers and consume source records."""
    xyz, indices, face_ids, normals = array('d'), array('I'), array('I'), array('d')
    checksum = mesh.body.body_id+mesh.body.revision+mesh.quarter_segments
    checksum += weight((mesh.tolerance, mesh.curve_bound, mesh.rounding_bound, mesh.error_bound))
    for vertex in mesh.vertices:
        xyz.extend(vertex.rounded_position)
        checksum += vertex.id+weight(vertex.position)+weight(vertex.source)
    volume6 = 0.
    area = 0.
    corner_count = 0
    for triangle in mesh.triangles:
        ids = triangle.vertices
        points = tuple(tuple(xyz[3*i+a] for a in range(3)) for i in ids)
        p, q, r = points
        u, v = tuple(b-a for a,b in zip(p,q)), tuple(b-a for a,b in zip(p,r))
        cross = (u[1]*v[2]-u[2]*v[1], u[2]*v[0]-u[0]*v[2], u[0]*v[1]-u[1]*v[0])
        length = math.hypot(*cross)
        if not math.isfinite(length) or length == 0:
            raise ArithmeticError('indexed rounded triangle has unusable area')
        indices.extend(ids)
        face_ids.append(triangle.face_id)
        normals.extend(c/length for c in cross)
        volume6 += p[0]*(q[1]*r[2]-q[2]*r[1])+p[1]*(q[2]*r[0]-q[0]*r[2])+p[2]*(q[0]*r[1]-q[1]*r[0])
        area += length/2
        checksum += triangle.id+triangle.face_id+sum(ids)
        for corner in triangle.corners:
            checksum += corner.chart_id+weight(corner.uv)
            corner_count += 1
    edge_nodes = boundary_nodes = 0
    boundary_length = 0.
    for edge in mesh.edge_polylines:
        checksum += edge.edge_id
        for parameter, vid in zip(edge.parameters, edge.vertices, strict=True):
            checksum += weight(parameter)+vid
            edge_nodes += 1
        for a,b in zip(edge.vertices, edge.vertices[1:]):
            boundary_length += math.dist(xyz[3*a:3*a+3], xyz[3*b:3*b+3])
    for use in mesh.boundary_uses:
        checksum += use.coedge_id+use.face_id+use.edge_id+use.orientation
        for parameter, vid, uv in zip(use.parameters, use.vertices, use.uv, strict=True):
            checksum += weight(parameter)+vid+weight(uv)
            boundary_nodes += 1
    if not all(math.isfinite(value) for value in xyz) or not math.isfinite(volume6+area+boundary_length):
        raise ArithmeticError('nonfinite indexed mesh consumer output')
    metrics = (('vertices', len(xyz)//3), ('triangles', len(face_ids)), ('corners', corner_count),
               ('edge_polylines', len(mesh.edge_polylines)), ('edge_parameter_occurrences', edge_nodes),
               ('boundary_uses', len(mesh.boundary_uses)), ('boundary_parameter_occurrences', boundary_nodes),
               ('rounded_signed_volume', volume6/6), ('rounded_triangle_area', area),
               ('physical_boundary_length', boundary_length), ('source_checksum', checksum),
               ('indexed_buffer_bytes', len(xyz)*xyz.itemsize+len(indices)*indices.itemsize
                +len(face_ids)*face_ids.itemsize+len(normals)*normals.itemsize))
    return IndexedRead(xyz, indices, face_ids, normals, metrics)


def write_source(directory, body):
    tsv(directory/'body.tsv', ('shape','body_id','revision','origin_x','origin_y','origin_z','scale'),
        ((body.shape,body.body_id,body.revision,*(ratio(x) for x in body.frame.origin),ratio(body.frame.scale)),))
    tables = directory/'body-tables'
    tables.mkdir()
    for name, payload in body.packed_tables:
        (tables/(name+'.bin')).write_bytes(payload)
    tsv(tables/'sizes.tsv', ('table','bytes'), ((name,len(payload)) for name,payload in body.packed_tables))
    tsv(directory/'original-vertices.tsv', ('vertex_id','local_x','local_y','local_z'),
        ((v.id,*(ratio(x) for x in v.position)) for v in body.vertices))
    tsv(directory/'original-curves.tsv', ('curve_slot','kind','term','value'),
        ((i,c.kind,j,ratio(x)) for i,c in enumerate(body.curves) for j,x in enumerate(c.data)))
    tsv(directory/'original-edges.tsv', ('edge_id','start_vertex_id','end_vertex_id','curve_slot'),
        ((e.id,body.vertices[e.start].id,body.vertices[e.end].id,e.curve_slot) for e in body.edges))
    tsv(directory/'original-coedges.tsv', ('coedge_id','face_id','loop_id','edge_id','orientation','uv_kind'),
        ((c.id,body.faces[c.face_slot].id,body.loops[c.loop_slot].id,body.edges[c.edge_slot].id,
          c.orientation,c.uv_map.kind) for c in body.coedges))
    tsv(directory/'original-uv-maps.tsv', ('coedge_id','kind','term','value'),
        ((c.id,c.uv_map.kind,i,ratio(x)) for c in body.coedges for i,x in enumerate(c.uv_map.data)))
    tsv(directory/'original-loops.tsv', ('loop_id','face_id','hole','coedge_ids'),
        ((l.id,body.faces[l.face_slot].id,int(l.hole),','.join(str(body.coedges[i].id) for i in l.coedge_slots))
         for l in body.loops))
    tsv(directory/'original-faces.tsv', ('face_id','name','kind','orientation','loop_ids','chart_ids'),
        ((f.id,f.name,f.kind,f.orientation,','.join(str(body.loops[i].id) for i in f.loop_slots),
          ','.join(str(body.charts[i].id) for i in f.chart_slots)) for f in body.faces))
    tsv(directory/'original-charts.tsv', ('slot','chart_id','face_id','quadrant','degree_u','degree_v',
        'u0_hex','u1_hex','v0_hex','v1_hex','origin_x_hex','origin_y_hex','origin_z_hex','scale_hex'),
        ((i,c.id,body.faces[c.face_slot].id,c.quadrant,c.patch.degree_u,c.patch.degree_v,
          *(x.hex() for x in c.patch.uv+c.patch.origin),c.patch.scale.hex()) for i,c in enumerate(body.charts)))
    tsv(directory/'original-controls.tsv', ('patch_slot','control','X_hex','Y_hex','Z_hex','W_hex'),
        ((i,j,*(x.hex() for x in p.controls[4*j:4*j+4])) for i,p in enumerate(body.patches)
         for j in range(len(p.controls)//4)))


def mesh_svg(path, mesh, reader):
    """Actual cap triangles and full indexed wire view; no visibility solver."""
    body = mesh.body
    origin, scale = body.frame.origin, body.frame.scale
    points = tuple(tuple((reader.xyz[3*i+a]-origin[a])/scale for a in range(3))
                   for i in range(len(mesh.vertices)))
    top_id = next(f.id for f in body.faces if f.name == 'top')
    palette = ('#43b7ad','#72a6ce','#e9bb70','#ad93cf','#dd8d9a','#82c4a0','#ef975e')
    def top(p): return 290+280*p[0], 335-280*p[1]
    def iso(p): return 840+155*p[0]-105*p[1], 360+55*p[0]+70*p[1]-180*p[2]
    parts = ['<svg xmlns="http://www.w3.org/2000/svg" width="1160" height="650" viewBox="0 0 1160 650">',
             '<rect width="1160" height="650" rx="18" fill="#101923"/>',
             '<g font-family="monospace" fill="#dce9ed">',
             f'<text x="30" y="40" font-size="23">BRepGPU / {escape(body.shape)} / complete indexed CPU mesh</text>',
             '<text x="30" y="72" font-size="13">Exact source topology, rational construction and one shared rounded vertex array</text>',
             '<text x="80" y="112" font-size="16">Actual top-face triangles</text>',
             '<text x="685" y="112" font-size="16">All-face indexed wire view</text>']
    for i, face_id in enumerate(reader.face_ids):
        ids = reader.indices[3*i:3*i+3]
        color = palette[(face_id-1)%len(palette)]
        if face_id == top_id:
            pixels = ' '.join(f'{x:.3f},{y:.3f}' for x,y in (top(points[j]) for j in ids))
            parts.append(f'<polygon points="{pixels}" fill="{color}" fill-opacity=".37" stroke="{color}" stroke-width=".65"/>')
        pixels = ' '.join(f'{x:.3f},{y:.3f}' for x,y in (iso(points[j]) for j in ids))
        parts.append(f'<polygon points="{pixels}" fill="none" stroke="{color}" stroke-width=".55" opacity=".60"/>')
    # Physical wall seam has two source uses on one face, never two spatial edges.
    uses = {}
    for use in mesh.boundary_uses:
        uses.setdefault((use.edge_id,use.face_id), []).append(use)
    seam_edges = {edge for (edge,_),rows in uses.items() if len(rows)==2}
    for edge in mesh.edge_polylines:
        if edge.edge_id in seam_edges:
            pixels = ' '.join(f'{x:.3f},{y:.3f}' for x,y in (iso(points[j]) for j in edge.vertices))
            parts.append(f'<polyline points="{pixels}" fill="none" stroke="#ffd166" stroke-width="2.6"/>')
    parts.extend((f'<text x="30" y="555" font-size="15">{len(mesh.vertices)} shared vertices / {len(mesh.triangles)} triangles / {len(mesh.boundary_uses)} original coedge uses</text>',
                  f'<text x="30" y="585" font-size="13">World tolerance {escape(ratio(mesh.tolerance))}; certified total bound {float(mesh.error_bound):.6g}</text>',
                  '<text x="30" y="615" font-size="12">Colors name original faces. Gold: one physical seam / two atlas occurrences. Wire view has no hidden-line removal.</text>',
                  '</g></svg>'))
    path.write_text('\n'.join(parts)+'\n')


def export_mesh(directory, mesh, reader, quality):
    directory.mkdir()
    write_source(directory, mesh.body)
    tsv(directory/'bounds.tsv', ('field','value'),
        ((name,ratio(getattr(mesh,name))) for name in ('tolerance','curve_bound','rounding_bound','error_bound')))
    tsv(directory/'counts.tsv', ('field','value'),
        (('quarter_segments',mesh.quarter_segments),('vertices',len(mesh.vertices)),('triangles',len(mesh.triangles)),
         ('edge_polylines',len(mesh.edge_polylines)),('boundary_uses',len(mesh.boundary_uses))))
    tsv(directory/'vertices.tsv', ('id','x','y','z','rounded_x_hex','rounded_y_hex','rounded_z_hex',
        'source_kind','source_id','source_parameter_or_center'),
        ((v.id,*(ratio(x) for x in v.position),*(x.hex() for x in v.rounded_position),
          *source_values(v.source)) for v in mesh.vertices))
    tsv(directory/'triangles.tsv', ('id','face_id','v0','v1','v2'),
        ((t.id,t.face_id,*t.vertices) for t in mesh.triangles))
    tsv(directory/'corners.tsv', ('triangle','corner','vertex','chart_id','local_u','local_v'),
        ((t.id,i,t.vertices[i],c.chart_id,*(ratio(x) for x in c.uv))
         for t in mesh.triangles for i,c in enumerate(t.corners)))
    tsv(directory/'edge-polylines.tsv', ('edge_id','point','original_parameter','vertex'),
        ((e.edge_id,i,ratio(t),v) for e in mesh.edge_polylines for i,(t,v) in enumerate(zip(e.parameters,e.vertices))))
    tsv(directory/'boundary-uses.tsv', ('coedge_id','face_id','edge_id','orientation','point',
        'original_parameter','vertex','original_u','original_v'),
        ((u.coedge_id,u.face_id,u.edge_id,u.orientation,i,ratio(t),v,*(ratio(x) for x in uv))
         for u in mesh.boundary_uses for i,(t,v,uv) in enumerate(zip(u.parameters,u.vertices,u.uv))))
    tsv(directory/'qualification.tsv', ('metric','value'), quality.items())
    tsv(directory/'reader.tsv', ('metric','value'), reader.metrics)
    with (directory/'mesh.ply').open('w') as stream:
        stream.write('ply\nformat ascii 1.0\ncomment shared budgeted binary64 coordinates; geometric facet normals\n')
        stream.write(f'element vertex {len(reader.xyz)//3}\nproperty double x\nproperty double y\nproperty double z\n')
        stream.write(f'element face {len(reader.face_ids)}\nproperty list uchar uint vertex_indices\n')
        stream.write('property uint face_id\nproperty double nx\nproperty double ny\nproperty double nz\nend_header\n')
        for i in range(len(reader.xyz)//3):
            stream.write(' '.join(format(x,'.17g') for x in reader.xyz[3*i:3*i+3])+'\n')
        for i,face in enumerate(reader.face_ids):
            stream.write('3 '+' '.join(str(x) for x in reader.indices[3*i:3*i+3])+f' {face} '
                         +' '.join(format(x,'.17g') for x in reader.normals[3*i:3*i+3])+'\n')
    mesh_svg(directory/'mesh.svg',mesh,reader)
    (directory/'RECIPE.md').write_text(
        '# Complete canonical CPU triangle mesh\n\n'
        f'{mesh.body.shape}: {len(mesh.vertices)} shared vertices, {len(mesh.triangles)} oriented triangles, '
        f'{mesh.quarter_segments} subdivisions per original circle quarter.\n\n'
        f'Requested world tolerance {ratio(mesh.tolerance)}; curve bound {ratio(mesh.curve_bound)}, '
        f'actual rounded-vertex L1 bound {ratio(mesh.rounding_bound)}, total {ratio(mesh.error_bound)}. '
        'Exact vertex coordinates and corner UVs remain rational. One indexed rounded coordinate '
        'per global vertex supplies PLY and both SVG views. PLY normals are geometric facet normals, '
        'not smoothed source normals. Top view draws actual top-face triangles; the second view '
        'draws every face as a wireframe without hidden-line removal.\n\n'
        'Circle chords approximate the analytic trim: disk caps omit a narrow rim and plate caps '
        'include a narrow strip inside the analytic hole. The error bound covers these domains; '
        'not every triangle point is on the exact analytic trim. A plate retains its through-hole. '
        'No GPU/native mesher, Boolean, imported-body or general-triangulation claim applies.\n\n'
        'body-tables/ preserves original packed factory records; original-*.tsv names original '
        'vertices/edges/coedges/loops/faces/charts/maps/control bits. corners.tsv uses chart-local '
        'UV; boundary-uses.tsv uses original maps, including distinct seam atlas0/4 occurrences. '
        'Source IDs are typed within the retained process-local body/revision namespace.\n')


def main(args):
    if sys.platform != 'linux':
        raise RuntimeError('product callers run on the fleet')
    args.output_directory.mkdir(parents=True,exist_ok=False)
    pins = source_pins()
    start = time.perf_counter()
    body = make_body(args.shape,Frame(tuple(args.origin),args.scale))
    prepared = time.perf_counter()
    mesh = tessellate(body,tolerance=args.tolerance,max_quarter_segments=args.max_quarter_segments)
    built = time.perf_counter()
    reader = read_mesh(mesh)
    consumed = time.perf_counter()
    check_binding(mesh,body,args.tolerance,args.max_quarter_segments)
    quality = qualify_mesh(mesh)
    qualified = time.perf_counter()
    export_mesh(args.output_directory/'mesh',mesh,reader,quality)
    exported = time.perf_counter()
    tsv(args.output_directory/'timings.tsv', ('scope','milliseconds'),
        (('body_prepare',1000*(prepared-start)),('construct',1000*(built-prepared)),
         ('indexed_reader',1000*(consumed-built)),('construct_and_reader',1000*(consumed-prepared)),
         ('independent_qualification',1000*(qualified-consumed)),('filesystem_export',1000*(exported-qualified)),
         ('prepare_through_export_including_qualification',1000*(exported-start))))
    if source_pins() != pins: raise AssertionError('caller source changed during operation')
    tsv(args.output_directory/'sources.tsv', ('path','sha256'),pins)
    print(f'{body.shape}: vertices={len(mesh.vertices)} triangles={len(mesh.triangles)} '
          f'n={mesh.quarter_segments} complete_ms={1000*(consumed-prepared):.3f}',flush=True)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--shape',choices=('box','cylinder','plate'),default='plate')
    parser.add_argument('--tolerance',type=F,default=F(1,1024))
    parser.add_argument('--origin',nargs=3,type=float,default=(0.,0.,0.))
    parser.add_argument('--scale',type=float,default=1.)
    parser.add_argument('--max-quarter-segments',type=int,default=256)
    parser.add_argument('--output-directory',type=Path,required=True)
    main(parser.parse_args())
