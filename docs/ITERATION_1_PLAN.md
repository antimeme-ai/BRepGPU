# Iteration 1 implementation plan

2026-10-13. Reviewed plan for the [reviewed design](ITERATION_1_DESIGN.md). The delivery is a native CPU/CUDA rational
patch evaluator, Python batch caller, qualified carrier sampler and measured
CPU/GPU crossover. It does not accelerate an existing kernel's Boolean calls.

## Conceptual owners and file boundaries

| Unit | Owner | Files |
| --- | --- | --- |
| Runtime and language boundary | Runtime lane | `include/brepgpu.h`, `native/backend_api.h`, `native/runtime.cpp`, `python/brepgpu/__init__.py`, root `CMakeLists.txt` |
| Numerical evaluator and CUDA backend | Numerical lane | `native/evaluator.hpp`, `native/evaluator_cpu.cpp`, `native/cuda_backend.cu` |
| Independent fixtures and conformance | Oracle/caller lane | `python/brepgpu/fixtures.py`, `tests/test_qualification.py`, `tests/test_runtime.py`, `scripts/qualify_iteration1.py` |
| Native measurements and sampler | Integrator | `benchmarks/iteration1/benchmark.cpp`, `examples/sample_patch.py`, execution/measurement scripts, published results/docs |

The runtime owner publishes the canonical headers first after plan review.
Dependent owners use them; they do not invent parallel layouts. Each owner
implements and reviews one complete invariant. Reviews then cross these owner
boundaries. Root integrates changes and every valid finding. No adopted binding,
CAD, numerical or test framework dependency is needed.

## Concrete interface decisions

The public header is C-compatible. It defines fixed-width status enums, a
`uint64_t` handle, versioned byte-sized descriptors/diagnostics, and an ABI info
operation exposing version and descriptor sizes. Handle zero is invalid.
Initial support is Linux x86-64/native-endian binary64 and CPython 3.12.

| Interface | Decision |
| --- | --- |
| Context | Explicit CPU or CUDA backend, device ordinal, CPU thread count, optional profiling flag and optional CUDA plugin path |
| CPU threads | Default one; explicit bounded positive count for multicore comparisons; per-call creation/join cost belongs in public timing |
| Patch descriptor | `degree_u`, `degree_v`, homogeneous control offset/count, full body/kind/entity/revision lineage, chart ID, source UV rectangle and optional uniform world frame |
| Patch controls | Flat binary64 `(Ax,Ay,Az,W)` records, `i*(q+1)+j`; control offsets/counts are in records, not doubles |
| Query input | `uint32` patch slots and interleaved binary64 `(s,t)`, exact counts; query creation binds one immutable patch set |
| Result | Count/order/term metadata, profiling information, retained query/patch lineage; component-major binary64 jets and one `uint32` status per request |
| Error | Global code, stage, optional query index, native CUDA code and bounded message in caller-owned versioned storage |
| Conversion | A separately named native local-to-world/source-UV helper, uniform scale and bounded widths/origins from the reviewed design; no inferred nonlinear chart map |

Creation and evaluation initialize output handles to zero before any failure.
The ABI owns `create_context`, `create_patch_set`, `create_query_set`, `evaluate`,
`result_info`, `download`, `close`, and the named conversion operation. Functions
also have an unambiguous library prefix and ABI version. Exact field sizes and
symbol spelling are settled in the canonical header before consumers are coded.

Python exposes immutable `Patch` and `Lineage` values; `Engine(backend, device,
threads, profile)`; `create_patch_set`; `create_query_set(patches, patch_slots,
uv)`; `evaluate(queries, order)`; and `Result.download()` returning `JetBatch`.
`JetBatch` exposes `count`, `order`, `statuses`, component-major `data`, a vector
accessor, lineage and `require_ok()`. A named conversion method preserves status
and provenance. Exceptions expose the global code/stage/CUDA diagnostic.
All resource wrappers support explicit close and context-manager cleanup.

Raw ABI tests can use the module's declared low-level ctypes functions and
wrapper handles without adding a production diagnostic backdoor. Do not make
arbitrary external pointer values safe: raw callers must provide valid buffers
of their declared lengths, and tests use real allocated buffers.

The internal backend table has its own version, byte size and factory symbol.
It accepts canonical host descriptors/arrays and returns private opaque context,
patch, query and result tokens; it exports create/upload/evaluate/download/close
operations. Cleanup/close callbacks return status and diagnostics so restoration
and cleanup errors can be recorded without crossing an exception boundary.
The core owns public IDs/registry and process poison state. The
plugin owns CUDA allocations/stream/events and reports classified errors to the
core. Private tokens and function pointers do not become public handles or
geometric identity. Retain the plugin/private context through child cleanup.

Numerical CPU evaluation accepts canonical descriptors, controls, query arrays,
the full query count as the component-major stride, a disjoint `[begin,end)`
request range, order and caller-owned output/status storage. CPU workers never
reinterpret a chunk length as the full result stride. If thread construction
fails partway through, join every already-created worker before discarding the
unpublished result; do not leave joinable threads or release their output memory. CUDA uses one request/thread with
a grid-stride loop, checks the slot before reading patch memory, and streams
homogeneous channels to bound scratch. It computes only requested jet terms.

## Execution sequence

1. **Plan review and direct red oracles.** Review this plan in numerical,
   runtime and independent-caller lanes; resolve concrete findings. Define exact
   rational/analytic expectations, API lifetime cases and measurement regions.
   The first invocation must fail because the evaluator/library is absent,
   not be silently skipped. Save this outcome locally in ignored context.
2. **Canonical header and CPU whole unit.** Publish headers, then implement
   native owned snapshots, registry/admission, CPU Bernstein jets, exceptions,
   the Python batch wrapper and local-to-source conversion. A CPU-only build
   must configure, load and run with no CUDA plugin/runtime available.
3. **CUDA whole unit.** Implement bounded tensor de Casteljau jets, private
   backend resource ownership, thread-device save/select/restore, explicit
   completion, failure cleanup and shared-runtime loading. Integrate the sole
   core registry with the lazy versioned plugin. Build actual `sm_60` using the
   installed isolated toolkit.
4. **Qualify and attack lifetimes.** Run the same exact/analytic geometry corpus
   on CPU and each P100, exercise all requested grades and runtime states, then
   host/device memory checking. Keep numerical failures and bad inputs visible.
   Review code across conceptual boundaries and resolve every valid finding.
5. **Deliver measurements and sampler.** Build the native benchmark and Python
   carrier sampler. Qualify every timed workload against independent truth,
   measure actual public/native regions, and publish the material output and
   reproducible commands. Re-review newly changed code and rerun affected checks.
6. **Publish the complete iteration.** Update README/design status, issue #1,
   journal and results with actual capabilities/limits. Push owned source and
   public-safe evidence. Keep driver/toolkit archives, private logs and binaries
   ignored. Leave later query/topology work tracked separately.

Steps 2–3 can overlap once canonical headers and red expectations exist;
qualification and publication depend on integration. No optional reference
adapter is required to pretend success: owned analytic carriers supply the
first useful batch sampler. The SMLib adapter remains a separate experiment.

## Numerical implementation and direct oracles

CPU uses directly evaluated Bernstein basis derivatives for degrees 0–3;
CUDA uses tensor de Casteljau with degree-scaled difference nets. Both apply the
explicit homogeneous quotient identities, direct division, binary64 arithmetic
and requested-order semantics. Compiler flags disable unsafe math and implicit
contraction while allowing the explicit GPU two-FMA interpolation. Every failed
request gets NaN in every requested numerical field.

The oracle uses Python standard-library `Fraction` over actual binary64 input
ratios, exact Bernstein basis/derivative sums and independently derived analytic
polynomial/cylinder/bilinear/singular fixtures. All 16 degree pairs and orders
0/1/2, endpoints/inward neighbors, weight extrema/nonuniform data, cancellation,
mixed/shuffled/tail batches and empty batches are explicit cases. Position,
first and second terms independently meet `2^-40`, `2^-30`, `2^-20` maximum
absolute-error targets; the highest requested order never weakens lower grades.
Record maxima and offending inputs instead of averages alone.

Metamorphic operations stay inside the declared input envelope or expect a
structural unsupported outcome. Conversion tests include exponent-boundary
accepted transforms, the refused `2^600` witness and source-derivative scaling
with exact expected values. No finite differences or CPU parity serve as the
only truth for a derivative.

## Runtime implementation and fault oracles

The registry lookup promotes ownership, releases the registry lock, obtains
the context lock and rechecks OPEN/public validity. Context tracks child IDs,
not strong child pointers. Retained immutable dependencies keep allocation
storage alive after public parent handles close. Revoke IDs while locked;
release removed owning references outside locks so destructors cannot deadlock
while reacquiring the context lock. Private backend contexts remain alive through
the last child cleanup even after logical context close.

Validate/upload the same private native snapshot. The ordinary Python wrapper
copies to private arrays before a GIL-released call. Query slots are bound to
one patch set and result lineage is retained. Test wrong-kind, foreign/stale
handles, parent closure, same-sized different patch sets, sentinel-preserving
rejected downloads, misalignment, version/size mismatch, zero payloads and
overflow lengths using valid allocated memory.

Admission/close tests use controlled concurrent interleavings where practical.
Failure injection, if required, is test-build-only and targets a real allocation,
copy, completion or admission boundary; no hook is compiled into the default
production artifact. It must not change the numerical algorithm or turn tests
into a second receipt system. Fault tests use fresh processes for fatal state.

Every CUDA operation selects its context device on its calling host thread and
restores the prior device before output publication. Exercise both GPU ordinals
from different host threads, including a different current device. Successful
publication follows completion, restoration and final poison-state checking.
Fatal device errors poison CUDA use process-wide; no reset/automatic retry.
CPU can remain usable. Download rejection writes nothing; a runtime-copy failure
can leave an unusable partial destination and publishes no successful host arrays.

Use host sanitizers for actual invalid access/undefined behavior and the already
qualified Compute Sanitizer on both P100s. They attack memory faults independently
of the analytic geometry checks. Mutants, if used to attack sign/order/quotient
faults, run only on the fleet and only where they add evidence beyond direct
oracle failures; no mandatory mutation quota or coverage gate is introduced.

## Build and execution host

Root copies only active owned source into a new isolated checkout under the
execution host's `BRepGPU` directory, preserving installed `toolchains/` and
ignored `context/`. Read the host's active instructions before deployment.
Keep deployment confined to this project's owned source and build directories.
No Git repository is initialized in a parent workspace or an original archive.

Use CMake CPU-only and CPU+CUDA builds with C++17, normal portable x86-64 host
code, `CMAKE_CUDA_ARCHITECTURES=60-real`, shared cudart and explicit runtime
search path to the isolated toolkit. Native Threads/dl facilities and installed
compiler/SDK are platform tools; no new CAD/binding/test dependency is installed.
The CPU library has no CUDA loader dependency, even when CUDA is built separately.

Save private compiler/sanitizer/fault logs under ignored context. Published
metadata includes device model, compiler/runtime/driver, precision flags,
CPU model/thread settings, selected ordinals, source revision and timing scopes;
remove private hostnames/IPs/absolute operator paths.

## Measurement and visible deliverable

Native benchmark cases cover position/first/full jets; coherent and shuffled
queries on one/many patches; batch sizes 1,32,256,4096,65536,1048576 where memory
permits. Use one excluded warmup and seven measured samples initially. Report
native CPU one-thread and selected multicore thread counts separately, plus each
P100. No scalar Python reference timing is called native CPU arithmetic.

Public resident-input calls allocate distinct new results: allocation, required
checks, launch, completion and publication remain timed. Download is timed
separately; prepared host-to-host calls include input creation/upload, evaluate,
download and required result checking, with cleanup reported separately. First
use includes context/plugin initialization. Retain result A across a later call
and check it is unchanged. Memory reports include retained input dependencies.

Optional backend profiling uses CUDA events only when explicitly requested.
Normal public timings use profiling off. Event/kernel components come from
separately labeled profiling runs and include their measurement overhead there;
they do not substitute for unprofiled public wall time. A native CPU component
comparison has equivalent requested fields/prepared output buffers. Publish
raw TSV samples, qualification counts and min/median/max or percentiles adequate
for the sample count. Do not inflate confidence from seven shared-host samples.

The Python demonstration samples owned polynomial and rational-cylinder carriers
in batches and exports a point cloud/grid artifact with source/query lineage.
It labels the local/model coordinate convention, and never calls that grid a
trim-aware or chord-tolerance tessellation. Publish an actual sampler output
alongside commands and performance results so the iteration has a caller.

## Completion

This iteration is done when CPU-only loading, native/Python batch use, both-P100
qualification, meaningful failure/lifetime cases and appropriate memory checks
pass; code review findings are resolved; a sampler output and fair native/public
measurements are published; and the next consumer remains clear. If measurements
show no useful GPU crossover, publish that outcome rather than a manufactured
speedup. Product implementation begins only after this plan's review dispositions
are recorded below or in linked `papers/` reviews.

## Plan review disposition

Numerical, runtime and independent-caller reviewers found no blocking plan
findings. The full-result-stride CPU range, partial-thread-start cleanup and
diagnostic cleanup callbacks are explicit above. The runtime owner publishes
canonical declarations before dependent callers. Library/backend paths and
Python accessor/exception spellings are communicated at that handoff; no second
parallel type model is invented. Direct red oracles precede the associated
algorithm unit; an absent library/module is recorded plainly, never skipped.

[Runtime review](../papers/2026-10-13-iteration-1-plan-runtime-review.md) ·
[Numerical review](../papers/2026-10-13-iteration-1-plan-numerical-review.md) ·
[Caller review](../papers/2026-10-13-iteration-1-plan-caller-review.md).
