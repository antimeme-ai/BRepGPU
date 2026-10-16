# Canonical point trigonometry filter

Status: proposed numerical subunit for review. Production integration remains
under the accepted whole native mesher plan. The persistent numerical service
has integration priority; this proposal does not hold that work.

## Contract and placement

The filter encloses sine and cosine of an arbitrary owned Rational argument.
It is attempted only while finding the correctly rounded binary64 coordinates
of a canonical original-source point. A result becomes a point only when the
existing shared source-formula evaluator proves that every final coordinate's
two enclosure endpoints round to the same IEEE word. Otherwise a wider fixed
profile is attempted, then the existing exact-Rational point path runs.

Public enclosure queries, explicit-precision jets, metric/profile construction,
and their cache-history-independent constants keep their present arithmetic.
In particular, the filter must not be inserted into `trig_at_stage`, which
also serves those queries. The geometry owner retains one source-formula body;
a private finite point-only trig provider selects the filter or existing path.
Both analytic convenience entry points and real GeometryStore canonical-point
dispatch use this same point policy. No geometry callbacks or second evaluator
are introduced.

## Selected arithmetic

Use endpoint precision profiles P=128,192,256 with fractional arithmetic width
W=P+32. Signed fixed arrays of 32-bit limbs hold an integer headroom limb plus
the fractional limbs. Add/subtract are exact with checked overflow; multiplication
uses the full twice-width unsigned product and sign-aware directed truncation.
Discarded nonzero bits determine floor/ceiling, including negative operands.
Interval products take all four endpoint products; square includes zero where
needed. Polynomial evaluation and angle reconstruction perform no Rational GCD.
Budget work and cancellation checks cover each bounded limb operation. Capacity
or range uncertainty yields unresolved; resource/cancellation failure propagates.
This integer path has no floating environment, libm, FMA, FTZ, or excess-precision
assumption.

The first profile admits |x|<=2^128. The argument can have an arbitrary rational
denominator: outward conversion is an owned Integer division, not a dyadic-input
assumption. Larger arguments retain the existing exact path. A universal 512-bit
pi floor/ceiling bracket is derived using the owned Machin alternating series.
At width W and argument integer-bit bound B, use its prefix R=W+B+8 only when
R fits the caller's total transcendental working cap. No hidden 512-bit working
allocation is charged to a smaller precision policy.

An exact integer quotient locates k near 2x/pi. Unique nearest k is unnecessary:
compute the outward interval r=x-k*pi/2 and require r inside [-1,1]. The identity
holds for any integer k; this explicit range check establishes the polynomial
hypothesis. The pi bracket and integer products enclose reduction error even
near quadrant boundaries. Quadrant reconstruction uses exact signed k modulo4.

Choose j near 32*mid(r), with j in [-32,32], and set delta=r-j/32. Require
|delta|<=1/32. Immutable arithmetic tables enclose sin(j/32), cos(j/32), with
65 entries per profile. These universal constants retain no source or Budget.
Evaluate sin(delta), cos(delta), reconstruct by the addition formulas, restore
the quadrant, and intersect with the mathematical range [-1,1].

The proposed polynomial is a certified truncated Chebyshev approximation,
converted into even/odd power form for Horner evaluation in delta squared.
Generate an exact rational Taylor parent on |delta|<=1/32 with a proven tail;
convert it exactly to Chebyshev coefficients on [-1,1]; discard high terms only
after bounding their uniform contribution by the sum of their absolute
coefficients. Add the Taylor tail. Choose the smallest parity degree whose
combined approximation error meets the profile target, then enclose converted
coefficients on the fixed grid. Horner rounding and coefficient uncertainty are
handled by the interval operations, not folded into an unproved ulp constant.
This is near-minimax truncation, not a claim of a computed minimax polynomial.
Compare a certified Taylor-Horner control before choosing the final literal
tables: avoided limb multiplies and complete point cost determine the choice.

The 32 guard bits provide headroom, not a correctness proof. Every primitive
has directed integer endpoints. A final optional width guard can reject a
mathematically valid but unhelpful enclosure. All three profiles are tried in a
fixed order independent of context cache history; skipped/out-of-profile stages
leave the existing fallback and cap behavior intact.

## Implementation seam

The proposed source-independent primitive takes Budget&, Rational const&,
an explicit fixed profile, and total working cap, and returns an optional
SinCosEnclosure. It owns no GeometryContext or source table. Reuse the existing
enclosure value types through a narrow common header if needed. Keep the public
primitive in a point-specific namespace so accidental jet-policy adoption is
visible in review. The geometry provider shares the actual source expression;
only its point trigonometry operation changes. It accepts only final unique
rounding cells and falls back as a whole point when a required trig call cannot
be enclosed usefully.

## RED, independent checks, and cost evidence

Author a missing-API RED plus independent Python Integer/Fraction vectors for
signed directed arithmetic, carry, cancellation and overflow. Check small and
moderate angles by direct original-argument Taylor enclosures, without the
production reduction or tables. For large arguments use the distinct identity
pi=4*(atan(1/2)+atan(1/3)), independent whole-turn reduction and a direct Taylor
enclosure with a derivative-radius guard. Include both signs, j/32 and quadrant
boundaries, non-dyadic arguments, 2^127-scale inputs, tiny angles, and explicit
out-of-profile fallback. No libm nearest-rounding oracle is assumed.

Compare canonical coordinate IEEE words with the existing exact point path for
arbitrary frames, large origins and cancellation. Check unchanged explicit jet
and metric enclosures before/after fast point calls, precision-cap failure,
cache-history independence and cancellation. Run focused strict Release,
ASan/UBSan/leak, format/tidy checks on the fleet. Do not expand into a numerical
certification campaign.

Reuse the existing 1024-point client and identical input sequence. Report cold
and reused-context complete point costs, output checksums, table bytes, charged
work, divisions/GCDs and profile/fallback counts. Keep line/rational controls.
The existing circle row is 4.307 seconds/1024 original analytic points; this
is a scoped cost observation, not a whole-mesher speedup or target SLA.

## Primary reference study

CRlibm's [trigonometric implementation](https://raw.githubusercontent.com/taschini/crlibm/master/trigo_fast.c)
demonstrates table-assisted reduction, reconstruction, and a separate accurate
phase. [Towards the post-ultimate libm](https://www.acsel-lab.com/arithmetic/arith17/papers/ARITH17_Dinechin.pdf)
motivates certified multi-stage rounding decisions and integer-based accurate
arithmetic. [LLVM's sincos support](https://raw.githubusercontent.com/llvm/llvm-project/main/libc/src/__support/math/sincosf_utils.h)
provides a current primary example of a reduced table plus parity polynomials.
[Ng's argument-reduction report](https://userpages.cs.umbc.edu/phatak/645/supl/Ng-ArgReduction.pdf)
explains why large-argument reduction needs an explicit precision budget.
These are reference studies: no code/constants or dependency are adopted. The
arbitrary-rational input and final source-expression rounding contract are ours.
