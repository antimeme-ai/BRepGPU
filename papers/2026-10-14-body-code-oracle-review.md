# Body sampling exact-oracle source review

2026-10-14. Accepted after resolving BR-OR1 below. This is an independent
source review of [body_oracles.py](../python/brepgpu/body_oracles.py) and
[test_body.py](../tests/test_body.py), against the reviewed body design/plan.
No product import, local native execution or fleet job was performed here.
The coordinator owns the actual qualification outcomes.

Reviewed SHA-256: oracle
`43eb5f655e30aff530b250222d88c2419c83c88c0ded78d1c4471771e8730034`,
tests `389de80a4e54f83970dc23b840cd90e7f72a209b8c9dddc6fb9f9589c2d10b35`.

## BR-OR1: bind analytic carrier identities to actual controls — resolved

The first source proved curve-to-UV identities using an ideal analytic face
map, while checking actual patch inputs only for degree/frame/lineage/UV.
An altered homogeneous net could therefore pass snapshot or empty-batch
qualification despite violating the claimed complete carrier correspondence.

The owner now checks every actual homogeneous control exactly against the
canonical analytic family: bilinear plane corner values and weights, or the
quadratic wall numerator/denominator coefficients and their declared rotation,
with both extrusion endpoints. These values are exact dyadics. This binds the
subsequent ideal-face identities to the actual native inputs without invoking
the product evaluator or reproducing its floating-point algorithm. Canonical
vertex coordinates are checked independently too. No remaining blocker.

## Independent geometry and topology truth

Submitted binary64 UVs become exact Fractions. Disk and hole predicates use
the exact squared-radius expression; the plate hole threshold is 1/9. Rounded
arc points are classified as their supplied values, rather than promoted to
an intended exact boundary. Wall seam labels distinguish the two atlas ends
from internal quarter joins.

Expected positions use explicit plane maps or the quarter-circle expression
((1-u²)/(1+u²), 2u/(1+u²)), then the supplied positive frame. Expected normals
come from canonical material roles, including the cavity sign; the actual
Face.orientation is separately checked against that role. Fraction comparisons
check emitted floats with the declared 2^-36 position and 2^-30 normal grades.
CPU/CUDA parity supplies no expected geometry.

The polynomial cross-product identities cover entire edge parameter intervals,
not only sampled endpoints. They check line/rational-circle spatial curves
against the face UV maps; actual control binding supplies the missing carrier
link. Opposed edge uses, ordered closed loops, canonical outer domains,
hole winding, shared seam, coverage references and hole-sensitive Euler counts
check the small declared graphs. The independent struct decoder checks public
packed layouts, tags, padding, offsets and adjacency against logical records;
that serialization check is distinct from the analytic geometry check.

## Direct test coverage and limits

The nine tests cover immutable source copying, canonical graphs/bytes, exact
trim adversaries, public curve/UV evaluation, full 17-by-17 grids and reordered
mixed requests, frame extrema, face selection/empty input, structural validation
and incremental row cap, exact snapshot binding and closed handles. Original
request slot/UV bytes are compared to rows separately, so a self-consistent
wrong scatter cannot pass merely by describing its own changed UVs.

The cleanup test uses actual native query/result wrappers: each closes its
original handle before raising SystemExit/KeyboardInterrupt while download
raises a chosen ArithmeticError. It checks the exact primary object, both
consumed handles and a subsequent healthy sample. The coordinator recorded
its failure before the sampler-only child-scope fix; green execution is outside
this source review.

The checks qualify these three fixed factory families and supplied samples.
They do not establish arbitrary B-rep validity, generic trim algorithms or
floating-point correctness outside the admitted frame/carrier contract.
