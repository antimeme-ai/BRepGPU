# Complete bounded planar arrangements

## Purpose and admitted family

Deliver CPU region construction for later perforated cap/Boolean and meshing
consumers. Input is one owned single-chart rectangular planar `FaceRef`, zero
or more exact normalized-UV axial cuts, and zero or more existing complete
plane/wall circle `IntersectionSet` values. Each circle disk must lie strictly
inside one open grid cell; disks must be pairwise strictly disjoint. Multiple
circles in a cell are supported. No arbitrary clipped/crossing/tangent/nested/
coincident circle overlay, source hole caps, tilted/imported geometry, GPU
construction or language/integration redesign. Unsupported geometry raises
`UnsupportedArrangement`; it never silently drops a section or publishes a
partial arrangement.

Cuts accept int excluding bool, Fraction and finite float exactly as submitted,
strictly between0 and1. Input cut order is arbitrary; result cuts are canonical
sorted exact values, duplicates rejected. At most64 cuts per axis and256 sections
bound preparation/output size. Consume at most limit+1 inputs before rejecting
overlong/infinite iterables. Sections must retain the same original snapshot
object/face ID as the target on one operand, with the other a cylindrical wall;
both operand orders work. Circle sections are supported only on horizontal
rectangular faces. Empty/open/clipped/point/multicomponent sections reject.
One circle component must contain exactly four distinct rational quarter arcs,
each parameterized over[0,1], with its four original named join events and
unmodified spatial/bothUV maps. Complete source sections remain retained.

## Public values and identity

`arrange_face(face, sections=(), *, u_cuts=(), v_cuts=())` returns factory-only,
recursively immutable `Arrangement(face, sections, u_cuts, v_cuts, vertices,
edges, uses, loops, regions, outside_id=0)`. The body adapter is
`snapshot.arrange_face(face_id, sections=(), *, u_cuts=(), v_cuts=())`.
Records use dense result-local IDs, never pointers/persistent edited-body IDs:

- `Vertex(id, uv, position, source)` with exact rational tuples. Grid source is
  `("grid", i,j)` on augmented sorted cut axes. Circle anchor source is
  `("section", section_index, source_event_id)`. These semantic keys establish
  identity; rounded/equal XYZ values never weld unrelated source namespaces.
- `Edge(id, kind, start, end, source, piece_ids, event_ids)`. Grid segments have
  kind `line`; circle has kind `circle` and start=end at one declared anchor.
  Boundary source is `("boundary", original_coedge_id, original_edge_id, lo,hi)`
  with increasing exact original-edge parameter bounds. Interior lines name
  `("u_cut", sorted_cut_index)` or `("v_cut", sorted_cut_index)`. Circle source
  is `("section", section_index, component_id)` with ordered original piece IDs
  and all original event IDs. The retained target/sections qualify every ID.
- `Use(id, edge, orientation, region, twin, next, previous, loop)` with orientation
  ±1 relative to canonical edge traversal. Every edge has two opposite uses,
  including original boundary edges; one boundary use belongs to exterior.
- `Loop(id, region, hole, uses)` with an ordered nonempty cyclic tuple.
- `Region(id, kind, cell, section, outer_loop, holes, witness_uv, area)`.
  Exterior region0 has kind `outside`, no outer loop and one clockwise hole
  loop around the target boundary, witness(-1,-1), areaNone. Cell regions come
  next in row-major `(i,j)` with u varying fastest; disk regions follow section
  order. Cell kind `cell`, sectionNone; disk kind `disk`, cell identifies its
  containing grid cell and section its original selection index.
- `Area(rational, pi)` encodes exact normalized-UV area `rational + pi*π`.
  Exterior has no finite area. Cell rational term is its rectangle area and pi
  term minus its contained disk radii squared; disk has0 rational and +r² pi.

Loops orient regions on their left in positive target UV: cell outer CCW,
circle hole CW, disk outer CCW, exterior boundary CW. Original face orientation
is retained separately and does not silently reverse UV topology. Full circles
are one closed edge and one anchor, with a one-use disk loop and opposite hole
loop. Original chart joins remain source events/maps, not four fabricated
physical feature edges. Sections with duplicated geometry reject even when
body identity differs.

## Exact construction and readers

Read actual target rational affine controls/frame, including fourth-corner
identity, ordered axes and source boundary coedge mappings. No sampled bounds.
Build the Cartesian grid from augmented cuts0/1. Produce each adjacent grid
segment once; attach its two region uses by semantic cell/boundary incidence.
Canonical horizontal edges traverse positive U, vertical edges positive V.
Assemble each cell's four ordered outer uses and the exterior perimeter cycle.
Map each boundary segment to the original face coedge/edge and exact curve
subrange; original edge direction can oppose canonical arrangement direction.
Reader qualification must use the actual source orientation, not guess by slot.

Validate original full circle source/control geometry and event incidence,
derive exact rational UV center/radius, and establish strict containment in one
cell. Require squared center separation >(r1+r2)² for every pair, excluding
nested, tangent and coincident disks. Anchor at the first source component
piece's initial named event, preserve component order and all maps. Add one
closed edge, a disk region and opposite disk/hole uses. Every contained section
creates exactly one disk region and one hole in its cell; no curve is omitted.

`Arrangement.edge_uv(edge_id, t)` and `.edge_point(edge_id,t)` accept exact
rational/finite-float t in[0,1]. Lines interpolate exact original affine target
UV/world. Circle t traverses its four ordered source pieces in equal parameter
quarters, with t=1 returning the anchor; maps are read from retained source
pieces with the selected original operand side. This parameter is neither angle
nor arc length. Readers return rational tuples; they cannot round or relabel
section/chart identity. IDs exclude bool and reject outside table ranges.

`Arrangement.locate_uv(u,v)` returns the sorted tuple of ALL incident closed
region IDs, reading the actual emitted region/cell/section correspondence.
Outside the closed target domain returns(0,); on original boundary include0
along with incident cells. Internal grid edges have two cells and crossings four.
Circle boundary belongs to disk and its cell region; its interior only disk;
outside disks belongs to incident cells. Reject unsupported/nonfinite input.
This is source-face region membership, not a solid Boolean decision.

Every bounded region exposes a strict rational witness. Disk uses its center.
For a cell, choose U strictly between its lower boundary and the minimum circle
left footprint in that cell (or cell midpoint if none), with V at cell midpoint.
Strict disk/cell separation makes this witness valid without epsilon search.
Winding and areas describe UV; multiply by the original affine surface Jacobian
for physical area if a future consumer needs it.

## Completeness, failure and qualification

For k×l cells and m circles there are(k+1)(l+1)+m vertices,
2kl+k+l+m edges and kl+m bounded regions. The embedded graph has m+1 components;
including the exterior, V−E+F=m+2, not always2. These counts are necessary,
not the independent completeness oracle.

Independent source truth enumerates every cell-minus-disks and every disk and
matches an exact bijection to emitted regions, all ordered boundary subranges,
source maps and shared uses. Prove line affine/source identities and retain
complete source circle maps; rounded finite samples cannot prove coverage.
Check every twin/next/previous/loop/region, orientation, witness, exact area sum,
full original boundary/cut coverage and circle domain coverage. Literal cases
include bare rectangle, shared grid crossing, one circular cut, multiple holes
in one cell, mixed cells, reversed source operands, bottom/source orientations,
translated/scaled frames and exact subnormal/one-ULP separation guards.

Input errors, unsupported contacts, iterator failures or allocation/arithmetic
failure abort the complete call without changing snapshots/sections or exposing
partial output. Own values use existing standard-library arithmetic; no new
dependency, native ABI or GPU lifetime is introduced. Product tests, mutants,
callers and serial costs run only on the fleet. Publish actual graph/region/
source TSV and SVG; separately report source-section preparation, construction,
graph/reader consumption and independent qualification. Python's eventual role
and integration reuse remain explicitly after the breadth sweep.
