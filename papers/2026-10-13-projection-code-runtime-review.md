# Resident projection: runtime and Python code review

Adversarially reviewed the additive public/private projection headers, `native/runtime.cpp`, `python/brepgpu/__init__.py`, projection callback-model tests and associated CMake integration against the accepted design/plan. Numerical implementation is reviewed independently by the runtime owner; this report owns the runtime/Python/callback boundary. No runtime-owned product files were changed by this reviewer.

Initial reviewed runtime SHA-256: `a60dfad17b363b8185ad7771a471289f1b6cdb9057d5be201ab8f5bd5572be2e`; Python adapter SHA-256: `3167cf5091075829db7f60d1862138ddc44cddd517d9d06c495a1724adba4ec3`. Owners continue integrating changes, so dispositions identify corrected source separately.

## R2-1 — diagnostic alias can overwrite numeric/provenance output

**Moderate; valid finding, corrected in source.** The initial projection download/source paths checked every payload output pair but did not compare supplied diagnostic storage with those outputs. A download could write its fields and then the generic `call()` writeback overwrite the same storage with a diagnostic. Source export had the analogous issue. The new direct callback-model scenario already captured the intended rejection: aligned, actually allocated overlapping diagnostic/output storage must return `BG_INVALID_ARGUMENT` without changing any aliased storage or other destination sentinel.

An overlap rejection inside `call(diag,...)` would still write the rejection diagnostic and corrupt the aliased output. The owner therefore added a nonthrowing address/count preflight outside `call()`. It compares written diagnostic extent with every numeric/source/options output, reads no payload data and avoids multiplying potentially invalid extents. Touching interval endpoints remain disjoint; a true overlap rejects before any write. The ordinary checked shape/alignment/overflow and ten payload-pair checks remain inside the normal exception boundary.

Re-reviewed corrected runtime SHA-256 `e6fe2b63476787f02c1b2504226996abaa287a5e62b6d2ea4e1b562cd3e9779a`: this finding is resolved in source. The integrator reported the direct red against the earlier core; corrected target execution remains separate evidence.

## R2-2 — Python resource construction failure leaves a published native ID

**Moderate; valid finding, correction pending.** `Engine.create_projection_query_set` and `Engine.project` successfully publish a native handle before constructing `ProjectionQueries` or `ProjectionResult`. If Python allocation/construction raises, no wrapper owns that live ID. Its native storage and dependency references remain until context close. The existing evaluator creator paths use the same pattern.

Use one owned create-and-wrap-or-close helper for both operation families: guard the native creator invocation, `handle.value` extraction and resource construction in the same try block. Close any nonzero published native handle on failure while preserving the original exception and appending any cleanup failure note. Pass/retain the ctypes handle owner, so allocating `handle.value` happens inside the try rather than leaving an earlier unguarded allocation. Cover `BaseException`: a pending `KeyboardInterrupt` can be raised on return from a GIL-released native creator after its output handle has been populated but before a constructor-only helper runs. Engine context creation needs the same invariant. This extends the established native partial-publication cleanup rule across the Python boundary; it adds no geometry library or binding framework.

The independent caller owner has been asked to add the direct pre-fix oracle before the helper is written: inject a resource constructor that captures its native ID then raises `MemoryError`; after the failed API call, raw native metadata/project/close must reject that ID. The current path leaves it valid. Run against the actual core, not a mock of the solver or native close implementation. Restore the constructor in `finally`. A one-thread process makes public ID revocation and private dependency release directly observable; modeled token counts can additionally be inspected in the existing callback lane where useful.

The related native-return boundary can be attacked without asynchronous signal timing: invoke the actual native creator through the original wrapper, then raise a chosen `KeyboardInterrupt` before returning to the caller and inspect the already populated output ID. The same cleanup helper must revoke it. This is one partial-publication invariant, not another numerical solver or test framework.

## Ownership, loading and failure paths checked

Projection query/result kinds extend the sole checked registry. Admission retains under the registry lock, releases it before waiting for the recursive context mutex, and rechecks public ID/open context. Creation, evaluation, metadata, source export and download use this admission. Public context close revokes both families; a thread promoted before close still fails its later admission. Source/result dependencies remain native ownership, not borrowed caller memory or a second registry.

Projection queries retain their exact patch snapshot; results retain the query and an owned options snapshot. Device child accounting increments after a completed private callback and decrements during once-only token cleanup. Projection destructors dispatch through the retained v2 table, while context/patch destruction remains in v1. The private context and plugin library remain alive through child callbacks. `publish_owned` also covers projection query/result publication failures, preserving primary allocation failure and later cleanup fatality. The previous native publication and cleanup fixes were not bypassed by the new family.

Lazy extension negotiation takes place under the existing context admission lock. It requires canonical v2 layout, projection capability, every callback and **exact** base-table pointer identity. A v1-compatible-looking different table cannot substitute for the table that created context/patch tokens. Stable absence/nonfatal unsupported/layout refusal is cached; transient OOM/device failure is propagated and not cached. Fatal diagnostics, including anomalous success or unsupported return codes, pass through the common poison path before caching. A valid cached table remains associated with the retained plugin, not another dynamically loaded context.

ABI-2 options/info use separate version checking; shared diagnostics remain ABI-1. Options are copied before use and independently checked for supported limits, finite tolerance and zero reserved bits. Numeric query extents are exact, snapshots are owned, and per-row invalid values can still enter shape-valid batches for numerical classification. Empty CPU batches do not evaluate or access payload pointers; the unused worker-partition division is not executed. Full result stride is passed independently of worker range, and partial thread starts join before their result storage is released.

Download checks all five exact output extents, alignments and ten output-pair overlaps before transfer/copy; R2-1 adds diagnostic separation. Source export similarly includes the options destination in pairwise output checks and preserves original targets, seeds and options. Failed backend copies can partially write raw destinations, but Python constructs no batch until the numeric download and host source export both succeed. A context closed between those synchronous calls therefore produces an exception instead of a batch with partial provenance.

Process-wide fatal poison is common to both families. Projection failures cannot leave ordinary evaluation admissible in a poisoned CUDA process, and ordinary evaluator failures cannot leave resident projection results usable. CPU contexts and already copied host arrays remain independent. Cleanup is still permitted after poison, and no reset/retry appears in either path.

## Python and callback-model scope

Projection symbols and ABI checks are lazy. A new adapter can still construct an evaluator on an old core because ordinary `_declare` has no bg2 dependency. A new core can keep evaluating through a v1-only plugin while projection reports unsupported. Projection inputs are copied into private numeric arrays before a GIL-released native call; options are frozen values copied to a private descriptor. Download allocates five fresh arrays plus host provenance; another projection cannot overwrite them. The downloaded batch retains the loaded library and its host source values independently of public resource handles.

`require_stationary()` checks the distinct projection terminal status; it does not demand a curvature-positive flag or claim nearest distance. Rank/unresolved results retain their mandatory status and candidate-valid interpretation. Fields, flags and counters are exposed without per-iteration Python computation. Explicit resource close/context-manager cleanup retains the earlier primary-exception preservation policy, subject to resolving R2-2 for failed wrapper creation.

The callback model exercises new admission/parent retention, extension refusal/base/layout/transient/fatal negotiation, publication OOM plus fatal cleanup, partial download, cross-family poison and restoration failures. Its source explicitly identifies modeled callbacks rather than induced hardware faults. Actual device restoration, memory checks and geometry remain supported-target checks; callback-model success cannot supply that evidence.

## Disposition

R2-1 is resolved in corrected source. R2-2 awaits its direct red and owner correction/re-review. No additional lifetime, lazy loading, option, full-stride or poison blocker was found. Target compilation, independent geometry, old-interface compatibility and host/device memory checks remain integration work; this review does not make a GPU performance claim.

## Final source re-review

The runtime owner implemented `_create_resource` and guarded Engine context creation after the integrator recorded constructor-failure red. Re-reviewed Python SHA-256 `ae708f5cf7138706bff4350a4a5515bea6dc3e90e7c45f6b49fcedec366d9fd0`: **R2-2 resolved in source; no remaining runtime/Python source blocker found.**

The common helper retains a ctypes output handle and encloses `_invoke`, `.value` conversion and Python resource construction in one `BaseException` guard. Both evaluator and projection patch/query/result creators now use it. Engine similarly guards context creation and post-create initialization; on failure it clears the partial object's handle after closing the native publication. A failure before publication safely encounters a zero ID; a failure after publication can consume the actual populated ID.

`_close_unwrapped` calls native close using that ctypes owner, tolerates already invalid IDs, preserves the exact primary exception and attaches a native cleanup error object plus note/flags when available. If diagnostic allocation itself fails it still attempts close with the optional diagnostic omitted. Further cleanup/reporting failure is caught so it does not replace the primary exception. A partially initialized resource finalizer can also attempt close; the checked no-reuse ID registry makes the later attempt harmless rather than double-freeing a private token.

The independent constructor fixtures use the actual core, capture the published query/result/context ID, raise a chosen MemoryError, verify exception identity, and require subsequent raw native close to return `BG_INVALID_HANDLE`. They also check ordinary projection still works after the injected failure. The guard covers exceptions on native return in source, without requiring asynchronous signal timing. Target green and compatibility/memory execution remain the integrator's evidence, not a result inferred from this source review.

R2-1 remains resolved by the outside-call nonthrowing diagnostic-alias preflight. The combined revised boundary satisfies the reviewed publication/cleanup invariant without changing the v1 ABI or selecting a dependency.
