# Native mesher implementation plan: values, exact tasks, runtime and output

Status: accepted in the whole-plan review after design `f7aedef`.
Implementation is underway; acceptance of the plan does not claim delivery.
This lane owns one source/value/numerical-task/runtime/result contract; geometry
and topology owners supply their algorithms against it. It does not deliver a
collection of separately usable ports in place of the whole mesher.

## Module and ownership boundary

New host code lives in `native/mesher/`, namespace `bgmesh`, built under
`tooling/native_cpu_rigor.cmake` as C++20. Optional sm_60 bound kernels live in
`native/mesher/cuda/`, built as CUDA/C++17 with a plain bounded-buffer ABI; no
host C++20 containers cross the plugin boundary. No Boost/GMP/task runtime is
adopted. The existing owned integer engine, direct-CDT implementation and display
kernels are reused through native interfaces, not binary serialization per face.

| Module | Owned responsibility |
| --- | --- |
| `values.hpp`, `source.{hpp,cpp}` | Typed IDs/slots/ranges; BGSRC001 reader; immutable source values and incident CSR |
| `exact.{hpp,cpp}` plus narrow `face_domain/exact.hpp` extensions | Owner-explicit arbitrary rational arithmetic, immutable exact words, conversions |
| `interval.{hpp,cpp}`, `bound_tasks.{hpp,cpp}` | Outward host primitives; exact task definitions; CPU filtering/fallback; common packing |
| `cuda/bound_api.h`, `bound_cuda.cu` | Optional owned-device sessions and bounded certified residual batches |
| `resources.{hpp,cpp}`, `scheduler.{hpp,cpp}` | Reservations, owner arenas, explicit continuations, completion and cancellation |
| `mesh_values.hpp`, `mesh_io.{hpp,cpp}` | Full semantic NativeMesh values and sectioned binary read/write |
| `scene_view.{hpp,cpp}`, `main.cpp` | Stored-XYZ BGMESH01/native scene view; whole-run CLI and atomic publication |

Geometry owns original extraction/restriction/chart/trace/profile algorithms;
topology owns boundary/face state machines, contacts and refinement. Their public
values use the shared types below. Neither introduces a second bigint, source
model or serialization. The unit implementation plans may split files, preserving
these conceptual owners.

## 1. Immutable typed source, including a strict existing-format reader

Use `SourceId<Tag>{uint64_t}` and `Slot<Tag>{uint32_t}` as distinct types for
Product, Definition, Usage, Vertex, Edge, Coedge, Loop, Face, Carrier, Curve and
Style. IDs preserve external values; dense slots are implementation indices.
Typed `Range<T>{uint64_t first,count}` addresses a run-owned immutable array.
`Point2Exact`, `Interval`, `Binary64Word` and approximate jets remain distinct
types. Binary64 source/output words are decoded by bit operations, never aliasing
casts; preserve original bytes including signed zero in the source snapshot.

`SourceStore` owns the original BGSRC001 bytes, source digest/revision and typed
arrays mirroring every current source row. Variable source-ID lists, strings,
knots/multiplicities, homogeneous controls, saved extracted spans, parameter-map
pairs, style targets and diagnostic targets use flat owned arrays/ranges. Read
only after `seal()`. `SourceView` is a const lifetime-borrowed view. Native geometry
never contains OCCT handles. Derived exact geometry belongs to GeometryStore,
not to mutated source rows.

Reader operations are `u32/u64/i32/f64_word`, `take_bytes`, `row`, `vector` and
`utf8`, each taking an explicit end limit. Read the 88-byte header and13 ordered
sections from the existing writer, including metadata, styles and diagnostics.
Validate magic/version/reserved fields, declared total length, each section and
nested-row length, exact row consumption and EOF. Check addition/multiplication
before allocation, count<=payload/minimum record size, <=UINT32_MAX typed slots,
and the caller's byte/count/work limits. Retain the current 1GiB source-wire cap
unless an explicit reviewed format-limit change is requested. A short/trailing
read, malformed UTF-8, noncanonical booleans or nonzero reserved fields is an
error, not an ignored extension. Preserve the existing section order; unknown
tags/map kinds are typed unsupported/invalid inputs.

After structural decode, build sorted ID→slot tables once and resolve all typed
references. Require unique IDs in each source table, exact face/loop/coedge owner
links, definition inventories, endpoint uses and correct dimensions. Validate
finite original words, positive homogeneous weights, strictly increasing knots,
matching multiplicities/control counts, supported surface degree0..3 and curve
degree0..32, natural Bezier ranges, sense/orientation flags and explicit parameter
maps. SameParameter=false cannot become an identity map. Reject source adapter
error diagnostics. Validate original rigid usage transforms under the existing
source admission tolerance, then traverse the product usage graph with explicit
gray/black states to reject cycles and count placements before allocating them.
Do not recursively duplicate definition geometry. Geometry admission separately
handles source discontinuities and unsupported NURBS periodicity/singularities;
parser validity is not geometric accuracy. Source IDs are positive uint64 as in
BGSRC001; zero remains an optional-reference sentinel. Preserve positive
`units_mm` as original source-unit metadata: snapshot spatial values from the
adapter are already millimetres and must not be multiplied again. The current
STEP adapter itself rejects non-mm source parameters pending an explicit source-
UV conversion map; this reader neither repeats that check on metadata nor claims
to implement the missing adapter conversion.

Produce definition→face/edge/vertex and physical edge→coedge/face CSR from resolved
slots, preserving original ordered occurrence lists. Do not collapse seam uses
or two endpoint occurrences simply because their IDs/XYZ match. Original pcurve
authority is `!reconstructed`; this policy and all original maps/lifts survive.
Do not infer period values for NURBS from a finite interval.

API seam:

```cpp
Result<SourceStore> read_source(std::span<const std::byte>, RunResource&, Limits);
Result<SourceStore> load_source(const Path&, RunResource&, Limits);
SourceView SourceStore::view() const noexcept;
```

Errors include section/row/field and source ID where known. No partially decoded
SourceStore is published. If a snapshot identifier is needed, reuse/rehome the
existing owned SHA-256 routine; distinguish original STEP digest stored in BGSRC
from a digest of the snapshot bytes. This is source identity, not a receipt layer.

## 2. Exact words do not own a mutable worker budget

Shared exact values are `ExactWords`: canonical signed numerator magnitude and
positive denominator magnitude, stored as little-endian base2^32 limbs in sealed
run-owned tables. Zero is0/1; numerator/denominator are coprime; no high zero limb.
`ExactRef{ExactTableKey table,uint64_t index}` names both storage and entry.
`ExactTableKey{OwnerKey owner,uint64_t generation,uint32_t ordinal}` is assigned
by the coordinator before that owner's work, independent of worker order.
`ExactView` borrows immutable limb spans from the corresponding sealed pool.
Run-owned ExactTableRegistry retains each sealed pool until its last result/task
reference is released. A local ExactBuilder seals before cross-owner transfer;
the owner opens a new builder rather than mutate a submitted table. Final output
walks required references in canonical record order and reindexes into one wire
exact table. No unqualified local index or shared mutable global builder exists.
Mesh `Point2Exact` sites are required dyadic; rational clipped bound subcells and
source coefficients may have general denominators. A dyadic site check inspects
the canonical denominator's single set bit; topology never substitutes rounded
UV for an exact rational restriction point.

JobContext owns a nonshared `bgfd::Budget`, stop token, owner/generation ID and
scratch resource. Mutable `Rational` owns two `bgfd::Integer` values allocated
from that context. No shared sealed root contains `Integer`, whose arithmetic
would mutate its captured Budget. `import(ExactView, JobContext&)` copies once
into a job's cached temporary table; `freeze(Rational, ExactBuilder&)` copies
canonical limbs into an owner/result builder before sealing/transferring it.
Never move a budget-affine integer into another owner's allocator.

Extend the existing owned Integer, without changing its current callers' meaning:

```cpp
static Integer from_words(Budget&, int sign, std::span<const uint32_t>);
Integer clone_into(Budget&) const;
uint64_t bit_length() const;       // checked; zero has length0
Integer shift_left(uint64_t) const;
Integer shift_right_exact(uint64_t) const;
static DivRem divmod_nonnegative(const Integer&, const Integer&);
static Integer gcd_nonnegative(const Integer&, const Integer&);
Integer divide_exact(const Integer& positive_divisor) const;
```

All allocating/arithmetic calls charge the destination owner's budget. Read-only
source views do not charge/mutate source state. Internal routines reject mixed
owners unless their API explicitly clones. `divmod` rejects zero divisor and
returns q,r with n=q*d+r,0<=r<d. Signed rational division controls signs outside
unsigned division and rejects a zero numerator divisor.

Extend Budget's constructor with an optional `std::pmr::memory_resource*`
upstream defaulting to its present new/delete resource. Existing local byte/work
limits remain intact. The upstream owner adapter consumes a RunResource
reservation when acquiring a block and retains that charge until the block is
released, even when an arena resets objects. Failed upstream allocation rolls
back its reservation and leaves local live-byte counters unchanged. This hook
is necessary: the current hardwired new/delete Budget cannot account for actual
aggregate concurrent storage merely by naming an adapter in the scheduler.

Implement normalized base2^32 multi-limb division with quotient estimation,
correction/subtract/addback and remainder unnormalization (Algorithm D), with
single-limb and exact-power-of-two paths. Use uint64_t intermediates with checked
boundaries; do not depend on undefined signed overflow. GCD uses single-limb
Euclid and leading-limb Lehmer quotient batches with bounded signed matrices;
only agreed quotient bounds enter a batch. Apply the integer unimodular matrix,
require nonnegative reduced operands and strict progress; ambiguous/overflowing
batches use a full exact division step. This reaches for batched gcd rather than
a repeated per-bit subtraction loop. Keep a full exact Euclid control in the
arithmetic fixture. The algorithm/limit switches are operand-size based, never
source IDs. [Primary division reference](https://gmplib.org/manual/Basecase-Division),
[primary Lehmer implementation discussion](https://gmplib.org/manual/Lehmer_0027s-Algorithm).
Reading these selects no GMP dependency or copied code.

Rational add/subtract use denominator gcd and cancel the new numerator against
the shared denominator factor. Multiply/divide cross-cancel before products;
comparison cancels common factors before exact cross-products and can filter
first. Normalize signs/zero immediately. Avoid a GCD after every term in a
bounded polynomial loop: a task-local homogeneous net can clear a common positive
scale once, preserving numerator and weight scaling together. This is bounded
per-net/task work, not an LCM across thousands of unrelated mesh sites. Geometry
roots record their exact local scale; no derived floating net becomes original
truth. Existing integer multiplication is the basecase; record operand widths
and limb-product work to determine whether a separate stronger multiplication
unit is warranted rather than asserting a cost solely from site count.

Initial CDT dyadic sites may share a power-of-two scale for its integer adapter;
choosing the maximum dyadic exponent does not create a general-rational global
LCM. Keep original site coordinates and exact T0-transformed dyadics associated.

Implement `from_binary64`, `round_binary64_even` and `enclose_binary64` using
integer bit lengths, shifted divmod and remainder comparisons. Handle normal,
subnormal, carry to the next exponent, overflow and ties by comparing2*r to the
positive divisor; no ambient FP rounding dependency. Canonical computed zero is
+0; original raw source zero words remain preserved separately. Finite output
overflow is a precision/resource failure. Exact rational enclosure returns
neighboring outward binary64 endpoints or an unrepresentable status. It must not
discard a negative tiny value when producing a subnormal enclosure.

## 3. Geometry task values and CPU decision implementation

GeometryStore publishes immutable exact original/extracted span roots. Geometry
owners mutate OwnerGeometryCache restriction nodes locally. Cache identity is
source/root, exact restriction/map, bound revision and boundary generation where
dependent; full keys decide identity. No callback/lambda is serialized in a map.

Task common header: `TaskKey{owner,cell_or_edge,generation,ordinal}`, source/root
slot, source revision, boundary generation, kind/bound revision and referenced
exact-input/actual-point/allowance ranges. A `RationalTriangleTask` stores exact
span-local triangle corners and its map to the original output facet. A clipped
subcell's L values are exact affine combinations of actual output XYZ, not newly
rounded virtual physical nodes. The source-span index/clipper supplies every
nonempty covering subcell. A `RationalCurveTask` stores original interval and
actual endpoint XYZ. The common decision uses R=N-WL and exact rational tau:

```text
wstar=min exact W coefficients; require wstar>0
ACCEPT iff every coefficient dot(R_i,R_i) <= tau²*wstar²
```

Equality accepts; FAIL means conservative bound failure, not infeasibility.
`BoundDecision` is ACCEPT/FAIL/UNRESOLVED/ERROR; root/cell identity always returns.
Host topology applies decisions in canonical order. Task interval endpoints never
set sites, XYZ, proposal priority, tie policy or transaction order. Exact source
fallback supplies any unresolved rational comparison under its owner budget.
An exact evaluator can early-stop at the first failing coefficient in canonical
order; it does not need a floating max-norm result to decide acceptance.

CPU interval implementation procedures, shared semantically with CUDA:

- Finite exact input words have outward lo,hi; exact representable binary64
  points have equal endpoints. Exact constants/multinomial ratios are enclosed
  through rational conversion, not an assumed exactly represented division.
- Under checked nearest binary64 with no excess precision/FTZ/DAZ, add/subtract
  the appropriate endpoint pair and step once with nextafter outward. Multiply
  encloses all four endpoint products; take the outward min/max. Square uses
  zero as the lower bound when the interval spans zero. Division is permitted
  only by an interval proved positive, evaluating quotient endpoint combinations
  outward. Exact-zero short paths are derived identities, not flushed values.
- Any unproved environment, NaN/indeterminate infinity, unrepresentable enclosure
  or unsafe denominator returns UNRESOLVED. Propagate an explicit status, never
  compare NaN and accidentally accept. Correctly outward subnormal intervals
  remain valid. No shared FP-environment mutation; FP guards run on each worker.
- Restrict the exact source expression using enclosed de Casteljau coefficients;
  elevate N and multiply W by affine L with the actual Bernstein convolution
  factors. Triangle tensor-to-simplex conversion composes affine barycentric
  coordinate nets and uses bounded degree convolution/elevation. Propagate the
  original extraction enclosure and anchored coordinates through every step.
- Filter each squared coefficient comparison. A negative/zero upper difference
  proves acceptance for that coefficient, a positive lower difference proves
  failure, and overlap requests exact fallback. An uncertain positive wstar is
  resolved exactly before any division/acceptance. Coefficients proved pass by
  intervals do not require another exact pass merely to certify them.

Ordinary tensor degree(p,q)<=3 gives residual total degree<=7 and36 triangular
coefficients; curve degree<=32 gives residual degree<=33 and34 coefficients.
Curve elevation/product has the explicit degree n+1 coefficient rule using
i/(n+1) and(n+1-i)/(n+1), which the exact and interval paths evaluate under the
same representation. The geometry unit specifies original extraction and clipped
cell maps; this unit implements the bounded residual arithmetic, not a generic
unbounded symbolic engine.

Analytic bound decisions initially remain host geometry procedures. A named
implemented analytic bound may supply canonical certified rational constants
and an exact rational comparison to the task system; unsupported expressions
do not turn into approximate GPU acceptance. Source-trace mismatch, monotone
profiles, plane charts and pole maps retain their geometry-owner procedures and
typed tasks. This initial GPU family does not claim to solve those algorithms.

Public native header seam (owned vectors/ranges and move-only leases, not raw
borrowed temporaries):

```cpp
enum class BoundKind { rational_triangle, rational_curve };
enum class Decision { accept, bound_fail, unresolved };
struct TaskKey { OwnerKey owner; uint64_t cell, generation, ordinal; };
struct TriangleResidualInput {
  RootKey root; Point2Exact subcell[3];
  ExactRef L_at_subcell_vertices[3][3]; ExactRef tau;
};
struct CurveResidualInput {
  RootKey root; ExactRef t[2]; Binary64Word xyz[2][3]; ExactRef tau;
};
struct TaskHeader {
  TaskKey key; BoundKind kind; uint32_t bound_revision;
  uint64_t source_generation, boundary_generation, input_slot;
};
struct BoundAnswer { TaskKey key; Decision decision; };
// TaskBatch owns typed headers/payload arrays plus sealed ExactPool leases.
Result<BoundAnswers> filter_cpu(TaskBatch const&, ChildJobContext&);
Result<BoundAnswers> resolve_exact(TaskBatch const&, ChildJobContext&);
Result<BoundTicket> submit_device(TaskBatch&&, DeviceSession&, ChildLease&&);
Completion poll_device(BoundTicket&);  // not-ready, complete, or typed error
Result<BoundAnswers> collect_device(BoundTicket&&);
```

RootKey includes its immutable GeometryStore/root identity and source revision.
Triangle L values are exact affine images of the *original actual facet* and
have their output-node association retained in the cell map. A batch contains
only sealed references; lease ownership survives source/owner yields. Device
unresolved answers route only their matching tasks to `resolve_exact` in a new
child context. Native Result uses an owned `std::variant<T,NativeError>`; no
C++23 expected requirement or exceptions cross the C plugin ABI. Integer engine
exceptions are translated at task boundaries with stage/task/source context.

## 4. Actual P100 packing, kernels and fallback

`bound_api.h` contains only explicit fixed-width record layouts, pointer/count
buffers, ABI/version/size and status/error fields. APIs are create/upload_roots,
submit_batch, poll/wait_completion, collect and close. CPU-only never dlopens a
CUDA module. Host C++20 owns exact arrays; device views contain only their proved
interval images, actual binary64 words and exact-value identity references.
Every ABI entry catches exceptions, selects/restores its session's device and
returns initialized status/error records. Session/ticket tokens publish only
after successful initialization/device restoration. Close consumes its token
exactly once and drains owned events/stream; cleanup failures are reported.
Asynchronous submission transfers staging ownership until its ticket completes,
not merely until the submit function returns. Record sizes/alignment are checked
by host/plugin ABI-size probes; wire record layout is separately field-encoded.

Bucket by triangle(p,q)0..3 and curve n0..32. Use an initial128-thread block,
four warps, one task per warp. Coefficient lanes are contiguous;36 triangle or34
curve output coefficients use at most two lanes-per-coefficient rounds. Shared
memory is explicitly bounded from bucket degree: interval homogeneous nets have
four channels and separate lo/hi arrays; two scratch banks avoid per-thread
arrays spilling an entire net into local memory. Threads cooperate through
warp/block-safe indexed stages. Inactive lanes contribute neutral bounds;
all synchronization points are uniform. No warp-specific code assumes a partial
block's active mask is full. Packing widths determine bounded shared bytes before
launch; query device/kernel limits and reject invalid configurations.

Restrictions and convolution use fixed degree-bounded loops, no device malloc,
recursion, arbitrary integers or topology writes. Directed `__dadd_rd/ru`,
`__dsub_rd/ru`, `__dmul_rd/ru` and positive `__ddiv_rd/ru` implement endpoint
arithmetic; directed operations are not replaced by fused contractions. Squared
comparison needs no sqrt. Explicit exceptional/status lanes return unresolved.
The build targets60-real, CUDA12.8 strict division/sqrt, no fast math/FTZ. Store
operation work estimates per bucket and observed unresolved counts; kernel launch
count/packing is bounded by task batches rather than per-coefficient callbacks.

Immutable root interval images remain resident for the run. Each session owns
one nonblocking stream initially and a bounded pair of pinned input/output
staging slots with associated device buffers/events. Independent host work can
run while that session executes; adding streams is not necessary for the first
correct bounded implementation. A native completion pump queries events and
makes owner continuations ready; it performs no topology mutation. Multi-P100
uses independent sessions/task partitions and separate reservation accounting.
No device-wide synchronization is required for another owner's unrelated work.

GPU returns proved ACCEPT/FAIL or UNRESOLVED with task/generation. The host queues
only unresolved tasks for the same exact fallback used by CPU. Host decision,
site/proposal/XYZ and ordering rules guarantee identical successful NativeMesh
geometry across thread counts/backends; differing timings/fallback counters and
approximate display jets are not part of that byte-equality claim. A CUDA error
poisons the session; typed failure is default. Explicit `--gpu-fallback=cpu` may
drain/discard its batch and requeue identical tasks on CPU, reporting that route;
it must not silently retain the label of a completed accelerated run.

## 5. Bounded continuations, reservations and failure

RunResource is the only parent of mutable owner resources and output storage.
It charges actual reserved blocks, including idle retained arena blocks, pinned
buffers and all devices. A per-owner Budget requests blocks through an adapter
to that parent; it remains single-thread-owned. Exact arithmetic also charges
limb operations/division/gcd work and checks stop at bounded loops. No unbounded
global cache allocation bypasses accounting.

Scheduler task types are SourcePrepare, GeometryPrepare, EdgeOwnerStep,
FaceOwnerStep, BoundBatch, ExactFallback, DefinitionContactCommit and Publish.
An owner step runs until a bounded batch/request boundary, saves explicit state,
then returns READY/WAIT_CHILD/WAIT_BARRIER/DONE/FAILED. An owned continuation
stores its phase, generation, input/result buffers and pending-child count.
The coordinator alone changes continuation readiness and generation barriers;
workers never block on queued child futures. Stealing moves a ready owner step
between workers, not concurrent access to that owner's arena/Budget. A resource
lease moves with the owner step. No lock is held across a yield.

BoundBatch CPU children and ExactFallback children each own a separate child
JobContext/Budget and reserved resource lease. They import immutable ExactRefs;
they never borrow the suspended parent's mutable Rational cache or Budget.
Their immutable decision records transfer back on completion. Multiple children
can therefore execute without a Budget race. Child reservation includes its
configured maximum exact-limb scratch and result space before admission. A
fallback that cannot fit its declared child limit reports resource exhaustion;
it does not wait for extra bytes held by its parent/staging slot.

At admission reserve parent scratch plus its permitted outstanding child batch
and completion capacity. If unavailable, keep that parent unstarted. Reuse
bounded batch slots; do not enqueue unbounded children. An owner needing growth
must reserve before mutation or release recomputable cache blocks; if even one
unit cannot fit, report resource exhaustion. Retained result buffers remain
charged until transferred to the final result. A coordinator/completion reserve
cannot be consumed by suspended parents. Thus neither all worker slots nor all
bytes can be held by parents waiting for children. Unit plans specify actual
size formulas from degree/site counts, not unexplained magic capacities.

Concrete admission accounting is
`owner_arena_cap + outstanding_batch_cap*(packet_bytes+answer_bytes+
child_exact_byte_cap) + completion_record_bytes`, with every multiplication
checked before reserving. Packet bytes come from declared task/payload counts
and ABI record widths; child exact byte cap is a hard Budget limit, not an
unbounded promise to wait for future allocation. Exact fallback processes one
task at a time with bounded root-import cache and reuses its scratch. A task
whose arithmetic exceeds that cap fails explicitly. Device shared storage for
each warp is two banks*four channels*C*16 interval bytes, where C<=36 for a
triangle and C<=34 for a curve, plus fixed reduced-status storage. Four warps
therefore need at most18432 bank bytes per block plus the explicitly declared
status scratch. Staging/device buffers are reserved at exact packed capacity,
not estimated from output triangle counts.

Per-face failed-cell frontiers are ordered by exact fixed-M0 diameter and stable
cell key only. Failure reason is state, never a post-result priority change.
Wait for a higher-priority pending decision; do not refine a lower ready failure
first because its CPU/GPU batch finished earlier. Edge owners merge exact dyadic
split requests at a complete per-definition boundary epoch. From one snapshot,
every active current-generation face reaches its canonical first-request yield
(a deterministic bounded request set) or local-bound-settled horizon. Keep all
participants until ALL those current-generation results are available; a hard
current-generation failure fails the run. Only then merge the full request set
by source/leaf order and apply the schedule update. Do not apply an arrived
prefix that cancels slower requests. Retain unaffected completed results; changed
incident face generations restart and obsolete task results are discarded.
DefinitionContactOwner is the sole writer of a definition's actual-facet BVH and
pair state. Once all faces settle local bounds on the current snapshot, freeze
their facet tables for a contact epoch. Apply bounded FacetDeltas, query/classify
all candidate pairs (read-only classification can batch), then fold the complete
results in stable pair order and mark BOTH offending cells. Only after this full
fold do faces refine frozen marks and settle bounds again. Do not select a
completion-dependent prefix of contacts. Boundary requests end affected face
generations at the coordinator barrier. Post-settlement optimization has one
stable global(face,edge) queue and rechecks current actual-facet revisions/
contacts at each ordered commit. Independently safe parallel proposals cannot
become intersecting accepted flips together. Face workers do not approve contacts
against a stale shared BVH. This is the topology plan's selected epoch protocol.

`NativeError` codes distinguish invalid input, unsupported, unresolved geometry,
precision floor, resource exhaustion, cancellation, device error, I/O and
internal error; retain stage/source/task/generation plus counters. All worker
exceptions become FAILED results and decrement/complete dependencies. On stop:
stop submissions, invalidate descendants, drain running tasks/owned streams,
consume completions, then reclaim inputs/arenas. No source view dies before its
last task. Preserve the prior published immutable result. Successful geometry
is deterministic; first racing cancellation/resource error is not promised
deterministic unless the coordinator explicitly orders it.

## 6. NativeMesh values and full binary schema

Source/geometry/trace policy must be recoverable from output, without retaining
worker pointers or private code closures. The semantic shared records are:

| Table | Required fields/relations |
| --- | --- |
| Definitions | Original definition slot/ID, complete face/edge/vertex inventory references, physical/site/facet ranges |
| PhysicalNodes | Definition, canonical XYZ words, generator tag and original vertex/edge+dyadic parameter or face+site/cell lineage, quotient where applicable |
| Sites | Exact dyadic working-chart coordinate pair, chart/face/branch, PhysicalNode reference, stable source/generation/lineage identity |
| EdgeChains | Original edge+definition, ordered source-parameter/sample refs and shared canonical physical nodes; original endpoints retained |
| Occurrences | Original coedge/loop/face, sense/map/lifts, raw pcurve parameter/UV semantics, selected working sites, original-UV recovery descriptor, discrepancy/strip/map associations |
| Charts/Profiles | Original-UV identity; admitted physical-plane source projection+preconditioner+original rectangle+convex quadrilateral/inverse descriptor; selected geometric-line/graph correspondence; pole/apex quotient descriptor |
| Cells | Face/chart, finite exact domain and source map tag, original carrier/root association, actual physical interpolant/map, lineage and facet-association range |
| Facets/Corners | Nonzero actual triangle PhysicalNode refs, original face/material, per-corner original/recovered UV and carrier/orientation/branch refs |
| Quotients/Closures | Original pole/apex/seam/joint source IDs, occurrence multiplicity, working-domain collapse/closure maps and shared physical identity |
| Quality/Components | Requested positional tolerance and requested NODE source-residual policy, canonical exact local allowances/profile constants and named approximation policies; backend filter endpoints/maxima belong in TSV, not geometry bytes |
| Assembly/Appearance | Original usage instances/rigid transforms, source styles and separate explosion/label presentation values |

Map tags are finite enumerations with normative equations, not arbitrary bytecode:
`UVTriangle`, `PlaneXYTriangle`, `AnalyticPoleFanQuad`; chart tags `OriginalUV`,
`AnalyticUnwrappedUV`, `PhysicalPlaneConvexQ`; correspondence tags
`AffineParameter`, `GeometricLineScalar`, `CylinderArcOrder`. Each refers to
exact source/parameter/interval values and stored coefficients/axes, original
period lifts and its coverage range. Unsupported tags are rejected. Pole/fan
Cells retain their full source rectangle/sector and map→facet relation;
no1:1 UVtri/XYZtri assumption.
Clipped span subcells used only to evaluate a bound need not become mesh sites or
an added certificate artifact; the independent reader can reconstruct them from
the original source and stored actual cell map.

Choose wire magic `BGNMESH1`, version1, flags0. The96-byte header is magic8,
version/flags u32, file length u64, section count/reserved u32, original source
digest32 and source snapshot digest32. It is followed by a fixed ordered list
of sections. Each section header is tag8,count u64,payload_bytes u64,
record_width u32,flags u32. Fixed-width rows have exact declared widths;
variable rows use u64 row byte length then fields. Offsets/counts are explicit
u64, flags/tags u32, IDs u64, source/output scalar words u64, limbs u32; no native
padding. All reserved fields are zero. Parser validates whole section arithmetic,
exact consumption, ranges/ownership and supported tags before constructing a
sealed NativeMesh.

The exact ordered section/row schema below is selected for version1. Every row
has an implicit1-based row ID;0 is an absent optional reference. Original entity
IDs are explicitly labeled `source_*`. `id`/table references and counts are u64;
tags, flags and reserved fields are u32; orientations/lifts are i32; `word` is
an exact binary64 bit word u64; `e` is a1-based wire exact-table reference. All
unmentioned flags/reserved fields are zero. `List<T>` is count u64 followed by
exactly that many encoded T values; tuple elements have no alignment padding.
Variable rows use the section's record_width0 and outer u64 byte length; fixed
rows declare the field-derived width. A nested variant has tag u32, reserved0
u32 and its enumerated payload. Each table's ordinal order is canonical source/
owner/key order, never builder/table address or task completion.

| Tag | Row fields in order |
| --- | --- |
| `SOURCE__` | One variable row: byte_count u64, exact original BGSRC001 bytes |
| `EXACT___` | numerator sign i32, numerator_count u32, numerator_limb_offset u64, denominator_count u32, reserved u32, denominator_limb_offset u64 |
| `LIMBS___` | One fixed u32 limb per row; canonical exact descriptors reference this array |
| `DEFS____` | source_definition, List<face_ref>, List<node_ref>, List<site_ref>, List<facet_ref> |
| `FACES___` | source_face, definition_ref, chart_ref, domainmap_ref(0 for reconstructed identity), final_generation, List<loop_ref>, List<cell_ref>, List<facet_ref> |
| `NODES___` | definition_ref, XYZ word[3], generator variant, quotient_ref |
| `SITES___` | face_ref, chart_ref, exact_xy e[2], node_ref, branch variant(None/Use/Joint plus ref where present), lineage_ref |
| `LINEAGE_` | face_ref, generation, operation tag, List<parent_lineage_ref>, List<parent_site_ref>; initial/circumcenter/midpoint/optimization tags name actual ancestry |
| `EDGES___` | source_edge, definition_ref, final_generation, List<edge_sample_ref>, List<component_ref> |
| `EDGSAM__` | edge_ref, original_t e, node_ref, interval-tree path List<u32>(0/1) |
| `JOINTS__` | face_ref, source_vertex, chart_ref, branch ordinal u64, site_ref, List<use_ref>, original_uv_locator variant, quotient_ref |
| `USES____` | source_coedge, source_loop, face_ref, edge_ref, sense i32, reconstructed u32, original_parameter_map_kind u32, reserved u32, period_lift i32[2], List<use_sample_ref>, List<profile_ref>, List<component_ref> |
| `USESAM__` | use_ref, edge_sample_ref, site_ref, original_pcurve_parameter e(0 if unavailable), raw_pcurve_evaluation locator variant, recovery locator variant |
| `LOOPS___` | source_loop, face_ref, outer u32, traversal_adapter i32(+1/-1), List<use_ref> in original source order, List<site_ref> in normalized polygon order, List<closure_ref> |
| `CHARTS__` | source_carrier, chart variant (payload below) |
| `DOMMAPS_` | face_ref, reference_policy u32(AuthoritativeOriginal/HybridPullback), reserved u32, ordered List<nodal_fan_support_ref>, ordered List<graph_strip_support_ref>; apply all B fans, then all T strips |
| `SUPPORTS` | face_ref, support variant (payload below) |
| `PROFILES` | face_ref, optional_use_ref, source_curve, source_carrier, original_interval e[2], correspondence variant, List<component_ref> |
| `CELLS___` | face_ref, chart_ref, lineage_ref, map_ref, List<cellfacet_ref>, List<component_ref> |
| `CELLMAPS` | cell_ref, map variant (payload below) |
| `CELLFAC_` | cell_ref, facet_ref, interpolant variant (payload below) |
| `FACETS__` | face_ref, physical_node_ref[3], corner_ref[3], appearance_ref |
| `CORNERS_` | face_ref, site_ref, source_carrier, orientation i32, reserved u32, original_uv_locator variant |
| `QUOTIENT` | face_ref, source_carrier, quotient tag, List<source_vertex>, List<use_ref>, List<site_ref>, canonical_node_ref, List<cell_ref>, pole_descriptor variant |
| `CLOSURES` | face_ref, source_loop, source_vertex, incoming_use_ref, outgoing_use_ref, endpoint_site_ref[2], closure tag, reserved u32, quotient_ref, List<component_ref> |
| `COMPONTS` | owner variant(Definition/Face/Edge/Use/Cell plus ref), component tag u32, value_role u32(Allowance/CanonicalUpper), exact_value e, profile_ref; named nodal and whole-interval positional meanings are distinct |
| `QUALITY_` | One row: requested_positional_tolerance word, requested_NODE_source_residual word, geometry_policy_revision u32, reserved u32; no backend/timing fields |
| `INSTS___` | source_usage_path List<u64>, source_product, definition_ref, rigid_matrix word[12], explosion_offset word[3], appearance_ref |
| `APPEAR__` | source_style, rgb word[3], roughness word, label_source_product; source names/material details remain embedded |

Generator variants: `OriginalVertex(source_vertex)`;
`OriginalEdge(edge_ref,original_t e)`;
`FaceSite(face_ref,site_ref,lineage_ref)`;
`DeclaredQuotient(quotient_ref)`. Node→site→node cycles are semantic references
with checked ownership, not a recursive decoder/allocation requirement.

Loop normalization preserves original ordered use refs and original USES.sense.
For nonzero exact area a of the chosen chart-site cycle in original occurrence
order, desired orientation d is+1 outer/-1
hole, and traversal_adapter=d*sign(a). The normalized site cycle is original
or reversed with its first joint fixed. Its normalized segment's master
direction is original_use.sense*traversal_adapter; do not rewrite source sense
or reverse the original use list to conceal the adapter.

Chart variants: `OriginalUV(original_rect e[4])`;
`AnalyticUnwrappedUV(original_rect e[4],period_lift i32[2])`;
`PhysicalPlaneConvexQ(original_rect e[4],projection e[6],preconditioner e[4],
plane_origin e[3],plane_axes e[6],Q_corners e[8])`. The plane source map is the
inverse of that named source projection on Q, not an approximate UV lookup.
Its rectangle and convex-image proof are construction procedures, not a stored
boolean accepted as geometry truth.

Correspondence variants: `AffineParameter` (the row's original interval defines
theta=(t-a)/(b-a)); `GeometricLineScalar(covector e[3],p_a e,p_b e,node_a,node_b,
traversal i32,reserved u32)`, with sigma=(p(t)-p_a)/(p_b-p_a) and the retained
original curve defining p(t); `CylinderArcOrder(dual_frame e[12],orientation i32,
period_side i32,endpoint_ray_enclosures word[8],halfplane_anchor e[2])`. An
implemented checked profile carries source/endpoint coverage and residual
associations; an arbitrary expression/atan2 root is not a new profile tag.

Locator variants: `DirectOriginalUV(chosen_uv word[2],enclosure word[4])`;
`OriginalProjectionInverse(chosen_uv word[2],enclosure word[4],chart_ref)`;
`AnalyticBranch(chosen_uv word[2],enclosure word[4],lift i32[2],quotient_ref)`.
These are consumer recovery/evaluation descriptors, never exact topology roots.
Finite interval endpoints must be ordered; an undefined guide has an explicit
`UnavailableGuide(reason_tag)` and cannot supply a required display normal.

Authoritative domain maps are actual source-to-selected-polygon semantics, not
proof subdivisions. A reconstructed face uses domainmap_ref0 for its declared
identity realization. An authoritative face references `DOMMAPS_`; identity
supports may have empty lists. The source→polygon composition applies all
ordered disjoint nodal fan maps B, then the ordered pairwise-disjoint graph
strip maps T. No backend accepted flags, interval subdivision tree or cached
bound expressions are serialized. The geometry unit constructs/checks these
maps; fields below make the actual map reproducible from retained source values.

Reference policy tag1 `AuthoritativeOriginal` applies when every source use of
the face is authoritative: original pcurve arcs define its reference boundary.
Tag2 `HybridPullback` applies to a mixed face. Keep original authoritative arcs;
each reconstructed reference arc is B^-1 of that use's stored selected polygon,
with B the same face's full nodal map. At an authoritative incident joint its
original endpoint A is the reference joint; Q is the common chosen dyadic joint.
Two incident authoritative endpoints must agree before that joint is admitted.
On a nodal sector, the finite inverse map is

    B^-1(y)=y+orient(P_i,P_next,y)/orient(P_i,P_next,Q)*(A-Q).

Apply inverse B fans in reverse order. Then B sends reconstructed pullbacks
exactly to their selected polygons, while T acts only on transformed authoritative
arcs and excludes reconstructed polygons except fixed common Q endpoints. The
per-use reconstructed flags, stored polygon sites/profiles and these B map refs
fully define the hybrid reference; no extra callback/map expression exists.
Domain displacement charges the ordered B/T movements on all affected cells,
including reconstructed pullbacks. Original spatial-curve/reference discrepancy
also includes inverse-B movement; the policy does not grant a free tolerance.
An all-reconstructed face has domainmap_ref0 and its selected polygon is the
reference, not an implicit hybrid map. Decode rejects a reference-policy/per-use
authority mismatch or missing required support/source references.

`NodalFan` support payload is
`source_point variant,chosen_Q_site_ref,convex_P List<exact_point2>`.
`exact_point2` is e[2]. Source-point variants are
`OriginalPcurveAtParameter(use_ref,pcurve_s e)` and
`ProjectedPcurveAtParameter(chart_ref,use_ref,pcurve_s e)`. Here s is the exact
mapped original pcurve parameter, not original edge t unless the map is identity.
A includes that use's retained chart branch/integer period lift after original
pcurve evaluation; it is not raw unlifted pcUV. The projected variant evaluates
the named chart's original source projection at that source point. Do not apply
the t→s map again when the descriptor already supplies s.
A is the retained original pcurve/source projection value; it is not replaced
by rounded rational coordinates. In each fan sector(P_i,P_next), the declared
map is exactly

    B(x)=x+orient(P_i,P_next,x)/orient(P_i,P_next,A)*(Q-A).

It fixes P's boundary and is identity outside P. P is a rational convex polygon;
Q is the chosen dyadic chart site; analytic A is evaluated from its finite source
descriptor. Original source/sector semantics determine coefficients even when
A is transcendental. Actual support admission establishes positive denominators
and the common orientation; no boolean claiming that fact substitutes for data.

`GraphStrip` support payload is
`original_use_ref,original_edge_t_interval e[2],ordered pre_nodal_fan_refs,
chosen_endpoint_site_ref[2],coordinate_origin e[2],x_covector e[2],
y_covector e[2],x_extent e,c e,w0 e`.
Each pre_nodal_fan_ref names an earlier B support from the same face's DOMMAPS
list, in that order, including every fan that can affect the source arc.
The original use/map sends original t to its source pcurve point; the chart
projection, followed by those B maps, defines the transformed arc. Under the
stored invertible exact coordinate frame, that arc has the proved monotone
inverse defining f(x). Its chosen endpoint sites define chord g(x); their
coordinate endpoints must be x=0 and x=X=x_extent>0. The support declares

    w(x)=min(c*x,c*(X-x),w0),
    fiber knots (g-w,f,g+w) -> (g-w,g,g+w).

The map is affine between successive knots on each fiber, identity outside the
tapered region and at x endpoints. c,w0 are positive exact values. The source
arc, its monotone inverse, chord and finite frame/envelopes fully define T;
no arbitrary expression callback/bytecode or exact inverse-root topology site
is needed. `DOMMAPS_` supplies the full B-then-T composition used by the geometry
unit's domain-displacement rule. All support/source/site/face references are
checked for matching ownership, use interval and application order.

Map variants: `UVTriangle(site_ref[3])`; `PlaneXYTriangle(site_ref[3],chart_ref)`;
`AnalyticPoleFanQuad(quotient_ref,pole_descriptor,angular_limits e[2],
edge_node_ref[2],edge_uv e[4],cap_cut_chain List<site_ref>,xi_range e[2],
rho_range e[2],face_generation)`. Pole descriptor tags are `SphereNorth`,
`SphereSouth`, `ConeApex`; their source singular constants are respectively
+pi/2,-pi/2,-r/sin(alpha), not falsely serialized as exact rational constants.
Non-pole seam/joint quotient records use `NoPole` with no payload; a pole fan
cannot use that tag. All other source references come from its containing
quotient/chart record rather than an implicit global carrier.
The fan map is u=uL+xi*(uR-uL), v=v_pole+rho*(g(u)-v_pole), with g the stored
outer edge-UV interpolation. Initial admission requires the implemented adjacent
constant-u meridian construction. Other polar stars are unresolved. Store the
finite-source-pole-band discrepancy as its canonical component, not a claim
that a finite binary64 latitude equals exact pi/2.

Interpolant variants: `BarycentricTriangle` maps the cell's canonical barycentric
coordinates to the physical-node order of its `UVTriangle`/`PlaneXYTriangle`
site list; match those keys to the referenced facet's node set and reconstruct
the permutation. Facet row order may reverse for negative source face
orientation and must never change this source-domain interpolant.
`PoleBilinearFan` uses
Phi=(1-rho)P_pole+rho*((1-xi)P_a+xi*P_b) with node order specified by the pole map.
The pole role is the quotient's canonical node; A/B are the stored ordered
edge-node refs. Verify the facet has exactly those keys and derive its ordering
permutation by key matching, never assume pole/edge roles equal facet slots.
For pole-map source Jacobian sign sJ=sign(delta_u*(g-v_pole)), the role triangle
(Ppole,Qj,Qj1) has the opposite normal to Phi_(xi,rho). Retain those facet roles
iff -face.orientation*sJ=+1; otherwise swap the cut roles in the actual facet.
Store the role-to-facet permutation through physical keys. A blind face-only
flip is not the pole orientation rule.
This is enough to reconstruct actual physical interpolation without a UVtri:
XYZtri one-to-one assumption. Unknown variants fail, never silently default.

Canonical component values are host-defined exact allowances or deterministic
source/profile constants. CPU/P100-dependent interval upper endpoints, fallback
routes and per-batch maxima are diagnostic TSV observations, not NativeMesh
fields. Do not recompute every successful GPU bound on the host merely to create
another receipt. Source bytes are embedded once, preserving names/maps/styles/
diagnostics without an alternative source authority.

The existing request `source_residual_mm` means a **node** recovery/correspondence
cap, now named requested_NODE_source_residual in QUALITY_. Preserve its check at
original vertex/exact original Ce samples against original S evaluated at the
**stored chosen original-UV recovery dyadic**, and preserve original V versus
exact Ce at the endpoint-to-curve node check. This is not a comparison to actual
rounded emitted Q, and a physical-plane chart's implicit exact inverse Psi
cannot stand in for the stored recovery UV. It is not a whole-curve/polytrace approximation
budget or a promise of whole-curve closest-carrier distance below that cap.
Transport the node comparison by the source-only instance norm kappa, without
charging output matrix rounding twice. Whole physical curve/chord and original
Ce-to-selected/reference trace approximations use positional epsilon_def; the
face domain-displacement plus actual-cell sum also uses epsilon_def. A planar
circular chord at positional0.005 must not be forced to0.001 sagitta merely
because the requested node residual is0.001. This keeps the request meanings
while leaving whole positional accuracy unchanged.

Selected COMPONTS tags1..9 are, in order: `DefinitionPositional`,
`DefinitionNodeSource`, `NodeCarrierResidual`, `NodeEndpointCurveResidual`,
`TracePositionResidual`, `ChordPositionResidual`, `DomainDisplacement`,
`ActualCellPosition`, `PoleBandPosition`. value_role1 is the corresponding exact
allowance; value_role2 is a required host-canonical upper bound where actually
stored. NodeCarrierResidual associates with its use's sample set, endpoint-curve
with its edge, trace/chord with use/edge interval/profile, and domain/actual-cell
with its face/cell. Keep nodal checks and whole-interval positional checks as
separate tagged comparisons; one cannot silently consume the other's cap.

Serialization freezes owner results into canonical source/key order, creates
prefix ranges and reindexes references once. Exact rational descriptors use
offset/count/sign for numerator and offset/count for positive denominator in the
limb section. No pointer/budget identity appears on disk. Independent research
qualification adapts the existing Python reader to actual NativeMesh semantics;
production uses direct constructor invariants/bounds and complete reference
checks, not a second native certification framework.

Serialize ONLY exact values reachable from the semantic mesh records, traversed
in canonical record/field order. Never dump worker ExactBuilder pools, caches or
all registered exact tables: CPU/P100 filters/fallbacks can create different
temporaries even for identical geometry. Internal pool identity is not a
semantic ID/priority. Source bytes, chosen cells/maps and required source/profile
constants determine wire reachability and canonical reindexing; unrelated
temporary coefficient rows cannot perturb output bytes.

## 7. Stored-XYZ display view and CLI publication

`make_scene(NativeMesh, AssemblyView, AppearanceView)` builds owned SceneStore
arrays and SceneView borrowing them. Always set BGMESH01 Vertex flag1 so
`Jet.used` is the stored canonical XYZ for **every** vertex, including interior
sites. Source evaluation may supply normals; it must not replace physical-plane
chart/pole/canonical XYZ with an approximate UV reevaluation. Retain corner
duplicates for UV/normal/face occurrences; share the same physical key. Source
span choice uses the geometry span index, not a linear scan per triangle.
NativeMesh FACETS are already in the topology owner's final physical orientation;
copy their row order to the display view, without a second face-orientation
swap. The Vertex face-reversal flag controls the source normal exactly once,
independently of the stored cell-role/facet-row permutation.

Existing analytic kernels support declared pole/apex normal limits; regular
rational corners use original/recovered UV and compiled source span views.
An invalid/singular required normal is a typed display failure, not dropped
geometry. A declared alternative faceted-normal policy needs an explicit format/
consumer extension, not a hidden source carrier change. Full NativeMesh keeps
original authority; a display span's enclosed/rounded controls are derived.
Display vertices/facets remain definition-local; instances reference them.

Extend the actual display consumer explicitly; the current strict scene parser
and radius-sign heuristic do not already satisfy this contract. Keep the wire
record sizes/magic/version, and select known header feature flag1
`SOURCE_GEOMETRY_PROFILE`. Header flags0 retains legacy strict axis/instance
validation and old normal behavior. The new profile retains finite source axes
without an orthonormalization rewrite, source-admitted approximately rigid/
once-rounded positive-determinant instance matrices, and the all-stored-XYZ
requirement. Validate original carrier/radius/chart prerequisites and require
finite nonzero evaluated/declared limiting normals at actual display corners;
do not silently replace a failed carrier with an orthogonal synthetic one.

For new-profile vertices use these named flag values in shared CPU/CUDA code:

| Vertex value | Meaning |
| --- | --- |
|1|Stored canonical XYZ, required for every new-profile vertex|
|2|Original face orientation reversal, applied once|
|4|Declared analytic one-sided normal limit, cone/sphere only|
|8|Negative radial regular sheet, valid only with4|
|16|Sphere south pole, valid only with4 and sphere carrier|
|32|Transform normal by actual instance cofactor, required in new profile|

Decode rejects unknown/invalid combinations. make_scene obtains limit and sheet
from the admitted quotient/corner branch, never from the sign of a tiny radius
recomputed at rounded singular UV. Sphere limiting normal is
`north:+cross(X,Y), south:-cross(X,Y)`, multiplied by the declared sign(cos v)
of its regular sheet. Cone limiting normal is
`sin(alpha)*cross(Eprime,E)+cos(alpha)*cross(Eprime,Z)`, multiplied by declared
sign(rho), rho=r0+v*sin(alpha), on the regular side; this is not sign(r0).
These are actual source derivative-cross limits for nonorthogonal axes. Scale
frame vectors by a common positive factor before cross-products when needed,
preserving relative terms. A zero/unrepresentable result is a display precision
failure. Approximate source position/du/dv remain their declared UV jet values;
the flagged one-sided normal has separate semantics at the quotient.

`evaluate` propagates Vertex flag32 to named Jet.flags value2
`COFACTOR_INSTANCE_NORMAL`. Existing Jet.flags value1 remains the independent
singular-parameter diagnostic. `project` reads the per-jet cofactor bit; no hidden
global profile is required in the backend. For stored instance A with positive
exact determinant, compute `cofactor(A)*n`, then normalize. Use a common positive
matrix scale before forming its cofactor; normalization removes that scale.
This is inverse-transpose normal transport up to its positive determinant,
valid for approximately rigid matrices; rotating n by A is not substituted.
Face reversal is applied once before transport.

Require a source-profile-capable v2 CPU API/CUDA factory for these scenes, while
retaining the legacy v1 factory/profile. Api/Jet/Vertex wire record sizes need
not change. A new CLI selects `cpu_source_api` or
`brepgpu_om10_cuda_v2` with version/size checks before rendering. An old v1 plugin
otherwise silently ignores new Vertex flags, so accepting the header alone is
insufficient. Extend matched native/Python scene readers and independent consumer
checks to recognize only the named profile/flags. New finite profile fields add
no normal-angle accuracy guarantee or production dependency.

CLI: `brepgpu-mesh --source ... --output ... --backend cpu|p100 --threads ...`
with explicit tolerance, limits, device and optional declared CPU fallback.
The source adapter is an external producer; the mesher itself needs no OCCT,
Python or NVIDIA SDK. It owns source load→geometry preparation→boundary rounds→
face/refinement/contact→NativeMesh→optional BGMESH01/native consumer. Common
source/quality/policy inputs are recorded in result semantic fields. All-time
and stage/work/memory counters use one output channel, separating overlapping
worker sums from elapsed wall. No automatic geometry success from a CLI exit
that only wrote incomplete diagnostics.

Write full mesh and optional scene under a run-owned temporary sibling directory.
Check direct final inventory/generations, accepted bounds/contacts and semantic
references, finish/close all files, then atomically rename that directory into
an absent final destination. Existing results remain untouched by default;
explicit replacement uses a separately designed safe directory transaction,
not unlinking a successful result before work finishes. On failure keep only
explicitly incomplete diagnostics/scratch. Library consumers receive an immutable
MeshResult only at success; C ABI handles and Python bindings are optional later.

## 8. Implementation order, RED and finite verification

Parent integrates this plan with the geometry/topology plans and reviews it
before code. Sequence one dependency chain, publishing coherent commits:

1. **Source/value/exact foundation.** Write subplan and native fixture commands.
   Genuine RED for missing native source/rational capability, then implement
   strict reader and owner-explicit arithmetic. Existing immutable source
   fixtures drive semantic inventory/bit-word equality; independently authored
   binary corruptions test lengths/IDs/maps/knots/weights/ownership/cycles.
   Python int/Fraction-generated exact vectors oracle divmod/gcd/rational and
   round/enclosure values; include quotient addback, Lehmer ambiguity, Fibonacci
   ratios, huge quotients, signed zero, exponent limits and resource failure.
   Do not use the new engine to generate its own expected answers.
2. **Bound arithmetic and CPU task.** Geometry owner supplies original exact roots
   and map fixtures. RED old/no task path against original rational Bernstein
   residual answers, then implement host interval/filter/exact comparison. Use
   independently computed original coefficient vectors and exact actual L,
   including clipped cells, large origins, varying positive weights and exact
   threshold equality. Check proved signs and final decision, not just no crash.
3. **P100 task implementation.** Same task corpus; genuine RED absent device task,
   then degree buckets/packing/stream completion. Compare each proved bound
   decision to independent exact answers and required final CPU/P100 NativeMesh
   equality later. Interval primitive exact-operation vectors cover subnormals,
   overflow and zero denominators. A status of UNRESOLVED is permitted only when
   host fallback reaches the correct decision or explicitly exhausts resources.
4. **Runtime/resource state machines.** RED scripted deterministic completions
   exposing parent/child wait and failure paths. Implement scheduler/reservations,
   cancellation, completion leases and sole contact-owner commit seam. Script
   completion orders, bounded capacities, exception/device-error injection and
   stale generation results; assert no leaked lease, lost completion, stale
   mutation or false success. A wall timeout detects a deadlock; it is not a
   performance target or a substitute for the state oracle.
5. **Integrated result/display/CLI.** RED full required NativeMesh fields and
   stored-XYZ scene semantics before view/writer. A physical-plane/nontrivial
   recovered-UV fixture makes reevaluation visibly disagree with stored XYZ;
   actual CPU/P100 native consumers must use stored XYZ. Parser/adapters check
   actual complete output and source association, including seam/pole cells.
   Thread1/multithread/P100 produce identical NativeMesh geometry, no stale
   references, complete inventory; display numerical values have their own
   existing bounds. Wire roundtrip is useful I/O checking, not geometry proof.

   Direct display REDs also use nonorthogonal retained source axes, approximately
   rigid composed instance matrices and a cone apex UV whose floating evaluated
   rho has the wrong sheet sign. Independent source derivative-cross/cofactor
   answers check actual normals and stored XYZ on CPU/P100; matching flags or
   images alone are insufficient. Old strict-profile fixtures remain unchanged.

Run all executable tests/CAD/builds on isolated fleet paths, never the workstation
or historical active watch checkout. Host debug/release warnings, clang-format/
clang-tidy, ASan/UBSan/LSan attack arithmetic/ownership; TSan attacks the real
concurrent runtime, not exact arithmetic run in isolation. Targeted source/wire
fuzzing checks bounded refusal/cleanup. CUDA compute-sanitizer memcheck/racecheck
attacks actual bounded kernels and completion lifetime. Select only relevant
mechanisms per fault class; no test-of-test or second qualification campaign.

After independent code review/fixes, run the actual integrated workload families
already selected by the whole design and independent reader, including native
full-call CPU/P100 costs and unfavorable outcomes. Readiness is not inferred
from standalone rational vectors or copied external input counts. Complete watch
and required rendering remain the delivery goal; this foundation is not the
new whole mesher on its own.
