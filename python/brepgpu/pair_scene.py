"""Immutable body carrier scenes and a bounded, body-aware pair consumer.

CPU preparation encloses full rectangular carriers in a common world frame.
Set page bits are closed-AABB survivors, not contacts or intersections. No
body topology crosses the native boundary; only flat-six box records do.
"""
from contextlib import contextmanager
from dataclasses import dataclass
from fractions import Fraction
import math
import operator
import time

from . import BG_INVALID_HANDLE, BgError, Engine, HierarchyOptions
from .body import BodySnapshot


MAX_SCENE_ENTRIES = 65536
MAX_PAGE_PAIRS = 1048576
_NO_CHILD = (1 << 32) - 1
_REACHED_END, _FULL_DOMAIN, _PROFILED = 1, 2, 4


def _elapsed(start):
    return 1000. * (time.perf_counter() - start)


def _cleanup_note(primary, cleanup):
    try:
        primary.brepgpu_cleanup_error = cleanup
        if hasattr(primary, "add_note"):
            primary.add_note("Pair scene cleanup also failed: " + str(cleanup))
    except BaseException:
        pass


@contextmanager
def _child_scope(resource):
    try:
        yield resource
    except BaseException as primary:
        try:
            resource.close()
        except BaseException as cleanup:
            _cleanup_note(primary, cleanup)
        raise
    else:
        resource.close()


def _integer(value, name):
    if isinstance(value, bool):
        raise TypeError(name + " must be an integer")
    return operator.index(value)


def _engine(engine, cpu=False):
    if not isinstance(engine, Engine):
        raise TypeError("pair scenes require an Engine")
    if not engine.handle:
        raise BgError(BG_INVALID_HANDLE, message="pair scene engine is closed")
    if cpu and engine.backend != "cpu":
        raise ValueError("scene bounds must be prepared with a CPU engine")
    return engine


@dataclass(frozen=True, slots=True)
class PairSource:
    snapshot: BodySnapshot
    patch_slot: int
    face_id: int
    chart_id: int


@dataclass(frozen=True, slots=True, init=False)
class PairScene:
    bodies: tuple
    sources: tuple
    local_boxes: tuple
    boxes: tuple
    timings: tuple

    def __init__(self, *args, **kwargs):
        raise TypeError("PairScene construction is factory-only; use prepare_scene")


def _gather(bodies, face_ids):
    copied, known = [], {}
    for index, snapshot in enumerate(bodies):
        if index >= MAX_SCENE_ENTRIES:
            raise ValueError("scene body input exceeds 65536 entries")
        if type(snapshot) is not BodySnapshot:
            raise TypeError("scene bodies must be exact owned BodySnapshot values")
        if snapshot.body_id in known:
            raise ValueError("duplicate scene body")
        copied.append(snapshot)
        known[snapshot.body_id] = {face.id for face in snapshot.faces}
    selected = None
    if face_ids is not None:
        selected = set()
        for index, value in enumerate(face_ids):
            if index >= MAX_SCENE_ENTRIES:
                raise ValueError("scene face selection exceeds 65536 entries")
            try:
                body_id, face_id = value
            except (TypeError, ValueError) as error:
                raise TypeError("face keys must contain (body_id, face_id)") from error
            key = (_integer(body_id, "body ID"), _integer(face_id, "face ID"))
            if key[0] not in known or key[1] not in known[key[0]]:
                raise ValueError("selected face key is outside the scene bodies")
            if key in selected:
                raise ValueError("duplicate selected face key")
            selected.add(key)
    sources = []
    for snapshot in copied:
        for slot, chart in enumerate(snapshot.charts):
            face = snapshot.faces[chart.face_slot]
            if selected is not None and (snapshot.body_id, face.id) not in selected:
                continue
            if len(sources) >= MAX_SCENE_ENTRIES:
                raise ValueError("scene exceeds 65536 admitted charts")
            sources.append(PairSource(snapshot, slot, face.id, chart.id))
    return tuple(copied), tuple(sources)


def _local_roots(download, count):
    info = download.info
    if (int(info.patch_count), int(info.node_count), int(info.depth),
        int(info.backend), int(info.flags), int(info.node_size)) != (count, count, 0, 0, 0, 80):
        raise RuntimeError("scene hierarchy metadata differs from depth-zero AABB inputs")
    if len(download.roots) != count or len(download.nodes) != count or len(download.bounds):
        raise RuntimeError("scene hierarchy download has an unexpected extent")
    roots = tuple(int(value) for value in download.roots)
    if len(set(roots)) != count or any(not 0 <= root < count for root in roots):
        raise RuntimeError("scene hierarchy root identities are invalid")
    boxes = []
    for slot, root in enumerate(roots):
        node = download.nodes[root]
        if (int(node.patch_slot), int(node.left), int(node.right), int(node.flags),
            int(node.u_index), int(node.v_index), int(node.u_level), int(node.v_level)) \
                != (slot, _NO_CHILD, _NO_CHILD, 1, 0, 0, 0, 0):
            raise RuntimeError("scene root does not cover its original whole patch")
        box = tuple(float(value) for value in (*node.lower, *node.upper))
        if any(not math.isfinite(value) for value in box) or any(box[a] > box[a+3] for a in range(3)):
            raise RuntimeError("scene root is not a finite closed box")
        boxes.append(box)
    return tuple(boxes)


def _outward(value, lower):
    rounded = float(value)
    if not math.isfinite(rounded):
        raise ArithmeticError("world scene bound is not representable")
    represented = Fraction.from_float(rounded)
    if (lower and represented > value) or (not lower and represented < value):
        rounded = math.nextafter(rounded, -math.inf if lower else math.inf)
    if not math.isfinite(rounded):
        raise ArithmeticError("outward world scene bound is not finite")
    represented = Fraction.from_float(rounded)
    if (lower and represented > value) or (not lower and represented < value):
        raise ArithmeticError("world scene endpoint failed exact containment")
    return rounded


def _world_box(box, frame):
    scale = Fraction.from_float(frame.scale)
    origins = tuple(Fraction.from_float(value) for value in frame.origin)
    exact = tuple(origins[a % 3] + scale * Fraction.from_float(value)
                  for a, value in enumerate(box))
    result = tuple(_outward(value, index < 3) for index, value in enumerate(exact))
    if any(result[a] > result[a+3] for a in range(3)):
        raise ArithmeticError("world scene box endpoints are reversed")
    return result


def prepare_scene(cpu_engine, bodies, face_ids=None):
    """Prepare selected full carriers; face keys are (body_id, face_id)."""
    _engine(cpu_engine, cpu=True)
    wall = time.perf_counter()
    bodies, sources = _gather(bodies, face_ids)
    selection_ms = _elapsed(wall)
    start = time.perf_counter()
    if sources:
        patches = tuple(source.snapshot.patches[source.patch_slot] for source in sources)
        with _child_scope(cpu_engine.create_patch_set(patches)) as native_patches:
            with _child_scope(cpu_engine.create_hierarchy(native_patches,
                    HierarchyOptions(depth=0, max_nodes=len(sources), mode=0))) as hierarchy:
                local_boxes = _local_roots(hierarchy.download(), len(sources))
    else:
        local_boxes = ()
    local_bounds_ms = _elapsed(start)
    start = time.perf_counter()
    boxes = tuple(_world_box(box, source.snapshot.frame)
                  for box, source in zip(local_boxes, sources))
    world_bounds_ms = _elapsed(start)
    timings = (("selection_ms", selection_ms), ("local_bounds_ms", local_bounds_ms),
               ("world_bounds_ms", world_bounds_ms), ("prepare_wall_ms", _elapsed(wall)))
    scene = object.__new__(PairScene)
    for name, value in (("bodies", bodies), ("sources", sources),
                        ("local_boxes", local_boxes), ("boxes", boxes), ("timings", timings)):
        object.__setattr__(scene, name, value)
    return scene


@dataclass(frozen=True, slots=True)
class PairPageInfo:
    abi_version: int
    struct_size: int
    total_pairs: int
    start: int
    count: int
    next_start: int
    left_count: int
    right_count: int
    word_count: int
    backend: int
    flags: int
    kernel_ms: float


@dataclass(frozen=True, slots=True)
class ScenePairPage:
    info: PairPageInfo
    words: tuple
    left: PairScene
    right: PairScene
    timings: tuple

    def survivors(self):
        """Yield examined survivor occurrences; no intersection is asserted."""
        for word_slot, original in enumerate(self.words):
            word = original
            while word:
                bit = (word & -word).bit_length() - 1
                local = 32 * word_slot + bit
                if local >= self.info.count:
                    raise RuntimeError("pair page contains a set padding bit")
                flat = self.info.start + local
                left_slot, right_slot = divmod(flat, self.info.right_count)
                yield flat, self.left.sources[left_slot], self.right.sources[right_slot]
                word &= word - 1


def _page_copy(batch, left, right, start, budget, backend, profile):
    native = batch.info
    info = PairPageInfo(*(int(getattr(native, name)) for name in (
        "abi_version", "struct_size", "total_pairs", "start", "count", "next_start",
        "left_count", "right_count", "word_count", "backend", "flags")),
        float(native.kernel_ms))
    nleft, nright = len(left.sources), len(right.sources)
    total = nleft * nright
    count = min(budget, total - start)
    if (info.abi_version, info.struct_size, info.total_pairs, info.start, info.count,
        info.next_start, info.left_count, info.right_count, info.word_count, info.backend) \
            != (4, 80, total, start, count, start+count, nleft, nright, (count+31)//32, backend):
        raise RuntimeError("pair page metadata differs from the retained scene request")
    expected_flags = (_REACHED_END if start+count == total else 0) \
                   | (_FULL_DOMAIN if start == 0 and count == total else 0) \
                   | (_PROFILED if backend == 1 and profile and count else 0)
    if info.flags != expected_flags:
        raise RuntimeError("pair page flags differ from its coverage/profile scope")
    if expected_flags & _PROFILED:
        if not math.isfinite(info.kernel_ms) or info.kernel_ms < 0:
            raise RuntimeError("profiled pair page has invalid event duration")
    elif not math.isnan(info.kernel_ms):
        raise RuntimeError("unprofiled pair page must have NaN event duration")
    words = tuple(int(value) for value in batch.words)
    if len(words) != info.word_count or any(not 0 <= value < (1 << 32) for value in words):
        raise RuntimeError("pair mask download has an invalid extent or word")
    remainder = count % 32
    if remainder and words[-1] >> remainder:
        raise RuntimeError("pair mask has nonzero tail padding")
    return info, words


class PairDiscovery:
    """Borrow an engine, own two indexes and bind every page to exact scenes."""
    def __init__(self, engine, left_scene, right_scene):
        _engine(engine)
        if type(left_scene) is not PairScene or type(right_scene) is not PairScene:
            raise TypeError("pair discovery requires exact factory-created PairScene values")
        self._engine = engine
        self._left, self._right = left_scene, right_scene
        self._left_index = self._right_index = None
        wall = time.perf_counter()
        try:
            self._adopt_index("_left_index", left_scene.boxes)
            self._adopt_index("_right_index", right_scene.boxes)
            self._timings = (("index_upload_ms", _elapsed(wall)),)
        except BaseException as primary:
            self._close_owned(primary)
            raise

    def _adopt_index(self, name, boxes):
        resource = self._engine.create_pair_index(boxes)
        try:
            setattr(self, name, resource)
        except BaseException as primary:
            try:
                resource.close()
            except BaseException as cleanup:
                _cleanup_note(primary, cleanup)
            raise

    @property
    def left(self):
        return self._left

    @property
    def right(self):
        return self._right

    @property
    def timings(self):
        return self._timings

    def _ready(self):
        _engine(self._engine)
        if self._left_index is None or self._right_index is None \
                or not self._left_index.handle or not self._right_index.handle:
            raise BgError(BG_INVALID_HANDLE, message="pair discovery is closed")
        return self._left_index, self._right_index

    def _close_owned(self, primary=None):
        first = None
        for name in ("_right_index", "_left_index"):
            resource = getattr(self, name, None)
            if resource is None:
                continue
            try:
                resource.close()
            except BaseException as cleanup:
                if primary is not None:
                    _cleanup_note(primary, cleanup)
                elif first is None:
                    first = cleanup
                else:
                    _cleanup_note(first, cleanup)
            finally:
                if not resource.handle:
                    setattr(self, name, None)
        if primary is None and first is not None:
            raise first

    def close(self):
        self._close_owned()

    def __enter__(self):
        self._ready()
        return self

    def __exit__(self, kind, error, traceback):
        self._close_owned(error)
        return False

    def download_boxes(self):
        """Download actual resident inputs for independent qualification."""
        left, right = self._ready()
        return (tuple(tuple(float(value) for value in box) for box in left.download()),
                tuple(tuple(float(value) for value in box) for box in right.download()))

    def page(self, start=0, max_pairs=MAX_PAGE_PAIRS):
        left, right = self._ready()
        start = _integer(start, "page start")
        budget = _integer(max_pairs, "page budget")
        total = len(self._left.sources) * len(self._right.sources)
        if not 0 <= start <= total:
            raise ValueError("page start is outside the Cartesian scene domain")
        if not 1 <= budget <= MAX_PAGE_PAIRS:
            raise ValueError("page budget must be between 1 and 1048576")
        wall = time.perf_counter()
        with _child_scope(self._engine.discover_pairs(left, right, start=start,
                max_pairs=budget)) as result:
            native_page_ms = _elapsed(wall)
            begin = time.perf_counter()
            info, words = _page_copy(result.download(), self._left, self._right,
                start, budget, 0 if self._engine.backend == "cpu" else 1, self._engine.profile)
            download_ms = _elapsed(begin)
            cleanup_start = time.perf_counter()
        child_cleanup_ms = _elapsed(cleanup_start)
        timings = (("native_page_ms", native_page_ms), ("download_ms", download_ms),
                   ("child_cleanup_ms", child_cleanup_ms), ("reused_wall_ms", _elapsed(wall)))
        return ScenePairPage(info, words, self._left, self._right, timings)
