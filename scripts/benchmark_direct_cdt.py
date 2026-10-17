#!/usr/bin/env python3
"""Fresh CPU-only direct CDT comparisons with exact complete-domain qualification."""
from __future__ import annotations

import argparse
import csv
from dataclasses import asdict
import os
from pathlib import Path
import resource
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[1]
sys.path[:0] = [str(ROOT/'python'), str(ROOT/'tests')]
from brepgpu.face_domain import CDTStatistics, DirectCDT, ResourceError, triangulate
from brepgpu.imported_meshing import _improve_connectivity
from direct_cdt_cases import SCALING_FAMILIES, scaling_case, validate_scaling_input
from direct_cdt_oracle import qualify_cdt

NATIVE_FIELDS = tuple(CDTStatistics.__dataclass_fields__)
FIELDS = ('case family target_sites sites holes implementation seed metric_u metric_v '
          'repeat cpu status generation_seconds product_seconds input_validation_seconds '
          'qualification_seconds worker_seconds process_seconds product_peak_rss_kib peak_rss_kib '
          'encoded_bytes normalize_ns triangles checked_edges cocircular_edges detail').split() + [
              'native_'+field for field in NATIVE_FIELDS]


def write_row(path, row):
    temporary = path.with_suffix(path.suffix+'.tmp')
    with temporary.open('w', newline='') as stream:
        writer = csv.DictWriter(stream, fieldnames=FIELDS, delimiter='\t')
        writer.writeheader()
        writer.writerow(row)
    temporary.replace(path)


def worker(args):
    started = time.perf_counter()
    cpu = args.cpu if args.cpu is not None else min(os.sched_getaffinity(0))
    os.sched_setaffinity(0, {cpu})
    metric = tuple(map(float, args.metric.split(',')))
    row = dict(case=args.family+'-'+str(args.size), family=args.family,
               target_sites=args.size, implementation=args.implementation,
               seed=args.seed, metric_u=metric[0], metric_v=metric[1],
               repeat=args.repeat, cpu=cpu, status='GENERATING')
    def progress():
        if args.progress is not None:
            write_row(args.progress, row)
    progress()
    product_started = None
    qualification_started = None
    try:
        generation_started = time.perf_counter()
        case = scaling_case(args.family, args.size)
        points = case.points
        row.update(sites=len(points), holes=len(case.loops)-1,
                   generation_seconds=time.perf_counter()-generation_started,
                   status='CONSTRUCTING')
        progress()
        product_started = time.perf_counter()
        if args.implementation == 'direct':
            with DirectCDT(case.loops, points, metric=metric, seed=args.seed) as owner:
                output = owner.triangulate()
                row.update({'native_'+key: value for key, value in asdict(owner.stats).items()})
                row.update(encoded_bytes=owner.encoded_bytes, normalize_ns=owner.normalize_ns)
        else:
            constraints = {tuple(sorted((a, b))) for loop in case.loops
                           for a, b in zip(loop, loop[1:]+loop[:1])}
            initial = triangulate(case.loops, points)
            output = tuple(_improve_connectivity(initial, points, constraints, metric))
        row.update(product_seconds=time.perf_counter()-product_started,
                   product_peak_rss_kib=resource.getrusage(resource.RUSAGE_SELF).ru_maxrss,
                   triangles=len(output), status='QUALIFYING')
        progress()
        qualification_started = time.perf_counter()
        def input_reader(actual):
            start = time.perf_counter()
            result = validate_scaling_input(args.family, actual)
            row['input_validation_seconds'] = time.perf_counter()-start
            return result
        stats = qualify_cdt(case, output, metric=metric, input_validator=input_reader)
        if args.family == 'long-recovery' and args.implementation == 'direct':
            if int(row['native_crossed_edges']) <= 0 or int(row['native_cavity_occurrences']) <= 0:
                raise AssertionError('designated long-recovery input did not exercise native recovery')
        row.update(qualification_seconds=time.perf_counter()-qualification_started,
                   checked_edges=stats.interior_edges,
                   cocircular_edges=stats.cocircular_edges, status='PASS')
    except Exception as error:
        if product_started is not None and 'product_seconds' not in row:
            row['product_seconds'] = time.perf_counter()-product_started
        if qualification_started is not None:
            row['qualification_seconds'] = time.perf_counter()-qualification_started
        row.update(status='RESOURCE_EXHAUSTED' if isinstance(error, ResourceError) else 'FAIL',
                   detail=type(error).__name__+': '+str(error))
    row.update(worker_seconds=time.perf_counter()-started,
               peak_rss_kib=resource.getrusage(resource.RUSAGE_SELF).ru_maxrss)
    progress()
    writer = csv.DictWriter(sys.stdout, fieldnames=FIELDS, delimiter='\t')
    writer.writeheader()
    writer.writerow(row)
    return 0 if row['status'] == 'PASS' else 1


def main(args):
    if args.worker:
        return worker(args)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    families = args.families.split(',')
    sizes = tuple(map(int, args.sizes.split(',')))
    seeds = tuple(map(int, args.seeds.split(',')))
    implementations = args.implementations.split(',')
    if any(family not in SCALING_FAMILIES for family in families):
        raise ValueError('unknown scaling family')
    if any(implementation not in ('direct', 'ear-lawson') for implementation in implementations):
        raise ValueError('unknown implementation')
    failed = False
    with args.output.open('w', newline='') as stream:
        writer = csv.DictWriter(stream, fieldnames=FIELDS, delimiter='\t')
        writer.writeheader()
        stream.flush()
        # Breadth first: finish all families at a size before increasing it.
        for size in sizes:
            for family in families:
                for implementation in implementations:
                    # The deterministic baseline has no seed-sensitive algorithm.
                    actual_seeds = seeds if implementation == 'direct' else seeds[:1]
                    for seed in actual_seeds:
                        for repeat in range(1, args.repeats+1):
                            identity = f'{family}.{size}.{implementation}.{seed}.{repeat}'
                            progress = args.output.with_name(args.output.name+'.'+identity+'.progress.tsv')
                            progress.unlink(missing_ok=True)
                            command = [sys.executable, '-B', str(Path(__file__).resolve()),
                                       '--worker', '--family', family, '--size', str(size),
                                       '--implementation', implementation, '--seed', str(seed),
                                       '--repeat', str(repeat), '--metric', args.metric,
                                       '--progress', str(progress.resolve())]
                            if args.cpu is not None:
                                command += ['--cpu', str(args.cpu)]
                            process_started = time.perf_counter()
                            try:
                                result = subprocess.run(command, capture_output=True, text=True,
                                                        timeout=args.timeout,
                                                        env=dict(os.environ, CUDA_VISIBLE_DEVICES=''))
                                rows = list(csv.DictReader(result.stdout.splitlines(), delimiter='\t'))
                                if len(rows) != 1:
                                    raise RuntimeError('worker emitted no single result row: '+result.stderr[-500:])
                                row = rows[0]
                                if result.returncode and row['status'] == 'PASS':
                                    row.update(status='WORKER_ERROR', detail='worker failed after PASS result')
                            except subprocess.TimeoutExpired:
                                row = dict(case=family+'-'+str(size), family=family, target_sites=size,
                                           implementation=implementation, seed=seed, repeat=repeat)
                                if progress.exists():
                                    with progress.open() as saved:
                                        row.update(next(csv.DictReader(saved, delimiter='\t')))
                                phase = row.get('status', 'UNOBSERVED')
                                row.update(status=phase+'_TIMEOUT',
                                           detail=f'censored at {args.timeout}s; last observed phase {phase}')
                            except (RuntimeError, OSError, ValueError) as error:
                                row = dict(case=family+'-'+str(size), family=family, target_sites=size,
                                           implementation=implementation, seed=seed, repeat=repeat,
                                           status='WORKER_ERROR', detail=str(error))
                            row['process_seconds'] = time.perf_counter()-process_started
                            failed |= row['status'] != 'PASS'
                            writer.writerow(row)
                            stream.flush()
                            print(identity, row['status'], row.get('product_seconds', ''), flush=True)
    return 1 if failed else 0


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--worker', action='store_true')
    parser.add_argument('--family', choices=SCALING_FAMILIES, default='protected-strip')
    parser.add_argument('--families', default=','.join(SCALING_FAMILIES))
    parser.add_argument('--size', type=int, default=64)
    parser.add_argument('--sizes', default='64,256,1024,4096,8192')
    parser.add_argument('--implementation', choices=('direct', 'ear-lawson'), default='direct')
    parser.add_argument('--implementations', default='direct,ear-lawson')
    parser.add_argument('--seed', type=int, default=0)
    parser.add_argument('--seeds', default='0,1,2')
    parser.add_argument('--repeat', type=int, default=1)
    parser.add_argument('--repeats', type=int, default=1)
    parser.add_argument('--metric', default='1,1')
    parser.add_argument('--cpu', type=int)
    parser.add_argument('--timeout', type=float, default=30)
    parser.add_argument('--progress', type=Path)
    parser.add_argument('--output', type=Path, default=Path('context/direct-cdt-scaling.tsv'))
    raise SystemExit(main(parser.parse_args()))
