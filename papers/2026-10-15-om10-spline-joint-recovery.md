# Original spline joint endpoint candidates

This unit continues the source-import/owned-mesh invariant in
[the main study](2026-10-15-om10-import-mesh-study.md). It does not replace original
source topology, select a dependency, or introduce a general surface projector.

## Actual source diagnosis

The old all-face diagnostic scan found a nonclosing joint on face169982. An
unchanged-current read-only run reproduces it. Original carrier4256 is a clamped
degree(3,3) spline with7x4 unit-weight poles, Uknots0/.285714/.571429/.857143/1
and Vknots.00999999999999996/.035, endpoint multiplicities4. Its original loop
contains coedges138090..138094. Incoming coedge138092 uses original edge98001,
curve2575, at active end t=.025588974014147316. Outgoing coedge138093 uses
original edge98002, curve2576, at t=.0255992662053717; its declared edge range
is descending and coedge sense is negative. Both declare original vertex76059,
P=(.450022980198881,-1.6,4.88413304101425).

The two supplied foreign UV endpoints are respectively
(.43225527967847305,.010000000000000035) and
(.4324291379188916,.009999999999999969). Their original S(UV) evaluations agree
with the corresponding original C(t) endpoints to roughly2e-15mm. This is not
merely an inaccurate foreign inverse: the original active parameter intervals
leave a real source-profile gap at their declared shared vertex. C2575 has an
active end before its natural span end; C2576 starts at that natural endpoint.

The original vertex is off both original spatial curves. Independently measured
endpoint residuals are.000185231372716mm and.000211863192357mm. Original source
carrier derivatives near the joint are regular: norm(Su)~.5912,norm(Sv)~10.
Independent one-dimensional source-boundary projection minimizes near the first
provided U, with residual.000185231mm. This observation motivates a finite
candidate choice; the implementation will not claim to compute that optimum.

Independent original-basis coefficient analysis also splits both active curves
on their original knots and mapped carrier Uknots. Their natural-Vmin carrier
profile compositions differ by at most4.24e-15/6.98e-15mm in cubic Bernstein
coefficient norm. These source facts support this actual family; no exact
polynomial identity or generic inverse is inferred from floating coefficients.
The retained translated vertex tolerance is about.0002118634mm; it is not an
explicit original STEP per-vertex tolerance or an additional quality allowance.

## Selected finite candidate capability

Only an original directed loop joint whose incident coedges declare the SAME
original vertex can qualify. Resolve both source endpoint parameters from edge
direction and coedge sense, including descending edge ranges. Never merge
different source vertices or use a face/physical-vertex dictionary that welds
chart occurrences. The carrier must be an original clamped spline, both coedge
chart shifts zero, and both parameter maps the supported SameParameter identity.
Both raw/recovered endpoint UVs must lie inside the original global knot domain,
inside one common ORIGINAL U-knot cell, and within the existing128ULP numerical
grade of the SAME natural V endpoint. Both U and V endpoints must be clamped.
This selects one existing chart and a natural-boundary joint; it does not add
periodic spline projection or arbitrary interior-trim recovery.

Trigger only a gap larger than the existing small-joint numerical grade. Existing
source-column recovery remains responsible for its separately proven family.
Keep the two existing admissible endpoint UVs as the entire candidate set. Do
not move either to a guessed natural parameter or solve a new nonlinear problem.
In particular, retain the chosen candidate's actual V; there is no interior V
snap. Choose deterministically by original S(UV)-to-P residual, incoming first
for an exact computed tie. This is a finite ORIGINAL-source-evaluator choice,
not an inverse, uniqueness, or nearest-surface claim.

Admission requires finite residuals and all unchanged selected source limits:
each original C(t) endpoint to P, each raw S(UV) to its original C(t), and the
chosen original S(UV) to P must satisfy source_residual_mm. Neither translated
source tolerance nor global STEP uncertainty enlarges that selected limit. Both
candidate UVs must satisfy the original domain/cell restrictions. The global
metric-weighted UV difference must fit the existing tolerance/8 trim allocation;
otherwise reject this fixed endpoint perturbation immediately with original
face/loop/coedge/vertex IDs.
Actual ORIGINAL global metric is(.6437623318668229,10.000000000000773); its L1
endpoint correction bound is.000111923386267mm, below.000625mm. No tighter local
derivative machinery or quality widening is necessary for this source case.

Compute joint choices before INITIAL SHARED edge sampling. The per-coedge source
endpoint override is used both by the interval bound and later face boundary
construction. Every incident coedge still participates in the one shared edge
schedule. For a raw recovered pcurve interval, its old UV chord bound gains the
maximum metric-weighted displacement of its two endpoint UVs. Thus a changed
endpoint cannot enter a product polygon without the corresponding full-interval
approximation bound. Original spatial curves, all full active intervals and
mandatory knots, raw pcurves and their original parameter maps remain intact.

`BoundaryOccurrence.recovery_kind` is `spline_joint_endpoint_candidate` ONLY at
the two original endpoint occurrences. All interior samples retain their prior
kind. Emit mandatory `JointRecovery.kind` with that same value, original directed
incoming/outgoing complete PRE-choice occurrences, original vertex identity and
the selected UV. The chosen incoming point can have zero UV change and still
needs explicit metadata. Raw pcurve UV and actual pcurve parameter remain in
every occurrence. No source binary/schema or native scene format change.

Report `spline_joint_candidate_count` and maximum metric endpoint UV displacement
as a bound in millimetres; keep existing source residual/displacement metrics.
Full exact positive UV-chain, canonical physical incidence/vertex links and
finite physical probes still apply. The current triangle acceptance already
includes source S(UV)-to-canonical-XYZ corner displacement; it remains unchanged.

## Complete the original spatial-curve chord term

This diagnosis exposed a separate omitted term in the same source-boundary
invariant. The old original spatial-curve chord bound compares C(t) with the
chord through C(a),C(b), while the actual shared polygon uses canonical original
vertices at source endpoints. For interpolation weight lambda, the chord change
is bounded by max(norm(canonical(a)-C(a)),norm(canonical(b)-C(b))). Therefore use

    Bworld = Bideal_original_curve_chord + max(actual_endpoint_departures)

as one participant in the existing all-incident maximum against tolerance/8.
Interior shared samples use the same C(t) evaluation, so their computational
departure is zero. Cache the two ORIGINAL endpoint departures per explicit
edge-bound context, validate finite values and the unchanged source limit, and
fail immediately if a fixed departure alone exceeds the chord allocation. Do
not add source_residual_mm as an allowance. Keep the actual displacement in the
existing endpoint residual metric; final max_edge_chord_bound_mm includes the
corrected sum. No duplicate all-source bound evaluation is needed.

The existing final2*edge+joint-cell+native-rounding expression can conservatively
contain this term once Bworld is corrected. The joint-cell S(UV) displacement
cannot substitute for original C departure: original source LINE z=epsilon,
source plane and canonical vertices z=0 gives ideal C chord=0, UV chord=0 and
cell displacement=0, yet original-boundary error epsilon. This is a direct
missing-term counterexample, independently confirmed by the oracle owner.

All51,742 actual source curve endpoints were inventoried read-only: maximum
.000211863192357mm at the diagnosed outgoing endpoint, next.000185231372715mm,
and none exceed the unchanged.000625mm chord allocation or.001mm source gate.
The evaluator-only pass took.468s excluding source load. Floating original
evaluation/analytic derivative bounds retain their existing stated numerical
limitations; no universal certificate layer is added.

## Independent checks and scope

The independent qualifier can recompute both original endpoint source evaluations
and original-knot S(UV), resolve the original directed joint, check the chosen
member/minimum with its existing numerical comparison grade, and verify both
original raw records/parameters and the endpoint-only typed override. Existing
domain/physical/native checks remain the output checks. No new oracle framework.

Direct fixtures should distinguish finite endpoint choice from a generic surface
inverse, source-vertex identity from proximity, and actual original C departure
from ideal-carrier error. A clamped planar spline rectangle with a split boundary
and declared off-curve shared vertex gives explicit expected candidate/XYZ and
trim perturbation. A displaced original spatial LINE over unchanged planar
vertices gives the world-chord red. Actual169982 must pass at unchanged defaults,
then full original source qualification remains required before any rendering.
