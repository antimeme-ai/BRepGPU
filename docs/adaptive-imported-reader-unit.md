# Original-source saved-output reader breadth

2026-10-17. Finite PLAN; reader implementation follows parent review. Rendering
and this independent qualification are separate operations. This reader must
never block the first complete image or turn an incomplete/censored check into
a whole-watch PASS. No production code, dependency or native kernel changes.

## Actual gap and scope

Read native_adaptive_component.py::original_boundary_checker/original_facet/
check_output, native_adaptive_curves_caller.py::curve_h/curve_local/whole_chord,
native_adaptive_trimmed_caller.py::bernstein/prove_facet/qualify_face/cap_coverage,
and native_adaptive_model.py::check_output. Current imported component support
is analytic line/circle/ellipse and plane/cylinder, one original root per face;
curve_local only restricts degree1/2 NURBS. Model tests assume identity allowance
transport and authored box geometry. These are explicit gaps, not source faults.

The private retained source was inventoried on the fleet: all five analytic
carrier families plus635 rational carriers (624 degree2,1; one3,1; ten3,3),
original NURBS curves throughdegree5 in both dimensions. Every original use is
reconstructed, identity-map, zero periodic lift; no declared-degenerate or
zero-range spatial edge occurs. Dispatch uses source kind/degree/knots, never
these counts, source names or IDs. The finite numerical reader supports positive
rational curves degree<=32 and surfaces degrees<=3 in each axis, analytic
line/circle/ellipse and plane/cylinder/cone/sphere/torus with literal arbitrary
finite frames/radii. Existing mapped-cap source-quad reader stays the mechanism.
Unknown families, actual discontinuities, unproved line-image correspondence,
authoritative nonidentity domains and proof/work exhaustion are explicit failure
or UNRESOLVED, never skipped. No new inverse or source-domain producer.

## One original coefficient service for the existing readers

Add tests/native_adaptive_original_source.py as shared source algebra only.
One OriginalSourceOracle per loaded original SourceStore memoizes immutable
original-curve-span and carrier-span POWER coefficients, point terms and source
indices. Existing component/curve/trimmed readers consume it; no parallel mesh
reader or new acceptance/certificate tree. Existing authored fast formulas and
cap coverage paths remain available unchanged.

For B-splines, expand original unique knots/multiplicities, use natural
[K[p],K[ncontrol]], and enumerate positive original spans. Compute Cox basis
polynomials by level DP in normalized ORIGINAL span coordinates, then contract
with literal homogeneous controls. This is POWER algebra, independent of native
rounded/restricted Bernstein caches. Nonclamped knots, repeated knots, reversed
literal clocks and original right/left endpoint conventions are explicit.
Bezier controls are converted by the binomial identity on original [0,1].
Surface extraction is the tensor product of those one-dimensional polynomials.
Do not use saved adapter spans/display UV hints, interpolate a new polynomial
per emitted facet, recursively expand degree32 Cox, or rebuild old GeometryStore.
Source C0 admission compares exact one-sided rational endpoint values; surface
shared-edge identities are polynomial N_left W_right-N_right W_left=0. Unknown
or genuinely discontinuous required source closure refuses visibly.

## Actual rational facets and chords

Use the existing original-output simplex monomial-to-Bernstein identity and
SAME-L residual. For a carrier facet, locate candidate original spans by indexed
UV ranges. If all three exact UVs lie in one original closed box, no virtual
partition is needed. Otherwise clip ONLY at ORIGINAL knot boxes, carrying exact
original barycentric weights and actual emitted L; virtual points are not source
samples. Compose cached original homogeneous POWER with the affine piece UV.
Form R=N-WL before any norm hull, degree<=p+q+1<=7. Positive W coefficient hull
and vector residual coefficients prove ||R||<=tau*Wmin. If loose, subdivide the
same residual/weight polynomial virtually under the existing finite depth/work
cap. Sampling can falsify, never accept. Exact zero/equality remains included.

For original curve chords, partition only at original knot spans if necessary;
retain actual original t interpolation into SAME Q0/Q1. Compose cached span
POWER with that clock and form degree<=p+1 N-WL. Apply the existing whole-chord
residual hull/subdivision. The selected parameter upper must be proved when
parameter policy is selected. For geometric-line image policy, prove exact
control/polynomial collinearity and scalar monotonicity: derivative numerator
P'W-PW' has one sign, proved by exact Bernstein hull with bounded subdivision.
Stationary endpoints are allowed; identically constant/nonmatching scalar
extents refuse that image profile. Verify retained covector/source endpoint/
selected-Q scalars and traversal. Then SAME line-image departure is bounded by
actual endpoint departures. No min(parameter,image) without a checked map.

Chosen carrier traces S(affineUV) use the same original source polynomials and
SAME Q chord, degree<=p+q+1. Partition at original knot crossings in trace
parameter, preserving endpoint interpolation. ORIGINAL nodal delta remains
V/Ce(t) versus S(chosenUV), plus V versus Ce; constructed Q departure belongs to
epsilon. Raw mapped s/branch and all original source occurrence records remain
checked, including the explicit cap wedge sectors.

## Analytic originals and mapped caps

Extend existing original harmonic algebra to finite products of sin/cos factors
with exact original argument keys; combine identical literal factors/constants
before enclosure. This handles sphere/torus products and cone angle factors,
retaining original nonorthogonal frame columns and both literal ellipse radii.
Rational point values enter the same exact constant expressions. Existing
cylinder_trig exact Taylor intervals remain the trig oracle, including its
512-term explicit unresolved cap; no libm/Decimal value accepts a bound.

Whole ordinary analytic facets/traces use the existing independent pure/mixed
second-derivative Jensen/interpolation bound, literal source L1 frame bounds and
exact corner-departure intervals; virtual subdivision preserves SAME L. Sphere/
cone mapped facets keep prove_cap_facet on the finite reference quad and its
existing rectangular pure-second-derivative bound. cap_coverage remains the
single positive-chain coverage mechanism. No ideal-pole image is substituted
for the original finite source map, and no production cap constants are reused.

## Component/model adapter and world error

Remove the analytic one-root assertion: validate exported original closure by
source carrier and original positive knot spans (analytic one evaluator root).
Use existing per-face qualify_face and complete physical frontier mechanism.
For mapped results the frontier consumes their already source-checked protected
edges, not the pole locator triangle. Retained final incident chains must match.

Add saved-only model mode to SAME component reader: source bytes must equal
retained source.bgsource; read faces.tsv/queries.tsv/final-boundary, select query
and definition(s), and require the complete promised inventory. Local exact
allowances are read from metadata, never assumed equal to requested quality.
Do not invoke a native executable. Print separate per-definition/facet counts,
elapsed time and complete/UNRESOLVED/censored outcome. Partial selections label
their scope and cannot print complete-model PASS.

For optional complete scene/world qualification, compose ORIGINAL usage paths
exactly and verify existing scene instance matrices/provenance. Given local tau,
K2=max(1,max row-sum(abs(A^T A))) bounds original linear operator norm squared.
Let B be maximum squared ||(A-Astored)Q+(b-bstored)|| over actual definition
vertices. Convexity bounds this storage term for every emitted L. Prove
sqrt(K2*tau^2)+sqrt(B)<=requested epsilon using the exact nonnegative square-sum
criterion; prove K2*delta^2<=requested node cap^2. This uses actual emitted
vertices and avoids reproducing the production envelope/eta producer. If no
scene is retained, report LOCAL definition qualification only, not global image
placement accuracy. Existing scene/PNG structural check remains separate.

## Grounding, RED and bounded closure

Grounding already read: Filip/Magedson/Markot1986 §2/Theorems2/4 derivative
interpolation; Floater1992 §§1–3 rational quotient derivatives; Ma/Hewitt2002
§§3.6/4.1–4.4 Bernstein subdivision; Balazs/Guthe/Klein2004 §§4–7 source-to-mesh
composition. POWER/Cox recurrence, exact N-WL coefficient algebra and square-sum
world test are owned derivations. Native caches are never oracle input.

Before bodies, run SAME existing saved-output reader on one genuine rational
multi-span/degree5 source artifact and retain its unsupported-family RED. Reuse
existing authored C0/weighted outputs, north/cone mapped artifacts, and one
existing imported analytic component. If a needed source family has no saved
artifact, use the current bounded actual source caller once, not synthetic
success handles. No tests of the reader or alternate verifier suite.

Implement source algebra, wire into existing point/chord/facet calls and saved
model adapter, then run only this finite saved-artifact cohort on fleet. Check
one actual imported completed component when available; do not rerun native
low-level or full-watch meshing. Source code review attacks extraction endpoint/
clock conventions, SAME-L clipping and source error roles. Root owns any CMake
entry. Stop after this unit; full-watch qualification is a separately invoked
bounded read of its actual finished outputs.

Cost: exact Fraction source compilation is once per used span; residual work is
per actual facet/chord plus only necessary original-span partitions/virtual
tightening. Point/argument/cache indices prevent all-source rescans. This can
still take substantially longer than display, especially near bounds. Retain
explicit proof node/depth and optional elapsed limits; an exhausted reader
reports UNRESOLVED with checked counts. No claim of cheap complete-watch
qualification precedes an actual observation.
