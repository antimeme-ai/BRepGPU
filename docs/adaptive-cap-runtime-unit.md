# Typed cap-domain continuation and resident bounds

2026-10-17. DESIGN and finite implementation plan for review, not product code.
Own one runtime invariant: a produced source-qualified cap-domain plan may change
local cuts within the SAME face/query/boundary authority, and every ordinary or
mapped facet is accepted against its SAME captured actual Q before publication.
The existing SurfaceCacheSession, FaceTrimSession and device owner remain the
only numerical/controller lifetimes. No second source bank, point sampler,
forest, proof tree or coordinator is added.

## Grounding and ownership

Read the actual adaptive_surface_cache/view, adaptive_trimmed, adaptive_cache_detail,
adaptive_trim_types/regions, device_owner and current geometry cap plan. Numerical
source contracts are `docs/adaptive-seam-wedge-geometry-plan.md`: pinned USD/BRep
c9c5979f04cea8c38213004d4f04df0dcfb40d05 SmTess gap-UV polygon/MakePolygon and
shared-edge phases; OCCT8.0.1 b8f597c677811d1f9f4d8a97f5ae2825c0353a42 singular
sectors/shared edge discretization; Filip/Magedson/Markot1986 vector interpolation.
We reuse the selected packed resident frontier/ticket design and the actual
DefinitionBoundaryForest physical authority. These references justify preserving
one physical vertex with multiple chart sectors; none authorizes tolerance
inflation, coordinate welding or a private cut point on a source edge.

Geometry owns quotient source compilation/local RN+precise regularity and cap
math. Construction owns produced cap/domain/cut plans and mapped topology. Root
owns the versioned CUDA cap extension on the SAME DeviceSession, Component
continuation storage, caller/wire/display changes and CMake. This runtime lane
owns Surface/Face admission, local continuation, staged cap execution and lifetime
fixtures. No source_cache/numeric geometry edits are made by this lane.

## Shared immutable source and identity schema

Append `domain_revision` to SurfaceFaceIdentity. Append `domain_revision` AND
`face_generation` to FaceConstructionKey: the current construction key has no
independent face generation, and current SurfaceFaceIdentity fills it from the
boundary generation. These are separate counters after a local cut. Legacy
regular-face domain_revision is0; initial face_generation equals the admitted
physical face boundary generation for compatibility. Every local cut increments
both face_generation and domain_revision, with checked overflow. A subsequent
physical boundary replacement also advances the owner face_generation rather
than replacing it with a potentially smaller forest counter. New assignments
are separately qualified by assignment_epoch.

Construction's lightweight `adaptive_cap_domain.hpp` contains no full session
include. Proposed produced handle and rows (literal private row details belong
to the construction plan):

    CapDomainKey {source_generation,query,definition,face,boundary_generation,
                  face_generation,chart_revision,domain_revision,
                  assignment_epoch,profile_revision}
    ProducedCapDomainLease                 // producer-minted immutable handle
    CapCutControlLease                     // immutable, source-locator history
    CapDomainPiece {original_root,kind,finite_connector_uv,cut_uv,
                    physical_endpoint_locators,source_sheet_sign,cut_control}

Actual schema is native/mesher/adaptive_cap_domain.hpp; SurfaceCellCoverage
retains a produced classification lease and candidate row, so exact boxes/full
paths/cap-piece ranges address that immutable classifier operation. Ordinary
cap_coverage rows align selected leaves; mapped_cells retains material omitted
from ordinary sampling/bounds.

The handle retains the SAME SourceCache bank, SourceStore, SharedCurveSnapshot,
RunResource and sealed exact table. Its producer checks actual original wedge/
loop/use/joint/vertex relationships; an authored boolean cannot mint it. It may
contain ZERO pieces for a regular face in a quotient-capable multi-carrier bank;
that explicit empty inventory is still produced and bound to that face. It may
contain original UV/cut/map and source-only orientation facts before actual cut Q
exists. It contains NO claim of completed actual-facet bounds and no private Q.

FaceDomainData gains trailing ProducedCapDomainLease caps. A domain must match its full
CapDomainKey, source bank identity (not merely source ID/generation), resource,
physical snapshot, profile and full enclosing face key, including assignment.
Original loops/uses/sites remain retained even where the working UV connector
is construction-only. Cap facet source records use typed piece/role references;
ordinary UV triangle consumers must never reinterpret a topology proxy as an
original UV triangle. Surface/geometry work always retains the full domain.

## Admission and classification

SurfaceQuery gains trailing FaceDomainLease domain (empty remains legal for the
existing regular/trace restrictions). For quotient_trim it is mandatory, source-
and face-matched, with a produced matching caps handle even when empty. Ordinary
PatchSession continues to refuse quotient_trim. SurfaceCache and FaceTrim do not
accept trace_geometry as a regular mesh source. A quotient source on P100 also
requires the cap extension handshake proving local analytic-regularity/full-frame
semantics, even for an empty-cap regular face; an older base-only module must
refuse before any accepted work. Base ABI layouts remain unchanged.

FaceTrim still admits all selected shared sites/intervals against the EXACT
requested node/positional caps before domain construction. SourceTrace wedge
source-evaluation widening belongs to the separate constructive source-guard
unit, not this runtime admission. A quotient bank never suppresses original
V/Ce/S guards or enlarges epsilon/delta. Common original point evaluation is
separate from ordinary mesh admission.

Extend SurfaceClass with mapped_cap and retain a typed per-candidate range of
cap-piece indices plus flat piece-index pool. One box can overlap several cap
pieces. A mapped_cap label means all MATERIAL in that box belongs to the cap
plan, possibly with outside-D space too; it is not outside. Mixed ordinary/cap
material is band plus overlap metadata. A mixed box can remain an ordinary
band box only when its ACTUAL entire box proves local source regularity. Do not
wait for a slanted cut to align with a rectangular grid. Exact produced domain
classification determines outside, ordinary and mapped coverage before geometry.

Minted SurfaceBoundView retains the SAME classified_domain and current epoch,
ordinary PatchSelection plus sorted mapped classification cells/path ranges and
cap-overlap metadata. Prepared-root closure still includes outside branches.
For quotient operations the classifier also returns a producer-minted immutable
CapClassificationLease retaining the exact candidate lease, domain and raw
material roles/cap-piece ranges. SurfaceCache consumes these rows, not a caller
boolean or mutable labels alone. Existing public regular-only classifications
remain supported. Footprint overlay may alter ordinary packet/band role, but
cannot change outside/mapped/mixed source coverage or drop cap-piece ranges;
its produced transport retains the original classification capability. This is
provenance of the SAME classifier operation, not a second geometric checker.

Mapped cells need not create ordinary output corners or obtain an ordinary
regularity proof; they are retained coverage, not silently pruned material.
Ordinary packet/band facets use only the ordinary partition. Construction cap
facets cover the mapped partition. No second source-space coverage tree is built.
This first wedge profile has a strict cut leaving a regular opposite remainder;
fully mapped faces are a separately named unsupported topology until explicitly
supported by the construction plan.

## Explicit local transition

Proposed literal API, alongside existing begin/replace_boundary:

    SurfaceDomainReplacement {SurfaceBoundView expected;
        FaceDomainLease domain; SurfaceFaceIdentity face;
        ExactTable values; span<RootAllowance> allowances;}
    SurfaceCacheSession::replace_domain(const SurfaceDomainReplacement&);

    TrimDomainReplacement {SurfaceBoundView expected; FaceDomainLease domain;
        ExactTable values; ExactRef positional,node_source;}
    FaceTrimSession::replace_domain(const TrimDomainReplacement&);

The caller supplies a produced NEW domain, not mutable rows. Preflight requires
current selected view identity AND retained selection equality, same source bank,
resource/query/definition/face/assignment/profile/physical snapshot and boundary
counter; newer face/domain revisions; chart revision unchanged unless a produced
branch/chart proposal explicitly advances it; and exact quality equality by
value. Surface root closure and every per-root exact allowance remain unchanged.
The operation is idle/drained: no active jobs, ticket, point/facet operation or
unfolded classification. Completed results can first be captured/released using
the existing release_operation. An invalid/stale request changes nothing.

Stage new domain/classifier storage and required metadata/reservations before
commit. Then invalidate current cover, classification/provenance, frozen packet
footprint, ordinary/cap bounds and construction. Keep source bank/control tiers,
SAME PointPool, demanded stages, semantic point identities, exact-table ordinal,
operation ordinal, assignment epoch and cumulative work/statistics. Advance cover
revision on the next minted view. Old host leases remain readable; old resident
operations fail their captured full keys. This is not assignment retirement or
physical boundary replacement and never calls the device retire API.

FaceTrim internally consumes construction's produced local cut replacement after
ONE complete failed-facet horizon. No new needs_domain/coordinator controller is
introduced. It stages all chosen angular/radial cut changes, applies one checked
replacement, reclassifies once, reselects, samples/rebuilds once, and rebinds all
changed source maps/actual Q. Packet footprint freezing is now within one
(boundary_generation,domain_revision), not physical boundary alone. Source-only
control/point/bound tiers may remain cached, but all old domain-classification
flags must be invalidated. Keep the existing fixed FACE secant metric through a
local replacement when the chart is unchanged: the initial metric still comes
from the canonical first admitted ordinary leaf (regular remainder required),
not a new source-jet bootstrap. It does not reset semantic anchor birth horizons
or contraction history on a local cut. A source boundary replacement keeps its
existing separate transition and invalidates/reproduces the domain under that
new physical snapshot.

## Point cycle and physical authority

1. Construction produces a UV partition/cut PLAN without cut Q. Source-only sheet
   sign may be proved by geometry at this stage, never by a fake successful bound.
2. Classify/select through the SAME SurfaceCacheSession to obtain a real current
   view, with mapped coverage retained.
3. Resolve cut sites by typed authority. Interior source UV points are sampled
   using SurfacePointRequest and the SAME absolute/grid PointPool. A cut on an
   ORIGINAL protected meridian must use an existing captured PhysicalNodeKey or
   request canonical BoundaryKey refinement through AdaptiveBoundarySession.
   Never insert a private surface Q on the original source chain. Two seam
   sectors retain distinct occurrence/chart sites while sharing that physical Q.
4. Construction binds the settled actual Q/revisions to the plan; cap work reads
   pole/left/right roles from that captured construction and domain. Shared cut
   sites have identical values on cap and remainder. Numerical point promotion
   changes view/construction before a NEW bound; it never rewrites L in old work.

Missing physical cut sites use existing needs_boundary feedback and complete
original-definition union. Fixed original V/declared source representatives are
not promotable. An upper ideal-pole or band term is NOT proof of a true source
floor. Unsupported/unresolved fixed-source/precision cases stop visibly; do not
refine forever or report witnessed infeasibility from an upper bound.

## Resume across pooled boundary assignments

Component captures continuation on BOTH complete and needs_boundary before
retiring an owner. A completed face may later be invalidated by an incident
physical split, so it retains that continuation for the entire query and supplies
it when freshly assigning the pending face after forest convergence. Therefore BOTH-side CapCutControl cannot
live only in FaceTrim's mutable Impl or be reset on each physical epoch.

    FaceContinuationKey {source_bank,source_generation,query,definition,face,
        originating_boundary_generation,profile_revision}
    FaceContinuationData {resource,source,origin_key,values,positional,node_source,
                           CapCutControlLease cap_control,optional<MetricFactorLease> metric}
    using FaceContinuation = shared_ptr<const FaceContinuationData>;
    TrimFaceQuery::continuation             // trailing empty by default
    FaceTrimSession::continuation() const;  // complete OR needs_boundary, immutable capture

The continuation retains only necessary immutable cut/contraction/source-locator
history, exact quality and (once initially chosen) the sealed exact metric factor4
with its source/chart provenance. This small metric policy survives local cuts
and radial boundary retirement; it is not a retained mesh/cache. Fresh quality
still starts with the canonical initial metric. Other source/chart changes must
follow the constructive metric policy explicitly. It must not retain mutable cache pages, an unfinished
mesh, old global row offsets or old sampled Q. SourceEdgeID +exact ORIGINAL t,
original use/sector/junction and logical physical keys identify cut endpoints.
On resume validate same bank/source/query/face/profile/exact quality and a newer
compatible physical snapshot; recapture every Q/revision/row from that snapshot.
The old cut samples must still exist in the monotone within-query forest, or
resume fails. New assignment epoch is deliberately new; originating epoch is
provenance, not current resident authority. Construction reproduces a new domain
with current keys and carries forward BOTH-endpoint contraction history, checking
new selected UV/source gaps rather than pretending old Q/UV is current. After a
one-sided candidate improvement, yield needs_boundary with continuation only;
do not emit an old-cut cap that skips newly inserted meridian master nodes.
Commit a radial cut only after BOTH gaps prove the selected3/4 contraction and
each chosen cut is pole-adjacent in the CURRENT master chain. Each fan radial
boundary must be one actual current master interval. This is checked even when
both seam sectors share one original spatial edge/t.

Root Component stores one continuation per OriginalFaceID before retiring its
owner on complete OR needs_boundary, and passes it on any later assignment.
Construction CapCutDirection::retain seals initial history/fixed metric without
advancing angular/radial policy when a completed face is captured. Fresh QUALITY/query never reuses
that history: it follows the canonical BASE/cut policy from immutable source.
Coarse-after-fine must not inherit tighter cuts. Local domain replacement remains
within the same assignment and preserves its history directly.

## Resident cap operation and complete fold

Use a separate typed native auxiliary operation so existing mandatory
TrimFacetWork/SurfaceFacetResult contracts do not become nullable or reinterpreted:

    SurfaceCapFacetRequest {view,construction,domain,FacetAllowances,
        selected_facets,ordinal,TrimFacetLimits}
    SurfaceCacheSession::bound_caps(const SurfaceCapFacetRequest&);
    SurfaceCacheSession::cap_result() -> SurfaceCapFacetResult;
    SurfaceProgress::caps_complete

Geometry's next host adapter supplies `prepare_cap_facet_work` and
`pack_cap_facets` against the actual produced map schema. CapFacetWork retains
source/view/construction/domain/exact quality, full keys and copied selected
facet subset, exact cut/finite-source refs and actual Q roles. It does not sample.
Packer returns bounded POD batches of PreparedCapSource descriptor +CapFacetInput
from the reviewed geometry API. Host exact/filter physical-nonzero admission uses
the common actual_facet_nonzero helper. Ordinary bound rejects mapped_cap facets;
bound_caps rejects ordinary facets, wrong roles/pieces and stale source authority.

SurfaceCapFacetResult retains work, ordinal, full operation keys, sealed values
and sorted construction-facet answers, with conservative position/band/total
components, directional cut terms and accepted flag. Dispatch the exact face tau
unchanged: the cap TOTAL includes its named source band once. Do not subtract
that band before dispatch and add it again, or charge delta as another output
approximation allowance. A certified equality can
retain exact tau as accepted TOTAL threshold, labelled as such; diagnostic
components are not relabelled measured residuals or mandatory infeasibility.
Geometry may return typed precision/actual-point requests only with a proved
source-floor contract, never from interval width or ideal-pole upper alone.

The same one-outstanding-ticket native owner dispatches CPU NativeTask slices
with disjoint POD output ranges or root's cap CUDA extension on the existing
resident source session. No per-cap module/session, copied source bank, nested
scheduler waits or duplicate ordinary CPU geometry is added. Descriptor constants
are prepared once/source/profile tier; staging/results and actual transfers are
counted. All inputs are owned until CPU jobs/device ticket drain. The preregistered
module/token/reservation/poison lifetime and process-life failed-drain retention
remain unchanged, including failed create/close. Pooled retirement drains the
new operation before reusing pages; old host cap results survive.

FaceTrim partitions a complete sorted facet horizon by generator, executes
ordinary and cap auxiliary operations sequentially on the same owner, and folds
only after BOTH finish. No accepted partial face is published. Near-threshold or
uncertainty diagnostics select bounded precise comparison using the SAME source,
map and actual Q; far geometric bound failures go to cut refinement, not default
exact trig. Exact positive tau is native; downward fast tau0 is valid unresolved
input, not backend-dependent refusal. Preserve common CPU/P100 arithmetic/profile
and canonical result order. Changed map/cut/Q invalidates the corresponding
accepted answers; unchanged ordinary map/L proofs may be reused only under an
explicit exact-identity match. Publication requires ordinary+cap acceptance,
physical nonzero, complete typed domain partition and actual source orientation
once; root's versioned writer/normal consumer must retain mapped semantics or
refuse, never serialize a proxy as an ordinary original-UV facet.

## Files, dependency order, REDs and bounded qualification

After shared schema and plan review only:

1. Construction publishes lightweight produced CapDomain/control/continuation
   rows and mapped facet references; geometry publishes source/math handles;
   root publishes the optional SAME-session cap API/handshake. Agree headers
   before editing any product consumer. No full-session include cycle.
2. Own adaptive_surface_view.hpp, adaptive_surface_cache.hpp/.cpp,
   adaptive_trimmed.hpp/.cpp, narrow cache begin/replacement helpers and shared
   trailing key guards. Root owns Component continuation storage/CUDA/CMake;
   construction owns caps/domain/classifier/topology; geometry owns cap work
   preparation/packing/precise math. No independent commits.
3. Direct missing-operation RED against actual typed declarations, then coherent
   source-backed fixture. It uses a real produced UV/cut plan and real minted
   view: sample actual cuts only afterward, bind construction and invoke the
   resident cap operation. No fake completed cap or forged view.
4. Admission/stale tests: regular PatchSession/trace-purpose refusal, quotient
   without/foreign/older-assignment caps refusal, explicit empty-cap regular face,
   old module handshake refusal, oldview/domain/result rejection, same-boundary
   newer-cut success, unchanged exactquality/root closure, boundary transition
   distinct and true parameter alias sharing. Include mixed-box regular band.
5. Full-horizon/refusal/lifetime: two cut targets fold once; resource failure before
   commit leaves old state usable; active jobs/ticket prohibit replacement/retire;
   cancellation/healthy close drain; old host results survive domain/assignment
   changes. Existing failed-drain source review/protocol is reused, no driver
   fault-injection campaign. CPU workers tested under existing sanitizer/concurrency
   profiles, not a new broad corpus.
6. Actual source-backed CPU1/parallel/P100 cap+remainder construction with one
   forced physical-boundary round carries BOTH-end contraction control through
   capture->retire->newforest->freshassignment; cold/fine/coarse/repeat semantic
   policy and physical source incidence remain deterministic. Existing independent
   original Fsrc/SAME-L cap reader and chain reader consume the outputs, not a
   second bound/coverage certifier. Counters distinguish cap kernels/transfers,
   precise fallback, domain rebuilds, and physical boundary rounds; no GPU-win or
   termination-at-every-threshold claim.

This plan does not implement a general singular atlas, arbitrary authoritative
B/T maps, wire/display schema, new inverse, source-envelope expansion or blanket
trace admission. Shared fields/API owners must review this literal protocol
before product edits.
