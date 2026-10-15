# BRepGPU research

## Complete canonical trim-aware meshing

The [design](../docs/MESHING_DESIGN.md) and [plan](../docs/MESHING_PLAN.md)
follow independent [geometry](2026-10-15-meshing-semantics-study.md),
[whole-domain oracle](2026-10-15-meshing-oracles-study.md) and
[runtime/ownership](2026-10-15-meshing-runtime-study.md) studies. Actual OCCT
edge/pcurve/seam source, Triangle/CGAL domain contracts and trim tessellation
literature inform the owned bounded construction. Bibliographies:
[geometry](meshing-semantics.bib), [oracles](2026-10-15-meshing-oracles.bib),
[runtime](meshing-runtime.bib). Research is explicit about read scope and
small-loop simplifications incompatible with retained holes.

[Product review](2026-10-15-meshing-code-semantics-review.md) and
[oracle review](2026-10-15-meshing-oracle-semantics-review.md) retain the corrected
source-slot/type and whole-domain findings. [Caller/artifact review](2026-10-15-meshing-caller-result-review.md) and
[actual results](../results/meshing-2026-10-15/REPORT.md)
qualify both exact/rounded geometry, shared topology and error bounds. External
OCCT replay remains missing alpha debt. The operator's continuing NCSX trophy
subject has a separate [reference brief](../docs/NCSX_TROPHY_BRIEF.md) and
[acquisition inventory](ncsx-material-inventory.tsv).

## Trophy visualization studies

[Two actual-geometry candidates](../docs/TROPHY_GALLERY.md) use the current CUDA
patch evaluator and exact CPU arrangements. The [gallery plan](../docs/TROPHY_GALLERY_PLAN.md),
[surface design review](2026-10-14-trophy-surface-design-review.md),
[surface caller/display review](2026-10-14-trophy-surface-code-review.md) and
[arrangement design](2026-10-14-trophy-arrangement-design.md) retain source,
admission and display semantics. No new kernel or rendering dependency is adopted.

## External oracles and alpha qualification

The operator requires [external community qualification before alpha](../docs/ALPHA_QUALIFICATION.md),
while allowing incomplete/red suites during development. Our current gap is
executable community replay and independent-kernel comparison of actual owned outputs.

- [OCCT suite study](2026-10-14-external-occt-suite-study.md): 21,006 static DRAW case
  registrations, 40 groups; data availability, strong and weak assertions, source
  inventory and [bibliography](external-occt.bib). Counts are inventory, not executed passes.
- [CGAL exact oracle study](2026-10-14-external-cgal-oracle-study.md): pinned circular
  geometry/arrangement tests, semantic comparison design, real registration gaps,
  build costs and [bibliography](external-cgal-oracle.bib). No dependency adopted.
- [Independent oracle audit](2026-10-14-alpha-oracle-audit.md): existing model
  strengths, correlated blind spots, selected-mutant limits and exchange/input
  corpus roles. No universal complete public B-rep answer oracle was established.

## Exact planar arrangements

The [complete mixed-line/circle design](../docs/ARRANGEMENT_DESIGN.md) and
[whole-unit plan](../docs/ARRANGEMENT_PLAN.md) follow independent
[semantics](2026-10-14-arrangement-semantics-study.md),
[arithmetic/ownership](2026-10-14-arrangement-runtime-study.md) and
[complete-region oracle](2026-10-14-arrangement-oracles-study.md) studies.
Primary reading includes the official CGAL arrangement/DCEL/history contracts
and actual pinned OCCT face/wire builder source. Neither is an adopted dependency.
[Bibliography](arrangement-semantics.bib),
[source catalog](arrangement-semantics-sources.tsv) and
[runtime bibliography](arrangement-runtime.bib) retain exact reading scopes;
the unacquired full arrangements book is on [the shopping list](SHOPPING_LIST.md).

Three design reviews and three plan reviews accept a complete bounded family:
axial grid cuts and separated circle disks strictly inside individual cells,
including multiple holes. One physical circle edge retains every original
piece/map/event; ordered cell/disk/exterior regions have opposite uses and exact
source ranges, all-incident lookup, strict witnesses and rational-plus-pi areas.
The actual caller passed separate
[semantics](2026-10-14-arrangement-caller-semantics-review.md) and
[runtime](2026-10-14-arrangement-caller-runtime-review.md) source reviews,
including original boundary-curve consumption driven by emitted parameters.
[Independent oracle/tests review](2026-10-14-arrangement-oracle-runtime-review.md)
and [focused mutation review](2026-10-14-arrangement-mutation-runtime-review.md)
retain whole-domain/loop/source truth and actual assertion-kill requirements.
[Return-pass notes](2026-10-14-arrangement-round2-notes.md) record clipped/tangent
arrangements, cross-face Boolean identity and later accelerator work without
starting integration or the deferred Python architecture investigation.
[Whole code semantics](2026-10-14-arrangement-code-semantics-review.md),
[complete saved artifacts](2026-10-14-arrangement-artifact-review.md),
[actual SVG/result semantics](2026-10-14-arrangement-result-semantics-review.md)
and [performance review](2026-10-14-arrangement-performance-runtime-review.md)
accept the delivered bounded claims and the
[five full caller results](../results/arrangements-2026-10-14/REPORT.md).

## Exact material and finite-face classification

The bounded [CPU design](../docs/CLASSIFICATION_DESIGN.md) and
[whole-unit plan](../docs/CLASSIFICATION_PLAN.md) follow three independent
[semantics](2026-10-14-classification-semantics-study.md),
[arithmetic/runtime](2026-10-14-classification-runtime-study.md) and
[oracle](2026-10-14-classification-oracles-study.md) studies. Primary inputs
include Shewchuk/Yap exact geometric computation, official CGAL closed versus
regularized set semantics, and pinned OCCT classifier source. Reference study
selects no dependency. [Core bibliography](classification.bib),
[runtime bibliography](classification-runtime.bib),
[PDF catalog](classification-runtime-sources.tsv), and
[semantics bibliography](classification-semantics.bib) retain sources/read scope.

Three design and three plan reviews precede the recorded healthy existing body
caller and absent classification-entry red. Reviews retain
[complete geometry/identity/oracle code](2026-10-14-classification-code-semantics-review.md),
[actual caller and costs](2026-10-14-classification-caller-review.md) and
[focused mutants](2026-10-14-classification-mutation-review.md).
[Saved artifact review](2026-10-14-classification-artifact-review.md) checks
36 exact points, eight complete contact loci and actual SVG content.
[Performance review](2026-10-14-classification-performance-review.md) accepts
all twelve cases and explicit outside-wall qualification costs in the
[published results](../results/classification-2026-10-14/REPORT.md).
[Return-pass notes](2026-10-14-classification-round2-notes.md) keep broader
algebraic/curved contact and accelerator placement after the broad pass.
The operator's separate [Python-role investigation](2026-10-14-python-role-return-pass.md)
also waits until the breadth sweep is finished.

## GPU edge-use adjacency beachhead

The [owned CSR/reader API](../docs/ADJACENCY.md) follows independent
[topology](2026-10-14-adjacency-topology-study.md),
[runtime/scan](2026-10-14-adjacency-runtime-study.md), and
[oracle](2026-10-14-adjacency-oracle-study.md) studies. Primary inputs include
OCCT occurrence/seam semantics, Truck compression and Blelloch/Harris/Merrill
parallel scan and graph traversal literature. [BibTeX](adjacency-runtime.bib)
and [acquired-paper catalog](adjacency-runtime-sources.tsv) retain reading scope.

The [design](../docs/ADJACENCY_DESIGN.md) and [plan](../docs/ADJACENCY_PLAN.md)
passed separate topology/runtime/oracle reviews before the direct cylinder red.
Code reviews examine [native ownership](2026-10-14-adjacency-native-runtime-code-review.md),
[scan/topology](2026-10-14-adjacency-native-topology-code-review.md),
[typed interface](2026-10-14-adjacency-interface-code-review.md),
[independent truth](2026-10-14-adjacency-oracle-code-review.md),
[actual consumer/costs](2026-10-14-adjacency-caller-code-review.md), and
[qualification/mutants](2026-10-14-adjacency-qualification-code-review.md).
[Saved caller review](2026-10-14-adjacency-caller-artifact-review.md) independently
checks 120 exported occurrences and diagram labels. The
[measured results](../results/adjacency-2026-10-14/REPORT.md) retain all24 matched
CPU/P100 cases and separate cold/native/typed-consumer costs.
[Runtime](2026-10-14-adjacency-performance-runtime-review.md) and
[topology](2026-10-14-adjacency-performance-topology-review.md) reviews accept the
bounded claims. The [return-pass notes](2026-10-14-adjacency-round2-notes.md) retain later native
consumption, residency, scan and broader topology ideas without starting tuning.

## Analytic intersection beachhead

The [finite-face design](../docs/INTERSECTION_DESIGN.md) joins independent
[geometry/topology](2026-10-14-intersection-topology-study.md),
[oracle](2026-10-14-intersection-oracle-study.md), and
[runtime](2026-10-14-intersection-runtime-study.md) studies. Primary references
include pinned OCCT/SISL sources, Dokken's intersection thesis and algebraic
curve topology/lifting work; [source catalog](intersection-runtime-sources.tsv),
[BibTeX](intersection-runtime.bib), and
[oracle sources](intersection-oracle-sources.tsv) retain exact read scopes.
Research selects complete closed plane/wall intersections and coupled spatial/
two-UV representations; GPU curve sampling is a separate placement claim.
[Return-pass notes](2026-10-14-intersection-round2-notes.md) retain broader
geometry and integration ideas without starting the deferred optimization lane.
The [delivered API](../docs/INTERSECTIONS.md) and
[actual qualification/costs](../results/intersections-2026-10-14/REPORT.md)
include seventeen direct tests on CPU/bothP100s, five production mutants,
four exported callers and sixteen repeated comparison cases. Reviews retain
[independent core truth](2026-10-14-intersection-code-oracle-review.md),
[runtime](2026-10-14-intersection-core-runtime-review.md),
[consumer](2026-10-14-intersection-sampler-topology-review.md),
[oracle topology](2026-10-14-intersection-oracle-topology-review.md),
[oracle runtime](2026-10-14-intersection-oracle-runtime-review.md),
[serialized artifacts](2026-10-14-intersection-artifact-topology-review.md) and
[performance](2026-10-14-intersection-performance-runtime-review.md).

## Deferred integration return pass

At the operator's direction, [GPU setup, residency and integration polish](../docs/RETURN_PASS.md)
wait until AFTER the broad-pass big rocks. Source-grounded notes cover
[GPU/stream/plugin lifetime](2026-10-14-gpu-lifetime-reuse-study.md) and
[scene/template/index reuse](2026-10-14-scene-residency-reuse-study.md).
No optimization implementation or new profiling campaign is active.

## Conservative surface-pair beachhead

The [pair design](../docs/PAIR_DISCOVERY_DESIGN.md) and
[implementation plan](../docs/PAIR_DISCOVERY_PLAN.md) apply
[broad-phase literature](2026-10-14-pair-literature.md),
[bibliography](pair-discovery.bib) and independent
[runtime](2026-10-14-pair-runtime-study.md),
[topology](2026-10-14-pair-topology-study.md) and
[geometry oracle](2026-10-14-pair-oracle-study.md) source studies.
Carrier-box survivors preserve source identity and explicit page coverage;
they do not establish intersections.
The [delivered API](../docs/PAIR_DISCOVERY.md) and
[qualified results](../results/pairs-2026-10-14/REPORT.md) retain actual source/
mask exports and eight complete-caller cost cases. Reviews cover
[runtime code](2026-10-14-pair-code-runtime-review.md),
[scene consumer](2026-10-14-pair-code-scene-oracle-review.md),
[independent truth](2026-10-14-pair-oracle-code-review.md),
[serialized artifacts](2026-10-14-pair-artifact-oracle-review.md) and
[performance](2026-10-14-pair-performance-review.md).

## First body-level beachhead

The [owned body consumer](../docs/BODY_SAMPLING.md) now connects numerical jets
to canonical box/cylinder/one-hole-plate snapshots, trims, shared boundaries and
oriented normals. [Results](../results/body-2026-10-14/REPORT.md).

* [Topology source study](2026-10-14-body-topology-study.md),
  [oracle study](2026-10-14-body-oracle-study.md) and
  [runtime study](2026-10-14-body-runtime-study.md).
* [Design](../docs/BODY_SAMPLING_DESIGN.md),
  [plan](../docs/BODY_SAMPLING_PLAN.md),
  [body code review](2026-10-14-body-code-topology-review.md),
  [oracle code review](2026-10-14-body-code-oracle-review.md) and
  [sampler code review](2026-10-14-body-sampler-code-review.md).
* [Actual caller artifact review](2026-10-14-body-caller-artifact-review.md) and
  [complete-caller performance review](2026-10-14-body-performance-review.md).

CPU normal/row work dominates the measured caller; no end-to-end gain against
CPU24 is established. This is a completed narrow beachhead, with broader
geometry and optimization reserved for the return pass.

The first frumentarii dispatch covers three connected questions:

| Lane | Question | Report |
| --- | --- | --- |
| GPU numerics | How should batched FP64 patch evaluation, derivatives, subdivision and inverse evaluation work on P100? | [Report](2026-10-13-gpu-numerics.md) · [BibTeX](gpu-numerics.bib) |
| GPU topology | How do complete intersections, trims, face classification and shared topology migrate to device execution? | [Report](2026-10-13-gpu-topology.md) · [BibTeX](gpu-topology.bib) |
| Geometry oracles | Which direct checks and corpora establish useful geometric correctness and fair performance? | [Report](2026-10-13-geometry-oracles.md) · [BibTeX](geometry-oracles.bib) |

Research reports and annotated bibliographies are committed here. Acquired
PDFs remain outside Git. Reference source is study material in ignored quarantine;
no reference instructions or library dependencies are adopted by collection.

[Reference implementation map](REFERENCE_IMPLEMENTATIONS.md) ·
[Projects to inspect](PROJECTS.md) · [Source snapshot catalog](reference-catalog.tsv) ·
[Source bibliography](reference-snapshots.bib) · [Restoration manifest](../QUARANTINE.md) ·
[Literature still to acquire](SHOPPING_LIST.md)

The reports converge on a narrow first unit: positive-weight rational Bézier
patches through bidegree (3,3), returning position and first/second derivatives.
Analytic polynomial and exact rational cylinder fixtures qualify the output.
The later NURBS front end needs knot-span extraction and explicit one-sided
derivative semantics. The first unit has passed design, plan and implementation review and has
[qualified product measurements](../results/iteration1-2026-10-13/REPORT.md).

Starting literature:

- [NVIDIA USD BRep](https://github.com/NVIDIA-Omniverse/usd-brep) and its [known issues](https://github.com/NVIDIA-Omniverse/usd-brep/blob/main/KNOWN_ISSUES.md).
- [Krishnamurthy et al. 2008 GPU NURBS operations](https://mcmains.me.berkeley.edu/pubs/SPM08gpuSSI.pdf): evaluator, bounds, inverse evaluation and intersection discovery.
- [Lévy 2025 exact mesh CSG](https://arxiv.org/abs/2405.12949): exact predicates, constructions and arrangement combinatorics.
- [Valque and Lazard 2025 output rounding](https://doi.org/10.1111/cgf.70197): converting exact intersections to floating-point coordinates can introduce new intersections.
- [Yang's recent intersection work](https://yangjieyin.github.io/publications/): topology guarantees, overlap regions and small/tangent branches; algorithm and availability details remain research questions.

The existing CPU benchmark is a starting measurement and fixture collection,
not a substitute for numerical and topological contracts in the owned kernel.

## First-iteration source study and design review

| Unit | Source study | Adversarial review |
| --- | --- | --- |
| Numerical evaluator and direct fixtures | [Pinned source study](2026-10-13-first-iteration-numerical-study.md) · [Sources](first-iteration-numerical-sources.tsv) · [Restoration](first-iteration-numerical-restoration.md) · [BibTeX](first-iteration-numerical.bib) | [Numerical review](2026-10-13-iteration-1-numerical-review.md) |
| Runtime, ownership and hardware | [Runtime study](2026-10-13-first-iteration-runtime-study.md) · [Actual CUDA setup](../docs/CUDA_TOOLCHAIN.md) | [Runtime review](2026-10-13-iteration-1-runtime-review.md) |
| Task value, reference integration and topology | [Roadmap source study](2026-10-13-acceleration-roadmap-study.md) · [Sources](acceleration-roadmap-sources.tsv) · [Restoration](acceleration-roadmap-reference-restoration.md) | [Topology/roadmap review](2026-10-13-iteration-1-topology-roadmap-review.md) |

The integrated [roadmap](../docs/ACCELERATION_ROADMAP.md),
[first design](../docs/ITERATION_1_DESIGN.md) and
[topology architecture](../docs/TOPOLOGY.md) resolve the source findings and
reviewed contracts. The reviews retain their initial findings and subsequent
dispositions. The first numerical unit is implemented; the broader kernel roadmap remains
research and design.

## Implementation review

[Runtime and Python code review](2026-10-13-iteration-1-runtime-code-review.md) ·
[Independent caller and numerical code review](2026-10-13-iteration-1-caller-code-review.md) ·
[Runtime plan review](2026-10-13-iteration-1-plan-runtime-review.md) ·
[Numerical plan review](2026-10-13-iteration-1-plan-numerical-review.md) ·
[Caller plan review](2026-10-13-iteration-1-plan-caller-review.md)

The code reviews retain findings and corrected dispositions. Exact arithmetic
qualification, actual device memory checks and modeled native failure checks
provide distinct evidence; successful CPU/GPU parity alone is insufficient.

## Resident projection study

The next operation retains iterations and patch jets on device, returning a
seeded first-order candidate with separate rank and curvature diagnostics.

| Unit | Study | Design review |
| --- | --- | --- |
| Numerical algorithm | [Pinned kernels and optimization literature](2026-10-13-projection-numerical-study.md) · [BibTeX](projection-numerical.bib) | [Numerical review](2026-10-13-projection-design-numerical-review.md) |
| Independent geometry | [Exact projection and boundary oracles](2026-10-13-projection-oracle-study.md) · [BibTeX](projection-oracles.bib) | [Oracle review](2026-10-13-projection-design-oracle-review.md) |
| Runtime and ABI | [Lifetime and compatibility study](2026-10-13-projection-runtime-study.md) | [Runtime review](2026-10-13-projection-design-runtime-review.md) |
| P100 execution | [Resident consumer and bounded thread loop](2026-10-13-projection-gpu-study.md) | Integrated in the numerical review |

The [reviewed design](../docs/ITERATION_2_DESIGN.md) distinguishes stationarity
from a nearest-point guarantee. Saddles, unequal boundary candidates and weak
critical directions are deliberate parts of the qualification corpus.

## Conservative discovery and Wafer / GPU MODE

The next dispatch studied automatic candidate generation, exact-input enclosure
and bounded resident execution. The requested Wafer index and linked GPU MODE
lectures were acquired at pinned revisions; their actual kernels were inspected.

| Unit | Study and bibliography | Acquisition / source map |
| --- | --- | --- |
| Wafer and GPU MODE | [Selected-code study](2026-10-14-wafer-gpu-mode-study.md) · [BibTeX](wafer-gpu-mode.bib) | [Archives](wafer-gpu-mode-sources.tsv) · [Read files](wafer-gpu-mode-entrypoints.tsv) · [Restore](wafer-gpu-mode-reference-restoration.md) |
| Numerical conservatism | [Homogeneous intervals and distance bounds](2026-10-14-bounds-numerical-study.md) · [BibTeX](bounds-numerical.bib) | [Sources](bounds-numerical-sources.tsv) · [PDFs](bounds-numerical-pdfs.tsv) |
| Independent geometry | [Exact enclosure, exclusion and coverage oracles](2026-10-14-bounds-oracle-study.md) · [BibTeX](bounds-oracles.bib) | [Sources](bounds-oracle-sources.tsv) · [PDFs](bounds-oracle-pdfs.tsv) |
| Resident execution | [Packed hierarchy, queues and lifetime](2026-10-14-bounds-runtime-study.md) · [BibTeX](bounds-runtime.bib) | [Read files](bounds-runtime-entrypoints.tsv) · [PDFs](bounds-runtime-pdfs.tsv) |
| GPU traversal | [Architecture-dependent scheduling lessons](2026-10-14-bounds-gpu-study.md) · [BibTeX](bounds-gpu.bib) | [PDFs](bounds-gpu-pdfs.tsv) |

The integrated [next-operation recommendation](../docs/ITERATION_3_STUDY.md)
keeps conservative arithmetic, complete domain accounting and candidate quality
distinct. The operation is now [implemented and qualified](../docs/ITERATION_3.md),
with [retained native cost and attained precision](../results/discovery-2026-10-14/REPORT.md).

The next delivery now has a [reviewed design](../docs/ITERATION_3_DESIGN.md),
including a concrete three-mode comparison informed by the
[strong-bound numerical input](2026-10-14-discovery-strong-bounds-design.md) and
[independent strong-bound study](2026-10-14-strong-bounds-oracle-study.md).
[Numerical](2026-10-14-discovery-design-numerical-review.md),
[runtime](2026-10-14-discovery-design-runtime-review.md) and
[oracle](2026-10-14-discovery-design-oracle-review.md) reviews retain findings
and dispositions. The [implementation plan](../docs/ITERATION_3_PLAN.md) and
[qualification](../results/discovery-2026-10-14/QUALIFICATION.md) retain the
delivered scope and evidence.

The [off-grid separated-bound study](2026-10-14-offgrid-separated-bounds-study.md)
explains the measured axis relaxation and proposes exact extrusion recognition
with a separated height bound. [Bibliography](offgrid-bounds.bib),
[primary reading](offgrid-bounds-pdfs.tsv) and
[exact algebra](offgrid-bounds-exact-algebra.tsv) accompany it. This is a proposed
numerical strengthening; its floating-point implementation and hardware value
remain unmeasured.

## CUDA mode specialization experiment

The [design](2026-10-14-discovery-cuda-mode-specialization-design.md) and
[reviewed implementation plan](../docs/CUDA_DISCOVERY_MODE_PLAN.md) led to three
mode-specific kernels using unchanged shared arithmetic. The
[completed experiment](../results/discovery-modes-2026-10-14/REPORT.md) retains
exact/raw-output qualification, compiled resource observations and complete
matched/reversed measurements. It did not meet the performance criterion;
the original dynamic kernel remains the product. Candidate source is preserved
at its recorded commit rather than selected from register counts alone.
