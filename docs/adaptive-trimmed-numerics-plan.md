# Implement cached actual trimmed-facet numerics

Status: finite implementation PLAN for cross-review, 2026-10-16. No product
implementation until plan acceptance. Normative numerical/source DESIGN is
adaptive-trimmed-numerics-unit.md. That file lists primary papers, exact sections
and pinned reference functions. This plan implements that one operation rather
than another source evaluator or mesh constructor.

## Ownership and literal shared seam

Geometry owns native/mesher/adaptive_trimmed_geometry.hpp/.cpp,
adaptive_trimmed_numeric.hpp and adaptive_trimmed_types.h; the last two remain
CUDA17 host/device compatible and reuse adaptive_numeric.hpp arithmetic/types.
It also owns tests/native_adaptive_trimmed_geometry.cpp and extends the SAME
tests/native_adaptive_geometry_oracle.py and its source fixture. No library or
old GeometryStore prerequisite. Root owns CMake and CUDA operation adapters;
runtime owns execution/folding/lifetime; construction owns real facets/domains.

Runtime owns adaptive_surface_view.hpp with SurfaceBoundView, an opaque
producer-minted handle. Accessors are selection()->shared_ptr<const
PatchSelection>, key()->SurfaceViewKey, source()->const SourceCache& and
resource()->shared_ptr<RunResource>. No public resident-control pointer. This
header includes shared values, not SurfaceCacheSession implementation.
Construction owns adaptive_trim_types.hpp with FaceConstructionResult,
FaceDomainLease, RegionFacet and complete semantic FacetKey. Geometry includes
these lightweight headers, not construction or cache implementations.

The native numerical header declares:

```
struct FacetAllowances { ExactTable values; ExactRef face_position; };
struct TrimFacetLimits {
  uint64_t candidate_visits, pieces, exact_entries, work;
  uint32_t batch_records, precise_tasks;
};
struct TrimPieceRange { uint32_t first, count; };
enum class TrimBoundMethod : uint32_t { cached_bilinear, original_source };

TrimFacetWork prepare_trim_facet_work(
  JobContext&, SurfaceBoundView, FaceConstructionResult,
  FaceDomainLease, FacetAllowances, span<const uint32_t> selected_facets,
  ExactTableKey result_values, TrimFacetLimits);

TrimBaseBatch pack_base_pieces(JobContext&, TrimFacetWork,
                               span<const uint32_t> piece_indices);
TrimSourceBatch pack_source_pieces(JobContext&, TrimFacetWork,
                                   span<const uint32_t> piece_indices);
TrimPlaneBatch pack_plane_facets(JobContext&, TrimFacetWork,
                                 span<const uint32_t> facet_indices);
TrimPreciseResult compare_trim_piece_precise(JobContext&, TrimFacetWork,
                                             uint32_t piece, TrimBoundMethod);
TrimPreciseResult compare_trim_plane_precise(JobContext&, TrimFacetWork,
                                             uint32_t facet);
```

TrimFacetWork is a cheap copyable handle to shared_ptr<const TrimFacetData>.
Getters key(), values(), facets(), pieces(), plane_facets(), stats() expose
immutable native rows. The common key binds SurfaceViewKey, construction key,
request ordinal and selected immutable facet-key inventory. Data retain view,
construction/domain/source leases and allowance table. selected_facets are
copied/admitted before return; no borrowed span survives. Initially request all
facets; later request only new/revised components or packets. Unchanged accepted
source/map/L bounds may survive cover refinement under their original semantic
facet/source/point identities; old asynchronous completions still cannot settle
a new request. No per-edit whole-construction bound scan is selected.

Each TrimFacetRecord names a selected construction facet and either a contiguous
piece range or the direct-plane branch. Each TrimPieceRecord names parent row,
selection leaf, original evaluator root and contiguous table-local entries for
absolute original UV6 and SAME-L9. The work's retained ExactTable qualifies
those entries; do not repeat full ExactRef owner/generation per coefficient.
No barycentrics are required after SAME-L exact values are stored. Packing
normalizes absolute UV exactly to the required leaf or ORIGINAL evaluator box,
then transfers outward. Each bounded batch retains its work, copied row indices
and operation/method header, plus a PMR vector of POD inputs. Source fallback
packs only unresolved piece indices. Disjoint worker batches use private
JobContext scratch and one retained immutable work.

First-domain policies have E_domain=0 by explicit reconstructed-selected or
whole-use exact identity meaning. Non-null domain lease/source/face/use/chart/
boundary generations must match construction. Any nonidentity payload without
the actual B/T consumer refuses before work. Do not allocate a fictional domain
allowance or reinterpret the existing boundary authoritative_domain_upper.
FaceConstructionResult is already the construction owner's shared immutable
handle, not a type to wrap in another shared_ptr. Unchanged domain matching
compares source/query/face/boundary/chart/profile, not construction_revision.

The new HD header declares exactly:

```
struct SourceTriangleInput {
  OriginalRootSlot original;
  Interval normalized_uv[6];
  Interval affine_xyz[9];
  double allowance_down;
};
struct SourceTriangleBounds {
  NumericStatus status;
  double position_upper;
};
```

adaptive_trimmed_numeric.hpp supplies bound_source_triangle_rn64(descriptor,
original_controls,input) and bound_original_plane_rn64(descriptor,input).
Existing ActualFacetInput/Bounds and bound_actual_facet_rn64 remain the base
operation; add the range-aware covariance minimum there with no layout change.
Root's lightweight TrimCudaAPI getter is brepgpu_adaptive_trim_cuda_v1, with
source_triangles(DeviceSession*,SourceTriangleInput*,n,ticket,error). It uses the
SAME base session/poll/owner; AdaptiveCudaAPI ABI1 remains unchanged. CPU and
device adapters use these cooperative scalar helpers from the common HD header:

```
source_triangle_homogeneous_coefficient_rn64(
  root, original_controls, input, TriangleCoefficient{degree,i,j,k}, channel)
    -> Interval
source_triangle_residual_coefficient_rn64(
  root, restricted_triangle_homogeneous, input,
  TriangleCoefficient{degree,i,j,k}, xyz_channel) -> Interval
finish_source_triangle_rn64(root,input,max_residual_norm,weight_lower)
    -> SourceTriangleBounds
```

Triangle coefficient ordering is increasing i then j, k=degree-i-j; homogeneous
channels Nx,Ny,Nz,W fastest. First helper applies tensor-blossom partition weights
on the original source slab with at most16 scalar scratch intervals. CPU wrapper
and CUDA warp first compute/store at most28*4 restricted homogeneous intervals;
then degree d+1 residual uses
sum_k alpha_k/(d+1)*(N_(alpha-e_k)-W_(alpha-e_k)*L_k).
CPU wrapper may own one local triangular net; CUDA stages the net once perwarp,
not once perlane. Lane+=32 covers all36 residual coefficients. Every coefficient
uses identical operation order. Max reduction is permitted only on finite valid
norms; uncertainty propagates rather than NaNs being erased. Positive W lower
and residual norm upper feed the finish helper. No new device owner or per-thread
large tensor array. Source-bound descriptors cannot enter sample factories.

The finite domain dependency also adds an owner-local host helper in the
EXISTING adaptive_curve_geometry.hpp/.cpp target, avoiding a trace->full facet
geometry/cache link cycle:
original_curve_line_image(CurveSourceCache,CurveSlot,exact_s2,Budget)
-> optional<OriginalLineImage>. Output contains Rational endpoints4, covector2
and scalar endpoints2 in requested traversal order. From exact original positive
homogeneous restrictions, prove collinearity and nonnegative scalar derivative
numerator per source span (or reverse sign in descending traversal), strict
endpoint progress and complete interval coverage. Stationary endpoints are
allowed. Unknown analytic identities return empty. Construction/trace owns
whole-use map monotonicity, chosen-site exact line/order/endpoints and original
joints; no physical-Ce line tag or interval-overlap equality substitutes.

The retained work also returns typed point-floor requests for failed facets.
Actual source point/residual interval LOWER squared must exceed exact tau²;
width alone is never a floor. RN point facts clear the usual cases, with lazy
exact rational/plane or bounded analytic precision on uncertainty. Surface
sites and B corners name captured authority/stage and request next-tier promotion
through the SAME point pool. PointRecord/SurfaceSiteSource departure uppers may
clear routing without reevaluation, never add another positional charge.
Boundary floors are named source/precision unresolved in this first profile;
there is no new boundary-point promotion API or futile physical interval split.
Face runtime admits captured carrierTrace->actualChord <= exact face tau and
source node facts <= requested node cap before work. Original vertices remain
fixed. An exhausted surface-point profile also stops unresolved. This is the
same actual-facet source invariant, not a separate output certificate.

## Sequential implementation and real RED

1. Publish shared declarations with peers. Root registers
   brepgpu_mesher_adaptive_trimmed_geometry with the new .cpp, linking
   adaptive_geometry, adaptive_curve_geometry, exact/source/shared values;
   brepgpu_native_adaptive_trimmed_geometry links that and actual view/domain
   producers. No own CMake edits. Standalone pure numerical tests may use
   original SourceCache without inventing a bound-view capability.
2. Write direct source-derived fixture records before operation bodies. Genuine
   RED is missing bound_source_triangle/prepare_trim_facet_work operation or
   the old whole-leaf/fourcorner path refusing an actual cross-leaf facet.
   Dependency compile errors and concurrent stale objects are not geometric RED.
   Retain exact command/status in journal. Actual work-preparation RED uses the
   runtime's real minted view and construction's real retained domain/facets.
3. Implement common range-aware cached transfer and original affine-plane bound.
   Then implement original tensor-to-triangle N-WL restriction, reusing owned
   blossom/binomial identity and existing RN operations. Degree p/q<=3 and
   positive W admission are explicit. Implement analytic triangle Taylor bound
   with common original source formulas/directional constants and bound-only
   original vertex enclosures. No C2 assumption crosses original roots.
4. Implement native selected-leaf index, exact triangle/rectangle clipping and
   work preparation. Reuse/adapt the owned cell_bounds.cpp clip math without its
   old GeometryStore/BoundRootBank dependency. Box index visits are metered;
   closed contacts remain candidates. Exact convex result has <=7 vertices;
   <=5 positive fan triangles. Carry original facet barycentrics and evaluate
   SAME L exactly. Sum exact positive areas against the admitted disjoint
   NONOUTSIDE cover. Missing coverage fails the actual facet, not partial PASS.
   Direct original planes bypass index/clipping after source-envelope and
   selected-domain admission. Counts/limbs/reservations precede publication.
5. Implement demanded precise methods from the retained original source: exact
   positive rational tensor restriction and squared residual coefficients;
   exact cached coefficient/covariance threshold route; exact plane corner norm
   squared comparison; existing owned analytic enclosures at canonical demanded
   precision otherwise. Nonnegative guards precede squaring. Preserve exact
   remaining tau on certified equality. Conservative upper>tau is uncertified,
   not infeasibility. No synthetic numeric floor from coefficient interval width.
6. Integrate through runtime's actual changed-facet batches and root's actual
   Curve/Surface device owner extension. Same keys, immutable L and exact tau
   apply CPU and P100. Pack source tasks only after base nonacceptance; near
   threshold/width uncertainty is bounded precise work, not automatic geometry
   splitting. Complete runtime fold emits typed generator-qualified requests.
   BAND anchors and PACKET active-grid targets follow the accepted construction
   policy; numerics creates neither private source nodes nor connectivity.
7. Run the direct original-output reader once on CPU and both P100s; meaningful
   affected sanitizer/tidy checks follow. Independent peers review actual source
   math and typed coverage/lifetime seams; fix all valid findings before the
   root's coherent source-to-complete-face checkpoint. No watch run in this unit.

## Oracle, families and bounded execution

Extend native_adaptive_geometry_oracle.py with --trimmed-fixture EXECUTABLE;
retain existing --source/--analytic-fixtures/--cuda-module/--device semantics.
Use TSV/hex-float records for actual work geometry, bounds and method, plus exact
numerator/denominator fields where virtual coordinates are nondyadic. The reader
derives source from original POWER/Cox/positive homogeneous controls, never
production restricted nets. Exact polynomial residual subdivision independently
proves whole-facet requested/reported bounds where initial coefficient hull is
loose; explicit maxima handle quarter-twist/equality. Analytic closed derivatives
and true source expressions prove selected whole-facet bounds; finite probes
only falsify. CPU/P100 equality checks placement separately.

Use one fixed cohort: affine nonorthogonal plane displaced corners; exact
bilinear saddle eR=0 with skinny contained facet and nonzero nodal departures;
positive rational p/q<=3 and C0 derivative jumps; long facet crossing several
leaf/span boxes with nondyadic virtual cuts; closed-touch zero-area candidates;
leaf ordering/missing/overlap/stale-view attacks; original analytic regular
cylinder/cone/sphere/torus; tiny scales and threshold equality. Complete-face
reader adds real packet/band facets and anchor/active-target rebuilds. Do not
substitute synthetic view/domain structs for real producers to claim integration.

All compilation/execution uses the fleet feature checkout and isolated
build/adaptive-trimmed-geometry-release or -asan. Root provides actual CMake target
and module path. Build/run commands are recorded when they exist; no pretend
command is reported as executed. Atomic narrow uploads only, never in-place
formatting active shared fleet headers. No new all-watch or expanded-corpus
campaign. Stop affected checks after they settle the unit.

Record actual candidate visits, clipped vertices/pieces, source fallback/precise
tasks, point promotions, cache/anchor requests, reused work and transfer bytes.
One mostly-plane case must exercise direct no-clip dispatch; one long thin band
must expose candidate/piece cost. Performance is complete source/construction
cost, not isolated inequality throughput or a forced triangle quota.

Numerical failure routing closes in the same coefficient pass: retain an
`uncertainty_upper` diagnostic equal to the largest residual-coefficient
interval radius norm divided by positive Wmin (analytic/plane paths retain
vertex residual interval radius). It is neither an actual point departure nor
a lower bound. A nonaccepted result whose diagnostic is at least tau/4, or lies
within four diagnostic radii of tau, requests bounded same-task precise work;
far geometric failures keep the fast construction refinement path. This routing
heuristic avoids repeatedly splitting a zero residual whose original extracted
coefficient intervals dominate the allowance. Only precise same-task comparison
or a valid upper accepts; interval width never proves infeasibility or a point
promotion floor.
