# Revised adaptive design/plan: geometry review

Reviewed current `docs/ADAPTIVE_MESHER_DESIGN.md` and
`docs/ADAPTIVE_MESHER_PLAN.md`, the selected runtime finite numerical profile,
and algorithm packet/boundary-region procedure. Scoped geometry/source/error
ACCEPT, with the following wording finding sent to root for correction before
implementation:

An inactive refinement axis must mean proved AFFINE-SEPARABLE approximation
independence (`S_vv=0` AND `S_uv=0`, or the U counterpart), not literal independence
of S from that variable. A cylinder has nonzero S_v=Z and must retain its exact
one-direction path. Linear translation is allowed; point-rounding floors use
numerical promotion. Pure bilinear mixed dependence leaves both axes active.

The selected original S→actual bilinear B→SAME actual facet L inequality is
coherent, including rational N-WB and analytic directional interpolation bounds,
virtual leaf restrictions and one domain-displacement charge. Source-only delta
still compares original V/Ce to original S(chosenUV), never Q. Point departure is
included in actual chord/e_leaf and not added twice. The revised bounded shared
Q policy is explicit and preserves original vertices and edge ownership.

The finite RN-first profile has checked whole reduction/anchor domains and
retains coefficient, approximation and arithmetic uncertainty. Identical CPU/
P100 RN operations/padding plus authority-owned promotion close the previous
backend/visit-order policy gap. Exact work for axis optimization is correctly
removed. Lazy SourceStore→requested original root compilation and publication
adaptation avoid mandatory old full GeometryStore/trace preparation.

The implementation dependency plan now selects concrete source/cache/numeric
procedures and requires the first operation to reach actual conforming patch
triangles on CPU and P100. It explicitly distinguishes that checkpoint from
complete trims, components and the whole assembly. Typed per-unit API plans and
direct RED remain the next step; this review does not approve nonexistent code,
complete meshes or measured speedups.
