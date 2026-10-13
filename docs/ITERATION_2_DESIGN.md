# Resident seeded projection

Reviewed design, 2026-10-13. Numerical, oracle and runtime source studies are in
`papers/2026-10-13-projection-*-study.md`. Product implementation follows design
and plan review. This delivery consumes the owned iteration-one patch jets on
device and returns a compact seeded candidate.

## Mathematical result

One request names an existing immutable patch-snapshot slot, a local Cartesian
target X and a normalized seed u=(s,t). Patches retain the iteration-one envelope:
degrees 0..3 in both directions, positive homogeneous weights in [1,256], local
Euclidean controls in [-1,1]^3. Targets must be finite with each component in
[-4,4]; seeds must be finite in the closed unit square. No implicit seed clipping,
source-chart inference, world-to-local conversion or imported geometry occurs.

Minimize locally f(u)=.5||S(u)-X||^2 over [0,1]^2. The implementation returns a
seeded **first-order box-stationary candidate** when its computed projected
gradient meets tolerance and the evaluated chart passes the stated numerical
rank test. That status does not establish an isolated local minimum or a global
nearest point. Separate flags expose observed feasible negative curvature and
unresolved curvature. In particular, an exactly seeded regular saddle can be
stationary with a negative-curvature flag. Python calls the guard
`require_stationary()`, never `require_nearest()`.

No complete candidate discovery, branch selection guarantee, trim membership,
seam deduplication, surface-normal uniqueness or modeling topology is delivered.
Several seeds for one target remain independent rows. A farther boundary-local
candidate can be valid even when another seed finds a closer candidate.

With r=S-X, derive directly:

```text
g = (r·Ss, r·St)
G = [[Ss·Ss, Ss·St], [Ss·St, St·St]]
H = G + [[r·Sss, r·Sst], [r·Sst, r·Stt]]
rho = max_i |u_i - clip(u_i-g_i,0,1)|
```

The unscaled box residual rho is evaluated at the exact returned binary64 UV.
It is a local first-order diagnostic, not a spatial distance or UV-error bound.
At a lower bound g_i>=0 is admissible; at an upper bound g_i<=0; an interior
coordinate requires g_i=0. Small step or objective change alone never qualifies
a row. Numerical diagnostics refer to computed jets, not certified enclosures.

## Bounded solver

Defaults: gradient tolerance 2^-24; at most 64 accepted updates; at most 24
halvings after each initial full trial; infinity-norm raw step cap .25. Options
admit update limits 0..128, backtracking limits 0..24, and finite tolerances
[2^-32,2^-12]. Flags/reserved fields must be zero. Fixed algorithm constants are
not exposed as an unreviewed collection of tuning knobs.

For each current accepted UV:

1. Evaluate the full local jet, compute r, g, G, H and rho with finite checks.
   Count every actual full-jet or position-only callback invocation. Invalid
   input rejects before evaluation. Even a zero-update valid request evaluates
   its seed once. Use current cached full jets for terminal reporting when they
   still name that exact UV; never report a previous/trial residual.
2. Let metric_scale=max(1,||G||_infinity). Use scaled 2x2 Cholesky with pivot
   floor 2^-40*metric_scale to admit a numerically regular chart. A deficient or
   poorly conditioned metric returns RANK_UNRESOLVED, with a finite candidate
   and rank diagnostic, regardless of an accidentally small gradient. This is
   numerical admission, not a proof that the mathematical chart is singular.
3. If rho<=tolerance, classify curvature and return STATIONARY. If the accepted
   update budget is exhausted otherwise, return ITERATION_LIMIT. Rank and
   arithmetic failures take precedence over stationarity/budget.
4. For direction construction only, fix exact-bound coordinates whose computed
   gradient points outward: (u_i=0,g_i>=0) or (u_i=1,g_i<=0). Never freeze a
   merely nearby coordinate. Solve the reduced positive-definite H system
   Hp=-g with the same scaled pivot discipline. For any reduced M, let
   scale=max(1,||M||_infinity); normalize M and RHS by scale. Compare a and
   c-b*b/a to 2^-40 before square roots, then solve scaled Cholesky triangular
   systems. A one-dimensional solve tests its normalized diagonal similarly.
   Failed/nonfinite direction arithmetic uses the next safeguard while valid
   current jets remain usable. If unavailable, solve the reduced G_FF+mu*I
   system, mu=2^-20*max(1,||G_FF||_infinity), with the same normalization.
   Bound the raw step by
   scaling its infinity norm to at most .25.
5. Project once to z=clip(u+p), d=z-u. Require g·d<0. If the candidate direction
   is invalid/non-descent/zero, use p=-gamma*g with
   gamma=1/max(1,||G||_infinity,||H||_infinity) using full matrices, capped at
   .25, then the same projection/descent check. No feasible descent
   and a nonstationary current point returns STAGNATION.
6. Search the feasible chord at alpha=1,1/2,...,2^-max_backtracks. Bound each
   rounded trial to the square only to remove possible arithmetic endpoint
   excursions; do not snap interior iterates near a boundary. Use the actual
   rounded trial displacement in Armijo's right-hand side. Require its
   computed g·dx<0 before a position-only evaluation. A nonfinite evaluation
   returns ARITHMETIC. Use explicit FP64 add/multiply/divide with implicit
   contraction off; three-term dots accumulate axes 0,1,2. Trials are
   clip(u_i+alpha*d_i,0,1). Explicit evaluator FMA remains as in iteration one.
7. Compute stable actual-point distance change
   delta_f=.5*(S_trial-S_current)·((S_trial-X)+(S_current-X)). Require delta_f<0
   and delta_f<=2^-12*g·(u_trial-u_current). No positive fudge admits an
   increasing step. Accept and increment accepted_updates, then evaluate the
   new full jet before checking convergence/budget. If every attempted rounded
   displacement (including the initial trial) is zero, return STAGNATION;
   otherwise exhausted trials return LINESEARCH_LIMIT. max_backtracks=0 still
   attempts exactly the initial trial, so it cannot create an empty trial set. Never label rejection as convergence without the current
   full-jet rho test.

The scheme measures descent between evaluated FP64 positions, not exact-surface
monotonicity. Arithmetic floors can produce unresolved rows. Keep the current
accepted finite candidate on budget/search/stagnation/rank failures; no trial
with failed acceptance replaces it. No random reseeding, multi-start search or
automatic CPU fallback occurs. The worst callback count fits u32: one initial
full jet plus at most 128*(25 position trials + one full jet) = 3,329 calls.

CPU and GPU share these bounded iteration decisions while using independent
Bernstein and tensor de Casteljau jet evaluation. CUDA performs one bounded
solve per grid-stride request thread in one kernel, initially 128 threads/block.
Jets and iterates remain per-thread device scratch; no N-sized intermediate
jet download or host iteration loop exists. Optimize divergence/scheduling only
after measuring homogeneous and mixed iteration cases.

## Curvature and rank diagnostics

All flags describe evaluated numerical quantities. BOUNDARY means an exact
returned coordinate is 0 or 1. Rank flags distinguish an observed nonpositive
metric pivot from a positive pivot below the numerical floor, without claiming
a geometric singularity certificate.

Curvature classification is attached only to a stationary row. A bound is
strictly active for this diagnostic when its outward multiplier exceeds the
gradient tolerance. Remove only these coordinates from the diagnostic subspace;
weak bounds remain one-sided directions. This is a tolerance-relaxed critical
cone: a mathematically strict positive multiplier below tolerance can be treated
as weak. NEGATIVE therefore means a negative feasible directional curvature in
this relaxed diagnostic, not a verdict that the exact constrained point is a
nonminimum. First-order status remains a separate observed quantity. Use curvature threshold
2^-20*max(1,||H||_infinity) to separate robust signs from ambiguous near-zero
curvature. With no remaining directions, strict first-order activity qualifies
the reduced curvature diagnostic as POSITIVE. A reduced matrix sets POSITIVE
only when its smallest eigenvalue is strictly greater than the curvature
threshold (equivalently H_reduced-threshold*I is positive definite); checking
unshifted Schur pivots against that threshold is not equivalent. Equality is
AMBIGUOUS. With no weak bounds, a smallest eigenvalue strictly below the negative
threshold sets NEGATIVE. With weak bounds, set NEGATIVE only from a tested feasible critical
coordinate direction (H_jj strictly below the negative threshold on a retained
free or inward weak coordinate); otherwise an indefinite/flat matrix is
AMBIGUOUS. For two-dimensional eigenvalues use
half(a+c) +/- hypot(half(a-c),b); classification is numerical, not an enclosure. Never
infer feasible negative curvature from a full-H negative eigenvalue alone.

Flag values are CANDIDATE_VALID=1, BOUNDARY=2, RANK_DEFICIENT=4,
ILL_CONDITIONED=8, CURVATURE_POSITIVE=16, CURVATURE_NEGATIVE=32 and
CURVATURE_AMBIGUOUS=64. Every finite terminal candidate sets CANDIDATE_VALID.
Invalid/arithmetic rows clear all flags. Positive/negative/ambiguous curvature
bits are mutually exclusive and occur only on stationary rows; rank bits occur
on RANK_UNRESOLVED rows, with observed nonpositive versus small positive pivots
as specified. A stationary row may have BOUNDARY plus any one curvature bit.

These conservative flags may leave a known corner minimum ambiguous. They do
not discover distant alternative minima or prove an isolated/global minimum.
Metric and curvature failure are different: a regular flat cylinder candidate
is not labeled rank deficient.

## Public ABI and ownership

Every bg1 symbol/POD/status/layout and private v1 factory remains unchanged.
Add `include/brepgpu_projection.h`, public BG_PROJECTION_ABI_VERSION=2 and bg2
projection symbols. Existing handles, bg_status and ABI-1 diagnostics are reused.
Validate new descriptors with an explicit ABI-2 helper; the existing ABI-1
version helper and ABI-1 diagnostic validation remain unchanged. Options are a
32-byte versioned descriptor: four u32 header/update/backtrack
fields, FP64 tolerance, u32 flags/reserved. Projection info is a separate
48-byte versioned descriptor containing counts, backend/profile/field metadata
and optional event time. bg2_projection_abi_info reports canonical new sizes.

Operations follow the [runtime study](../papers/2026-10-13-projection-runtime-study.md):
create_projection_query_set(patch handle, slots, targets, seeds); project(query,
options); projection_info; projection_download; projection_sources. bg1_close
handles the added query/result kinds in its sole checked uint64 registry.
Wrong-kind/stale/foreign-context admissions fail before resource access.

Input is 44 bytes/row: slot u32, three FP64 target components, two FP64 seeds.
Numeric input extents are exact. Immutable native snapshots are copied before
validation/upload. Rows may be invalid while shape-valid batches still upload.
Per-row precedence: bad slot first, then nonfinite/out-of-domain seed/target,
then arithmetic/rank/solver outcomes. Empty batches accept null numeric pointers
and return empty results without launching while still validating options.

Output is SoA with full N stride: seven FP64 planes ordered
(s,t,Sx,Sy,Sz,distance_squared,rho), plus u32 status, accepted_updates,
evaluation_count and flags slabs: 72 bytes/row. Invalid input or arithmetic has
all seven fields NaN. Invalid-input counters/flags are zero; arithmetic counts
record actual attempted evaluations. Stationary and all other finite solver
terminations report their last accepted UV/position/distance/rho and counts.
If an arithmetic failure prevents current reporting, all numeric fields remain
NaN. STATIONARY=0, BAD_PATCH=1, BAD_PARAMETER=2, ARITHMETIC=3, UNSUPPORTED=4,
ITERATION_LIMIT=5, LINESEARCH_LIMIT=6, STAGNATION=7, RANK_UNRESOLVED=8. Existing
evaluator statuses keep their separate meaning. No curvature flag implies a
different unannounced status meaning.

Every project allocates a distinct result. Result retains query; query retains
the exact patch snapshot; children retain context/plugin. Public parent close
revokes handles, not dependent storage. Context close invalidates both evaluator
and projection kinds; admission/recheck, cleanup, publication failure and
process-wide CUDA poison follow the iteration-one contract. No second registry,
borrowed host/device buffer, asynchronous public callback or implicit retry is
introduced. Cleanup consumes each token once and retains the shared private
context through all dependent cleanup.

Download verifies type/count/alignment and every output pair's non-overlap
before writing. Backend copy failure can partially write any destination: all
five arrays are unusable and Python publishes none. Fatal failures affect both
operation families in every CUDA context; CPU and completed host arrays remain
usable. Source export copies retained patch descriptors, slots, original
targets/seeds and options; outputs must be mutually disjoint and separate from
descriptor/diagnostic storage. It is host provenance, not another GPU transfer.

## Optional plugin and Python

Keep brepgpu_cuda_backend_v1 exactly. Add a lazy v2 factory/table with a pointer
to that same v1 table; require pointer identity before sharing context/patch
tokens. Add query-create/close, project, result-close and download callbacks
only. The v2 factory is a pure static-table/version query with no CUDA calls,
allocation or device changes. Absence, an honest nonfatal UNSUPPORTED response,
layout/capability mismatch or wrong base identity is a stable unsupported
projection capability; evaluation still works. Propagate other factory errors
through the common diagnostic/poison path, including fatal flags with an OK or
UNSUPPORTED code. Cache only stable capability refusal or valid tables, never
transient OOM/device failure. A fatal factory error disables both CUDA operation
families consistently with the existing poison contract. Do not load a second plugin/context or
guess opaque-token compatibility. CUDA calls select/restore the calling thread's
device and complete work on the existing context stream before publication.

Python lazily declares bg2 symbols and checks their ABI only on projection use.
New adapter plus old core still evaluates; new core plus v1-only plugin still
evaluates. Projection on either old interface is explicitly unsupported.
Add frozen ProjectionOptions, ProjectionQueries, ProjectionResult and an owned
ProjectionBatch with field/status/count/flag accessors, copied provenance and
require_stationary(). Ordinary input copies are private before CDLL releases
the GIL. No per-iteration Python work or new binding/numeric library is adopted.
Downloaded provenance adds host copies/packing beyond the 72-byte device
result; Python timing reports this separately. All outputs are local; an
explicit later consumer maps source UV/world coordinates and handles trims.

## Direct qualification

Analytic/exact-binary-input oracles derive geometry, distance, gradient, metric
and curvature independently of the solver. Required cases:

- Orthogonal and tilted planes: known interior/edge/corner solutions, including
  the coupled boundary case where clipping a free Newton solution is wrong.
- Rational quarter-cylinder: exact dyadic radial targets, nonlinear refinement,
  boundary solutions, two unequal endpoint-local minima and an interior saddle.
- Parabolic cylinders: two isolated minima, interior and weak-boundary saddles,
  quartic flat curvature, and seed-sensitive candidate identity.
- Weak corner S=(s,t,st), X=(0,0,-2): indefinite full H, yet no feasible negative
  critical curvature; never set NEGATIVE from its forbidden eigenvector.
- Constant/one-direction/pole charts, a controlled condition family, mixed bad
  rows, max_iterations=0, max_backtracks=0 and convergence at the initial seed.

Every finite candidate must agree with independent geometry at reported UV,
distance to the original target, projected-gradient residual and specified
flags/status. Stable successful fixture basins also check known UV/point/distance
with tolerance derived from their known conditioning. A deliberately conditioned
chart S=(s,2^-19*t,0), target=(1/4,2^-20,1/4), seed=(1/4,1/4) passes the metric
floor yet is stationary at the seed under the default tolerance. Its t is not
the nearest t=1/2. Check its geometry/rho/status; do not demand universal UV
accuracy from the first-order predicate. Another exact threshold fixture is
S=(s+t-1,2^-10*(s-t),0), target=0, seed=(.5,.5): the smallest Hessian eigenvalue
equals the chosen curvature threshold and must be AMBIGUOUS. Include a weak
bound with outward multiplier 2^-25 below default tolerance; its NEGATIVE relaxed
curvature flag is compatible with an exact strict boundary-local minimum. Exact saddle seeds may
return STATIONARY with NEGATIVE; regular flats may be AMBIGUOUS; neither is
counted as an established minimum. All known invalid rows have NaN fields.
Final candidate distance must not exceed seed distance beyond the stated
evaluation/oracle error budget. Direct bad-kind/capacity/overlap/lifetime/device
and modeled fault checks attack different runtime faults.

Preserve old iteration-one qualification on CPU and both P100s. Exercise real
old-core and v1-only-plugin compatibility with preserved binaries, host
ASan/UBSan and Compute Sanitizer on each card. No parity-only geometry oracle
or required mutation quota is introduced.

## Measurement and caller

Measure native CPU 1/24 workers and each P100 separately using equivalent
requests/options/results. Benchmark orthogonal/tilted planes, nonlinear
cylinder projections and mixed iteration/boundary/status cases at 1,32,256,
4096,65536 and selected larger sizes where reasonable. Seeds differ from known
solutions in nonlinear throughput cases. Record stationary/unresolved/rank/
curvature counts, accepted-update and evaluator-call distributions, not merely
successful native return codes. No failed-query-only throughput is a solved
projection speedup.

Report first use; input generation; native snapshot/upload; public resident
project including fresh output allocation/sync/publication; download; prepared
native host-to-host including status consumption; cleanup; and Python packing/
dispatch/provenance separately. Native downloads reuse prepared destinations;
Python download allocates five owned arrays and copies retained host provenance.
Caller timing names whether target generation and file/visual export are inside
its region. The synthetic caller has no source-CAD export/import or automatic
fallback cost; report those as absent, not as an unmeasured speedup.
Optional CUDA events use separate profiled
contexts. Match the operation/precision/termination contract for CPU comparisons.
Seven samples after one excluded warmup initially; shared-host variation remains
visible. No other owned build/test job overlaps the published matrix.

The actual Python caller projects an independently generated off-surface point
cloud onto the cylinder and polynomial carriers, exports target/seed/candidate
records and stationarity/curvature statuses, and shows input-to-candidate segments.
It publishes qualified output and measured caller costs. It does not call the
point cloud a trimmed mesh, complete nearest-point map or accelerated Boolean.

## Review disposition

[Runtime review](../papers/2026-10-13-projection-design-runtime-review.md),
[numerical review](../papers/2026-10-13-projection-design-numerical-review.md) and
[independent oracle review](../papers/2026-10-13-projection-design-oracle-review.md)
completed and re-read the corrections. Factory error propagation, ABI2 checks,
scaled solves, gradient safeguards, relaxed curvature meaning and exact threshold
oracles are explicit above. No remaining design blocker was reported.
