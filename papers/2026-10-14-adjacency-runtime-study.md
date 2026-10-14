# Owned GPU edge-use adjacency: algorithms and runtime

Research and design advice, 2026-10-14. This study inspected the existing native
runtime, CUDA backend, ABI4 pair extension, Python bindings, build configuration,
the acquired Wafer/GPU MODE references, and the primary publications below. No
reference code was executed or copied into production; no dependency was adopted.
The recommendation is a bounded synchronous topology operation in a separate
library and build tree. Retained sessions and reusable allocation pools remain
round-two work, following the operator's broad-first instruction.

## Literature read and what it changes

Blelloch defines scan through an associative operation and gives a tree-based
parallel construction. For this operation integer counts have an especially useful
property: admitted total membership bounds every intermediate sum, so addition
can remain exact. His survey connects scan to allocation and sorting. The CMU
1990 report index currently links the later book chapter; the acquired PDF is
that chapter, not an asserted identical 1990 report.
[CMU report index](https://www.cs.cmu.edu/afs/cs.cmu.edu/project/scandal/public/papers/CMU-CS-90-190.html),
[acquired chapter](https://www.cs.cmu.edu/~guyb/papers/Ble93.pdf).

Harris, Sengupta and Owens describe a work-efficient block scan, scanning block
totals, adding block prefixes, and padding partial blocks. This supplies the
first owned implementation's global structure. Its examples target earlier
hardware and use floating-point values: neither their bank-count constants nor
reported speedups transfer to P100. We should use exact integer counts and
review the actual shared-memory layout for Pascal.
[GPU Gems chapter 39](https://developer.nvidia.com/gpugems/gpugems3/part-vi-gpu-computing/chapter-39-parallel-prefix-sum-scan-cuda).

Merrill and Garland's decoupled look-back reduces global scan traffic toward one
input read and one output write. Importantly, section 4.4 discusses deadlock when
active blocks await predecessors that have not been scheduled, and descriptor
publication requires consistent status/value visibility. This is a strong
round-two candidate when scan traffic actually dominates. The first beachhead
should use kernel boundaries and hierarchical scan: simpler forward progress,
bounded scratch, no inter-block polling. This choice is an engineering inference,
not a claim that hierarchical scan is faster.
[NVIDIA technical report NVR-2016-002](https://research.nvidia.com/sites/default/files/pubs/2016-03_Single-pass-Parallel-Prefix/nvr-2016-002.pdf).

Merrill, Garland and Grimshaw store graph adjacency as offsets of length n+1 and
concatenated members. Their section 5.1 studies serial, warp, scan and CTA
gathering and shows why degree distribution matters. Our first downstream reader
can avoid variable-length row traversal: classify by degree and inspect the two
members only when a unique opposed mate is possible. Later arbitrary edge-use
enumeration may need adaptive scheduling. Degree-two B-rep fixtures alone cannot
qualify the general packed incidence constructor's contention behavior.
[Scalable GPU Graph Traversal](https://research.nvidia.com/sites/default/files/pubs/2012-02_Scalable-GPU-Graph/ppo213s-merrill.pdf).

Satish, Harris and Garland explain GPU radix and merge sorting organized around
parallel primitives. Sorting is relevant only if the API promises canonical
physical row order. An atomic reservation order does not provide it. For this
beachhead, explicitly unspecified row order and schedule-independent typed
membership/reader semantics avoid a needless sorting stage. If a future consumer
requires ordered rows, stable radix partition or parallel merge construction
deserves separate measurement; serial insertion into a high-degree row does not.
[NVIDIA sorting technical report](https://www.nvidia.com/docs/io/67073/nvr-2008-001.pdf).

Pascal supports the integer atomic operations needed for histogram and scatter
reservation. GP100 has 64 KiB shared memory per SM and a 48 KiB per-block limit;
occupancy also depends on registers and thread count. No Hopper-only primitive,
Tensor Core operation, grid-wide cooperative barrier or inter-device atomic is
needed. CUDA 12.8.1 remains the selected sm_60 toolchain.
[Pascal tuning guide](https://docs.nvidia.com/cuda/pascal-tuning-guide/index.html),
[CUDA 12.8.1 atomic functions](https://docs.nvidia.com/cuda/archive/12.8.1/cuda-c-programming-guide/index.html#atomic-functions).

## Wafer/GPU MODE follow-through

The already-acquired Wafer README directly points to the decoupled-look-back
paper in its foundational exercises. That paper was acquired and read for this
study. A path search of the pinned GPU MODE archive did not find a standalone
scan/CSR kernel file; this is not a claim that every notebook or lecture was
exhaustively reviewed. The existing selected-code study's partial-block barrier,
immutable-input and measurement cautions apply directly here.
[Pinned Wafer README](https://github.com/wafer-ai/gpu-perf-engineering-resources/blob/1c52412c3e36b678b43bc414522a790a8cb0212c/README.md),
[previous code study](2026-10-14-wafer-gpu-mode-study.md).

## Recommended operation

Input is one admitted snapshot incidence table: edge slots, face slots and
orientations indexed by coedge slot; E and C are explicit. Python retains typed
identity and snapshot lineage. Native slots are dense indices, never pointers or
logical identity. Do not upload geometry, loops or UV maps merely to construct
edge-use incidence. Keeping two coedges distinct also preserves a periodic seam
even when their face slots are equal.

Output is CSR `offsets[E+1]`, `members[C]`, and an actual consumer's classification
and mate/neighbor arrays. Every input coedge appears exactly once in its edge
row. Rows have no physical ordering guarantee; integer offsets and semantic
classification are deterministic. Tests compare typed membership without losing
multiplicity, and reader outputs by original coedge identity. Reordered snapshot
slots must change only indices and preserve typed semantics after remapping.

The GPU construction is:

1. Zero E counters. Each coedge atomically increments its admitted edge counter.
2. Exclusive scan counts using fixed-size, zero-padded block tiles. Write each
   tile's sum to an auxiliary array, recursively scan that array on the GPU, then
   add its tile prefix to local offsets. Set the final offset to C. Every block
   lane participates in every block barrier, including the partial final tile.
3. Reset E reservation cursors. Each coedge reserves one position with integer
   atomic add and writes its original slot at `offsets[edge]+reservation`.
4. Launch the reader after fill on the same private stream. A degree-two row
   with opposed orientations yields the distinct mate for each coedge and its
   source face. Equal face slots are permitted. Degree zero, one, greater than
   two, and degree-two orientation conflict have explicit classifications and
   an invalid mate sentinel. They do not manufacture a unique neighbor.

The reader really consumes the constructed members, rather than reimplementing
an independent direct-edge lookup or reading only counts. Degree classification
does not prove manifoldness of a whole body; it answers the admitted edge-use
question. Construction keeps all members for high-degree rows. One hot row may
serialize atomics but does not require O(C squared) search, a degree-sized stack
buffer, or a silent degree cap.

Do not copy block totals back for a CPU scan. That introduces a synchronization
and transfer boundary inside the claimed GPU constructor and weakens the value
of a GPU adjacency beachhead. Recursive totals require a geometrically decreasing
amount of scratch and terminate in one block. For a fixed admitted bound, compute
all hierarchy extents before allocating anything.

The CPU1 implementation can count, scan and fill serially. CPU24 can partition
coedges and use checked integer atomics for counts/cursors, with independent
rows for the reader. A serial prefix on the CPU path is allowed if disclosed;
do not describe the complete CPU24 path as parallel scan unless it actually is.
Neither CPU result order nor CPU parity is the semantic oracle.

## Admission, memory and failure

Recommended bound is C <= 2^22 with E and face count separately bounded, checked
as uint64 before narrowing to slots. Reserve UINT32_MAX for invalid mate/face;
all legal slots and every cumulative offset must fit below it. Check E+1,
products for every byte extent, and hierarchy allocation sizes before use.
Validate every edge/face slot and orientation on the host before GPU allocation
or output writes. C>0 with E=0 is invalid; C=0 is meaningful and emits E+1 zero
offsets and the appropriate degree-zero classifications. No zero-block launch.

Under the C bound, per-edge counts and all subset totals are <= C; uint32 count
and prefix arithmetic is exact. Do not use floating-point scan. Track every
allocation immediately after its success so a later failure cleans earlier
buffers. Scratch, device buffers, stream and profiling events are call-owned.
Keep the first error; append secondary synchronization/free/device-restore
failures instead of replacing the primary cause.

Preflight failure writes no caller arrays. Runtime failures may have partially
written private host buffers; the wrapper discards the entire operation and
publishes no result. Do not promise transactional arbitrary caller-buffer writes
unless native staging is implemented. Synchronize before host buffers leave
scope and before destroying temporary storage. Restore the prior CUDA device
before reporting success. No device reset, process exit, opaque pointer exposure
or retained session is needed.

## Integration with existing binaries

Inspected `native/runtime.cpp`, `native/pair_runtime.inc`,
`native/pair_backend_api.h`, `native/pair_cuda.inc`, `native/cuda_backend.cu`,
`python/brepgpu/__init__.py`, and root `CMakeLists.txt`. Existing operations retain
resource handles, enforce extension/base identity, and use CUDA save/select/
restore with guarded cleanup. Those behaviors are useful references, but adding
topology to their resource graph would widen this iteration unnecessarily.

A dedicated synchronous ABI5 library can provide an ABI-layout query and one
bounded CPU/CUDA operation consuming owned flat buffers. Python checks the exact
ABI sizes/version before calling it. No mutation of v1-v4 table layout or old
handles is necessary. A separate `build/topology-release` tree keeps pair and
discovery qualification binaries intact. The existing CUDA 12.8.1 sm_60 flags
can build the new target. This is a design recommendation; no code or binary was
built as part of this research.

## Costs to expose

Record external caller wall time, packing/admission, native host preflight,
device selection/runtime initialization, stream creation, allocation, upload,
count/scan/fill, reader, download, synchronization, cleanup/device restoration,
and Python typed consumption. CUDA-event scopes measure device work only and
need honest labels; a host native call includes launch/allocation/sync costs.
Inputs/outputs and scratch bytes are separate; kernel scratch traffic is not
host/device transfer traffic. Compute expected upload/download byte totals from
the actual frozen ABI, not from an aspirational compact representation.

Measure first call in a fresh process, then repeated fresh-operation calls after
runtime initialization. Repeating this synchronous operation is warm runtime,
not resident topology: it still allocates, uploads, builds, downloads and frees.
Use identical incidence bits for CPU1, CPU24 and each P100, one excluded warmup
and repeated trials. Include tiny real box/cylinder/plate snapshots, replicated
body batches, sparse rows, and a high-degree contention fixture. The latter is a
packed-incidence workload, not a fabricated manifold CAD body. Benchmark flags
and mates must be consumed and independently qualified before timing claims.

Round two may examine allocation/session reuse, upload elision, scan look-back,
warp-aggregated histogram updates, downstream GPU residency and bulk typed output.
No speedup is established by this study. Its constructor, reader and complete
caller will each need measured costs; an integer scan speedup alone is not a
GPU B-rep modeling speedup.

## Acquisition

[Bibliography](adjacency-runtime.bib) and [PDF acquisition catalog](adjacency-runtime-sources.tsv)
record original sources and ignored local PDF filenames/hashes. Restore a PDF
with `curl -fL SOURCE_URL -o papers/pdfs/FILENAME`, then verify the catalog SHA256.
The source archives already acquired for Wafer/GPU MODE remain intact; their
restoration manifest is unchanged. No additional reference implementation was
imported for this study.
