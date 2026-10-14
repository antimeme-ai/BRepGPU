"""Independent original-input enclosure and closed-box/page truth.

Expected bounds come from exact submitted homogeneous controls and frames;
expected bits come from Fractions of actual resident endpoint values. No product
classifier, rounding helper, native CPU parity or pointer identity supplies truth.
"""
from fractions import Fraction
import math
import struct

MAX_PAIRS = 1 << 20
MAX_INDEX_COUNT = 1 << 16


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def exact(value):
    value = float(value)
    require(math.isfinite(value), "nonfinite oracle value")
    return Fraction.from_float(value)


def checked_boxes(records):
    records = tuple(tuple(record) for record in records)
    require(len(records) <= MAX_INDEX_COUNT, "box collection exceeds admitted count")
    answer = []
    for record in records:
        require(len(record) == 6, "box record is not flat lowerXYZ/upperXYZ")
        values = tuple(map(exact, record))
        require(all(values[a] <= values[a+3] for a in range(3)), "reversed actual box")
        answer.append(values)
    return records, tuple(answer)


def same_boxes(actual, intended):
    actual, _ = checked_boxes(actual)
    intended, _ = checked_boxes(intended)
    require(len(actual) == len(intended) and all(
        struct.pack("<6d", *a) == struct.pack("<6d", *b)
        for a, b in zip(actual, intended)), "resident boxes differ from immutable uploaded inputs")
    return actual


def qualify_scene(scene, actual_boxes=None):
    require(all(isinstance(getattr(scene, name), tuple)
                for name in ("bodies", "sources", "local_boxes", "boxes", "timings")),
            "scene storage is not immutable tuples")
    require(len({id(body) for body in scene.bodies}) == len(scene.bodies), "duplicate scene body object")
    require(len({body.body_id for body in scene.bodies}) == len(scene.bodies), "duplicate scene body namespace")
    stored, worlds = checked_boxes(scene.boxes)
    _, locals_ = checked_boxes(scene.local_boxes)
    require(len(scene.sources) == len(worlds) == len(locals_), "source/local/world count differs")
    if actual_boxes is not None:
        same_boxes(actual_boxes, stored)
    order, faces, selected = [], set(), {}
    control_count = 0
    for source, local, world in zip(scene.sources, locals_, worlds):
        snapshot = source.snapshot
        body_slots = [i for i, body in enumerate(scene.bodies) if body is snapshot]
        require(len(body_slots) == 1, "source lost exact scene snapshot identity")
        slot = source.patch_slot
        require(isinstance(slot, int) and 0 <= slot < len(snapshot.charts), "source patch slot outside snapshot")
        chart = snapshot.charts[slot]; face = snapshot.faces[chart.face_slot]
        require((source.face_id, source.chart_id) == (face.id, chart.id), "source face/chart lineage differs")
        key = (snapshot.body_id, face.id)
        faces.add(key); selected.setdefault(key, []).append(slot)
        order.append((body_slots[0], slot))
        patch = snapshot.patches[slot]
        require(chart.patch == patch, "source chart and native patch disagree")
        lineage = patch.lineage
        require(lineage is not None and (lineage.body_id, lineage.entity_kind, lineage.entity_id,
                lineage.revision, lineage.chart_id) ==
                (snapshot.body_id, 5, face.id, snapshot.revision, chart.id), "original patch lineage differs")
        require(isinstance(patch.controls, tuple) and 0 <= patch.degree_u <= 3 and 0 <= patch.degree_v <= 3
                and len(patch.controls) == 4*(patch.degree_u+1)*(patch.degree_v+1), "original control shape differs")
        controls = tuple(tuple(map(exact, patch.controls[i:i+4])) for i in range(0, len(patch.controls), 4))
        require(all(1 <= c[3] <= 256 and all(abs(c[a]) <= c[3] for a in range(3)) for c in controls),
                "original controls outside positive-weight carrier envelope")
        scale = exact(snapshot.frame.scale); origin = tuple(map(exact, snapshot.frame.origin))
        require(scale in (Fraction(1,4), Fraction(1,2), 1, 2, 4) and len(origin) == 3
                and all(abs(o) <= 16 for o in origin), "source frame outside declared family")
        require(patch.origin == snapshot.frame.origin and patch.scale == snapshot.frame.scale,
                "native patch frame differs from retained source")
        for a in range(3):
            values = tuple(c[a]/c[3] for c in controls)
            lower, upper = min(values), max(values)
            require(local[a] <= lower <= upper <= local[a+3], "local root excludes original exact control extrema")
            require(world[a] <= origin[a]+scale*lower <= origin[a]+scale*upper <= world[a+3],
                    "world box excludes exact original carrier extrema")
            require(world[a] <= origin[a]+scale*local[a]
                    and origin[a]+scale*local[a+3] <= world[a+3], "world transform excludes actual root endpoints")
        control_count += len(controls)
    require(order == sorted(set(order)), "sources are repeated or out of canonical body/chart order")
    for snapshot in scene.bodies:
        for face in snapshot.faces:
            key = (snapshot.body_id, face.id)
            if key in selected:
                require(tuple(selected[key]) == face.chart_slots, "selected face lost a carrier chart")
    require(len({name for name, _ in scene.timings}) == len(scene.timings)
            and all(isinstance(name, str) and math.isfinite(ms) and ms >= 0 for name, ms in scene.timings),
            "invalid scene preparation timings")
    return dict(bodies=len(scene.bodies), charts=len(worlds), faces=len(faces), controls=control_count)


def overlaps(left, right):
    return all(left[a] <= right[a+3] and right[a] <= left[a+3] for a in range(3))


def expected_words(left_boxes, right_boxes, start=0, max_pairs=MAX_PAIRS):
    _, left = checked_boxes(left_boxes); _, right = checked_boxes(right_boxes)
    total = len(left)*len(right)
    require(isinstance(start, int) and 0 <= start <= total, "invalid oracle start")
    require(isinstance(max_pairs, int) and 1 <= max_pairs <= MAX_PAIRS, "invalid oracle page budget")
    count = min(max_pairs, total-start)
    words = [0]*((count+31)//32)
    for k in range(count):
        i, j = divmod(start+k, len(right))
        if overlaps(left[i], right[j]):
            words[k//32] |= 1 << (k%32)
    return tuple(words)


def qualify_page(page, left_boxes=None, right_boxes=None, *, start=None, max_pairs=None, profiled=None):
    scenes = hasattr(page, "left") and hasattr(page, "right")
    if scenes:
        if left_boxes is None: left_boxes = page.left.boxes
        if right_boxes is None: right_boxes = page.right.boxes
        qualify_scene(page.left, left_boxes); qualify_scene(page.right, right_boxes)
    require(left_boxes is not None and right_boxes is not None, "actual boxes required for raw page truth")
    left, _ = checked_boxes(left_boxes); right, _ = checked_boxes(right_boxes)
    info = page.info
    total = len(left)*len(right)
    require(info.abi_version == 4 and info.struct_size == 80, "page ABI metadata differs")
    require((info.left_count, info.right_count, info.total_pairs) == (len(left), len(right), total),
            "page source dimensions differ")
    require(isinstance(info.start, int) and 0 <= info.start <= total
            and 0 <= info.count <= min(MAX_PAIRS, total-info.start), "page interval invalid")
    if start is not None: require(info.start == start, "page starts outside requested position")
    if max_pairs is not None:
        require(1 <= max_pairs <= MAX_PAIRS and info.count == min(max_pairs, total-info.start),
                "page did not examine the requested bounded interval")
    require(info.next_start == info.start+info.count and info.word_count == (info.count+31)//32,
            "page cursor or word extent differs")
    end = info.next_start == total; full = info.start == 0 and info.count == total
    require(info.backend in (0, 1) and info.flags & ~7 == 0 and info.flags & 3 == int(end)+2*int(full),
            "page backend/coverage flags differ")
    measured = bool(info.flags & 4)
    require(not measured or (info.backend == 1 and info.count > 0), "profile flag set on CPU/empty page")
    require((math.isfinite(info.kernel_ms) and info.kernel_ms >= 0) if measured else math.isnan(info.kernel_ms),
            "profile duration does not match profile scope")
    if profiled is not None:
        require(measured == bool(profiled and info.backend == 1 and info.count), "profile request was not honored")
    words = tuple(page.words)
    require(len(words) == info.word_count and all(isinstance(w, int) and 0 <= w < 1 << 32 for w in words),
            "word type/count differs")
    # Explicit budget count preserves empty pages: expected_words needs >=1 budget.
    expected = expected_words(left, right, info.start, max(1, info.count))
    require(words == expected, "actual closed-box mask or tail padding differs")
    survivors = sum(word.bit_count() for word in words)
    if scenes:
        require(isinstance(page.words, tuple) and isinstance(page.timings, tuple), "scene page is not immutable")
        actual = iter(page.survivors())
        for k in range(info.count):
            if not words[k//32] & (1 << (k%32)): continue
            flat = info.start+k; i, j = divmod(flat, len(right))
            row = next(actual, None)
            require(row is not None and len(row) == 3 and row[0] == flat
                    and row[1] is page.left.sources[i] and row[2] is page.right.sources[j],
                    "survivor index or exact source mapping differs")
        require(next(actual, None) is None, "consumer emitted an unexamined/tail survivor")
        require(len({name for name, _ in page.timings}) == len(page.timings)
                and all(math.isfinite(ms) and ms >= 0 for _, ms in page.timings), "invalid page timings")
    return dict(total_pairs=total, start=info.start, count=info.count, next_start=info.next_start,
                word_count=info.word_count, survivors=survivors, excluded=info.count-survivors,
                reached_end=int(end), full_domain=int(full))
