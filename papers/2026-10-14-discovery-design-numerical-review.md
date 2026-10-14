# Iteration 3 integrated design numerical review

2026-10-14. Reviewed [integrated design](../docs/ITERATION_3_DESIGN.md),
[numerical algorithm input](2026-10-14-discovery-numerical-design.md),
[three-mode input](2026-10-14-discovery-strong-bounds-design.md) and the existing
owned CPU/CUDA scalar adapters and `project_local` decisions. This is a
code-independent design disposition, not implementation approval or measured
performance. Product/test implementation still follows plan review.

**Disposition: no remaining numerical design blocker.** The integrated
document inspected at SHA-256
`e6c764b0503d3ac3c13a5c42f874fbeb03cd5c4a5bedd78d4b88cfcc43e59e9c`
includes the three modes, original-input upper witnesses, root-first scalar
accounting, duplicate-root seed handling, tie-left ordering and impossible
stack failure. Minor subsequent runtime wording changes do not alter this
numerical conclusion unless they change the contracts listed below.

## Findings and disposition

| Item | Severity if uncorrected | Concrete issue | Integrated disposition |
| --- | --- | --- | --- |
| N3-1 | Blocker | Ordinary rounded subdivision or final-box epsilon would enclose a changed nominal carrier rather than the original binary-input patch. | Resolved: outward homogeneous intervals throughout midpoint restriction; exact positive-weight ratio hull; exported actual cache for exact qualification. |
| N3-2 | Blocker | Ordinary projector distance could underestimate the feasible witness and falsely exclude another region. | Resolved: every adopted UV uses a fresh original-patch interval position/distance upper. Finite unresolved/rank/saddle candidates remain eligible; solver arithmetic failures invalidate the row. |
| N3-3 | Blocker | Comparing differently normalized carriers' local distances would select in different metrics. | Resolved by scope: one named patch/local target; no cross-patch nearest selection. |
| N3-4 | Blocker | DOP target projection rounded to a scalar or sum of nonorthogonal slab distances could overstate the lower bound. | Resolved: target interval, nearest interval endpoint, downward squared separation divided by exact norm², maximum of slab lowers and combined-coordinate AABB lower. |
| N3-5 | Blocker | Bernstein C0 falsely assumed nonnegative; padded C4 zeros treated as active denominators; rounded product constants treated exact. | Resolved: negative C0 coefficient intervals admitted; only K=(2p+1)(2q+1) active coefficients; exact integer-quotient factor enclosure and positive active C4 intersection. |
| N3-6 | Blocker | Partial traversal leaf minimum or dropped frontier could raise L past an omitted true minimum. | Resolved: every visit-capped row returns the precomputed root lower. Completed traversal uses retained leaf lower minimum with monotonically decreasing feasible U and strict pruning. |
| N3-7 | Medium | Root-only/degree-zero seed could be projected twice, root scalar work hidden, consumed caps mistaken for withheld work. | Resolved: root scalar callback counted; root leaf duplicate seed skipped even at depth0; REFINEMENT_LIMIT reflects withheld root/eligible leaf solve; visits reserve both ordering children before either calculation. |
| N3-8 | Medium | Fixed stack13 overflow could be mislabeled as ordinary resource exhaustion despite degree/depth/tree invariants. | Resolved: overflow is arithmetic/invariant failure, not a new cap option or a successful coverage flag. |
| N3-9 | Medium | Squared gap could be mislabeled as distance accuracy or conflated with completed coverage. | Resolved: tolerance_squared0..1/default2^-24, upward U−L test, separate cap/coverage flags. Zero carries no attainability promise. |
| N3-10 | Medium | “Verified CPU environment” might check only creator RN, leaving worker FTZ/DAZ/traps or nonbinary evaluation unsupported. | Resolved numerical input: actual Linux-x64 builder/worker environment and binary64 compilation checks, no environment mutation, batch unsupported/no publication; runtime lane owns inspection/admission integration. |

## Mathematical and execution-path checks

For any covered node, outward homogeneous controls enclose its exact
restriction. Exact weights stay positive and rational normalized Bernstein
weights form a convex combination of Euclidean controls. Every Cartesian box
and homogeneous directional-support interval therefore contains the actual
carrier. Parent/global invariant intersections are safe. Empty/nonfinite
intersections mean failure; degeneracy is not exclusion.

DOP support uses `(n·A)/W`, not projected Cartesian box corners. With exact
norm²2/3, a conservative target-to-slab gap is no larger than the projection
of any feasible residual onto n; its squared/norm² quantity is a valid distance
lower. Maximum retains validity. The retained AABB sum remains necessary because
maximizing separate coordinate slabs would lose its combined-coordinate bound.

For Bernstein mode the enclosed product factors generate five exact quadratic
polynomial coefficient nets. Active C4 coefficients are convex combinations
of W_i W_j and lie[1,65536]. Exact squared distance is a normalized
positive-D-coefficient average of exact N_i/D_i. Interval expansion and the
one-division lower rule are valid: when Nlo<0, use Dlo; otherwise use Dhi.
Taking the minimum active ratio lower, clamping that function lower to0 and
maximizing with AABB lower is conservative. Dependency widening can prevent
gap success or pruning, but cannot create false exclusion. Streamed coefficient
evaluation needs no49-entry per-thread temporary net.

Under strict LB>U pruning a globally minimizing covered leaf cannot be removed:
its LB<=d*²<=U at every comparison. Completed traversal therefore retains at
least one such leaf; min(U,retained leaf lower minimum) bounds d*² below. At
a visit cap the root lower covers every omitted subtree. A feasible point
upper remains valid without stationarity or regular chart rank. Impossible
empty-retained completion or reversed brackets fail rather than qualify.

The initial scalar position and projector callback counts are bounded by
`1+64*(1+32*(24+2))=53313`. Valid rows perform1+refinements interval witness
calls separately. Visits are at most8191 per named patch even though the
option ceiling permits a larger budget; a child pair is computed at most once
and stored with its IDs. Fixed depth12 DFS needs13 pending entries. No query
node control net or unbounded queue is introduced.

Cache admission must account for0/160/3920 extra bytes per node, not only nodes.
At1048576 total nodes Bernstein auxiliary storage alone is4110417920 bytes;
device/host allocation failure retains the existing unpublished cleanup rules.
No partial hierarchy, automatic depth reduction or mode fallback is permitted.
CPU builds all modes and uploads identical cache bytes; CPU/GPU query endpoints
may differ in tightness because arithmetic strategies differ, so workload parity
is not presumed.

## What the plan must preserve

The accepted numerical input supplies complete primitive operations, packing,
fixed evaluation order, exact product-factor construction, sign-aware lower
division, failure outputs and actual-work counters. The forthcoming plan should
name the canonical shared structs and ownership boundaries without changing
these rules. The runtime environment guard must check actual numerical workers
and reject unsupported arithmetic before result publication. Empty query batches
perform no numerical work. Old ABI1/2 numerical behavior stays unchanged.

Direct qualification uses exact binary-input affine restriction/product
coefficients/support dots/witness distances, plus analytic global bracket cases.
CPU parity and sampled surfaces do not qualify exclusion. Stage performance
comparison after correctness on all three modes; measure depths0/2/4/8, actual
cache bytes/setup/upload, resident full calls, useful counts/bracket widths and
repeated-query amortization on CPU1/24 and both P100s. The user-requested
Bernstein experiment is part of this delivery, not deferred after an assumed
DOP win. No numerical performance winner is established by this review.
