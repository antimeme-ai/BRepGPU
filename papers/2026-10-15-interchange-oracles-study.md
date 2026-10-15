# Reference-to-owned carrier qualification

2026-10-15. Bounded oracle/corpus study; no implementation, reference execution,
or dependency adoption. Read project doctrine, the model-integration roadmap,
alpha policy, existing CPU benchmark and saved USD output, pinned NVIDIA schema,
Python bindings and tests, SMLib extraction declarations, and OCCT conversion
contracts. Proposed scope awaits the design and plan tribunal.

## Recommended first family

Deliver one actual reference-to-owned path for **positive-weight, nonperiodic,
single-clamped-span tensor NURBS carriers**, with complete source face inventory.
A source NURBS with counts equal to orders and only repeated endpoint knots is
already a rational Bernstein patch after an affine parameter map. This tests a
real interchange boundary without secretly implementing general knot insertion,
analytic angle conversion or trimmed topology. An explicit unsupported record is
a useful output; silently omitting a face is not.

The pinned NVIDIA schema stores Euclidean XYZ separately from positive weights,
U rows with V varying fastest, orders equal to degree plus one, and expanded knot
vectors. The owned evaluator consumes homogeneous controls. Conversion therefore
must multiply XYZ by W exactly once, preserve count/order distinctions and
record the original source domain. These are directly specified conventions,
not conclusions from a render. [Pinned NVIDIA schema](https://github.com/NVIDIA-Omniverse/usd-brep/blob/c9c5979f04cea8c38213004d4f04df0dcfb40d05/source/schema/omniSolid/resources/schema.usda).

Use a small reference-produced fixture family:

| Fixture | Purpose and expected disposition |
| --- | --- |
| Existing asymmetric polynomial surface from the CPU benchmark | Healthy pinned reference evaluator and a known analytic first-jet oracle; the existing 4×4 grid has S=(12u,9v,27u(1−u)v(1−v)). Actual exported representation must be inspected before declaring it supported. |
| Rational 3×4 net, degree (2,3), source domain U=[−2,3], V=[4,6] | Main admission: unequal orders, variable weights, nonzero domain origins and unequal scales make transposition and derivative-map faults visible. |
| Same carrier geometry used by distinct source faces, one opposite face use | Preserve occurrence/face identity and orientation despite equal geometry; intrinsic derivatives remain derivatives of the carrier. |
| Mixed model containing supported carriers and at least one multispan/analytic carrier | Every source face appears exactly once as converted or unsupported, including source type and reason. No successful whole-body-import claim. |

A concrete dyadic rational net is P[i,j]=(i/4,j/8,(2i+3j+ij)/64),
w[i,j]=1+(i+2j)/16, i=0..2, j=0..3. Its weighted coordinates are exactly binary64
representable, avoiding an accidental disagreement about conversion rounding in
the first decisive test. U knots are [−2,−2,−2,3,3,3], V knots
[4,4,4,4,6,6,6,6]. Add one ordinary non-dyadic positive-weight record to exercise
rounded homogeneous construction separately; do not use a circle fitted from
samples as its expected geometry. The recipe is proposed input, not an already
acquired or qualified source fixture.

The existing Python wheel supports scalar first jets and domains but does not
expose the complete control/knot extraction interface. The pinned C++
`SmBSplineSurface` exposes `GetDegree`, `GetKnots` and `GetControlPointNet`; the
last returns Euclidean controls and an empty weight array for a nonrational
surface. Python's surface normal follows intrinsic du×dv; face orientation is
separate. [Pinned bindings](https://github.com/NVIDIA-Omniverse/usd-brep/blob/c9c5979f04cea8c38213004d4f04df0dcfb40d05/source/SmPyLib/src/SmPyTypes.cpp),
[pinned extraction declarations](https://github.com/NVIDIA-Omniverse/usd-brep/blob/c9c5979f04cea8c38213004d4f04df0dcfb40d05/source/SMLib/inc/SmBSplineSurface.h).

Feasible routes are an isolated native reference emitter using the existing
reference build, or reference export followed by reading the actual USD
attributes. Choose after the runtime owner's environment probe. Do not substitute
user-supplied controls directly into owned patches and call that an import.
Retain original bytes/reference records independently of the converted controls;
never pass borrowed SMLib objects into the owned runtime. New rational native
fixture construction/export still needs an actual fleet probe; it is not proven
available through Python `create_surface_from_points`.

## Direct oracle

The oracle reads original source counts, knots, Euclidean coordinates and weights
without importing conversion helpers. Evaluate source Cox–de Boor basis and its
derivative with exact Fraction arithmetic, treating submitted binary64 values as
exact rationals. Form numerator A, denominator W and their partials, then use
S=A/W and S_U=(A_U W−A W_U)/W², likewise V. This is an independent source-basis
model rather than repeating the converter's array copy or the owned Bernstein
implementation. Endpoints require explicit one-sided closed-domain semantics.

Bind every evaluation to the **actual requested** source face and UV. For owned
coordinates s,t, source U=−2+5s, V=4+2t; require S_s=5S_U and S_t=2S_V before
any separate world/local scale. Check positions and each signed derivative
component, not only unit normals: normalizing conceals a missing chain-rule
factor. For a reversed or swapped chart, use the complete signed Jacobian.
Face-use orientation multiplies the derived face normal; it does not by itself
change intrinsic surface derivatives.

Require source-face/status bijection, source revision/asset identity, surface
record identity, chart identity, original domain, affine map, source orientation
and exact original-array retention. Reordering faces or packed geometry must
change slots without changing those keys. Equal geometric values must not merge
faces. Reject bool/int aliases and malformed index/array types where the owned
record contract requires typed identities.

For the admitted one-span family, complete conversion truth is cheap: check the
entire original control net and homogeneous correspondence, not just a grid.
The single-span basis identity establishes the carrier map over the full domain;
source-model first jets at corners, edges and asymmetric interior parameters
exercise the actual native caller and layout. Samples alone do not establish a
whole-domain bound for an arbitrary conversion.

For non-dyadic wP, distinguish exact source geometry from actual rounded owned
controls. Record/check each constructed homogeneous scalar against correctly
rounded multiplication, bound conversion displacement/derivative effects or
restrict the admitted numerical contract, and apply the established native
grades to the **actual emitted** controls. Do not silently use a loose reference
comparison tolerance as a claimed universal conversion error bound.

One reference evaluator comparison serves a different purpose: compare source
positions and natural-parameter derivatives at identical original UVs before
conversion, then compare the actual owned batch through the recorded map.
Reference parity is useful differential evidence, while the literal/exact model
provides the geometric answer. Preserve source numerical values and submitted
queries so evaluator disagreement can be diagnosed. No finite-difference second
oracle is needed for this rational fixture.

## Assertion-sensitive faults and negative cases

| Fault | Decisive direct assertion |
| --- | --- |
| Drop weights, preweight twice, or forget numerator weights | Variable-weight source Fraction position and both first partials; full homogeneous-net correspondence |
| Transpose controls or exchange counts/orders | Unequal 3×4 dimensions, nonsymmetric XYZ and whole source-net indexing |
| Normalize U/V but omit origin or derivative scale | Non-unit domains and signed componentwise chain-rule results at interior UVs |
| Reverse chart but lose derivative sign | Endpoint source map plus signed jets at asymmetric interior UVs |
| Treat opposite face use as the same outward orientation | Same geometry/opposite use, original orientation and signed face-normal check |
| Merge equal carriers or misattach a face after permutation | Complete typed source-face/status bijection and duplicate-geometry identities |
| Accept malformed packed slices | Wrong weight/control/knot lengths, nonmonotone knots, NaN/Inf, nonpositive weights, invalid face/surface index raise an explicit error before native evaluation |
| Approximate an unsupported source silently | Multispan/periodic/analytic record emits an explicit unsupported result with original provenance; no replacement patch |
| Confuse UV bounds with a trim | General-trim source metadata remains unqualified-for-trims even where carrier sampling succeeds |

Run only actual production-site mutants with healthy baseline/retry on the fleet;
these named faults characterize specific sensitivity, not a suite-wide mutation
score. External caller timing must separate reference extraction/export and owned
construction/upload/evaluation/reading; no SMLib Boolean acceleration follows.

## Existing corpus and alpha limits

The saved `results/cpu-2026-10-13/usd_roundtrip_bore16/bored16.usda` has six planes
and sixteen cone-schema surfaces with zero semi-angle (cylinders), general trims
and seventeen loops on each cap. Its face range is not the cap's material region.
It is a valuable mixed inventory/provenance case; it is **not** a positive-weight
NURBS-only fixture. The current benchmark checks volume/bounds/probes and merely
requires a nonplanar face to survive. It does not qualify our carrier adapter,
parameter maps or trim preservation.

Pinned NVIDIA `TestFiles/usd_TestFiles/sphere_nurbs.usda` is actual text, with two
3×3-order NURBS surfaces and 7×5 controls: multispan/singular/seam corpus debt,
not a passing first-family fixture. `TestFiles/occt_breps/bezier_surface.brep`,
`bspline_surface.brep` and `periodic_bspline.brep` are relevant future adapters.
The NVIDIA malformed-surface validator fixtures include wrong control/weight
lengths, knot lengths/order, nonpositive weights and wrong types. Inventory their
applicability; running NVIDIA validation alone does not check our outputs.

OCCT's pinned `GeomConvert_BSplineSurfaceToBezierSurface` explicitly decomposes
along knot spans using knot insertion and preserves carrier orientation. It is
an appropriate future independent conversion/evaluation reference, not a reason
to adopt OCCT or claim general decomposition now. [Pinned OCCT contract](https://github.com/Open-Cascade-SAS/OCCT/blob/b8f597c677811d1f9f4d8a97f5ae2825c0353a42/src/ModelingData/TKGeomBase/GeomConvert/GeomConvert_BSplineSurfaceToBezierSurface.hxx).

Per `docs/ALPHA_QUALIFICATION.md`, applicable external cases must be inventoried
against advertised scope before seeing results. Missing data/runner/adapters,
unsupported promised cases and disagreements stay visibly non-green. A narrow
carrier study does not make STEP import, general USD composition, cross-session
persistent naming, topology conversion or trims alpha-qualified. NIST/CAx-IF
exchange/validation properties remain relevant to eventual exchange claims;
they do not independently determine these derivative-map answers. No broader
release gate or library adoption is proposed here.

## Assessment of the concrete normalization proposal

Root's proposed typed OpenUSD read of actual exported arrays fits this scope:
clamped single-span degree 0..3, retained file hash/prim path/source face index
and original face-use occurrences, and an explicit trims-not-imported marker.
Degree-zero axes need one control/order one, positive domain length and exactly
zero derivative; singular normal is not malformed position geometry.

A common exact power-of-two weight factor preserves rational basis coefficients.
Admission must establish that the actual factor exists with every normalized
weight in [1,256] and exact finite scaling. A weight ratio at most 256 alone is
insufficient: weights 1.1 and 270 have that ratio but no common power-of-two
factor putting both in that interval. Reject unsupported numeric ranges rather
than underflowing, clamping or rounding their weights.

If those weights remain exact, rounded homogeneous XYZ only changes Cartesian
control positions. With nonnegative rational partition-of-unity coefficients,
the maximum Cartesian control displacement delta bounds position error over the
whole carrier in the same norm. Independently compute delta from actual emitted
Hxyz/Hw reconstructed through the recorded world frame against original source
XYZ, using exact rationals. This includes any rounded frame-origin subtraction
or coordinate construction; counting only the final multiplication is unsafe.

That position bound alone says nothing sufficient about derivative error. For
single-span source degree p, interval length L and weight ratio R, a safe bound
on conversion first-partial error is 4pR*delta/L. Indeed the Bernstein derivative
absolute sum is at most 2p/L, and differentiating normalized rational coefficients
bounds their absolute derivative sum by twice R times that quantity. The same
argument applies tensorwise; degree zero gives zero. Native evaluation error is
separate and transformed through the recorded scale/Jacobian. Either adopt such
a declared conservative budget or describe source derivative comparisons only
as sampled diagnostics; do not imply they follow from a whole-position bound.
