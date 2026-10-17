# Implement the trimmed surface-cache runtime

Status: finite PLAN for review, 2026-10-16. Implements
adaptive-trimmed-runtime-unit.md and the reconciled construction/numerical
plans. Root owns CMake, caller, wire and original-output reader. No product code
before this plan's review. No additional research or dependency is selected.

## Owned source and dependency order

Runtime owns new adaptive_surface_view.hpp, adaptive_surface_cache.hpp/.cpp,
adaptive_trimmed.hpp/.cpp; narrow extraction of adaptive_cache.hpp/.cpp;
source_cache.hpp/.cpp prepared analytic panels; adaptive_curves.hpp/.cpp external
boundary continuation; tests/native_adaptive_surface_cache.cpp and
tests/native_adaptive_trimmed_runtime.cpp. Construction owns shared
adaptive_trim_types.hpp/regions/CDT. Geometry owns adaptive_trimmed_geometry and
HD types/numerics. Root owns TrimCudaAPI/adaptive_cuda integration, preserving
AdaptiveCudaAPI ABI1 and the same session/ticket/drain implementation.

Grounding is the DESIGN's literal pinned SmTess::BuildTree/Phase2/
AddQuadBoundaries and SmSurfaceCache::BuildTreeBase references, Balazs2004
§§5–7, Ma/Hewitt2002 §4 and Filip1986 interpolation/reconnection warning.
Packed scan/count/immutable source paths reuse already read Blelloch1993,
Karras2012, Tzeng2010 and Benyoub/Dupuy2024 contenders. The new complete-horizon
and active-grid progress procedures below are owned derivations, not attributed
to an unstated reference theorem. Numerical source bounds use geometry's
reviewed original N-WL/analytic/plane procedures, not new runtime mathematics.

## Literal shared lightweight header

adaptive_surface_view.hpp includes ONLY adaptive_types.hpp and owned values.
It must not include adaptive_trim_types.hpp, avoiding the construction-result
cycle. In namespace bgmesh::adaptive:

```
struct SurfaceFaceIdentity {
  Slot<DefinitionTag> definition; Slot<FaceTag> face;
  uint64_t source_generation, boundary_generation, face_generation, chart_revision;
};
struct SurfaceViewKey {
  SelectionKey selection;
  SurfaceFaceIdentity face;
  auto operator<=>(const SurfaceViewKey&) const = default;
};
class SurfaceBoundView { // VALUE handle, not shared_ptr alias
  shared_ptr<const SurfaceBoundViewData> data_; // opaque host data
  // private producer constructor; SurfaceCacheSession is friend
public:
  SurfaceBoundView() = default;
  explicit operator bool() const noexcept;
  shared_ptr<const PatchSelection> selection() const;
  SurfaceViewKey key() const;
  const SourceCache& source() const;
  shared_ptr<RunResource> resource() const;
  span<const SurfaceClass> classes() const; // selected-leaf order
  span<const RootSlot> prepared_roots() const; // complete current query, includes pruned outside
  shared_ptr<const FaceDomainData> classified_domain() const;
};
struct CandidateBox { LeafKey key; array<ExactRef,4> box; };
struct CandidateBoxesData {
  shared_ptr<RunResource> resource; SourceCache source; ExactTable values;
  SurfaceViewKey key; uint64_t request;
  pmr::vector<CandidateBox> boxes; pmr::vector<uint64_t> path_words;
};
using CandidateBoxes = shared_ptr<const CandidateBoxesData>;
enum class SurfaceClass : uint32_t { outside, interior, band };
struct CandidateClassification {
  CandidateBoxes candidates; pmr::vector<SurfaceClass> labels;
  shared_ptr<const FaceDomainData> domain; // classifier provenance, null for generic selections
};
```

Every LeafKey.path range addresses its containing selection/candidate path_words.
Classification retains the actual candidate lease and one ordered label per row;
wrong count/key/source/request refuses before mutation. Only the cache mints
SurfaceBoundView. Empty default handles allow record construction, never valid
operation admission. The host view remains readable after owner close; only an
active submitted operation pins a bank/module/reservation. Close drains active
operations, invalidates future resident requests and does not wait for arbitrary
external host-view destruction.

Append `uint64_t cover_revision=0` to SelectionKey. Every changed cover OR active
point-tier promotion increments it with overflow checks; source cache_revision
keeps its existing source-generation meaning. Old snapshots retain old values.

## Surface service and root-facing face API

adaptive_surface_cache.hpp includes shared view/scheduler and forwards the
construction/numerical handles where possible. Each request's span is copied
and validated before return. One active operation per session:

```
struct SurfaceQuery {
  uint64_t generation; SurfaceFaceIdentity face; SourceCache source;
  ExactTable values; span<const RootAllowance> allowances;
};
enum class SurfaceProgress { progressed, pending, needs_classification,
  selected, points_complete, facets_complete };
class SurfaceCacheSession {
  SurfaceCacheSession(shared_ptr<RunResource>,SourceCache,CacheLimits,
                      const stop_token& = {});
  void open_device(const CacheDevice&);
  void begin(const SurfaceQuery&);
  SurfaceProgress step(JobContext&);
  CandidateBoxes candidates() const;
  void classify(CandidateClassification);
  SurfaceBoundView selection() const;
  void refine(const SurfaceRefineRequest&);
  void sample(const SurfacePointRequest&);
  SurfacePointResult point_result() const;
  void bound(const SurfaceFacetRequest&);
  SurfaceFacetResult facet_result() const;
  CacheStats stats() const noexcept;
  void release_operation(); void release_query(); void cancel() noexcept; void close();
};
```

SurfaceRefineRequest retains captured view, sorted LeafKeys,
construction's PacketGridTargets and typed point promotions. SurfacePointRequest
retains view, one exact table, ordered absolute UV2 entries, demanded stages and
request ordinal. SurfacePointResult retains the request/key plus ExactTable and
PointRecords in the same order, with original owner/stage/constructed generation.
No callback or parent Budget crosses this seam. SurfaceFacetRequest retains
view, FaceConstructionResult, FaceDomainLease, FacetAllowances, copied selected
facet indices and request ordinal. SurfaceFacetResult echoes that work identity,
one exact bound per selected facet and complete typed undecided/promotion rows.
Successful per-facet source bounds do not depend on subsequent cover changes;
reuse them only while source/map/exact UV/actual XYZ/FacetKey/allowance stay the
same. Old asynchronous work never settles a new view/request.

SurfaceQuery may name a canonical NONEMPTY SUBSET of compiled roots for the
named face carrier. Validate uniqueness/ownership and do not start unused
carrier roots. PatchSession wrapper supplies its original complete root list.

adaptive_trimmed.hpp declares the ROOT-FACING complete operation:

```
struct TrimFaceQuery {
  uint64_t generation; Slot<DefinitionTag> definition; Slot<FaceTag> face;
  SharedCurveSnapshot boundary;
  ExactTable values; ExactRef positional, node_source;
};
struct TrimFaceLimits {
  CacheLimits surface; TrimFacetLimits facets;
  uint64_t regions, anchors, exact_bits, construction_work;
};
enum class TrimFaceProgress { progressed, pending, needs_boundary, complete };
struct FaceTrimResultData {
  shared_ptr<RunResource> resource;
  FaceConstructionResult construction;
  SharedCurveSnapshot boundary;
  SurfaceBoundView surface;
  ExactTable bound_values;
  ExactRef positional_upper, requested_node_source;
};
using FaceTrimResult = shared_ptr<const FaceTrimResultData>;
class FaceTrimSession {
  FaceTrimSession(shared_ptr<RunResource>,SourceCache,TrimFaceLimits,
                  const stop_token& = {});
  void open_device(const CacheDevice&);
  void begin(const TrimFaceQuery&);
  TrimFaceProgress step(JobContext&);
  span<const BoundaryKey> boundary_requests() const;
  void replace_boundary(SharedCurveSnapshot);
  FaceTrimResult result() const;
  TrimFaceStats stats() const noexcept;
  void release_result(); void cancel() noexcept; void close();
};
```

Result positional_upper is the exact requested face tau, a valid CONSERVATIVE
accepted upper after every facet passes, not a measured maximum. All facets use
that same exact upper; no vector of duplicated threshold refs is required.
requested_node_source retains the input meaning; the boundary snapshot retains
actual nodal facts. Statistics/backend observations stay outside semantic result.
Construction already retains original source/domain/metric/site/facet identities;
surface retains final cover key, boundary retains complete curve/use source facts.
No nested shared_ptr around construction's existing shared handle alias.

The caller fixes SourceCache envelope before quality queries from ORIGINAL
analytic declared finite domain / rational natural knot domain. Reject chosen
chart points outside it with a named profile error; never size it from a fine
output chain and then silently use that envelope for a coarse query. Full
period panels use prepare_trim_source_cache below. No new eager source model.

At begin/replace_boundary admit every named face interval's retained
carrier_trace_to_actual_chord_upper <= EXACT requested face tau, and every named
site's node_source_upper <= requested node cap. A mismatched snapshot is
invalid_source, not a request to pretend its boundary was built more accurately.
For this identity-domain profile that whole trace bound already covers original
S(chosen UV) versus SAME actual Q on every protected edge. A later proved
boundary-corner floor >tau therefore contradicts the captured boundary premise
and returns named source/precision unresolved. Do not split a physical interval
to cure that fixed discrepancy, privately change Q or introduce an external
physical-point promotion API in this checkpoint. Surface/B-corner demands still
promote through the cache's own shared point authority. General physical-point
promotion/nonidentity domain coupling is a future explicit capability.

## Extraction, selection and point ownership

First move existing PatchSession::Impl numerical ownership into the surface
service without changing its arithmetic, child paths, BASE replay or device
lifetime. PatchSession owns that service and its existing packet/facet wrapper.
Preserve old actual caller/CPU/P100 compatibility with one affected check, not a
new broad campaign. The new surface service stops at selected source leaves;
it never builds a rectangle PacketMesh simply to return leaves.

Insert complete candidate classification BEFORE point/control/bound generation.
Store outside branch masks as part of the current query's complete internal
partition, not geometry rows. Accepted PatchSelection is the disjoint NONOUTSIDE
partial cover. The view retains the sorted complete CURRENT prepared-root closure,
including outside-pruned branches, and query-fixed original domain provenance.
Complete face work admits exactly that domain and all carrier roots before using
the constructive clipping partition. A generic subset/externally labeled view
cannot silently authorize a complete face. No repeated rational clipped-area
sum or second coverage tree is required after this premise.
Construction's classifier handles the whole candidate horizon. Before footprint
settlement, classify new children; afterward inherit packet/band/outside roles.
Initial pinch demotion precedes fixed packet union, with no later demotion.

Reuse compact grid PointKey lookup and the SAME point-tier pool. Every absolute
request first finds the indexed canonical closed PREPARED root. If BOTH exact
root-relative coordinates are dyadic64 within the selected cap, it uses existing
Core.point and its packed SampleInput interval DAG, even when the anchor arrives
before that grid corner becomes active. Otherwise it uses the sparse full-limb
exact-UV identity and outward absolute-UV interval DAG. A noncompact point cannot
later become a finite-profile compact grid point. This source-only classification
closes both arrival orders without allowing an arrival-dependent rounded XYZ.
Absolute exact anchor UV may exceed depth64; construction's explicit exact-bit
cap and the sample owner's finite byte/work caps apply. Original/prepared indexed
lookup is rehomed from the existing curve geometry helper, never replaced by an
all-root/all-point scan. Common samples remain original-root CPU/P100 evaluations;
physical Q remains its separate shared boundary authority.

## Complete packet-target reservation and mutation

Construction produces PacketGridTarget with old FacetKey, captured SurfaceViewKey,
prepared RootSlot and canonical normalized Dyadic64 numerator/exponent per axis.
It chooses the first k=1..64 nearest-even centroid pair proved strictly inside
the old facet; no exhaustive grid enumeration. Runtime revalidates that actual
facet/source/view and target before any selected-cover mutation.

Build ONE temporary union split DAG for ALL targets plus ordinary requested leaf
splits. At each affected selected leaf, compute the MAXIMUM still-needed target
denominator depth over all its targets for U and V (already-endpoint axes need
zero), choose the greater maximum (U tie), retain BOTH children and route each target through
one canonical closed-containing child (lower child on a split-line tie).
Do not impose 2:1 balancing or refine both incident sides merely to make a
hanging point: the existing common side schedule propagates its active incidence.
Stop a target path when both coordinates are rectangle endpoints. Multiple
targets sharing a path/point/control payload reserve it once. A target already
cached but not active still requires this selection path.

Forced target axes take priority over ordinary approximation heuristics and
affine_separable_axes: even a plane must acquire the requested active interior
corner. Ordinary source-bound failures resume in the next complete horizon;
no geometry-preference flag blocks a construction-required target path.

Preflight unique node metadata, child controls, exact new point keys, frontier,
host output/table bytes, device append capacities and bounded child scratch for
the COMPLETE DAG. Reuse same-stage children without letting warm history choose
a different split axis/path. Acquire reservations before installs. Failed
preflight leaves selection and point revisions unchanged. Execute restrictions/
sample/bounds in dependency waves, fold canonically and retain required targets
across ordinary source-bound refinements. Those later unknown refinements get
their own complete preflight horizon; there is no claim to pre-reserve an unknown
entire future adaptive tree. Publish a new view only when every target is ACTIVE.
Construction then verifies each target in the rebuilt common packet schedule.
Cap/depth/no-target/path failures return unresolved, never unchanged-facet success.

Band requests separately stage arbitrary-word strict-interior anchors and cache
splits, then one rebuild per affected connected component for the whole horizon.
All requested samples must complete before construction commit. Old immutable
results remain readable on cancellation; poisoned post-install owner state cannot
produce a successful current face.

## Actual-facet execution and whole-face continuation

Use the numerical plan's literal prepare_trim_facet_work with current minted
view, actual construction, domain, exact face tau and changed facet indices.
Initial request includes all facets; subsequent requests include revised packet/
component facets only. Pack base unresolved pieces and direct original plane
rows; failed base pieces receive direct original SourceTriangleInput tasks and
then the finite demanded precise route. Never replace SAME L or infer a floor
from interval width alone. Actual physical nonzero is exact-filtered before
success, including tiny binary64 facets whose raw cross products underflow.

CPU uses private JobContexts/disjoint output ranges. P100 uses existing
AdaptiveCudaAPI facets and root's TrimCudaAPI source_triangles on the SAME live
DeviceSession; plane tasks use that source operation. No percell open/close or
second allocator/poll/owner. Geometry's cooperative common coefficient helpers
stage one p/q<=3 source slab and triangular net per warp, not per thread.
Native retained work owns original source/view/construction/allowance tables;
host envelopes echo full request/chunk/stage keys before completion admission.

Complete all requested facets before returning a construction horizon. Geometry
provides exact comparisons; runtime max-folds all positive pieces and handles
exact tau equality without rounding it above itself. A conservative upper
failure is undecided, not witnessed infeasibility. Typed actual point authority
and B-corner authority remain distinct. Original fixed vertices are immutable.

FaceTrimSession step order: prepare_face_domain -> classify/select source cover
-> initial packet footprint -> build_face_regions -> bound changed facets ->
plan_construction_refinement -> complete reserve/sample/cache/region rebuild.
Only complete source/domain/bounds/topology yields result. Requests for original
BoundaryKeys yield needs_boundary, not an evaluator callback or partial result.

Add `AdaptiveBoundarySession::refine(SharedCurveSnapshot expected,
span<const BoundaryKey> requests)`. It is a continuation of the SAME query
forest/allowances, not begin(newquery). Validate retained expected snapshot,
source/query/generations and all keys before sole guarded owner mutation; count
and reserve the complete horizon, run existing physical and trace folds to a
new complete result. No new physical authority. FaceTrimSession replace_boundary
requires the same original source/definition/query, strictly current generation
and settled snapshot, then restarts this incident face's domain/footprint/
construction. Numerical source caches persist. Retain no transient old node
slot without source-key remapping. The caller owns this explicit loop.

## RED, GREEN and reviewed completion

1. Author tests/native_adaptive_surface_cache.cpp against missing service/view
   API before bodies; record actual missing operation RED on the isolated fleet.
   Publish agreed lightweight headers promptly after RED. Extract owner and
   preserve actual old patch caller behavior with the affected CPU/P100 check.
2. Add real source-backed candidate masking/tiny-hole fixture. Outside child
   geometry counters remain zero; nonoutside selected cover is real. Refine,
   promote and reject stale/partial/wrong-owner views/completions before mutation.
3. Direct packet target cases: a skinny old zipper triangle survives arbitrary
   split but the forced target becomes active, including warm inactive sample,
   shared target paths, exact cap refusal and multi-target preflight failure.
   Band exact anchor alias creation orders share one point-tier record. Actual
   original-source point models check XYZ/departure separately from parity.
4. Real domain/shared-boundary -> regular cache -> packet+band constructor ->
   native facet work -> complete result and boundary-refine/replace continuation.
   No authored PASS geometry callback. Root's actual tiny-hole/full-face reader
   checks complete topology and original whole-facet error CPU1/4/both P100s.
5. Scope sanitizer/concurrency tests to new service lifetimes/worker paths,
   cancellation, old-view lifetime and failure reservations. Existing device
   owner protocol remains shared; root's affected source-triangle device tests
   attack that new operation. Actual-source peers review ownership, numerical
   routing and construction horizons; resolve findings before coherent commits.

Use fleet cached-adaptive-mesher builds adaptive-trimmed-runtime-release/asan/
tsan; no local product execution. Root registers libraries
brepgpu_mesher_adaptive_surface_cache and brepgpu_mesher_adaptive_trimmed plus
the two fixtures after files exist. Root owns exact argv fixture integration.
Measure avoided outside work, target DAG/point reuse, candidate/piece/source
fallback counts, component rebuilds, transfer bytes and complete cold/prepared
CPU/P100 costs on the fixed caller cohort. Keep unsupported/censored results;
no watch retry or another broad certification campaign in this unit.

## Coordinator cancellation and total construction work

Actual source review found that coordinator cancel reached the surface service
but not the persistent classifier's private Budget. The coordinator now owns a
stop source; constructor and per-step caller tokens forward into it, and both
classifier and surface service retain that token. Per-step scratch contexts use
the remaining query construction allowance. Their work is charged to the caller
and accumulated across construction horizons and boundary replacement. Classifier
construction work and classify/classify_points deltas are included; each private
classifier operation temporarily limits its SAME Budget to the remaining outer
allowance. A failed compute step cancels the owner and cannot later publish.

This follows the existing NativeTask scratch lifetime, metered Budget and
stop-token protocol; it adds neither an evaluator nor a geometry checker.
The existing real-work fixture reuses its original source and boundary for
inactive-step refusal, caller/external cancellation, absent partial result and
actual tiny construction-budget refusal. Its geometric output still goes to
the same independent reader. Relevant source review and ASan work run close this
seam; prior service-only cancellation checks are not substituted for it.
