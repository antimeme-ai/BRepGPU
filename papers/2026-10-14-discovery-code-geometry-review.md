# Iteration3 implementation review: independent geometry lane

2026-10-14. Reviewed native/discovery_math.hpp and native/discovery_cpu.cpp
against the accepted three-mode design, exact original-input oracle and plan.
Also inspected benchmarks/discovery/fixtures.hpp as an independent model/truth
check. No native execution was performed in this lane; root owns fleet runs.
The numerical owner's independent review of our oracle implementation is in
[the separate oracle review](2026-10-14-discovery-code-oracle-review.md).

**Source disposition: no unresolved numerical or geometry defect found.**
This is a source argument and direct finite-corpus qualification, not a theorem
about arbitrary compiler/environment behavior. Runtime resource-fault review
and execution results remain separate evidence.

## Enclosure and original-input identity

CPU elementary RN operations expand each primitive with nextafter; CUDA uses
explicit directed FP64 operations. There is no final-padding substitute for
intermediate enclosure. Multiplication and positive-denominator division examine
all endpoint pairs; the query-only ratio lower chooses the smallest positive
denominator for negative numerator lower and the largest for nonnegative lower.
Negative zero has real value0; this branch choice remains sound. Signed/subnormal
zeros can widen CPU bounds but cannot justify false exclusion. The environment
inspector checks nearest rounding, binary64 width/evaluation, SSE and x87 masks,
FTZ/DAZ and fast-math; runtime owns actual-thread enforcement.

Dyadic splitting retains homogeneous channels and clips only mathematically
known envelopes. Child Cartesian bounds and DOP supports intersect parent
bounds. DOP directions match the canonical sign table, use integer norm2/3,
and add lower information to the summed AABB distance. A zero-width plane or
point is a valid bound, never treated as an empty volume.

Bernstein product factors enclose exact positive binomial ratios. Ordered
coefficient pairs compute C0=AA, C1..3=AW and C4=WW correctly. C0 global range
includes negative coefficients; C1..3 are signed. C4's positive[1,65536]
intersection is justified by the positive weight-product convex averages.
Only active K coefficients participate in query bounds; padding is zero.
The target norm and signed products are enclosed before reconstructing N;
clamping only the final minimum lower to0 and retaining AABB are sound.

Original-patch witness evaluation encloses parameter complements, numerator
and positive denominator before residual/square/sum upper arithmetic. Scalar
XYZ and projector distance remain descriptive. Refinement copies the returned
six descriptive fields only when its freshly enclosed upper improves, with
source UV/point/distance correspondence retained. Rank/budget/saddle outcomes
remain usable UV witnesses, and arithmetic inability clears candidate geometry.

CPU scalar and interval adapters both use the original record offset and
named patch degrees. Build node/roots/caches are per retained patch slot;
packed indices do not become pointer-defined source identity. Actual resident
cache download plus exact cell/source checks qualify CPU construction and
GPU storage together.

## Traversal and work semantics

The root gets one scalar evaluation and, when admitted, one projector call;
root leaves skip duplicate midpoint refinement. Callback counts sum initial
scalar plus actual projector attempts; interval certificate evaluations are
separate documented work. Budget0 still yields an initial valid witness.

Lower computation visits reserve both children before either is evaluated.
Near-first traversal breaks ties left-first; IDs/lowers are pushed together.
The depth12 build admits the fixed13-entry stack; unexpected overflow is an
arithmetic invariant failure. Pop verifies source/cell/index shape before
strict lower>upper pruning. Retained leaves contribute their lower regardless
of where a local refinement subsequently travels. Older pruned regions have
lower greater than an older upper, so cannot hide a point below the final upper.
Visit limitation returns root lower, covering every unvisited region without
pretending a partial leaf reduction is complete. Gap precision and completed
coverage are independently flagged. An all-pruned nonempty tree is an invariant
failure, which exposes equality pruning in the exact-zero CUDA fixture.

## Independent delivered-value model inspection

The native benchmark's targets/control values are dyadic and its analytic
minima fit binary64 exactly for the finite construction:

- Quarter-circle homogeneous controls produce A=(1-s²,2s), W=1+s² and z=t.
  Target=lambda*(1-s²,2s),z=t has radial norm lambda*(1+s²); its admitted angle
  is inside the arc and d*²=(lambda*(1+s²)-1)². Axial targets have d*²1 and
  continuum nearest UVs.
- Parabolic controls give a=2s-1,z=a². Normal targets
  (a0-2a0lambda,t,a0²+lambda), lambda1/16, have unique global distance
  lambda²(4a0²+1); their full-box scalar objective is strictly convex. Twin
  targets(0,t,3/4) have d*²1/2 by the independent square identity.
- Diagonal plane targets(s+1/4,s-1/4,t) have d*²1/8; the three-axis plane and
  target(1/4,1/4,1/4) have d*²3/16. Orthoplane offsets±1/2 have d*²1/4.

These exact minima qualify timed-row brackets. They do not alone qualify every
returned UV upper; the independent Fraction unit checks original-UV witnesses
and caches on its direct corpus. Numerical-owner benchmark findings request
an exact GAP-width check and independent expanded analytic candidate sanity.
Root is implementing them. Those descriptive sanity checks must stay distinct
from exact original-UV upper checks in standalone/caller qualification.
Counter/outcome populations and cache/setup cost accompany latency so stronger
bounds are compared by task value.

## Direct caller evidence currently known

Owned discovery_fixtures.py uses exact power restriction and polynomial
convolution, not production splitter/traversal or an iterative nearest reference.
Tests/test_discovery.py has15 public methods; standalone qualifier uses103 cases
and three modes. It preserves source bits, returned fields, downloaded cells,
cache counts, exact witness/lower expectations and every violation separately.
Raw rejection tests use actual allocated storage, not forged pointers or GPU
crashes. New callback device restoration and Python native-success ownership
failure paths extend existing direct mechanisms.

Root reports the complete standalone qualifier passed on CPU and both P100s:
103 cases,309 rows,11415 actual downloaded nodes per run,0 violations.
The native numerical check passed and constant-third public case passed CPU
and both P100s. Full public/sanitizer and compatibility results will be appended
after root supplies actual outcomes. No speedup or final hardware qualification
is inferred from source review alone.
