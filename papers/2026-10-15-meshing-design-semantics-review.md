# Canonical meshing design: geometry semantics review

2026-10-15. Reviewed the complete proposed `docs/MESHING_DESIGN.md` before the
implementation plan. Exact construction, shared-source identity and analytic
approximation contracts are sound. Two bounded clarifications were sent to
root and are now integrated. **Final disposition: accepted, no remaining
blocking finding.** No code or execution.

## M1 — prove actual rounded embedding using rounded domains

The exact-plus-rounded separation and error sum are correct. Each rounded
triangle is the affine image of its exact triangle using the same barycentric
coordinates. Maximum exact per-vertex L1 displacement bounds that movement in
Euclidean norm, hence the whole rounded shell's Hausdorff error is bounded by
the exact curve bound plus rounding bound. Reserving half the tolerance for
curves is conservative and transparent; budget failure publishes no output.

The promised independently qualified rounded embedding needs more than triangle
signs, shared indices and nondegeneracy by themselves. Require the actual rounded
rims to form simple strictly convex loops. For plate caps, require those loops
strictly inside the corresponding rounded outer square. Check rounded cap/side
planarity, equal top/bottom XY at corresponding rim vertices and strictly
ordered heights. Apply the same positive-chain coverage argument to actual
rounded cap domains and rounded wall chord strips, with only expected
inter-face intersections. These canonical-domain checks avoid a new general
triangle-intersection framework. Root was asked to make this direct rounded
proof explicit rather than infer embedding from displacement alone. The revised
design now explicitly requires these rounded-ring, plane, height and positive
boundary-chain checks, then derives expected intersections of the canonical
extruded domains. M1 resolved.

## M2 — distinguish raw chart winding from oriented projection

The qualifier correctly requires triangle signs to follow source face
orientation, but subsequently calls the source boundary outer CCW/hole CW.
Original bottom/negative-orientation boundaries are CW in stored raw UV. Specify
an orientation-normalized projection, such as reflecting V for a negative
face, before demanding CCW outer/CW hole words; or compare signed words in raw
UV with the face orientation factor throughout. Both triangles and independent
source words must use the same convention. This is a wording ambiguity, not a
fault in the declared outward construction templates. The revised design
explicitly requires triangle sign=Face.orientation and source boundary words
in the SAME raw UV convention, or reflection of V for negative faces throughout.
Wall coordinates likewise use source-oriented (lambda,z). M2 resolved.

## Accepted parts

The rational quarter formula and angle-speed bound imply sagitta at most
R/(2n²) for dyadic local subdivision n. This controls the complete extruded wall
and planar disk/hole approximation in both Hausdorff directions, including
honest hole-strip slivers. No finite midpoint sample or rounded trim predicate
substitutes for that bound. Exact rational construction, actual float rounding
and complete rounded consumption are distinct named scopes.

The plate's four corner fans plus four side triangles tile its complete chordal
cap domain without irrational 45-degree circle samples, discarded hole triangles
or extra outer-edge knots. Cylinder cap fans and ruled wall strips are complete.
Counts and Euler characteristics agree with the proposed closed sphere/genus-one
shells, while full boundary words, positive coverage, opposite uses and cyclic
vertex links provide stronger direct truth.

Original physical vertices/edge parameters establish global identity; original
coedges preserve distinct seam occurrences, raw lifted wall UV and chart-local
UV. t=0/1 aliases one anchor while retaining distinct parameter occurrences.
Reading original records avoids existing float evaluator coercion. Outward
bottom and hole-wall orientation, exact source/frame correspondence, bounded
atomic admission, immutable publication and explicit deferred general mesh/
GPU/external alpha scope are all correctly retained.
