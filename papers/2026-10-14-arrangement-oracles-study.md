# Planar arrangement beachhead: independent truth and bounded scope

Research only, before design/plan/red. Recommend a complete mixed line/circle
arrangement of one owned rectangular face: full axial grid cuts and finitely
many pairwise strictly disjoint circles, each strictly inside one grid cell.
Allow several circles in one cell so a real region can have multiple ordered
hole loops. Circle/grid clipping, tangencies, overlap and nesting are explicit
unsupported cases for this first unit. The operation must still emit actual
regions/shared topology, not only a layout or graph sketch. No new dependency,
generic arrangement algorithm, native port or architecture decision is selected.

## Read scope and primary references

Read doctrine/roadmap/topology architecture, owned body/FaceRef semantics,
complete plane/wall intersection component/piece/event records and current
classification contracts. Existing full circles already retain exact spatial
and bothUV maps; component direction does not assign Boolean coedge orientation.

Read [CGAL's official arrangement manual](https://doc.cgal.org/latest/Arrangement_on_surface_2/index.html),
the DCEL representation/traversal and circle/segment traits sections. It
distinguishes geometry from incidence, twin directed uses, face-left loop
traversal, inner/outer boundary components and curve history. A connected hole
boundary is not automatically one other face, which matters when deriving
adjacency. Its circle traits discuss quadratic algebraic intersection points;
the proposed separation scope deliberately avoids needing those new events.
These references inform contracts, not dependency adoption or owned correctness.

Read retained OCCT8.0.1 revision
`b8f597c677811d1f9f4d8a97f5ae2825c0353a42`,
`BOPAlgo_BuilderFace.cxx`'s PerformLoops/PerformAreas path. It constructs wires,
distinguishes growths and holes, classifies holes to candidate parent faces and
keeps internal/avoided shapes separate. The source reinforces that discovering
closed cycles alone does not assign correct region ownership or completeness.
Its tolerance/classifier machinery is not selected as the exact oracle.
[Pinned source](https://github.com/Open-Cascade-SAS/OCCT/blob/b8f597c677811d1f9f4d8a97f5ae2825c0353a42/src/ModelingAlgorithms/TKBO/BOPAlgo/BOPAlgo_BuilderFace.cxx).

## Why the separated family is a useful first unit

A grid contributes true cut/cut and cut/boundary vertices with shared linear
edges. Circle loops contribute curved boundaries, disconnected graph components,
disk regions and holes in their surrounding cells. Several circles in a cell
exercise multi-hole assignment. This establishes ordered regions, explicit
twins and original source correspondence without using epsilon topology or
turning the next theme into a general tangency/algebraic event solver.

Compared with one arbitrarily clipped circle, the separation gate gives a much
stronger independent complete-region oracle and a smaller first geometry family.
Arbitrary clipping/tangency would require exact branch-order and boundary-contact
contracts that are useful return-pass work, not prerequisites for this unit.
Retain those possibilities in notes; do not silently discard a circle which
violates admission and still call the arrangement complete.

Work in the original face's exact parameter plane. Full vertical/horizontal
cuts are strictly interior, unique exact rational coordinates. Their endpoints
are the original rectangle boundary. Circle center/radius and all separations
are exact. For every circle, its closed bounding box lies strictly between one
adjacent pair of x cuts/bounds and one adjacent pair of y cuts/bounds; pairwise
distance² is strictly greater than(sum of radii)². Zero/negative radius,
duplicates, touching/overlapping/nested circles, coincident cuts, boundary cuts
and incompatible sources fail explicitly.

World circles become parameter circles only under equal-length orthogonal face
basis vectors. Owned horizontal top/bottom faces have that square parameter map;
other original rectangle orientations may have unequal U/V scale. Either limit
circle-bearing inputs to the supported horizontal square family supplied by
current plane/wall sections, or explicitly represent the affine image. Never
reuse a world radius as a UV radius without the original frame/basis conversion.
Grid-only arrangements can independently support the other affine rectangles.

## Complete independent region model

Let nx/ny count interior axial cuts; K=(nx+1)(ny+1) grid cells and M circles.
The model does NOT reproduce product cycle extraction or halfedge next links.
It enumerates the following exact region domains directly from original input:

1. For each open grid cell, remove the closed disk interiors/boundaries of every
   circle strictly in that cell to obtain the open surrounding region.
2. For each circle, its open disk is a separate region.

Thus K+M bounded regions exist, each with a known semantic signature: a specific
cell minus its listed disks, or one particular disk. The hypotheses prove that
each surrounding cell region is connected and that these are ALL components
of the rectangular face away from input curves. Establish a bijection between
emitted regions and these domains, including complete circle-hole assignments.
Region count alone cannot prove that bijection; wrong holes, duplicate regions
and omitted/duplicated geometry can preserve Euler and counts.

Specify an exterior face or an explicit exterior sentinel for boundary twins.
If an exterior region is represented, it has the rectangle's clockwise boundary
as an inner component and no fabricated bounded outer component. Bounded regions
need exactly one outer loop; surrounding cells have one clockwise hole loop for
each contained circle, and disks have no hole. Hole boundaries remain shared
curves, not material-removal decisions or missing disk faces. Arrangement
regions become candidate pieces for later material selection.

The model determines exact membership for any rational point: locate its cell
by source cut inequalities, then compare squared distance to every circle in
that cell. Circle/grid/boundary points lie on explicit strata, with all incident
regions retained rather than arbitrary single-region selection. If the public
consumer only exposes region loops, qualify their entire domain signatures and
use known literal interior witnesses; do not claim sampling proves completeness.

## Whole topology and curve coverage

Every product edge has exactly two opposite directed uses, including original
face-boundary edges whose opposite use belongs to exterior/sentinel. Verify
`twin(twin(h))=h`, reversed endpoints and shared curve/source identity, distinct
uses and consistent left region. All valid next/prev links are inverse, targets
equal successor sources, and traversal visits every declared loop use once.
Every use occurs in exactly one region boundary component; no unreachable
halfedges or hidden dropped input curves remain.

Compare actual oriented loop edge words with independent cell/rectangle sides
and circle quarter sequences, allowing cyclic rotation but not reversal of the
declared parameter-plane orientation. Bounded cell/disk outer loops are CCW;
circle holes are CW. Opposed original material-face normals remain source
metadata, not a reason to reverse a purely parameter-plane arrangement rule.
At a grid crossing the four exact outgoing directions establish the cyclic
order without atan2. A loop with correct counts but swapped next links fails
its expected region-domain boundary word and endpoint continuity.

Grid source coverage is every consecutive interval between exact intersections
on each full cut and each rectangle side, once per undirected edge. Circle
coverage is the complete source closed component, typically its four retained
rational quarter pieces, each once geometrically and twice directionally.
Do not count atlas occurrences as new physical spans or drop the last seam
join. Verify complete source parameter-interval coverage, no gap/overlap and
preserved original chart/piece/event provenance independently from output links.

For a four-quarter circle representation, expected counts are
V=(nx+2)(ny+2)+4M,
E=(ny+2)(nx+1)+(nx+2)(ny+1)+4M and2E directed uses.
If a different valid source decomposition is admitted, derive extra split nodes
from the original exact pieces instead of forcing four. Euler's relation with
M+1 connected graph components is a useful structural consequence, not the
region oracle or independent geometric embedding evidence.

## Exact map and identity correspondence

Retain the arranged original face identity and every section's source body/
revision/face/chart/piece/event identities. A source cut grid has logical input
identity separate from its coordinate. A geometric event can be canonicalized
inside this arrangement only through its exact construction/source incidences;
nearby points and unrelated equal-coordinate entities are not a persistent name.
Input permutations change storage slots, not semantic curve/region signatures.

For every full fragment, prove original face world(UV(t)) equals its retained
exact spatial curve by polynomial cross multiplication with original homogeneous
controls/maps. Endpoints additionally resolve to the intended graph vertex and
original event occurrence. Circle source maps have positive denominators and
exact direction/range; reversing a halfedge changes traversal, not the represented
spatial curve or original source identity. Sampling a few points or rendering a
polyline cannot certify complete map correspondence.

The arranged face must actually match the intended side of each section's source
reference, or an explicit exact adapter must reproject and record correspondence.
Equal geometry in another body does not authorize silently rebinding topology.
Original wall seam lifts remain distinct source occurrences even when a planar
region event has one physical spatial location.

## Strong literal and symbolic-area fixtures

- No cuts/circles: one rectangle region, its complete four-edge outer loop and
  boundary twins/exterior convention.
- One vertical and one horizontal cut: four ordered cell regions, exact central
  degree4 crossing and complete shared edge uses.
- One circle with no grid: one disk plus surrounding rectangle-with-hole,
  two regions and the same curved edge set traversed in opposite directions.
- Two disjoint circles in one cell: one surrounding region with TWO holes and
  two disk regions. This detects wrong parent and hole-order truncation.
- Several occupied/empty grid cells: complete K+M domain signature set, not only
  witnesses near convenient corners or one sample per original face.
- Swapped/reordered section sources and cut coordinates, translated/scaled
  original face: preserve semantic signatures and exact original world/UV maps.
- A positive separation smaller than a binary64 ULP: preserve distinct events
  and small positive region extents. Exact zero/tangent separation rejects.

Optional exact area records can use A+Bπ with rational coefficients: a grid
cell's area minusπ times the sum of its circle radii²; disk areaπr². Sum all
bounded regions to the original rectangle area andπ coefficient0. Parameter
area and world area differ by the original plane Jacobian; report which one.
This attacks omitted/duplicated region area, while the complete domain/loop model
still determines holes and correspondence. Avoid approximate numerical area
integration as the primary circle-region oracle.

## Qualification, mutations and caller

Direct actual assertions should expose missing disk/cell, wrong hole parent,
hole omission/orientation, wrong twin/next/source endpoint, omitted final circle
quarter, wrong source piece or side UV, float-welded near events and silent
acceptance of unsupported touching/clipping. Select actual product sites after
reviewed design/plan/red; build/import/crash failures do not earn mutant kills.
Typed semantic permutations, exact domain checks and whole-map identities are
the mechanisms, not CPU/GPU parity or a global checksum.

A useful caller exports all input sources, exact events/UV/world vertices,
fragments/source ranges, twins/next/prev, region outer/hole loop words and all
bounded region domains. An actual consumer reads loops/region membership or
builds a source-derived preview; the preview is not mesh embedding proof.
Measure preparation/admission, complete construction, result publication and
consumer costs separately from independent geometry qualification. Keep actual
object counts, source/binary pins and unsupported outcomes. No Boolean or GPU
arrangement speed claim follows from this CPU boundary construction, and
residency/bulk integration remains deferred until after the broad-pass big rocks.
