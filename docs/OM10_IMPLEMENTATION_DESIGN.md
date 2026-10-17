# Complete OM10 source, owned mesh and CPU/P100 image design

Design reviewed by source/mesh, native display and independent-oracle owners,
2026-10-15. Findings on finite arithmetic, evaluation-only qualification,
supersample metadata and projected diagnostics are incorporated. Implements the operator's
whole movement assembled and exploded/labeled, CPU-only and GPU-accelerated,
with measured graphics. Actual OM10 renders follow implementation qualification;
synthetic renderer fixtures and nonvisual source extraction are development work.
No single-part substitution, source-face omission or foreign product triangles.

## Source and ownership

The supplied MAN VA01.3 source hash is
`c60675abe3314dca55a31da0406deef851e128e2ee8608536525f926f56d61e3`.
Its107 solid definitions,9,162 faces and166 expanded leaf paths are separate
complete inventories. Preserve the original in ignored quarantine and retain
source/restoration/license metadata. Publication identifies OpenMovement and
the exact revision; source-derived assets retain applicable source attribution.

Use pinned OCCT8.0.1 commit b8f597c677811d1f9f4d8a97f5ae2825c0353a42 as the
previously discussed isolated STEP/XDE source/reference adapter. Its only product
boundary is an owned versioned binary snapshot, not handles, pointers or JSON.
OCCT reconstructs missing pcurves during translation; that foreign work and any
source/tolerance/topology changes remain explicit. Its independent scalar and
shape diagnostics are reference comparisons. Its mesher is not the product mesher.

Our modules own curve evaluation, source-domain sampling, trimmed triangulation,
refinement, mesh/source identity, assembly states, nativeCPU/CUDA geometry/display,
labeling and callers. No new rendering/triangulation/Python framework dependency.
The existing patch/runtime ABIs and canonical-family guards remain unchanged.
New native image work is a separate optional executable/plugin and build tree.

## 1. Source adapter and typed geometry snapshot

Adapter executable `brepgpu-step-source INPUT.stp OUTPUT.bgsource` performs one
headless XDE translation. Build only required STEP/XDE/geometry/mesh-reference
toolkits and dependencies; no GUI, Draw, OpenGL, Tcl/Tk or optional JSON toolkit.
Source scope and bytes are bounded, all failures identify source entity/stage.

Use original STEP entity bindings through the transfer process for vertices,
edges, faces and solids. Never assign original source IDs from traversal order.
Retain product definitions and original occurrence transform chains separately
from geometry carrier frames. Compare ordered wire traversal with raw coedge
inventory so an explorer stopping early fails. Multiple/null transfer results,
changed topology and unsupported entities are emitted as explicit diagnostics;
they cannot disappear into an otherwise successful whole-model result.

The exact BGSRC001 source binary schema is in the
[import study](../papers/2026-10-15-om10-import-mesh-study.md#integration-schema-candidate-bgsrc001-version-1):
88-byte header,13 counted sections including retained source styles, and
length-delimited rows. Geometry owner supplies the immutable
Python reader `brepgpu.imported_source.load_source`. It retains definition/
assembly/usage records; analytic and original spline carriers; original vertices,
edge curves/ranges and same-parameter/degeneracy flags; every distinct coedge UV
occurrence, loop and face; original/translated tolerances and source correspondence.
Original source units are millimetres; model uncertainty, translation residuals,
chosen mesh quality and output rounding are different quantities.

Analytic carriers retain independent O/X/Y/Z and original UV. Plane/cylinder/
cone/sphere/torus formulas and first derivatives follow the actual source maps;
cone V is slant distance. Indirect carrier axes are preserved. Only B-splines
undergo degree-preserving knot insertion to homogeneous Bézier spans, retaining
original intervals and derivative scaling. Curve support includes original
degreefive; recovered pcurve family is inventoried on first extraction and its
actual degrees/parameter correspondence supported or explicitly rejected.

Binary schema has magic/version, explicit little-endian scalars, counted/ranged
sections and source hash. Validate sizes/products/ranges before allocation,
finite values, versions/types/reserved bits and complete ownership. No dumped
native structs or process-local enums. Failed calls publish no partial source.

## 2. Owned full trimmed mesh

Geometry owner implements `brepgpu.imported_meshing.mesh_imported(source,quality)`
as one complete invariant with the reader/source curve interpretation. Output
retains original source, physical keys and complete face/corner provenance.

Sample each physical source edge once, splitting knots and refining controlled
chord approximation. Coedges consume the same sample keys in their own directed
UV lifts. SameParameter=false requires an explicit map, not an assumed identical
t. Evaluate actual spatial-edge/surface discrepancy; recovered UV is not exact
source data merely because both source and result came from OCCT.
Actual OM10 has zero non-SameParameter edges. Unproved continuation/projection
maps are explicitly rejected by the mesher; seed pairs do not establish a unique
branch. This delivery does not promise general non-SameParameter recovery.
Original B-spline source carriers and extracted Bézier display spans have
separate typed records despite their respective format-local kind6 tags.

Recover continuous periodic trim domains, retaining both same-face seam uses.
Preserve degenerate physical endpoints and nonzero UV boundary spans. Establish
valid finite polygonal domains with nonconvex boundaries and all holes; crossing
or ambiguous recovery fails or drives source-boundary refinement. No tiny-hole
deletion, centroid filtering, outer fan or geometry-coordinate welding.

Use owned robust orientation with exact fallback, visible multiple-hole bridges
and complete ear triangulation grounded in the Eberly/Shewchuk studies. Duplicate
bridge occurrences share physical keys. Preserve collinear boundary constraints.
The implemented bridge variant checks complete segment visibility and material
membership before ranking visible endpoints by distance. It does not use the
invalid nearest-reflex shortcut identified in the literature; independent full
positive UV chains check the resulting domain rather than its bridge heuristic.
Refinement is conforming: split both incident interior triangles; changing a
physical edge updates every incident face. New interior samples refer to actual
original carrier UV/span. Handle sphere/cone singularities by their actual
physical quotient; do not emit zero-area physical triangles.
Retain positive UV cells which collapse under a declared physical singularity
as explicitly marked coverage cells. Only repeated physical keys supported by
that source quotient can mark a collapsed cell. Full UV coverage includes them;
physical incidence/native display omit them. Their curved source interiors can
have area despite collapsed corner XYZ, so approximation bounds/refinement still
apply to them. This representation clarification was reviewed during implementation.

Actual source face165503 passes an original sphere-pole vertex with three source
coedges; OCCT inserts an extra UV-only pole edge. Zero original degenerate edges
therefore does not mean zero singular vertices. Preserve the original source
edge inventory and explicitly reconcile the extra foreign chart connector.
Owned `ChartClosure(face_id,loop_id,source_vertex_id,incoming_coedge_id,
outgoing_coedge_id,uv_start,uv_end,kind)` records original traversal direction.
Retain both actual incoming-end and outgoing-start UV occurrences with one
physical source vertex. The connector must be at the proven analytic pole/apex,
map constantly to that vertex and stay within the admitted periodic span.
Normalized coverage can reverse traversal without changing this provenance.
No fake original coedge or coordinate-nearness weld is introduced. Source and
independent owners reviewed this amendment; root accepted before implementation.

Canonical spatial-curve XYZ owns shared boundary positions. Interior positions
come from source evaluation. Mesh corners retain separate UV/carrier/orientation
even when physical positions coincide. Output source mesh and display-corner
mesh are distinct: sharp boundaries and seams may duplicate display vertices
without losing physical incidence identity.

Quality distinguishes owned polygonal coverage, controlled carrier/chord bounds,
actual binary64 displacement and measured foreign recovery residuals. Use
analytic/control-hull derivative bounds where claimed, including crossed knot
spans; a midpoint probe is not a universal flatness bound. Do not claim a whole
source Hausdorff certificate from finite OCCT probes. Exhausted budget or excessive
actual source discrepancy is an error, not a tolerance widening or OCCT mesh fallback.

## 3. Native CPU/CUDA consumer and display

Native lane owns standalone `brepgpu-om10-native` and explicit CUDA plugin,
sharing owned scalar carrier/raster math. CPU does not link or initialize CUDA.
The exact BGMESH01/BGVIEW01 records are specified in the
[native study](../papers/2026-10-15-om10-native-display-study.md), including80-byte
scene header,56-byte display vertex,192-byte carrier and192-byte view request.
Freeze no undocumented padding: readers/writers use explicit fields and reject
unknown bits, invalid indices, lengths, weights, domains and transforms.

Scene vertices hold canonical XYZ, original UV, carrier slot, physical key and
boundary/orientation flags. Source first jets produce actual interior XYZ and
all display normals; shared boundaries retain canonical XYZ and expose measured
surface positions for residual checking. This evaluation is consumed by the
image. Source knots/spans and analytic parameters stay in original coordinates.
Zero-view requests still evaluate and write source jets, allowing actual CPU/P100
geometry qualification before any watch image. Finite inputs do not guarantee
finite arithmetic: reject nonfinite intermediate/result evaluation, placement,
depth or shading with the source/request row; never silently discard geometry.
At analytic singularities use declared limiting normals; an unsupported singular
normal is explicit, with any required facet-normal policy separately reviewed.

Rigid assembled matrices and authored explosion offsets remain distinct. At
alpha0 return assembled placement directly before explosion arithmetic. Apply
rotation to normals. Camera is orthographic and checked right-handed. Geometry
remains unquantized; projected XY snaps to1/256 sample pixels with shared rounding,
range-limited for safe signed64 predicates. Integer top-left coverage, unbiased
barycentrics, maximum FP64 depth and lowest stable primitive-ID exact ties define
visibility. Stable IDs map back to occurrence path/source face.

Both paths use complete16×16-tile CSR bins without truncated candidate lists.
Native CPU workers own disjoint tiles; CUDA samples choose an order-independent
winner. Retain resident input/workers/device storage for named reused requests.
Lighting uses actual source-face normals, documented metallic illustrative
palette/Lambert/fill/Blinn and common supersample/sRGB policy. No fake geometry,
unqualified shadows or graphics API assumptions. ID/depth/source-jet output is
available for direct qualification and label anchors. Allocation/launch/sync/
download failures invalidate output; close frees ownership/restores device.
Final-pixel metadata selects the nearest-center supersample, then lowest sample
index on ties (upper-left at2x); metadata is never averaged. Optional complete
sample metadata supports the independent scalar oracle.

CPU/device libm can perturb evaluated XYZ and projected snap thresholds. Exact
coverage applies to fixed snapped inputs. Tiny independent specification fixtures
have exact expected winners; actual whole-source comparisons require source
position/normal accuracy and isolate any reported edge-band visibility differences,
not an unsupported promise of universal byte-identical CPU/P100 images.
Retained per-backend projected snapped coordinates/depth permit attribution to
actual geometry/rounding differences; unexplained visibility differences fail.

## 4. Assembly, explosion, labels and actual caller

Root owns immutable assembly/BOM/occurrence values over the geometry owner's
source model. Full instance identity is the root-to-leaf usage path, not the
product ID or one usage slot. Compose nested source rigid transforms in explicit
order, preserving root names/styles and references to reusable definitions.
Independent raw STEP frame/usage checks supply expected placement, not a second
read of the already-composed product matrix.

Authored explosion offsets are in assembly-world coordinates and derived from
explicit subassembly/leaf layout records. All166 paths and107 definitions stay
present; geometry does not change with explode alpha. At alpha0 the original
assembled values are returned directly. This is an illustration, not simulated
mechanical disassembly or collision-free removal. A hierarchical whole view,
detail plates and complete BOM make dense parts legible without166 unreadable
leader lines on one hero. Every callout names a source instance or explicit group.

Root caller writes native scene/views from actual owned meshes, runs the selected
CPU/P100 process, consumes actual RGBA/ID/depth/source output and creates PNG/SVG
with source-linked labels and timing panels. Camera/framing is based on actual
transformed source bounds. Original names remain; authored English aliases,
palette and explosion are recorded. Same settings across matched backends.
Static SVG-to-PNG presentation composition is separate/named if outside the
measured caller; do not hide it in a purported complete PNG wall.

## 5. Direct qualification, sequencing and timing

Use the independent oracle lane's [study/subplan](../papers/2026-10-15-om10-oracle-benchmark-study.md).
Before production, fleet red tests target reviewed APIs for source import/mesh,
assembly/explosion, carrier math and tiny scalar-raster answers. Missing API red
is stated accurately; later expanded cases are separate. Per-unit greens and
adversarial source review resolve findings before actual OM10 trophy rendering.

Qualify original source/converted jets, all face domains by positive exact saved-UV
boundary chains, shared physical incidence/vertex links, complete source inventory,
rigid placements and callouts. Native tests include ownership/failure paths and
appropriate host/device memory checks. A kernel success, parity or image alone
does not establish geometry. Actual diagnostics may reveal source issues; resolve
them explicitly without shrinking the whole-model goal or silently repairing.

After every capability and complete source geometry/assembly pass, render actual
assembled/exploded CPU and each P100 with the same quality/view/palette. Inspect
saved images, labels, source maps and output. Fix actual presentation defects and
rerun affected work; no unrelated certification campaign follows.

Native stage timing includes decode, session/setup/upload, source evaluation,
instance/project, triangle/bin preparation, display/resolve, download, writes
and close. Full cold STEP-to-image adds source import/mesh, labels/encoding and
external process teardown. Reused scene update has symmetric retained geometry
and includes changed placements/camera, complete image/download/labels/output;
it is not another model-build time. CPU1/useful native multicore and each P100
use the same complete source. Independent qualification is separate and included
where a wall is explicitly called qualified. Repeated alternating measurements
report median/range, no selected speedup or asynchronous launch-time headline.

Final graphics show whole assembled and exploded labeled geometry, honest counts,
actual CPU/P100 seconds and concise stage/scope legends. Publish actual source/
native pins, recipes, BOM/placements, indexed output and costs with attribution,
preserve branch commits/merge and clean fleet/main. External alpha qualification
remains incomplete. No broader functional watch, original feature history or
general GPU solid-construction claim.
