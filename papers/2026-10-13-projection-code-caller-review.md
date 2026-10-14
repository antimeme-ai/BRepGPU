# Projection implementation: independent caller code review

2026-10-13. Reviewed the shared solver, CPU/CUDA scalar evaluator adapters,
native benchmark and Python cloud caller against the accepted design/plan.
The independent caller unit owns the exact fixture corpus, public tests and
standalone qualifier. Supported-host execution belongs to the integrator.

## Numerical and geometric findings

No new numerical blocker in this source pass. `projection_solver.hpp` forms
`r=S-X`, the prescribed gradient, Gram matrix and full distance Hessian. Rank
and direction systems screen normalized diagonal/Schur pivots before solving;
the clarified floor is strict, and the exact equality case is now in the
independent corpus. Rank precedes first-order stationarity and update budget.
Candidate fields use the current full jet and exact current UV. Accepted trial
counts and actual evaluator callbacks are separately counted.

The initial endpoint and each rounded chord displacement must have negative
computed directional derivative. Stable position-based distance change is
strictly negative and satisfies Armijo before accepting a trial. The next full
jet is recomputed before terminal reporting; rejected trials do not replace
the candidate. Zero backtracks still attempts the initial trial. The shared
classifier distinguishes ordinary rank refusal from a tolerance-relaxed
curvature diagnostic, uses reduced eigenvalues rather than unshifted pivots
for curvature separation, and avoids forbidden full-H eigenvectors at weak
corners. A saddle can correctly remain STATIONARY with NEGATIVE.

CPU scalar evaluation retains the selected descriptor's original control
offset while using the full owned control array and a one-request stride.
CUDA evaluates the same homogeneous channels from its selected patch offset.
Both outer batch writers retain full N as their SoA stride. Bad slots precede
target/seed reads and invalid rows have zero counters/flags. CUDA performs the
bounded solve inside one grid-stride request thread without host iteration.
Its new query/result callbacks preserve private context/device selection,
completion, restoration and unpublished-token cleanup patterns.

## Direct oracle/caller mechanism

The initial actual public red was an absent ProjectionOptions import in a
direct plane-answer test, before projection product implementation. The
completed corpus contains 104 cases: analytic affine/cylinder/parabolic
answers, exact saddle/flat/weak-boundary/corner/equality diagnostics, zero update
and zero backtrack outcomes, strict metric-floor equality, admitted non-nearest
conditioned seed, general controls across all16 degree pairs and mixed invalid
rows. Exact supplied-input geometry at returned UV checks every candidate's
point, distance and box residual with the accepted conditional budgets.
Known-UV expectations remain limited to named conditioned basins.

Public tests additionally attack frozen options, copied inputs, distinct patch
bindings/results/options, retained metadata after parent closure, context and
kind invalidation, worker/full-stride/tail batches, host threads, allocated raw
version/extent/all-ten-overlap rejection and diagnostic/payload alias rejection.
The standalone qualifier emits all input bits, exact expectation ratios,
violations, maxima/worst witnesses and work/status distributions. A requested
missing backend/capability fails qualification; compatibility refusal is a
separate expected operation.

The runtime review identified a Python wrapper-construction lifetime gap:
native create/project can succeed before a Python constructor raises. Added
direct tests that inject a chosen MemoryError after real native creation,
preserve its identity and require the captured native handle already revoked.
The engine partial-initialization path is also covered. These tests use actual
native handles and close any leaked token during their failing raw check.
The first injection invocation had an incorrect `mock.patch` call and failed
before fault injection; corrected to `patch.object`. That failed harness
invocation is not a product red. Actual target fault-red and corrected helper
green outcomes must be recorded by the executing integrator/runtime owner.

The integrator subsequently ran the corrected direct fault-red: three precise
failures confirmed surviving native context, query and result handles after
chosen constructor errors. The runtime owner was then authorized to implement
wrap-or-close cleanup; final helper green remains to be recorded.

## Integrator findings and delivery assessment

**C1: work distribution artifact.** The native benchmark currently emits
update/evaluation minimum, maximum and mean. These are useful summary statistics,
but the plan calls for distributions. Add per-case histograms or explicitly
name/document the more limited summary. Histogram construction belongs outside
resident/prepared-host timers. No slow/unresolved requests should be removed.

**C2: cloud cleanup preserves primary errors and drains all children.** The
current child-close loop stops on its first exception, and a later engine-close
exception can replace the original project/download exception. Attempt all
closes and preserve the primary error with cleanup notes, following the wrapper
context-manager behavior. These are caller failure-path concerns, not a finding
against the native ownership model.

**C3: condition comment.** The native known-solution check's eigenvalue-at-least-
one comment is local at the named cylinder answers, not throughout their
entire parameter rectangle. For a target close to the first cylinder endpoint,
the distance Hessian can be negative at the opposite endpoint. The independent
known-UV comparison remains conservative for this positive-quadrant radial
family: the angular gradient has the solution's sign and its magnitude bounds
parameter error by a fixed factor. Clarify the comment to avoid treating an
indefinite intermediate Hessian as a contradiction.

The native benchmark's expanded long-double geometry and derivatives are
independent of the Bézier evaluator and iterative solver. Its bad/regular/
adverse statuses and flags are explicitly qualified. Kernel profiling is a
separate context; resident time includes output allocation/completion;
prepared host-to-host includes snapshot/upload, project, download and status
consumption. Expanded analytic checks and statistics are outside those timers.

The cloud caller independently constructs exact dyadic radial targets or
parabolic normal targets, with distinct seeds and analytic UV/distance answers.
It reuses the exact supplied-input oracle rather than a second iterative
reference optimizer. It checks finite fields, guards stationarity, retains
target/seed/candidate/status/flags and controls, and exports target-to-candidate
segments over an explicitly synthetic untrimmed carrier. Its timer names
separate input preparation, first use, snapshot/upload, project,
allocating download/provenance, cleanup, qualification and export. Reported
72-byte device result and 44-byte per-request host source payload match the
ABI; descriptor/options and Python object costs remain additional.

All accepted findings require owner integration and affected target reruns
before publication. Source reading alone does not claim supported-host green
or establish GPU speedups, closest points on trimmed CAD, or full B-rep work.

## Integrated finding re-review

C1 is resolved in source. The benchmark accumulates actual per-request update
and evaluator-call histograms during its independent qualification pass, outside
the timed native regions. A per-case TSV records excluded warmup, resident,
prepared-host and profiled samples separately; the runner supplies and retains
the paths. Minimum/maximum/mean remain convenient summaries in the timing table.

C3 is resolved: the known-solution Hessian comment explicitly limits itself to
the local named answers and disclaims a cylinder bound over the entire box.

C2 now attempts all ordinary exception cleanup paths and preserves active
primary errors with notes. A remaining breadth issue was reported: collecting
only Exception lets a KeyboardInterrupt/SystemExit from a close terminate the
loop and replace a primary. Collect BaseException as well to fulfill the
attempt-all rule, then preserve/re-raise the primary or first cleanup error.

The Python native-publication helper is now enclosed around the actual native
invocation, handle-value conversion and resource construction. It passes the
owned ctypes handle object directly to cleanup, catches BaseException and
preserves the primary. The engine context has the same guarded boundary.
Added an actual-native post-return KeyboardInterrupt test at the owner's
request; its target execution is distinct from the constructor-red above.

Re-read the final caller cleanup: BaseException is now collected for every
child/context close, preserving the active primary or raising the first cleanup
failure with additional notes. C2 is resolved. C1–C3 have no remaining source
finding.

Added direct actual CUDA projection-family restoration on the selected backend:
opposite-device main-thread query creation, project, download/provenance and
all closes; worker-thread project/download/result cleanup; and restoration of
each thread's previous device. This one behavior is skipped only for an
explicit CPU run. CUDA runs require the runtime and both devices. Final source
has sixteen public projection test methods and the 104-case exact qualifier.
Supported-host final execution, sanitizer and compatibility results still belong
to the integrator; source-review disposition is no remaining blocker.

## Final supported-host execution disposition

The integrator reported the final current-source outcomes:

- Sixteen projection methods plus twenty-seven existing public methods: all
  43 passed on each P100 without skips. CPU passed with its two explicitly
  device-only behavior checks skipped.
- The 104-case standalone projection qualifier passed on CPU and each P100
  with zero violations.
- Compute Sanitizer ran the same 43-case suite on each P100: zero errors,
  zero leaked bytes and zero leaked allocations.
- All 21 native CTest cases passed in Release and host ASan builds. The CPU
  24-worker, 4,096-request benchmark passed all six corpora under ASan.
- Actual constructor/post-native-return exception cleanup and diagnostic-alias
  cases passed after their owner corrections.

These are integrator-run execution results, with source milestone `b4cc5f6` for
the completed caller/benchmark integration. No remaining caller/oracle source
finding. Compatibility completion, the published performance matrix and final
publication remain integrator work. These outcomes qualify the stated finite
corpus and runtime fault classes; they do not extend the product beyond its
seeded first-order candidate contract.
