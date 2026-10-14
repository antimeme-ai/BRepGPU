"""CPU body requests/normal consumption around one owned native first-jet batch.

Body classification, gathering, frame conversion, normals and scatter are host
work. The borrowed Engine evaluates the admitted untrimmed carrier parameters.
"""
from array import array
from dataclasses import dataclass
import math
import operator
import time

from . import BG_INVALID_HANDLE, BG_QUERY_OK, BgError, Engine, QueryError
from .body import BodySnapshot, REJECTED, INTERIOR, BOUNDARY, INVALID

MAX_RESOLUTION = 257
MAX_CANDIDATES = 10 * MAX_RESOLUTION * MAX_RESOLUTION
_NAN_VECTOR = (math.nan, math.nan, math.nan)
_PHASES = ("gather_ms", "query_ms", "evaluate_ms", "download_ms", "convert_ms",
           "normal_scatter_ms", "child_cleanup_ms", "reused_wall_ms")


def _snapshot(value):
    if not isinstance(value, BodySnapshot):
        raise TypeError("expected an owned BodySnapshot")
    return value


def _elapsed(start):
    return 1000. * (time.perf_counter() - start)


@dataclass(frozen=True)
class Candidate:
    patch_slot: int
    u: float
    v: float

    def __post_init__(self):
        slot = operator.index(self.patch_slot)
        if not 0 <= slot < (1 << 32):
            raise ValueError("candidate patch slot does not fit uint32")
        object.__setattr__(self, "patch_slot", slot)
        # Nonfinite/out-of-domain coordinates remain explicit INVALID rows.
        object.__setattr__(self, "u", float(self.u))
        object.__setattr__(self, "v", float(self.v))


@dataclass(frozen=True, init=False)
class SampleRequest:
    snapshot: BodySnapshot
    candidates: tuple
    labels: tuple
    admitted_indices: tuple

    def __init__(self, snapshot, candidates):
        snapshot = _snapshot(snapshot)
        copied = []
        for index, candidate in enumerate(candidates):
            if index >= MAX_CANDIDATES:
                raise ValueError("candidate request exceeds the body sampling cap")
            if not isinstance(candidate, Candidate):
                raise TypeError("candidates must contain Candidate values")
            row = Candidate(candidate.patch_slot, candidate.u, candidate.v)
            if row.patch_slot >= len(snapshot.charts):
                raise ValueError("candidate patch slot is outside this snapshot")
            copied.append(row)
        rows = tuple(copied)
        labels = tuple(snapshot.classify(c.patch_slot, c.u, c.v) for c in rows)
        if any(label not in (REJECTED, INTERIOR, BOUNDARY, INVALID) for label in labels):
            raise RuntimeError("body classifier emitted an unknown label")
        admitted = tuple(i for i, label in enumerate(labels)
                         if label in (INTERIOR, BOUNDARY))
        object.__setattr__(self, "snapshot", snapshot)
        object.__setattr__(self, "candidates", rows)
        object.__setattr__(self, "labels", labels)
        object.__setattr__(self, "admitted_indices", admitted)


def prepare_grid(snapshot, resolution, face_ids=None):
    """Create an immutable classified grid in snapshot face/chart order."""
    snapshot = _snapshot(snapshot)
    resolution = operator.index(resolution)
    if not 2 <= resolution <= MAX_RESOLUTION:
        raise ValueError("resolution must be an integer between 2 and 257")
    known = {face.id for face in snapshot.faces}
    selected = known if face_ids is None else set()
    if face_ids is not None:
        for value in face_ids:
            face_id = operator.index(value)
            if face_id not in known:
                raise ValueError("selected face ID is outside this snapshot")
            if face_id in selected:
                raise ValueError("duplicate selected face ID")
            selected.add(face_id)
    charts = tuple(slot for face in snapshot.faces if face.id in selected
                   for slot in face.chart_slots)
    if len(charts) * resolution * resolution > MAX_CANDIDATES:
        raise ValueError("grid exceeds the body sampling cap")
    denominator = resolution - 1
    candidates = (Candidate(slot, u / denominator, v / denominator)
                  for slot in charts for u in range(resolution) for v in range(resolution))
    return SampleRequest(snapshot, candidates)


@dataclass(frozen=True)
class SampleRow:
    index: int
    patch_slot: int
    face_id: int
    chart_id: int
    u: float
    v: float
    trim: int
    native_status: int
    position: tuple
    normal: tuple

    def __post_init__(self):
        for name in ("position", "normal"):
            values = tuple(float(value) for value in getattr(self, name))
            if len(values) != 3:
                raise ValueError(name + " must contain three components")
            object.__setattr__(self, name, values)


@dataclass(frozen=True)
class SampleBatch:
    snapshot: BodySnapshot
    rows: tuple
    timings: tuple

    def __post_init__(self):
        _snapshot(self.snapshot)
        rows = tuple(self.rows)
        if any(not isinstance(row, SampleRow) for row in rows):
            raise TypeError("sample batch rows must be SampleRow values")
        timings = tuple((str(name), float(value)) for name, value in self.timings)
        if tuple(name for name, _ in timings) != _PHASES:
            raise ValueError("sample timing phases differ from the caller contract")
        if any(not math.isfinite(value) or value < 0 for _, value in timings):
            raise ValueError("sample timings must be finite nonnegative milliseconds")
        object.__setattr__(self, "rows", rows)
        object.__setattr__(self, "timings", timings)

    @property
    def count(self):
        return len(self.rows)

    @property
    def evaluated_count(self):
        return sum(row.native_status != -1 for row in self.rows)


def _check_statuses(jets, request, stage):
    indices = request.admitted_indices
    if jets.count != len(indices) or jets.order != 1 or len(jets.data) != 9 * len(indices) \
            or len(jets.statuses) != len(indices):
        raise RuntimeError("body first-jet output shape differs from the admitted request")
    failures = tuple((indices[i], int(status)) for i, status in enumerate(jets.statuses)
                     if status != BG_QUERY_OK)
    if failures:
        details = []
        for index, status in failures:
            candidate = request.candidates[index]
            chart = request.snapshot.charts[candidate.patch_slot]
            face = request.snapshot.faces[chart.face_slot]
            details.append((index, face.id, chart.id, candidate.u, candidate.v, status))
        error = QueryError(failures)
        error.body_rows = tuple(details)
        if hasattr(error, "add_note"):
            error.add_note(stage + " failed body rows (index, face, chart, u, v, status): "
                           + repr(error.body_rows[:4]))
        raise error


def _check_provenance(jets, request, slots, parameters):
    # These owned JetBatch source buffers come from bg1_result_sources. Compare
    # bytes so submitted signed zeros/UV bits are preserved, not only float==.
    if jets._slots.tobytes() != slots.tobytes() or jets._uv.tobytes() != parameters.tobytes():
        raise RuntimeError("native body query provenance differs from gathered inputs")
    expected = tuple(request.snapshot.patches[slot].lineage for slot in slots)
    if jets.lineage != expected:
        raise RuntimeError("native body face/chart lineage differs from the snapshot")


def _rows(request, converted):
    snapshot = request.snapshot
    answer = [None] * len(request.candidates)
    for index, candidate in enumerate(request.candidates):
        chart = snapshot.charts[candidate.patch_slot]
        face = snapshot.faces[chart.face_slot]
        label = request.labels[index]
        if label not in (INTERIOR, BOUNDARY):
            answer[index] = SampleRow(index, candidate.patch_slot, face.id, chart.id,
                                      candidate.u, candidate.v, label, -1, _NAN_VECTOR, _NAN_VECTOR)
    if len(request.admitted_indices) != converted.count:
        raise RuntimeError("body scatter consumed a different admitted row count")
    for native_index, index in enumerate(request.admitted_indices):
        original = request.candidates[index]
        label = request.labels[index]
        if label not in (INTERIOR, BOUNDARY) or answer[index] is not None:
            raise RuntimeError("body gather map points to a nonadmitted candidate")
        chart = snapshot.charts[original.patch_slot]
        face = snapshot.faces[chart.face_slot]
        position = converted.vector("S", native_index)
        su = converted.vector("Ss", native_index)
        sv = converted.vector("St", native_index)
        if not all(math.isfinite(value) for value in (*position, *su, *sv)):
            raise ArithmeticError("nonfinite body first jet at candidate " + str(index))
        cross = (su[1] * sv[2] - su[2] * sv[1],
                 su[2] * sv[0] - su[0] * sv[2],
                 su[0] * sv[1] - su[1] * sv[0])
        length = math.hypot(*cross)
        if not math.isfinite(length) or length == 0:
            raise ArithmeticError("unusable body normal at candidate " + str(index))
        normal = tuple(face.orientation * value / length for value in cross)
        if not all(math.isfinite(value) for value in normal):
            raise ArithmeticError("nonfinite oriented normal at candidate " + str(index))
        answer[index] = SampleRow(index, original.patch_slot, face.id, chart.id,
                                  original.u, original.v, label,
                                  int(converted.statuses[native_index]), position, normal)
    if any(row is None for row in answer):
        raise RuntimeError("body scatter omitted an admitted candidate")
    return tuple(answer)


class BodySampler:
    """Borrow an Engine and own one PatchSet for an immutable body snapshot."""
    def __init__(self, engine, snapshot):
        if not isinstance(engine, Engine):
            raise TypeError("body sampler requires an Engine")
        self._snapshot = _snapshot(snapshot)
        self._engine = engine
        self._patches = None
        if not engine.handle:
            raise BgError(BG_INVALID_HANDLE, message="body sampler engine is closed")
        # The creator protects its wrapper construction; guard our subsequent
        # ownership assignment as well, preserving the primary failure.
        patches = engine.create_patch_set(snapshot.patches)
        try:
            self._patches = patches
        except BaseException as error:
            try:
                patches.close()
            except BaseException as cleanup:
                try:
                    error.brepgpu_cleanup_error = cleanup
                    if hasattr(error, "add_note"):
                        error.add_note("Body sampler construction cleanup also failed: " + str(cleanup))
                except BaseException:
                    pass
            raise

    @property
    def snapshot(self):
        return self._snapshot

    def _ready(self):
        patches = self._patches
        if patches is None or not patches.handle:
            raise BgError(BG_INVALID_HANDLE, message="body sampler is closed")
        if not self._engine.handle:
            raise BgError(BG_INVALID_HANDLE, message="body sampler engine is closed")
        return patches

    def close(self):
        patches = self._patches
        if patches is not None:
            try:
                patches.close()
            finally:
                # An allocation failure can leave the handle owned/retryable;
                # a consumed handle must remain closed even if cleanup raises.
                if not patches.handle:
                    self._patches = None

    def __enter__(self):
        self._ready()
        return self

    def __exit__(self, kind, error, traceback):
        try:
            self.close()
        except BaseException as cleanup:
            if error is None:
                raise
            try:
                error.brepgpu_cleanup_error = cleanup
                if hasattr(error, "add_note"):
                    error.add_note("Body sampler cleanup also failed: " + str(cleanup))
            except BaseException:
                # Never replace the primary failure while adding its note.
                pass
        return False

    def sample(self, request):
        patches = self._ready()
        if not isinstance(request, SampleRequest):
            raise TypeError("body sampling requires a SampleRequest")
        if request.snapshot is not self._snapshot:
            raise ValueError("prepared request belongs to a different exact body snapshot")
        wall_start = time.perf_counter()
        start = wall_start
        slots = array("I", (request.candidates[i].patch_slot for i in request.admitted_indices))
        parameters = array("d", (value for i in request.admitted_indices
                                 for value in (request.candidates[i].u, request.candidates[i].v)))
        gather_ms = _elapsed(start)
        start = time.perf_counter()
        with self._engine.create_query_set(patches, slots, parameters) as queries:
            query_ms = _elapsed(start)
            start = time.perf_counter()
            with self._engine.evaluate(queries, order=1) as result:
                evaluate_ms = _elapsed(start)
                start = time.perf_counter()
                local = result.download()
                _check_statuses(local, request, "native evaluation")
                _check_provenance(local, request, slots, parameters)
                download_ms = _elapsed(start)
                start = time.perf_counter()
                converted = local.convert()
                _check_statuses(converted, request, "world/source conversion")
                convert_ms = _elapsed(start)
                start = time.perf_counter()
                rows = _rows(request, converted)
                normal_scatter_ms = _elapsed(start)
                cleanup_start = time.perf_counter()
        child_cleanup_ms = _elapsed(cleanup_start)
        # This internal span ends after child cleanup. External caller wall also
        # includes the timing tuple and immutable SampleBatch publication below.
        reused_wall_ms = _elapsed(wall_start)
        timings = tuple(zip(_PHASES, (gather_ms, query_ms, evaluate_ms, download_ms,
                            convert_ms, normal_scatter_ms, child_cleanup_ms, reused_wall_ms)))
        return SampleBatch(self._snapshot, rows, timings)
