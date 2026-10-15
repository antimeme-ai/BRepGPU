# Mesh the actual convex-profile extrusion

Connect the existing complete convex-profile ExtrusionResult to a closed CPU
indexed mesh. Consume its actual vertex/edge/coedge/face/chart records and retain
its original profile and history. No canonical body, generalized feature protocol,
new dependency, native build or GPU construction is needed.

## Request and output

`extrusion_meshing.tessellate(feature_result, *, tolerance)` and lazy
`ExtrusionResult.tessellate` admit the exact factory ExtrusionResult type. Bare
ExtrudedSolid, profiles and other result families reject. Positive int excluding
bool, Fraction and finite binary64 tolerances are interpreted exactly in world
units. Malformed input raises TypeError/ValueError; actual rounded embedding or
error-budget failure raises existing MeshBudgetError and publishes nothing.
Inputs and prior meshes remain unchanged. Owned immutable factory results are
the trusted input boundary; forged-object/deserialization support is not added.

Factory-only immutable ExtrusionMesh retains feature_result, actual body=solid,
tolerance, rounding_bound, error_bound, vertices, triangles, edge_polylines and
boundary_uses. `history` returns the actual feature history. Shared mesh-reader
properties curve_bound=Fraction(0) and quarter_segments=0 describe the exact
polygonal geometry; there is no circle subdivision or maximum-segment argument.
Reuse the existing immutable mesh leaf record types. Source tuples name actual
result vertices, scoped by retained body/revision; face/chart/coedge/edge IDs
refer to that result. Original profile ancestry and affine maps remain in the
retained history, reached using EntityKey rather than reconstructed coordinates.

## Exact construction

All admitted extrusion faces are affine planes bounded by convex polygons.
Allocate only the actual2N result vertices, exact world=origin+scale*local;
round each once. Every physical edge has its actual shared endpoint slots and
intrinsic parameters(0,1). Every directed coedge occurrence retains its orientation
and actual UV map. Construct each actual face's directed loop word from those
records. Bottom orientation is already reversed in its coedge word: do not apply
face.orientation again. Fan a convex face word from its first vertex, producing
N-2 triangles per cap and two per side. Total2N vertices and4N-4 triangles.
No coordinate welding, inferred shape tags, added interior vertices or refiners.

Each corner's UV is the exact inverse of the actual affine chart with its own
Frame, using the full2×2 Gram system for its two control-edge vectors. Require
exact source-plane reproduction, positive/nondegenerate material orientation and
valid parameter domain. This handles non-axis side planes. Controls/maps/history
are retained unchanged; no rounding or reparameterizing source geometry.

## Approximation, rounded embedding and complete coverage

Exact planar triangles reproduce the original trimmed solid boundary, so source
approximation error is zero. Let rho be the actual maximum L1 displacement from
exact vertex to saved binary64 vertex. Corresponding barycentric interpolation
bounds every rounded triangle in both directions by rho. Publish error_bound=
rounding_bound=rho<=submitted tolerance. No automatic tolerance widening.

Rounded lower/upper layers must retain identical XY, a common Z within each cap,
strictly ordered heights, and globally strict convex CCW profile support: every
nonincident vertex lies left of every rounded directed profile edge. Local turns
alone do not suffice. This establishes a simple embedded rounded prism. Check
actual cap/side triangle nondegeneracy and material orientation in that rounded
prism. Rounded oblique side planes may differ from the original source planes;
that is the measured bounded perturbation, not grounds to reject honest rounding.

One independent qualifier first uses existing extrusion_oracles.qualify_result
for original profile/feature/history truth, then checks the actual mesh. It must
not call the mesher, private inverse/triangulation helpers or output adjacency
for expected geometry. Derive exact source positions, edge/coedge/UV chains and
source affine patch values independently. Check positive triangle orientation
and complete directed integer boundary-chain equality per original face, using
source-plane coordinates for exact geometry and actual rounded edge/Z coordinates
for oblique side rectangles. These establish whole-domain multiplicity one;
area, vertex samples, volume and images cannot substitute. Repeat domain and
embedding checks on actual output float ratios. Require two opposite uses per
mesh edge, one cyclic link per vertex, connectedness and genus zero.

## Bounded tests, consumer and stopping rule

Direct cases cover triangle/chamfer/16/32-vertex profiles, a thin valid lattice
profile, non-axis sides, frame scale/translation extremes, subnormal origins,
height endpoints and tolerance equality/just-below an actual nonzero rounding
bound. Test wrong input types, factory/recursive immutability, healthy retry and
preserved prior output. A few altered real meshes directly check missing/duplicate/
reversed triangles, wrong chart/UV/source indices, split shared boundaries and
incorrect rounded coordinates/error. No separate mutation campaign or test score.

Run direct genuine red before the new API exists; implement; direct green;
independent code review; one affected compatibility run; resolve actual findings.
Use fleet execution only. Run actual triangle/chamfer/16-profile callers, qualify
each returned mesh, export exact/source/history/PLY/SVG and measure useful
construction+indexed/history consumption with qualification/export separate.
One bounded serial set of repeated costs is enough; no causal hardware/ranking
claim or general mesh certification. A32-vertex admission case is a direct test,
not another performance campaign.

Stop after the direct obligations, affected compatibility, actual caller review
and any finding-driven fixes pass. Publish/merge the complete bounded connection
and its honest limitations. Do not add broad numerical/GPU reruns, mutation
scores, a second coverage mechanism or another receipt layer. Existing external
alpha qualification and reference disagreements remain unchanged and open.
