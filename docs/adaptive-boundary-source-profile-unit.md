# Retain the assembly source profile through shared boundary traces

2026-10-17. Implemented; root actual-source review and bounded direct checks
accepted the handoff and coverage admissions below. This unit owns only
the boundary-session carrier-bank handoff and its direct fixture. It introduces
no source factory, pole wedge, new evaluator or CUDA operation.

## Problem and selected interface

The component caller compiles one regular SourceCache from the exact outward
carrier rectangles returned by prepare_adaptive_assembly. Current
AdaptiveBoundarySession::prepare_traces instead calls trace_carrier_requests,
which derives a SECOND rectangle from hints and padded guide hulls and opens a
trace-only SourceCache. That second rectangle can exceed the original B0 profile
used for transform-storage eta. Its automatic expansion is inappropriate once
the caller has prepared exact assembly allowances.

Keep the existing standalone constructor for callers that make no AssemblyStore
claim. Add this explicit overload, with the agreed argument order:

```cpp
AdaptiveBoundarySession(resource, source, definition, curves,
                        SourceCache carriers, CurveLimits limits, stop_token);
```

The supplied-bank constructor retains the SAME SourceCache lease. It verifies
source/resource identity, equal source generation and numerical profile revision,
and all required face carrier original/prepared closure before any forest/query
mutation. In this mode prepare_traces never invokes trace_carrier_requests or a
source-cache factory. It does not union hints, expand endpoints or relabel purpose.
The coordinator owns the straightforward producer composition: compile its bank
from AdaptiveAssembly's retained cache_domain rows, pass that exact bank to both
the boundary session and face pool. This unit does not duplicate B0 computation
or make the boundary layer depend on AssemblyStore.

Append `std::shared_ptr<const SourceCache> carrier_source` to
SharedCurveSnapshotData, analogous to curve_source. A forward declaration suffices
in the common header. Every emitted snapshot retains its actual carrier bank,
including standalone auto-prepared sessions. The component verifies
`&snapshot->carrier_source->view()==&fixed_source.view()`. Retained snapshots stay
readable after session close; the pointer exposes no mutable device storage.

## Admission is the PREPARED rectangle, not the original evaluator span

Read actual source_cache.cpp preparation and adaptive_curve_geometry.cpp source
lookup. Rational original roots retain FULL original spans and closed neighboring
evaluator roots; they can extend beyond the requested prepared rectangle.
Consequently find_original_carrier_root alone cannot enforce this profile.

Build one owner-local indexed rectangle directory from the immutable cache's
PREPARED roots. SourceCache's factory already admits one rectangular request per
carrier and constructs its positive disjoint span/panel partition. Unioning the
closed prepared boxes therefore recovers that admitted rectangle; this is not
a new coverage checker. The required original roots must exist and prepared
original references must refer to their same source carrier. Missing carrier or
empty/invalid prepared closure refuses before opening a query. Other carriers
in a fixed assembly-wide bank are retained without being rebuilt.

At prepare_carriers, check EVERY chosen exact original UV task against this
directory before source carrier sampling and interval tasks. A selected affine UV segment between
two admitted endpoints stays in the SAME convex rectangle. Entire selected
trace coverage therefore follows by construction; endpoint checks here do not
claim coverage of a nonlinear original authoritative pcurve.

For each AUTHORITATIVE use in supplied-bank mode, also bound its entire ORIGINAL
mapped-s image inside the admitted rectangle. All map breakpoints are already
mandatory forest samples; exact mapped_s extrema over that complete source-use
chain cover the continuous piecewise map. Use the common curve service: a cheap
original homogeneous/analytic hull first; on ambiguous rectangle containment use
enclose_curve_hull_precise for original line/rational sources. No RN hull is
clipped to the rectangle. If that service has no tighter finite analytic profile,
return typed unresolved rather than inventing raw source containment. A retained
checked WHOLE-use line-image identity already proves the original image equals
the selected segment; its admitted endpoints and convex rectangle cover the whole
raw image directly, avoiding weaker control-hull work. Nonzero
period lifts, if present, use the existing certified periodic interval machinery;
unknown periodic axes refuse. A conservative unresolved is not invalid geometry.

Reconstructed raw guides are proposals, so they need not lie inside the supplied
profile. Their CHOSEN UV must lie inside, and original V/Ce→S(chosenUV) guards and
whole carrier-trace→actual-chord bounds remain unchanged. A guide outside does
not authorize expanding the prepared carrier rectangle. Out-of-profile chosen
sites return explicit unsupported/unresolved and publish no snapshot.

## Purpose and the three singular candidates

Both regular_patch and trace_geometry caches provide original evaluator roots
to the common trace functions. Accepting either for boundary evaluation changes
no cache purpose. Component face pools still require the actual regular prepared
bank; an independent trace-only cache cannot be passed off as face capability.
The existing standalone full-period trace case remains legal, while the same
trace-only lease is refused by PatchSession/SurfaceCacheSession as before.

The three actual cone/sphere singular-envelope candidates remain explicitly
unsupported by the current source factory's regularity admission. trace_geometry
only bypasses angular-width restriction: it still rejects a cone apex or sphere
pole crossing. This handoff cannot manufacture those missing roots, suppress the
failure, shrink the assembly envelope or silently use the old geometry callbacks.
Source-vertex wedge/cap work will later provide separate admitted source
evaluation and regular remainder/fan roles under its own source-map contract.

## Grounding and implementation boundaries

Actual sources read: adaptive-component-coordinator-unit.md; adaptive_curves.cpp
prepare_traces/trace_carrier_requests/freeze; adaptive_traces.cpp
validate_request/prepare_trace_sites/prepare_trace_intervals; source_cache.cpp
original/prepared root construction; adaptive_assembly.cpp profile freeze; common
original curve hull helper. No new algorithm is selected: convex containment and
original positive-weight hulls are the SAME owned mathematics from the assembly,
curve and trimmed-numerics plans.

Reference ownership is consistent with pinned USD/BRep
c9c5979f04cea8c38213004d4f04df0dcfb40d05 SmTess::Phase1SetupBrep and
Phase2CreateFacePolygons: shared original edges feed face charts. OCCT
b8f597c677811d1f9f4d8a97f5ae2825c0353a42 BRepMesh_EdgeDiscret::Tessellate2d
updates every occurrence from one physical chain. These references motivate
retaining one supplied source/profile; their repair/tolerance policies do not
permit expanding an already transported original source envelope.

Owned files: adaptive_curves.hpp/.cpp, the trailing common snapshot field in
adaptive_curves_types.hpp, existing tests/native_adaptive_curves_runtime.cpp and
its existing source fixture. No coordinator, surface runtime, trace implementation
or CUDA files are edited. A helper private to boundary session computes admission
from supplied roots and delegates raw source hulls to existing geometry service.
Keep existing task order, source guards, numerical cache and device dispatch.

## Direct RED and finite completion

Extend the existing real shared-boundary fixture, not another producer. Prepare
a legitimate fixed carrier bank covering its exact original face rectangle and
construct the new overload. Missing overload is direct RED. Demand a real complete
boundary snapshot and pointer identity to that bank after close; existing source
reader/guard assertions remain the direct geometry oracle.

In that SAME source operation prepare a legitimate narrow subrectangle. The
session must refuse an outside CHOSEN site and expose no partial result, even
for rational sources whose original evaluator spans still cover those points.
Missing carrier closure and a foreign SourceStore bank refuse; a narrow valid
analytic profile must never call auto envelope preparation. Existing seam source
checks prove distinct UV occurrences still share actual physical Q. For an
authoritative bowed pcurve test already present in the source families, require
whole raw-hull containment/refusal, not merely endpoint membership.

After reviewed header/fixture RED, implement the handoff/admission, run the
affected existing CPU source-backed boundary fixture, one scoped ASan/UBSan/leak
path and strict format/tidy. A single existing P100 boundary fixture may verify
unchanged typed service integration; no new GPU numerical campaign is required.
Request actual-source review, fix concrete findings, save journal/checkpoint and
return to component integration. No imported movement mesh or pole run belongs
to this bounded unit.
