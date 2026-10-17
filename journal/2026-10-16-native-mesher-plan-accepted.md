# Whole native mesher: implementation plan accepted

The whole-design branch now has a reconciled implementation plan and independent
algorithm, geometry and runtime reviews. The final fixes preserve loop traversal
separately from physical facet orientation, distinguish nodal recovery residual
from whole positional approximation, and allow ambient PL-map supports outside
the natural parameter rectangle without evaluating the carrier there. Actual
reference/final domains and evaluated endpoint hulls remain inside it.

The implementation starts with shared native source values, owned exact
arithmetic, original-coefficient geometry and mutable connectivity. Each unit
first observes direct RED cases on the fleet. Numerical execution has one owner
for CPU and P100; geometry and shared-boundary/face invariants retain their
conceptual owners. Root integrates the source, runtime, result and consumer.
No new library adoption or Python production callback is selected.

This milestone is an accepted plan, not delivered meshing or a watch result.
The next commits must contain actual RED tests and native implementation.
