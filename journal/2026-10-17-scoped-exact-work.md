# Scoped exact-work allowance

Facet preparation advertised a smaller work allowance than its caller's Budget,
but checked only between operations. A Rational operation could consume the
caller's larger allowance before the facet check ran. Following the existing
owned Budget/Integer charge protocol, WorkScope temporarily lowers the SAME
Budget's limit to the smaller remaining allowance. It preserves allocator,
arithmetic owner, stop token and cumulative work; destruction restores only the
previous cap. Nested scopes must stay in stack order, with no reset inside them.

The existing exact fixture's narrow --budget-only mode first failed for the
missing API, then passed nested, zero, exception, overflow and held-owner cases
on the fleet. Actual source review and configured static analysis accepted the
implementation. No arithmetic-vector or mutation campaign was added. This bounds
the operation; it does not fix the separate BAND point-placement defect or claim
weighted-face performance. Geometry work consumes the scope in the face unit.
