# Adjacency prepared interface and reader code review

Reviewed root's `python/brepgpu/adjacency.py` against the accepted design,
plan, standalone `include/brepgpu_topology.h` and independent oracle/test unit.
Accepted on static review; no blocking correctness finding. Actual fleet greens
and native fault campaigns remain root-owned execution, not claimed here.

## Identity and immutable source mapping

`Source` normalizes integral fields, rejects bool and enforces uint64/positive
namespace fields. Separate source arrays supply the entity kind; returned mate
and neighbor methods have distinct coedge/face meanings despite overlapping
numeric IDs. `TopologyInput` copies source tables to tuples, requires exact
Source type, checks typed uniqueness and one body/revision per instance, freezes
binary/iterable payloads and checks every reference's bounds, sign and lineage.
Bytearray/memoryview mutation cannot change the frozen owned bytes. Retained
snapshot lineage is checked where snapshots are present; generic incidence
records do not claim to be complete or qualified geometric bodies.

`pack_bodies` keeps independent edge and face offsets, records each repeated
snapshot as its own occurrence ordinal and retains original snapshots. Neither
equal geometry nor repeated body IDs weld occurrences. Its field projection
is simple but still independently attacked by complete literal signed-use
and original loop/UV qualification. General records may retain snapshots yet
represent only subsets or altered incidence; the API docs do not claim such
records are canonical bodies, and `qualify_input(literal=True)` requires the
complete factory relation when that stronger claim is made.

`permuted` interprets new-slot→old-slot orders, validates complete bijections,
builds inverse edge/face maps and rewrites each coedge reference while retaining
source identity and signs. Original loop traversal and UV provenance remain in
the unchanged snapshots; no arbitrary CSR row order becomes loop authority.
The direct mixed/repeated permutation test checks independently decoded source
truth plus consumer mate/neighbor dictionaries in typed occurrence space.

## ABI and ownership boundary

ctypes descriptor, timings and info field widths/order match the current
standalone header; ABI info sizes are checked before the operation. Pointer
counts and output extents agree with the C signature. Endianness/array widths
are checked before compact binary input publication. c_char_p/cast wrappers
and the retained input keep all three immutable byte buffers alive through the
synchronous native call. The header permits input-array overlap; repeated empty
bytes therefore create no invalid borrowing contract. Payload outputs are
separate ctypes arrays and structs remain live until native return.

Only successful native status and compatible timing structure publish a
result. Out-of-memory status becomes MemoryError, other native errors propagate
with diagnostics. No caller receives runtime-partial buffers. Every output is
copied to immutable owned bytes before ctypes destinations are dropped; results
hold their input rather than a process/native pointer and stay usable after
the local library object or call resource lifetime. Native allocation/stream/
device restoration correctness belongs to the native owner's separate review
and real fault tooling, not inference from this Python shape.

## Result readers and timing scope

Public reader slots reject bool, nonintegral and out-of-range values. Edge rows
check offset bounds and map actual membership indices into retained coedge
sources; mate/neighbor methods handle the reserved invalid sentinel and resolve
actual native output through the corresponding typed source table. No different
input can be supplied to reinterpret a result. The independent oracle decodes
the actual raw arrays directly, avoiding circular trust in these readers.

The wrapper reports allocation, native call, immutable-byte publication and
wrapper wall as distinct spans. Native phase timings are retained as named
values. Publication_ms covers bytes/result field creation; timing tuple creation
is inside wrapper wall but after that narrower publication span. This difference
is legitimate when docs/results name each scope exactly. No complete consumer
or kernel-only speed claim follows from wrapper_wall alone.

The direct tests target semantic methods, literal source truth and ownership,
not getters as a substitute for native qualification. Actual malformed C calls
and guarded resource failures stay in the native unit; this review adds no
duplicate fault oracle or new implementation machinery.
