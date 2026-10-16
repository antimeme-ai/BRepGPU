# Native sampled-face domains

Proposal for the operator-authorized mesher surgery and first production C++
geometry slice. This unit consumes an exact immutable sampled face, finds its
first boundary crossing, and emits source-node triangles for an admitted outer
loop with holes. It replaces quadratic candidate enumeration and Python-owned
predicate execution together. It does not change sampling, source preparation,
quality budgets, Lawson flips, refinement, display or CUDA.

Read the [diagnosis](../papers/2026-10-15-om10-mesher-diagnosis.md),
[numerical seam study](../papers/2026-10-15-cpp-migration-numerical-seams.md),
[algorithm study](../papers/2026-10-16-native-meshing-algorithm-options.md) and
[external-case study](../papers/2026-10-16-native-meshing-reference-test-plan.md).
The retained Eberly ear-clipping paper explains bridge occurrences and ear
construction; saved-value exactness follows the retained Shewchuk study. These
are studied algorithms, not selected code or dependencies.

## Contract and ownership

One native context owns decoded coordinates, loop/node arrays, indexes, scratch,
statistics and output. Context creation copies all borrowed input before return.
Destruction is explicit. The binding checks common-denominator bit growth and total encoded bytes during
normalization, before constructing each expanded coordinate; exceeding the
caller budget raises the same resource category. This includes Python-side
preparation rather than permitting an unbounded LCM before native admission.
A context is not concurrently mutable; independent
contexts may run concurrently. No native code calls Python. Initial Python owns
original UV/provenance and checks returned original node IDs against that owner.
The context carries a caller-provided opaque source/preparation/schedule token;
results expose that exact token. The wrapper creates a new context when boundary
coordinates or schedules change, never reusing an index by STEP hash alone.

Input has at least one loop, each with at least three node slots; every slot
references an existing node. Supplied nodes may be unreferenced (they produce no
triangles); every referenced node remains owned. `first_crossing` permits repeated
slot occurrences so it can reproduce the existing shared-node exclusion. Strict
triangulation requires every referenced slot occur once across original loops,
then inserts bridge copies internally. Reused original slots are INVALID_DOMAIN.
Each input node has a distinct uint64 source-local node ID (including valid ID0)
and exact UV; loops
contain uint32 node slots. Coordinates may agree for distinct nodes, while
identity never follows coordinate equality. Original loop order, node order,
seam uses and pole/chart closure occurrences remain unchanged. Internally inserted
bridge copies have distinct occurrence slots that refer to existing node slots.
No coordinate, node ID or physical sample key is created, removed or rounded.
Results contain triples of original node IDs in deterministic construction order.
Physical keys and carrier/source maps remain with the initial producer.

Two operations share the context:

- `first_crossing` returns NONE or the earliest segment pair `(i,j)` in the
  existing flattened loop/segment order, with its four original node IDs.
  Same-loop segment pairs sharing a node ID are excluded exactly as today;
  equal coordinates with different IDs do not trigger this exclusion.
- `triangulate` requires valid strict outer-CCW/hole-CW simple loops: no collapsed
  boundary edges, crossings or touching distinct loops, holes strictly inside
  the outer loop and pairwise exterior. It validates these conditions, bridges
  holes and emits positive triangles. Invalid input returns a typed diagnostic,
  not partial triangles. Structural validation precedes either operation; winding
  and containment validation belongs to triangulate, so boundary-only checks do
  not silently gain new rejection semantics.

The shared boundary-only operation preserves the existing node-sharing exclusion
rather than claiming it alone establishes simplicity for arbitrary reused IDs.
Triangulate additionally rejects repeated node slots within an original loop and
nonadjacent same-loop equal coordinates. Existing imported face loops allocate
unique node occurrences; bridge duplicates appear only after original validation.

## Exact representation

The stdlib-only Python binding converts every supplied coordinate to `Fraction`
once, preserving saved binary64 values exactly. It rejects NaN and infinities.
For each axis it computes a positive common denominator `D`, then writes signed
integers `X = numerator * (D / denominator)`. Native receives canonical signed
little-endian base-2^32 limbs: zero has sign zero/count zero; nonzero has sign
+1/-1 and a nonzero high limb. Python retains the original coordinates.

Native owns a small signed integer type supporting comparison, negation,
addition, subtraction and schoolbook multiplication. It does not need general
division, gcd, decimal conversion or a rational class. Canonical forms and checked
sizes are enforced at decode. Carry accumulation uses uint64 operations with
explicit bounds; multiplication adds one limb product plus the existing limb
and carry, whose maximum fits uint64. No signed overflow is part of arithmetic.

Positive independent axis scales preserve coordinate order, zero and orientation:
`orient = (Bx-Ax)*(Cy-Ay) - (By-Ay)*(Cx-Ax)`. Closed segment touches, inclusive
triangle containment and winding use exact signs. Midpoints used for bridge
admission remain doubled integer coordinates: compare `2*A` to `H+O`, and orient
against that doubled point. No rational division or rounded midpoint is needed.

Hole-bridge candidate distance intentionally changes from the old rounded sqrt
ranking to exact original-UV squared distance. The integer comparison is
`dx^2 * Dv^2 + dy^2 * Du^2`; the omitted common denominator is positive. The
binding supplies positive exact integer weights `Dv^2, Du^2`. Equal exact distances
use current outer occurrence position, then original node ID. Distinct exact
distances that previously collided after rounded sqrt may now change the bridge
and triangle sequence as well; parity is not promised for those inputs. Hole selection
preserves first-max traversal ties by `(max U, -V)` and the first rightmost/lower
hole vertex. This declared ranking change may change diagonals; it must still
pass the independent complete-domain oracle. No other numeric tolerance changes.

Unbounded integers mean dynamically sized within available resources, not an
unbounded service promise. Explicit input-byte, scratch-byte and work budgets
are checked with overflow-safe arithmetic. Exhausting a budget/allocation returns
RESOURCE_EXHAUSTED with no geometric answer. The native work counter charges
BVH visits, geometric predicate calls and each limb compare/add/subtract/multiply
iteration, including sorting and distance comparisons. A multiplication checks
the overflow-safe product of limb counts against remaining work before beginning
and charges its actual inner iterations; a single huge operand cannot evade the
cap. An owned stdlib `pmr::memory_resource` accounts allocations/deallocations
for coordinate limbs, temporary arithmetic, indexes, links, candidate capacities
and results, with high-water bytes; fixed context storage is included separately.
All BigInt copies/moves and arithmetic temporaries retain the context resource
explicitly; no default-resource vector or allocator-copy escape is permitted.
This capacity accounting is distinct from process RSS and original Python owner.
Initial defaults are1GiB native tracked bytes,256MiB encoded input and2^34 work
units, explicitly caller-overridable; tests force small limits. These are resource
limits, not a geometric tolerance or accepted performance claim.
The default admitted budget and
actual high-water usage are reported by the benchmark; no false NONE/no-ear
answer may conceal exhaustion. Exact bit width is not silently capped to64/128.

## Candidate indexes

Sort exact X and Y coordinate values once, assign equal values equal uint32
ranks, and retain source node slot as the deterministic tie breaker. Rank order
preserves closed-box overlap exactly; it is not a numeric approximation. More
than uint32-representable nodes/ranks returns RESOURCE_EXHAUSTED.

Use one owned packed median-split BVH representation with small leaves (initial
leaf size8), closed rank AABBs and uint32 object slots. Choose split axis by rank
extent; compare doubled rank-box centers in uint64, then original object slot.
Equal centers are safe. Build uses total deterministic comparison and median
partition; recursion depth is logarithmic, queries use explicit bounded stacks.
No coordinate conversion to double, open endpoints or epsilon expansion occurs.

Segment BVH objects retain flattened segment ordinal, original loop ordinal and
endpoint node slots. For each increasing `i`, query overlapping closed boxes;
ignore `j <= i` and the original same-loop node-sharing exclusion. Apply exact
segment-touch predicates to candidates and retain the smallest successful `j`.
Return immediately after finishing that `i`; traversal order cannot choose the
crossing. Candidate storage need not sort or materialize an all-pairs list.

For a hole-free validated simple loop, an exact all-cyclic-turns-positive test
admits a strict-convex fast path. Every successive cursor ear is empty in that
domain, so emit the identical linked-cursor triangle sequence without building
a point BVH or scanning blockers. Any zero/negative turn uses the generic path;
no weak/bridged polygon receives this shortcut.

The ear point BVH indexes the bridged polygon's occurrence slots, including
coincident bridge copies. For each ear attempt compute its closed rank AABB once
and visit only overlapping leaves. Reject dead occurrences, current three slots
and points equal to any ear corner; otherwise use the exact inclusive three-turn
predicate. Any blocker suffices, so query order need not reproduce the old first
blocker. Removal marks a slot dead and updates linked neighbors in constant time.
Keep the existing cursor, strict positive-turn condition, stall policy and final
triangle ordering. No reflex-only shortcut is assumed for bridged weak polygons.
An immutable tree can retain dead entries; statistics expose that cost before a
future dynamic-index design is selected.

Hole bridging initially uses indexed segment visibility against the current
outer loop and all remaining holes, rebuilding the segment BVH after each bridge.
It preserves existing shared-coordinate endpoint rules, exact on-segment tests,
original-loop midpoint material winding and current splice occurrence word.
Candidate bridge vertices are ranked by the exact distance above. Winding scans
remain linear; dense many-hole behavior is measured, not claimed subquadratic.
The BVH does not remove worst-case quadratic overlapping-box output.

## ABI and errors

New `native/face_domain` host-only C++20 library has its own target. Existing
CPU/CUDA C++17 targets and current pinned watch files remain unchanged. Public
C ABI uses fixed-width POD descriptors, opaque context pointer, caller-owned
input spans, copied output through checked capacity-query/copy functions and a
versioned statistics POD. Every exported descriptor/statistics POD carries ABI
version and byte size; mismatched/truncated or nonzero unknown reserved fields
are rejected before reading later fields. No STL type, borrowed native vector, exception or
allocator ownership crosses C. Create/run catch allocation and other exceptions;
contexts use RAII internally. Destroy is noexcept and null-safe. Native functions
are nodiscard internally; typed ID/slot wrappers prevent accidental interchange.

Statuses distinguish OK, INVALID_INPUT, CROSSING, INVALID_DOMAIN, NO_BRIDGE,
NO_POSITIVE_EAR, RESOURCE_EXHAUSTED and INTERNAL_ERROR. Boundary NONE is an OK
result kind, not a resource failure. Failed runs clear previous output; later
operations cannot retrieve stale success. Error text is bounded, diagnostics
contain offending source IDs where available. All lengths/products/offsets are
checked before indexing or allocation. The binding raises distinct exceptions
and checks exact source-token/node-ID round trips before handing output onward.

## Direct correctness and performance claims

Use Python Fraction as an independent arithmetic oracle, the unchanged Python
boundary/ear model for promised ordering, and `qualify_domain` for positive full
original boundary chains. For bounded fixtures also directly reject crossing
triangle interiors and verify each unconstrained edge has two opposite uses.
These latter checks attack embedding/source-occurrence defects beyond summary
counts. Original physical-shell/source/quality checks remain integration oracles.

External first cases: complete NVIDIA planar L/triangle/sampled-circle recipe;
Manifold SimpleHole, Eberly, Concave2 and AlignedHoles with original saved binary64
values and preserved attribution. Their native upstream counts/statuses are not
our geometric oracle. Unsupported/touching cases remain explicit rejections.
Authored cases cover non-dyadic exact planes, zero/cancellation, wide limbs,
closed-box touches, shared identities versus shared coordinates, collinear
samples, holes/bridge duplicates, all failure cleanup and context reuse.

Optimized fleet performance measures normalization+packing, native decode/rank/
index, crossing, bridges, ears and output separately, with end-to-end wall and
peak RSS. Actual counters include BVH node/leaf visits, candidate segment pairs,
exact touches/orientations, ear attempts/candidate occurrences/dead skips,
bridge candidates, maximum limbs and tracked scratch high-water bytes. Full
process RSS captures Python normalization/storage too. Native one-core timing
precedes concurrency. Reference geometry admission is separate from construction.

A sparse dyadic rectangular-strip perimeter family scales256..8192 boundary
nodes. Its closed segment boxes intersect only locally: at8192 require exact
segment candidates under `64*N`, rather than ~N^2/2 enumeration. Ear candidate
work is reported rather than falsely promised linear for this family. A strict-convex family checks the exact fast path, sequence and emitted-ear
count with zero blocker queries and no point-index build; a separate
concave dense-box family reveals broad-query behavior without that shortcut. A
many-hole family checks rebuilding/bridge costs. Each family has exact shape
construction and independent actual-output qualification.

The actual face167661 (8300 samples observed live) is the primary practical target.
A complete UV/occurrence snapshot has not yet been captured: reproduce and retain
it from unchanged source/schedules before that isolated trial.
Before any whole-watch retry it must produce a valid complete source-node domain
within a predeclared300s isolated run budget, under2GiB complete process peak RSS;
legacy baseline may be censored at300s, never converted into a numeric speedup.
If native still spends quadratic candidate work on sparse boxes, or cannot meet
the isolated budget, diagnose and return to the design before a full retry.
Three fresh optimized repetitions show all results/median/range where affordable.
No universal complexity, alpha readiness or whole-watch speedup is claimed here.
