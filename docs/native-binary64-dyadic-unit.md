# Canonical binary64 dyadic construction

2026-10-17. Finite PLAN only; no arithmetic body edit before parent review.

The live sampled watch stack reaches Rational::from_double -> from_ratio ->
divide_exact during facet candidate preparation. This identifies avoidable
normalization on that sampled path, not the cause of every elapsed second or
large integer operands. Current exact.cpp already decodes raw finite binary64
bits into sign, a nonzero <=53-bit mantissa m, and e in[-1074,971], representing
sign*m*2^e. It then invokes general arbitrary-integer normalization even though
the denominator is a known power of two.

## Source basis and literal invariant

Owned exact.cpp::from_double, integer, Integer::shift_left and the private
Rational(Canonical) constructor provide the complete required machinery.
CPython v3.12.3 Objects/floatobject.c::float_as_integer_ratio_impl lines1478–1540
is the independent reference/oracle: it obtains an integral significand with a
power-of-two exponent and constructs numerator/denominator by shifts rather
than a general rational GCD. Its documented contract is exact, lowest terms,
positive denominator. Source:
https://github.com/python/cpython/blob/v3.12.3/Objects/floatobject.c#L1478
GMP headers/runtime are installed on the fleet, but mpq/set_d.c source is not in
the inspected local reference collection; no GMP behavior is asserted and no
new dependency is proposed.

For e>=0, denominator1 makes the shifted signed numerator canonical already.
For e<0, let k=min(ctz(m),-e). Strip k bits from m and add k to e. Either e becomes0
and the denominator is1, or e remains negative and m is odd, hence
GCD(m,2^-e)=1. The private Canonical constructor is sound by this literal proof.
ctz is called only after the existing nonzero check; k<=52, so the machine-word
right shift is defined. New e remains bounded. Sign is applied once. Both zero
encodings still map to canonical rational0; all exponent2047 encodings still
fail as nonfinite. No floating operations, rounding-mode dependence or FTZ/DAZ
assumption enter this constructor.

## Owned change

Change ONLY exact.cpp::Rational::from_double: preserve IEEE decoding/nonfinite/
zero guards, strip the known common power of two before Integer allocation,
construct the signed numerator and positive denominator in the same Budget,
perform the necessary existing left shift, and return Canonical directly.
Avoid a zero-bit shift. Keep all generic from_ratio/GCD/division bodies unchanged.
Integer allocations/shifts retain cancellation, bytes/work limits and ownership.
Reduced charged work is intended; arithmetic values/exported word ABI are not
changed. No geometry, tolerance, key, device ABI or source-name dispatch changes.

## One direct oracle and bounded costs

Extend the existing tests/native_mesher_exact_vectors.py +tests/exact_test.cpp
conversion section rather than adding another exact checker. Add a versioned
conversion-only section so its >10000 cases do not duplicate costly unrelated
rational arithmetic. Expected sign/numerator/denominator words come from
float.as_integer_ratio/Fraction independently. Include both signs and EVERY
finite exponent field0..2046 with zero/minimum/odd/dense/alternating/high-bit
fraction patterns; explicit mantissa trailing-zero0..52 cases around subnormal,
normal, unit/integer and maximum-exponent boundaries. Include signed0 and retain
existing NaN/infinity refusal. Existing downward-rounding +FTZ/DAZ execution
applies to these conversions too.

First run expanded value oracle on OLD body (must pass exact values), and record
its conversion-only work/division counters and elapsed cost. A direct new claim
is zero arbitrary-integer divisions in that conversion section; old body supplies
the genuine cost RED while semantic words stay GREEN. Then implement and require
identical Fraction words/new zero-division property. No generic arithmetic vector
criteria are weakened; run the existing exact suite once for compatibility.

Add a mode to the SAME native exact test driver for a fixed-count from_double
cost loop, with checksum of resulting integer limbs to retain real work. Use the
same saved finite bit-word input sequence before/after, warm-up then bounded
repeats, reporting input count/hash, elapsed, work/divisions, bytes/limbs and
checksum. In addition to exponent strata, the ignored operational harness can
extract the actual finite binary64 source fields from the unchanged watch
SourceStore (frames/knots/controls/parameters/XYZ), then use that identical saved
sequence for both binaries. This is source-value conversion cost, not full-watch
speedup or proof that conversion dominates every workload. No competing whole
watch run, new quality family or numerical campaign.

Run strict Release native exact target, existing conversion oracle with rounding
modes, scoped existing ASan/UBSan exact target and actual source review. No GPU
execution is needed: the changed code is the native host arbitrary-integer
constructor. Parent owns CMake/commits. Save actual old/new counts/costs and one
scoped source verdict, then stop this arithmetic unit.
