# Adaptive shared-curve numerical geometry

The accepted `docs/adaptive-curve-geometry-unit.md` and
`papers/native-adaptive-curves-geometry.md` govern this source/cache/numerical
unit. It supplies the actual shared physical-edge and occurrence-trace owners;
it does not claim a complete face-domain mesher. Parent owns CMake, CUDA session
implementation, caller and publication.

Original source controls are homogeneous, including dimension-two stride three.
The local DP Cox-in-Bernstein extraction computes every recurrence level once
(O(p³) per original B-spline span, O(p²) scratch). Original Bezier controls already
are the Bernstein net and are copied directly; the first degree-32 reader showed
that routing these through the recurrence caused large artificial interval
width. Adaptive paired children share one de Casteljau midpoint pyramid O(p²).
Original analytic curves use the literal two stored radii and original frame.

Genuine RED preceded implementation: the direct source-backed fixture compiled,
then fleet `clang++-18 context/adaptive-curves-geometry-red.o
build/adaptive-geometry-release/libbrepgpu_mesher_source.a
build/adaptive-geometry-release/libbrepgpu_mesher_exact.a -o
context/adaptive-curves-geometry-red` failed to link the absent cache preparation,
original-root lookup and original sample operations. Actual log:
`context/adaptive-curves-geometry-red.log`. An earlier mistyped library path was
an incidental command error and is not the RED.

All geometry execution uses the isolated fleet cached-adaptive-mesher checkout.
Strict build command:

    cmake -S native/mesher -B build/adaptive-curves-geometry-release-unit -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=/usr/bin/clang++-18 -DBUILD_TESTING=ON
    cmake --build build/adaptive-curves-geometry-release-unit --target brepgpu_native_adaptive_curves_geometry -j 4

Fixture authoring:

    python3 -B tests/native_mesher_geometry_fixture.py context/original-geometry.bgsource
    python3 -B tests/native_adaptive_curve_fixture.py context/original-geometry.bgsource context/adaptive-curves.bgsource

The direct reader uses raw original Cox basis evaluation, exact Fraction
polynomial interpolation, positive homogeneous residual coefficients and Decimal
analytic formulas. It reads actual numerical output rather than comparing with
the producer's extracted net. The independent source families cover degree32,
C0 quadratic, nonclamped natural knot domains, varying positive weights,
nonorthogonal circle/ellipse frames, line, fixed actual endpoints, paired
children, reversed pcurve intervals and virtual same-L carrier restrictions.
It checks original point enclosures/departures and whole chord coefficient or
second-derivative bounds, not merely finite sampled chord distances.

    python3 -B tests/native_adaptive_curves_oracle.py build/adaptive-curves-geometry-release-unit/brepgpu_native_adaptive_curves_geometry context/adaptive-curves.bgsource

Final CPU output so far: `original-curve-reader PASS 81 cpu`. The fixture also
asserts precise equality acceptance, fixed original negative-zero preservation,
and exact original hull admission on a natural boundary. Both P100 reader runs
and sanitizer/tidy closure are in progress; they are not yet claimed here.

Two reader mistakes were corrected explicitly: the natural endpoint of a
nonclamped Cox basis needs its one-sided natural-domain convention, and original
circle-kind source expressions still use BOTH stored radii, just as the original
source reader/evaluator does. A proposed circle M2 radius special case was
reverted before GREEN; no changed original geometry is retained.

Construction's natural-boundary ambiguity finding is addressed by
`enclose_curve_hull_precise`: exact restriction of original positive homogeneous
nets and exact Cartesian control hulls, not clipping padded RN boxes without
proof. Unknown analytic exact-hull profiles remain explicitly unsupported. The
new helper has a direct rational natural-boundary assertion and peer review
requested. The direct affine UV service is indexed original-span restriction,
not a PatchSession per use or an eager old GeometryStore preparation.

The final convex source sampler passed the SAME original-source reader on CPU,
P1000 and P1001: 81 records each. Strict host target, ASan/UBSan/leak reader and
actual-source tidy also passed. Final module location is
build/adaptive-curves-root-cuda/libbrepgpu_mesher_adaptive_cuda.so.

The whole degree32 shared-boundary caller provided a real numerical-stability
RED: `source-node enclosure does not establish cap: upper=2.13261574029026215e-07
cap=1.00000000000000002e-08 original_use=500`. Reproducer is
`tests/native_adaptive_curves_caller.py::source('degree32')`. Its actual log was
preserved at context/adaptive-curves-geometry-stability-red.log. The curve-only
convex interpolation and direct Bezier-net path fix artificial dependency growth;
the independent reader now also requires unforced p32 departures below 1e-10.
The source/error allowance is unchanged. Shared patch interpolation is unchanged.

Numerical peer reviewed and accepted pre-sort finite request admission, exact-t
overlap->needs_wider, fixed-vertex word preservation, the Cox/Bezier/index/cache
contracts, residuals, paired split and the new convex helper. Construction peer
accepted exact natural-boundary hull fallback. Their scoped reviews are direct
actual-source reads, not extra geometry campaigns.

A final cross-unit source read found the precise bound response omitted its
already-computed endpoint numerical upper; retaining this field prevents the
runtime's promotion branch from losing an actual point floor when it replaces an
RN bound response. The existing precise equality assertion also checks the .125
endpoint floor. This tiny response-field closure is being checked against the
same fixture before commit.

Final affected closure after the precise-floor fix: strict CPU reader PASS81;
ASan/UBSan/leak reader PASS81; named actual-source tidy PASS. Precise endpoint,
image and residual norms now reuse the already-owned relative-grid helper, so a
true 2^-600 endpoint departure does not acquire an artificial absolute 2^-192
floor. Existing line equality assertions cover both .125 and 2^-600. The common
HD source is unchanged by this host precise response closure; the coherent
convex-module reader has PASS81 on each P100. Final outcomes and module/source
hashes are retained in results/adaptive-curves-geometry-2026-10-16.

Cross-reviewed adaptive_traces.cpp numerical/domain portion: fixed two original
radius substitutions before scoped acceptance. Report is
papers/2026-10-16-adaptive-trace-numerical-review.md. Runtime cross-review found
a missing near-threshold escalation route (READY conservative upper>tau despite
small endpoint departure); runtime owner accepted and is closing a bounded
heuristic-to-exact route, with exact task comparison deciding acceptance. No
heuristic proves a bound, and large failures still request subdivision.
