# Seam-wedge cap construction implementation plan

2026-10-17. DESIGN + literal PLAN for review. NO product edits before holistic
root/runtime/numerical agreement. Read `papers/native-adaptive-cap-construction.md`,
the geometry plan and runtime unit together; these are one operation, not three
independent fallback meshers.

## Ownership and implementation boundary

Construction owns new `adaptive_cap_domain.hpp/.cpp`, narrow `adaptive_traces`
and shared-curve wedge transport, `adaptive_trim_types` source-map fields,
`adaptive_regions` partition/classifier/fan construction, and one focused native
fixture plus extension of the existing original-output reader. Runtime owns
Surface/Face session keys, classification capability transport, replace_domain,
cap tasks and continuation orchestration. Numerical owns source-only admission,
symbolic progress, RN/precise cap math and quotient source purpose. Root owns
Component continuation storage, CUDA extension, wire/display adaptation, CMake
and final whole caller. No edits to scene files during root's current closure.

Grounding for each mechanism is the paper's pinned function list: UV sector/
connector from SmTess MakePolygon; endpoint authority from OCCT Tessellate3d/2d;
cache+remaining-band architecture from Phase2CreateFacePolygons; bounds from
Filip et al plus the owned whole-cap derivation. Exact partition/incidence and
the BOTH-side state machine below are owned derivations, not reference claims.

## Lightweight common schema

`adaptive_cap_domain.hpp` includes common source/curve/cap POD/exact types, no
session or `adaptive_trim_types.hpp`. Forward declarations break dependency
cycles. Names below are selected interface proposals, not product declarations.
Define SeamWedge in the existing `adaptive_curves_types.hpp` BEFORE the snapshot
that owns its pool (including only the lightweight cap POD enum). Cap-domain
imports that row; shared curve types never include the full cap-domain header.

```cpp
struct CapDomainKey {
  uint64_t source_generation, query;
  Slot<DefinitionTag> definition; Slot<FaceTag> face;
  uint64_t boundary_generation, face_generation, chart_revision, domain_revision;
  uint64_t assignment_epoch, profile_revision;
};
struct SeamWedge {
  Slot<VertexTag> vertex; Slot<LoopTag> loop;
  std::array<Slot<CoedgeTag>,2> meridian_uses;
  std::array<uint32_t,2> shared_endpoint_sites;
  AnalyticCapKind kind;
  std::array<ExactRef,4> finite_connector_uv;
};
struct PhysicalCapEndpoint {
  Slot<EdgeTag> edge; Slot<CoedgeTag> sector;
  ExactRef original_t; PhysicalNodeKey expected_key;
};
struct SurfaceCapPoint { OriginalRootSlot original_root; };
struct CapPointPlan {
  std::array<ExactRef,2> uv;
  std::variant<PhysicalCapEndpoint,SurfaceCapPoint> authority;
};
struct CapDomainPiece {
  uint32_t cap; OriginalRootSlot original_root;
  std::array<uint32_t,2> cut_points;
  std::array<ExactRef,4> cut_uv;
  std::array<ExactRef,2> finite_v;
  std::array<uint32_t,3> positive_chart_roles;
  int32_t regular_sheet_sign;
  BoundaryKey cut_key; // kind face_cut; no original coedge association
};
struct CapDomainRow {
  uint32_t wedge;
  std::array<uint32_t,2> angular_to_sector; // increasing U -> original source pair
  uint32_t pole_boundary_node; // qualified by captured curves->boundary
  Range<CapPointPlan> points; Range<CapDomainPiece> pieces;
  uint32_t transcendental_bits = 96, norm_fractional_bits = 96;
};
```

`expected_key` exists only for an already settled endpoint. A missing midpoint
is represented by a BoundaryKey request, never by inventing a future key.
`SurfaceCapPoint` at prepared-panel boundaries uses the common original carrier
point key; no winner-leaf point evaluation. Both seam endpoints carry different
sector/UV and the SAME original_t/key/current Q.
SeamWedge retains incoming/outgoing ORIGINAL occurrence order. CapDomainRow
canonicalizes its angular endpoints to increasing U and retains the permutation;
no assumption that original source traversal increases U. Per-piece finite_v
is the exact interpolation of the full connector at THAT piece's two angular
coordinates, not a copy of full-cap endpoint V into every piece. Cap rows/points/
pieces are flat PMR pools with checked ranges, one cap initially and explicit
empty pools for regular faces. The pole row may name current captured physical
authority before cut Q exists; no new pole sample is requested.

`CapCutControlLease` is immutable and retains resource/source/query/definition/
face/profile identity, exact table, prior cutUV4, two physical endpoint locators,
angular level, cut revision, pending-radial mask and optional exact fixed metric
factor4. It carries no Q, mutable Budget, cache page, unfinished construction,
resident handle or global snapshot row offset. Origin boundary/assignment are
provenance; resumed authority comes from the new validated snapshot.

`ProducedCapDomainLease` is a copyable value handle to immutable private data;
only the source-validating producer can mint it. Accessors expose key(),source(),
curves(),values(),wedges(),caps(),points(),pieces(),control(). Data retain SAME resource,
SourceCache, SharedCurveSnapshot and sealed exact table. No authored flag or public
data constructor can mint admission. A face in a quotient bank with zero wedges
gets an explicit source-checked empty plan. `FaceDomainData::caps` stores this
handle; use one spelling consistently, not a second public mutable CapDomainData.

SharedCurveSnapshot appends a wedge row pool using its values table; runtime
freeze remaps every wedge ExactRef and occurrence index. `prepare_trace_sites`
preserves both recognized wedge UVs/carrier samples and constant-U proposals;
ordinary joints keep existing merging. No duplicate source-use inventory.

`FaceConstructionKey` appends independent face_generation/domain_revision;
SurfaceFaceIdentity appends domain_revision. CapDomainKey matches all counters,
exact source/cache lease and physical snapshot. Local cut: face/domain advance,
boundary unchanged. New boundary: face generation increases monotonically;
assignment epoch remains a separate stale-work guard. Construction revision is
connectivity revision, not a domain/source revision.

The full selected boundary graph inserts a typed
`CapConnectorKey{wedge,domain_revision}` provenance variant on its coverage-only
edge. Extend DomainBoundaryEdge provenance rather than filling a counterfeit
BoundaryKey. Source edges retain source BoundaryKeys; the carved ordinary graph
uses actual face_cut BoundaryKeys on cut intervals. Connector issues route to
this cap producer/profile, never the physical forest. Original loop/coedge
inventory is still the original three uses, separately from construction cycles.

## Finite producer and binding API

```cpp
struct CapDomainRequest {
  SourceCache source; SharedCurveSnapshot curves; CapDomainKey key;
  ExactTableKey table_key; CapCutControlLease continuation;
  uint32_t transcendental_bits = 96, norm_fractional_bits = 96;
};
struct CapDomainPreparation {
  ProducedCapDomainLease plan; // empty if source requests remain
  CapCutControlLease continuation;
  std::pmr::vector<BoundaryKey> boundary_requests;
};
CapDomainPreparation prepare_cap_domain(const CapDomainRequest&, JobContext&);
// In adaptive_regions.hpp, which already imports full SurfacePointResult:
struct BoundaryCapBinding { uint32_t node; };
struct SurfaceCapBinding { uint32_t point; };
struct CapPointBinding { uint32_t plan_point;
  std::variant<BoundaryCapBinding,SurfaceCapBinding> source; };
// RegionInput retains domain + current cut_points SurfacePointResult for interior
// cut points; producer derives bindings, callers cannot author arbitrary Q.
```

A binding is qualified by RegionInput's captured domain/boundary snapshot or
current cut-point result lease, exact UV, actual Q and revision;
matching exact value/source identity is mandatory before building facets.
Root/runtime can derive an immutable cut-point request from `plan.points()` and
batch only SurfaceCapPoint rows through SAME SurfacePointRequest. There is no
sampler callback. `prepare_face_domain` accepts the produced plan and returns
full selected domain + ordinary construction loops; its original source loops
remain distinct and complete. Classification owns the same persistent index.

Numerical API agreed with owner:
`admit_cap_map(prepared,cutUV4,finiteV2,precision,Budget)` returns symbolic pole,
sheet sign and positive roles without fake XYZ. `cap_cut_progress(prepared,
oldCut4,newCut4,precision,Budget)` returns two `CapProgressState` values:
proved/not_contracted/unresolved. Only proved clears a pending side; the others
are not source invalidity. No new transcendental evaluator in construction.

## Construction/classification and facet records

1. Validate original one-loop/three-use incidence, repeated nondegenerate edge,
   opposite use senses, common pole vertex and original opposite vertex. Every
   original use/site/t interval stays accounted for. Require reconstructed policy
   and one uniquely admitted analytic pole/regular sheet.
2. Choose initial pole-adjacent strict interior master sample; missing means one
   canonical real BoundaryKey request. Do not emit a partial plan/mesh.
   Identify the pole side by original endpoint/physical key in the captured edge
   chain. Preserve literal Edge.parameter_range direction; do not reapply
   curve_same_sense or infer pole side from the increasing numerical t value.
3. Use actual per-sector chosen UV, exact nonzero same-sign finite heights, strict nonempty
   regular remainder and exact selected graph. Retain connector as construction
   coverage edge, never source edge. Carve finite trapezoid with linear cut.
   Require the WHOLE opposite-edge selected chain strictly on the regular side
   of that cut and no source graph contacts/crossings. Three-use incidence alone
   does not prove this geometric partition.
4. Produce base angular partition including every prepared U-panel boundary,
   source-derived width cells, dyadic levels and exact cut point plans.
   Panel intersections use exact rational xi=(U−u0)/(u1−u0) and V=g(xi).
   They need not be dyadic. Existing SurfaceCacheSession::absolute_point accepts
   arbitrary Rational refs and its sparse original-UV index; existing common-D
   region CDT accepts the same coordinates. Do not round these V values or
   relabel them compact grid points. Within each base cell use exact midpoint
   subdivision until source width admission, then uniform angular child levels.
   Run source-only admit_cap_map on EACH resulting angular piece, not the full
   possibly2pi wedge: its explicit source contract admits width<=pi/2. All pieces
   must return consistent source pole/sheet/positive role conventions. No Q is
   needed to perform this admission.
   Endpoint source authority is physical; interior source authority is Surface.
5. Classifier labels outside / ordinary interior / ordinary band / mapped cap,
   with complete piece overlap ranges. Mixed boxes stay band and ordinary work
   requires whole-box local regularity; refine only as needed, not to exact cut
   alignment. Mapped-only boxes do not create ordinary corners or get erased.
   Immutable CapClassificationLease retains the operation/domain/candidates.
   Reuse the full selected graph's persistent index for material membership;
   cap membership is exact convex-trapezoid/linear-cut intersection, not another
   containment tree. Ordinary construction loops are the cut-carved graph used
   by directed CDT. Cache leaf lines never produce private meridian endpoints.
6. Existing packet builder and directed band CDT consume only ordinary remainder
   and the SAME cut chain. Fixed face metric is the existing canonical first
   admitted ordinary-leaf secant metric, retained across local domain changes.
   Cap-only/empty remainder is unsupported in this first implementation.
7. Add `FacetGenerator::mapped_cap`. `RegionFacet` appends optional
   `MappedCapFacet{piece,source_role_to_facet[3],normal_sector}`. Sites are actual
   used mesh sites; the pole is BoundarySiteSource at captured original vertex,
   with one locator UV but all original source-sector bindings retained. Append
   optional `WedgeSectorBinding{wedge,sector}` to SourceOccurrenceBinding: only
   this explicit source-qualified case allows the raw occurrence chosen UV to
   differ from the bound pole site's locator UV. Ordinary bindings keep exact
   UV equality. Piece
   map refs, not pole locator UV, carry numerical coverage. Nonzero physical
   facet checks use actual Q; source orientation applied once at publication.
8. Cap cut edges and meridian spokes carry explicit protected provenance.
   Meridians use actual current source BoundaryKeys; cut edges use face_cut keys
   stable within the cut revision. Connector has neither. Changed mapped facets
   enter changed_facets on map/Q/revision changes. Same cap cut site indices/Q
   appear in ordinary CDT and fan, preventing cracks without another checker.

`MappedCapFacet.normal_sector` names kind/sheet/angular piece. It eventually
drives an explicit analytic limit source-normal convention (cone midpoint angular
sector, sphere pole limit). Root's scene/wire rejects until that finite descriptor
is implemented; no ordinary-UV proxy evaluation. New source maps require versioned
artifact records and extension of the SAME coverage reader, not silent wire reuse.

## Complete failed-horizon continuation

Fold ALL ordinary+cap answers first. Ordinary failures keep current central-anchor/
forced-grid algorithms. A mapped failure selects angular vs radial from removable
bound terms, angular on ties. Angular updates level/revision/domain, same physical
boundary. Radial records oldcutUV and BOTH pending bits, requests actual nearest
pole interval. Resume validates source/query/face/profile/exact quality, rebinds
every sample by source t/use/key, and compares BOTH gaps through geometry.

One-sided improvement cannot clear the horizon. Keep old comparison values,
request next pole-adjacent split, and return needs_boundary with immutable
continuation ONLY. Once BOTH prove3/4, commit the CURRENT pole-adjacent sample as
cut; never skip newly inserted source nodes under an old published fan. Root
captures continuation before pooled retirement and passes it with next assignment.
Completed faces also capture control+metric before retirement: a neighboring
face may later refine their shared source boundary. `CapCutDirection::retain`
seals that capture WITHOUT advancing level/revision or creating a radial request.
On external-boundary resume with pending mask0 adopt the current pole-adjacent
master cut and keep angular level/metric; require BOTH3/4 only for a pending radial
failure, not for a fictitious fresh history. Root stores the continuation for the
whole current query on complete AND needs_boundary. Regular zero-cap faces return
no cap continuation.

Fresh query starts canonical base cuts. Checked angular/depth/work/piece counters
and unresolved source/profile floors bound inability to progress. Fixed upper
terms are conservative diagnostics, never proofs that source is infeasible.

## Direct RED / GREEN order and independent oracle

Before bodies, author source→shared traces→produced plan→real view/PointPool→
construction call; capture missing declared operation link RED on fleet. Use
existing cap fixture generator/source formula where compatible, extending it
with THREE real uses/repeated meridian, not old fictitious collapsed use.

Preselect north sphere, south sphere, cone; one cone has genuinely variable-U
rational reconstructed guide. No source-ID dispatch. Direct cases attack:

* two wedge chart sites stay distinct while vertex/current Q/key remain shared;
  whole original three-use inventory unchanged; no fake coedge or source interval;
* missing interior master cut returns real request, no private endpoint sample;
  settled cap+ordinary share all cut nodes bitwise; exact source-role permutation
  and source orientation once; cap n facets for n cut intervals;
* literal one-sided candidate history cannot commit; retire/reassign across TWO
  epochs and require BOTH3/4 before commitment; stale source/query/profile/quality,
  assignment and current-point revisions reject without changing accepted state;
* angular continuation changes face/domain but not physical boundary counter;
  variable-U proposal still executes original V/Ce/S guards; deliberately bad
  source guard refuses, no tolerance widening or ideal-pole substitution;
* actual source map/SAME L compared by existing independent original sphere/cone
  formulas at finite connector, cut and interior rho, including nonorthogonal
  large-Z whole-band regression. Feed each cap's finite reference QUADRILATERAL
  (two connector corners, two cut corners) to the SAME exact atomic-chain coverage
  oracle alongside ordinary triangles. Do not count the pole's locator-UV
  triangle as its source region. Quad lower corners both use actual Qpole in L;
  the physical mesh still has one triangle. Reference connector edges remain
  explicit construction-only boundary, and physically collapse with no invented
  source coedge. SAME incidence engine checks internal angular sides/cut reverse
  incidence; no second general intersection certifier;
* ordinary zero-cap legacy compatibility; unsupported authoritative curved,
  unadmitted sheet/empty remainder and bounded control exhaustion refuse visibly.

Run focused CPU1 construction/source cases first. After source peer review use
the SAME finite integrated operation CPU4/bothP100 placements as root schedules;
geometry's independent kernel reader is reused, not repeated by this lane.
Sanitizer/concurrency scope is new plan/control/binding lifetime only if selected
by root. No watch/performance cohort or alpha claim in this unit.

Implementation order after review: lightweight types and true missing-operation
RED; source wedge trace transport; produced cut/history plan and physical requests;
ordinary partition/classifier + explicit fan; root/runtime operation integration;
source/chain GREEN; actual source peer review. Parent owns target registration.
