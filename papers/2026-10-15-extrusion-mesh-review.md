# Extrusion-result meshing: continuing independent review

2026-10-15. One review record covering the bounded semantics/design/plan and
subsequent actual code. User asks for direct rigorous testing and an explicit
stop before certification machinery. This is an engineering review, not release
qualification or a second evidence system.

## Initial semantics recommendations

Read current doctrine, actual source/result representation and independent
extrusion truth, canonical/cut mesh implementation and acquired edge/face
reference sources; recommendations are in
[the source-grounded study](2026-10-15-extrusion-mesh-study.md).

Require actual-result ownership, 2N shared source vertices, 3N original physical
edge chains and 6N distinct uses, 4N−4 triangles, complete actual convex cap
coverage, exact original UV and one application of occurrence/face orientation.
The critical rounded-embedding condition is a convex product shell in saved
float ratios, allowing bounded displacement from original oblique side planes.
No circle budget, general triangulator, canonical guard widening or native work
is needed. Exact tolerance equal to measured rho must be accepted; below rejected.

Design, plan and implementation observations will continue in this file.

## Design review — accepted

Read `docs/EXTRUSION_MESH_DESIGN.md`. The design resolves cap winding once,
keeps actual result/history ownership, uses exact affine inverse and shared
source boundaries, and explicitly permits bounded rounded oblique-plane motion
while checking the whole rounded product embedding. Whole-domain chain equality
and positive facets address gaps/overlap independently from counts or volume.
The exact equality/below-rho tests and a real 32-vertex direct case cover the
important admitted boundaries. The stop condition is concrete and appropriate.

Read-only `curve_bound=Fraction(0)` and `quarter_segments=0` are acceptable
compatibility descriptions for existing mesh readers; no subdivision argument
or implied curved approximation is introduced. No open design finding.
