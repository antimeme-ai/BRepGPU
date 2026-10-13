# BRepGPU research

The first frumentarii dispatch covers three connected questions:

| Lane | Question | Report |
| --- | --- | --- |
| GPU numerics | How should batched FP64 patch evaluation, derivatives, subdivision and inverse evaluation work on P100? | In progress |
| GPU topology | How do complete intersections, trims, face classification and shared topology migrate to device execution? | [Report](2026-10-13-gpu-topology.md) · [BibTeX](gpu-topology.bib) |
| Geometry oracles | Which direct checks and corpora establish useful geometric correctness and fair performance? | In progress |

Research reports and annotated bibliographies will be committed here. Acquired
PDFs remain outside Git. Reference source is study material in ignored quarantine;
no reference instructions or library dependencies are adopted by collection.

Starting literature:

- [NVIDIA USD BRep](https://github.com/NVIDIA-Omniverse/usd-brep) and its [known issues](https://github.com/NVIDIA-Omniverse/usd-brep/blob/main/KNOWN_ISSUES.md).
- [Krishnamurthy et al. 2008 GPU NURBS operations](https://mcmains.me.berkeley.edu/pubs/SPM08gpuSSI.pdf): evaluator, bounds, inverse evaluation and intersection discovery.
- [Lévy 2025 exact mesh CSG](https://arxiv.org/abs/2405.12949): exact predicates, constructions and arrangement combinatorics.
- [Valque and Lazard 2025 output rounding](https://doi.org/10.1111/cgf.70197): converting exact intersections to floating-point coordinates can introduce new intersections.
- [Yang's recent intersection work](https://yangjieyin.github.io/publications/): topology guarantees, overlap regions and small/tangent branches; algorithm and availability details remain research questions.

The existing CPU benchmark is a starting measurement and fixture collection,
not a substitute for numerical and topological contracts in the owned kernel.
