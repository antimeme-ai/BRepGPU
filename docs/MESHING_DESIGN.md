# Complete canonical trim-aware mesh

Design for issue32, 2026-10-15. This is a CPU exact construction beachhead over
the existing immutable box, cylinder and one-hole-plate snapshots. General
trim triangulation, adaptive rational meshing and GPU placement are later units.
No external dependency or native ABI is selected.

## Operation and approximation contract

`tessellate(body, *, tolerance, max_quarter_segments=256)` returns a complete
immutable indexed triangle mesh bound to that actual BodySnapshot. A lazy
`body.tessellate(...)` adapter exposes the same operation. Accept factory body
snapshots only. Tolerance is a positive finite int/Fraction/binary64 value;
bools and other types fail. Preserve the exact submitted value. The segment
budget is a power-of-two integer in1..1024, excluding bool. Invalid requests
raise before construction; insufficient subdivision or coordinate-rounding
budget raises MeshBudgetError and publishes no partial output.

Box planes are exact. Circle boundaries become inscribed straight polygons.
Cylinder caps omit a thin circular rim; plate caps include a thin strip in the
analytic hole. This operation completely covers the stated chordal domains and
preserves hole topology. It does not claim every triangle lies in the exact
analytic trim. The two-sided Hausdorff distance between each approximated face
and its analytic source, and between each circle polygon and source ring, is
bounded in world units by tolerance. The same bound controls the complete shell.

Exact rational vertices define the geometric construction. Also expose one
shared binary64 XYZ per physical mesh vertex for ordinary mesh consumers. Bound
the actual per-vertex rounding displacement using exact L1 differences (which
bound Euclidean displacement). Convex barycentric interpolation bounds every
triangle's displacement by the maximum vertex bound. Let curve_bound be the
analytic approximation bound and rounding_bound the largest actual L1 vertex
error. Require curve_bound+rounding_bound<=tolerance. Do not infer that rounded
triangles retain exact incidence geometry or embedding merely from this error
bound; qualify the actual exported rounded shell separately.

## Subdivision and bound

For circle radius R in the common world frame, quarter parameter s maps to
((1-s²)/(1+s²),2s/(1+s²)); angle speed is2/(1+s²)<=2. Uniform quarter steps1/n
have angle<=2/n, hence sagitta<=R/(2n²). Choose the first dyadic n within the
budget for which that rational bound<=tolerance/2, reserving the remainder for
actual output rounding. Circle radius and source frame scale remain exact.
Box has n=0 and curve_bound=0. No trig, rounded UV classification or sampled
midpoint deviation selects refinement. Even very small tolerances fail visibly
when a finite subdivision/rounding budget cannot meet them.

## One physical vertex, many occurrences

All returned values are recursively immutable. The mesh retains the submitted
snapshot, requested tolerance, n, curve/rounding/total bounds, vertices, triangles,
edge polylines and boundary uses. IDs for new mesh vertices/triangles are dense
zero-based indices; original source IDs remain their one-based typed IDs.

MeshVertex stores id, exact world XYZ, rounded world XYZ and a typed source:
`('vertex', original_vertex_id)`, `('edge', original_edge_id, exact_t)`, or
`('face', original_face_id, 'center')`. Namespace comes from mesh.body's actual
snapshot/body_id/revision. No coordinate hashing or tolerance welding establishes
identity. Begin with original vertices in original order. Add each physical
edge's samples once, then disk-center interiors. Closed circle t=0/t=1 share
one original vertex index; their parameter occurrences remain distinct.

MeshTriangle stores id, three shared vertex indices, original face_id and three
MeshCorner records. Each corner names original chart_id and exact chart-local
UV, even when a seam/join has another occurrence at the same vertex. Wall chart
q uses local s in[0,1],v in[0,1]; q joins are not invented B-rep edges. Normals
remain per-face/corner in later consumers, never one smoothed normal per vertex.

EdgePolyline stores original edge_id, canonical increasing original parameters
and shared vertex indices including repeated closure. BoundaryUse stores each
original coedge_id, face_id, edge_id, orientation, directed polyline indices,
directed original parameters and exact original UV-map values. Wall boundary
UVs are lifted atlas a in[0,4],v in[0,1], explicitly distinct from chart-local
corner UVs. Seam a=0 and a=4 remain separate coedge occurrences of one shared
physical line. Original coedge orientation reverses traversal exactly once.

Concrete frozen record fields (tuples throughout):

```
MeshVertex(id, position, rounded_position, source)
MeshCorner(chart_id, uv)
MeshTriangle(id, vertices, face_id, corners)
EdgePolyline(edge_id, parameters, vertices)
BoundaryUse(coedge_id, face_id, edge_id, orientation, parameters, vertices, uv)
Mesh(body, tolerance, quarter_segments, curve_bound, rounding_bound, error_bound,
     vertices, triangles, edge_polylines, boundary_uses)
```

Mesh itself is factory-only; records retain recursively immutable native values.
The qualifier reads these values directly and never reruns the mesher.

## Construction from original records

Read exact Fraction ratios of original source vertex, curve, UV-map and planar
control records. Existing Curve.evaluate/UVMap.evaluate coerce to float; do not
use them for exact boundary construction. The small owned exact line/circle
and lifted-map readers consume those original records directly. World mapping
is exact origin+scale*local. Plane UV inversion uses original affine chart
controls; circle wall UVs come from the original ring parameter and chart slot.

Allocate every original vertex once. Lines use their original endpoint vertex
indices. Each circle edge uses4n segments with exact t=k/(4n), sharing the
original closed anchor. Build all edge polylines and every original coedge use
before faces. Bind all face triangles to those physical indices.

- Box and plate outer rectangular sides: two triangles per original oriented
  quadrilateral loop. All original outer physical edges remain unsplit.
- Cylinder caps: one source-plane center, fan to every consecutive ring pair;
  orient top/bottom by the original face orientation.
- Plate caps: each original outer square corner fans to all n chords of its
  corresponding circle quadrant. Add four side triangles, each joining an
  original outer square edge's two endpoints to the corresponding circle cardinal
  point. This covers the square minus the inscribed polygon without irrational
  corner rays, extra outer-edge knots or a general triangulation algorithm.
- Cylindrical wall: connect corresponding bottom/top ring pairs into quads,
  split each by the same diagonal, attach exact quarter-local UVs and apply the
  source face orientation (inward for the hole). The physical seam closes on
  shared original vertex indices while retaining both atlas occurrences.

Every emitted triangle must have distinct indices and nonzero exact area. The
factory-only input family bounds these templates; they are not a general mesh
algorithm for arbitrary faces. With N=4n, expected topologies are box V8/T12,
cylinder V2N+2/T4N (Euler2), plate V2N+8/T4N+16 (Euler0).

## Independent qualification

Use the existing body oracle once to validate the canonical source/control
contract, then an owned independent mesh qualifier. It must not call product
meshing/refinement/readers to compute expected geometry or coverage.

Check every vertex's exact source identity/parameter, actual rounded coordinates,
all original edges/coedges/loops and source face/chart/corner correspondence.
Require the full ordered physical edge coverage, both opposite coedge uses,
closed anchors, seam multiplicity and no invented body edges at chart joins.
Check every actual mesh edge has exactly two opposite triangle uses, every
vertex link is one cycle, and the shell is connected. Expected genus and
analytic volume brackets are supporting facts, not complete coverage oracles.

For each planar face, independently reconstruct its complete chordal-domain
boundary from original records. Require every projected triangle strictly
positive in the face orientation. Sum the FULL directed triangle-edge integer
multiset, preserving multiplicity; all internal edges cancel and the remaining
chain must equal the complete source outer CCW/hole CW polygon word. Winding
additivity then proves coverage multiplicity1 inside and0 outside: no holes,
extra pieces, overlap or missing triangles. This is a direct whole-domain
argument, not an area/centroid/Euler check. The independent canonical boundary
polygon must be simple, not merely a chain supplied by the product.

For each wall chord strip, use exact (lambda,z), where lambda is the affine
coordinate along its actual XY chord; source rational s is not affine in XY.
The same positive-chain argument covers the whole ruled rectangle. Exact
circle endpoint order/minor-arc constraints and the exact dot-product sagitta
inequality independently bound each original arc, rather than repeating the
product's R/(2n²) refinement formula. Check face orientation geometrically,
including the inner wall. Check the actual rounded exported triangles' signs,
shared indices and nondegeneracy against the preserved exact correspondence;
record rounding separately from source approximation. In raw planar UV, require
triangle sign=Face.orientation and compare directed boundary words in that SAME
raw UV convention. Equivalently reflect V for negative faces to make all tests
CCW; do not mix these conventions. For walls use the corresponding source
oriented (lambda,z) convention.

The rounded shell needs its own bounded embedding argument: rounded circle rings
must remain strictly convex simple CCW polygons, their top/bottom XY values must
agree exactly and cap heights must be strictly ordered. Plate rings remain
strictly inside the rounded square. Planar cap/side vertices must stay on their
expected common planes. Repeat the positive boundary-chain coverage check on
actual rounded planar domains and each actual rounded wall chord strip. These
canonical extruded domains then intersect only along their expected common
boundaries; a positive triangle sign alone does not establish this fact.

Direct cases span all three shapes, every admitted frame scale, translated and
subnormal-origin frames, coarse/fine tolerance thresholds and budget rejection.
Include closed ring endpoints, cap holes, every wall quadrant/seam, duplicate/
missing/folded triangles and same-coordinate distinct-index faults. Selected
fault injections only show those assertions detect named faults; they do not
replace these geometric oracles or external qualification.

## Delivery and measurement

Research, design/plan review, direct fleet red tests, implementation, fleet green
and independent code review precede publication. Product execution and injected
faults run only on the fleet. Keep protected native libraries unchanged.

The actual caller writes a triangle PLY, original body/controls and source maps,
vertices/triangles/corners/polylines/coedges with exact ratios, submitted tolerance,
error bounds, full qualification and a readable SVG mesh view. Publish a plate
mesh showing an actual through-hole, rather than another point-grid rendering.
Measure complete CPU construction, returned indexed-mesh traversal and rounded
export consumption; source preparation, independent qualification and filesystem
export are separate phases. Use serial cases and bounded first/warmup/repeated
calls with medians/ranges. This CPU placement has no RAM/VRAM transfer, CUDA
kernel or GPU speedup claim. Record future native/GPU bulk construction and
occurrence sampling in return-pass notes without starting that tuning lane.

Applicable external community meshing qualification remains incomplete. Link
actual inspected OCCT meshing assertions/cases and their missing adapter/data
status. These canonical analytic models do not satisfy the alpha corpus gate.
