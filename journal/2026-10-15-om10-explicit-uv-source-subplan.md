# Explicit source UV preparation subplan

2026-10-15, reviewed before code. Whole model no-processing source
extraction fails at original coedge109547. The complete audit finds24,602
nonplanar missing-pcurve uses, all original SameParameter flags true and all
original edge bindings preserved. All375 multi-loop faces are planes; all635
spline surfaces are open/nonperiodic. Raw neutral input remains NVIDIA's
deliberately supported missing-UV format; this unit serves owned extraction and
OCCT query faces only.

Design: `papers/2026-10-15-om10-explicit-uv-source-preparation.md`.

1. Root/oracle/native adversarial design and bounded-plan review. Read exact
   selected methods/callees, not class names. No whole-shape healing.
2. Genuine fleet authored extraction red before implementation. Existing real
   whole-source missing109547 failure is retained. Extend the literal source
   fixture with a bounded pole/seam case only where needed, preserving existing
   box/cylinder fixture and its NVIDIA producer tests.
3. Add one shared `native/source_uv/source_uv.hpp/.cpp` capability: deep detached
   geometry/topology, original-use chain validation, missing-PC projection,
   direct checked projector then single-PC attachment; fresh existing-PC-only
   FixAddPCurve for native dual seams; direct FixSeam/FixShifted; whole-basis
   true-period/readback/source-correspondence checks. Preserve raw foreign PCs
   for owned extraction. Optional detached query copy applies narrowly admitted
   existing source-column/finite-joint/pole2D corrections before natural-domain
   admission, with actual endpoint/control perturbation bounds unchanged.
   Guard3D changes directly; use existing original geometry qualifier once for
   actual emitted coefficients. No audit framework or generic repair engine.
4. Integrate prepared shape mappings into `native/step_source/main.cpp` and its
   CMake target. Keep raw graph matching separate. Add only optional query-BRep
   output from those same prepared solids; preserve BGSRC001 fields/IDs and
   default positional CLI. Raw NVIDIA exporter does not invoke preparation.
   Emit preparation wall times only to stdout/external timing rows; keep
   BGSRC001 bytes deterministic across fresh CPU/GPU benchmark callers. Parent
   integrates one query export in qualify_om10.py's query-source directory and
   binds it to source snapshot/original SHA with existing input/report hashes;
   the NVIDIA finite qualifier consumes that prepared query set, not raw neutral.
5. Focused source formula/parameter/seam/pole/unsupported failures green, one
   affected source/reference reader run as justified, independent code review.
6. Fresh complete owned source +107 query V4 files, existing independent raw
   geometry/placements/166 occurrence checks and old/new actual delta. No V4
   full meshing until parent reviews the actual source. Native query owner uses
   these explicitly prepared files and retains strict missing-pcurve preflight.

Initial direct projector research used copies on one representative of each14
actual nonplanar carrier/curve families. All returned2D output;17 probes per
representative have maximum8.75e-5mm residual. This does not qualify untested
edges or universal projection error. A probe initially reused CurveOnSurface's
null output interval and failed on zero-range TrimmedCurve; corrected the
diagnostic to retain original3D interval separately. No production source was
altered by that experiment. Mature projector cache/status/alias/sphericalV and
coincident-UV nudge hazards are recorded in the independent method review.

Root and independent oracle accepted the revised design/subplan before implementation. Native review accepted the checked-projector/existing-PC-only seam path. Genuine authored fleet red is the next step; no reconstruction implementation at acceptance.

Bounded new-projector known-input probe confirms three beyond-grade natural-U
excursions on172494/172983/173010 and the existing169982 endpoint gap;173009's
9.24e-14 excess is within128ulp(4). Root correctly identified that an adapter
hard-domain gate would block already reviewed source-based owned recoveries.
The revised design preserves raw projected PCs for that consumer and explicitly
prepares a query copy, without another STEP parse or projection. Native review
found unchecked null seam dereference in FixAddPCurve's missing-PC branch;
selected direct projector checks plus existing-PC-only seam attachment remove
that failure path. Oracle review requires whole-basis period translation and
complete ordered original-use ownership; both are now explicit. Await revised
root/peer acceptance before genuine red or production.

Root revised review accepts source bindings and raw/query split, with two
required integration details now written: no wall times in deterministic
BGSRC001 fields; one optional query export in the existing qualification caller
and existing source/report hashes, consumed by the reference finite qualifier.
No extra persistent protocol or receipt mechanism.

Final root/oracle design and bounded-plan ACCEPT recorded before code. The
focused source fixture scope is existing authored box/cylinder plus a literal
three-great-circle spherical octant through the north pole, with original use/
period failure cases. Source owner owns the shared module, adapter and existing
qualification caller; reference owner owns consuming prepared query files.
Include native/source_uv in existing source pins. Whole V4 remains gated on
focused green/code review and actual complete original source/query export.

## Authored red before production

The existing reference box/cylinder STEP includes explicit SURFACE_CURVE/PCURVE
representations, so it does not exercise this source gap. The first test attempt
correctly exposed that fixture mismatch: extraction succeeded, then the original
geometry oracle rejected unsupported SURFACE_CURVE wrapper ID139. Retained that
initial run; it is not evidence of a missing-PC failure.

Added separate authored `tests/fixtures/source-uv.step`: same box/cylinder and
source topology/product IDs, with all15 EDGE_CURVE references pointing directly
to the existing spatial basis curve of their SURFACE_CURVE/SEAM_CURVE wrapper.
The original reference fixture and its NVIDIA tests stay intact. Unreferenced
2D records are retained; no source geometry, units or topology is fabricated.

The corrected fleet command `PYTHONPATH=python python3 -m unittest discover
-s tests -p test_source_uv.py -v` is genuinely RED before any production code:
original cylinder coedge400 has missing recovered UV; the optional query CLI is
absent; the new literal octant native fixture entry is absent. Three methods,
0.115s, two failures/one missing-entry error. The ignored
`context/om10-source-uv-direct-spatial-red.txt` retains the exact output.
The tests check existing raw original coefficient/assembly oracles, independent
original C(t)/S(UV) evaluation, deterministic BGSRC bytes, V4 query presence and
literal spherical octant curves/analytic surface. Negative native fixture modes
must actually mutate native input and traverse production guards; mode-name
refusal in the fixture would not establish the failure claim.

The first whole raw-source diagnostic fails at original face164999/carrier14349,
four-use full-circle/seam cylinder. Retained endpoints show a2π seam-slot
mismatch and native WireExplorer2/4 uses although source word and3D C/S agree.
The authored reversed-cylinder wall with original circle range[π,3π] directly
reproduces2/4,0.014s, before the correction. Selected fix: normalize only the
mature fixer's temporary working wire to its local-FORWARD convention; preserve
original source word and original-face final readback. Native method research
shows SetFace(REVERSED) alone would not change ShapeAnalysis_Edge's support-only
PCurve lookup, so that proposed shortcut was rejected without implementation.
Root accepted the source-derived full-word temporary-view correction.

## Implementation review checkpoint

Shared source/query preparation, adapter and qualification integration are
implemented. Five direct source-UV methods and five affected source-reader
methods pass on the fleet (0.298s), including missing-PC extraction, unchanged
original geometry/placements, deterministic snapshot/query export and literal
forward/reversed pole and seam fixtures. Genuine reversed-cylinder and
reversed-pole reds preceded their fixes. Root, independent math and native code
reviews accept the addressed implementation. Native review caught the face
state initialization order, seam convention and parent/child orientation bugs;
final actual query use-cycle checks include declared connector placement.

This checkpoint establishes the bounded authored capability, not the watch.
One fresh actual complete source/query export and a bounded native sanitizer
run are starting. Complete original geometry/assembly and actual source delta
remain required before full V4 meshing or any target render.

Fresh complete source/query attempt reaches definitions1713–1716 then the first
pole-bearing1717 and fails without publishing a source snapshot. Same literal
F/R octants nested in a shell reproduce the actual cause before correction:
face1/carrier2 Add(query pole wire), parentFree0, TopoDS_FrozenShape,2failures
in0.022s. Root accepted a narrow RAII query-face mutability scope for wire
Remove/Add, restoring savedFree even on exception. This changes only detached
query construction permissions; original/raw objects and all3D/physical word
contracts remain unchanged. Add actual face Free-state assertions to these same
fixtures and final FaceState. Native exception type/site now remains in source
failure messages. Earlier ASan3root-face cases passed0.157s; the final bounded
sanitizer run must cover these stronger nested fixtures after correction.

The scoped query-face Free-state fix passes the same10direct/affected methods
in0.258s. The three stronger nested native methods also pass ASan/UBSan in0.169s
without suppressions. Root reviewed the saved-state scope and exception path.
The next actual source/query attempt passes the prior seam and frozen-pole
cases, exporting definitions1713–1767 before a distinct169982 query-domain
control escape; no complete source snapshot is published. That numerical
representation case remains open and is not a watch pass.

After frozen-child correction10focused/reader methods pass0.258s; stronger nested
ASan/UBSan3methods pass0.169s. Fresh actualv2 exports1713–1767 then rejects
169982's interior query control. Bounded actual probe finds use138090 control2
V=.035000000000000017/control9=.035000000000000052 versus originalglobalVmax
.035000000000000003, max4.86e-17 below128ulp(1). Root/oracle clarify that
existing owned 'global ends' means carrier domain boundaries at any coedge t,
not only pcurve endpoint controls. The earlier control-endpoint-only finding
was incorrect and its restriction is retired. Reviewed amendment: positive
nonperiodic fixed-basis query controls may move only to ORIGINALGLOBALdomain
bounds within unchanged128ULP; preserve rawPC/sourceknots/originalt and apply
cumulative naturalmetric/finalC–S. Add one literal8ULP middlecontrol/512ULP
negative tensor-plane fixture before production change; no genericprojection or
extrapolation theorem.

The authored middle-control case genuinely fails before restoration (0.015s,
`context/om10-source-uv-global-controls-red.txt`). The reviewed operation now
accepts the8ULP control while preserving the raw middle coefficient, degree,
poles, original interval and literal C/S agreement; the512ULP version rejects.
Periodic/rational edits explicitly reject before segmentation. Eleven focused
source-UV/source-reader methods pass0.274s. A fresh actual complete source/query
attemptv3 and four bounded native ASan/UBSan methods are running; no complete
watch qualification or render is implied.

The four bounded native methods, including the new middle-control case, pass
ASan/UBSan0.212s. Actual attemptv3 passes prior169982 and exports definitions
1713–1790, then fails closed at170920/carrier4257/coedge143084 on a larger
natural-domain escape; no complete snapshot is published. This is not handled
by widening the numerical128ULP grade. Direct original-curve/projector study
is in progress before any further production change.

The reviewed global-domain coefficient operation passes11focused/affected
methods in0.274s and four bounded native ASan/UBSan methods in0.212s. The actual
next source/query run passes169982 and exports definitions1713–1790, then stops
at170920/carrier4257/coedge143084 on a beyond-grade domain excursion. No source
snapshot is published. Original ellipse1147 versus cubic tensor boundary
approximation is the next studied source-conversion case; its new design has
not yet been selected or implemented. The numerical grade stays unchanged.

### Next bounded source-chart unit (plan, no production change yet)

Actual170920 is a conic/natural-boundary approximation family, not numerical
control noise. Preserve freshv3/guide probes and original source coefficients.
Implement only after root/peer review of the source-chart admission in the study.
One literal tensor-plane/conic fixture should first fail because its small
original circular bulge projects outside the tensor rectangle; the selected
natural-side representation must retain literal original C(t)/radius/range,
complete physical word and original source geometry. Check final selected
constant-side UV/C–S and diagnostic metric/provenance, and reject a larger bulge
that exceeds the unchanged allocation/source grade. Reuse the same whole
preparation and existing use/period/preservation guards; do not add a second
qualifier. Keep the512ULP LINE fixture failing, since it is not this conic family.

After genuine red, add one shared raw-source conic chart helper, deterministic
source diagnostics and source-to-query cumulative charge initialization. Run
existing focused source-UV/source-reader checks plus this direct method, peer
review, and one fresh actual complete source/query attempt. Full original
geometry/assembly checks and old/new delta remain required before V4; no mesh,
watch raster or broad corpus campaign is part of this source unit.

Root, independent math and native reviews ACCEPT the bounded source-chart plan.
The literal tensor-plane/circular-bulge test genuinely fails before helper code:
coedge10 unsupported query natural-domain escape,0.017s, retained in
`context/om10-source-chart-red.txt`. The test requires unchanged original circle
radius/t/literal C(t), raw and query selected side, full physical word and
rejection of a larger bulge beyond allocation. Production is unchanged at this
red checkpoint; implementation follows only this reviewed source approximation.

The reviewed shared helper passes twelve focused source-UV/source-reader methods
in0.368s. The new literal conic boundary method also passes ASan/UBSan in0.145s
using the existing sanitizer target and OCCT build, without suppressions. A
durable fresh actual source/query attemptv4 passes the previous170920 case but
stops at170932/carrier4259/coedge143134: selected source UV leaves the original
domain. No complete source snapshot is published, so complete original-source
and assembly oracles and old/new delta have not run. The failed conversion is
retained privately; exact selected coefficient/evaluation values are under
read-only investigation before selecting any further production change.

Read-only170932 values identify one-ULP native constant-coordinate evaluation
rounding with exact in-domain25-control hull and unchanged original active t.
A private635-spline-face diagnostic finds630 passes and5 finite-domain failures
(170932,170934 selected chart;171042,171044,172417 final query). It writes no
source or query files and does not replace whole-solid alias/ownership checks.
The study now proposes an unchanged128ULP finite-probe recovery only after an
explicit positive nonperiodic active B-spline in-domain hull check, without
changing stored curves or coefficient admission. An authored nonbinary constant
coordinate fixture must reproduce red before implementation. Root/peer design
review is pending; production remains unchanged. Owned mesher recovery metrics
and final actual UV rounding charges remain separate and required.

Root and independent review accept the finite-probe design/subplan. The first
single-span quadratic fixture did not expose native rounding (both dyadic and
nonbinary active intervals), so its sensitivity assertion failed; neither run
counts as production red. A four-control quadratic with one literal interior
knot .37 and Greville U coefficients gives literal original LINE C(t)=(t,Vmin,0)
on [0,1.3925571192667849], with all V coefficients exactly
-1.7889624832941899. It genuinely fails the existing final domain checker at
face1/carrier2/use10 in0.012s. The private red log is retained. Production is
unchanged at this checkpoint; the accepted shared finite-probe helper follows.

The shared finite-probe helper now passes thirteen focused source-UV/source-reader
methods in0.335s, including the literal four-control original-line case. Stored
raw/query degree, controls and active interval remain unchanged; the fixture
explicitly observes native Value(t) outside Vmin while literal C(t) and finite
C/S remain within their stated grade. Root/native and independent code reviews
accept the detached active hull and fixed recovery rule without findings. One
bounded ASan/UBSan run of that new method is pending before the fresh actualv5
source/query attempt. No whole-source or mesh pass is claimed.
