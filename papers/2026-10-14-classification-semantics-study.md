# Exact material membership and finite-face contact semantics

Research, 2026-10-14. Read current project/shared doctrine, owned body records,
body truth, complete plane/wall sections and the broad-pass roadmap. Read pinned
OCCT classifier source/tests, retained OpenSCAD CGAL integration, official CGAL
manuals and Shewchuk's primary paper. This report specifies mathematics and
bounded public meaning; it selects no dependency or eventual shipping language.
No product code or execution is claimed.

## Recommended complete beachhead

Deliver two useful semantic operations: exact point membership in every owned
box/cylinder/one-hole plate, and complete closed contact between two owned
axis-aligned rectangular planar faces. Inputs retain their immutable source
snapshots. Point queries accept exact rational world coordinates, including the
exact value of submitted finite binary64 coordinates. Face queries return the
complete finite locus, its dimension, both original UV representations and a
precise relation. This advances classification before arrangements/Booleans;
it does not deepen general SSI or investigate Python's final product role.

Finite crossing/touching/disjoint and coplanar overlap/coincidence are supported
without pretending this family includes smooth curved tangency. Disk/hole-cap
overlap, cylindrical face contact and algebraic-coordinate query points should
be explicitly unsupported in this unit. Existing exact plane/wall sections
remain useful geometry, but adding their classification is a separate broader
operation rather than a hidden requirement for this pass.

## Primary sources: precise lessons and their limits

CGAL's regularized polygon operations apply closure-of-interior semantics and
remove lower-dimensional features. Its `do_intersect` documentation specifically
asks whether polygon interiors intersect. Such a predicate can legitimately
return false for closed polygons touching at an edge or point. Our contact
operation must retain those loci; an area-only regularized intersection cannot
answer this request. This is a semantic distinction, not a library defect.
[Regularized set operations](https://doc.cgal.org/latest/Boolean_set_operations_2/index.html),
[interior intersection predicate](https://doc.cgal.org/latest/Boolean_set_operations_2/group__boolean__do__intersect.html).

The CGAL Nef manual separately provides point-set inclusion, interior, boundary,
closure and regularization; boundary marks select lower-dimensional cells.
An oriented manifold boundary is a restricted representation, not something
every Boolean point set possesses. We infer that later solid publication should
declare regularization separately from this contact service: a zero-dimensional
touch is valuable classification even if a later solid operation discards it.
[Nef point-set semantics](https://doc.cgal.org/latest/Nef_3/index.html).

Shewchuk develops exact sign predicates for floating inputs, with adaptive
precision certified by error analysis. His short paper assumes radix two/exact
rounding and does not address exponent overflow/underflow. It motivates exact
decisions and measured filters, but does not by itself qualify arbitrary tiny
radial-square arithmetic or exact output constructions. For this narrow family,
rational arithmetic can establish direct truth before a separately reviewed
filter or accelerator. Exact predicates must not receive already rounded local
coordinates as if those were original inputs.
[Author's paper](https://people.eecs.berkeley.edu/~jrs/papers/robust-predicates.pdf),
[author's research page](https://www.cs.cmu.edu/~quake/robust.html).

Pinned OCCT `BRepClass3d_SClassifier` classifies with a caller tolerance,
vertex/edge proximity and line/surface transitions; its source explicitly
avoids tangent transitions when probing the infinite point. `Face()` supplies
one determining face, not necessarily every incident boundary face. Its
`State()` returns OUT for internal error states. These are important adapter
limits: we cannot interpret a tolerance-based ON or failure-mapped OUT as our
exact closed-material truth or complete supporting-face set.
[Classifier source](https://github.com/Open-Cascade-SAS/OCCT/blob/b8f597c677811d1f9f4d8a97f5ae2825c0353a42/src/ModelingAlgorithms/TKTopAlgo/BRepClass3d/BRepClass3d_SClassifier.cxx),
[public contract](https://github.com/Open-Cascade-SAS/OCCT/blob/b8f597c677811d1f9f4d8a97f5ae2825c0353a42/src/ModelingAlgorithms/TKTopAlgo/BRepClass3d/BRepClass3d_SClassifier.hxx).

Retained OpenSCAD CGAL integration uses rational Gmpq kernels for Nef geometry,
allocates new immutable-backed Nef values for modifying operations and maintains
selection marks/hole boundaries in projected polygons. Its outline conversion
uses `to_double`; that display/export conversion must not become exact contact
input. No OpenSCAD or CGAL code/dependency is selected for our implementation.
[Kernel choices](https://github.com/openscad/openscad/blob/21780e0987e72f6ea9ceaa5a8e2d41f58fe66b96/src/geometry/cgal/cgal.h),
[projection and conversion](https://github.com/openscad/openscad/blob/21780e0987e72f6ea9ceaa5a8e2d41f58fe66b96/src/geometry/cgal/cgalutils-project.cc).

## Exact closed material, including the hole

Let exact world p map to local `(x,y,z)=(p−origin)/scale`. All frame values are
the exact dyadic numbers stored in the owned snapshot; division/subtraction must
not first round back to binary64. Define closed predicates:

- Box: `|x|≤3/4`, `|y|≤3/4`, `|z|≤1/2`.
- Cylinder: `x²+y²≤1/16`, `|z|≤1/2`.
- Plate: the box predicates AND `x²+y²≥1/16`.

Exterior means any predicate fails. Interior means all applicable inequalities
are strict. Boundary means all predicates hold and at least one is equality.
Only after feasibility may equality constraints emit original supporting face
IDs. This ordering is essential: `(0,0,1/2)` is outside the plate because it lies
in the through-hole, even though the infinite cap plane contains it. Likewise,
`(1/4,0,2)` is outside the cylinder/plate despite its extended wall equality.

Box slab equalities map to top/bottom/front/back/right/left. Cylinder radial
equality maps once to its wall; cap+wall rims emit two faces. Plate radial
equality maps to its cavity wall, whose material sense is opposite the outside
of a filled cylinder; rim points additionally emit the cap. Outer box corners
emit all three incident planes. Boundary witnesses are sorted face IDs in the
retained source body/revision, not guessed nearest faces or new entity IDs.

The cylinder atlas seam is not an extra material face. At `(1/4,0,0)`, solid
membership is boundary supported by one wall face. Artificial seam coedges or
quarter-chart joins do not multiply the boundary witness. The existing UV trim
classifier has its own seam-cut parameter-domain labels; reuse of that label
as solid membership would conflate parameter cuts and material boundaries.

## Finite rectangular-face locus

A rectangle has one exact affine chart `P(u,v)=P00+uU+vV`, `u,v∈[0,1]`, with U
and V along different world coordinate axes. Its third coordinate is fixed.
Confirm the actual net is affine, nondegenerate and axis aligned. Read the exact
frame and controls rather than inferring coordinates from a face name alone.

Its closed point set is the product of three world intervals: two positive
intervals and one singleton. Intersect all three intervals with the other
rectangle. A negative width yields the empty set. Otherwise the number of
positive widths gives dimension zero, one or two. This proves completeness
for the declared family: there are no missing branches, tolerances or seeds.

- Dimension −1: no vertices.
- Dimension 0: one exact world point.
- Dimension 1: two exact lexicographically ordered endpoints.
- Dimension 2: four exact corners ordered positively in the LEFT UV chart.

The locus is the convex hull of these vertices. Original left/right UV values
must be exact affine inverses and reproduce each world point from each original
net. They need not be dyadic: a `3/2` chart span introduces thirds. Approximate
SVG coordinates are output presentation only. These vertices do not constitute
new body topology or a persistent shared-event identity.

## Separate carrier, dimension, finite relation and normals

For parallel normal axes, exact plane coordinates determine `coplanar` versus
`parallel_disjoint`. Different normal axes give `transverse`, even when finite
faces are disjoint. Carrier relation alone cannot decide finite contact.

Coplanar point/segment loci are `touch`. Positive area is `overlap`, except that
equal COMPLETE rectangular domains are `coincident`. Carrier coincidence does
not imply domain coincidence: distinct finite trims can share one infinite
plane yet be disjoint, touch or overlap. Equal geometric domains may originate
from different bodies/faces and keep both source identities. Partial containment
is overlap, not identical-domain coincidence.

A transverse segment is `crossing` iff its midpoint lies strictly in the
relative interior of both rectangles. Here this is a complete test, not a
sampling heuristic: the plane constants must lie inside each other's fixed
in-plane intervals, while a midpoint of a positive overlapping remaining
interval is strictly within both original intervals. If either fixed coordinate
lies on a rectangle edge, the whole segment lies on that face's boundary and
the relation is `touch`. A transverse singleton is touch. Endpoints can be on
trim boundaries while the open segment is a true relative-interior crossing.

These words describe finite FACE sets. They do not claim solid penetration or
smooth tangent contact. Existing plane/wall `Component.tangent` is a carrier
flag and must not later be treated as either global solid relation or complete
contact classification without its own reviewed semantics.

Normal relation uses the oriented original `U×V` multiplied by original face
orientation. It is same/opposite for parallel axes and orthogonal for different
axes. Opposed normals do not destroy closed-set coincidence. Positive left-UV
area ordering is independent of that outward normal sign and is NOT future
Boolean coedge orientation. Source-swapping can change area vertex sequence;
the world set, dimension, finite relation and paired original UV meaning remain.

## Concrete fixtures and fault attacks

The canonical top face at z=1/2 and a front face translated by `(0,3/4,1/2)`
have a transverse interior segment with midpoint `(0,0,1/2)`: crossing. The
untranslated top/front share their boundary edge: touch. Coplanar top faces
translated by `(3/2,0,0)` touch on a segment; `(3/2,3/2,0)` touch at one corner;
`(1/2,0,0)` share positive area. A top face and another box's bottom face
translated by `(0,0,1)` have identical domains and opposite normals.

Exact small values expose bugs that ordinary grids miss:

- Cylinder query `(1/4,2^-1074,0)` is exterior because its radial square exceeds
  the radius by `2^-2148`; plate membership at that point is interior. Squaring
  in binary64 would erase the second term.
- Plate query `(1/4,2^-1074,1/2)` is supported only by the top cap, not the hole
  wall. Rounding would fabricate an additional supporting face.
- Translate a parallel top face by z=`2^-1074`. Its exact carrier differs from
  z=1/2 even though rounded addition can return 1/2; finite contact is empty.
- Signed zero has the same geometric value, whereas one-ULP radial or planar
  shifts can change the result. NaN/infinity/wrong arity/type must fail instead
  of becoming exterior/disjoint.

Attack source swapping, extreme valid frame scales/origins, different body
namespaces with identical geometry, non-dyadic UV outputs, opposing coincident
normals and unsupported hole-cap/disk/wall faces. A candidate bound or sampled
point cloud cannot independently establish any of these exact decisions.

## Independent truth and actual use

For material points, derive solid extents and wall radius/height from original
vertices/curves/face roles, keeping sign/orientation and the hole's opposite
material sense explicit. This independently attacks mismatch between declared
shape/frame constants and actual source geometry. Check every emitted boundary
face, not just the three labels or successful return.

For rectangular contact, a distinct oracle can use original polygon half-space
tests plus exact edge/plane and coplanar edge/edge intersections, preserving
contained corners and collinear edge endpoints. Compute affine rank and the
complete exact vertex set from those candidates. This is not another copy of
the product's interval/max/min construction. Independently invert original
affine control nets to check both UV sets. A literal relation fixture adds
source-order and crossing/coincidence expectations.

A runnable consumer can classify an ordered rational point grid in the three
solids and emit complete exact contact loci/labels/UV maps for the above face
families. That is directly useful to later region witnesses and arrangements,
although it neither produces Boolean bodies nor imports general CAD models.
Measure exact input conversion, construction, immutable result publication and
actual consumption; report preparation and independent qualification separately.
There is no automatic GPU, CPU24 or final-language decision in this research.

## Source acquisition and later work

The Shewchuk ten-page primary PDF was acquired under ignored `papers/pdfs/`;
its source/hash/restoration command are recorded in the accompanying source TSV.
OCCT/OpenSCAD snapshots were read in the existing quarantine with their intact
pinned archives and restoration manifests; no archive or extracted source was
changed. CGAL manuals were read live as primary documentation; no CGAL archive
or dependency adoption is claimed.

After the broad pass: exact filtered/native/GPU placement, broader polynomial
or algebraic points, disk/hole overlap, smooth contact, curve-stratum contact,
regularized solid set operations, robust ray classification of general shells
and shared-event history. Investigating the eventual role of Python remains
after the breadth sweep as the operator explicitly directed.
