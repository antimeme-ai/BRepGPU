# Owned extrusion: independent profile-to-solid qualification

2026-10-15. Research only. Read current AGENTS/doctrine, feature roadmap, actual
body/Boolean construction and independent oracles, sampling/native patch
contracts, pinned OCCT prism/linear-extrusion/history implementations and actual
regression recipes, and pinned NVIDIA sweep bindings/tests plus our CPU baseline.
No product execution, implementation, new dependency or build.

## A useful bounded family

A rectangle with a circular hole is a good hole/seam fixture but alone looks
almost identical to the preceding through-bore result. A simple arbitrary polygon
adds concavity, intersections and triangulation choices unnecessary to establish
extrusion. Recommend a **strict convex CCW polygon with 3..16 original vertices**,
optionally one centered circular hole, extruded along positive Z between two
original endpoint values. This admits actual triangle, trapezoid and hexagon
profiles as well as the rectangle-with-hole member. No box/plate factory supplies
the output; the complete source profile determines its sides and cap trims.

Use immutable independently named profile vertex/edge/loop/region records.
Input vertices and z endpoints can be finite original binary64 values treated
as exact dyadic ratios. A useful native envelope is XY and z within [-1,1], with
a stated minimum nonzero edge length/height if the same normal-consumer precision
contract is promised. Reuse the existing positive power-of-two Frame only as the
world placement; an arbitrary swept direction would introduce a separate frame/
rounding contract. Reject reversed winding, repeated/collinear vertices,
self-crossing/nonconvex polygons, zero/negative height and malformed identities
explicitly; do not silently sort or repair source vertices.

For the optional hole, require r>0 and strict disk containment in every original
edge half-plane. For CCW edge d=b−a and centered circle c=0, this is independently
checked by `cross(d,c−a)>0` and
`cross(d,c−a)^2 > r^2*dot(d,d)`. No sqrt/epsilon is needed. Tangent, intersecting,
multiple or off-center circles are separate scope. If keeping this first unit
smaller, defer the optional hole rather than dropping the nonrectangular profile;
the polygon family alone still delivers an actual profile-driven feature.

## Exact material and complete boundary

Let P be the closed polygon material (minus the open disk if present), and
I=[z0,z1]. The solid is exactly P×I. Independent membership evaluates all original
CCW half-plane inequalities, optional `x²+y²−r²>=0`, and the two height
inequalities. Strict material interior makes all inequalities strict. Closed
supports give all incident face IDs, including outer vertical edges/corners,
cap edges, both hole rims and the hole wall. Cap points inside the hole are
exterior even on the cap plane. No production point classifier supplies expected
truth, and `polygon AND NOT closed_disk` would wrongly delete the inner wall.

The full-boundary model is

`boundary(P×I) = (boundary(P)×I) union (P×{z0,z1})`.

Require a bijection from output faces to the two entire source-profile cap
regions and one entire swept side domain per original boundary edge. A hole adds
one whole cylindrical side with four angular charts. Original strict convexity,
simple/disjoint loops and positive height prove those surfaces intersect only
on their declared shared boundaries. Full source-word/domain equality and
rational map identities therefore establish embedding/completeness without a
quadratic pairwise-intersection checker or a general cap triangulator. A centroid,
Euler characteristic, volume, or a few correctly placed walls does not.

Both cap loops must be complete original profile words, with lower cap sense
reversed and upper sense preserved. Outer sides have domain (s,t)∈[0,1]² and
map `(1−s)*a+s*b, z0+(z1−z0)*t`. For a CCW outer edge the intrinsic du×dv points
outward (dy,−dx,0); the hole wall reverses cylinder sense. Keep circle seam U0/U4
uses separate on the same physical wall face; internal quarter joins are atlas
joins, not physical trim boundaries.

## Controls, frames and UV without hidden fitting

A fixed cap plane carrier over XY[-1,1]² avoids computing/rounding a new bounding
rectangle. Its four controls are (±1,±1,zcap,1); the exact polygon/hole trim can
use Fraction UV `(x+1)/2,(y+1)/2`. These exact UVs need not be binary64. An actual
native candidate is classified at its copied float UV, separately from an exact
curve/trim query; never promise that rounding an exact boundary UV remains on
that boundary.

Each line-wall bilinear control is one original XY endpoint combined with one
original z endpoint and W=1. No fitted samples or rounded coordinate differences
are needed. For the circle, the existing four rational quadratic quarter nets
use `(r,0,1),(r,r,1),(0,2r,2)` and endpoint Z multiplied by W; these are exact
power-of-two products inside the admitted envelope. Source controls, emitted
controls and retained frames must have exact bit correspondence; unsupported
numeric construction must reject rather than claim zero rounding.

Use unit normalized axial parameter t and explicitly name its derivative:
S_t=(z1−z0)*ez in local coordinates. Do not mix it with OCCT's natural unbounded
length parameter or an assumed source V in length units. Cap and line-wall
Patch.uv may remain unit squares; circle angular charts can retain the existing
quarter-index UV convention. Exact source maps carry the declared conversion.

Native first-jet qualification can use an independent expanded Bernstein/source
formula at actual submitted bits and existing native grades. Published world
position/unit-normal rows may use the established direct component limits
2^-36/2^-30, checked independently; those normal limits do not follow merely
from a partial-derivative grade. Non-axis polygon sides have generally irrational
unit normals: use exact integer-square-root interval bounds around
sqrt(dx²+dy²), then derive component intervals. Reusing an axis-normal lookup or
normalizing the production derivative again would not be independent truth.

## Topology, volume and source history

For N outer vertices and H in{0,1} circular holes with one physical seam anchor
per ring, the complete expected tables have:

| Records | Expected count |
| --- | --- |
| Vertices | 2N+2H |
| Edges | 3N+3H |
| Faces | N+2+H |
| Coedge occurrences | 6N+6H |
| Loops | N+2+3H |
| Charts | N+2+4H |

Check the entire graph/source bijection before these totals. Each physical edge
has two opposite occurrences; vertex links are single cycles, distinguishing
closed-ring endpoints; one connected shell is required. Corrected Euler is
V−E+F−2H=2−2H, giving genus H. For H=1 this includes the two annular cap holes;
plain V−E+F is insufficient. Source polygon shoelace area minus pi*r² times
height gives exact rational+pi volume by integrating the constant cross-section.
This is a supporting invariant, not the material/embedding oracle.

History should make **lower copy, upper copy and sweep-generated entity**
relations explicit. Each source vertex has two endpoint images and supports a
vertical edge; each source boundary edge has lower/upper edge images and supports
one side face (with chart occurrences where needed); each source loop has two
cap-loop images; the source material region supports the two caps and solid.
State the exact source kinds and whether same-dimensional translated copies are
called modified/generated. Do not call every output retained merely because a
source point lies at z=0. Both endpoint copies retain original profile identity,
original intrinsic edge parameter, axial parameter map and orientation relation.
Fresh output namespaces must distinguish separately allocated equal profiles.
Generated seam anchors/wall chart joins name the actual source circle occurrence,
not a coordinate-welded polygon vertex. Forward/reverse lookup must cover every
promised source kind and every output kind, including coedges/charts if claimed.

OCCT's [pinned prism contract](https://github.com/Open-Cascade-SAS/OCCT/blob/b8f597c677811d1f9f4d8a97f5ae2825c0353a42/src/ModelingAlgorithms/TKPrim/BRepPrimAPI/BRepPrimAPI_MakePrism.hxx)
distinguishes first/last profile shapes and sweep-generated topology; its
implementation returns the swept shape for Generated(S), rather than inventing
our profile history labels. Its [linear-extrusion surface contract](https://github.com/Open-Cascade-SAS/OCCT/blob/b8f597c677811d1f9f4d8a97f5ae2825c0353a42/src/ModelingData/TKG3d/Geom/Geom_SurfaceOfLinearExtrusion.hxx)
keeps U on the basis curve, V along extrusion and does not itself detect every
degenerate case. These are study references, not adopted machinery.

## Direct failures, fleet cases and real costs

A bounded direct family should include a non-axis triangle and trapezoid,
rectangle-with-hole, asymmetric hexagon, all admitted placements/scales/heights,
source-order cyclic rotations with stable lineage, separately allocated equal
geometry, and one-ULP/exact rational witnesses around every side/cap/hole support.
Admission negatives cover nonconvex/bow-tie/repeated/clockwise/collinear words,
zero height, hole tangency/intersection, unsupported curves and bool/index aliases.
Do not silently convert unsupported inputs into empty or repaired profiles.

Named direct faults are omitted/swapped side face, wrong endpoint height, cap
kept as the bounding rectangle, lost circle hole, reversed outer/inner sense,
collapsed seam occurrence, wrong source parameter scale, missing upper-copy
history and merged equal-profile identities. Whole source domains/map identities,
public exact material readers, native rows, incidence and precise history each
attack their own faults. Inject a real late assembly/validation exception, assert
source profile records/bytes and previous completed results unchanged, then one
healthy retry. Namespace gaps are acceptable; no partial result/history escapes.
No new native fault infrastructure or broad mutation score is needed.

Measure actual profile validation/preparation, CPU construction/history,
material/edge/history consumption, native sampling/normal and optional existing
ABI5 adjacency, cleanup and complete caller wall. Keep qualification, acquisition,
export/visualization and process startup separate. Use three different real
profiles rather than repeating a canned shape to manufacture a large GPU batch.
Run CPU1/CPU24/P1000/P1001 consumers with matched parameters; construction remains
CPU and cold setup remains visible. No general feature/CAD speedup follows.

## Existing foreign reference and alpha limits

The already installed USD BRep 1.0.0 public `create_line_segment` and
[linear_sweep](https://github.com/NVIDIA-Omniverse/usd-brep/blob/c9c5979f04cea8c38213004d4f04df0dcfb40d05/source/SmPyLib/src/SmPySweeps.cpp)
can construct an actual nonrectangular straight-profile comparison in an isolated
fleet process. Its API says direction magnitude is ignored, distance sets sweep
length, and capped closed profiles form solids; it preserves source curves.
Our existing CPU baseline already sweeps circles, but its mass/bounds checks are
not an independent extrusion oracle. Actual multi-loop/hole behavior of this
specific API still needs a fleet probe; do not infer it from a different sweep.
If a reference polygon extrusion followed by cylinder subtraction is used for
hole geometry, name that different reference construction and never compare it
as equal-operation throughput. Retain actual source values, returned geometry,
queries, warnings and timeouts. World boundary/closest-point and volume checks
are feasible; no public reference history mapping or material API is assumed.

The pinned upstream Python sweep test only asserts Brep type and a nonempty face
list. OCCT `tests/bugs/modalg_7/bug31031` specifically detects a vertex-prism
history regression (zero deleted/one generated), useful source-history corpus
but not a polygon-solid truth oracle. `bug_gh544` requires separate model data
and primarily tests nontermination in a subsequent fuse. Inventory applicability
honestly: these are neither a god-level extrusion suite nor automatically green
BRepGPU tests. Existing external OCCT/neutral-exchange/feature adapters remain
required alpha debt under ALPHA_QUALIFICATION.md; missing data, unsupported promised
cases and oracle disagreements remain non-green. No new library adoption or
extra broad release gate is proposed.

## Selected scope — supersedes the earlier optional-hole proposal

Root selected **3..32 vertices, no holes**, exact coordinates on the 1/256 grid
within [-3/4,3/4]², and centered positive height h on that grid in [1/4,2].
The lower/upper cap planes are exactly z=−h/2,+h/2; retain the existing positive
power-of-two Frame. The earlier optional hole and 16-vertex proposal above is
research comparison only and is not the selected design. Concavity, circle
holes, clockwise normalization and arbitrary sweep directions remain unsupported.

Validate global strict support: for each directed edge, every nonincident source
vertex has strictly positive cross product. Local consecutive-turn tests alone
are insufficient to exclude winding stars. This finite integer predicate
establishes the strict convex simple CCW domain. Nonzero grid edges have length
at least 1/256, and height is at least 1/4. Every construction control is an exact
source endpoint combination; cap height division by two is exact, circle control
issues do not apply, and the fixed profile cap carrier is XY[-1,1]² at z=0.
Caps translate that carrier without altering XY/UV, with lower sense−1 and upper
sense+1. Side maps are `source_edge(s)+(v−1/2)*h*ez`, s,v∈[0,1].

### Independent side-normal bound

Let DX,DY be integer source edge differences after multiplying XY by256.
Then M=DX²+DY² is a positive integer and the exact outward side unit normal is
`(DY,−DX,0)/sqrt(M)`. Uniform positive world scale and positive h cancel from
normal direction. If M is a perfect square, each component is exactly rational.
Otherwise let K=isqrt(M<<128). Exact bounds are

`K/2^64 < sqrt(M) < (K+1)/2^64`.

Divide each signed integer numerator by the appropriate positive endpoint to
obtain a rational interval containing its true unit-normal component. Since
|DX|,|DY|≤384 and both square-root endpoints are at least1, the interval width
is less than384*2^-64, hence less than2^-55. No floating sqrt or production
normalization supplies the expected answer. Requiring the actual returned
binary64 component's maximum exact distance to **both** interval endpoints to be
at most2^-30 proves its error against the true component is at most2^-30.
Using distance to just the nearest endpoint would be weaker. Caps have literal
unit normals ±ez. Preserve the separate source-world position2^-36 check at
actual submitted UV bits and each original/emitted patch frame.

### Complete source-to-output history

A full profile has n vertices, n edges, n coedges, one loop, one material face
and one chart: 3n+3 source entities. Every source entity has two distinct
same-kind modified cap images, giving6n+6 modified output origins. No source is
retained or deleted in this declared centered extrusion convention. The sides
add8n generated entities: n vertical edges, n side faces, n side charts, n side
loops and4n side coedges. Total14n+6 equals the complete output counts
(2n vertices,3n edges,6n coedges,n+2 loops,n+2 faces,n+2 charts).

The oracle must reconstruct actual lower/upper source images and each generated
side entity's source edge/vertex/coedge role, not merely verify those totals.
Cap affine source XY/UV identity, signed height translation and orientation must
be explicit; side source-edge parameter and axial parameter are distinct.
History maps must address the actual source Profile namespace/revision/IDs and
all source/output kinds. Exact map/domain equalities establish the complete
boundary of profile×interval; shared identities, opposed uses and cyclic vertex
links establish one closed genus-zero shell (Euler2). No canonical body relabel,
manufactured rectangle or volume-only argument establishes that claim.
