# Cached adaptive geometry numerical unit

The original-source geometry reader passes on native CPU and both P100s. The
same emitted values are compared with independent Fraction source formulas and
high-precision original-argument trigonometric formulas; CPU parity is not the
oracle. This is a numerical-unit result, not a complete B-rep mesher result.

The device path actually evaluates original rational/analytic surfaces, restricts
a rational child control net, bounds original source against the four actual
bilinear corners, and bounds an actual affine facet. It covers nonorthogonal
plane/cylinder/cone/sphere/torus frames, finite very large/small plane frames,
and weighted extrusions in both directions and after degree elevation. The
reader checks source enclosure, stored-point departure and sampled original
source-to-actual-bilinear inequalities. The full-interval inequalities derive
from the reviewed production bounds; finite samples alone are not a universal
Hausdorff or embedding proof.

The main reader reports 214 observations and seven analytic families. Its 129
scalar trig cases, subnormal arithmetic case and precise rational fallback are
host checks even when the device backend is selected. The seven analytic
families each emit 25 actual device points and a source-to-bilinear bound. Three
additional weighted-extrusion bounds and the selected extreme-weight original
coefficient fallback also pass; that coefficient fallback is deliberately host
work. These counts do not represent independent benchmarks.

Strict Clang18 Release, ASan/UBSan/leak checking and owned-source clang-tidy pass.
No suppression was added. The genuine initial RED linked the actual source
reader and failed on the missing source-cache preparation, source evaluation
and actual-facet operations before their bodies existed.

The common CUDA17 header initially failed strict NVCC because host
`numeric_limits::infinity()` was called from device functions. It now constructs
the same infinity by its IEEE word. Numerical review also corrected interval
torus-radius addition, invalid interval propagation and scaled frame Gram
construction. Root module review corrected malformed root-net admission,
out-of-source node boxes and failed all-affine bounds requiring numerical
promotion rather than ineffective geometric splits.

A subsequent review tightened actual-facet transfer: take the smaller of the
six-coefficient residual upper and `||D||/4 + max_i ||B(p_i)-L_i||`. The latter
uses the unit-box covariance bound and preserves the same actual affine facet.
The exact fallback compares the sum of two nonnegative square roots by squared
arithmetic, retaining threshold equality. Two independent hand-derived saddle
facets, including a displaced actual facet, fail against the old body and pass
after the change. The updated CPU Release and sanitizer reader reports 216
observations plus the same seven analytic families; affected tidy also passes.
The final registered CTest run also passed the revised independent reader on
both P100s: 216 observations plus the same seven analytic families. The current
module SHA256 is
`e7c942eef2955d8e8adce4a2f1ab19d4584c621ca25dbb788728b6e8747fffd8`;
the current Release reader is
`e8f598e10cf59f429d0d1b7969b07e1d1b830e7b30f3a62d981b84f7d18636b5`.
The earlier hashes below identify the initial observation, which remains retained.

The observed device module SHA256 is
`4dc828ac7f701c84c10ce3fec024cb534e456ef87aeb38d300f8f2330d9f11d1`.
The Release reader is
`1f2a2e0104e06da5327bb4513f05a0834d8b8c5ada2c58e2a2dabc7f97ab9d0a`;
the sanitizer reader is
`22efa3ec8d5844b6a3f486b296175f27fdd67e83bacdab79dd5435668f0aa03a`.
Operational logs remain in ignored `context/`. See
[results.tsv](results.tsv) and
[the finite unit plan](../../docs/adaptive-geometry-unit.md).

No wall-time speedup, global injectivity, singular atlas, arbitrary trim or
whole-watch completion is claimed here. Complete source-to-cache-to-triangle
operations are measured by the adaptive runtime/caller, separately.
