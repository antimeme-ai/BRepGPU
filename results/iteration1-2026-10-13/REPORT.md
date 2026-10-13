# First owned CPU/P100 evaluator measurements

The owned native evaluator delivers batched FP64 positions and first/second
rational patch derivatives. All 144 benchmark cases passed independent expanded
polynomial checks. Both P100s also passed exact binary-input and analytic
qualification, 27 public caller cases, and device memory/leak checking.

For **1,048,576 bicubic queries with the full six-term jet on one patch**, the
P100 device-0 resident call is **18.2× faster** than this native CPU
implementation with 24 requested workers. Its prepared native host-to-host path
is **1.98× faster**. These are operation-specific exploratory results
on a shared machine, not complete CAD modeling speedups.

## Full-jet results

Medians of seven samples after one excluded warmup; identical query bits,
control nets, output fields and public C ABI on each backend:

| Backend | Resident evaluation | Download | Prepared native host-to-host |
| --- | ---: | ---: | ---: |
| CPU, 1 worker | 496.728 ms | 47.918 ms | 551.755 ms |
| CPU, 24 workers | 111.498 ms | 49.367 ms | 168.977 ms |
| P100, device 0 | 6.115 ms | 54.976 ms | 85.214 ms |
| P100, device 1 | 6.150 ms | 49.293 ms | 73.264 ms |

Each public evaluation allocates a fresh result and includes checks, execution,
completion and handle publication. Downloads use prepared host output buffers.
Host-to-host timing includes native patch/query snapshots and upload, evaluation,
download and status consumption within an existing context. Input generation,
independent truth checks and child cleanup are outside that region. Python
packing/dispatch and context initialization are outside the native timers.
The separately measured phases are different operations and their medians should
not be added to reconstruct the host-to-host median.

## Batch size matters

Resident full-jet calls on the same one-patch fixture:

| Queries | CPU, 1 worker | P100, device 0 |
| --- | ---: | ---: |
| 1 | 0.000754 ms | 0.073188 ms |
| 4,096 | 1.388457 ms | 0.079030 ms |
| 1,048,576 | 496.727727 ms | 6.114872 ms |

At one query the CPU is faster. At 4,096 queries GPU overhead is amortized.
Twenty-four CPU workers are requested explicitly; native thread creation and
join remain inside each call, with no automatic small-batch threshold. A
one-query request actually uses one worker on either CPU setting.

The full jet and status occupy 148 bytes/query, or 148 MiB at the largest count.
Downloading to pageable host memory costs much more than the resident GPU call.
Input/host buffer placement, NUMA traffic and other host activity can affect
transfers; the per-device host-to-host difference is not evidence of a different
geometric algorithm. A downstream resident consumer is a useful next step.

## Qualification

Each backend executes 1,749 valid corpus requests and 19,170 direct component
checks. Exact truth uses ratios of the actual supplied binary64 inputs, alongside
independent polynomial/rational maps and literal dyadic jets. All sixteen degree
pairs and orders 0/1/2 pass. Worst errors:

| Derivative grade | Native CPU | Either P100 | Target |
| --- | ---: | ---: | ---: |
| Position | 3.210e-16 | 2.823e-16 | 2^-40 = 9.095e-13 |
| First | 3.411e-13 | 3.411e-13 | 2^-30 = 9.313e-10 |
| Second | 7.550e-10 | 1.221e-9 | 2^-20 = 9.537e-7 |

[CPU witnesses](cpu/QUALIFICATION.md) · [Device 0 witnesses](cuda0/QUALIFICATION.md) ·
[Device 1 witnesses](cuda1/QUALIFICATION.md) · [Check summary](checks.tsv).
These are finite-corpus grades, not per-result certified enclosures.

The 27 caller cases cover ownership, independent result storage, source lineage,
invalid-query NaNs, ABI rejection without destination writes, source/world frame
conversion and calling-thread device restoration. One GPU-specific restoration
case is inapplicable in the CPU invocation. Both full P100 suites report zero
Compute Sanitizer memory errors and zero leaked allocations.

Six fresh-process native scenarios pass under ASan/UBSan using an owned callback
model. They attack admission/close races, unpublished allocation failure,
cleanup fatality, partial download failure and restoration failure. The composed
allocation/fatal-cleanup case first failed against the old core with the expected
fatal-cause assertion and passed after repair. These model core error paths;
actual GPU hardware faults were not induced. The product CPU benchmark also
passed a 24-worker full-jet ASan/UBSan run. Test seams are disabled in the default
product build.

## Methods and provenance

[Raw 4,680 timing rows](benchmark/samples.tsv) · [All case summaries including
min/median/max](benchmark/summary.tsv) · [Build/run instructions](../../docs/ITERATION_1.md) ·
[Sanitized environment](environment.tsv) · [Source hashes](source.sha256) ·
[Measured binary hashes](binaries.sha256) · [Compiler resources](kernel-resources.tsv).

The exact owned source is at commit
[`15dd0f4`](https://github.com/antimeme-ai/BRepGPU/tree/15dd0f439dfd5d7a2a6e69845d3daf396883cfd0);
every file in source.sha256 matches that commit. No source copies or reference
kernel code are needed. The matrix spans 1, 32, 256, 4,096, 65,536 and 1,048,576
queries; positions, first and full jets; one coherent patch and sixteen shuffled
patches; CPU with 1/24 workers and each GPU separately. Sixteen patches form a
small resident set, not a large imported model.

Hardware: two Xeon E5-2697 v2 sockets, 48 logical CPUs, two P100-PCIE-16GB GPUs;
GCC 13.3.0, CUDA 12.8.93, driver 580.178.04. CPU uses direct differentiated
Bernstein sums; CUDA uses streamed homogeneous tensor de Casteljau. Both enforce
the same supported input and result contract. The CPU implementation has no
hand-tuned SIMD or persistent thread pool. Fast math and implicit contraction
are disabled; CUDA uses explicit FP64 fused interpolation and direct division.

CPU workers are unpinned. Clocks, SMT placement and unrelated shared-host
workloads are uncontrolled. No other BRepGPU build or test job ran during the
published matrix. Seven samples expose variation without establishing a
production performance guarantee.

The optional `profiled_kernel` rows bracket one kernel submission with CUDA
events in a separate profiling context. They can include stream scheduling or
submission gaps, particularly for tiny batches, and alter instrumentation costs.
They are not the unprofiled public wall time, a preallocated-loop throughput
measurement, or a basis for subtracting overhead from another run.
Compiler output shows zero register spill loads/stores, with 64/128/192 bytes of
per-thread stack for orders 0/1/2. Zero reported spills does not mean zero local
memory traffic.

## Actual callers

Both Python samplers ran through the P100 backend with 4,225 points and first
derivatives. The cylinder has zero observed position error and 2.221e-16 maximum
first-derivative/locus error. The dyadic polynomial sampler has zero observed
position/first-derivative error on this grid; its CPU execution also passed.

[Cylinder grid](sampler/cylinder-grid.svg) · [Cylinder point cloud](sampler/cylinder-points.ply) ·
[Cylinder recipe and exact inputs](sampler/RECIPE.md) ·
[Polynomial grid](sampler-polynomial/polynomial-grid.svg) ·
[Polynomial recipe and exact inputs](sampler-polynomial/RECIPE.md).

These are local untrimmed carrier samples. They do not perform trim
classification, certify a tessellation chord bound, or construct closed solids.
General NURBS span extraction, imported-model adapters, resident query consumers
and topology work follow the [roadmap](../../docs/ACCELERATION_ROADMAP.md).
