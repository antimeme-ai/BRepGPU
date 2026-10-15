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
endpoints. Require finite results and measured residual≤selected source limit
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
