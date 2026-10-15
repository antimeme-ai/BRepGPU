# Actual Boolean-result mesh: implementation and oracle review

2026-10-15. Reviewed cut_meshing.py, the CutResult lazy adapter, independent
cut_meshing_oracles.py and direct tests against accepted CUT_MESH_DESIGN/PLAN.
No product execution performed in this review; parent owns the active fleet
qualification run and its result is a separate completion requirement.

**Accepted, no blocking code finding.** Physical mesh generators read the actual
CutSolid vertices/curves in the result Frame. Complete source edge chains precede
all coedge/face construction; closed anchors and both seam occurrences preserve
physical identity. Annular cap rings come from actual hole-loop ownership; wall
rings come from actual original circle_wall coedges. Triangle corners invert
retained plane charts in their own patch Frame; wall corners retain cutter
V[1/4,3/4] and their original quarter. No canonical body reconstruction, shape
guard widening, source refit or carrier normalization occurs. CutMesh retains
actual CutResult/CutSolid and reads exact original history without a second map.

The bounded annulus fan/bridge and wall-strip construction preserves material
orientation and coherent shared topology. Actual rounded output checks strict
ring convexity/angular order, containment, cap correspondence, coplanarity and
triangle sense. The geometric bound is in world units; measured binary64 L1
perturbation is included before publication. Resource subdivision is bounded
before allocating the mesh, and an unmet budget publishes no partial result.

The oracle derives expected generators/maps/loops from original data rather than
calling tessellator helpers. Its strict triangle signs plus complete integer
boundary-chain equality prove whole exact/rounded polygonal-domain coverage;
its wall checker uses physical chord fraction rather than nonlinear source U.
Global edge uses, connectedness and cyclic vertex links independently check the
manifold, while exact arc and radial inequalities check actual approximation.
The original Boolean source/history contract is checked once by its existing
independent qualifier. Reused elementary predicates contain no product mesher.

Tests check actual passed-object retention, all admitted scales, world/error
thresholds, translated/subnormal Frames, complete original source/history,
explicit raw wall V/cutter Frame, invalid/budget failures, prior-output survival,
canonical rejection and immutable publication. Selected missing/duplicate/reverse/
filled-hole, UV/source/identity-alias and actual-rounded/budget alterations fail
specific direct checks rather than asserting no crash or status alone. These
cases are selected sensitivity, not a suite-wide mutation or alpha-readiness claim.

Trusted factory-only objects are the declared input boundary; forged untrusted
records are not silently added as a new validation requirement. No further test
layer, GPU/native rebuild or optimization is justified by this source review.
