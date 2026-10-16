"""Qualify actual signed-cap fixture output without loading an instrumented DSO."""
from __future__ import annotations

import argparse
from collections import defaultdict
import csv
from fractions import Fraction as F
import io
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path[:0] = [str(ROOT/'python'), str(ROOT/'tests')]
from brepgpu.imported_oracles import _simple_domains, turn
from direct_cdt_oracle import _determinant, qualify_cdt
from face_domain_external_cases import ExternalCase


BOUNDARIES = {
    'cap-clockwise': ((50, 50), (0, 1), (-50, 50), (-50, -50), (50, -50)),
    'cap-vertical': ((50, 50), (10, 50), (-10, 50), (-50, 50), (-50, -50), (50, -50)),
}


def admitted_old_star(name):
    """Independent actual precondition: a valid exact DT star before deletion."""
    boundary = tuple(tuple(map(F, point)) for point in BOUNDARIES[name])
    _simple_domains((boundary,), (True,))
    apex = (F(0), F(0))
    for a, b in zip(boundary, boundary[1:]+boundary[:1]):
        if turn(apex, a, b) <= 0:
            raise AssertionError('old DT star requires positive original fan triangles')
        for site in boundary:
            rows = [(x, y, x*x+y*y, F(1)) for x, y in (apex, a, b, site)]
            if _determinant(rows) > 0:
                raise AssertionError('old fan is not Delaunay against actual original sites')
    return ExternalCase(name, (tuple(range(len(boundary))),), tuple(enumerate(boundary)))


def qualify_output(text):
    reader = csv.DictReader(io.StringIO(text), delimiter='\t')
    if reader.fieldnames != ['case', 'seed', 'kind', 'a', 'b', 'c']:
        raise AssertionError('signed-cap fixture output schema differs')
    groups = defaultdict(lambda: {'triangles': [], 'stats': None})
    for row in reader:
        if row['case'] not in BOUNDARIES:
            raise AssertionError('signed-cap output has an unknown original domain')
        seed = int(row['seed'])
        if not 0 <= seed < 16:
            raise AssertionError('signed-cap output seed is outside the selected cohort')
        group = groups[row['case'], seed]
        values = tuple(int(row[key]) for key in ('a', 'b', 'c'))
        if row['kind'] == 'triangle':
            group['triangles'].append(values)
        elif row['kind'] == 'stats' and group['stats'] is None:
            if any(value < 0 for value in values):
                raise AssertionError('signed-cap branch counters must be nonnegative')
            group['stats'] = values
        else:
            raise AssertionError('signed-cap output kind/duplicate statistics differs')
    expected = {(name, seed) for name in BOUNDARIES for seed in range(16)}
    if set(groups) != expected:
        raise AssertionError('signed-cap fixture omitted or added a selected case/seed')
    cases = {name: admitted_old_star(name) for name in BOUNDARIES}
    clockwise = vertical = 0
    for (name, seed), group in groups.items():
        if group['stats'] is None:
            raise AssertionError(f'signed-cap output omitted statistics for {(name, seed)}')
        qualify_cdt(cases[name], group['triangles'])
        cw, zero, _ = group['stats']
        clockwise += cw
        vertical += zero
    if clockwise == 0 or vertical == 0:
        raise AssertionError('selected signed-cap runs did not execute both signed seed branches')
    return len(groups), clockwise, vertical


def main(args):
    result = subprocess.run([str(args.fixture.resolve())], text=True, capture_output=True, timeout=30)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(result.stdout)
    args.output.with_suffix(args.output.suffix+'.stderr').write_text(result.stderr)
    if result.returncode:
        raise RuntimeError(f'native signed-cap fixture failed with exit {result.returncode}; '
                           f'stderr retained in {args.output.name}.stderr')
    count, clockwise, vertical = qualify_output(result.stdout)
    print(f'PASS {count} actual signed-cap outputs; clockwise seeds {clockwise}; vertical seeds {vertical}')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fixture', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    main(parser.parse_args())
