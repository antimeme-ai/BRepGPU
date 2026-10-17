# Native source metric and exact proposals

Implemented `native/mesher/metrics.hpp/.cpp` as the topology owner's common
proposal mechanism. It consumes the geometry owner's original point jets,
not a second source evaluator. Selected point-jet midpoints are rounded to
53-significand dyadics with separate exact exponents. A zero chosen normal
requires an implemented quotient profile. Physical box/chart scales and the
definition-local allowance enter the accepted Gram/curvature/floor formula.

The symmetric curvature absolute uses its positive/negative semidefinite sign,
or `(II^2-det(II)I)/sqrt((a-c)^2+4b^2)` when indefinite. A positive dyadic scalar
denominator preserves PSD. The SPD matrix's small eigenvalue uses det/large
rather than subtracting near-equal eigenvalues. Stable principal-vector branches
and separate-exponent scales form the final factor by EXACT dyadic products;
positive expanded determinant is checked. No factor underflows to zero merely
because its exponent exceeds binary64's range. These are proposal approximations,
never an error-bound acceptance argument.

The sqrt helper normalizes into [1,4), uses the existing exact rational sqrt
bracket, and resolves ambiguous adjacent binary64 values by comparing their
exact squared midpoint. The exact2x2 circumcenter supports mixed metric entries;
chosen coordinates round once, with nonfinite choices sent to routine spent
rejection. Phi evaluates the complete rational expression and rounds once.
Finite metric normalization cancels exactly from the potential.

Authored missing-header RED preceded implementation. The first strict native
closure passes isotropic and rotated-ridge kappa=1/16/256 cases, with directional
ratios1/1501/2.064e6/8.38886e9. Expanded ASan/UBSan/LSan execution and actual-source
engine/fixture lint pass negative/indefinite curvature, source-backed weighted
and knot point jets, exact center/Phi, sqrt2, exact/near midpoint rounding,
2^-4000 scaling and overflowed proposal/Phi rejection. Two final full mixed
center/Phi literal assertions were added afterward, strictly compiled and
independently rederived by root; latest root CMake execution will cover them.

Scoped independent code review ACCEPT is recorded in
`papers/2026-10-16-native-mesher-metric-pointjet-code-review.md`. The factor ratios
establish directional proposal behavior, not completed adaptive mesh optimality
or whole-operation speedup. Actual face refinement and bound tasks remain the
next consumers.
