# Iteration 1 runtime and Python code review

Reviewed the initial implementation of `native/runtime.cpp`, `python/brepgpu/__init__.py`, `include/brepgpu.h` and `native/backend_api.h` against the accepted iteration 1 design and plan. This is an adversarial source review of publication, ownership, cleanup, thread admission and fatal GPU behavior. Target execution and the independent exact geometry corpus belong to the integration and qualification lanes; successful source inspection does not replace them.

The product files were uncommitted working-tree additions above `8481002`. Reviewed runtime SHA-256: `b747b198d4580a9aacf5908e8a048a1846c4e49deead1af9e721abeca36d135d`; reviewed Python adapter SHA-256: `1a2dc29156eb78f454103c910833231158f4d4afff4f86226a76049e243a5314`. These identify the source under review while independent owners continue integrating it.

## Findings

### R-N1 — partial control records receive different CPU/CUDA outcomes

**Moderate; resolve before qualification.** `bg1_create_patch_set` checks referenced control ranges but does not require `control_doubles` to be divisible by four. The private CUDA `patch_create` requires `doubles % 4 == 0` for a nonempty patch set. A degree `(0,0)` descriptor with offset zero/count one and the array `(0,0,0,1,99)` therefore succeeds on CPU but fails on CUDA. The public input contract describes homogeneous four-double records, and selecting the backend should not alter this structural contract.

Require complete records once in the core before snapshot/upload. This also covers an empty patch set carrying incomplete unused storage; deciding this at the shared boundary avoids dependence on whether a private backend happens to execute a descriptor loop. Unused *complete* records can remain accepted if that is intended. The direct raw C oracle supplies five valid allocated doubles and one constant patch, checks `BG_INVALID_ARGUMENT`, zero output handle and a coherent diagnostic on both selected backends, then verifies a four-double input still evaluates to the exact constant jet. No forged pointer or numerical parity oracle is needed.

### R-N2 — a later fatal cleanup cause disappears from the returned diagnostic

**Moderate; resolve before fault qualification.** `Context::record` stores the first nonzero cleanup diagnostic and calls `observe` for later diagnostics, but does not merge their flags or messages. If one child close fails nonfatally and a later child or context close fails fatally, process poison is set correctly while `bg1_close` returns the earlier diagnostic with no fatal flag. The caller loses the fact that continued CUDA use requires a process restart, and loses the secondary cleanup cause.

Preserve the first code, stage and backend code, OR fatal flags from every subsequent cleanup failure, and append a bounded secondary message. The CUDA backend's `Operation` accumulator already follows this pattern. The direct fault oracle uses a conforming test backend whose first actual cleanup fails nonfatally and its second actual cleanup fails with `BG_DIAG_FATAL`; context close must return a nonzero status carrying that flag, preserve the primary error, and reject later CUDA creation/evaluation with `BG_CONTEXT_UNUSABLE`. Run this in its own process because poison is intentionally permanent. This attacks error aggregation, not a fabricated geometry algorithm.

## Paths checked without an additional finding

Registry lookup retains an object under the short registry lock, releases that lock before waiting for the context mutex, then rechecks the public ID and context state. Publication and revalidation take the registry lock only while the context lock is already held. Close removes public IDs while locked and releases removed references outside the registry lock. This matches the required lock order and prevents a lookup that waited behind close from admitting stale work.

Patch, query and result objects retain immutable dependencies. Closing public parents does not destroy storage needed by live descendants. A context holds child IDs rather than strong child pointers, avoiding a native reference cycle; the private plugin/context remains available through the last device-token cleanup. Context close waits for already admitted work and revokes child IDs. A thread that retained an object during lookup but has not passed admission can extend its private allocation lifetime briefly; it must subsequently reject the revoked ID. This retention is consistent with the written ownership model and is not successful operation admission.

The creator/evaluator paths initialize output handles to zero, validate owned host snapshots, and publish only after the private callback has completed and after the global poison check. The owned CUDA callbacks publish no tokens on failure, so child accounting increments only after a successful private token. Partial CPU worker startup joins already-created workers before releasing their result buffers. Worker ranges preserve the full query count as the SoA stride. Empty query evaluation avoids the unused partition-division path.

Rejected downloads check exact capacities, alignment and mutual output overlap before writing numeric payloads. Raw callers remain responsible for valid storage and the stated separation between diagnostic/descriptor/handle storage and payload arrays. Numeric writes on a later CUDA transfer/restoration failure are not published as usable Python results: the wrapper raises before constructing `JetBatch`.

The Python adapter copies incoming controls, slots and parameter values to private arrays before GIL-released calls and keeps those owners alive through native completion. Read-only and strided logical inputs use the copy path. Results download to fresh owned arrays; separate native results have separate allocations. Downloaded batches retain host provenance and the loaded core independently of public patch/query IDs, so named conversion can operate after those parents close. Conversion copies mutable downloaded numeric arrays before its native call and rejects repeated world/source conversion. Per-query failures require explicit inspection or `require_ok`; they are not silently raised as whole-batch native failures.

Explicit Python close tolerates already revoked native IDs. Context-manager cleanup adds a note to an existing exception instead of replacing it. The current wrapper removes the initial Engine self-reference by assigning its `_engine` to `None` and using its own `_lib` for close. Finalizers skip interpreter teardown; explicit close remains the supported resource-management path.

Fatal operation diagnostics are observed process-wide before failure reaches the caller. Subsequent CUDA admissions and publications reject poisoned state; CPU contexts remain available. No device reset or automatic retry exists. The review found no additional fatal-state publication gap in the ordinary operation paths; R-N2 concerns the diagnostic returned by cleanup.

## Remaining execution evidence

The independent qualification lane must still run all orders and degree pairs against exact rational truth, on CPU and each P100. Runtime tests must exercise empty sets, retained sources after parent close, stale/wrong-kind/foreign-context IDs, snapshot mutation, rejected-download sentinels, actual cross-thread device restoration, controlled close/admission interleavings and partial failure cleanup. Fault injection should target the real private backend/publication boundaries and stay outside production builds. Linux sanitizer and CUDA memory checks remain integration work; a local sanitizer initialization stall supplies no qualification result.

The first manual target build and native dyadic benchmark were reported successful by the integration lane. Its provisional single timing sample is not a performance claim and does not replace the exact-oracle or runtime fault tests.

## Disposition

R-N1 and R-N2 were sent to the runtime owner and integrator. Re-reviewed corrected runtime source SHA-256 `356c365b90227ffc1f9d517de8c358c031efa6aa7aced8a8a8a5b3cc9b67b3f2`:

- **R-N1 resolved in source:** the shared core rejects `doubles % 4` before admission, snapshot or upload, including the empty-descriptor case.
- **R-N2 resolved in source:** cleanup retains the primary diagnostic identity, ORs subsequent flags and appends terminating bounded secondary text. A callback returning success with a fatal flag is also normalized to device failure and observed. The runtime owner is adding the direct nonfatal-child/fatal-context cleanup fault case; its execution result remains separate evidence.

No remaining source blocker was found in this review. No numerical source, runtime source or Python source was changed by this review. The execution evidence listed above remains required before claiming qualification.

## Final composed publication/cleanup failure re-review

The integration review identified a further composed failure: registry publication can throw after the private backend successfully allocates a token, and destroying that unpublished token can then report a fatal device failure. Ordinary stack unwinding cleaned the token and poisoned CUDA, but the outward allocation exception could lose the cleanup fatal flag. The runtime owner added `publish_owned` for context, patch, query and result publication and a sixth guarded callback-model scenario, `publication-oom-fatal-cleanup`.

Independently inspected the helper and `tests/native_runtime_faults.cpp` / `tests/runtime_fault_backend.cpp`. **No blocker found.** `publish_owned` first captures the original exception, consumes the unpublished object, then reconstructs the original failure identity and merges cleanup flags and bounded text before throwing the combined diagnostic. An original allocation failure therefore remains `BG_OUT_OF_MEMORY`; fatal cleanup is also exposed and has already poisoned the process through `Context::record`.

Reviewed final source SHA-256 values: runtime `ea936896dadfaec510bec3172e272f98935c68db1e09b720affa0505bd4e846d`; direct fault caller `6f4e172027eb7df6b9f0647da3c1515e660131d0b8ec868ff247a169e8068c15`; callback model `a9301f63dfb93d441596bc37ef88b01c2eba07fb4269f523b0fd191e5b911ad4`.

For context creation, `publish_owned(c,c)` passes a reference to the caller's shared pointer and a distinct by-value retained context pointer. Marking that retained context closed and calling `finish_backend` consumes its private token while the plugin is still alive. Resetting the caller pointer cannot destroy the retained context prematurely, and its later destructor finds the token already null. No child token exists on this path.

For patch/query/result creation, the admission object continues holding the context and operation lock. The unpublished object's reset invokes cleanup through the recursive context mutex, consumes its one private token and decrements device-child accounting. Retained patch/query dependencies and the plugin remain alive throughout the relevant callback. The context remains open for a nonfatal publication failure; a fatal cleanup instead makes subsequent CUDA admission unusable. The temporary base-class ownership used by `publish` is unwound before the helper's catch, and a failed child-ID insertion removes the registry entry before cleanup, so it does not retain a hidden unpublished object.

The sixth direct oracle successfully models the intended boundary in source: allocate an actual private result token, throw `std::bad_alloc` at the registry-publication hook, then consume the result token in a callback that reports `BG_DIAG_FATAL`. It asserts the outward OOM code, zero public handle, fatal flag and cleanup text, no live unpublished result token, rejection of further CUDA evaluation, retained fatal history on context close, complete private-token drainage and continued CPU operation. It runs as a separate process and labels itself an owned callback model rather than hardware fault injection. The hooks are guarded by the test-build macro; they do not modify the numerical evaluator.

This source review does not claim the sixth scenario has executed. The integrator will run it against the earlier core for the direct red result and against the corrected sanitizer build for green. Earlier five sanitizer scenarios were reported passing by integration. The final product geometry and device matrix are likewise execution evidence owned by integration.

One nonblocking diagnostic detail remains: the helper merges the context's sticky cleanup history, so a previous nonfatal cleanup error can accompany a later publication failure even if this particular unpublished cleanup succeeds. That history is retained intentionally by the runtime; calling the appended text “context cleanup history” would make its scope clearer. It does not affect token lifetime, failure identity, poison or output publication.
