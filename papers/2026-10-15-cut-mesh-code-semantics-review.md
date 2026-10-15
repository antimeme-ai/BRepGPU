# Cut-result mesher and independent oracle: code semantics review

2026-10-15. **ACCEPTED; no blocking findings.** Static review of the actual
`python/brepgpu/cut_meshing.py`, `cut_meshing_oracles.py`,
`tests/test_cut_meshing.py` and lazy `CutResult.tessellate` adapter against the
accepted design/plan, original Boolean constructor/maps/oracle and the acquired
meshing literature. No geometry, tests or fault injections ran locally in this
review; parent owns direct fleet green and affected compatibility.

## Actual construction

The factory accepts exact CutResult only, retains the actual result/body and
history objects, uses existing immutable leaf records and changes no canonical
mesher admission. Source tuples remain result-scoped; unchanged typed history
retains complete operand/section/arrangement ancestry rather than assigning mesh
slots to input entities.

Physical edge samples are allocated once using exact output edge geometry.
Closed anchors alias t0/1; line endpoints reuse actual original vertex slots.
Every coedge preserves its own original map, with direction applied once. Cap
hole ownership is read from actual loops. Wall rings are selected through actual
wall coedge ownership and ordered by raw original V. Plane inversion applies each
actual patch's own origin/scale. Wall corners retain chart quadrant and original
V1/4..3/4 while the saved lifted boundary maps keep seam U0/4. The output is neither
a relabelled canonical plate nor a refitted/reparameterized wall carrier.

The four-corner cap fans plus outer-side triangles cover the admitted square
minus inscribed ring polygon; radius1/2 is strictly inside half-width3/4. Ruled
wall strips and outer rectangles use the same physical boundary indices. Face
sense reverses bottom/negative planes and inward wall. Counts and distinct-index
nondegeneracy catch construction loss, while the independent qualifier checks
the full domains instead of trusting those counts.

Subdivision is bounded before geometry allocation. World radius and tolerance
are exact ratios. The reported sagitta and actual saved-float displacement sum
must meet the request. The dedicated rounded guard reads saved float ratios,
checks convex/quadrant-ordered rings, matching extruded XY, strict heights and
strict outer nesting, then coplanarity and outward triangle signs. For this
trusted fixed source family these checks establish the required extrusion
embedding; general arbitrary-trim triangulation is not implied.

## Independent whole-output checks

The qualifier validates the retained result with the established independent
Boolean oracle, including original frames, curve records and history. Expected
mesh generators and maps use independent source readers rather than product
readers. Corner reproduction evaluates each saved chart with its own patch frame;
raw wall V-band and lifted seam checks distinguish atlas/chart occurrences.

Each planar face's expected boundary comes from original result loops and each
wall strip uses affine spatial-chord coordinates. Positive triangle orientation
and full integer boundary-chain equality establish exact domain coverage. The
same argument is repeated on actual rounded planar/strip coordinates. Original
minor-arc inequalities bound the whole arc, and minimum radial distance checks
the complete exact triangles. Independent ring simplicity, convexity, height and
nesting checks plus physical opposite uses, cyclic links, connectedness and
Euler0 cover the declared rounded geometry and topology claims.

The direct suite covers every admitted source scale, translated extrema and
subnormal origins, coarse/refined requests and exact threshold transitions,
immutability, type/budget rejection and healthy retry. Altered actual records
attack missing/duplicate/flipped/filled-hole triangles, source/map/chart/seam
correspondence, split identities, saved rounded values and reported bounds.
Those are named fault sensitivities rather than an implied general mutation
score. Native library and device campaigns are not relevant to this CPU-only
constructor.

No valid issue was found requiring production or oracle edits. Acceptance is for
the inspected code semantics; saved caller artifacts and measured interpretations
still need their own review after fleet execution.
