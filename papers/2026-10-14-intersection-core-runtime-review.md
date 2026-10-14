# Exact intersection construction: runtime and numerical review

2026-10-14. Accepted with the FP-environment scope clarified. Static review of
[python/brepgpu/intersections.py](../python/brepgpu/intersections.py), body factory input records and the reviewed
intersection design/plan. Reviewed source SHA-256 `ce94248ed7f9f67b36a077b731a5e6282f99a24162741878a9c318730d6a1b63`. No product execution.
Root separately reported the actual first circle construction and direct
consumer tests green; this report is source review, not new qualification.

## Exact arithmetic and finite binary64 conversion

The one-radical and two-radical sign algorithms preserve the sign when squared
magnitudes are used only for opposing terms. The two-field comparison reduces
its squared magnitude difference to one radical; it does not approximate roots
or require factoring. Arithmetic recognizes rationally related radicands through
gcd and perfect-square quotient tests, rejecting unsupported mixed arithmetic.
Rational/perfect-square normalization and deliberately unhashable Scalars avoid
misidentifying algebraically equal endpoint values through representation hashes.

For positive rational conversion, exact integer bit lengths locate the exponent;
integer quotient/remainder quantizes either the normal significand or subnormal
2^-1074 grid. Irrational conversion uses exact ordered-float comparisons.
Negative brackets reverse the positive bounds. Midpoint comparison and the low
significand bit implement ties-to-even, including signed underflow-to-zero.
Values beyond finite binary64 are explicitly rejected. No floating discriminant,
approximate root or tolerance controls topology.

IR-C1, support clarification: these conclusions assume the supported CPython
execution environment, round-to-nearest and gradual underflow. Rational
quantization itself uses integers, but surrounding float input decoding/equality
must not be advertised as arbitrary FTZ/DAZ independent. CPython3.12
`float.as_integer_ratio` calls frexp; glibc2.39's subnormal frexp branch scales
its floating argument. This source path makes such broader independence
unsupported. Root accepted the explicit ordinary-environment scope in the design
and delivery guide; no bit-decoder/environment framework is selected here.
Sources: [CPython3.12 float implementation](https://raw.githubusercontent.com/python/cpython/v3.12.0/Objects/floatobject.c),
[glibc2.39 frexp implementation](https://raw.githubusercontent.com/bminor/glibc/glibc-2.39/sysdeps/ieee754/dbl-64/s_frexp.c).
No altered-environment experiment was executed by this reviewer.

## Construction, maps and graph

Original finite frame/control values enter Fraction arithmetic before geometry
subtraction, products or divisions. Primitive integer quadratic coefficients
avoid discriminant cancellation; linear/double/identically-zero cases remain
separate. Exact sorting merges equal roots even from different radical fields.
Uncapped exact dyadic witnesses lie inside every enumerated root-free open cell;
there is no rounded sample deciding an admitted interval.

Horizontal rectangle and disk/hole predicates have degree at most two after the
positive circle denominator is removed. All roots, endpoints and admitted cells
are retained. Identically-zero constraints become whole-span metadata; isolated
admitted roots survive when no curve incidence reaches their event. Atlas joins
share physical nodes but preserve source occurrences. Axial wall boundary tags
are attached to spans and events. The declared wall quarter parameter is native
u, not lifted quadrant+u.

Vertical branches use exact signed discriminants and exact height/lateral bounds.
The generator's rational circle inverse has positive radius+px after exact
quadrant admission. Cardinal/seam branches retain two chart occurrences sharing
one span and endpoint events; singleton height intersections are points.
Operand swap consistently swaps coupled source maps, source slots and tags.
Physical graph connectivity/closedness uses grouped spans once, not duplicated
chart occurrences. Source/UV/spatial maps remain inspectable and coupled on one
common exact parameter domain.

## Resource and failure scope

Only exact factory snapshots/FaceRefs enter the face operation. Supported geometry
has fixed chart/constraint counts; exact arithmetic allocation remains fallible.
Construction deliberately has no sampling precision cap. Any allocation,
unsupported-input or internal invariant failure occurs before a final immutable
IntersectionSet is published. Scalar is also a public algebraic value and does
not promise bounded arbitrary-user integer costs. Sampling has its separate
candidate/subdivision caps, source/grade checks and native children.

The operation owns no native resources, mutates neither snapshot nor topology,
and remains usable after Engine closure. General SSI, arrangements/coedges,
imported inputs and global GPU construction are outside this implementation.
Independent whole-map/graph/literal-fixture oracles and direct mutation assertions
are still required by the plan; this source review does not replace them.
No unresolved construction blocker found.
