# 2026-10-13: projection independent caller implementation

Accepted plan revision `1e2cba4`. Sub-plan: derive supplied-input exact geometry
and named analytic/status/curvature cases; capture the absent public operation
as a direct red; consume canonical runtime declarations; then build one complete
public caller/oracle and standalone witness-producing qualifier. Root owns
supported-host execution, native/device fault checks and benchmark/caller output.

Captured the public red before projection implementation: one direct plane
answer test failed because ProjectionOptions was absent; no skip. The first
attempt used the environment's Python3.9 and failed an existing Python3.12-type
annotation; reran with available Python3.13 to establish the actual absent-API
red. The ignored log records that ImportError. This is a public-API red,
distinct from the numerical owner's direct native solver red.

Wrote the exact projection fixture module and public tests covering geometric
answers, first-order/curvature/rank status, budgets/counters, conditioned-chart
limits, snapshots/options/source identity, parent closure, independent results,
foreign/wrong-kind/context closure, host threads and allocated-buffer raw ABI
rejections. Added the integrator's strict rank-floor equality case. Wrote the
standalone TSV qualifier with exact input/output ratios, worst-field witnesses,
status/work distributions and nonzero requested-backend failure exits. Syntax
checked locally; native geometry execution remains on the supported host.

Added actual-native constructor-failure cleanup oracles for projection query,
result and partial engine initialization at the runtime owner's request. First
target invocation failed on my incorrect mock API before fault injection;
corrected every call to patch.object and used subtests so query/result reds
both execute. That first invocation is not accepted as a product red.

Cross-reviewed shared solver, CPU/CUDA scalar adapters and integrator benchmark/
cloud source. Saved `papers/2026-10-13-projection-code-caller-review.md`. No new
numerical blocker; reported work-distribution output, caller cleanup primary-
error preservation and a local-versus-global Hessian condition comment to the
owning integrator. Target execution and owner finding dispositions remain pending.

The corrected constructor fault-red reached real native allocation and failed
precisely for surviving context, query and result handles. The integrator
reported all three expected lifetime violations before the runtime owner began
the helper correction. The integrator accepted caller cleanup and histogram
findings and wrote their integration change plan; source re-review/target green
follow those owned edits.

Added an actual post-native-return KeyboardInterrupt lifetime test and direct
new-family CUDA current-device restoration on opposite-device main/worker
threads. Final source has16 public projection methods (one explicit CPU-only
skip for CUDA behavior) and104 qualification cases. Re-read integrated work
histograms, local condition wording and attempt-all BaseException caller
cleanup; C1–C3 resolved, no remaining source-review blocker. Syntax checks pass;
final supported-host runs remain with the integrator.

The integrator reported final current-source green: 43 public cases on each
P100 without skips; CPU passing with 2 explicit device-only skips; 104 projection
qualifier cases on all 3 backends with 0 violations; both matching CUDA memory
checks with 0 errors/leaked bytes/allocations. Native 21-case CTest passed Release
and ASan; CPU 24-worker, 4096-request six-corpus benchmark passed ASan. Constructor,
post-native-return interrupt and diagnostic-alias corrections passed their
direct target cases. Appended these outcomes to the caller review disposition.
Caller/benchmark integration milestone is `b4cc5f6`. This independent unit is
complete; compatibility/performance matrix/publication remain integrator-owned.
