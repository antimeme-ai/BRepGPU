# Exact material and finite-face classification: oracle study

Research for the next breadth-first theme, not an implementation or architecture
decision. The tentative scope is exact material membership on the three owned
primitive bodies and complete closed rectangle/rectangle contact on their axis-
aligned planar faces. Integer/Fraction/finite-binary64 points are mathematical
values; supporting original face IDs and both surface UV representations are
part of output truth. Placement remains undecided until design/plan review.
No segment-solid classifier, general curved arrangement or library is selected.

## Doctrine, owned geometry and primary reference lessons

Read current doctrine, roadmap, body record/factory/classification/oracle code,
intersection scope and exact construction/oracle study. Existing UV trim labels
classify a query on one carrier chart; they do not classify material in3D.
Existing intersection tangent flags describe the plane/cylinder carriers;
they do not supply every finite-domain or solid-contact semantic.

Read pinned OCCT8.0.1 source at
`b8f597c677811d1f9f4d8a97f5ae2825c0353a42`, already retained/restorable through
the quarantine manifest:
`TopAbs_State.hxx`, `BRepClass3d_SClassifier.cxx`'s point entry, State/Rejected
and face-state handling, plus `BRepClass3d_SolidClassifier_Test.cxx`'s analytic
box/sphere examples. Its tolerance-based tests are useful pattern references,
not exact-input oracles. Critically, State returns OUT for internal states it
does not otherwise recognize, including error; owned unsupported/unresolved
must never silently become exterior/disjoint.
[Pinned classifier source](https://github.com/Open-Cascade-SAS/OCCT/blob/b8f597c677811d1f9f4d8a97f5ae2825c0353a42/src/ModelingAlgorithms/TKTopAlgo/BRepClass3d/BRepClass3d_SClassifier.cxx).

[Shewchuk1997](https://www.cs.cmu.edu/~quake/robust.html) studies correct signs
of floating-input geometric predicates and adaptive exact arithmetic. Its
platform/exact-arithmetic assumptions matter; merely replacing epsilon with a
named robust predicate does not establish arbitrary exponent-range support.
These are arithmetic techniques to study, not selected dependencies or timing
evidence for this operation.

[CGAL's official regularized-set manual](https://doc.cgal.org/latest/Boolean_set_operations_2/index.html)
distinguishes interior overlap, boundary-only contact and disjoint sets.
Regularization removes lower-dimensional features, so an area-only Boolean
intersection is insufficient to preserve point/curve contacts. The manual also
treats hole boundaries as members of a closed polygonal set. The owned design
should make these choices explicitly rather than inherit a library convention.
[Polygon bounded-side documentation](https://doc.cgal.org/latest/Polygon/classCGAL_1_1Polygon__2.html)
separates interior/boundary/exterior from orientation-dependent side.

[Attene2020, abstract read](https://arxiv.org/abs/2105.09772) warns that exact
predicates on rounded intermediate constructions need not classify the original
construction correctly. This supports retaining original exact intersection
parameters/maps or source constraints when later region predicates consume
constructed points. The abstract is the read scope here, not a claim to have
implemented or benchmarked its indirect-predicate algorithms.

## Point membership specification

Canonical local constants are a=3/4, h=1/2, r=1/4. Let q denote the exact local
point obtained from world point p and original frame by `(p-origin)/scale`,
without a floating intermediate. The closed material sets are:

- Box: `|qx|<=a, |qy|<=a, |qz|<=h`.
- Cylinder: `qx²+qy²<=r², |qz|<=h`.
- Plate: box constraints and `qx²+qy²>=r²`.

Interior satisfies every corresponding inequality strictly. Boundary belongs
to the closed set and has at least one equality. Everything else is exterior.
The plate removes the open through-hole; cavity wall and cap hole rims remain
material boundary. Test closed-set admission BEFORE collecting equalities:
a point on a cap's carrier inside its hole is exterior, not boundary.

An independent model can stay in world coordinates: compare original exact
axis plane values `origin_axis±scale*extent`, and compare
`(px-ox)²+(py-oy)²` with `(scale*r)²`. Use exact original dyadic bits or rational
numerator/denominator arithmetic and clear denominators if product code uses
local Fraction expressions. Product classifier/rounded body evaluator must
not supply expected signs. Literal fixtures below remain the strongest narrow
source of truth; the model extends their breadth rather than replacing them.

Supporting face output is the COMPLETE set of active original boundary faces:
top1/bottom2/front3/back4/right5/left6 for box/plate; cylinder top1/bottom2/wall3;
plate cavity wall7. Positive x maps to right5, negative x to left6; positive y
to back4 and negative y to front3. Cylinder seam coordinates yield one wall
face ID despite two coedges/four charts. Cap rims yield both cap and wall IDs;
box corners yield three IDs. Interior/exterior return no supporting faces.
Preserve body/revision/kind identity; geometrically equal distinct bodies do
not acquire equal source identities.

| Exact local point | Box | Cylinder | Plate |
| --- | --- | --- | --- |
| (0,0,0) | interior | interior | exterior |
| (1/2,0,0) | interior | exterior | interior |
| (1/4,0,0) | interior | boundary wall3 | boundary wall7 |
| (1/4,0,1/2) | boundary top1 | boundary top1+wall3 | boundary top1+wall7 |
| (0,0,1/2) | boundary top1 | boundary top1 | exterior |
| (3/4,3/4,1/2) | boundary top1+back4+right5 | exterior | same three boundary IDs |
| (3/20,1/5,0), exact Fractions | interior | boundary wall3 | boundary wall7 |
| (1/4,2^-1074,0) | interior | exterior | interior |

## Contact specification and independent complete corner oracle

Admit only original rectangular planar faces initially, including parallel and
perpendicular axis planes. Disk/rectangle-hole/cylinder faces are explicitly
unsupported for this contact operation even though their body membership is
supported. A plane carrier alone cannot establish a finite face overlap.

Closed intersection dimension is -1 empty,0 point,1 segment,2 area. Coplanarity
of carriers is separate from equality of the two finite domains. Coplanar
disjoint rectangles are disjoint; identical finite rectangle domains have
dimension2 and coincidence; a proper positive-area intersection is overlap.
Opposed normal orientations do not negate geometric coincidence. Keep normal
agreement and source identity separate from contact dimension/relation.

Nonparallel positive-length intersections count as crossing only when an open
part lies in both relative face interiors. Boundary-only line/point contacts
are touching. A pair of adjacent faces of one box shares an edge but does not
cross in both relative interiors. Finite-domain singleton clipping is touching
even though the infinite planes are transverse.

Use an independent small rational linear-constraint model, different from an
interval-overlap product implementation. Obtain declared original world corners
and each rectangle's support plane plus four inward edge halfplanes. For
coplanar carriers, enumerate intersections of the support plane with every
pair of edge-support planes; for independent carriers enumerate their two
planes with each edge-support plane. Solve each rank3 linear system exactly,
retain every solution satisfying ALL closed inequalities, and deduplicate exact
world tuples. Contained original corners are included naturally by their own
edge-support constraints. No rounded product contact vertex supplies a witness.

Every nonempty bounded rectangle intersection has an extreme point in this
complete candidate set. Affine rank of all retained corners determines0/1/2;
an empty set proves disjoint in this admitted bounded convex family. This is
complete corner enumeration, not finite sampling for an arbitrary curved trim.
For dimension1 the midpoint of its extreme endpoints tests both relative
interiors exactly. For dimension2 order extreme vertices on the support plane,
check closed polygon orientation/incidence and equality of the complete domains
against original corners. The design chooses output ordering; the oracle first
compares the complete exact corner set and then that declared ordering.

Each output corner must have one exact spatial tuple and exact left/right UVs
obtained independently from the two ORIGINAL affine rectangle carriers. Verify
UVs within closed[0,1]² and exact world correspondence. Reversed source order
preserves world contact while swapping UV/source roles. Different bodies with
equal corners remain distinct typed sources; source roles must not be sorted
away for the convenience of a symmetric geometry operation.

Useful literal contact fixtures on square [-3/4,3/4]² at z=1/2:

- Same rectangle: area9/4, same domain, four known corners. Same-domain opposite
  normal can be another box's bottom at translated z=1.
- Translate x by3/2: one shared segment x=3/4, length3/2.
- Translate x,y by3/2: singleton corner(3/4,3/4,1/2).
- Translate x by1: proper overlap rectangle x in[1/4,3/4], area3/4.
- Translate x beyond3/2 by one exact representable step: disjoint.
- Raise plane height by any positive exact dyadic amount: parallel disjoint,
  never coincident due to a rounded world height.
- Top/right faces of one canonical box: full shared edge, touching.
- Horizontal top face and a translated perpendicular rectangular face whose
  intersection runs through both relative interiors: crossing segment. Choose
  literal exact endpoints/UVs before assigning that label.

## One-ULP, extreme and original-construction corpus

For every axis boundary and cylinder/plate cardinal radius, submit the exact
boundary value and `nextafter` neighbors on both sides. Include mixed active
constraints and points beyond z or outer bounds even when radial equality holds.
The smallest-subnormal off-axis witness in the table attacks underflow or loss
when adding y² to r². Signed zeros have the same geometric value but retained
float input bits may remain distinguishable request metadata.

Exact Fraction(3/20),Fraction(1/5) lie on radius1/4. Their nearest binary64
roundings generally do not; qualify each SUBMITTED binary value, not the ideal
decimal or symbolic point from which it was rounded. Test arbitrary integral/
rational values with large numerator/denominator exponents and finite DBL_MAX,
smallest normals/subnormals. A float filter must reject inconclusive signs and
use an exact stage or explicit unresolved result; its output must not rely on
ordinary overflow/underflow comparisons. Invalid NaN/infinity/type/extent and
unsupported body/face families remain distinct from geometric exterior.

Original Frame origins may be any admitted binary64, including2^-1074, even
though scale is a power of two. A default box and another whose origin_z is
2^-1074 have distinct exact top planes despite both rounded carrier evaluations
returning0.5. Their top faces are parallel disjoint, not coincident. For a box
with origin_x=2^-1074, world x=0.75 is strictly inside its right plane, while
world x=-0.75 lies outside its left plane. These attack premature float frame
addition/subtraction. Test extremes origin±16 and scales1/4..4 with exact
constructed world queries; when a world query is rounded to binary64, classify
that rounded value rather than assume frame invariance preserved it.

## Intersection context: useful, but bounded

The delivered plane/wall constructor supplies exact coupled maps and events.
It can provide existing exact constructions for future region callers without
regenerating approximate intersection points. Distinguish carrier tangency
from finite trim tangency, contact at wall height endpoints and the material
side of a hole wall. D=r²-offset² gives empty/tangent/two generators only for
the admitted vertical carrier pairing; horizontal sections are circles whose
finite trims may create endpoint/touch behavior.

Do not require a new general SSI/contact classifier to finish the rectangle
unit. If later context output annotates existing curves, declare exactly which
carrier/finite-domain/material relation it means and independently qualify that
field. Exact predicates on a rounded sampled intersection are predicates on
the rounded point, not automatically on its original curve endpoint.

## Assertion-sensitive mutations and measured value

Select actual product sites after accepted design/plan and a direct healthy-
environment red. Candidate distinct fault classes are: plate hole sign reversed
or ignored; strict equality replacing closed membership; boundary equalities
collected before rejection; only first supporting face returned; seam/chart
face duplication; premature float frame subtraction; float radial square/sign;
coplanarity replacing finite-domain coincidence; point/segment regularized away;
touch segment marked crossing by carrier normals alone; wrong source UV/face
mapping after reversal; omission of a contact corner.

Every mutant needs a literal/model assertion on actual product output; build,
import or runtime errors do not earn kills. Use the subnormal/origin witness
for numeric mutations, cap-hole/corner face lists for membership provenance,
and exact degenerate contacts/corner sets for contact mutations. CPU parity,
status-only success and a contact render are not independent geometry evidence.
Native/device mutants and memory checks apply only if the selected architecture
actually introduces those paths; no architecture is inferred here.

Benchmark exact complete operation plus preparation, fallback/refinement,
source/vertex/UV publication and caller consumption. Partition ordinary, exact
boundary, near-boundary, huge-exponent and unsupported/refused workloads rather
than report a bulk average concealing exact-stage costs. Pin original inputs,
source/binaries and retained identities; exclusion/warmup/repeat scopes are
explicit. Point membership throughput does not claim an accelerated Boolean
or complete planar arrangement. Defer session/lifetime/bulk integration tuning
until the broad-pass big rocks finish, as directed.
