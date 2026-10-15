# Exact canonical-body meshing: runtime and source ownership

2026-10-15. Recommend a complete **CPU exact mesh construction** beachhead for
the existing factory box/cylinder/one-hole plate. Keep GPU evaluation as later
optional consumption, with an explicit parameter-rounding budget. No new ABI,
dependency, retained session or general triangulator is needed. This is source
and primary-reference study only; no build or product execution occurred.

The [source catalog](meshing-runtime-sources.tsv) pins actual current body,
sampling and native-wrapper sources and six read OCCT8.0.1 reference files at
commit `b8f597c677811d1f9f4d8a97f5ae2825c0353a42`. The existing quarantine manifest
already preserves that archive and restoration recipe. No reference was adopted.

## What the existing runtime provides

`BodySnapshot` owns immutable vertices, curves, edges, coedges, loops, faces and
charts. Face/chart lineage is already typed by original body/revision/entity
kind/face/chart. Cylinder and plate hole walls are one face each with four
quarter charts; quarter joins have no body edge. The physical vertical seam has
two coedge occurrences, with lifted atlas u=0 and u=4. Each rim is one closed
physical edge, with a single original anchor vertex and four geometry spans.

`BodySampler` owns a PatchSet inside a borrowed Engine, copies a bounded request,
classifies actual submitted binary64 UV values on CPU, uploads admitted queries,
evaluates first jets, downloads/status/provenance-checks, converts frames,
computes per-face oriented normals and scatters owned rows. It intentionally
rejects rounded UV points inside the plate hole. No point in that implementation
already builds indexed trimmed triangles or makes a chord claim.

Consequently, sampling a grid then discarding triangles with rejected corners
would not implement complete trimmed meshing. It misses boundary strips and
does not produce a conforming shared rim. The plate cap's exact circular UV
coordinates contain a factor1/3 and are not representable in binary64; they
must not be falsely relabelled as exact native boundary UVs.

## Construction and output ownership

One owned CPU operation should consume the original snapshot plus a positive
exact world deflection (int excluding bool, Fraction, or finite float converted
before arithmetic). It publishes recursively immutable exact world vertices,
oriented triangle indices, per-triangle face identity, per-corner original chart
and rational UV, physical EdgePolylines and per-coedge BoundaryUses. Bind the
exact snapshot object and original typed IDs; another body/frame/revision must
not reuse dense slots accidentally.

Every global mesh vertex has a semantic generator: original vertex ID, physical
edge ID plus canonical exact parameter, or original face ID plus an interior
generator. Resolve a physical edge endpoint to its original vertex generator.
For closed circles, t=0 and1 resolve to the same vertex while both endpoint
parameter occurrences remain in the polyline. Never merge by rounded XYZ.
Face-chart/coedge occurrences map to that global vertex and remain separate:
sharp corners need separate face normals; the seam has two distinct source uses;
chart joins are mesh adjacency rather than invented B-rep features.

State whether corner UV is chart-local or lifted atlas UV. The natural triangle
corner contract is chart-local rational UV with original chart ID; a wall
BoundaryUse retains original lifted pcurve UV, including0 versus4. Original
coedge sign reverses traversal once; evaluate its map at the original canonical
curve parameter without applying the orientation a second time.

Important existing-source detail: `Curve.evaluate`, `UVMap.evaluate` and
`BodySnapshot.edge_point` coerce parameters to float and return rounded local
values. The exact constructor must read the original curve/pcurve data and
evaluate its rational recipe before applying Fraction(origin)+Fraction(scale)*p.
Wrapping those existing floating outputs in Fraction would preserve a rounding
error, not restore the source geometry.

## Complete narrow triangulations

Box: original eight global vertices and two triangles per rectangular face.
No refinement is necessary for geometric deflection on planes.

Let n be the common power-of-two subdivisions per original circle quarter and
N=4n. Reuse those exact physical ring parameters on all incident faces.
Cylinder: N triangles fan from each cap's typed face-interior center;2N wall
triangles join corresponding top/bottom ring segments. No axial subdivision
is needed for cylinder distance error: the surface is a straight extrusion and
each pair of triangles covers its planar chord strip completely. Counts are
V=2N+2,F=4N,E=6N, hence Euler2.

Plate: each cap has four quarter-circle chord fans, one from each ORIGINAL
outer square corner, plus four triangles joining each outer side's two corners
to its facing inner cardinal point. This avoids added outer-edge midpoints or
irrational radial intersections. A quarter's corner sees its whole inner arc:
with outer halfwidth a and inner radius r, C·P>=a*r>r². Original four side faces
keep two triangles each; inward-oriented hole wall uses the same2N strip
triangles. Thus V=2N+8,F=4N+16,E=6N+24 and Euler0. Orientation follows the original
face/chart sign, including bottom and inward hole wall. Counts alone are not
the geometry/completeness oracle: match all expected sector/side triangle domains
and shared physical edge chains independently.

## Geometric error contract

An owned derivation supplies a simple exact conservative bound. On a rational
quarter, theta(t)=2atan(t), so |theta′|<=2. A parameter step1/n has angle<=2/n;
using1−cos(x)<=x²/2, its world-radius-R sagitta is at most **R/(2n²)**.
The constructor only evaluates the final rational inequality; no trig or square
root is needed. Radius comes from the original circle record and exact frame.
Find the smallest admitted power-of-two n satisfying the world tolerance, with
an explicit maximum level and checked output-count formulas BEFORE allocating
point/triangle arrays. Reject an over-fine request wholly, rather than cap its
resolution and silently miss tolerance. Box deflection is exactly0.

Chord strips bound the entire cylindrical wall, not only sampled points. The
cap fans cover the disk polygon or rectangle-minus-inner-polygon completely;
the difference from the true trimmed face is bounded by the same sagitta. In
particular, inscribed hole chords lie inside the removed analytic disk: some
plate-cap triangle points lie in that narrow analytic-hole strip. The supported
claim is a **two-sided Hausdorff approximation of the trimmed face**, not that
every triangle point belongs to the exact trim. Topology remains a complete
closed shell with a preserved through hole. State this distinction explicitly.

Keep the exact Fraction mesh as the certified output. A separate float export
must account for its own displacement. With one shared rounded coordinate per
global vertex, let delta be the maximum exact L1 displacement from its Fraction
position. Every point of a triangle moves by at most delta through barycentric
interpolation, so the exported geometric bound is construction_bound+delta.
Report this sum; to retain a requested total epsilon, reserve separate budgets
up front or refuse a failed export budget. Check actual float triangles for
collapse/orientation loss rather than silently omitting them. No crack can be
introduced by independently rounding duplicate face corners when each triangle
references the same global float vertex array.

## References translated into owned choices

The official OCCT meshing guide separates boundary linear/angular deflection
from interior-face deflection, and describes edge-first discretization followed
by face work. That separation supports an explicit boundary/surface error
contract rather than a point-cloud visual claim.
[Official mesh guide](https://occt3d.com/dev/doc/overview/html/occt_user_guides__mesh.html).

Actual `BRepMesh_EdgeDiscret` discretizes a shared3D edge, then maps its parameter
list to each pcurve; it uses original topological endpoints rather than letting
incident faces independently approximate their endpoints. `BRepMeshData_PCurve`
retains face/orientation and UV/parameter/node-index arrays. We should own the
same important separation of physical vertices and face occurrences, without
copying its mutable pointer graph or adopting its implementation.
[Pinned edge implementation](https://github.com/Open-Cascade-SAS/OCCT/blob/b8f597c677811d1f9f4d8a97f5ae2825c0353a42/src/ModelingAlgorithms/TKMesh/BRepMesh/BRepMesh_EdgeDiscret.cxx),
[pcurve record](https://github.com/Open-Cascade-SAS/OCCT/blob/b8f597c677811d1f9f4d8a97f5ae2825c0353a42/src/ModelingAlgorithms/TKMesh/BRepMeshData/BRepMeshData_PCurve.hxx).

`BRepMesh_ModelPostProcessor` explicitly handles two pcurves on one face when
committing edge polygons. This is the reference case for our seam's two
occurrences; reducing by face ID alone would lose one.
[Pinned postprocessor](https://github.com/Open-Cascade-SAS/OCCT/blob/b8f597c677811d1f9f4d8a97f5ae2825c0353a42/src/ModelingAlgorithms/TKMesh/BRepMesh/BRepMesh_ModelPostProcessor.cxx).

Its cylinder splitter derives angular sampling from radius and deflection,
while axial node creation is intentionally absent in that implementation's
current interior loops. Our exact analytic extrusion proof is sufficient for
the narrower factory cylinders and avoids a generic surface triangulator.
[Pinned cylinder splitter](https://github.com/Open-Cascade-SAS/OCCT/blob/b8f597c677811d1f9f4d8a97f5ae2825c0353a42/src/ModelingAlgorithms/TKMesh/BRepMesh/BRepMesh_CylinderRangeSplitter.cxx).

## Actual caller, errors and deferred GPU placement

Benchmark the complete new CPU operation first: original body preparation,
tolerance admission/count planning, original source validation, exact edge/UV
construction, complete topology/corner publication, actual indexed triangle/
boundary reader, budgeted float conversion and actual export. Attribute those
phases without adding independent medians to infer a total. Also time complete
construction-plus-reader and actual export-inclusive wall directly. Qualification
after each output is separately named and timed. Small/fine tolerances on all
three bodies and translated/scaled frames should change actual polygon counts,
not merely repeat an easy unqualified triangle loop. Retain original records,
tolerance/bound, counts, full graph/source data, exact rational output and
independent geometry/topology results. No CPU/GPU speedup comparison exists for
an exact CPU constructor that does not invoke a GPU operation.

An allocation, source mismatch, unsupported input, resource cap, iterator or
arithmetic failure aborts the whole request and leaves original snapshots and
previous outputs intact. No partial success, dropped face or silent coarse
fallback. Do not add stateful native resources solely for this constructor.

Later, an actual geometry consumer can batch its chart occurrences through the
existing Engine first-jet ABI. It must keep exact UV, submitted binary64 UV,
actual slot/lineage and observed source displacement separate. Current
BodySampler cannot blindly mask those rounded plate-hole boundary requests.
One deterministic representative may supply each global sampled position after
all occurrences pass correspondence budgets; normals remain per face/corner.
Report setup/context, patch upload, packing/query upload, evaluation/completion,
jet/status/source download, conversion, normal computation, global-index gather,
output publication, child/context cleanup and cold/reused complete wall. CUDA
logical query transfer is20 bytes/occurrence and first-jet/status output76 bytes,
plus actual source buffers/host copies and geometry upload; no network transfer
is implied. That is a return-pass integration/placement study, not an extra
implementation lane now. External community mesh checks and original CAD-mesh
comparison remain part of advertised-scope pre-alpha qualification.
