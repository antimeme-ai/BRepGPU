"""Independent exact analytic truth for the three declared body fixtures.

No product classifier/curve/evaluator supplies expected geometry. Polynomial
cross-products check complete rational edge/UV identities; Fraction checks the
actual binary UV and emitted positions/normals. Native calls belong to callers.
"""
from fractions import Fraction as F
import math
import struct

REJECTED, INTERIOR, BOUNDARY, INVALID = range(4)
COUNTS = {"box": (8, 12, 6, 6, 24, 6), "cylinder": (2, 3, 3, 3, 6, 6),
          "plate": (10, 15, 7, 9, 30, 10)}
FACE_NAMES = {"box": ("top", "bottom", "front", "back", "right", "left"),
              "plate": ("top", "bottom", "front", "back", "right", "left", "wall"),
              "cylinder": ("top", "bottom", "wall")}
SIGNS = dict(top=1, bottom=-1, front=1, back=-1, right=1, left=-1)
NORMALS = dict(top=(0, 0, 1), bottom=(0, 0, -1), front=(0, -1, 0),
               back=(0, 1, 0), right=(1, 0, 0), left=(-1, 0, 0))
POSITION_LIMIT, NORMAL_LIMIT = F(1, 1 << 36), F(1, 1 << 30)


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def exact(value):
    return value if isinstance(value, F) else F.from_float(float(value))


def rotate(x, y, quadrant):
    return ((x, y), (-y, x), (-x, -y), (y, -x))[quadrant]


def role(snapshot, patch_slot):
    require(0 <= patch_slot < len(snapshot.charts), "invalid oracle patch slot")
    chart = snapshot.charts[patch_slot]
    face = snapshot.faces[chart.face_slot]
    return chart, face


def trim_label(snapshot, patch_slot, u, v):
    chart, face = role(snapshot, patch_slot)
    if not math.isfinite(u) or not math.isfinite(v) or not 0 <= u <= 1 or not 0 <= v <= 1:
        return INVALID
    u, v = exact(u), exact(v)
    if face.name == "wall":
        return BOUNDARY if v in (0, 1) or (chart.quadrant == 0 and u == 0) \
            or (chart.quadrant == 3 and u == 1) else INTERIOR
    radius_squared = (2*u-1)**2 + (2*v-1)**2
    if snapshot.shape == "cylinder":
        return REJECTED if radius_squared > 1 else BOUNDARY if radius_squared == 1 else INTERIOR
    if snapshot.shape == "plate" and face.name in ("top", "bottom"):
        if radius_squared < F(1, 9):
            return REJECTED
        if radius_squared == F(1, 9):
            return BOUNDARY
    return BOUNDARY if u in (0, 1) or v in (0, 1) else INTERIOR


def body_geometry(snapshot, patch_slot, u, v):
    chart, face = role(snapshot, patch_slot)
    u, v = exact(u), exact(v)
    if face.name == "wall":
        denominator = 1+u*u
        x, y = rotate((1-u*u)/denominator, 2*u/denominator, chart.quadrant)
        sign = -1 if snapshot.shape == "plate" else 1
        return (x/4, y/4, v-F(1, 2)), (sign*x, sign*y, F(0))
    a, h = (F(1, 4) if snapshot.shape == "cylinder" else F(3, 4)), F(1, 2)
    x, y, z = a*(2*u-1), a*(2*v-1), h*(2*v-1)
    point = {"top": (x, y, h), "bottom": (x, y, -h),
             "front": (x, -a, z), "back": (x, a, z),
             "right": (a, x, z), "left": (-a, x, z)}[face.name]
    return point, tuple(F(n) for n in NORMALS[face.name])


def ideal_geometry(snapshot, patch_slot, u, v):
    point, normal = body_geometry(snapshot, patch_slot, u, v)
    return tuple(exact(o)+exact(snapshot.frame.scale)*p for o, p in zip(snapshot.frame.origin, point)), normal


# Small polynomial/rational values are expected analytic expressions, not a
# second implementation of native Bernstein subdivision or surface evaluation.
def poly(values):
    values = list(map(exact, values))
    while len(values) > 1 and values[-1] == 0:
        values.pop()
    return tuple(values)


def add(a, b):
    return poly((a[i] if i < len(a) else 0)+(b[i] if i < len(b) else 0)
                for i in range(max(len(a), len(b))))


def mul(a, b):
    values = [F(0)]*(len(a)+len(b)-1)
    for i, x in enumerate(a):
        for j, y in enumerate(b):
            values[i+j] += x*y
    return poly(values)


def factor(a, value):
    return poly(x*exact(value) for x in a)


def rational(a):
    return poly(a), (F(1),)


def radd(a, b):
    return add(mul(a[0], b[1]), mul(b[0], a[1])), mul(a[1], b[1])


def rscale(a, value):
    return factor(a[0], value), a[1]


def same(a, b):
    return add(mul(a[0], b[1]), factor(mul(b[0], a[1]), -1)) == (F(0),)


def quarter_rational(q):
    denominator = (F(1), F(0), F(1))
    if q == 0:
        x, y = (1, 0, -1), (0, 2)
    elif q == 1:
        x, y = (0, -2), (1, 0, -1)
    elif q == 2:
        x, y = (-1, 0, 1), (0, -2)
    else:
        x, y = (0, 2), (-1, 0, 1)
    return (poly(x), denominator), (poly(y), denominator)


def curve_expression(curve, quadrant=0):
    if curve.kind == "line":
        require(len(curve.data) == 6, "line schema differs")
        return tuple(rational((exact(curve.data[i]), exact(curve.data[i+3])-exact(curve.data[i])))
                     for i in range(3))
    require(curve.kind == "circle" and len(curve.data) == 2, "unknown curve/schema")
    radius, z = map(exact, curve.data)
    require(radius == F(1, 4) and z in (-F(1, 2), F(1, 2)), "noncanonical circle")
    x, y = quarter_rational(quadrant)
    return rscale(x, radius), rscale(y, radius), rational((z,))


def uv_expression(mapping, quadrant=0):
    t = rational((F(quadrant, 4), F(1, 4))) if mapping.kind.startswith("circle_") else rational((0, 1))
    data = tuple(map(exact, mapping.data))
    if mapping.kind == "line":
        require(len(data) == 4, "UV line schema differs")
        return tuple(rational((data[i], data[i+2]-data[i])) for i in range(2))
    if mapping.kind == "circle_plane":
        require(len(data) == 2 and data[0] == F(1, 4) and data[1] in (F(1, 4), F(3, 4)),
                "noncanonical circle plane map")
        x, y = quarter_rational(quadrant)
        return tuple(radd(rscale(a, data[0]/(2*data[1])), rational((F(1, 2),))) for a in (x, y))
    if mapping.kind == "circle_wall":
        require(len(data) == 1 and data[0] in (0, 1), "noncanonical circle wall map")
        return rscale(t, 4), rational((data[0],))
    require(mapping.kind == "seam_wall" and len(data) == 1 and data[0] in (0, 4), "noncanonical seam map")
    return rational((data[0],)), t


def compose_face(snapshot, face, mapping, quadrant):
    u, v = uv_expression(mapping, quadrant)
    if face.name == "wall":
        require(mapping.kind in ("circle_wall", "seam_wall"), "wrong wall boundary map")
        q = quadrant if mapping.kind == "circle_wall" else (0 if mapping.data[0] == 0 else 3)
        s = radd(u, rational((-q,)))
        ss = mul(s[0], s[0])
        dd = mul(s[1], s[1])
        x = add(dd, factor(ss, -1)), add(dd, ss)
        y = factor(mul(s[0], s[1]), 2), add(dd, ss)
        if q == 1:
            x, y = rscale(y, -1), x
        elif q == 2:
            x, y = rscale(x, -1), rscale(y, -1)
        elif q == 3:
            x, y = y, rscale(x, -1)
        return rscale(x, F(1, 4)), rscale(y, F(1, 4)), radd(v, rational((-F(1, 2),)))
    a, h = F(1, 4) if snapshot.shape == "cylinder" else F(3, 4), F(1, 2)
    x = radd(rscale(u, 2*a), rational((-a,)))
    y = radd(rscale(v, 2*a), rational((-a,)))
    z = radd(v, rational((-h,)))
    c = lambda value: rational((value,))
    return {"top": (x, y, c(h)), "bottom": (x, y, c(-h)),
            "front": (x, c(-a), z), "back": (x, c(a), z),
            "right": (c(a), x, z), "left": (c(-a), x, z)}[face.name]


def value(expression, t):
    return sum(x*t**i for i, x in enumerate(expression[0])) / sum(x*t**i for i, x in enumerate(expression[1]))


def uv_endpoint(mapping, t):
    q, local = (3, F(1)) if t == 1 else (0, F(0))
    return tuple(value(a, local if mapping.kind.startswith("circle_") else t)
                 for a in uv_expression(mapping, q))


def qualify_tables(snapshot):
    """Decode the declared export contract, independently of its packer."""
    layouts = (("header", "<8sIIQQII4d"), ("vertices", "<Q3d"),
               ("curves", "<II6d"), ("edges", "<QIII"), ("uv_maps", "<II4d"),
               ("coedges", "<QIIIIi"), ("loops", "<QIIII"),
               ("faces", "<Q12sIiIIII"), ("charts", "<QIiI"),
               ("patches", "<IIII4d"), ("controls", "<4d"),
               ("loop_coedges", "<I"), ("face_loops", "<I"),
               ("face_charts", "<I"), ("shell_faces", "<I"))
    require(tuple(name for name, _ in snapshot.packed_tables) == tuple(name for name, _ in layouts),
            "packed table names/order differ")
    tables = {}
    for (name, payload), (_, fmt) in zip(snapshot.packed_tables, layouts):
        require(isinstance(payload, bytes) and len(payload) % struct.calcsize(fmt) == 0,
                "packed record extent differs: "+name)
        tables[name] = tuple(struct.iter_unpack(fmt, payload))
    require(tables["header"] == ((b"BGbody01", 1, 0, snapshot.body_id, snapshot.revision,
            ("box", "cylinder", "plate").index(snapshot.shape)+1, 1,
            *snapshot.frame.origin, snapshot.frame.scale),), "packed header differs")
    require(tables["vertices"] == tuple((v.id, *v.position) for v in snapshot.vertices), "packed vertices differ")
    require(tables["edges"] == tuple((e.id, e.start, e.end, e.curve_slot) for e in snapshot.edges), "packed edges differ")
    for name, records, kinds, width in (("curves", snapshot.curves, ("line", "circle"), 6),
            ("uv_maps", tuple(c.uv_map for c in snapshot.coedges),
             ("line", "circle_plane", "circle_wall", "seam_wall"), 4)):
        require(len(tables[name]) == len(records), "packed geometry count differs")
        for decoded, record in zip(tables[name], records):
            require(decoded[:2] == (kinds.index(record.kind)+1, len(record.data))
                    and decoded[2:2+len(record.data)] == record.data
                    and decoded[2+len(record.data):] == (0.,)*(width-len(record.data)),
                    "packed geometry/tag/padding differs: "+name)
    require(tables["coedges"] == tuple((c.id, c.edge_slot, c.face_slot, c.loop_slot, i, c.orientation)
            for i, c in enumerate(snapshot.coedges)), "packed coedges differ")
    for name, expected in (("loop_coedges", tuple(i for l in snapshot.loops for i in l.coedge_slots)),
                          ("face_loops", tuple(i for f in snapshot.faces for i in f.loop_slots)),
                          ("face_charts", tuple(i for f in snapshot.faces for i in f.chart_slots)),
                          ("shell_faces", tuple(range(len(snapshot.faces))))):
        require(tuple(row[0] for row in tables[name]) == expected, "packed adjacency differs: "+name)
    require(len(tables["loops"]) == len(snapshot.loops) and len(tables["faces"]) == len(snapshot.faces), "packed owner counts differ")
    offset = 0
    for decoded, loop in zip(tables["loops"], snapshot.loops):
        require(decoded == (loop.id, loop.face_slot, int(loop.hole), offset, len(loop.coedge_slots)), "packed loop range differs")
        offset += len(loop.coedge_slots)
    loop_offset = chart_offset = 0
    for decoded, face in zip(tables["faces"], snapshot.faces):
        require(decoded == (face.id, face.name.encode("ascii").ljust(12, b"\0"),
                ("rectangle", "disk", "rectangle_hole", "cylinder").index(face.kind)+1,
                face.orientation, loop_offset, len(face.loop_slots), chart_offset, len(face.chart_slots)), "packed face ranges differ")
        loop_offset += len(face.loop_slots); chart_offset += len(face.chart_slots)
    require(tables["charts"] == tuple((c.id, c.face_slot, c.quadrant, i) for i, c in enumerate(snapshot.charts)), "packed charts differ")
    require(len(tables["patches"]) == len(snapshot.patches), "packed patch count differs")
    offset = 0
    expected_controls = []
    for decoded, patch in zip(tables["patches"], snapshot.patches):
        count = len(patch.controls)//4
        require(decoded == (patch.degree_u, patch.degree_v, offset, count, *patch.uv), "packed patch/control range differs")
        expected_controls.extend(tuple(patch.controls[i:i+4]) for i in range(0, len(patch.controls), 4))
        offset += count
    require(tables["controls"] == tuple(expected_controls), "packed homogeneous controls differ")


def qualify_snapshot(snapshot):
    require(snapshot.shape in COUNTS, "unsupported oracle body")
    groups = (snapshot.vertices, snapshot.edges, snapshot.faces, snapshot.loops,
              snapshot.coedges, snapshot.charts)
    require(tuple(map(len, groups)) == COUNTS[snapshot.shape], "canonical body graph counts differ")
    require(tuple(face.name for face in snapshot.faces) == FACE_NAMES[snapshot.shape], "face role/order differs")
    require(snapshot.body_id > 0 and snapshot.revision == 1, "invalid source namespace/revision")
    require(isinstance(snapshot.frame.origin, tuple) and len(snapshot.frame.origin) == 3
            and all(math.isfinite(o) and abs(o) <= 16 for o in snapshot.frame.origin)
            and snapshot.frame.scale in (.25, .5, 1., 2., 4.), "unsupported frame")
    for records in groups:
        require(isinstance(records, tuple) and tuple(r.id for r in records) == tuple(range(1, len(records)+1)),
                "typed immutable record IDs differ")
    require(isinstance(snapshot.curves, tuple) and len(snapshot.curves) == len(snapshot.edges)
            and all(isinstance(c.data, tuple) for c in snapshot.curves), "immutable curve slots/count differ")
    require(isinstance(snapshot.patches, tuple) and len(snapshot.patches) == len(snapshot.charts), "patch/chart shape differs")
    require(isinstance(snapshot.packed_tables, tuple) and snapshot.packed_tables
            and len({name for name, _ in snapshot.packed_tables}) == len(snapshot.packed_tables)
            and all(isinstance(name, str) and isinstance(payload, bytes) for name, payload in snapshot.packed_tables),
            "packed tables are not owned immutable bytes")
    qualify_tables(snapshot)
    for slot, chart in enumerate(snapshot.charts):
        require(0 <= chart.face_slot < len(snapshot.faces), "chart face outside table")
        face = snapshot.faces[chart.face_slot]
        require(slot in face.chart_slots and chart.patch == snapshot.patches[slot], "chart/patch identity mismatch")
        patch = chart.patch
        require(isinstance(patch.controls, tuple), "patch controls are not frozen")
        require((patch.degree_u, patch.degree_v) == ((2, 1) if face.name == "wall" else (1, 1)), "wrong carrier degree")
        require(patch.origin == snapshot.frame.origin and patch.scale == snapshot.frame.scale, "patch frame differs")
        lineage = patch.lineage
        require(lineage is not None and (lineage.body_id, lineage.entity_kind, lineage.entity_id,
                lineage.revision, lineage.chart_id) == (snapshot.body_id, 5, face.id, 1, chart.id), "patch lineage differs")
        require(chart.quadrant in range(4) if face.name == "wall" else chart.quadrant == -1, "wrong quadrant")
        expected_uv = (chart.quadrant, chart.quadrant+1, 0, 1) if face.name == "wall" else (0, 1, 0, 1)
        require(patch.uv == expected_uv, "source chart rectangle differs")
    incidence = [[] for _ in snapshot.edges]
    for i, coedge in enumerate(snapshot.coedges):
        require(0 <= coedge.edge_slot < len(snapshot.edges) and 0 <= coedge.face_slot < len(snapshot.faces)
                and 0 <= coedge.loop_slot < len(snapshot.loops) and coedge.orientation in (-1, 1), "bad coedge reference")
        incidence[coedge.edge_slot].append(coedge)
        face = snapshot.faces[coedge.face_slot]
        loop = snapshot.loops[coedge.loop_slot]
        require(loop.face_slot == coedge.face_slot and i in loop.coedge_slots, "coedge ownership differs")
        edge = snapshot.edges[coedge.edge_slot]
        require(0 <= edge.curve_slot < len(snapshot.curves), "bad edge curve reference")
        curve = snapshot.curves[edge.curve_slot]
        for q in range(4) if curve.kind == "circle" else (0,):
            expected = curve_expression(curve, q)
            mapped = compose_face(snapshot, face, coedge.uv_map, q)
            require(all(same(a, b) for a, b in zip(expected, mapped)), "complete edge/UV correspondence differs")
    for i, edge in enumerate(snapshot.edges):
        require(0 <= edge.start < len(snapshot.vertices) and 0 <= edge.end < len(snapshot.vertices), "bad edge vertex reference")
        uses = incidence[i]
        require(len(uses) == 2 and sum(use.orientation for use in uses) == 0, "edge does not have two opposed uses")
        curve = snapshot.curves[edge.curve_slot]
        for parameter, vertex, q, local in ((0, edge.start, 0, F(0)), (1, edge.end, 3, F(1))):
            point = tuple(value(a, local if curve.kind == "circle" else F(parameter)) for a in curve_expression(curve, q))
            require(point == tuple(map(exact, snapshot.vertices[vertex].position)), "edge/vertex endpoint differs")
        if curve.kind == "circle":
            require(edge.start == edge.end, "circle invented extra topology vertices")
    referenced_coedges, referenced_loops, referenced_charts = [], [], []
    for face_slot, face in enumerate(snapshot.faces):
        expected_sign = (-1 if snapshot.shape == "plate" else 1) if face.name == "wall" else SIGNS[face.name]
        require(face.orientation == expected_sign, "face orientation differs from material role")
        expected_kind = "cylinder" if face.name == "wall" else "disk" if snapshot.shape == "cylinder" else \
            "rectangle_hole" if snapshot.shape == "plate" and face.name in ("top", "bottom") else "rectangle"
        require(face.kind == expected_kind, "face trim kind differs")
        require(len(face.chart_slots) == (4 if face.name == "wall" else 1), "face chart coverage differs")
        if face.name == "wall":
            require(tuple(snapshot.charts[i].quadrant for i in face.chart_slots) == (0, 1, 2, 3), "wall atlas order differs")
        referenced_charts.extend(face.chart_slots); referenced_loops.extend(face.loop_slots)
        require(sum(bool(snapshot.loops[i].hole) for i in face.loop_slots)
                == int(expected_kind == "rectangle_hole"), "face hole count differs")
        for loop_slot in face.loop_slots:
            loop = snapshot.loops[loop_slot]
            require(loop.face_slot == face_slot and loop.coedge_slots, "loop/face ownership differs")
            referenced_coedges.extend(loop.coedge_slots)
            uses = [snapshot.coedges[i] for i in loop.coedge_slots]
            starts = [uv_endpoint(c.uv_map, F(0) if c.orientation == 1 else F(1)) for c in uses]
            ends = [uv_endpoint(c.uv_map, F(1) if c.orientation == 1 else F(0)) for c in uses]
            require(all(end == starts[(i+1) % len(starts)] for i, end in enumerate(ends)), "ordered UV loop does not close")
            if len(uses) == 1 and uses[0].uv_map.kind == "circle_plane":
                winding = uses[0].orientation
            else:
                area = sum(start[0]*end[1]-end[0]*start[1] for start, end in zip(starts, ends))
                require(area != 0, "loop has no oriented area")
                winding = 1 if area > 0 else -1
            require(winding == expected_sign*(-1 if loop.hole else 1), "loop outer/hole orientation differs")
    for used, records in ((referenced_coedges, snapshot.coedges), (referenced_loops, snapshot.loops),
                          (referenced_charts, snapshot.charts)):
        require(sorted(used) == list(range(len(records))), "records missing or multiply owned")
    if snapshot.shape != "box":
        seam_uses = [c for c in snapshot.coedges if c.uv_map.kind == "seam_wall"]
        require(len(seam_uses) == 2 and seam_uses[0].edge_slot == seam_uses[1].edge_slot
                and seam_uses[0].face_slot == seam_uses[1].face_slot
                and {c.uv_map.data[0] for c in seam_uses} == {0, 4}, "same-face periodic seam collapsed")
    euler = len(snapshot.vertices)-len(snapshot.edges)+len(snapshot.faces)-sum(bool(loop.hole) for loop in snapshot.loops)
    require(euler == (0 if snapshot.shape == "plate" else 2), "hole-sensitive boundary Euler differs")
    return {"vertices": len(snapshot.vertices), "edges": len(snapshot.edges), "faces": len(snapshot.faces), "euler": euler}


def qualify_batch(batch):
    qualify_snapshot(batch.snapshot)
    require(isinstance(batch.rows, tuple) and batch.count == len(batch.rows), "batch row count/storage differs")
    stats = dict(count=batch.count, accepted=0, rejected=0, invalid=0, boundary=0,
                 max_position_error=0., max_normal_error=0.)
    max_position, max_normal = F(0), F(0)
    for index, row in enumerate(batch.rows):
        chart, face = role(batch.snapshot, row.patch_slot)
        require((row.index, row.face_id, row.chart_id) == (index, face.id, chart.id), "scatter/source identity differs")
        expected_label = trim_label(batch.snapshot, row.patch_slot, row.u, row.v)
        require(row.trim == expected_label, "submitted binary UV trim label differs")
        require(isinstance(row.position, tuple) and isinstance(row.normal, tuple)
                and len(row.position) == len(row.normal) == 3, "row vector ownership/shape differs")
        if expected_label in (REJECTED, INVALID):
            stats["rejected" if expected_label == REJECTED else "invalid"] += 1
            require(row.native_status == -1 and all(math.isnan(x) for x in (*row.position, *row.normal)), "rejected/invalid row was evaluated")
            continue
        stats["accepted"] += 1
        stats["boundary"] += expected_label == BOUNDARY
        require(row.native_status == 0 and all(math.isfinite(x) for x in (*row.position, *row.normal)), "admitted row failed native/normal work")
        point, normal = ideal_geometry(batch.snapshot, row.patch_slot, row.u, row.v)
        for actual, expected in zip(row.position, point):
            error = abs(exact(actual)-expected); max_position = max(max_position, error)
            require(error <= POSITION_LIMIT, "world position exceeds exact analytic target")
        for actual, expected in zip(row.normal, normal):
            error = abs(exact(actual)-expected); max_normal = max(max_normal, error)
            require(error <= NORMAL_LIMIT, "oriented unit normal exceeds analytic target")
    require(batch.evaluated_count == stats["accepted"], "evaluated/gather count differs")
    require(isinstance(batch.timings, tuple) and len({name for name, _ in batch.timings}) == len(batch.timings)
            and all(isinstance(name, str) and math.isfinite(ms) and ms >= 0 for name, ms in batch.timings), "invalid immutable timing records")
    stats["max_position_error"], stats["max_normal_error"] = float(max_position), float(max_normal)
    return stats
