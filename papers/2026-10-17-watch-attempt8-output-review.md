# Complete native watch output audit

Read-only peer review of the completed CPU12 and P100 attempt8 artifacts.
No native build, meshing run, GPU job or original-output geometric qualification
was launched by this review. Both processes exited zero and emitted the complete
model marker. The supplied output is the native cached mesher's result.

## Inventory and semantic artifacts

Parsed the retained original source inventory and compared its exact
definition/face word with both output indices. Each has exactly 107 unique
query/definition rows and 9,162 unique query/definition/face rows, with neither
missing nor extra original faces. Every indexed mesh header names its expected
original definition and face, has the matching face-boundary generation, and
retains the source digest. Query generation 1 inside the mesh and output query
ordinal 0 are different counters. All indexed boundary snapshots exist; every
definition has one final-boundary manifest referencing an existing snapshot.
The indices retain 110 distinct boundary snapshots, including prior face captures.

Per-definition facet counts match the mesh headers and total **273,978** in
both runs. The final face index is byte-identical between CPU and P100, and
each definition has the same final facet count. Each scene header has 107
definitions, 166 instances, 258,489 display vertices and 273,978 triangles.
All 166 label, occurrence and usage-path identities are unique; label occurrences
and original definitions agree with the scene instance/definition records.
The source contains 196 usage records; those are not 196 leaf placements.

The retained source copies match byte hashes. CPU and P100 scene buffers,
view buffers and labels each match byte hashes. Assembled and exploded PNGs
exist for both; their pixel hashes differ across display backends, so this
review does not claim pixel identity. The matching scene/view/label buffers
establish the same scene inputs and view/label choices.

Final component contact reports total 2,218,277 candidate pairs and **zero
retained forbidden pairs** on each backend. This describes the native final
component contact passes, not an independent inter-instance assembly collision
qualification. Both runs report 9,169 assignments, three boundary rounds and
186 retained-face reuse events; assignments/reuse events are not unique faces.

## Numerical device work and resource accounting

Curve columns are per-component query counts and are summed across the 107
rows. Surface columns are cumulative snapshots of the same retained owner pool:
their last row is the total. All surface cumulative columns were monotone;
summing their rows would count earlier work repeatedly.

| Actual counter | CPU12 | P100 |
|---|---:|---:|
| Curve device points, sum | 0 | 465,553 |
| Curve device bounds, sum | 0 | 420,028 |
| Curve uploaded bytes, sum | 0 | 356,455,092 |
| Curve downloaded bytes, sum | 0 | 78,020,312 |
| Surface device points, final cumulative | 0 | 215,809 |
| Surface device bounds, final cumulative | 0 | 169,649 |
| Surface device cap facets, final cumulative | 0 | 229 |
| Surface uploaded bytes, final cumulative | 0 | 579,874,360 |
| Surface downloaded bytes, final cumulative | 0 | 61,834,128 |
| Cap facet tests, final cumulative | 229 | 229 |
| Domain replacements, final cumulative | 11 | 11 |
| Run peak including reservations, bytes | 8,357,302,903 | 8,752,509,773 |
| Process peak RSS, KiB | 1,987,632 | 2,072,772 |

The positive P100 geometry counters are distinct from display rendering work.
They do not mean every operation ran on the GPU. The reservation-inclusive
RunResource peaks remain below the configured 32 GiB cap and are distinct from
RSS; neither peak should be summed across component rows.

## Timing and scope

| Recorded time, seconds | CPU12 | P100 |
|---|---:|---:|
| Process wall | 875.16 | 872.50 |
| Process user | 1,605.28 | 1,571.69 |
| Process system | 7.24 | 8.09 |
| Complete meshing, sum of component scopes | 847.712234 | 845.785060 |
| Scene | 12.088713 | 11.836033 |
| Assembled render total | 0.222594 | 0.046141 |
| Exploded render total | 0.114025 | 0.042798 |

Each backend retains **889 unique timing scopes**, including source preparation,
per-component phases, scene, display, annotations and PNG writing. Render totals
are their named display scopes, not whole-process or PNG-inclusive times. These
were concurrent development runs; no uncontended throughput or speedup claim
follows from their wall-time difference.

Verdict: the output inventory, counter aggregation and matching semantic scene
artifacts support the complete native CPU/P100 delivery claim. This audit does
not add a whole-watch original-source error certificate or turn the independent
external reference's retained failures into a complete reference image.
