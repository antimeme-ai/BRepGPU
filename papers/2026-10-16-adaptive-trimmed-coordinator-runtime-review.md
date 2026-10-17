# Trimmed-face coordinator: runtime source review

Scoped actual-source review of `native/mesher/adaptive_trimmed.cpp` after the
parent's first complete plane/tiny-hole caller closure. This review covers the
outer coordinator and its use of the independently owned surface service; it
does not self-approve the surface service implementation or replace the
independent original-source/chain oracle.

ACCEPT for the current identity-domain profile. Admission retains the exact
source, definition, face, query and immutable boundary snapshot. It checks the
requested node cap and whole protected carrier-trace cap before numerical
refinement, and admits chosen UV against the fixed source envelope. Every new
boundary generation rebuilds the domain classifier and footprint. Same-quality
replacement preserves the query and exact allowances; the inner service requires
strictly newer face/boundary generations after release. Fresh quality queries
remain separate and reset the point decision ladder to BASE.

The phase transitions retain whole operations: classification precedes source
child work, anchor results are real service results in request order, completed
operations are released without discarding the selected query, and a facet
horizon is checked against both construction and captured surface keys. Failed
unchanged facets are explicitly rechecked after point promotion. Actual point
floors are batched through the common authority before rebuilding; physical
boundary floors stop visibly. The coordinator never substitutes private XYZ or
changes SAME-L within an old bound work packet.

Anchor compaction discards the old point-result handle and resamples the retained
logical requests before the next construction. This preserves source-point
provenance rather than manufacturing a shortened result. Fixed packet footprints
and explicit packet-grid targets preserve progress; boundary requests yield to
the existing shared owner rather than making a second forest. Only changed/new
facets, plus unchanged previously failed facets, receive the next numerical
horizon. The accepted final exact tau is labeled a conservative threshold, not
an observed residual.

Owner destruction closes/drains the inner service before releasing its captured
state. The surface owner's preregistered device record remains responsible for
exceptional failed-drain retention. No extra execution campaign was requested by
this review. The parent's independent full caller/original-source checks are the
integration evidence; further service source review belongs to the sibling.

2026-10-17 final inner service reread: scoped ACCEPT for
`adaptive_surface_cache.cpp` target preflight/DAG commit, exact compact/sparse
source point identity and sealed leases, point promotion/reselection, captured
construction/view/domain/subset admission, disjoint SourceJob output ranges and
whole fold. `release_query()` now calls `release_operation()` before discarding
its view: none/complete auxiliary phases and no live ticket/jobs are required,
then point/facet payloads are cleared. The earlier lifecycle finding is resolved.
This review does not replace the numerical owner's source-bound review. No
additional execution was requested or performed for this reread.

Follow-up: the parent selected a combined owned stop_source forwarded from both
external lifetime and caller operation tokens, plus per-step private construction
contexts and failure cancellation/accounting. The cancellation and exception
retention logic is scoped ACCEPT. A remaining accounting seam was reported:
persistent classifier work lives in its own Budget and must be charged by delta
against the advertised total (including initial build), with a same-owner scoped
cap for each call; otherwise it must be documented as a separate cap. This is a
concrete accounting closure request, not a new geometry algorithm or extra test
campaign. The parent owns its resolution in the outer coordinator.

## Final cancellation and classifier-accounting reread

2026-10-17, actual updated source plus augmented
`tests/native_adaptive_trimmed_work.cpp`: scoped ACCEPT. The accounting closure
requested above is implemented. The root owns one stop_source; external lifetime
and per-step caller tokens forward into it. The source service, persistent
classifier and private per-step context retain that combined token. Lifetime
ordering keeps the stop source alive through children and callback teardown.
A computation or caller chargeback failure cancels the owner before rethrow, so
partially advanced state cannot resume as successful work. The original
computation exception is retained if chargeback also fails.

Each step's context is capped by the remaining construction work. The
`remaining_work` subtraction checks both cumulative work and the current context
before subtraction. Initial classifier construction receives only the remaining
allowance after domain preparation, and its initial work is charged to the
context. Both classifier operations take an explicit allowance, install a
same-owner scoped cap before arithmetic, and expose monotone work deltas. The
metered wrapper accounts those deltas on normal and exceptional paths; no second
classifier Budget or foreign Rational was introduced. Boundary replacement also
uses the remaining construction allowance. The advertised owner cap therefore
includes retained classifier work instead of resetting it per boundary epoch.

The existing real-work fixture preserves its original-source virtual-piece
output and adds protocol assertions using that SAME source and boundary. It
checks complete bound identity/subset order/exact allowance; rejects old
construction against a newer view; observes owner cancellation; and checks an
inactive outer call, a pre-requested caller stop after real classifier creation,
terminal cancellation/no partial output, and external stop before begin. These
are the actual checks; this review does not claim an execution that interrupts
a classifier midway through an arithmetic instruction.

`limited.construction_work = 1` is explicitly ONE WORK UNIT, with memory limits
unchanged. Its expected outcome is `bgfd::Failure` carrying
`BGFD_RESOURCE_EXHAUSTED`, not a one-byte allocation failure or an untyped source
refusal. It is a bounded admission-cap check, not a measured solver threshold.
The parent's current strict/instrumented executions own the test results; this
source review added no extra executions or geometry families.
