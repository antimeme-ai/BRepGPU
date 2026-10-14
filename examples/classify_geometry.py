#!/usr/bin/env python3
"""Actual exact CPU caller, exported rational records and serial complete costs."""
import argparse
import csv
from fractions import Fraction as F
import hashlib
from html import escape
import math
from pathlib import Path
import platform
import statistics
import sys
import time

from brepgpu.body import Frame, make_body
from brepgpu.classification import classify_points, contact_faces
from brepgpu.classification_oracles import qualify_point, qualify_contact
from brepgpu.intersections import FaceRef

ROOT = Path(__file__).resolve().parents[1]
SOURCES = ('python/brepgpu/__init__.py', 'python/brepgpu/body.py', 'python/brepgpu/intersections.py',
           'python/brepgpu/classification.py', 'python/brepgpu/classification_oracles.py',
           'tests/test_classification_entry.py', 'tests/test_classification.py',
           'examples/classify_geometry.py')


def tsv(path, header, rows):
    with path.open('w', newline='') as stream:
        writer = csv.writer(stream, delimiter='\t', lineterminator='\n')
        writer.writerow(header)
        writer.writerows(rows)


def ratio(x):
    return str(x.numerator) + '/' + str(x.denominator)


def literal_points():
    return ((0, 0, 0), (.25, 0, 0), (.25, math.ulp(0.), 0), (.15, .2, 0),
            (F(3, 20), F(1, 5), 0), (.75, .75, .5), (.25, 0, .5),
            (0, 0, .5), (.5, 0, 0), (0, 0, 1), (sys.float_info.max, 0, 0),
            (-.75, 0, 0))


def point_queries(count):
    literals = literal_points()
    return tuple(literals[i] if i < 12 else literals[(i // 24) % 12] if i % 24 == 0
                 else (F((17*i) % 97 - 48, 64), F((31*i) % 89 - 44, 64),
                       F((19*i) % 71 - 35, 64)) for i in range(count))


def contact_queries(count):
    left = make_body('box')
    recipes = (('coincident-opposite', (0., 0., 1.), 2),
               ('overlap', (.5, 0., 0.), 1), ('edge-touch', (1.5, 0., 0.), 1),
               ('point-touch', (1.5, 1.5, 0.), 1),
               ('coplanar-gap', (math.nextafter(1.5, math.inf), 0., 0.), 1),
               ('parallel-disjoint', (0., 0., math.ulp(0.)), 1),
               ('crossing', (0., .75, .5), 3), ('transverse-touch', (0., 1.5, .5), 3))
    pool = tuple((name, FaceRef(left, 1), FaceRef(make_body('box', Frame(origin)), face))
                 for name, origin, face in recipes)
    return tuple(pool[i % 8] for i in range(count))


def consume_points(rows):
    counts = dict(interior=0, boundary=0, exterior=0)
    checksum = 0
    for row in rows:
        counts[row.label] += 1
        checksum += row.snapshot.body_id + row.snapshot.revision
        checksum += sum(row.boundary_faces)
        checksum += sum(v.numerator.bit_length() + v.denominator.bit_length() for v in row.point)
    return tuple(counts[k] for k in ('interior', 'boundary', 'exterior')) + (checksum,)


def consume_contacts(rows):
    counts = dict(disjoint=0, touch=0, crossing=0, overlap=0, coincident=0)
    checksum = 0
    for row in rows:
        counts[row.relation] += 1
        checksum += row.left.snapshot.body_id + row.right.snapshot.body_id
        checksum += row.left.snapshot.revision + row.right.snapshot.revision
        checksum += row.left.face_id + row.right.face_id + row.dimension + int(row.same_domain)
        checksum += len(row.carrier_relation) + len(row.normal_relation)
        for vertex in row.vertices:
            checksum += sum(v.numerator.bit_length() + v.denominator.bit_length()
                            for v in vertex.position + vertex.left_uv + vertex.right_uv)
    return tuple(counts[k] for k in ('disjoint', 'touch', 'crossing', 'overlap', 'coincident')) + (checksum,)


def write_points(path, rows):
    tsv(path, ('query', 'shape', 'body_id', 'revision', 'origin_x', 'origin_y', 'origin_z',
               'scale', 'x', 'y', 'z', 'label', 'boundary_faces'),
        ((i, r.snapshot.shape, r.snapshot.body_id, r.snapshot.revision,
          *(ratio(F(v)) for v in r.snapshot.frame.origin), ratio(F(r.snapshot.frame.scale)),
          *(ratio(v) for v in r.point), r.label, ','.join(map(str, r.boundary_faces)))
         for i, r in enumerate(rows)))


def write_contacts(path, queries, rows):
    tsv(path, ('query', 'recipe', 'left_body', 'left_revision', 'left_face', 'right_body',
               'right_revision', 'right_face', 'carrier', 'dimension', 'relation',
               'same_domain', 'normal', 'vertex', 'x', 'y', 'z', 'left_u', 'left_v',
               'right_u', 'right_v'),
        ((i, query[0], r.left.snapshot.body_id, r.left.snapshot.revision, r.left.face_id,
          r.right.snapshot.body_id, r.right.snapshot.revision, r.right.face_id,
          r.carrier_relation, r.dimension, r.relation, int(r.same_domain), r.normal_relation,
          j, *(ratio(v) for v in vertex.position + vertex.left_uv + vertex.right_uv))
         if vertex else (i, query[0], r.left.snapshot.body_id, r.left.snapshot.revision,
          r.left.face_id, r.right.snapshot.body_id, r.right.snapshot.revision, r.right.face_id,
          r.carrier_relation, r.dimension, r.relation, int(r.same_domain), r.normal_relation,
          -1, '', '', '', '', '', '', '')
         for i, (query, r) in enumerate(zip(queries, rows))
         for j, vertex in (enumerate(r.vertices) if r.vertices else ((-1, None),))))
    sources = {}
    for _, left, right in queries:
        for ref in (left, right):
            sources[(ref.snapshot.body_id, ref.face_id)] = ref
    tsv(path.with_name('contact-sources.tsv'),
        ('body_id', 'revision', 'face_id', 'face_name', 'orientation',
         'origin_x', 'origin_y', 'origin_z', 'scale'),
        ((r.snapshot.body_id, r.snapshot.revision, r.face_id, r.face.name, r.face.orientation,
          *(ratio(F(v)) for v in r.snapshot.frame.origin), ratio(F(r.snapshot.frame.scale)))
         for r in sources.values()))


def diagram(path, point_rows, contact_rows):
    # Coordinates and labels are read from actual qualified results.
    out = ['<svg xmlns="http://www.w3.org/2000/svg" width="1000" height="490" viewBox="0 0 1000 490">',
           '<rect width="1000" height="490" rx="16" fill="#101923"/>',
           '<g font-family="monospace" fill="#d7e5ef">',
           '<text x="28" y="35" font-size="21">Exact material regions + closed face contacts</text>']
    colors = dict(interior='#37d1b0', boundary='#ffd166', exterior='#a6b4c6')
    for panel, shape in enumerate(('box', 'cylinder', 'plate')):
        ox = 165 + 330*panel
        out.append(f'<text x="{ox-55}" y="72" font-size="17">{shape}</text>')
        body = next(r.snapshot for r in point_rows if r.snapshot.shape == shape)
        origin = tuple(F(v) for v in body.frame.origin)
        scale = F(body.frame.scale)
        # Canonical bodies extrude these original vertex/curve profiles. Read
        # source geometry rather than hardcoding an unrelated display silhouette.
        if shape != 'cylinder':
            xmin, xmax = (origin[0]+scale*f(F(v.position[0]) for v in body.vertices) for f in (min,max))
            ymin, ymax = (origin[1]+scale*f(F(v.position[1]) for v in body.vertices) for f in (min,max))
            x, y = ox+120*float(xmin), 195-120*float(ymax)
            width, height = 120*float(xmax-xmin), 120*float(ymax-ymin)
            out.append(f'<rect x="{x:.3f}" y="{y:.3f}" width="{width:.3f}" height="{height:.3f}" fill="#183a43" stroke="#37d1b0"/>')
        if shape != 'box':
            radius = scale*F(next(c.data[0] for c in body.curves if c.kind == 'circle'))
            cx, cy, rr = ox+120*float(origin[0]), 195-120*float(origin[1]), 120*float(radius)
            out.append(f'<circle cx="{cx:.3f}" cy="{cy:.3f}" r="{rr:.3f}" fill="'+('#183a43' if shape=='cylinder' else '#101923')+'" stroke="#37d1b0"/>')
        visible = [r for r in point_rows if r.snapshot.shape == shape and r.point[2] == 0
                   and all(abs(x) <= 1 for x in r.point[:2])]
        for r in visible:
            x, y = ox+120*float(r.point[0]), 195-120*float(r.point[1])
            out.append(f'<circle cx="{x:.3f}" cy="{y:.3f}" r="4" fill="{colors[r.label]}"/>')
        sub = next(r for r in visible if r.point[0] == F(1,4) and r.point[1] > 0 and r.point[1] < F(1,1<<1000))
        out.append(f'<text x="{ox-142}" y="312" font-size="11">(1/4, 2^-1074, 0): {sub.label}</text>')
    out.append('<text x="28" y="345" font-size="12">green interior / gold boundary / gray exterior; displayed coincident pixels retain exact distinct values</text>')
    for i, row in enumerate(contact_rows):
        x = 30 + (i%4)*245
        y = 379 + (i//4)*48
        out.append(f'<text x="{x}" y="{y}" font-size="12">{escape(row.relation)} / dim {row.dimension}</text>')
        out.append(f'<text x="{x}" y="{y+17}" font-size="10">{escape(row.carrier_relation)} / {escape(row.normal_relation)}</text>')
        # All exported recipes lie in the original left top plane z=1/2.
        # Draw the actual emitted XY locus; float conversion is display only.
        cx, cy = x+207, y-2
        pixels = tuple((cx+18*float(v.position[0]), cy-18*float(v.position[1])) for v in row.vertices)
        coords = ' '.join(f'{px:.3f},{py:.3f}' for px,py in pixels)
        if row.dimension == 2:
            out.append(f'<polygon points="{coords}" fill="#183a43" stroke="#37d1b0" stroke-width="2"/>')
        elif row.dimension == 1:
            out.append(f'<polyline points="{coords}" fill="none" stroke="#ffd166" stroke-width="2"/>')
        elif row.dimension == 0:
            px, py = pixels[0]
            out.append(f'<circle cx="{px:.3f}" cy="{py:.3f}" r="3" fill="#ffd166"/>')
        else:
            out.append(f'<path d="M{cx-4},{cy-4} L{cx+4},{cy+4} M{cx+4},{cy-4} L{cx-4},{cy+4}" stroke="#a6b4c6"/>')
    out.append('</g></svg>')
    path.write_text('\n'.join(out)+'\n')


def main(args):
    if sys.platform != 'linux':
        raise RuntimeError('product callers and measurement run on the fleet')
    args.output.mkdir(parents=True, exist_ok=False)
    source_hashes = tuple((name, hashlib.sha256((ROOT/name).read_bytes()).hexdigest()) for name in SOURCES)
    tsv(args.output/'sources.tsv', ('path', 'sha256'), source_hashes)
    tsv(args.output/'environment.tsv', ('key', 'value'),
        (('placement', 'CPU exact single process'), ('python', platform.python_version()),
         ('system', platform.system()), ('release', platform.release()),
         ('architecture', platform.machine()), ('cpu', next((line.split(':',1)[1].strip() for line in
            Path('/proc/cpuinfo').read_text().splitlines() if line.startswith('model name')), 'unknown')),
         ('warmup', 1), ('repeats', 5), ('qualification', 'outside complete caller; recorded separately')))
    raw = []
    prepares = []
    exported_points = []
    exported_contacts = []
    cases = tuple(('points-'+shape+'-'+str(n), 'point', shape, n)
                  for shape in ('box','cylinder','plate') for n in (12,1536,12288)) + \
            tuple(('contacts-'+str(n), 'contact', '', n) for n in (8,128,512))
    for name, kind, shape, count in cases:
        start = time.perf_counter()
        if kind == 'point':
            body = make_body(shape)
            queries = point_queries(count)
            operation = lambda: classify_points(body, queries)
            consumer, qualifier = consume_points, qualify_point
        else:
            queries = contact_queries(count)
            operation = lambda: tuple(contact_faces(q[1], q[2]) for q in queries)
            consumer, qualifier = consume_contacts, qualify_contact
        prepare_ms = 1000*(time.perf_counter()-start)
        prepares.append((name, kind, count, prepare_ms))
        expected_digest = None
        for phase, sample in (('cold',0), ('warmup',0), *(('repeat',i) for i in range(5))):
            start = time.perf_counter()
            rows = operation()
            built = time.perf_counter()
            digest = consumer(rows)
            end = time.perf_counter()
            check_start = time.perf_counter()
            if len(rows) != count:
                raise AssertionError('operation changed submitted query count')
            for query, row in zip(queries, rows):
                if kind == 'point':
                    if row.snapshot is not body or row.point != tuple(F(value) for value in query):
                        raise AssertionError('point result changed submitted source or exact world query')
                elif row.left is not query[1] or row.right is not query[2]:
                    raise AssertionError('contact result changed submitted source references')
                qualifier(row)
            qualified = time.perf_counter()
            if expected_digest is None:
                expected_digest = digest
            elif digest != expected_digest:
                raise AssertionError('actual consumer changed on retained input')
            raw.append((name, kind, count, phase, sample, 1000*(built-start),
                        1000*(end-built), 1000*(end-start), 1000*(qualified-check_start),
                        ','.join(map(str,digest)), 'PASS'))
        if count == (12 if kind=='point' else 8):
            if kind == 'point':
                exported_points.extend(rows)
                write_points(args.output/(shape+'-points.tsv'), rows)
            else:
                exported_contacts.extend(rows)
                write_contacts(args.output/'contacts.tsv', queries, rows)
        print(name, 'qualified', count*7, 'outputs', flush=True)
    tsv(args.output/'preparation.tsv', ('case','kind','count','prepare_ms'), prepares)
    header = ('case','kind','count','phase','sample','operation_ms','consume_ms','complete_ms',
              'qualification_ms','consumer_digest','result')
    tsv(args.output/'raw.tsv', header, raw)
    summary = []
    for name, kind, shape, count in cases:
        for j, scope in ((5,'operation'),(6,'consume'),(7,'complete'),(8,'qualification')):
            vals = [r[j] for r in raw if r[0]==name and r[3]=='repeat']
            summary.append((name, scope, count, statistics.median(vals), min(vals), max(vals)))
    tsv(args.output/'summary.tsv', ('case','scope','count','median_ms','min_ms','max_ms'), summary)
    diagram(args.output/'classification.svg', exported_points, exported_contacts)
    if source_hashes != tuple((name, hashlib.sha256((ROOT/name).read_bytes()).hexdigest()) for name in SOURCES):
        raise AssertionError('source changed during serial campaign')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    main(parser.parse_args())
