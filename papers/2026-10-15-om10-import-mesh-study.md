# Whole OM10 import and owned trimmed meshing

Research/design input, 2026-10-15. The requested target is all 107 supplied solid
definitions, all 9,162 source faces and all 166 expanded assembly occurrences,
assembled and exploded, with matched CPU/P100 work. This study executes no CAD,
product geometry, tests or renderer. No production code is written here.

## Recommendation

Use the discussed OCCT dependency as an isolated STEP/XDE source adapter and
independent reference evaluator. Export an owned binary snapshot. Own all product
boundary sampling, trim-domain triangulation, refinement, CPU/CUDA carrier
evaluation and image production. OCCT's mesher may provide a separately named
comparison, never unnamed product triangles.

Preserve original analytic UV rather than converting every carrier to NURBS.
Own direct plane/cylinder/cone/sphere/torus evaluation on CPU and CUDA. Extract
the 635 B-spline surfaces into the 2,839 original knot-span rectangles and use
owned rational Bézier evaluation there. This closes the actual carrier family
without an unnecessary parameter-changing analytic conversion.

The source has no PCURVE records. STEP translation reconstructs face-UV curves;
that is foreign source-conversion work, and the snapshot must say so. Retain
original spatial curves and check the face/edge discrepancy rather than asserting
that reconstructed UV is exact source data. Do not silently change edge or
vertex tolerances to make a meshing result green.

## Sources actually read

- The supplied source's retained static inventories in
  [the STEP study](2026-10-15-om10-step-source-study.md) and
  [the complete carrier/trim study](2026-10-15-om10-whole-assembly-geometry-gaps.md).
- OCCT 8.0.1, commit `b8f597c677811d1f9f4d8a97f5ae2825c0353a42`, locally
  retained by [the restoration manifest](acceleration-roadmap-reference-restoration.md):
  `STEPCAFControl_Reader.hxx`; `BRep_Tool.hxx`; `BRepTools_WireExplorer.hxx`;
  `BRepTools_NurbsConvertModification.cxx`; `BRepBuilderAPI_NurbsConvert.hxx`;
  `GeomConvert_BSplineSurfaceToBezierSurface.hxx`;
  `GeomConvert_BSplineCurveToBezierCurve.hxx`;
  `BRepMesh_DelaunayBaseMeshAlgo.cxx`; `BRepMesh_Deflection.cxx`; and
  `ElSLib.cxx`, especially original analytic D0/D1/D2 conventions.
- David Eberly, *Triangulation by Ear Clipping*, revised 2024-08-03, sections
  1–5 including the corrected minimum-angle bridge selection and multiple holes.
  [Original PDF](https://www.geometrictools.com/Documentation/TriangulationByEarClipping.pdf)
  acquired unchanged as ignored `papers/pdfs/eberly-triangulation-ear-clipping.pdf`.
- Jonathan Shewchuk, *Robust Adaptive Floating-Point Geometric Predicates*,
  1996, introduction, filtered signs, exact expansion arithmetic and arithmetic
  assumptions. [Author PDF](https://people.eecs.berkeley.edu/~jrs/papers/robust-predicates.pdf)
  acquired unchanged as ignored `papers/pdfs/shewchuk-robust-predicates.pdf`.
- Shewchuk's [Triangle algorithm description](https://www.cs.cmu.edu/~quake/tripaper/triangle1.html)
  and [exact arithmetic discussion](https://www.cs.cmu.edu/~quake/triangle.exact.html).
  These teach separation of constrained domains, predicates and refinement;
  Triangle is not selected as a dependency or copied implementation.
- Existing retained [meshing oracle study](2026-10-15-meshing-oracles-study.md),
  including its reading of Piegl/Richard 1995 and the positive-boundary-chain
  argument. The new imported family requires broader topology and source-error
  handling; earlier canonical qualifications do not establish this delivery.

The acquired Eberly PDF SHA-256 is
`736e0a7e3c5b1d82ec245fc3f2a6ba4bcdf61dbe6f5c75050c023bb9ff3d5ede`;
the Shewchuk PDF SHA-256 is
`2e615f8f4479da5b2cfee772ad43143db95ac39c10b94ad14844d54bc1b04a14`.
Restore either by downloading its exact linked author URL to the ignored path,
then compare SHA-256 before using it. Original PDF bytes were not modified.

## Why blanket NurbsConvert is the wrong default

The conversion class advertises conversion of analytic 3D geometry except planes.
Its actual modification implementation locates new curve parameters and new UV
by extrema/projection, constructs/reparameterizes curves and carries tolerance
updates. Analytic circle angle and rational quadratic parameter are different
coordinates. Converting just carrier controls while retaining old pcurves is
wrong; converting the whole shape changes more source geometry than this caller
needs. Keeping original UV makes imported trim reconstruction, direct source
comparison, periodic lifts and GPU evaluation substantially simpler.

For each analytic carrier retain independent origin O and axes X/Y/Z. Do not
derive Y from a cross product: OCCT gp_Ax3 can be indirect. Let
`e(u)=cos(u) X + sin(u) Y`:

| Kind | Original source-UV position |
| --- | --- |
| Plane | `O + u X + v Y` |
| Cylinder | `O + R e(u) + v Z` |
| Cone | `O + (R+v sin(a)) e(u) + v cos(a) Z` |
| Sphere | `O + R cos(v) e(u) + R sin(v) Z` |
| Torus | `O + (Rmajor+Rminor cos(v)) e(u) + Rminor sin(v) Z` |

Cone V is slant distance, not axial Z. Face orientation is a separate sign on
the normal. OCCT's torus implementation explicitly snaps tiny intermediate
terms; our owned formulas need not reproduce that source implementation detail
bit-for-bit. Independent comparison must use a stated numeric budget.

For B-splines retain the original full knots, multiplicities, poles and weights.
Span extraction by knot insertion produces homogeneous controls plus original
`[u0,u1]×[v0,v1]`. Evaluate in normalized span coordinates and divide partials
by the corresponding original span width. Span-edge ownership is deterministic;
evaluation at a repeated knot must retain which side supplies a derivative.
Source degree-five edge curves need degree-five support, not cubic fitting.
Recovered pcurves can have a different degree: the reader must inventory it and
the CPU curve evaluator must support the observed family explicitly.

## Proposed adapter and public API

The source adapter is a separate native executable linked against OCCT STEP,
XDE, geometry and B-rep toolkits. It does not link the product renderer or mesher.
Build headless, without Draw, GUI/OpenGL, VTK, FFmpeg, Tcl/Tk or examples. Pin the
existing source revision and record compiler/build flags and enabled toolkits.
The LGPL/exception notices remain in the separate adapter distribution; own
consumers do not link OCCT. No new triangulation library is required.

Suggested commands/API, subject to integration design review:

```
brepgpu-step-source INPUT.stp OUTPUT.bgsource
brepgpu-import-mesh INPUT.bgsource OUTPUT.bgmesh --tolerance-mm VALUE
load_source(path) -> ImportedSource
mesh_imported(source, quality) -> ImportedMesh
```

The adapter reads original source bytes, records their SHA-256, performs one XDE
translation, and exports original-to-translated correspondence and diagnostics.
Use STEPCAF name/color/material modes, XDE definition/occurrence references and
original transfer bindings. Do not infer source IDs from traversal order or
assume a translated face slot is the original STEP entity number. Face/edge
source correspondences may be one-to-many; report any change explicitly.

Use BRep_Tool curves and surfaces with their locations, not convenience reads
that accidentally double-apply locations. Preserve local part geometry; assembly
transforms are separate. Traverse each ordered wire with its actual face. Compare
the number of returned occurrences with a raw occurrence inventory:
BRepTools_WireExplorer explicitly warns that invalid wires may yield only a
partial traversal. A convenient explorer ending is not proof of complete import.

### Owned binary source fields

Use explicit little-endian scalars and counted sections, no dumped C++ structs,
pointers, JSON, hidden OCCT handles or process-local enum values. Header contains
magic, schema version, source SHA-256, adapter revision, millimetre conversion,
section count and length. Every range is checked before allocation/indexing.
Unknown versions/types, overflows, nonfinite numbers and trailing bytes fail.

| Section | Required fields |
| --- | --- |
| Definitions | Stable definition key, original product/solid IDs, names, local bounds, complete face/edge/vertex ranges |
| Assembly definitions/usages | Original product and usage IDs, parent/referred definition, rigid 3×4 source placement, names/styles |
| Faces | Stable/source IDs, definition, orientation, carrier index, ordered wire ranges, original tolerances |
| Wires | Stable/source ID, face, original outer/inner designation, complete ordered coedge range |
| Coedges | Distinct occurrence key, original oriented-edge ID, physical edge, traversal sign, UV curve/range, periodic lift, reconstructed-UV flag |
| Edges | Stable/original IDs, start/end topological vertices, source 3D curve/range, SameParameter and Degenerated flags, source tolerance |
| Vertices | Stable/original IDs, source local XYZ, original tolerance |
| Carriers | Tagged analytic frame/parameters, or original B-spline degrees/knots/poles/weights; location and periodic flags |
| Bézier spans | Parent carrier/curve ID, original parameter rectangle/interval, degrees, full homogeneous controls |
| Pcurves | Actual reconstructed 2D carrier, degrees/knots/controls or analytic parameters, original face and edge/range owners |
| Correspondence/diagnostics | Source entity to translated entities, translation warnings/errors, tolerances before/after, flags affecting geometry |

Source model uncertainty 0.01 mm, imported tolerances, measured reconstruction
residuals and chosen mesh error are separate fields. Original color/material
identity is retained even when an authored display palette is later selected.
Private original header author/path strings must not leak to public result files.

## Owned mesh algorithm

One geometry owner owns source-curve interpretation, shared boundary sampling,
periodic chart domains and face triangulation. The display consumes the completed
mesh; it must not heal or repair it while drawing.

1. Evaluate each original physical edge once in its original spatial parameter.
   Split at source curve knots and adaptively refine against the declared chord
   error. Preserve original vertex IDs at endpoints and closed-edge identity.
   Use degree-preserving homogeneous de Casteljau/derivative bounds for spline
   spans; analytic circle/ellipse error follows curvature bounds. No coordinate
   welding establishes topology.
2. Each coedge maps that same sample chain into its own UV occurrence. Respect
   traversal reversal, source range and both seam occurrences. If SameParameter
   is false, identify an explicit mapped parameter function; never assume t is
   shared. Reject ambiguous projection rather than pick the nearest UV branch
   silently. Compute the actual `|C(t)-S(uv)|` residual and retain the worst case.
3. Unwrap each periodic wire into a continuous UV lift, preserving original seam
   occurrences. An edge shared twice on a face is two UV boundaries and one
   physical edge. Establish a finite chart domain; cylinder/sphere/cone/torus
   parameter singularities require explicit occurrence handling. A degenerate
   edge can have a nonzero UV span and one physical vertex.
4. Verify sampled-domain simplicity and hole containment. Refine where chords
   introduce crossings or alter separation. No small-hole deletion, default
   convex hull, outer fan or hole-centroid filtering is acceptable. Multiple
   exterior components, if present, are separate domain components.
5. Bridge holes rightmost-first using the directed visibility construction in
   Eberly. Its corrected selection minimizes angle, not Euclidean distance.
   Duplicate bridge *occurrences*, retaining shared physical vertices. Ear clip
   using strict positive orientation and inclusive blocking with explicit
   coincident endpoint treatment. Collinear source boundary points cannot simply
   disappear: retain their constraints or split the resulting boundary triangle.
   Use filtered robust signs with an owned exact fallback. Never use a tolerance
   on orientation signs to make ears appear.
6. Refine interior triangles using source-carrier bounds. Splitting an interior
   edge updates both incident triangles. Splitting a physical source edge is a
   shared-boundary operation and updates every incident face before retriangulating.
   Independent face refinement must not create T-junctions.
7. Emit canonical source-edge XYZ for boundary vertices and face-evaluated XYZ
   for interiors. Keep per-corner original UV/carrier/face for normals and source
   comparison, separately from physical position identity. Triangle winding
   respects the original face/shell orientation. Never emit zero-area physical
   triangles at poles; collapse physical singularities while retaining distinct
   UV corner occurrences and the complete chart boundary.

### Refinement and error claims

Midpoint checks are useful diagnostics, not universal chord bounds. For analytic
surfaces use bounds on second partials in original UV. A conservative triangle
position-interpolation bound is
`0.5*(Muu*du² + 2*Muv*du*dv + Mvv*dv²)`, with vector-norm derivative bounds and
the triangle's UV extents. Add actual boundary carrier-to-edge residual and
binary64 placement/evaluation error rather than replacing the requested budget
with the largest imported tolerance.

Bézier rational derivative bounds come from positive-weight homogeneous control
hulls and quotient differentiation. A triangle crossing a source knot line
cannot blindly use one span's Hessian. Either constrain/split the UV domain at
that line, or use a conservative piecewise first-derivative Lipschitz bound over
all crossed continuous spans. The latter is slower but converges without
assuming C1 continuity. The actual source inventory determines whether this
fallback is needed; a degree count alone does not establish knot continuity.

At singular charts the usual regular-surface inverse estimate may not exist.
Handle the collapsed topology explicitly and use forward geometric bounds.
For foreign reconstructed pcurves, distinguish rigorous owned polygonal coverage
and owned carrier approximation bounds from measured source reconstruction
agreement. Do not claim a mathematically certified whole-source Hausdorff error
from a finite comparison against OCCT.

## Independent acceptance and fault classes

The independent oracle lane can read the original raw binary source fields,
owned mesh corners and physical edge sample keys without calling triangulation
helpers. Required direct mechanisms are:

- Complete original inventory and correspondence: no missing solid, face, edge,
  occurrence or source parent. Require 107 definitions/9,162 faces/166 paths for
  the supplied revision, with all changes named rather than silently dropped.
- Exact signs and integer directed boundary-chain equality on saved binary64 UV
  values for every polygonal domain. Positive triangles plus the full chain
  detect gaps, overlaps, holes filled and duplicate/reversed triangles without
  copying the ear algorithm.
- Per physical solid, opposed shared-edge incidence, connected vertex links,
  no T-junctions and correct singular/seam multiplicity. Geometrically close
  points with different source IDs must not be welded together.
- Original analytic/independent spline evaluation and finite source-reference
  probes attack wrong frames, missing weights, span scaling, nonlinear UV
  mistakes and transposed controls. Include D1 and orientation, not positions only.
- Actual face-to-edge residual and tolerance compliance attack recovered-pcurve
  disagreement; independent source volume/area/bounds comparisons give a
  separate whole-shape check without becoming the sole oracle.
- Analytic fixtures: thin rounded plate with bore; nonconvex outline with many
  holes; closed cylindrical seam; sphere/cone singular boundary; multispan
  rational surface; degree-five curve; reversed/indirect source frame.
- Explicit errors for corrupt binary ranges, unsupported type, invalid topology,
  ambiguity, exhausted refinement and unacceptable source discrepancy. No
  fallback to an OCCT mesh or shaded partial assembly.

CPU/P100 parity is compatibility evidence, not the independent geometry oracle.
Actual whole-source qualification completes before final beauty renders.

## Integration notes and unresolved measurements

The source geometry snapshot and renderer mesh are separate binaries. Rendering
can duplicate a physical vertex for sharp face normals while retaining its
physical key. CUDA evaluates original carrier first jets; interior XYZ comes
from that evaluation, shared boundary XYZ remains canonical and its measured
surface residual is checked. GPU placement/display costs must consume all 166
instances. Source translation and CPU meshing costs remain in cold wall time.

The first fleet source extraction must report actual imported counts, pcurve
degrees, SameParameter flags, source-to-translated tolerance changes, singular
edges and source-reference residuals. These are currently unknown; claiming they
are harmless would be guessing. Resolve real findings in the complete owned
geometry lane, without shrinking the requested whole-assembly target.

## Integration schema candidate: BGSRC001 version 1

The geometry lane owns the adapter, immutable reader and mesher together. This
section fixes concrete proposed integration names for design/plan review; it is
not a compatibility promise beyond this delivery.

`load_source(path)` returns frozen `ImportedSource` with tuple fields:
`source_sha256`, `units_mm`, `source_uncertainty_mm`, `root_product_id`,
`adapter_revision`, `products`, `definitions`, `usages`, `vertices`, `edges`,
`coedges`, `loops`, `faces`, `carriers`, `curves`, `styles`, `diagnostics`.
IDs are original STEP entity numbers. Referenced-ID lookup methods are named
`product(id)`, `definition(id)`, `face(id)`, `edge(id)`, `curve(id)` and
`carrier(id)`; unknown ID raises ValueError. No mutable dictionaries escape.

- `Product(id, name, reference, definition_id, style_id)` uses PRODUCT_DEFINITION ID;
  `definition_id` is original MANIFOLD_SOLID_BREP ID or zero for an assembly.
- `SolidDefinition(id, product_id, face_ids, edge_ids, vertex_ids)` retains
  definition-local geometry. Complete inventory is explicit.
- `Usage(id, parent_product_id, child_product_id, name, transform, style_id)` uses original
  NEXT_ASSEMBLY_USAGE_OCCURRENCE ID; transform is 12 row-major binary64 values,
  a child-local to parent-local 3×4 rigid map. Preserve all 196 defined usages;
  root-to-leaf expansion, separately checked, supplies 166 leaf occurrences.
- `Vertex(id, xyz, tolerance)` retains original spatial endpoint and source ID.
- `Edge(id, vertex_ids, curve_id, parameter_range, curve_same_sense,
  same_parameter, degenerate, tolerance)` keeps source 3D-curve traversal distinct
  from coedge orientation. Range is `(t_at_start_vertex,t_at_end_vertex)` and
  can decrease; `curve_same_sense` preserves original EDGE_CURVE.same_sense.
- `Coedge(id, edge_id, loop_id, sense, curve_id, parameter_range, periodic_shift,
  reconstructed, parameter_map_kind, parameter_map)` has a distinct source
  ORIENTED_EDGE ID and recovered 2D curve.
  Sense is +1/-1 relative to the physical edge parameter chain.
  Parameter-map nodes are `(edge_t,pcurve_s)` pairs. Kind1 means identical
  parameters; kind2 is an explicit affine endpoint map; kind3 is monotone
  continuation/projection with nodes bracketing the owned mesher's solve.
  Kind3 interpolation is a seed only: actual spatial-edge/source-face residual
  must pass and the selected periodic/projection branch must be unambiguous.
  SameParameter=false cannot silently use kind1.
- `Loop(id, face_id, outer, coedge_ids)` uses original EDGE_LOOP ID. Outer is
  source FACE_OUTER_BOUND classification; FACE_BOUND orientation is retained
  by the ordered coedge traversal rather than dropped.
- `Face(id, definition_id, carrier_id, orientation, loop_ids, tolerance, style_id)` uses
  original ADVANCED_FACE ID; orientation is +1/-1 on carrier normal.
- `Carrier(id, kind, frame, radii, uv_bounds, degrees, knot_u, mult_u, knot_v,
  mult_v, control_shape, controls, spans)` retains original analytic or spline
  data. `frame` is O,X,Y,Z, 12 values; controls are homogeneous XYZW in
  definition-local coordinates, U-major/V-minor; each extracted `SurfaceSpan`
  has original bounds, degrees and homogeneous controls. Kinds analytic1..5
  match display; kind6 here is original B-spline, with extracted spans supplied
  as kind6 rational Bézier carriers only when writing the display scene.
- `Curve(id, dimension, kind, frame, radii, parameter_range, degree, knots,
  multiplicities, controls, spans)` retains 3D spatial or 2D recovered carrier.
  Kinds line1, circle2, ellipse3, Bézier4, B-spline5. Frame has 12 values (2D
  data has zero Z entries). Controls have dimension+1 homogeneous scalars.
  `CurveSpan` retains source interval, degree and homogeneous controls. Pcurve
  records use owned IDs in a reserved high-bit namespace since no original
  PCURVE entity exists; their original coedge owner is explicit.
- `Diagnostic(stage, severity, source_ids, message)` states translation changes
  and failures. Ordinary translations must not retain private header strings.
- `Style(id, source_ids, rgba, has_color, material_name, density_unit, density,
  has_density)` retains source appearance/materials, separate from any authored
  palette. Product, face and usage style IDs remain distinct; zero means absent.
  Display precedence is usage override, face, then product definition. Density
  retains its original unit text; no unannounced kg/m³ assumption is made.

### Exact wire encoding

All integers are little-endian unsigned unless explicitly `i32`; floats are
IEEE binary64. Header is exactly 88 bytes:

`magic[8]=BGSRC001, u32version=1, u32flags=0, source_sha256[32], f64units_mm,
f64source_uncertainty_mm, u64root_product_id, u32section_count=13,
u32reserved=0, u64total_file_bytes`.

Thirteen sections occur once each, in this order: `META____`, `PRODUCTS`,
`SOLIDS__`, `USAGES__`, `VERTICES`, `EDGES___`, `COEDGES_`, `LOOPS___`,
`FACES___`, `CARRIERS`, `CURVES__`, `STYLES__`, `DIAGS___`. Each header is
`char tag[8], u64row_count, u64payload_bytes`. Every row is
`u64row_bytes` followed by exactly its record fields. Strings are
`u32UTF8bytes, byte text[length]`; strings have no NUL terminator. Vectors are
`u64count, packed elements[count]`; nested span vectors contain separately
length-prefixed span records. File size, payload size and row size must all agree.

| Record | Fields in exact order after row length |
| --- | --- |
| META____, one row | string adapter_revision |
| PRODUCTS | u64 id, u64 definition_id, u64 style_id, string name, string reference |
| SOLIDS__ | u64 id, u64 product_id, vector-u64 face_ids, vector-u64 edge_ids, vector-u64 vertex_ids |
| USAGES__ | u64 id,parent_product_id,child_product_id,style_id, string name, f64 transform[12] |
| VERTICES | u64 id, f64 xyz[3], f64 tolerance |
| EDGES___ | u64 id,start_vertex_id,end_vertex_id,curve_id, f64 range[2], u32 curve_same_sense, u32 same_parameter, u32 degenerate, u32 reserved0, f64 tolerance |
| COEDGES_ | u64 id,edge_id,loop_id,curve_id, i32 sense, u32 reconstructed, f64 range[2], i32 periodic_shift[2], u32 parameter_map_kind, u32 reserved0, vector-f64 parameter_map_pairs |
| LOOPS___ | u64 id,face_id, u32 outer, u32 reserved0, vector-u64 coedge_ids |
| FACES___ | u64 id,definition_id,carrier_id,style_id, i32 orientation, u32 reserved0, f64 tolerance, vector-u64 loop_ids |
| CARRIERS | u64 id, u32 kind, u32 reserved0, f64 frame[12], f64 radii[2], f64 uv_bounds[4], u32 degrees[2], u32 control_shape[2], vector-f64 knot_u, vector-u32 mult_u, vector-f64 knot_v, vector-u32 mult_v, vector-f64 controls, vector-surface-span spans |
| SurfaceSpan | f64 bounds[4], u32 degrees[2], vector-f64 controls |
| CURVES__ | u64 id, u32 dimension, u32 kind, f64 frame[12], f64 radii[2], f64 range[2], u32 degree, u32 reserved0, vector-f64 knots, vector-u32 multiplicities, vector-f64 controls, vector-curve-span spans |
| CurveSpan | f64 range[2], u32 degree, u32 reserved0, vector-f64 controls |
| STYLES__ | u64 id, vector-u64 source_ids, f64 rgba[4], u32 has_color, u32 has_density, string material_name, string density_unit, f64 density |
| DIAGS___ | u32 severity, u32 reserved0, string stage, vector-u64 source_ids, string message |

Booleans are exactly 0/1. Orientation/sense is exactly ±1. High-bit IDs occur
only for reconstructed 2D curves. Original source IDs are positive and below
2^63. All references must resolve with expected ownership. Definition edge/
vertex inventories can include shared records but cannot silently omit a face's
actual boundary. Unused analytic fields/vectors are canonical zero/empty.

`mesh_imported(source, quality)` returns frozen `ImportedMesh(source, quality,
vertices, triangles, faces, edge_samples, timings)`. Source `MeshVertex` is
`(physical_key, xyz, source_kind, source_id, source_parameter, uv)`.
Kinds original vertex1, original edge2, face interior3 name the corresponding
original source ID. Source parameter is t only for kind2, otherwise zero; UV
is original face UV only for kind3, otherwise `(0,0)`. Every `MeshTriangle` contains original
face ID, three physical keys and three `MeshCorner(uv, carrier_slot,
face_orientation)` values. `FaceMesh` retains complete ordered boundary chains
of `(coedge_id, edge_id, sample_key, source_parameter, uv)` occurrences plus its
triangle range. `EdgeSamples` retains ordered original t and XYZ for each edge,
with both endpoints and every knot represented. The oracle checks these actual
values, not a triangulator-private cache.

`ImportedMesh.write_scene(path, instances, materials)` emits the native lane's
exact BGMESH01 format, duplicating display corners where UV/carrier/face normal
requires it while retaining definition-scoped physical keys. Root supplies
instances and materials; geometry code does not choose explosion or labels.

## Actual-source findings and reviewed implementation amendments

The first nonvisual fleet extraction and strict reader succeeded with the exact
original hash, 107 solids, 9,162 faces, 9,768 loops, 51,742 coedges, 25,871 edges,
17,129 vertices, 138 products, 196 usages and 219 retained style/material records.
All original edge bindings have SameParameter=true, no original edge is flagged
degenerate, and recovered UV curves require degrees through five. Independent
raw-source geometry comparison found maximum scalar discrepancy 2.22e-16 mm
before subsequent adapter fidelity corrections. These are development findings,
not complete mesh acceptance or render results.

The owned bridge implementation checks candidate segment intersections against
all original/current boundaries and tests its midpoint in the same material
component before selecting the shortest *verified visible* bridge. This is a
deliberate simpler variant than Eberly's minimum-angle search. The disproved
shortcut chooses a nearest reflex vertex without establishing visibility;
our algorithm does not use that shortcut. The original Eberly counterexample
is a direct test. Independent positive-domain boundary chains remain decisive.

Actual non-SameParameter continuation is outside this observed source family.
Do not treat affine seed nodes as a proof of a unique projection branch. The
owned mesher explicitly rejects unproved kind3 continuation rather than claiming
a general projection solver. Original metadata stays available; nothing from
this actual whole-source target is omitted by that rejection.

### Sphere-pole chart closure: design amendment for review

Original face165503 has sphere carrier930, R2.9, UV U approximately π..3π and
V1.3857136715..π/2. Its original loop28562 contains three occurrences: a complete
lower circle85181 and seam85182 twice. Both seam uses meet original pole vertex
67548, but their pole UV values have distinct U. A complete finite UV polygon
therefore requires a connector between those two UV endpoints. The translated
face inserts a 3D-degenerate/null-curve edge for this chart purpose; it is not an
original EDGE_CURVE and must never be assigned an invented original source ID.
Thus original degenerate-edge count zero does not mean singular vertices zero.

Retain all original source topology. Classify any translated added pole edge
separately, checking that it is flagged degenerate and collapses to the actual
original source pole vertex. The owned mesher derives its own `ChartClosure`:
`(face_id, source_vertex_id, incoming_coedge_id, outgoing_coedge_id,
uv_start, uv_end, kind)`, where kind is `sphere_pole` or `cone_apex`.
`FaceMesh.chart_closures` is a tuple of these explicit values. The boundary chain
contains both the incoming source edge's actual final UV occurrence and the
outgoing edge's actual initial UV occurrence, sharing their original physical
vertex key. Their connecting segment is a generated chart boundary, not an
original coedge. Retain both actual pcurve parameters/raw UV values.

Independently verify same original vertex ownership, original edge endpoint
parameters, the analytic singular parameter and the carrier-to-original-vertex
residual. A sphere closure has constant V at a declared pole; a cone closure
has constant apex V. A generic UV gap must not be labeled singular because its
physical endpoints are close. UV partition includes the generated connector.
Physical incidence uses the quotient. Explicit collapsed UV cells remain in
the UV partition but do not produce native physical triangles; their curved
image approximation still must satisfy the same refinement/error obligation.

The initial cylinder face164988 refinement failure was diagnosed with its actual
UV triangle and bound: 20 *propagated neighbor split counts* were reached while
its bound was 0.003895744 mm against the unchanged 0.00375 mm surface allocation.
Budget must count actual nested dyadic vertex construction, not increment a
neighbor's entire triangle depth merely because a conforming split reaches it.
This corrects bookkeeping without relaxing requested geometric quality.

The remaining cylinder boundary floor exposed an unnecessarily loose Hessian
bound: a qualified radius14.9925 source arc chord spans0.01761002rad and has
bound0.0005815mm, but the initial crude surface bound exceeded0.00375mm and
could not improve while that boundary stayed fixed. Root and independent
oracle owners reviewed the sharper interpolation formula. Expand each vertex
around the barycentric point p: the weighted first derivative terms cancel.
The vector remainder is bounded by one half times
`Muu Var(u)+2 Muv sum(lambda*abs(delta_u*delta_v))+Mvv Var(v)`.
Cauchy bounds the mixed sum by `sqrt(Var(u)*Var(v))`; a variable supported on
an interval of width D has variance at most D²/4. The valid uniform-Hessian
bound is therefore
`(Muu*du²+2*Muv*du*dv+Mvv*dv²)/8`.
This applies only with uniform derivative bounds over the entire triangle;
crossed spline knots retain the separately convergent piecewise Lipschitz bound.
No quality allocation or source boundary was relaxed. Quadratic `f(u)=u²`
attains its univariate limit at an edge midpoint and `f(u,v)=uv` attacks the
mixed term. Floating-point bound calculations remain subject to their explicit
numerical limitations; they are not a newly claimed exact universal certificate.

Actual spline face165086 subsequently exposed a second false boundary floor.
One saved UV endpoint lies a few binary64 ulps past U=-1. That is retained,
not snapped away. The source carrier4194 is degree(2,1), with U multiplicity1
at that knot, hence C1. Applying a first-derivative Lipschitz bound to every
crossed knot was needlessly weak. For positive rational B-splines, original
`degree - multiplicity >= 1` supplies continuous first derivatives. A C1,
piecewise-C2 map has the same Taylor integral remainder using the componentwise
maximum Hessian bounds over all intersected spans. Root and independent review
accepted that correction; genuine C0 crossings keep the conservative Lipschitz
fallback. Crossings use the actual strict saved-UV interval, without an epsilon
that could hide a source knot. A globally quadratic two-span C1 fixture and a
genuine C0 kink distinguish the two cases.

Curve.kind4 Bézier carrier parameters are explicitly natural[0,1]. Active edge
and coedge ranges are separate fields and may be proper subintervals. The full
carrier control net must not be reinterpreted as already restricted to that
active interval. Adapter, reader, product evaluator and independent oracle use
the same original-domain convention. Actual OM10 has no kind4 curve, but a
direct negative fixture prevents an ambiguous carrier-domain admission.


### Reviewed exact UV construction and foreign global-end recovery

Actual face165122 contains an interior shared diagonal whose endpoint V values
are adjacent binary64 numbers (0.15 and 0.14999999999999997). Its two positive
parent cells have no representable binary64 four-child midpoint fan. Preserve
ordinary binary64 UV where its children remain positive; otherwise construct
the exact rational midpoint of the saved endpoints. Every predicate containing
a Fraction bypasses the float filter and uses exact rational arithmetic.
Centroids use the same positive-child requirement and exact construction when
necessary. This preserves the owned UV partition; it does not claim that the
separately rounded native UV values partition that domain. Geometry evaluation
and native inputs explicitly round UV to binary64. Hessian bounding boxes round
outward. A global first-partial bound times the exact UV rounding displacement
produces max_native_uv_rounding_bound_mm, required <= tolerance/16. The measured
combined allocation 2*edge_chord_bound + surface_interpolation_bound +
native_uv_rounding_bound must remain <= tolerance. Independent native checking
compares the submitted rounded values and original-carrier displacement.

The same recovered pcurve extends U=-8.56e-15 outside the original global spline
domain [0,2]. Original STEP supplied no UV curves. Recover FOREIGN UV only at
global original knot-domain ends, with per-axis grade
128*ulp(max(1,abs(domain_lower),abs(domain_upper))). Outside that grade fail;
never snap an interior knot. Retain raw pcurve_uv and actual pcurve parameter;
the corrected carrier point must remain within source_residual_mm of the
original physical edge sample. Record foreign_uv_recovery_count and
max_foreign_uv_recovery_shift_mm (in-domain first-partial norm times UV shift,
a conversion-displacement estimate; raw out-of-domain UV is not evaluated or
claimed to belong to the original source domain).
Strict span selection replaces the former generic extrapolation tolerance.
Root accepted this bounded amendment after actual parent-cell diagnosis and
independent oracle review. Direct adjacent-double fan, excessive-overhang
rejection and retained-raw tiny-overhang checks attack these failure classes.


Actual cylinder face165361 demonstrated that maximum arithmetic construction
ancestry still does not measure geometric resolution under propagated conforming
splits: levels18/20/20 retained a freely splittable0.1736rad width and0.0270mm
bound. Root and oracle reviewed the budget correction. Carrier max_refinement
now controls a metric resolution floor G*2**(-max_refinement), where G is the
initial sampled face-domain diagonal in declared global first-partial-norm UV
metric. Selected free-edge metric length (or centroid triangle diameter) must
exceed that finite floor. This is a parameter/Lipschitz metric, not a claimed
physical edge length. Source edge adaptive recursion retains its original
dyadic-depth limit. Carrier max_refinement also bounds created interior nodes
per face by2**max_refinement (initial boundary nodes excluded), terminating
nonconvergent fixed-boundary floors even when their free edges stay long.
Default20 therefore admits20 metric halvings and1,048,576 interior constructions;
no approximation tolerance, source face, or quality allocation is relaxed.
Budget failure rejects the complete mesh. Direct metric-floor and work-cap
faults plus original165361 are the immediate tests before the complete rerun.
