# Implement cached trimmed-face construction

Status: finite implementation PLAN for review, 2026-10-16. Implements the
reviewed choices in adaptive-trimmed-construction-unit.md; no product code yet.
Construction owner owns one complete exact region/anchor invariant. Runtime and
numerics own their actual source-cache execution/bounds. Root owns caller/CMake.

## Owned files and dependencies

Construction: new native/mesher/adaptive_trim_types.hpp,
adaptive_regions.hpp/.cpp and adaptive_region_cdt.hpp/.cpp; narrow extension
native/direct_cdt/core.hpp/.cpp for typed directed regions; extract reusable
segment admission/index mechanics from mesher/domain.cpp into
mesher/domain_index.hpp/.cpp without a second implementation. Existing old
single-outer domain path calls the same index and unchanged public CDT wrapper.
No MutableFaceTopology/LEPP dependency in the new region owner.

Tests: tests/native_adaptive_regions.cpp plus extend the existing independent
exact chain reader through tests/native_adaptive_regions_oracle.py; its functions
reuse tests/face_domain_oracle.py rather than clone its geometry predicates.
Reuse root original source fixtures or add authored records to the same source
fixture generator. No root CMake edits from this lane. No code until plan review.

Grounding per mechanism: design source table names NVIDIA AddQuadBoundaries,
TraceLaminaLoop/TriangulateSingleFace; OCCT protected wire/index paths;
Balázs2004 §§5–7 and Ma/Hewitt2002 §4. Existing owned core's D&C/recovery/right
flood is the actual chosen algorithm. Euler derivation, fixed Gram quality
proposal and strict-point progress are owned derivations, explicitly not
unexamined reference guarantees.

## Literal common values: adaptive_trim_types.hpp

Namespace bgmesh::adaptive. Existing SourceStore, SourceCache, ExactTable,
SharedCurveSnapshot, PointKey, LeafKey, SelectionKey and BoundaryKey remain.
All exact refs below address the one values table of their containing record.
No CUDA kernel includes this C++20 retained-lease header.

```
struct FaceConstructionKey {
  uint64_t source_generation, query, boundary_generation;
  uint64_t chart_revision, profile_revision, construction_revision;
  Slot<DefinitionTag> definition; Slot<FaceTag> face;
};
enum class DomainPolicy { reconstructed_selected, authoritative_exact_image };
struct WholeUseLineImage {
  Slot<CoedgeTag> use; Slot<CurveTag> raw_curve;
  array<ExactRef,4> endpoints; // exact ORIGINAL A.x,A.y,B.x,B.y
  array<ExactRef,2> covector, scalar_endpoints;
  Range<UseCurveSite> chosen_sites; // retained curves->sites, whole use
  int32_t raw_traversal; // actual map/lift remains in original source
};
struct FaceDomainUse { Slot<CoedgeTag> use; DomainPolicy policy;
                      optional<WholeUseLineImage> image; };
struct FaceDomainData { resource; SourceStore source; SharedCurveSnapshot curves;
  FaceConstructionKey key; ExactTable values; pmr::vector<FaceDomainUse> uses; };
using FaceDomainLease = shared_ptr<const FaceDomainData>;

enum class FacetGenerator { packet, band };
struct FacetKey { FacetGenerator generator; uint64_t owner, revision;
                  uint32_t ordinal; };
// Full identity is (retained FaceConstructionKey, FacetKey), stored once/header.
struct BoundarySiteSource { uint32_t boundary_node; };
struct SurfaceSiteSource { OriginalRootSlot original; ArithmeticStage stage;
                           bool anchor; uint64_t birth_horizon; };
struct ConstructionSite {
  array<ExactRef,2> uv; array<double,3> actual_xyz;
  uint64_t constructed_revision;
  variant<BoundarySiteSource,SurfaceSiteSource> source;
};
struct SourceOccurrenceBinding { uint32_t shared_site, construction_site; };
// COMPLETE curves->sites bindings preserve use/joint/branch/lift occurrences.
// Only loop-adjacent named junctions unify; physical node alone never unifies UV.
struct RegionFacet { FacetKey key; array<uint32_t,3> sites;
                     uint32_t component; }; // UINT32_MAX for packet
struct RimIntervalKey { uint64_t footprint_side, revision; uint32_t ordinal; };
struct ProtectedRegionEdge { array<uint32_t,2> sites;
                            variant<BoundaryKey,RimIntervalKey> provenance; };
struct RegionLoop { bool positive; Range<uint32_t> sites;
                    Range<ProtectedRegionEdge> edges; };
struct RegionComponent { uint64_t id, revision;
                         Range<RegionLoop> loops; Range<RegionFacet> facets; };
struct FaceConstructionResultData {
  resource; SourceStore source; SourceCache surface_source;
  SharedCurveSnapshot curves; FaceDomainLease domain;
  SurfaceBoundView surface; PacketMesh packets; FaceConstructionKey key;
  ExactTable values; array<ExactRef,4> fixed_metric_factor;
  pmr::vector<ConstructionSite> sites;
  pmr::vector<SourceOccurrenceBinding> source_bindings;
  pmr::vector<uint32_t> loop_sites;
  pmr::vector<ProtectedRegionEdge> edges;
  pmr::vector<RegionLoop> loops;
  pmr::vector<RegionComponent> components;
  pmr::vector<RegionFacet> facets;
};
using FaceConstructionResult=shared_ptr<const FaceConstructionResultData>;
```

`resource` fields mean shared_ptr<RunResource>. Containers use that resource;
source/surface/domain generations and actual retained leases are checked before
reading refs. SurfaceBoundView lives in runtime's shared lightweight
adaptive_surface_view.hpp, not cache implementation. Bound work derives UV6/XYZ9
from RegionFacet.site indices once; no duplicate XYZ or complete envelope per
virtual piece. Source authority requests derive from retained physical key or
exact surface point UV/stage/revision; no coordinate callback or hidden evaluator.

Rim side IDs are assigned canonically to the initial frozen geometric footprint.
Their geometry stays fixed; added atomic samples change side revision/ordinal,
not its original source identity. Component IDs are assigned in canonical initial
minimum directed-edge order and persist during local rebuild. Facet ordinals
are canonical oriented triples within each current generator revision. These
are semantic generation-local IDs, not vector-allocation addresses/source IDs.

## Literal host operations and runtime handoff

```
prepare_face_domain(DomainRequest,JobContext&) -> FaceDomainLease | BoundaryRequest
classify_candidates(FaceDomainLease,CandidateBoxes,JobContext&)
    -> CandidateClassification
build_face_regions(RegionInput,JobContext&) -> FaceConstructionResult
plan_construction_refinement(RefinementInput,JobContext&)
    -> ConstructionRefinement
rebuild_face_regions(RebuildInput,JobContext&) -> FaceConstructionResult
```

DomainRequest retains complete shared curves, source/face key and exact
allowances. Geometry supplies original line-image primitives; construction
matches complete use/site/joint inventory and produces the non-null policy.
General missing maps refuse explicitly; no boundary upper becomes a face map.

RegionInput retains domain, minted SurfaceBoundView, PacketMesh, source cycle
adapters and any retained anchor values. Surface result MUST be the classified
current face view, not an unrelated complete PatchSession result. PacketMesh
must retain its same selection. RebuildInput additionally retains prior result,
complete new surface/rim view and sampled anchors/promotions. It names the
complete affected component set. Unchanged immutable components may be reused;
no old exact refs address a new table without canonical copy/remapping.

ConstructionRefinement retains input result/key and contains sorted requests:
BoundaryKeys; failed FacetKeys; LeafKeys; BandAnchorLocator; PacketGridTarget;
point authority promotions. BandAnchorLocator names old facet, containing retained source leaf
and exact chosen arbitrary-word UV. PacketGridTarget names old facet,
prepared RootSlot and normalized Dyadic64 numerator/depth per axis. It also
retains the original SurfaceViewKey and original source lease.

Packet target: normalize old facet centroid to prepared root, try k=1..64 exact
nearest-even dyadics, choose first strict-interior valid pair. Runtime constructs
one union split DAG for ALL targets before mutation. Within a target's selected
leaf, split greater remaining denominator depth (U tie); choose canonical child
at a closed split-line tie. Keep companions. Stop at target corner; ordinary
side schedules propagate hanging incidence. Require target NEW ACTIVE in rebuilt
packet output; inactive cached numerical existence is irrelevant. No capped
point/path gives unresolved. BAND anchors instead permit arbitrary-word exact
UV. Round the ACTUAL parent centroid and require every exact barycentric
coordinate >=1/6. Locate one containing leaf through retained overlap-piece
indices (plane shortcut: metered retained-leaf scan), split that leaf, sample
the central point and rebuild its component. Do not take a first clipped-piece
centroid or split all crossed leaves. A direct star has child area <=2/3 of
parent; no global CDT contraction is claimed. See the DESIGN central-anchor
amendment for the owned barycentric derivation.
No bound-failure packet demotion or new packet triangulation API is introduced.

## Exact admission/CDT procedure

Extract current SegmentIndex to accept borrowed exact coordinate views plus
original segment locations, not a MutableFaceTopology implementation callback.
Preserve closed contact rejection, grouped exact Y-prefix winding and work counts.
Use this ONE implementation for old domain and new directed regions.

Initial source graph preserves every use and endpoint-joint occurrence. Return
coarse DomainIssue locations to the existing real boundary-request classifier.
Admit normalized original loops and reversed packet rims as disjoint simple
cycles. Positive loops require other-winding0; negative loops1. All strict
interior anchors require total1 plus no contact. Duplicate chart coordinates
from unconnected occurrences are issues, not welds. These indexed queries charge
actual candidate work and do not claim universal log-time point location.

Build fixed FACE Gram factor from canonical accepted band/covered leaf midpoint
actual B derivatives. Use exact positive G and owned exponent-safe sqrt53/round53
Cholesky arithmetic. Expose only that arithmetic from metrics.cpp to a lightweight
exact-metric helper; do not link original_source_metric/GeometryStore preparation.
Transform all sites with SAME T; common-D clearing preserves full metric.

Add typed solve_directed_regions(predicates,budget,boundary_slots,loops,
RegionInventory{boundary_sites,positive_cycles,negative_cycles},seed,stats).
Existing solve wrapper supplies its B=N,C=1 assumptions unchanged. Recover ALL
constraints; right void flood; assert left retained/right void, all sites used,
positive output, checked T=2N−B+2H−2C. Initial retained dual BFS labels components;
loop left incidence assigns their component. Local rebuild reuses admitted cycles
and accumulated anchors under the fixed metric. Seed/site order is canonical.

## Direct RED, implementation order and bounded closure

1. Author real source→shared curves→selected source view fixture and missing
   operations RED. Literal topology cases use real retained source leases, not
   fake successful numerical services. Register source/test files only when they
   exist; root owns CMake targets adaptive_regions and native_adaptive_regions.
2. Extract/reuse exact index; RED new interior/multicomponent inventory. Cases:
   disjoint outers, outer+hole, nested alternating packet islands, collinear
   protected samples, strict interior anchors, outside/on-edge/duplicate anchors,
   contacts and exact non-dyadic/common-D shear. Existing old domain cases remain.
   The SAME independent Fraction chain reader checks actual triangles, every
   atomic boundary incidence, positive embedding/count and anchor incidence;
   no pairwise second certifier or expected-diagonal snapshots.
3. Implement domain inventory/whole-use identity producer, classifier, packet rim
   reversal and first global band CDT; freeze one complete selected face result.
   Direct oracle includes tiny contained hole, many aligned holes, reversed loops,
   actual source period seam and exact diagonal nonlinear-speed identity. Missing
   map and unsupported profile cannot emit partial success.
4. RED true construction progress: skinny packet triangle surviving arbitrary
   one-axis split must activate a capped strict-interior grid target; band old
   triangle must gain an arbitrary-word central anchor with barycentrics >=1/6. Two failures sharing target
   or leaf reserve once and rebuild affected component once. Existing cached but
   inactive sample must become active. No target under cap returns unresolved.
   Integrate actual runtime sampling/bounds, not supplied PASS placeholders.
5. Actual failing curved band facet→same-source tightening→anchor/split→new CDT
   completes; original analytic plane dispatch remains a direct fast path. Exact
   old/new facet identities, stale key/cover/physical revision, capacities and
   cancellation attack ownership/failure classes. Root extends ONE whole-output
   reader/caller for original whole-facet error and CPU/P100 semantics.
6. Fleet strict source/build, scoped ASan/UBSan/leaks and actual-source tidy;
   runtime concurrency and CUDA tests stay their owners' distinct fault classes.
   Independent actual-source reviews resolve findings before coherent commits.
   Stop affected checks after finite claims pass. No watch/perf escalation here.

First meaningful integration returns COMPLETE selected face, not just a polygon
or clipper. Source admission/supported policies/failed outcomes remain explicit.
Measure N/B/C/H/anchors, packet/band fraction, indexed candidates, CDT recovery,
common-D bit growth, clips, complete horizons and per-component rebuilds. Complete
cold/warm CPU1/parallel/P100 comparisons belong root caller scope and actual costs,
not a numeric speed target or another certification campaign.

## Actual interface refinements during implementation

`RegionInput` carries ALL sampled anchors, births and component IDs. The IDs are
known from the failed old facet; a boundary restart uses the initial global
constructor again. `RegionLoop` retains optional original source-loop identity
and traversal adapter. `RegionFacet.leaf` is a current selected-leaf locator,
qualified by the retained view; `FacetKey.owner` is a persistent packet identity
matched by complete source split path. Actual site/circuit changes advance its
revision. `changed_facets` lists only new/changed output indices; first output
lists all. Unchanged band components and packets retain prior acceptance.

`freeze_packet_footprint(PacketMesh,SurfaceBoundView,Budget&)` retains the initial
accepted cover roles and full face identity. `apply_footprint` consumes the raw
persistent-classifier result: outside stays outside, packet descendants remain
interior, and nonoutside band descendants remain band. Exact split-path prefix
lookup does not rebuild the source index.

Whole-use original line-image matching is shared with `adaptive_traces`. The
explicit per-use descriptor carries original endpoints/covector/scalars and
traversal. It does not identify individual mapped-s subintervals with selected
subsegments. Under that whole-use profile the per-interval authoritative domain
upper is NULL; it is never rewritten to a false flattening zero. Original node
obligations and honest interval UV component uppers remain separate.
