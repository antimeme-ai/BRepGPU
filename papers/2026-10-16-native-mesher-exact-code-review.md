# Independent exact-arithmetic code review

Scope: actual `native/mesher/exact.cpp`, `exact.hpp` and the narrow existing
Integer/Budget changes, reviewed by root independently of the implementation
owner. This is a foundational arithmetic review, not mesher acceptance.

Reviewed normalized radix2^32 division, quotient correction and addback; leading
limb GCD batching; cross-cancelled canonical rational arithmetic; owner transfer;
nearest-even conversion and outward IEEE neighbors; assignment failure behavior.

The Algorithm D product plus borrow fits uint64: with radix B, the maximum
product is (B-1)^2+(B-1)=B²-B. Quotient-estimate comparison only shifts the
residual while it is below B, so that comparison also fits. The normalized
dividend has its extra high limb, and the negative subtraction branch performs
the required addback. Single-word and power-of-two divisors have separate exact
paths. No numerical finding remains in this reviewed implementation.

GCD's leading63-bit tails are enclosed by signed matrix coefficient bounds.
Quotients must agree throughout the interval; ambiguous cases use full division.
The bounded unimodular matrix preserves GCD, and exact nonnegative decrease is
required before committing a batch. This gives correctness even independently
of a performance expectation about how often batching succeeds.

Rational operations cancel common factors before products and preserve canonical
zero/signs. Mixed owners are rejected. Copy assignment builds the complete new
value before replacing either integer; allocation/work failure cannot publish
a half-new numerator/denominator. Shared source values remain independent words,
not arithmetic values that mutate another owner's budget.

Binary64 conversion constructs exponent/significand bits from exact quotient and
remainder, including ties-to-even and subnormal carry. Outward neighbors compare
the rounded value to the exact rational before adjusting a bit. A nonfinite
required result is an explicit resource/range failure. These paths do not depend
on floating rounding mode or FTZ/DAZ. Independent Python Fraction/integer vectors
attack those answers; release and ASan/UBSan checks and the existing shared-header
CDT compatibility checks are GREEN as recorded in the unit journal.

Verdict: **ACCEPT for this exact-arithmetic unit**. This does not review future
interval filters, GPU restriction, whole-cell bounds, scheduler or source geometry.
