#!/usr/bin/env python3
"""Render every matched CUDA specialization case from retained comparison TSV."""
import argparse
import csv
from html import escape
import math
from pathlib import Path


PHASES = ('resident_discover', 'cached_hierarchy_host_to_host')
MODES = ('AABB', '26-DOP', 'Bernstein')
CONDITION = ('count', 'corpus', 'mode', 'depth', 'max_visits',
             'max_refinements', 'tolerance_squared')


def text(x, y, value, size=13, color='#edf4fc'):
    return (f'<text x="{x}" y="{y}" font-size="{size}" fill="{color}">'
            f'{escape(str(value))}</text>')


def render(rows, path):
    selected = [row for row in rows if row['phase'] in PHASES]
    if not selected:
        raise ValueError('no matched native operation phases')
    indexed, cases = {}, {}
    for row in selected:
        case = int(row['case_id'])
        key = (case, row['device'], row['phase'])
        if key in indexed:
            raise ValueError('duplicate case/card/native phase')
        if row['backend'] != 'cuda' or row['threads'] != '1':
            raise ValueError('expected the reviewed individual-card comparison')
        condition = tuple(row[k] for k in CONDITION)
        if case in cases and cases[case] != condition:
            raise ValueError('case ID mixes requested conditions')
        cases[case] = condition
        if row['quality_equal'] != '1' or row['work_equal'] != '1':
            raise ValueError('variant quality or work differs')
        for variant in ('dynamic', 'specialized'):
            if row[variant + '_samples'] != '7':
                raise ValueError('each native phase requires seven samples')
            a, m, b = (float(row[variant + '_' + k])
                       for k in ('min_ms', 'median_ms', 'max_ms'))
            if not all(math.isfinite(x) for x in (a, m, b)) or not 0 < a <= m <= b:
                raise ValueError('native times must be finite, positive and ordered')
        indexed[key] = row
    devices = sorted({row['device'] for row in selected}, key=int)
    columns = [(device, phase) for device in devices for phase in PHASES]
    for case in cases:
        if any((case, *column) not in indexed for column in columns):
            raise ValueError('incomplete case/card/native phase matrix')
    width, cell_width, row_height = 350 + 230 * len(columns), 222, 60
    height = 192 + row_height * len(cases)
    content = text(28, 38, 'CUDA mode specialization: measured native cost', 24)
    content += text(28, 64, 'All matched cases · seven samples · positive percentages mean lower median time', 13, '#a9bed0')
    for j, (device, phase) in enumerate(columns):
        x = 335 + 230 * j
        content += text(x, 99, 'P100 ' + device, 16)
        content += text(x, 119, 'resident' if phase == PHASES[0] else 'cached host-to-host', 12, '#a9bed0')
    for i, case in enumerate(sorted(cases)):
        count, corpus, mode, depth, visits, refs, tolerance = cases[case]
        y = 137 + i * row_height
        content += text(28, y + 20, f'{case:02d}  {corpus} · {int(count):,}', 13)
        content += text(28, y + 40, f'{MODES[int(mode)]} · depth {depth} · visits {visits} · refs {refs}', 11, '#a9bed0')
        for j, (device, phase) in enumerate(columns):
            row = indexed[(case, device, phase)]
            before, after = (float(row[v + '_median_ms']) for v in ('dynamic', 'specialized'))
            improvement = 100 * (before - after) / before
            color = '#3c8c70' if improvement >= 5 else '#a56347' if improvement <= -5 else '#385a73'
            title = (f'case {case}; card {device}; {phase}; squared tolerance {tolerance}; '
                     f'dynamic {before:g} ms [{row["dynamic_min_ms"]}, {row["dynamic_max_ms"]}]; '
                     f'specialized {after:g} ms [{row["specialized_min_ms"]}, {row["specialized_max_ms"]}]; '
                     f'GAP {row["dynamic_gap_rows_min"]}/{count}, '
                     f'max width {row["dynamic_max_gap_max"]}; quality/work equal')
            x = 335 + 230 * j
            content += (f'<g><title>{escape(title)}</title><rect x="{x}" y="{y}" '
                        f'width="{cell_width}" height="52" rx="7" fill="{color}"/>')
            content += text(x + 10, y + 22, f'{improvement:+.1f}%', 17)
            content += text(x + 10, y + 41, f'{before:.3f} → {after:.3f} ms', 12)
            content += '</g>'
    content += text(28, height - 31, 'Dynamic → specialized medians. Cell titles retain observed ranges and attained precision; ranges are not confidence intervals.', 12, '#a9bed0')
    content += text(28, height - 12, 'Setup and first use are separate. Independent medians are not additive. Reverse-order confirmation decides adoption.', 12, '#a9bed0')
    path.write_text(f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" viewBox="0 0 {width} {height}">'
                    '<title>CUDA discovery mode specialization measurements</title>'
                    '<rect width="100%" height="100%" rx="16" fill="#101923"/>'
                    '<g font-family="system-ui,sans-serif">' + content + '</g></svg>\n')


def main():
    parser = argparse.ArgumentParser(__doc__)
    parser.add_argument('--comparison', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    with args.comparison.open(newline='') as stream:
        rows = list(csv.DictReader(stream, delimiter='\t'))
    render(rows, args.output)


if __name__ == '__main__':
    main()
