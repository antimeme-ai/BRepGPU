# Independent original analytic numerical-core review

Scope: actual `geometry_analytic.hpp/.cpp` through original point enclosure and
canonical rounding dispatch, before later jet/chord additions. Static read only;
owner runs the existing direct-series/source integration fixtures and rigor.

**ACCEPT in this numerical-core scope.** No sign, enclosure or rounding defect
found. This does not approve later derivative/chord/refinement algorithms.

The dyadic compression uses exact signed floor/ceil: negative lower endpoints
increment the unsigned magnitude before negating, while negative upper endpoints
truncate toward zero. Every addition/product/division endpoint remains exact
before outward compression. Product bounds select all four corners and the
square lower bound includes zero whenever its argument straddles zero.

Machin atan partial sums have the correct alternating omitted-term sign and
remainder endpoints. The chosen term count exceeds requested absolute accuracy
for both reciprocal5 and239, and the compressed atan/pi arithmetic still encloses
the exact identity. Range-reduction pi precision includes the source argument's
integer magnitude before multiplication by its exact integer quadrant. Selecting
a quadrant from the exact midpoint is a locator: any selected integer preserves
the trigonometric identity, and the explicit reduced interval in[-1,1] guards
the Taylor hypothesis. Low-limb modulo4 and negative-quadrant correction agree
with the signed exact integer.

Sine's n retained odd terms admit the stated order2n+1 Lagrange remainder;
cosine's n retained even terms admit order2n, since the intervening coefficient
is zero. Factorial reciprocal bounds use |reduced|<=1. Interval recurrence and
outward compression may widen the result but do not exclude the true value.
Equal nearest-even endpoint words prove a single rounding cell; sign-changing
underflow that yields distinct signed zeros safely remains unresolved. A typed
precision refusal does not pretend to identify an unproved exact analytic zero.

Original curve/surface formulas retain raw finite frames and their coefficients;
plane/line rational paths remain exact, while cone/sphere/torus/circle/ellipse
use the same source formulas without frame normalization. Rational-store dispatch
imports original roots into the context owner. Cached pi references remain live
through their synchronous use; returned enclosures own values and do not retain
references to a subsequently replaced pi cache or another worker Budget.

One nonblocking policy clarification was sent to the owner: refine() could set
stage bits equal to maximum_bits, while trigonometric work requires those bits
plus guard32 and argument magnitude, so that nominal final stage is refused
before evaluation. This is conservative, not an enclosure bug. The owner selected
clarifying maximum as the total transcendental working cap and reserving the
guard/magnitude when choosing a final stage; do not claim every maximum_bits
stage was attempted under the older schedule.

Narrow cleanup rereview: ACCEPT the implemented policy/cache resolution. The
point refinement cap now reserves the maximum observed guard/argument cost; the
32/128 profile reaches and attempts stage95 for unit arguments. Pi entries are
keyed by exactly requested precision, so an earlier high-precision query cannot
silently tighten a later lower-precision result. The Budget-backed deque retains
borrowed entry references through appends. Jet trig-range queries take an explicit
request precision and call the stage evaluator directly, independent of point
context stage or previous pi-cache history. Their midpoint enclosure plus global
unit derivative bound includes extrema; no approximate critical-angle decision
is introduced. This addendum reviews these seams only, not all jet/chord code.
