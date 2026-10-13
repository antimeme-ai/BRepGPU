# Iteration 1 runtime boundary: adversarial review

Review date: 2026-10-13. Scope: [iteration design](../docs/ITERATION_1_DESIGN.md), [roadmap](../docs/ACCELERATION_ROADMAP.md) and [topology identities](../docs/TOPOLOGY.md), restricted to the complete ownership/status/concurrency boundary. No product implementation exists to inspect; findings are design contradictions or missing decisions, not reproduced implementation bugs.

Disposition of the initial draft: **resolve the operation/device/measurement contracts below before implementation**. The basic architecture is sound: CPU independence, optional CUDA, checked handles, bound immutable queries, retained result lineage, synchronous completion and explicit downloads support a narrow useful delivery. The [qualified CUDA setup](../docs/CUDA_TOOLCHAIN.md) removes the compiler blocker; it does not remove these runtime decisions.

## R1 — Resident output reuse conflicts with immutable result lifetime

The API publishes a new owned result handle from every `evaluate`, while the benchmark text says resident re-runs use prepared buffers and separates allocation from resident evaluation. Geometry/query residency is established, but output reuse is not.

Concrete counterexample: retain `result_a`, evaluate again into a supposedly reused output buffer and obtain `result_b`; the second kernel overwrites `result_a`'s data. Avoiding that corruption by allocating a new buffer makes the public resident call include allocation after all.

Required decision: for iteration 1, preferably allocate a distinct result for each evaluation and include its allocation/status-validation/completion in resident **API wall time**. Call the geometry and queries resident, not all outputs preallocated. Report kernel event time separately. A benchmark-only preallocated native kernel measurement may be useful, but must be labeled as a component and must not claim to time the public new-result API. A future reusable output/workspace API needs a separate lifetime contract; do not introduce it implicitly in the benchmark.

Direct oracle: retain two results from different queries/orders and verify that the first result's downloaded values remain unchanged. Count newly allocated result bytes in the corresponding operation record.

## R2 — Download failure lacks an operation-specific publication rule

The failure table says allocation/launch/copy/completion failures publish no result handle, and the following paragraph says no result is usable on batch failure. That describes create/evaluate, but a download starts with an already published resident result.

Required decision:

- A failing creator/evaluate initializes its output handle to invalid and publishes nothing; it preserves pre-existing inputs/results unless the runtime is fatally poisoned.
- Download rejects incompatible destination length/format/alignment before writing any bytes. That rejection preserves the resident result.
- A copy/completion failure can leave a host destination partially written. The caller receives a failure and must discard that destination; no atomic host-buffer update is promised unless additional staging is deliberately implemented and timed.
- Preserve the resident result after an ordinary rejected download or healthy-context host allocation failure. For actual runtime failures, determine its continued validity from the runtime error class. Fatal device errors invalidate CUDA use for the process.

Direct oracle: an undersized destination retains its sentinel contents, and a subsequent correct download of the same result succeeds. A modeled copy failure checks that no successful host output is offered and the reported resident-result state follows the chosen error class.

## R3 — Device binding must happen on every host thread

Choosing a GPU when creating a context does not bind later Python/native calling threads to it. CUDA's selected device belongs to the calling host thread; allocations/launches follow that selection, and a launch into another device's stream fails. [CUDA 12.8.1 device selection and stream behavior](https://docs.nvidia.com/cuda/archive/12.8.1/cuda-c-programming-guide/index.html#device-selection).

Concrete counterexample: create a GPU-1 context on thread A; thread B, whose current device is GPU 0, evaluates it. Its mutex serializes work but does not select GPU 1. The resulting stream/device mismatch can fail before the geometry kernel does any useful work.

Required decision: while holding the context's operation lease/lock, explicitly set and check its device before every CUDA creation/upload/evaluate/download/resource-cleanup path. Define whether to restore the caller's previous device afterward; restoration failures must not silently turn a failed operation into success. If the first implementation leaves the selected device current, document that side effect. Device-reset recovery remains prohibited because the CUDA primary context is shared with other users in the process.

Direct oracle: use both GPU ordinals and call each context from threads with a deliberately different current device. Check actual results/device identity, not only a successful status.

## R4 — Close needs an admission and lock-order rule

The draft correctly says lookup retains native ownership and context close prevents new calls. It does not specify whether a call waiting for the operation mutex has been admitted, or where it rechecks closure.

Concrete interleaving:

1. A promotes a query/context from the registry and pauses before obtaining its operation lock.
2. B closes the context, invalidates children and destroys the stream.
3. A obtains the operation lock and executes using its retained object, whose memory exists but whose runtime resources are closed.

Required decision: separate retained host ownership from permission to operate. One simple protocol is registry lookup/promote, release registry lock, acquire context operation lock, then recheck context OPEN and public-handle validity before any access/launch. Keep the operation lock through completion and output-handle publication. Close takes the same operation lock, marks the context closed, revokes handles and frees resources in a defined order. Never wait for an operation lock while holding the registry lock. An alternative explicit admission-count protocol is acceptable, but the design must choose one.

This also applies to patch/query/result close. Existing dependent objects may retain allocations after the public handle is closed, but a new operation through that closed public handle cannot become authorized by its retained pointer alone. Avoid native context/child ownership cycles: the registry and dependency graph need a clear destruction direction.

Direct oracle: deterministic interleavings of lookup, close, lock acquisition and publication produce either an admitted operation followed by closure, or a closed-handle/context error; no use of a freed stream and no handle published after closure has won admission.

## R5 — State which Python entry point owns the snapshot

The draft's raw-buffer no-mutation contract is a legitimate native caller precondition, and retaining argument objects avoids deallocation. It does not prevent another Python thread from changing an `array` while `ctypes.CDLL` has released the GIL. [Python ctypes documentation](https://docs.python.org/3.12/library/ctypes.html).

Required decision: the ordinary Python entry point first packs/copies into private wrapper-owned arrays, keeps their exported views alive, and calls native code with those private buffers. Native code copies once into its owned immutable snapshot, then validates and uploads **that same snapshot**. This avoids validating one image and uploading another. Caller-side mutation while initial packing occurs remains outside the snapshot construction contract, particularly for external native writers.

An advanced borrowing entry point can defer that copy under an explicit no-mutation/no-resize/no-alias precondition. It must be named/labeled and have its copy/packing costs reported separately. A direct C caller must provide valid stable input memory for the entire synchronous call. Neither path can make forged raw C pointers safe.

Direct oracle: after the ordinary wrapper has taken its snapshot, mutating the original caller array cannot change the created native patch/query values. Do not pretend that mutating an explicitly borrowed buffer in violation of its contract is a conformance test.

## R6 — ABI and optional plugin compatibility need a concrete boundary

Exporting version information does not by itself prevent a layout mismatch. The draft accepts POD data and lengths but should name the first platform/width/alignment contract and require compatibility before interpreting descriptors.

Required decision: initial Linux x86-64/native-endian binary64 support; fixed-width handles, counts, slots/statuses and explicit descriptor/diagnostic sizes and ABI versions. Check short buffers, multiplication/addition overflow and alignment before typed access. A zero-element payload can have a null pointer and requires an explicit branch avoiding null-pointer arithmetic/copies/launches. Query indices outside the patch range must become statuses before any control-net dereference. Finite arithmetic failure should consistently make every requested numeric field NaN for that failed query, with status authoritative.

The CPU core should own the runtime handle namespace/registry; the CUDA module supplies backend operations for those owned objects. If separate modules maintain separate registries with identical integer IDs, a foreign handle can accidentally identify a valid local object. Either prevent that structure or include an unambiguous library namespace. Reject CUDA plugin ABI/capability mismatch before publishing a CUDA context or accepting payloads. Keep the plugin loaded while any of its objects or destructors remain live.

For CPU-only loading, check actual link/import behavior when the CUDA plugin or its runtime dependencies are missing. Importing or using CPU mode must not first load CUDA via a global initializer or eager capability query. Requested CUDA failure is explicit; it is not a reason to substitute a CPU implementation silently.

Direct oracle: mismatched plugin ABI, stale/wrong-kind handles, null/empty payloads, deliberately misaligned buffers, extreme counts, non-native formats and missing optional CUDA dependencies either fail before dispatch or behave exactly as the declared empty case. Native memory tooling attacks invalid reads/leaks independently of geometry oracles.

## Fatal state and diagnostics

The initial draft already correctly rejects automatic reset/retry after illegal device addresses and marks the CUDA backend unusable for the process. Preserve that rule through concurrent contexts: poison publication must be shared, and no in-flight call may publish a successful result after it observes the fatal state. CPU-only work can remain available, but it does not repair the CUDA runtime. [CUDA runtime error definitions](https://docs.nvidia.com/cuda/archive/12.8.0/cuda-runtime-api/group__CUDART__TYPES.html).

Use caller-owned bounded diagnostics rather than global last-error storage. Specify behavior for absent/short diagnostic buffers without throwing a second exception. Clean up partial allocations without replacing the original failure by a destructor error. Do not run CUDA destructors from unmanaged interpreter/global shutdown after CUDA runtime teardown; explicit close and retained library/context ownership are the normal path, with finalization best-effort.

## Scope and retained decisions

The topology document's distinction among entity IDs, snapshot slots and runtime handles is retained. Binding a query set to one immutable patch set and retaining that lineage in the result closes the same-sized-set rebinding fault. Closing public parent handles while dependent objects retain storage is also coherent once R4 defines operation permission. No topology mutation or general B-rep validity is promised by the first evaluator.

This review recommends operation-specific lifecycle/error tests, direct geometric oracles, and native/device memory tooling because they attack different fault classes. It does not add a second system to certify the first system's test records. Numerical target derivations and adapter geometry were outside this runtime review.

The design owner received R1–R3 immediately and the ownership/ABI clarification set before this report. A subsequent review should inspect the revised contracts; this report's initial findings do not automatically describe the final design.

## Integrated re-review

Re-read the revised iteration design and roadmap on 2026-10-13 after the owner integrated the runtime findings. **R1–R6 are resolved at design level; no remaining runtime architecture blocker was found.** This disposition authorizes progression to implementation planning/review, not an assertion that unimplemented code already conforms.

| Finding | Revised contract | Disposition |
| --- | --- | --- |
| R1 output reuse | Public resident-input calls allocate distinct results and include allocation/checks/completion/publication; retained results are unchanged; private preallocated kernel work is labeled separately | Resolved |
| R2 download failure | Rejected destinations are untouched and preserve resident state; post-validation failures make the destination unusable; healthy/fatal runtime states determine resident validity | Resolved |
| R3 device/thread binding | Every CUDA entry saves/selects/restores the calling thread's device, reports restoration failure and uses the owned stream | Resolved |
| R4 close races | Lookup promotes ownership, releases registry lock, obtains context lock and rechecks OPEN/public-handle validity; close shares that ordering; context stores child IDs rather than strong child pointers | Resolved |
| R5 snapshot consistency | High-level Python copies privately; native structural checks precede one owned snapshot, whose bytes are validated and uploaded; raw C borrowing retains its explicit precondition | Resolved |
| R6 ABI/plugin | Linux x86-64/binary64/fixed widths and version/size/alignment checks; one CPU registry; internal versioned CUDA factory; plugin lifetime retained; lazy shared runtime loading independent of CPU import | Resolved |

The revised fatal-state and result-publication policy also serializes process-wide poison with CUDA success publication. Per-query statuses remain resident until an explicit consumer/download, and every failed query's requested fields are NaN. This keeps resident timing from depending on an implicit status download and preserves the distinction between batch runtime failures and numerical query failures.

One implementation ordering detail follows directly from the revised contracts: successful creator/evaluator output publication must occur **after** stream completion and device-restoration success, or any restoration failure must revoke an unpublished tentative output before returning. A result cannot escape first and then accompany a batch-error return that promised output handle zero. This detail was sent to the design owner; it should be explicit in the reviewed plan. Similarly, the CPU/CUDA cleanup path must follow the selected context lock/lifetime protocol instead of relying on the Python GIL.

The exact measured setup probe and CMake files were copied into `scripts/toolchain-probe/` with matching source hashes. The [setup document](../docs/CUDA_TOOLCHAIN.md) now names checkout-relative source paths and ignored output paths, so its commands no longer depend on an unpublished probe file. No probe implementation was changed and no redundant runtime tests were rerun.

The design now explicitly places successful handle/host-output publication after stream completion, device restoration and the final poison-state check. The ordering clarification is integrated into the design as well as retained for the implementation plan.
