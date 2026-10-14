# BRepGPU research

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
