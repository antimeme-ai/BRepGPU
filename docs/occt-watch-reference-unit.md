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

## Failed complete attempt: bounded diagnosis

The first actual watch attempt returned107 failed definitions at the first
missing-face-triangulation guard. Original storage membership/orientation and
unmeshed-source admission had passed. The current catch loses OCCT status flags,
so this does not yet identify a reference-kernel failure. First add retained
post-mesher flags, number of triangulated original faces and the missing original
FaceID to the existing producer diagnostics. Run ONE retained original small
definition with the unchanged parameters and no source re-export. Inspect the
OCCT context/model-builder failure only if those diagnostics require it.
IncrementalMesh::Perform calls setDone even for a null model, so IsDone by itself
is not a complete diagnostic. No face omission, healing or tolerance change is
selected. Any adapter correction needs source explanation and peer review before
one affected authored check and the next complete120s attempt.

### Selected missing-UV preparation

The retained four-face diagnosis reports flags4=IMeshData_Failure; three planar
faces triangulate. The missing cylinder face has four absent CurveOnSurface
representations, Free=false, Locked=false, with SameParameter flags unchanged.
No OCCT exception occurs. Planar implicit pcurves explain why the authored box
fixture and the other three faces passed; missing curved-support UV is the
demonstrated prerequisite gap, not a wire-self-crossing diagnosis.

Reuse existing brepgpu::prepare_source_uv(original,bindings,SourceUVOptions) from
native/source_uv/source_uv.cpp without modifying it. Select prepare_query=false:
ONLY its raw copied-topology projection/seam/whole-period stage. Explicitly do
not import its optional column/endpoint/domain/pole-connector query edits.
The raw stage includes conic_source_chart bounded guide-coefficient selection
and guarded FixSeam/FixShifted whole-period translations; report these too.
Use existing projection_precision_mm=1e-7, source_residual_mm=.001 and requested
tolerance_mm=.005; disclose finite projection probes rather than a certificate.
Invoke only when an actual original face/edge lacks a pcurve. Build bindings
from the admitted original root faces/wires and ShapeExtend_WireData ordered
uses, retaining exact original FaceIDs. Local edge/vertex/wire reference IDs are
BinTools shape identities, not invented STEP entity labels. Deep-copy geometry
and topology through the selected helper. Its EdgeState/FaceState guards reject
changes to 3D curves/ranges, vertices, source surfaces and original flags; seam
and periodic operations retain their existing explicit guards. Any unsupported
profile or changed physical source refuses visibly.

Mesh prepared.raw_shape and map EVERY pre-admitted original face through
prepared.mapped(original_face); never reinterpret new storage indices or use
traversal order as source labels. Original bytes/manifest remain untouched.
Record projection/seam counts, finite residual and chart-approximation inventory
in separate preparation reporting. Link this existing owned helper and already
adopted TKShHealing only into the standalone reference target. The actual missing
original small definition is the behavior RED; require the SAME four source
faces to triangulate, then the existing authored geometry/placement check and
source review before one complete107-definition attempt. Further query topology
or preparation changes require a separate observed need and reviewed plan.

### Selected source-linked QUERY copy after v2

The v2 complete operation triangulated all faces in101 definitions but refused
six: four OCCT triangle lists contain two distinct UV seam/pole nodes with
bit-identical XYZ at a sphere pole/cone apex; another cone and six finite
BSpline faces have no triangulation despite existing pcurves. Retain those
exact original jobs as RED, not a claim that the immutable 3D CAD is invalid.

Use the SAME existing `prepare_source_uv` helper with `prepare_query=true` for
each definition. It first deep-copies original geometry/topology and completes
the reviewed RAW stage, then deep-copies that raw shape into `query_shape`.
Select ONLY that existing result: mesh `prepared.query_shape`, and resolve each
pre-admitted original FaceID through `prepared.query_mapped(original_face)`.
Do not derive source IDs from new wires, added edges or traversal ordinals.
The original face inventory remains exactly the original job word.

The selected query stage runs the existing `query_columns`,
`finite_spline_joints`, `query_domain`, then `query_poles` operations. It may
change copied pcurve coefficients/endpoints and insert typed degenerate pole
connector edges on copied wires. Such connectors carry no 3D curve, reuse the
same original physical pole vertex twice and have a linear nonrational UV
branch; `require_source_uv_uses(face,connectors)` admits only those declared
extra edges. Original physical curves/ranges/vertices/flags and carrier surfaces
are checked again by `EdgeState`/`FaceState`; original-use quotient, branch and
period guards remain unchanged. No source-helper edits, face healing, omitted
faces or triangle filtering is selected.

The existing per-use query budget limits cumulative physical-metric pcurve
perturbation to requested deflection/8 (.000625mm at .005); source correspondence
probes retain their separate .001mm cap. Report source projection and query
columns/joints/connectors, maximum finite source/query residuals and maximum
query UV metric perturbation. These are explicit copied trim-query semantics,
not exact trim preservation or a certified .005mm surface bound. Images must
say OCCT REFERENCE with source-linked QUERY preparation.

First strict-build this adapter-only selection and replay the SAME six failed
retained definitions, unchanged requested deflection/angular deflection. Every
original face must receive nonempty triangulation and every actual triangle
must stay finite/noncollapsed; all old gates remain. Require independent actual
adapter review before any full run. If all six close, run the existing authored
nine-face geometry/normal/placement operation once for the changed map boundary,
then ONE new full v3 original107/9162/166 operation under the agreed120s cap.
Any helper admission or remaining triangulation failure is retained explicitly;
no escalation to generic repair is implied.

#### Observed QUERY-copy outcome and stop

The strict adapter build and SAME six retained replay completed. All six still
fail unchanged mesh gates: four sphere/cone pole/apex triangle lists contain
true physical zero-area triangles (now status0); seven faces across the other
two definitions remain untriangulated (status4). The query stage emitted1,132
column edits,4,538 joint-endpoint edits and five typed pole connectors. Maximum
reported query metric perturbation is5.6273145154406268e-14mm, and finite query
correspondence residual is5.3448215025199131e-5mm. No full v3 or further authored
operation was run because the six-job prerequisite failed. Root stopped further
external-reference repair; raw101/107 remains the best complete-definition
inventory, and no complete watch images exist. This is an explicit incomplete
reference operation, not a benchmark of successful OCCT rendering.
