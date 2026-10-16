#!/usr/bin/env python3
"""Fresh-process exact domain scaling; TSV outcomes include censored timeouts."""
from __future__ import annotations
import argparse
import csv
from dataclasses import asdict
from fractions import Fraction
import math
import os
from pathlib import Path
import resource
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'python'))
from brepgpu.face_domain import FaceDomain, read_snapshot
from brepgpu.imported_meshing import _boundary_intersection, _triangulate
from brepgpu.imported_oracles import qualify_domain

STATS = ('memory_peak work bvh_visits leaf_box_tests crossing_bvh_visits crossing_box_tests segment_candidates segment_predicates orientations '
         'ear_attempts ear_blocker_queries ear_candidates dead_skips bridge_candidates '
         'max_limbs strict_convex decode_ns crossing_ns bridge_ns ear_ns').split()
FIELDS = ['case', 'implementation', 'repeat', 'status', 'nodes', 'holes', 'triangles',
          'construction_seconds', 'qualification_seconds', 'peak_rss_kib', 'encoded_bytes',
          'normalize_ns', *STATS, 'detail']


def authored(family, count):
    if count < 8:
        raise ValueError('scaling size must be at least8')
    if family == 'sparse-strip':
        q = max(2, count//4)
        word = ([(Fraction(i, q)*2**20, Fraction(0)) for i in range(q)] +
                [(Fraction(2**20), Fraction(i, q*1024)) for i in range(q)] +
                [(Fraction(q-i, q)*2**20, Fraction(1, 1024)) for i in range(q)] +
                [(Fraction(0), Fraction(q-i, q*1024)) for i in range(q)])
        return (tuple(range(len(word))),), dict(enumerate(word))
    if family == 'strict-convex':
        points = {i: (Fraction(i), Fraction(i*i)) for i in range(count)}
        return (tuple(points),), points
    if family == 'dense-concave':
        points = {}
        for i in range(count):
            angle = 2*math.pi*i/count; radius = 1 if i % 2 == 0 else .55
            points[i] = (Fraction(radius*math.cos(angle)), Fraction(radius*math.sin(angle)))
        return (tuple(points),), points
    if family == 'holes':
        side = max(1, math.isqrt(max(1, (count-4)//4))); extent = 4*side+2
        words = [[(0, 0), (extent, 0), (extent, extent), (0, extent)]]
        for x in range(side):
            for y in range(side):
                a, b = 4*x+1, 4*y+1
                words.append([(a, b), (a, b+1), (a+1, b+1), (a+1, b)])
        points = {}; loops = []
        for word in words:
            indices = []
            for point in word:
                index = len(points); points[index] = tuple(map(Fraction, point)); indices.append(index)
            loops.append(tuple(indices))
        return tuple(loops), points
    raise ValueError('unknown authored family')


def worker(args):
    loops, points, token = (*authored(args.family, args.size), 0) if args.snapshot is None else read_snapshot(args.snapshot)
    row = {name: '' for name in FIELDS}
    row.update(case=args.snapshot.name if args.snapshot else args.family+'-'+str(args.size),
               implementation=args.implementation, repeat=args.repeat, nodes=len(points), holes=len(loops)-1)
    start = time.perf_counter()
    def progress():
        if args.progress:
            temporary = args.progress.with_suffix('.tmp')
            with temporary.open('w', newline='') as stream:
                writer = csv.DictWriter(stream, fieldnames=FIELDS, delimiter='\t')
                writer.writeheader(); writer.writerow(row)
            temporary.replace(args.progress)
    row['status'] = 'CONSTRUCTING'; progress()
    try:
        if args.implementation == 'native':
            with FaceDomain(loops, points, token=token) as domain:
                triangles = domain.triangulate(); row.update({k: v for k, v in asdict(domain.stats).items() if k in STATS})
                row.update(encoded_bytes=domain.encoded_bytes, normalize_ns=domain.normalize_ns)
        else:
            if _boundary_intersection(loops, points) is not None:
                raise ValueError('legacy source boundary crossed')
            triangles = tuple(_triangulate(loops, points))
        row.update(construction_seconds=f'{time.perf_counter()-start:.9f}', triangles=len(triangles))
        row['status'] = 'QUALIFYING'; progress()
        check = time.perf_counter()
        qualify_domain(tuple(tuple(points[i] for i in word) for word in loops),
                       tuple(tuple(points[i] for i in triangle) for triangle in triangles))
        if any(i not in points for triangle in triangles for i in triangle):
            raise ValueError('foreign source node')
        row.update(qualification_seconds=f'{time.perf_counter()-check:.9f}', status='PASS')
        if args.family == 'sparse-strip' and args.implementation == 'native' and args.snapshot is None:
            if int(row['crossing_bvh_visits'])+int(row['crossing_box_tests']) >= 64*len(points):
                raise ValueError('sparse boundary search work budget failed')
        if args.family == 'strict-convex' and args.implementation == 'native' and args.snapshot is None:
            if row['strict_convex'] != 1 or row['ear_blocker_queries'] != 0:
                raise ValueError('strict convex fast path not used')
    except Exception as error:
        if not row['construction_seconds']:
            row['construction_seconds'] = f'{time.perf_counter()-start:.9f}'
        row.update(status='FAIL', detail=type(error).__name__+': '+str(error))
    row['peak_rss_kib'] = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss
    progress()
    writer = csv.DictWriter(sys.stdout, fieldnames=FIELDS, delimiter='\t'); writer.writeheader(); writer.writerow(row)
    return 0 if row['status'] == 'PASS' else 1


def main(args):
    if args.worker:
        return worker(args)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    failed = False
    with args.output.open('w', newline='') as output:
        writer = csv.DictWriter(output, fieldnames=FIELDS, delimiter='\t'); writer.writeheader(); output.flush()
        for size in map(int, args.sizes.split(',')):
            for implementation in args.implementations.split(','):
                for repeat in range(1, args.repeats+1):
                    command = [sys.executable, '-B', str(Path(__file__).resolve()), '--worker', '--family', args.family,
                               '--size', str(size), '--implementation', implementation, '--repeat', str(repeat)]
                    progress_path = args.output.with_name(args.output.name+f'.{size}.{implementation}.{repeat}.progress.tsv')
                    command += ['--progress', str(progress_path.resolve())]
                    if progress_path.exists():
                        progress_path.unlink()
                    if args.snapshot:
                        command += ['--snapshot', str(args.snapshot.resolve())]
                    try:
                        result = subprocess.run(command, capture_output=True, text=True, timeout=args.timeout,
                                                env=dict(os.environ, CUDA_VISIBLE_DEVICES=''))
                        rows = list(csv.DictReader(result.stdout.splitlines(), delimiter='\t'))
                        if len(rows) != 1:
                            raise RuntimeError('worker did not return exactly one TSV row: '+result.stderr[-500:])
                        row = rows[0]
                        if result.returncode != 0 or row['status'] != 'PASS':
                            failed = True
                    except subprocess.TimeoutExpired:
                        row = dict(case=args.snapshot.name if args.snapshot else args.family+'-'+str(size),
                                   implementation=implementation, repeat=repeat)
                        if progress_path.exists():
                            with progress_path.open() as stream:
                                row.update(next(csv.DictReader(stream, delimiter='\t')))
                        phase = row.get('status', 'UNOBSERVED')
                        row.update(status=phase+'_TIMEOUT', detail=f'censored at{args.timeout}s; last observed phase{phase}')
                        failed = True
                    except (RuntimeError, OSError) as error:
                        row = dict(case=args.snapshot.name if args.snapshot else args.family+'-'+str(size),
                                   implementation=implementation, repeat=repeat, status='WORKER_ERROR', detail=str(error))
                        failed = True
                    writer.writerow(row); output.flush()
                    print(row['case'], implementation, repeat, row['status'], row.get('construction_seconds', ''), flush=True)
    return 1 if failed else 0


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--worker', action='store_true')
    parser.add_argument('--family', choices=('sparse-strip', 'strict-convex', 'dense-concave', 'holes'), default='sparse-strip')
    parser.add_argument('--size', type=int, default=256)
    parser.add_argument('--sizes', default='256,512,1024,2048,4096,8192')
    parser.add_argument('--implementation', choices=('native', 'legacy'), default='native')
    parser.add_argument('--implementations', default='native')
    parser.add_argument('--repeat', type=int, default=1)
    parser.add_argument('--repeats', type=int, default=3)
    parser.add_argument('--timeout', type=float, default=300)
    parser.add_argument('--snapshot', type=Path)
    parser.add_argument('--progress', type=Path)
    parser.add_argument('--output', type=Path, default=ROOT/'context/face-domain-scaling.tsv')
    sys.exit(main(parser.parse_args()))
