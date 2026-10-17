# Complete OCCT watch reference images

2026-10-17. Short PLAN; parent review before code. This is an external OCCT CPU
reference visualization, never owned BRepGPU meshing or NVIDIA output. OCCT can
also fail. A requested deflection is not a proved whole-surface error allowance.

Use the existing original STEP exporter unchanged: one fresh OCCT transfer with
shape processing disabled, original107 definitions/9162 faces and166 placed
occurrences, original face IDs bound to BinTools storage indices. No STEP/BRep
repair, source modification, dropped definitions or replacement triangles.
Reuse its source.bgocct reader and original assembly/style graph.

Grounding: adopted OCCT revision b8f597c677811d1f9f4d8a97f5ae2825c0353a42,
BRepMesh_IncrementalMesh.hxx constructor/Perform/Parameters/GetStatusFlags;
BinTools_ShapeSet::Read/ReadSubs/Shape; BRep_Tool::Triangulation;
BRepLib_ToolTriangulatedShape::ComputeNormals and Poly_Triangulation accessors.
Actual source says absolute deflection requires isRelative=false; the existing
angular default is0.5rad. ComputeNormals uses source UV normals when available,
with OCCT's documented triangulation fallback. Keep that distinction visible.
No new dependency or meshing theorem is selected.

Add only native/occt_reference/{CMakeLists.txt,main.cpp},
examples/render_om10_occt_reference.py and one focused test file. Build separately
against the already installed reference OCCT, never the main mesher build tree.
The native batch reads fresh original definition BinTools and face-index maps,
requires exactly the original face inventory/orientations, and runs
BRepMesh_IncrementalMesh with absolute .005mm deflection,0.5rad angle and serial
meshing initially. Require IsDone, record all status flags, and require a nonempty
finite triangulation for EVERY source face. Export actual binary64 points and
OCCT normals, applying triangulation location and original orientation exactly
once, into the existing BGREFM01 raw display boundary with original face IDs.
Face-local vertex keys are transport keys, not a claimed sewn source topology.
No repair or missing-face continuation produces a successful definition.

The Python caller reuses load_definition/write_assembly, original assemble,
authored_layout/camera_view/callouts/write_png and the existing raw native CPU
display. Derive bounds from actual reference meshes; preserve all original
occurrence paths/matrices. Emit independently authored assembled/exploded views,
source-linked BOM/labels, original source digest, exact inventories, per-face
triangle counts, OCCT flags/parameters and separate source/meshing/display walls.
Label every image OCCT REFERENCE; illustrative palette/explosion are owned
presentation. These cameras are unmatched until explicitly reused in a later
comparison. Do not imply final5micrometre CPU/P100/NVIDIA qualification.

Before implementation use the existing real box/cylinder STEP fixture for the
missing-operation RED. After strict fleet build, check its full original face
word, actual triangle vertices/normals against original box/cylinder equations,
original three placements including rotation, two nonempty native frames, and
missing-source/no-replace refusal. This is one direct reference-boundary check,
not a new numerical certification campaign. Then run ONE bounded fresh complete
watch operation (cap120s,CPU1 meshing/declared CPU display width), only after root
coordinates resources. Require107/9162/166 and every face has triangles before
rendering; any failure retains diagnostics and no complete-image claim. No
additional adapter, NVIDIA repair, geometry cohort or product-mesher change.
