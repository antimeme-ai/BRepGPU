# Intersection components and exact trim events: independent oracle study

Research only, 2026-10-14. Recommend the current analytic body's **axis-aligned
plane/plane and plane/cylinder** family first. Positive power-of-two scale and
translation preserve it. They do not create oblique planes: ellipses require a
new input family. Return carrier components and explicit clipping/event state;
never relabel unresolved/tangent/coincident cases as complete empty output.

## Primary literature and actual reference reads

Dokken's thesis separates occurrence discovery, loop exclusion and geometric
approximation. Its sufficient loop-exclusion conditions concern whole-domain
orientation/gradient information; they cannot be inferred from a finite set of
Newton seeds or boundary samples. Its implicit/parametric reformulation motivates
using the known analytic equations for this smaller family. Read sections 2.4,
2.5.2–2.5.5 and the representation discussion in chapter 5.
[Dokken, 1997; revised January 2000](https://www.sintef.no/globalassets/upload/ikt/9011/geometri/gaia-ii/ph_d_dokken_1997.pdf).

Gleicher/Kass distinguish interval exclusion/refinement from connecting samples.
Their connection stage needs extra assumptions and discusses ambiguous cases.
It is a useful warning against making topology completeness follow from a
visually plausible point chain. Read sections 4–6 of the retained publisher PDF.
[Gleicher and Kass, Graphics Interface 1992](https://graphicsinterface.org/wp-content/uploads/gi1992-28.pdf).

Read pinned SISL `s1859`, `s1310`, `SISLIntcurve` and `examples/example13.cpp`:
`SISLIntcurve` separates spatial and two parameter curves. `s1859` first returns
topological descriptions and reports intersecting surface regions with warning
10; `s1310` option 2 generates all three curves, while warning 3 can retain only
part of a trace. The example requests only the spatial curve, and returns 0
when catching an error versus 1 on normal completion. It is a source example,
not an executable correctness oracle or conventional exit-status test.
[SISL s1859](https://github.com/SINTEF-Geometry/SISL/blob/e0a82b1a851e932657f3e1af25033fbbcd33e907/src/s1859.c),
[s1310](https://github.com/SINTEF-Geometry/SISL/blob/e0a82b1a851e932657f3e1af25033fbbcd33e907/src/s1310.c),
[example13](https://github.com/SINTEF-Geometry/SISL/blob/e0a82b1a851e932657f3e1af25033fbbcd33e907/examples/example13.cpp).

Read pinned OCCT `IntAna_QuadQuadGeo::Perform(plane,cylinder)` lines 543–722:
it separates line/circle/ellipse results, uses tolerances for near-parallel and
tangent cases and can collapse a near-circle ellipse to circle. These production
choices are not exact-input oracle truth. They demonstrate why our narrow
classification and approximation tolerances must be explicit.
[OCCT source](https://github.com/Open-Cascade-SAS/OCCT/blob/b8f597c677811d1f9f4d8a97f5ae2825c0353a42/src/ModelingData/TKGeomBase/IntAna/IntAna_QuadQuadGeo.cxx),
[official interface](https://occt3d.com/dev/doc/refman/html/class_int_ana___quad_quad_geo.html).
No implementation/reference dependency is adopted.

## Complete carrier truth from original maps

Interpret original control/frame binary64 values as exact Fractions. First bind
those actual controls to the known affine or rational-quarter maps; shape names
alone cannot establish that inputs match the analytic family. World cylinder
center/radius/height and plane equations follow from the submitted frames.
Source chart rectangles remain distinct from normalized local coordinates.

- Nonparallel axis-aligned planes have exactly one infinite line. Intersect its
  parameter interval with both original rectangular carrier domains. Parallel
  distinct planes are complete empty; coincident planes require explicit
  unsupported/region output, rather than a fabricated curve.
- A horizontal plane through a bounded cylinder's height has one full carrier
  circle, with four rational chart pieces. Outside the height it is empty.
  A circle at a height endpoint lies on a domain boundary; state whether this
  whole-boundary case is supported.
- For vertical plane x=h and cylinder center `(cx,cy)`, radius r, compute
  `delta=r²−(h−cx)²` exactly. Delta<0 is empty; delta=0 is carrier tangency and
  requires explicit refusal/special handling; delta>0 gives exactly two lines
  `y=cy±sqrt(delta)`. Clip both against plane/wall domains. The y-plane case is
  symmetric. A numerical discriminant tolerance must not silently change this
  exact-input classification.

These classifications prove every carrier component in the admitted family.
They do not depend on tracing seeds. A bounded work failure cannot imply empty
geometry. General rational SSI, arbitrary cylinder axes and cylinder/cylinder
intersection remain outside this proposed first operation.

## One spatial curve and both parameter representations

Use one common parameter on every piece. For a quarter let
`D=1+s²`, `C(s)=((1−s²)/D,2s/D)`, s in[0,1], with the declared signed quadrant
rotation. The horizontal spatial circle is `(cx+r*Cq_x,cy+r*Cq_y,h)`; cylinder
UV is `(s,(h−z0)/(z1−z0))` and lifted face UV `(q+s,v)`. Plane UV is its original
world affine inverse applied to the same point. For a vertical generator use
world z as the common parameter, the fixed cylinder root s and both original
affine height maps.

Prove complete correspondence by rational polynomial cross-products on each
piece when coefficients are exact; otherwise certify the declared approximation
error over the whole interval with exact/interval coefficient bounds. A few
residual samples supplement that bound but do not replace it. Retain exact
model/root descriptions separately from rounded output control/endpoint values.
An irrational constant cylinder parameter cannot be encoded exactly as an
ordinary binary64 pcurve coefficient.

One full circle is **one closed component**, not four unrelated curves. Its
piece graph is a cycle; quadrant boundaries are exact shared events and a=0/4
has two UV occurrences of the same spatial seam event. Preserve both source
occurrences and common event identity. Do not weld endpoints by sample-distance
threshold or assume equal derivative speeds across the rational chart joins.

## Exact clipping and irrational events

Carrier/domain clipping precedes trim membership. Substitute the common curve
into each relevant rectangle, disk or rectangle-minus-open-disk inequality.
Positive rational denominators permit exact sign predicates. Enumerate all real
constraint roots, plus chart/domain endpoints, order them exactly and decide
membership on every root-free open interval using one exact rational witness.
Continuity and complete root enumeration establish the entire interval's sign;
this is not finite sampling offered as completeness.

For current lines, disk/hole cuts are quadratic. For horizontal circles,
rectangle cuts are quadratic; circle/disk or circle/hole cuts also reduce to
quadratic after using the exact circle identity. A generic unreduced squared
expression can be quartic; remove only proved positive/extraneous factors.
Constant nonzero constraints have no events; identically zero constraints mean
boundary coincidence and need explicit handling.

Represent a nonrational event by its exact polynomial plus branch/root ordinal
and a rational isolating interval, or an equally strong quadratic algebraic
value. Square-root/root ordering and sign checks can use exact rational/integer
arithmetic. Distinct event brackets must be separated; an iteration cap means
unresolved, not missing roots. Simultaneous constraint roots need shared event
identity, not duplicate nearly equal endpoints.

Returned float XYZ/UV values approximate such an event; they are not automatically
exact BOUNDARY inputs. In particular the plate hole has no binary64 UV boundary
witness. Even its centerline's dyadic world endpoints ±1/4 map to nonbinary plane
UV 1/3 and2/3. Off-center y=1/8 cuts at x=±sqrt(3)/8: both spatial and parameter
endpoints require algebraic identity/approximation bounds. Likewise vertical
plane x=1/8 on a radius1/4 cylinder yields two irrational generator positions;
the positive quadrant root s=1/sqrt(3) is not a dyadic endpoint.

Transverse carriers can have **trim** tangency: a repeated clipping root may
produce an isolated admitted point or leave an arc connected. Entire intersection
curves may coincide with a trim edge (aligned hole/rim). Define these separately
from carrier tangency, with explicit supported point/boundary components or
refusal. Dropping zero-length events or treating an identically-zero polynomial
as no root is incorrect. The closed trim contract includes admitted endpoints.

## Proportional direct oracle campaign

Start with one full interior horizontal circle and its four-piece UV cycle;
a rectangle-clipped circle crossing a seam; two vertical generator branches;
a plane/plane line split into two plate-hole segments; an off-center irrational
cut; and complete empty separated cases. Add dyadic transforms, swapped source
order, orientation changes, endpoint ties, one-ULP discriminant neighbors and
explicit tangent/coincident/trim-degenerate refusals. Count connected spatial
components and verify ordered piece/event incidence, complete intervals and
source-domain/trim membership, plus the coupled whole-piece geometry bounds.
Missing one component, losing seam adjacency, swapping a pcurve source, ignoring
a hole, and misclassifying an irrational endpoint are distinct later mutation
fault classes. No broad general-SSI campaign is needed for this analytic unit.

## Round-two findings to carry into design

1. Current body frames imply axis-aligned circles/generators, not oblique ellipses.
2. Algebraic event identity and float endpoint approximation are different outputs;
   exact trim semantics prohibit forcing rounded UV to BOUNDARY.
3. Whole component completeness needs analytic classification plus complete root/
   interval coverage; successful residuals or a closed-looking chain are weaker.
4. Carrier tangency, carrier coincidence, trim tangency, trim coincidence and
   domain-boundary components need separate support/refusal decisions.
5. A circle's spatial component/event graph must survive chart/seam splitting
   and retain both parameter occurrences; graph identity cannot come from epsilon
   welding or borrowed addresses.

The acquired Dokken PDF is ignored research material; source/hash/restoration
is recorded in `intersection-oracle-sources.tsv`. Existing reference archives
remain intact under their prior manifests. No product/test/performance changes.
