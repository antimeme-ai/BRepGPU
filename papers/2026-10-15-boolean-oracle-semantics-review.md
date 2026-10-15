# Boolean independent oracle: semantics review

Disposition: **accepted after O1 correction**. Source-only review of
`python/brepgpu/boolean_oracles.py` against the accepted design, complete public
construction contract and actual source representations. Direct test expansion
was still in progress at the first read; no product execution from this lane.

O1: several graph ownership tuples checked immutable shape and value equality
without strict integer element types. Python bool aliases can satisfy indexing,
tuple equality and Counter equality for face loop/chart slots and loop coedge
slots. Chart metadata and the output revision also need strict integer checks.
Retained vertex/Curve data used rational/value equality although the source
preservation contract requires actual original binary64 values; Fraction/int
aliases could pass that narrower storage contract. Sent exact locations and
recommended direct falsified-slot/source-value cases to the independent owner.

The whole-map mechanisms otherwise appear sound: all quarter-circle spatial and
both incident UV maps use independent rational-polynomial identities; lifted
seam U0/U4 closures, opposite physical edge uses, closed-edge endpoint ports and
connected vertex links are checked. Complete original source history is rebuilt
by roles, including source ranges/UV bands, orientation and actual section/
arrangement occurrence keys; it does not trust returned count labels.

Requested explicit final coverage for public exact material/trim reader probes:
the initial result qualifier checked complete stored domains/maps but did not
yet call its independent material helper. The still-growing direct test suite
must attack those public reader faults at outer faces, cap rims, seam, corners
and nextafter queries. Native sample truth already computes independent actual-
binary64 trim labels, original-source world points and inward wall normals.

Final review: verified the actual owner correction. Graph face/loop slot
elements, chart metadata, result revision and native lineage now require strict
integer types. Retained source vertices/curves check original float types and
binary64 bits. Direct falsified bool-slot/chart/revision and Fraction source-
value cases exercise these guards. O1 is resolved.

`qualify_result` now invokes independent public reader checks: 49 exact world
material/support probes span every outer face/edge/corner, four rim directions,
the seam, removed cap disks and nextafter/subnormal offsets. Another 140 trim
queries distinguish exact rational rims from their rounded binary64 neighbors,
the original wall V band, internal chart joins, true seam and invalid domains.
These checks address reader behavior separately from the whole stored-map proof.

Read all ten completed direct methods. They exercise every admitted scale,
translated source frames, full all-kind history, exact public maps/material,
actual CPU/P100 sampler placement and query binding, the actual existing typed
adjacency reader under slot permutations, falsified geometry/history/aliases,
and late construction failure preserving inputs and a usable previous result.
The wall normal truth uses the inward unit radial vector in the original cutter
frame. Native position/normal limits are explicit, not inferred from CPU parity.
Selected falsified outputs establish named fault sensitivity, not mutation
coverage or external Boolean alpha qualification. No blocker remains.
