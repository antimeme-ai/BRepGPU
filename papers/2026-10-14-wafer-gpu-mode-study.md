# Wafer's GPU-performance map and linked GPU MODE code

Research only, 2026-10-14. We acquired both the Wafer resource repository and the
GPU MODE lecture repository it explicitly links. Wafer's repository is a
curriculum/resource index, not a shipped kernel compiler or benchmark evaluator.
The linked lecture repository contains teaching kernels and profiling examples.
Neither is selected as a dependency. The useful next action is to apply their
execution lessons to our owned conservative-discovery operation while keeping
its geometry and failure oracles independent.

## Identity, acquisition and actual reading

The inspected Wafer README identifies itself as an AI performance-engineering
resource list and links GPU MODE lectures as its practical companion. The
references are pinned to Wafer `1c52412c3e36b678b43bc414522a790a8cb0212c` and GPU
MODE `77a8df418834e5789c12da23e7d2719e0efabef1`.
[Wafer README](https://github.com/wafer-ai/gpu-perf-engineering-resources/blob/1c52412c3e36b678b43bc414522a790a8cb0212c/README.md),
[GPU MODE README](https://github.com/gpu-mode/lectures/blob/77a8df418834e5789c12da23e7d2719e0efabef1/README.md).

Intact source ZIPs and clean extracted references are in ignored quarantine.
Acquisition sizes, hashes, revisions and URLs are recorded in
[the archive catalog](wafer-gpu-mode-sources.tsv); exact commands and licensing
observations are in [restoration instructions](wafer-gpu-mode-reference-restoration.md).
The Wafer ZIP imports 10 files; the approximately 188 MiB lecture ZIP imports
194. We read the Wafer README, contribution policy and guide checker; GPU MODE's
README/LICENSE, shared extension helper, lecture 1 square/profiling examples,
lecture 8 occupancy/divergence/coarsening/shared-window examples, lecture 9
reductions/precision examples and lecture 18 profiler driver. Individual paths
and hashes are recorded in [the inspected-file catalog](wafer-gpu-mode-entrypoints.tsv).
We did not execute reference scripts, install their packages, create accounts,
download models or run their benchmarks. This is a selected-code study, not a
review of every lecture or a measured performance reproduction.

## What the Wafer map contributes

The map connects compiler/PTX/SASS reading, foundational reductions/scan,
roofline analysis, profiling and workload accounting to higher-level scheduling.
That order is useful for BRepGPU: first identify the admitted geometry operation,
its traffic and work distribution, then measure an owned implementation. Its
`check_guide.py` checks resource-guide links/anchors and formatting; it does not
certify kernel outputs. The modern inference, Tensor Core, TMA and low-precision
sections do not establish applicability to our FP64 P100 operation.
[Guide checker](https://github.com/wafer-ai/gpu-perf-engineering-resources/blob/1c52412c3e36b678b43bc414522a790a8cb0212c/scripts/check_guide.py),
[resource index](https://github.com/wafer-ai/gpu-perf-engineering-resources/blob/1c52412c3e36b678b43bc414522a790a8cb0212c/README.md).

## Actual code findings and their limits

These are source-inspection findings, with analytic witnesses where stated;
we did not run the examples. They do not discredit the lecture material as
teaching material. They explain why it cannot become our production contract.

| Inspected example | Concrete observation | BRepGPU consequence |
| --- | --- | --- |
| Lecture 1 square extension | A float32, flat-indexed kernel is launched from a Torch wrapper using the default launch stream. The wrapper does not explicitly guard the input device or Torch's current stream, validate contiguous two-dimensional storage, or check completion errors. A printed tiny answer supplies only one example. | Keep our checked snapshots, device save/select/restore, private stream, explicit synchronization, diagnostics and retained result handles. A convenient JIT wrapper cannot replace them. |
| Lecture 8 occupancy | The occupancy-query call is commented out; block size 1024 and minimum grid 40 are literals. Managed-memory copy code does not compare output values. | Neither number is an experimentally determined launch choice for our FP64 solver or hierarchy traversal. |
| Lecture 8 divergence | The timing helper copies the first kernel's mutated output back into its caller's input; the second kernel then consumes those different bits. There is one timed launch per variant and no equality assertion. | Variant comparisons must use identical immutable requests, repeat samples, and qualify outputs before reporting speed. |
| Lecture 8 shared window | The shared version loads one block without neighboring halo values. For all-one input, window size 5 and block size 256, interior index 256 produces 3 rather than the direct kernel's 5. Its barrier is also inside `idx<n`, unsafe for a partial last block. | Shared-memory staging must preserve geometric neighborhoods and boundary handling. Fast output without a direct oracle is insufficient. |
| Lecture 9 two-device reduction | Each device receives the full same 10,000-one input, then the host adds both device results. The stated computation therefore produces 20,000, rather than partitioning one 10,000-element reduction. | Multi-GPU work ownership must be explicit. Our two P100 measurements are separate per-device trials; a future distributed query batch must partition IDs without duplicates or omissions. |

Primary code:
[lecture 1 wrapper](https://github.com/gpu-mode/lectures/blob/77a8df418834e5789c12da23e7d2719e0efabef1/lecture_001/load_inline.py),
[occupancy](https://github.com/gpu-mode/lectures/blob/77a8df418834e5789c12da23e7d2719e0efabef1/lecture_008/occupancy.cu#L26-L31),
[divergence](https://github.com/gpu-mode/lectures/blob/77a8df418834e5789c12da23e7d2719e0efabef1/lecture_008/divergence.cu#L46-L62),
[window sum](https://github.com/gpu-mode/lectures/blob/77a8df418834e5789c12da23e7d2719e0efabef1/lecture_008/privatization2.cu#L20-L40),
[two-device reduction](https://github.com/gpu-mode/lectures/blob/77a8df418834e5789c12da23e7d2719e0efabef1/lecture_009/multistream-reduce.cu#L41-L104).
CUDA requires conditional block barriers to have a condition uniform across the
block, supporting the partial-block finding.
[CUDA 12.8 synchronization functions](https://docs.nvidia.com/cuda/archive/12.8.1/cuda-c-programming-guide/index.html#synchronization-functions).

The coarsening example does separate warm-up from CUDA-event timing, though it
uses a single measured sample and supplies no elementwise oracle. The Torch
profiler examples explicitly schedule wait/warm-up/active phases and can record
CPU/CUDA activity, memory and stack information. Those are useful measurement
patterns. The shared helper's CUDA-error macro exits the process; that behavior
would violate our error-reporting/library-lifetime contract. Precision examples
illustrate lost small contributions and order-dependent summation, not a recipe
for conservative geometric intervals.
[Coarsening](https://github.com/gpu-mode/lectures/blob/77a8df418834e5789c12da23e7d2719e0efabef1/lecture_008/coarsening.cu),
[profiler driver](https://github.com/gpu-mode/lectures/blob/77a8df418834e5789c12da23e7d2719e0efabef1/lecture_018/kernels/profile_fused_kernel_add_mul_activation.py),
[helper](https://github.com/gpu-mode/lectures/blob/77a8df418834e5789c12da23e7d2719e0efabef1/utils.py),
[precision](https://github.com/gpu-mode/lectures/blob/77a8df418834e5789c12da23e7d2719e0efabef1/lecture_009/accuracy.py),
[association](https://github.com/gpu-mode/lectures/blob/77a8df418834e5789c12da23e7d2719e0efabef1/lecture_009/nondeterminism.py).

## P100 applicability and profiler support

Do not translate lecture 8's generic `ncu` command into a P100 qualification.
The current official Nsight Compute support table lists Pascal as unsupported;
Nsight Compute 1.0 supported GP10x while explicitly excluding GP100. Merely
choosing the first release would therefore not solve P100 support.
[Current support table](https://docs.nvidia.com/nsight-compute/ReleaseNotes/topics/gpu-support.html),
[Nsight Compute 1.0 requirements](https://developer.nvidia.com/nsight-compute-1_0).

Use the already-qualified CUDA 12/sm_60 setup, native host-wall and private-stream
CUDA-event scopes, compiler register/stack/spill reports, and binary inspection
for the next implementation. Retain Compute Sanitizer as an independent memory
fault mechanism; it does not prove interval enclosure. Nsight Systems tracing
was not installed or qualified by this study; its specific version and hardware
support must be checked before any new trace claim.
[Owned toolchain](../docs/CUDA_TOOLCHAIN.md),
[owned operation guide](../docs/ITERATION_2.md).

For GP100, NVIDIA documents a 64k 32-bit register file per SM, 255-register
per-thread limit, 64 concurrent warps per SM, 64 KiB shared memory per SM and
48 KiB per-block limit. These ceilings make a hierarchy heap, interval-net
scratch and the existing solver's register use material design costs. Occupancy
alone cannot choose the fastest block size.
[Pascal tuning guide](https://docs.nvidia.com/cuda/pascal-tuning-guide/index.html#occupancy).

## Apply the lessons to conservative discovery

Our recommendation is a resident, bounded named-patch traversal followed by
batched candidate refinement. Precompute immutable interval-enclosed boxes and
packed chart/source IDs once; upload once; measure target ingestion, traversal,
candidate projection, synchronization and compact download separately as well
as together. Keep CPU 1/24-thread and each P100 comparisons on identical input
bits, budgets, candidate limits and numerical rules. Record node visits,
frontier high-water, accepted candidates, solver calls and unresolved rates
alongside latency. A variant that drops a branch is a different operation.

First implement deterministic per-query bounded traversal; consider compaction
or stage queues only when measured divergence/register pressure justifies their
additional traffic and admission machinery. Overflow must retain conservative
unresolved coverage and an explicit status. Neither tutorial block coarsening,
float reduction nor a faster printed answer certifies B-rep geometry. The full
residency/queue proposal is in
[the bounds-runtime study](2026-10-14-bounds-runtime-study.md).

## Bibliography and boundaries

Pinned repository/file URLs above are the primary implementation bibliography;
[wafer-gpu-mode.bib](wafer-gpu-mode.bib) records the repository and official
hardware/profiler references. GPU MODE supplies an Apache-2.0 license; the Wafer
README declares MIT without a LICENSE file in this archive. No code was copied
into product sources, no library was adopted, and no speedup or full-repository
correctness claim follows from this inspection.
