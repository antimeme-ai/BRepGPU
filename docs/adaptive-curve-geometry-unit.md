# Original curve and selected affine-trace numerical service

Implements the accepted adaptive-shared-curves-unit.md and the procedures/
literature in papers/native-adaptive-curves-geometry.md. One source numerical
owner supplies original dim2/3,p<=32 roots, bounded original points/fixed-vertex
departures, paired O(p²) restriction, actual chord bounds and direct affine-UV
carrier trace bounds. No per-use PatchSession or eager GeometryStore.

Headers are adaptive_curve_types.h, curve_source_cache.hpp and
adaptive_curve_geometry.hpp; HD formulas are adaptive_curve_numeric.hpp. The
literal source/request/result records follow the accepted report. Parent owns
CMake/CUDA wrapper; runtime batches/folds and construction owns chosen traces.
Root numeric table/constants remain common adaptive_numeric.hpp. CPU+P100 use
identical RN DAGs and immutable source/profile leases.

Observe genuine missing-operation RED with actual SourceStore fixture before
implementing the .cpp/HD bodies. Direct fixture checks original curve7 C0 roots,
spatial weighted line, paired control restrictions and SAME actual endpoint
residuals; extend SAME reader to independently authored p32/nonclamped/analytic
and fixed-vertex cases. Affine trace tests use original carrier9 rational formula
and actual chord, not a production source evaluator as expected truth.

Implement Bernstein Cox DP O(p³) once/root, canonical endpoint ownership with
exact discontinuity guard, selected exact-root fallback and line image proof.
Then common HD sampling/restriction/anchored residual and analytic M2; then
checked/precise services and direct original-span affine trace restriction/
derivative bounds. Test whole residuals/independent original formulas, not CPU
parity only. Close scoped strict/ASan/tidy, source review and actual P100 reader
through parent's typed API. No extra corpus/certification campaign or watch run.

The degree-32 whole-caller source guard exposed avoidable interval dependency in
`a+t*(b-a)` across repeated de Casteljau levels. Use a curve-only convex
interpolator `(1-t)*a+t*b`, with exact endpoint shortcuts, for original curve
samples and arbitrary initial restrictions; paired midpoint restriction already
uses the convex half-sum. This is the positive Bernstein/de Casteljau recurrence
studied above, not a changed source or tolerance policy. CPU and device call the
same helper; patch interpolation is unchanged. The existing degree32 source
reader additionally requires small unforced point departures, while the actual
shared-curve caller tests the original nodal cap. No new corpus or controller.
