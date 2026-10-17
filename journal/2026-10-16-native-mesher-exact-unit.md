# Native mesher exact arithmetic unit

Milestone1 follows accepted whole-mesher design/plan, not a source-specific port.
Owned scope: `native/mesher/exact.hpp/.cpp`, narrow existing Integer/Budget
extensions, authored actual native unit and independent original Python
Fraction/integer/IEEE vector producer. No new dependency or source model.

API sent to parent and geometry/topology owners before implementation. Rational
values capture exactly one worker Budget; explicit clone changes owner. Frozen
ExactWords contain canonical base2^32 numerator/denominator words and no Budget.
Integer Algorithm D and bounded certified Lehmer batches are selected per plan.
Nearest-even/outward conversions use integer arithmetic and IEEE bit assembly,
including negative subnormal enclosures, without ambient rounding assumptions.

## Genuine fleet RED before implementation

Authored independent producer generated277 rational and171 integer vectors
(346396 bytes), covering Fraction arithmetic, dense/sparse division, long
Fibonacci GCD, IEEE normals/subnormals/ties/overflow and exact binary64 decoding.
Fixture includes owner mismatch, malformed canonical words, zero division,
resource exhaustion and upstream allocation failure/recovery.

Actual isolated fleet build failed exit1 before implementation: Integer had no
budget/from_words/divmod_nonnegative and Budget had no upstream constructor.
Compiler output retained in ignored exact-unit context. No old checkout or
ongoing CAD/Python job was touched. This is API/behavior RED, not a fabricated
failing assertion after an implementation already existed.

## Implementation and focused GREEN

Implemented normalized radix2^32 Algorithm D with single-word/power-of-two paths,
trial correction and addback. GCD batches certified quotient intervals from the
leading63 bits: unknown tails are enclosed in[0,1), row-wise signed matrix bounds
must agree on the quotient; bounded unimodular transforms require exact positive
progress, otherwise use a full division step. Rational arithmetic cross-cancels
and normalizes zero/signs; mixed owners require explicit clone. Copy assignment
is transactional. Extended the existing Budget with a real upstream resource,
noncopyable owner identity, exact-work charges and branch observations; failed
upstream allocation leaves local live-byte accounting unchanged.

Independent original vectors grew by two hand-derived cases that force D3 trial
correction and D6 addback. Final277 rational+173 integer vectors pass, with
19,324 divisions,2,998 trial corrections,4 addbacks and23,854 Lehmer batches.
Final charged work7,366,186; max262 limbs; peak8,376 budgeted bytes. No output
depends on the ambient floating environment: vectors run while FE_DOWNWARD and
MXCSR FTZ/DAZ are enabled. Conversion uses integer div/rem, exact midpoint/tie
comparison and IEEE word assembly, including negative underflow/endpoints.

Root's actual CMake/strict LinuxClang18/libstdc++13 profiles: Release GREEN0.06s;
ASan+UBSan with leak detection GREEN2.19s. Root independently inspected Algorithm
D, Lehmer interval proof, cross-cancellation and IEEE conversion and reported no
numeric finding; final scoped review pending latest assignment/copy reread.
Shared existing face-domain/direct-CDT compatibility CTests are the remaining
relevant integration check; no unrelated benchmark/certification campaign.

Final independent vector payload SHA256:
`3b65d081397b474fd5c6b588b8f059c24eacfa0a2649a456b10596291d7b8038`.
Exact implementation SHA256 before repository-profile formatting:
`13a14c7bb0fc0af322b400256e95f00da9959b5d9cdd37713ed281535d6e0f84`.
Raw vector/build logs stay ignored; authored producer remains versioned.

All four existing direct-CDT/face-domain compatibility CTests passed in the
isolated Release build (0.01s). Parent independently reviewed the latest code,
including transactional assignment, and recorded **ACCEPT** in
`papers/2026-10-16-native-mesher-exact-code-review.md`. This closes the exact unit;
no further arithmetic campaign is planned. The next conceptual unit is actual
source-root residual restriction/comparison on CPU/P100, with exact fallback.

Actual-source clang-tidy with the repository config and Release compile database
passed; only non-user warnings were suppressed. Applied and checked the named
100-column native clang-format profile on implementation/header/shared Integer
header/fixture and copied these formatting-only edits back. No redundant vector
rerun after formatting. The implementation hash above describes the tested
preformat source, not a new protocol/receipt to regenerate after whitespace.
