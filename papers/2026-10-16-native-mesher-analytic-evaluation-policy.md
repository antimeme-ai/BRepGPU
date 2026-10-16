# Analytic numerical evaluation: remove work before accelerating arithmetic

Scope: read-only study of the current owned native source. No implementation,
new benchmark, dependency or numerical-policy change is selected by this note.
The recommendation is to replace repeated general interval-jet evaluation with
compiled analytic residuals and cached derivative contracts, using fixed
interval arithmetic for the evaluations that remain. This preserves bounded
whole-interval acceptance; it is not sampled sagitta or a libm accuracy guess.

## What the actual source does

- `native/mesher/geometry_analytic.cpp::trig_at_stage` reduces each exact rational
  argument using an explicit-precision Machin pi enclosure, then calls rational
  `taylor` twice. Each call performs `bits/4+8` interval terms, arbitrary-rational
  products/divisions/compression and a rational remainder construction.
  GeometryContext caches pi by required precision, not argument values.
- `trig_range` computes that midpoint result and expands each side by half the
  argument width using the valid global derivative bound1. For radius>=2 it
  directly returns [-1,1]. The interval mechanism includes extrema, but narrow
  queries still pay the whole midpoint evaluation.
- `geometry_jets.cpp::enclose_analytic_curve_jet` computes position, first and
  second derivative vectors together. Surface jets similarly compute all six
  position/derivative vectors; cone angle trigonometry repeats for each query.
- `analytic_curve_chord_upper` first constructs the full interval jet, then two
  full endpoint jets. Only afterward does it compute the existing exact Gram
  spectral curvature bound and choose the smaller curvature bound. Consequently
  circles/ellipses pay trig even when the Gram bound already suffices.
- `cell_bounds.cpp::bound_analytic_cell` constructs one full rectangle jet to
  obtain three Hessian norms, plus three complete point jets for corner
  departures. It then uses its explicit Hessian/8 inequality.
- `geometry_correspondence.cpp::analytic_coupled` bounds the spatial curve and
  selected carrier trace independently, subtracts their second-derivative
  intervals, and repeats source-curve/carrier endpoint evaluations. Shared
  harmonic structure can therefore lose correlation before subtraction.
- `point_trig.cpp::try_sin_cos` already owns directed integer fixed arithmetic,
  exact reduction, generated 128/192/256-bit profiles and bounded optional
  fallback. It currently feeds canonical POINT only; public explicit jets and
  retained profile constants deliberately retain the older policy.

These are source-level avoided-work opportunities, not a measured attribution
of a particular censored component.

## The substantive alternative: compile the residual first

For the existing analytic-carrier branch of `analytic_coupled`, selected UV is
an affine function of original spatial parameter t on each admitted segment:
u=a+b*t, v=c+d*t, with exact rational a,b,c,d derived from the retained trace.
The original spatial line/circle/ellipse also has a finite analytic expression.
Construct R(t)=C_original(t)-S_original(u(t),v(t)) BEFORE interval evaluation.

Use a small owned normal form with an affine polynomial term and vector
amplitudes multiplying sin(phi+omega*t) / cos(phi+omega*t). Keys contain exact
rational frequency and phase. Cylinder uses one angular frequency; sphere and
torus products reduce by exact product-to-sum identities to a finite set of
sum/difference frequencies; cone amplitudes have polynomial degree at most1.
All original nonorthogonal frame axes, origins and radii remain exact input
coefficient words. Cone sin(alpha)/cos(alpha) are bounded original-source
constants, not rounded replacement geometry.

Combine identical keys and exact coefficients first. Normalize negative
arguments with exact sine/cosine parity. Only apply periodic equivalence when
there is an explicit exact symbolic period identity; nearby dyadic phases,
rounded pi, matching doubles or nearly matching frames do not establish one.
A rounded imported meridian axis leaves its actual coefficient mismatch rather
than being silently promoted to the analytic carrier axis.

A circle-on-cylinder with identical source coefficients and u=t then cancels
as an expression, without evaluating either trigonometric curve. The same
applies to matching sphere meridian harmonics. Near-coincident imported forms
leave small actual coefficient residuals, so their WHOLE residual can often be
bounded without subdividing the interval or calling trig. If original and
selected expressions are not in this finite admitted class, retain the existing
rational net residual or general enclosure path; never reinterpret a nonlinear
pcurve as an affine trace.

For a paired vector term A*cos(theta)+B*sin(theta), its norm is at most the
spectral norm of [A B], independent of theta. The owned exact two-column Gram
bound already supplies that mechanism. Bound degree1 amplitudes over the exact
finite t interval; sum term bounds and the polynomial residual bound. When that
upper is too loose, evaluate the SAME residual normal form with local intervals
or derivative enclosures. Do not first destroy cancellation by evaluating C and
S independently again. An upper exceeding tolerance is an insufficient bound,
not a proof that the geometry error exceeds tolerance.

## Closed-form contracts replace unnecessary jets

For C(t)=O+A*cos(t)+B*sin(t), sup||C''|| is bounded by ||[A B]||2. A line has
C''=0. The existing whole-chord inequality remains

`max(endpoint departures) + sup||C''|| * (t1-t0)^2 / 8`.

Compute/cache that source-only Gram quantity once. Reuse endpoint source and
actual-XYZ departure enclosures already obtained for recovery/canonical point
construction, keyed by the actual source parameter and stored XYZ words. Try
the cached bound before requesting a tighter local jet. This removes repeated
endpoint/trig work without claiming a sampled chord test is sufficient.

For surfaces let E=X*cos(u)+Y*sin(u), F=[X Y Z], C>=||[X Y]||2 and K>=||F||2.
Obtain C by the owned two-column Gram method; a rational Gershgorin bound on
F^T F followed by the owned outward square root gives K without assuming an
orthonormal frame. Useful valid global second-derivative norm bounds are:

| Carrier | ||S_uu|| | ||S_uv|| | ||S_vv|| |
|---|---:|---:|---:|
| Plane |0|0|0|
| Cylinder radius r | abs(r) C |0|0|
| Cone rho=r0+v*sin(alpha) | max abs(rho) C | abs(sin(alpha)) C |0|
| Sphere radius r | abs(r) K | abs(r) K | abs(r) K |
| Torus major R, minor r | (abs(R)+abs(r)) C | abs(r) C | abs(r) K |

The sphere bounds follow because the coefficient vector for each listed
second derivative has Euclidean norm<=1. For cone, cache the constant-angle
interval once and derive max|rho| on the requested v range. Local certified
trig can tighten these bounds if necessary. Feed the SAME actual-cell Hessian
inequality and actual corner departures; do not evaluate newly sampled S as
virtual physical corners. A closed-form bound is a valid bound, but replacing a
previous tighter bound can change refinement and must be an explicit policy.

## Arithmetic for what still needs evaluating

First contender: reuse the owned fixed integer sin/cos evaluator behind a new
explicit jet/profile backend, with an exact input enclosure and deterministic
128/192/256 stage selection. Bound an argument interval using proved midpoint
values plus Lipschitz expansion initially; certified critical-point subdivision
can later tighten ranges. At each source expression stage, keep bounded dyadic
intervals or the existing guarded binary64 interval primitive rather than
promoting every intermediate to a normalized general Rational. Original source
coefficients enter through exact-to-interval conversion. Overflow, inadequate
range reduction, denominator containing zero or an inconclusive decision takes
the owned exact/adaptive path; it is not success.

The binary64 primitive already states nearest mode, FLT_EVAL_METHOD==0 and no
FTZ/DAZ on host, with explicit directed CUDA intrinsics. Those guards, subnormal
handling and outward operations remain mandatory. Ordinary libm sin/cos is not
an enclosure and cannot be accepted by assuming a number of ulps.

The fixed primitive can preserve the source and whole-interval guarantee, but
its endpoints need not equal the old rational Taylor endpoints. Persisted bound
constants affect adaptation/output, so this is a reviewed numerical-policy
revision, not a transparent change justified by the point-only result. Keep
canonical nearest-even XYZ unchanged. Select one deterministic common host
profile/normal-form recipe; CPU/P100 filters answer that task's acceptance
contract and exact fallback resolves uncertainty. Neither task completion order
nor which device produces a tighter enclosure selects the output mesh or
serialized constants. No second host check of every successful GPU decision is
proposed.

## Cache identity and lifetime

Source-only profiles key original GeometryStore/source generation, curve or
carrier ID, original coefficient identity, and explicit numerical-policy /
precision. Query enclosures additionally key exact argument interval, expression
or residual identity, chart/branch map and requested precision. Endpoint
residuals also key the actual XYZ words and use-specific recovery where relevant.
Never reuse by rounded UV, approximate spatial proximity, incidental
GeometryContext.bits(), a different source generation or a higher cached
precision changing a lower-precision result. Per-owner mutable bounded caches
may reference immutable prepared profiles; eviction changes cost only. Shared
caches hold no mutable Budget or owner-local Rational references.

## Decision and limits

Prefer compiled residuals + closed-form source bounds, followed by fixed
interval local evaluation and existing exact fallback. Merely substituting a
faster Taylor implementation in every full jet leaves duplicate work and lost
correlation intact. A double sampled curvature estimator is cheap but does not
preserve the current whole-curve/whole-cell guarantee; it can at most propose
subdivision and never authorize publication.

Implementation, if selected, should compare complete trace/face preparation on
independent circle/ellipse/meridian, nonorthogonal sphere/cone/torus, phase-shift,
near-cancellation and out-of-profile inputs; derive direct analytic residual
oracles and keep a general-path control. Existing real-component runs are the
whole-operation control. This note runs none of those experiments and states
no numerical runtime SLA or new mesher completion claim.
