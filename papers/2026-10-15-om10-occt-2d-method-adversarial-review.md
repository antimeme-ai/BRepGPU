# Selected OCCT2D methods: mutation and chart review

Read-only adversarial design input,2026-10-15. Reviewed the retained OCCT source
at commit b8f597c677811d1f9f4d8a97f5ae2825c0353a42, restored by the existing
quarantine manifest. No CAD execution, tests, source repair or product edit.

The selected direct calls can fit a narrowly declared2D representation stage on
detached topology. They do not establish original-parameter correspondence or
correct complete chart closure merely by returning success. Broad fixing entry
points have a substantially larger mutation surface and are outside that stage.

## FixSeam: swap, not construction

ShapeFix_Wire.cxx1615–1634 calls ShapeAnalysis_Wire::CheckSeam, then swaps the
two returned PCurves with UpdateEdge and sets their **face-specific** range.
CheckSeam at ShapeAnalysis_Wire.cxx839–884 requires an existing seam and two
non-null PCurves; it normalizes the face to FORWARD and reads FORWARD/REVERSED
edge branches. It swaps only if SelectForwardSeam returns2. That selector
(ShapeAnalysis_Curve.cxx852–966) uses line directions/locations or the endpoint
chord of bounded curves. It is not a missing-seam constructor or general
periodic-branch proof. False can mean absent/inapplicable/undecidable as well as
already correctly ordered.

The header's orientation warning matters: WireData represents a forward wire
with orientation carried by its edges. The wire must enter a FORWARD face or
be extracted without composing the face orientation. A direct array index is
not automatically the original ORIENTED_EDGE index after wire normalization.

## FixShifted: mutation scope and semantic limits

Read ShapeFix_Wire.cxx1661–2133 and direct ShapeBuild_Edge::ReplacePCurve
(474–504). These selected bodies replace **new transformed2D curves**, retain
the partner seam PCurve and reset face-PCurve ranges/UV endpoint caches. They
do not call a3D curve/surface setter, change original vertex coordinates, insert
physical edges, or invoke SameParameter. Passing0 to UpdateEdge does not lower
an existing nonnegative tolerance: BRep_TEdge::UpdateTolerance is max-update.

Important limits remain:

* The method assumes a sorted closed wire. It skips certain degenerated uses,
  analyzes near-singular vertices and has a special bi-meridian branch. It does
  not construct missing pole closures. Its implementation comment retains a
  sphere/two-meridian limitation; do not infer complete pole/seam support.
* Closure is broader than a true period. It explicitly treats a spherical
  surface as V-closed and uses its V-span,π, in shift logic. Yet the actual
  Geom_SphericalSurface source reports IsVClosed=false,IsVPeriodic=false and
  V bounds[-π/2,π/2] (116–165). Translating sphere V byπ changes the physical
  point, generally to its antipode. Similar caution applies to merely closed,
  nonperiodic spline directions. A2D-only edit is not automatically faithful.
* Connection tests use fractions of the closed range, including0.2-range
  thresholds; singularity handling uses source tolerances. A returned shift
  is a proposed chart correction, not a certified original-t inverse.
* FAIL1 can be accumulated while other edges continue to be modified. Special
  meridian replacements can precede a later false return, such as a void-box
  exit. Boolean success is neither complete coverage nor a no-edit guarantee
  on failure. Read status flags and actual resulting PCurves/uses.

The direct storage operations are narrower than BRep_Builder::Range(E,f,l),
whose default updates all representations, including3D. The face/surface+location
overload only changes that PCurve representation. Do not replace it with the
convenient global-range overload while integrating.

## Detachment and physical-use identities

BRep_Builder::UpdateCurves at104–313 matches supporting **surface handle plus
relative location**, rather than STEP face or ORIENTED_EDGE identity. It removes/
replaces that edge's corresponding2D representation and may inherit the existing
3D parameter range. It preserves the3D curve representation itself. Consequently
all uses sharing that edge TShape and support can observe the update.

A copied TopoDS wrapper/wire is not isolation. Detach topology nodes for the
whole definition while retaining its shared physical-edge graph; copying each
face independently can duplicate physical edges instead. Keep original↔copy
mapping and original edge-use multiplicities/orientations explicit. Check each
actual occurrence's selected seam branch after attachment, including aliases
across wires/faces with the same support.

ShapeExtend_WireData defaults chained=true and keeps TopoDS iterator order,
reversing its internal sequence for a reversed wire. That does not establish
connectivity. Its chained=false fallback uses WireExplorer; the header explicitly
warns some disconnected/seam uses can be omitted. A non-null context can apply
replacements during Load/UpdateWire. Use the reviewed source-use sequence and
complete counts, rather than silently accepting a convenient reduced traversal.

## ProjectCurveOnSurface: geometry inputs and state

Perform takes const curve handles and First/Last **by value**; inspected direct
branches do not replace/mutate the input3D curve or surface. Its temporary
Geom_TrimmedCurve wrapper copies the basis (Geom_TrimmedCurve.cxx54–73).
Plane projection, ProjLib fallback and sampled/isoparametric construction emit
new2D curves. Original-t sampling, endpoint correction, point insertion/removal
and interpolation operate on local arrays. The API documents same-parameter
output; keep the original3D interval and verify the actual emitted curve covers
that interval rather than retiming the original edge to fit a result.

The tool is stateful. SetSurface with the same analysis handle retains endpoint
cache; Perform updates it. Branch selection can therefore depend on call order.
AdjustOverDegenMode defaults1; -1 disables the special correction,0 chooses the
upper chart side and1 the lower when that branch activates. It can shift sampled
UV across singularities. Choose/cache/order that policy explicitly and preserve
it in diagnostics. Its23-or-more sample construction and finite residual tests
do not establish an exact inverse on an entire curve.

One pole/collapse branch deserves explicit admission: checkPoints2d
(2373–2452) can synthesize a minimal two-point output when coincident-point
removal leaves fewer than two points, nudging the last sampled UV in both
coordinates by its working precision. This leaves source3D objects unchanged
but must not silently become an original physical edge or an exact pole closure.

## Integration boundary to retain

Invoke only the individually reviewed operations, with fully detached target
topology and copied projection geometry. Original3D carriers,curves,range bits,
vertices,tolerances,orientations and physical edge uses remain the source values.
At original t, require reconstructed UV to evaluate on the original carrier and
match the original spatial curve under the declared source residual, with actual
finite/domain/period/seam checks. Singular UV closures must be explicitly chart
objects; do not pass through FixDegenerated to fabricate/remove physical edges
or null an original3D curve. False/partial statuses require explicit outcomes.

Do not call Perform/FixEdgeCurves to reach these methods indirectly. Their
broader paths include SameParameter,3D curve addition/removal,vertex tolerances,
degenerated-edge changes and topology repair; default SameParameter and3D-add
work is enabled, while some destructive removal branches require explicit flags.
FixSameParameter
explicitly changes flags,tolerances and may regenerate/reparameterize PCurves.
FixAddPCurve also contains a DONE3-conditioned3D UpdateEdge/Range branch, even
though the current projector does not set DONE3. These wrappers add avoidable
preservation obligations.

The NVIDIA bridge supports the raw neutral shape without these PCurves and its
analytic path reconstructs/discards them. Keep the reconstruction cost in the
owned extract/query pipeline where required; do not force it into NVIDIA's
measured import input or call a query-source preparation failure NVIDIA failure.
This review selects no implementation and authorizes no CAD rerun.
