# Meshing the actual through-bore result: semantics study

2026-10-15. Recommend a separate owned CPU `tessellate_cut(result, *, tolerance,
max_quarter_segments=256)` consuming the actual immutable `CutResult`. Return an
immutable `CutMesh` retaining that exact result, its actual solid and the existing
mesh record kinds. Consume the result's physical edges, oriented loops, exact
coedge maps, chart carriers and retained provenance; do not construct a canonical
plate or widen the canonical mesher's input type. This is one useful connection
between existing operations, not a general result mesher or GPU construction.

This study read current project doctrine, owned source, prior mesh/Boolean
studies and acquired reference source. It ran no product, test or reference
program. No dependency adoption or language decision is proposed.

## Sources and what their mechanisms contribute

Owned sources read at base `8aae54d4edfc3b0c89e2201a7e1948308e62fdff`:

| File | SHA256 | Reading scope |
| --- | --- | --- |
| `python/brepgpu/boolean.py` | `b1bed397e0ccaae260727f630124735946bffff0acee7b5768b8fc21b3363a21` | actual result records/readers, region selection, assembly and all history/map fields, admission |
| `python/brepgpu/boolean_oracles.py` | `cfef0ca3ec59dea2ea1a1ac1c75f50935726d7f99b850cf45d4c74a681e68939` | original-input geometry, trim/material, source maps and whole-result qualifier |
| `python/brepgpu/meshing.py` | `16658fa752834b9c24ca7647d7b5c451dabc3742287c54108becc5ce6276f63e` | complete canonical construction, exact/binary64 records, rounded guard and bound |
| `python/brepgpu/meshing_oracles.py` | `278f2037d27e8544c2e08b9acb0176ba90e362ef8b71d2f3ca2a354152e10de2` | independent source generators, full oriented boundary-chain coverage and rounded embedding |

Also read `docs/MESHING.md`, `docs/MESHING_DESIGN.md`, the relevant
`tests/test_meshing.py` cases, `docs/POST_BREADTH_ROADMAP.md`, and the existing
[meshing semantics study](2026-10-15-meshing-semantics-study.md) and
[Boolean/history study](2026-10-15-boolean-history-semantics-study.md). Existing
bibliographies are `meshing-semantics.bib` and `boolean-history-semantics.bib`.

Reread acquired OCCT 8.0.1 source at commit
`b8f597c677811d1f9f4d8a97f5ae2825c0353a42`: `BRepMesh_ShapeVisitor.cxx`
(SHA256 `09630707ede03dded9cf8e5a1026a4cefdd642eb60fbdc2ebac8f5985a1ee04b`),
and edge-discretization 3D/2D propagation sections in `BRepMesh_EdgeDiscret.cxx`
(SHA256 `b98326849dd038a55271667e2ba78fb40b22f850036791944ebf62ff3a46e73b`).
The visitor deduplicates a physical edge before attaching oriented pcurve uses;
3D discretization supplies parameters read by each face pcurve. This confirms
the required order: physical edge samples first, occurrence maps second, faces
last. The visitor's policy can skip a failed inner wire; our complete operation
must instead raise, because silently losing a hole changes material and genus.
[Pinned visitor](https://github.com/Open-Cascade-SAS/OCCT/blob/b8f597c677811d1f9f4d8a97f5ae2825c0353a42/src/ModelingAlgorithms/TKMesh/BRepMesh/BRepMesh_ShapeVisitor.cxx),
[pinned edge discretizer](https://github.com/Open-Cascade-SAS/OCCT/blob/b8f597c677811d1f9f4d8a97f5ae2825c0353a42/src/ModelingAlgorithms/TKMesh/BRepMesh/BRepMesh_EdgeDiscret.cxx).

Reopened the official OCCT mesh guide, whose architecture separates the shared
edge skeleton and face work. Its deflection controls do not certify our exact
world bound or rounded embedding. No healing policy is imported.
[Official OCCT 8.0.1 mesh guide](https://occt3d.com/dev/doc/overview/html/occt_user_guides__mesh.html).

Reopened Shewchuk's primary Triangle section on constrained segment recovery,
hole removal and quality refinement. Its explicit treatment of hole constraints
is the relevant lesson; triangulating an unlabelled point cloud is insufficient.
Our admitted separated circle/square domains permit a complete smaller owned
construction. No Delaunay or angle-quality claim follows.
[Primary Triangle paper, section 3](https://www.cs.cmu.edu/~quake/tripaper/triangle3.html).

## Actual result contract and hazards

`difference` currently admits the actual same-origin canonical box and cylinder,
with cutter scale exactly twice box scale. Box scales are 1/4,1/2,1,2. The result
has box-local radius **1/2**, not the canonical plate's 1/4. It owns 10 vertices,
15 edges, 30 coedges, 9 loops, 7 faces and 10 charts in a fresh body namespace.
There are two generated closed rings, one modified clipped seam, two modified
annular caps, four retained outer rectangles and one reversed cutter wall.

Topological vertices and curves use `solid.frame`, the original box frame.
Planar charts retain their source box frames. The four wall charts retain the
original **cutter** controls and cutter frame. Their actual axial trim is original
V `[1/4,3/4]`; it is not a carrier rescaled to V `[0,1]`. Applying `solid.frame`
to every chart doubles down on the wrong geometry. Normalizing the band in mesh
corner metadata without an explicit source map also lies about correspondence.

`solid.coedge_uv` maps the clipped seam's canonical q in `[0,1]` to that original
V band. Separate seam uses retain lifted U=0 and U=4. Ring coedges map to original
V=1/4 or 3/4. Ring t=1 and t=0 share one physical anchor, with distinct parameter
occurrences. `orientation` reverses the boundary chain once; map evaluation at
the same underlying t must not reverse it a second time.

The existing canonical `_rounded_guard`, face builder and oracle all contain
shape/domain assumptions. They cannot be reused wholesale by adding a `.shape`
attribute or broadening `isinstance`. Reusing immutable mesh record definitions
and genuinely common arithmetic helpers is reasonable. Canonical behavior and
its admitted inputs should remain unchanged.

## Exact API and retained ownership

Prefer `brepgpu.cut_meshing.tessellate_cut(CutResult, *, tolerance,
max_quarter_segments=256) -> CutMesh`. A lazy result method can be added if useful,
but is not needed for this unit. Reject `BodySnapshot`, bare `CutSolid` and
unrelated feature results: bare `CutSolid` alone does not own the retained input,
section, arrangement and history envelope that this API promises to preserve.

`CutMesh` is factory-only and recursively immutable. Recommended fields:

```
result, tolerance, quarter_segments, curve_bound, rounding_bound, error_bound,
vertices, triangles, edge_polylines, boundary_uses
```

Expose `solid` as an alias/property for `result.solid`, not a reconstructed value.
Vertex sources `('vertex', id)` and `('edge', id, t)`, triangle face IDs,
chart-local corner IDs/UVs, and boundary-use IDs all belong to that **output
solid's** body/revision. The exact retained `result.history` maps these output
entities to original operands. Do not relabel mesh slots as operand IDs, flatten
many-to-many ancestry into one parent, or invent a second composed history.
Generated ring edges retain their section/component/event/arrangement ancestry
through the unchanged result envelope. A caller must actually read those links.

Reuse current tolerance admission: positive finite int/Fraction/binary64 value,
bool excluded, exact submitted ratio retained. Segment budget: power of two
1..1024, default256. Share `MeshBudgetError` for unmet subdivision, rounding or
rounded embedding budgets. Unsupported type raises TypeError. No partial shell,
silent tolerance widening, omitted hole or coarse fallback is returned. Inputs
and earlier outputs remain unchanged. This is not a hostile-object security API.

## Construction and approximation

Build every physical edge exactly once from actual output vertex/curve records.
Lines use endpoints; each ring gets N=4n segments at t=k/N. Preserve the two
original ring anchors and add N-1 samples per ring. Build every directed coedge
chain from the published actual maps before assembling face triangles.

Resolve each cap's outer quadrilateral and hole through actual face/loop/coedge
records. Use the four-corner fans and four outer-side triangles from the bounded
square-minus-inscribed-ring construction, with radius read from actual curve
data. At each corner compute plane UV using that actual planar carrier. Resolve
wall quadrant by the chart's `quadrant`, not accidental slot/ID coincidence.
Extrude corresponding lower/upper ring chords, with corner U=j/n and original
V=1/4 or3/4 from the actual trim/mapping. Use the actual face orientation; the
wall faces toward the void. Outer rectangles use their actual original loops.

With N=4n, the shell has V=2N+8, T=4N+16 and undirected edges6N+24, hence Euler0.
These are consequences and budget checks, not geometric qualification.

Read world radius R from actual ring curve radius times actual topology-frame
scale. The rational-quarter speed bound gives sagitta delta=R/(2n²). Select the
first admitted dyadic n with delta<=tolerance/2. All cap mismatch is in the strip
between each arc and chord; all wall mismatch is the corresponding ruled strip.
Thus the complete exact mesh approximates each actual trimmed analytic face in
both directions by delta. Straight cap triangles can enter the analytic void by
this amount; never claim every triangle interior is material-IN.

Each vertex stores exact world XYZ and one shared actual binary64 XYZ. Let rho
be the maximum exact L1 displacement of these saved rounded vertices. Barycentric
interpolation bounds every facet's perturbation by rho. Require delta+rho<=the
submitted tolerance, including at translated origins. A small numeric bound
alone does not establish that the rounded mesh is embedded.

## Independent whole-output qualification

Start with existing independent `boolean_oracles.qualify_result(result)` for the
retained source/result contract. A new independent mesh qualifier must derive
expected geometry from original operand equations and the actual recorded
result's maps, not call product meshing/refinement helpers or rerun construction.
Using the established input qualifier is not a substitute for directly checking
the mesh. In particular independently distinguish box scale from cutter scale.

Check complete source vertex generators, every ring sample's exact original
curve parameter, every physical polyline, all30 directed coedge occurrences,
original chart corner reproduction using each patch's own frame, and the actual
V-band/cap-ring/seam correspondences. Check retained result identity and all mesh
provenance namespaces. Preserve separate chart-local U0/1 occurrences at quarter
joins and lifted U0/4 seam uses without duplicating physical vertices.

For each planar face reconstruct complete outer/hole polygon words independently
from the source loops. Require strict projected orientation and equality of the
FULL integer directed triangle boundary chain with that polygon boundary. Winding
additivity then establishes multiplicity one inside and zero outside: missing,
extra and overlapping triangle regions fail. Area, centroid membership, Euler
and volume alone cannot establish this. Independently establish that the expected
outer/hole polygons are simple and strictly nested.

For each wall chord strip use affine `(lambda,z)` on the actual spatial chord,
not rational arc parameter s as though it were affine. Check its complete
oriented boundary chain and positive triangle orientation over the actual clipped
height. Original minor-arc dot-product inequalities independently bound full
chords against analytic arcs; finite test points do not prove maximum deviation.

Repeat the complete face/strip orientation and chain argument on the actual
saved binary64 coordinates interpreted as exact ratios. Require each rounded
ring simple/strictly convex, original quadrant order/cardinal anchors, exactly
matching upper/lower XY arrays, strict cap height order, strict nesting inside
the rounded outer rectangle, and exact expected common coordinate planes. Those
extruded-domain prerequisites establish no unintended inter-face intersections.
Check all physical mesh edges have two opposite uses, every vertex link is one
cycle, the shell is connected and Euler0. This independently checks geometric
embedding and manifold topology rather than inferring them from a small rho.

Direct cases should include every admitted source scale, nontrivial translated
and tiny/subnormal origins, coarse n=1 and finer meshes, exact tolerance thresholds,
malformed requests, exhausted segment budget, and atomic rounding failure. A
selected fault set should attack wrong radius, cutter-frame loss, V0/1 substitution,
missing/fill-hole triangles, source-history substitution and seam occurrence loss.
These named sensitivities do not establish broad mutation score or alpha readiness.

## Delivery implications

Publish a caller consuming actual mesh arrays, all boundary/corner maps and
retained history, exporting the actual shared rounded coordinates to PLY and
views. Record source Boolean creation, mesh construction, useful complete mesh
consumption, independent qualification and filesystem export separately. No
native/CUDA loading or speedup is required for this exact CPU connection.
Existing native libraries stay unchanged. Applicable external community tests
remain non-green until their actual required adapters/data/output comparisons
pass; this unit does not alter the alpha gate.
