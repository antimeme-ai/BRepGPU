# Complete signed sharp-cylinder offset

2026-10-15. An actual owned cylinder now offsets inward/outward into an immutable
closed solid on CPU, with exact changed cap/wall carriers, original-carrier normal
feet and complete source history. Existing CPU/both-P100 native first jets and
ABI5 adjacency consumers read its actual geometry. [API/support](../../docs/OFFSET.md),
[design](../../docs/OFFSET_DESIGN.md), [reviewed plan](../../docs/OFFSET_PLAN.md).
The optional foreign comparisons remain non-green, with full outcomes below.

![Actual source overlay and sharp offset with P100 samples](costs/outset-cuda-device0-threads1-r1/offset.svg)

## Complete geometry, joins and original source correspondence

Actual canonical source radius r=1/4, half-height h=1/2 and original bounded Frame
remain explicit. Signed distance d is nonzero on the1/256 grid in[-1/8,+1/8].
The intersection join gives R=r+d,H=h+d and exact material x²+y²<=R²,|z|<=H.
Positive R/H exclude collapse/inversion for this family. Negative d equals
Euclidean erosion here. Positive d has sharp rims, not rounded Euclidean dilation:
a new rim point moves radially and axially by d, distance sqrt(2)*d to the source.
General surface offsets, singularity repair and rounded joins are not established.

Two disk-cap control nets span[-R,R]² at±H; four rational wall quarters have
exact degree(2,1) dyadic controls and weights1,1,2. Controls represent the actual
new dimensions, with original rawUV/Frame bits and fresh complete Lineage. New
vertex/Curve/UVMap geometry uses Fractions. Original source admission preserves
its actual signed-zero bits; original float rotations differ from new exact
Fraction construction at some zero controls. No old source or canonical guard
is replaced/widened, no fitting or native build is introduced.

The complete graph has2vertices/3edges/6coedges/3loops/3faces/6charts=23entities.
Two closed rim circles each retain distinct cap/wall uses. The seam retains two
opposed same-face occurrences at rawtau0/4. A closed ring's start/end ports stay
distinct at its one anchor; each anchor has one three-port cycle. Full words,
carrier coverage and both UV maps close a connected genus-zero shell. Exact
world volume is pi*2*H*R²*scale³; tight bounds are origin±scale*(R,R,H).

Every original entity has exactly one modified same-kind image:23input records,
23output origins and no retained/deleted/generated roles. E/CO source parameters
mean boundary_parameter correspondence, not coincidence or unique face-normal
movement at a sharp rim. Source UV normal_foot descriptors name actual original
F/CH owners and explicit target domains. Wall F/L arguments use rawtau[0,4];
wall charts and coedges use normalized/intrinsic coordinates. Contextual owners
are not additional causal image parents; absolute traversal/material senses are
not applied twice. Whole reciprocal history and actual map readers are retained.

source_foot reads exact original carrier extensions and returns exact world
source/target points, outward unit normal and signed world d*scale, with
point=source_point+distance*normal. It separately reports original chart-square
membership and effective original trim. Outward cap feet may lie inside the
old square yet outside its disk; outward wall feet can exceed original V[0,1].
At a sharp rim, its two incident carriers have different source feet. No extended
source query is passed to old native code or float-coerced source.classify.
Exact output material returns every bounded supporting face and no extra seam
material face. Query rounding and native admission remain separate.

## Qualification

All three independent design reviews and all three plan reviews accepted before
implementation. Actual healthy native cylinder point/normal passed; then28new
methods and the actual caller failed on missing modules. Construction's first
run exposed original signed-zero admission; literal original float rotations
fixed that check while new exact dyadic nets remained unchanged.

| Unit | Actual fleet outcome |
| --- | --- |
| Complete CPU construction/history/feet | 8 methods passed,0.257s after original signed-zero admission correction |
| Native sampler/lifetime | 8 each CPU1/CPU24/P1000/P1001,0.169/0.190/2.335/4.395s |
| Independent whole geometry/maps/native/topology | 12 each CPU1/CPU24/P1000/P1001,2.858/2.923/3.879/3.684s; zero skips |
| Existing affected contracts | 144 methods,143passed/one expected CUDA-only skip,42.860s; skipped device-restoration method passed separately on CUDA,0.689s |

[Placement records](qualification.tsv). These28new methods cover signed extrema,
odd/minimum grid distances, all Frame scales, complete material/support strata,
exact/non-axis/subnormal/nextafter cases, full circle/seam parameterization,
original payloads, precise all-kind ancestry and every source-foot domain case.
Expected geometry/graph/descriptors/normal feet derive independently from actual
original source/d/Frame, without production offset/trim/map/validator helpers.
Whole quarterwise rational boundary identities and coefficient/domain comparisons
supplement direct public readers. Counts, parity and attractive graphics are not
the geometry oracle. Deliberate false ancestry, owner/extension flags, controls,
reader outputs and native rows target named faults; no mutation score is claimed.

Actual ABI5 arrays and typed readers qualify two ordinary pairs and one same-face
pair under full source-slot permutations. Native descriptor/status rejection and
late host failure close all three actual children; healthy retry on the same
borrowed Engine succeeds. [Five protected native hashes](native-preservation.tsv)
match before/after all callers/references. No dependency or native build selected.

Native truth uses actual copied binary64 UV before exact admission, with no
snapping. Across48 callers, maximum world-position component error is
2.714809509272081e-16 and oriented unit-normal component error1.288016747429897e-16,
below unchanged direct limits2^-36/2^-30. Every recipe has1734candidates,
1550admitted,184rejected,0invalid and174boundary rows. All admitted native statuses
succeed; rejected/invalid direct cases retain status-1/NaNs. [Construction review](../../papers/2026-10-15-offset-construction-root-review.md),
[independent construction review](../../papers/2026-10-15-offset-construction-oracle-code-review.md),
[oracle review](../../papers/2026-10-15-offset-oracle-semantics-review.md),
[sampler review](../../papers/2026-10-15-offset-sampler-root-review.md),
[saved actual geometry review](../../papers/2026-10-15-offset-unit-artifact-semantics-review.md).

## Complete caller costs

Three signed distance/Frame recipes timesCPU1/CPU24/P1000/P1001, one excluded
warmup and three measured fresh processes each:48qualified calls,12excluded
warmups and36measured observations in12cells. This is the same small23-entity/
six-chart graph in every recipe. Distance/scale changes are not larger CAD models.

| Recipe | Local d | Frame origin | Scale | Candidates/admitted |
| --- | ---: | --- | ---: | ---: |
| inset | -1/8 | (0,0,0) | 1 | 1734/1550 |
| outset | +1/8 | (2,-3,1) | 1/2 | 1734/1550 |
| odd | +17/256 | (-2,1,1/2) | 2 | 1734/1550 |

Shared Linux x86_64 fleet,Python3.12.3,two Tesla P100-PCIE16GB,
driver580.178.04,existing CUDA12.8.1/sm60 binaries. Clocks,other fleet load and
caches are uncontrolled. No concurrent project GPU qualification/tuning job ran.
All product geometry/tests/reference/benchmarks execute on the fleet.

Prepared-source→consumed includes actual CPU offset publication, exact material/
edge/UV/forward-reverse-history reads,59history-map reader rows evaluating available
maps,18actual source-foot reads and their owner/point/normal/distance checks,
request admission, Engine/children setup, upload/evaluation/download/provenance/
status/Frame/normal/scatter consumption, cleanup, topology packing/library/context/
build and actual paired/seam CSR mate/neighbor readers. Factory→consumed also
includes original source creation from its already-owned Frame. Independent
qualification, artifact/SVG/reference/import/process launch remain outside those
walls; full child-process wall is saved separately. Loaded library,requested
source/distance/join/request and observed placement are bound inside the caller.

| Recipe | Placement | Median source→consumed ms | Range ms | Median factory→consumed ms |
| --- | --- | ---: | ---: | ---: |
| inset | CPU1 | 90.472 | 85.670–130.930 | 90.888 |
| inset | CPU24 | 95.072 | 87.753–104.049 | 95.436 |
| inset | P1000 | 784.377 | 576.933–787.804 | 784.825 |
| inset | P1001 | 1149.549 | 1131.337–1185.433 | 1149.972 |
| outset | CPU1 | 86.931 | 86.861–89.875 | 87.354 |
| outset | CPU24 | 88.627 | 87.592–92.008 | 89.004 |
| outset | P1000 | 586.370 | 556.081–666.535 | 586.769 |
| outset | P1001 | 1359.455 | 1116.802–1398.714 | 1359.849 |
| odd | CPU1 | 84.916 | 84.253–88.645 | 85.306 |
| odd | CPU24 | 93.716 | 92.661–126.087 | 94.204 |
| odd | P1000 | 638.930 | 594.870–730.727 | 639.323 |
| odd | P1001 | 1180.092 | 1096.438–1247.596 | 1180.603 |

CPU1 has the best median in all three recipes. Every P100 complete-call range is
slower than its corresponding CPU1 range. CPU1/24 ranges overlap for inset/outset;
CPU1's odd range is below CPU24's. These unpinned three-observation cells do not
establish causal thread/clock/load behavior. No complete-call GPU win or GPU
feature construction is established. Outset diagnostic phase medians are:

| Outset phase ms | CPU1 | CPU24 | P1000 | P1001 |
| --- | ---: | ---: | ---: | ---: |
| cpu_offset | 5.921 | 5.075 | 5.403 | 5.955 |
| material_boundary_history_consume | 13.312 | 12.836 | 13.089 | 14.614 |
| request_prepare | 31.679 | 30.674 | 28.717 | 31.258 |
| engine_setup | 2.357 | 2.087 | 258.204 | 637.740 |
| sample_and_consume | 32.448 | 35.113 | 39.872 | 34.938 |
| sampling/normal_scatter_ms | 26.973 | 28.066 | 32.064 | 27.817 |
| engine_cleanup | 0.015 | 0.015 | 43.278 | 85.475 |
| adjacency_build | 0.161 | 1.205 | 210.926 | 521.023 |
| independent_qualification | 296.718 | 496.691 | 333.973 | 264.302 |

Phase medians are **not additive**: construction/sampling/topology rows nest
inside enclosing walls and medians may select different repetitions. Native
evaluate_ms is unprofiled synchronous wrapper wall, not isolated kernel time.
Topology has a separate per-call context instead of borrowing sampling's Engine.
Source-factory and prepared-source walls end at one shared timestamp. Setup,
cleanup, exact host request and row work remain recorded return-pass opportunities,
without changing this branch into a tuning lane. [Raw observations](costs/raw.tsv),
[summary](costs/summary.tsv), [scope](costs/scope.tsv), [controller pin](costs/sources.tsv),
[independent final performance review](../../papers/2026-10-15-offset-performance-runtime-review.md).

## Optional same-world reference remains non-green

Existing isolated USD BRep1.0.0 receives exact same-world cylinder base/radius/
height and signed distance, verified representable in binary64 together with
original anchors/bounds. offset_brep uses explicit extended_offset=True and
self_intersection=True, a pure new-result sharp operation. Original volume/
manifold/bounds and all reference-edge endpoint ports/ranges/three-point probes
are retained before/after when offset runs. A matching original factory is
checked first at the already-declared criteria; its failure blocks offset.

Unchanged criteria: requested volume accuracy1e-8, volume relative1e-8,
bounds1e-8*scale and both closest-point returned-distance/recomputed-residual
1e-7*scale. Each attempted offset gets63witnesses:15exact edge points and48
spread native interior rows,eight per actual chart covering all six charts.
[Independent reference artifact review](../../papers/2026-10-15-offset-reference-artifact-semantics-review.md).
All final adapters exit nonzero with complete measured outcomes. No flag,input,
accuracy request or tolerance was changed to chase a pass.

| Recipe | Source preflight | Offset result | Volume absolute/relative error | Boundary maximum | Witnesses |
| --- | --- | --- | --- | ---: | ---: |
| inset | PASS | FAIL: volume,boundary | 2.58668528e-07 / 7.02606932e-06 | 7.23172228e-07 | 63 |
| outset | PASS | FAIL: volume,boundary | 5.03320981e-07 / 7.29142815e-06 | 1.08475834e-06 | 63 |
| odd | FAIL: original source volume | NOT RUN | output not measured | not measured | 0 |

For inset/outset, original preservation, result manifold and bounds pass;
volume and boundary fail. Owned exact world pi coefficients are3/256 and45/2048.
The volume absolute errors2.586685282912615e-7 and5.033209811849959e-7 exceed
limits3.681553890925539e-10 and6.902913545485385e-10. Boundary maxima
7.231722276216292e-7 and1.084758341894971e-6 exceed1e-7 and5e-8.
[Inset outcomes](reference-inset/outcomes.tsv), [outset outcomes](reference-outset/outcomes.tsv).

Odd stops before any offset: original source manifold/bounds pass, but reference
source volume1.5707963471414912 differs from analytic pi/2 floating value
1.5707963267948966 by2.034659463134858e-8 (relative1.2953044442664586e-8),
above limit1.5707963267948965e-8. This is a failed source-volume measurement
criterion, not a measured offset geometry error. offset_attempted=False,
comparison_stage=source_factory and all five output criteria are NOT_RUN.
[Source preflight](reference-odd/source-preflight.tsv), [complete outcomes](reference-odd/outcomes.tsv).

The first adapter stopped at that finite original-source failure without retaining
its values. A reviewed reporting correction now exports original world input,
body/edge probes,pins and three measured source-preflight outcomes before the
gate; it then marks blocked outputs NOT_RUN. Source and output thresholds/API/
flags remain unchanged. Final three comparisons use the corrected pinned adapter;
initial observations are retained privately. This reference-only correction did
not change or rerun the owned48-call campaign.

Pinned-source investigation finds public closest-point queries use
SM_ZONE_TOL_3D=1e-5, coarser than the declared boundary comparison limits.
Offset/intersection/lift/stitch paths also have Brep-derived tolerances, and
source construction has conditional analytic versus rotational-sweep branches.
Installed compile flags/runtime carrier selection were not traced. Thus a failed
closest-point observation cannot isolate actual offset carrier error from query/
trim/assembly error. Volume requests also do not provide a certified global error
enclosure. These limits explain why attribution remains open; they do not turn
any failed or unrun criterion green or establish an upstream defect.
[Source investigation](../../papers/2026-10-15-offset-reference-tolerance-investigation.md),
[open discrepancy48](https://github.com/antimeme-ai/BRepGPU/issues/48).
Earlier [revolution volume issue43](https://github.com/antimeme-ai/BRepGPU/issues/43)
remains separate and open. Foreign material/history/full coverage are not qualified.

## Artifacts and return pass

Each owned caller retains full source/result graph/carriers, precise histories,
59map-reader rows,18normal feet,10material probes,exact edge/UV reads,actualnative
rows,ABI5 inputs/CSR arrays/typed readers,13ownedsource pins and used native hashes.
The controller has its own pin; references separately pin adapter and actual
owned input artifacts. Source-overlay SVG uses actual original body-reader curves,
exact output rings/seam and admitted native points; no result mesh is implied.

The planned extrusion/revolution/fillet/offset breadth sequence is complete.
Broader source families/aspect ratios, rounded/variable/partial offsets,
singularity/self-intersection handling, history composition and result meshing
remain. [Round-two notes](../../papers/2026-10-15-offset-return-pass-runtime-notes.md)
retain normal-foot domains, revisions/ownership and later residency/native/Python
questions. This unit starts no integration/tuning/rewrite or matched NCSX timing.

Original OCCT offset cases and their source parameters have been studied/pinned;
canonical rescaling does not replay their different aspect ratios or open-shell
operations. Required community alpha adapters/corpora remain incomplete/non-green.
No alpha release or waiver follows from local greens,finite reference witnesses,
source studies or the completion of narrow theme beachheads.
