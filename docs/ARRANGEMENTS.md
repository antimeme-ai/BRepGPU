# Exact regions, holes and shared boundary uses

Construct a complete owned CPU subdivision of one rectangular planar face using
axial grid cuts and disjoint interior circle sections. Each disk lies strictly
inside one open grid cell. Multiple holes in one cell are supported. Clipped,
tangent, nested, coincident or overlapping disks raise `UnsupportedArrangement`;
no unsupported source is silently omitted. General arrangements and GPU
construction remain future units.

```python
from fractions import Fraction
from brepgpu.body import Frame, make_body
from brepgpu.arrangement_oracles import qualify_arrangement

plane = make_body("box", Frame((0., 0., -.5)))
wall = make_body("cylinder")
section = plane.intersect_faces(1, wall, 3)
arr = plane.arrange_face(1, (section,))
qualify_arrangement(arr)

# Exterior0, perforated cell1 and disk2 share exact boundaries.
assert arr.locate_uv(Fraction(1,2), Fraction(1,2)) == (2,)
assert arr.locate_uv(Fraction(2,3), Fraction(1,2)) == (1,2)
```

`arrange_face(FaceRef, sections=(), *, u_cuts=(), v_cuts=())` retains the actual
target and immutable source sections. Cuts accept int excluding bool, Fraction
and finite float exactly, strictly within(0,1). Unordered inputs canonicalize;
duplicate cuts reject. Limits are64 cuts per axis and256 sections; overlong or
infinite inputs are rejected after reading limit+1 items. Source/body/revision/
face/chart and piece/event IDs retain their original namespace. A circle-bearing
source is a horizontal square chart with equal world-axis spans; line-only
grids also support the other owned rectangular faces.

The factory-only `Arrangement` owns immutable vertices, edges, uses, loops and
regions. IDs are dense local table slots. Semantic grid and section-event keys
establish shared vertices; no rounded XYZ welding. Each edge has two opposite
`Use` records with region, twin, next, previous and loop IDs. Every use belongs
to one ordered loop. Exterior region0 owns the clockwise target perimeter.
Cell regions have a CCW outer loop and CW holes; disk regions have a CCW circle
outer loop. Orientation is positive target UV, with original face orientation
retained separately.

Each full circle is **one closed edge and one anchor**, retaining all four
source pieces and their spatial/bothUV maps and named events. Chart joins do
not become new feature edges. Original boundary segments retain coedge/edge
IDs and directed source curve parameters t0/t1. Evaluate original source t as
`(1-t)*t0+t*t1`; t0 may exceed t1. Do not apply source coedge orientation again.

`edge_point(edge_id,t)` and `edge_uv(edge_id,t)` return exact rational tuples.
Lines interpolate canonical endpoints; a circle traverses its four ordered
source pieces in equal parameter quarters. This parameter is neither angle
nor arc length. `Use.orientation` reverses that traversal for the opposite use.

`locate_uv(u,v)` returns all incident **closed** region IDs: one inside a region,
two on an internal edge/circle, four at a grid crossing, plus0 on the original
target boundary. Outside returns(0,). Exact rational witnesses lie strictly in
every emitted region. `Region.area` gives exact normalized-UV area as
`Area.rational + Area.pi * π`; exterior has no finite area. These are face
regions, not material selection or an assembled Boolean solid.

Input/type/arithmetic/iterator failures abort the complete request and preserve
snapshots/sections. No new dependency, native ABI, CUDA lifetime or persistent
edited-body naming is introduced. Python's shipping role stays after the breadth
sweep. See the [design](ARRANGEMENT_DESIGN.md) and [plan](ARRANGEMENT_PLAN.md).
