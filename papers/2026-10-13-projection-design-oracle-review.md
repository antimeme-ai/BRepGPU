# Projection design: independent geometry/oracle review

2026-10-13. Reviewed `docs/ITERATION_2_DESIGN.md` against the exact fixtures in
`2026-10-13-projection-oracle-study.md` and the numerical source study. No
implementation or new tests were written.

The narrow result contract is coherent: `STATIONARY` accepts a numerically
regular first-order box candidate, including a saddle or flat candidate;
curvature flags remain separate. `require_stationary()` is an appropriate
guard. It must not become a minimum or nearest-point assertion downstream.
This resolves the main geometric concern in the research study without
requiring a complete constrained-minimum classifier.

## Findings requiring explicit integration decisions

**O1: robust curvature needs an eigenvalue-equivalent definition.**
“Robust SPD” with threshold `delta_H` is ambiguous if implemented as Cholesky
pivots exceeding `delta_H`. Those pivots are not eigenvalues. An exact fixture
from the numerical reviewer makes this observable: degree-one affine carrier
`S=(s+t-1,a(s-t),0)`, `a=2^-10`, target zero and seed `(1/2,1/2)` has
`H=G=[[1+a²,1-a²],[1-a²,1+a²]]`, norm infinity `2`, and smallest eigenvalue
`2^-19`, exactly the proposed threshold. Its ordinary Schur pivots both exceed
the threshold. Under strict eigenvalue separation it is `AMBIGUOUS`, not
`POSITIVE`; rank still passes. Specify `H_reduced-delta_H*I` positive definite
for POSITIVE, and the analogous strict negative separation for NEGATIVE.
The direction-solve pivot floor is a different mechanism and threshold.

**O2: weak-bound flags describe a tolerance-relaxed cone, not a nonminimum
verdict.** The design removes a bound only when its multiplier exceeds the
stationarity tolerance. For `S=(s,t,s²)`, target
`X=(-2^-25,1/2,3/4)`, and seed `(0,1/2)`, the exact lower-bound multiplier
is `2^-25>0`, below default tolerance. The seed is an actual strict boundary
local minimum, yet the diagnostic retains the weak coordinate and finds
`Hss=-1/2`. A NEGATIVE flag is consistent with an observed negative second
derivative along a feasible direction, but does not prove a descending
critical direction at this exact point. Name the tolerance-relaxed critical
direction convention and explicitly forbid treating the flag as a nonminimum
verdict. This leaves the first-order status unchanged.

**O3: define the fallback scale and reduced system norm before coding.**
The step construction currently says “scaled negative-gradient direction” and
“same scaled pivot discipline.” Specify the divisor for that fallback and
whether each Newton/GN pivot guard uses its actual reduced matrix norm.
These choices affect accepted-update/evaluation distributions and reproducible
failure cases, even though the first-order termination contract is independent
of a particular iteration path. The plan should also require `g·actual_dx<0`
for each rounded Armijo trial, and state the position difference/dot-product
operation order shared by both backends. An endpoint descent check is not an
explicit check of every rounded chord displacement.

## Exact fixture disposition under this contract

| Fixture/result | Expected status and diagnostic |
| --- | --- |
| Well-conditioned plane interior/edge/corner | STATIONARY, known analytic UV/position/distance, correct boundary signs |
| Tilted coupled-boundary target | STATIONARY at `(0,1/2)`; `(0,11/21)` is not the resolved analytic answer |
| Positive radial cylinder target | STATIONARY with known rational UV/point/distance; nonlinear throughput seed differs from solution |
| Negative-quadrant cylinder endpoint seeds | Both STATIONARY; strict activity removes `s`, reduced curvature positive; farther endpoint remains a valid local result |
| Exact cylinder/parabolic saddle seed | STATIONARY with NEGATIVE; accepting first-order stationarity is correct here |
| Cylinder-axis continuum / quartic parabolic minimum | STATIONARY with AMBIGUOUS; regular metric must not become RANK_UNRESOLVED |
| Weak-boundary parabolic saddle | STATIONARY with NEGATIVE from a feasible coordinate direction |
| Weak corner `S=(s,t,st)`, target `(0,0,-2)` | STATIONARY, conservative AMBIGUOUS permitted; forbidden `(1,-1)` eigenvector must not set NEGATIVE |
| Constant, curve-like, singular-pole chart | RANK_UNRESOLVED with retained finite candidate and rank flags |
| Invalid slot/seed/target | Specified bad-row status, all numerical fields NaN, zero counters/flags |
| Nonstationary seed with zero update budget | ITERATION_LIMIT, exact seed retained, one full-jet callback |
| Already stationary seed with zero update budget | STATIONARY after rank check, one full-jet callback |

Known-UV assertions need conditioning qualifications. The exact affine family
`S=(s,epsilon*t,0)`, `epsilon=2^-19`, target
`(1/4,epsilon/2,1/4)` and seed `(1/4,1/4)` passes the proposed rank floor:
`G=diag(1,2^-38)`. Its box residual is only `2^-40`, below default tolerance,
so the seed is honestly STATIONARY although its `t` differs by `1/4` from the
analytic closest UV. The independent oracle must check reported geometry,
distance and residual here; it must not impose universal UV accuracy.
With `epsilon=2^-21`, the positive second metric pivot is `2^-42`, below the
floor, so RANK_UNRESOLVED is expected. Avoid equality-threshold cases until the
design specifies the comparison convention.

For the positive radial cylinder target `(3/4,1,1/4)`, the exact angular gradient
is `(2s-1)(s+2)/(1+s²)²`. Its sign follows `s-1/2`; throughout the rectangle
its magnitude is at least `|s-1/2|`. With tolerances below `1/2`, a small exact
projected residual therefore bounds `|s-1/2|`, while `t` follows the plane
coordinate. This supplies a conditioned analytic solution oracle without
assuming every surface has comparable conditioning.

## Arithmetic, counters and lifetime checks

The stable distance-change identity addresses the known plane cancellation
witness directly. With target `(1/2,1/2,1)` and seed
`(1/2+2^-30,1/2)`, a tolerance `2^-32` requires an actual update: exact initial
rho is `2^-30`; the UV correction is representable and has stable change
`-2^-61`. Rounded objective subtraction must not replace the stated identity.
Final rho must refer to the exact returned UV, not a rejected trial or stale
cached jet. The design correctly qualifies descent as between evaluated
positions rather than a certified exact-surface monotonicity theorem.

Independent final-distance comparison needs an explicit finite-corpus error
budget in the plan. Conditional on position errors at most `E0`, local residual
components at most `5` give exact distance-squared discrepancy at most
`30*E0+3*E0²` per endpoint, before output arithmetic rounding. Twice that term
can compare seed and candidate on the qualified corpus. This is a calculation
from empirical evaluator targets, not a universal pointwise enclosure.
Exact recomputation at returned binary64 UV should separately check each
reported point, distance and box residual with justified rounding allowances.

The callback upper bound `3329` is coherent for a single chord search, 128
accepted updates and up to 25 position trials per update. Initial stationary/
rank cases require one evaluation; invalid rows require zero. Every finite row
has at least `1+accepted_updates` evaluations. The plan should settle no-motion
trial counting and distinguish actual zero-displacement stagnation from an
exhausted nonzero trial at `max_backtracks=0`; an empty set of remaining trials
must not make STAGNATION true by vacuity. No oracle needs to reproduce the
solver's loop to check these bounds and exact initial-terminal cases.

Patch/query/result retention, fresh result ownership, original target/seed
export and per-result options copies preserve source identity. Equal-sized
patch snapshots remain distinct; multiple seeds remain separate row identities.
No world target transform is inferred from the output frame metadata. The
iteration-one numeric ABI and plugin compatibility are separately exercised,
as are invalid-buffer and device-restoration faults.

## Delivered-value and measurement disposition

The same CPU/GPU operation, options, seeds, geometry and output checks are
required. Nonlinear timing inputs must need work instead of starting already
stationary. Record stationarity, rank, unresolved and curvature counts plus
update/evaluation distributions; these make a fast-rejection result visible.
The current prepared-host/resident/Python timing separation is appropriate.
The cylinder/polynomial point-cloud caller must label all exported rows as
candidates with their status/flags and count unresolved rows in its report.
No result table should relabel STATIONARY as “nearest solved” or count saddle/
flat candidates as established minima.

After O1–O3 and the counter/error-budget conventions are explicit, the narrow
design has no geometry/oracle blocker. Full minimization, branch discovery,
trim containment, periodic chart handling and global nearest guarantees remain
future operations with different oracles.

## Re-review disposition

Re-read the revised design after integration. O1 is resolved: the smallest
reduced eigenvalue must be strictly above `delta_H`, equality is ambiguous,
and the shifted-SPD equivalence is explicit. The affine equality witness is
now a required case. The implementation still needs numerical qualification
of its small eigenvalue calculation, as already stated by the design.

O2 is resolved: the diagnostic explicitly uses a tolerance-relaxed cone and
NEGATIVE does not establish that the exact point is a nonminimum. The exact
weak-positive-multiplier boundary-local-minimum witness is included. This is
consistent with STATIONARY remaining a first-order candidate status.

O3 is resolved: each reduced solve normalizes its actual matrix/RHS, reduced
Gram damping is defined, the gradient fallback uses the specified full-matrix
divisor, and each actual rounded chord trial must have negative `g·dx` before
evaluation. The design specifies contraction, dot accumulation and trial UV
construction. The attempted-displacement test includes the initial trial, so
zero backtracks cannot produce an empty-set stagnation classification.

The fixed candidate/rank/curvature flag semantics, mutually exclusive curvature
bits and admitted-but-inaccurate conditioned-chart fixture are also explicit.
No remaining independent geometry/oracle design blocker. The implementation
plan should turn the reported-field error allowances and counter cases above
into concrete qualification budgets; those budgets qualify a finite corpus and
must not become a universal UV or distance guarantee.
