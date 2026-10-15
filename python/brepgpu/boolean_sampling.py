"""Owned native first-jet and oriented-normal consumer for a CutResult.

Construction and exact trim admission are CPU work. The borrowed Engine evaluates
unchanged source carriers, each with its original coordinate/parameter frame.
"""
from array import array
import ctypes as C
from dataclasses import dataclass
import math
import operator
import struct
import time

from . import (BG_ABI_VERSION, BG_INVALID_HANDLE, BG_QUERY_OK, BgError, Engine,
               PatchDesc, QueryError)
from .body import REJECTED, INTERIOR, BOUNDARY, INVALID
from .body_sampling import Candidate, SampleRow, _child_scope
from .boolean import CutResult

MAX_RESOLUTION = 257
MAX_CANDIDATES = 10 * MAX_RESOLUTION * MAX_RESOLUTION
_NAN = (math.nan, math.nan, math.nan)
_PHASES = ('gather_ms', 'patch_ms', 'query_ms', 'evaluate_ms', 'download_ms',
           'convert_ms', 'normal_scatter_ms', 'child_cleanup_ms', 'sample_wall_ms')


def _result(value):
    if type(value) is not CutResult:
        raise TypeError('expected a factory CutResult')
    return value


@dataclass(frozen=True, init=False)
class CutRequest:
    result: CutResult
    candidates: tuple
    labels: tuple
    admitted_indices: tuple

    def __init__(self, result, candidates):
        result = _result(result)
        copied = []
        for index, candidate in enumerate(candidates):
            if index >= MAX_CANDIDATES:
                raise ValueError('cut request exceeds candidate budget')
            if not isinstance(candidate, Candidate):
                raise TypeError('cut candidates require Candidate values')
            actual = Candidate(candidate.patch_slot, candidate.u, candidate.v)
            if actual.patch_slot >= len(result.solid.charts):
                raise ValueError('candidate patch slot outside CutResult')
            copied.append(actual)
        copied = tuple(copied)
        labels = tuple(result.solid.classify(c.patch_slot, c.u, c.v) for c in copied)
        if any(label not in (REJECTED, INTERIOR, BOUNDARY, INVALID) for label in labels):
            raise RuntimeError('cut trim classifier returned an unknown label')
        admitted = tuple(i for i, label in enumerate(labels)
                         if label in (INTERIOR, BOUNDARY))
        for name, value in (('result', result), ('candidates', copied),
                            ('labels', labels), ('admitted_indices', admitted)):
            object.__setattr__(self, name, value)


@dataclass(frozen=True, init=False)
class CutSamples:
    result: CutResult
    request: CutRequest
    rows: tuple
    timings: tuple
    backend: str
    device: int

    def __init__(self, *args, **kwargs):
        raise TypeError('CutSamples is factory-only; use sample')

    @property
    def count(self):
        return len(self.rows)

    @property
    def evaluated_count(self):
        return len(self.request.admitted_indices)


def prepare_grid(result, resolution=17):
    """Prepare actual binary64 UV; wall uses the retained original V band."""
    result = _result(result)
    if isinstance(resolution, bool):
        raise TypeError('resolution excludes bool')
    resolution = operator.index(resolution)
    if not 2 <= resolution <= MAX_RESOLUTION:
        raise ValueError('resolution must be integer in 2..257')
    solid = result.solid
    if len(solid.charts) * resolution * resolution > MAX_CANDIDATES:
        raise ValueError('cut grid exceeds candidate budget')
    denominator = resolution - 1

    def candidates():
        for slot, chart in enumerate(solid.charts):
            wall = solid.faces[chart.face_slot].kind == 'cylinder'
            for i in range(resolution):
                for j in range(resolution):
                    v = j / denominator
                    if wall:
                        v = .25 + .5 * v
                    yield Candidate(slot, i / denominator, v)
    return CutRequest(result, candidates())


def _bits(value):
    return struct.pack('<d', value)


def _check(jets, request, slots, uv, backend, *, converted):
    """Check actual downloaded ownership, complete metadata and every status."""
    count = len(request.admitted_indices)
    patches = request.result.solid.patches
    info = jets.info
    if (jets.count != count or jets.order != 1 or info.count != count or
            info.order != 1 or info.terms != 3 or
            info.abi_version != BG_ABI_VERSION or info.struct_size != C.sizeof(info) or
            info.patch_count != len(patches) or info.backend != backend or
            not math.isfinite(info.kernel_ms) or info.kernel_ms < 0 or
            jets._converted is not converted or len(jets.data) != 9 * count or
            len(jets.statuses) != count or len(jets._descriptors) != len(patches) or
            len(jets._slots) != count or len(jets._uv) != 2 * count):
        raise RuntimeError('native cut first-jet metadata differs from request')
    if jets._slots.tobytes() != slots.tobytes() or jets._uv.tobytes() != uv.tobytes():
        raise RuntimeError('native cut query slots/UV bits differ from request')
    offset = 0
    for patch, descriptor in zip(patches, jets._descriptors):
        lineage = patch.lineage
        expected = dict(abi_version=BG_ABI_VERSION, struct_size=C.sizeof(PatchDesc),
            degree_u=patch.degree_u, degree_v=patch.degree_v, control_offset=offset,
            control_count=len(patch.controls)//4, flags=3, body_id=lineage.body_id,
            entity_id=lineage.entity_id, entity_kind=lineage.entity_kind,
            revision=lineage.revision, chart_id=lineage.chart_id)
        if any(getattr(descriptor, name) != value for name, value in expected.items()):
            raise RuntimeError('native cut descriptor identity/layout differs from submitted patch')
        if (any(_bits(a) != _bits(b) for a, b in zip(descriptor.uv, patch.uv)) or
                any(_bits(a) != _bits(b) for a, b in zip(descriptor.origin, patch.origin)) or
                _bits(descriptor.scale) != _bits(patch.scale)):
            raise RuntimeError('native cut descriptor source frame/domain differs from submitted patch')
        offset += len(patch.controls)//4
    if jets.lineage != tuple(patches[slot].lineage for slot in slots):
        raise RuntimeError('native cut per-query Lineage differs from request')
    failures = tuple((request.admitted_indices[index], int(status))
                     for index, status in enumerate(jets.statuses) if status != BG_QUERY_OK)
    if failures:
        raise QueryError(failures)


def _rows(request, converted):
    solid = request.result.solid
    rows = [None] * len(request.candidates)
    for index, candidate in enumerate(request.candidates):
        if request.labels[index] in (INTERIOR, BOUNDARY):
            continue
        chart = solid.charts[candidate.patch_slot]
        face = solid.faces[chart.face_slot]
        rows[index] = SampleRow(index, candidate.patch_slot, face.id, chart.id,
            candidate.u, candidate.v, request.labels[index], -1, _NAN, _NAN)
    if converted.count != len(request.admitted_indices):
        raise RuntimeError('cut scatter count differs from admitted rows')
    for native_index, index in enumerate(request.admitted_indices):
        candidate = request.candidates[index]
        chart = solid.charts[candidate.patch_slot]
        face = solid.faces[chart.face_slot]
        if rows[index] is not None or request.labels[index] not in (INTERIOR, BOUNDARY):
            raise RuntimeError('cut scatter index names a nonadmitted row')
        position = converted.vector(0, native_index)
        du = converted.vector(1, native_index)
        dv = converted.vector(2, native_index)
        if not all(math.isfinite(x) for x in (*position, *du, *dv)):
            raise ArithmeticError('nonfinite cut source-world first jet')
        normal = (du[1]*dv[2]-du[2]*dv[1], du[2]*dv[0]-du[0]*dv[2],
                  du[0]*dv[1]-du[1]*dv[0])
        length = math.hypot(*normal)
        if not math.isfinite(length) or length == 0:
            raise ArithmeticError('cut native first jet has unusable normal')
        normal = tuple(face.orientation*x/length for x in normal)
        if not all(math.isfinite(x) for x in normal):
            raise ArithmeticError('nonfinite cut oriented normal')
        rows[index] = SampleRow(index, candidate.patch_slot, face.id, chart.id,
            candidate.u, candidate.v, request.labels[index],
            int(converted.statuses[native_index]), position, normal)
    if any(row is None for row in rows):
        raise RuntimeError('cut scatter omitted a requested row')
    return tuple(rows)


def sample(engine, request) -> CutSamples:
    """Borrow Engine and own one complete patch/query/result lifetime per call."""
    if not isinstance(engine, Engine):
        raise TypeError('cut sampling borrows an Engine')
    if type(request) is not CutRequest:
        raise TypeError('cut sampling requires a CutRequest')
    if not engine.handle:
        raise BgError(BG_INVALID_HANDLE, message='cut sampler engine is closed')
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
    batch = object.__new__(CutSamples)
    for name, value in (('result', request.result), ('request', request), ('rows', rows),
                        ('timings', tuple(zip(_PHASES, phases))),
                        ('backend', 'cuda' if observed_backend else 'cpu'),
                        ('device', engine.device)):
        object.__setattr__(batch, name, value)
    return batch
