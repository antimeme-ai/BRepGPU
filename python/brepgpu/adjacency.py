"""Owned immutable packed topology input and synchronous ABI5 adjacency consumer.

Rows contain coedge occurrences, in unspecified order. A same-face opposed pair
is a seam only when the original body establishes that geometric interpretation.
No topology editing, radial ordering, or whole-solid certification is implied.
"""
from array import array
import ctypes as C
from dataclasses import dataclass
import operator
import os
from pathlib import Path
import struct
import sys
import time

from . import BgError, Diagnostic
from .body import BodySnapshot

INVALID = (1 << 32) - 1
MAX_RECORDS = 1 << 22
UNUSED, BOUNDARY, PAIRED, SAME_FACE_PAIR, ORIENTATION_CONFLICT, MULTIPLE_USES = range(6)
_NATIVE_PHASES = ("admission_ms", "context_ms", "allocate_ms", "upload_ms",
                  "count_ms", "scan_ms", "fill_ms", "reader_ms", "download_ms",
                  "cleanup_ms", "total_ms")


def _integer(value, lower, upper, label):
    if isinstance(value, bool):
        raise TypeError(label + " must be an integer, not bool")
    answer = operator.index(value)
    if not lower <= answer <= upper:
        raise ValueError(label + " outside supported range")
    return answer


@dataclass(frozen=True, slots=True)
class Source:
    """One occurrence of a typed entity; containing table supplies its kind."""
    instance: int
    body_id: int
    revision: int
    entity_id: int

    def __post_init__(self):
        for name in ("instance", "body_id", "revision", "entity_id"):
            object.__setattr__(self, name, _integer(getattr(self, name),
                0 if name == "instance" else 1, (1 << 64) - 1, name))


def _numbers(data, signed=False):
    answer = array("i" if signed else "I")
    answer.frombytes(data)
    return answer


def _freeze_array(values, count, signed, label):
    if isinstance(values, (bytes, bytearray, memoryview)):
        data = bytes(values)
        if len(data) != count * 4:
            raise ValueError(label + " has wrong byte extent")
        return data
    numbers = array("i" if signed else "I")
    for index, value in enumerate(values):
        if index >= count:
            raise ValueError(label + " has wrong element count")
        numbers.append(_integer(value, -(1 << 31) if signed else 0,
                               (1 << 31) - 1 if signed else INVALID, label))
    if len(numbers) != count:
        raise ValueError(label + " has wrong element count")
    return numbers.tobytes()


def _lineage(source):
    return source.instance, source.body_id, source.revision


@dataclass(frozen=True, slots=True, init=False)
class TopologyInput:
    """Validated owned source tables and compact slot/sign payloads.

    General records describe incidence only. pack_bodies retains the original
    factory snapshots so a caller/oracle can also resolve loop and UV provenance.
    """
    edge_sources: tuple
    face_sources: tuple
    coedge_sources: tuple
    coedge_edges: bytes
    coedge_faces: bytes
    orientations: bytes
    snapshots: tuple

    def __init__(self, edge_sources, face_sources, coedge_sources, coedge_edges,
                 coedge_faces, orientations, *, snapshots=()):
        if sys.byteorder != "little" or array("I").itemsize != 4 or array("i").itemsize != 4:
            raise RuntimeError("topology binary input requires little-endian 32-bit integers")
        tables = tuple(tuple(rows) for rows in (edge_sources, face_sources, coedge_sources))
        for rows in tables:
            if len(rows) > MAX_RECORDS:
                raise ValueError("topology record cap exceeded")
            if any(type(source) is not Source for source in rows):
                raise TypeError("typed source tables require Source records")
            if len(set(rows)) != len(rows):
                raise ValueError("duplicate typed source occurrence")
        edge_table, face_table, coedge_table = tables
        instances = {}
        for rows in tables:
            for source in rows:
                lineage = source.body_id, source.revision
                if instances.setdefault(source.instance, lineage) != lineage:
                    raise ValueError("an instance names multiple body revisions")
        count = len(coedge_table)
        edges = _freeze_array(coedge_edges, count, False, "coedge edge")
        faces = _freeze_array(coedge_faces, count, False, "coedge face")
        signs = _freeze_array(orientations, count, True, "orientation")
        for source, edge, face, sign in zip(coedge_table, _numbers(edges),
                                          _numbers(faces), _numbers(signs, True)):
            if edge >= len(edge_table) or face >= len(face_table):
                raise ValueError("coedge references a slot outside its typed table")
            if sign not in (-1, 1):
                raise ValueError("orientation must be -1 or +1")
            if _lineage(source) != _lineage(edge_table[edge]) or _lineage(source) != _lineage(face_table[face]):
                raise ValueError("coedge reference crosses instance or body revision")
        snapshots = tuple(snapshots)
        if any(not isinstance(body, BodySnapshot) for body in snapshots):
            raise TypeError("retained snapshots require BodySnapshot")
        if snapshots:
            for instance, lineage in instances.items():
                if instance >= len(snapshots) or lineage != (snapshots[instance].body_id,
                                                           snapshots[instance].revision):
                    raise ValueError("source lineage differs from retained snapshot occurrence")
        for name, value in zip(("edge_sources", "face_sources", "coedge_sources",
                                "coedge_edges", "coedge_faces", "orientations", "snapshots"),
                               (*tables, edges, faces, signs, snapshots)):
            object.__setattr__(self, name, value)

    @property
    def edge_count(self): return len(self.edge_sources)

    @property
    def face_count(self): return len(self.face_sources)

    @property
    def coedge_count(self): return len(self.coedge_sources)

    def permuted(self, edge_order=None, face_order=None, coedge_order=None):
        """Orders name old slots at each new slot; rewrite every dependent slot."""
        def order(values, count, name):
            if values is None:
                return tuple(range(count))
            answer = tuple(_integer(v, 0, count - 1, name) for v in values)
            if len(answer) != count or len(set(answer)) != count:
                raise ValueError(name + " must contain every original slot exactly once")
            return answer
        eo = order(edge_order, self.edge_count, "edge order")
        fo = order(face_order, self.face_count, "face order")
        co = order(coedge_order, self.coedge_count, "coedge order")
        edge_inverse = [0] * self.edge_count
        face_inverse = [0] * self.face_count
        for new, old in enumerate(eo): edge_inverse[old] = new
        for new, old in enumerate(fo): face_inverse[old] = new
        edges, faces, signs = _numbers(self.coedge_edges), _numbers(self.coedge_faces), _numbers(self.orientations, True)
        return TopologyInput(tuple(self.edge_sources[i] for i in eo),
            tuple(self.face_sources[i] for i in fo), tuple(self.coedge_sources[i] for i in co),
            (edge_inverse[edges[i]] for i in co), (face_inverse[faces[i]] for i in co),
            (signs[i] for i in co), snapshots=self.snapshots)


def pack_bodies(bodies):
    """Flatten snapshots as distinct batch instances, retaining all use identities."""
    bodies = tuple(bodies)
    if any(not isinstance(body, BodySnapshot) for body in bodies):
        raise TypeError("pack_bodies requires immutable BodySnapshot instances")
    for name in ("edges", "faces", "coedges"):
        if sum(len(getattr(body, name)) for body in bodies) > MAX_RECORDS:
            raise ValueError("topology record cap exceeded")
    edge_sources, face_sources, coedge_sources = [], [], []
    edges, faces, signs = array("I"), array("I"), array("i")
    edge_offset = face_offset = 0
    for instance, body in enumerate(bodies):
        for records, target in ((body.edges, edge_sources), (body.faces, face_sources),
                                (body.coedges, coedge_sources)):
            target.extend(Source(instance, body.body_id, body.revision, record.id) for record in records)
        edges.extend(edge_offset + coedge.edge_slot for coedge in body.coedges)
        faces.extend(face_offset + coedge.face_slot for coedge in body.coedges)
        signs.extend(coedge.orientation for coedge in body.coedges)
        edge_offset += len(body.edges)
        face_offset += len(body.faces)
    return TopologyInput(edge_sources, face_sources, coedge_sources, edges.tobytes(),
                         faces.tobytes(), signs.tobytes(), snapshots=bodies)


class _Desc(C.Structure):
    _fields_ = [("abi_version", C.c_uint32), ("struct_size", C.c_uint32),
                ("backend", C.c_uint32), ("device", C.c_int32),
                ("threads", C.c_uint32), ("reserved", C.c_uint32),
                ("edge_count", C.c_uint64), ("face_count", C.c_uint64),
                ("coedge_count", C.c_uint64)]


class _Timings(C.Structure):
    _fields_ = [("abi_version", C.c_uint32), ("struct_size", C.c_uint32)] + [
        (name, C.c_double) for name in _NATIVE_PHASES]


class _Info(C.Structure):
    _fields_ = [(name, C.c_uint32) for name in (
        "abi_version", "struct_size", "desc_size", "timings_size", "diagnostic_size", "cuda_compiled")]


def _slot(value, count):
    return _integer(value, 0, count - 1, "snapshot slot")


def _read(data, slot):
    return struct.unpack_from("<I", data, 4 * slot)[0]


@dataclass(frozen=True, slots=True, init=False)
class AdjacencyResult:
    input: TopologyInput
    offsets: bytes
    members: bytes
    kinds: bytes
    mates: bytes
    neighbors: bytes
    timings: tuple
    backend: str
    device: int
    threads: int

    def __init__(self, *args, **kwargs):
        raise TypeError("AdjacencyResult is published by AdjacencyLibrary.build")

    def edge_uses(self, edge_slot):
        edge_slot = _slot(edge_slot, self.input.edge_count)
        start, stop = _read(self.offsets, edge_slot), _read(self.offsets, edge_slot + 1)
        if not 0 <= start <= stop <= self.input.coedge_count:
            raise RuntimeError("native adjacency row outside output extent")
        return tuple(self.input.coedge_sources[_read(self.members, i)] for i in range(start, stop))

    def edge_kind(self, edge_slot):
        return _read(self.kinds, _slot(edge_slot, self.input.edge_count))

    def mate(self, coedge_slot):
        slot = _read(self.mates, _slot(coedge_slot, self.input.coedge_count))
        return None if slot == INVALID else self.input.coedge_sources[slot]

    def neighbor(self, coedge_slot):
        slot = _read(self.neighbors, _slot(coedge_slot, self.input.coedge_count))
        return None if slot == INVALID else self.input.face_sources[slot]


class AdjacencyLibrary:
    """Separate synchronous library. Every build allocates/uploads/frees anew."""
    def __init__(self, path=None):
        if sys.byteorder != "little" or C.sizeof(C.c_uint32) != 4:
            raise RuntimeError("ABI5 requires little-endian 32-bit slot storage")
        selected = path if path is not None else os.environ.get("BREPGPU_TOPOLOGY_LIBRARY")
        if selected is None:
            raise ValueError("supply a topology library path or BREPGPU_TOPOLOGY_LIBRARY")
        self.path = Path(selected).resolve()
        self._lib = C.CDLL(str(self.path))
        U, P, S = C.c_uint64, C.POINTER(C.c_uint32), C.POINTER(C.c_int32)
        self._lib.bg5_abi_info.argtypes = [C.POINTER(_Info), U]
        self._lib.bg5_abi_info.restype = C.c_uint32
        self._lib.bg5_adjacency.argtypes = [C.POINTER(_Desc), U, P, P, S, U,
            P, U, P, U, P, U, P, P, U, C.POINTER(_Timings), U, C.POINTER(Diagnostic), U]
        self._lib.bg5_adjacency.restype = C.c_uint32
        info = _Info()
        code = self._lib.bg5_abi_info(C.byref(info), C.sizeof(info))
        if code:
            raise BgError(code, message="topology ABI query failed")
        expected = (5, C.sizeof(_Info), C.sizeof(_Desc), C.sizeof(_Timings), C.sizeof(Diagnostic))
        if tuple(getattr(info, name) for name, _ in _Info._fields_[:5]) != expected or info.cuda_compiled not in (0, 1):
            raise RuntimeError("topology ABI5 layout mismatch")
        self.cuda_compiled = bool(info.cuda_compiled)

    def build(self, input, *, backend="cpu", device=0, threads=1):
        wall_start = time.perf_counter()
        if type(input) is not TopologyInput:
            raise TypeError("build requires validated TopologyInput")
        if backend not in ("cpu", "cuda"):
            raise ValueError("backend must be cpu or cuda")
        device = _integer(device, 0, (1 << 31) - 1, "device")
        threads = _integer(threads, 1, 24, "threads")
        if backend == "cpu" and device != 0:
            raise ValueError("CPU device must be zero")
        e, f, c = input.edge_count, input.face_count, input.coedge_count
        descriptor = _Desc(5, C.sizeof(_Desc), 0 if backend == "cpu" else 1,
                           device, threads, 0, e, f, c)
        allocation_start = time.perf_counter()
        output = ((C.c_uint32 * (e + 1))(), (C.c_uint32 * c)(),
                  (C.c_uint32 * e)(), (C.c_uint32 * c)(), (C.c_uint32 * c)())
        borrowed = tuple(C.c_char_p(data) for data in (input.coedge_edges, input.coedge_faces, input.orientations))
        pointers = (C.cast(borrowed[0], C.POINTER(C.c_uint32)),
                    C.cast(borrowed[1], C.POINTER(C.c_uint32)),
                    C.cast(borrowed[2], C.POINTER(C.c_int32)))
        timings, diagnostic = _Timings(), Diagnostic()
        allocation_ms = (time.perf_counter() - allocation_start) * 1000
        native_start = time.perf_counter()
        code = self._lib.bg5_adjacency(C.byref(descriptor), C.sizeof(descriptor),
            *pointers, c, output[0], e + 1, output[1], c, output[2], e,
            output[3], output[4], c, C.byref(timings), C.sizeof(timings),
            C.byref(diagnostic), C.sizeof(diagnostic))
        native_ms = (time.perf_counter() - native_start) * 1000
        if code:
            error = BgError(code, diagnostic)
            if code == 4:
                raise MemoryError(error)
            raise error
        if (timings.abi_version, timings.struct_size) != (5, C.sizeof(_Timings)):
            raise RuntimeError("topology timing ABI mismatch")
        publication_start = time.perf_counter()
        result = object.__new__(AdjacencyResult)
        for name, value in zip(("input", "offsets", "members", "kinds", "mates", "neighbors",
                               "backend", "device", "threads"),
                              (input, *(bytes(buffer) for buffer in output), backend, device, threads)):
            object.__setattr__(result, name, value)
        publication_ms = (time.perf_counter() - publication_start) * 1000
        phases = tuple((name, float(getattr(timings, name))) for name in _NATIVE_PHASES)
        object.__setattr__(result, "timings", phases + (
            ("wrapper_allocation_ms", allocation_ms), ("native_call_ms", native_ms),
            ("publication_ms", publication_ms), ("wrapper_wall_ms", (time.perf_counter() - wall_start) * 1000)))
        return result
