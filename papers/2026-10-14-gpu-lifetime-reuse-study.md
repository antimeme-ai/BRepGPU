# GPU setup/teardown and deferred lifetime reuse

2026-10-14. Research notes only. The operator explicitly deferred integration,
session/pool implementation and new measurements until the broad pass's big
rocks are complete. No optimization lane is active.

## What the existing observations mean

The actual exported pair callers recorded cold_engine_ms 273.507/484.439 and
engine_cleanup_ms 39.374/79.682 on P1000/P1001. Index cleanup was separately
0.284/0.476 ms, and page-child cleanup 0.042/0.050 ms. These are single caller
observations, not per-API timings or repeat distributions. The P1000 caller
has four bodies per side and the P1001 caller sixteen; these observations are
not a matched backend comparison.

Read native/runtime.cpp Plugin/load_plugin/Context/finish_backend/bg1_close,
native/cuda_backend.cu DeviceScope/context_create/destroy/Pending/allocate/finish,
pair_cuda.inc and examples/discover_pairs.py's timer boundaries. Engine creation
includes Python/core/plugin admission, dlopen/factory negotiation, device
selection/capability queries and stream creation. Engine cleanup includes core
ownership/registry destruction, backend stream synchronization/destruction,
device restoration and potentially final Plugin dlclose. Page/index buffer
cleanup has already occurred in the named separate spans. No trace separates
these remaining causes; module/fatbinary teardown during unload is a hypothesis,
not an established explanation of 39/79 ms.

Our native Context owns a stream and registry lifetime. It does not explicitly
create/destroy a CUDA primary context. Source has no cudaDeviceReset call.
CUDA 12.8.1 documents cudaSetDevice initialization of the selected device's
primary context and its shared process/device mapping; primary contexts persist
until explicitly reset. New Engine creation therefore does not imply a new
primary context every time. Avoid resetting a shared primary context as ordinary
library cleanup. [CUDA 12.8.1 runtime/driver interaction](https://docs.nvidia.com/cuda/archive/12.8.1/cuda-runtime-api/group__CUDART__DRIVER.html).

DeviceScope also restores the prior device with cudaSetDevice. In a cold process
that can initialize another device while restoring it; source permits this,
but the recorded timings do not isolate whether it happened or its cost.

Module/kernel loading can be deferred until first use; preloading a selected
kernel through cudaFuncGetAttributes is documented. Actual loading mode and
first-use costs must be recorded before changing policy. Warming an entire
backend could load unused kernels, so a warmup must have a stated workload.
[CUDA 12.8.1 lazy loading](https://docs.nvidia.com/cuda/archive/12.8.1/cuda-c-programming-guide/index.html#lazy-loading).

## Owned options to revisit after the broad pass

| Option | Concrete reuse | Constraint/tradeoff |
| --- | --- | --- |
| Long-lived process with explicit per-device Engine session | Keep existing Engine, plugin ownership and stream open across user operations | First use/final drain remain real; explicit owner/shutdown, bounded resident data and process restart on fatal CUDA error |
| Retained scene/patch/index handles | Reuse prepared CPU bounds and uploaded immutable geometry | Cache by exact snapshot/revision and owning Engine/device/plugin identity; live borrowers prevent eviction; body topology stays CPU |
| Retained plugin ownership across Engine recreation | Separate module/library reuse from stream lifetime | Measure first; exact library identity, callback pointers and unload order must remain valid; no hidden permanent dlopen cache or RTLD_NODELETE policy by default |
| Small leased device-buffer pool | Reuse page/query/result allocations after their owning token closes | Preserve independent results, capacities, tail initialization, stream completion, allocation failures and cleanup/fatal diagnostics; bound reserved bytes |

The simplest integration unit is an explicit long-lived process/session around
the existing ABI, not new CUDA contexts or a replacement runtime. One retained
Engine per selected device can hold its resident inputs; temporary page handles
still close normally. Session close drains children and reports cleanup, and
borrowed operations cannot keep using a closed public Engine. Preserve existing
caller-device restoration and context serialization. A session is not permission
to remove synchronous completion or the process-wide CUDA poison rule.

Pooling addresses a different cost: current pair calls allocate a fresh mask
and free it after download. A slot must remain leased through its result token's
lifetime, even after computation finishes, so a subsequent result cannot overwrite
a downloadable predecessor. Ordinary cudaFree can synchronize; source already
calls finish explicitly before frees. [CUDA 12.8.1 memory management](https://docs.nvidia.com/cuda/archive/12.8.1/cuda-runtime-api/group__CUDART__MEMORY.html).
Stream destroy does not itself guarantee synchronous drain, so the existing
explicit completion boundary must not be removed casually.
[CUDA 12.8.1 stream management](https://docs.nvidia.com/cuda/archive/12.8.1/cuda-runtime-api/group__CUDART__STREAM.html).

cudaMallocAsync is a possible later alternative, not an assumed P100 capability:
query cudaDevAttrMemoryPoolsSupported. Its pool release threshold can return
memory at synchronization, so a zero threshold may defeat desired reuse in our
synchronous APIs. Capability, reservation limits and failure semantics need
qualification before selecting that path. [CUDA 12.8.1 stream-ordered allocator](https://docs.nvidia.com/cuda/archive/12.8.1/cuda-runtime-api/group__CUDART__MEMORY__POOLS.html).

## One deferred measurement proposal

After the broad pass, measure the same qualified workload in fresh-process,
same-process Engine recreate, and retained-Engine/resident-input modes. Record
first device initialization, first used kernel, per-operation wall, input upload,
page close, final index/stream close and final plugin release separately. An idle
retained Engine using the identical plugin is a bounded way to compare nonfinal
versus final plugin release without adopting a global library cache. Attribute
dlclose/module work only after a scoped host/API trace or direct instrumentation;
do not infer it by subtraction of unrelated observations.

Keep correctness checks and prior device restoration, serial owned trials,
source/binary identities and shared-host limitations. Record final drain and
process exit separately from reused operation latency: amortization moves cost,
it does not erase it. Only if remaining allocation cost is material should a
separate bounded-buffer experiment follow. No measurements or implementation
from this proposal were performed here.
