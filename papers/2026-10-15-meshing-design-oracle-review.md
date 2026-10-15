# Canonical meshing design: independent oracle review

Accepted `docs/MESHING_DESIGN.md`; no blocking finding. Reviewed the actual
complete design against original body/trim/source contracts, the independent
meshing study and external alpha policy. No code or product execution.

The contract correctly separates exact source-derived chordal-domain coverage
from true circular material: an inscribed disk loses a thin rim and an inscribed
hole adds a bounded strip. It preserves hole topology and original parameter
correspondence while stating a world Hausdorff bound. Exact CPU construction,
reported approximation/actual rounding displacement, and the rounded exported
shell each have distinct claims. No GPU consumer is required for this breadth unit.

Strictly positive projected triangles plus full integer directed-edge cancellation
to the independently reconstructed simple boundary establish complete coverage
through winding additivity. This directly excludes gaps, overlaps and extra
material off edges and extends to closed regular domains; it is not area/Euler/
centroid sampling. Each wall strip uses actual chord fraction/lifted height,
so nonlinear source rational s is not mistaken for an affine spatial coordinate.
The independent exact endpoint sagitta test does not repeat the product's
conservative angle-speed refinement selection.

Typed shared physical IDs, per-corner chart-local UV, separately lifted original
coedge UV and canonical source parameters retain both seam occurrences and cap/
wall rings. Independent edge incidence, connected shell and single-cycle vertex
links exclude unglued seams and pinched vertices. Material orientation explicitly
includes the plate's inward hole wall. Counts are narrow template facts rather
than the geometric completeness mechanism.

Actual rounded embedding now has a sufficient bounded argument: simple strictly
convex ordered circle polygons; matching top/bottom XY; strictly ordered cap
planes; hole containment in the outer square; complete positive-chain coverage
on actual rounded cap/side/chord domains; and no degeneracy/reversed triangles.
These extruded canonical domains meet only at declared common boundaries. The
actual vertex L1 displacement plus exact chord error bounds the entire rounded
triangle soup's displacement, without assuming identity indices prove embedding.

Implementation/plan cautions, not blockers:

- Boundary cancellation must preserve all integer multiplicities. Wall seam UV
  lifts/per-strip projected occurrences cannot be merged solely by physical IDs
  before the local domain check; global physical incidence is a separate check.
- 'Strictly convex simple CCW' requires complete ordered polygon validity, not
  just a local-turn test that can miss a star polygon's multiple winding.
- Use the same raw plane UV orientation convention for every triangle and its
  boundary, or consistently reflect it for negative faces. Never mix conventions.
- Qualify actual returned rounded coordinates, not a fresh private re-rounding
  used as a substitute. Verify reported budgets against original exact values.

The source/control oracle is appropriately reused once rather than counted as
another independent vote. Fine/coarse/threshold/budget/Frame cases and actual
production faults remain necessary after plan/red. Required external meshing
adapters/data and unsupported promised alpha families remain non-green;
canonical tests and the selected mutant campaign do not satisfy that release gate.
