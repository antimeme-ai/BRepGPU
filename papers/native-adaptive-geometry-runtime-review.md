# Adaptive geometry numerical core: runtime peer review

Read actual `adaptive_numeric.hpp`, generated `adaptive_rn_tables.hpp`, and the
SourceCache constant conversion, Gram scaling and affine-separation routines.
Scoped source ACCEPT for the numerical admission/evaluation/node-bound unit.
No extra test campaign was run by this reviewer; geometry owns independent
original-source readers, and the connected caller exercises CPU/both P100s.

CPU/P100 use explicit RN operations plus the same bit-neighbor padding and
fixed arithmetic order. Invalid intervals propagate before zero shortcuts.
Scaled Euclidean norm avoids squaring world magnitudes directly. Checked
quadrant reduction needs only an integer k with its entire residual in the
proved domain; no unproved unique nearest quadrant assumption is used.
The power-Horner coefficient intervals and uniform errors derive from the
owned P128 source, and the anchor addition formula retains both error terms.
Literal source frames and original span evaluators remain authoritative.

The rational residual retains the actual bilinear interpolation and positive
weight denominator, including common anchoring. Direction scores are heuristics,
not sign predicates: anchored homogeneous differences plus denominator variation
avoid per-control divisions and retain nonlinear-weight sensitivity. Exact
original-support degree-one separation checks equal weights and proportional
homogeneous displacement; this proves an affine extruded axis, rather than
assuming every degree-one rational direction is harmless. Local immersion is
an acceptance requirement, separate from positional error and global embedding.

One efficiency finding was sent to the geometry owner: the six quadratic
Bernstein facet coefficients alone can bound a rectangular half-triangle by
|D|/2 even though its actual bilinear covariance is at most |D|/4. Retain the
minimum of that valid coefficient bound and the already selected universal
|D|/4 + max actual-vertex departure bound. This changes tightness, not truth;
final affected connected checks belong to the existing cohort, not a new
certification campaign. Runtime never publishes untested actual facets.

The owner implemented that selected minimum and an exact nonnegative sum-of-roots
comparison. Actual revised source reread ACCEPT: with A=|D|²/16,
B=max vertex residual squared and r=allowance−e>=0, acceptance is
r²>=max coefficient squared OR (s=r²−A−B>=0 AND s²>=4AB).
This is equivalent to the corresponding nonnegative sum-of-norm bound and
includes equality. Conservative reported square-root uppers remain separate
from exact threshold truth. The same actual affine map is retained throughout.
