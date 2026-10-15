#!/usr/bin/env python3
"""Compose final SVG posters from actual matched OM10 benchmark images/tables.

Run on the fleet. Existing static SVG-to-PNG conversion is separately timed.
No geometry is generated, modified, inferred or retimed by this compositor.
"""
import argparse
import csv
from pathlib import Path
import shutil
import statistics
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'examples'))
from om10_scene import View, labeled_svg


def rows(path):
    with Path(path).open(newline='', encoding='utf-8') as f:
        return tuple(csv.DictReader(f, delimiter='\t'))


def view_from_bytes(path):
    data = Path(path).read_bytes()
    if data[:16] != struct.pack('<8sII', b'BGVIEW01', 1, 1) or len(data) != 208:
        raise ValueError('poster needs an actual single-view cold request')
    r = struct.unpack_from('<4I19dII2d', data, 16)
    return View(r[0], r[1], r[2], r[4], r[5:14], r[14:17], r[17:19], r[19], r[20:23], r[23])


def main(args):
    if sys.platform != 'linux':
        raise ValueError('OM10 presentation composition executes on the fleet')
    args.output.mkdir(parents=True, exist_ok=False)
    summary = {(r['scope'], r['view'], r['backend']): r for r in rows(args.benchmark/'summary.tsv')}
    observations = rows(args.benchmark/'observations.tsv')
    outputs = rows(args.benchmark/'outputs.tsv')
    for state in ('assembled', 'exploded'):
        # Fixed first GPU0 image; displayed times are all-repeat medians. There
        # is no image/timing repeat search or choice of the fastest observation.
        selected = args.benchmark/f'cold-00-{state}-p100-0'
        view = view_from_bytes(selected/'views.bgview')
        png = selected/f'000-{state}.png'
        metadata = {r['field']: r['value'] for r in rows(selected/'metadata.tsv')}
        tolerance_um = float(metadata['tolerance_mm'])*1000
        callout_rows = rows(selected/f'000-{state}-callouts.tsv')
        callouts = tuple((r['label'], float(r['label_x']), float(r['label_y']),
                          float(r['anchor_x']), float(r['anchor_y']),
                          'start' if float(r['label_x']) < view.width/2 else 'end') for r in callout_rows)
        panel = []
        for backend, title in (('cpu1', 'CPU1'), ('cpu24', 'CPU24'), ('p100-0', 'P100 / 0'), ('p100-1', 'P100 / 1')):
            native = []
            for row in outputs:
                if row['run'].startswith('cold-') and row['view'] == state and row['backend'] == backend:
                    native += [float(t['seconds']) for t in rows(args.benchmark/row['relative_directory']/'timings.tsv')
                               if t['stage'] == 'complete_native_process_including_exit']
            if len(native) != len({r['repeat'] for r in observations
                                  if r['scope'] == 'cold_complete_process' and r['view'] == state and r['backend'] == backend}):
                raise ValueError('complete matching native/cold observations are required')
            cold = float(summary['cold_complete_process', state, backend]['median_seconds'])
            panel.append((title, f'{statistics.median(native):.3f} / {cold:.3f} s'))
        n = summary['cold_complete_process', state, 'p100-0']['n']
        labeled_svg(args.output/(state+'.svg'), png, view,
            title='OM10 / '+state.upper(),
            subtitle='CPU STEP + owned trim meshing · CPU / P100 first jets, placement + raster',
            counts=(('source solids', 107), ('source faces', '9,162'), ('placed parts', 166)),
            timings=panel, timing_header=f'NATIVE PROCESS / FULL STEP→IMAGE · MEDIAN OF {n}',
            callouts=callouts,
            attribution=f'OpenMovement OM10 MAN VA01.3 · {tolerance_um:g} μm owned mesh target · full wall ends at PNG + labeled SVG; final SVG rasterization separate')
        shutil.copyfile(selected/f'000-{state}-callouts.tsv', args.output/(state+'-callouts.tsv'))
        # Preserve actual CPU and GPU pixels as comparison artifacts too.
        for label in ('cpu24', 'p100-0'):
            shutil.copyfile(args.benchmark/f'cold-00-{state}-{label}'/f'000-{state}.png',
                            args.output/(state+'-'+label+'-native.png'))
    for name in ('summary.tsv', 'observations.tsv', 'benchmark-pins.tsv'):
        shutil.copyfile(args.benchmark/name, args.output/name)


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--benchmark', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    main(p.parse_args())
