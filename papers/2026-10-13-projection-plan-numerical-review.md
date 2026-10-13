# Resident projection implementation plan: numerical review

Reviewed `docs/ITERATION_2_PLAN.md` against the accepted [projection design](../docs/ITERATION_2_DESIGN.md), the [source study](2026-10-13-projection-numerical-study.md) and resolved [numerical design findings](2026-10-13-projection-design-numerical-review.md). Reviewed plan SHA-256: `42640823a602f37d3492dcb3ecc95127baef5b2be7f2b9d94c98a00448ce8d8d`.

**No numerical implementation-plan blocker found.** The plan gives one owner the entire bounded solver and its CUDA resource contract while retaining independent jet algorithms and an independent geometry oracle. It explicitly defers product/test implementation until review and canonical-header handoff.

## Numerical consistency

The plan follows the reviewed finite algorithm rather than restoring the source study's larger alternative scope. There is one feasible-chord search per update; 24 halvings permit 25 trials; at most 128 accepted updates give the stated 3,329-callback bound. A zero-update request still evaluates its valid seed; an initial stationary seed does not require a fictitious update. Counts name actual evaluator calls, not estimated loop complexity. No host iteration, reseeding or automatic CPU fallback is introduced.

The shared host/device solver is the appropriate boundary for fixed operation order, normalized pre-square-root diagonal/Schur tests, same-scale RHS, reduced Newton/GN steps, full-matrix gradient scalar, endpoint cap/projection, actual rounded displacement descent and stable distance-change acceptance. Sharing those explicit rules does not make CPU/GPU parity an independent geometry oracle. CPU Bernstein and CUDA tensor de Casteljau remain separate evaluation paths; exact rational and analytic truth qualify their reported results.

Terminal handling follows the design: validate before evaluation; arithmetic/rank before stationarity and budget; use a full jet that names the exact returned UV; retain the accepted candidate for finite unresolved outcomes; never publish a rejected trial or a stale gradient. Surface evaluation arithmetic failure produces unusable NaN output. Rank diagnostics and stationary-only curvature diagnostics remain distinct. Regular saddles can be stationary candidates, regular flat curvature can be ambiguous, and weak-boundary negative flags describe the tolerance-relaxed diagnostic rather than a mathematical nonminimum verdict.

## Scalar evaluator and canonical interface handoff

The planned CPU reuse can call the existing Bernstein range helper with one stack-sized query: one selected descriptor, slot zero, full query count one, `[0,1)` range and stack output/status storage. The selected descriptor's original control offset still addresses the unchanged full immutable control array. That preserves iteration-one arithmetic without inventing another polynomial evaluator or allocating per-iteration host objects. Alternatively, an explicitly owned scalar helper is acceptable if its arithmetic remains identical to the existing path; changing the ordinary evaluator is unnecessary for this delivery.

The CUDA callback can consume the existing compact device patch descriptor/control offsets and tensor jet helpers directly in its thread. The tiny callback returns either the requested full jet or position and a truthful evaluation outcome. Exactly one counter increment belongs to each actual invocation, including a failed invocation. Trial displacement rejection occurs before the position callback and therefore does not increment that counter. Cache hits do not increment it. These are implementation details of the already written counter contract, not a second solver oracle.

The runtime-owned header handoff must carry full batch `N` separately from the worker range, the seven-plane field order and four distinct integer outputs, canonical option limits and the projection query/status/flag meanings. The plan explicitly requires that handoff before dependent coding. No numerical header should silently duplicate a public POD, enum value or private function-table signature. Internal scalar jet scratch is owned numerical machinery and need not become another public ABI.

## Qualification and rounding

The distance allowance propagates position error through squared distance. The gradient allowance propagates both position and first-derivative errors, and includes a sum-of-absolute-products arithmetic allowance that remains useful under cancellation. Nonexpansive box projection then bounds the residual discrepancy by the largest gradient discrepancy plus the stated rounding allowance. The plan correctly labels these as finite-corpus allowances conditional on empirical evaluator grades, not universal runtime bounds.

Every stationary row must separately satisfy its reported computed threshold and the exact returned-UV residual within that allowance. Known analytic answers are checked only for the named stable, conditioned fixture basins; their reduced-Hessian and map-based allowances must be stated per fixture. They must not be extended to a saddle, quartic flat, condition-family limit or an arbitrary carrier. The deliberately rank-admitted but inaccurate-UV seed demonstrates why a projected-gradient status is not a universal inverse-error guarantee.

Curvature expectations come from independent exact geometry. The strict eigenvalue-threshold equality fixture, forbidden weak-corner eigenvector and weak positive multiplier directly attack the classification contract; they do not duplicate the shared solver's numerical branch code. Ordinary iteration-one second-derivative qualification remains required by the retained evaluator suite. The projection flags add fixture-specific matrix-sign evidence rather than promoting that empirical second-derivative grade into a certified Hessian enclosure.

Exact endpoint distance comparison with both evaluation allowances attacks final nonincrease. It is deliberately weaker than certifying every exact-surface iterate; the reported contract makes that distinction. Counters use necessary bounds and analytically direct initial/zero-update cases rather than a parallel optimizer that guesses the iteration trajectory.

## P100 resources and complete operation

The single-thread bounded loop and stack-sized scalar callback allocate no dynamic numerical scratch and write no all-query intermediate jet buffer. Private query/result allocations, events, synchronization and device restoration belong to the numerical CUDA owner. The core retains context/patch/query dependencies through token cleanup, so the private solver never substitutes raw public IDs for owned device storage.

Seven double planes plus four integer slabs require checked total extents, distinct allocations or correctly aligned checked slices. Callback completion/restoration precedes token publication; partial creation and result-publication failure consume unpublished allocations and propagate cleanup fatality. The additive v2 factory must return the exact existing v1 table identity, preserving the private context/patch token domain. The plan's compatibility and callback-model checks attack these boundary facts independently of the numerical corpus.

Register/stack/spill inspection is required but not a reason to add wavefront scheduling before measurements. The current evaluator's register report is not the solver's future resource report. The target loop limit is finite, but different line-search/update paths can still diverge within a warp; mixed-case distributions and unresolved populations must remain in the published timings. Equivalent options, required fields, allocation/publication, transfers and status consumption make CPU/P100 comparisons reviewable. No GPU throughput claim follows before those actual runs.

## Disposition

The numerical lane supports accepting this plan. No product or test code was written by this review. Implement the numerical unit only after the integrator confirms acceptance and the runtime owner supplies the canonical headers. Cross-owner code review, supported-host geometry/runtime checks, both-device memory checks and measured caller/benchmark output remain the completion conditions already specified by the plan.
