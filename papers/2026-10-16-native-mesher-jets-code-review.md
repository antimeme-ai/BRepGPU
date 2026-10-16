# Original interval jets and analytic chord code review

The parent reviewed actual `geometry_jets.cpp` against the original formulas,
not CPU/GPU parity. Homogeneous derivatives carry original parameter widths and
degrees. Centering by one exact root point removes common translation before
quotient differentiation; it does not change derivatives. The first, pure
second and mixed quotient rules include every denominator derivative. Positive
homogeneous weights justify Cartesian coefficient hulls. Source dispatch selects
one root and restriction rejects a query crossing its bounds, so the code does
not assert C2 across a C0 knot.

Analytic plane, cylinder, cone, sphere and torus first/second partials follow the
retained frame/radius expressions, including nonorthogonal original frames and
dimension-two curve projections. Explicit requested precision, rather than a
previous point evaluation's refinement stage, selects bound constants.

The analytic actual-chord bound uses M2*dt^2/8 plus the maximum of the two
original-point versus actual emitted endpoint departures. It charges endpoint
error once. Whole-range second-derivative interval norms and the largest
eigenvalue of the exact two-column frame Gram matrix are both upper bounds;
choosing their minimum remains safe. Both nested square roots use outward exact
dyadic bounds, including arbitrary nonorthogonal ellipse frames.

Literal polynomial derivatives, independent rational quotient/mixed-partial
values, translation invariance, analytic extrema, cache-history invariance and
actual endpoint offsets directly exercise these claims. Strict and
ASan/UBSan checks pass. The original analytic point core has a separate numerical
peer review; that review is not substituted for this jet/chord inspection.

Decision: ACCEPT this scoped original-jet/analytic-curve-chord implementation.
General knot-crossing actual-cell clipping, charts, inverse recovery and
source-domain displacement remain the planned consumers and are not claimed
complete by this unit.
