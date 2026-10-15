#!/usr/bin/env python3
"""Actual full-task observations and repeated matched native OM10 display.

Four fresh STEP-to-labeled-image calls are single observations. Repeated native
processes consume their unchanged prepared scene and per-pose requests. Resident
frame walls have a separately named narrower scope. No stage-median total.
"""
import argparse
import csv
import hashlib
from pathlib import Path
import statistics
import struct
import subprocess
import sys
import time


ROOT = Path(__file__).resolve().parents[1]
BACKENDS = (('cpu1', 'cpu', 1, 0), ('cpu24', 'cpu', 24, 0),
            ('p100-0', 'cuda', 24, 0), ('p100-1', 'cuda', 24, 1))


def read_rows(path):
    with Path(path).open(newline='', encoding='utf-8') as f:
        return tuple(csv.DictReader(f, delimiter='\t'))


def write_rows(path, header, rows):
    with Path(path).open('w', newline='', encoding='utf-8') as f:
        writer = csv.writer(f, delimiter='\t', lineterminator='\n')
        writer.writerow(header)
        writer.writerows(rows)


def main(args):
    if sys.platform != 'linux':
        raise ValueError('OM10 modeling/render/benchmarks execute on the Linux fleet')
    if args.repeats < 3 or args.resident_frames < 2:
        raise ValueError('at least three native repeats and two resident frames are required')
    args.output.mkdir(parents=True, exist_ok=False)
    script = ROOT/'examples/render_om10.py'
    pin = hashlib.sha256(script.read_bytes()).hexdigest()
    driver = Path(__file__).resolve()
    driver_pin = hashlib.sha256(driver.read_bytes()).hexdigest()
    binary_paths = (args.adapter, args.native, args.plugin)
    binary_hashes = tuple(hashlib.sha256(p.read_bytes()).hexdigest() for p in binary_paths)
    common = [sys.executable, str(script), '--step', str(args.step.resolve()),
              '--adapter', str(args.adapter.resolve()), '--native', str(args.native.resolve()),
              '--plugin', str(args.plugin.resolve()), '--mode', 'render',
              '--width', str(args.width), '--height', str(args.height),
              '--supersample', str(args.supersample), '--tolerance', str(args.tolerance),
              '--source-residual', str(args.source_residual),
              '--azimuth', str(args.azimuth), '--elevation', str(args.elevation)]
    records, outputs, input_hashes, cold_requests, prepared = [], [], {}, {}, {}
    def observations():
        write_rows(args.output/'observations.tsv', ('scope', 'repeat', 'view', 'backend',
            'seconds', 'description'), records)
    for view_index, view in enumerate(('assembled', 'exploded')):
        order = (BACKENDS[1], BACKENDS[2])
        if view_index:
            order = tuple(reversed(order))
        for label, backend, threads, device in order:
            name = f'cold-00-{view}-{label}'
            output = args.output/name
            command = common+['--view', view, '--backend', backend, '--threads', str(threads),
                              '--device', str(device), '--output', str(output)]
            start = time.perf_counter()
            with (args.output/(name+'.log')).open('w') as log:
                subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
            elapsed = time.perf_counter()-start
            hashes = tuple(hashlib.sha256((output/p).read_bytes()).hexdigest()
                           for p in ('scene.bgmesh', 'views.bgview', 'sources.tsv'))
            if view in input_hashes and input_hashes[view] != hashes:
                raise ValueError('matched source/scene/view/software inputs changed across full callers')
            if input_hashes and (hashes[0], hashes[2]) != (next(iter(input_hashes.values()))[0], next(iter(input_hashes.values()))[2]):
                raise ValueError('assembled/exploded full callers changed source geometry/software')
            request = (output/'views.bgview').read_bytes()
            if request[:16] != struct.pack('<8sII', b'BGVIEW01', 1, 1) or len(request) != 208:
                raise ValueError('full task must emit exactly one actual view request')
            input_hashes[view] = hashes
            cold_requests[view] = request[16:]
            prepared.setdefault(view, output)
            records.append(('cold_complete_process', 0, view, label, elapsed,
                            'one observation; fresh STEP through PNG/labeled SVG and interpreter exit'))
            outputs.append((name, view, label, output.name))
            observations()
            print(name, format(elapsed, '.6f'), 's; n=1', flush=True)
    # Pin the actual product sources throughout subsequent native measurements.
    source_pins = read_rows(prepared['assembled']/'sources.tsv')
    def check_inputs():
        for view, output in prepared.items():
            current = tuple(hashlib.sha256((output/p).read_bytes()).hexdigest()
                            for p in ('scene.bgmesh', 'views.bgview', 'sources.tsv'))
            if current != input_hashes[view]:
                raise ValueError('prepared native scene/view/source inputs changed')
        if any(hashlib.sha256((ROOT/r['path']).read_bytes()).hexdigest() != r['sha256'] for r in source_pins):
            raise ValueError('actual product source changed during benchmark')
    def native_command(output, label, backend, threads, device, views):
        command = [str(args.native.resolve()), '--scene', str((prepared['assembled']/'scene.bgmesh').resolve()),
                   '--views', str(views.resolve()), '--output-directory', str(output.resolve()),
                   '--backend', backend, '--threads', str(threads)]
        if backend == 'cuda':
            command += ['--device', str(device), '--plugin', str(args.plugin.resolve())]
        return command
    for repeat in range(args.repeats):
        order = BACKENDS[repeat % len(BACKENDS):]+BACKENDS[:repeat % len(BACKENDS)]
        states = ('assembled', 'exploded') if repeat % 2 == 0 else ('exploded', 'assembled')
        for view in states:
            for label, backend, threads, device in order:
                check_inputs()
                name = f'native-{repeat:02d}-{view}-{label}'
                output = args.output/name
                command = native_command(output, label, backend, threads, device, prepared[view]/'views.bgview')
                start = time.perf_counter()
                with (args.output/(name+'.log')).open('w') as log:
                    subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
                elapsed = time.perf_counter()-start
                records.append(('native_complete_process', repeat, view, label, elapsed,
                                'prepared scene through raw image/IDs/depth and native exit; no STEP/mesh/PNG/SVG'))
                outputs.append((name, view, label, output.name))
                observations()
                print(name, format(elapsed, '.6f'), 's', flush=True)
    resident_views = args.output/'resident.bgview'
    resident_views.write_bytes(struct.pack('<8sII', b'BGVIEW01', 1, 2*args.resident_frames)
                              +b''.join(cold_requests[state] for _ in range(args.resident_frames)
                                        for state in ('assembled', 'exploded')))
    for label, backend, threads, device in BACKENDS:
        check_inputs()
        name = 'resident-'+label
        output = args.output/name
        command = native_command(output, label, backend, threads, device, resident_views)
        with (args.output/(name+'.log')).open('w') as log:
            subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
        request_bytes = resident_views.read_bytes()
        if len(request_bytes) != 16+192*2*args.resident_frames:
            raise ValueError('resident view count/size differs')
        for frame in range(2*args.resident_frames):
            if request_bytes[16+192*frame:16+192*(frame+1)] != cold_requests[('assembled', 'exploded')[frame % 2]]:
                raise ValueError('resident and cold actual packed view requests differ')
        for row in read_rows(output/'timings.tsv'):
            if row['stage'] == 'reused_frame_wall':
                frame = int(row['frame'])
                records.append(('resident_native_frame', frame//2,
                                ('assembled', 'exploded')[frame % 2], label,
                                float(row['milliseconds'])/1000,
                                'retained scene/source jets; current placement/bins/image/download/raw writes/host frame cleanup'))
        outputs.append((name, 'both', label, output.name))
        observations()
    check_inputs()
    if hashlib.sha256(script.read_bytes()).hexdigest() != pin:
        raise ValueError('caller changed during benchmark')
    if hashlib.sha256(driver.read_bytes()).hexdigest() != driver_pin:
        raise ValueError('benchmark driver changed during benchmark')
    if tuple(hashlib.sha256(p.read_bytes()).hexdigest() for p in binary_paths) != binary_hashes:
        raise ValueError('selected adapter/native/plugin binaries changed during benchmark')
    write_rows(args.output/'observations.tsv', ('scope', 'repeat', 'view', 'backend', 'seconds', 'description'), records)
    grouped = {}
    for scope, _, view, backend, elapsed, _ in records:
        grouped.setdefault((scope, view, backend), []).append(elapsed)
    write_rows(args.output/'summary.tsv', ('scope', 'view', 'backend', 'n', 'median_seconds', 'min_seconds', 'max_seconds'),
        ((scope, view, backend, len(values), statistics.median(values), min(values), max(values))
         for (scope, view, backend), values in sorted(grouped.items())))
    write_rows(args.output/'outputs.tsv', ('run', 'view', 'backend', 'relative_directory'), outputs)
    write_rows(args.output/'benchmark-pins.tsv', ('role', 'sha256'),
        (('benchmark_driver', driver_pin), ('complete_caller', pin),
         *((role, value) for role, value in zip(('source_adapter', 'native_executable', 'cuda_plugin'), binary_hashes))))


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--step', required=True, type=Path)
    p.add_argument('--output', required=True, type=Path)
    p.add_argument('--adapter', type=Path, default=ROOT/'build/om10-step-source/brepgpu-step-source')
    p.add_argument('--native', type=Path, default=ROOT/'build/om10-release/brepgpu-om10-native')
    p.add_argument('--plugin', type=Path, default=ROOT/'build/om10-release/libbrepgpu_om10_cuda.so')
    p.add_argument('--repeats', type=int, default=3)
    p.add_argument('--resident-frames', type=int, default=3)
    p.add_argument('--width', type=int, default=2400)
    p.add_argument('--height', type=int, default=2000)
    p.add_argument('--supersample', type=int, choices=(1, 2), default=2)
    p.add_argument('--tolerance', type=float, default=.005)
    p.add_argument('--source-residual', type=float, default=.001)
    p.add_argument('--azimuth', type=float, default=-58.)
    p.add_argument('--elevation', type=float, default=52.)
    main(p.parse_args())
