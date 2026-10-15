# Mesh the actual convex-profile extrusion result

2026-10-15. Bounded semantics study for the next result-to-mesh connection.
Read current doctrine, `extrusion.py`, `extrusion_oracles.py`, the exact extrusion
contract, and current canonical/cut mesh records and construction. No product
execution, new dependency, general triangulator or certification system.

## Source reading and concrete consequences

The owned [extrusion contract](../docs/EXTRUSION_DESIGN.md) admits an original
strictly convex CCW lattice polygon with 3–32 vertices and a positive centered
height, in the existing translation/positive power-of-two Frame. Its result is
an actual product solid, with two polygon-trimmed affine caps, one affine side
per source edge, 2N output vertices, 3N physical edges and 6N directed uses. The
constructor and independent source oracle were both read; no canonical body
recipe needs to enter this connection.

Re-read acquired OCCT `BRepMesh_EdgeDiscret.cxx` and
`BRepMesh_FaceDiscret.cxx`, pin
`b8f597c677811d1f9f4d8a97f5ae2825c0353a42`, specifically physical-edge processing,
coherent 3D/incident-2D tessellation, complete face dispatch and explicit failed
face status. File hashes/restoration are already retained in
[meshing-semantics-sources.tsv](meshing-semantics-sources.tsv). The direct lesson
is shared physical-edge geometry plus separate face-occurrence parameters,
followed by complete face construction. It does not prescribe OCCT's parallel
machinery, caches or dependencies for this small owned operation.

Re-read Shewchuk's author-hosted Triangle paper sections on
[triangulation structures](https://www.cs.cmu.edu/~quake/tripaper/triangle2.html)
and [constrained domains/refinement](https://www.cs.cmu.edu/~quake/tripaper/triangle3.html).
They distinguish preserving source boundary segments, removing non-domain
triangles, and later quality refinement. Here strict convexity removes the
need for a general constrained triangulator: a corner fan covers each cap
exactly. No minimum-angle quality or Delaunay property is claimed. Indexed
triangles carry source identities without constructing a mutable pointer mesh.

## Recommended public operation

Add a separate `extrusion_meshing.tessellate(ExtrusionResult, *, tolerance)` and
lazy `ExtrusionResult.tessellate`. Keep exact factory type admission and actual
result/solid/history identity. Reuse immutable MeshVertex, MeshCorner,
MeshTriangle, EdgePolyline and BoundaryUse value records. Do not widen canonical
or CutResult guards, forge a BodySnapshot, copy/rebuild the profile, or create
new mesh-local substitutes for output source entities.

All boundary edges are straight; do not add circle subdivision controls or an
artificial curve approximation allowance. Allocate each of the actual 2N solid
vertices once. Physical edge polylines contain original parameter endpoints
0 and 1 and original start/end indices. Every actual coedge gets its own
BoundaryUse with traversal sign applied once and exact original coedge UV.
A mesh retains the entire result; its history is the retained result history,
whose typed output keys lead to the original six-kind profile ancestry.

Each cap contains N−2 fan triangles and each side contains two triangles:
4N−4 triangles total. The N=3 case must work without an assumed quadrilateral
cap. The cap domain is the actual submitted polygon rather than the full square
carrier. Build face boundary indices from actual loops/coedges/edge endpoints;
no coordinate welding or XY sorting. A directed lower cap loop already has
outward winding. Either triangulate that word directly, or derive chart-positive
winding and apply face sense once; never apply lower face sense twice.

## Exact affine parameters and rounding

Invert the actual affine patch using its own control values and world frame.
For local origin p and derivative columns a,b, solve the exact Gram system
with coefficients a·a,a·b,b·b and right sides (x−p)·a,(x−p)·b. Extrusion's a,b are
orthogonal even on oblique XY sides, so the existing separate-projection formula
also applies within this explicit family. A genuine 2×2 inverse makes the
assumption visible. Require reconstruction from resulting UV to equal the exact
world mesh point. Nonaxis side direction does not imply a curved carrier.

Exact facets cover the actual planar boundary, so geometric approximation error
is zero. Round each shared world vertex exactly once to binary64 and publish
rho = max_vertex sum_coordinate |Fraction(saved_float) − exact_coordinate|.
Corresponding barycentric points on every exact/rounded triangle differ in L1
by at most rho. Thus rho also bounds two-sided Euclidean distance for their
whole union. Positive exact tolerance may equal rho; reject tolerance below it
with MeshBudgetError before publishing. A zero-roundoff output can meet every
strictly positive exact tolerance; no arbitrary floating lower limit is needed.

Crucially, tiny/subnormal frame origins do not justify demanding that rounded
vertices remain on every *original* oblique side plane. Some world sums round
the small translation away and some retain it. They remain a product shell:
both layers have identical rounded XY, all lower/upper Z are respectively equal,
and every side is an edge of that rounded polygon times the shared Z interval.
Check strict height order, matching layer XY and globally strict CCW supports
of the rounded polygon using exact ratios of saved floats. This establishes
its simple convex product embedding. Check actual triangle nondegeneracy and
outward sense. Do not silently drop facets, heal vertices or inflate tolerance.

## Direct rigorous testing, then stop

Use source polygon/height/Frame expressions for expected geometry, independent
of the mesher's affine inverse and triangulation template. Read actual output
vertices/roundings, original edge parameter chains, every directed use and UV,
every triangle corner's actual carrier evaluation, complete positive planar
coverage and per-face boundary-chain equality. Check opposite mesh-edge uses,
connected shell and cyclic vertex links; counts and volume supplement rather
than replace those claims. Existing generic winding/chain/patch predicates can
be reused where their assumptions fit. New expected answers must not call the
product mesher or import its private helper calculations.

Exercise triangle, asymmetric nonaxis profile and an actual 32-vertex admitted
lattice polygon; all five Frame scales, translated/extreme/subnormal origins,
several exact grid heights; tolerance equal to/below actual rho, zero-roundoff
very small tolerance, invalid values/types, immutable retained result/history,
source namespace and healthy reuse after rejection. A finite deterministic
family of cyclic shifts/rotations/scales attacks source-order and map mistakes.
Keep this a bounded direct suite rather than a random campaign or result badge.
Selected altered outputs should expose dropped/duplicate/flipped triangles,
cap carrier-square fill, split shared indices, wrong source/map/face/chart/UV,
wrong rounded coordinates and a false reported error. These attack distinct
faults in the output contract; they do not certify a test suite or alpha.

Run affected existing meshing/extrusion compatibility methods and one actual
indexed/history/export caller on the fleet. Qualify the actual returned output,
name prepared-result construction and indexed/history consumption costs, and
keep oracle/export time separate. Publish the actual image and source-rich
artifacts once direct checks and review pass, then stop. This unit adds no
native code, GPU mesher, external-suite adapter or release-qualification claim.
