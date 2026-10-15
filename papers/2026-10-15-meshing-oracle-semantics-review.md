# Meshing independent oracle and tests: geometry semantics review

2026-10-15. Read complete `python/brepgpu/meshing_oracles.py`, all ten methods
in `tests/test_meshing.py` and the separate healthy-native/missing-entry test.
Geometry/domain reasoning accepted; one typed-field false-pass finding O1 was
reported and corrected. **Final disposition: accepted, no remaining blocker.**
This was source-only review with no product or test execution.

## O1 — Python equality aliases bypass typed source contracts

Initial source generator validation checked tuple membership in the expected
dictionary without validating its elements. Python treats `True` as equal to
integer 1, and a dyadic float as equal to its Fraction. Thus `('vertex',True)`
could pass for original vertex 1, and a float edge parameter could pass for
the required exact Fraction parameter. Original polyline/use vertex-index
tuples and BoundaryUse face/edge/orientation fields similarly relied on value
equality without strict integer types.

Requested explicit source-kind arity/types, strict integer original IDs and
indices excluding bool, and exact Fraction edge parameters. This concerns
published identity/type semantics rather than a geometric arithmetic fault.
Root and oracle owner received the finding before final acceptance. No reviewed
product change was requested. Verified actual revised source: each source kind
has explicit arity/native type checks, all physical index chains require native
ints, and BoundaryUse ownership/sign fields require native ints. The existing
output-corruption method now directly exercises bool-ID, float-parameter and
index/ownership equality aliases. O1 resolved.

## Whole-domain and rounded embedding reasoning

The oracle validates original body/control facts with the existing independent
body qualifier, then independently reconstructs every expected source generator,
edge interval and coedge UV occurrence. Product meshing/refinement/readers do
not generate expected data. Using actual output polylines after their complete
source/parameter/identity equality checks does not introduce a free expected
boundary chosen by the product.

For each planar face, strict orientation signs and the full directed integer
edge multiset establish winding multiplicity over the independent simple source
polygon domain. Opposite internal edges cancel algebraically while every outer
and hole interval remains with its multiplicity. This proves complete domain
coverage rather than inferring it from area, centroid or Euler totals. Wall
triangles are partitioned by their original quarter/interval and projected into
actual affine chord lambda and z. Source rational s is used for source identity,
not incorrectly treated as an affine chord coordinate. The same signed-chain
argument covers each entire ruled rectangle, in exact and rounded coordinates.

Original patch Bernstein evaluation independently reproduces every triangle
corner's exact world point from original chart controls/local UV. Complete
lifted coedge maps retain both seam occurrences. Circle minor-arc order and
the exact endpoint dot-product inequality bound the actual chord sagitta,
separately from checking reported conservative refinement metadata. Full actual
float conversion and L1 displacement are checked.

Exact and rounded rings receive unique-position, single-quadrant monotone
traversal and strict-turn checks. Together these establish simple convex ordered
rings, rather than relying on local turns alone. Paired XY, ordered heights,
plate nesting, common face planes and positive complete domain chains establish
the canonical embedded shell. Full opposite edge incidence, one cyclic link per
vertex and connected shell checks attack separate topology defects. Counts/genus
are supporting consequences. The earlier planar expected-chain recomputation
was already replaced by one precomputation per face; remaining work is linear
in emitted topology with bounded per-triangle arithmetic.

## Direct tests and actual request binding

The ten methods plus entry exercise complete coarse box/cylinder/plate shells,
large-tolerance retained holes, all frame scales, source orientations, refinement
threshold equality/nextafter, finer geometry and exact subnormal rounding
budgets. They inspect rim closure, original parameters, wall chart joins and
same-face seam occurrences; they also cover immutable publication, malformed
requests, resource failure and healthy retry. Missing/duplicate/reversed/filled
hole triangles and incorrect source/seam/rounded records are rejected through
the actual whole-output qualifier. These bounded fault demonstrations do not
establish broad mutation or community qualification.

Intrinsic `qualify_mesh(mesh)` validates the source and tolerance retained in
that mesh. Caller/test code must additionally bind result.body by object identity
to the actual submitted body, compare exact submitted tolerance and enforce the
submitted segment budget before awarding a request pass. The current test
helper checks body identity and exact tolerance; root was reminded to preserve
all three checks in the upcoming actual caller. No general imported-domain,
GPU or alpha-readiness claim is justified by these tests.
