"""Source-linked, authored OM10 illustration translations over stored meshes.

This is an explanatory layout, not a collision-free disassembly planner.
Original instance matrices and every non-explosion scene byte are preserved.
"""
from dataclasses import dataclass
import math
import struct

from om10_scene import camera_view, dot, box_corners
from brepgpu.imported_assembly import transform_point

HEADER = struct.Struct('<8sII8Q')
INSTANCE = struct.Struct('<QII15d')
MODULE_NAMES = {
    'support': ('A', 'MAINPLATE / PLATINE'),
    'train': ('B', 'TIME TRAIN / ROUAGE'),
    'bridges': ('C', 'BRIDGES / PONTS'),
    'power': ('D', 'BARREL & WINDING POWER'),
    'winding': ('E', 'WINDING & SETTING WORK'),
    'escapement': ('F', 'PALLET FORK / ANCRE'),
    'regulator': ('G', 'BALANCE & HAIRSPRING'),
    'motion': ('H', 'MOTION WORK / MINUTERIE'),
    'calendar': ('I', 'CALENDAR / QUANTIÈME'),
}
# Root subassemblies are original public product references, not solid IDs.
GROUPS = {
    'OM10-00205': ('support', 'mainplate garniture', 0.),
    'OM10-00302': ('winding', 'setting mechanism', 0.),
    'OM10-00117': ('power', 'barrel', 0.),
    'OM10-00201': ('power', 'click', 12.),
    'OM10-00148': ('train', 'grande moyenne mobile', 11.),
    'OM10-00160': ('train', 'moyenne mobile', 21.),
    'OM10-00163': ('train', 'seconds mobile', 31.),
    'OM10-00145': ('train', 'escape mobile', 41.),
    'OM10-00183': ('bridges', 'train bridge', 57.),
    'OM10-00154': ('motion', 'intermediate mobile', 0.),
    'OM10-00103': ('escapement', 'pallet fork', 0.),
    'OM10-00100': ('escapement', 'pallet bridge', 7.),
    'OM10-00191': ('regulator', 'balance cock garniture', 17.),
    'OM10-00108': ('regulator', 'balance mobile', 0.),
    'OM10-00126': ('motion', 'cannon mobile', 5.),
    'OM10-00198': ('bridges', 'barrel bridge', 73.),
    'OM10-00157': ('motion', 'minute mobile', 10.),
    'OM10-00301': ('calendar', 'date disc', 11.),
    'OM10-00216': ('calendar', 'date intermediate mobile', 2.),
    'OM10-00208': ('calendar', 'date driving mobile', 5.),
}
ROOT_PARTS = {
    'OM10-00306': ('winding', 'setting cover', 14.),
    'OM10-00211': ('power', 'click spring', 12.),
    'OM10-00197': ('power', 'ratchet', 17.),
    'OM10-00144': ('power', 'click spacer', 8.),
    'OM10-00206': ('motion', 'hour wheel', 16.),
    'OM10-00142': ('calendar', 'date finger', 2.),
    'OM10-00210': ('calendar', 'date spring', 4.),
    'OM10-00212': ('calendar', 'date idler', 2.),
    'OM10-00217': ('calendar', 'date plate', 23.),
    'OM10-00204': ('calendar', 'date jumper', 5.),
    'OM10-00209': ('calendar', 'date jumper spring', 8.),
    'OM00-00142': ('support', 'fixing clamp', 0.),
}
# Authored associations for root-level screws: these are not source contact data.
ROOT_FASTENERS = {
    40: ('support', 8.), 41: ('support', 8.),
    79: ('winding', 19.), 81: ('power', 20.), 87: ('power', 23.),
    91: ('power', 20.), 92: ('power', 20.),
    107: ('bridges', 64.), 108: ('bridges', 64.),
    111: ('motion', 5.), 119: ('escapement', 12.),
    132: ('regulator', 23.), 133: ('regulator', 23.),
    158: ('calendar', 29.), 159: ('calendar', 29.),
    163: ('support', 7.), 165: ('support', 7.), 166: ('support', 7.),
}
TARGETS = {
    'power': (-38., 22.), 'winding': (-38., -20.),
    'escapement': (23., 0.), 'regulator': (38., 25.),
    'motion': (35., -30.), 'calendar': (-3., -48.),
}


def require(value, message):
    if not value:
        raise ValueError(message)


def scene_header(data):
    require(len(data) >= HEADER.size, 'scene header truncated')
    h = HEADER.unpack_from(data)
    require(h[:3] == (b'BGMESH01', 1, 1), 'original source-profile1 scene required')
    nd, ni, nv, nt, nc, nm, nh, reserved = h[3:]
    require(reserved == 0 and len(data) == 80+40*nd+136*ni+56*nv+24*nt+192*nc+32*nm+32*nh,
            'scene section sizes differ')
    return nd, ni, nv, nt, nc, nm, nh


def translated_scene(data, offsets):
    """Change exactly the three declared explosion doubles for each original ID."""
    nd, ni, *_ = scene_header(data)
    result = bytearray(data)
    seen = set()
    for i in range(ni):
        at = 80+40*nd+136*i
        identifier = INSTANCE.unpack_from(data, at)[0]
        require(identifier not in seen and identifier in offsets, 'duplicate/missing occurrence offset')
        seen.add(identifier)
        value = tuple(offsets[identifier])
        require(len(value) == 3 and all(math.isfinite(x) for x in value), 'invalid occurrence translation')
        struct.pack_into('<3d', result, at+112, *value)
    require(seen == set(offsets), 'unknown occurrence offset')
    # Compare disjoint unchanged slices; no geometry hash or second mesh checker.
    previous = 0
    for i in range(ni):
        at = 80+40*nd+136*i+112
        require(result[previous:at] == data[previous:at], 'non-explosion scene bytes changed')
        previous = at+24
    require(result[previous:] == data[previous:], 'non-explosion scene tail changed')
    return bytes(result)


def stored_geometry(data):
    nd, ni, nv, *_ = scene_header(data)
    start = 80+40*nd+136*ni
    bounds = []
    for i in range(nd):
        _, first, count, _, _ = struct.unpack_from('<5Q', data, 80+40*i)
        require(count > 0 and first+count <= nv, 'invalid stored definition range')
        low, high = [math.inf]*3, [-math.inf]*3
        for j in range(first, first+count):
            p = struct.unpack_from('<3d', data, start+56*j)
            for a in range(3):
                low[a] = min(low[a], p[a]); high[a] = max(high[a], p[a])
        bounds.append((*low, *high))
    instances = [INSTANCE.unpack_from(data, 80+40*nd+136*i) for i in range(ni)]
    boxes = {}
    for row in instances:
        points = tuple(transform_point(row[3:15], p) for p in box_corners(bounds[row[1]]))
        boxes[row[0]] = points
    return boxes


@dataclass(frozen=True)
class LayoutRecord:
    occurrence_id: int
    display_label: int
    source_usage_path: str
    product_id: int
    definition_id: int
    source_group: str
    module: str
    role: str
    offset: tuple
    reason: str


def role_for(occurrence, groups, label):
    parent = groups.get(occurrence.path[:1])
    reference = occurrence.reference
    if parent is not None:
        require(parent.reference in GROUPS, 'unassigned original source subassembly '+parent.reference)
        module, role, lift = GROUPS[parent.reference]
        if module == 'power' and parent.reference == 'OM10-00117':
            role, lift = {
                'OM10-00121': ('barrel drum', 0.),
                'OM10-00119': ('barrel cover', 11.),
                'OM10-00118': ('barrel arbor', -5.),
                'OM10-00120': ('mainspring', 5.5),
            }[reference]
        elif module == 'winding':
            ancestors = [g for path, g in groups.items() if occurrence.path[:len(path)] == path]
            if any(g.reference == 'OM10-00305' for g in ancestors):
                role, lift = 'setting baseplate garniture', -5.
            elif reference == 'OM10-00307':
                role, lift = 'setting bridge', 8.
            elif reference == 'OM10-00304':
                role, lift = 'crown-wheel plate', 4.
            elif reference == 'OM10-00225':
                role, lift = 'winding stem', 0.
            elif 'ressort' in occurrence.name.casefold():
                role, lift = 'setting spring', 3.
        elif parent.reference == 'OM10-00108':
            if reference == 'OM10-00112': role, lift = 'balance staff', -4.
            elif reference in ('OM10-00115', 'OM10-00116'): role, lift = 'hairspring and collet', 7.
            elif reference in ('OM10-00110', 'OM10-00111'): role, lift = 'roller and impulse jewel', -2.
        return module, role, lift, parent.name
    if reference in ROOT_PARTS:
        module, role, lift = ROOT_PARTS[reference]
        return module, role, lift, 'root usage'
    require(label in ROOT_FASTENERS and occurrence.name.casefold() == 'vis',
            'unassigned original root occurrence '+str(label)+' '+reference+' '+occurrence.name)
    module, lift = ROOT_FASTENERS[label]
    return module, ('train bridge fastener' if label in (107,108) else 'authored fastener association'), lift, 'root usage'


def make_layout(match, labels, data):
    require(len(labels) == len(match.assembly.occurrences) == 166, 'complete166-occurrence source required')
    boxes = stored_geometry(data)
    groups = {g.path: g for g in match.assembly.groups}
    by_label = {int(r['occurrence']): int(r['label']) for r in labels}
    plate = next(o for o in match.assembly.occurrences if o.name == 'Platine brute')
    plate_points = boxes[plate.id]
    extent = [max(p[a] for p in plate_points)-min(p[a] for p in plate_points) for a in range(3)]
    thin = min(range(3), key=lambda a: extent[a])
    n = tuple(float(i == thin) for i in range(3))
    a = tuple(float(i == (thin+1)%3) for i in range(3))
    from om10_scene import cross
    b = cross(n, a)
    datum = tuple((min(p[k] for p in plate_points)+max(p[k] for p in plate_points))/2 for k in range(3))
    base_view = camera_view(plate_points, width=3840, height=4000, azimuth=-50., elevation=35., axes=(a,b,n))
    right, up = base_view.basis[:3], base_view.basis[3:6]
    roles = {}; offsets = {}
    for occurrence in match.assembly.occurrences:
        module, role, lift, group = role_for(occurrence, groups, by_label[occurrence.id])
        delta = tuple(lift*x for x in n)
        # Broad bridges have different source-aligned axial levels, with a small
        # authored side separation to retain each outline in the illustration.
        if module == 'bridges':
            side = -5. if role in ('train bridge','train bridge fastener') else 5.
            delta = tuple(delta[k]+side*right[k] for k in range(3))
        roles[occurrence.id] = (module,role,group)
        offsets[occurrence.id] = delta
    for module, target in TARGETS.items():
        points = [tuple(p[k]+offsets[identifier][k] for k in range(3))
                  for identifier in boxes if roles[identifier][0] == module for p in boxes[identifier]]
        require(points, 'empty authored module '+module)
        projected = [(dot(right, tuple(p[k]-datum[k] for k in range(3))),
                      dot(up, tuple(p[k]-datum[k] for k in range(3)))) for p in points]
        center = tuple((min(p[k] for p in projected)+max(p[k] for p in projected))/2 for k in range(2))
        delta = tuple((target[0]-center[0])*right[k]+(target[1]-center[1])*up[k] for k in range(3))
        for identifier in offsets:
            if roles[identifier][0] == module:
                offsets[identifier] = tuple(offsets[identifier][k]+delta[k] for k in range(3))
    records = []
    for o in match.assembly.occurrences:
        module,role,group = roles[o.id]
        records.append(LayoutRecord(o.id,by_label[o.id],'/'.join(map(str,o.path)),o.product_id,
            o.definition_id,group,module,role,offsets[o.id],
            'Authored explanatory translation; original rotation and geometry unchanged'))
    return tuple(records), boxes, (a,b,n)


def diagram_views(boxes, offsets, axes):
    original = [p for points in boxes.values() for p in points]
    exploded = [tuple(p[k]+offsets[identifier][k] for k in range(3))
                for identifier,points in boxes.items() for p in points]
    views = [camera_view(points,width=3840,height=4000,alpha=float(i),supersample=2,
                        azimuth=-50.,elevation=35.,axes=axes,padding=(210,210,260,180))
             for i,points in enumerate((original,exploded))]
    from dataclasses import replace
    return tuple(replace(v,background=(.84,.88,.90)) for v in views)
