# GPU traversal lessons for conservative patch discovery

Primary-paper study, 2026-10-14. Both acquired papers were read in full;
[the PDF catalog](bounds-gpu-pdfs.tsv) records sources, hashes and read scope.
This is scheduling research, not a P100 benchmark or geometry certificate.

## What the traversal studies establish

Aila and Laine's 2009 study separates ray/node traversal from primitive tests
and investigates idle SIMT lanes and uneven work distribution on a GTX285.
Persistent threads improve their measured traversal workloads by fetching
batches from a global pool. Speculative traversal can improve lane utilization
while increasing memory traffic. The paper also distinguishes measured kernels
from simulated queue/refill benefits. It studies traversal of an existing BVH,
not hierarchy construction, rational surfaces or outward FP64 bounds.
[Primary publication and PDF](https://research.nvidia.com/publication/2009-08_understanding-efficiency-ray-traversal-gpus).

The 2012 Fermi/Kepler addendum demonstrates why that scheduling prescription
cannot be transferred unchanged: persistent threads are unnecessary on its
Fermi workloads, while lane refill and memory access choices matter on Kepler.
Its integer min/max substitutions include ray-specific arguments that permit
some incorrect intermediate values without changing the final predicate. Such
arguments do not establish conservative FP64 box distances for our operation.
Neither paper measures GP100 or our candidate/projection workload.
[Primary addendum](https://research.nvidia.com/sites/default/files/publications/nvr-2012-02.pdf).

## Consequences for the owned operation

The following are BRepGPU design inferences, not results from those papers:

- Start with one bounded traversal per query over immutable packed nodes.
  Compare uniform easy queries with mixed, curved, boundary and unresolved
  queries; record node visits and refinement work alongside elapsed time.
- Keep conservative arithmetic independent of scheduling. A faster queue may
  reorder cells, but it must preserve source identity, every unvisited subtree
  and the same declared exclusion contract.
- Measure hierarchy construction and upload separately from repeated resident
  queries. Our interval-valued subdivision can cost more than a box predicate;
  query throughput alone cannot determine the useful amortization point.
- Keep traversal scratch bounded. The existing projector uses 166 registers
  per thread on the measured build; fusing interval nets, traversal and solver
  state therefore needs measurement before it becomes the default.
- Introduce persistent pools, compaction or separate refinement queues only
  after measurements show that their coordination, traffic and lifetime costs
  buy delivered work on the two P100s.

The [Wafer/GPU MODE code study](2026-10-14-wafer-gpu-mode-study.md) adds concrete
benchmark, ownership and boundary-handling lessons. The
[runtime study](2026-10-14-bounds-runtime-study.md) considers hierarchy layout
and stage queues. [Numerical](2026-10-14-bounds-numerical-study.md) and
[oracle](2026-10-14-bounds-oracle-study.md) studies supply the separate geometry
contract. Bibliography: [bounds-gpu.bib](bounds-gpu.bib).
