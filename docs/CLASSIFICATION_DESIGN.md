# Exact material membership and finite rectangular-face contact

## Purpose and support

This CPU beachhead supplies semantic decisions needed by later face arrangements
and solid operations. It is separate from conservative candidate discovery,
existing plane/wall intersection construction and the GPU adjacency reader.
It does not select Python's eventual shipping role; that decision remains after
the breadth sweep. No new dependency, GPU placement or integration tuning.

Support immutable factory `BodySnapshot` box, cylinder and one-hole plate in
their declared axis-aligned power-of-two frames. Point coordinates accept
`int` (excluding bool), `Fraction`, and finite Python float, exactly as submitted.
Reject unsupported coordinate types, wrong arity and nonfinite values rather
than converting an invalid query into an exterior result. Material membership
means the closed solid, independently of chart seams. Exact world-to-local
subtraction/division uses rational values, never rounded local coordinates.

Face contact supports two existing `FaceRef` values whose faces are rectangular
planar faces. Both parallel and perpendicular axis-aligned carriers are supported.
Disk, hole-cap and cylindrical faces raise `UnsupportedContact`; unsupported is
never an empty intersection. Wider curved-trim and algebraic-point support are
return-pass units, not prerequisites for these rational witness operations.

## Public values

`classify_point(snapshot, point)` returns an immutable factory-only
`PointClassification`: retained snapshot, exact rational world point, label
`interior`, `boundary` or `exterior`, and sorted original face IDs supporting
that point. Exterior has no boundary faces, even if an extended carrier contains
the point. Interior has none; boundary has every incident true geometric face.
A cylinder's atlas seam is not an additional material boundary. Hole-wall
orientation reverses material sense; the cylindrical hole interior is exterior.
`classify_points(snapshot, iterable)` preserves query order and owns an immutable
tuple of these results; invalid input fails without publishing a partial result.
`BodySnapshot.classify_point(point)` is the direct entry point.

`contact_faces(left, right)` returns an immutable factory-only `FaceContact`:
retained left/right FaceRefs, `carrier_relation` (`coplanar`, `parallel_disjoint`,
`transverse`), intersection dimension -1/0/1/2, `relation`
(`disjoint`, `touch`, `crossing`, `overlap`, `coincident`), `same_domain` and
`normal_relation` (`same`, `opposite`, `orthogonal`). Vertices are exact rational
`ContactVertex(position, left_uv, right_uv)` values with exact left and right
UV coordinates on the original
single rectangular charts. Empty has no vertices; point one; segment its two
lexicographically ordered endpoints; area four corners in left-chart positive
parameter orientation (not a Boolean coedge orientation). The complete closed
locus is their convex hull. IDs remain source IDs, not newly manufactured topology.
`BodySnapshot.contact_faces(face_id, other, other_face_id)` constructs FaceRefs.

## Geometry and labels

Material constraints are the canonical box slabs x/y in [-3/4,3/4], z in [-1/2,1/2];
cylinder z slabs plus x²+y² <= 1/16; plate box slabs plus x²+y² >= 1/16.
First establish feasibility. Only then collect equality constraints and map
them to original face IDs. This prevents an infinite cap or hole cylinder from
claiming boundary outside the closed solid.

Read each rectangle's actual rational planar control net and exact frame.
Confirm its single affine axis-aligned chart. Derive ordered world intervals,
including a singleton interval on its fixed axis. Intersect all three closed
intervals. Any negative width means empty; otherwise the count of positive
widths is the locus dimension. Parallel carrier planes are coplanar only when
their exact fixed coordinates agree. Perpendicular carriers remain transverse
even if finite faces are disjoint.

For coplanar area contact, equal complete face domains mean `coincident`,
otherwise `overlap`. Coplanar segment/point contact is `touch`. A transverse
segment is `crossing` exactly when its midpoint is strictly in the relative
interior of both rectangles; otherwise `touch`. A transverse singleton is
`touch`. These are finite-face relations, not solid penetration decisions;
opposing coplanar normals do not alter closed-set coincidence. Normal relation
comes from oriented original U×V and face orientation. Equality uses exact
rational values, with no tolerance welding or rounded sampled evidence.
`same_domain` is false for empty or transverse results; it means equality of
the complete closed face sets, not equality of their intersected subdomains.

## Ownership and failure

Inputs and results are immutable owned Python values, consistent with existing
research APIs. Retained snapshots/FaceRefs preserve body/revision/face/chart
identity and lifetime. Outputs expose no native handles and mutate no body.
Exceptions fail the complete request. No JSON internal representation, dependency
adoption, heuristic fallback, implicit backend routing or silent unsupported case.

## Independent qualification and actual consumer

Literal point fixtures include centers, caps, corners, hole rim, hole void,
wall seam, radial one-ULP cases, frame translations/scales, signed zero,
subnormal offsets and exact rational coordinates. An independent model derives
extent/radius and face geometry from original vertices/curves/loops, without
calling product classification or its plane helpers. Rectangle truth comes from
original polygon half-space membership and pairwise edge/plane intersections,
then exact affine rank and vertex sets, not product interval intersection.
Require actual output world and both UV values to reproduce original controls.

Run a direct healthy old-body caller before the new absent entry-point red.
Run meaningful assertion-sensitive product mutants on the fleet. Review product,
oracles and actual consumers, fixing valid findings. Export rational TSV caller
records and a geometry-derived SVG. Measure complete ordered batches across
small and larger point/contact fixtures, including exact conversion/publication,
with preparation and independent qualification separately reported. No GPU or
CPU24 comparison is claimed for this CPU exact construction operation.
