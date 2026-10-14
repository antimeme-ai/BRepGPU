# Native adjacency and wrapper: second topology review

2026-10-14. Independently read `native/topology.cpp`, `native/topology_cuda.cu`,
`native/topology_internal.hpp`, `include/brepgpu_topology.h` and the complete
`python/brepgpu/adjacency.py`. Read the native check fixture and corrected
public-reader tests as context. Accepted: no new incidence, scan, source identity
or seam finding. No implementation edits or execution in this review.

## Count/scan/fill and schedule independence

CPU1 uses value-initialized owned integer counts and direct increments. CPU24
uses atomics for count/reservation; count workers join before the serial prefix
loads, and fill workers join before readers inspect members. Relaxed atomic
reservations give unique per-edge destinations; thread joins establish phase
ordering. Cursor reset follows consumption of each original count. Each input
coedge is emitted once to its one admitted edge row. Row order is deliberately
unspecified; neither native reader assumes a slot ordering within a pair.

The GPU 256-lane upsweep/downsweep visits unconditional barriers, pads partial
tiles with zero, emits each block's full sum, clears the root for an exclusive
scan and writes only valid output lanes. The host allocates sum/prefix arrays
for each shrinking level before launching. Recursive scan consumes the sum
array as the next input and uses that level's prefix array for uniform-add;
stream ordering resolves every dependency without CPU sum readback. At the
admitted maximum E=2^22, the hierarchy is E→16,384→64→1, well within eight
levels and the 32-allocation ledger. All partial/complete sums are at most
C=2^22, so uint32 additions cannot overflow. The final offset explicitly writes
C after all E exclusive prefixes.

GPU count and fill atomics operate on independent zeroed buffers. Fill reserves
one destination per input occurrence, and its completed stream phase precedes
the reader. Zero-edge/zero-use inputs avoid count/fill/reader zero-block launches;
the allocated E+1 offset and one-lane final write still produce a valid empty
result. A highly concentrated row introduces atomic contention, not quadratic
work or truncated storage. Construction is O(E+C).

## Actual reader semantics

Both native paths read degree from the completed offsets. Only degree two
reads two member slots; opposed signs write the exact other coedge and its face
to each original use. Equal signs leave both mates/neighbors invalid. Degree
zero, one and greater than two publish their explicit kind while sentinel
initialization leaves all unpaired uses invalid. The reader visits each edge
once and at most two uses per paired edge, remaining O(E+C), including skew.

Same-face comparison uses face slots belonging to this immutable input. It
therefore preserves the cylinder/plate seam's two distinct occurrences and emits
the same owning face. It never deduplicates members by face, coordinates or
orientation. Closed edge endpoints and geometry charts are not construction
keys. Pair ordering may change between schedules but both mate directions and
neighbor identities remain identical. Kind names correctly avoid geometric or
radial-order claims for arbitrary incidence fixtures.

## Source binding and storage permutations

The wrapper freezes Source keys with uint64 validity and separates kind through
typed containing arrays. Every instance maps to one body/revision across all
tables, including unused sources. Coedge references must match edge AND face
lineage. Repeated snapshots carry distinct instance ordinals while retaining
unchanged body/geometry identity. Geometry remains in the original snapshots;
no entity ID is inferred from a device pointer or slot.

Permutation arguments explicitly name old slots at new slots. The wrapper
constructs inverse edge/face maps, reorders coedge source/sign arrays and rewrites
both reference fields. Original snapshots and their canonical loop order remain
unchanged. The resulting new immutable input is retained by its result. General
record construction validates incidence; full body provenance qualification is
the explicit literal oracle rather than an implied geometric certificate.

Every public reader selects through its result's own input. Edge rows validate
offset extent and resolve actual native members; kind reads actual emitted
bytes; mate and neighbor read actual sentinel/slot buffers before resolving
their respective typed table. The result owns immutable byte copies after the
synchronous call. No borrowed ctypes destination or device allocation survives
as a public entity reference.

## Admission, failures and measured scope

Native structural range/alignment/alias checks precede dereferencing input
payloads. Descriptor/slot/sign admission precedes payload output writes. Inputs
may overlap one another under the documented stable valid-storage requirement,
while descriptor/output/timing/diagnostic capacities must be disjoint. CPU
worker launch failure joins every launched worker before unwinding. GPU paths
record allocations, finish/finalize the stream, free resources and restore the
previous device before the outer call publishes successful timings. A failed
operation returns no Python result; partial native outputs are discarded.

Header and wrapper layouts agree on ABI5, exact array counts, uint32 slots and
the 2^22 cap. Native timing phases are synchronized host spans, not event/kernel
durations. CPU and CUDA place some initialization in different named phases;
native total/full caller scopes include it in both cases. This review does not
claim runtime/sanitizer/qualification execution beyond root's separately
recorded fleet results. No return-pass optimization is needed to accept this
bounded topology construction unit.
