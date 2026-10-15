#!/usr/bin/env python3
"""Compose final SVG posters from actual matched OM10 benchmark images/tables.

Run on the fleet. Existing static SVG-to-PNG conversion is separately timed.
No geometry is generated, modified, inferred or retimed by this compositor.
"""
import argparse
import csv
from pathlib import Path
import shutil
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path[:0] = [str(ROOT/'python'), str(ROOT/'examples')]
from om10_scene import View, labeled_svg


def rows(path):
    with Path(path).open(newline='', encoding='utf-8') as f:
        return tuple(csv.DictReader(f, delimiter='\t'))


def view_from_bytes(path, index):
    data = Path(path).read_bytes()
    if data[:16] != struct.pack('<8sII', b'BGVIEW01', 1, 2) or len(data) != 408:
        raise ValueError('poster needs the actual two-view complete-caller request')
    for frame in range(2):
        if (struct.unpack_from('<d', data, 32+192*frame)[0] != float(frame) or
                struct.unpack_from('<I', data, 184+192*frame)[0] != 0):
            raise ValueError('poster requests must render assembled alpha0 then exploded alpha1')
    r = struct.unpack_from('<4I19dII2d', data, 16+192*index)
    return View(r[0], r[1], r[2], r[4], r[5:14], r[14:17], r[17:19], r[19], r[20:23], r[23])


def main(args):
    if sys.platform != 'linux':
        raise ValueError('OM10 presentation composition executes on the fleet')
    args.output.mkdir(parents=True, exist_ok=False)
    summary = {(r['scope'], r['view'], r['backend']): r for r in rows(args.benchmark/'summary.tsv')}
    for index, state in enumerate(('assembled', 'exploded')):
        # Fixed full-task GPU0 image; native times are all-repeat medians. There
        # is no image/timing repeat search or choice of the fastest observation.
        selected = args.benchmark/'cold-00-both-p100-0'
        view = view_from_bytes(selected/'views.bgview', index)
        frame_name = f'{index:03d}-{state}'
        png = selected/(frame_name+'.png')
        metadata = {r['field']: r['value'] for r in rows(selected/'metadata.tsv')}
        tolerance_um = float(metadata['tolerance_mm'])*1000
        callout_rows = rows(selected/(frame_name+'-callouts.tsv'))
        callouts = tuple((r['label'], float(r['label_x']), float(r['label_y']),
                          float(r['anchor_x']), float(r['anchor_y']),
                          'start' if float(r['label_x']) < view.width/2 else 'end') for r in callout_rows)
        panel = []
        for backend, title in (('cpu1', 'CPU1'), ('cpu24', 'CPU24'), ('p100-0', 'P100 / 0'), ('p100-1', 'P100 / 1')):
            native = summary['native_complete_process', state, backend]
            if int(native['n']) < 3:
                raise ValueError('poster requires repeated matched native process observations')
            panel.append((title, f"{float(native['median_seconds']):.3f} s"))
        task_panel = []
        for backend, title in (('cpu24', 'CPU24'), ('p100-0', 'P100 / 0')):
            full = summary['cold_complete_process', 'both', backend]
            if int(full['n']) != 1:
                raise ValueError('full task panel describes one actual observation')
            task_panel.append((title, f"{float(full['median_seconds']):.3f} s"))
        native_counts = {summary['native_complete_process', state, backend]['n']
                         for backend in ('cpu1','cpu24','p100-0','p100-1')}
        if len(native_counts) != 1:
            raise ValueError('native backends have different repeat counts')
        n = native_counts.pop()
        labeled_svg(args.output/(state+'.svg'), png, view,
            title='OM10 / '+state.upper(),
            subtitle='CPU STEP + owned trim meshing · CPU / P100 first jets, placement + raster',
            counts=(('source solids', 107), ('source faces', '9,162'), ('placed parts', 166)),
            timings=panel, timing_header=f'THIS POSE · NATIVE PROCESS · MEDIAN n={n}',
            task_timings=task_panel,
            task_timing_header='FRESH STEP → BOTH LABELED VIEWS · n=1 · SHARED TOTAL',
            callouts=callouts,
            attribution=f'Association openmovement / OM10 MAN VA01.3 · CC BY-SA 3.0 · {tolerance_um:g} μm mesh target · native: prepared scene → raw image · final SVG rasterization separate')
        shutil.copyfile(selected/(frame_name+'-callouts.tsv'), args.output/(state+'-callouts.tsv'))
        # Preserve actual CPU and GPU pixels as comparison artifacts too.
        for label in ('cpu24', 'p100-0'):
            shutil.copyfile(args.benchmark/f'cold-00-both-{label}'/(frame_name+'.png'),
                            args.output/(state+'-'+label+'-native.png'))
    for name in ('summary.tsv', 'observations.tsv', 'benchmark-pins.tsv'):
        shutil.copyfile(args.benchmark/name, args.output/name)


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--benchmark', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    main(p.parse_args())
