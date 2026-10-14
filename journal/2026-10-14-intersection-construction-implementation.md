# Exact intersection construction implementation

2026-10-14. Root observed the genuine fleet red: healthy actual plane/wall
sampling, then missing `BodySnapshot.intersect_faces`. Implement the whole
accepted construction unit in `python/brepgpu/intersections.py` and a thin
lazy delegate in `body.py`, following the design and plan.

Subplan: own immutable quadratic Scalars with exact sign/comparison and
binary64 rounding; inspectable positive-denominator ExactMaps; exact FaceRefs
and result records. Enumerate all rational-quadratic trim roots, classify every
event and open cell, retain named trim tangencies, join only established chart
incidences and group duplicate generator occurrences into physical spans.
Construct both UV maps from the same common parameter and preserve original
body chart slots. Publish only the complete immutable result; unsupported
families and allocation failures publish none. Add the body-facing method by
lazy import/delegation, without a second intersection path.

Coordinate public fields with sampler/oracle owners. Local checks are AST/source
inspection only; root owns native execution and fleet qualification. No native
code, dependencies or other owners' files are included in this unit.

The whole construction and lazy adapter are on disk. AST parsing passed for
both files; root's actual first full-circle direct test passed on the fleet.
No product execution occurred here. Handoff records match the agreed source
slots, exact maps, boundary constraints, dense local IDs and span grouping.

Scalar arithmetic recognizes rationally equivalent radicands with gcd and
integer square roots, without factorization. Rational binary64 brackets use
exact integer quantization at normal or subnormal spacing; irrational brackets
use exact ordered finite-float comparisons. Both round at exact midpoints with
ties to even. This basic value conversion avoids repeated searches for the
rational interior samples without changing the numeric contract. Point pieces
retain active source-boundary annotations on their singleton domains.

Sampler and oracle owners received the actual API and review targets. Further
qualification, independent code review and corrections remain pending; one
circle green is not a claim of full supported-family qualification.

Independent runtime source review correctly narrowed the FP claim: integer
quantization itself is exact, but CPython float decoding/extraction is qualified
under round-to-nearest with gradual underflow. Corrected the module/quantization
comments to match that declared environment. No arbitrary DAZ/FTZ support or
new FP guard/decoder is introduced in this pass.
