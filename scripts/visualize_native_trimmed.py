#!/usr/bin/env python3
"""Draw one external SVG view of existing complete selected-face BGATRM files.

This reads saved native XYZ/connectivity through the existing output reader. It
neither evaluates a carrier nor constructs geometry. Run on the fleet after the
benchmark has finished; it does not invoke a native operation or GPU renderer.
"""
from pathlib import Path
import argparse
from collections import Counter
import hashlib
from html import escape
import math
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path[:0] = [str(ROOT / 'python'), str(ROOT / 'tests')]
from native_adaptive_trimmed_caller import read_face

PANELS = (('plane_many_holes', 'Plane / 16 holes'),
          ('bowl', 'Quadratic bowl'),
          ('weighted', 'Weighted rational patch'),
          ('saddle', 'Bilinear saddle'))
PALETTE = {1: '#8ac6d1', 2: '#efb16e'}
WIDTH, HEIGHT = 1280, 1080
PANEL_WIDTH, PANEL_HEIGHT = 594, 420


def dot(a, b):
    return sum(x*y for x, y in zip(a, b))


def unit(v):
    n = math.sqrt(dot(v, v))
    return tuple(x/n for x in v)


def cross(a, b):
    return (a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2],
            a[0]*b[1]-a[1]*b[0])


def panel(mesh, family, label, x, y):
    xyz = [tuple(map(float, site['xyz'])) for site in mesh['sites']]
    # A fixed orthographic camera per view class; no fitted surface or height
    # exaggeration. The planar panel is overhead so all holes remain legible.
    view = (0., 0., 1.) if family == 'plane_many_holes' else unit((1.7, -2.8, 6.0))
    right = (1., 0., 0.) if family == 'plane_many_holes' else unit(cross((0., 0., 1.), view))
    up = cross(view, right)
    projected = [(dot(p, right), -dot(p, up), dot(p, view)) for p in xyz]
    low = tuple(min(p[a] for p in projected) for a in (0, 1))
    high = tuple(max(p[a] for p in projected) for a in (0, 1))
    plot = (x+35, y+72, PANEL_WIDTH-70, PANEL_HEIGHT-98)
    scale = min(plot[2]/(high[0]-low[0]), plot[3]/(high[1]-low[1]))
    center = ((low[0]+high[0])/2, (low[1]+high[1])/2)
    screen = [(plot[0]+plot[2]/2+(p[0]-center[0])*scale,
               plot[1]+plot[3]/2+(p[1]-center[1])*scale) for p in projected]
    counts = Counter(facet['key'][0] for facet in mesh['facets'])
    total = len(mesh['facets'])
    lines = [f'<g aria-label="{escape(label)}: {total} actual triangles">',
             f'<rect x="{x}" y="{y}" width="{PANEL_WIDTH}" height="{PANEL_HEIGHT}" '
             'rx="14" fill="#ffffff" stroke="#dae2e8"/>',
             f'<text x="{x+22}" y="{y+29}" class="panel-title">{escape(label)}</text>',
             f'<text x="{x+22}" y="{y+51}" class="count">{total:,} triangles '
             f'· {counts[1]:,} packet / {counts[2]:,} band</text>']
    # Painter ordering changes only presentation; every polygon uses the three
    # actual saved site positions and the saved facet generator.
    ordered = sorted(enumerate(mesh['facets']),
                     key=lambda row: (sum(projected[i][2] for i in row[1]['sites']), row[0]))
    for _, facet in ordered:
        points = ' '.join(f'{screen[i][0]:.3f},{screen[i][1]:.3f}' for i in facet['sites'])
        lines.append(f'<polygon points="{points}" fill="{PALETTE[facet["key"][0]]}" '
                     'stroke="#39535f" stroke-width="0.55" stroke-opacity="0.66" '
                     'stroke-linejoin="round"/>')
    # The saved source-edge constraints outline physical trims; packet rims
    # retain the ordinary triangle stroke rather than looking like source edges.
    for edge in mesh['edges']:
        if edge['kind'] != 1:
            continue
        a, b = (screen[i] for i in edge['sites'])
        lines.append(f'<path d="M{a[0]:.3f},{a[1]:.3f} L{b[0]:.3f},{b[1]:.3f}" '
                     'fill="none" stroke="#213744" stroke-width="1.25"/>')
    lines.append('</g>')
    return lines, counts


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input', type=Path,
                        default=ROOT/'context/adaptive-trimmed-2026-10-17')
    parser.add_argument('--output', type=Path,
                        default=ROOT/'assets/adaptive-trimmed/faces.svg')
    args = parser.parse_args()
    lines = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{WIDTH}" height="{HEIGHT}" '
             f'viewBox="0 0 {WIDTH} {HEIGHT}" role="img" aria-labelledby="title description">',
             '<title id="title">Complete selected faces · C++ / CUDA</title>',
             '<desc id="description">External orthographic view of actual saved native '
             'CPU1 triangle meshes at 0.005 mm. Colors distinguish cached packets from '
             'connected boundary bands. This is not the GPU renderer.</desc>',
             '<style>text{font-family:Arial,Helvetica,sans-serif;fill:#1c3443}'
             '.panel-title{font-size:20px;font-weight:600}.count{font-size:14px;fill:#526b78}'
             '.note{font-size:15px;fill:#526b78}</style>',
             f'<rect width="{WIDTH}" height="{HEIGHT}" fill="#f2f5f7"/>',
             '<text x="36" y="43" font-size="29" font-weight="600">'
             'Complete selected faces · C++ / CUDA</text>',
             '<text x="36" y="72" class="note">Actual native output · 0.005 mm positional '
             'tolerance · orthographic views fitted per panel</text>',
             '<rect x="36" y="91" width="15" height="15" rx="3" fill="#8ac6d1"/>',
             '<text x="59" y="104" font-size="15">Cached packet facets</text>',
             '<rect x="257" y="91" width="15" height="15" rx="3" fill="#efb16e"/>',
             '<text x="280" y="104" font-size="15">Connected boundary band facets</text>']
    for index, (family, label) in enumerate(PANELS):
        path = args.input / f'{family}-cpu1/native/query-1.bgatrm'
        mesh = read_face(path)
        x, y = 36+(index % 2)*614, 128+(index // 2)*438
        drawing, counts = panel(mesh, family, label, x, y)
        lines.extend(drawing)
        digest = hashlib.sha256(path.read_bytes()).hexdigest()
        lines.append(f'<!-- {family}-cpu1/native/query-1.bgatrm SHA256 {digest} -->')
        print(f'{family}\t{len(mesh["facets"])} triangles\t'
              f'{counts[1]} packet\t{counts[2]} band')
    lines += ['<text x="36" y="1031" class="note">External view of saved geometry; '
              'this figure was not drawn by the GPU renderer.</text>',
              '<text x="36" y="1055" class="note">CPU1 artifacts shown. The observed '
              'CPU1 / CPU4 / P1000 / P1001 artifacts matched byte for byte.</text>', '</svg>']
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text('\n'.join(lines)+'\n')


if __name__ == '__main__':
    main()
