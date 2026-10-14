#!/usr/bin/env python3
"""Render owned SVG comparisons from retained native discovery summaries.

No smoothing, extrapolation or hidden sample selection. The figures show the
same depth, requested tolerance and method, or explicitly compare precision
populations when methods differ. SVG generation uses only the standard library.
"""
import argparse
import csv
from html import escape
import math
from pathlib import Path


COLORS = {"CPU1": "#a9bed0", "CPU24": "#52b5ee", "P100 0": "#7ddaaa", "P100 1": "#d7a7ff"}
MODES = {0: "AABB", 1: "26-DOP", 2: "Bernstein"}
CORPORA = ("plane", "diagonal", "cylinder", "axis", "parabolic", "twins", "capped")


def label(row):
    return "CPU" + row["threads"] if row["backend"] == "cpu" else "P100 " + row["device"]


def condition(row):
    return (int(row['max_visits']), int(row['max_refinements']),
            float(row['tolerance_squared']).hex())


def operation_key(row):
    return (row['corpus'], row['mode'], row['depth'], row['count'], *condition(row))


def unique_rows(rows):
    indexed = {}
    for row in rows:
        key = (label(row), *operation_key(row), row['phase'])
        if key in indexed:
            raise ValueError('duplicate complete operation/phase summary')
        indexed[key] = row
    return indexed


def seven_samples(rows):
    if any(int(row['samples']) != 7 for row in rows):
        raise ValueError('published comparison requires seven recorded samples')


def text(x, y, value, size=15, color="#edf4fc", anchor="start"):
    return f'<text x="{x:.2f}" y="{y:.2f}" fill="{color}" font-size="{size}" text-anchor="{anchor}">{escape(str(value))}</text>'


def svg(width, height, title, subtitle, content):
    return ('<svg xmlns="http://www.w3.org/2000/svg" '
            f'width="{width}" height="{height}" viewBox="0 0 {width} {height}">'
            f'<title>{escape(title)}</title><desc>{escape(subtitle)}</desc>'
            '<rect width="100%" height="100%" rx="16" fill="#101923"/>'
            '<g font-family="system-ui,sans-serif">' + text(32, 42, title, 24)
            + text(32, 69, subtitle, 13, "#a9bed0") + content + '</g></svg>\n')


def timing(rows, output, mode=2, corpus="cylinder"):
    selected = [r for r in rows if r["phase"] == "resident_discover"
                and int(r["mode"]) == mode and int(r["depth"]) == 8 and r["corpus"] == corpus]
    if not selected:
        raise ValueError("no common-depth timing rows")
    seven_samples(selected)
    conditions = {condition(r) for r in selected}
    if len(conditions) != 1:
        raise ValueError('timing chart mixes requested budgets or tolerances')
    visits, refinements, tolerance = next(iter(conditions))
    counts = sorted({int(r["count"]) for r in selected})
    if len(counts) < 2:
        raise ValueError("timing chart requires multiple actual batch sizes")
    values = [float(r[k]) for r in selected for k in ("min_ms", "max_ms")]
    if min(values) <= 0:
        raise ValueError("log timing chart requires positive recorded times")
    lo = math.floor(math.log10(min(values)))
    hi = math.ceil(math.log10(max(values)))
    if hi == lo:
        hi += 1
    left, right, top, bottom = 90, 780, 125, 440
    xlo, xhi = math.log10(counts[0]), math.log10(counts[-1])
    xp = lambda n: left + (math.log10(n) - xlo) * (right - left) / (xhi - xlo)
    yp = lambda ms: bottom - (math.log10(ms) - lo) * (bottom - top) / (hi - lo)
    content = ''
    for exponent in range(lo, hi + 1):
        y = yp(10 ** exponent)
        content += f'<path d="M {left} {y} H {right}" stroke="#294051"/>'
        content += text(left - 12, y + 5, f'{10 ** exponent:g}', 12, '#a9bed0', 'end')
    for n in counts:
        x = xp(n)
        content += f'<path d="M {x} {top} V {bottom}" stroke="#233544"/>'
        content += text(x, bottom + 24, f'{n:,}', 12, '#a9bed0', 'middle')
    content += text(90, 108, 'milliseconds · log scale', 12, '#a9bed0')
    content += text((left + right) / 2, bottom + 49, 'queries · log scale', 13, '#a9bed0', 'middle')
    series = [(backend, color) for backend, color in COLORS.items()
              if any(label(r) == backend for r in selected)]
    for index, (backend, color) in enumerate(series):
        batch = sorted([r for r in selected if label(r) == backend], key=lambda r: int(r['count']))
        if len(batch) != len(counts):
            raise ValueError(f'incomplete series {backend}')
        points = ' '.join(f'{xp(int(r["count"])):.2f},{yp(float(r["median_ms"])):.2f}' for r in batch)
        content += f'<polyline points="{points}" fill="none" stroke="{color}" stroke-width="2.5"/>'
        for r in batch:
            x = xp(int(r['count']))
            a, b = yp(float(r['min_ms'])), yp(float(r['max_ms']))
            content += f'<path d="M {x} {a} V {b}" stroke="{color}" stroke-width="2"/>'
            content += f'<circle cx="{x}" cy="{yp(float(r["median_ms"]))}" r="4" fill="{color}"/>'
        last = batch[-1]
        attained = f'{int(float(last["gap_rows_min"])):,}/{int(last["count"]):,} GAP'
        content += text(812, 155 + 57 * index, backend, 16, color)
        content += text(812, 175 + 57 * index, f'{float(last["median_ms"]):.3f} ms', 13)
        content += text(812, 193 + 57 * index, attained, 11, '#a9bed0')
    content += text(32, 521, 'Seven samples after excluded warm-up; points are medians, vertical marks are observed ranges.', 12, '#a9bed0')
    content += text(32, 542, 'Native result allocation + traversal/refinement + synchronization. Download and consumption are separate.', 12, '#a9bed0')
    output.write_text(svg(1060, 564, f'Resident discovery · {corpus} · {MODES[mode]}',
                          f'Depth8 · squared tolerance {float.fromhex(tolerance):g} · visits {visits} · refinements {refinements} · largest-batch precision in legend', content))


def precision(rows, output, count=4096, corpora=CORPORA):
    selected = [r for r in rows if r['phase'] == 'resident_discover'
                and int(r['depth']) == 8 and int(r['count']) == count
                and label(r) in ('CPU24', 'P100 0') and r['corpus'] in corpora]
    seven_samples(selected)
    by_key = unique_rows(selected)
    conditions = {}
    for corpus in corpora:
        admitted = {condition(r) for r in selected if r['corpus'] == corpus}
        if len(admitted) != 1:
            raise ValueError('precision chart mixes requested budgets or tolerances for ' + corpus)
        conditions[corpus] = next(iter(admitted))
    tolerances = {c[2] for c in conditions.values()}
    refinement_limits = {c[1] for c in conditions.values()}
    if len(tolerances) != 1 or len(refinement_limits) != 1:
        raise ValueError('precision chart requires one common tolerance/refinement limit')
    content = ''
    cell_width, cell_height = 147, 66
    for panel, backend in enumerate(('CPU24', 'P100 0')):
        start = 170 + panel * 470
        content += text(start, 112, backend, 18, COLORS[backend])
        for mode in range(3):
            content += text(start + mode * cell_width + 68, 140, MODES[mode], 14, '#edf4fc', 'middle')
        for i, corpus in enumerate(corpora):
            y = 154 + i * cell_height
            if panel == 0:
                content += text(32, y + 31, corpus, 15)
            for mode in range(3):
                r = by_key[(backend, corpus, str(mode), '8', str(count),
                            *conditions[corpus], 'resident_discover')]
                fraction = float(r['gap_rows_min']) / count
                x = start + mode * cell_width
                content += f'<rect x="{x}" y="{y}" width="138" height="57" rx="7" fill="#386079" fill-opacity="{.15 + .8 * fraction:.3f}"/>'
                content += text(x + 9, y + 22, f'{float(r["median_ms"]):.3f} ms', 14)
                content += text(x + 9, y + 36, f'{int(float(r["gap_rows_min"])):,}/{count:,} GAP', 11, '#a9bed0')
                content += text(x + 9, y + 50, f'mean width {float(r["mean_gap_max"]):.2g}', 10, '#a9bed0')
    footer = 188 + len(corpora) * cell_height
    content += text(32, footer, 'Brighter cells mean more gap-qualified rows. Runtime and achieved precision participate in method choice.', 12, '#a9bed0')
    visit_limits = ', '.join(str(n) for n in sorted({c[0] for c in conditions.values()}))
    content += text(32, footer + 21, f'Recorded visit budgets: {visit_limits}; refinements {next(iter(refinement_limits))}. Unresolved remains a valid outcome.', 12, '#a9bed0')
    output.write_text(svg(1110, footer + 45, 'What stronger geometry bounds buy',
                          f'{count:,} queries · depth8 · squared tolerance {float.fromhex(next(iter(tolerances))):g} · resident median and minimum GAP population', content))


def comparisons(rows, output):
    phases = ('resident_discover', 'cached_hierarchy_host_to_host')
    by_key = unique_rows(rows)
    fields = ('corpus', 'mode', 'depth', 'count', 'max_visits', 'max_refinements',
              'tolerance_squared', 'phase', 'gpu', 'cpu24_median_ms',
              'gpu_median_ms', 'cpu24_over_gpu', 'cpu24_gap_min', 'gpu_gap_min',
              'cpu24_coverage_min', 'gpu_coverage_min', 'gpu_cache_bytes',
              'cpu24_mean_gap_min', 'cpu24_mean_gap_max', 'gpu_mean_gap_min', 'gpu_mean_gap_max',
              'cpu24_max_gap_max', 'gpu_max_gap_max', 'extra_hierarchy_setup_ms',
              'estimated_hierarchy_reuse_calls', 'hierarchy_estimate_scope')
    with output.open('w', newline='') as stream:
        writer = csv.DictWriter(stream, fields, delimiter='\t', lineterminator='\n')
        writer.writeheader()
        for r in rows:
            gpu = label(r)
            if not gpu.startswith('P100') or r['phase'] not in phases:
                continue
            key = operation_key(r)
            cpu = by_key[('CPU24', *key, r['phase'])]
            seven_samples((r, cpu))
            ct, gt = float(cpu['median_ms']), float(r['median_ms'])
            csetup = by_key[('CPU24', *key, 'hierarchy_build_upload')]
            gsetup = by_key[(gpu, *key, 'hierarchy_build_upload')]
            seven_samples((csetup, gsetup))
            extra = max(0., float(gsetup['median_ms']) - float(csetup['median_ms']))
            # This estimate compares independent medians and hierarchy setup
            # only; it is not a measured first-use host operation.
            amortization = 'no savings' if ct <= gt else str(math.ceil(extra / (ct - gt)))
            writer.writerow(dict(corpus=r['corpus'], mode=r['mode'], depth=r['depth'], count=r['count'],
                max_visits=r['max_visits'], max_refinements=r['max_refinements'], tolerance_squared=r['tolerance_squared'],
                phase=r['phase'], gpu=gpu, cpu24_median_ms=ct, gpu_median_ms=gt, cpu24_over_gpu=ct / gt,
                cpu24_gap_min=cpu['gap_rows_min'], gpu_gap_min=r['gap_rows_min'],
                cpu24_coverage_min=cpu['coverage_rows_min'], gpu_coverage_min=r['coverage_rows_min'],
                cpu24_mean_gap_min=cpu['mean_gap_min'], cpu24_mean_gap_max=cpu['mean_gap_max'],
                gpu_mean_gap_min=r['mean_gap_min'], gpu_mean_gap_max=r['mean_gap_max'],
                cpu24_max_gap_max=cpu['max_gap_max'], gpu_max_gap_max=r['max_gap_max'],
                gpu_cache_bytes=r['cache_bytes_min'], extra_hierarchy_setup_ms=extra,
                estimated_hierarchy_reuse_calls=amortization,
                hierarchy_estimate_scope='hierarchy_only_independent_medians_excludes_first_use_patch_setup'))


def main():
    parser = argparse.ArgumentParser(__doc__)
    parser.add_argument('--summary', type=Path, required=True)
    parser.add_argument('--offgrid-summary', type=Path)
    parser.add_argument('--output-directory', type=Path, required=True)
    args = parser.parse_args()
    with args.summary.open(newline='') as stream:
        rows = list(csv.DictReader(stream, delimiter='\t'))
    unique_rows(rows)
    args.output_directory.mkdir(parents=True, exist_ok=True)
    timing(rows, args.output_directory / 'resident-discovery.svg')
    precision(rows, args.output_directory / 'bound-methods.svg')
    comparisons(rows, args.output_directory / 'comparisons.tsv')
    if args.offgrid_summary:
        with args.offgrid_summary.open(newline='') as stream:
            offgrid = list(csv.DictReader(stream, delimiter='\t'))
        unique_rows(offgrid)
        precision(offgrid, args.output_directory / 'offgrid-bounds.svg',
                  corpora=('cylinder_offgrid', 'axis_offgrid'))
        comparisons(offgrid, args.output_directory / 'offgrid-comparisons.tsv')
    (args.output_directory / 'FIGURES.md').write_text(
        '# Discovery figure and comparison scope\n\n'
        'Owned SVGs read retained native summaries, with no smoothing or extrapolation. '
        'Seven recorded samples are required. Timing compares one complete requested '
        'budget/tolerance condition; method panels reject mixed conditions within each corpus. '
        'Comparisons pair CPU24 and a P100 by corpus, mode, depth, count, visits, refinements, '
        'squared tolerance and named phase. Attained gap populations, coverage and widths are '
        'retained alongside each ratio; equal requested conditions do not imply identical '
        'attained precision.\n\n'
        'Estimated hierarchy reuse calls compare the positive difference of independent '
        'hierarchy-build/upload medians with the saving per separately timed operation. '
        'This is a hierarchy-only conditional estimate, excluding first use, patch setup '
        'and other application costs. Independent medians are not additive measured calls; '
        'a row without operation savings has no break-even. Resident estimates additionally '
        'assume reused resident query buffers and exclude download/consumption; host-to-host '
        'estimates include a new query snapshot, download and native consumption each call.\n')


if __name__ == '__main__':
    main()
