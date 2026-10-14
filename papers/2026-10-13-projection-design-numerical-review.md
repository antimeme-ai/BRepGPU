# Resident projection design: numerical review

Reviewed `docs/ITERATION_2_DESIGN.md` against the [numerical source study](2026-10-13-projection-numerical-study.md), with emphasis on active bounds, curvature/rank, stable solves and distance acceptance, exact returned-UV metrics and finite work. This review changes no product code. The first draft has three implementation-critical numerical specification omissions; all were sent to the integrator promptly.

Reviewed design source SHA-256: `2627b7052d1e6c6c4378d1682a3cd65249369619a25816d98d2c4310b9d69db5`.

## N2-1 — define the normalized Cholesky pivots and RHS

**Moderate; clarify before implementation.** “Scaled 2×2 Cholesky with pivot floor `2^-40*metric_scale`” is ambiguous unless the design says whether these are original diagonal/Schur quantities, normalized quantities, or square-root factor pivots. Applying the scaled floor to an already normalized matrix double-scales the screen; comparing a square-root pivot to a matrix-entry threshold changes the admission rule. “Same scaled pivot discipline” in the direction solver propagates the ambiguity.

Specify `m=max(1,||M||_infinity)`, normalized entries `a=M00/m,b=M01/m,c=M11/m` and normalized RHS `h=-g/m`. Compare `a` and the computed normalized Schur diagonal `c-b*b/a` to `2^-40` before taking square roots. Solve the normalized system with that RHS, or specify an algebraically equivalent unscaled rule that compares original diagonal/Schur quantities against `2^-40*m`. State the one-coordinate case too. Preserve finite checks and the nonpositive-versus-positive-small rank distinction. Matrix scaling and derivative arithmetic must not use unsafe reassociation or fast reciprocal shortcuts.

The direct rule oracle uses a regular affine plane under allowed control scaling and a condition family crossing the declared pivot floor. A scalar control-net amplitude scales `G` quadratically; normalized and unscaled implementations must make the same admission decision. Include large admitted metric entries so a mistakenly double-scaled threshold cannot hide behind small fixtures. This is an implementation contract test, not a request for a universal condition theorem.

## N2-2 — the gradient safeguard's scalar is unspecified

**Moderate; clarify before implementation.** The draft says “a scaled negative-gradient direction” but does not define its scalar. A unit gradient, a raw negative gradient and a curvature-scaled gradient can produce different trial chords, iteration counts and unresolved fractions even after the common quarter-square cap. That leaves the shared CPU/CUDA outer algorithm and its benchmark comparison underspecified.

Specify the direction before the common cap, for example `p=-g/max(1,||G||_infinity,||H||_infinity)` on unfrozen coordinates, with fixed coordinates zero. Then apply the same common infinity-norm cap, endpoint projection and actual displacement descent test. A different deliberate scalar is acceptable; it must be written once and used by both backends.

Use an affine plane or curved patch where the full-Hessian proposal is unavailable or clipping removes descent, while a scaled projected-gradient displacement exists. The expected fallback direction/chord follows directly from supplied `g,G,H`, and its resulting evaluation/update counts should agree with the written cap rules. Avoid adding a second search silently: this design explicitly permits one chord search, so a rejected Newton/GN chord terminates unresolved rather than invoking the research study's optional second gradient search.

## N2-3 — robust curvature positivity needs an eigenvalue threshold

**Moderate; clarify before implementation.** The curvature threshold is meant to separate robust signs from an ambiguous band, but “robust SPD reduced matrix” does not identify its mathematical test. Reusing the direction solver's Cholesky pivot test with the curvature threshold is not equivalent to requiring the smallest eigenvalue to exceed that threshold. Coupled matrices expose the difference.

Define `POSITIVE` for a nonempty reduced space as `lambda_min(H_reduced)>delta_H`, equivalently positive definiteness of `H_reduced-delta_H*I`, with a stated numerical evaluation rule. Without weak bounds, `NEGATIVE` requires `lambda_min<-delta_H`. With weak bounds, a coordinate witness must be an admissible critical coordinate and satisfy its diagonal quadratic form `<-delta_H`; the remaining sign band or unsupported weak-boundary cone assessment is `AMBIGUOUS`. The empty strictly active critical space is the separately documented vacuous positive case. This threshold is independent of the `2^-40` direction-solve pivot floor.

Direct exact fixture: `a=2^-10`, affine plane `S=(s+t-1,a*(s-t),0)`, target zero, seed `(1/2,1/2)`. Canonical degree `(1,1)`, weight-one Cartesian controls are `(-1,0,0),(0,-a,0),(0,a,0),(1,0,0)`. The regular chart has

```text
H = G = [[1+a², 1-a²], [1-a², 1+a²]]
||H||_infinity = 2
lambda_min = 2*a² = 2^-19 = delta_H
```

Both ordinary Cholesky Schur pivots exceed `delta_H`, but a strict eigenvalue-threshold classification is ambiguous. Testing `H-delta_H*I` gives the expected zero eigenvalue. The independent oracle owner received this fixture. It uses exact supplied binary inputs and attacks the displayed classification rule directly.

## Design decisions that are coherent

Rank admission precedes the stationarity and update-budget tests. Thus constant, curve-like and numerically unresolved charts cannot pass merely because their gradients vanish, including a zero-update request. The documented floor is a numerical policy in local coordinates, not a proof of geometric singularity or an inverse-error bound. The rank distinction between nonpositive and positive-but-small pivots is meaningful once N2-1 makes the pivot definition explicit.

Only exact-bound outward-gradient coordinates are fixed during direction construction. A near-bound coordinate remains free; an exact bound with an inward gradient is released. Removing these components before solving the reduced system avoids using a clamped unconstrained plane solution as the final answer. The actual projected endpoint displacement must still descend; the positive-definite unconstrained direction alone does not guarantee that property after clipping.

The curvature subspace differs correctly from the direction-construction set: only clearly strict outward multipliers are removed, while weak active coordinates remain one-sided critical directions. The weak-corner `S=(s,t,st),X=(0,0,-2)` attack cannot obtain a negative flag merely from its forbidden full-Hessian eigenvector. The conservative coordinate-witness rule can leave legitimate boundary curvature ambiguous; that is expressly permitted. Empty strict critical spaces have a vacuous diagnostic, not a computed positive full Hessian.

`STATIONARY` is explicitly a first-order candidate status. A regular exactly seeded saddle may carry `NEGATIVE`, and a regular quartic flat may carry `AMBIGUOUS`. Neither becomes an isolated-minimum or nearest-point claim. Metric admission and curvature classification are separate, so flat objective curvature does not imply a rank-deficient chart.

The distance safeguard uses the stable evaluated-point difference identity and the actual rounded trial UV displacement. Negative change and Armijo are both required; no positive fudge admits worsening distance. The feasible chord prevents a projected-path kink from invalidating the chosen directional derivative. The final position, distance, gradient and residual come from a full jet that names the exact returned UV. A cache is legitimate when that UV is unchanged; an old seed/trial jet is not. Small motion/objective change cannot replace the residual test.

Arithmetic/rank failures precede apparent stationarity and budget exhaustion. Every accepted update obtains its full jet before checking those conditions. An accepted position trial followed by a failed full-jet evaluation therefore produces the explicitly unusable arithmetic output, rather than a point with stale derivative metrics. Limits/search/stagnation/rank otherwise retain only the accepted finite candidate; rejected trials do not replace it.

## Counts, scope and qualification limits

This design narrows the source study's proposal to one chord search, at most 128 updates, and `max_backtracks` counting halvings **after** an initial full trial. A zero-backtrack option still tries `alpha=1`; the largest search has 25 trials. The stated maximum is therefore `1+128*(25+1)=3,329` evaluator callbacks. Skipped zero-displacement trials do not invoke callbacks. Initial full evaluation counts once even when the update cap is zero; cached terminal reporting adds no hidden call. These choices are internally consistent and should be repeated in the implementation plan rather than inheriting the study's alternative two-search bound.

Input target/seed/domain and per-row status precedence are explicit. Arithmetic failures have unusable numeric fields, while other unresolved numerical outcomes have a finite accepted candidate. Classification/status/candidate interpretation must remain distinct in the Python guard. Reported residual tolerance and matrix sign thresholds are observed numerical criteria; the source-study error-propagation discussion does not turn iteration-one empirical error budgets into runtime enclosures.

The one-request/thread bounded kernel, local scratch, independent CPU/GPU jet algorithms and compact final output fit the researched first scope. The resource/stack usage and mixed-iteration distribution remain measured facts to establish after implementation. Equivalent targets/seeds/options/status consumption, unresolved counts and separated resident/transfer/Python timing are appropriate. No performance or full-surface closest-point claim follows from this design review.

## Disposition

N2-1 through N2-3 await explicit definitions and re-review. No other numerical blocker was identified in the current draft. Product implementation remains downstream of accepted design and plan review.

## Integrated design re-review

Re-read revised design SHA-256 `06b5fb853e4df4fe79dcc018b379c4d727c074cb59384cbb5dcd0adf410ebcf2` and the added oracle dispositions. **All three numerical findings are resolved; no remaining numerical design blocker found.**

- **N2-1 resolved:** reduced matrices and RHS are normalized by the same scale. The normalized matrix diagonal and Schur diagonal are compared against `2^-40` before square roots; one-dimensional solves use the normalized diagonal. The original metric-floor wording is equivalent to that normalized rule. Failed direction arithmetic proceeds to the next safeguard while valid current jets remain available.
- **N2-2 resolved:** the negative-gradient safeguard uses the full-matrix scalar `gamma=1/max(1,||G||_infinity,||H||_infinity)` before the common cap and projection. GN damping separately uses the reduced Gram norm. The design explicitly fixes arithmetic contraction, dot accumulation and rounded trial construction. It requires the actual trial dot product to be negative before position evaluation. The deliberate single-search scope and maximum 3,329 callbacks remain unchanged.
- **N2-3 resolved:** curvature positivity requires the smallest reduced eigenvalue strictly above the curvature threshold, with equality ambiguous. The negative threshold and admissible weak-coordinate witnesses are explicit. The two-dimensional eigenvalue formula is specified, separate from direction-solve pivots. The exact coupled-plane threshold fixture is now mandatory qualification.

The additional tolerance-relaxed cone wording is important: an outward multiplier between zero and the gradient tolerance can be treated as weak, so a negative relaxed directional-curvature flag is not a mathematical verdict that the exact constrained candidate is a nonminimum. The newly required weak-bound fixture directly checks this distinction. Interior saddles, flat regular charts and strict/weak boundary activity retain their separate meanings.

The revised qualification also explicitly includes a conditioned regular chart that passes the metric floor and meets the projected-gradient tolerance at a seed whose UV differs from the nearest UV. This records the actual first-order predicate's limits instead of silently requiring universal parameter accuracy. Exact returned-UV geometry/residual checks and stable-basin known-solution checks remain complementary direct oracles.

Flag values and combinations are now concrete: candidate validity and exact boundary location are independent, rank flags belong to rank-unresolved rows, curvature flags are exclusive and stationary-only, and unusable invalid/arithmetic rows clear flags. Initial-trial and no-empty-trial conventions remove the zero-backtrack ambiguity. Design approval is supported from this numerical lane; implementation still follows the separately reviewed plan.
