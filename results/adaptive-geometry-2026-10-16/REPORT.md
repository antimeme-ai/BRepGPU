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
