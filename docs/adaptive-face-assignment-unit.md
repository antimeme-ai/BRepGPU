# Bounded reusable face assignments

2026-10-17. Finite runtime subplan for review before implementation. Root owns
the CUDA retirement extension, CMake and component coordinator. This lane owns
the native face/cache retirement invariant and one actual-face fixture.

## Source-grounded choice

`adaptive-component-unit.md` selects a bounded owner pool, one complete shared
boundary horizon and immutable host results. Current FaceTrimSession::begin
requires a newer query and resets table ordinals; SurfaceCacheSession::begin
allows equal-query continuation only for its current face. Core nodes, point
tiers, exact-anchor index and controls persist after release_query. DeviceSession
also retains host_point_valid and host_control_valid. Those are the actual
mechanisms this unit changes. NVIDIA cache/face staging cited by the component
design motivates retained immutable source work; the epoch/drain/ownership rules
below are owned lifecycle reasoning, not a numerical theorem from that source.

The source bank is one SAME retained multi-face SourceCache for the pool lifetime.
It contains the requested original/prepared roots; each face selects only its
carrier's allowances. Device create uploads this fixed bank once. No bank rebind,
geometry policy change or source-generation replacement is introduced.

## Literal public schema and state

* Append `uint64_t assignment = 0` to TrimFaceQuery. Zero preserves the existing
  single-face strictly increasing-quality API. Explicit pooled assignments are
  nonzero and strictly increase per FaceTrimSession, even across quality changes.
  A session selects legacy or pooled mode on first successful begin; do not mix
  the modes. Global quality cannot decrease. Exact SourceStore/SourceCache lease,
  resource, definition/face relationship, complete boundary and allowance checks
  still precede mutation.
* Append `uint64_t assignment_epoch = 0` to SurfaceFaceIdentity and
  FaceConstructionKey. Existing complete comparisons then distinguish old work.
  Explicit domain/construction-to-view checks also compare this field. No facet
  generator or external artifact wire format changes are needed: this is owner
  capability identity, not geometry.
* Add `void FaceTrimSession::retire_assignment()` for pooled complete OR
  needs_boundary state. The caller retains result() or COPIES boundary_requests()
  before retirement; the span does not survive retirement. It clears mutable
  construction/classifier/anchor state and returns the owner to idle. The prior
  immutable FaceTrimResult, boundary and view leases remain readable. Legacy
  release_result is unchanged; pooled release_result delegates to retirement.
* Add `void SurfaceCacheSession::retire_assignment()`. It accepts only drained
  selected/idle state with no jobs or ticket and no incomplete auxiliary work.
  Complete auxiliary results are released exactly as release_operation does.
  It invalidates native resident captures and clears mutable page contents.
  Active/pending retirement fails before mutation; close/cancel are not reusable
  assignment reset operations.

Same-face boundary replacement keeps the same assignment, requires advancing
face/boundary generations and retains current demanded point tiers. A new
assignment starts its own BASE policy selection, even at equal quality. Its
surface begin may be lazy: if domain construction already yields needs_boundary,
there is no surface operation or device epoch to install for that assignment.
The next actual surface begin advances directly from the last installed epoch.

## Exact identities and admission order

Validate the full candidate, every exact reference/allowance and checked epoch/
ordinal capacity before retiring or binding state. Rejected preflight leaves an
idle owner usable with its prior counters unchanged. Same-quality assignment
never resets either face or surface exact-table ordinal. A newer quality may use
the legacy reset rule because generation changes; within one quality the owner,
generation and ordinal can never identify different pools. Bounds, candidates,
point work and construction keep captured assignment identity; an old view is
not eligible merely because face, quality or source coordinates coincide.

Surface begin stages validated allowance rows before begin_core mutates its
query_values/allowance arrays. For an explicit new assignment it requires prior
retirement (or an initially empty owner), advances the device epoch if open,
then commits native query/identity. Failure of the device barrier is a visible
failed owner operation, not a successful reassignment; existing close/retry/final
poison retention protects source/module/reservation lifetime.

## Pages and the actual device barrier

Native retirement clears nodes, point data/tier/index maps, sparse exact-anchor
index, root-node slot map, child control tail, bound tiers, all frontier/task/
answer vectors, classification/target maps and auxiliary work/results. Controls
retain exactly the immutable source prefix. Vector capacities may be reused;
their existing caps bound the single owner's high-water storage. Keep cumulative
work, statistics, table ordinals within quality, batch IDs, source bank, workers,
module and reservations. Old host values were copied into RunResource-owned
immutable tables/selections, so none borrow mutable cache array offsets.

Root's `AssignmentCudaAPI::retire(session,expected,next,error)` runs once at each
actual explicit surface assignment begin, initially0->assignment. It requires no
outstanding operation, validates expected/monotone next, synchronizes the stream,
clears point-valid flags and child-control validity beyond the immutable prefix,
and preserves the bank/buffers/stream/tickets/counters. Public native retirement
does not guess the next epoch; retained device capacity remains bounded and
unreachable through native active-view APIs until the next begin barrier.
Restrictions must never overwrite immutable source controls. No base ABI change
or second CUDA lifetime implementation is selected. Root directly tests stale/
busy epoch and root-residency behavior; this lane tests actual native captures.

## Implementation files/order

Own adaptive_trimmed.hpp/.cpp, adaptive_surface_cache.hpp/.cpp,
adaptive_surface_view.hpp and adaptive_cache_detail.hpp. Coordinate the one
trailing FaceConstructionKey field and exact guard additions in adaptive_trim_types,
regions and trimmed geometry with their owners; do not independently upload
shared files. Preserve PatchSession's strict legacy behavior and numerical code.

1. Author the actual fixture against the proposed public API and capture its
   missing-operation/equal-quality refusal RED on the fleet before bodies.
2. Add identity/admission and bounded page retirement in the shared core, then
   SurfaceCache and FaceTrim outer wrapper. Bind the reviewed CUDA extension.
3. Run the focused direct fixture, independent existing original-source chain
   reader, relevant CPU concurrency/host lifetime sanitizer and both P100 native
   paths. Root owns module protocol/memcheck. Source-review actual code, close
   concrete findings and rerun only affected checks. No commits in this lane.

## One finite fixture and its oracle

Add tests/native_adaptive_face_assignments.cpp plus its external source/reader
driver. Use one actual SourceStore definition, several original faces with
distinct carrier identities, one complete shared boundary authority and one
multi-face SourceCache. A surface node/point cap sufficient for ONE affine face
but insufficient for accumulating all distinct carrier pages makes retirement
necessary. Run more assignments than that cap can accumulate, all at one quality,
then a later quality. Every completed result is written through the existing
BGACRV/BGATRM writers and checked by the existing exact source/chain reader.
Retain the first result through reassignment and healthy close and read it again;
its exact refs, source association and XYZ must remain intact.

Capture a real old view, then attempt sample/bound/refine submission after a new
assignment and require precise invalid_source before work/mutation. Try a bad
face/foreign table/nonmonotone epoch, then complete a valid assignment through the
same idle owner. Compare exact-table keys across same-quality assignments: no key
may be reused for another table. Exercise the needs_boundary retirement route
with a real coarse closed curved boundary whose selected polygon needs source
refinement; capture its BoundaryKeys and finish the shared horizon outside the
owner. No fabricated completed face or supplied bound decision is allowed.

CPU1, actual parallel CPU and both P100s use the same fixture. Assert current
private storage remains under its configured caps, separate from intentionally
retained host outputs; one module open/close and monotonically counted uploads
remain observable through existing stats. This is a bounded lifetime claim,
not a performance speedup or a new numerical qualification campaign. If the
real needs_boundary source cannot be expressed by existing admitted operations,
report that gap rather than injecting an authored progress state.
