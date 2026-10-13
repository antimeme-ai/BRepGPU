# Open work

Track implementation and research in this repository's
[GitHub issues](https://github.com/antimeme-ai/BRepGPU/issues).

| Work | State | Deliverable |
| --- | --- | --- |
| [#1: First P100 numerical geometry unit](https://github.com/antimeme-ai/BRepGPU/issues/1) | Source study and design reviewed; plan next | [First design](docs/ITERATION_1_DESIGN.md); CPU/CUDA six-jet batch API and direct geometric oracles |
| [#2: Intersections and topology architecture](https://github.com/antimeme-ai/BRepGPU/issues/2) | Architecture reviewed; later operation designs remain | [Packed topology and identity](docs/TOPOLOGY.md); [iterative acceleration roadmap](docs/ACCELERATION_ROADMAP.md) |
| [#3: CPU phase profiles and fair comparisons](https://github.com/antimeme-ai/BRepGPU/issues/3) | Exploratory baseline published | Native batched CPU comparison, meaningful phase profiles, transfer/fallback accounting |
| [#4: Sphere chord-tolerance discrepancy](https://github.com/antimeme-ai/BRepGPU/issues/4) | Reproduced in reference runtime | Public triangle witnesses; independent confirmation and upstream-ready report |

The first three frumentarii reports are published, including independent geometry
oracles. Product code follows research, design review and plan review.
See the [research index](papers/README.md).

CUDA 12.8.1 is installed and qualified on both initial P100 GPUs.
[Recreate the isolated toolchain](docs/CUDA_TOOLCHAIN.md).
