# Projection plan: adversarial runtime review

2026-10-13. Reviewed `docs/ITERATION_2_PLAN.md` against the integrated iteration-2 design, the runtime study/review, and the existing evaluator public/private ABI, ownership, CUDA callback and ctypes boundaries. Scope is the entire runtime/ABI/lifetime/source-compatibility unit. No product or test implementation occurred.

**Disposition: no blocking runtime finding. The plan is ready for acceptance from this lane.** Numerical algorithm and oracle correctness remain separately reviewed units. The following checks explain the acceptance and identify constraints the implementation owner must preserve.

## Additive ABI and interface handoff

The plan assigns canonical public/private declarations to one runtime owner before consumers implement against them. The handoff includes exact field order, widths/version/size, options, status/flag values and the full-N CPU range contract. This avoids divergent POD definitions and SoA chunk-stride corruption.

The first sequence step explicitly preserves byte-compatible bg1 APIs, uses a distinct ABI2 descriptor validation path and retains the existing ABI1 diagnostic/helper path. This resolves R-P2 from the design review. No existing evaluator query/result accepts a new token kind; bg1_close alone recognizes added kinds through the sole checked registry. Distinct new result-info/download APIs prevent projection fields from being passed through the unrelated evaluator layout/conversion helper.

The canonical header handoff should keep the reviewed exact array/count/version/null/empty-batch contract visible. A count overflow, short descriptor, wrong version, misalignment or wrong handle must be rejected before copying/reading an unsupported payload or writing destination slabs. The plan's raw rejection/sentinel tests directly attack these faults; no alternate compatibility structure or second registry is needed.

## Loading, capability refusal and factory failure

The plan requires pure v2 factory negotiation bound to the same v1 table/token domain, with unchanged CPU-only loading and both old-interface evaluation paths. It explicitly tests pure refusal/malformed base while evaluation remains usable, factory runtime/fatal propagation and transient errors not being cached. Combined with the integrated design's handling of fatal flags accompanying OK or UNSUPPORTED, this resolves R-P1.

Implementation must classify stable missing/unsupported/version/layout/capability/base mismatch separately from an invoked factory's unexpected error. Tables and any cached refusal live in the existing plugin/context ownership/admission discipline. Only a validated table or stable refusal is cached. A present failure diagnostic goes through common status/poison handling, not a generic unsupported shortcut; fatal state disables both CUDA families, while nonfatal failure does not prevent an explicit later retry.

Preserved actual core/plugin binaries are the correct direct compatibility mechanism. New adapter/old core must still evaluate without declaring bg2; new core/old plugin must still evaluate with projection explicitly refused; old evaluator ABI calls/new core-plugin remain qualified. These checks inspect the real loader/dispatch boundary rather than a Python imitation of legacy behavior. Baselines remain privately outside overwritten build output, with publication limited to source identity and sanitized outcomes.

## Snapshot and resource lifetime

The plan extends the existing handle kinds and immutable snapshots, not the ownership model. Its numerical private query/result tokens depend on the v1 private patch/context, and the core keeps those exact dependencies/plugin alive. A projection result retains its query and original target/seed arrays, plus the same options snapshot that drove the solve. Public patch/query close revokes their IDs while dependent resources/provenance remain usable; context close revokes both operation families and drains private tokens after retained work quiesces.

Admission promotes under the registry lock, releases that lock, acquires the context lock and rechecks OPEN/public validity. Publication, cleanup errors and process poison reuse the existing common mechanisms. The planned blocked-admission/context-close case and publication-OOM/fatal-cleanup composition directly test the risky interleavings and once-only cleanup. No registry ownership may be released while holding a global lock if its destructor can reacquire context/registry locks.

Python uses frozen options and private ordinary input copies before CDLL releases the GIL. The native call must copy/validate/use one owned options image, retain it in the result and upload the same owned query arrays it validated. A second read of mutable caller memory after validation is prohibited by the reviewed design; this is an implementation invariant, not an added API feature. No borrowed output/device pointer or per-iteration Python loop is introduced.

The full-batch CPU range and join-on-partial-thread-start cleanup are explicit. CPU workers write disjoint ranges at the shared N stride; allocation failure must never free an unpublished result while already-started workers still write it.

## Device and failure boundary

The numerical lane owns the complete projection private CUDA resource callbacks, including event handling and cleanup. Every callback selects/restores the caller's device, completes owned-stream work and publishes its token only after successful restoration. The plan tests actual thread device preservation on both P100s and separately uses modeled callbacks for failure classification; it makes no claim to induce hardware faults.

Four integer output slabs plus seven FP64 planes are independently owned or checked nonoverlapping slices, and each public project allocates a distinct result. No result points into later-call scratch. Download preflight checks all shapes and ten output-pair overlaps before writes. Backend failure may partially write any of the five arrays; Python returns no batch. Cleanup/fatal state crosses evaluator/projection boundaries, CPU remains usable, and completed host arrays stay independent of the failed resident resource.

The planned fresh-process publication OOM plus fatal cleanup and partial-download cases preserve primary failure identity, zero public handle, token drainage and common CUDA poison. Extending the existing callback model at real new dispatch boundaries is sufficient; a separate fault framework would weaken ownership and add no direct claim.

## Measurement and actual caller

R-P3 is reflected concretely in the plan: identical native CPU1/CPU24/CUDA0/CUDA1 requests/options/fields, fresh resident result allocation, prepared download destinations, separately measured cleanup/first-use/upload/status consumption, and profiled contexts independent of default wall-time runs. Full matrix timing excludes overlapping owned build/test jobs and reports unresolved/rank/curvature distributions rather than successful native return codes alone.

The actual Python caller separately times preparation, query packing/upload, project, download/provenance, independent qualification and file/visual export. Its normal result materialization includes the five new arrays and retained source/options copies; that cost must not disappear by quoting only a prepared native interval. The caller's synthetic carrier/target scope explicitly excludes imported CAD export, global search, topology and automatic fallback. No reference source/export cost is silently treated as zero in a broader CAD claim.

Source extraction/import remains outside this iteration, consistent with the reviewed design. No proposed library adoption, protected-data mutation or extra platform setup appears in the plan. Deployment touches only the project's owned source/build directories and preserves actual baselines/unrelated data.

## Acceptance and implementation limits

The plan includes direct public and numerical red cases before operation implementation, green qualification on the supported CPU/two GPUs, retained evaluator qualification, relevant host/device memory checks, cross-review and integration of valid findings. Runtime failure/compatibility checks bear on real dispatch/lifetime behavior; they are not tests that certify another oracle. No runtime correction is required before plan acceptance.

Acceptance here is limited to the written runtime plan. Canonical headers and code still require direct caller execution and adversarial source review. Precise numerical thresholds/classification/rounding claims are owned by their separate reviews; this runtime acceptance does not turn empirical numerical budgets or status flags into a global geometric certificate.
