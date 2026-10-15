# A complete planar arrangement beachhead

Research, 2026-10-14. Read current doctrine/roadmap, owned topology, exact
plane/wall sections and finite-face classification. Read primary CGAL arrangement
documentation and the actual pinned OCCT `BOPAlgo_BuilderFace` source/header.
No implementation, dependency adoption, execution or language-role decision.

## Scope verdict

Prefer a horizontal rectangular face partitioned by rational axial chords and
strictly separated full interior circles, each wholly inside one grid cell.
Consume existing complete horizontal plane/wall circle sections; optionally
consume the exact transverse rectangular-face contact segments as chord sources.
This gives degree-four crossings, shared boundary events, opposed coedges,
ordered regions and multiple holes, with a complete combinatorial oracle.

One arbitrarily clipped/tangent circle sounds smaller but requires substantially
more semantics: tangent boundary vertices can join distinct loop occurrences,
zero-dimensional contacts may not split a region, coincident trim arcs need
ownership rules, and pinched cells need a declared regular/open/closed meaning.
Those are valuable return-pass units. Merely accepting the existing section's
complete arc list does not solve embedding, region association or cyclic order.
The grid/interior-circle family is narrow intentionally, with no lowered
correctness standard and no claim about a generic arrangement sweep.

## Primary reference lessons

CGAL's arrangement manual separates curve geometry from DCEL incidence. Edges
have twin directed uses; face boundaries may have multiple connected components
and holes, while isolated vertices are different records. Its algorithmic
decomposition can add vertices where curves are split into monotone pieces.
That is a representation refinement, not evidence of a new physical CAD event.
Our owned representation can keep a full circle edge with piecewise geometry
instead of exposing each source chart as a physical edge.
[Primary arrangement manual](https://doc.cgal.org/latest/Arrangement_on_surface_2/index.html).

Pinned OCCT BuilderFace saves original face orientation, works on a forward
face, forms wires, distinguishes growth/hole loops, assigns holes to containing
faces and handles internal shapes separately. Its point-in-face context and
wire classification operate under its own geometric/tolerance contracts.
We learn the separation of loop construction, containment and source orientation;
we do not copy its pointers, tolerance decisions or package implementation.
[Actual source](https://github.com/Open-Cascade-SAS/OCCT/blob/b8f597c677811d1f9f4d8a97f5ae2825c0353a42/src/ModelingAlgorithms/TKBO/BOPAlgo/BOPAlgo_BuilderFace.cxx).

Fogel/Halperin/Wein's 2012 book is an identified follow-on reference, with
publisher metadata verified; the full book was not acquired or represented as
read. The primary manual and retained source suffice for this bounded design.
[Publisher](https://link.springer.com/book/10.1007/978-3-642-17283-0).

## Exact admission

The target is one existing horizontal, single-chart rectangular `FaceRef`.
Retain its complete body/revision/face/chart provenance and exact control net.
Grid coordinates are rational, distinct and strictly inside its two intervals;
cuts span the whole rectangle. A contact-sourced cut must name this exact target
lineage and represent the complete axial chord, not a partial or boundary touch.
Reject duplicate/overlapping cuts instead of quietly combining source histories.

Each circle comes from one complete existing plane/wall section, bound to the
target lineage in either operand order. Require its full circle component and
source geometry, not a sampled polyline or a clipped arc accepted as closed.
Its center/radius are rational in this owned family. Exact extents must lie
strictly inside one grid cell. For every pair, require
`distance(center_i,center_j)^2 > (radius_i+radius_j)^2`.
This separates closed DISKS; tangent, coincident, overlapping and nested disks
are explicitly excluded. Disjoint circumferences alone would wrongly admit
nesting. The strict tests use original exact values and can admit arbitrarily
small positive rational clearance; no epsilon margin is an identity rule.

Repeated sections of the same source pair need explicit duplicate rejection.
Different source cylinders with equal circles also fail separation, rather than
being merged by coordinates. All failures preserve inputs and publish no partial
arrangement. General splines, tilted faces, clipped/tangent circles and curves
crossing cells remain outside this first family.

## Records, shared events and one closed circle

Use owned result-local vertices, edges, coedges/halfedges, loops and regions.
Slots are typed addresses in one immutable result, never persistent body names.
Each edge has one geometric source and a pair of opposite uses with explicit
region/loop ownership. Source snapshots, sections, original parameter maps and
query results remain retained; no new whole-body snapshot is implied.

Grid crossings have canonical source-incidence keys `(u-cut-source,v-cut-source)`.
Cut/boundary events use `(cut-source,original-boundary-edge)`; original corners
use the original vertex source. These keys, rather than near XYZ values, create
one shared event across incident edges. A circle anchor has `(section-source,
declared-anchor)` provenance. Result-local section event/span integers alone
cannot name an event across independent section results: include source/result
context. Input order may change result slots; semantic comparison resolves the
complete source-incidence keys.

A full circle should be one CLOSED arrangement edge with one explicit anchor
vertex and equal start/end slots, matching the owned cap-ring model. Retain the
entire ordered four-chart section and maps on that edge as geometry/provenance.
Atlas joins are map breakpoints, not four new feature edges. Its disk region
uses the edge once CCW and its containing cell uses the twin once CW as a hole.
The same spatial edge identity serves both; neither geometric reversal nor
opposed source operand order creates a second circle edge.

Canonical loop order is the authority. Any next/previous fields derive from it
and must agree; twins reverse source/target and traversal without changing the
underlying edge. At degree-four grid events, cyclic order comes from exact
axis directions; no floating atan2 or arbitrary reservation order is needed.

## Regions, exterior and orientation

For k columns, l rows and m circles, bounded regions are the kl grid cells plus
one disk per circle. A cell has one rectangular outer cycle and one circular
hole cycle for each contained disk. Disk regions have one outer circle cycle.
Orient loops with region interior on the left in the TARGET UV chart: outer
cycles CCW, hole cycles CW. Original source face orientation is separate
metadata; these are not already Boolean outward coedge assignments.

Include an explicit exterior region/twin cycle if calling the object a complete
DCEL. Rectangle-boundary edges then have one bounded use and one exterior use;
internal cuts/circles have two bounded uses. The exterior has the reversed
whole rectangle boundary, including all cut endpoint splits. A restricted model
without exterior twins must instead declare its boundary convention explicitly.

With one anchor per circle and full exterior:

`V=(k+1)(l+1)+m`, `E=2kl+k+l+m`, `F=kl+m+1`.

The embedded graph has m+1 components, so `V−E+F=m+2=1+components`.
Demanding the connected-graph value 2 would reject correct interior circles.
Alternatively the hole-aware sum over bounded regions is
`V−E+sum(1−holes(region))=1` for the target rectangle.
Plain bounded-region counts alone do not establish disk-cell Euler topology.

## Membership and useful exact witnesses

Circle centers are rational witnesses for disk regions. A grid-cell midpoint
can lie in a hole, so do not assign it blindly to the remaining cell region.
Choose a positive rational displacement near a cell corner using the strictly
positive circle-to-cell clearances. For example, pick delta smaller than cell
width/height and every circle's left-side clearance; `(xmin+delta,ymin+delta)`
then lies inside the cell and to the left of every hole disk. No floating search
or rounded section endpoint is needed.

A point reader should return exact vertex/edge/region incidence, keeping closed
boundary membership distinct from strict region interiors. A grid crossing is
incident to four cells, a regular cut point to two, and a circle point to its
disk plus surrounding cell. Decide whether exterior is included in boundary
incidence and document it. Queries off the source plane fail rather than being
silently projected. Region material selection for a future Boolean remains a
separate operation, although the rational witnesses can already use existing
solid membership. Source plate-wall material sense must not be mistaken for
filled-cylinder disk selection.

## Independent truth and actual consumer

Use direct exact source maps for whole-edge circle/line correspondence; loop
endpoint equality alone cannot prove a curve or pcurve. Check every twin,
directed loop closure, cyclic incidence, source key, cell-hole assignment and
all expected region counts. Whole-circle geometry must retain all source pieces
and both original surface representations, including duplicate seam lifts.
Exact containment/separation gives independent embedding conditions; topology
incidence alone cannot rule out curve crossings or incorrect hole ownership.

Independent witnesses/point location, source-order permutations and hole-aware
Euler attack different defects. Literal grid/circle fixtures establish complete
expected regions; an independent geometric reader checks output boundaries
rather than copying the constructor's cell-building helper. Direct mutants
should target shared event duplication, wrong twin region, reversed hole,
missing source circle piece, hole assigned to wrong cell and grid-crossing
ordering. Mutants/native work stay on the fleet.

A usable caller exports actual records/source maps, traverses each region and
locates exact witness/boundary points; an owned SVG draws those output loops and
holes. Measure construction, source preparation, output consumption and complete
wall, with independent qualification separately visible. CPU is a valid first
placement. GPU/session/native-language decisions and generic arrangement depth
remain after the broad-pass big rocks.
