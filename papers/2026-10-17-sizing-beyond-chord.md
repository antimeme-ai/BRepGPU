# Sizing beyond the chord rule

Research lane M4-G14 of the fast-mesher campaign, 2026-10-17. Brief:
[docs/fast-mesher-campaign/briefs/M4-G14.md](../docs/fast-mesher-campaign/briefs/M4-G14.md),
read with [M4-common.md](../docs/fast-mesher-campaign/briefs/M4-common.md).
Stub: [lifecycle map §4.1 G14](2026-10-17-geometry-lifecycle-map.md) (stage S19,
with S8, S11, S14). Bead `brepgpu-12s.15`. Nothing was built, run, meshed or
measured. The lane read reference source at pinned revisions, the consumers'
source and documentation, and the papers listed in
[2026-10-17-sizing-beyond-chord-sources.tsv](2026-10-17-sizing-beyond-chord-sources.tsv).

It builds on, and does not repeat:
[fast-tessellation literature](2026-10-17-fast-tessellation-literature.md)
(chord rules, optimal counts, consumer resolutions),
[NVIDIA SmTess study](2026-10-17-nvidia-smtess-study.md) (SMLib pipeline),
[parallel growth clusters](2026-10-17-parallel-growth-clusters.md) (lattice
seams, bands, the grading band remark) and the
[lifecycle map](2026-10-17-geometry-lifecycle-map.md) S8, S11, S14, S19.

Tags: **[src]** a source establishes it; **[code]** read in a reference
implementation or this repository at file:line; **[own]** this lane's
derivation or inference; **[inf]** inferred from a source but not stated by
it. Reference paths are relative to the repository root; `quarantine/` is
ignored and restorable from [QUARANTINE.md](../QUARANTINE.md) and
[reference-catalog.tsv](reference-catalog.tsv).

Scope boundary with G13 (shading fidelity): G13 owns the *value* and visual
basis of any angle or cap rule and the per-format normal policy. This lane owns
*which sizing terms exist*, where they enter the sizing field and shared-edge
sampling, what they cost, and which consumer needs them.

## 0. Decision-relevant findings

1. **The steel-manned NVIDIA baseline is not chord-only. Its SDK default is a
   25° curve and 25° surface angle tolerance with chord off.**
   `SmTessellationDefaults` sets `kChordHeightTol = 0.0`, `kCurveAngleTolDeg =
   25.0`, `kSurfaceAngleTolDeg = 25.0`, `kMaxEdgeLength = 0.0`,
   `kMaxAspectRatio = 0.0`, "shared by the C++ API, the Python bindings and
   hdUsdBrep" [code `quarantine/sources/usd-brep/source/SM_API/inc/SmApiTessellationParams.h:15-24`].
   Our driver initializes omitted angles to 0/0 [code
   `native/om10_nvidia/main.cpp:85,158`], so every recorded NVIDIA watch time is
   a chord-only arm, while design §9.6 specifies NVIDIA "at its default angles
   and the requested chord". A chord-only owned mesher compared against a
   25°-angle NVIDIA arm emits a coarser mesh on every feature with radius below
   about 42ε (§4.1). The comparison is quality-matched only if both sides carry
   the same angle term or both run without it. This is an operator question
   (§8).
2. **Every mature tessellator adds an angle (normal-turning) term to the
   chord; none of them adds grading by default except the FEM meshers.**
   OCCT `Angle = 0.5` rad on edges and on analytic surface steps, and
   `AngleInterior = 2·Angle` on interior links [code IMeshTools_Parameters.hxx,
   BRepMesh_IncrementalMesh.hxx:104-106]; SMLib 25°/25°; Parasolid
   `curve_chord_ang` and `surface_plane_ang`; Gmsh `MeshSizeFromCurvature`
   (elements per 2π) and its OCC STL path `StlAngularDeflection = 0.3` rad;
   Netgen `curvaturesafety = 2`, which works out to ≈0.5 rad per element.
   Grading exists only where elements feed a PDE solver: Netgen
   `grading = 0.3`, MMG `hgrad = 1.3`, and Gmsh's `Extend` field and
   `MeshSizeExtendFromBoundary`. Parasolid, SMLib and OCCT have no grading
   parameter at all (§2).
3. **For the alpha consumers, ε alone is sufficient except for the renderer.
   The renderer needs a per-segment angle cap.** PrusaSlicer and Cura slice
   triangle by triangle and detect overhangs from layer outlines, not facet
   normals. Each simplifies its own slice paths at 0.0125 mm (PrusaSlicer
   G-code resolution) or 0.5 mm length / 0.025 mm deviation (Cura).
   fTetWild simplifies its input inside 0.8 of its own envelope before
   inserting anything, and sizes tetrahedra by its own target length ℓ. The
   renderer's silhouette and shadow-terminator artefacts depend on how far the
   normal turns per facet. That is a scale-free angle; chord error is not
   scale-free. PhysX, which comes with physics, needs a maximum edge length
   (§3).
4. **Grading is a large cost that buys nothing the named consumers use.**
   A smooth size transition from boundary spacing s at slope g costs about
   (4/√3)/g triangles per boundary sample [own, §4.3]. That is 7.7 at Netgen's
   0.3 and 8.8 at MMG's ln 1.3. A boundary-only plane costs about one triangle
   per sample. Planes are 40% of OM10 faces. Without grading, a face's
   interior depends only on its own carrier and ε. An edit or a re-sharpened ε
   then invalidates only the incident faces, through the shared edges. With a
   grading field, influence spreads over the grading distance and across
   non-adjacent faces (Netgen's octree is global) (§4.4).
5. **The angle term needs one quantity the design does not compute: a lower
   bound on the first derivative per B-spline span.** Normal turning across a
   triangle is bounded by the maximum principal curvature times the triangle
   diameter. On freeform spans that curvature needs ‖D²S‖ over a lower bound on
   the singular values of DS. Design §6.1 keeps only `D_uu, D_uv, D_vv`. Every
   reference evaluates curvature at sample points instead: Netgen at triangle
   corners and centroid, Gmsh at a point, OCCT at link ends. SMLib uses
   control-polygon turning angles, and none of the sources read shows that
   test is a bound. For analytic carriers the angle term is closed form
   (§4.1) [own].

## 1. What each consumer needs from triangle sizes and shapes (question 1)

### 1.1 PrusaSlicer (tag `version_2.9.6`)

- **Storage.** Vertices are float32 (`typedef Eigen::Matrix<float,3,1>
  stl_vertex`) [code `bundled_deps/admesh/admesh/stl.h:42`].
- **Repair on load.** `TriangleMesh::repair` merges vertices that match
  exactly. Only if edges are still unconnected does it run a nearby merge, two
  iterations growing by bounding diameter / 10000. It then removes unconnected
  facets, fixes normal directions and values, and flips everything if the
  volume is negative. Hole filling is compiled out: "the current algorithm does
  more harm than good … let the slicing algorithm close gaps in 2D slices"
  [code `src/libslic3r/TriangleMesh.cpp:120-196`]. Degenerate facets, meaning
  two identical vertex indices after the exact merge, are removed
  [code `TriangleMesh.cpp:790-810`; `admesh/connect.cpp:441-452`].
- **Slicing.** Each facet is intersected with the layer plane. Horizontal
  facets are skipped ("any valid horizontal triangle must have a vertical
  triangle connected") [code `TriangleMeshSlicer.cpp:448-449`]. Degenerate
  facets are kept on purpose: "we may want to slice the degenerate triangles
  because of the connectivity information they carry" [code
  `TriangleMeshSlicer.cpp:534-535`]. The facet normal is used only to classify
  a facet Up, Down, Vertical or Degenerate in slab projection [code
  `TriangleMeshSlicer.cpp:528-536`]. A grep of the FDM support generator found
  no facet-normal use; its overhangs come from layer polygons [code
  `src/libslic3r/Support/SupportMaterial.cpp`].
- **Resolutions that discard detail downstream.** `slice_closing_radius`
  0.049 mm closes cracks under 2× that radius; `gcode_resolution` 0.0125 mm;
  `resolution` (input simplification) 0 by default [code
  `src/libslic3r/PrintConfig.cpp:2739-2758, 3275-3283`].
- **Sizing statement [own].** No maximum edge length, grading or minimum
  angle. Slicing time is linear in triangle count, so the fewest triangles
  that meet ε are best. Shape matters in one case only: a triangle whose
  distinct vertices round to the same float32 point becomes degenerate and is
  dropped. That is the export-rounding problem of S13, not sizing.

### 1.2 Cura (CuraEngine `5.11.0`, Cura `5.11.0` `fdmprinter.def.json`)

- **Vertex meld on load: 0.03 mm.** `const int vertex_meld_distance =
  MM2INT(0.03)`. A face whose vertices meld to the same point is discarded:
  "the face has two vertices which get assigned the same location. Don't add
  the face" [code `src/mesh.cpp:15, 40-52, 132-145`]. Any edge shorter than
  30 µm therefore collapses inside Cura, whatever we emit.
- **Downstream resolutions.** `meshfix_maximum_resolution` 0.5 mm (minimum
  segment length after slicing), `meshfix_maximum_deviation` 0.025 mm,
  `minimum_polygon_circumference` 1.0 mm, `meshfix_union_all` true,
  `meshfix_extensive_stitching` false [src `resources/definitions/fdmprinter.def.json`].
- **Overhangs.** `support_angle` (default 50°) is applied to layer outlines:
  `computeBasicAndFullOverhang` offsets the layer below by
  tan(angle)·layer_height and subtracts, with no facet normals [code
  `src/support.cpp:1458-1480`].
- **Sizing statement [own].** ε only. Features under 30 µm (meld) and loops
  under 1 mm circumference are Cura's own losses. D-features forbids us from
  pre-coarsening to match them. The mesher can report counts of edges shorter
  than 0.03 mm and closed loops under 1 mm when a Cura target is named.

### 1.3 glTF renderer (spec 2.0.1; renderer not yet named)

- glTF requires flat normals when none are given: "client implementations
  MUST calculate flat normals" [src glTF 2.0 §3.7.2.1]. Shading interpolates
  vertex normals barycentrically across the triangle [src Hanika 2021 §4.1].
- **Silhouette.** An outline's deviation is a chord error in screen space:
  ε × (pixels per mm). At 0.005 mm on a 3840-px poster of a ~40 mm watch it is
  about 0.5 px. Zoomed until a 0.1 mm pin fills 1000 px, it is about 50 px,
  and the pin's 7-segment outline (§4.1) is visible [own]. Chord error is
  view-dependent; the per-segment turning angle is the scale-free measure of
  how polygonal an outline looks [own].
- **Shadow terminator.** Hanika: coarse tessellation "is often compensated by
  using smooth shading, employing interpolated vertex normals. This mismatch
  between geometric normal and shading normal causes various issues for light
  transport", and the obvious remedy is to "resolve issues with coarse
  tessellation by tessellating more finely" [src Hanika 2021 abstract, §4.2].
  The mismatch is set by the normal turning across one facet, not by the sag.
  Exact per-corner normals make the mismatch more visible, not less [own].
- **Long thin triangles.** On cylinders and cones the exact normal is constant
  along a ruling, so interpolation along a long strip triangle is exact [own].
  On planes nothing varies. Doubly curved surfaces are already capped by the
  chord rule. A maximum edge length buys the renderer nothing [own]. Parasolid
  documents one exception: a large facet spanning an inflection that passes
  the tolerance and angle tests but "looks flat … if the graphics library uses
  the normals at the facet vertices" [src Parasolid FD §107.2.17]. That
  failure comes from sampled acceptance tests. An a-priori bound taken from
  the span's sup of second derivatives sizes the inflection region by its
  largest curvature, so it does not arise [own].
- **Sizing statement [own].** ε plus a per-segment angle cap θ_max on curves
  and on surface normal turning. G13 sets the value. Mature defaults are
  15° (Mechanical Desktop STL export, per Stratasys 2015), 0.3 rad ≈ 17°
  (Gmsh OCC STL), 25° (NVIDIA SDK) and 0.5 rad ≈ 28.6° (OCCT, Netgen).
  No maximum length; no grading. View-dependent extra density (Parasolid's
  `density` option, §2.3) belongs to sharpen-in-place, not to the default mesh.

### 1.4 fTetWild (`8118f81`), the analysis stand-in

- **Envelope.** Defaults `eps_rel = 1e-3` and `ideal_edge_length_rel = 1/20`,
  both of the bounding-box diagonal [code `src/Parameters.h:59-62, 110-125`].
  The paper states the role: "the ϵ-envelope represents the maximal deviation
  from the input surface the user is willing to accept. For instance, in
  additive manufacturing applications ϵ can be the machining precision"
  [src Hu et al. 2020 §3].
- **Input handling.** Duplicate vertices are merged and duplicate faces
  removed (`remove_duplicates`). The soup is then simplified by edge collapse
  and swaps inside `eps_simplification = 0.8·eps` [code
  `src/Simplification.cpp:25-60, 133-200`; `Parameters.h:144`]. The paper:
  "self-intersections, degenerate and near-degenerate faces and gaps contained
  in the envelope are automatically removed", and the preprocessing eliminates
  "near-degenerate or overly refined triangles in the input" [src Hu et al.
  2020 §2, §3, §3.3].
- **Output density** is set by ℓ and an optional background sizing field
  (`--bg-mesh`), not by input triangle size. fTetWild "aim[s] to generate
  as-coarse-as possible meshes". TetGen, which "preserves the input surface
  geometry exactly", by contrast "generates a dense tetrahedral mesh around
  the surface if the input surface mesh is dense" [src Hu et al. 2020 §4.4;
  code `src/main.cpp:254`].
- **Sizing statement [own].** ε only. The analysis deviation of the tet
  boundary from the exact solid is at most ε + ε_env. At fTetWild's default
  ε_env = 1e-3·d, the envelope dominates any request ε = 0.005 mm once d > 5 mm.
  The mesher should report ε + ε_env when fTetWild is the target and leave
  ε_env to the user. Input shape and density are irrelevant to the output; they
  cost only fTetWild's simplification time. Watertight, oriented, closed input
  matters for its winding-number filtering, and the topology contract already
  gives that.
- **Not the named consumer, recorded for scope.** A surface-preserving volume
  mesher (TetGen `-p`) or direct shell/BEM analysis turns surface triangle size
  and shape into element size and shape. That would need a CAE profile with a
  maximum length, a minimum angle and grading (§4.3 costs). D-consumers names
  fTetWild, so that profile is out of alpha.

### 1.5 PhysX (with physics; tag `107.3-physx-5.6.1`)

- `meshEdgeLengthMaxLimit` defaults to 500, multiplied by
  `PxTolerancesScale::length` (default 1). An edge above it raises a warning
  only: "corresponding triangles are not removed" [code
  `physx/include/cooking/PxCooking.h:315-331, 361-372`;
  `physx/include/common/PxTolerancesScale.h:78-92`]. The cooking result
  `eLARGE_TRIANGLE` warns of "large triangles, which should be tessellated to
  ensure better simulation and CCT stability" [src PhysX 5.6.1 Geometry docs].
- Cleaning removes triangles with area below `meshAreaMinLimit` (default 0:
  exactly zero only). Welding is snap-to-grid at `meshWeldTolerance` (default
  0), and validation warns when two vertices fall within it [code
  `PxCooking.h:280-312`]. Convex meshes are limited to 255 vertices and faces;
  dynamic triangle meshes need an SDF [src Geometry docs].
- **Sizing statement [own].** ε plus a maximum edge length L_max ≤
  500·`length` in the scene's PhysX units. Contact fidelity (G17, D-imprint)
  is separate and later.

## 2. What Netgen, Gmsh, Parasolid and the CAD tessellators add beyond chord error (question 2)

### 2.1 Netgen (`31e85f8`)

- **No sag tolerance exists.** Every term is a size h:
  - **Edge length:** h ≤ len/`segmentsperedge` (default 1) along each edge
    [code `libsrc/occ/occgenmesh.cpp:573-588`].
  - **Edge and face curvature:** `ComputeH` returns
    max(minh, maxh·κ < 1 ? maxh : 1/κ) with κ = curvature·`curvaturesafety`,
    i.e. h = R/curvaturesafety (default 2) [code `occgenmesh.cpp:61-79`].
  - **Face curvature evaluation.** Max |principal curvature| is sampled at
    the corners and centroid of an OCCT triangulation, recursively, and
    skipped when curvature < 1e-3 in model units [code
    `occgenmesh.cpp:120-170, 640-705`].
  - **Close edges:** edges are cut into ≤100 pieces at 10° turning, and h ≤
    distance/`closeedgefac` between non-adjacent pieces, floored at 1e-3 of
    the box. The OCC default is `closeedgefac = 2` [code
    `occgenmesh.cpp:713-830`; `python_occ.cpp:40-43`].
  - **maxh, minh,** per-solid, per-face and per-edge maxh, and user points
    [code `occgenmesh.cpp:480-540`].
- **What the curvature rule means [own].** A chord of length R/2 subtends
  about 0.5 rad. The default is an angular criterion of roughly 28.6° per
  element, and its sag grows with R as R(1 − cos 0.25) ≈ 0.031 R: 31 µm at
  R = 1 mm, 3.1 mm at R = 100 mm.
- **Grading.** `LocalH::SetH` stores h in an octree cell and pushes
  h + grading·h_box to the face neighbours, recursively [code
  `libsrc/meshing/localh.cpp:191-255`]. The size field is Lipschitz with slope
  about `grading` (default 0.3). It is a 3D field, so it grades across faces
  and through space, not only along a surface.
- **Presets** run from very_coarse (curvaturesafety 1, grading 0.7,
  closeedgefac 0.5) to very_fine (5, 0.1, 5) [code `python/meshing.py:4-60`].
- **Why.** Every term serves finite elements: elements per radius, a smooth
  size field, enough elements across a gap. The parameter comment reads
  "safety factor for curvatures (elements per radius)" [code
  `libsrc/meshing/meshtype.hpp:1853-1856`].

### 2.2 Gmsh (`b4e84a4`)

- **Size at a point.** The minimum of: the model bounding-box size; sizes
  given at points (`MeshSizeFromPoints = 1`); curvature size if
  `MeshSizeFromCurvature > 0`; the background field; and per-entity sizes.
  The result is clamped to [`MeshSizeMin` 0, `MeshSizeMax` 1e22] and scaled
  by `MeshSizeFactor` [code `src/mesh/BackgroundMeshTools.cpp:211-281`;
  manual §"Specifying mesh element sizes"].
- **Curvature term.** lc = 2π/(κ·N), N elements per 2π, off by default (0)
  [code `BackgroundMeshTools.cpp:48-72`; `src/common/DefaultOptions.h:1449-1454`].
  It is an angle criterion, 2π/N per element. Gmsh has no chord or sag control
  for its own meshes. Its OCC STL path does: `StlLinearDeflection = 0.001`
  relative to curve length, and `StlAngularDeflection = 0.3` rad
  [code `DefaultOptions.h:1765-1773`].
- **Extend from boundary (default 1).** Boundary vertex sizes are interpolated
  into the interior as points are inserted, and each new triangle takes
  min(interpolated boundary size, background size) [code
  `src/mesh/meshGFaceDelaunay.cpp:530-542, 951-963`]. The manual warns this
  causes "unintentional refinement in nearby entities" and advises turning it
  off when a field specifies sizes.
- **Explicit grading fields.** `Extend`: F = min(SizeMax, Σw_i / Σ(w_i/s_i))
  with w_i = 1/d_i² and s_i = SizeBnd_i + (Ratio − 1)·d_i, default Ratio 1.2
  [code `src/mesh/Field.cpp:2659-2700`]. `Threshold`: linear or sigmoid ramp
  from SizeMin to SizeMax between DistMin and DistMax of a Distance field
  [code `Field.cpp` ThresholdField, around 650-700]. `SmoothRatio` 1.8 applies
  to BAMG only [code `DefaultOptions.h:1763`].
- **Minimum counts.** 7 nodes per circle and 3 per curve
  [code `DefaultOptions.h:1489-1495`]; `ToleranceEdgeLength` skips model edges
  shorter than a tolerance (default 0) [code `DefaultOptions.h:1800`].
- **Why.** Same as Netgen: the sizes are element sizes for a solver. The
  growth ratio in `Extend` is "Element size growth ratio (>= 1)".

### 2.3 Parasolid (V35 Functional Description, ch. 107)

- **Curve terms:** `curve_chord_tol` (distance), `curve_chord_max` (maximum
  chord length) and `curve_chord_ang` (sum of the chord-to-tangent angles at
  both ends, ≤ π/2) [src FD §107.2.8].
- **Surface terms:** `surface_plane_tol` and `surface_plane_ang`, the latter
  "the sum of the angular deviation between the surface normal and facet
  planar normal at any two positions within the facet boundary" [src §107.2.9].
- **Size terms:** `max_facet_width` and `min_facet_width`. The minimum
  "overrides other input tolerance values": refinement stops below it "regardless
  of the other criteria" [src §107.2.7, §107.2.12].
- **Defaults are internal.** "If no explicit tolerance value is provided,
  Parasolid is free to calculate its own internal tolerance values. These are
  generally case-specific" [src §107.1.3, §107.2.8-9].
- **Local tolerances.** Per-face or per-body overrides of the curve and
  surface tolerances [src §107.2.10].
- **Display options.** View-dependent density at silhouettes or near-parallel
  facets (`density`), extra splits around inflections (`inflect`), and a
  `quality` option whose extra checks are for "facets for purposes other than
  visualisation" [src §107.2.3, §107.2.17, §107.2.18].
- **No grading or proximity parameter** appears in ch. 107. Small-feature
  ignoring (§107.2.15) is defeaturing, which is G10's subject.

### 2.4 The CAD tessellators: OCCT 8.0.1 and NVIDIA SMLib

- **OCCT parameter defaults.** `Deflection 0.001`, `Angle 0.5`,
  `AngleInterior −1 → 2·Angle`, `DeflectionInterior → Deflection`,
  `MinSize −1 → 0.1·min(deflections)` ("to prevent sinking into amplification
  in case of distorted curves and surfaces"), `ControlSurfaceDeflection true`,
  `AdjustMinSize false`, `Relative false` [code
  `src/ModelingAlgorithms/TKMesh/IMeshTools/IMeshTools_Parameters.hxx`;
  `BRepMesh/BRepMesh_IncrementalMesh.hxx:86-106`].
- **OCCT curves.** Curves are discretized by `GCPnts_TangentialDeflection`
  with 0.5·Angle and 0.5·Deflection [code
  `BRepMesh_CurveTessellator.cxx:73-122`].
- **OCCT analytic surfaces.** The parameter step is
  Δu = max(min(2·acos(1 − d/R), Angle), MinSize/R) [code
  `src/ModelingData/TKGeomBase/GCPnts/GCPnts_TangentialDeflection.cxx:495-517`;
  `BRepMesh_CylinderRangeSplitter.cxx:29-31`]. The cylinder's along-ruling
  node count is commented out, so OCCT emits pure strips too [code
  `BRepMesh_CylinderRangeSplitter.cxx:50-62`]. Cones keep a logarithmic
  v-subdivision, ratio max(1, ln(Δv/(Δu·R))) [code
  `BRepMesh_ConeRangeSplitter.cxx:44-49`].
- **OCCT interior control.** Deflection is tested at triangle centroids and
  link midpoints, and the angle between surface normals at link ends is held
  to AngleInterior [code `BRepMesh_DelaunayDeflectionControlMeshAlgo.hxx:138-226, 351-368`].
- **SMLib parameters.** Chord, curve angle, surface angle, max edge length (a
  negative value means 0.025·|v|·bbox diagonal) and aspect ratio [code
  `SM_API/src/SmApiBrep.cpp:686-721`].
  - The surface angle test is the maximum end-tangent turning of the
    control-net iso-polygons in u and v ≤ tolerance. It falls back to surface
    evaluation above 540° [code `SMLib/src/SmSurfaceCache.cpp:3665-3740`].
  - The curve angle test is the accumulated turning of the leaf control
    polygon [code `SmCurveCache.cpp:712, 864-887, 1047-1048`].
  - Max side length and aspect add splits only after chord and angle pass
    [code `SmSurfaceCache.cpp:3778-3890`].
  - Max edge length is "an edge-length subdivision target … not a hard cap"
    [code `SmApiTessellationParams.h:20-22`].
  - The only grading-like rule is the 1.1× UV-size edge split described in
    the SmTess study.

### 2.5 Why they add these: synthesis

| Term | Who has it | Default | Stated or evident purpose |
|---|---|---|---|
| Angle per segment / normal turning | OCCT, SMLib, Parasolid, Gmsh (curvature, STL), Netgen (curvaturesafety) | 0.5 rad; 25°; internal; off / 0.3 rad; ≈0.5 rad | Visual smoothness and minimum points on small radii; for FEM, elements per radius [src §2.1-2.4] |
| Max edge length | SMLib, Parasolid (max chord, max facet width), Netgen/Gmsh maxh, PhysX check | off; off; 1e10 / 1e22; 500·length | FEM element size; physics contact stability [src §1.5, §2] |
| Min size | OCCT MinSize, Parasolid min width, Netgen minh, SMLib min UV ratio | 0.1·d; off; 0; 0.001 of domain | Stop runaway refinement on distorted geometry [src OCCT comment; Parasolid §107.2.12] |
| Grading | Netgen, MMG, Gmsh Extend / ExtendFromBoundary | 0.3; ln 1.3 = 0.262 per length; 1.2 / on | FEM element quality [src §2.1-2.2; MMG `src/common/mmgcommon_private.h:122`, `isosiz.c:298-345`] |
| Proximity | Netgen close edges | factor 2 | Elements across thin regions for FEM [src §2.1] |
| Aspect | SMLib (off), Mechanical Desktop STL (off) | 0 | Shape for downstream; off by default [src] |

MMG's rule is h(b) ≤ h(a) + ln(1.3)·|ab|: the user's 1.3 is stored as
`MMG5_HGRAD 0.26236426446` and applied as a slope [code
`quarantine/sources/mmg/src/common/mmgcommon_private.h:122`;
`src/common/isosiz.c:298-345`]. Netgen's 0.3 and MMG's 0.262 agree: FEM
practice grades sizes at a Lipschitz slope near 0.3.

**[own] reading.** The CAD tessellators, the ones built to make display and
export meshes, add an angle term and a min-size stop and leave grading out.
The FEM meshers add grading and proximity, and two of them have no chord
term at all. Our consumers are display and export consumers plus one FEM
mesher, fTetWild, that remeshes its input anyway.

## 3. Grading at a fine/coarse boundary, shared edges and growth clusters (question 3)

1. **The shared edge already grades, one-sidedly.** Design §5.2 item 1 samples
   each edge at the maximum density over the curve's own rule and every
   incident face's admissible length. The fine face imposes its density on the
   edge. The coarse face inherits fine boundary samples and absorbs the jump
   inside its own triangulation. NVIDIA does the same with its 1.1× UV-size
   edge split. Graded PD³ bounds the jump at separators by a neighbour ratio
   R2 = 1.5 [src parallel-growth-clusters §1, §4].
2. **ε does not need the coarse face to grade [own].**
   - **Planes:** any triangulation of the boundary chain has zero interior
     deviation.
   - **Cylinders and cones:** a triangle spanning no more than Δθ_max lies
     within ε whatever its shape. Extra boundary samples between θ-slab lines
     make fans inside a slab, which still meet ε.
   - **B-spline lattice plus band:** band refinement already sizes every
     emitted triangle by the face's own admissible length (design §5.3, "every
     emitted edge … is sized by the face's local admissible length"). A band
     between fine trim samples and a coarse lattice front is ε-correct without
     grading. Its triangles are fans: one coarse front vertex sees
     about h_c/s_f fine trim samples.
3. **What the ungraded transition costs in shape [own].** Fans produce
   needles, not caps, whenever the apex is far from the fine boundary. Caps
   appear where nearly collinear boundary samples bound a locally convex
   region, as in the ear triangles of a boundary-only disc. Needles are
   harmless to every named consumer (§1). Caps matter only for normals
   reconstructed from facets, and that rule is G13's (design §4.7, the 150°
   rule). The growth-cluster band quality hypothesis H2′ is the measurement
   that decides it.
4. **Lattice grading toward a trim.** The 2:1 balance of the dyadic quadtree
   is a fixed grading of ratio 2 per level. It is what makes seam vertices
   integer-keyed. Grading the level function toward a fine trim, the
   "log-Lipschitz bound" the growth-cluster lane left unproven, would shorten
   band fans. It costs about 2.3 extra triangles per trim sample (§4.3,
   g ≈ 1) and more Balance work, which p4est reports as the dominant seam cost.
   [own on cost; src parallel-growth-clusters §0 item 3, §4]
5. **Seams and clusters are indifferent to grading.** Keys depend on lattice
   coordinates and the neighbour levels read through the ghost layer. Grading
   changes neither ownership nor the metamorphic oracle mesh(K=1) ≡ mesh(K=k).
   Band separators end on existing trim samples, and a fine trim offers more
   endpoints. [own; src parallel-growth-clusters §3]
6. **Incremental remeshing argues against a global grading field [own].**
   - Without grading, a face's interior is a function of its carrier and ε,
     and its boundary is the shared-edge maximum over the incident faces.
     Changing ε on one face, or editing it, changes only its edges and
     therefore only its incident faces.
   - Netgen's octree LocalH and Gmsh's fields are global 3D fields computed
     before meshing. Grading at slope g carries a local fine size a distance of
     about (H − s)/g into neighbours and across non-adjacent faces.
   - Such a field is compatible with parallel face meshing, because it is
     static once built. It is not compatible with sharpen-in-place or the
     complication roadmap's local edits without re-deriving the field.

## 4. Cost of each sizing term (question 4)

Triangle count is the cost unit here. Time per triangle is ours to measure;
§6 names the measurements.

### 4.1 Angle cap θ_max

- **Analytic closed forms [own].**
  - Circle, cylinder, cone slab: Δθ = min(2·acos(1 − ε/R), θ_max).
  - Sphere: circumradius ρ ≤ min(√(2Rε − ε²), R·sin(θ_max/2)).
  - Torus: the chord rule's curvature with 1/r and |cos φ/(R_c + r cos φ)|
    gives h ≤ θ_max/κ_max.
  - The angle term binds only where 2·acos(1 − ε/R) > θ_max, that is
    R < ε/(1 − cos(θ_max/2)):

| θ_max | R below which it binds | Min segments per circle | Relative sag at the cap |
|---|---|---:|---:|
| 15° | 116.9 ε | 24 | 0.86% |
| 0.3 rad (17.2°) | 89.1 ε | 21 | 1.12% |
| 25° | 42.2 ε | 15 | 2.37% |
| 0.5 rad (28.6°) | 32.2 ε | 13 | 3.11% |

- **Full-circle segment counts, chord rule versus chord plus 25° [own]:**

| R (mm) | ε = 0.005 chord | + 25° | ε = 0.025 chord | + 25° |
|---|---:|---:|---:|---:|
| 0.05 | 7 (51.7°) | 15 | 3 (120°) | 15 |
| 0.1 | 10 (36.4°) | 15 | 5 (82.8°) | 15 |
| 0.25 | 16 | 16 | 7 (51.7°) | 15 |
| 0.5 | 23 | 23 | 10 (36.4°) | 15 |
| 1 | 32 | 32 | 15 | 15 |
| 2 | 45 | 45 | 20 | 20 |

- **Cost.** The extra triangles are confined to features of radius under
  42ε (0.21 mm at ε = 0.005; 1.05 mm at ε = 0.025). There at most 15/n_chord
  times as many segments around, and nothing changes elsewhere. The increase is
  exactly predictable from carrier radii before meshing: one census over
  sources, no meshing. Freeform spans need the first-derivative lower bound of
  finding 5. Near poles that bound goes to zero while the 3D curvature stays
  finite, so a parameter-space normal bound is the wrong formulation there.
  Bound the 3D normal turning instead, which needs principal-curvature bounds
  per span [own]. The literature read gives no a-priori per-span bound on
  principal curvature for rational B-splines. This lane does not settle it;
  §6 names the work.

### 4.2 Maximum edge length L_max

- Planes become N ≈ (4/√3)·A/L_max² + O(boundary) triangles instead of about
  one per boundary sample. Cylinder strips get ⌈ℓ/L_max⌉ ruling splits, giving
  2n·⌈ℓ/L_max⌉ triangles [own].
- For the physics profile, L_max = 500·`PxTolerancesScale::length`. On a watch
  in mm with length 1 that is 500 mm and never binds [own]. No alpha consumer
  needs L_max (§1).

### 4.3 Grading at slope g

- Size grows from boundary spacing s as h(x) = s + g·x up to H. Equilateral
  triangles of area (√3/4)h² over a boundary of length L give
  N = (4/√3)(L/g)(1/s − 1/H) ≈ (4/√3)·n_f/g, where n_f = L/s boundary samples
  [own]. For a closed loop this is an upper bound, since the zone shrinks
  inward.

| g | Source of the value | Triangles per boundary sample |
|---|---|---:|
| 1 (2:1 quadtree, size doubles per cell) | lattice balance | 2.3 |
| 0.3 | Netgen default | 7.7 |
| 0.262 | MMG default (ln 1.3) | 8.8 |
| 0.2 | Netgen fine | 11.5 |

- A boundary-only plane CDT has n − 2 triangles for n samples [own]. Graded
  plane interiors therefore cost about 8× the plane triangle count at FEM
  defaults. This is consistent with Delaunay refinement's size-optimal
  O(∫ dA/lfs²) counts, a classical result (Ruppert 1995) not read in this
  lane.

### 4.4 Proximity sizing with factor c

- A strip of width d and length ℓ meshed with c elements across costs about
  (4/√3)·c²·ℓ/d triangles, against about 2ℓ/s boundary-only [own]. No named
  consumer needs it.
- The design's ε-inflated thin-feature query (§5.6) is an embedding rule. It
  tightens local tolerance where clearance is below 2ε; it is not a sizing
  field. Its cost belongs to G-lanes on embedding and export rounding.

### 4.5 Minimum size

No triangle cost. In OCCT and Parasolid it is a silent stop: refinement ends
"regardless of the other criteria" [src Parasolid §107.2.12]. Design §2
forbids silent accepts, so a minimum size may exist only as a geometry-derived
termination bound. Reaching it yields `valid_goal_unmet` or `refused`, never
`valid` (design §5.3).

## 5. Sizing statement per consumer profile

The admissible length at a point on a face is the minimum of the terms the
profile enables. Edge samples take the minimum over the curve's own terms and
every incident face's terms, as design §5.2 already does for ε.

| Profile | Consumers | Terms | Not used, and why |
|---|---|---|---|
| **Fabricate** | PrusaSlicer, Cura (STL, 3MF) | ε | No L_max: slicing is per facet and linear in count. No grading or minimum angle: needles and zero-area facets are tolerated or kept (PrusaSlicer), and overhangs come from layer outlines. Report edges under 0.03 mm and loops under 1 mm circumference for Cura targets; never coarsen (D-features) |
| **Render** | one glTF renderer | ε + θ_max on curves and on surface normal turning (value from G13; mature range 15°–28.6°; 25° matches the NVIDIA default) | No L_max: exact normals interpolate exactly along rulings and planes. No grading. View-dependent density is sharpen-in-place, not default |
| **Analyse** | fTetWild | ε; report ε + ε_env with ε_env = fTetWild's envelope (default 1e-3·d) | Input size and shape are simplified away inside 0.8·ε_env; output size is fTetWild's ℓ. A surface-preserving FEM consumer (TetGen-class) would need a CAE profile (L_max, minimum angle, grading at g ≈ 0.3, ~8× plane triangles); out of alpha |
| **Simulate** (later) | PhysX | ε + L_max ≤ 500·`PxTolerancesScale::length`; welded; no exactly-zero-area triangles | Contact discretization is G17 / D-imprint |
| **Compare** | NVIDIA steel-man arm | Same terms as the NVIDIA arm: chord only against 0/0 angles, or ε + 25° on curves and surfaces against the SDK default | Quality must match; operator question 1 |

Every profile keeps the same topology contract (shared edges, seams,
orientation). Profiles change only sample density. Two profiles of one source
differ only in the edge chains and faces the extra term touches, so the
general admitted path and its oracles do not fork [own].

## 6. Hypotheses, falsifiers and the measurements that would settle them

1. **H-angle-cost.** On OM10 and the NIST/CAx-IF corpus at ε = 0.005 mm, a 25°
   angle term raises the total triangle count by under 10%, and the increase
   equals the closed-form prediction from carrier radii on analytic faces.
   - Measurement: a census of cylinder, cone, torus and sphere radii below
     42ε per input, read from decoded sources on the fleet; no meshing.
   - Falsifier: the census predicts ≥ 10%, or emitted counts differ from the
     prediction on analytic faces.
2. **H-no-grade-slicer.** PrusaSlicer and Cura G-code from chord-only meshes
   equals G-code from graded meshes of the same parts at the same ε, within
   the slicers' own resolutions (0.0125 mm and 0.025 mm).
   - Measurement: slice authored families both ways.
   - Falsifier: any path difference above those resolutions.
3. **H-no-grade-ftetwild.** fTetWild's tet count, minimum dihedral angle
   histogram, insertion failures and run time do not differ by more than
   run-to-run spread between chord-only and graded inputs at the same ε and
   envelope.
   - Falsifier: any un-inserted faces, or a quality shift beyond spread.
4. **H-grade-cost.** A Delaunay-refined plane graded at slope g has
   (4/√3)·n/g triangles within a factor of 2.
   - Falsifier: counts outside that band. Relevant only if a CAE profile is
     ever named.
5. **H-local-invalidation.** Without grading, changing ε on one face changes
   the keyed triangle sets of that face and its edge-incident faces only.
   - Oracle (metamorphic): every other face is identical by key.
   - Falsifier: any other face changes.
6. **Renderer value of θ_max.** That belongs to G13's reflection-line and
   silhouette renders. This lane predicts visible polygonal silhouettes on
   R < 42ε features at zoom with θ off.

Not settled by the literature read:

- an a-priori per-span principal-curvature bound for rational B-splines, which
  the freeform angle term needs (§4.1). It needs an owned derivation and its
  test oracle: dense evaluation of normal turning per emitted triangle;
- whether SMLib's control-polygon turning test bounds the true turning
  (needed for a fair comparison of angle semantics).

## 7. Contradictions found

- **Design §6.1 versus §9.6.** The density rules are chord-only, while the
  NVIDIA arm is specified "at its default angles", which are 25°/25° (finding
  1). The recorded runs used 0/0 (`native/om10_nvidia/main.cpp:85`).
- **Design §5.3 versus mature practice on cylinders.** There is no
  contradiction: OCCT also dropped along-ruling nodes on cylinders
  (`BRepMesh_CylinderRangeSplitter.cxx:56-62`, commented out).
- **Lifecycle S14 "CAE consumers want a cap".** That is true for
  surface-preserving FEM. It is not true for fTetWild, the operator's named
  analysis stand-in (§1.4).

## 8. Operator questions

1. **NVIDIA angle arm.** Design §9.6 says "default angles" (SDK 25°/25°),
   while our driver and every recorded run use 0/0. Which arm or arms are the
   steel-man? The owned mesher must carry the matching term for the speed
   ratio to compare like meshes. Options:
   - (a) both chord-only;
   - (b) both ε + 25° (SDK default);
   - (c) both, reported separately.
2. **Is the angle term in the default output, or only in the render profile?**
   G13 sets its value. Whether STL/3MF exports for slicers carry it changes
   their triangle counts on sub-0.2 mm features (§4.1).
3. **Which glTF renderer** (D-consumers leaves it unnamed). A ray tracer shows
   the shadow terminator; a rasterizer shows mainly silhouettes. The angle
   value G13 picks may depend on it.

## Defects found

None in our code by reading. One discrepancy is listed for the dispatcher:
`native/om10_nvidia/main.cpp:85` defaults omitted curve and surface angles to
0, not to the SDK's `SmTessellationDefaults` 25°/25°. AGENTS.md records this as
the intended legacy 0/0 setting. It conflicts with design §9.6's "default
angles", and operator question 1 decides which is right.

## For SHOPPING_LIST.md

- Borouchaki H., Hecht F., Frey P.J., "Mesh gradation control", IMR 1997 /
  IJNME 43 (1998). The canonical metric gradation paper; not found open. MMG
  implements its isotropic rule, which this lane read in code.
- Laug P., Borouchaki H., "Surface meshing with metric gradation control"
  (2012; HAL hal-02285955 record, full text behind Civil-Comp).
- Ruppert J., "A Delaunay refinement algorithm for quality 2-dimensional mesh
  generation", J. Algorithms 18 (1995): size optimality, cited from standard
  knowledge in §4.3, not read.
- Chiang M.J.-Y., Li Y., Burley B., "Taming the shadow terminator", SIGGRAPH
  2019 Talks: not acquired.
- Karis B. et al., "Nanite: a deep dive" (SIGGRAPH 2021 course), for
  rasterizer cost of tiny triangles: not read. No quantitative
  tiny-triangle cost is claimed here.

## Acquisition notes

- Two PDFs were added to ignored `papers/pdfs/`; hashes are in the sources TSV.
  - `hu-schneider-wang-zorin-panozzo-2020-ftetwild-1908.03581.pdf` is
    75,127,028 bytes, large for the laptop's free space. It was fetched before
    its size was known; §2-4 were read.
  - `hanika-2021-hacking-shadow-terminator-rtg2.pdf` is 1.2 MB.
- PrusaSlicer `version_2.9.6`, CuraEngine and Cura `5.11.0`, and PhysX
  `107.3-physx-5.6.1` files were read from GitHub raw URLs at those tags into
  a session scratch directory outside the repository. They are not
  quarantined; a later lane that needs them durably should add them to the
  quarantine manifest.
