# First iteration: runtime and host/device boundary

Research and initial read-only inventory date: 2026-10-13. The initial study proposed the runtime contract without installing or launching GPU work. Subsequent explicit authorization installed an isolated CUDA compiler/toolchain and qualified it on both P100s. The [setup instructions and qualification](../docs/CUDA_TOOLCHAIN.md) record that work. No CAD product implementation or CAD GPU performance benchmark was performed.

## Recommendation

Use an **owned C ABI with a standard-library Python `ctypes` adapter** for the first batched evaluator. Build a CPU library that loads without CUDA and an optional CUDA library loaded only when that backend is requested. Keep device objects behind checked handles. Public operations complete synchronously; geometry, queries and results can remain resident between calls, and downloading is explicit.

This keeps numerical code independent of Python, gives the native benchmark the same entry points, and avoids adopting a binding dependency for a small batch API. A direct owned CPython extension is a credible next option if general buffer interoperability or binding overhead becomes material. No binding-overhead advantage has been measured.

## Actual machine capabilities

The authorized read-only inventory of the initial execution host produced:

| Component | Observed |
| --- | --- |
| GPUs | Two NVIDIA Tesla P100-PCIE-16GB |
| Per-device memory | 16,384 MiB reported by `nvidia-smi` |
| Compute capability | 6.0 on both devices |
| NVIDIA driver | 580.178.04 |
| CPU | Two Intel Xeon E5-2697 v2 sockets; 12 physical cores/socket, two threads/core; 48 logical CPUs |
| GCC / G++ | 13.3.0 |
| CMake | 3.28.3 |
| Python | CPython 3.12.3, including the existing experiment environment |
| Python development headers | `Python.h` present; interpreter configuration command available |
| Architecture / C library | x86-64 / glibc 2.39 |
| CUDA compiler | Initially absent from inspected locations; now isolated nvcc 12.8.93 from the CUDA 12.8.1 release |
| Toolkit packaging | Official component archives in a user-owned prefix; no system package installation |
| Loader inventory | System driver `libcuda` present; shared `libcudart.so.12` loads from the isolated prefix |

The initial bounded inspection covered PATH lookup, the compiler-location index, standard CUDA directories and package/loader inventories. It did not prove no private or nonstandard SDK existed elsewhere, but identified the missing accessible compiler as a build blocker. **That blocker is now resolved** with the isolated CUDA 12.8.1 component toolchain. The working driver did not need replacement.

NVIDIA's current matrix lists CUDA 12.x as Pascal's final toolkit family and R580 as its final driver branch. Build native `sm_60` code explicitly; do not use the newest toolkit's default architecture or assume that the driver-reported CUDA ceiling identifies an installed compiler. CUDA 12.9's compiler documentation retains `sm_60`; CUDA 12.8 supports GCC 6–14 on x86-64, so the observed GCC 13.3 fits that documented host-compiler range. [NVIDIA architecture matrix](https://docs.nvidia.com/datacenter/tesla/drivers/latest/cuda-toolkit-driver-and-architecture-matrix.html), [CUDA 12.9 compiler guide](https://docs.nvidia.com/cuda/archive/12.9.1/cuda-compiler-driver-nvcc/index.html), [CUDA 12.8 Linux guide](https://docs.nvidia.com/cuda/archive/12.8.0/cuda-installation-guide-linux/index.html).

Implemented toolchain decision: pin CUDA release 12.8.1, nvcc 12.8.93, cudart 12.8.90 and Compute Sanitizer 2025.1.0.0. Native `sm_60` C++17 compilation, shared runtime linking, standalone cubin output and CMake configuration/build all passed. An FP64 analytic probe passed on both devices with zero numerical error, and sanitizer memcheck/full leak checks reported no errors or leaked bytes. Keep this compiler identity and explicit architecture in every product result.

The initial inventory established driver visibility and hardware identity; subsequent setup qualification also established successful allocation, transfers and kernel execution on both cards. Neither activity establishes exclusive access or peer access. Start product work with one explicitly selected device; the second card can run independent benchmark processes later.

## Binding options

| Option | Concrete benefit | Costs and limits | Decision |
| --- | --- | --- | --- |
| Owned C ABI + `ctypes` | No third-party binding dependency; reusable native interface; ordinary shared library independent of the Python ABI | Must explicitly declare every prototype; generic read-only/noncontiguous buffer borrowing is awkward; handle and error contracts are owned work | Recommended first adapter, one call per batch |
| Owned CPython extension | Direct buffer protocol, strong exporter references, typed exceptions and owned Python resource types | Python API/build coupling; reference counting, GIL transitions and partial-failure cleanup must be implemented | Keep as a measured follow-up, feasible on inventoried host |
| pybind11 | Convenient C++ class/function bindings, exceptions and buffer adapters | Proposed third-party adoption; still requires explicit lifetime/GIL work; NumPy conveniences may add runtime dependencies | Study only; not selected |

`ctypes.CDLL` releases the GIL while calling native functions; native code must not touch Python objects. It assumes an `int` return unless `restype` is supplied. Set `argtypes` and `restype` for every symbol, including handles and count outputs. `from_buffer` borrows a writable exporter; `from_buffer_copy` creates a copy. Keep every backing object and borrowed view alive for the complete call. [Python 3.12 ctypes documentation](https://docs.python.org/3.12/library/ctypes.html).

For a direct extension, acquire a buffer with the required contiguity/format flags, retain its exporter reference while native work runs, and release it exactly once on success and every error path. A buffer's logical byte count does not authorize flattening a strided/indirect object. `Py_buffer` and relevant functions entered the Stable ABI in 3.11; using the Limited API may widen Python-version support, but platform ABI and semantic compatibility still need explicit support scope. [Python buffer protocol](https://docs.python.org/3.12/c-api/buffer.html), [C API stability](https://docs.python.org/3.12/c-api/stable.html).

pybind11 does not automatically release the GIL for ordinary bound C++ calls. A memoryview created from unmanaged C++ memory does not supply ownership; its source allocation must outlive it. Its buffer support can be used without NumPy, whereas `py::array` facilities add a NumPy runtime dependency. These are useful conveniences, not an answer to the ownership contract. [pybind11 GIL documentation](https://pybind11.readthedocs.io/en/stable/advanced/misc.html), [buffer/NumPy documentation](https://pybind11.readthedocs.io/en/stable/advanced/pycpp/numpy.html).

## What the existing experiment actually calls

The [owned CPU driver](../benchmarks/cpu/benchmark.py) evaluates a patch through 10,000 scalar `Surface.evaluate(u,v)` calls, or scalar `Surface.derivatives(u,v)` calls, and builds a Python list of results. The inspected USD BRep binding converts each result into tuples and returns position plus first U/V derivatives. The inspected definitions have no explicit GIL-release guard. This baseline therefore includes scalar binding and Python-loop costs.

The reference also contains explicit preflight machinery for live Python wrappers borrowing B-rep-owned surfaces/curves that a mutation may replace. That illustrates a fault to avoid: a Python object retaining a native address does not make kernel ownership immutable. Our first uploaded patches should own a snapshot rather than borrow a source kernel's mutable surface objects. [USD BRep scalar bindings](https://github.com/NVIDIA-Omniverse/usd-brep/blob/main/source/SmPyLib/src/SmPyTypes.cpp), [borrowed-geometry preflight](https://github.com/NVIDIA-Omniverse/usd-brep/blob/main/source/SmPyLib/src/SmPyBorrowedGeometry.cpp).

The local source snapshot is study material; those live links can move. No reference binding code is incorporated into the proposed owned implementation.

## Proposed native values and layouts

Use native arrays and ordinary C structs at the ABI; no JSON, Python objects, STL containers, C++ exceptions, device pointers or compiler-dependent C++ classes cross it. Initial support is native-endian x86-64 with IEEE binary64 and fixed-width integer fields. An ABI version and descriptor byte size identify each descriptor; mismatches fail before payload access. Explicit C symbols use the platform C calling convention.

The following is a proposed layout, subject to the numerical owner's degree/knot contract. It supports tensor patches and later multiple-patch batches without pretending to be a complete B-rep representation.

| Buffer | Type and layout | Invariant |
| --- | --- | --- |
| Patch metadata | Fixed C descriptor containing degrees/counts plus 64-bit offsets into flat knot/control arrays | Offsets and lengths fit supplied arrays; no raw device address |
| Controls | FP64 homogeneous `(wx,wy,wz,w)` tuples, patch-major, then `i` in U, then `j` in V; component index `4*(i*n_v+j)+c` | Finite stored coordinates; positive accepted weights; packer rejects overflow when multiplying Euclidean coordinates by weights |
| Knots | Separate flat FP64 U/V arrays for the general spline form | Nondecreasing; count `n_control + degree + 1` in each direction, with the numerical contract's endpoint/multiplicity conditions |
| Query batch | Separate contiguous `uint32 patch_id`, FP64 `u`, FP64 `v` arrays | Equal count; patch indices in range; every output preserves query index |
| Result jets | FP64 structure-of-arrays: `((term*3+coordinate)*query_count + query_index)` | Terms ordered `S, S_u, S_v, S_uu, S_uv, S_vv`; first-order mode uses three terms and position-only uses one |
| Query status | One fixed-width status and validity mask per query | A valid bit authorizes its corresponding term; failed terms do not become plausible zeros |

For a Bézier-only first kernel, use its degrees and control counts and omit knots from that kernel's input type. Do not pretend that a single-span Bézier patch covers arbitrary trimmed NURBS; spline decomposition is a separate operation and cost.

A count is a count of elements, not bytes. Every native entry point checks integer multiplication/addition for overflow before calculating byte counts or allocating. Check pointer/null rules, descriptor size/version, scalar type, count/shape compatibility, buffer extent, alignment and supported degree. Define empty batches explicitly: no payload pointer is dereferenced and the result count is zero. Reject negative Python dimensions before converting them to unsigned values.

Python's `array` module supplies compact native numeric buffers, but actual item sizes are platform dependent. Assert the expected widths for `array('d')` and the chosen integer storage. An exported buffer/view prevents a storage-resizing mistake; it does not prevent another thread from modifying the same bytes. [Python array documentation](https://docs.python.org/3.12/library/array.html).

Initially the high-level adapter packs caller data into private owned arrays and snapshots it into native objects on creation/upload. Output host arrays are newly allocated and filled before being exposed to the caller. Direct C callers must keep every input allocation valid and stable until the synchronous call returns, including during its snapshot copy. A separate advanced `evaluate_into` entry point can later borrow caller buffers under an explicit no-mutation/no-resize/no-alias contract; count checks cannot make a forged C pointer safe.

Malformed layout must fail structurally before dispatch. Validly shaped queries outside the supported parameter domain can receive per-query statuses if mixed-query operation is part of the numerical contract. The public wrapper checks statuses before offering a convenience view of all-success geometry; raw diagnostic access remains available.

## Handles, lifetimes and calls

Use integer handles checked against an owned registry, including object kind, generation and parent context. They are process-local capabilities for this library instance, not serialized entity identity. Never dereference a caller-supplied pointer as an object handle. A stale, wrong-kind or cross-context handle fails cleanly.

The proposed synchronous surface API has these operations:

| Operation | Ownership and completion |
| --- | --- |
| `create_context(cpu)` / `create_context(cuda, device)` | Select backend explicitly; CUDA mode creates the context's private stream and reports capability/load failure |
| `create_patch_batch(context, host_arrays)` | Validate and own a native snapshot; upload in CUDA mode; return only after upload completion |
| `create_query_batch(context, host_arrays)` | Same snapshot/completion rule, allowing repeated resident evaluation |
| `evaluate(context, patch_batch, query_batch, order)` | Return a new owned result handle only after computation and required status checking complete |
| `download(result, host_output)` | Validate capacities and copy jets/statuses; return only after host output is ready |
| `close(object)` | Release its owned resources once; double-close at Python level is harmless; use after close fails |
| `close(context)` | Close/invalidate owned children in a documented order; no later operation uses them |

Python wrapper objects retain the library and parent context strongly and support explicit `close()`/context managers. `close()` is the primary cleanup path. Finalization is best-effort and cannot report reliable exceptions during interpreter shutdown. Do not expose borrowed memoryviews onto device storage; downloaded host arrays own their storage independently of the device result's lifetime.

Serialize operations per native context with a mutex. The same context's `close` cannot race a call after `ctypes` releases the GIL. The registry must retain an in-flight resource until the operation ends. Different contexts can execute independently; do not advertise cross-context resource sharing or multi-device batches in this iteration.

Bind a context to its device and re-establish the selected device inside every native CUDA operation, because callers may arrive on different host threads or share the process with other CUDA users. Use the owned stream for all copies/kernel launches, and synchronize that stream before returning success. This leaves device data resident while giving Python a simple completion and lifetime contract.

CUDA kernel launches are asynchronous. Even the nominally synchronous host-to-device copy from pageable memory can return after staging rather than final DMA completion. The public wrapper's synchronous guarantee must therefore come from explicit stream completion, not the copy function's name. [CUDA API synchronization behavior](https://docs.nvidia.com/cuda/archive/12.8.0/cuda-runtime-api/api-sync-behavior.html), [CUDA stream API](https://docs.nvidia.com/cuda/archive/12.8.0/cuda-runtime-api/group__CUDART__STREAM.html).

Do not retain a Python callback or issue callbacks from native worker threads in the first API. Cancellation of long-running GPU work is not promised. Benchmark fault cases execute in fresh subprocesses so a stuck or poisoned GPU process cannot trap the complete run.

## Status, exceptions and device failure

Return a fixed-width global status for the operation and a caller-owned diagnostic record with stage, query index where applicable, native CUDA error code and bounded UTF-8 message storage. Avoid a process-global last-error string. C++ implementation exceptions are caught at the C boundary; allocation failure maps to an allocation status and unexpected exceptions map to an internal-error status. No exception unwinds through `ctypes`.

Suggested status categories are `OK`, `INVALID_ARGUMENT`, `UNSUPPORTED`, `INVALID_HANDLE`, `OUT_OF_MEMORY`, `BACKEND_UNAVAILABLE`, `DEVICE_FAILURE`, `CONTEXT_UNUSABLE` and `INTERNAL_ERROR`. Per-query numerical statuses distinguish parameter-domain errors, an invalid denominator and non-finite output. Final spelling and numeric values belong in the owned header.

Python translates structural input errors to `ValueError`/`TypeError`, allocation failure to `MemoryError`, and backend/device failures to owned exceptions carrying the diagnostic fields. Device failure never returns partial arrays advertised as a successful batch. An explicit diagnostic result can retain per-query numerical failures when the kernel executed successfully; that is different from a failed runtime operation.

Build the CPU library without a link dependency on CUDA. Loading the package and using CPU mode succeeds on a machine with no NVIDIA driver. Requested CUDA mode reports a missing backend library, unavailable driver, unsupported device or allocation failure explicitly. Do not silently substitute CPU computation in a GPU benchmark. If a future `auto` mode exists, it reports its selected backend.

Resources use RAII internally. A failing constructor publishes no handle and releases allocations already acquired. Healthy-context cleanup releases each host/device allocation, stream and event once. If cleanup itself fails, preserve the original operation failure and record cleanup diagnostics rather than masking it.

CUDA documents illegal device addresses and some other launch errors as leaving the process inconsistent; continuing requires process termination and relaunch. Mark the CUDA backend unusable for the process, invalidate affected results, and require process restart for those error classes. Do not promise that opening another CUDA context repairs this state. Do not retry the same work automatically or call a device reset that destroys other users' CUDA state. Cleanup can be best-effort in that fatal state; the process restart is the honest GPU recovery boundary. [CUDA runtime error definitions](https://docs.nvidia.com/cuda/archive/12.8.0/cuda-runtime-api/group__CUDART__TYPES.html).

## Benchmark boundary and blocking decisions

The native CPU and CUDA paths should consume identical packed geometry/query arrays and emit the same jet/status layout. The [analytic/model oracle report](2026-10-13-geometry-oracles.md) supplies correctness independently of either backend. Binding selection does not qualify the math.

Report first-use time, Python packing/snapshot time, native validation/upload time, resident compute time, download time and host-to-host operation time separately. For resident compute, synchronize the owned stream, bracket the relevant kernel with CUDA events, and repeat evaluation without implicit upload/download. That event measurement is a component; show wrapper wall time for the corresponding synchronous call too. CPU native batching and Python end-to-end batching are separate comparisons.

Include the checks required to deliver valid results in product timing; only the independent benchmark oracle is outside it. Show batch sizes, output order, derivative order and bytes moved. An empty batch is a semantic case, not an arithmetic throughput result. Warmups, initialization and allocator reuse must be labeled, and no scalar Python baseline should be presented as a native CPU arithmetic competitor.

Before implementation, resolve these concrete decisions in the integrated design:

1. Resolved: isolated CUDA 12.8.1 components compile C++17 `sm_60` and run correctly on both P100s. Use the [qualified setup](../docs/CUDA_TOOLCHAIN.md), including its archive-layout `lib64` symlink.
2. Adopt the owned C ABI / `ctypes` proposal or explain the requirement for a direct CPython extension. pybind11 remains an unadopted library proposal.
3. Set the first patch family/degree range, rational input limits, derivative order, knot-side policy and per-query failure contract with the numerical owner.
4. Commit to the layout/ownership and synchronous resident/download contract before writing independent CPU/CUDA adapters.
5. Decide whether CUDA artifacts bundle or dynamically require the CUDA runtime. CPU operation must remain independent in either case; package redistribution conditions and actual loader qualification follow that selection.

## Primary-source bibliography

- Python Software Foundation. [ctypes, Python 3.12](https://docs.python.org/3.12/library/ctypes.html): declarations, borrowing, copying and GIL behavior.
- Python Software Foundation. [Buffer protocol, Python 3.12](https://docs.python.org/3.12/c-api/buffer.html): exporter ownership, format, shape, strides and release.
- Python Software Foundation. [C API stability](https://docs.python.org/3.12/c-api/stable.html) and [thread initialization API](https://docs.python.org/3.12/c-api/init.html): ABI and interpreter constraints.
- Python Software Foundation. [array](https://docs.python.org/3.12/library/array.html): native numeric storage and item widths.
- pybind11 maintainers. [Miscellaneous/GIL](https://pybind11.readthedocs.io/en/stable/advanced/misc.html) and [buffer/NumPy support](https://pybind11.readthedocs.io/en/stable/advanced/pycpp/numpy.html): capabilities and ownership limitations; reference study only.
- NVIDIA. [CUDA architecture/driver matrix](https://docs.nvidia.com/datacenter/tesla/drivers/latest/cuda-toolkit-driver-and-architecture-matrix.html): Pascal support limits.
- NVIDIA. [CUDA 12.8 Linux installation guide](https://docs.nvidia.com/cuda/archive/12.8.0/cuda-installation-guide-linux/index.html): compiler support.
- NVIDIA. [CUDA 12.9 compiler driver](https://docs.nvidia.com/cuda/archive/12.9.1/cuda-compiler-driver-nvcc/index.html): virtual/native architecture selection.
- NVIDIA. [CUDA 12.8 runtime synchronization](https://docs.nvidia.com/cuda/archive/12.8.0/cuda-runtime-api/api-sync-behavior.html), [stream API](https://docs.nvidia.com/cuda/archive/12.8.0/cuda-runtime-api/group__CUDART__STREAM.html), [error types](https://docs.nvidia.com/cuda/archive/12.8.0/cuda-runtime-api/group__CUDART__TYPES.html): completion and fatal-state behavior.
- NVIDIA USD BRep. [SmPyTypes.cpp](https://github.com/NVIDIA-Omniverse/usd-brep/blob/main/source/SmPyLib/src/SmPyTypes.cpp), [SmPyBorrowedGeometry.cpp](https://github.com/NVIDIA-Omniverse/usd-brep/blob/main/source/SmPyLib/src/SmPyBorrowedGeometry.cpp): inspected scalar API and mutable borrowed-object behavior.

Documentation was accessed on 2026-10-13. Inventory/setup figures are direct observations, while ABI/lifecycle decisions are proposals. CUDA setup probes are qualified; no CAD GPU speedup or binding-performance superiority has been established.
