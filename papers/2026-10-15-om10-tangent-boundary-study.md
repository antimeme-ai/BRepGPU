# OM10 near-tangent original circular boundaries

Independent source study, 2026-10-15. Read-only diagnostics ran on the fleet.
Original input and the reviewed source snapshot remain private. This study
addresses the current failures on plane faces167654/167656; it does not approve
a product change or establish exact isotopy of the encoded source curves.

## Original geometry and the failing approximation

Face167654 has five original circular coedges. The crossing sampled segments
belong to original edges91130 and91105, incident at original vertex71450.
Their original spatial carriers are circle8034, radius1.2mm, and circle8019,
radius1.275mm. In physical XZ the centers are approximately(0,0) and
(-0.0430182327263314,-0.0614364033216744). The independently evaluated original
curves reproduce the saved spatial samples; the plane uses its owned exact
affine inverse, so foreign pcurve perturbation does not explain this failure.

The center distance is0.07500000000000011mm, compared with the encoded radius
difference0.07499999999999996mm. Evaluating the saved binary64 coefficients as
exact reals gives a difference1.58022e-16mm. The original shared vertex has
radial residuals2.22e-16/4.44e-16mm. These are nominally internally tangent
circles. They are **not exactly tangent as encoded**: the corresponding ideal
radius/center circle equations have two intersections about8.03e-8mm from the
nominal tangent point. The saved rotated XZ frame also has an exact squared-norm
residual1.417e-16, so that number does not solve intersections of the complete
saved-frame curve equations. Source incidence and finite source precision must
remain explicit.

The actual polygon crossing is substantially farther from that joint. Exact
rational intersection of the saved XZ chord endpoints gives segment fractions
0.927128577934/0.318306094305 and crossing point
(0.72164619497345,0.95871105445713)mm. Its radial deficits from the two original
circles are4.16e-5/8.33e-5mm. Original sample parameters near the joint are:

| Original circle | Parameter samples in source order |
| --- | --- |
|8034, R1.2|7.179030087304801,7.211073241540634,7.243116395776467|
|8019, R1.275|0,0.02454369260617026,0.04908738521234052|

Projecting these points onto the common nominal tangent gives distances from
the joint of0.038445/0.076851mm on one arc and0.031290/0.062561mm on the other.
The independently selected chord grids interleave across the narrow cusp.
Face167656 has the mirrored original pair91103/91134, with original shared
vertex71448 and the same sampling mechanism.

## Constraint on a correction

Independent chordal deflection limits do not ensure a simple boundary at a
tangent cusp. Both the separation of the source arcs and the chord departure
are quadratic in distance from the joint. Blindly halving both independent
parameter grids can preserve the relative crossing pattern.

A coordinated collar may use a shared local distance parameter, mapped back
to each original circle parameter, while retaining all original edges,
parameters, vertex identities and interval endpoints. Common tangent-coordinate
cuts are a useful geometric model: on a neighborhood where the original arcs
are ordered graphs, aligned breakpoints preserve the graph order under linear
interpolation. Radius-scaled arc-length cuts are another candidate, whose
actual finite output must pass the existing exact boundary-simplicity check.
Neither argument silently removes the tiny overlap in the saved real-valued
circle equations.

Coordination must include every incident face's unchanged error allocation.
Extra independent cuts on the inner arc can invalidate an otherwise simple
coarse pairing. Source edge samples must be constructed globally, so neighboring
faces consume the same physical keys and original parameters. Adding face-only
cuts would introduce a different topology fault. Unsupported geometry,
ambiguous source incidence, exhausted work or unmet error bounds must reject;
no source edge or UV cell may be dropped to obtain a picture.

The bounded direct tests should target the observed crossing and an authored
unequal-radius cusp with mismatched independent grids. Existing original-curve
residual, complete UV-chain, physical incidence and native source provenance
checks remain the direct checks for those independent fault classes. A second
permanent qualification mechanism is unnecessary.

Reviewed owner design: collect initial global edge schedules, replace only
optional cuts in disjoint protected endpoint collars with common nominal
R*delta_t steps, retain mandatory cuts and reject any candidate that violates
an incident bound or exact final planar-loop simplicity. Final physical keys
are created only after the shared schedules stabilize. Candidate count and
sample cardinality retain existing limits. Admission additionally checks
positive unequal radii, finite nonzero center separation, regular parallel
source circle frames, original endpoint correspondence to the declared shared
vertex, and nominal internal tangency within
J=min(selected source residual,max(retained translated edge/vertex tolerances)).
This is a cheap source-approximation admission diagnostic; its use does not
establish exact source validity. A substantial crossing pair outside J must
reject without widening J or replacing an unsupported edge. Accepted this
design and bounded subplan; production and actual whole-solid checks remain
the owner's next steps.

Subsequent narrow implementation review accepted the global schedule after
correcting reversed-loop segment attribution and requiring both stored circle
radii to agree. Owner32 direct methods and the existing five independent
geometry/native integration methods pass. Those results concern the mechanism
and affected fixtures; the complete actual assembly remains pending its
retained source/closed-solid/native qualification run.
