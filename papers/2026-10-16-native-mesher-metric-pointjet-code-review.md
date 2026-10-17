# Original point jets and fixed metric proposals: scoped code review

The parent inspected the actual point-jet extension, private homogeneous
polynomial evaluator and metric/proposal implementation. This review concerns
their numerical/ownership semantics, not completed adaptive-mesh performance.

The polynomial wrapper checks degree, stride, complete shape, owner and unit
parameter before indexing. It shares the existing de Casteljau engine and
deliberately permits signed derivative weight channels. Original source point
jets differentiate homogeneous nets in original span units and evaluate their
quotient first, pure second and mixed derivatives exactly at the selected point.
They do not use finite differences or a vanishing-width range query. The stable
root choice at a knot selects a one-sided proposal jet; it asserts no C2 theorem
across the knot.

Metric preparation selects fixed dyadic point-jet values, forms Gram and second
fundamental data, and constructs an SPD proposal with the stated physical and
parameter-scale floor. The indefinite symmetric absolute-value formula
(II^2-det(II)*I)/sqrt((a-c)^2+4b^2) is PSD; positive/negative semidefinite branches
use the corresponding sign. Positive rounded denominators preserve that property.
Eigenvalue preparation uses det(B)/lambda_large for the small value and chooses
the noncancelling principal-vector branch. The final factor is an exact dyadic
product with strictly positive determinant; it defines the actual fixed metric.

The owned 53-significand square-root proposal separates its exact exponent,
normalizes before rounding, and compares the exact squared adjacent-float midpoint
to the original rational. It neither underflows tiny factors to zero nor relies
on ambient libm rounding. Zero chosen normals require a quotient profile.

Circumcenters solve the full exact 2x2 metric equations and round once; nonfinite
chosen candidates reach routine spent rejection. Shape potential evaluates the
whole rational expression before one nearest-even rounding. Its denominator
12*det(M)*area2^2 equals the accepted 48*det(M)*A^2. The independent mixed-factor
literal yields center (65/4,65/4) and potential 37249/768 as expected.

Decision: ACCEPT these point-jet and proposal units. Literal weighted source jets,
knots, isotropic/rotated-ridge metric families, extreme-exponent rounding, exact
mixed centers and potentials pass focused strict and sanitizer checks. Metrics
choose proposals and ordering; original geometry bounds still decide admission.
No optimality theorem or whole-operation speedup follows from factor ratios.
