# Projection plan: independent oracle review

2026-10-13. Reviewed `docs/ITERATION_2_PLAN.md` against the revised design and
the oracle/source studies. No blocker. No product or test implementation was
written during this review.

The plan preserves the first-order candidate contract. Saddles and regular
flat candidates can be STATIONARY with separate flags; rank refusal and bounded
solver failures remain explicit. It requires exact supplied-input geometry at
reported UV, retained target/seed/options/source association, meaningful native
and Python consumers, equivalent CPU/GPU workloads and visible failure counts.
The canonical runtime/header handoff precedes dependent implementation; there
is no second POD layout or registry.

## Qualification allowances and corpus

The distance allowance follows exact expansion of `||r+e||²`; the gradient
allowance follows expansion of `(r+e0)·(D+e1)`. The additional dot-product
rounding allowance is conservative for this small bounded arithmetic. Applying
the maximum gradient allowance to box rho is valid because clipping is
nonexpansive. Every reported candidate position is checked independently with
E0 at its actual binary64 UV. Stationary requires both reported rho within
tolerance and exact recomputed rho within tolerance plus its declared allowance.
These rules do not promote the empirical evaluator grades to universal bounds.

Known-UV checks are confined to named well-conditioned analytic fixture basins;
conditioned, flat and rank-refused cases instead check their geometry, distance,
rho and status. The admitted conditioned-chart non-nearest seed is explicitly
expected. The strict curvature-equality, weak multiplier, regular saddle,
quartic ambiguity and forbidden-corner eigenvector cases are direct independent
expectations. Their flags cannot silently become minimum claims.

The unit should preserve direct necessary counter bounds plus exact
initial-stationary/zero-budget/invalid-row cases, rather than recreating an
iterative reference solver. Existing public ABI and evaluator tests, runtime
fault injection and memory tools remain distinct mechanisms for their own fault
classes. Missing requested projection/backend capability must fail the new
qualification; genuine old-interface compatibility tests expect explicit
unsupported projection while still exercising evaluation.

## Concrete corpus improvement accepted by the integrator

**Reachable zero-backtrack line-search failure.** Use
`S=((2s-1)^3,t,0)`, degree `(3,1)`, unit weights, with the four cubic x controls
`(-1,1,-1,1)` extruded in t. Target `X=(2^-10,1/2,0)` and seed
`(33/64,1/2)` are exact dyadic inputs. At the seed:

```text
a=2s-1=1/32
Sx=1/32768, Ss_x=3/512, Sss_x=3/4
r_x=-31/32768
g_s=-93/16777216
Gss=9/262144
Hss=-177/262144
```

The regular metric passes admission; the full Hessian direction is not SPD.
The prescribed reduced Gram damping uses `mu=2^-20`, since the free t metric
is one. Its uncapped Gauss–Newton s step is `93/592`, below the quarter-step
cap, and has negative directional derivative. The initial trial's cubic x
coordinate is above `0.04`, far beyond target `2^-10`, so its exact distance
increases by a large margin. With `max_backtracks=0`, the expected termination
is LINESEARCH_LIMIT; seed/point/distance/rho remain retained, accepted updates
are zero and evaluations are two. This is one closed-form attack of the
specified initial trial, not another iterative solver predicting arbitrary
traces. It directly distinguishes line-search failure from stagnation and
stationarity. The integrator accepted this addition to the plan.

**Well-conditioned nonlinear parabolic throughput family.** The integrator
proposed `S=(a,t,a²)`, `a=2s-1`, and independent normal targets
`X=(a0-2*a0*lambda,t0,a0²+lambda)`, where `a0∈{-1/2,-1/4,1/4,1/2}` and
`lambda=1/16`. Expected UV is `((a0+1)/2,t0)`; expected squared distance is
`lambda²(4a0²+1)`. The residual at that UV is `(2*a0*lambda,0,-lambda)` and
is exactly normal to `(2,0,4*a0)` and `(0,1,0)`.

For arbitrary iterates, `Hss=4(1+6a²-2Xz)`. Here `Xz<=5/16`, so
`Hss>=3/2` throughout the rectangle; `Htt=1` and `Hst=0`. The objective is
strictly convex on this particular carrier/query family and has the named
unique solution. This gives a defensible fixture basin across the whole box,
with deliberately distinct dyadic seeds and known distance, while the product
still makes no global-nearest guarantee for arbitrary carriers. It is suitable
for independent solver qualification, throughput and the off-surface cloud
caller. The integrator accepted its explicit inclusion.

## Delivery disposition

The native timer scopes include fresh result allocation, completion and required
status consumption. Prepared native destinations and allocating Python
downloads have separate scopes; caller preparation, qualification and export
are named. Nonlinear seeds differ from solutions, and adverse outcomes remain
in the measured population with their distributions. Thus a fast rejection-only
batch cannot masquerade as useful projection throughput.

Proceed after the integrator records the accepted plan. The independent unit
owns `projection_fixtures.py`, `test_projection.py` and `qualify_projection.py`
as one complete fixture/public-caller qualification mechanism. The direct red
invocation should use the agreed runtime names and fail while the operation is
absent. No remaining geometry/oracle plan blocker.
