# Watch-inspired composed extrusion and bore: geometry study and review

2026-10-15. Continuing geometry-owner note for the operator's selected actual
profile → extrusion → through-cut → mesh chain, presented as an unusual
mechanical-watch-inspired assembly. This is a source-grounded design study;
no product execution, implementation or dependency adoption occurred here.

## Owned source reading

Read current AGENTS/BLACKBIRD, POST_BREADTH_ROADMAP, `extrusion.py`,
`boolean.py`, `cut_meshing.py`, `extrusion_meshing.py`, original body records,
and relevant intersection/arrangement guards. Extrusion's actual input is an
immutable strictly convex CCW 3–32-vertex 1/256-lattice profile, coordinates
bounded by ±3/4, followed by a centered positive grid height in [1/4,2]. Its
Frame provides translation and a positive power-of-two uniform scale, without
rotation. All six entity kinds have explicit original-profile ancestry.

Existing Boolean construction is materially specialized, not just guarded:
section readers require canonical BodySnapshot/FaceRef; planar arrangements
require four rectangular boundary uses; selection and material readers assume
box dimensions; assembly and meshing use fixed entity slots and quadrants.
Do not widen these guards, relabel an extrusion as a box, or imply general
Boolean support. A dedicated analytic extrusion-bore result is the smallest
complete connection. Reuse immutable owned mesh/topology value records and
actual source carriers, not canonical recipe reconstruction.

## Smallest complete family

Admit one actual ExtrusionResult plus one actual owned canonical cylinder
cutter, with parallel local/world Z axes and an exact closed circular section
strictly inside the source convex profile. Cutter caps must extend strictly
past both extrusion caps. Preserve the actual cutter wall's original controls,
Frame, quarter charts and raw V subinterval; do not normalize a clipped wall
back to [0,1]. Existing cylinder Frame choices already support multiple useful
bore scales. An arbitrary-radius cutter constructor is a separate choice, not
required for a compelling first assembly.

Transform the actual cutter center/radius into the extrusion frame exactly.
For each original CCW profile edge A→B, write d=B−A and
s=cross(d,C−A). Admit precisely when s>0 and s²>r²(d·d). These exact rational
predicates establish strict containment of the whole disk without square roots;
they reject tangent, crossing and outside disks, including non-axis edges.
Compare actual world cap heights to the cutter's actual world height interval
strictly. Use the resulting raw source V endpoints, not a fixed quarter band.

The final material is source convex prism ∩ {squared distance to C ≥ r²}.
An exact point reader checks every source half-plane, both cap heights and the
circle constraint, returning every supporting final face on equality. The cap
trim reader combines the original profile trim with the actual circular hole;
the side trims remain unchanged; the reversed cutter wall uses its raw band.
The analytic cap section is a complete circle; strict containment means no
clipped arrangement or arbitrary SSI solver is needed. Explicitly call this a
specialized analytic difference, not the existing general arrangement path.

For N source profile vertices, preserve 2N vertices and 3N straight edges,
add two complete circle anchors/rings and one seam. Result counts are:
2N+2 vertices, 3N+3 edges, 6N+6 coedges, N+5 loops, N+3 faces and N+6 charts.
Both cap outer loops are original; each gains one hole; the inward wall has
one loop with two distinct seam occurrences. Two cap holes make the genus-one
boundary Euler characteristic zero; raw V−E+F alone ignores these hole loops.
Exact volume is (profile area − πr²) times original world height, with Frame
scale accounted for once. Counts and volume supplement direct domain checks.

## Complete cap meshing without a general triangulator

For M=4q exact rational circle samples, reuse each physical rim sample between
cap and wall, and preserve separate chart/occurrence UVs. There is a particularly
small guaranteed bridge: let H=(Cx+r,Cy), the rightmost hole sample; choose an
original outer polygon vertex P with maximal X. Strict disk containment gives
P.x>H.x. Convexity keeps segment H→P inside the outer polygon, and its open
segment has x>Cx+r, hence is outside the whole circular hole. This requires no
Steiner point or source-edge subdivision.

[Eberly, *Triangulation by Ear Clipping*, 2024](https://www.geometrictools.com/Documentation/TriangulationByEarClipping.pdf),
sections 2–4, describes opposite outer/hole winding, a visible bridge represented
by two opposite directed occurrences, and ear clipping of that resulting word.
For this strictly convex outer domain, the maximal-X construction above avoids
the general reflex-vertex bridge search. Implement an owned exact-rational
version; duplicate bridge occurrences retain the same physical vertex index.
Ear tests must handle those duplicates explicitly and reject zero-area ears.
Do not erase constraints to make a failed triangulation finish.

Each cap has N+M triangles; sides have 2N; inward wall has 2M, for total
V=2N+2M and T=4N+4M. The new diagonal is mesh-local and must not become a
fictional source B-rep edge. Actual cap corners invert their retained affine
chart. Original side charts/UVs remain unchanged; wall corners use original
quarter parameters and raw V. Circle chord interiors approximate the boundary;
cap facets may cover the small exact-material-exterior slivers between circle
and chord. State the same bounded-distance contract as the existing cut mesh,
not exact material membership of every triangle interior.

The circle bound r_world/(2q²), plus actual saved-float rounding displacement,
provides the existing conservative distance budget. Exact rounded-output guards
must preserve strict convex outer and inner rings, matching Z layers, positive
height, hole separation, outward facets and incident edge correspondence.
If rounding destroys clearance, reject with MeshBudgetError, retaining inputs.

## Composed source maps, not flattened parent names

Keep the actual feature result and cutter alive. Direct cut history covers every
six-kind input entity and output entity, with complete dispositions. Original
extrusion outer vertices/edges/uses/loops persist; cap faces/charts become
modified; side faces/charts persist; circle births name actual cap/wall parents;
anchor births additionally name the actual cutter seam. Cutter caps/rims are
deleted, wall/quarter charts/loop are modified, and seam edge/two seam uses keep
separate original raw parameter intervals and orientation. Deleted inputs can
still parent generated output entities.

Composition must expose paths through actual intermediate EntityKeys and map
owners/domains: final → extrusion entity → original profile, or final → cutter.
Retain each step's maps rather than multiply unlike map representations or
collapse generated side ancestry to a fictitious same-kind source. Original
extrusion `source_t`, `source_uv`, axial maps and lower-cap orientation retain
their original definitions. A final cap face/chart can map directly through
unchanged cap UV to original profile UV; a generated side uses edge/coedge
parameter plus axial position. Generated rings inherit cap-face/profile-face
ancestry but are not original profile boundary edges. Reject foreign/stale keys.
A compact immutable chain-path reader is sufficient; no arbitrary edit graph or
persistent naming framework is required.

## Material watch visualization

The admitted family can produce actual bored polygonal wheels, thin ring-like
plates, faceted bridges, axle collars and layered supporting parts. Compose
these actual closed meshes in one assembly scene. Profiles can encode lattice
versions of desired orientations; renderer-only transformations must not be
presented as a new source geometry constructor. Existing Frame translations and
positive uniform scales supply honest part placement.

Convex profiles cannot make involute gear teeth, skeletonized multi-hole plates,
spiral hairsprings or a working escapement. A visually strong first study can use
layered bored wheels, asymmetric convex bridges and separate repeated convex
markers. If separate teeth/markers are added, they are separate parts rather
than one secretly fused gear. Label the piece as watch-inspired geometry, with
actual per-part chain/history, not a functional horological movement or an
imported historical mechanism. Attractive rendering is useful output, not the
geometry oracle.

## Relevant rigorous checks and stopping boundary

Check actual source-derived final material, every source carrier/control/Frame,
complete topology and every history/map path. Include non-axis/asymmetric
profiles, triangle and 32-vertex ends, centered/off-center bores, varying source
scale/height/raw V band, nonzero translations, near tangency rejection, cutter
cap contact rejection, invalid source/key types and healthy reuse after failure.
The independent oracle must calculate expected answers from original profile,
height and actual cutter parameters rather than call product helpers.

For mesh coverage, use one direct exact planar mechanism: all positive cap
triangles have disjoint interiors and directed boundaries equal the complete
outer/hole polygon words; check exact total cap area as a supplement. Check
actual carrier/corner UV agreement, complete source boundary uses, shared
physical indices, opposed mesh edges, connected shell and cyclic vertex links.
Exercise bridge duplicates and source-order shifts explicitly. Direct altered
outputs can attack missing/duplicate/fill-hole facets, wrong shared indices,
reversed wall, wrong original UV/band and dropped composed ancestry.

Run the bounded direct suite and affected extrusion/Boolean/mesh compatibility
suite on fleet. Run the actual watch caller, inspect its returned meshes and
source/history tables, publish geometry/render timing scopes separately, review
and stop. No mutation campaign, extra certificate layer, release badge, broad
numerical/GPU rerun or inferred alpha qualification belongs to this unit.

## Independent design review

Read `docs/WATCH_CHAIN_DESIGN.md` against the owned sources and this study.
Accepted for the stated analytic family. The added same-Z-origin restriction
is explicit and still admits the selected watch-like parts; off-center XY
supports, source wall V interval, complete counts and original carrier retention
are correct. Maximal-X bridge visibility follows directly from strict disk
containment and convexity, and avoids side-boundary subdivision. The design
correctly distinguishes occurrence duplicates from physical vertex identity,
chord-domain coverage from analytic material membership, and actual extrusion
ancestry from same-kind name substitution. The direct exact/rounded boundary-chain
mechanism attacks the real completeness claim without a second certificate.

Implementation must retain the design's full 3–32-vertex admitted family, handle
bridge-endpoint duplicates as occurrences, and preserve both cutter seam uses.
No design defect or selected dependency identified. No product code/execution.

## Independent implementation-plan review

Read `docs/WATCH_CHAIN_PLAN.md` and checked dependency ordering against design.
Accepted: independent source/model oracle and genuine fleet missing-API red
precede production; one geometry owner retains the whole source/cut/map/mesh
invariant; independent actual-output review and one compatibility run precede
the visual caller/publication. The plan keeps measured work disjoint from tests
and keeps appearance from enlarging the geometry claim. No second certificate,
new dependency or unbounded qualification campaign is introduced.

Coordinated concrete append layout with the oracle owner: retain all original
feature entity IDs/slots. Append lower/upper anchors 2N+1/2N+2; lower/upper rings
3N+1/3N+2 and seam3N+3; hole loopsN+3/N+4 and wallloopN+5; hole uses6N+1/6N+2,
then wall U0-seam+, upper-ring+, U4-seam−, lower-ring−. Append wallfaceN+3 and
its four original quarter chartsN+3 throughN+6. New ring/hole/wall-rim births
name actual feature cap face and cutter wall face; anchors additionally name
actual cutter seam. They have no ambiguous single source map for multiple
parents. Retained edges/coedges use source_range[0,1], retained/modified feature
faces/charts source_uv[0,1]². Cropped cutter seam/range and wall/UV band retain
original owners; reversed cutter occurrences carry orientation−1.

Immediate history reuses existing owned Boolean records. Composed root records
also reuse HistoryRecord: paths containing any generated step are births;
other paths are images, with modified status if any image step was modified.
No images means deleted even if births survive. Path steps retain actual object
identity. Implementation remains gated on root authorization after all reviews
and the fleet red observation.
