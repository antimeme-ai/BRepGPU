"""Coarse exact sampled-face operations backed by the owned C++20 engine.

Original values remain Fraction-valued in Python during this first migration
slice. No callback crosses into Python during a native geometry operation.
"""
from __future__ import annotations

import ctypes as C
from dataclasses import make_dataclass
from fractions import Fraction
import math
import os
from pathlib import Path
import struct
import time


class DomainError(ValueError):
    """Input or exact source domain is not admitted by the native operation."""


class ResourceError(RuntimeError):
    """The declared operation resource budget was exhausted; no answer exists."""


class _Integer(C.Structure):
    _fields_ = [('offset', C.c_uint64), ('count', C.c_uint32), ('sign', C.c_int32)]


class _Node(C.Structure):
    _fields_ = [('id', C.c_uint64), ('x', _Integer), ('y', _Integer)]


class _Loop(C.Structure):
    _fields_ = [('offset', C.c_uint64), ('count', C.c_uint64)]


class _Input(C.Structure):
    _fields_ = [('abi', C.c_uint32), ('size', C.c_uint32),
                ('reserved', C.c_uint64), ('token', C.c_uint64),
                ('memory_limit', C.c_uint64), ('work_limit', C.c_uint64),
                ('nodes', C.POINTER(_Node)), ('node_count', C.c_uint64),
                ('loops', C.POINTER(_Loop)), ('loop_count', C.c_uint64),
                ('slots', C.POINTER(C.c_uint32)), ('slot_count', C.c_uint64),
                ('limbs', C.POINTER(C.c_uint32)), ('limb_count', C.c_uint64),
                ('x_weight', _Integer), ('y_weight', _Integer)]


class _Error(C.Structure):
    _fields_ = [('abi', C.c_uint32), ('size', C.c_uint32),
                ('status', C.c_int32), ('reserved', C.c_uint32), ('message', C.c_char * 192)]

    def __init__(self):
        super().__init__()
        self.abi = 1
        self.size = C.sizeof(_Error)


class _Crossing(C.Structure):
    _fields_ = [('abi', C.c_uint32), ('size', C.c_uint32),
                ('found', C.c_uint32), ('reserved', C.c_uint32),
                ('segment_i', C.c_uint64), ('segment_j', C.c_uint64), ('ids', C.c_uint64 * 4)]


_STAT_NAMES = ('token memory_peak work node_count segment_count bvh_visits leaf_box_tests crossing_bvh_visits crossing_box_tests '
               'segment_candidates segment_predicates orientations ear_attempts '
               'ear_blocker_queries ear_candidates dead_skips bridge_candidates '
               'triangles max_limbs strict_convex decode_ns crossing_ns bridge_ns ear_ns').split()


class _Stats(C.Structure):
    _fields_ = [('abi', C.c_uint32), ('size', C.c_uint32)] + [(name, C.c_uint64) for name in _STAT_NAMES]


Statistics = make_dataclass('Statistics', [(name, int) for name in _STAT_NAMES], frozen=True)


def _uint(value, label):
    if not isinstance(value, int) or not 0 <= value < 1 << 64:
        raise DomainError(f'{label} must be an unsigned64-bit integer')
    return value


def library_path():
    """Resolve the exact selected binary before loading or provenance hashing."""
    configured = os.environ.get('BREPGPU_FACE_DOMAIN_LIBRARY')
    root = Path(__file__).resolve().parents[2]
    candidates = ([Path(configured)] if configured else
                  [root / 'build' / 'face-domain-release' / 'libbrepgpu_face_domain.so',
                   root / 'build' / 'face-domain-debug' / 'libbrepgpu_face_domain.so',
                   root / 'build' / 'native-face-domain-release' / 'libbrepgpu_face_domain.so',
                   root / 'build' / 'native-face-domain' / 'libbrepgpu_face_domain.so',
                   root / 'build' / 'direct-cdt-release' / 'libbrepgpu_direct_cdt.so',
                   root / 'build' / 'direct-cdt-debug' / 'libbrepgpu_direct_cdt.so'])
    path = next((p for p in candidates if p.is_file()), None)
    if path is None:
        raise RuntimeError('native face-domain library unavailable; set BREPGPU_FACE_DOMAIN_LIBRARY')
    return path.resolve()


def _library():
    lib = C.CDLL(str(library_path()))
    lib.bgfd_abi_size.argtypes = [C.c_uint32]
    lib.bgfd_abi_size.restype = C.c_uint32
    expected = (1, *map(C.sizeof, (_Input, _Error, _Crossing, _Stats, _Node, _Integer, _Loop)))
    if any(lib.bgfd_abi_size(i) != value for i, value in enumerate(expected)):
        raise RuntimeError('native face-domain ABI or record layout differs')
    lib.bgfd_create.argtypes = [C.POINTER(_Input), C.POINTER(C.c_void_p), C.POINTER(_Error)]
    lib.bgfd_first_crossing.argtypes = [C.c_void_p, C.POINTER(_Crossing), C.POINTER(_Error)]
    lib.bgfd_triangulate.argtypes = [C.c_void_p, C.POINTER(_Error)]
    lib.bgfd_copy_triangles.argtypes = [C.c_void_p, C.POINTER(C.c_uint64), C.c_uint64,
                                      C.POINTER(C.c_uint64), C.POINTER(_Error)]
    lib.bgfd_get_stats.argtypes = [C.c_void_p, C.POINTER(_Stats), C.POINTER(_Error)]
    lib.bgfd_destroy.argtypes = [C.c_void_p]
    for name in ('bgfd_create', 'bgfd_first_crossing', 'bgfd_triangulate', 'bgfd_copy_triangles', 'bgfd_get_stats'):
        getattr(lib, name).restype = C.c_int
    lib.bgfd_destroy.restype = None
    return lib


def require_available():
    """Load the actual binary, validate layouts and resolve every required symbol."""
    _library()


def _check(code, error):
    if code:
        text = bytes(error.message).decode('utf-8', errors='replace')
        if code == 6:
            raise ResourceError(text)
        if code == 7:
            raise RuntimeError(text)
        raise DomainError(text)


def _exact_inputs(loops, points, input_limit):
    try:
        coordinates = {identifier: tuple(Fraction(x) for x in point) for identifier, point in points.items()}
        words = tuple(tuple(word) for word in loops)
    except (ValueError, TypeError, OverflowError) as error:
        raise DomainError('finite exact coordinates and loop words required') from error
    if not coordinates or not words or any(len(p) != 2 for p in coordinates.values()):
        raise DomainError('nonempty two-dimensional source nodes and loops required')
    if len(coordinates) >= 2**32-1 or sum(map(len, words)) >= 2**32-1:
        raise ResourceError('dense source count exceeds uint32')
    for identifier in coordinates:
        _uint(identifier, 'node ID')
    if any(len(word) < 3 for word in words) or any(i not in coordinates for word in words for i in word):
        raise DomainError('loop requires at least three existing source nodes')
    # Bound fixed records before common-denominator expansion.
    fixed = 40 * len(coordinates) + 16 * len(words) + 4 * sum(map(len, words)) + C.sizeof(_Input)
    if fixed > input_limit:
        raise ResourceError('encoded input budget exhausted')
    max_bits = 8 * (input_limit-fixed)
    denominators = [1, 1]
    for point in coordinates.values():
        for axis in range(2):
            divisor = point[axis].denominator
            reduced = denominators[axis] // math.gcd(denominators[axis], divisor)
            if reduced.bit_length() + divisor.bit_length() > max_bits + 1:
                raise ResourceError('common denominator exceeds input budget')
            denominators[axis] = reduced * divisor
    # Reserve both squared distance weights before expanding any node. A
    # conservative product-bit bound avoids allocating an over-budget product.
    projected = fixed
    for denominator in denominators:
        projected += 4*((2*denominator.bit_length()+31)//32)
    if projected > input_limit:
        raise ResourceError('distance weights exceed input budget')
    scaled = {}
    for identifier, point in coordinates.items():
        values = []
        for axis in range(2):
            numerator = point[axis].numerator
            if numerator == 0:
                values.append(0)
                continue
            factor = denominators[axis] // point[axis].denominator
            bits = abs(numerator).bit_length() + factor.bit_length()
            required = 4*((bits+31)//32)
            if required > input_limit-projected:
                raise ResourceError('normalized coordinates exceed aggregate input budget')
            value = numerator * factor
            projected += 4*((abs(value).bit_length()+31)//32)
            values.append(value)
        scaled[identifier] = tuple(values)
    if any(2*d.bit_length() > max_bits+1 for d in denominators):
        raise ResourceError('distance weights exceed input budget')
    return words, coordinates, scaled, tuple(denominators), fixed


class FaceDomain:
    """Own one immutable native domain; independent instances may run concurrently."""
    def __init__(self, loops, points, *, token=0, memory_limit=1 << 30,
                 input_limit=1 << 28, work_limit=1 << 34):
        start = time.perf_counter_ns()
        self._handle = C.c_void_p()
        self.token = _uint(token, 'token')
        self.input_limit = _uint(input_limit, 'input_limit')
        self.memory_limit = _uint(memory_limit, 'memory_limit')
        self.work_limit = _uint(work_limit, 'work_limit')
        self.loops, self.points, scaled, denominators, fixed = _exact_inputs(loops, points, input_limit)
        self._denominators = denominators
        self._scaled = scaled
        identifiers = tuple(scaled)
        slots = {identifier: i for i, identifier in enumerate(identifiers)}
        limbs = []

        def integer(value):
            offset = len(limbs); sign = (value > 0) - (value < 0); value = abs(value)
            count = (value.bit_length() + 31) // 32
            if count >= 2**32 or fixed + 4*(offset+count) > input_limit:
                raise ResourceError('encoded exact input budget exhausted')
            data = value.to_bytes(4*count, 'little')
            limbs.extend(int.from_bytes(data[i:i+4], 'little') for i in range(0, len(data), 4))
            return _Integer(offset, count, sign)

        nodes = (_Node * len(identifiers))(*(_Node(i, integer(scaled[i][0]), integer(scaled[i][1])) for i in identifiers))
        x_weight = integer(denominators[1]**2)
        y_weight = integer(denominators[0]**2)
        words = []; indices = []
        for word in self.loops:
            words.append(_Loop(len(indices), len(word)))
            indices.extend(slots[i] for i in word)
        word_array = (_Loop * len(words))(*words)
        index_array = (C.c_uint32 * len(indices))(*indices)
        limb_array = (C.c_uint32 * len(limbs))(*limbs)
        record = _Input(1, C.sizeof(_Input), 0, token, memory_limit, work_limit,
                        nodes, len(nodes), word_array, len(word_array), index_array, len(index_array),
                        limb_array, len(limb_array), x_weight, y_weight)
        self.encoded_bytes = fixed + 4*len(limbs)
        self.normalize_ns = time.perf_counter_ns() - start
        self._lib = _library()
        error = _Error()
        _check(self._lib.bgfd_create(C.byref(record), C.byref(self._handle), C.byref(error)), error)

    def _live(self):
        if not self._handle.value:
            raise DomainError('native face-domain context is closed')

    @property
    def stats(self):
        self._live(); result = _Stats(); result.abi = 1; result.size = C.sizeof(_Stats); error = _Error()
        _check(self._lib.bgfd_get_stats(self._handle, C.byref(result), C.byref(error)), error)
        if result.token != self.token:
            raise RuntimeError('native source token differs')
        return Statistics(*(getattr(result, name) for name in _STAT_NAMES))

    def first_crossing(self):
        self._live(); result = _Crossing(); result.abi = 1; result.size = C.sizeof(_Crossing); error = _Error()
        _check(self._lib.bgfd_first_crossing(self._handle, C.byref(result), C.byref(error)), error)
        if not result.found:
            return None
        ids = tuple(result.ids)
        if any(i not in self.points for i in ids):
            raise RuntimeError('native crossing contains foreign source nodes')
        return ids

    def triangulate(self):
        self._live(); error = _Error()
        _check(self._lib.bgfd_triangulate(self._handle, C.byref(error)), error)
        count = C.c_uint64()
        _check(self._lib.bgfd_copy_triangles(self._handle, None, 0, C.byref(count), C.byref(error)), error)
        values = (C.c_uint64 * count.value)()
        _check(self._lib.bgfd_copy_triangles(self._handle, values, count.value, C.byref(count), C.byref(error)), error)
        if count.value % 3 or any(i not in self.points for i in values):
            raise RuntimeError('native triangles contain invalid source nodes')
        return tuple(tuple(values[i:i+3]) for i in range(0, count.value, 3))

    def close(self):
        if self._handle.value:
            self._lib.bgfd_destroy(self._handle); self._handle.value = None

    def __enter__(self):
        self._live(); return self

    def __exit__(self, *_):
        self.close()

    def __del__(self):
        if getattr(self, '_handle', None) is not None and self._handle.value:
            self.close()


def first_crossing(loops, points, **kwargs):
    with FaceDomain(loops, points, **kwargs) as domain:
        return domain.first_crossing()


def triangulate(loops, points, **kwargs):
    with FaceDomain(loops, points, **kwargs) as domain:
        return domain.triangulate()


class _CDTInput(C.Structure):
    _fields_ = [('abi', C.c_uint32), ('size', C.c_uint32),
                ('seed', C.c_uint64), ('reserved', C.c_uint64), ('domain', _Input)]


_CDT_STAT_NAMES = (
    'token seed node_count triangles validation_work work memory_peak max_limbs '
    'validation_ns prepare_ns construct_ns copy_ns merge_steps crossed_edges '
    'cavity_occurrences cavity_insertions secondary_deletions max_cavity quadedge_peak triangle_peak constraints '
    'location_steps hash_probes secondary_calls initial_triangles void_triangles '
    'order_ns initial_ns conversion_ns recovery_ns selection_ns '
    'axis_compares orientations incircles line_distance_compares scalar_signs interval_signs '
    'exact_signs exact_zeros floating_disabled enclosure_values exact_lifts').split()


class _CDTStats(C.Structure):
    _fields_ = [('abi', C.c_uint32), ('size', C.c_uint32)] + [
        (name, C.c_uint64) for name in _CDT_STAT_NAMES]


CDTStatistics = make_dataclass('CDTStatistics', [(name, int) for name in _CDT_STAT_NAMES], frozen=True)


def _strict_uint(value, label):
    if type(value) is not int:
        raise DomainError(f'{label} must be an unsigned64-bit integer')
    return _uint(value, label)


def direct_cdt_library_path():
    configured = os.environ.get('BREPGPU_DIRECT_CDT_LIBRARY')
    root = Path(__file__).resolve().parents[2]
    candidates = ([Path(configured)] if configured else [
        root / 'build' / 'direct-cdt-release' / 'libbrepgpu_direct_cdt.so',
        root / 'build' / 'direct-cdt-debug' / 'libbrepgpu_direct_cdt.so'])
    path = next((p for p in candidates if p.is_file()), None)
    if path is None:
        raise RuntimeError('native direct CDT library unavailable; set BREPGPU_DIRECT_CDT_LIBRARY')
    return path.resolve()


def _cdt_library():
    lib = C.CDLL(str(direct_cdt_library_path()))
    lib.bgcdt_abi_size.argtypes = [C.c_uint32]
    lib.bgcdt_abi_size.restype = C.c_uint32
    expected = (1, C.sizeof(_CDTInput), C.sizeof(_Error), C.sizeof(_CDTStats),
                C.sizeof(_Node), C.sizeof(_Integer), C.sizeof(_Loop), C.sizeof(_Input))
    if any(lib.bgcdt_abi_size(i) != value for i, value in enumerate(expected)):
        raise RuntimeError('native direct CDT ABI or record layout differs')
    lib.bgcdt_create.argtypes = [C.POINTER(_CDTInput), C.POINTER(C.c_void_p), C.POINTER(_Error)]
    lib.bgcdt_construct.argtypes = [C.c_void_p, C.POINTER(_Error)]
    lib.bgcdt_copy_triangles.argtypes = [C.c_void_p, C.POINTER(C.c_uint64), C.c_uint64,
                                       C.POINTER(C.c_uint64), C.POINTER(_Error)]
    lib.bgcdt_get_stats.argtypes = [C.c_void_p, C.POINTER(_CDTStats), C.POINTER(_Error)]
    lib.bgcdt_destroy.argtypes = [C.c_void_p]
    for name in ('bgcdt_create', 'bgcdt_construct', 'bgcdt_copy_triangles', 'bgcdt_get_stats'):
        getattr(lib, name).restype = C.c_int
    lib.bgcdt_destroy.restype = None
    return lib


def require_direct_cdt_available():
    """Resolve all direct symbols and record layouts before source preparation."""
    _cdt_library()


def _metric_weights(metric, denominators, available):
    try:
        scales = tuple(Fraction(value) for value in metric)
    except (ValueError, TypeError, OverflowError) as error:
        raise DomainError('metric requires two finite positive exact scales') from error
    if len(scales) != 2 or any(value <= 0 for value in scales):
        raise DomainError('metric requires two finite positive exact scales')
    # Primitive positive integral ratio of the exact squared metric weights.
    # Cancel before products/squares so a common huge scale costs no extra bits.
    left = [scales[0].numerator, denominators[1], scales[1].denominator]
    right = [scales[1].numerator, denominators[0], scales[0].denominator]
    for i in range(len(left)):
        for j in range(len(right)):
            common = math.gcd(left[i], right[j])
            left[i] //= common
            right[j] //= common
    weights = []
    for factors in (left, right):
        bits = 2 * sum(value.bit_length() for value in factors)
        if 4*((bits+31)//32) > available:
            raise ResourceError('metric weights exceed encoded input budget')
        product = math.prod(factors)
        value = product * product
        available -= 4*((value.bit_length()+31)//32)
        weights.append(value)
    return scales, tuple(weights)


class DirectCDT:
    """Own exact fixed sites and one bounded native constrained construction."""
    def __init__(self, loops, points, *, metric=(1., 1.), seed=0, token=0,
                 input_limit=1 << 28, memory_limit=1 << 30, work_limit=1 << 34):
        start = time.perf_counter_ns()
        self._handle = C.c_void_p()
        self.seed = _strict_uint(seed, 'seed')
        self.token = _strict_uint(token, 'token')
        self.input_limit = _strict_uint(input_limit, 'input_limit')
        self.memory_limit = _strict_uint(memory_limit, 'memory_limit')
        self.work_limit = _strict_uint(work_limit, 'work_limit')
        try:
            words = tuple(tuple(word) for word in loops)
            identifiers = tuple(points)
        except TypeError as error:
            raise DomainError('source mapping and loop words required') from error
        for identifier in identifiers:
            _strict_uint(identifier, 'source node ID')
        for word in words:
            for identifier in word:
                _strict_uint(identifier, 'loop source node ID')
        self.loops, self.points, scaled, denominators, fixed = _exact_inputs(
            words, points, input_limit)
        fixed += C.sizeof(_CDTInput) - C.sizeof(_Input)
        coordinate_bytes = sum(4*((abs(value).bit_length()+31)//32)
                               for point in scaled.values() for value in point)
        if fixed + coordinate_bytes > input_limit:
            raise ResourceError('encoded exact input budget exhausted')
        self.metric, weights = _metric_weights(metric, denominators,
                                               input_limit-fixed-coordinate_bytes)
        slots = {identifier: i for i, identifier in enumerate(identifiers)}
        limbs = []

        def integer(value):
            offset = len(limbs)
            sign = (value > 0) - (value < 0)
            value = abs(value)
            count = (value.bit_length()+31)//32
            if count >= 2**32 or fixed + 4*(offset+count) > input_limit:
                raise ResourceError('encoded exact input budget exhausted')
            data = value.to_bytes(4*count, 'little')
            limbs.extend(int.from_bytes(data[i:i+4], 'little') for i in range(0, len(data), 4))
            return _Integer(offset, count, sign)

        nodes = (_Node * len(identifiers))(*(
            _Node(i, integer(scaled[i][0]), integer(scaled[i][1])) for i in identifiers))
        wx, wy = map(integer, weights)
        loop_records = []
        indices = []
        for word in self.loops:
            loop_records.append(_Loop(len(indices), len(word)))
            indices.extend(slots[i] for i in word)
        word_array = (_Loop * len(loop_records))(*loop_records)
        index_array = (C.c_uint32 * len(indices))(*indices)
        limb_array = (C.c_uint32 * len(limbs))(*limbs)
        source = _Input(1, C.sizeof(_Input), 0, token, memory_limit, work_limit,
                        nodes, len(nodes), word_array, len(word_array), index_array, len(index_array),
                        limb_array, len(limb_array), wx, wy)
        record = _CDTInput(1, C.sizeof(_CDTInput), seed, 0, source)
        self.encoded_bytes = fixed + 4*len(limbs)
        self.normalize_ns = time.perf_counter_ns() - start
        self._lib = _cdt_library()
        error = _Error()
        _check(self._lib.bgcdt_create(C.byref(record), C.byref(self._handle), C.byref(error)), error)

    def _live(self):
        if not self._handle.value:
            raise DomainError('native direct CDT context is closed')

    def triangulate(self):
        self._live()
        error = _Error()
        _check(self._lib.bgcdt_construct(self._handle, C.byref(error)), error)
        count = C.c_uint64()
        _check(self._lib.bgcdt_copy_triangles(self._handle, None, 0, C.byref(count), C.byref(error)), error)
        expected = len(self.points) + 2*(len(self.loops)-1) - 2
        if count.value != 3*expected:
            raise RuntimeError('native direct CDT result count differs from source domain')
        values = (C.c_uint64 * count.value)()
        _check(self._lib.bgcdt_copy_triangles(self._handle, values, count.value,
                                            C.byref(count), C.byref(error)), error)
        if count.value != len(values) or any(i not in self.points for i in values):
            raise RuntimeError('native direct CDT result contains invalid source nodes')
        return tuple(tuple(values[i:i+3]) for i in range(0, count.value, 3))

    @property
    def stats(self):
        self._live()
        result = _CDTStats()
        result.abi = 1
        result.size = C.sizeof(result)
        error = _Error()
        _check(self._lib.bgcdt_get_stats(self._handle, C.byref(result), C.byref(error)), error)
        if result.token != self.token or result.seed != self.seed:
            raise RuntimeError('native direct CDT source identity differs')
        return CDTStatistics(*(getattr(result, name) for name in _CDT_STAT_NAMES))

    def close(self):
        if self._handle.value:
            self._lib.bgcdt_destroy(self._handle)
            self._handle.value = None

    def __enter__(self):
        self._live()
        return self

    def __exit__(self, *_):
        self.close()

    def __del__(self):
        if getattr(self, '_handle', None) is not None and self._handle.value:
            self.close()


def constrained_delaunay(loops, points, **kwargs):
    with DirectCDT(loops, points, **kwargs) as domain:
        return domain.triangulate()


def write_snapshot(path, loops, points, *, token=0, input_limit=1 << 28):
    """Write owned lossless BGFAC001 exact input, outside measured construction."""
    words, coordinates, scaled, denominators, _ = _exact_inputs(loops, points, input_limit)
    body = bytearray()

    def append(data):
        if len(data) > input_limit-40-len(body):
            raise ResourceError('snapshot input budget exhausted')
        body.extend(data)

    def integer(value):
        sign = (value > 0) - (value < 0); value = abs(value)
        count = (value.bit_length()+7)//8
        if count+9 > input_limit-40-len(body):
            raise ResourceError('snapshot input budget exhausted')
        append(struct.pack('<bQ', sign, count)); append(value.to_bytes(count, 'little'))

    integer(denominators[0]); integer(denominators[1])
    for identifier in coordinates:
        append(struct.pack('<Q', identifier)); integer(scaled[identifier][0]); integer(scaled[identifier][1])
    for word in words:
        if 8+8*len(word) > input_limit-40-len(body):
            raise ResourceError('snapshot input budget exhausted')
        append(struct.pack('<Q', len(word))); append(struct.pack('<'+'Q'*len(word), *word))
    header = struct.pack('<8sQQQQ', b'BGFAC001', _uint(token, 'token'), len(coordinates), len(words), len(body))
    Path(path).write_bytes(header+body)


def read_snapshot(path, *, input_limit=1 << 28):
    """Return (loops, original exact points, token) from a bounded owned snapshot."""
    path = Path(path)
    if path.stat().st_size > input_limit:
        raise ResourceError('snapshot input budget exhausted')
    with path.open('rb') as stream:
        raw = stream.read(input_limit+1)
    if len(raw) > input_limit:
        raise ResourceError('snapshot input budget exhausted')
    cursor = 0

    def take(count):
        nonlocal cursor
        if count < 0 or count > len(raw)-cursor:
            raise DomainError('truncated face snapshot')
        value = raw[cursor:cursor+count]; cursor += count; return value

    if len(raw) < 40:
        raise DomainError('truncated face snapshot header')
    magic, token, nodes, loops, size = struct.unpack('<8sQQQQ', take(40))
    if magic != b'BGFAC001' or size != len(raw)-40 or nodes == 0 or loops == 0 or nodes > len(raw)//26 or loops > len(raw)//32:
        raise DomainError('invalid face snapshot header')

    def integer():
        sign, count = struct.unpack('<bQ', take(9)); data = take(count)
        if sign not in (-1, 0, 1) or ((count == 0) != (sign == 0)) or (count and data[-1] == 0):
            raise DomainError('noncanonical face snapshot integer')
        return sign*int.from_bytes(data, 'little')

    du, dv = integer(), integer()
    if du <= 0 or dv <= 0:
        raise DomainError('invalid face snapshot denominator')
    points = {}
    for _ in range(nodes):
        identifier, = struct.unpack('<Q', take(8))
        if identifier in points:
            raise DomainError('duplicate face snapshot source ID')
        points[identifier] = (Fraction(integer(), du), Fraction(integer(), dv))
    words = []
    for _ in range(loops):
        count, = struct.unpack('<Q', take(8))
        if count < 3 or count > (len(raw)-cursor)//8:
            raise DomainError('invalid face snapshot loop')
        word = struct.unpack('<'+'Q'*count, take(8*count))
        if any(i not in points for i in word):
            raise DomainError('face snapshot loop references missing source ID')
        words.append(word)
    if cursor != len(raw):
        raise DomainError('face snapshot trailing bytes')
    return tuple(words), points, token
