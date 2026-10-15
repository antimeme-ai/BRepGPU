# Complete owned box-edge fillet

2026-10-15. One actual full vertical edge of the owned canonical box now rounds
into a complete immutable closed solid on CPU. Existing CPU/both-P100 native
first-jet and adjacency consumers read its actual geometry and topology.
[API/support](../../docs/FILLET.md), [design](../../docs/FILLET_DESIGN.md),
[reviewed plan](../../docs/FILLET_PLAN.md).

![Actual rounded box and P100-evaluated samples](costs/mid-cuda-device0-threads1-r1/fillet.svg)

## Complete geometry and original source maps

Select exactly one original vertical edge9..12. Local radius is on the exact
1/256 grid in[1/16,1/2]; the bounded original Frame remains. Six original plane
control/raw-UV/Frame payloads retain their binary64 bits. Two rounded cap domains
and two shortened original side intervals supply exact closure; one rational
quarter-cylinder degree(2,1) band has exact dyadic controls and weights1,1,2.
The four shortened cap edges and their incident UV maps retain exact nonbinary
2r/3 subranges. No shortened replacement plane or fitted surface is substituted.

The output has10 vertices,15 edges,30 distinct coedges,7 loops,7 faces and7 charts.
All15 physical edges have opposed ordinary uses; the connected genus-zero shell
has one three-port cycle at each vertex. Two complete side contact rails agree
in world position and outward normal on original and blend carriers. Cap joins
remain sharp. This exceptional orthogonal-plane family is not a general
rolling-ball, endpoint-blend, interacting or variable-radius solver.

All62 original entities have33 retained,24 modified and5 deleted dispositions.
All76 output origins have33 retained,24 modified and19 generated roles. Original
edge/coedge parameter maps identify actual owners and distinguish coincident
source-curve correspondence from selected-edge axial spine correspondence.
Every source UV map names one original face/chart, including contextual owners
that are not causal parents. Deleted selected-edge uses can parent new rails
without claiming surviving images. Four changed-domain charts are modified even
though their plane controls remain unchanged. Actual forward/reverse histories,
map evaluation and both contact-map readers are consumed inside caller timing.

Material membership and all supporting face IDs follow exact reflected rounded-
square inequalities after exact Frame inversion. Unused portions of the full
circle and artificial section splits are not physical boundary supports.
World volume is exactly (9/4-r²)*scale³ + pi*(r²/4)*scale³. Whole rational
edge/both-UV/carrier identities are coefficient checked before publication.
Failures preserve the original source and previously published results.

## Qualification

All three independent design and plan reviews accepted before the actual fleet
red: the healthy existing native prerequisite passed, then28 new methods and the
real caller failed on missing implementation. The implementation introduced no
native build or dependency; [all five protected native hashes match before/after](native-preservation.tsv).

| Unit | Actual fleet outcome |
| --- | --- |
| CPU construction/history/contact | 8 methods passed,2.407s after bounded Map2/parent admission correction |
| Native sampler/lifetime | 8 each CPU1/CPU24/P1000/P1001,1.087/1.080/3.104/4.734s |
| Independent whole feature/native/topology | 12 each CPU1/CPU24/P1000/P1001,6.845/6.554/7.469/7.474s; zero skips |
| Existing affected contracts | 116 methods,115 passed and one expected CUDA-only skip,35.412s; skipped device-restoration method passed separately on CUDA,0.457s |

[Placement records](qualification.tsv). These28 new methods cover all four corners, radius extremes, every bounded Frame
scale, original payload preservation, actual-float/nonbinary joins, complete
source maps/contacts and incorrect reciprocal ancestry/context owners. Exact
material witnesses include subnormal and nextafter perturbations, all supports,
removed corner and unused-circle interior. Whole independent expected graph,
source ranges and rational identities come from original source geometry,
without production fillet/trim/material/Map2 helpers. Direct deliberately falsified
records, native metadata/status and output target named faults; no mutation score
is claimed. Late host failure closes all three actual native child handles;
retry on the same borrowed Engine succeeds. Actual ABI5 arrays and typed
mates/neighbors retain the standalone result namespace.

Native truth uses the actual copied binary64 UV, with exact pre-evaluation trim
admission and no snapping. Direct world position and oriented unit-normal
component limits remain2^-36 and2^-30. Rejected/invalid rows retain explicit
status-1 and NaNs. Qualification, cost and reference details follow below.

Across all48 callers the maximum world-position component error is
2.800374311525101e-16 and oriented unit-normal component error is
4.426331796538329e-16, below the unchanged direct limits. Every admitted
native status is successful. Grid17 gives2023 candidates in every recipe;
small/mid/large admit1987/1871/1791 and reject36/152/232. These differences
come from actual original-plane trim domains, not a resized batch.

[Independent construction review](../../papers/2026-10-15-fillet-construction-oracle-code-review.md),
[oracle review](../../papers/2026-10-15-fillet-oracle-semantics-review.md),
[sampler review](../../papers/2026-10-15-fillet-sampler-root-review.md),
[saved actual artifact review](../../papers/2026-10-15-fillet-unit-artifact-semantics-review.md).

## Complete caller costs

Three actual source-edge/radius/Frame recipes timesCPU1/CPU24/P1000/P1001,
one excluded warmup and three measured fresh processes each:48 qualified calls,
12 excluded warmups and36 measured observations in12 cells. Every recipe has
the same graph and seven charts. The labels small/mid/large refer to radius and
placement recipes, not increasing CAD complexity or a larger-model study.

| Recipe | Original edge | Local radius | Frame origin | Scale | Candidates/admitted |
| --- | ---: | ---: | --- | ---: | --- |
| small | 9 | 1/16 | (0,0,0) | 1 | 2023/1987 |
| mid | 11 | 3/8 | (2,-3,1) | 1/2 | 2023/1871 |
| large | 12 | 1/2 | (-2,1,1/2) | 2 | 2023/1791 |

Shared Linux x86_64 fleet, Python3.12.3, two Tesla P100-PCIE16GB,
driver580.178.04, existing CUDA12.8.1/sm60 binaries. Clocks, other fleet load
and caches are uncontrolled. No simultaneous project GPU qualification or tuning
job ran during the campaign. All actual geometry and tests execute on the fleet.

Prepared-source→consumed wall includes CPU feature publication, exact material/
edge/UV/history reads and256 history-map reader rows evaluating available exact
source maps,10 actual contact-map
reads, request preparation, Engine and native child setup, upload/evaluation/
download/provenance/status/frame/normal/scatter consumption, cleanup, topology
packing/library/context/build and actual typed CSR mates/neighbors.
Factory→consumed additionally includes creating the original box from its
already-owned Frame. Independent qualification, artifact/SVG writes, reference
comparisons, imports and process launch remain outside those useful walls.
Full child-process wall is retained separately. The caller binds actual original
source, selected edge and requested radius before measuring its consumers.

| Recipe | Placement | Median source→consumed ms | Range ms | Median factory→consumed ms |
| --- | --- | ---: | ---: | ---: |
| small | CPU1 | 286.904 | 278.940–382.030 | 288.246 |
| small | CPU24 | 289.333 | 279.962–293.499 | 290.008 |
| small | P1000 | 974.977 | 829.047–996.041 | 975.621 |
| small | P1001 | 1442.017 | 1380.764–1448.643 | 1442.736 |
| mid | CPU1 | 427.743 | 316.744–598.645 | 428.721 |
| mid | CPU24 | 299.542 | 298.392–482.458 | 300.191 |
| mid | P1000 | 817.285 | 806.975–1040.178 | 817.940 |
| mid | P1001 | 1531.336 | 1526.229–1595.921 | 1531.998 |
| large | CPU1 | 399.260 | 277.053–457.380 | 400.836 |
| large | CPU24 | 548.120 | 281.962–548.266 | 548.781 |
| large | P1000 | 836.671 | 836.367–885.882 | 837.813 |
| large | P1001 | 1337.354 | 1321.417–1379.311 | 1338.160 |

CPU1 has the best small/large medians; CPU24 has the best mid median.
Every P100 complete-call range is slower than its corresponding best-CPU range.
CPU ranges overlap and vary materially; these three-observation unpinned cells
establish no causal thread/clock/load result. This is CPU construction with native
GPU consumers, and no complete-call GPU win. Separate geometry and topology
context setup, cleanup and exact host request work remain return-pass candidates.
For the displayed mid recipe, diagnostic phase medians are:

| Mid phase ms | CPU1 | CPU24 | P1000 | P1001 |
| --- | ---: | ---: | ---: | ---: |
| cpu_fillet | 93.518 | 67.832 | 64.724 | 62.968 |
| material_boundary_history_consume | 43.148 | 20.886 | 15.160 | 17.484 |
| request_prepare | 179.800 | 168.141 | 154.280 | 247.336 |
| engine_setup | 2.085 | 2.106 | 279.024 | 579.819 |
| sample_and_consume | 41.882 | 40.536 | 39.998 | 40.645 |
| sampling/normal_scatter_ms | 34.729 | 31.973 | 31.816 | 32.562 |
| engine_cleanup | 0.016 | 0.015 | 38.810 | 85.079 |
| adjacency_build | 0.210 | 4.064 | 229.018 | 506.416 |
| independent_qualification | 299.506 | 291.981 | 370.390 | 568.620 |

Diagnostic medians are **not additive**. Construction, sampling and topology
phases nest inside enclosing walls, and medians can select different repetitions.
Native evaluation is synchronous unprofiled wrapper wall, not isolated kernel
time. Topology initializes a separate per-call context instead of borrowing the
sampling Engine. [Raw observations](costs/raw.tsv), [summary](costs/summary.tsv),
[timing scope](costs/scope.tsv), [controller pin](costs/sources.tsv).

## Optional same-world foreign comparison

The already-installed isolated USD BRep1.0.0 receives the exact same world box,
radius and selected original vertical edge. All eight original world vertices,
box dimensions, selected endpoints and radius are verified exactly representable
in binary64 before construction. A deep copy is made; selection uses copied-edge
endpoints, mutation returns that copied body, and borrowed copied child handles
are discarded after mutation. Original volume/manifold/bounds/all12edge endpoint,
range and three-point probes remain exactly unchanged afterward.

Unchanged criteria are volume relative1e-8, bounds1e-8*scale and boundary
returned-distance/closest-point-residual1e-7*scale. Each actual boundary set has
75 exact edge points plus56 spread native face-interior samples:131 witnesses
covering all seven faces. Full outcomes are saved before a nonzero failure exit.
All three final adapters exited0; no accuracy setting or tolerance was changed.

| Recipe | All criteria | Volume absolute error | Volume relative error | Maximum boundary distance/residual | Witnesses |
| --- | --- | ---: | ---: | ---: | ---: |
| small | PASS | 1.64313008e-14 | 7.30552218e-15 | 2.2887834e-16 | 131 |
| mid | PASS | 1.50324198e-12 | 5.41752351e-12 | 2.39149358e-15 | 131 |
| large | PASS | 1.05279696e-09 | 5.99174301e-11 | 2.81086019e-15 | 131 |

All source-preservation/manifold/volume/bounds/boundary criteria pass:
[small outcomes](reference-small/outcomes.tsv), [mid outcomes](reference-mid/outcomes.tsv),
[large outcomes](reference-large/outcomes.tsv). Foreign material/history and full
boundary correspondence are not qualified by finite closest-point witnesses.
The earlier [revolution triangle volume disagreement](https://github.com/antimeme-ai/BRepGPU/issues/43)
remains open with its unchanged failing threshold. These selected foreign passes
neither resolve it nor complete required external community alpha qualification.

## Artifacts and remaining work

Each caller retains original source vertices/Frame, full source/output graph and
carrier tables, forward/reverse ancestry, actual history/contact-map reads,
material/edge/UV reads, native rows, ABI5 inputs/CSR arrays and typed readers.
Thirteen actual owned source pins and used native hashes remain visible;
references separately pin adapter and input artifacts. The graphic is actual
exact line/quarter-arc wireframe with admitted native samples, not a result mesh.

Analytic offset is next as a separate researched and reviewed breadth unit.
Multiple or interacting edges, partial ends, arbitrary supports, variable radii,
rolling-ball corners and result meshing remain later feature families.
[Round-two notes](../../papers/2026-10-15-fillet-return-pass-runtime-notes.md) retain
source-map, native ownership, lifetime/residency and Python-role constraints.
Integration, tuning and matched NCSX rendering timing remain deferred until after
the breadth big rocks. Required external community alpha suites remain incomplete
and non-green; this delivery does not waive their adapters/corpora.
