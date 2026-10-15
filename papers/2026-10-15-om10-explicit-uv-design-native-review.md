# Explicit source UV design: native review

Read-only design/subplan review, 2026-10-15. Reviewed
[the proposed source preparation](2026-10-15-om10-explicit-uv-source-preparation.md)
and its journal subplan against the pinned OCCT8.0.1 source at
`b8f597c677811d1f9f4d8a97f5ae2825c0353a42` and the earlier
[selected-method audit](2026-10-15-om10-occt-2d-method-adversarial-review.md).
No CAD execution, test campaign or production edit accompanied this review.

The proposed whole-definition deep copy, original-to-copy bindings, immediate
3D/range/vertex/tolerance checks, final occurrence readback and existing raw
coefficient qualifier address distinct source-preservation failures. Keeping the
original raw neutral input on the NVIDIA path also assigns conversion cost to
the consumer that needs it. No additional preservation framework is needed.

The original-t line/B-spline pole correction and chart-only connectors are
compatible with the native limiting-normal contract: cone V is slant length and
its canonical apex is host `-R/sin(angle)`; sphere V is bounded rather than
periodic. Copy/segment must retain the original parameter interval, and both
seam slots must survive final readback after a corrected branch is attached.
Positive B-spline basis weights bound the stated UV control perturbation; they
do not by themselves bound a composed surface's world displacement. The design
correctly retains measured C(t)/S(UV(t)) residuals as finite checks rather than
claiming a continuous world-error theorem from those weights.

## Required failure-path adjustment

The selected `ShapeFix_Edge::FixAddPCurve` wrapper has a failure path that the
proposed post-call checks cannot safely inspect:

1. At ShapeFix_Edge.cxx537 it ignores the boolean returned by
   `myProjector->Perform(...)`.
2. For a seam, it subsequently calls `c2d->Copy()` without a null check
   (cxx555). `Standard_Handle.hxx125` returns the raw pointer from operator->.
3. The reviewed projector can finish with null output and FAIL1 after failed
   interpolation (ShapeConstruct_ProjectCurveOnSurface.cxx771–774). Its
   contract therefore does not establish a non-null curve for that call site.

A post-call FAIL check or 3D guard runs too late to prevent the null seam
dereference. `catch(Standard_Failure)` and optional signal machinery are not a
checked non-null output contract. Specify a fail-closed boundary before coding.
Calling the already reviewed projector directly, checking its actual boolean,
FAIL flags, non-null output and original interval, then attaching only checked
2D branches through the reviewed storage operations would resolve this without
a second projection pass. An alternative needs to address this exact path.
Preserve the existing true-period/branch checks on any provisional seam pair.

The source owner proposed and this reviewer accepted the following bounded
resolution: run the per-face projector directly and reject false/FAIL/null or
invalid original-t output; attach the checked single PCurve first; for a true
seam only, invoke a **fresh** FixAddPCurve object after checking HasPCurve for
the identical edge orientation, support and location. HasPCurve and PCurve both
use the same CurveOnSurface lookup (ShapeAnalysis_Edge.cxx155–208), so the
wrapper takes its existing-PC branch rather than the unchecked projection path.
The fresh object's unused projector starts with OK status, avoiding stale DONE3.
This retains its mature provisional seam translation, including torus handling,
without projecting twice. Final flags, pair readback and source guards remain
required. This resolves the reported null path when incorporated into the plan.

## Remaining scope

Root separately identified the proposed strict natural-domain grade's
interaction with the existing source-grounded original-column recoveries. This
review does not settle or duplicate that recovery amendment. With that decision
and the projection failure boundary resolved, the bounded sequence is suitable:
authored missing-PC/pole/seam red, shared preparation, focused formula/failure
greens and code review, then one fresh source/query export with the existing
independent source/placement checks. No whole meshing, reference query pilot or
watch raster follows from this design review alone.

## Final revised review

Reread the revised study and subplan after root/source/oracle review: ACCEPT.
The checked direct projection and fresh existing-PC-only seam path are now
written explicitly. Complete ordered source-use ownership and whole-basis
constant true-period comparison remove ambiguity from endpoint-only checks.
Raw projected PCurves remain the owned snapshot input, preserving its existing
source-grounded recoveries. The optional deep query copy applies only the
specified column, retained-two-candidate joint and proven pole corrections;
original-t intervals, both seam slots, unchanged3D geometry and retained-carrier
derivative perturbation bounds remain required. No larger generic grade is
introduced. Preparation wall times stay outside deterministic source bytes.
One qualification export and existing source/report hashes bind the prepared
query files to the actual source; the NVIDIA import still consumes raw neutral.
No remaining native design/plan blocker. Implementation and its independent code
review remain the source owner's next bounded unit, not watch render approval.
