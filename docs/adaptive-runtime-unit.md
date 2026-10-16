# Adaptive cache execution unit

2026-10-16. Implements the accepted first checkpoint in ADAPTIVE_MESHER_PLAN.md.
Runtime owns adaptive_types.hpp, adaptive_cache.hpp/.cpp,
cuda/adaptive_api.h and cuda/adaptive_cuda.cu plus direct runtime fixtures.
Geometry owns source_cache.hpp/.cpp, CUDA17 adaptive_geometry_types.h and the
shared host/device numerical header. Construction owns adaptive_packets.hpp/.cpp.
Root owns CMake and the complete patch caller. Product code follows this finite
unit's peer review and missing-operation RED; all execution is fleet-only in the
new cached-adaptive-mesher checkout. No external dependency.

## Operation, concrete public seam and ownership

Use namespace bgmesh::adaptive. SourceCache is prepared directly from SourceStore
and explicit requested original carrier rectangles by the geometry owner. It
retains the ACTUAL immutable source lease, not merely numeric IDs/generation.
Original-span evaluator roots are distinct from prepared query-root intersections;
points evaluate the canonical ORIGINAL root. Every RootSlot is qualified by the
retained SourceCache. Tagged carrier/curve source identities never alias IDs.

The runtime public declarations are selected as follows; geometry's actual
RootSlot/SourceCache declarations are included, not redeclared:

```cpp
struct RootAllowance { RootSlot root; ExactRef positional; };
struct PatchQuery {
  uint64_t generation;
  SourceCache source; // must be the actual session cache lease
  ExactTable values;
  std::span<const RootAllowance> allowances;
};
struct CacheLimits {
  uint32_t nodes, points, control_scalars, frontier, ambiguities;
  uint32_t cpu_threads; // actual bounded CPU kernel worker width
  uint32_t max_axis_depth; // 1..64 in the first dyadic64 bucket
  uint64_t host_bytes, device_bytes, child_bytes, child_work, total_work;
};
struct CacheDevice { int32_t device; std::filesystem::path module; };
struct CacheStats {
  uint64_t queries, epochs, nodes_created, nodes_reused;
  uint64_t point_evaluations, point_hits, control_restrictions, exact_fallbacks;
  uint64_t device_points, device_bounds, device_restrictions, device_epochs;
  uint64_t uploaded_bytes, downloaded_bytes;
  uint64_t host_reserved_bytes, host_peak_bytes, device_reserved_bytes, device_peak_bytes;
  uint64_t packet_rounds, actual_facet_tests, actual_facet_failures;
}; // cumulative observations, no semantic mesh fields
struct PacketMeshData; // supplied by packet owner; actual complete triangles
class PatchSession {
 public:
  PatchSession(std::shared_ptr<RunResource>, SourceCache, CacheLimits,
               std::stop_token = {});
  ~PatchSession();
  void open_device(const CacheDevice&); // optional, before begin; explicit errors
  void begin(const PatchQuery&); // copies span; retains immutable table/source
  StepState step(JobContext&); // ready while GPU pending; no blocking future
  std::shared_ptr<const PacketMeshData> result() const; // only final actual-bound PASS
  std::shared_ptr<const PatchSelection> selection() const; // diagnostic snapshot
  CacheStats stats() const noexcept;
  void release_result();
  void cancel() noexcept;
  void close(); // failed drain retains live owner for retry
};
```

The caller's PatchQuery span is borrowed only by begin. All allowances must be
positive exact values in its retained table and name each requested prepared
root exactly once; canonicalize root order. Fast acceptance converts the exact allowance DOWN;
never round it nearest and accidentally enlarge it. Equality/interval ambiguity
uses the same exact threshold comparison fallback. Reject foreign SourceCache roots,
foreign tables/resources, duplicate/missing roots, stale/nonmonotone query
generations and begin while an operation/result is live. Session has one mutable
epoch at a time. `step` checks its own stop token and JobContext token. Immutable
completed output can outlive the session through retained source/table leases.
The root caller does not construct GeometryStore/PreparedRun to satisfy types.

`PatchSession` integrates the packet builder and the actual-facet feedback loop.
A four-corner leaf acceptance is NEVER exposed as completed PatchMesh. Runtime
links geometry+packets; packets links only shared values+geometry/exact, avoiding
a cache dependency cycle. Final result contains positive conforming triangles,
actual XYZ, exact construction coordinates, source/chart maps and leaf identity.
It is an untrimmed patch operation, not complete source B-rep publication.
Phase1 requests one complete coherent rectangle, possibly across source spans.
Any packet `demoted_leaf_slots` remaining before boundary-region integration is
unsupported/incomplete, NEVER successful missing area. Empty/partial PacketMesh
cannot stand in for the requested nonempty complete patch.

## Shared finite values and identities

adaptive_types.hpp defines C++ leased values; adaptive_geometry_types.h defines
CUDA17 POD numeric views. No mutable Rational or Budget crosses owners/devices.
Use distinct RootSlot, NodeSlot, CachePointSlot and query-local PointSlot types.

- RootKey: qualified SourceCache + tagged original source kind/ID, ORIGINAL span
  ordinals, prepared rectangle/chart identity. Prepared RootSlot never replaces
  the original evaluator root key.
- Path: append-only parent/axis/child records, checked depth and generation;
  root plus complete axis/child sequence is identity. Inline prefix/hash only
  accelerates comparisons; equality compares the complete retained path. Frozen
  selection materializes paths into one metered word pool, not perleaf heaps.
- Dyadic64: normalized numerator/exponent n/2^e, e<=64. Zero=(0,0), one=(1,0);
  endpoint1 at depth64 never computes uint64(2^64). Check midpoint arithmetic
  before mutation; needing a deeper coordinate returns typed unresolved/depth
  exhaustion on BOTH backends. A future wide bucket cannot truncate this key.
- PointKey: original carrier/chart + exact original parameter identity + numeric
  stage/generation, qualified by actual SourceCache. Normalize common root-knot
  endpoints to the checked continuous original junction's canonical owner.
  Query-local PointSlots are assigned by canonical PointKey sort, never cache
  allocation/completion order. Physical identity is not coordinate equality.
- PointRecord: chart_xy[2] ExactRefs, original_uv[2] locator refs (separate even
  when equal), actual_xyz[3] binary64, original departure interval, source/root/
  profile association. First ordinaryUV roots use equal chart/UV values; finite
  plane maps may keep distinct coordinates and locators without changing APIs.
- SelectedLeaf: RootSlot, full LeafKey, exact chart box[4], PointSlot corners[4]
  in (00,10,01,11) order, cached source-to-actualB upper, actual twist summary,
  numeric/point-stage identity and selected generation.
- PatchSelection: retained SourceCache, shared RunResource, ExactTable, packed
  PointRecords/SelectedLeaves/path words and complete query/cache/profile key.
  It is immutable through packet creation and actual-facet tests.

Every pending command/result is bound to session serial, source lease/cache
revision, query generation, profile/stage, root/path and operation ordinal.
Slots have checked physical revisions. Host retains the immutable submission
and verifies its ticket; a device-returned slot alone cannot route work across
sessions. No new semantic certificate or receipt fields are introduced.

## Resident arrays, queues and device procedures

Allocate metered slabs for SoA nodes, degree-bucket interval controls, points and
stage-tagged bound summaries. Parent metadata/base summaries survive split;
parent controls may be reclaimed only at a completed epoch and reconstructed
along the SAME path+stage. BASE and higher stages are immutable distinct rows.
A new query follows cold base-to-promotion decisions even if a tighter row is
cached. Quality changes do not activate every historically refined descendant.

Keep TWO frontier arrays plus command/count/scan/sort scratch, ambiguity and
point-miss queues, output staging and retained roots resident. Capacity counts
and byte arithmetic are checked uint64; pool indices are checked uint32 and
never equal reserved sentinels. Profile/depth limits are explicit data.

CPU1 runs the same finite batches directly. For cpu_threads>1, the session owns
NativeScheduler and at most4*cpu_threads typed range jobs. Each retains immutable
EpochData and writes a disjoint preallocated output slice, using its own scheduler
JobContext/reservation/stop token. The coordinator submits then polls completions
without waiting; no worker submits children or borrows the session's Budget.
Fold the full logical horizon before commands. Thus --threads names actual
parallel numerical work, not an idle outer scheduler around one serial task.

Each complete logical epoch runs:

1. Canonically order requested root/path/stage work. Stable bucket execution by
   family/degree preserves logical output indices. Generate missing point keys;
   stable sort/unique and map requests to the single PointPool authority.
2. Device or CPU evaluates source points with geometry's SAME RN64/padding DAG,
   restricts missing interval controls and computes source-to-actualB bounds,
   twist and heuristic directional scores. Direct analytic source kernels and
   rational kernels BOTH run on P100. Old BoundRuntime per-task upload is not
   used as a stand-in for resident adaptive state.
3. Mark accept/split/point-promotion/numeric-ambiguity commands. Use fixed U axis
   ties and fair active-axis policy; no exact trig merely to choose an axis.
   Collect full sorted promotion/ambiguity horizon. Parent/incident rows cannot
   commit using stale PointPool generations.
4. Own integer exclusive scan (block scan plus block-sum scan) assigns child/
   control/point/next-frontier positions. Stable radix passes handle fixed POD
   key words; exact wide identity comparisons, when required, are a bounded
   host task. Count ALL next arrays and reserve capacity BEFORE any mutation.
   On insufficient configured capacity fail the query, never fit a racing subset.
5. Apply chosen restrictions/commands in canonical slots, compact next frontier
   and reclaim old payloads at the event barrier. Point promotion creates a new
   constructed generation and invalidates every dependent leaf/bound/request.
6. Freeze selected leaves, invoke actual packet side schedules/zipper, then test
   EVERY actual facet against cached e+B-SAME L. GPU receives actual PointSlots
   and emitted roles; generic hanging facets use the quadratic residual, not
   a four-corner shortcut. Fold complete failures, tighten the SAME facet when
   useful, then refine implicated cache leaves and rebuild packets. Complete
   result only after all actual emitted facets accept. Additionally require each
   actual binary64 XYZ facet to have exact nonzero physical area (exact dyadic
   cross product); positive chart area alone is insufficient. Collapsed physical
   facets request finite source/cache refinement or explicit unsupported, never
   silent triangle deletion/success.

Analytic sample/bound work is thread-per-task. Rational bicubic control work is
warp-cooperative, degree-bucketed with explicit shared workspace; 64 homogeneous
scalar intervals occupy1024B per net. Staging parent/two-child nets plus residual
workspace must be counted per warp/block. Use lane-strided coefficients and
stage barriers, not one thread's large local arrays. Geometry supplies finite
coefficient/restriction procedures with external scratch. Measure actual register/
shared/spill/kernel costs in the qualified complete operation; tune block/warp
count without changing numeric order or policy. CPU uses the same operation DAG.

## Common fallback, lifetime and resource behavior

Use rn64_anchor32_p128_v1, then fixed128/192/256, then original exact according to
geometry's finite policy. A shared-authority promotion transaction applies the
whole sorted request set; no per-face/private Q replacement. Coordinate error
is positional epsilon once; source delta remains original Ce/V versus original
S. Numeric-floor failure requests tighter arithmetic, not endless subdivision.

Reserve session host/device caps via existing ResourceReservation; CUDA external
allocations retain the reservation, host PMR blocks consume prepaid capacity.
Exact child leases are reserved before admission and have private Budget/work
limits; never concurrently borrow a mutable parent Budget. Reserve worst-case
child work before launch, settle actual/refund unused under serialized session
counter. Memory/work exhaustion and cancellation fail atomically; no partial mesh.

Pre-register device/module/buffer/event/staging/reservation owner before CUDA
acquisition. One persistent session per selected device, no perleaf cudaMalloc
or source upload. Root pages upload once; epoch queues/control deltas and final
mesh downloads are measured. Nonblocking event polling returns ready while
pending, allowing independent owners to progress. Explicit close drains and
releases; failed close retains live state for retry. Final failed drain retains
its already registered poisoned owner for process lifetime without new
allocation/free/dlclose. Device restore failures remain explicit errors.

## Direct RED, GREEN and delivery

First author a native runtime fixture against missing PatchSession and generate
literal original-source examples independently: bilinear saddle, positive-rational
bubble, C0 two-span surface, nonorthogonal cylinder/cone/sphere/torus. The fixture
uses real SourceStore→SourceCache→PatchQuery→actual packet facets on CPU/P100.
Missing API/link is genuine RED, not an unrelated absent dependency failure.

Check original POWER/rational/analytic values and whole known bound inequalities,
positive exact chart areas, complete patch coverage and literal common side
incidence. Exercise deliberately hanging neighbors; actual facet tests must reject
an exactly collapsed physical facet and an off-surface actual corner even if the source leaf itself is flat. CPU1/parallel/
both P100 semantic bytes compare determinism separately from those geometry
oracles. Assert nonzero real device point/bound/restriction and adaptive epoch
counts on analytic and rational cohorts, not fabricated device counts.

The same fixture covers identical warm query, fine→coarse→fine, evicted SAMEpath/
stage replay, tighter cached bound not silently replacing BASE, stale generation,
foreign source/table, capacity exactly enough/one short, dyadic64 endpoint/depth,
stop-before-submit and stop-with-pending-device, healthy close/reuse. Test bounded
known failures; no driver-fault injection campaign. Run meaningful strict Release,
ASan/UBSan/leaks host paths and relevant CUDA memcheck if available, actual-source
review, targeted clang-tidy/format. No blanket reruns or tests for tests.

Final actual-source review covers kernel math/indices, stage/history, concurrent
lease lifetime and count-before-mutation paths. Root wires CMake targets after
headers/tests physically exist. Keep complete cold/prepared/warm source-to-patch
operation times, memory, transfers, misses/fallbacks and mesh counts. No GPU
speedup or full-component completion is promised by the unit.
