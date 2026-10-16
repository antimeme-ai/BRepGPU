# Adaptive patch caller source review

2026-10-16. Geometry owner reviewed actual adaptive_main.cpp,
output_directory.hpp and tests/native_adaptive_caller.py. Scoped ACCEPT after
one source-domain finding: default rational query rectangle used adapter
carrier.uv_bounds instead of original natural knot domain. Root fixed it using
expanded original knot ordinals degree/control_count; explicit rectangles remain
query inputs. Authored source now has deliberately wrong uv_bounds while the
reader still requires the complete original unit rectangle.

Exact positional-mm/source.units_mm conversion is preserved as Rational;
source coordinate output and original exact UV are retained. The operation
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
