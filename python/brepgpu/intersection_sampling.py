"""Two-sided consumption of exact CPU intersections using unchanged position ABI.

Request construction, exact maps, provenance checks, world scatter and model
checks are host work. CUDA only evaluates the supplied source-surface rows.
"""
from array import array
from contextlib import contextmanager
from dataclasses import dataclass, field
from fractions import Fraction
import math
import operator
import time

from . import BG_INVALID_HANDLE, BG_QUERY_OK, BgError, Engine, QueryError, _descriptor
from .intersections import IntersectionSet, Scalar

MAX_RESOLUTION = 4097
MAX_CANDIDATES = 1 << 19
MAX_SUBDIVISIONS = 4096
WORLD_TOLERANCE = Fraction(1, 1 << 36)
_PHASES = ("gather_ms", "query_ms", "evaluate_ms", "download_ms", "convert_ms",
           "world_scatter_ms", "grade_ms", "child_cleanup_ms", "reused_wall_ms")


class SamplingPrecisionError(ArithmeticError):
    """The bounded request preparation cannot resolve an inward interval."""


class SamplingArithmeticError(ArithmeticError):
    """An actual source position failed the exact world-model grade."""


def _intersection(value):
    if type(value) is not IntersectionSet:
        raise TypeError("expected an owned IntersectionSet")
    return value


def _scalar(value):
    # Scalar/Fraction records are recursively immutable; coercion copies scalar
    # coefficients rather than retaining a caller-defined numeric object.
    value = Scalar(value)
    return Scalar(value.a, value.b, value.d)


def _exact_vector(values, width):
    values = tuple(_scalar(value) for value in values)
    if len(values) != width:
        raise ValueError("exact vector has the wrong component count")
    return values


def _elapsed(start):
    return 1000. * (time.perf_counter() - start)


def _attach_cleanup(primary, cleanup):
    try:
        primary.brepgpu_cleanup_error = cleanup
        if hasattr(primary, "add_note"):
            primary.add_note("Intersection sampling cleanup also failed: " + str(cleanup))
    except BaseException:
        # Metadata, formatting and add_note failures remain secondary.
        pass


@contextmanager
def _child_scope(resource):
    try:
        yield resource
    except BaseException as primary:
        try:
            resource.close()
        except BaseException as cleanup:
            _attach_cleanup(primary, cleanup)
        raise
    else:
        # Without a primary failure, close failure prevents batch publication.
        # Nested scopes still attempt the remaining closes.
        resource.close()


def _inward_window(lo, hi):
    """Strict rational a<b inside lo<hi; at most 4096 midpoint subdivisions.

    Brackets only supply an outer rational interval. All decisions thereafter
    compare exact rationals to exact algebraic endpoints, including intervals
    narrower than binary64 spacing.
    """
    if not lo < hi:
        raise ValueError("an inward window needs a nonempty exact interval")
    lower, inward_lower = lo.bracket()
    inward_upper, upper = hi.bracket()
    a, b = Fraction.from_float(inward_lower), Fraction.from_float(inward_upper)
    if lo < a < b < hi:
        return a, b
    left, right = Fraction.from_float(lower), Fraction.from_float(upper)
    if not left < right:
        raise SamplingPrecisionError("no finite outer interval for this piece")
    subdivisions = 0

    def midpoint(a, b):
        nonlocal subdivisions
        if subdivisions >= MAX_SUBDIVISIONS:
            raise SamplingPrecisionError("inward interval exceeds the subdivision cap")
        subdivisions += 1
        return (a + b) / 2

    while True:
        middle = midpoint(left, right)
        if middle <= lo:
            left = middle
        elif middle >= hi:
            right = middle
        else:
            break
    a = midpoint(left, middle)
    while a <= lo:
        left = a
        a = midpoint(left, middle)
    b = midpoint(middle, right)
    while b >= hi:
        right = b
        b = midpoint(middle, right)
    return a, b


@dataclass(frozen=True, slots=True)
class IntersectionCandidate:
    piece_id: int
    component_id: int
    span_id: int
    parameter: Scalar
    location: str
    left_uv: tuple
    right_uv: tuple
    reference_position: tuple
    _native_left_uv: tuple = field(init=False, repr=False)
    _native_right_uv: tuple = field(init=False, repr=False)

    def __post_init__(self):
        for name in ("piece_id", "component_id", "span_id"):
            object.__setattr__(self, name, operator.index(getattr(self, name)))
        if self.location not in ("lo", "hi", "point", "interior"):
            raise ValueError("unknown intersection candidate location")
        object.__setattr__(self, "parameter", _scalar(self.parameter))
        object.__setattr__(self, "left_uv", _exact_vector(self.left_uv, 2))
        object.__setattr__(self, "right_uv", _exact_vector(self.right_uv, 2))
        object.__setattr__(self, "reference_position", _exact_vector(self.reference_position, 3))
        if any(not 0 <= value <= 1 for value in (*self.left_uv, *self.right_uv)):
            raise SamplingArithmeticError("exact source UV is outside its normalized chart")
        # Exact-to-binary conversion belongs to request preparation. Reused
        # gather only packs these immutable normalized binary64 coordinates.
        for source, target in (("left_uv", "_native_left_uv"),
                               ("right_uv", "_native_right_uv")):
            rounded = tuple(float(value) for value in getattr(self, source))
            if any(not math.isfinite(value) or not 0 <= value <= 1 for value in rounded):
                raise SamplingArithmeticError("rounded source UV is outside its normalized chart")
            object.__setattr__(self, target, rounded)


@dataclass(frozen=True, slots=True, init=False)
class IntersectionSampleRequest:
    result: IntersectionSet
    candidates: tuple
    resolution: int

    def __init__(self, *args, **kwargs):
        raise TypeError("IntersectionSampleRequest construction is factory-only; use prepare_samples")


def prepare_samples(result, resolution):
    """Prepare endpoints and rational interiors in immutable piece order.

    Resolution counts every occurrence separately. Chart endpoints can coincide
    geometrically, and grouped generator spans retain both source occurrences.
    This is a bounded sampler; it does not change complete construction.
    """
    result = _intersection(result)
    if isinstance(resolution, bool):
        raise TypeError("resolution must be an integer")
    resolution = operator.index(resolution)
    if not 2 <= resolution <= MAX_RESOLUTION:
        raise ValueError("resolution must be between 2 and 4097")
    count = 0
    for piece in result.pieces:
        count += 1 if piece.kind == "point" else resolution
        if count > MAX_CANDIDATES:
            raise ValueError("intersection request exceeds the candidate cap")
    candidates = []
    seen = set()
    for piece in result.pieces:
        if piece.id in seen:
            raise ValueError("intersection contains duplicate piece IDs")
        seen.add(piece.id)
        if piece.kind == "point":
            if piece.lo != piece.hi:
                raise ValueError("point occurrence has a nonzero parameter interval")
            parameters = ((piece.lo, "point"),)
        else:
            if not piece.lo < piece.hi:
                raise ValueError("curve occurrence needs a positive parameter interval")
            interiors = ()
            if resolution > 2:
                if not piece.lo.b and not piece.hi.b:
                    # Exact rational endpoints need no search or mixed-field
                    # interpolation. Their equal-spaced interiors are rational.
                    a, b = piece.lo.a, piece.hi.a
                else:
                    a, b = _inward_window(piece.lo, piece.hi)
                # For irrational endpoints these are rational witnesses in an
                # inward interval, not algebraic equal-spacing claims.
                interiors = tuple((Scalar(a + (b-a)*Fraction(i, resolution-1)), "interior")
                                  for i in range(1, resolution-1))
            parameters = ((piece.lo, "lo"), *interiors, (piece.hi, "hi"))
        for parameter, location in parameters:
            candidates.append(IntersectionCandidate(
                piece.id, piece.component_id, piece.span_id, parameter, location,
                piece.left_uv(parameter), piece.right_uv(parameter), piece.position(parameter)))
    request = object.__new__(IntersectionSampleRequest)
    object.__setattr__(request, "result", result)
    object.__setattr__(request, "candidates", tuple(candidates))
    object.__setattr__(request, "resolution", resolution)
    return request


def _float_vector(values, width):
    values = tuple(float(value) for value in values)
    if len(values) != width or not all(math.isfinite(value) for value in values):
        raise SamplingArithmeticError("actual sample vector must be finite with the required width")
    return values


@dataclass(frozen=True, slots=True)
class IntersectionSampleRow:
    index: int
    piece_id: int
    component_id: int
    span_id: int
    parameter: Scalar
    location: str
    reference_position: tuple
    left_slot: int
    right_slot: int
    left_uv: tuple
    right_uv: tuple
    left_position: tuple
    right_position: tuple
    left_status: int
    right_status: int

    def __post_init__(self):
        object.__setattr__(self, "parameter", _scalar(self.parameter))
        object.__setattr__(self, "reference_position", _exact_vector(self.reference_position, 3))
        for name, width in (("left_uv", 2), ("right_uv", 2),
                            ("left_position", 3), ("right_position", 3)):
            object.__setattr__(self, name, _float_vector(getattr(self, name), width))


@dataclass(frozen=True, slots=True)
class IntersectionSampleBatch:
    result: IntersectionSet
    rows: tuple
    timings: tuple
    source_descriptors: bytes
    native_slots: bytes
    native_uv: bytes
    dense_sources: tuple

    def __post_init__(self):
        _intersection(self.result)
        rows = tuple(self.rows)
        if any(type(row) is not IntersectionSampleRow for row in rows):
            raise TypeError("sample rows must be IntersectionSampleRow values")
        timings = tuple((str(name), float(value)) for name, value in self.timings)
        if tuple(name for name, _ in timings) != _PHASES:
            raise ValueError("sample timing phases differ from the consumer contract")
        if any(not math.isfinite(value) or value < 0 for _, value in timings):
            raise ValueError("sample timings must be finite nonnegative milliseconds")
        object.__setattr__(self, "rows", rows)
        object.__setattr__(self, "timings", timings)
        for name in ("source_descriptors", "native_slots", "native_uv"):
            object.__setattr__(self, name, bytes(getattr(self, name)))
        object.__setattr__(self, "dense_sources",
                           tuple((operator.index(side), operator.index(slot))
                                 for side, slot in self.dense_sources))

    @property
    def count(self):
        return len(self.rows)

    @property
    def evaluated_count(self):
        return 2 * len(self.rows)


def _check_statuses(jets, request, stage):
    expected = 2 * len(request.candidates)
    if jets.count != expected or jets.order != 0 or len(jets.data) != 3*expected \
            or len(jets.statuses) != expected:
        raise RuntimeError("intersection position output shape differs from its request")
    failures = tuple((i, int(status)) for i, status in enumerate(jets.statuses)
                     if status != BG_QUERY_OK)
    if failures:
        error = QueryError(failures)
        error.intersection_rows = tuple((i//2, "left" if i%2 == 0 else "right", status)
                                        for i, status in failures)
        raise error


def _within_model(actual, reference):
    value = Fraction.from_float(actual)
    if not reference.b:
        return abs(value-reference.a) <= WORLD_TOLERANCE
    return abs(Scalar(value)-reference) <= WORLD_TOLERANCE


def _grade(rows):
    for row in rows:
        for axis, reference in enumerate(row.reference_position):
            left, right = row.left_position[axis], row.right_position[axis]
            if not _within_model(left, reference) or not _within_model(right, reference) \
                    or abs(Fraction.from_float(left)-Fraction.from_float(right)) > WORLD_TOLERANCE:
                error = SamplingArithmeticError("world-model grade failed at candidate "
                                                + str(row.index) + ", axis " + str(axis))
                error.candidate_index, error.axis = row.index, axis
                raise error


class IntersectionSampler:
    """Borrow an Engine and cache both selected faces' source charts."""
    def __init__(self, engine, result):
        if not isinstance(engine, Engine):
            raise TypeError("intersection sampler requires an Engine")
        self._engine = engine
        self._result = _intersection(result)
        self._patches = None
        if not engine.handle:
            raise BgError(BG_INVALID_HANDLE, message="intersection sampler engine is closed")
        sources = tuple((side, slot)
                        for side, ref in enumerate((result.left, result.right))
                        for slot in ref.face.chart_slots)
        if len(set(sources)) != len(sources):
            raise ValueError("selected face has duplicate chart slots")
        dense = {key: index for index, key in enumerate(sources)}
        pieces = {piece.id: piece for piece in result.pieces}
        if len(pieces) != len(result.pieces):
            raise ValueError("intersection contains duplicate piece IDs")
        for piece in result.pieces:
            if (0, piece.left_slot) not in dense or (1, piece.right_slot) not in dense:
                raise ValueError("intersection occurrence references a chart outside the selected face")
        patches = tuple((result.left if side == 0 else result.right).snapshot.patches[slot]
                        for side, slot in sources)
        expected = []
        offset = 0
        for patch in patches:
            expected.append(bytes(_descriptor(patch, offset)))
            offset += len(patch.controls)//4
        # Prepare all fallible owned metadata before publishing native ownership.
        self._dense_sources = sources
        self._dense = dense
        self._pieces = pieces
        self._source_bytes = b"".join(expected)
        adopted = engine.create_patch_set(patches)
        try:
            self._patches = adopted
        except BaseException as primary:
            try:
                adopted.close()
            except BaseException as cleanup:
                _attach_cleanup(primary, cleanup)
            raise

    @property
    def result(self):
        return self._result

    def _ready(self):
        patches = self._patches
        if patches is None or not patches.handle:
            raise BgError(BG_INVALID_HANDLE, message="intersection sampler is closed")
        if not self._engine.handle:
            raise BgError(BG_INVALID_HANDLE, message="intersection sampler engine is closed")
        return patches

    def close(self):
        patches = self._patches
        if patches is not None:
            try:
                patches.close()
            finally:
                if not patches.handle:
                    self._patches = None

    def __enter__(self):
        self._ready()
        return self

    def __exit__(self, kind, primary, traceback):
        try:
            self.close()
        except BaseException as cleanup:
            if primary is None:
                raise
            _attach_cleanup(primary, cleanup)
        return False

    def _gather(self, request):
        slots, uv = array("I"), array("d")
        for candidate in request.candidates:
            piece = self._pieces[candidate.piece_id]
            for side, original, rounded in ((0, piece.left_slot, candidate._native_left_uv),
                                             (1, piece.right_slot, candidate._native_right_uv)):
                slots.append(self._dense[(side, original)])
                uv.extend(rounded)
        return slots, uv

    def _provenance(self, local, slots, uv):
        if local.info.patch_count != len(self._dense_sources) \
                or bytes(local._descriptors) != self._source_bytes:
            raise RuntimeError("native intersection source descriptors differ from cached inputs")
        if local._slots.tobytes() != slots.tobytes() or local._uv.tobytes() != uv.tobytes():
            raise RuntimeError("native intersection slot/UV provenance differs from gathered inputs")

    def _rows(self, request, converted, uv):
        rows = []
        for index, candidate in enumerate(request.candidates):
            piece = self._pieces[candidate.piece_id]
            left, right = 2*index, 2*index+1
            rows.append(IntersectionSampleRow(
                index, candidate.piece_id, candidate.component_id, candidate.span_id,
                candidate.parameter, candidate.location, candidate.reference_position,
                piece.left_slot, piece.right_slot,
                tuple(uv[2*left:2*left+2]), tuple(uv[2*right:2*right+2]),
                converted.vector("S", left), converted.vector("S", right),
                int(converted.statuses[left]), int(converted.statuses[right])))
        return tuple(rows)

    def sample(self, request):
        patches = self._ready()
        if type(request) is not IntersectionSampleRequest:
            raise TypeError("intersection sampling requires an IntersectionSampleRequest")
        if request.result is not self._result:
            raise ValueError("prepared request belongs to a different exact intersection result")
        wall_start = time.perf_counter()
        start = wall_start
        slots, uv = self._gather(request)
        gather_ms = _elapsed(start)
        if not request.candidates:
            timings = tuple(zip(_PHASES, (gather_ms, 0., 0., 0., 0., 0., 0., 0.,
                                          _elapsed(wall_start))))
            return IntersectionSampleBatch(self._result, (), timings, b"", b"", b"", ())
        start = time.perf_counter()
        with _child_scope(self._engine.create_query_set(patches, slots, uv)) as queries:
            query_ms = _elapsed(start)
            start = time.perf_counter()
            with _child_scope(self._engine.evaluate(queries, order=0)) as result:
                evaluate_ms = _elapsed(start)
                start = time.perf_counter()
                local = result.download()
                _check_statuses(local, request, "native evaluation")
                self._provenance(local, slots, uv)
                # These immutable artifacts copy actual returned source buffers.
                # Expected metadata is never passed off as native provenance.
                source_bytes = bytes(local._descriptors)
                native_slots, native_uv = local._slots.tobytes(), local._uv.tobytes()
                download_ms = _elapsed(start)
                start = time.perf_counter()
                converted = local.convert()
                _check_statuses(converted, request, "world conversion")
                convert_ms = _elapsed(start)
                start = time.perf_counter()
                rows = self._rows(request, converted, uv)
                world_scatter_ms = _elapsed(start)
                start = time.perf_counter()
                _grade(rows)
                grade_ms = _elapsed(start)
                cleanup_start = time.perf_counter()
        child_cleanup_ms = _elapsed(cleanup_start)
        # Caller wall separately includes immutable batch publication below.
        timings = tuple(zip(_PHASES, (gather_ms, query_ms, evaluate_ms, download_ms,
                                     convert_ms, world_scatter_ms, grade_ms,
                                     child_cleanup_ms, _elapsed(wall_start))))
        return IntersectionSampleBatch(self._result, rows, timings, source_bytes,
                                       native_slots, native_uv, self._dense_sources)
