# Explicit UV source preparation for OM10

Design for review,2026-10-15. This is the next source capability after the
processing-policy correction, not a restoration of whole-shape healing. No
implementation is authorized until this design/subplan is reviewed.

## Actual source and selected boundary

With actual no-processing flags, all107 original solids and9,162 faces export
faithfully to the raw NVIDIA neutral boundary. The owned extractor stops at
missing pcurve coedge109547, edge83676, circle5435 on cylinder14336 of face164974.
The complete no-processing audit finds51,742 coedge uses/25,871 spatial edges,
zero SameParameter-false edges and zero unmatched original/actual edges:

| Carrier | Missing pcurve uses |
|---|---:|
| cylinder |18,947|
| cone |2,830|
| sphere |64|
| torus |220|
| B-spline |2,541|

All27,140 planar uses receive on-demand plane pcurves. Every nonplanar face has
one original wire. The375 multi-loop faces,981 loops total, are all planes;
each has exactly one original FaceOuterBound at index0. Native geometry audit
also finds that all635 spline surfaces are neither closed nor periodic in U/V.
Thus this unit need not invent periodic-hole placement or closed nonperiodic
spline sheet selection. Those unsupported cases must fail explicitly.

The raw no-processing neutral remains **NVIDIA's input**. Pinned NVIDIA code
explicitly accepts missing pcurves, keeps original3D loops and lazily constructs
UV trims, including general NURBS. Adding this owned projection cost to its
fresh wall would be an artificial baseline handicap. The new preparation serves
the **owned source snapshot and OCCT boundary-query copy**. It is named and
timed separately. No NVIDIA triangle or owned triangle replaces its input.

Read pinned OCCT8.0.1 revision
`b8f597c677811d1f9f4d8a97f5ae2825c0353a42`: ShapeConstruct_ProjectCurveOnSurface,
ShapeFix_Edge::FixAddPCurve, ShapeFix_Wire::FixSeam/FixShifted,
ShapeExtend_WireData::Init/ComputeSeams, ShapeBuild_Edge::ReplacePCurve,
BRep_Builder's curve-on-surface UpdateEdge/Range and BRepBuilderAPI_Copy.
Also read the independent
[method/callee review](2026-10-15-om10-occt-2d-method-adversarial-review.md).
These are explicitly foreign source conversion, inside the already authorized
isolated OCCT bridge. Product trimming, triangulation and display remain owned.

## One shared native preparation, original geometry retained

Add `native/source_uv/source_uv.hpp/.cpp`, linked by the owned source adapter.
It prepares a deep detached copy of one original solid using
`BRepBuilderAPI_Copy(copyGeom=true,copyMesh=false)`. Copying TopoDS wrappers or
a wire alone is not isolation. Original raw transfer bindings remain available
for graph matching and source identity. Copied face/edge/vertex bindings are
resolved through the copier's original→copy map for geometry extraction.
Each face binding carries original face/carrier ID and original ordered loops;
each use carries original loop/coedge/edge IDs, source directed endpoint IDs,
composed native orientation and the native edge. This provides seam ownership
without guessing from coincident endpoints or unordered native exploration.

Before preparing a face, capture its copied surface handle/location/orientation
and its original edge/vertex incidence. Capture each copied edge's3D curve
handle/location/range, endpoint vertices/coordinates, SameParameter/SameRange,
degenerate flag and tolerances. Check those values after selected operations.
No numerical source-coefficient fingerprint framework is introduced: reviewed
selected bodies do not mutate input3D geometry values, the original is fully
detached, and the existing independent original-geometry qualifier checks the
actual emitted coefficients once after full extraction. A changed3D handle,
range, physical incidence, vertex, tolerance or orientation fails preparation.
The internal Modified cache flag may change; it is not original source geometry.

Source surfaces/pcurves are keyed by **supporting surface handle+location**, not
STEP face ID. Preflight shared edge/support keys. Distinct faces requiring
conflicting representations on the same supporting key fail explicitly; do not
let later face preparation silently overwrite an earlier validated occurrence.
After all faces, reread every original use from the final prepared shape.

## Exact selected operations and ordered occurrence contract

For each nonplanar face, require the admitted one-wire scope. Build
ShapeExtend_WireData with chained=true, preserving all native uses and reversing
the sequence for a reversed wire as its implementation specifies. Check the
entire directed physical vertex chain, including closing pair, by original
vertex identity. Compare the complete ordered original EDGE_CURVE/ORIENTED_EDGE
word, orientation and multiplicity too; seams can share both endpoint vertices.
Every native use must retain its original ORIENTED_EDGE owner. No chained=false fallback, edge omission, reorder, merge or
coordinate welding. Keep the original face-composed source word for all source/final readback.
For the selected fixer only, derive its local-FORWARD working view: when the
original face is REVERSED, reverse the temporary wire, which composes every
edge orientation and reverses the sequence through WireData. ShapeAnalysis_Edge
PCurve intentionally delegates by surface/location and ignores face orientation;
CheckSeam/ReplacePCurve also normalize FORWARD. Merely passing a REVERSED face
would not correct that convention. The temporary slots map by exact full-word
reverse+orientation to original ORIENTED_EDGE owners. No source wire is reordered
or modified. Retain the original face orientation on the prepared shape.

Use a fresh projector per face, deterministic original wire order, fixed
projection precision1e-7mm and documented default pole adjustment mode1. Its endpoint cache is deliberate per-face state; it must not leak between
faces. The source3D parameter interval is captured **before** asking for a
possibly absent pcurve; null CurveOnSurface can overwrite its output interval.

For each unique edge/support whose pcurve is missing, call the reviewed
ShapeConstruct_ProjectCurveOnSurface directly. Check its boolean, FAIL status,
non-null2D result and unchanged original-t interval before attaching anything.
Attach that checked single pcurve with BRep_Builder's2D UpdateEdge and
face-specific Range. Do not call FixAddPCurve's missing-PC projection branch:
it ignores the projector boolean and can dereference a null result when adding
a seam, before a post-call guard could raise a source error.

For an original seam, first verify the single attached pcurve is readable and
non-null, then use a **fresh** ShapeFix_Edge::FixAddPCurve with isSeam=true.
It takes the existing-PC branch, adding the second branch with the pinned
mature TranslatePCurve logic, including doubly periodic torus handling; there
is no second projection or new seam heuristic. A fresh object's projector has
no stale DONE3 state. Guard3D immediately regardless, and reject missing dual
readback or failure flags. This retains the existing narrow source converter
without its unchecked missing-PC failure path.

Then call **direct** FixSeam for seam occurrences and direct FixShifted on the
closed ordered wire. The first swaps provisional dual pcurves into the native
forward/reversed slots; it does not create them. The second supplies mature
closed-chart period alignment. Neither a boolean success nor false return is
trusted: check status FAIL flags and actual final pcurve readback. FixShifted
can modify curves and still report failure/false.

Never invoke ShapeFix_Wire::Perform/FixEdgeCurves, FixShape, FixFaceOrientation,
FixReorder, FixConnected, FixSameParameter, add/remove3D-curve operations or
tolerance adjustment. No original physical edge may be split or removed.

After seam ordering, retain the actual branch used by every directed occurrence
before/after shift. Shifts are admitted only as true carrier periods in periodic
dimensions, within the existing128ULP binary64 construction grade. Check a **constant
whole-pcurve translation**, not merely equal endpoint deltas or finite world
samples: retain original range, type, degrees, knots/multiplicities and weights,
and compare line coefficients or every B-spline pole against the same integer
carrier-period translation. A bi-meridian replacement or any other basis/range
change fails this phase. Nonperiodic
coordinate changes by these shift operations fail. In particular, sphereV is
not periodic byπ although FixShifted internally treats its bounded V span as
closed; a nonzero sphericalV translation is rejected, not relabeled a period.
The separate proven pole endpoint correction below is not a period operation.
The original nonperiodic B-spline knot rectangle remains the admitted domain.

Projected pcurve intervals/knots, finite control values and selected branches
remain explicit foreign data. Probe original C(t) against original S(pcurve(t))
at17 deterministic samples per active original spatial-knot span, including
exact saved endpoints. A raw projected UV outside the original natural domain
uses OCCT's foreign carrier extension for this diagnostic only, not an original
source-domain evaluation. Final query correspondence probes are in-domain. Require finite results and measured residual≤selected source limit
(.001mm in the watch caller); preserve maximum and offending IDs. Check native
same-parameter interval identity; do not set source flags to manufacture it.
These are finite correspondence probes, not a universal source error certificate.
The owned shared-edge/trim/surface bounds and independent whole-mesh checks
still establish their own stated output contract.

The projected pcurves remain the BGSRC001 foreign source data. Do not place a
new hard-domain gate in front of the owned source-column, finite-joint and
analytic recoveries already implemented in imported_meshing. The generic owned
128ULP global endpoint grade remains unchanged; typed source-based recovery is
still required for any larger excursion.

## Separate query representation and already grounded recoveries

The bounded actual new-projector probe on faces169982/172494/172983/173009/173010
confirms that representation distinction is necessary. For U[0,4] the existing
global grade is128*ulp(4)=1.1368683772161603e-13. New projected values include
U=-1.2227333525817207e-13 on172494, U=4.000000000000135 on172983 and
U=4.0000000000001323 on173010, all outside that grade. Face173009's
U=4.0000000000000924 is inside it. Face169982 retains the real joint gap
U=.43225527967847249 versus .43242913791889115. Its largest17-probe
curve/surface residual is.0002588342311mm; these are foreign conversion effects,
not permission to widen numerical or source budgets.

After the one projection/period pass, keep that raw-PC detached shape for owned
BGSRC extraction. If query output is requested, deep-copy it once and apply
only the following existing source-grounded2D capabilities to the query copy.
There is no second STEP read or projection. Preserve every raw pcurve in the
owned snapshot so its actual recovery provenance and independent checks remain
available. A query correction never changes source3D curves, carriers, vertices,
physical topology, original parameter intervals, orientations or tolerances.

* For a clamped source tensor's natural-U endpoint column, reuse the earlier
  original LINE/column admission: V degree1, two equally weighted positive
  controls, exactly two V knots with multiplicity2, U endpoint multiplicity
  degree+1, nonperiodic carrier, unique fixed column selected by the whole raw
  guide, original LINE and declared endpoints on that column. The native query
  implementation may restrict this proof to the actual axis-aligned column:
  exactly one saved world coordinate differs between its two Euclidean poles,
  the LINE direction has only that component, and the other saved coordinates
  match exactly. This covers the four known actual faces without adding native
  arbitrary-precision geometry machinery. The owned Python capability keeps its
  reviewed broader exact-rational admission.
* On the verified LINE's unchanged original-t interval, replace only its query
  pcurve by the column's affine U/V map. Correct an adjacent non-LINE pcurve
  endpoint only at the SAME original directed loop joint and original vertex
  incident to that verified LINE. Select the column per occurrence, never by a
  face+vertex map that could weld natural-U seam branches. Retain source IDs,
  old endpoint UV and measured endpoint displacement in query diagnostics.
* For the reviewed finite spline joint case, reuse original clamping/domain,
  same-original-U-knot-cell, same natural-V-endpoint and parameter-identity
  eligibility. Evaluate the two retained endpoint candidates against the
  declared original vertex; choose only one of those two (minimum residual,
  original incoming candidate on a tie). Both original curve endpoints and
  their raw curve/surface correspondence must meet the unchanged source budget.
  No solver, invented candidate or carrier-point optimum is claimed.
* Query endpoint changes use the same unchanged-t line/nonrational-clamped
  B-spline endpoint editing described below for poles. A line becomes a
  degree1 B-spline on its original interval. Retain both seam slots. An
  unsupported representation fails explicitly. Bound the UV change by the
  actual positive-basis endpoint/control perturbation, then measure it with the retained source natural-domain derivative metric
  (a world bound only for wholly in-domain paths, as qualified below); require at most
  tolerance_mm/8 (.000625mm for the watch) and recheck finite source residuals.

The per-use admission budget accumulates every successive metric-weighted UV
change by triangle inequality and stays≤tolerance_mm/8; reporting a maximum
individual edit would miss combined column/joint/domain/pole changes. Name it
`max_query_uv_metric_mm`, the original-natural-domain metric-weighted UV gate.
For endpoint/control paths wholly inside that rectangle, the derivative hull
also bounds the composed world displacement. It does **not** bound arbitrary
OCCT extrapolation outside the original domain. In a typed column recovery the
new pcurve reconstructs the proved original LINE/column; an outside foreign guide
is only branch/provenance data, not valid original-domain source geometry.
Generic128ULP outside endpoint correction retains its numerical scope plus the
final in-domain finite C(t)/S(queryUV) checks, with no extrapolation theorem.
Raw and corrected finite correspondence maxima are reported separately. The
owned raw-source mesher's geometry/approximation contracts remain unchanged.

A generic query **coefficient** correction can affect any control of the admitted
nonperiodic nonrational pcurve, not only its first/last control. "Global ends"
refers to original carrier u0/u1/v0/v1. It never means an interior source-carrier
knot. For each out-of-domain control coordinate, require its excursion≤
128*ulp(max(1,abs(globalLower),abs(globalUpper))), then set that coordinate to
the corresponding original global boundary. Leave every in-domain coordinate
unchanged. Preserve pcurve degree, knots, multiplicities, weights and active
t interval; source raw pcurves stay untouched. A line may first receive its
stated unchanged-t linear B-spline representation. Reject periodic/rational
editing and every beyond-grade generic excursion.

Positive basis partition of unity gives uniform coordinate change at most the
largest admitted control change; the final control hull lies in the original
rectangle. Accumulate its natural-domain metric charge with prior edits and
apply final in-domain finite C/S checks. Coefficient correction is not identical
to pointwise `_recover_global_uv` (clamp and spline evaluation do not commute);
it uses the same original global boundary and admission grade for a separate
query representation. It adds no extrapolation theorem. Existing owned point
recovery permits this grade at any original coedge t, so the earlier restriction
to first/last pcurve controls was an overly strict interpretation, now retired.

Actual169982/use138090 has25 projected controls: interior index2 V=
.035000000000000017 and index9 V=.035000000000000052 versus originalVmax=
.035000000000000003. The4.86e-17 excursion is below the unchanged2.84e-14
128ULP unit-scale grade. An authored unit tensor plane with original bottom
LINE and quadratic2D middle control8ULP below naturalV0 exercises this operation;
the same512ULP construction must fail. No new source curve/inverse family is
needed for this numerical coefficient case.

After these explicit query corrections, require the finite nonperiodic query
representation inside its original natural domain. The generic128ULP global
endpoint correction is still the only untyped numerical admission. Never clamp
an interior knot, raise the grade or use a foreign UV excursion as evidence of
source column membership. Unsupported residual/domain cases remain source
preparation failures. Query representation is foreign reconstructed2D trim with
finite correspondence checks and stated perturbation bounds; it is not an
exact source-validity or isotopy assertion.

## Source singular vertices and chart-only connectors

Shifted/projection output is not proof of a physical pole. Reuse the existing
analytic original-vertex proof: an original vertex lies at the sphere pole or
cone apex within2^-32mm of its retained analytic carrier. Both adjacent uses
must be incident to that same original vertex. Arbitrary near points cannot
be merged. CanonicalV on the **query copy** is ±π/2 for sphere or exactly host
`-R/sin(angle)` for cone, matching the already tested limiting-normal convention.

If an endpoint needs canonical poleV, change only its derived2D representation.
Admit the actual needed line or nonrational B-spline pcurve: a line is represented
by a degree1 B-spline on the unchanged original-t interval; a copied B-spline
is segmented on that interval without parameter normalization, then only its
clamped endpoint pole is changed. Parameter-range identity and finite/world
residual gates remain. Positive basis weights bound this2D control perturbation;
record its actual UV/world change. Unsupported representation/parameterization
fails instead of reinterpreting angle as rational parameter.

Between distinct incoming/outgoing UV endpoints on that proven constant-V
singular locus, create a **chart-only** degenerate edge on the prepared copy:
same original physical vertex at both ends, no3D curve and a linear2D connector.
No original EDGE_CURVE/ORIENTED_EDGE ID is invented. Retain source face, vertex
and incoming/outgoing use ownership and the canonical endpoint coordinates.
The original physical-use cycle is unchanged after quotienting only these
explicit connectors. Reuse the existing singular-connector recognition/proof
in the source adapter rather than implementing a second pole checker.

Faces inside a detached copied solid are frozen topology children. Temporarily
set only that query face's Free flag true in an RAII scope around the declared
wire Remove/Add, then restore the saved flag on success or exception. Do not
free any original/raw parent or alter source coordinates/tolerances. Native
TopoDS_Builder::Add explicitly rejects a frozen parent; the same octant nested
in a shell reproduces that exact failure for both orientations. Final FaceState
checks the restored flag, and the actual expanded-wire/connector-neighbor check
still proves the retained original physical word.

This gives OCCT's query face a closed chart boundary and preserves all original
source records. The owned mesher still retains its positive UV cells and explicit
ChartClosure/physical quotient, with every approximation budget unchanged.
Projection's coincident-point UV nudge is never promoted to an original source
edge or a pole proof. No general zero-area triangle exemption is added.

## API, output and query integration

The native public boundary is small:

```
SourceUVOptions { projection_precision_mm=1e-7, source_residual_mm=.001,
                  tolerance_mm=.005, prepare_query=false }
PreparedSourceUV prepare_source_uv(original_solid, original_face_bindings,
                                    original_edge_bindings, options)
PreparedSourceUV.shape()                 // detached raw projected-PC solid
PreparedSourceUV.mapped(original_shape)  // original→raw-PC copy binding
PreparedSourceUV.query_shape()           // optional derived query solid
PreparedSourceUV.query_mapped(original) // original→query binding, rebuilt wires too
PreparedSourceUV.stats()                 // counts, finite probe maxima, time
PreparedSourceUV.chart_connectors()      // source-linked singular occurrences
```

Bindings carry original IDs and native shapes only inside the isolated process.
Use the prepared values for existing BGSRC001 extraction; keep product graph
matching against original transfer shapes. Existing binary fields already store
synthetic2D curve IDs, original source-t ranges and reconstructed=true. Metadata
names the deterministic operation policy/precision, counts and maximum measured
residual; connectors retain separate deterministic diagnostics. Preparation wall
times go ONLY to stdout/external timing rows, never BGSRC001 META/DIAGS bytes:
the full benchmark intentionally compares fresh CPU/GPU source snapshots byte
for byte. BGOCC's existing embedded phase times are a different contract. No JSON, changed
source IDs or new persistent geometry protocol. Raw and query wall times remain separate stdout timing rows; deterministic
query-only correction counts/maxima remain separately named.

Add optional `--query-brep-dir NEW_DIRECTORY`, `--source-residual-mm VALUE`
and `--tolerance-mm VALUE` to the ordinary source adapter; preserve its existing
two positional arguments and defaults. The actual caller passes the same
selected source/mesh grades when preparing query files. Unsupported/nonfinite
or nonpositive option values fail before extraction.
It writes unmeshed BinToolsV4 `definition-<original-id>.brep` files from the same
query-copy solids, before publishing the complete source snapshot. The caller
binds those107 files to that freshly returned source object's definition IDs
and original SHA. Root's qualify_om10.py requests this export once into its qualification output's
query-source directory. Its existing input/report hash mechanism records the
owned source snapshot, original STEP SHA and complete original-definition file
list/hashes. The NVIDIA geometry qualifier consumes THESE prepared query files,
while the NVIDIA import/tessellation worker still consumes the raw no-processing
neutral files from its own fresh source transfer. No new manifest or protocol is
introduced. Query preparation is outside benchmark observations and named in
qualification metadata. No second STEP parse or duplicate UV algorithm.
The raw NVIDIA exporter remains unchanged and does not call this module.

The finite query helper still preflights every actual face-edge pcurve and uses
only trimmed FACE compounds, never filled solids. Its complete source inputs
are now an explicitly prepared query representation of original unchanged3D
geometry. Actual finite query outcomes and solver deflection remain named;
no exact-Hausdorff or source-isotopy claim is introduced.

## Evidence and implementation sequence

Already retained genuine capability failure is missing coedge109547 under actual
no-processing. Add an authored box/cylinder extraction test that currently fails
on missing nonplanar pcurves, plus a bounded source-known pole/seam fixture.
Check actual original-t C/S agreement, opposite seam UV occurrences, natural
bounds, complete use chains, exact original3D coefficients/ranges/vertices and
actual query-usable pcurves. A wrong sphereπ shift, dropped use, changed source
curve/range, missing pole proof, failed conversion or incompatible domain must
raise. Use literal source formulas and existing raw geometry/assembly mechanisms,
not a second audit engine or a broad campaign.

After focused green/review, perform one fresh whole extraction plus query export,
then existing independent original source/placement/assembly qualification and
the actual old/new geometry/foreign-UV delta. Root decides V4 only after that
source unit closes. No source data changes, tolerance/cap increase, native image
or target benchmark is authorized by this design.

## Source chart approximation for conics at a natural boundary (design)

Actual fresh attemptv3 reaches original face170920/carrier4257, definition1791.
Original edge100412 is ellipse1147 on a nonperiodic clamped cubic tensor carrier.
Its unconstrained reconstructed guide has interior U about1.000002 outside the
original U[0,1] domain. Both endpoints have U1. The other three original circle/
ellipse edges likewise project near the other natural sides. This exceeds the
numerical128ULP grade and is a distinct source-conversion capability, not a
reason to widen that grade. The original source has no pcurves.

The selected next candidate is explicitly a bounded SOURCE CHART APPROXIMATION,
serving both owned raw-source UV and the subsequent query copy. It does not
change any original3D curve, carrier, vertex, interval, physical edge or use.
Matching endpoints alone does not prove that a curved original trim is a natural
surface boundary: a conic can bulge away between its endpoints. The complete
positive-basis proximity/admission below bounds the selected chart change;
finite original C/S checks establish their stated finite scope, not a universal
curve-on-surface or isotopy theorem. Original owned meshing still has to pass its
independent trim, actual cell and original spatial curve error obligations.

Admission is narrow and deterministic:

* Original supporting carrier is a nonperiodic clamped B-spline in both axes;
  original spatial curve adaptor identifies a Circle or Ellipse. No original
  LINE, generic spline, periodic spline or arbitrary trim projection is promoted.
* Reconstructed guide is an editable nonrational, nonperiodic B-spline over the
  unchanged original active t interval. Its two active endpoint UVs identify
  exactly one original global U/V side within the unchanged128ULP numerical
  endpoint grade. Both original C endpoints and declared geometric endpoint
  vertices agree with their carrier endpoint positions within the selected
  source residual. Original directed use/branch identity remains unchanged.
* At least one guide coefficient escapes that natural domain beyond the
  numerical grade; this strategy is named separately from numerical recovery.
  Every guide coefficient is considered. The candidate sets the identified
  coordinate of every coefficient to that exact ORIGINAL natural side, leaving
  the other coordinate, degree, knots, multiplicities, weights and original t
  unchanged. Its other-coordinate coefficient hull must already lie in the
  original global interval exactly; otherwise this source candidate rejects.
  Numerical correction in the later query copy cannot admit an invalid raw
  source candidate.
  No interior source knot is clamped and there is no pointwise branch switch.
* The maximum coefficient-coordinate change, multiplied by the retained
  original natural-domain derivative metric, must be finite and fit tolerance/8.
  The positive partition of unity bounds the complete fixed-basis UV change.
  This is a metric-weighted chart admission, not a derivative theorem about an
  extrapolated original surface outside its natural domain. The whole guide
  hull must be near the same unique side; endpoint matching does not suffice.
* The candidate must pass the existing original-t C/S probes at exact saved
  endpoints and17 points per original spatial knot span, with final UV inside
  the original domain and residual at most the unchanged source_residual_mm.
  Failed/ambiguous candidates reject with original face/coedge IDs. No selected
  tolerance, original geometry or source interval is widened.

The source operation applies only to a newly reconstructed missing pcurve; it
never rewrites a preexisting input pcurve. The editable guide is copied and
segmented at the exact original active interval first, preserving original-t
knots/basis on that interval. The source operation occurs once, before
attachment and the existing narrow
seam/period normalization. Those methods and final per-use readback retain the
selected candidate rather than a stale unconstrained curve. The unconstrained
construction is retained explicitly as diagnostic provenance: original face/
coedge, chosen axis/side, original guide coefficient excursion and metric charge,
and finite original-C/guide-carrier versus original-C/selected-carrier residuals.
The chosen source pcurve is marked as constrained reconstruction in deterministic
BGSRC diagnostics; timings remain stdout-only. Subsequent query-copy edits start
with this per-use metric charge, so cumulative source-chart-plus-query admission
still fits tolerance/8. The original reference export for NVIDIA remains raw
NONE-policy geometry; no owned projection cost is imposed on that baseline.

The mature alternatives were studied rather than assumed to constrain output.
Changing the projector precision to the original retained edge tolerance leaves
the same outside guide: isAnIsoparametric internally uses Precision::Confusion.
The explicit-domain GeomProjLib::Curve2d probe returns null for these four edges;
its implementation also permits initial-UV fallback and unconstrained fitting,
so bounds are not a final output contract. Neither alternative is adopted.

This conic strategy admits only a single original use of the edge on that
supporting face. Repeated/seam use is explicitly unsupported for this family;
its charge and guide provenance therefore belong to exactly that original
coedge. Existing analytic seam conversion is unchanged. Distinct incident faces
retain their independently bound source use/PC and are not conflated by physical
edge identity. This avoids charging only the first of multiple inherited seam
branches; no multi-use source-chart approximation is claimed.

Read-only original170920 diagnostics evaluate257 original-t samples on each of
four original uses. Selected constant-side candidates have maximum observed
C/S residuals8.7536478e-5,3.4654782e-5,4.4364051e-5,2.1239102e-5mm for
143084–143087 respectively. Unconstrained143084 actually reaches U outside by
2.1374286e-6, so this is not only a sufficient-control-hull false alarm. Maximum
whole-coefficient side changes are2.357606e-6,8.074709e-7,3.148823e-4,
2.040235e-7; original nonrational derivative hulls Lu=.03456325923,
Lv=.04533651339 give metric charges8.148655e-8,3.660791e-8,1.088336e-5,
9.249712e-9mm, each below unchanged.000625mm. Each other-coordinate coefficient
hull is wholly inside its original interval. These are finite diagnostic
observations and conservative coefficient-metric calculations, not proof of
exact original natural-boundary identity.

### Follow-up: native evaluation rounding at a proven in-domain coefficient hull

Fresh actual source/query attemptv4 passes170920, then fails at170932/4259/use143134.
A read-only original-source probe reproduces the selected nonrational B-spline:
all25 V coefficients equal original Vmin=-1.7889624832941899, the other U
coefficient hull is[0,1], and the original active interval remains
[0,1.3925571192667849]. At t=0.08703481995417406 the native evaluator returns
(.062521822147424355,-1.7889624832941902), one ULP below Vmin. Samples5,6,10,15
show the same one-ULP constant-coordinate escape. This is finite evaluation
rounding of a mathematically in-domain positive basis; it does not justify
admitting an outside coefficient hull or changing source coefficients.
Pinned OCCT8.0.1 commit b8f597c677811d1f9f4d8a97f5ae2825c0353a42 explains
this observation: `src/FoundationClasses/TKMath/BSplCLib/BSplCLib.cxx`, `Eval` for
dimension2 forms a knot-ratio weight X, computes Y=1-X and sums separately rounded X*coordinate
and Y*next_coordinate; it does not preserve a nonbinary constant through a
subtraction-based interpolation. `BSplCLib_CurveComputation.pxx` routes
nonrational D0 through Eval and CoordsToPointDirect. The128ULP rule below is a
fixed recovery admission, not a proved global bound on this foreign evaluator.

A private read-only per-face run of the current helper covers all635 original
spline faces:630 pass and5 fail.170932/143134 and170934/143140 fail the selected
source-chart finite domain probe;171042/143630,171044/143642 and172417/151423
fail the final query finite domain probe. The diagnostic publishes no source
or query bytes and is not whole-solid qualification: it does not exercise
cross-face support aliases or frozen parent ownership.

Selected correction, subject to review: use one bounded finite-probe recovery
for spline UV values only after proving the active nonrational, nonperiodic
B-spline coefficient hull lies inside the original global carrier rectangle.
The conic selected candidate already meets this condition. The final query
domain operation has inspected/corrected every active coefficient before final
correspondence, but the finite checker should verify the hull precondition
directly rather than depend on call order. Copy/segment the pcurve to the saved
original active range for this check; do not attach or alter that copy. Leave
unrelated analytic carriers and all source geometry unchanged.

For a finite native Value(t) outside a global side, accept recovery to that
original side only within unchanged128*ulp(max(1,abs(lo),abs(hi))) on the axis;
larger escapes reject. Evaluate the finite selected C/S residual at the
recovered in-domain UV, retaining the unchanged .001mm source grade. This is a
bounded numerical probe rule with a positive-basis domain precondition, not a
new chart approximation, projection, coefficient edit, or extrapolation bound.
The raw native pcurve, knots, weights, original t and IDs stay unchanged.

This follows the existing owned mesher `_recover_global_uv`, which already
recovers native UV at original global sides with that grade and records the
actual metric-weighted recovery shift. Its later `_uv_rounding_bound` separately
charges actual exact-to-binary64 corner conversion in the final mesh error sum.
Neither mechanism is replaced or credited to this source finite-probe rule.
Source-chart and query coefficient perturbations retain their existing
cumulative tol/8 admission; this unit does not count finite evaluator rounding
as another coefficient deformation or claim a whole-curve world-error bound.
Actual owned mesh residual/rounding checks remain required at whole mesh time.

Add a literal nonbinary constant-coordinate tensor-plane/native B-spline
fixture that first reproduces the strict finite-check failure, while retaining
the existing outside-hull beyond-grade rejection. Test exact preserved raw/query
coefficients, original t and literal spatial C(t), and recovered finite residual.
Then apply the one shared finite-probe recovery in the conic candidate check and
final spline correspondence, run focused checks and peer review, and perform
one fresh actual complete source/query attempt before any full mesh go.
