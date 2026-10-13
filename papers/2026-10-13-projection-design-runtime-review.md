# Projection design: adversarial runtime review

2026-10-13. Reviewed `docs/ITERATION_2_DESIGN.md`, the projection runtime/numerical studies, and the existing public/private headers, registry, CUDA callback boundary and ctypes adapter. Scope is the complete ownership/ABI/loading/failure/measurement unit. No projection implementation or dependency change was made.

**Disposition: one high-severity loading/failure clarification must be integrated before plan acceptance.** The shared registry/dependency graph, distinct projection kinds, compact shape, per-entry device discipline and timing separation otherwise form a coherent additive extension. Numerical solver/classification review remains with its owning lane.

## R-P1 — High: extension refusal must not swallow a factory runtime/fatal failure

The optional-plugin section says a missing/incompatible extension yields UNSUPPORTED projection without disabling evaluation. This is correct for an absent symbol, unsupported interface version, missing capability, malformed table or mismatched base identity. It is not correct for an invoked factory that returns a device/runtime error or a fatal diagnostic. The existing v1 factory runs through the common `backend_status` path, which records process poison. A new lazy loader that maps every v2 problem to UNSUPPORTED can erase OOM/device failure and ignore fatal flags, leaving evaluator operations apparently enabled on an unusable process.

Concrete correction:

- Make the owned v2 factory a pure, nonallocating version/static-table query: no context creation, CUDA call, stream work, lazy device initialization or token allocation. It returns the same stable v1 base table pointer and the static extension table.
- Separate capability refusal from an invoked factory's diagnostic. Missing symbol/version/layout/capability/base identity returns explicit UNSUPPORTED with no token and preserves evaluator use. An unexpected nonzero factory status propagates its original diagnostic through common status/poison handling; a fatal flag poisons both operation families even if the factory incorrectly returns OK.
- If extension discovery is cached, distinguish UNPROBED, AVAILABLE and UNSUPPORTED under the existing context/plugin admission discipline. Cache only stable capability refusal. Do not turn transient allocation/device errors into a sticky unsupported capability or unload/reload the already-used plugin to retry.

The direct runtime attack is a modeled v2 factory with a nonfatal error, and a fresh process with a fatal diagnostic (including fatal+OK anomaly), followed by evaluator/projection admissions. Check original status/cause, zero creator handle, common poison on fatal and CPU continuity. This extends the same actual callback boundary as the existing fault tooling; it is not a second layer that certifies other tests.

## R-P2 — Medium implementation constraint: preserve both descriptor version domains

The design explicitly gives projection options/info/ABI-info version 2 and preserves diagnostic version 1. The existing core helper `version()` hardcodes `BG_ABI_VERSION=1`; reusing it unchanged for new descriptors would reject every canonical projection caller, while globally changing its constant would break evaluator callers. The design's policy is correct, but the implementation plan must assign the version choice to each concrete validation path.

Concrete plan correction: use an explicit expected-version argument or separate version-2 descriptor validation without altering ABI-1 validation. Public diagnostics continue through the ABI-1 call boundary. Raw calls must reject version-1 projection options/info and version-2 diagnostics without writing payloads, while preserved old evaluator descriptors still succeed. Python projection declarations/ABI-info checking remain lazy so evaluator use against the old core does not require missing bg2 symbols.

This is an implementation handoff constraint, not an architectural blocker if the plan explicitly preserves those checks.

## R-P3 — Low measurement clarification: name prepared buffers and external export scope

The design already records input generation, native snapshot/upload, fresh resident result allocation/completion/publication, download, status consumption, cleanup and Python packing/dispatch/provenance. It also correctly excludes imported geometry from this first operation. The actual Python caller exports target/seed/candidate records and a visual artifact, so two costs still need explicit labels in the plan/report:

1. A prepared native host-to-host row may reuse caller-owned download buffers; a Python `ProjectionResult.download()` allocates five new arrays and copies source/options provenance. Say which destination allocation is inside each row, use the same policy on CPU/GPU native rows, and include all normal Python result/provenance materialization in its caller measurement.
2. Record file/visual export separately and state whether the advertised complete caller interval includes it. The synthetic caller incurs no source CAD export/import cost. A later SMLib/USD extraction comparison must include source extraction, normalization and world-target conversion in that consumer's complete-task interval rather than reusing the synthetic caller's timing label.

These are labeling/accounting corrections, not a requirement to add an importer or another benchmark system. The current reference is the source/export study, not a selected dependency. A prepared resident/native timing cannot be presented as source-CAD-to-exported-output time.

## Accepted boundaries and attacks retained in the plan

The proposal correctly preserves every existing bg1 layout/status/symbol and v1 callback table. Requiring exact v2 base-table pointer identity before passing private context/patch tokens prevents a dangerous opaque-token reinterpretation. Missing v2 support remains a projection capability outcome; CPU operation never loads a GPU module. The new core must retain the v2 table's owning plugin through all added-token cleanup, not merely while negotiating it.

The distinct projection query/result kinds fit the existing single checked registry. A result retains its exact query, and that query retains the immutable patch snapshot. Host targets/seeds and the copied options naming a result remain available after public parent close. The context tracks public IDs without retaining children, and private native context closure waits for both operation families' retained tokens. Promotion before context locking, then OPEN/public-identity recheck, remains necessary for both new kinds. Releasing revoked registry ownership outside locks and retaining the private context/plugin during cleanup preserves the previously reviewed deadlock/lifetime discipline.

The new result must store the same owned options snapshot used by the CPU/GPU solver; no second read of a caller's options after GIL release or after native validation is allowed. This follows the runtime study's copy/validate/use contract and should be concrete in the plan. Query shape/alignment/overflow checks happen before snapshots; per-row slot/target/seed failures remain ordered outcomes rather than inconsistent whole-batch CPU/CUDA rejection.

The five output slabs have exact extents, including empty batches, and all ten distinct output pairs must be checked for overlap before writes. Public diagnostic/descriptor/handle storage obeys its separate-storage contract. A backend download failure makes all destinations unusable even if only one contains partial writes; Python publishes none. Host provenance export preflights its entire count/version/alignment/non-overlap contract before writing any source payload.

Fresh result allocation is part of resident projection wall time on all backends. Device scratch is per request/thread rather than an all-N jet history. Each callback selects/restores the calling thread's device and synchronizes the existing owned stream before result/host success publication; no second context/stream or host iteration loop is introduced. Fatal operation or cleanup state crosses the evaluator/projection boundary in both directions, with CPU and completed downloaded host arrays usable afterward.

Required direct runtime checks from the design remain useful: preserved old core/new adapter evaluation; actual v1-only plugin/new core evaluation and projection refusal; exact base mismatch; projection/evaluator wrong-kind and stale admissions; immutable targets/seeds/options; retained provenance; empty/count/alignment/overlap rejection; context close races; per-thread device restoration; publication OOM plus cleanup fatal propagation. Extend the existing guarded hooks/model rather than duplicating the lifetime invariant in a second registry or a test-only production pathway. Capability and cross-family poison checks must exercise real core dispatch, not a Python stub.

## Scope and result

R-P1 requires explicit design wording. R-P2 should appear in the implementation plan; R-P3 belongs in the caller/measurement plan. None requires a dependency, topology work, additional source import scope or a product implementation before review. No other blocker was found in this runtime unit. Re-review follows integrated changes.

## Integrated re-review disposition

Re-read the integrated iteration-2 design after the integrator's corrections. **No runtime/ABI/lifetime design blocker remains.**

- **R-P1 closed.** The v2 factory is explicitly a pure static-table/version query with no allocation/CUDA/device work. Stable absence, honest nonfatal unsupported response, malformed capability/layout and base mismatch are separated from other factory errors. Common diagnostic/poison handling includes fatal flags accompanying OK or UNSUPPORTED, and only stable refusal/valid tables are cached. This preserves evaluator availability only when capability refusal is safe and crosses operation families on actual fatal state.
- **R-P2 closed as a design/plan handoff.** New descriptors explicitly use an ABI-2 helper; the existing ABI-1 helper and diagnostic validation are unchanged. The plan and direct raw callers must realize this stated split; no alternate descriptor domain is implied.
- **R-P3 closed.** The measurement contract explicitly states prepared native destination reuse versus Python's five owned arrays and copied host provenance, requires the caller interval to name target generation and file/visual export inclusion, and labels source-CAD export/import and automatic fallback as absent for the synthetic caller. Those costs cannot be inferred into a source-CAD speedup.

The integrated fixed status/flag values do not alter evaluator meanings. CANDIDATE_VALID distinguishes finite terminal candidates; invalid/arithmetic flags are cleared. Existing common context/registry ownership, per-entry device restoration, exact output preflight, failure cleanup and fatal/publication serialization remain intact across both operation families. Numerical admission, residual and classification decisions are reviewed by their owning lane. This disposition accepts the runtime design for planning; it does not authorize product code before the separately reviewed plan.
