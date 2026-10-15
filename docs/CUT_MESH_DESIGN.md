# Mesh the actual through-bore Boolean result

This second-pass connection consumes an existing owned CutResult and emits a
closed, bounded-error indexed mesh. It connects modeling to meshing without
reconstructing a canonical plate or changing the existing canonical API. CPU
exact construction, no new dependency, native ABI, GPU mesher or language choice.

## Request and output

`brepgpu.cut_meshing.tessellate(cut_result, *, tolerance,
max_quarter_segments=256)` admits only the exact factory CutResult type from the
existing strict through-bore difference. The lazy `CutResult.tessellate` adapter
calls it. A bare CutSolid, canonical body and feature result are rejected. The
full CutResult retains actual source operands, sections, cap arrangements,
selections and complete history; geometry alone does not replace those records.
Factory-only records are a trusted owned input boundary, as for existing bodies.
No public construction or untrusted forged-object/deserialization support is added.

Tolerance admits exact int excluding bool, Fraction or finite binary64, positive,
interpreted exactly in world units. Segment budget is an integer excluding bool,
a power of two in 1..1024. Type/value errors reject malformed requests;
MeshBudgetError rejects an unachievable subdivision or rounded geometry budget.
Failure publishes nothing and preserves operands, result and previous meshes.

A factory-only recursively immutable CutMesh retains `cut_result`, `body` (the
actual cut_result.solid), tolerance, quarter_segments, curve_bound,
rounding_bound, error_bound, vertices, triangles, edge_polylines, boundary_uses.
`history` exposes the retained cut_result.history. Reuse existing immutable
MeshVertex/MeshCorner/MeshTriangle/EdgePolyline/BoundaryUse value records, without
widening meshing.Mesh or canonical tessellate. Source tuples retain original
result vertex IDs or original result edge ID/exact parameter, scoped by the
actual retained body ID/revision. Complete original-operand ancestry stays
accessible through typed EntityKey and the retained Boolean history, not copied
or replaced by mesh-local IDs. Face/chart/coedge IDs always refer to CutSolid.

## Source-first construction and domains

The admitted result has outer half-width3/4, half-height1/2 and hole radius1/2 in
its topological frame. Actual curves, edge endpoints, coedge maps, loops,
face/chart records and controls supply construction. Do not call make_body,
relax canonical guards, manufacture .shape or packed tables, or refit carriers.
CutSolid.edge_point and coedge_uv supply exact result boundary maps. They use the
result frame for physical curves and original carrier UVs for occurrences.

Each result vertex is allocated once. Every original line uses shared endpoint
indices and t0/1. Each closed circle uses t=k/(4n) in the original rational
quarter parameter, with one shared closed anchor and repeated endpoint occurrence.
Build physical edge polylines and every directed coedge use before faces.
Use original oriented loops to resolve outer corners and each face's circular
boundary; no coordinate welding or guessed matching by approximate positions.

Planar rectangles use two triangles. Each annular cap uses four corner fans over
its actual rim quarters plus four outer-side triangles, as in the researched
canonical construction. Radius1/2 remains strictly below outer half-width3/4,
so the same positive-domain construction is valid. Each original wall quadrant
connects corresponding lower/upper ring intervals into two triangles per chord
strip. Preserve every face sense, including the inward wall.

Invert each planar chart's actual affine controls and patch frame to obtain
exact corner UV. The wall retains the original cutter controls/frame (scale is
twice the topological box scale), with raw source V=[1/4,3/4]; its triangles use
those two V endpoints, never normalized0/1. Quarter U is chart-local0..1.
BoundaryUse.uv retains atlas0/4 seam lifts, both distinct same-face coedges,
and original V endpoints. A source edge t remains distinct from cutter V.
Topological quarter joins create no artificial B-rep edges. With N=4n, complete
output has V=2N+8, T=4N+16, 15 physical edge polylines and30 coedge uses.

## Error and rounded embedding

At world hole radius R, choose the first dyadic n with R/(2n*n)<=tolerance/2,
bounded before mesh allocation. The rational-quarter derivative has angular
speed<=2, giving minor-arc angular length<=2/n and sagitta<=R/(2n*n).
This covers both wall chord strips and cap-domain discrepancy in both directions.
The polygonal hole is inscribed: a thin mesh strip enters the analytic void.
Do not claim all triangle interiors are material-IN. Complete chordal coverage
and bounded distance to the true boundary are separate required claims.

Round every shared exact world vertex once to binary64. Compute actual maximum
L1 coordinate displacement rho; barycentric interpolation bounds every triangle's
displacement by rho. Publish error_bound=curve_bound+rho<=tolerance. Check exact
triangle nondegeneracy and actual rounded orientation before publication.
Rounded rings must be strictly convex, simple, quadrant ordered, strictly inside
the rounded outer rectangle, with matching top/bottom XY and ordered heights.
Planar vertices must remain coplanar with original boundary planes. These exact
checks on ratios of output floats preserve the supported extruded shell embedding.
No discarded triangles, hidden healing or silent budget clamping is permitted.

## Direct independent qualification

An independent qualifier first checks the actual Boolean result against original
operands/sections/arrangements/history via the existing exact result oracle.
It then reads the returned mesh and source records without calling the mesher,
its private geometric readers or triangulation templates for expected answers.
Generic independently implemented chain/winding/patch predicates may be shared
with canonical qualification; factory assumptions cannot be inherited blindly.

Check exact source generators and actual roundings, ordered physical edge and
coedge chains, original map/source correspondence, every corner's original
patch evaluation with its own patch frame and raw UV band. Require whole positive
triangle boundary-chain equality for every planar chordal domain and wall chord
strip in affine chord coordinates, separately for exact and rounded geometry.
Independently establish simple nested boundary polygons and arc/chord inequalities.
Check all mesh edges have two opposite uses, one cyclic link per vertex, connected
shell and genus one. Counts, screenshots, volume and CPU parity cannot substitute.

Direct tests cover every admitted box scale, translated/extreme/subnormal origins,
coarse/fine thresholds, just-below thresholds, invalid type/value/budget, immutable
retained input/history and healthy retry after failure. Altered actual outputs
attack missing/duplicate/flipped/filled-hole triangles, split rim indices, wrong
source parameters, seam/chart UV, normalized cutter V, wrong patch-frame usage,
rounded coordinates and reported error. Selected faults support these distinct
assertions, not a suite-wide mutation score. All execution occurs on the fleet.

## Actual consumer, measurement and publication

A dedicated caller constructs actual operands and Boolean result, constructs and
reads mesh buffers, follows typed result-to-original history, independently
qualifies output and exports exact/rounded/source tables, PLY and actual SVG.
Reuse compatible indexed consumption/display functions only; source export must
retain real operands/result sections/arrangements/selections/history and controls.
Measure operand preparation, Boolean construction, prepared-result meshing,
indexed/history reading and directly wrapped result-to-consumed wall. Independent
qualification, repeat correspondence and filesystem export are separately named.

Four serial cases: coarse unit tolerance1/2, refined unit1/1024, translated scale2
at1/4096, and subnormal-origin scale1/4 at1/4096. They vary mesh resolution/frames,
not Boolean complexity. One first output, one excluded warmup, five repeats each;
qualify first outputs, compare full later outputs outside useful timing. No
native/GPU calls or device transfers occur; no GPU speedup claim applies.
Do not overlap costs with tests/fault campaigns or rebuild protected libraries.

Publish actual artifacts, independent review, source hashes and scoped costs.
External community meshing adapters remain incomplete and alpha non-green;
this narrow result mesh is no completion of that corpus. Feature-result meshing,
operation composition, runtime study and NCSX construction remain separate units.
