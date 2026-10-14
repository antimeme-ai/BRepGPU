# Edge-use adjacency beachhead

Status: implemented contract after adversarial design/plan/code review, 2026-10-14.

## Purpose and bounded claim

Construct packed edge-to-coedge CSR on CPU or one P100, then consume that CSR
through a mate/face-neighbor reader. This delivers GPU topology construction on
immutable records already owned by the body layer. It does not edit bodies,
construct Boolean topology, infer geometric coincidence or certify a solid.

Research inputs are the topology, runtime and oracle studies in papers/. The
algorithm uses owned count/scan/fill, informed by parallel scan literature;
there is no new dependency. Existing ABI1–4 libraries and archives stay intact.
A separate `brepgpu_topology` library provides synchronous ABI5 calls. It does
not extend Engine lifetime machinery or introduce persistent device sessions.

## Semantic input

A prepared immutable input owns E edge sources, F face sources, C coedge sources,
and three C-element arrays: edge slot uint32, face slot uint32, orientation int32.
A typed source is `(instance, body_id, revision, entity_id)`; its containing
edge/face/coedge array supplies the entity kind. Body IDs, revisions and entity IDs are positive uint64;
instance is a uint64 batch occurrence, distinct from geometric entity identity.
Each instance maps to exactly one body/revision across all source tables.
All source keys within a kind are unique. Every coedge references an edge and a
face with the same instance/body/revision. No coordinate or pointer defines a
reference. Source identity stays on CPU; the GPU receives only slots/signs.

Packing a body batch preserves factory edges/coedges/faces, loop membership and
UV occurrence through retained snapshots and source keys. A repeated snapshot
is an explicitly separate batch instance of the same geometry, not a new body
namespace. This permits representative large topology-only construction trials
without pretending repeated primitive components are a complicated single solid.
A general record-input constructor also admits combinatorial fixtures, with no
claim that their topology is a geometrically qualified B-rep. Slot permutations
preserve keys and rewrite both edge and face references.

Input limits: E,F,C each at most 2^22; E,F may be zero only when C is zero;
coedge slots are in range, orientations are exactly -1/+1. These limits ensure
32-bit counts/prefixes cannot overflow, with UINT32_MAX reserved as invalid.
Native admission checks counts and exact payload extents before reading arrays.
Python validates source keys and references before invoking native work.

## Semantic output and actual reader

Output owns offsets[E+1], members[C], kinds[E], mates[C], neighbors[C].
`offsets[0]=0`, offsets are nondecreasing, `offsets[E]=C`. The row of edge e
contains every and only original coedge slot referencing e, exactly once.
Within-row order is unspecified. Loop traversal remains the original source
loop order; CSR storage order never replaces it.

The reader runs after fill and reads those rows. It emits:

| Kind | Condition | Mate/neighbor |
| --- | --- | --- |
| unused (0) | degree 0 | none |
| boundary (1) | degree 1 | invalid |
| paired (2) | degree 2, opposite signs, different faces | other coedge and its face |
| same-face pair (3; seam for qualified bodies) | degree 2, opposite signs, same face | other coedge and same face |
| orientation conflict (4) | degree 2, equal signs | invalid |
| multiple uses (5; no radial order promised) | degree greater than 2 | invalid: no unique mate |

All unassigned mate/neighbor entries are UINT32_MAX. Degree>2 retains complete
membership rather than truncating to two. A seam preserves distinct coedge IDs
and UV maps despite same edge/face. A closed edge whose endpoints coincide has
ordinary incidence semantics. Degree-two pairing alone is not manifold vertex,
loop closure, geometric embedding or watertightness certification.

Python returns immutable owned binary arrays and retained immutable input, with
methods mapping an edge row or reader slot back to typed source occurrences.
A returned result stays usable after the synchronous native call/library object.
It does not expose device pointers, numeric process addresses or mutable buffers.

## Owned construction and execution

CPU1 uses direct count, exclusive prefix, fill, reader. CPU24 uses bounded
parallel ranges and atomic count/fill, a serial prefix scan, and parallel edge
reader; worker creation/join costs are part of the measured operation. Worker
launch failures must join launched workers before propagating. Any temporary
counts/cursors belong to the call.

GPU allocates call-owned compact inputs, counts/cursors and output arrays on one
owned stream. Count atomics count uses. An owned recursive block exclusive scan
(256 lanes, padded zeros and unconditional barriers) scans counts and recursively
scans block totals; uniform-add propagates prefixes. offsets[E] is C, and fill
uses atomic cursors plus offsets. A post-fill reader examines each edge row and
writes at most its two unique mate assignments; initial sentinels cover other
rows. All work is O(E+C), including high-degree rows. No CPU block-total readback,
quadratic per-row sort or dependency scan primitive is used. Single-stream
ordering separates count, scan, fill and reader. E=0 returns one zero offset; C=0 with E>0 returns E+1 zero offsets, unused
edge kinds and empty membership/reader arrays. No zero-sized kernel is launched.

The CUDA device is selected explicitly and the previous device restored on all
paths. Backend/runtime availability is reported, with no silent CPU fallback.
Every resource receives a cleanup attempt on success/failure; runtime cleanup
errors can prevent reclamation and are reported without publication. A cleanup or
device-restore error prevents successful publication. Primary failure wins when
cleanup also fails, with diagnostic cleanup information where feasible. This is
a synchronous call: no retained background work after return.

## ABI5 admission and failure

The input descriptor carries canonical struct_size and ABI5. Output ABI-info
and timings structures are written by the call, with canonical size and ABI5;
no output initialization is required.
The public header names exact element counts for payloads and minimum byte
capacities for structs; larger capacities are accepted. Diagnostic is optional
only as null with zero capacity. CPU device must be zero; CUDA device is
nonnegative.
Payload pointers must be aligned, valid and stable for the call. All output
arrays and descriptor/timings/diagnostic buffers are nonoverlapping with each
other and input arrays; input arrays may overlap each other. Checked pointer
extent arithmetic rejects wraparound before accesses. Native count/extent,
backend/device/thread and slot/sign validation precedes payload output writes.
Structural extent/alias admission precedes any diagnostic write. An unsafe
diagnostic destination receives no write; the status still reports rejection.
Preflight rejection leaves payload outputs untouched. Runtime failure can leave
partial outputs: callers discard every destination, and Python never returns a
partial result. Timings are published only after successful cleanup. Diagnostic
is the existing ABI1 diagnostic shape; this does not change the old core ABI.
CPU thread count is 1..24. CUDA ignores thread count after validating that range.

The standalone CUDA-enabled library contains CPU and GPU paths. A CPU-only build
returns unsupported/backend unavailable for CUDA. ctypes verifies ABI sizes via
an exported ABI information function before calling the operation. The native
operation retains no borrowed pointer beyond its synchronous duration.

## Qualification and performance

A separate oracle enumerates original coedges into typed edge-key dictionaries,
then derives kinds and pair relationships directly from source records; it never
repeats native prefix/count/fill. Literal primitive expectations establish the complete signed edge/coedge/face
source relation, plus retained original loop/UV provenance for box, cylinder
and hole plate; degrees and seams alone are insufficient. The oracle decodes
actual returned binary arrays independently of product result-reader methods. Storage permutation, row-order
freedom, high-degree and disconnected fixtures test identity and incidence;
invalid native extents/ranges/aliases/signs test preflight no-writes. Native host
sanitizers and both-P100 Compute Sanitizer attack memory faults. Direct production
mutants include actual CUDA builder and CUDA reader changes, alongside CPU and
wrapper changes targeting lost occurrences, wrong references, seam deduplication,
signs and mate/face errors; only assertion failures earn kills.

Benchmark serial fresh processes for CPU1/CPU24/P1000/P1001, small canonical
bodies and larger repeated batches, including a measured globally shuffled
coedge assembly. Keep input packing and independent oracle
costs separate. Report cold load/context initialization, native admission,
allocation, upload, count, scan, fill, reader, download and cleanup, native total,
Python publication and full caller wall. Native phase clocks are synchronized
host spans, not kernel-only events. Every timed result consumes the reader and
is checked outside its wall interval. Explicit host/device bytes count topology
slot/sign upload and CSR/reader download; CPU identity tables are not DMA.
Device residency and bulk wrapper/reader improvements are round-two notes,
not an implementation lane. Actual caller TSVs and an owned SVG show topology
incidence and the same-face seam; no preview is presented as geometric proof.
