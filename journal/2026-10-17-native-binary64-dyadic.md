# Canonical binary64 dyadic construction

The parent accepted docs/native-binary64-dyadic-unit.md before implementation.
The sampled watch call stack reached from_double -> from_ratio -> divide_exact;
that observation selected this bounded arithmetic unit, not a general GCD
rewrite or a claim that conversion accounts for the complete meshing cost.

Rational::from_double now removes the known common power of two from its nonzero
machine-word mantissa before allocating Integers. It returns the existing
Canonical constructor with denominator1 or an odd numerator over a power of two.
IEEE decoding, sign, signed-zero normalization, nonfinite refusal and Budget
ownership remain in the same path. Generic from_ratio/GCD/division are unchanged.
The plan records the literal invariant and CPython v3.12.3 independent reference.

The existing exact vector generator now emits a separate conversion section:
34,138 raw binary64 words covering both signs, every finite exponent field and
crafted sparse/dense/trailing-zero mantissas. Python as_integer_ratio/Fraction
supplies canonical sign/numerator/denominator words. The native driver checks
them under its existing downward rounding and FTZ/DAZ environment. Both signed
zeros and nonfinite refusal classes are included. The old implementation passed
all words, then produced the genuine new cost-property RED: 123,870 arbitrary
divisions. The new implementation passes all words with zero divisions.

Strict Release and optimized ASan/UBSan/leaks runs passed this same expanded
suite. Existing 277 rational and173 general integer vectors also passed; their
Algorithm D trial corrections/addbacks and Lehmer criteria remain enabled.
The conversion-only run charged1,329,510 work and152 peak bytes versus the old
5,791,886 work and592 bytes. No device code changed and no GPU campaign ran.

The SAME native driver includes a bounded conversion-cost mode using existing
Budget counters. Four observations are recorded in
results/native-dyadic-2026-10-17/conversion-costs.tsv. Each input sequence is
identical before/after; warm-up is excluded and the timed loop includes limb
consumption. Strata273,104 conversions measured0.523970s before and0.0531726s
after. An unchanged original-source finite-field prefix262,144 conversions
measured0.444204s before and0.0376527s after. Checksums match in each pair.
The source container SHA256 is
1174672845d4920b5aa719b2ddeac56c062535779caee36bcc94f911f30d30a4.
The prefix walks source vertices/carriers/curves/edges/coedges in retained order;
it does not model emitted-point frequency. These are single conversion-kernel
observations, with no spread or complete-mesh speedup claim. Raw packets, old
binary and logs remain under ignored context/native-binary64-dyadic/.

The geometry peer read the actual arithmetic change and accepted it: zero and
nonfinite guards precede ctz; nonzero53-bit mantissa has at most52 trailing zeros;
the bounded strip preserves the value and establishes canonical coprimality;
exponent negation/shifts are defined; sign and same-Budget ownership are retained.
The peer found no rounding-mode/FTZ dependence or generic GCD semantic change.
The parent independently reviewed the actual from_double change and accepted
the same canonicality and guard argument. Two-file clang-format18 and the
repository clang-tidy18 configuration passed (only suppressed external-header
warnings); git diff whitespace checks passed. No further arithmetic work or
whole-watch rerun belongs to this unit.
