# Iteration 1 plan: runtime and Python review

Reviewed [implementation plan](../docs/ITERATION_1_PLAN.md) on 2026-10-13 against the [reviewed design](../docs/ITERATION_1_DESIGN.md) and the prior [runtime adversarial findings](2026-10-13-iteration-1-runtime-review.md).

**Disposition: recommend accepting this plan for the complete runtime/Python invariant. No remaining runtime architecture blocker was found.** Product implementation still requires the integrator's acceptance. The canonical public C header and internal backend table are the first implementation artifacts; consumers use those definitions rather than independent layouts.

The plan preserves all resolved design obligations: sole CPU-owned registry, lazy optional CUDA loading, fixed version/size/alignment checks, private Python copies followed by one validated native snapshot, query-to-patch binding, retained result lineage, context admission rechecks after lock acquisition, registry references released outside locks, device save/select/restore before publication, process-wide CUDA poison, operation-specific download failure and honest distinct-result allocation in public timing.

Implementation requirements checked with the numerical owner:

- The CPU evaluator's range entry receives the **full query count** as the SoA plane stride plus `[begin,end)` request indices. Passing a worker-local count as stride would write incompatible planes or collide with another worker's output.
- Runtime starts/joins CPU workers. If creating a later `std::thread` throws, it joins every worker already created before releasing the unpublished output. A default `std::thread` destructor is not a substitute for this failure path.
- Backend resource-close callbacks return status and bounded diagnostics, allowing device restoration and cleanup failures to be recorded without replacing an earlier operation failure. Child tokens retain the private backend context/plugin until cleanup ends, independently of public handle closure.
- Success publication is the final operation step after completion, device restoration and the shared poison-state check. New handles start at zero; failure leaves them zero. Existing resident results are preserved or invalidated according to the operation-specific runtime class.

The oracle owner will exercise actual allocated-buffer ABI errors, immutable snapshots, dependency lifetime, per-query statuses, NaN filling and provenance through the canonical Python/raw API. Test-build-only admission/failure seams may exercise deterministic close interleavings and real runtime error/cleanup paths. No hooks appear in default production artifacts and no arbitrary forged pointers become a supported input contract.

CMake builds CPU independently of CUDA and uses the qualified pinned CUDA compiler/runtime for the optional module. Per-context profiling is opt-in; normal public wall measurements include allocation/checks/synchronization/publication and exclude only the external truth oracle. Retained result buffers are not reused behind live handles.

This review establishes a workable implementation plan, not implemented correctness. Source review and direct CPU/both-device qualification follow each owner delivering its whole unit. No library adoption, product code or Git mutation occurred during this review.

## Implementation disposition

The accepted runtime unit is implemented in the owned public/private headers, native registry, standard-library ctypes adapter and optional CUDA CMake targets. Cross-lane source review found and closed complete-record input parity and cleanup fatal-flag propagation defects. A composed publication OOM plus fatal unpublished-result cleanup was reproduced against the preserved prior core (`FAIL: publication OOM lost fatal cleanup cause`) and corrected while retaining the original OOM identity and zero public output handle. Six fresh-process native CTests passed under ASan/UBSan on the target: promoted-handle/context-close interleaving, publication OOM cleanup, OOM plus fatal cleanup, first nonfatal then fatal cleanup, partial fatal download, and failed restoration without result publication. These are owned backend callback models; they do not claim actual injected hardware failures. Numerical peer re-review found no remaining lifetime helper blocker. Final public caller, exact-corpus and timing qualification is recorded separately by the integrator.
