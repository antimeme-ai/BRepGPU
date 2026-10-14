"""Independent source-relation and downloaded-byte truth for ABI5 adjacency.

No native count/scan/fill implementation or product reader supplies expected
membership. Literal body loops additionally qualify body-to-flat projection.
"""
from collections import Counter
import struct


INVALID = (1 << 32) - 1
MAX_COUNT = 1 << 22
_FACE_NAMES = {
    "box": ("top", "bottom", "front", "back", "right", "left"),
    "cylinder": ("top", "bottom", "wall"),
    "plate": ("top", "bottom", "front", "back", "right", "left", "wall"),
}
_OUTER = (
    ((5, 1), (6, 1), (7, 1), (8, 1)),
    ((4, -1), (3, -1), (2, -1), (1, -1)),
    ((1, 1), (10, 1), (5, -1), (9, -1)),
    ((12, 1), (7, -1), (11, -1), (3, 1)),
    ((2, 1), (11, 1), (6, -1), (10, -1)),
    ((9, 1), (8, -1), (12, -1), (4, 1)),
)
_LINE_UV = (
    ((0., 0., 1., 0.), (1., 0., 1., 1.), (1., 1., 0., 1.), (0., 1., 0., 0.)),
    ((0., 1., 0., 0.), (1., 1., 0., 1.), (1., 0., 1., 1.), (0., 0., 1., 0.)),
    ((0., 0., 1., 0.), (1., 0., 1., 1.), (0., 1., 1., 1.), (0., 0., 0., 1.)),
    ((0., 0., 0., 1.), (1., 1., 0., 1.), (1., 0., 1., 1.), (1., 0., 0., 0.)),
    ((0., 0., 1., 0.), (1., 0., 1., 1.), (0., 1., 1., 1.), (0., 0., 0., 1.)),
    ((0., 0., 0., 1.), (1., 1., 0., 1.), (1., 0., 1., 1.), (1., 0., 0., 0.)),
)


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def _key(source):
    values = (source.instance, source.body_id, source.revision, source.entity_id)
    require(all(type(value) is int for value in values), "source identity is not integral")
    require(0 <= values[0] < (1 << 64) and all(0 < value < (1 << 64) for value in values[1:]),
            "source identity outside uint64 contract")
    return values


def _decode(payload, count, signed=False):
    require(type(payload) is bytes, "array is not owned immutable bytes")
    require(len(payload) == 4 * count, "array byte extent differs")
    code = "<i" if signed else "<I"
    return tuple(value[0] for value in struct.iter_unpack(code, payload))


def _model(source):
    groups = (source.edge_sources, source.face_sources, source.coedge_sources)
    require(all(type(group) is tuple for group in groups), "source tables are not immutable tuples")
    counts = tuple(map(len, groups))
    require(counts == (source.edge_count, source.face_count, source.coedge_count), "public table counts differ")
    require(all(count <= MAX_COUNT for count in counts), "source count exceeds admitted cap")
    e, f, c = counts
    require(c == 0 or (e > 0 and f > 0), "coedges require edge and face tables")
    keys = tuple(tuple(_key(item) for item in group) for group in groups)
    namespaces = {}
    for group in keys:
        require(len(set(group)) == len(group), "duplicate typed source key")
        for instance, body, revision, entity in group:
            require(instance not in namespaces or namespaces[instance] == (body, revision),
                    "one instance names multiple body revisions")
            namespaces[instance] = body, revision
    edges = _decode(source.coedge_edges, c)
    faces = _decode(source.coedge_faces, c)
    signs = _decode(source.orientations, c, True)
    relation = {key: [] for key in keys[0]}
    for use, (edge, face, sign) in enumerate(zip(edges, faces, signs)):
        require(edge < e and face < f, "submitted reference outside source table")
        require(sign in (-1, 1), "submitted orientation differs from +/-1")
        require(keys[2][use][:3] == keys[0][edge][:3] == keys[1][face][:3],
                "submitted reference crosses an instance/body/revision")
        relation[keys[0][edge]].append(use)
    return counts, keys, namespaces, edges, faces, signs, relation


def _literal_records(shape):
    """(edge ID, face ID, sign, loop ID, hole, UV kind, UV data)."""
    if shape == "cylinder":
        return (
            (2, 1, 1, 1, False, "circle_plane", (.25, .25)),
            (1, 2, -1, 2, False, "circle_plane", (.25, .25)),
            (1, 3, 1, 3, False, "circle_wall", (0.,)),
            (3, 3, 1, 3, False, "seam_wall", (4.,)),
            (2, 3, -1, 3, False, "circle_wall", (1.,)),
            (3, 3, -1, 3, False, "seam_wall", (0.,)),
        )
    require(shape in ("box", "plate"), "no literal primitive source contract")
    rows = []
    loop = 0
    for face, uses in enumerate(_OUTER, 1):
        loop += 1
        for (edge, sign), uv in zip(uses, _LINE_UV[face-1]):
            rows.append((edge, face, sign, loop, False, "line", uv))
        if shape == "plate" and face <= 2:
            loop += 1
            rows.append((14 if face == 1 else 13, face, -1 if face == 1 else 1,
                         loop, True, "circle_plane", (.25, .75)))
    if shape == "plate":
        rows.extend((
            (15, 7, 1, 9, False, "seam_wall", (0.,)),
            (14, 7, 1, 9, False, "circle_wall", (1.,)),
            (15, 7, -1, 9, False, "seam_wall", (4.,)),
            (13, 7, -1, 9, False, "circle_wall", (0.,)),
        ))
    return tuple(rows)


def _literal_projection(source, model):
    counts, keys, namespaces, edges, faces, signs, relation = model
    snapshots = source.snapshots
    require(type(snapshots) is tuple and len(snapshots) == len(namespaces),
            "literal input lacks one retained original snapshot per instance")
    require(set(namespaces) == set(range(len(snapshots))), "literal body occurrence ordinals differ")
    expected = ({}, {}, {})
    for instance, body in enumerate(snapshots):
        require(body.shape in _FACE_NAMES, "unsupported literal body")
        require(namespaces[instance] == (body.body_id, body.revision), "retained snapshot lineage differs")
        require(body.revision == 1, "literal fixture revision differs")
        require(tuple(face.name for face in body.faces) == _FACE_NAMES[body.shape], "primitive face roles differ")
        rows = _literal_records(body.shape)
        e = 3 if body.shape == "cylinder" else 15 if body.shape == "plate" else 12
        require((len(body.edges), len(body.faces), len(body.coedges)) == (e, len(_FACE_NAMES[body.shape]), len(rows)),
                "primitive source graph counts differ")
        prefix = (instance, body.body_id, body.revision)
        for entity in range(1, e+1):
            expected[0][prefix+(entity,)] = None
        for entity in range(1, len(body.faces)+1):
            expected[1][prefix+(entity,)] = None
        loop_uses = {}
        for entity, wanted in enumerate(rows, 1):
            edge_id, face_id, sign, loop_id, hole, uv_kind, uv_data = wanted
            original = body.coedges[entity-1]
            require(original.id == entity, "retained coedge identity differs")
            require((body.edges[original.edge_slot].id, body.faces[original.face_slot].id,
                     original.orientation, body.loops[original.loop_slot].id) ==
                    (edge_id, face_id, sign, loop_id), "retained signed primitive use differs")
            owner = body.loops[original.loop_slot]
            require((owner.hole, owner.face_slot) == (hole, face_id-1), "retained hole/loop ownership differs")
            require((original.uv_map.kind, original.uv_map.data) == (uv_kind, uv_data),
                    "retained UV occurrence differs from literal primitive")
            loop_uses.setdefault(loop_id, []).append(entity-1)
            expected[2][prefix+(entity,)] = (prefix+(edge_id,), prefix+(face_id,), sign)
        require(len(body.loops) == len(loop_uses), "retained loop count differs")
        for loop_id, uses in loop_uses.items():
            require(body.loops[loop_id-1].coedge_slots == tuple(uses), "original directed loop order differs")
    for kind in range(3):
        require(set(keys[kind]) == set(expected[kind]), "flattened typed primitive source set differs")
    for use, key in enumerate(keys[2]):
        require((keys[0][edges[use]], keys[1][faces[use]], signs[use]) == expected[2][key],
                "flattened signed primitive use differs from source truth")


def qualify_input(source, *, literal=False):
    model = _model(source)
    if literal:
        _literal_projection(source, model)
    (e, f, c), keys, namespaces, edges, faces, signs, relation = model
    return dict(edges=e, faces=f, coedges=c, instances=len(namespaces))


def qualify_result(result, *, literal=False):
    source = result.input
    model = _model(source)
    if literal:
        _literal_projection(source, model)
    (e, f, c), keys, namespaces, edges, faces, signs, relation = model
    offsets = _decode(result.offsets, e+1)
    members = _decode(result.members, c)
    kinds = _decode(result.kinds, e)
    mates = _decode(result.mates, c)
    neighbors = _decode(result.neighbors, c)
    require(offsets[0] == 0 and offsets[-1] == c, "CSR first/final offsets differ")
    require(all(0 <= a <= b <= c for a, b in zip(offsets, offsets[1:])), "CSR offsets outside ordered extent")
    totals = Counter()
    for edge, key in enumerate(keys[0]):
        wanted = relation[key]
        actual = members[offsets[edge]:offsets[edge+1]]
        require(Counter(actual) == Counter(wanted), "CSR typed edge-use multiplicity differs")
        degree = len(wanted)
        kind = (0 if degree == 0 else 1 if degree == 1 else 5 if degree > 2 else
                4 if signs[wanted[0]] == signs[wanted[1]] else
                3 if keys[1][faces[wanted[0]]] == keys[1][faces[wanted[1]]] else 2)
        require(kinds[edge] == kind, "reader edge kind differs")
        totals[kind] += 1
        for use in wanted:
            if kind in (2, 3):
                other = wanted[1] if use == wanted[0] else wanted[0]
                require(mates[use] == other, "reader mate coedge identity differs")
                require(neighbors[use] == faces[other], "reader neighbor face identity differs")
            else:
                require(mates[use] == INVALID and neighbors[use] == INVALID, "ambiguous reader use acquired a mate")
    return dict(edges=e, faces=f, coedges=c, instances=len(namespaces),
                unused=totals[0], boundary=totals[1], paired=totals[2],
                same_face_pair=totals[3], orientation_conflict=totals[4], multiple_uses=totals[5])
