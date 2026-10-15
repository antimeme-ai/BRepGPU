# Saved planar arrangement graphs and diagram: independent review

Accepted `results/arrangements-2026-10-14/caller/`; no finding. Read all five
complete serialized graphs and original source tables using an owned stdlib-only
CSV/Fraction/XML checker in ignored `context/arrangements/`. No product imports,
product execution, or measured-source edits occurred locally. This review checks
saved geometry, topology, provenance and display; fleet execution and performance
qualification remain separate mechanisms.

## Original sources and complete exact geometry

The five literal recipes match their saved original target bodies, revisions,
face/chart IDs and frames: bare rectangle, one circle, a two-by-two cut grid with
three circles including two in one cell, 64 holes in one cell, and 32-by-32 cells
with 64 circles. Source namespaces contain 137 distinct bodies with the expected
1–137 IDs; every section's target identity equals the actual target record and
its other source is the declared cylindrical wall. All eleven pinned source
hashes match current source text.

Verified all 665 source chart rows and 3,716 homogeneous control rows against
the original box plane and four cylinder quarter patches, including chart slots,
IDs, quadrant domains, orientations, frames and original weighted controls.
Original world frames independently determine every disk center/radius and
its strictly containing grid cell. The actual circle footprints are strictly
separated from cell cuts/borders and one another.

All 528 original arc pieces, 132 complete closed components and 528 source events
retain their exact IDs, order, spans, slots, domains, incidences and atlas joins.
Checked every one of 13,200 serialized coefficient rows: the spatial maps equal
the exact world quarter-circle rational formulas, left UV maps equal the original
target plane's affine inverse, and right UV maps retain native local quarter
parameter and wall height. This is complete coefficient equality, rather than
agreement at sampled positions. Shared source event endpoints agree exactly;
the four source chart/event identities survive the single closed graph edge.
The anchor references original source event 0, not four synthetic graph vertices.

All twenty original boundary coedge/edge records retain the expected canonical
line maps and spatial endpoints. Every serialized boundary segment reproduces
both original UV and world endpoints using its actual directed source parameters.
Top and left canonical positive-axis segments correctly retain decreasing
original parameters. No second traversal-sign reversal is applied.

## Complete domain and graph checks

Independently enumerated original cut-grid vertices and segments and one closed
circle edge per source section, then required a complete bijection with saved
records. Every vertex has the exact UV/world position and original grid or
source-event identity. Every edge has the correct complete source description;
interior cuts and original boundary subranges remain distinguishable.

For every original cell, the corresponding region has exactly its CCW rectangular
outer loop and the complete set of contained clockwise circle holes. Each source
disk has exactly one CCW outer loop. The outside region retains the complete
clockwise original perimeter, its hole role, and no finite-area value. These
checks enumerate source domains and directed loop words; counts or Euler
characteristics are not used as geometric truth.

All uses occur exactly once in their declared loop. Every next/previous link
matches that loop, directed endpoints meet, and every edge has exactly two
opposed uses with reciprocal twins in distinct owning regions. All region,
loop, edge and vertex IDs form the complete dense exported namespaces.

| Recipe | Vertices | Edges | Uses | Loops | Regions incl. outside | Circles | Locations |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| bare | 4 | 4 | 8 | 2 | 2 | 0 | 9 |
| single | 5 | 5 | 10 | 4 | 3 | 1 | 11 |
| mixed | 12 | 15 | 30 | 11 | 8 | 3 | 23 |
| many-holes | 68 | 68 | 136 | 130 | 66 | 64 | 137 |
| grid32 | 1,153 | 2,176 | 4,352 | 1,153 | 1,089 | 64 | 2,245 |
| Total | 1,242 | 2,268 | 4,536 | 1,300 | 1,168 | 132 | 2,425 |

Each bounded region's exact normalized UV area is its source cell rectangle
minus precisely its disk areas, or its source disk area, as rational + π times
rational. Summing all bounded areas gives exactly 1 + 0π. Every region witness
belongs strictly to that region; disk witnesses equal their source centers.

Independently reconstructed the literal location query sequence from all saved
region witnesses, all graph vertices and the three declared additional queries.
All 2,425 saved answers equal the complete source-domain closed membership set,
including exterior on the original border, all grid-crossing incident cells,
and both cell/disk regions at circle anchors. Query IDs, ordering and reported
counts agree. The saved query suite is finite; these records alone do not claim
exhaustive point-location behavior for every rational point.

## SVG corresponds to the complete saved graph

Parsed the actual SVG and reconstructed its declared panel transform directly
from exact saved geometry. Every one of 2,268 edge polylines matches the complete
expected reader path at the displayed three-decimal rounding. All 132 circles
use their exact rational quarter parameterization at the declared 65 display
samples, with the expected circle/line colors.

All 1,163 bounded-region fill paths reproduce their actual outer loops and every
hole, including 64 holes in the large one-cell case and the complete 32-by-32
grid. Directions and joins follow each saved use; the even-odd fill rule and
region colors agree. The 17 vertex dots on the single/mixed panels and all
corresponding region witness labels match their actual coordinates. Recipe,
region-count and edge/opposed-use labels match the exact exports.

The diagram explicitly states that display samples are not a mesh bound. This
review supports exact source-coupled topology and closed region construction
for the admitted grid plus strictly interior disjoint-circle family; it does not
extend the implementation to clipped, tangent, overlapping or general curves,
or imply a GPU arrangement implementation.
