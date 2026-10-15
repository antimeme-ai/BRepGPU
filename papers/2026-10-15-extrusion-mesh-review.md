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

## Implementation plan review — accepted

Read `docs/EXTRUSION_MESH_PLAN.md`. Dependency order is appropriate: direct
independent model and genuine API-missing red, product implementation/green,
actual code review, one affected compatibility pass and actual indexed/history
callers. The explicit global rounded support checks and complete source/UV/domain
obligations address distinct geometry faults. Three fresh repeats per small
caller establish scoped observed costs without a performance campaign. Existing
export readers are reused without broad API widening; no curved/tessellation
quality claim is attached to flat faces. No open plan finding. Proceed with the
bounded implementation and directly relevant tests, then stop as specified.

## Actual code, oracle, tests and caller review — accepted

Read all of `python/brepgpu/extrusion_meshing.py`, the lazy result adapter,
`extrusion_meshing_oracles.py`, `tests/test_extrusion_meshing.py`,
`examples/mesh_extrusion.py`, and the small optional-footer change to the existing
mesh export/view helper. No product execution by this reviewer.

The product allocates actual source vertices once, preserves all original edge
and directed use records, forms actual face-loop fans without applying lower
sense twice, solves original affine parameters exactly and checks reconstruction.
Its rounded guard establishes a whole globally convex product shell and checks
actual facet sense. Actual max L1 displacement, exact Fraction tolerance and
atomic private publication match the design. No native/context/GPU scope grows.

The independent oracle reconstructs expected world positions from original
profile/height, evaluates actual chart corners with the existing independent
Bernstein expression, verifies complete directed source maps and per-face chain
equality on exact and saved-float geometry, and checks opposite triangle-edge
uses plus one cyclic link at each vertex and a connected genus-zero shell.
Positive triangles lie inside the convex domains, so boundary-chain equality
checks complete multiplicity-one coverage rather than relying on counts or
area. It does not import product mesher helpers for expected answers.

The ten direct methods cover maximum admitted 32-vertex and thin minimum-support
polygons, nonaxis sides, five scales/height endpoints, frame limits/subnormal and
ordinary nonexact world sums, exact tolerance equality/failure/retry, original
history/typed source ownership and immutability. Selected corruptions directly
exercise geometry/coverage/maps/identities/rounding claims rather than a mutation
score. An optional zero-roundoff extremely small Fraction tolerance was suggested
as one extra assertion in an existing method, not a further testing layer.

The caller retains one feature per case, times new meshing plus actual indexed
and original source-history reads, independently checks first returned output,
then compares full outputs/buffers outside measured work. Original source/result
records, controls, typed history and actual PLY/SVG are exported. The optional
footer correctly removes the periodic-seam description for this family and
preserves defaults for existing callers. Three fresh repeats are appropriately
limited observations with source construction/checking/export separate. No open
correctness finding. Proceed with the already planned affected compatibility
and saved actual caller review, then stop.
