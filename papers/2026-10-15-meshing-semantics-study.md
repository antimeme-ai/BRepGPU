# Complete bounded body meshing: geometry and topology semantics

2026-10-15. Recommend a complete owned CPU mesh of the existing canonical box,
cylinder and one-through-hole plate snapshots. Consume original physical edges
once, retain every coedge occurrence, triangulate the actual bounded face
domains, and publish one closed outward mesh with an explicit world-space
approximation bound. This is a useful meshing operation without a general
constrained triangulator, clipped sampling grid or new dependency. Language
architecture and GPU placement remain separate deferred decisions. This study
contains no product implementation or product/fleet execution.

Source identities and actual reading scopes are in
`meshing-semantics-sources.tsv`; bibliography is `meshing-semantics.bib`.

## Source study and its consequences

Read current doctrine, project instructions, acceleration roadmap, original
`body.py` factory/curve/coedge/chart records and the corresponding analytic
body-oracle expressions. Actual dimensions are half-width 3/4, z in [-1/2,1/2],
circle radius 1/4. Frames have positive power-of-two scale in [1/4,4] and bounded
binary64 origins. Cylinder has two disk faces and one four-chart wall; plate
has two square-with-hole caps, four outer planar sides and one inward-facing
four-chart hole wall. Each rim is one closed original edge and anchor. The
wall seam has two coedge occurrences on the same original face.

Read pinned OCCT 8.0.1 commit
`b8f597c677811d1f9f4d8a97f5ae2825c0353a42` BRepMesh model/shape visitor, edge
discretization, face dispatch and seam preprocessor code. The model visitor
deduplicates physical edges, then attaches each face-use pcurve with its
orientation. Edge discretization samples the 3D curve and propagates the same
sample parameters into every pcurve; face triangulation follows that skeleton.
This is the right dependency order for avoiding cracks. The seam preprocessor
distinguishes forward/reversed pcurves of one edge; its actual amplification
special case is cones, not proof that our cylinder needs added height levels.
The visitor can mark a bad internal wire and continue; our complete bounded
operation must instead reject atomically rather than silently omit a hole.
[Pinned edge discretization](https://github.com/Open-Cascade-SAS/OCCT/blob/b8f597c677811d1f9f4d8a97f5ae2825c0353a42/src/ModelingAlgorithms/TKMesh/BRepMesh/BRepMesh_EdgeDiscret.cxx).

Current official BRepMesh documentation separates curve deflection from face
interior deflection and describes the shared skeleton/face workflow. Its
tolerance floor and general healing behavior are not our exact bound contract.
[Official mesh guide](https://occt3d.com/dev/doc/overview/html/occt_user_guides__mesh.html).

Read Shewchuk's primary Triangle paper sections on triangulation structures and
Ruppert refinement. Boundary constraints and explicit holes determine the domain;
an unconstrained triangulation of sampled points does not suffice. Refinement
adds distinct angle/size guarantees with additional termination conditions.
Our fixed canonical construction can own a much smaller complete algorithm,
but must not claim Delaunay, minimum angle, aspect-ratio, simulation suitability
or general PSLG support. Triangle is reference literature, not an adopted
dependency. [Primary paper, domain and hole construction](https://www.cs.cmu.edu/~quake/tripaper/triangle3.html).

## Proposed minimal request and result

`mesh_body(snapshot, max_deviation=...)` accepts the existing immutable owned
snapshot and a strictly positive finite int excluding bool, Fraction or finite
binary64 tolerance interpreted as its exact value in world units. Admit only
the three original canonical body families and existing frames. Bound refinement
explicitly, for example quarter subdivision n=2^level with level at most 12.
Reject an impossible requested tolerance before allocation/publication; do not
clamp it silently or emit a partial mesh. The exact limit belongs in reviewed
design, not an unbounded loop.

Publish an immutable result retaining the actual snapshot, requested tolerance,
chosen n, exact achieved upper bound, exact rational world vertices, triangles
with original face identity, per-corner original chart/local UV occurrences,
and ordered original coedge-to-mesh chains with original curve parameters.
Dense mesh IDs are result slots, not persistent entity identity. Distinguish:

* original vertex key `(body,revision,vertex_id)`;
* original edge interior sample key `(body,revision,edge_id,t)`;
* face-interior key `(body,revision,face_id,local_role)` for disk centers.

Retain the original type namespace. Ring t=1 aliases t=0's physical anchor;
quarter joins are shared mesh vertices on that edge, not new B-rep features.
Coordinate equality never welds distinct original edges, bodies or instances.
Closing an edge-use chain can repeat its first ID at the end without publishing
a duplicate physical vertex. Opposite coedges reverse the ordered chain, while
their UVs are evaluated from each original pcurve at the underlying canonical
curve parameters. Do not apply orientation twice.

The same cylinder seam world vertices/segment appear at u=0 and u=4 in two
chart/use occurrences of one original face. Per-corner attributes preserve
these occurrences while triangles share physical vertex IDs. At the other
quarter joins, neighboring charts similarly retain their own local u=0/1.
Resolve source chart IDs by face/quadrant meaning, never assume chart slot is
the persistent ID. No seam crack, normal discontinuity weld, or duplicate shell
is justified by an atlas occurrence.

## Exact common rim and certified chord bound

For each quarter use local s=j/n, j=0…n, n dyadic. Original circle geometry is

`p(s)=r*((1-s²)/(1+s²), 2s/(1+s²))`,

followed by the quarter rotation and fixed z. The original full-edge parameter
is t=(quadrant+s)/4. Evaluate the authoritative source records with rational
arithmetic and apply the exact original frame. Existing `Curve.evaluate` and
`UVMap.evaluate` coerce to float, so they cannot implement this exact path.
Do not replace this rational parameter by angle or equal arc length.

Let R=scale*r be the exact world radius. Since the polar angle is 2 atan(s),
each interval has angle Δθ at most 2/n. Its circular sagitta obeys

`h = R*(1-cos(Δθ/2)) <= R*Δθ²/8 <= R/(2*n²) = δ`.

The first inequality follows from 1−cos(x)≤x²/2. Thus an exact rational
comparison δ≤requested tolerance selects the first allowed dyadic n without
acos, rounded square roots or empirical sampling. This is a conservative bound,
not the tightest observed error. Box faces and lines are exact with bound zero.

The wall is the vertical extrusion of each chord, split into two planar
triangles. Height interpolation adds no curvature, so the same radial sagitta
bound holds everywhere on every wall facet. The same polygon is consumed by
both caps, so there is no boundary mismatch. No additional height refinement
is required for this error contract; better aspect ratio is separate work.

For disk caps, the inscribed polygon misses a circular strip of width at most
δ. For plate caps, square-minus-inscribed-polygon includes thin slivers inside
the exact circular hole, each within δ of the exact rim. Both directions of
surface Hausdorff distance for the complete closed body boundary are bounded
by δ: all planar differences lie in those strips, and all curved differences
lie between each chord and its arc. This is an approximation of the complete
trimmed boundary, not a promise that every triangle interior is exactly material
IN. Claiming exact trim inclusion of straight chords against a curved hole
would be false. All mesh vertices still lie on their declared exact face domains.

## Complete triangulation recipes

Write N=4n for the common ring vertex count. Every triangle must have three
distinct physical IDs and nonzero exact cross product.

**Box.** Split each of the six rectangles into two triangles. Resolve all four
corners through original vertex/coedge identity; all six faces share the same
eight physical vertices. Choose either deterministic diagonal and orient it
against the original face's oriented normal.

**Cylinder.** On each cap, add its exact planar center and fan to all N directed
rim intervals. On each wall interval connect its bottom and top shared samples
as a vertical quadrilateral, split by one deterministic diagonal. Reverse cap
and wall triangles where original outward orientation requires it. This gives
two complete disk domains and the complete closed wall, including the seam.

**Plate.** Each outer side rectangle uses its two triangles. The hole wall uses
the same extruded ring strips, oriented inward toward the void. Each cap has
four corner fans and four side triangles:

1. For each circle quadrant, take the corresponding square corner (NE for the
   east→north arc, NW for north→west, and so on). Join that corner to every
   consecutive pair of that quarter's shared rim samples.
2. For each square side, join its two original corner vertices to the circle's
   cardinal point nearest that side: north for top, east for right, and so on.

These triangles cover the square minus its inscribed circular polygon exactly.
The four corner fans fill the arc-facing bands; the four side triangles fill
the remaining straight-sided bands. Their common spokes agree, no triangle
fills the polygonal hole, and no original outer square edge needs subdivision.
Because half-width a=3/4 is greater than r=1/4, every square corner lies on the
exterior side of every chord in its corresponding quadrant. Fan orientations
are strict and visibility does not cross the polygon. This avoids radial
projection to square corners: a true 45-degree circle point has irrational
coordinates and generally is not a dyadic rational-quarter sample.

For all planar faces, orient triangles so their exact cross product has positive
dot with `Face.orientation*(U×V)` from original chart controls. On cylinder wall
the dot with the outward radial vector is positive; on plate hole wall it is
negative. Positive frame scale preserves those signs. UV winding alone is not
outward orientation on the bottom cap or inner wall. Preserve original face
and chart provenance for every triangle corner.

| Body | Vertices | Triangles | Undirected mesh edges | Euler characteristic |
| --- | ---: | ---: | ---: | ---: |
| Box | 8 | 12 | 18 | 2 |
| Cylinder | 2N+2 | 4N | 6N | 2 |
| Plate | 2N+8 | 4N+16 | 6N+24 | 0 |

These counts supplement complete topology, not replace it. All meshes have one
connected closed shell; the plate has a through-hole and genus one. Each
undirected mesh edge has exactly two opposite directed triangle uses. A physical
seam segment's two uses belong to the same original wall face, whereas rim
segments join cap and wall. Expected signed enclosed volume is positive.
The polygon-cylinder volume is below the analytic cylinder; the plate volume
is above the analytic plate because the polygon hole is inscribed. Scalar
volume agreement alone cannot qualify a mesh.

## Direct qualification and next scope

Independent truth should reconstruct the expected source boundary intervals,
per-face polygonal domains and exact orientation from original data, rather than
run the mesher again. Check complete coverage/no overlap in planar fan domains,
the full wall strip parameter partition, every original coedge chain and seam
occurrence, source point/UV correspondence, positive triangle area, all opposite
edge uses and closed vertex links. Check connectedness, topology and volume as
separate consequences. Test all three bodies, coarse and refined requests,
frame extremes, exact boundary tolerances just below/equal/above a dyadic
threshold, atomic over-limit failure and malformed tolerance input. A plot or
finite deviation samples cannot prove the analytic maximum bound.

Actual callers should read complete triangles, corner charts/UVs, physical-edge
chains, shared adjacency and bounds; retain full outputs for review. Record
construction, consumption, independent qualification and export separately.
Community OCCT mesh cases and external output checks remain alpha release work
under `docs/ALPHA_QUALIFICATION.md`; this narrow step does not make that gate
green. General trimmed NURBS, arbitrary holes, arrangement-region meshing,
adaptive triangle quality, nonuniform transforms, tolerance healing, float-export
rounding bounds and GPU construction are later units. They must not inflate
this complete bounded construction into an uncertified general mesher.
