# Shared original curves: runtime and physical authority

Research and finite API design, 2026-10-16. No production edits, library adoption,
builds or CAD executions in this study. Implements checkpoint 2 of
ADAPTIVE_MESHER_PLAN.md. The operation is original definition -> complete shared
physical edge chains plus every occurrence's selected trace; a curve evaluator
alone does not complete it. Trim-region triangulation and complete face-domain
homeomorphism remain subsequent operations.

## Actual reading and selected mechanisms

Owned revision read: f6f2cae7f25a5dc99823fdabc557719631a4a0cf, including
`adaptive_cache.cpp` (tiers, frontier, complete count/fold, workers, async device
owner), `source_cache.cpp`, `boundary.cpp` and `boundary_evaluation.cpp`,
`meshing_run.cpp::DefinitionOwner::{evaluate_boundary,boundary_horizon}` and
`geometry_boundary.cpp::{locate_boundary_site,choose_boundary_joint}`.

NVIDIA USD/BRep pin c9c5979f04cea8c38213004d4f04df0dcfb40d05; restoration and
released binary archive identity are in QUARANTINE.md. Read these exact source
procedures and released `SmCurveCache.h`:

- `source/SMLib/src/SmCurveCache.cpp::Tessellate` (1454–1867) extracts original
  spans, restricts requested intervals, and halves failing spans. `BuildTree`
  (358–568) constructs parent summaries from ordered leaves; optional simplification
  discards descendants. Parent/leaf block records store interval, continuity,
  control-polygon length/chord/turn summaries. Our storage keeps query selection
  distinct from retained refinements. Their minimum-parameter stopping override
  does not establish our positional acceptance and is not selected.
- `SmTess.cpp::SmCurveTessDriver::TessellateCurve` (1320–1438) tries curve bisection
  before its local CurveCache path; the presence of a cache class does not mean
  every curve uses it or that caches persist across calls. `TessellateEdge`
  (3009–3066) splits one shared topology edge. `ClampAndSplitEdge` (2884–2987)
  retains source vertices and removes near-endpoint samples; we retain source
  endpoints but do not silently delete original facts using proximity.
- `SmTess.cpp::Phase1SetupBrep` (11673–11843) repeatedly constructs face polygons
  after shared-edge changes. It explicitly revisits all faces; our source CSR
  permits incident-only invalidation. `IsoEdgeSubdivide` (2696–2752) aligns some
  UV-isoline edges to surface-cache boundaries. Our connected boundary-region
  design instead keeps the shared chain intact: leaf/trim intersections are
  classification events, not private physical samples.

Pinned source links:
[CurveCache](https://github.com/NVIDIA-Omniverse/usd-brep/blob/c9c5979f04cea8c38213004d4f04df0dcfb40d05/source/SMLib/src/SmCurveCache.cpp),
[SmTess](https://github.com/NVIDIA-Omniverse/usd-brep/blob/c9c5979f04cea8c38213004d4f04df0dcfb40d05/source/SMLib/src/SmTess.cpp).

Parallel algorithm reading:

1. Schwarz/Stamminger, *Fast GPU-based Adaptive Tessellation with CUDA*, 2009,
   §2 and §3 plus full one-page kernel/dataflow supplement. Shared boundary
   schedules precede counts/scans/output; they discuss copying common positions
   because reversing mathematically equivalent evaluation orders can change
   floating results. We select one physical Q authority, stable scan allocation,
   and shared RN arithmetic. Their screen-space rational-patch factors are not
   a proof for our original B-rep curves or occurrence maps.
   [Paper](https://michael-schwarz.com/research/publ/files/cudatess-eg09.pdf),
   [supplement](https://michael-schwarz.com/research/publ/files/cudatess-eg09-suppl.pdf).
2. Blelloch/Fineman/Gibbons/Shun, *Internally Deterministic Parallel Algorithms
   Can Be Fast*, PPoPP 2012, §§5.1–5.2. Stable collection operations and explicit
   complete reserve/commit rounds motivate our deterministic request union and
   preflight. Pure edge splits commute after duplicate keys are removed; no
   speculative Delaunay conflict machinery is necessary for that transaction.
   [Primary paper](https://www.cs.cmu.edu/~guyb/papers/BFGS12.pdf).
3. Blelloch, *Prefix Sums and Their Applications*, CMU-CS-90-190, §§1.2–1.3:
   work-efficient up/down sweep, stable packing and integer radix allocation.
   Reuse our owned scan implementation; count before writes and reserve both
   children from each accepted split. No imported scan library.
   [Primary chapter](https://www.cs.cmu.edu/afs/cs/academic/class/15451-s14/www/LectureNotes/PrefixSumBlelloch.pdf).
4. Spielman/Teng/Üngör, *Parallel Delaunay Refinement: Algorithms and Analyses*,
   2002, Algorithms 2/4 and independence definition. Candidate conflicts belong
   to actual topology edits. Its logarithmic-round conclusions require its
   meshing assumptions and do not prove termination of our carrier recovery,
   seam handling or shared trace fixed point.
   [Primary paper](https://arxiv.org/pdf/cs/0207063).

## Selected ownership and avoided work

Reuse **DefinitionBoundaryForest as the sole physical owner**, with a fresh
cheap forest per quality query. It already retains original endpoints, declared
quotients, initial spatial-knot and use-map-t cuts, sorted whole-face requests,
original-direction chains and incident generation updates. Do not implement a
second authority. Its existing monotone retired-leaf state cannot itself be the
warm cache: a coarse query must reselect roots, not inherit fine leaves.

Persist a dedicated **CurveSourceCache and numerical cache** outside that forest.
Compile only live requested spatial curves/pcurves, not the old eager
GeometryStore. Original curve span/extraction is shared across physical edges
that reference it. Prepared restrictions carry their exact original interval;
identical restrictions share controls, different intervals never alias. Index
original knot ranges once rather than scanning all knots for every reused edge.

Three identities must remain distinct:

- numerical sample: actual source lease, original CurveSlot/span, exact t,
  arithmetic/profile tier; never ephemeral BoundarySampleSlot;
- physical sample: definition + original edge/t authority, or literal original
  VertexID/declared quotient; coordinate equality never joins edges;
- use site: original CoedgeSlot and loop junction occurrence, mapped original s,
  original lift/branch and selected original UV/construction chart point.

A shared numerical Ce(t) result may serve several edges, while physical keys
remain distinct. Literal endpoint vertices override numerical curve endpoints.
Consequently chord-bound caches must include actual endpoint XYZ bits/revisions,
not only root/path. Cache ordinary canonical endpoint-chord bounds once; a
physical vertex override can use that bound plus the maximum endpoint departure,
then the tighter SAME actual-chord residual only when useful. This avoids
repeating all source restriction work for adjacent edge uses.

Within each fresh forest, ordinals follow canonical complete request horizons,
not persistent-cache insertion order. PhysicalNodeKey is scoped to that retained
query boundary; cross-query logical sample comparisons use original edge plus
exact t, not an allocation ordinal. Same-input fresh/warm queries reproduce
their canonical keys, while different quality queries retain separate leases. Promotion uses one physical-owner revision
transaction: validate expected sample key/revision, install all revised interior
Q values, increment affected incident-face generations together, then rebuild
those occurrence bounds. Wire or ordinary spatial-chord failures enter one narrow owner-only
`DefinitionBoundaryForest::request_owner_splits(epoch, keys)` method. It checks
current epoch/live intervals/definition, unions with face requests, and does not
invent a requesting face for a wire. This extends the existing sole owner.

PhysicalNodeKey remains logical source identity. The retained boundary snapshot
adds a parallel uint64 constructed_revisions pool, one value per nodes row. A
revision proposal carries current sample slot AND expected PhysicalNodeKey AND
expected constructed_revision; the session also supplies its current query and
BoundaryEpoch. Validate the entire unique sorted proposal horizon, then install
Q and increment revision plus affected face generations atomically. Old leased
snapshots retain their old Q/revision; no live consumer observes a same-revision
replacement. Numeric provenance is original curve/t/profile/tier, not the slot.
Original vertices have revision0 and never promote. Numerical BASE and
promoted records remain separate. Fresh queries replay the same BASE-first
policy; a warm exact result cannot silently change a coarse query's Q or chain.

## Concrete native interface and output rows

Namespace `bgmesh::adaptive`; runtime owns `adaptive_curves.hpp/.cpp`, geometry
owns dedicated CurveSourceCache and 1D numerical PODs, construction owns
`adaptive_traces.hpp/.cpp`. Slot/ref fields are qualified by retained source and
ExactTable leases. All variable rows use PMR pools under the run resource.
Proposed declarations selected with the construction owner:

```cpp
struct FaceCurveAllowance {
  Slot<FaceTag> face;
  ExactRef positional, node_source;
};
struct CurveBoundaryQuery {
  uint64_t generation;
  ExactTable values;
  ExactRef default_positional, default_node_source; // also cover wire edges
  std::span<const FaceCurveAllowance> overrides;
};
struct CurveLimits {
  uint32_t nodes, points, control_scalars, frontier, requests, ambiguities;
  uint32_t max_depth, cpu_threads;
  uint64_t host_bytes, device_bytes, child_bytes, child_work, total_work;
};
struct CurveStats {
  uint64_t queries, epochs, nodes_created, nodes_reused;
  uint64_t point_evaluations, point_hits, control_restrictions, chord_tests;
  uint64_t trace_tests, exact_fallbacks, requested_splits, installed_splits;
  uint64_t invalidated_faces, device_points, device_bounds, device_restrictions;
  uint64_t uploaded_bytes, downloaded_bytes;
  uint64_t host_reserved_bytes, host_peak_upper_bytes;
  uint64_t device_reserved_bytes, device_peak_bytes;
};
struct UseCurveTrace {
  Slot<CoedgeTag> use;
  Slot<FaceTag> face;
  uint64_t face_generation, boundary_generation;
  Range<UseCurveSite> sites;
  Range<UseCurveInterval> intervals;
};
struct UseCurveSite {
  uint32_t boundary_node; // index in retained boundary->nodes
  Slot<LoopTag> loop;
  uint32_t junction_occurrence; // UINT32_MAX at a nonjunction interior site
  ExactRef original_t, mapped_s;
  std::array<ExactRef,2> chosen_original_uv, chart_xy;
  std::array<int32_t,2> original_periodic_shift, selected_branch;
  ExactRef node_source_upper; // maximum of required ORIGINAL guards, never Q
};
enum class CurveTraceAuthority : uint32_t {
  reconstructed_selected=1, authoritative_original=2
};
struct CylinderAffineIdentityRecord {
  Slot<CurveTag> spatial; Slot<CarrierTag> carrier;
  std::array<ExactRef,2> original_t;
  std::array<ExactRef,4> chosen_uv;
  ExactRef constant_v; int32_t orientation;
};
struct PhysicalCurveInterval {
  BoundaryKey boundary_key;
  std::array<uint32_t,2> boundary_nodes;
  std::array<ExactRef,2> original_t;
  ExactRef parameter_chord_upper;
  std::optional<ExactRef> image_chord_upper;
  CurveCorrespondence selected_correspondence;
  std::optional<GeometricLineRecord> image_map;
};
struct UseCurveInterval {
  BoundaryKey boundary_key; // actual physical interval authority
  std::array<uint32_t,2> sites; // indices in snapshot site pool
  CurveTraceAuthority authority;
  CurveCorrespondence correspondence; // finite geometry-owned map tag
  ExactRef carrier_trace_to_actual_chord_upper;
  ExactRef source_to_selected_trace_upper;
  std::array<ExactRef,2> uv_chord_component_upper;
  std::optional<ExactRef> authoritative_domain_upper;
  std::optional<GeometricLineRecord> image_map;
  std::optional<CylinderAffineIdentityRecord> cylinder_identity;
};
struct SharedCurveSnapshotData {
  std::shared_ptr<RunResource> resource;
  SourceStore source;
  Slot<DefinitionTag> definition;
  uint64_t query_generation;
  ExactTable values;
  DefinitionBoundarySnapshot boundary;
  // boundary->constructed_revisions parallels boundary->nodes; see transaction below
  std::pmr::vector<PhysicalCurveInterval> physical_intervals;
  std::pmr::vector<UseCurveTrace> uses;
  std::pmr::vector<UseCurveSite> sites;
  std::pmr::vector<UseCurveInterval> intervals;
};
using SharedCurveSnapshot = std::shared_ptr<const SharedCurveSnapshotData>;
class AdaptiveBoundarySession {
 public:
  AdaptiveBoundarySession(std::shared_ptr<RunResource>, SourceStore,
      Slot<DefinitionTag>, CurveSourceCache, CurveLimits,
      const std::stop_token& = {});
  void open_device(const CacheDevice&);
  void begin(const CurveBoundaryQuery&);
  StepState step(JobContext&);
  SharedCurveSnapshot result() const;
  CurveStats stats() const noexcept;
  void release_result();
  void cancel() noexcept;
  void close();
};
```

`GeometricLineRecord` is `{array<ExactRef,3> covector;
array<ExactRef,2> curve_scalar,trace_scalar,original_t; int32_t traversal;}`.
Its t interval is increasing, scalar/endpoints paired to that order; traversal
retains the original use direction.
`CurveCorrespondence` uses the geometry proposal finite tags affine_parameter,
monotone_line_image and cylinder_affine_identity. The latter requires the exact
original harmonic-frame/height identity and literal selected u=t over its
original interval, with the retained CylinderAffineIdentityRecord. General
cylinder_arc_order is NOT emitted. A tag without its implemented producer refuses. Raw analytic pcurve coordinates are represented by the
original use + mapped-s + retained lift descriptor, never false exact rounded UV.
Sites and intervals follow ORIGINAL use traversal; literal t and mapped-s remain
stored even when decreasing. Interior samples use junction_occurrence=UINT32_MAX;
true endpoints name the original loop-adjacency junction. node_source_upper is
the maximum of all applicable ORIGINAL guards: V↔Ce, V/Ce↔S(chosenUV), and
declared alias residuals. An authoritative interval without its required
authoritative_domain_upper fails, rather than interpreting absent as zero.
A two-use seam may have literal u=0 and stored u=2pi with both shifts zero;
CoedgeSlot/loop-junction identity keeps them distinct despite common physical Q.
The authoritative_domain_upper is a physical displacement bound, not a claim
that this checkpoint constructs an onto face-domain map.

PhysicalCurveInterval rows retain spatial bounds for EVERY physical interval,
including wires with no use rows. parameter_chord_upper and image_chord_upper
retain their different correspondences. carrier_trace_to_actual_chord_upper is
the separate S(selected affine UV)↔SAME actual Q chord bound; it is not the
source_to_selected_trace_upper (Ce↔Sselected). Positional acceptance requires
the selected physical chord bound and each carrier trace bound. The Ce↔Sselected
upper may be a valid sum/coupled tightening under their compatible map and is
never compared to the nodal cap.

Every definition edge and every incident definition use comes from SourceStore
CSR. Caller overrides are optional restrictions, not an optional incidence list.
Wire edges use mandatory definition defaults; shared edge caps are the minimum
of the default and applicable incident restrictions. Node caps retain their
original meaning, separately from positional bounds; no arbitrary tol/8 budget.
Missing curves, unsupported inverse/profile, exhausted depth/work or one failed
use prevent complete successful publication. Selected branches are geometry
results, not independent runtime guesses.

## CPU/P100 procedures and finite stopping behavior

1. Prepare lazy original 1D roots (dim2/3, p<=32); canonical exact source intervals
   and original node endpoints remain in host tables. Original curve knots and
   use-map t breaks remain initial physical cuts. Pcurve knot preimages used for
   bounds are virtual exact subintervals unless explicitly requested at authority.
2. Batch pending forest samples across edges; map to numerical point keys,
   stable-deduplicate and reuse requested tier only. Evaluate actual geometry on
   CPU workers or P100. Keep original vertices fixed. Retain actual departure.
3. Bound spatial chords and all current use traces. Rational source uses the
   degree p+1 residual N-W*actualL and positive Wmin; analytic line/circle/ellipse
   uses cached original-frame M2*h²/8 plus actual endpoint departures. Geometric
   ready-but-large bounds split; they do not all trigger exact arithmetic.
4. Every current-generation participant reaches its canonical first request
   horizon or settles. Union/deduplicate split keys and promotions, then count
   every new node/control/sample/output request before mutation. Reuse settled
   unaffected results as empty arrivals; completion timing cannot select a subset.
5. Install shared changes and rerun only incident traces. Finish when the full
   current horizon requests nothing and all bounds/occurrence rows accept.
   Halving contracts smooth geometric error but cannot cure original mismatch
   or numerical floors. Promote/refuse finitely; depth or iteration limits never
   force success. No universal convergence theorem is claimed.

Root owns a **versioned CurveCudaAPI export in the existing CUDA module**, using
its existing DeviceSession lifetime machinery with mode=curve, same reservations,
one pending ticket, poll/drain and poisoned-final-owner rules. It is not another
lifetime implementation and does not invent a zero-dimensional surface root.
Curve descriptors, controls and operation packets are distinct 1D PODs. Proposed
operations: samples, paired_children, nodes, poll, stats, destroy. A paired split
creates both restrictions from one de Casteljau midpoint pyramid in O(p²), not
O(p³) repeated blossom per child/coefficient. Degree32 homogeneous nets have at
most132 interval scalars; residuals have34 coefficients. Degree buckets use
cooperative lanes/shared slabs, not a private full net per thread. CPU executes
identical restriction order and RN padding; P100 supplies real source evaluation,
restriction, chord bounds and mark/scan/compact. Host handles exact identity,
incidence and compact source/precision ambiguity. No per-edge CUDA create/free.

## Direct oracles and honest performance scopes

The parent caller's independent actual-output reader owns the single geometry
oracle: exact original polynomial/rational residuals, analytic interpolation
models, literal original V endpoints, every source edge/use, oriented exact t/s
maps and same physical key/XYZ across all incidence. CPU/GPU parity alone is not
that oracle. Include weighted p32/nonclamped/C0, narrow spike, nonuniform-speed
straight line, original endpoint offset, descending edge t, same-face cylinder
seam, shared many-face edge, wire edge, failed guide inverse and declared
collapsed-edge inventory. Real p32 buckets must run, not only compile.

Compare fresh-cold and fine/coarse/fine/repeat plus independent fresh-coarse
queries. Report original source preparation, curve cache compilation, physical
forest setup, sample/restriction/chord work, trace recovery/bounds, request rounds,
output/close and process wall separately. Report retained/cache-hit nodes and
points, selected samples, affected versus total faces, exact fallback count,
transfer bytes and actual aggregate memory reservations. In particular many
short edges may lose to CPU launch/transfer cost even with successful device
work. Many uses of one curve should show avoided source work without confusing
shared numerical geometry with welded physical identity. NVIDIA imported-source
warm is not established tree-warm. No curve-only or checkpoint result is a
complete trimmed face, component, watch mesh or GPU speedup claim.

### Restriction contender checked beyond the reference default

Read Woźny/Chudy, *Fast subdivision of Bézier curves*, arXiv2509.15691v2
(30Apr2026), §§2–5/Algorithm4.1. Their scaled convolution/FFT method improves
asymptotic subdivision to O(p log p), extends to homogeneous rational controls,
and shares a coefficient-only transform across dimensions. Their unscaled
variant has numerical instability; the scaled method still needs transform
error enclosures for our contract. For the admitted p<=32 bucket we select the
single convex midpoint pyramid: both children share intermediates, at most528
interpolation pairs per homogeneous channel and bounded shared storage. This
is a concrete finite-degree/locality selection, not an asymptotic fastest claim.
An owned certified FFT is not justified merely to replace this small operation;
the contender is retained for wider degrees and measured future crossover.
[Primary full paper](https://arxiv.org/pdf/2509.15691).

## Owner transaction refinement for implementation planning

Do not commit promotions before resolving the same horizon's new split points.
Stage guarded owner promotions without changing installed Q; existing
prepare_splits/complete_evaluations then commits new samples, retired intervals,
point replacements and incident generations in ONE owner transaction. A failed
new-point evaluation leaves the prior leased boundary untouched. The narrow
proposal is:

```cpp
struct BoundaryPointRevision {
  BoundarySampleSlot sample;
  PhysicalNodeKey expected_key;
  uint64_t expected_constructed_revision;
  Vec3 xyz;
};
void DefinitionBoundaryForest::request_owner_splits(
    BoundaryEpoch, std::span<const BoundaryKey>);
void DefinitionBoundaryForest::stage_owner_promotions(
    BoundaryEpoch, std::span<const BoundaryPointRevision>);
```

The wrapper retains numerical source/t/profile/tier/departure beside its staged
records and validates that provenance before calling the sole physical owner.
The forest checks finite XYZ, current epoch/key/revision, unique sample targets
and nonvertex eligibility, reserves the complete combined mutation, and increments
constructed revision on actual installation. Empty split batches can still commit
promotions. Both operations share the existing final complete_evaluations gate;
no second authority or completion-driven partial installation is introduced.
