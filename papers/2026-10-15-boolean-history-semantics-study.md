# First curved Boolean: geometry, topology and history

Recommendation: admit one exact CPU **regularized box-minus-through-cylinder**
family over the actual immutable canonical input snapshots. Build a separate
owned cut-solid snapshot from real intersections, cap arrangements and a clipped,
reversed tool wall. Do not call `make_body('plate')`, relabel an input snapshot or
generalize the canonical body factory as a prerequisite. No implementation or
product/reference execution was performed in this study.

The parent selected an even tighter initial family after the scope discussion:
**identical box/cutter XYZ origins and cutter scale twice box scale**. This is
sound and sufficient for the first breadth delivery. All translated origins and
four admitted box scales remain available; offsets below are a supported-design
extension opportunity, not a requirement for this delivery. The selected wall
trim range is exactly [1/4,3/4]. Existing line/circle/UVMap records suffice:
topology circles use box-local radius 1/2 at z=±1/2; wall circle maps retain
original v=1/4 or 3/4 and seam line maps retain lifted u=0 or 4 with that v range.
Importantly, wall controls still use the **original cutter patch frame** while
topological curves use the box frame. A downstream reader must honor each
chart's own Patch frame, rather than applying a single CutSolid frame to every
source control. Canonical-only body readers remain outside this type's contract.

## Literature and actual reference mechanisms

The acquired Hoffmann–Hopcroft–Karasick report, *Robust Set Operations on
Polyhedral Solids* (1987), sections 1–4, separates numerical uncertainty from
consistent incidence/topology decisions. Its representation discussion also
explains why a manifold-only model cannot represent every regularized Boolean.
Our bounded family avoids those unresolved configurations through exact input
predicates and strict separation, rather than copying a tolerance-based
coincidence policy. This does not solve general robustness. The retained PDF's
filename on the author's server says 1989; its actual title pages identify the
1987 technical report. [Primary author-hosted report](https://www.cs.purdue.edu/cgvlab/www/resources/papers/Hoffmann-IEEE-1989-Robust_set_operations_on_polyhedral_solids.pdf).

OCCT 8.0.1 separates its intersection data from result building. Its actual
`BRepAlgoAPI_BooleanOperation::Build` dispatches the intersection/build stages;
`BOPAlgo_BOP` constructs images in increasing topological dimension and fills
history after assembly. This supports retaining our existing intersection objects
and arrangements as operation inputs rather than reconstructing their geometry
from a final recipe. [Pinned builder source](https://github.com/Open-Cascade-SAS/OCCT/blob/b8f597c677811d1f9f4d8a97f5ae2825c0353a42/src/ModelingAlgorithms/TKBO/BRepAlgoAPI/BRepAlgoAPI_BooleanOperation.cxx).

The actual `BRepTools_History.hxx` distinguishes modified, generated and removed
relations. A removed source has no modified descendant, but may generate lower-
or higher-dimensional output. It recommends modified for changed parameter
bounds and generated for dimension-changing construction. An explicit retained
relation is useful for our fresh-namespace snapshots, where unchanged geometry
still receives new output IDs. These are operation mappings, not a persistent
naming solution. [Pinned history contract](https://github.com/Open-Cascade-SAS/OCCT/blob/b8f597c677811d1f9f4d8a97f5ae2825c0353a42/src/ModelingData/TKBRep/BRepTools/BRepTools_History.hxx).

OCCT's optional non-destructive mode prevents modifying arguments, including
tolerances and pcurves. Our immutable inputs make this unconditional. NVIDIA's
pinned C++ API instead documents in-place workpiece mutation and tool deletion
on success, with possible input modification on failure; `SmPyBooleans.cpp`
exposes consuming wrappers. Any optional reference experiment must operate on
copies, never define our transaction semantics. [OCCT Boolean guide](https://occt3d.com/dev/doc/overview/html/specification__boolean_operations.html),
[NVIDIA actual C++ ownership contract](https://github.com/NVIDIA-Omniverse/usd-brep/blob/c9c5979f04cea8c38213004d4f04df0dcfb40d05/source/SM_API/inc/SmApiBrep.h).

The classic Requicha–Voelcker boundary-evaluation paper
(DOI 10.1109/PROC.1985.13108) was identified, but its full primary text was not
acquired in this bounded pass; it is not represented as a completed reading.
The mechanisms above and the owned exact family suffice for the present scope.

## Exact admitted geometry

Let the box frame be origin b and scale s, and cutter origin c and scale t.
Actual canonical data has box XY halfwidth 3s/4, height s, cylinder radius t/4
and height t. Require the real box/cylinder snapshot types and actual supported
orientation; no transformed or imported-carrier approximation is implied.

Use exact binary-input ratios to require:

- `abs(cx-bx)+t/4 < 3s/4` and `abs(cy-by)+t/4 < 3s/4`;
- `cz-t/2 < bz-s/2 < bz+s/2 < cz+t/2`.

With current power-of-two scales in [1/4,4], these imply **t=2s**, s≤2,
`abs(cx-bx)<s/4`, `abs(cy-by)<s/4`, `abs(cz-bz)<s/2`. The comparison predicates,
not that simplification, should define admission. It is a useful translated,
off-axis hole family with different cutter/box frames. Tangency, cutter caps
coincident with box caps, partial-depth holes, wall-side clipping, disjoint
results, interacting cutters and arbitrary orientation explicitly fail support.

At s=1 this operation cuts radius 1/2; the canonical plate has radius 1/4.
Consequently `make_body('plate', box.frame)` is demonstrably wrong even in the
centered case. Center offsets also invalidate its hardcoded trim formulas.

The result is the closed regularized difference. In this family its exact
material set is the closed box intersected with
`(x-cx)^2+(y-cy)^2 >= (t/4)^2`. The cylindrical cut boundary remains in the
result. Classifying `A and not B` using closed membership for both operands
incorrectly discards that boundary. Both caps are closed rectangles minus the
**open** circle disk, matching the existing arrangement's cell closure.

## Actual construction and usable result

Compute each original box cap versus the actual cutter wall using
`intersect_faces`; retain its complete IntersectionSet, four exact rational
spatial/bothUV pieces and named events. Feed each into `arrange_face` without
grid cuts. Retain the cap's cell-with-hole region, discard the disk region, and
record original region IDs plus a strict witness classification against the
actual cutter. Admission ensures every circle is interior and complete. The
four original box side faces are retained, not intersected with fabricated
planes. Any unexpected section/component/region classification aborts.

The cutter wall is clipped to original normalized axial range
`v0=(bz-s/2-cz)/t+1/2`, `v1=(bz+s/2-cz)/t+1/2`, strictly inside (0,1).
Its outward normal reverses for subtraction. No new surface coefficients are
needed: copy original source chart controls and frames with fresh output
Lineage, preserve original chart parameters, and attach this v-band as trim.
The original full carrier domain is distinct from the output face domain.
Boundary and downstream query readers must apply the emitted band/map, not
sample the removed portions of the full carrier.

Recommend an immutable factory-only `CutSolid` (name illustrative) owning source
snapshots, output vertex/edge/use/loop/face/chart tables, source maps, sections,
cap arrangements and history. Each face retains its source carrier/frame and
an exact output-to-source chart map; no single invented common frame is needed.
Expose exact material classification, physical edge point/both-face UV readers,
face-domain classification and oriented source-carrier evaluation. Those readers
make the result usable without claiming existing canonical-only `BodySampler`,
`tessellate`, `classify_point` or adjacency adapters accept its type. Sharing
record patterns is fine; coercing its shape tag to `plate` is not.

Keep one physical circle edge per cap and one anchor at its original wall seam.
Retain all four quarter-chart events as geometry/provenance, rather than promoting
all atlas joins to feature vertices. The seam is the clipped original cutter
seam, occurring twice on the same wall face with opposite orientation and
distinct lifted U=0/U=4 maps. A local seam parameter q maps to
`v0+(v1-v0)*q`; do not apply occurrence orientation twice. Ring parameter 0/1
aliases the same anchor. Source-incidence keys identify all sharing; coordinate
welding is unnecessary and would erase occurrence meaning.

Copy all eight box corners and twelve outer edges using source identities.
Construct two ring anchors, two shared closed ring edges and the clipped seam.
The expected shell has V=10, E=15, F=7, loops=9, coedges=30, charts=10. Every
physical edge has two opposite uses, including the same-face seam. Two annular
cap holes mean Euler is `V-E+F-2=0`, not the misleading naive value 2: this is
one connected genus-one boundary, with no separate cavity shell. Exact enclosed
volume is `(9-pi)*s^3/4`, independently of admitted center offsets. Counts or
Euler alone cannot prove embedding, boundary correspondence or material truth.

## Source history and transaction

Use fresh typed output body/revision/entity identity. Retain actual input
snapshot references plus namespace/revision/kind/entity IDs; role distinguishes
workpiece from cutter. Keep a complete reverse map as well as forward maps.
Every original supported V/E/F must have a declared relation, including no
descendant, and every output element must have its exact construction origin.

Recommended history: four box sides, eight box corners and twelve box edges are
retained; two box caps are modified by their retained arrangement region;
cutter wall is modified by clipping and orientation reversal; original cutter
seam is modified to its exact axial subinterval. Both cutter caps, original
cutter rim edges and original cutter vertices are removed. The two rings are
generated by their specific cap/wall section components; anchors are generated
at their section/seam incidences. Generated support links and modified links
must not be conflated: a modified cap can also generate a ring. Record full
source coedge/loop occurrences separately from geometric-entity history.

For planes, output UV→source UV is identity; wall band uses the exact affine
axial map only if the output chooses to normalize its band. Edge ranges and
use orientations must make that choice explicit. Store the original maps,
source chart IDs, section piece/event IDs and arrangement region/loop/use IDs.
Changing only history labels after generating a plate recipe supplies none of
this correspondence.

Admit and build into private drafts; validate complete geometry/topology/history
before publishing one immutable result. An unsupported predicate, malformed
input, impossible section, assembly error or budget failure returns no candidate.
Inputs remain unchanged on every path; no reference pointer or consumed handle
may inhabit the owned result. General history composition, editing and persistent
cross-session naming remain future units.

## Direct qualification and costs

An independent exact oracle should derive the closed material predicate from
actual input frames and check all incident boundary face IDs, including cap-ring
points and seam points. It should reconstruct complete region selection and
ring/seam maps independently, prove source-carrier correspondence, opposite use
pairing, ordered loop closure and local vertex links. Geometric boundary classes
are six box plane portions plus the reversed clipped cylinder; strict separation
proves no other branches or self-intersections. Verify full history coverage and
exact source ranges, not just expected counts.

Include centered and non-dyadic binary offset cases, all admitted scales,
off-center Z, near-strict-boundary inputs, exact touching/partial-depth rejection,
swapped operands, and snapshot immutability on rejection. Repeated identical
geometry inputs must produce distinct output namespace while retaining correct
input IDs. Real saved output must expose new hole radius/center and trimmed
wall range so a recipe substitution cannot pass unnoticed.

One complete caller should read every new topology/history relation and perform
actual exact material/boundary queries. Measure admission, intersections,
arrangements, classification, assembly/history, publication and full consumption;
independent truth work and optional reference copies/cuts are separate. CPU
assembly establishes the theme; existing native source-carrier sampling is an
optional consumer, not GPU Boolean construction or a claimed modeling speedup.
Applicable broad external Boolean suites still must pass before alpha under
`docs/ALPHA_QUALIFICATION.md`; this study does not change that incomplete gate.
