"""Native first-jet/oriented-row consumption of an owned OffsetResult.

The existing Boolean provenance/row helpers operate on the same typed patch,
chart and face fields. Public factory/request guards remain operation-specific.
"""
from array import array
from dataclasses import dataclass
import operator
import time

from . import BG_INVALID_HANDLE, BgError, Engine
from .body import REJECTED, INTERIOR, BOUNDARY, INVALID
from .body_sampling import Candidate, SampleRow, _child_scope
from .boolean_sampling import _check, _rows
from .offset import OffsetResult

MAX_RESOLUTION = 257
MAX_CANDIDATES = 1_048_576
_PHASES = ('gather_ms', 'patch_ms', 'query_ms', 'evaluate_ms', 'download_ms',
           'convert_ms', 'normal_scatter_ms', 'child_cleanup_ms', 'sample_wall_ms')


def _result(value):
    if type(value) is not OffsetResult:
        raise TypeError('expected a factory OffsetResult')
    return value


@dataclass(frozen=True, init=False)
class OffsetRequest:
    result: OffsetResult
    candidates: tuple
    labels: tuple
    admitted_indices: tuple

    def __init__(self, result, candidates):
        result = _result(result)
        copied = []
        for index, candidate in enumerate(candidates):
            if index >= MAX_CANDIDATES:
                raise ValueError('offset request exceeds candidate budget')
            if not isinstance(candidate, Candidate):
                raise TypeError('offset candidates require Candidate values')
            actual = Candidate(candidate.patch_slot, candidate.u, candidate.v)
            if actual.patch_slot >= len(result.solid.charts):
                raise ValueError('candidate patch slot outside OffsetResult')
            copied.append(actual)
        copied = tuple(copied)
        labels = tuple(result.solid.classify(c.patch_slot, c.u, c.v) for c in copied)
        if any(label not in (REJECTED, INTERIOR, BOUNDARY, INVALID) for label in labels):
            raise RuntimeError('offset trim classifier returned an unknown label')
        admitted = tuple(i for i, label in enumerate(labels)
                         if label in (INTERIOR, BOUNDARY))
        for name, value in (('result', result), ('candidates', copied),
                            ('labels', labels), ('admitted_indices', admitted)):
            object.__setattr__(self, name, value)


@dataclass(frozen=True, init=False)
class OffsetSamples:
    result: OffsetResult
    request: OffsetRequest
    rows: tuple
    timings: tuple
    backend: str
    device: int

    def __init__(self, *args, **kwargs):
        raise TypeError('OffsetSamples is factory-only; use sample')

    @property
    def count(self):
        return len(self.rows)

    @property
    def evaluated_count(self):
        return len(self.request.admitted_indices)


def prepare_grid(result, resolution=17):
    """Copy all carrier squares; exact cap disks and wall labels select rows."""
    result = _result(result)
    if isinstance(resolution, bool):
        raise TypeError('resolution excludes bool')
    resolution = operator.index(resolution)
    if not 2 <= resolution <= MAX_RESOLUTION:
        raise ValueError('resolution must be integer in 2..257')
    if len(result.solid.charts) * resolution * resolution > MAX_CANDIDATES:
        raise ValueError('offset grid exceeds complete candidate budget')
    denominator = resolution - 1
    return OffsetRequest(result, (Candidate(slot, i / denominator, j / denominator)
        for slot in range(len(result.solid.charts))
        for i in range(resolution) for j in range(resolution)))


def sample(engine, request) -> OffsetSamples:
    """Borrow Engine; own patch/query/result children for the complete call."""
    if not isinstance(engine, Engine):
        raise TypeError('offset sampling borrows an Engine')
    if type(request) is not OffsetRequest:
        raise TypeError('offset sampling requires an OffsetRequest')
    if not engine.handle:
        raise BgError(BG_INVALID_HANDLE, message='offset sampler engine is closed')
    started = previous = time.perf_counter_ns()
    phases = []

    def phase():
        nonlocal previous
        now = time.perf_counter_ns()
        phases.append((now-previous)/1e6)
        previous = now

    slots = array('I', (request.candidates[i].patch_slot for i in request.admitted_indices))
    uv = array('d', (value for i in request.admitted_indices
                    for value in (request.candidates[i].u, request.candidates[i].v)))
    phase()
    with _child_scope(engine.create_patch_set(request.result.solid.patches)) as patches:
        phase()
        with _child_scope(engine.create_query_set(patches, slots, uv)) as queries:
            phase()
            with _child_scope(engine.evaluate(queries, order=1)) as evaluated:
                phase()
                local = evaluated.download()
                observed_backend = int(engine.backend == 'cuda')
                _check(local, request, slots, uv, observed_backend, converted=False)
                phase()
                world = local.convert()
                _check(world, request, slots, uv, observed_backend, converted=True)
                phase()
                rows = _rows(request, world)
                phase()
    phase()
    phases.append((time.perf_counter_ns()-started)/1e6)
    batch = object.__new__(OffsetSamples)
    for name, value in (('result', request.result), ('request', request), ('rows', rows),
                        ('timings', tuple(zip(_PHASES, phases))),
                        ('backend', 'cuda' if observed_backend else 'cpu'),
                        ('device', engine.device)):
        object.__setattr__(batch, name, value)
    return batch
