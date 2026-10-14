# Exact material and rectangular-face decisions

This CPU operation classifies exact rational witness points against the owned
box, cylinder and one-hole plate. Finite-face contact is separately limited to
their rectangular planar faces. Closed boundaries remain part of the material;
unsupported input raises an exception rather than returning exterior/disjoint.

```python
from fractions import Fraction
from brepgpu.body import make_body, Frame
from brepgpu.classification import classify_points, contact_faces
from brepgpu.intersections import FaceRef

plate = make_body("plate")
rows = classify_points(plate, ((0, 0, 0), (Fraction(1, 4), 0, 0), (0.5, 0, 0)))
# labels: exterior (hole), boundary (wall), interior

left = make_body("box")
right = make_body("box", Frame((0.5, 0, 0)))
contact = contact_faces(FaceRef(left, 1), FaceRef(right, 1))
# coplanar overlap, dimension 2, exact world corners with both source UVs
```

`snapshot.classify_point(point)` returns a `PointClassification` retaining the
snapshot, exact rational `point`, string `label`, and sorted `boundary_faces`
containing every supporting original face ID. An exterior point has none,
even when it lies on an infinite extension of a boundary carrier. The cylinder
seam is one physical face; its chart/coedge occurrences add no material face.
`classify_points` returns an owned immutable tuple in input order. A malformed
row or failed iterator fails the whole call without publishing a partial batch.

Coordinates accept int excluding bool, Fraction and finite float. A float means
its exact submitted binary value, not an intended decimal. World-to-local frame
arithmetic remains rational; no tolerance or local floating normalization.
Other coordinate types, nonfinite values and wrong arity raise. Algebraic Scalar
points are a future support unit; choose rational region witnesses for this API.

`snapshot.contact_faces(face_id, other, other_face_id)` or `contact_faces` returns
a retained pair of `FaceRef` values, `carrier_relation`, `dimension`, `relation`,
`same_domain`, `normal_relation` and immutable `vertices`. Each `ContactVertex`
contains `position`, `left_uv`, `right_uv`, all exact rational tuples on the
original charts. The convex hull of its vertices is the complete closed locus.

| Dimension | Vertices | Finite-face relation |
| --- | --- | --- |
| -1 | none | disjoint |
| 0 | one | touch |
| 1 | two lexicographic endpoints | touch, or transverse crossing through both relative interiors |
| 2 | four corners in positive left-UV orientation | overlap, or coincident complete domains |

Plane carrier relation is coplanar, parallel_disjoint or transverse independently
of finite contact. Normal relation is same, opposite or orthogonal independently
of empty contact. Opposing normals can share a coincident finite domain.
`same_domain` concerns complete closed faces, not just their common patch.
These labels describe faces; they do not claim a solid penetration/Boolean result.
Disk, hole-cap and wall pairs raise `UnsupportedContact`. General circle overlap,
near-coincident splines, arrangements and Boolean assembly remain future themes.

The [design](CLASSIFICATION_DESIGN.md) and [whole-unit plan](CLASSIFICATION_PLAN.md)
state exact semantics and independent qualification. Python's eventual shipping
role is explicitly scheduled [after the breadth sweep](RETURN_PASS.md).
