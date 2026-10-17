# Cached adaptive subdivision: native runtime and CPU/P100 comparison

Research/design checkpoint,2026-10-16. No product implementation, dependency,
CAD execution or benchmark is introduced. Selected runtime candidate below is
concrete enough for design/plan review. It supersedes using the previous
per-triangle numerical-service submission loop as the adaptive mesher.

## Sources read and reference boundary

Pinned USD/BRep source is commit c9c5979f04cea8c38213004d4f04df0dcfb40d05;
matched release asset1.0.0.v1-0-0.7419.dbb624c8 has archive SHA256
`a30bcd6f1c9028bd29092288e0963d5450ab2b9105fc92a034a6c8ce9d8de2b3`.
[QUARANTINE.md](../QUARANTINE.md) records restoration. Source and release build
identifiers are distinct. Read actual `source/SMLib/src/{SmCurveCache,
SmSurfaceCache,SmCacheMgr,SmTess}.cpp` plus their released headers and SmTree /
SmMemBlockMgr / SmTrimSrfCache. Exact source can be inspected at the
[pinned repository](https://github.com/NVIDIA-Omniverse/usd-brep/tree/c9c5979f04cea8c38213004d4f04df0dcfb40d05).

Curve caches retain interval/continuity/length/chord-height/turning information
in binary decomposition nodes. Surface nodes retain UV domains, U/V split,
inside/outside/boundary class, normal bounds and trim incidence. Geometry,
parent and auxiliary records use block managers, with pointer tree links.
SmCacheMgr.cpp375–445 replaces an object's cache and evicts over-capacity
entries; checkout lifetime delays deletion. SmSurfaceCache.cpp2423–2969
constructs/restricts child patch state. SmCurveCache.cpp1454–1867 tessellates
spans and builds the tree. These are mechanisms to study, not a GPU-ready layout
or an owned accuracy contract. No reference code/library is adopted.

Current owned code read: `geometry_store.hpp`, `bound_tasks.hpp`,
`bound_runtime.hpp/.cpp`, `numerical_service.hpp/.cpp`,
`cuda/bound_api.h`, `cuda/bound_cuda.cu`, `interval.hpp`, `point_fixed.hpp`,
`point_trig.cpp`, `native/om10_nvidia/main.cpp` and the NVIDIA reference plan.
The existing CUDA service retains original rational roots, buckets degrees and
executes real restriction/residual kernels, but uploads each task batch and
downloads decisions. It has no resident adaptive node/frontier/point forest;
analytic cells are host tasks. It is a useful numerical/lifetime foundation,
not the requested new adaptive GPU operation.

## Selected operation and placement

Deliver one callable native operation on either CPU or P100:

`source roots + quality + shared boundaries -> adaptive cache -> conforming actual triangles`.

The first checkpoint includes source-root compilation, original-domain cache
construction, actual point construction, bound/mark/split, neighbor-side
conformity, actual-L error checks and triangle output on independent source
families. It is not an allocator, evaluator or render-only demonstration. The
complete trimmed B-rep operation adds settled shared source-edge chains, face
trim classification/connected boundary bands, exact local connectivity,
quotients/poles/contact handling and existing semantic publication.

P100 performs original rational subpatch restriction, direct analytic point and
flatness evaluation, cached bound decisions, frontier mark/split/scan/compact,
point evaluation, interior-side/packet emission and broadphase traversal.
CPU performs the SAME policy on the SAME packed descriptors. CPU exact work is
reserved for genuinely ambiguous predicates/interval decisions and boundary
connectivity where selected, not all analytic geometry or all points. Existing
CPU display versus CUDA display is a separate consumer measurement.

The first callable seam is a session-owned `PatchQuery`: retained original
SourceStore, stable query generation, explicit original carrier/chart IDs and
finite original parameter rectangles, definition-local positional allowances,
selected geometry/numeric policy and run/resource limits. `begin` selects from
canonical roots; `step` returns progressed/pending-device/complete/error without
blocking on a future. A session has at most one mutable adaptive epoch; unrelated
owners are batched into that epoch. A completed `PatchMesh` retains point keys,
exact original parameters/chart maps, actual XYZ, positive conforming facets and
root/leaf coverage associations. It is an actual untrimmed patch result, not a
fabricated complete imported B-rep. Source/quality/contact assembly integration
then consumes the same session rather than introducing another constructor.
The same-quality warm call builds a NEW result view from retained cache; it must
report separately if it merely returns an already retained final mesh. Runtime
statistics and backend identity are outside semantic mesh fields.

## Selected packed cache and ownership seams

1. Immutable `SourceCache` retains SourceStore/GeometryStore, original IDs,
   source generation, extraction revision, original knot/span or analytic-chart
   domains and fixed geometry policy. Rational homogeneous roots and analytic
   original-frame descriptors are separate finite families. Do not approximate
   analytic source by a new rational geometry unless its source-map/error
   contract is separately selected.
2. `RootTable` rows identify original curve/carrier+root, family/degree, exact
   parameter endpoints, coefficient/constants ranges, point/bound profile and
   source-coordinate map. CPU preparation compiles/extracts once; it never
   restricts every eventual leaf before upload.
3. `NodeTable` is SoA metadata: root, parent/two child indices, complete split
   axis/child path, exact root-relative dyadic box descriptor, optional control
   slot, bound-summary slot and generation. Use uint32 checked pool indices and
   uint64 checked byte offsets; capacities cannot alias reserved sentinels.
   Semantic identity is root+path, never pool offset. An inline path plus bounded
   spill words supports configured depth; exhaustion is explicit, not wraparound.
4. `ControlPool` has degree-bucket slabs for homogeneous interval nets. Retain
   immutable roots, live-leaf nets and explicit checkpoints; free split-parent
   payloads at epoch barriers after child construction. Retain parent metadata
   and source-error summaries for warm reselection. Reconstruct an evicted net
   by replaying the SAME split path from a checkpoint, not a different direct
   restriction whose rounding differs.
5. `PointPool` stores source-evaluation key, constructed XYZ, source-departure
   enclosure and profile/generation. A point key uses original carrier/chart and
   exact parameter words; root endpoints normalize to shared original axis-knot
   identities. At an admitted continuous root junction choose the lexicographically
   lowest original (Uspan,Vspan) whose CLOSED box contains the point; a declared
   discontinuity retains distinct branch identities. Evaluate from that canonical
   original root in fixed tensor order,
   independently of which leaf requests it. Two restriction paths reaching the
   same UV can have different rounded nets, so first-leaf-wins corner selection
   is forbidden. Sharing a value does not weld distinct physical node keys.
6. One shared curve-cache authority constructs each physical edge's parameter
   sample and XYZ. Every incident face consumes the same physical key/XYZ.
   Original vertices remain literal source XYZ, including declared alias policy.
7. `QueryView` owns selected leaf/edge masks, boundary generation, trim BVH,
   packet rims and pending exact work. Available cached refinements are not all
   active geometry. A coarser/warm query reselects canonically from roots, so
   cache history does not force gratuitously fine output.

These records retain source and immutable-table leases; they never capture a
mutable owner Budget. Existing prepaid ResourceReservation/RunResource,
stop-token, private exact-child and failed-drain ownership rules apply.

## Deterministic numerical profile: selected

Use explicit binary64 RN operations in the SAME order on host and P100, with
contraction/reassociation disabled. Device uses explicit RN add/multiply/divide
and the SAME bit-neighbor outward padding as host. Do not retain today's
host-RN-padding/device-directed-instruction mismatch for adaptive decisions.
Host nearest mode, no excess precision and FTZ/DAZ checks remain mandatory.
Finite overflow/subnormal handling is specified; unsupported numeric profiles
queue a bounded common fallback rather than becoming accepted geometry.

Select **binary64 RN-first** sin/cos, then directed fixed128/192/256, then
original exact fallback. This replaces the earlier fixed-integer-primary
candidate. The common compact RN primitive runs on CPU AND P100; the existing
host bigint canonical-nearest wrapper is not the ordinary device path.

The finite first profile is `rn64_anchor32_p128_v1`:

1. Generate immutable binary64 LOWER/UPPER literals for pi/2, the existing P128
   sin/cos anchors j/32 and P128 Chebyshev power coefficients, using exact
   integer/Fraction conversion of our existing table generator's dyadic bounds.
   Retain each polynomial's explicit uniform remainder, rounded UP. The owned
   P128 parent supplies sine degree15/cosine degree16 on |delta|<=1/32. Do not
   reinterpret a rounded coefficient as exact or omit coefficient uncertainty.
   Constants are emitted as IEEE words, including a deterministic RN locator
   for 2/pi. This is one continuation of the existing approximation derivation,
   not a new imported polynomial/proof stack.
2. Input is a deterministic binary64 interval enclosing the EXACT original
   parameter (original endpoint plus exact root-relative subdivision fraction).
   Produce it by the same ordered enclosure arithmetic on both backends;
   nonrepresentable/overflowed input is unresolved. Use the RN midpoint locator
   and RN multiply by the fixed 2/pi locator to choose k by explicitly specified
   ties-to-even integer conversion. Require |k|<=2^30 before int32 conversion;
   its binary64 value and modulo4 are then exact. This finite signed-integer
   cap avoids conversion overflow; it is unrelated to scene dimensions.
3. Compute R=X-k*[pi/2] using padded RN interval operations. Require the ENTIRE
   R subset of [-1,1]. k need not be the mathematically nearest quadrant:
   sin(x)=sin(k*pi/2+r) for ANY integer k, and the checked R carries reduction
   error. In particular |k| times the pi bracket width is retained; there is no
   unexplained constant reduction-ulp bound.
4. Choose j by ties-even RN rounding of 32*midpoint(R), require -32<=j<=32,
   and compute Delta=R-j/32. Require the ENTIRE Delta subset of
   [-1/32,1/32]. Use symmetry for negative anchor j. Failure is unresolved,
   never an unchecked clamp. Compute Z=Delta^2 with a zero-aware interval square;
   evaluate the sine/cosine parity polynomials by fixed highest-to-lowest
   Horner order, add their +/- uniform remainders, reconstruct with the anchor
   addition formulas in fixed order, restore k modulo4 and intersect [-1,1].
5. Every endpoint add/subtract/multiply/divide uses RN then the SAME one-word
   IEEE outward neighbor operation on host/device. Sign-aware interval extrema,
   signed zero and subnormal endpoints are explicit; gradual underflow is
   required. NaN/overflow, division through zero or an unbounded intermediate
   is unresolved. No FMA, native trigonometry, backend RD/RU shortcut or
   algebraic reassociation is allowed. Source formulas use the same operation
   DAG on CPU/P100, including a fixed origin/anchor reconstruction order.
6. Let final coordinate enclosure be [l,h]. Construct Q with the same scalar
   RN midpoint operation `RN(RN(l*0.5)+RN(h*0.5))`, canonicalizing zero to +0;
   this avoids midpoint-sum overflow. Q need not lie within [l,h]: its ORIGINAL
   departure is still bounded by the padded maximum of |Q-l| and |Q-h| per
   coordinate, then a shared bounded norm. Original vertices bypass this
   construction and preserve original XYZ. Rational evaluation uses the same
   fixed-order Bernstein/de Casteljau DAG and positive-W guarded division.
7. A failed primitive/positivity/profile check goes to directed fixed128,
   then192, then256 in that order, with the existing finite pi/remainder proof;
   unsupported arguments or unresolved requested accuracy finally use original
   exact evaluation under an owned bounded child lease. The first delivery may
   compact rare fixed/exact work to host; ordinary analytic AND rational point,
   bound, restriction and adaptive work must actually execute on P100.

PointPool keys include the selected numeric stage/profile generation. Start a
new query from the same canonical base profile, even when a higher tier exists
in the cache. If a point's enclosure floor prevents the actual facet/chord bound,
its authority proposes promotion; apply the complete sorted promotion set at a
shared generation barrier and invalidate all dependent bounds/incident faces.
A face's visit order or private epsilon must never replace a shared Q. Recompute
selection canonically after promotion; warm history is only a cache hit, not a
permission to select a different point. Numerical-floor failure is distinct
from geometric curvature failure: subdivision cannot chase a fixed point ulp.

Constructed XYZ is the deterministic bounded result of this stated operation,
not a new promise of correctly-nearest transcendental source XYZ. Its original
Ce/S departure is enclosed and charged in actual curve-chord/face positional
error once. Delta still compares ORIGINAL V/Ce(t) against S(selectedUV); it is
not a free Q-error allowance. Original source values remain unchanged and the
semantic geometry policy increments. If a profile's numeric departure cannot
fit available positional error, increase its specified precision or refuse;
geometric subdivision alone need not reduce evaluation roundoff.

Bound-summary acceptance uses common canonical predicates; unresolved original-
source acceptance or point-floor cases may enqueue higher-precision tasks with
immutable keys. Axis scores are HEURISTICS: compare the shared RN upper scores,
use fixed U ties/overlaps and fair active-axis shrinking, without exact trig
merely to choose an axis. Device completion order and backend-specific endpoints
never choose different children. Ordinary acceptance needs no host recheck.

Keep BASE and promoted root/control/summary/point stages as distinct immutable
records. A new query executes the same base-to-escalation policy from source;
a tighter cached bound cannot silently replace BASE and accept a leaf that the
cold query would refine. Replay includes both split path AND arithmetic stage.
A point promotion creates a new constructed-record generation even when logical
source parameter identity is unchanged; invalidate all dependent registry,
contact, boundary and publication generations at their common barrier.

## Cached bounds and actual output

Leaf B uses the four actual PointPool XYZ corners. Cache a bound e for S-B,
actual bilinear twist D=B11-B10-B01+B00, directional split scores and point
uncertainty. Positive rational source uses its retained homogeneous net and
N-WB coefficients / positive lower W. Analytic source uses original-frame
cached Gram/directional curvature constants and direct source formulas; no
arbitrary-rational Taylor jets per leaf.

For a triangle contained in the unit leaf, bilinear-to-affine error is bounded
by norm(D)/4 plus max corner departure of B from the actual triangle L. The
factor follows from |Cov(x,y)|<=sqrt(Var(x)Var(y))<=1/4 for positive
barycentric weights. This reuses one leaf bound across transition triangles.
For cross-leaf triangles clip/index overlaps and use B minus the SAME original
actual L on virtual corners, never newly sampled S corners. Domain movement
remains its separate charged component. Bound failure requests owned cache
refinement; the old whole-face LEPP engine is not hidden behind this API.

## Device adaptive epoch and batching

- Begin with canonical frontier order by definition/source root/path. Batch
  independent owners together, then stable-bucket execution by family/degree /
  numerical profile. Keep logical slots so kernels write into canonical order.
- Evaluate requested point keys in bulk, restrict missing live nets, classify
  leaves against packed selected trim data and compute source bounds/split
  commands. Generate ambiguity and source-boundary requests by count+scan.
- Resolve only compact ambiguous records on host, with preregistered child
  memory/work reservations. Other independent owners can progress; no worker
  waits while holding an owner lock. A face/definition cannot commit an epoch
  before its own complete ambiguity/request horizon is resolved.
- Integer exclusive scans assign child/node/control/output slots from canonical
  logical command order. Check total required capacity BEFORE mutation; a race
  cannot decide which partial faces fit. Split in bulk, compact the next frontier
  and reclaim dead payloads only after completion barriers.
- Freeze boundary generations and selected leaves before side/rim construction.
  Leaf/trim crossings classify boundary-band work; they do not create private
  physical edge samples. GPU interior packets and exact connected boundary-band
  connectivity consume the SAME settled source chains and ordered side events.
- Publish source/root/path/side-key order, not kernel completion order. CPU/P100
  source-linked mesh semantics match; runtime counters are outside semantic data.

Select segmented scan/compaction kernels with persistent MEMORY as phase1.
Do not require one always-running uberkernel or dynamic parallelism. A bounded
block-local subtree/donation scheduler is a serious later contender if frontier
barriers/tails dominate, but must produce the same logical commands. No perleaf
cudaMalloc, module load, source upload or blocking future is allowed.

## Spatial indexes and memory policy

Use packed deterministic LBVH for selected trim segments and frozen physical
facets where broadphase needs it. Stable Morton keys include original ordinal
for duplicate centers. Bounds are outward; Morton quantization affects work
ordering, not geometry classification. Build/rebuild indexes per relevant
boundary/contact epoch; do not confuse a BVH over existing leaves with the
adaptive source-subdivision tree whose leaves are not known in advance.

Reserve root banks, live control slabs, two frontier buffers, point storage,
scan/sort scratch, ambiguity queues and output staging in the declared cap.
Small exact fallbacks never wait for unreserved growth. Cache budget eviction
uses deterministic barrier policy and drops only replayable cache payloads.
GPU OOM/capacity failure is an explicit whole-operation outcome, not missing
parts or hidden CPU substitution. Multiple P100s partition independent
DEFINITIONS first so shared physical boundaries remain local. Replicated root
bytes and exchanged boundary data are counted, not called free transfers.

## Lifetime and invalidation

| Change | Reuse | Invalidate/reselect |
|---|---|---|
| Same request/source/profile | Roots, nodes, points, bounds, trim index | New output lease only |
| Quality change | Source geometry, existing restrictions/summaries | Canonical selected frontier and edge/trim query; point profile if changed |
| Trim/use-map change | Unchanged carrier geometry restrictions | Incident trace/classification, boundary band, corresponding source map |
| Source coefficient/knot edit | Unrelated dependency roots | Dependent roots/points/bounds and incident queries |
| Placement/appearance change | Definition-local cache | Instance/display rows; leaves only if effective world allowance changes |
| Numeric policy/profile change | Original source inventory | Affected point/bound cache entries and query selection |

A cache is explicitly owned by the native session/application. Source edits do
not mutate a leased cache under running kernels: create new generations and
retire old ones after completion. Healthy close drains/releases all. Failed
final drain retains its already registered poisoned module/buffer/reservation
owner for process lifetime, as the current runtime does. Cancellation does not
publish half a mesh or allow premature DMA reclamation.

## Algorithm contenders and primary reading

[Karras2012](https://research.nvidia.com/sites/default/files/pubs/2012-06_Maximizing-Parallelism-in/karras2012hpg_paper.pdf),
§§3–4, builds radix hierarchies from sorted keys and handles duplicate Morton
codes with index ties. Select this for trim/contact indexes, not as a substitute
for deciding unknown adaptive leaves. Its GPU speed numbers are not our target.

[Tzeng/Patney/Owens2010](https://escholarship.org/content/qt9r15d4zk/qt9r15d4zk.pdf),
§§3–5, compares block/distributed/stealing/donation queues for irregular work.
It motivates a bounded donation contender if level-frontier tails dominate;
its rendering workload does not establish B-rep error or determinism contracts.

[Benyoub/Dupuy2024](https://arxiv.org/html/2407.02215v1), §§3.1–3.3,
separates a bitfield/sum-tree pool manager from mesh subdivision. Its rank/select
allocator supports sparse incremental allocate/free; its fixed-size blocks,
reduction refresh and neighbor edits have costs. It separates pool capacity
from geometric depth, but subdivision index WIDTH still limits depth. Select
scan-frontier plus slab pools for initially monotone source-cache refinement;
CBT allocation is the concrete alternative for heavy edit/coarsen churn. Neither
its triangle compatibility chains nor a camera-error policy establishes our
trim/source-edge contracts. No mesh-shader hardware is assumed on sm60.

## Comparison contract

Compare NVIDIA CPU, owned CPU1/parallel CPU and owned P100 over the SAME original
source definitions/placements and declared positional/angle controls. Retain
strict no-partial outcomes and both predeclared NVIDIA healing policies. The
same tolerance number does not imply the same source normalization, bound or
output topology; report actual admitted geometry/quality and counts separately.

Measure independently:

1. Cold process source-to-mesh: input decode/adapter, source compilation, root /
   cache preparation, context/module/allocation/upload, adaptive geometry,
   boundary/conformity/contact, actual mesh download and complete output close.
2. Prepared-source cold-cache operation: already admitted original source,
   empty adaptive host/device cache, through complete native triangles.
3. Warm same-quality remesh: retain declared roots/nodes/points and a fresh
   output; expose whether a prior final mesh is merely being returned.
4. Warm quality-change operation: replay/reselect/refine canonically; report
   retained and additional nodes, exact fallback, changed boundary/index work.
5. Source/quality invalidation and display: report separately; display is not
   adaptive construction time. Process exit wall and device-event kernel time
   are distinct, and overlapping parallel stages are not summed as wall.

Record retained host/pinned/device/temporary peaks, uploaded/downloaded bytes,
root/leaf/control/point counts, node visits/splits, frontier rounds, ambiguity
counts and host exact work. These explain work, not certification receipts.

Current NVIDIA adapter constructs/imports then deletes each SmBrep; its existing
results do not establish warm cache reuse. More specifically, pinned
`SmTess.cpp:3069–3121` creates a NEW SmTessSrfCache and calls BuildTree;
`:12134–12157` removes/deletes that face cache in Phase3DeleteFacePolygons.
Keeping an imported SmBrep/context is therefore an **imported-object-warm,
tessellation-cache-cold** operation in this path, unless an actually exposed
alternate path establishes otherwise. A reference-only warm mode must retain
actual source/context, emit fresh SmPolyBrep triangles and label the cache scope
it really retains. SmApiTessellate's FALSE is bAllowPartial, not a cache flag.
Never compare our prepared warm cache with NVIDIA cold conversion/import as if
equal, or invent a reference tree-warm API because its source contains caches.

First real-operation families include positive rational multispans/C0 boundaries,
analytic cylinder/sphere/cone/torus, warped planar and rotated-ridge carriers,
shared physical edges, trims/holes, seams/quotients and bounded large-degree /
ill-conditioned-weight refusals. Complete original components and the full
watch supplement them. Measurements test whether batching/reuse/device work
wins, including tiny cases; no GPU speedup is presumed or runtime SLA invented.
