# Exact plane/wall construction: independent oracle code review

Reviewed `python/brepgpu/intersections.py`, SHA256
`ce94248ed7f9f67b36a077b731a5e6282f99a24162741878a9c318730d6a1b63`.
Source review only; the coordinator executes native callers and direct tests.
No blocking source finding.

The exact scalar comparison uses signs of squared magnitudes only after checking
opposed signs, including comparisons across two quadratic fields. Arithmetic
recognizes equivalent fields by a rational square ratio; unrelated-field
arithmetic refuses rather than creating an unjustified single-radical answer.
Rational binary64 brackets use integer quantization at the normal/subnormal
boundary; negative brackets reflect adjacent endpoints, and midpoint parity
selects ties to even. The new direct test supplies independent Fraction answers
for half-minimum subnormals, normal transitions, ordinary ties and maximum finite
values, plus independent integer-square-root intervals for irrational rounding.
Values beyond maximum finite are explicitly refused, including values that a
less restrictive conversion might round back to maximum finite.

The horizontal constructor splits at every exact quadratic trim root plus
quarter endpoints, classifies exact roots and rational interior witnesses, and
emits isolated admitted roots separately. Roots remain events when admitted on
both sides. Named atlas joins identify exactly shared quarter endpoints without
floating position welding. The vertical constructor determines the exact
radius-squared discriminant before branch creation, intersects exact height and
lateral intervals, and retains all supporting quarter occurrences. A cardinal
generator therefore has two UV occurrences and one physical span. A tangent
trim point differs from a tangent carrier. Point spans contribute no graph
edges; the graph computes closure from distinct physical spans.

Original-control binding is checked independently in the oracle by composing
both submitted UV maps with the original homogeneous Bernstein controls and
cross-multiplying against the spatial map over the entire piece. Domain and
trim signs are checked over intervals, including an interior quadratic minimum;
an admitted interior zero must split the piece. Event labels, endpoint world
identities and grouped graph degrees are checked independently. These checks
prove validity of submitted pieces, not completeness for arbitrary unknown
configurations. Literal expected complete cases cover the circle, four clipped
arcs, hole exclusion, lens/trim touches, generator counts and subnormal
near-tangent separation. The exact caller recipes expose a separate
`recipe_counts` completeness check.

Limits remain deliberate: factory bodies, one axis-aligned planar face and one
vertical cylindrical wall, positive power-of-two frames; no general SSI,
coincidence classification, imported controls, tolerance welding or mesh claim.
Finite samples qualify only consumption of the exact result, never component
completeness. Runtime sampling and oracle execution are separate review and
qualification steps.
