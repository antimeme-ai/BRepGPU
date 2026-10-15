"""Owned OM10 camera, authored presentation and native image output helpers."""
from array import array
from dataclasses import dataclass
import base64
import html
import math
from pathlib import Path
import struct
import sys
import zlib

from brepgpu.imported_assembly import explosion, placed_transform, transform_point


BACKGROUND = (.006, .009, .015)


def dot(a, b):
    return (a[0]*b[0]+a[1]*b[1])+a[2]*b[2]


def cross(a, b):
    return (a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0])


def movement_axes(points):
    """Identify the thin source direction; preserve an orthonormal world basis."""
    bounds = tuple((min(p[i] for p in points), max(p[i] for p in points)) for i in range(3))
    thin = min(range(3), key=lambda i: bounds[i][1]-bounds[i][0])
    normal = tuple(float(i == thin) for i in range(3))
    right = tuple(float(i == ((thin+1)%3)) for i in range(3))
    return right, cross(normal, right), normal


@dataclass(frozen=True)
class View:
    width: int
    height: int
    supersample: int
    alpha: float
    basis: tuple
    center: tuple
    pixel_center: tuple
    pixels_per_unit: float
    background: tuple = BACKGROUND
    flags: int = 0


def camera_view(points, *, width, height, azimuth=-58., elevation=52., alpha=0.,
                supersample=1, padding=(250, 250, 160, 220), axes=None, flags=0):
    points = tuple(tuple(float(x) for x in p) for p in points)
    if not points or any(len(p) != 3 or not all(math.isfinite(x) for x in p) for p in points):
        raise ValueError('camera needs finite actual source-bound points')
    if type(width) is not int or type(height) is not int or not 1 <= width <= 8192 or not 1 <= height <= 8192:
        raise ValueError('invalid native image dimensions')
    if supersample not in (1, 2) or width*supersample > 8192 or height*supersample > 8192:
        raise ValueError('invalid native supersampling dimensions')
    if not math.isfinite(alpha) or not 0. <= alpha <= 1.:
        raise ValueError('invalid explode alpha')
    azimuth, elevation = math.radians(azimuth), math.radians(elevation)
    a, b, n = movement_axes(points) if axes is None else axes
    ca, sa, ce, se = math.cos(azimuth), math.sin(azimuth), math.cos(elevation), math.sin(elevation)
    right = tuple(-sa*a[i]+ca*b[i] for i in range(3))
    toward = tuple(ce*ca*a[i]+ce*sa*b[i]+se*n[i] for i in range(3))
    up = cross(toward, right)
    center = tuple((min(p[i] for p in points)+max(p[i] for p in points))/2 for i in range(3))
    projected = tuple((dot(right, tuple(p[i]-center[i] for i in range(3))),
                       dot(up, tuple(p[i]-center[i] for i in range(3)))) for p in points)
    xmin, xmax = min(p[0] for p in projected), max(p[0] for p in projected)
    ymin, ymax = min(p[1] for p in projected), max(p[1] for p in projected)
    left, right_margin, top, bottom = padding
    available = (width-left-right_margin, height-top-bottom)
    if min(available) <= 0 or max(xmax-xmin, ymax-ymin) <= 0:
        raise ValueError('empty camera viewport or collapsed scene bounds')
    scale = .96*min(available[0]/max(xmax-xmin, 1.e-300),
                    available[1]/max(ymax-ymin, 1.e-300))
    pixel_center = (left+available[0]/2-scale*(xmin+xmax)/2,
                    top+available[1]/2+scale*(ymin+ymax)/2)
    return View(width, height, supersample, alpha, (*right, *up, *toward), center,
                pixel_center, scale, BACKGROUND, flags)


def project_point(view, point):
    delta = tuple(point[i]-view.center[i] for i in range(3))
    return (view.pixel_center[0]+view.pixels_per_unit*dot(view.basis[:3], delta),
            view.pixel_center[1]-view.pixels_per_unit*dot(view.basis[3:6], delta),
            dot(view.basis[6:9], delta))


def pack_views(views):
    views = tuple(views)
    result = bytearray(struct.pack('<8sII', b'BGVIEW01', 1, len(views)))
    for view in views:
        result.extend(struct.pack('<4Id9d3d2dd3d2I2d', view.width, view.height,
                      view.supersample, 0, view.alpha, *view.basis, *view.center,
                      *view.pixel_center, view.pixels_per_unit, *view.background,
                      view.flags, 0, 0., 0.))
    return bytes(result)


def write_png(path, width, height, rgba):
    if type(width) is not int or type(height) is not int or min(width, height) < 1:
        raise ValueError('PNG dimensions must be positive integers')
    if len(rgba) != width*height*4:
        raise ValueError('PNG must consume the entire actual native RGBA buffer')

    def chunk(tag, data):
        return struct.pack('>I', len(data))+tag+data+struct.pack('>I', zlib.crc32(tag+data)&0xffffffff)

    rows = b''.join(b'\0'+rgba[i*width*4:(i+1)*width*4] for i in range(height))
    data = b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR', struct.pack('>2I5B', width, height, 8, 6, 0, 0, 0))
    data += chunk(b'IDAT', zlib.compress(rows, 6))+chunk(b'IEND', b'')
    Path(path).write_bytes(data)


def box_corners(bounds):
    return tuple((x, y, z) for x in (bounds[0], bounds[3])
                 for y in (bounds[1], bounds[4]) for z in (bounds[2], bounds[5]))


def placed_corners(assembly, definition_bounds, layout=None, alpha=0.):
    bounds = dict(definition_bounds)
    offsets = ((0., 0., 0.),)*len(assembly.occurrences) if layout is None else layout.offsets
    return tuple(transform_point(placed_transform(o, offsets[o.id-1], alpha), p)
                 for o in assembly.occurrences for p in box_corners(bounds[o.definition_id]))


@dataclass(frozen=True)
class LayoutRecord:
    occurrence_id: int
    group_path: tuple
    tier: float
    offset: tuple
    reason: str


def authored_layout(assembly, definition_bounds):
    """Separate real source groups without changing any component's geometry."""
    bounds = dict(definition_bounds)
    assembled = placed_corners(assembly, definition_bounds)
    a, b, normal = movement_axes(assembled)
    center = tuple((min(p[i] for p in assembled)+max(p[i] for p in assembled))/2 for i in range(3))
    groups = {g.path: g for g in assembly.groups}
    records = []
    for occurrence in assembly.occurrences:
        parent_path = occurrence.path[:1]
        group = groups.get(parent_path)
        label = ((group.name+' '+group.reference) if group is not None else
                 occurrence.name+' '+occurrence.reference).casefold()
        name = occurrence.name.casefold()
        if 'platine' in label:
            tier, reason = 0., 'main plate datum'
        elif any(s in label for s in ('quanti', 'calend', 'disque')):
            tier, reason = -2., 'calendar layer'
        elif any(s in label for s in ('coq', 'raquette', 'réglage')):
            tier, reason = 5., 'balance support layer'
        elif 'pont' in label:
            tier, reason = 4., 'bridge layer'
        elif any(s in label for s in ('balancier', 'spiral')):
            tier, reason = 3., 'oscillator layer'
        elif any(s in label for s in ('barillet', 'rochet', 'cliquet')):
            tier, reason = 2., 'barrel and winding layer'
        elif any(s in label for s in ('mobile', 'roue', 'ancre', 'pignon')):
            tier, reason = 1.5, 'train and escapement layer'
        elif any(s in label for s in ('vis', 'goupille')):
            tier, reason = 5.8, 'fastener layer'
        else:
            tier, reason = .8, 'source component layer'
        # Separate the actual internal leaves as well as root subassemblies.
        # These authored offsets expose the barrel/spring/cover and oscillator;
        # the geometry and source placement at alpha0 stay unchanged.
        detail = 0.
        if len(occurrence.path) > 1:
            if 'couvercle' in name:
                detail = 4.5
            elif 'ressort de barillet' in name:
                detail = 2.4
            elif any(s in name for s in ('arbre', 'axe', 'tige')):
                detail = -2.
            elif 'vis' in name:
                detail = 3.
            elif any(s in name for s in ('pierre', 'rubis', 'palette', 'cheville')):
                detail = 1.6
            elif any(s in name for s in ('spiral', 'virole')):
                detail = 2.8
            elif 'planche' in name:
                detail = 1.2
            elif 'pignon' in name:
                detail = -.8
            reason += f'; authored internal leaf lift {detail:g} mm'
        local_center = tuple((bounds[occurrence.definition_id][i]+bounds[occurrence.definition_id][i+3])/2 for i in range(3))
        world_center = transform_point(occurrence.transform, local_center)
        radial = tuple(world_center[i]-center[i] for i in range(3))
        px, py = dot(radial, a), dot(radial, b)
        length = math.hypot(px, py)
        radial_distance = 0. if tier == 0. else 3.5
        if length > .1:
            radial_direction = tuple((px*a[i]+py*b[i])/length for i in range(3))
        else:
            radial_direction = (0., 0., 0.)
        offset = tuple((tier*5.+detail)*normal[i]+radial_distance*radial_direction[i] for i in range(3))
        records.append(LayoutRecord(occurrence.id, parent_path, tier, offset, reason))
    return explosion(assembly, (r.offset for r in records)), tuple(records), (a, b, normal)


PALETTE = (
    ('rhodium', (.43, .49, .57), .28),
    ('brass', (.61, .35, .085), .30),
    ('steel', (.22, .29, .38), .24),
    ('ruby', (.45, .012, .055), .22),
    ('spring', (.065, .14, .25), .34),
)


def authored_material(name):
    name = name.casefold()
    if any(s in name for s in ('pierre', 'rubis', 'palette', 'contre-pivot', 'cheville')):
        return 3
    if any(s in name for s in ('ressort', 'spiral')):
        return 4
    if any(s in name for s in ('planche', 'roue', 'rochet', 'serge', 'tambour', 'couvercle')):
        return 1
    if any(s in name for s in ('vis', 'pignon', 'axe', 'arbre', 'tige', 'goupille')):
        return 2
    return 0


def visible_anchors(path, width, height, *, step=3, valid_ids=None):
    """Select actual visible source-instance samples near their visible centroid."""
    ids = array('Q')
    ids.frombytes(Path(path).read_bytes())
    if sys.byteorder != 'little':
        ids.byteswap()
    if len(ids) != width*height:
        raise ValueError('native instance-ID image has wrong length')
    if valid_ids is not None and set(ids)-set(valid_ids)-{(1 << 64)-1}:
        raise ValueError('native image contains an unknown source occurrence ID')
    totals = {}
    for y in range(0, height, step):
        for x in range(0, width, step):
            id = ids[y*width+x]
            if id == (1 << 64)-1:
                continue
            count, sx, sy = totals.get(id, (0, 0, 0))
            totals[id] = count+1, sx+x, sy+y
    selected = {}
    for y in range(0, height, step):
        for x in range(0, width, step):
            id = ids[y*width+x]
            if id not in totals:
                continue
            count, sx, sy = totals[id]
            distance = (x-sx/count)**2+(y-sy/count)**2
            if id not in selected or distance < selected[id][0]:
                selected[id] = distance, x, y
    return {id: (r[1], r[2], totals[id][0]*step*step) for id, r in selected.items()}


def labeled_svg(path, png_path, view, *, title, subtitle, counts, timings,
                callouts=(), timing_header='', task_timings=(), task_timing_header='',
                attribution='Association openmovement · OM10 MAN VA01.3 · CC BY-SA 3.0 · BRepGPU mesh, palette, lighting and labels'):
    """Compose source-linked typography over the actual native image pixels."""
    esc = html.escape
    encoded = base64.b64encode(Path(png_path).read_bytes()).decode('ascii')
    w, h = view.width, view.height
    rows = [f'<svg xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" width="{w}" height="{h}" viewBox="0 0 {w} {h}">',
            f'<title>{esc(title)}</title>',
            '<desc>Source: Association openmovement, OM10 MAN VA01.3. Owned tessellation, illustrative palette, lighting, authored explosion and labels by BRepGPU.</desc>',
            '<metadata xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#" xmlns:dc="http://purl.org/dc/elements/1.1/" xmlns:cc="http://creativecommons.org/ns#"><rdf:RDF><cc:Work rdf:about=""><dc:source rdf:resource="https://openmovement.org/project/om10/"/><dc:creator>Association openmovement (source); BRepGPU (visualization)</dc:creator><cc:license rdf:resource="https://creativecommons.org/licenses/by-sa/3.0/"/></cc:Work></rdf:RDF></metadata>',
            f'<image width="{w}" height="{h}" xlink:href="data:image/png;base64,{encoded}"/>',
            '<g font-family="DejaVu Sans,Arial,sans-serif" fill="#e5edf6">',
            f'<text x="70" y="68" font-size="19" letter-spacing="5" fill="#6cddd0">BRepGPU / OPENMOVEMENT</text>',
            f'<text x="67" y="125" font-size="48" font-weight="700">{esc(title)}</text>',
            f'<text x="70" y="166" font-size="21" fill="#9aaec1">{esc(subtitle)}</text>']
    for i, (label, value) in enumerate(counts):
        x = 70+i*270
        rows.extend((f'<text x="{x}" y="{h-146}" font-size="30" font-weight="600">{esc(str(value))}</text>',
                     f'<text x="{x}" y="{h-116}" font-size="16" fill="#8fa7bc">{esc(label)}</text>'))
    if timing_header:
        rows.append(f'<text x="{w-570}" y="{h-190}" font-size="14" fill="#6cddd0">{esc(timing_header)}</text>')
    for i, (label, value) in enumerate(timings):
        x = w-570
        rows.append(f'<text x="{x}" y="{h-155+i*29}" font-size="20"><tspan fill="#8fa7bc">{esc(label)} </tspan>{esc(value)}</text>')
    if task_timing_header:
        rows.append(f'<text x="{w-1390}" y="{h-190}" font-size="14" fill="#6cddd0">{esc(task_timing_header)}</text>')
    for i, (label, value) in enumerate(task_timings):
        rows.append(f'<text x="{w-1390}" y="{h-155+i*29}" font-size="20"><tspan fill="#8fa7bc">{esc(label)} </tspan>{esc(value)}</text>')
    for text, x, y, ax, ay, anchor in callouts:
        elbow = x+12 if anchor == 'start' else x-12
        rows.extend((f'<path d="M {ax:.2f} {ay:.2f} L {elbow:.2f} {y-8:.2f} L {x:.2f} {y-8:.2f}" stroke="#6cddd0" stroke-width="1.2" opacity=".65" fill="none"/>',
                     f'<circle cx="{ax:.2f}" cy="{ay:.2f}" r="3" fill="#6cddd0"/>',
                     f'<text x="{x:.2f}" y="{y:.2f}" font-size="16" text-anchor="{anchor}" stroke="#111b25" stroke-width="4" paint-order="stroke">{esc(text)}</text>'))
    rows.append(f'<text x="70" y="{h-39}" font-size="15" fill="#8097ab">{esc(attribution)}</text>')
    rows.append('</g></svg>\n')
    Path(path).write_text('\n'.join(rows), encoding='utf-8')
