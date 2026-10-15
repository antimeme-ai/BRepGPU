# Complete off-axis profile revolution

2026-10-15. Actual convex XY profiles now construct owned closed curved solids
on CPU, preserving their original placement and complete feature history.
Existing CPU/both-P100 first-jet and adjacency consumers read the result.
[API/support](../../docs/REVOLUTION.md), [design](../../docs/REVOLUTION_DESIGN.md),
[reviewed plan](../../docs/REVOLUTION_PLAN.md).

![Actual 16-edge revolution and P100-evaluated points](costs/many16-cuda-device0-threads1-r1/revolution.svg)

## Complete geometry, periodic topology and source ancestry

The exact factory PlanarProfile remains an actual XY polygon with 3–32 original
CCW strictly convex vertices on the 1/256 grid. Radial X lies in [1/4,3/4]; axial
Y remains in [-3/4,3/4]. The positive-Y axis passes through the retained Frame
origin. At zero rotation the meridian is the actual original source embedding.
There are no full-turn end caps. Positive radial clearance and global strict
support establish a complete embedded boundary, the source boundary times S1.

Every original edge produces one swept face, with four exact rational degree(2,1)
quarter charts covering all directions/endpoints. Dyadic homogeneous controls
use weights 1/1/2 and the original Frame. No sqrt-weight rounding, fitting,
coordinate welding or canonical-solid substitution is used. Matching positions
and normals at chart joins do not imply equal raw angular parameter speeds.
Each source vertex produces a closed ring edge; each source edge has a meridian
seam whose two opposed uses belong to the same face. Closed-ring endpoints
remain distinct ports in each four-port vertex link. All physical uses, complete
UV words, connected genus-one incidence and exact source-domain maps are checked.

| Recipe | Source N | Output V/E/uses/faces/charts | Source entities | Output origins | Modified/generated | Native candidates/admitted |
| --- | ---: | --- | ---: | ---: | --- | --- |
| triangle | 3 | 3/6/12/3/12 | 12 | 39 | 12/27 | 972/972 |
| chamfer | 6 | 6/12/24/6/24 | 21 | 78 | 24/54 | 1944/1944 |
| many16 | 16 | 16/32/64/16/64 | 51 | 208 | 64/144 | 5184/5184 |

Every source vertex/edge/coedge has 1/1/2 modified same-kind images. Source
loop/face/chart are consumed from boundary images while retaining contextual
generated descendants. All 3N+3 sources and all 13N outputs are covered, with
4N modified and 9N generated origins. Precise generating IDs/roles and exact
source_t/source_uv/periodic maps are checked; reciprocal but wrong ancestry is
rejected. Periodic emits the rational quarter coordinate, not radians. Faces
and loops use raw tau; charts use normalized native parameters. Late failure
preserves source and previous result and exposes no partial feature/history.

Material membership uses exact sign-aware A+B*sqrt(X²+Z²) comparisons after
exact Frame inversion, returning every supporting face. It never rounds a radial
square root or squares away coefficient signs. Volume is the exact pi coefficient
from twice the source polygon's radial first moment. Original source lines and
explicit circle_y curves supply exact boundary readers. Separate result types
preserve existing canonical/Boolean/extrusion and packed/mesher contracts.

## Qualification

| Unit | Actual fleet outcome |
| --- | --- |
| CPU construction/history | 8 methods passed, 0.867 s after final typed-slot correction |
| Native sampler/lifetime | 8 methods each CPU1/CPU24/P1000/P1001, 0.479/0.391/2.241/3.619 s |
| Independent whole feature/native/topology | 11 methods each CPU1/CPU24/P1000/P1001, 10.844/11.103/10.827/11.472 s, zero skips |
| Affected existing contracts | 89 methods: 88 passed, one existing CUDA-only skip, 22.875 s; that device-restoration method passed separately on CUDA, 0.582 s |

These are 27 new methods across applicable placements. Actual 32-vertex/128-chart
and minimum-edge fixtures, every Frame scale, cyclic relabeling, distinct equal-
geometry namespaces, off-axis admission, malformed records and type aliases are
covered. Direct exact rational material witnesses include nonsquare radii,
sign-squaring traps, all supporting faces, subnormal/nextafter perturbations and
the central hole. Actual same-face seams and internal quarter joins have separate
checks. Deliberately falsified controls/history/maps/native output and slot
permutations target named faults; no broad mutation score is claimed.

The independent model reconstructs full graph/carrier/domains/history from
original profile vertices and Frame. Denominator-cleared rational identities
check entire ring/seam-to-incident-UV maps, not just a sampled fit. Native truth
uses actual copied binary64 UV; unit-normal bounds use independent integer-sqrt
intervals for original lattice-edge lengths, multiplied by exact circle factors.
Across all 48 callers, the largest world-position component error was
3.1974423109204507e-16; the largest conservative
oriented unit-normal component bound was 1.841801707394375e-16.
Limits remain 2^-36 and 2^-30. Every grid row is admitted and every native status
successful; invalid-query direct cases retain explicit status-1/NaNs.

Actual ABI5 readers return N ordinary ring pairs and N same-face seam pairs,
including full edge/face/coedge permutations. Descriptor/status rejection and
late host failure close all three actual native child handles, then retry on
the borrowed Engine succeeds. All five protected native core/CUDA/topology
hashes match before/after; no native build or new dependency was introduced.
[Construction review](../../papers/2026-10-15-revolution-construction-root-review.md),
[independent code review](../../papers/2026-10-15-revolution-construction-oracle-code-review.md),
[oracle review](../../papers/2026-10-15-revolution-oracle-semantics-review.md),
[sampler review](../../papers/2026-10-15-revolution-sampler-root-review.md).

## Complete caller costs

Three actual profiles times CPU1/CPU24/P1000/P1001, one excluded warmup and
three measured fresh processes each: 48 qualified calls, 12 excluded warmups,
36 measured observations in 12 cells. Grid9 gives distinct topology/query sizes;
these are small features, not a large-model throughput study. Shared Linux
x86_64 fleet, Python 3.12.3, two Tesla P100-PCIE16GB, driver580.178.04,
existing CUDA12.8.1/sm60 binaries. Clocks/load/filesystem caches are uncontrolled.
No simultaneous project GPU qualification or residency/tuning jobs ran.

Prepared-profile→consumed wall includes complete CPU feature publication, exact
material/edge/UV/history reads, actual affine history-map evaluation and original
profile boundary reads, request work, Engine and child setup, upload/evaluation/
download/provenance/status/frame/normal/scatter consumption, cleanup, topology
packing/library/context/build and actual typed CSR mates/neighbors. Retained
history-map-reads.tsv has 111/222/592 real map-reader rows. Input-points→consumed
also includes profile creation from already-owned exact point tuples.
Independent qualification, exports/SVG, foreign comparison, imports and process
launch are outside; full child wall is separately saved. No rows are included
solely to inflate native batch size.

| Recipe | Placement | Median profile→consumed ms | Range ms | Median points→consumed ms |
| --- | --- | ---: | ---: | ---: |
| triangle | CPU1 | 53.963 | 53.931–62.725 | 54.736 |
| triangle | CPU24 | 65.551 | 64.319–79.615 | 66.418 |
| triangle | P1000 | 658.223 | 653.901–746.388 | 660.001 |
| triangle | P1001 | 1172.783 | 1108.347–1279.689 | 1173.379 |
| chamfer | CPU1 | 285.688 | 161.101–303.390 | 289.063 |
| chamfer | CPU24 | 161.197 | 121.315–173.927 | 162.540 |
| chamfer | P1000 | 746.626 | 677.137–817.885 | 747.887 |
| chamfer | P1001 | 1270.603 | 1190.142–1468.337 | 1271.832 |
| many16 | CPU1 | 410.476 | 357.085–519.315 | 415.964 |
| many16 | CPU24 | 361.477 | 359.308–405.451 | 366.867 |
| many16 | P1000 | 1008.083 | 889.299–1280.253 | 1013.678 |
| many16 | P1001 | 1754.468 | 1586.533–1823.588 | 1759.767 |

CPU1 has the best triangle median; CPU24 has the best chamfer/many16 medians.
Every P100 complete-call range is slower than the corresponding best-CPU range.
CPU ranges vary and overlap for many16; these unpinned three-observation cells
do not isolate thread/clock/load causes. CPU construction remains CPU-owned.
Separate cold geometry/topology setup, cleanup, exact host reads and scatter
remain return-pass candidates. For many16, diagnostic phase medians are:

| many16 phase ms | CPU1 | CPU24 | P1000 | P1001 |
| --- | ---: | ---: | ---: | ---: |
| cpu_revolution | 58.957 | 53.128 | 61.456 | 89.253 |
| material_boundary_history_consume | 146.493 | 124.100 | 140.150 | 210.745 |
| request_prepare | 69.638 | 66.438 | 93.059 | 65.349 |
| engine_setup | 2.103 | 2.104 | 280.940 | 523.191 |
| sample_and_consume | 122.174 | 111.272 | 109.388 | 116.211 |
| sampling/normal_scatter_ms | 100.604 | 89.127 | 88.627 | 91.586 |
| engine_cleanup | 0.019 | 0.018 | 41.314 | 157.477 |
| adjacency_build | 0.189 | 4.435 | 232.483 | 463.622 |
| independent_qualification | 1377.393 | 1375.075 | 1735.442 | 1498.105 |

Diagnostic medians are **not additive**: sampling/construction/topology stages
nest in enclosing walls, and medians may select different repetitions. Internal
construction total_ms precedes final returned metadata; the enclosing constructor
wall includes publication. evaluate_ms is synchronous unprofiled wrapper wall,
not isolated kernel time. Topology initializes its own per-call context rather
than borrowing sampling's Engine. [Raw calls](costs/raw.tsv), [summary](costs/summary.tsv),
[timing scope](costs/scope.tsv), [controller pin](costs/sources.tsv),
[caller review](../../papers/2026-10-15-revolution-caller-runtime-review.md),
[final performance review](../../papers/2026-10-15-revolution-performance-runtime-review.md).

## Optional same-world reference: one comparison remains red

The already-installed isolated USD BRep1.0.0 reference receives exact world source
line endpoints, positive Y axis through Frame.origin and full360 rotational_sweep.
Original parameter domains and three probes per curve remain unchanged afterward;
this is that explicit probe scope, not a full foreign curve/history certificate.
Returned finite bounds/manifold/volume and actual closest points are measured.
Every witness set includes five samples per physical edge plus two spread native
interior rows per quarter chart, eight per face. Foreign representation/face
counts/parameters/history/material are not substituted for owned truth.

| Recipe | Overall comparison | Volume absolute error | Volume relative error | Boundary residual maximum | Witnesses |
| --- | --- | ---: | ---: | ---: | ---: |
| triangle | FAIL: volume | 1.28457741e-08 | 2.0609047e-08 | 3.14018492e-16 | 54 |
| chamfer | PASS | 5.91094451e-10 | 3.21111394e-09 | 1.26097096e-15 | 108 |
| many16 | PASS | 2.95846907e-08 | 8.03592493e-09 | 1.42177919e-15 | 288 |

All three source-preservation/manifold/bounds/boundary comparisons pass. Thresholds
remain volume relative1e-8, bounds1e-8*scale and boundary distance/residual1e-7*scale.
The triangle volume fails: reference0.6233075396566397 versus exact-source pi-volume
floating value0.6233075268108655; absolute difference1.2845774111092112e-8 exceeds
6.233075268108656e-9. Independently, source area53/256 and centroid radius23/48
give pi coefficient1219/6144, agreeing with owned exact geometry. No evidence
found justifies replacing that value with the foreign volume.

A tighter requested reference integration accuracy returned the same result;
actual pinned public wrapper clamps values below1e-8 to1e-8. The implementation's
nested area-derived controls do not establish a guaranteed global volume bound.
This explains why the request cannot tighten, but does not turn the failed
comparison into a pass or establish an upstream defect. The final adapter keeps
request1e-8 and **writes every outcome before exiting nonzero for the triangle**.
It exits0 for chamfer/many16. No tolerance was widened. [Open discrepancy](https://github.com/antimeme-ai/BRepGPU/issues/43),
[actual source investigation](../../papers/2026-10-15-revolution-reference-volume-investigation.md),
[triangle outcomes](reference-triangle/outcomes.tsv),
[chamfer outcomes](reference-chamfer/outcomes.tsv), [many16 outcomes](reference-many16/outcomes.tsv).
The finite reference witnesses do not establish full geometry/material/history
qualification or complete required external alpha suites.

## Artifacts and remaining work

Every caller retains actual source points/Frame, complete output graph/carriers,
forward/reverse ancestry and map-reader outputs, exact material/edge/UV reads,
actual native samples, packed topology inputs/CSR arrays and typed reader output.
Fifteen owned source pins and actually used native hashes remain visible; the
reference separately pins its adapter and exact owned input artifacts. The SVG
contains actual rational ring polylines/source seams and native points, with
explicit CPU construction and observed native placement. It is not a result mesh.
[Saved artifact review](../../papers/2026-10-15-revolution-unit-artifact-semantics-review.md).

Fillet is next, then analytic offset as a separate unit. Partial angles, arbitrary
axes, pole degeneracies, curved/nonconvex/holed meridians and result meshing are
later feature families. GPU construction/residency/integration, the Python-role
investigation and NCSX matched rendering timing remain deferred. Required external
community alpha qualification is incomplete; this delivery and the two passing
optional foreign cells do not waive the remaining corpus/adapters or the recorded
triangle disagreement.
