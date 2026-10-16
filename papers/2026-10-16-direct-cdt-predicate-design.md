# Exact predicates for an ambitious native constrained triangulator

This is a researched design recommendation, not an implemented predicate or a
performance result. The operator's requirement is to reach for strong methods
before selecting an implementation. A direct constrained Delaunay constructor
and a fixed-input connectivity improver should share the predicate machinery
below; an interpreter-to-C++ translation alone is not the proposed endpoint.

## Selection and literature

Select a **filtered exact predicate architecture** from the outset. Ordinary
floating-point evaluation handles well-separated signs, increasingly precise
enclosures handle uncertain signs, and owned integers resolve the remainder.
Exact zero is an actual geometric result. No epsilon decides topology.

Read in the retained primary sources:

- Shewchuk, *Adaptive Precision Floating-Point Arithmetic and Fast Robust
  Geometric Predicates*, DCG 18, 305–363, 1997: sections 2, 4 and 5 explain
  error-free expansion operations, adaptive determinants and their arithmetic
  assumptions. The paper explicitly leaves general exponent-range handling
  outside its arithmetic model. [Author's paper/code page](https://www.cs.cmu.edu/~quake/robust.html).
- Attene, *Indirect predicates for geometric constructions*, CAD 126, 102856,
  2020; retained arXiv revision v2, 2025-01-23: sections 4 and 5 supply the
  relevant construction-error, denominator-sign and cache reasoning.
  [Pinned revision](https://arxiv.org/abs/2105.09772v2).
- Bartels, Fisikopoulos and Weiser, *Fast Floating-Point Filters for Robust
  Predicates*, arXiv:2208.00497v1, PDF dated 2022-08-02: sections 2–4 study
  underflow-aware error bounds, fewer runtime branches and polynomial filter
  generation. Its input model is floating point; our integer-enclosure errors
  still have to be included. [Pinned PDF](https://arxiv.org/pdf/2208.00497v1).
- Shewchuk's [Triangle numerical discussion](https://www.cs.cmu.edu/~quake/tripaper/triangle6.html)
  connects predicate difficulty to triangulation order: alternating cuts avoid
  some near-collinear tests induced by tall thin merges. Predicate costs and
  algorithm choice interact; a robust predicate does not repair a poor work
  schedule.

These works motivate architecture and contenders. Their reported timings do
not predict our workload, exact rational inputs, CPU or memory behavior.
The integer normalization, scaling identities and enclosure proposal below are
our derivation. No reference implementation is selected as a dependency.

## Exact mathematical object

Retain original exact UV coordinates and source identities. An immutable
preparation represents them losslessly as `(Xi/Du, Yi/Dv)`, with common positive
axis denominators. The actual existing finite positive binary64 metric values
`mu,mv` are interpreted exactly, rather than recomputed or rounded again.

For positive integer weights choose

```
Rx = exact(mu)^2 / Du^2
Ry = exact(mv)^2 / Dv^2
W  = lcm(denominator(Rx), denominator(Ry))
wx = W*Rx; wy = W*Ry
```

For `i=a,b,c`, put `dxi=Xi-Xd`, `dyi=Yi-Yd` and
`qi=wx*dxi^2+wy*dyi^2`. The required sign is

```
I = qa*(dxb*dyc-dyb*dxc)
  + qb*(dxc*dya-dyc*dxa)
  + qc*(dxa*dyb-dya*dxb).
```

For positively oriented `abc`, `I>0` means `d` lies inside its metric-space
circle; zero means cocircular. The ordinary Euclidean incircle determinant in
coordinates `(mu*u,mv*v)` equals `mu*mv*I/(W*Du*Dv)`, a positive multiple.
Independent axis denominator scaling preserves orientation but **does not**
preserve circles. Unweighted integer incircle would be wrong. Existing bridge
distance weights `Dv^2,Du^2` are a different object and must not be reused here.

Orientation uses `(Xb-Xa)*(Yc-Ya)-(Yb-Ya)*(Xc-Xa)` with no metric required.
For constraint recovery, point-on-segment additionally needs exact coordinate
order; midpoint or intersection constructions must carry their own exact
representation. A future crossing-segment arrangement cannot round newly
constructed rational intersections and treat them as original binary64 inputs.
The initial admitted polygon domains contain no crossing constraints, avoiding
that construction in the first direct CDT unit.

## Normalization without changing geometry

Subtract one exact input anchor `(X0,Y0)` in the predicate view. This removes
large irrelevant translations before approximating coordinates. Never perform
that subtraction after rounding coordinates to double.

Let `x=(X-X0)*2^-kx`, `y=(Y-Y0)*2^-ky`, choosing integer exponents from actual
integer bit lengths so `|x|,|y|<=1`. Define

```
ax = wx*2^(2*kx-kL)
ay = wy*2^(2*ky-kL).
```

Choose `kL` so `0<ax,ay<=1` mathematically. The computed determinant on `x,y`
with weights `ax,ay` equals `I*2^-(kx+ky+kL)`; its sign and zero are unchanged.
Orientation receives the positive factor `2^-(kx+ky)`. Exponents are metadata,
not an instruction to allocate enormous shifted integers. Extreme anisotropy
can make one floating enclosure tiny or uninformative; fall back to integers.

Do not convert an arbitrary integer to double with an unchecked cast. For
nonnegative integer `N`, extract an at-most-53-bit leading significand `q` and
discarded-bit count `t`, retaining whether any discarded bits are nonzero.
Then `q*2^t <= N <= (q+1)*2^t`, with equality at the lower endpoint when the
discarded part is zero. Combine `t` with the normalization exponent before
constructing binary64 endpoints. Normal endpoints are exactly representable;
when the endpoint enters the subnormal range, round it outward by inspecting
the omitted bits. A value smaller than the least subnormal receives a valid
`[0,least_subnormal]` enclosure. Negatives swap/negate endpoints. Zero receives
`[0,0]`. Avoid double rounding and avoid overflowing a temporary `2^t`.
This gives an enclosure of the **original integer expression**, not merely an
approximation of its rounded binary64 image. Weight enclosures use the same
routine and explicit exponents.

## Selected cascade and contender boundaries

1. **Face-scoped scalar filter.** Cache normalized point approximations and
   certified conversion radii. Derive a single outward upper error bound `E`
   over the actual face ranges for each fixed expression DAG. Evaluate that
   scalar expression with its declared operation order and return a sign only
   when `abs(value)>E`. Include all input-enclosure uncertainty and every
   arithmetic rounding step. This can be evaluated with a single final sign
   test in the common case. No borrowed orient2d constant covers our weighted
   determinant and constructed inputs.
2. **Local dynamic filter.** Evaluate the determinant with outward intervals
   from cached point/weight enclosures. Return positive only for `lo>0`,
   negative only for `hi<0`. Intervals touching zero are uncertain. Recompute
   with locally translated square lifts when an anchored lift cache produces
   loose subtraction; neither path may drop input error. Compare local and
   cached variants on the whole operation before retaining both permanently.
3. **Exact fallback.** Evaluate the integer determinant. This resolves signs
   at arbitrarily small separation and exact zero. Resource exhaustion is a
   distinct error, never zero or a retained diagonal presented as an answer.

The static bound in stage 1 has a concrete construction: attach an absolute
magnitude bound and an error bound to every node of the fixed arithmetic DAG.
Leaf errors are conversion radii. For sum/difference, add child errors and an
outward bound on that operation's rounding. For multiplication use
`|a|*rb + |b|*ra + ra*rb + round_error`, where `a,b` are approximate values
bounded over the admitted ranges and `ra,rb` bound their errors. Apply the
rules to the whole DAG once when preparing the context. This can be looser
than a local semi-static filter, which is why stage 2 exists. A more aggressive
semi-static formula derived using the Bartels/FPG approach remains a direct
contender; its own bound must include the leaf-construction errors. We do not
need a general expression compiler to own two reviewed determinant formulas.

For these rules, an outward arithmetic rounding term can conservatively use
`u*M + sigma`, where `u=2^-53`, `sigma` is the least positive subnormal and
`M` bounds the magnitude of that operation's exact result on its approximate
operands. Compute the term and accumulated error outward; do not round a
half-subnormal correction to zero. The bound must cover approximate operand
ranges, including conversion endpoint overshoot, rather than assuming rounded
values obey the original exact range unchanged.

For the first outward interval implementation, ordinary round-to-nearest
operations followed by `nextafter` toward each required bound are simple and
reviewable. Addition/subtraction select the monotone endpoint pairs;
multiplication encloses the four endpoint products. All finite scaled inputs
are bounded near unity, so these short expression DAGs stay far from overflow.
One adjacent representable step encloses roundoff, including gradual underflow.
This costs more instructions than the scalar stage. Directed-rounding blocks,
specialized sign-aware interval products and SIMD batches are contenders, not
requirements to import a general interval library. If a rounding mode is changed,
restore it on every path and account for its cost and compiler semantics.

A further adaptive contender is to form the six query differences exactly in
integers, then enclose and normalize those differences locally before multiplying.
It avoids the uncertainty caused by subtracting two globally rounded nearby
coordinates, without paying for full arbitrary-precision determinant products.
Reweight both axes consistently after query-local scaling. This can be valuable
for a face with very different global and local feature scales; compare its
integer subtraction/enclosure cost with ordinary dynamic-filter failures before
retaining another stage. A face-wide scalar filter alone is not expected to
resolve every small local triangle.

## Exact arithmetic should also eliminate work

The existing owned signed base-2^32 integer has add/subtract/multiply and budget
accounting. Plain arbitrary-integer per-query translated lifts are a useful
correctness baseline, not the desired common path.

For immutable points and metric, cache exact anchored lifts

`Li=wx*(Xi-X0)^2+wy*(Yi-Y0)^2`.

Use `Li-Ld` in the third determinant column. It differs from the translated
square lift by
`2*wx*(Xd-X0)*dxi + 2*wy*(Yd-Y0)*dyi`. Adding multiples of columns one and two
to column three leaves the determinant unchanged, including exact zero.
This reduces a straightforward query from roughly 21 integer multiplications
to 9 after about 4 per prepared point. That is an arithmetic count, not a
runtime ratio; larger cached operands or allocation can offset it. Cache both
the exact lift and, where worthwhile, its certified floating enclosure. Source
translation is removed before constructing either.

Also consider a checked fixed-width integer specialization: if actual operand
bit bounds prove every intermediate fits signed 128-bit arithmetic, the whole
query can avoid allocation. For example, if every delta satisfies
`|dx|<2^bx`, `|dy|<2^by`, and weights satisfy `wx<2^bwX`, `wy<2^bwY`, then

`|I| < 2^(max(bwX+2*bx,bwY+2*by)+bx+by+4)`.

An exponent at most 127 is a sufficient conservative signed-128 bound for the
translated expression, including intermediates. Orientation similarly has
`|O|<2^(bx+by+1)`. Derive bounds from actual inputs and checked integer
metadata, not expected CAD dimensions. The compiler-specific 128-bit type
needs an explicit implementation boundary under the current pedantic profile.
Fallback remains arbitrary precision. Measure whether this check belongs
before the floating cascade for an entire eligible face; do not add a branch
to every predicate without an operation-level gain.

Expansions remain a serious contender for a restricted exactly dyadic input
path or carefully constructed integer expansions. Arbitrary rational UVs and
metric normalization are not generally represented by one binary64 per input.
Passing rounded UVs into an exact-on-double expansion implementation proves
the wrong sign. Large arbitrary integers can also exceed the finite exponent
range of unscaled expansions. An expansion stage earns adoption only with its
input representation, range treatment and whole-call costs made explicit.

## Required arithmetic and cache contracts

- Binary64 radix two with 53-bit significand; nearest-even rounding, gradual
  underflow, no flush-to-zero or denormals-are-zero. Unsupported runtime state
  disables floating filters and uses exact arithmetic. Do not silently change
  an application's floating environment permanently.
- No reassociation, reciprocal approximation, implicit FMA contraction or
  excess-precision evaluation. The current Clang profile already disables
  fast math and contraction. Explicit FMA is an optimization candidate only
  with a bound derived for that expression; a compiler introducing FMA can
  change an error argument even if its arithmetic is individually more accurate.
- Reject NaN/nonpositive/nonfinite metrics at admission. Any unexpected
  nonfinite enclosure result falls back to exact arithmetic or reports an
  internal error; it never certifies a sign. Strict comparison with zero
  prevents `0==error_bound` from certifying an underflowed determinant.
- Bind caches to immutable point values, denominators, original identity map,
  exact weights, anchor and scaling exponents. A refinement insertion creates
  or extends a context through an explicit owned operation; it cannot mutate
  points behind an existing lift cache. Metric changes rebuild weighted caches.
- Preserve exact cocircular zero. A direct constructor may select any valid
  diagonal among ties under its declared deterministic schedule, while an
  improver may retain the input diagonal. Symbolic perturbation would be a
  separately specified topology policy, not a numerical fix.

## Oracles and performance questions

Use independent Python `Fraction`/integer expansion of the original UV/metric
formula for exact signs, rather than another call to native helpers. Include
hand-derived inside/outside/zero cases, permutation sign identities, exact
anisotropic examples, huge translated coordinates, independently large
denominators, positive/negative cancellation, and quantities below binary64's
subnormal range. Explicitly distinguish rational geometry from the rounded
double geometry whose sign can differ. Test integer-enclosure inclusion
directly against exact rational endpoints. Test filter failure as a permitted
result and every accepted sign against the oracle; test budget errors as errors.

For actual triangulations, predicate parity is insufficient. Check positive
triangles, exact original-domain coverage, constraint recovery, incidence and
local weighted Delaunay legality on every unconstrained flippable edge. Exact
cocircular ties must not require an identical diagonal. These are independent
of the watch and remain valid for concave boundaries, holes, long strips,
regular cocircular grids and strongly anisotropic metric inputs.

Measure preparation/enclosure/lift bytes and time, predicate counts by stage,
filter hit/failure by geometry family, exact limb work, and complete construction
time/RSS. Include adversarial near-degenerate and dynamic-range families where
filters mostly fail. A high filter-hit rate does not establish fast topology;
a low rate can reveal a construction order that manufactures hard predicates.
The watch is one retained consumer, not a numerical contract or tuning family.

## Retained literature acquisition

The Shewchuk and Attene PDFs were already retained in `papers/pdfs/`; this
study read their local text and checked the primary author/arXiv pages.
New retained source, acquired 2026-10-16:

```
papers/pdfs/bartels-fisikopoulos-weiser-2022-fast-floating-point-filters-2208.00497v1.pdf
SHA256 34d5dc537499cd61814c4b9b80d0b6196f08330a468a034decc6bab79632d8f1
curl -fsSL https://arxiv.org/pdf/2208.00497v1 -o papers/pdfs/bartels-fisikopoulos-weiser-2022-fast-floating-point-filters-2208.00497v1.pdf
```

The arXiv HTML rendering displayed a different date during this reading. The
versioned original PDF above, whose title page says 2022-08-02, underpins this
study. The PDF remains ignored; this note records restoration and hash.
