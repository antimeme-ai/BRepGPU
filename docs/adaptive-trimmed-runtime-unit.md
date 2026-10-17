# Trimmed-face surface-cache runtime

Status: DESIGN for peer and integrated review, 2026-10-16. The implementation
plan follows this design's review. No product edit, build, test or benchmark is
part of this checkpoint. This is the runtime lane of
[adaptive-trimmed-face-unit.md](adaptive-trimmed-face-unit.md), coordinated with
[adaptive-trimmed-construction-unit.md](adaptive-trimmed-construction-unit.md)
and [the numerical study](../papers/native-adaptive-trimmed-facet-bounds.md).

## Operation and implementation boundary

One face operation consumes its ORIGINAL SourceStore, settled
SharedCurveSnapshot, admitted domain-policy lease, regular surface SourceCache
and exact quality. It emits a complete selected face, or typed boundary,
numerical, unsupported, cancelled or exhausted work. It must include every
source loop/use occurrence and the complete selected material, including holes
and disconnected boundary-band components. A regular patch mesh, interior
packet set or successful facet subset is not this result.

Extract the existing adaptive_cache.cpp numerical owner into
`SurfaceCacheSession`. Keep its packed nodes, original-root point evaluation,
resident control bank, tier caches, CPU scheduler and typed DeviceOwner. Existing
`PatchSession` becomes the untrimmed rectangle consumer of that SAME owner and
still returns its fully tested PacketMesh. Do not introduce a second adaptive
tree, evaluator, mutable point authority or old GeometryStore prerequisite.

The trimmed-face coordinator, called `FaceTrimSession` here, owns construction
state and calls this numerical service through typed operations. Construction
does not call an evaluator callback or mutate the cache. The service does not
privately replace any actual facet XYZ or physical boundary point.

## Source basis and selected improvements

Read pinned NVIDIA USD/BRep
`c9c5979f04cea8c38213004d4f04df0dcfb40d05`,
`source/SMLib/src/SmTess.cpp::SmTessSrfCache::BuildTree`13215–13395,
`Phase2CreateFacePolygons`12023–12125 and `AddQuadBoundaries`5334–5410.
It retains the source surface cache, inserts the shared phase1 boundaries,
classifies cached cells and separates inside polygons from remaining boundary
regions. `SmSurfaceCache.cpp::BuildTreeBase`4996–5085 and
`SubdivideByBlock`4115–4268 use initial span/panel arrays and shared split-grid
vertices. These are direct architecture precedents. Its pointer trees,
approximate analytic cubic panels, sampled tests and forced success thresholds
are not our numerical or source contracts. Restoration is in QUARANTINE.md.

Balázs/Guthe/Klein2004 §§5.2,6,7 supplies the binary-subdivision/interior-versus-
trim structure and conservative bilinear/quarter-twist contender. Ma/Hewitt2002
§§3.6,4.1–4.4 exposes hanging interfaces and closing trim regions. We retain
source-keyed shared edge chains, rather than adopting private grid/trim samples.
Filip/Magedson/Markot1986 §§2–3 explicitly warns that reconnecting approximants
does not preserve their old approximation bounds; hence actual SAME-L tests
after packet/band connectivity. The ordinary curved-triangle bounds in our
numerical study are owned derivations, not an unstated theorem from that paper.

For runtime, retain the already selected packed frontier, bounded count/scan/
fill and immutable root/path scheme. Blelloch1993 scan supplies the compaction
primitive. Karras2012 §§3–4 motivates sorted packed indexes, not a theorem about
adaptive geometry. Tzeng/Patney/Owens2010 §§3–5 and Benyoub/Dupuy2024
§§3.1–3.3 are the examined queue/pool contenders: one bounded, stable frontier
fits this append-oriented cache; a continuously running queue or CBT allocator
is not selected merely because it is GPU-oriented. Primary reading/restoration
details remain in papers/native-adaptive-subdivision-runtime.md and the two
focused trimmed-face studies.

Owned source read: adaptive_cache.cpp phases/point/freeze/facet folds,
adaptive_types.hpp, adaptive_packets.cpp, source_cache.cpp analytic admission,
adaptive_curves.cpp fresh query forest and staged trace folds,
adaptive_device_owner.hpp, and adaptive_curve_geometry.cpp's indexed original
carrier-root lookup. The changes below address actual seams in those units.

## Original analytic roots and full-period faces

Select an explicit regular analytic preparation profile with angular panels of
width at most a certified binary64 LOWER bound of pi/2. This is a deterministic
source-preparation choice to limit coarse harmonic ranges and avoid endpoint-
only coarse panels. It is not a necessary condition of the existing eR theorem
or a claim that such panels establish embedding. Existing regular admission
currently rejects width >=2pi; tiling replaces that whole-request restriction.

Add `prepare_trim_source_cache`, retaining the ordinary regular-patch purpose.
Separate original source admission (finite independent frame, radius and regular
v/radial-sheet conditions) from prepared-panel angular admission. One original
analytic evaluator descriptor retains the complete requested original box and
original source formulas. Prepared rectangles tile that box: u for cylinder,
cone, sphere and torus, and v for torus. No analytic source is approximated by a
new rational surface. Poles, singular sheets and alternate PlaneXY maps remain
named specialized dependencies, not ordinary UV support by assertion.

Choose a cut with an exact midpoint locator, round to binary64 ties-even, then
CHECK that the chosen value is strictly inside and in the exact middle half.
The stored dyadic cut itself is exact; do not claim it equals a nonrepresentable
midpoint. Recursively split until the exact panel width satisfies the profile.
No interior representable cut, panel cap or arithmetic cap gives typed unresolved
or exhaustion. Adjacent panels use the identical cut words, with canonical
ordered span_u/span_v ordinals. All tile corners/side points evaluate through
the one original analytic descriptor and exact UV key. Rational originals keep
their actual knot-span evaluator ownership and continuity checks.

Parameter u=0 and a stored value near 2pi are distinct source coordinates.
Neither modulo arithmetic nor a symbolic periodic identity welds them. The
settled physical seam uses the same PhysicalNodeKey/XYZ while its two chart
occurrences remain separate. Full sphere poles still require the declared
singular map; this profile handles regular periodic faces, not every analytic
face. A trace-purpose cache is never relabeled as regular.

## Shared records and immutable views

Reuse `PatchSelection`, `SelectedLeaf`, `PointRecord`, `PointSlot` and `LeafKey`.
Do not create a second public selected-leaf model. `PointRecord.chart_xy` is the
construction coordinate; original_uv is retained separately. Ordinary UV maps
make them equal, but consumers must not infer this for future charts.

Add an explicit monotone `cover_revision` to SelectionKey. The existing
cache_revision is the source-cache generation and cannot distinguish two covers
produced during one query. Every sealed changed cover gets a new revision.
Point promotion also increments cover_revision, even if its selected boxes do
not change, because their actual B corners and eR have changed. Older immutable
selections retain their old exact values, XYZ and constructed revisions.
Complete operation keys retain SourceCache/SourceStore identity, session, query,
source/profile revision, cover revision and owning face/boundary generations.
Key equality alone cannot substitute for the retained actual source lease.

The cache mints `SurfaceBoundView`, an immutable capability retaining the EXACT
PatchSelection plus its cache owner epoch. Its producer checks current selected
child branches, root/path-derived boxes, no duplicate selected node and no
ancestor/descendant overlap. The selected leaves cover every nonoutside
candidate region; outside masks derive from admitted trim classification.
Geometry accepts this minted view, not an arbitrary mutable public selection
passed as a bound authority. Per-facet exact clipped-area equality then checks
that the selected leaves actually cover that facet. These are respectively the
cache's own selection invariant and the actual facet operation's coverage check,
not another proof tree or output certifier.

Host selections retain their source and ExactTable after session close. They do
not retain a hidden CUDA context. `SurfaceBoundView` is a host capability minted
by the session, not a GPU allocation lease. A submitted operation separately
pins its active owner/bank until jobs and ticket drain. Close invalidates future
resident requests from a view, while its host snapshot remains readable; close
does not wait indefinitely for an external consumer to release a host view.
Restricted controls stay in the owning bank. A resident handle is opaque
{bank-generation,root/path,stage,range}; only the session consumes it after
checking the view. No external span points into a growing vector, no geometry
consumer dereferences a device offset, and no child net is downloaded merely
to expose leaves. GPU child controls currently have host placeholders, so such
a host-borrow API would already be wrong.

Base band-facet work needs only actual B corners, eR and exact boxes in the
selection. Original SourceCache roots remain the fallback authority. The rare
direct original rational restriction may reconstruct the selected source piece
or use an owning resident operation; it never requires eager exact child nets.

## Finite service protocol

The following are selected native operation seams, with exact header layouts
closed in the implementation plan. Shared rows own immutable ExactTable leases;
only the call's temporary spans are borrowed.

```
SurfaceCacheSession(resource, regular_source_cache, limits, stop_token)
open_device(CacheDevice)
begin(SurfaceQuery{query, face_key, values, root_allowances})
step(JobContext&) -> progress | pending | input_ready | operation_complete
candidate_boxes() -> immutable CandidateBoxes
classify(CandidateClassification{captured_key, complete_ordered_labels})
selection() -> SurfaceBoundView
refine(SurfaceRefineRequest{captured_key, sorted_leaf_keys,
                            packet_grid_targets, point_promotions})
sample(SurfacePointRequest{captured_key, exact_uvs, demanded_stages})
sample_result() -> immutable SurfacePointResult
bound(SurfaceFacetRequest{captured_key, actual_facets, domain_policy})
bound_result() -> immutable SurfaceFacetResult
cancel(); close()
```

One operation is mutable at a time. Calling with stale/wrong source, cover,
face, boundary, profile or completion key fails before mutation. No new operation
is accepted while previous children/tickets/results are outstanding. READY
polling lets the coordinator schedule other owners; it never blocks a worker on
a child future. CPU width is actual disjoint batch work in NativeScheduler,
not merely selecting a worker for a serial operation. Explicit close remains
retryable on drain failure; final failed teardown retains its preregistered
poisoned owner/module/reservations for process life as in DeviceOwner today.

CandidateBoxes is ONLY pending topology metadata: immutable exact box/path rows
and a captured complete horizon key, not a second leaf/control/point model.
Before requesting child points, controls or bounds, yield those boxes to the
construction trim index. Closed-box/source-segment contact plus exact winding
classifies outside/interior/band; any undecided contact is band. A tiny hole
inside a box cannot be missed through a four-corner test. Outside boxes skip
geometry for this query. Nonoutside children are freshly classified until the
first accepted cover and pinch closure. The untrimmed PatchSession wrapper
supplies all-interior classification internally.

At that first cover the construction owner fixes the geometric packet union
for this boundary generation, after pinch demotions. Packet descendants remain
packet, band descendants remain band and outside remains outside. Cache splits
do not grow the packet footprint through an existing band or consume anchors.
The common side schedule supplies new rim events; only components incident to
those changed rims need rebuilding. Pinch demotion is settled before establishing
this footprint. There is no later topology/bound-failure packet demotion within
the generation under the selected active-grid target rule. Boundary generation
changes discard this footprint and restart classification against the new
complete source chains.

## Exact anchors and actual point authorities

Grid corners retain existing compact root-relative Dyadic64 keys/depth caps.
Strict interior anchors additionally use exact arbitrary-word ORIGINAL UV
coordinates in the same PointPool, hashed with full owned limb equality.
Do not truncate them to 64 bits or require a binary64 interior coordinate.
Unify semantic exact UV identity with a grid point when they coincide. Reuse
the indexed original carrier-root locator (currently in
adaptive_curve_geometry.cpp), with the same lower-closed-span owner rule;
do not introduce another evaluator or scan all spans for each anchor.

Select a compact fast grid index plus a sparse exact-anchor index, BOTH mapping
to the same point-tier record pool. An anchor carries its positive-overlap
leaf/root locator. Compute its exact root-relative coordinate; when representable
as the existing normalized Dyadic64/endpoint key, probe that compact key before
inserting an anchor. Indexed touching prepared-root aliases handle a cut point,
without scanning all roots. Otherwise probe the sparse index by canonical
original owner plus exact absolute UV, with full limb equality. On a later grid
lookup, a compact hit is sufficient; on a miss, if the sparse index is nonempty,
derive that new grid point's exact absolute UV and probe it before allocation.
Install a compact alias if it finds an earlier anchor. Thus both creation
orders share one XYZ/stage/revision record. The extra exact work is on grid
misses after anchors exist, not every cached grid lookup; meter it explicitly.
Two tagged indexes are lookup accelerators, not two semantic point identities.

Construction chooses a strict positive triangle/leaf overlap locator, then a
checked dyadic interior point. The service evaluates it from the canonical
original source using the common CPU/P100 point profile. Outward parameter
packing and demanded-tier fallback account for arbitrary exact UV. An explicit
anchor-bit/work cap is separate from the cache's axis-depth64 cap. The returned
point lease carries exact parameter, stage, constructed revision, actual Q and
departure. A later promotion changes constructed revision and invalidates every
incident facet/bound result before reuse. Prior warm higher tiers never bypass
the fresh query's BASE-to-promotion policy.

Physical boundary sites remain the shared physical authority's immutable Q,
logical key and constructed revision. Their chart-site identity includes use/
loop-junction occurrence and branch/lift, so seam sectors do not merge through
PhysicalNodeKey alone. No private resampling of Q is permitted. A numerical
floor in a surrogate B corner targets its cache PointKey. A floor in an actual
facet corner targets the typed PhysicalNode or SurfacePoint authority carried
by that construction site. Returned requests name that authority; geometry
keeps SAME L unchanged until the coordinator installs a new revision.

## Actual facet bounds and canonical feedback

`SurfaceFacetRequest` retains the minted view, immutable construction result,
complete FacetKeys, exact chart triples, actual XYZ triples and their authority
references. Geometry builds an indexed selected-leaf box query, exact virtual
triangle/rectangle clipping and original barycentrics. Virtual points never
enter topology or source inventory. Every positive piece uses SAME actual L.
Closed-touch candidates contribute zero-area pieces only; exact positive areas
must cover the whole triangle. Unknown domain maps are unsupported, not zero.

The ordinary fast bound is eR + upper||B-L|| + named E_domain, paid once.
For an original analytic plane S=O+uX+vY, use the direct affine specialization:
S-L is affine, so its norm is bounded by the maximum of its three original
triangle-vertex residuals plus E_domain. This needs no leaf clipping or eR
twist transfer. The source envelope/selected-domain checks still apply; rare
exact original-plane rational threshold comparison handles ambiguity. This is
source-algebra dispatch, not a source-ID or observed-watch specialization.
CPU/P100 run the same shared padded RN coefficient/transfer operation. Downward
allowance conversion is only a sufficient fast gate; exact allowance and
same-task fallback handle ambiguity/equality. Failed conservative upper does
not prove source infeasibility. The numerical owner's finite dispatcher first
tightens SAME L through demanded arithmetic/direct original rational or analytic
bounds before requesting a construction change. No C2 argument crosses original
knots. A cache-only refinement retry remains part of this SAME cache and bounded
policy; it does not count as changing a failing actual triangle.

Return complete facet results and typed requests only after folding the entire
captured horizon in semantic FacetKey order. The outer owner then atomically
reserves the full unique leaf/target/anchor/promotion set. Select two concrete
progress mechanisms according to the actual facet generator:

* Failed BAND triangle: install at least one NEW arbitrary-word exact source
  anchor strictly inside it, split implicated cache leaves and rebuild its
  connected region once for the complete horizon.
* Failed PACKET triangle: choose a canonical prepared-root-relative dyadic point
  strictly inside it within the explicit axis-depth64 cap. Force selection
  refinement in the SAME cache until this point is an ACTIVE grid corner and
  common-schedule vertex, then rebuild packets and incident rim components.
  A retained numerical point from an earlier query is reusable: NEW means new
  active construction incidence, not a fresh allocation. Check that the target
  is active before accepting the rebuilt packet generation. A conforming mesh
  with a vertex strictly inside the old triangle cannot emit that triangle
  unchanged. An arbitrary one-axis leaf split has no such guarantee: a skinny
  zipper triangle can lie wholly in one child. No capped strict-interior target
  or refinement path gives typed unresolved, not private arbitrary packet
  triangulation or silently demoting all failing packets into a different engine.

Multiple failures sharing targets, anchors or leaves are exact-deduped before
counts/reservations. The implementation plan must specify the refine-to-grid
request's exact target/path fields and complete multi-target reservation before
selection mutation. No lower-ready failure mutates construction while higher
pending work could change the horizon. This is a progress policy, not a
termination or triangle-quality theorem.

## Whole-face state and boundary generations

`FaceTrimSession` retains source/face identity, SharedCurveSnapshot, domain map,
SurfaceCacheSession, trim index, current bound view, packet footprint/rim data,
region/anchor tables and current complete result horizon. Its state sequence is:

1. Validate the complete source/use/joint snapshot and finite domain policy.
   Request a real missing boundary/map capability or return unsupported.
2. Select/classify/bound regular cache leaves. Settle packet pinch demotions and
   establish the initial footprint. Build packets only for that packet subset.
3. Construction combines source cycles and REVERSED packet rims in the directed
   band CDT, selects all components/holes, preserves protected source chains and
   produces one complete actual construction result.
4. Submit packet and band actual facets through the same surface bound service.
   Fold all results. Route original BoundaryKey requests to the definition
   owner, actual-point promotions to their authorities and construction requests
   through the complete reserve/sample/refine/rebuild horizon above.
5. Return success only with complete selected material, unchanged settled source
   inventory, accepted whole actual facets and exact nonzero actual physical
   triangles. This is not the later definition contact/assembly publication
   claim. Resource/depth/precision exhaustion never publishes a partial face.

Only the definition coordinator may commit physical boundary splits/promotions.
The session yields immutable requests; it does not hold a lock or worker while
waiting. A new boundary generation invalidates incident construction/domain/
point associations. Unrelated face generations need not restart just because a
global boundary table grew; remap retained source occurrence keys to the new
snapshot, never trust a transient BoundarySampleSlot. Source changes create a
new SourceCache, while exact quality changes start a fresh canonical query over
retained numerical tiers. Matrix/instance transport remains the definition's
existing allowance operation, outside the source cache.

## Capacity, placement and direct checks for the later plan

Count all unique requests, child controls, samples, bound pieces and output rows
before mutating the epoch. Reserve owner-local jobs and exact fallback scratch;
no child borrows a parent mutable Budget, and no fallback waits for unreserved
memory while retaining the frontier. Results/tables retain RunResource leases.
Cancellation drains jobs/device work, prevents publication and preserves prior
immutable results. No per-cell module/session open or repeated full patch mesh
is allowed merely to obtain a selected leaf view.

CPU owns exact classification, graph/CDT, clipping and barycentric preparation.
P100 owns real point, restriction, eR, actual bilinear-facet coefficient and
admitted direct-residual batches on the existing resident bank. It is neither
render-only nor a device inequality after all-cell host exact extraction.
No raw full p3q3 interval net is silently copied into every thread's local
array; existing cooperative degree-bucket restriction/residual kernels remain
the numerical path. Record actual transfers including controls/counts/results.

The finite implementation plan must author direct REDs for the missing view/
service operation and complete original-source face operation, then check:
tiny contained hole, narrow sliver, many aligned holes, packet islands/pinch,
C0 multispans, arbitrary hanging depths, exact anchor beyond binary64 spacing,
same-face full-period seam, real curvature requiring an anchor, authoritative
identity and missing-map refusal. Independent actual-output exact-domain and
original polynomial/rational/analytic models test geometry; CPU/P100 equality
tests deterministic placement separately. Stale/partial/mismatched completion,
source/cover rebuild, cancellation and budget failure test the actual lifetime
claim. Reuse the same original-output reader; do not add a second geometry
certifier to the hot path.

Performance families distinguish mostly-interior, mostly-outside, thin-band,
many-hole, large-cross-leaf facet and genuine refinement cases. Compare complete
cold process and prepared fresh quality queries, CPU1/width and both P100s;
separate compilation, boundary, classification, packet/CDT, actual bounds,
anchors, output and close. Count outside geometry avoided, leaf candidates,
positive pieces, original fallback, new/reused points/controls, component
rebuilds, bytes and reserved/peak memory. Bound experiment duration and report
censors; no GPU win or stronger global geometry guarantee is presumed.
