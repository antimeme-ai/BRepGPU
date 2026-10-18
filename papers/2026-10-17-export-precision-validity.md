# Export precision and reader-equivalent validity

Research lane M4-G3 of the fast-mesher campaign, 2026-10-17. Brief:
[docs/fast-mesher-campaign/briefs/M4-G3.md](../docs/fast-mesher-campaign/briefs/M4-G3.md)
with [M4-common.md](../docs/fast-mesher-campaign/briefs/M4-common.md). Stub:
[lifecycle map §4.1 G3](2026-10-17-geometry-lifecycle-map.md). Bead
`brepgpu-12s.4`. Sources, revisions and read depth are in
[2026-10-17-export-precision-validity-sources.tsv](2026-10-17-export-precision-validity-sources.tsv).

The lane read the consumers' import code at pinned tags, read the snap-rounding
literature and CGAL's implementation, and ran one measurement on the fleet. The
measurement meshed every unique STEP file in `corpus/` with OCCT 8.0.1
`BRepMesh_IncrementalMesh` as a **stand-in producer**. BRepMesh is not our
product and its meshes are not a statement about ours. The probe welded each
solid by B-rep identity, rounded it the way each writer and reader does, rebuilt
what each consumer holds, and checked that against the FP64 mesh with exact
predicates. PrusaSlicer's STL import ran as PrusaSlicer's own repair code.

Tags: **[src]** a source establishes it (file:line or section); **[meas]**
measured here; **[own]** this lane's derivation; **[inf]** inferred from a source
but not stated by it.

## 0. Answer

1. **Plain float32 rounding breaks a few percent of valid solids, and almost
   all of those break because the FP64 mesh was valid only by roundoff.** On the
   clean items of a stand-in producer (OCCT BRepMesh: 5,608 solids in 444 parts
   at 0.025 mm; 5,442 in 428 at 0.005 mm), index-preserving float32 in the
   file's own frame gives a new degenerate, flipped or self-intersecting
   element, or lost manifoldness, in **3.4% / 2.4% of items (5.9% / 4.7% of
   parts)**. Centring each item gives **2.6% / 2.1%**. At the file frame, 98.9%
   of flipped triangles had an FP64 altitude under 1 pm, and 93% of new
   intersecting pairs were under 1 nm apart in FP64 (§3.3–3.4).
2. **Far from the origin, real geometry breaks.** Items baked at +10 m fail at
   7.1% / 8.4%; at +100 m, 18.6% / 32.3%, with vertex moves of 6.7 µm, larger
   than ε = 5 µm. Recentring per item removes this, wherever the consumer keeps
   the item transform (three.js, lib3mf, multi-instance PrusaSlicer). It does
   not help PrusaSlicer with a single-instance foreign 3MF, which bakes to world
   float32, or STL, which has no transform (§4.4).
3. **The guarantee survives plain rounding exactly when three local conditions
   hold** [own, §1]: every triangle's altitude > 2δ (C1); every non-adjacent or
   vertex-adjacent pair more than 2δ apart in the sense of CGAL's test (C2); no
   knife-edge fold within 2δ/h⊥ (C3). δ is the largest vertex move, at most
   (√3/2)·ulp(R): ≤ 26 nm for frames within 512 mm. Deviation grows by at most
   δ. Across both tolerances, five placements and five emulations, **no element
   that satisfied the conditions failed** (§3.4).
4. **Recommended remedy, all formats: quantization-aware construction plus an
   offline reader-equivalent oracle. Not snap rounding.** Choose stored
   positions at construction, in the written frame (world frame for 3MF, so
   that PrusaSlicer's bake is covered). Enforce C1 in the quality pass and add
   2δ to the near-contact budget (C2/C3). Weld by identity only. Write exact
   float32 values (shortest decimals in 3MF). Re-check the written bytes per
   consumer offline, at 2.4–5.2 µs per triangle. Snap rounding (Lazard–Valque,
   CGAL `apply_iterative_snap_rounding`) repairs soups that already intersect,
   outputs doubles, re-triangulates and does not guarantee success; its float32
   variant fails 42% of the time in its authors' tests (§4.1).
5. **Re-import procedures, read in code** (§2): PrusaSlicer 2.9.6 (STL repair
   pipeline, run here as its own code), Cura 5.13 (float32 recentring, integer
   micrometres, **30 µm vertex meld that drops faces**), lib3mf 2.5 (decimal →
   double → float; geometric checks none), three.js r186 (float32 as stored,
   float64 node matrices), fTetWild (grid weld at 1e-8·diag; drops faces with
   area/2 ≤ 1e-8·diag).
6. **3MF items (Q4).** Items that overlap MUST be united, just like components;
   consumers MAY move items. Separate items avoid fusing only if they do not
   overlap as read, and float32 can create a 2δ overlap at FP64 contact. Cura
   deliberately overlaps touching meshes by 0.15 mm. This goes to the operator
   (§5, §8 question 1).
7. **Correction.** The lifecycle map's "rounding creates self-intersections in
   about 12%" measures something else: Thingi10K models that already
   self-intersect, whose exact arrangement is rounded to doubles (§7).
8. **Consumer limits no export removes:** Cura's meld removes faces in about
   half of the clean items (1.6–2.8% of triangles). fTetWild's area filter
   removes faces in 27–29% of items. Both act on any input with edges under
   30 µm or slivers, whatever its precision (§3.5).

## 1. What rounding can and cannot do: the bound and three local conditions [own]

Notation. A writer turns each FP64 coordinate x into the nearest float32,
fl(x). Per coordinate, |fl(x) − x| ≤ ½·ulp(x), with ulp(x) = 2^(⌊log₂|x|⌋−23)
for normal numbers. A vertex moves by at most

  δ(R) = (√3/2)·2^(⌊log₂R⌋−23), where R is the largest coordinate magnitude in
  the frame the file is written in.

| R (coordinate magnitude) | ulp | δ(R) | δ / 5 µm | δ / 25 µm |
|---|---|---|---|---|
| < 64 mm | 3.8 nm | 6.6 nm | 0.13% | 0.03% |
| 256–512 mm | 30.5 nm | 26.4 nm | 0.5% | 0.1% |
| 0.5–1 m | 61 nm | 52.9 nm | 1.1% | 0.2% |
| 1–2 m | 122 nm | 106 nm | 2.1% | 0.4% |
| 8–16 m | 977 nm | 846 nm | 17% | 3.4% |
| 64–128 m | 7.81 µm | 6.77 µm | 135% | 27% |

The probe confirms the table: the largest vertex displacement it measured at
each placement is in §3.3.

Four consequences follow from convexity and the definition of the CGAL
intersection test that the oracle uses (§3.1):

1. **Deviation.** A point of a triangle is a convex combination of its vertices,
   so it moves by at most the largest vertex move. The written mesh is within
   ε + δ of the target if the FP64 mesh is within ε. This is the meshing-oracles
   rule (rho added to the geometric error) with rho = δ
   [src `papers/2026-10-15-meshing-oracles-study.md` "If float coordinates are
   exported"].
2. **Orientation (C1).** Measure each triangle's altitude h onto its longest
   edge. Project the rounded triangle onto the FP64 triangle's plane. The apex
   moves by at most δ, and the base line moves by at most δ at the apex's foot
   (the foot lies inside the longest edge). So the signed altitude changes by at
   most 2δ. **If h > 2δ the triangle can neither flip nor degenerate.**
3. **Contact (C2).** Two triangles that share no vertex and are more than 2δ
   apart cannot touch after rounding: every point moves by at most δ. Two
   triangles that share one vertex intersect improperly only if the edge
   opposite the shared vertex of one meets the other (CGAL
   `self_intersections.h:280-297`, and a sector argument: any common point
   other than the shared vertex lies on a ray from it, which leaves each
   triangle through its opposite edge). **If each opposite edge is more than 2δ
   from the other triangle, the pair stays proper.**
4. **Folds (C3).** Two triangles that share an edge intersect improperly only
   when they are coplanar and on the same side of the edge (CGAL
   `self_intersections.h:236-260`). Rounding turns each triangle about the
   shared edge by at most about 2δ/h⊥, where h⊥ is its altitude onto that edge.
   This is first order and valid when h⊥ ≫ δ, which C1 supplies. A fold can
   therefore appear only where the same-side angle θ between the two triangles
   satisfies θ ≤ 2δ/h⊥(f) + 2δ/h⊥(g), a knife edge. The far vertex's distance to
   the other plane is not the right measure. A distance far above 2δ still folds
   when the neighbour is a needle, because the needle's plane swings freely.

C1 also keeps every edge longer than 2δ: an edge is at least as long as the
altitude onto the longest edge, so no two vertices of one triangle can collapse.
C2 does the same for vertices of non-adjacent triangles. C1–C3 together are
sufficient for plain rounding to keep every FP64 guarantee except exact
deviation, which grows by δ. They are local and are the same kind
of clearance the thin-feature lane already budgets: its table reserves "2·u32"
for float32 export if export does not snap-round
[src `papers/2026-10-17-thin-feature-embedding.md` §3.7]. This paper's
recommendation is that it does not snap-round (§4), so 2δ goes into that
budget.

Float32 is relative precision. A coordinate near zero keeps nanometre spacing.
Two consequences: rounding hurts in the coordinate directions where values are
large, and recentring an item about its own centre shrinks R to the item's half
extent. Uniform grids behave differently: CuraEngine's integer micrometres
(§2.2) and the snap-rounding grids in §4.1 have the same spacing everywhere.

## 2. Re-import procedures, per format and consumer (Q1)

Each subsection ends with the procedure the oracle emulates. Line numbers are at
the revisions in the sources TSV.

### 2.1 PrusaSlicer 2.9.6

**STL.** `load_stl` calls `TriangleMesh::ReadSTLFile(path)` with `repair = true`
[src `Format/STL.cpp:26-31`, `TriangleMesh.hpp:144`]. Binary facets are read
as raw float32 [src `admesh/stlinit.cpp:161`]; ASCII vertices with
`fscanf("%f")`, straight to float [src `stlinit.cpp:178-183`]. Then
`trianglemesh_repair_on_import` [src `TriangleMesh.cpp:98-199`]:

1. `stl_check_facets_exact`. It first **removes every facet with two exactly
   equal vertices** and counts them as degenerate [src `admesh/connect.cpp:441-455`].
   Collinear facets with distinct vertices are kept. Then it hashes each edge
   by its two float32 vertices, sorted, with −0 turned into +0, and pairs equal
   keys from different facets. A third facet on the same key stays unpaired
   [src `connect.cpp:54-85, 177-232`].
2. If any facet has fewer than three neighbours, `stl_check_facets_nearby` runs
   twice. Its tolerance is `shortest_edge` for the first pass: over all edges,
   the smallest of each edge's largest per-axis extent. The second pass adds
   bounding diameter / 10 000. Endpoints of unmatched edges are binned on a grid
   of that spacing, matched by bin, and **moved** onto their partner's position
   [src `TriangleMesh.cpp:116-137`; `connect.cpp:54-60, 87-114, 277-430`]. For a
   400 mm part the second pass uses a 0.04 mm grid.
3. `stl_remove_unconnected_facets` if facets are still unconnected
   [src `TriangleMesh.cpp:145-148`].
4. Hole filling is compiled out ("does more harm than good")
   [src `TriangleMesh.cpp:152-162`].
5. Fix normal directions by neighbour agreement, recompute normal values,
   compute volume and **reverse every facet if the volume is negative**
   [src `TriangleMesh.cpp:166-183`; `admesh/util.cpp:293`].
6. Verify neighbours; re-run the exact check if degenerate facets were removed
   [src `TriangleMesh.cpp:186-196`]; build shared vertices.

**3MF.** PrusaSlicer has its own parser, not lib3mf. Vertex coordinates go
through `fast_float::from_chars` into `float` and are multiplied by a float unit
factor [src `Format/3mf.cpp:250-256, 2103-2111`]. Transforms parse with `atof`
into a double `Transform3d` [src `3mf.cpp:272-298`]. Triangles keep their
indices. No repair runs on 3MF meshes: the code has
`//FIXME do the mesh fixing?` [src `3mf.cpp:2649`]. Two transformations of the
geometry do run. For a 3MF from another producer (`m_version == 0`) whose object
has one instance, the item transform is **baked into the float vertices**:
`(T · v.cast<double>()).cast<float>()` [src `3mf.cpp:2642-2650`;
`admesh/stl.h:278-286`]. A mesh with negative volume is flipped
[src `3mf.cpp:2652-2653`].

Emulation. STL: write the item as binary STL (float32 per corner, as OCCT's
writer does [src `RWStl.cxx:54-62`]) and run PrusaSlicer's own admesh and
`trianglemesh_repair_on_import`, extracted verbatim and compiled into the lane
harness (§3.1). 3MF: fl(x), indices kept (`idx32`). 3MF with a recentred item:
fl(x − c), then fl(c + fl(x − c)) in double (`prusa3mfbake`).

### 2.2 Cura 5.13.0 (Uranium 5.13.0, CuraEngine 5.13.0)

**STL.** Uranium's reader loads with numpy-stl into a float32 record
[src Uranium `plugins/FileHandlers/STLReader/STLReader.py:75-83`; numpy-stl
`stl/base.py:250-259`]. It negates y and swaps y and z, which is exact. It does
not weld. `MeshFileHandler` then recentres the node:
`SceneNode.setCenterPosition` transforms the vertices by −centre
[src Uranium `UM/Mesh/MeshFileHandler.py:42-47`; `UM/Scene/SceneNode.py:119-131`].
`Matrix.getData()` returns `astype(numpy.float32)` [src `UM/Math/Matrix.py:125-130`],
so that subtraction is float32 arithmetic [src `UM/Mesh/MeshData.py:505-517`].
At slice time the world transform is applied the same way, in float32
[src Cura `plugins/CuraEngineBackend/StartSliceJob.py:485-495`]. The triangle
soup crosses to the engine as raw float32 bytes [src pyArcus
`src/PythonMessage.cpp:149-152`; CuraEngine
`src/communication/ArcusCommunicationPrivate.cpp:112-141`].

**3MF.** libSavitar parses coordinates with pugixml `as_float` into float32
[src libSavitar `src/MeshData.cpp:12-20, 57-62`]. Cura's reader keeps item and
component transforms on nodes [src Cura `plugins/3MFReader/ThreeMFReader.py:66-96,
134-140`]. From there the path is the STL path.

**CuraEngine.** Each corner is converted to double and then to integer
micrometres by `MM2INT`, which rounds half away from zero
[src `src/utils/Matrix4x3D.cpp:49-55`; `include/utils/Coord_t.h:28`].
`Mesh::addFace` looks each corner up in a hash keyed by 30 µm cells and **melds
it into the first stored vertex in that bucket within 30 µm**. It **drops the
face** if two of its corners meld [src `src/mesh.cpp:15-23, 40-52, 132-148`].
Corners in neighbouring cells are never compared, so the meld depends on where
the cell boundaries fall. Connectivity comes later from shared vertex indices;
an unmatched edge logs "Mesh has disconnected faces!"
[src `src/mesh.cpp:209-230`]. Defaults that act after slicing: union of
overlapping volumes inside a mesh on, maximum resolution 0.5 mm, maximum
deviation 0.025 mm, touching meshes made to overlap by 0.15 mm, carving of
overlapping meshes only with more than one extruder
[src Cura `resources/definitions/fdmprinter.def.json:8198, 8232, 8244, 8277, 8303`].

Emulation (`cura`): x32 = fl(x); c32 = fl((min32 + max32)/2) per axis;
xl = fl(x32 − c32); xw = fl(xl + t32) with t the drop onto the plate;
p = MM2INT(xw); meld exactly as `findIndexOfVertex`, in face order; drop faces
with a repeated corner.

### 2.3 lib3mf v2.5.0

Reader: `fnStringToFloat` is `(float) fnStringToDouble`, which is fast_float to
double [src `Source/Common/NMR_StringUtils.cpp:147-177`]. Coordinates above 1e9
are refused [src `NMR_ModelReaderNode100_Vertex.cpp:85-101`]. A triangle with
repeated indices throws `NMR_ERROR_INVALIDMODELCOORDINATEINDICES`
[src `NMR_ModelReaderNode100_Triangles.cpp:115-171`]. Nothing geometric is
checked on read. `IsManifoldAndOriented` is an index-level query a caller may
make [src `Source/API/lib3mf_meshobject.cpp:371-374`].

Writer, for anyone tempted to use it: vertices are written fixed-point with six
digits after the point by default (`m_nDecimalPrecision(6)`), produced by
truncating `fValue * 10^6` toward zero [src `Source/Model/Writer/NMR_ModelWriter.cpp:52`;
`v100/NMR_ModelWriterNode100_Mesh.cpp:55, 431-471, 698-710`]. That is a
nanometre grid in millimetres with directed rounding, applied after float32.
Our writer should write the shortest decimal that round-trips the float32
value. Both fast_float paths (PrusaSlicer straight to float; lib3mf through
double) then recover exactly fl(x) [own].

Emulation: `idx32`.

### 2.4 glTF renderer: three.js r186

`GLTFLoader` hands the POSITION accessor to a `Float32Array` view on the buffer
[src `examples/jsm/loaders/GLTFLoader.js:3166-3211`]. It does not weld, and it
sets flat shading when NORMAL is absent [src `GLTFLoader.js:3550-3560`]. Node
matrices and TRS stay JavaScript float64 [src `GLTFLoader.js:4436-4450`]. The
renderer forms `modelViewMatrix = cameraInverse · matrixWorld` in float64 and
uploads float32 [src `src/renderers/WebGLRenderer.js:2160`;
`ShaderChunk/project_vertex.glsl.js`]. A node translation therefore costs no
precision in the vertex data. glTF requires float32 POSITION with exact `min`
and `max` [src glTF 2.0 `Specification.adoc:1480-1511, 1600, 1638`].
Distances are metres after the node's global transform [src
`Specification.adoc:701, 2136`], so millimetre positions under a 0.001 node
scale are legal. Underscore attributes such as `_FACEID` are
application-specific [src `Specification.adoc:1587`]. Blender's importer does
not merge vertices unless asked (`merge_vertices` default False); when asked it
dedupes exact float32 positions plus normals truncated to 1/50 000
[src glTF-Blender-IO `__init__.py:1744-1754`; `blender/imp/gltf2_blender_mesh.py:276, 762-790`].

Emulation: `idx32` for millimetre positions under a node scale; `gltf32m` for
positions converted to metres before rounding, fl(0.001·x). That is a
different grid.

### 2.5 fTetWild (8118f81, geogram v1.9.6, libigl)

`MeshIO::load_mesh` calls `GEO::mesh_load` [src `src/MeshIO.cpp:219-275`].
fTetWild pins geogram v1.9.6 [src `cmake/FloatTetwildDependencies.cmake:81-86`].
At that tag the STL handler creates three fresh vertices per facet, a soup in
double precision, and `mesh_load` only calls `facets.connect()`
[src geogram v1.9.6 `mesh_io.cpp:2358-2422, 2438-2470, 4850-4912`]. The
held newer geogram (390b14d) adds an exact colocation
(`mesh_repair(COLOCATE | DUP_F, 0.0)`) to the STL loader. That is not what
fTetWild runs. `simplify` then calls `remove_duplicates`, which does the welding:
it merges vertices on a grid of 1e-8 × bbox diagonal
(`igl::remove_duplicate_vertices` rounds V/ε and keeps unique rows)
[src fTetWild `src/Simplification.cpp:133-205`; `src/Types.hpp:21-26`; libigl
`remove_duplicate_vertices.cpp`]. It drops repeated-index and duplicate faces,
and **drops faces whose area/2 ≤ 1e-8 × diagonal** (an area compared with a
length) [src `Simplification.cpp:190-205`]. The default envelope is 1e-3 ×
diagonal [src `src/Parameters.h:58-59, 119`]. fTetWild accepts soups,
self-intersections and open meshes by design; inside and outside come from
winding numbers [src `src/main.cpp:546-593`].

Emulation (`ftetwild`): fl(x); exact weld, then the grid merge at 1e-8·diag
(the result equals the grid merge alone, because exact duplicates share a
cell); drop repeated, duplicate and small-area faces.

### 2.6 What every reader shares

All five readers store float32 or coarser. None checks self-intersection. Only
PrusaSlicer's STL path repairs, and its repair can move vertices by up to
diameter/10 000 when any edge is left unmatched. Cura is the outlier: its 30 µm
meld is coarser than either tolerance the campaign uses (5 and 25 µm). STL
readers recover topology from exact float32 equality, so identity-welded FP64
vertices come back exactly connected *unless two distinct vertices round to the
same float32*. Every copy of one vertex rounds the same way because it comes
from one double [own].

## 3. Measurement: what plain rounding breaks (Q2)

### 3.1 Method

**Producer (stand-in).** `mesh_dump.cpp` reads each STEP file with
`STEPControl_Reader` and meshes it with
`BRepMesh_IncrementalMesh(shape, ε, relative = false, angle = 0.5, parallel = false)`
at ε = 0.025 mm and 0.005 mm. It links the pinned OCCT 8.0.1 fleet build. Each
solid becomes one **item**; free shells and free faces become items too. Within
an item, vertices are **welded by B-rep identity**, never by position. A node on
an edge polygon is keyed by (edge, polygon index), an end node by its
`TopoDS_Vertex`, and every node of a degenerate edge by its vertex (a pole is
one vertex). Triangle order is reversed for reversed faces and mirrored
locations. Nodes are FP64; the probe asserts `IsDoublePrecision()`
[src OCCT `Poly_ArrayOfNodes.hxx:26-38`]. This is how our product shares
chains (design §4.3), applied to OCCT's triangles. BRepMesh's pole fans carry
triangles with two corners on the pole. Under identity welding they repeat an
index, and the oracle excludes them as FP64-degenerate (§3.2).

**Origins.** Each item is analysed at five placements: `file` (the STEP
frame); `centred` (bbox centre moved to the origin by a dyadic shift, exact in
FP64); and `plus1m`, `plus10m`, `plus100m` (the centred item moved to
(+D, +D, +D) for D = 1, 10, 100 m). The last three stand for parts baked into
assembly or site coordinates.

**Emulations** (§2): `f64` (baseline); `idx32` (fl(x), indices kept: 3MF
through PrusaSlicer or lib3mf, glTF in millimetres through three.js);
`stl32weld` (fl(x), then a weld by exact float32 equality, the topology an STL
reader recovers); `gltf32m` (fl(0.001·x), indices kept); `prusa3mfbake`
(PrusaSlicer's baked single-instance 3MF with the item recentred by the
writer); `cura`; `ftetwild`. PrusaSlicer's STL import is run, not emulated:
`analyze --write-stl` writes the binary STL and `prusa_stl` runs it (next
paragraph).

**PrusaSlicer harness.** `prusa_stl.cpp` compiles PrusaSlicer 2.9.6's bundled
admesh sources unchanged. It includes `trianglemesh_repair_on_import` extracted
verbatim from `TriangleMesh.cpp:98-199` at build time, and links Eigen 3.4.0
headers. Four small headers stand in for Boost: a discarding `BOOST_LOG_TRIVIAL`,
an `object_pool` backed by `new`, the little-endian macro, and `nowide`
`fopen`/`freopen`. The harness calls `stl_open`, the repair, and
`stl_generate_shared_vertices`, prints admesh's counters, and writes the indexed
mesh PrusaSlicer holds. The oracle then checks that mesh against the item's FP64
counts.

**Oracle** (`analyze.cpp`), per item, placement and emulation:

- *Degenerate*: repeated index, or collinear corners by exact `orient2d` in all
  three axis projections. *New degenerate*: degenerate after, not before.
- *Flipped*: non-degenerate before and after, with the dot product of the FP64
  and rounded normals ≤ 0.
- *Lost manifoldness*: more edges with a use count other than 2, more edges
  used twice in one direction, or more pinched vertices (a vertex whose
  triangles form more than one edge-connected fan) than the FP64 mesh.
  Degenerate and removed triangles are excluded before counting, so a collapsed
  triangle shows as two open edges.
- *Self-intersection*: every pair of non-degenerate triangles whose exact
  bounding boxes overlap (median-split BVH, self-traversal), tested with
  Shewchuk's exact `orient3d`/`orient2d`. CGAL's rules apply: shared edge means
  coplanar and same side; shared vertex means an opposite edge meets the other
  triangle; otherwise closed triangle–triangle intersection
  [src CGAL `self_intersections.h:215-307`]. *New* pairs are those absent from
  the FP64 set, by triangle id.
- *Collapsed*: distinct vertices with identical stored coordinates. *Removed*:
  faces a consumer drops (Cura meld, fTetWild filters).
- *Displacement*: the largest vertex move, measured on the stored values.
- Two falsifiers of §1: flips among triangles whose FP64 altitude exceeds
  2·(measured largest move), and new pairs whose FP64 separation exceeds it.
  The derivation predicts zero. A detail mode prints every flip and new pair
  with its FP64 altitude, edge lengths, separation and, for shared edges, the
  fold angle.

**Checks on the oracle.** A self-test of eleven predicate configurations
(crossing, parallel, coplanar overlap, touching at a point, proper and folded
shared edges, flat shared edge, fan neighbours, piercing at a shared vertex,
coplanar sectors, duplicate face, degenerate) and a BVH needle-in-haystack
case passes. The four authored rounding controls of §4.2 are flagged, and the
clean control is not. On the stand-in, the FP64 baseline of a clean solid
reports zero for every count by construction of the "clean" filter (§3.2).

**Fleet context.** Meshing ran with 12 single-threaded workers pinned to socket
1 (`numactl --cpunodebind=1 --membind=1`). Analysis ran with 8 workers × 2
OpenMP threads per tolerance, 0.025 mm on socket 0 and 0.005 mm on socket 1,
plus a 6-worker helper on socket 0 at the end. Load average was 22–40 during
analysis; most of that was other lanes' compiles and probes (`g1-probe`,
`clang++-18`, `tui_fuzz`). Governor `schedutil`; mean reported frequency
2.66–3.0 GHz; SMT on; AutoNUMA on (`numa_balancing = 1`). Inputs were warm in
the page cache, having just been written. Timings in §3.6 are therefore upper
bounds under contention, not benchmarks.

### 3.2 Corpus and the stand-in's own FP64 state

Corpus: 478 unique STEP files (by SHA-256) under `corpus/sources` (NIST MBE
PMI, CAx-IF library, five printer TDPs). Every file meshed at both tolerances
with no failures and no missing edge polygons. 467 files yield at least one
item; 11 contain no solid, shell or face that meshes. The stand-in produced
17.47 M triangles at 0.025 mm and 58.67 M at 0.005 mm in 6,044 items (6,011
solids, 33 free shells).

The stand-in is not clean in FP64 [meas]. At 0.025 / 0.005 mm:

- 102 / 219 items already self-intersect;
- 380 / 430 items are open or non-manifold by identity;
- 1,713 items carry index-degenerate pole triangles, two corners on one pole
  vertex.

The rates below use the **clean** items only: solids that in FP64 are closed,
consistently oriented, free of pinched vertices and free of self-intersections,
after the pole triangles are set aside. That leaves 5,608 items in 444 parts
(15.15 M triangles) at 0.025 mm and 5,442 items in 428 parts (46.79 M
triangles) at 0.005 mm. These are the items whose FP64 guarantees match what our
product promises. 1,490 of the 5,608 still contain FP64 zero-area triangles
(mostly pole fans), which the counts exclude.

### 3.3 Failure rates under plain rounding

An item **fails** if, as the consumer holds it, it has any new degenerate
triangle, flipped triangle, new self-intersecting pair or lost manifoldness. A
part fails if any of its items fails. Clean items only; first figure 0.025 mm,
second 0.005 mm.

**Index-preserving float32** (`idx32`: 3MF read by PrusaSlicer or lib3mf,
glTF in millimetres read by three.js):

| Placement | Items failing | Parts failing | Largest vertex move |
|---|---|---|---|
| file frame | 193 (3.4%) / 130 (2.4%) | 26 (5.9%) / 20 (4.7%) | 123 nm / 125 nm |
| centred | 143 (2.6%) / 113 (2.1%) | 19 (4.3%) / 22 (5.1%) | 62 nm / 63 nm |
| +1 m | 250 (4.5%) / 226 (4.2%) | 22 / 24 | 127 nm / 134 nm |
| +10 m | 397 (7.1%) / 459 (8.4%) | 36 (8.1%) / 46 (10.7%) | 844 nm / 844 nm |
| +100 m | 1,043 (18.6%) / 1,760 (32.3%) | 56 (12.6%) / 73 (17.1%) | 6.74 µm / 6.75 µm |

At the file frame, 0.025 mm, the failures in triangles: 106 new degenerate,
895 flipped, 1,547 new intersecting pairs (184 across a shared edge, 673
across a shared vertex, 690 disjoint) out of 15.15 M triangles. Collapsed vertices: 3 items.

In the STEP frame, 5,008 of the 6,044 items lie
within 512 mm of the origin, 1,019 within 0.5–1 m, 7 within 1–2 m, and 10
farther out. The farthest coordinate is 3.81 m, in the CAx-IF `as1-pe-203`
assembly; its δ bound is 211 nm against 123 nm measured. TDP parts are modelled
in machine coordinates. That is why `file` sits between `centred` and `+1 m`.

**STL read by exact float32 equality** (`stl32weld`): the same items fail as
under `idx32`, because rounding is the same. A weld turns some touching pairs
into merged vertices. At +100 m and 0.025 mm, the 101,549 new intersecting pairs
under `idx32` become 24,706 after the weld. 828 items gain a degenerate
triangle, and after the weld 522 of them lose manifoldness.

**PrusaSlicer STL import, run** (`prusa_stl`; failing = PrusaSlicer had to
change something beyond the pole triangles, or its result has more
intersecting pairs than the FP64 mesh):

| Placement | Items | Parts | What PrusaSlicer did |
|---|---|---|---|
| file | 87 / 85 | 10 / 18 | 85 / 83 items keep new intersecting pairs; 3 / 3 items lose collapsed facets |
| centred | 27 / 53 | 9 / 17 | intersecting pairs only |
| +10 m | 302 / 392 | 22 / 43 | 81 / 173 items lose collapsed facets |
| +100 m | 928 / 1,627 | 42 / 66 | 564 / 811 items lose collapsed facets; 8 / 7 items get facets reversed by the normal fix; 4 / 6 items end with open edges |

PrusaSlicer's nearby merge never ran on a clean item: `edges_fixed` was 0 at
every placement and tolerance. Removing coincident-vertex facets was enough to
re-close the meshes, except for 4 / 6 items at +100 m. It reports fewer
failures than `stl32weld` because it does not count flips and keeps collinear
facets. Its flips are still there (§3.4).

**glTF in metres** (`gltf32m`) fails 179 / 142 items at the file frame,
against 193 / 130 for millimetres. That is a different grid with the same
mechanism. **PrusaSlicer 3MF with a recentred, baked item**
(`prusa3mfbake`): 181 / 136 items at the file frame. Baking returns every
vertex to the world grid with one extra double rounding, so recentring in the
file does not help PrusaSlicer (+10 m: 397 / 459, identical to `idx32`).

### 3.4 What the failures are

The detail pass prints every flip and every new pair with its FP64 geometry.
For `idx32` at 0.025 mm, in the files that have any defect [meas
`out/detail_0.025.tsv`]:

| Placement | Flipped triangles with FP64 altitude < 1 pm | New pairs with FP64 gap < 1 pm | 1 pm – 1 nm | 1 nm – 2δ | > 2δ |
|---|---|---|---|---|---|
| file | 1,079 of 1,091 (98.9%) | 1,114 of 1,728 (64%) | 502 (29%) | 110 (6.4%) | 2 |
| centred | 818 of 818 (100%) | 924 of 1,280 (72%) | 336 (26%) | 16 (1.3%) | 4 |
| +10 m | 1,221 of 1,552 (79%) | 933 of 6,893 | 599 | 5,357 (78%) | 4 |
| +100 m | 1,176 of 2,982 (39%) | 2,114 of 65,221 | 459 | 62,584 (96%) | 64 |

At 0.005 mm [meas `out/detail_0.005.tsv`]:

| Placement | Flipped triangles with FP64 altitude < 1 pm | New pairs with FP64 gap < 1 pm | 1 pm – 1 nm | 1 nm – 2δ | > 2δ |
|---|---|---|---|---|---|
| file | 2,007 of 2,123 (94.5%) | 82,348 of 91,833 (89.7%) | 6,456 (7.0%) | 2,845 (3.1%) | 184 |
| centred | 1,912 of 1,917 (99.7%) | 83,220 of 90,415 (92.0%) | 5,878 | 1,131 | 186 |
| +10 m | 2,219 of 3,778 (59%) | 82,362 of 161,092 | 7,031 | 71,600 (44%) | 99 |
| +100 m | 2,422 of 14,971 (16%) | 69,886 of 534,597 | 4,030 | 460,459 (86%) | 222 |

The detail tables cover every item in a file that has a defect, clean or not.
At 0.005 mm, 76,720 of the 91,833 file-frame pairs come from one item
(CAx-IF `s1-id-214`, item 0). That item already has 7,253 intersecting pairs
in FP64 and is outside the clean set. Items, not pairs, are the failure unit in
§3.3.

What this shows:

- **Near the origin the stand-in's failures are FP64 noise.** Almost every
  flipped triangle has an altitude below a picometre. These are zero-area
  triangles that FP64 keeps by roundoff, for example three nearly collinear
  nodes along a straight edge (a 1.5 × 2.9 mm triangle with 4e-13 mm altitude
  in NIST CTC-02). Most new pairs are features that FP64 holds apart by less
  than a nanometre. A mesher that emits neither has nothing for rounding to
  break.
- **Far from the origin, real slivers fail.** As δ grows to 0.85 µm (+10 m) and
  6.7 µm (+100 m), triangles and gaps of genuine size (1 nm to 2δ) dominate.
  This is the origin problem, and recentring removes it (§4.4).
- **The §1 derivation was not falsified.** Over both tolerances, five
  placements and every emulation except Cura: no triangle with FP64 altitude
  above 2δ flipped or degenerated; no new pair across a vertex, or between
  disjoint triangles, had an FP64 gap above 2δ. The exceptions are folds across
  a shared edge whose far-vertex gap exceeds 2δ (2 at the file frame, 64 at
  +100 m). Every such pair was checked. At 0.025 mm, 393 of the 394
  involve a triangle with altitude ≤ 2δ; the remaining one has a fold angle
  within the C3 bound (re-run with fold angles). At 0.005 mm, 3,933 of the
  4,120 involve a triangle with altitude ≤ 2δ, and all 187 others have fold
  angles within the bound. No case was unexplained. C1 and C3 together held
  everywhere [meas `out/detail2/`, `out/detail2_005/`]. Cura is excluded from this check because its meld moves
  vertices by up to 30 µm, not δ.
- At-risk triangles (altitude ≤ 2δ) are rare: 2,935 of 15.15 M at the file
  frame, 0.025 mm (0.019%); 5,204 of 46.79 M at 0.005 mm (0.011%). At
  +100 m they become 256,516 (1.7%) and 1.45 M (3.1%).

### 3.5 Consumer-specific losses that are not float32's doing

These losses happen in the consumer whatever the export does.

- **Cura: the 30 µm meld removes faces in about half the clean items.**
  2,598 of 5,608 items lose 249 k triangles (1.6%) at 0.025 mm. At 0.005 mm,
  2,641 of 5,442 lose 1.31 M (2.8%). These track the triangles with an edge
  shorter than 30 µm: 670 k (4.4%) and 4.46 M (9.5%) of FP64 triangles. Meld
  displacement reaches 30 µm, larger than either tolerance. Corners melded
  across hash cells are compared only inside one bucket, so which faces vanish
  depends on cell boundaries.
- **fTetWild: the area filter removes faces in 27–29% of clean items.**
  10.4 k triangles at 0.025 mm and 10.9 k at 0.005 mm have area/2 ≤
  1e-8·diag. That matches the 11.3 k and 12.1 k FP64 triangles below the
  threshold, so this is fTetWild's filter acting on the producer's slivers, not
  rounding. fTetWild tolerates the resulting holes and intersections by design.
  Its default envelope of 1e-3·diagonal (0.1 mm on a 100 mm part) is far
  coarser than ε; callers who want ε must pass `-e`.
- **PrusaSlicer** removes coincident-vertex facets, which only rounding
  creates (§3.3).

### 3.6 Cost of the oracle

The oracle's exact self-intersection pass took 46.1 s of 2-thread time for
17.47 M FP64 triangles at 0.025 mm and 138.6 s for 58.67 M at 0.005 mm: 2.4–2.6
µs per triangle, with 7.9–8.1 box-overlapping candidate pairs per triangle.
One full emulation check (round, weld, topology, pinch, intersection, compare)
costs 4.6–5.2 µs per triangle. The largest item, 246 k triangles, took 0.37 s.
These figures were taken under load average 22–40 from other lanes (§3.1). They
are upper bounds and show that a reader-equivalent oracle is affordable
**offline**. It is not proposed for the hot path.

## 4. Remedies: guarantee and cost (Q3)

### 4.1 Snap rounding with intersection removal

**What the literature establishes.**

- *Guaranteed 3D snap rounding exists but is impractical.* Devillers, Lazard and
  Lenhart round interior-disjoint polygons to integer vertices. Their bounds:
  every face within L∞ Hausdorff 3/2 grid units of its image; relative positions
  kept "up to collapse"; O(n¹³) output and O(n¹⁵) time in the worst case,
  O(n⁴√n) and O(n⁵) under smoothness assumptions [src DLL2020 abstract, §1,
  Theorem 1]. Valque's practical version of it fails on 35% of the 527 hard
  Thingi10K models and runs at about 100 vertices/s [src LV2025 §2, citing
  Valque 2024; thesis not read].
- *The practical method is a heuristic.* Lazard–Valque: find properly
  intersecting pairs; snap their vertices to cell centres of the finest uniform
  grid whose values are floats; also snap every vertex in those cells; compute
  the exact arrangement (CGAL autorefine, EPECK); round new vertices to
  **doubles**; repeat at most 5 times [src LV2025 §3]. It resolved every
  self-intersecting Thingi10K model, one only on a coarser grid, at 16.5–18.9k
  arrangement vertices/s serial [src LV2025 §4.2–4.3, Table 2]. The authors
  write that "supporting evidence remains elusive" for why it works, and that
  iterating naive rounding is not known to terminate [src LV2025 §1, §5].
- *Its float32 variant is worse.* "Rounding all coordinates to floats (after
  computing self-intersections) performs very poorly": it fails on 42% of
  self-intersecting Thingi10K models, and 24% after an hour of iteration
  [src LV2025 §5].
- *CGAL ships it.* `autorefine_triangle_soup(..., apply_iterative_snap_rounding(true))`.
  Its defaults are `snap_grid_size` 23 and `number_of_iterations` 5. The grid
  size is documented as scaling points to [−2^gs, 2^gs]. The code's per-axis
  scale 2^(gs − e − 1), with e the `frexp` exponent of the largest magnitude,
  puts the largest coordinate in [2²¹, 2²²). The documented return is `false` "if
  the algorithm was unable to produce such a result within the allowed number
  of iterations", with "a partial result from the final iteration, with no
  guarantee of its validity" [src CGAL `autorefinement.h:1699-1722`;
  `internal/triangle_soup_snap_rounding.h:262-263, 283-305, 307-360, 545`]. The
  manual reports Thingi10K: 3,870 inputs with self-intersections, 529 still
  failing after autorefinement with naive rounding, 0 with iterative snap
  rounding [src CGAL `doc/PMP_Boolean_operations.txt:175-210`].
- *The earlier heuristic.* Zhou et al. round to double, find the intersecting
  triangles, round their vertices to single precision, self-union, and repeat.
  It succeeds on 99.95% of their outputs; plain rounding left 2.19% non-solid
  [src Zhou et al. 2016 §6.1].

**Fit to our problem [own].** These methods repair a soup that already
intersects, typically the exact output of a Boolean. Our FP64 mesh is
intersection-free by construction. What breaks on export is a handful of
near-degenerate elements (§3.4). Snap rounding would:

- re-triangulate faces and merge vertices, which breaks chain identity, face
  ids and source maps (design §5.5);
- produce doubles, not float32, in the published method; the float32 variant
  is the one that fails;
- give no guarantee, by its own return contract, and carry the autorefinement
  cost of an exact arrangement on every export.

It answers the wrong question for a valid mesh. It stays the right tool for a
*source* that self-intersects (lifecycle S21; design §4.9 "faithful plus
reported"), and that is not this gap.

**Cost and guarantee.** Guarantee: none for float32 output. With doubles,
"intersection-free if it returns true". Cost: an exact arrangement plus ≤ 5
rounds, measured at 16–19k arrangement vertices/s serial [src LV2025 Table 2].
**Not recommended for export.**

### 4.2 CGAL's snap-rounding fixtures

`PMP_Boolean_operations/test/PMP_Boolean_operations/data-snap_rounding/` holds
ten OFF soups: `collapse_1..3`, `no_collapse_1..2`, `intersection_when_rounded_1..3`,
`cubes`, `coplanar_cubes` [src CGAL tree]. `test_snap_rounding.cpp` asserts the
vertex count after autorefinement, the triangle count after snapping, and
`!does_triangle_soup_self_intersect` [src `test_snap_rounding.cpp:65-85`].
These inputs **already self-intersect**; they test the repair, not the export.
`intersection_when_rounded_1` is a triangle piercing two others with a 1e-30
coordinate. `collapse_1` relies on a −1e-10 offset vanishing on CGAL's uniform
grid. **Under float32 that offset survives**, because float32 is relative
precision. The fixtures therefore do not transfer to float32 as-is [own].

The test-plans paper proposed them as seeds for an output-precision oracle
[src `2026-10-17-mesher-test-plans-meshgen.md` B6]. The seeds that do transfer
are the configurations they encode, re-authored so that the FP64 input is valid
and the hazard sits where coordinates are large. The lane wrote four such
controls (`controls.py`, §9), all at |x| ≈ 300 mm where ulp = 30.5 nm:

| Control | FP64 | Rounded (every emulation) |
|---|---|---|
| Sliver whose apex is 2 nm off its base line | valid, closed | degenerate triangle, 2 intersecting pairs |
| Two boxes 1 nm apart | disjoint | faces coincide: 64 intersecting pairs index-preserving; 4 non-manifold edges after an STL weld |
| Two boxes whose corners are 1 nm apart | disjoint | 25 touching pairs index-preserving; a pinched vertex after an STL weld |
| Same boxes 1 mm apart | disjoint | clean |

Every control changes the oracle's verdict in every emulation, and the clean
control stays clean [meas `out/controls.g3m`]. These are the negative controls
§10 of the design asks for ("each oracle is specified against the degenerate
configurations it must flag").

### 4.3 Quantization-aware construction

**Mechanism [own].** Choose every vertex's stored position at construction
time, in the frame the file will use: fl(x) for float32 formats, or a declared
integer grid (§4.5). Run the construction's own validity checks on the stored
values, not on FP64. Then make C1–C3 construction rules:

- **C1**, every triangle's altitude onto its longest edge > 2δ. Triangles that
  fail are what the quality rules already target (design §4.7 refines caps; the
  same pass collapses or splits needles). On the stand-in that is 0.011–0.019%
  of triangles at the file frame (§3.4).
- **C2/C3**, every non-adjacent pair farther apart than 2δ, no opposite edge
  within 2δ of its neighbour, and no knife-edge fold within the C3 angle. These
  are the near-contact classes of design §5.6 and the thin-feature lane's
  clearance budget. Rounding adds 2δ to the clearance that budget must reserve
  [src thin-feature-embedding §3.7 table, "Reserve 2·u32"].

**Guarantee.** With C1–C3 checked on the stored coordinates, the written file
has every FP64 guarantee except exact deviation. Deviation is within ε + δ by
convexity, so build to ε − δ or charge δ to the request (§1 table: δ ≤ 0.5% of
5 µm below 512 mm). Every copy of a vertex in every format is the same stored
value, so cracks are impossible: there is no rounding at write time.

**Cost.** One rounding per vertex. One altitude test per triangle, which can be
exact with `orient2d` on stored values or a filtered floating test. A local
collapse or split for the rare failures. The C2/C3 terms reuse the near-contact
query the design already runs, with 2δ added to its inflation. No arrangement,
no re-triangulation, no extra pass over the output.

**What remains unproven.** C2 is enforced by the planning query only where that
query is sound, which is G4's question. The offline oracle (§4.5) is what
catches a miss.

**Hypothesis H1** (testable in the arena): a mesher that enforces C1 and the
2δ-inflated clearance produces zero new degenerate, flipped or self-intersecting
elements under `idx32` and `stl32weld` at the file frame and when centred, on
the whole corpus, at both tolerances. **Falsifier:** any such element in the
reader-equivalent oracle's report.

### 4.4 Recentring origins per item

**Mechanism.** Write each item's vertices in a frame centred on the item. The
item's placement goes into the format's transform: 3MF `<item transform>`, a
glTF node translation. R then becomes the item's half extent, not its distance
from the world origin.

**Measured effect** (`idx32`, clean items, 0.025 / 0.005 mm): +10 m fails 397 /
459 items and +100 m fails 1,043 / 1,760. Centred, every placement fails only
143 / 113, and the largest move falls from 6.74 µm to 62 nm. The residual 143 /
113 are the near-noise elements of §3.4, which recentring cannot fix. When
centred, 98% of new pairs had an FP64 gap under 1 nm and 99.7–100% of flips an
altitude under 1 pm.

**Guarantee.** δ is set by the item's own extent (table in §1). For a 300 mm
part, δ ≤ 26 nm wherever the item sits in the assembly. The transform must
reach the consumer in at least double precision.

**Consumers decide whether recentring survives** [src §2]:

| Consumer | Item transform kept? | Recentring helps? |
|---|---|---|
| three.js | yes, float64 node matrices; modelView formed in float64 | yes |
| lib3mf | transform parsed to a float32 `NMATRIX3` [src `Include/Common/Math/NMR_Geometry.h:85-87`; `Source/Common/Math/NMR_Matrix.cpp:436-455`]; vertices stay local | yes for validity, as long as the calling application does not bake the transform into float32 vertices: rounding the translation shifts the whole item rigidly by at most δ(world) and changes nothing inside it |
| PrusaSlicer 3MF, foreign file, one instance per object | **baked into float32 world vertices** | **no**: `prusa3mfbake` = world rounding plus one more |
| PrusaSlicer 3MF, objects with several instances | kept as double `Transform3d` | yes |
| Cura | transform applied in float32 numpy at slice time | partly: Cura recentres each mesh on load anyway, then rounds world positions in float32 and integer µm |
| STL readers | STL has no transform | n/a: only moving the whole file helps |

**Cost.** One subtraction per vertex at write time, or none if construction
already works in item frames.

### 4.5 Float32 as a declared quantization with a reader-equivalent oracle

**Mechanism.** Treat the format's precision as a declared quantization, as
KHR_mesh_quantization does for integer storage with a dequantizing node
transform [src KHR_mesh_quantization README]. Write the stored values exactly:

- binary float32 for STL and glTF;
- the shortest round-tripping decimal of each float32 for 3MF, so both
  fast_float paths recover it exactly (§2.3);
- glTF `min`/`max` taken from the stored floats [src glTF §accessor bounds].

Then run the **reader-equivalent oracle** offline on the bytes. That oracle is
§3.1's: parse; rebuild what each named consumer holds; check degeneracy,
orientation, manifoldness by index, pinched vertices, exact self-intersection
against the FP64 set, and the displacement bound. The verdict is per format
and per consumer. It can fail: the authored controls of §4.2 are its negative
controls.

**Guarantee.** Detection, not prevention. Together with §4.3 it is the "export:
validity rechecked on written bytes" entry of design §10, made precise.

**Cost.** 2.4–2.6 µs per triangle for the exact intersection pass and 4.6–5.2
µs for a full emulation check, 2 threads under load (§3.6). Offline only.

## 5. 3MF separate items (Q4)

**What the specification requires** (3MF Core 1.4.0, published 6 Feb 2025)
[src spec_core@997b385]:

- §3.4.3.1, items: "A consumer MUST apply the transform prior to outputting the
  object." "consumers MAY rearrange the items for manufacturing in order to
  better pack the build volume." "Sometimes objects are arranged in the
  coordinate space so as to be manufactured in an interlocking fashion;
  producers of these objects SHOULD collect them as components ... as 3D
  manufacturing devices MUST NOT transform components of an object relative to
  each other."
- Same section: "**Items SHOULD NOT overlap, but if they do, 3D manufacturing
  devices MUST unite the final geometry.**" Properties on the overlap come from
  the last item. "each referenced object MUST resolve internal
  self-intersections before merging them in the build level."
- §4.2, components: "If the components overlap, 3D manufacturing devices MUST
  unite the final shape."
- §4.1, meshes of type model: every edge shared by exactly one other triangle
  *by vertex index*, consistent orientation, outward normals. Producers SHOULD
  NOT self-intersect; consumers MUST accept self-intersecting and overlapping
  objects under the positive fill rule. Triangles SHOULD have non-zero area.
  v1, v2, v3 MUST be distinct (§4.1.4.1). Producers SHOULD NOT duplicate vertices
  "unless coalescing duplicates would create non-manifold edges" (§4.1.3).
- §3.3: a transform with negative determinant "MUST NOT change the sign of its
  volume". §4.1.3.1: coordinates are decimals of arbitrary precision
  (ST_Number). §3.4: `unit` defaults to millimetre.

The specification does not use "touch". Touching items have zero-volume
overlap; uniting them changes nothing on paper. In a printer they fuse once the
slicer sees overlapping or coincident walls [inf].

**What lib3mf requires.** Only index validity and distinct indices per triangle
on read; no geometric or overlap check [src §2.3]. Overlap handling is left to
the device.

**What the named consumers do** [src]:

- PrusaSlicer loads each item of a foreign 3MF as an instance of its object.
  Single-instance objects are baked to world float32 (§2.1). Items stay
  separate ModelObjects that the user or auto-arrange may move. The lane did not
  locate an inter-object union in the slicing code it read. Whether two
  overlapping objects extrude twice or are united is **not determined**; slicing
  two overlapping items and reading the G-code would settle it.
- Cura keeps items as separate nodes. `multiple_mesh_overlap` (0.15 mm,
  default) *deliberately* grows touching meshes into each other. Overlap carving
  between meshes is on only with more than one extruder [src
  `fdmprinter.def.json:8232, 8244`].

**Consequence for D-formats [own].** The operator chose "3MF with separate
items, not united components". The spec gives separate items one thing that
components do not: the consumer may move them. It does not exempt them from
union: overlapping items MUST still be united. Separate items avoid fusing only
if they do not overlap *as the consumer reads them*. Float32 rounding moves
each surface by up to δ, so two items in FP64 contact (gear flanks, a pin in a
bore) can overlap by up to 2δ after reading. Before any rounding, two
separately meshed curved surfaces in contact can already interpenetrate by up
to the chord error ε [inf; the thin-feature lane's subject]. Cura then adds
0.15 mm on purpose.
An assembly written as separate items keeps parts apart only where FP64
clearance exceeds 2δ, or where the consumer re-arranges the items. This goes to
the operator (§8).

## 6. Recommendation per format

Common to all three formats [own, from §1, §3, §4]:

- **No snap rounding at export.** The FP64 mesh is already valid. What breaks
  is local and is prevented by C1–C3.
- **Quantization-aware construction.** Stored positions are chosen at
  construction in the written frame. C1 (altitude > 2δ) is enforced by the
  quality pass. C2/C3 enter the near-contact budget as +2δ.
- **Deviation budget.** Report ε as met by FP64 + δ. With the §1 table, δ is
  ≤ 0.5% of 5 µm for frames within 512 mm.
- **Welding is identity, never position.** Every copy of a vertex is the same
  stored value. STL readers' exact-equality weld then reproduces the identity
  weld, as long as no two distinct vertices share a stored value; C2 rules that
  out. A producer-side positional weld is forbidden: it would merge distinct
  vertices, which EXT_mesh_manifold's authors also warn against.
- **Offline reader-equivalent oracle** per format and named consumer (§4.5),
  with the §4.2 controls as negative controls. It runs on the written bytes; it
  is not in the hot path.

| | STL (binary) | 3MF (separate items) | glTF (face ids, normals) |
|---|---|---|---|
| Re-import the oracle emulates | fl32 per corner; exact float32 weld (−0 = +0); PrusaSlicer's repair run as code; Cura µm + 30 µm meld; fTetWild grid weld at 1e-8·diag + area filter | decimal → float32 (PrusaSlicer direct, lib3mf via double); indices kept; no repair; PrusaSlicer bakes single-instance foreign items into world float32 | float32 POSITION as stored (three.js); no weld; float64 node matrices |
| Failure rate under plain rounding, clean items, file frame, 0.025 / 0.005 mm | 3.4% / 2.4% of items (exact weld); PrusaSlicer run: 1.6% / 1.6% of items, 2.3% / 4.2% of parts | 3.4% / 2.4% (index-preserving); PrusaSlicer baked: 3.2% / 2.5% | 3.4% / 2.4% (mm under node scale); 3.2% / 2.6% (metres) |
| Same, at +100 m without recentring | 18.6% / 32.3% | 18.6% / 32.3% | 18.6% / 32.3% (mm); 17.6% / 29.3% (m) |
| Selected remedy | quantization-aware construction in the file frame; whole-file recentring only if δ(R) exceeds the budget (operator question 3) | quantization-aware construction with δ taken at the **world** frame, because PrusaSlicer bakes; vertices written item-local under the item transform; shortest float32 decimals | quantization-aware construction in the item frame; positions in mm under a 0.001 node scale, or in m, with the check run on the stored grid; node translation carries placement |
| Welding | none at write; identity-welded values | shared indices (MUST be manifold by index) | per-face and per-normal split vertices from one stored value; `EXT_mesh_manifold` manifold primitive carries the identity weld |
| Cost | one rounding per vertex; one altitude test per triangle; +2δ on the existing clearance query; oracle 2.4–5.2 µs per triangle offline | same | same |

Named-consumer limits that no export removes, to be reported rather than
fixed: Cura's 30 µm meld (operator question 2) and slicing resolution
(0.5 mm / 0.025 mm deviation); fTetWild's area filter and its 1e-3·diag default
envelope (pass `-e` near ε); PrusaSlicer's facet reversal on negative volume.

## 7. Corrections to earlier papers

1. **The "about 12%" statistic does not measure our case.** The lifecycle map
   (finding 3, S13, X4) reads Lazard–Valque as "rounding exact coordinates to
   floats created new self-intersections in about 12% of self-intersecting
   Thingi10K models". The source's 527 of 4,524 are *already self-intersecting*
   models. Their mathematically exact arrangement (autorefinement) gains new
   intersections when its *new* vertices are rounded **to doubles**
   [src LV2025 §1, §4.2]. Nothing in that number is float32 export of a valid
   mesh. The measured rates for that case are in §3.3.
2. **X4's remedy framing.** "Snap-rounding with intersection removal" is a
   repair for self-intersecting soups. For an intersection-free FP64 mesh the
   failures are local near-degeneracies that C1–C3 rule out (§1, §3.4, §4.3).
3. **"PrusaSlicer keeps them [zero-area triangles] for connectivity"**
   (fast-tessellation §5) is half right. PrusaSlicer's STL import *removes*
   every facet with two exactly equal vertices [src `admesh/connect.cpp:441-455`]
   and keeps collinear facets whose vertices are distinct. Its 3MF import keeps
   both.
4. **S13's 3MF rule.** "Overlapping items or components MUST be united" is
   correct for both. Design §5.5's "3MF with components for instances" and
   D-formats' "separate items, not united components" both meet the union rule
   whenever parts overlap after reading (§5).
5. **lib3mf and float32.** S13 says lib3mf stores float32; correct. It also
   *writes* six fixed decimals by truncation by default (§2.3), which S13 did not
   record.
6. **geogram version.** fTetWild runs geogram v1.9.6, whose STL loader returns
   a soup. The exact-colocation STL path in the held geogram (390b14d) is newer
   than what fTetWild pins (§2.5).

## 8. Defects, operator questions

**Defects in our code.** None found. The repository has no STL, 3MF or glTF
writer yet [src lifecycle map S13]. The example PLY writers
(`examples/mesh_body.py:239-247`, `examples/render_watch.py:81-95`) write
`%.17g` doubles, which round-trip.

**Lane probe note.** PrusaSlicer computes open edges in `uint32_t` arithmetic
and stores the result in an `int` [src `TriangleMesh.cpp:232-235`;
`TriangleMesh.hpp:104`], which recovers the signed value. The first harness
build printed it as a 64-bit `long` and showed wrapped values near 8.6e9. The
harness now also prints the signed count, and the summaries use only that.

**Operator questions.**

1. **3MF separate items vs union (D-formats).** The spec says overlapping items
   MUST be united, and consumers MAY move items (§5). Separate items stay
   separate only if, as read, they do not overlap. Parts in FP64 contact
   (meshing gears, a pin in a bore) can overlap by up to 2δ after float32
   reading, and Cura grows touching meshes by 0.15 mm on purpose. Is the intent
   (a) "parts a consumer may lay out separately", which items deliver if the
   writer guarantees a gap of more than 2δ between items or leaves contact pairs
   to the consumer's arrangement, or (b) "the assembly in place, parts not
   fused", which 3MF cannot express by items or components?
2. **Cura's 30 µm meld (D-consumers).** CuraEngine merges vertices within 30 µm
   and drops the faces that collapse (§2.2, §3.5). That is coarser than both
   tolerances in use (5 and 25 µm). Do we accept Cura as a consumer that
   coarsens below our ε, reported as a consumer limit, or must the Cura-targeted
   export avoid edges shorter than 30 µm? The second conflicts with ε at small
   fillets and threads.
3. **STL coordinate frame.** STL has no transform. Writing the source frame
   keeps δ under 2% of 5 µm only within about 2 m of the origin (§1 table).
   Writing recentred coordinates moves the part in the file. Slicers re-place
   parts anyway [src Uranium `MeshFileHandler.py:42-54`]. Default proposed:
   source frame, with refusal or a recorded recentring when δ(R) exceeds the
   budget. Is a recentred STL acceptable when that triggers?

## 9. Reproduction

Everything ran in the lane directory on the fleet host (`lanes/M4-G3/`).
Nothing ran on the laptop. Reference code was fetched there at the pinned
revisions in the sources TSV, with `git clone --depth 1 --filter=blob:none
--sparse` at the tag, `git rev-parse HEAD` recorded, and `.git` removed. The
fetch script and its record are `refs/fetch.sh` and `refs/FETCHED.tsv`.
QUARANTINE.md was not edited (common brief); the dispatcher may want to add
these references.

| Probe file | Lines | SHA-256 | What it does |
|---|---|---|---|
| `mesh_dump.cpp` | 172 | `a3942aa3…a5fdd26f` | OCCT 8.0.1 read and BRepMesh; identity-welded FP64 items to a binary dump |
| `analyze.cpp` | 840 | `7c194934…7fde6f50` | emulations, exact oracle, self-test, STL writer, detail mode. The bulk runs used the build before the fold-angle field was added to the detail output; nothing else differs |
| `prusa_stl.cpp` + `repair_on_import.inc` | 49 + 103 | `62a0fb61…5000229`, `571df210…46198` | PrusaSlicer 2.9.6 admesh and repair, run unchanged |
| `shim/` (5 headers) | — | in the lane directory | Boost stand-ins for the harness only |
| `predicates.c` | — | `f8662c3f…92029` | Shewchuk, as held in quarantine |
| `controls.py` | — | `01852c28…e80c` | authored negative controls |
| `run_mesh.py`, `run_analyze.py`, `run_analyze_helper.py`, `aggregate.py`, `detail.py` | — | `4d9797a5…`, `b7ceb4b8…`, `05973f7f…`, `c0864cc6…`, `5cdf4b8a…` | drivers |

Builds: `clang++-18 -std=c++17 -O2` for `mesh_dump` against the pinned OCCT 8.0.1
install, with RPATH so transitive toolkits resolve. `g++ -std=c++17 -O2
-ffp-contract=off -fopenmp` for `analyze`; `predicates.c` compiled with
`-ffp-contract=off`. `g++ -std=c++17 -O2 -DNDEBUG` for `prusa_stl`, with
`-Iprobe/shim -Irefs/eigen`.

Commands:

- `python3 probe/run_mesh.py 12 0.025,0.005` writes `out/files.tsv` (id,
  SHA-256, bytes, corpus path) and `out/mesh/<ε>/`.
- `python3 probe/run_analyze.py <ε> 8` writes `out/an/<ε>/` and `out/pr/<ε>/`.
- `python3 probe/aggregate.py 0.025 0.005` writes `out/summary_*.tsv`.
- `python3 probe/detail.py <ε> 8` writes `out/detail_<ε>.tsv`.
- `./build/analyze --selftest`.
- `python3 probe/controls.py out/controls.g3m && ./build/analyze out/controls.g3m ctl`.

Outputs stay in the lane directory. Summary tables are quoted in §3.

## For SHOPPING_LIST.md

- Fortune S., "Vertex-rounding a three-dimensional polyhedral subdivision",
  DCG 22(4), 1999. Not fetched (publisher site); cited through LV2025 and
  DLL2020. It would settle whether its n-dependent grid could be bounded for
  mesh sizes we emit.
- Milenkovic V., Sacks E., "Geometric rounding and feature separation in
  meshes", CAD 108, 2019 (Elsevier). Not fetched; its feature-separation LP is
  the closest published relative of C2.
- Valque L., *3D Snap Rounding*, PhD thesis, Université de Lorraine, Dec 2024.
  Not located on HAL during this lane. It holds the practical guaranteed
  algorithm's failure analysis.
- Trettner P., Nehring-Wirxel J., Kobbelt L., "EMBER: exact mesh booleans via
  efficient & robust local arrangements", ACM TOG 41(4), 2022. The guessed
  author-site URL returned HTML, not a PDF. It is relevant only as a
  plane-based, integer-grid alternative for construction (§4.3), not for
  export.
