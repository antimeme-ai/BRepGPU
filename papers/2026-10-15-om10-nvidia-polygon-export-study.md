# Actual NVIDIA polygon export boundary and bounded continuation

The genuine OM10 CPU preparation runs both declared healing policies against
the unchanged no-processing original source. Both policies produce 102 complete
definition meshes and five producer failures. This study separates two official
bridge failures from three failures at our triangle-only export boundary.
Actual run facts are in the [component journal](../journal/2026-10-15-om10-actual-nvidia-geometry.md).
No image, complete reference model or full source-distance PASS exists.

## Exact reference revision and interfaces read

NVIDIA USD/BRep v1.0.0 source revision
[`c9c5979f04cea8c38213004d4f04df0dcfb40d05`](https://github.com/NVIDIA-Omniverse/usd-brep/tree/c9c5979f04cea8c38213004d4f04df0dcfb40d05),
matched to the already isolated official Linux runtime:

- [`SmPoly.h`](https://github.com/NVIDIA-Omniverse/usd-brep/blob/c9c5979f04cea8c38213004d4f04df0dcfb40d05/source/SMLib/inc/SmPoly.h):
  directed polygon topology, physical vertices, corner data, original face labels,
  user fields and triangulation entry points.
- [`SmPoly.cpp`](https://github.com/NVIDIA-Omniverse/usd-brep/blob/c9c5979f04cea8c38213004d4f04df0dcfb40d05/source/SMLib/src/SmPoly.cpp):
  `TriangulatePlanar`, `TriangulateNonPlanar`, `MakeManifoldEdge`,
  `IsConvexCorner`, and the SDK STL export path.
- [`SmOwningTopology.h`](https://github.com/NVIDIA-Omniverse/usd-brep/blob/c9c5979f04cea8c38213004d4f04df0dcfb40d05/source/SMLib/inc/SmOwningTopology.h):
  `GetAll` walks the actual circular linked list without sorting.
- [`SmuTessellate.cpp`](https://github.com/NVIDIA-Omniverse/usd-brep/blob/c9c5979f04cea8c38213004d4f04df0dcfb40d05/source/BREP_SM_USD/src/SmuTessellate.cpp):
  official USD mesh extraction preserves arbitrary outer polygon counts, emits
  float positions/normals, and ignores inner loops. It is not a lossless f64
  triangle extraction alternative for this comparison.

Searching all installed public headers finds `SmPolyFace::TriangulatePlanar`
and `TriangulateNonPlanar`, plus the source-aware `SmTess` family. There is no
triangle-only switch among `SmTessellationParams` quality controls.

## Actual returned rings

A private inspector imports the real retained USD files under the same healing
policy and requested 0.005 mm chord height, both angles zero, strict partial
output disabled. It reads SDK f64 points, corner normals, physical keys and
directed loops without modifying topology or producing replacement geometry.

| Definition | Policy | Triangles | Other rings |
| --- | --- | ---: | --- |
| 1808 | 0 and 1 | 17,620 | Six four-edge rings |
| 1756 | 0 | 100 | Two 46-edge rings |
| 1756 | 1 | 98 | Two 41-edge rings |
| 1797 | 0 | 5,335 | One 55-edge ring |
| 1797 | 1 | 314 | Two seven-edge rings |

All these nontriangles have exactly one loop, distinct physical keys and
nonzero actual corner normals. The six quads in definition 1808 are exactly
planar, simple and strictly convex for the retained binary64 coordinates. Each
is an axis-extruded rectangle: two opposite edges change only Y, and the other
two lie at fixed distinct Y values with identical X/Z endpoint pairs.

The spring/spiral rings are close to a plane (largest observed residual
1.54e-14 mm), but their dominant-plane projections have proper crossings:

| Definition/policy | Polygon ordinals | Proper crossing pairs |
| --- | --- | --- |
| 1756/0 | 44, 97 | 341, 337 |
| 1756/1 | 44, 95 | 319, 315 |
| 1797/0 | 2694 | 96 |
| 1797/1 | 82, 174 | 1, 1 |

These predicates operate on exact rational representations of the original
binary64 coordinates using the existing independent segment mechanism. The
planes are numerical diagnostics; do not call the projected crossings exact
intersection of slightly skew 3D segments.

The first 1756/0 crossing lies 0.539983 mm from its nearest edge endpoint;
the smallest pairwise separation of its four endpoints is 0.783235 mm. The
first 1797/0 crossing lies 0.029104 mm from its nearest endpoint. These are
not sub-tolerance seam touches. The raw ring has no repeated XYZ values under
different physical keys. SDK report 1797/heal=1 also has four spine edges and
`output_manifold=0`; successful triangle export would not make that shell pass.

The stronger read-only inspector checks actual loop closure flags, exact
edge→loop/face ownership, CCW/CW links and reversed radial endpoint identities.
This matters because `GetEndPolyVertex` itself derives the endpoint from the
next linked edge; that check alone would not independently establish ordering.
There is no `GetVertexEnd` API on `SmPolyEdge` in this revision. `GetPolyEdges`
is explicitly the directed polygon walk, not an unordered vertex bag.
Complete private rows, original f64 corners and first crossing XYZ/keys are in
`context/om10-actual-nvidia-polygon-inspection-v2`.

## Official triangulator contracts

`TriangulatePlanar` supports complex planar faces and holes but documents a
non-self-intersecting precondition. It mutates the original polygon and creates
children. The output list excludes the triangle remaining in the original
object; therefore final extraction must re-enumerate the complete PolyBrep.
The SDK STL exporter demonstrates that sequence, but silently continues some
failures and omits nontriangles; those behaviors are unsuitable here.

`MakeManifoldEdge` copies `m_pOriginalFace` and public `m_lUserLong1/2` into
new faces. It creates diagonal edges without assigning original corner
`AuxData`. Existing outgoing-edge normal extraction cannot simply run after
triangulation. Snapshot actual normals per original polygon and physical vertex,
retain parent ancestry, and emit those same values for new diagonal corners.
Different parents may legitimately retain different normals at the same vertex.

Another hazard: `IsConvexCorner` reads `GetOriginalFace()->GetSurface()` and
interprets raw X/Y as UV without checking `GetOKBackPtrs`. Returned meshes are
already in XYZ. A raw-space triangulation call must temporarily detach the
candidate polygon's original face label, retaining its exact parent label
separately and restoring child labels from parent ancestry afterward. Merely
toggling the PolyBrep back-pointer flag does not disable that branch.

`TriangulateNonPlanar` assumes a reasonably flat, regularly shaped single outer
loop, may return success without editing faces with holes, and explicitly says
spiral shapes may not work. It is not a route around the crossed-ring precondition.

## Proposed narrow reference capability

Admit only SDK polygons already triangulated, plus single-loop four-edge
axis-extruded rectangles established by exact coordinate equalities. This admits
the six observed date-disc quads without introducing a floating planarity
tolerance, general triangulator, vertex movement, ring reordering or quality
change. Invoke only the official `TriangulatePlanar`; own no display triangles.

Admission is symmetric across all three coordinate axes. In the actual directed
cyclic loop, after a possible cyclic shift, require finite A/B/C/D and four
distinct physical identities. For one axis k: A[k]=D[k], B[k]=C[k],
A[k]!=B[k]; on both other coordinates A=B and C=D, with at least one other
coordinate A!=D. These equalities establish a nonzero extrusion perpendicular
to a nonzero cross-section and an alternating rectangular boundary. A coordinate
multiset or bow-tie ordering is insufficient. No epsilon is used.

Before mutation, snapshot all original physical vertex pointers/ordinal keys and
exact XYZ, every original polygon's live source-face label, directed boundary
and actual per-corner normals. Reject conflicting normals within one parent.
Tag parent polygon ordinals in the public user field; the official split copies
that ancestry into children. Temporarily clear only admitted rectangle face
labels to request raw-space geometry, then perform the official operation.

After mutation, re-enumerate all polygons and vertices. Require unchanged
physical vertex identity/XYZ, triangular single loops, exact retained parent
tag and source-face label, and unchanged original directed boundary. Each
rectangle must yield two noncollapsed, consistently directed triangles with
the same four boundary edges and one opposite shared diagonal. Recover normals
from the same original parent+physical vertex; do not calculate replacements.
Restore parent labels and temporary user data. Any error rejects the complete
definition; publish no partial mesh.

Keep a final polygon-pointer→parent index map before restoring temporary user
fields; extraction reads that retained ancestry map. Unknown/missing parent tags
are hard errors and are never reconstructed from coordinates or source labels.
An RAII guard restores original source labels and parent user-field values for
every valid tagged polygon it can re-enumerate on success and ordinary failure.
If SDK mutation fails, throws or leaves unknown ancestry, reject the complete
definition and let the existing caller destroy that PolyBrep before proceeding
to another definition; never reuse a partly mutated object. The helper does not
delete the mesh or source Brep. Original source faces stay alive throughout.

## Native fixture and implementation seam

Factor the existing `Corner`, `Triangle`, `Mesh` and extraction routine into
`native/om10_nvidia/raw_mesh.hpp/.cpp`. Both the existing producer and a small
native fixture call that exact extraction routine and link the already selected
SDK. Keep main's CLI, binary writer and BGREFM01 layout unchanged. Add only the
new source helper files to the NVIDIA caller's source pins; the owned rendering
caller and its active qualification pins remain untouched.

The literal SDK fixture constructs rectangular polygon input and source-face
labels through the existing SDK API. Authored normal bytes are explicitly test
inputs. It checks literal physical coordinates/keys, directed boundary, exactly
two triangles per rectangle, the opposite shared diagonal and unchanged normals.
Exercise all three extrusion axes, cyclic shifts and reversed input loops.
A rectangle and an untouched sibling triangle share a physical vertex but have
deliberately different literal parent-corner normals and original user fields.
Check byte-exact normal preservation without normalization/recomputation, the
untouched triangle's original boundary, and restoration of each parent's own
user data. This detects an incorrect global-vertex normal map and tag leakage.
A crossed loop and a finite nonrectangle reject. This fixture
tests the extraction/SDK-mutation boundary; it is not original CAD truth or a
replacement for actual source/physical admission.

The unchanged actual date-disc producer failure is the real-model RED. Native
fixture RED must be observed before implementation. The native helper then has
one focused GREEN run, affected producer checks, bounded ownership sanitizers
and independent reviews before actual date-disc reruns under both policies.

This proposed unit should turn definition 1808 into a complete genuine SDK mesh;
1756/1797 remain explicit unsupported export outcomes because the official
planar triangulator precondition fails. The two bridge failures remain separate.
Final actual reference reports must name official polygon triangulation and its
time; do not imply these extra triangles were returned directly by the first
`SmApiTessellate` call. This design has not authorized implementation.
