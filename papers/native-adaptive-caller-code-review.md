# Adaptive patch caller source review

2026-10-16. Geometry owner reviewed actual adaptive_main.cpp,
output_directory.hpp and tests/native_adaptive_caller.py. Scoped ACCEPT after
one source-domain finding: default rational query rectangle used adapter
carrier.uv_bounds instead of original natural knot domain. Root fixed it using
expanded original knot ordinals degree/control_count; explicit rectangles remain
query inputs. Authored source now has deliberately wrong uv_bounds while the
reader still requires the complete original unit rectangle.

The original review incorrectly accepted positional-mm/source.units_mm division.
Runtime review corrected the source contract: SourceStore coordinates are
already millimetres and units_mm is metadata. Root removed the division and
added an unchanged-geometry units_mm=.001 test. Source coordinate output and
original exact UV are retained. The operation
calls SourceStore→requested SourceCache→actual PatchSession and publishes only
complete packet output. It does not construct eager old GeometryStore or fill
missing geometry through old face preparation. Wire retains source digest,
actual XYZ, exact chosen UV and actual facet roles. Capacity/nonfinite guards,
new temporary directory, no-replace rename and owner-only cleanup are coherent.

The independent integer/Fraction reader checks actual physical noncollapse,
positive chart facets, incidence/whole rectangle coverage and actual affine
interior errors against original polynomial/rational source formulas. Those
finite samples attack wrong-output mechanisms; they do not constitute a
universal whole-cell error proof. Product completion uses the actual bounds.
This review did not run product code, performance measurements or source models.

Subsequent source-only review of the coupled quadratic fixture (`make_bowl`,
historical name) and its whole-facet Fraction branch: ACCEPT. Its exact degree2
Bernstein controls give z=3u²/4+v²/4+uv. On any chart triangle the source-to-nodal
interpolant difference is the weighted sum of U/V variances and covariance.
Their range bounds give 3range(U)²/16+range(V)²/16+range(U)range(V)/4. Requiring
every actual nodal departure norm to fit the remaining allowance therefore
bounds the complete SAME actual affine facet, not merely sampled interiors.
This source is a coupled saddle; the argument needs no convex-source premise.
The weighted rational case still uses explicitly identified falsification
samples. No additional execution was requested or performed for this review.
