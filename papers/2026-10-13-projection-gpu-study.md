# Resident projection on P100

The iteration-one measurement motivates a consumer that performs several
geometric steps before returning: device-0 full-jet evaluation took 6.115 ms
for 1,048,576 queries, while the prepared native host path took 85.214 ms.
Those are different operations, not components whose medians can be subtracted.

The CUDA 12.8.1 guides advise reducing transfers, retaining intermediate data
on device and measuring representative workloads. Divergent paths within a
warp can reduce useful work; register and local-memory requirements also affect
execution. These are performance considerations, not geometry guarantees.
[Best practices](https://docs.nvidia.com/cuda/archive/12.8.1/cuda-c-best-practices-guide/index.html),
[programming guide](https://docs.nvidia.com/cuda/archive/12.8.1/cuda-c-programming-guide/index.html).

The first owned projection kernel should assign one request to a thread and
run a bounded solver there. It reads existing packed patch controls and uploaded
targets/seeds; intermediate jets remain device scratch. One compact candidate
record is written at termination. It needs no host iteration loop, per-step
jet transfer, shared scratch between requests or second public handle registry.

Use separate homogeneous and mixed-iteration benchmark cases. Publish stationary,
unresolved, rank and curvature counts, accepted-update and evaluator-call
distributions, upload/download/public latency and separately instrumented event
intervals. Fast rejection is not solved-query throughput. Compare equivalent
native CPU calls using the same settings and direct analytic answers.

Start with the existing 128-thread launch size, real sm_60 code and strict FP64
settings. Read compiler register/stack/spill reports and actual measurements
before adding wavefront queues, compaction or persistent scheduling. The
iteration-one compiler already reports stack storage without register spills;
additional solver scratch must be measured rather than presumed resident in
registers. CPU/device numerical qualification remains independent of profiling.
