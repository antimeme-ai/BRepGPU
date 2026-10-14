# Discovery oracle and benchmark review

Reviewed the exact Fraction restriction/convolution/cache/returned-UV oracle,
qualification runner, public caller tests and native analytic benchmark fixtures
and consumption. Report: [oracle source review](../papers/2026-10-14-discovery-code-oracle-review.md).

No defect found in the exact original-input oracle. Re-derived the native dyadic
global distances for planes, rational cylinder, normal parabola and competing
parabolic minima; representability is sufficient for exact bracket comparisons.
Reported R3-OR1 missing GAP-width qualification and R3-OR2 witness-consumption scope
to the benchmark owner. The existing plan explicitly separates native timed global
bracket truth from exact Fraction witness/cache qualification; the report preserves
that distinction. No product/test edits, native execution, mutations, new libraries
or Git changes were made for this review.

Re-reviewed the owner's benchmark fixes. R3-OR1 is resolved by error-free two-sum
binary difference comparison, with exact residual-sign handling when the rounded
width equals the requested tolerance. Strict compiler settings preserve its
arithmetic sequence. R3-OR2 is resolved as the planned scope plus added independent
expanded-map XYZ/descriptive-distance and upper-contradiction checks. The comments
explicitly reserve exact original-UV upper qualification for the Fraction oracle;
the long-double corruption allowance does not relax native global bracket or gap
comparisons. Appended final source hashes and dispositions to the report. No new
execution claim or product edit; integration owns the target test matrix.
