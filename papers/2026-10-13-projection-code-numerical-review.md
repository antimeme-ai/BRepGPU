# Projection numerical/CUDA code review

2026-10-13. Independent adversarial review of `native/projection_solver.hpp`, `native/projection_cpu.cpp` and the complete new query/result/kernel/callback/factory unit in `native/cuda_backend.cu`, against the reviewed iteration-2 design/plan and canonical projection public/private headers. This reviewer owns the runtime boundary, not the numerical implementation. No numerical files were edited.

**Disposition: no blocking numerical/CUDA source finding.** Supported-host exact-oracle, memory and performance execution remains required and is owned by the integrator. Source review does not independently certify general convergence or exact surface behavior.

## Mathematical decisions and terminal state

The shared solver derives residual, gradient, Gram metric and full distance Hessian from six-jet components with the specified axis accumulation order. Raw box projected-gradient residual names the current reported UV and local objective. The regularity screen executes before stationarity and budget, with strictly positive normalized diagonal/Schur values greater than the 2^-40 floor. Equality returns the small-positive rank outcome, matching the integrator's explicit design clarification.

The reduced active set uses exact bounds and outward gradient sign only. Positive-Hessian directions fall back to the damped reduced Gram system, then a scaled negative-gradient direction if projection destroys descent or the candidate direction is unavailable. Raw steps are capped before endpoint projection; line search follows the single feasible chord, checks actual rounded displacement slope and uses position-only evaluation plus stable actual-point distance change. Zero/backtracking caps are handled explicitly: zero updates still evaluates the seed; zero halvings still attempts a full trial. Small step/objective change never establishes stationary success.

On accepted updates the next loop recomputes the full jet at the exact new UV before rank/stationarity/budget/classification. Exhausted search retains current accepted point/jet/residual and updates only the evaluator count; rejected trial data is not published as a candidate. Invalid input and arithmetic failure create all-NaN numerical fields and clear flags. Finite solver/rank terminations retain CANDIDATE_VALID and exact reported state, without a minimum/global promise.

Curvature flags inspect the reviewed tolerance-relaxed feasible subspace. Strict outward multipliers remove coordinates, weak bounds retain one-sided directions, and robust positive classification checks the reduced smallest eigenvalue rather than unshifted Schur pivots. Weak-bound negative classification uses feasible coordinate directions, not an arbitrary infeasible full-H eigenvector. The numerical flag convention and ambiguity band remain separate from first-order status.

All loops are bounded by validated options; initial full evaluation plus at most 128 accepted updates with 25 value trials and one full jet each fits the declared 3329 u32 count. Counts include actual evaluator calls, including failed calls; unchanged rounded or non-descent trial points do not invent evaluations. No reseeding, hidden multi-start, trim classification or fallback dispatch occurs.

## CPU and device indexing

CPU scalar evaluation reuses the unchanged owned Bernstein evaluator with N=1 stack arrays. Passing an individual original patch descriptor together with the original controls correctly retains that descriptor's control_offset. The range helper checks bad slot before target/seed validity and writes field planes with full batch N, not worker range length. Empty ranges dereference nothing.

CUDA uses the same solver decisions with independent tensor de Casteljau homogeneous jets and rational recovery. The kernel checks slot before patch access, uses the validated record-unit control offset, checks target/seed envelope, and writes all seven planes/four integer slabs with full N stride in grid-stride loops. There is no all-N derivative history, host iteration loop or result pointing into a later call's scratch. Per-thread state may spill to device local memory; compiler/resource measurements, not the word scratch, determine that cost.

## Private allocation, completion and cleanup

New query/result structs belong only to the projection callbacks. They depend on the same v1 private context/patch tokens; the core supplies retained ownership and the callback create paths check context association. Allocation counts cover target/seed/result multiplications and byte conversion. All partial allocations are retained by Pending until explicit release; failure cleanup occurs before guarded status/diagnostic publication and preserves prior failure identity plus later fatal flags.

DeviceScope saves/selects/restores on creation, project, download and destruction. Each work path synchronizes the existing owned stream; token release is after DeviceScope destruction, so restoration errors prevent publication. Failed selection records an error and performs best-effort cleanup without guessing another device. Fatal-runtime cleanup may be unable to free every physical allocation, but consumes private tokens and exposes errors for common process poison; it does not reset/recover the device implicitly.

Each project allocates its own seven floating planes and four integer slabs. Optional events bracket kernel submission only, while public resident wall time includes allocation/completion/restoration/publication elsewhere in the core. Event create/record/elapsed/destroy errors affect operation success, and Pending cleans the result rather than exposing it after a cleanup failure.

Download validates trusted token association and exact capacities before queued copies, skips zero-size transfers, and completes the stream before return. Later copy/completion/restoration failure may leave partial host writes, as the public boundary states; the core and Python must discard all destinations. Public alias/alignment validation remains the core's job and is separately under runtime review.

## Compatibility and pure factory

The separate v2 static table references exactly `&table`, the unchanged v1 table returned by its original factory. New projection callbacks do not reinterpret tokens through v1 query/result functions. The factory only initializes output/table/diagnostic state and performs a version/static-table check; it contains no allocation, CUDA call, device selection or context creation. This meets the reviewed lazy-extension contract and lets a new core validate base pointer identity.

The runtime's separate diagnostic-alias and Python wrapper partial-publication findings are being corrected in their owning unit. They do not require changes to these numerical files. No numerical/CUDA source correction is requested by this review. Real old-binary compatibility, both-card device restoration/memcheck and independent geometry/status/curvature qualification still supply the direct execution checks required by the plan.
