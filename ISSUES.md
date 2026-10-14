# Open work

Track implementation and research in this repository's
[GitHub issues](https://github.com/antimeme-ai/BRepGPU/issues).

| Work | State | Deliverable |
| --- | --- | --- |
| [#1: First P100 numerical geometry unit](https://github.com/antimeme-ai/BRepGPU/issues/1) | Implemented and qualified | [Owned CPU/CUDA six-jet API](docs/ITERATION_1.md); [exact oracles and both-P100 results](results/iteration1-2026-10-13/REPORT.md) |
| [#2: Intersections and topology architecture](https://github.com/antimeme-ai/BRepGPU/issues/2) | Architecture reviewed; breadth-first theme beachheads selected, operation designs remain | [Packed topology and identity](docs/TOPOLOGY.md); [theme map and first-pass sequence](docs/ACCELERATION_ROADMAP.md#breadth-first-delivery-policy) |
| [#3: CPU phase profiles and fair comparisons](https://github.com/antimeme-ai/BRepGPU/issues/3) | Native CPU/P100 comparisons published; modeling profiles remain | [Resident/download/host timings](results/iteration1-2026-10-13/REPORT.md); broader CAD phase profiles remain |
| [#6: Resident seeded projection](https://github.com/antimeme-ai/BRepGPU/issues/6) | Implemented and qualified; PR #7 | [Resident solver/API](docs/ITERATION_2.md), [exact local-candidate oracles and all 144 native CPU/P100 cases](results/projection-2026-10-14/REPORT.md) |
| [#8: Conservative bounds and automatic candidates](https://github.com/antimeme-ai/BRepGPU/issues/8) | Implemented, qualified and measured; PRs #10/#11 | [Three-mode operation/API](docs/ITERATION_3.md), [reviewed design](docs/ITERATION_3_DESIGN.md) and [plan](docs/ITERATION_3_PLAN.md); [qualification](results/discovery-2026-10-14/QUALIFICATION.md) and [444 native comparisons](results/discovery-2026-10-14/REPORT.md) |
| [#15: First body-level consumer](https://github.com/antimeme-ai/BRepGPU/issues/15) | Implemented, independently qualified and measured | [Immutable bodies and sampling/normal API](docs/BODY_SAMPLING.md); [three actual callers, qualification and 24-case costs](results/body-2026-10-14/REPORT.md) |
| [#17: Conservative surface-pair discovery](https://github.com/antimeme-ai/BRepGPU/issues/17) | Implemented, independently qualified and measured | [Resident common-world chart pair API](docs/PAIR_DISCOVERY.md); [CPU/P100 qualification, actual callers and eight matched cost cases](results/pairs-2026-10-14/REPORT.md) |
| [#20: Complete analytic face intersections](https://github.com/antimeme-ai/BRepGPU/issues/20) | Implemented, independently qualified and measured; PR21 | [Exact components and coupled source maps](docs/INTERSECTIONS.md); [17 direct CPU/P100 tests, five mutants and sixteen matched consumer cases](results/intersections-2026-10-14/REPORT.md) |
| [#19: GPU lifetime/residency and integration polish](https://github.com/antimeme-ai/BRepGPU/issues/19) | Explicitly deferred until AFTER the broad-pass big rocks | [Measured opportunities, source notes and ownership constraints](docs/RETURN_PASS.md); no active optimization lane |
| [#4: Sphere chord-tolerance discrepancy](https://github.com/antimeme-ai/BRepGPU/issues/4) | Reproduced in reference runtime | Public triangle witnesses; independent confirmation and upstream-ready report |

The first three frumentarii reports are published, including independent geometry
oracles. The first owned numerical unit passed design, plan and code review, exact
qualification on CPU/both P100s, and native/device memory checks.
See the [research index](papers/README.md).

CUDA 12.8.1 is installed and qualified on both initial P100 GPUs.
[Recreate the isolated toolchain](docs/CUDA_TOOLCHAIN.md).
