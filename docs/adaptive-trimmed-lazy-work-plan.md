# Whole-facet filters before numerical-leaf partition

2026-10-17. Finite implementation PLAN; no product edits before review.

Delivered. The subsequent [deferred exact XYZ unit](adaptive-trimmed-lazy-xyz-plan.md)
replaces clipped pieces' eager XYZ payload below with exact barycentrics;
ordinary packs enclose the same affine map and precise requests evaluate it exactly.

## Actual problem and existing machinery

The active watch stack is in prepare_trim_facet_work while other scheduler
workers wait. This establishes expensive preparation at that sample, not giant
integer operands or the cause of every elapsed second. Actual source currently
builds LeafIndex, intersects every nonplane facet with candidate numerical leaf
rectangles, constructs rational barycentrics, and seals15 exact coordinates per
positive virtual triangle BEFORE trying a bound. Runtime then runs cached base
and original source bounds on those already-materialized pieces.

The existing affine plane branch already avoids all this. Existing
adaptive_trimmed_numeric.hpp::bound_source_triangle_rn64 accepts one ORIGINAL
root and the SAME emitted triangle L. It implements anchored homogeneous
N-WL/Wmin for p,q<=3 rational roots and the existing original analytic derivative
bound. cuda/adaptive_trim_api.h::TrimCudaAPI::source_triangles already dispatches
that exact POD operation on the same resident bank/session. No evaluator,
mathematical bound, precision tier or device ABI changes are proposed.

Grounding remains docs/adaptive-trimmed-numerics-unit.md: Filip/Magedson/Markot
1986 §2/Theorems2/4 and its Theorem5 warning; Balázs/Guthe/Klein2004 §§4–7;
Ma/Hewitt2002 §§3.6/4.1–4.4. Pinned NVIDIA USD/BRep
c9c5979f04cea8c38213004d4f04df0dcfb40d05 SmSurfaceCache::SubdivisionTest3490–3655
and SmTess::Phase2CreateFacePolygons12023–12125 motivate cache locality, not
permission to use its sampled flatness as our actual-facet certificate. Our
selected change is scheduling the ALREADY implemented source theorem before
an artificial numerical-leaf partition. ADAPTIVE_MESHER_RETURN_PASS records it.

## One immutable capture and literal operations

Retain current TrimFacetWork and its source/view/construction/domain/exact-tau
leases and complete canonical requested-parent rows. Factor current preparation
admission into ONE shared implementation; do not add an independent preflight
or source authority. All current identity, complete original/use policy,
classified-domain provenance, prepared-root closure, exact positive chart area,
actual physical nonzero, limits and exact-table alias checks remain mandatory.

Geometry header additions, using existing handle types:

```
TrimFacetWork prepare_trim_facet_candidates(
  JobContext&, SurfaceBoundView, FaceConstructionResult, FaceDomainLease,
  FacetAllowances, span<const uint32_t> construction_facets,
  ExactTableKey, TrimFacetLimits);
TrimSourceBatch pack_original_facets(
  JobContext&, const TrimFacetWork&, span<const uint32_t> parent_rows);
TrimPreciseResult compare_trim_original_facet_precise(
  JobContext&, const TrimFacetWork&, uint32_t parent_row);
TrimFacetWork materialize_trim_facet_fallback(
  JobContext&, const TrimFacetWork& candidates,
  span<const uint32_t> fallback_band_parent_rows, ExactTableKey);
```

Rows append optional whole_original (OriginalRootSlot). This is source support,
not an accepted decision. Existing original_plane remains; packet parents retain
one actual known-leaf piece immediately. Extend TrimPieceRecord with
TrimPieceGeometry { sealed_virtual, whole_parent }. For whole_parent, UV/XYZ
come directly from its retained construction parent; scalar ranges are empty.
Existing piece packers switch on this explicit tag. Thus lazy merged work copies
packet metadata without recopying15 exact coordinates for every accepted packet.
sealed_virtual is the unchanged old barycentric clipping payload/format.

Keep existing prepare_trim_facet_work eager entry for its direct numerical work
fixture, implemented through the SAME admission/materialization helpers. The
runtime uses the candidate entry. No mutable ready/accepted flag enters a
producer record. Only actual executor results decide acceptance.

## Selected route and procedure

1. Prepare immutable candidate work. Plane uses the existing direct route.
   A PACKET facet uses its captured facet.leaf locator: require in-range, same
   source carrier/root and exact containment of ALL three UV corners in that
   selected leaf's closed rectangle. Emit one whole_parent piece; no LeafIndex,
   clipping, barycentrics or virtual point evaluation. Unknown/contradictory
   packet ownership refuses; do not silently choose a nearby leaf.
2. For BAND facets, choose the original root containing the exact actual triangle
   centroid via existing indexed original lookup. Then require ALL three exact
   UV corners inside that root's ORIGINAL closed rectangle. Convexity establishes
   the whole triangle support. Centroid membership alone never suffices. This
   handles C0 corners whose canonical point-owner root is on the other side.
   If containment fails, mark deferred cache partition, not invalid source.
3. Session runs real asynchronous bounded batches: known-leaf packets through
   existing cached ActualFacet kernel, then existing source/precise fallback for
   those packet nonaccepts; planes through existing direct plane operation;
   eligible Bands through pack_original_facets and SAME SourceTriangle CPU/P100
   executor. No hidden host numerical pass in candidate preparation. Use exact
   tau comparison and existing uncertainty-triggered precise routing/caps.
4. A direct Band result can accept only if physical_nonzero AND source bound
   proves <=exact tau. Numeric nonaccept/uncertainty is not source infeasibility,
   a point floor, or permission to refine before the old tightening path.
5. After the complete direct horizon, union deferred/cross-original-span Bands
   with direct-nonaccepted Bands, sort/dedup parent rows, and materialize ONLY
   that union through existing LeafIndex/clip_polygon/SAME-L barycentric code.
   Retain one index within that materialization call, not one index per parent.
   Returned merged work keeps ALL original parent rows and known-leaf packet
   pieces; accepted direct Bands have zero pieces. Fallback rows have nonempty
   canonical positive pieces or the existing typed failure. A fresh table/key
   seals virtual coordinates; all stage handles retain the same capture. Its key
   must differ from candidates.values and every retained construction/selection/
   domain/allowance table. Do not replace candidate work or prepass task buffers
   until the outstanding prepass ticket/jobs have completely drained.
6. Run unchanged cached base→source/precise on new virtual Band pieces. Packet
   failure already has its known-leaf piece/source outcome and is not evaluated
   twice. Fold by explicit parent/construction-facet maps, never stage position.
   Final accepted flag is from the appropriate direct result OR all materialized
   pieces, always AND physical_nonzero. Zero pieces alone NEVER means acceptance.
7. Only then produce final complete SurfaceFacetResult in captured caller subset
   order. Every FAILED nonplane parent has its existing known-leaf packet piece
   or canonical Band fallback pieces. Run unchanged point-floor probes over
   failed parents/piece corners; unchanged construction refinement consumes the
   same complete parent word and positive overlap pieces. Central Band anchors
   and forced packet targets retain current rules. Full ordinary+mapped horizon
   gate and publication remain unchanged.

## Bounds, history and failure semantics

No original-span crossing is silently assigned to one span. C0/cross-span Bands
use existing cache partition, whose leaves already respect original root spans.
No cached surrogate changes L or samples virtual source points. Normalization
uses original root bounds for source tasks and selected leaf bounds for base.
Domain E remains the currently admitted reconstructed-selected/exact-image
meaning; no boundary-upper-to-interior-map inference. Mapped caps stay in their
existing separate operation. Fresh table ordinals qualify every merged work;
stale view/domain/quality/subset completions refuse. Cancel/error drains the
current single executor operation and retains both capture/materialization
leases until its ticket/jobs return. Existing work/bytes/pieces/precise caps apply
across BOTH phases cumulatively, not reset for lazy fallback.

## Ownership and bounded direct checks

One implementation owner covers adaptive_trimmed_geometry.hpp/.cpp, the existing
work fixture and adaptive_surface_cache.cpp phase/queues/fold. Numerical and
runtime peers review source support and failed-parent piece/lifetime invariants;
no topology algorithm changes. Root owns CMake/device integration; CUDA
POD/kernel/ABI unchanged.

After plan acceptance, extend the existing REAL minted-view work fixture before
body. Missing operation is true compile/link RED, followed by direct assertions:

- Existing same-source BAND crossing numerical leaves but contained in one
  original span can pass existing original source bound with candidate work
  clip_vertices==0/pieces==0. This attacks the actual avoided work.
- A real packet has one known-leaf piece, exact SAME Q/UV and zero clipping.
- A nonaccepting direct Band and a C0-crossing Band return canonical fallback
  pieces under complete parent indices; source proof and old point-floor/refine
  paths remain exercised. No fabricated successful view/result or evaluator.
- Existing stale subset/quality/table checks and reversed caller-order fixture
  remain; fail/cancel after prepass cannot publish a partial result.

Run only affected work/runtime and existing original-output reader for rational
weighted/saddle and C0, CPU1 plus the already selected P100 operation. Existing
independent whole-source coefficient/analytic reader is unchanged. Do not repeat
low-level geometry cohorts or extend qualities. Then parent may retry the SAME
bounded watch with explicit prior censor retained. Record preparation time,
clip vertices/pieces, source/base task counts and complete outcome separately.
The source prepass has a cost: weak whole-span bounds may fall back and add work.
This is a measured tradeoff, not an asserted general speedup or denominator fix.
