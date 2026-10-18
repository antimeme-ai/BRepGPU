# Native mesher: original geometry, correspondence and coverage implementation plan

Status: accepted in the independent whole-plan review after design `f7aedef`.
Implementation is underway. This specifies procedures and admission; completed
capabilities and their limits are recorded in the implementation journals.
It is the geometry owner of [the complete operation](NATIVE_MESHER_PLAN.md),
using [runtime values/tasks](native-mesher-runtime-plan.md) and supplying
[shared features and face construction](native-mesher-topology-plan.md).

## 1. Truth, ownership and initial admission

The immutable original source coefficients, knots, analytic formulas, vertices,
spatial curves, authoritative pcurves and explicit occurrence maps remain the
inputs. Saved extracted span controls and reconstructed pcurves are hints.
Source tolerance metadata never enlarges requested epsilon. The output names
its chosen chart/domain, original discrepancy, correspondence and quotient.
There is no promise of distance to an unspecified exact solid behind inconsistent
representations, or eventual success for every parseable source.

Here epsilon denotes the effective definition-local positional allowance passed
by the root operation after transport through every actual assembly instance,
including its stored-transform error. Geometry does not independently reset it
to the user's world tolerance. The separate local `delta_node` is the requested
**node** source-recovery cap, transported only by the original source instance
norm kappa; no stored-output-matrix eta enters that source-only comparison.

Preserve the existing `source_residual_mm` input meaning. At each original
vertex/edge sample and each incident use, compare source target V or exact
Ce(t_i), respectively, to original S at the **stored chosen original-UV recovery
parameter**. This is the actual retained dyadic locator value, including branch/
lift; it is not an unrepresented exact inverse root. Even a physical-plane
chart's implicit Psi does not substitute for that stored recovery parameter.
Also retain the original endpoint-to-curve node check `||V-Ce(t_endpoint)||`
against delta_node when an original curve exists. Constant/declared quotient
edges without such a curve use their explicit source-generator branch, not an
invented curve comparison. These are source-geometry checks; canonical XYZ
rounding already enters actual mesh residual R and is not added here again.

Whole-interval Ce versus selected/reference trace approximation and original
Ce versus actual physical chord each use the **positional epsilon** allowance,
not delta_node. The face-domain-displacement plus actual-cell sum likewise uses
epsilon. No claim of a whole-curve closest-carrier residual below delta_node is
made. Source uncertainty remains separate. NativeMesh uses distinct node-carrier,
node-endpoint-curve, trace-position, chord-position, domain and actual-cell tags;
neither input budget silently replaces the other.

Compile rational spatial/pcurves through degree32 and positive rational tensor
carriers through degree(3,3), plus the existing plane/cylinder/cone/sphere/torus
formulas and supported analytic curve kinds. Preserve curve dimension: a
dimension2 homogeneous row is `(x,y,w)`, not a stride4 world point. An analytic
dimension2 curve uses the XY components of its retained frame expression;
inventing a new Z component changes that source. Degrees0 and constant curves
take explicit constant branches before division by a range/derivative.

For NURBS compile the true natural knot rectangle
`[U[p],U[n+1]] x [V[q],V[m+1]]`, once per original carrier. The adapter's
`carrier.uv_bounds` is a face-envelope hint, not authority to truncate those
roots. A carrier can serve several faces. Each face generation separately owns
its chosen working-chart bounds, including all lifted seam copies and traces;
those query the original roots and determine proposal metric scales. A NURBS
query outside the natural rectangle is unsupported, not extrapolated silently.
There is no NURBS periodic metadata in BGSRC001: do not infer it from bounds or
2pi. Internal positional discontinuities reject unless matching exact one-sided
limits are established. A declared analytic seam/pole is distinct from an
unrecognized NURBS singularity; general new atlases remain unsupported.

Use namespace `bgmesh`, shared `SourceView`, typed slots and immutable
`ExactWords`. `GeometryStore` owns sealed source-derived exact roots and frame
data. `OwnerGeometryCache` owns restrictions/query scratch for one face/edge.
Every arithmetic operation takes `JobContext&`; no root captures a mutable
`bgfd::Budget`. Runtime owns rational/interval arithmetic and residual task
execution. Geometry owns the source-to-task construction below. No new library,
Python callback or foreign evaluator is selected.

## 2. Original B-spline extraction and reusable local hierarchy

Implement exact Cox recurrence **on each positive knot span**, avoiding an
assumption that original arrays are clamped or that a rounded saved net is exact.
Expand source knots using original multiplicities. On span `[U[k],U[k+1]]`, put
`u=U[k]+s*(U[k+1]-U[k])`. The degree0 basis is1 for index k and0 otherwise on
the span interior. Build higher degrees by

    N[i,r] = (u-U[i])/(U[i+r]-U[i]) * N[i,r-1]
           + (U[i+r+1]-u)/(U[i+r+1]-U[i+1]) * N[i+1,r-1].

A zero denominator contributes zero. Each multiplier is affine in s. For an
affine multiplier with endpoint values a0,a1 and degree(r-1) Bernstein vector b,
the degree r product has coefficient

    c[j] = (r-j)/r * a0*b[j] + j/r * a1*b[j-1],

with missing b entries zero. Thus this is a finite exact rational transform,
not numerical sampling/inversion of a Vandermonde system. Apply its active
coefficients to original homogeneous controls. Apply independent U/V transforms
to the tensor net. Extract one-sided closed span values consistently; shared
limits at continuous knots must agree exactly. At an internal multiplicity
degree+1, compare complete rational limit curves by homogeneous cross products,
not just endpoints or raw control equality, before admission.

Each root records original curve/carrier, knot-span ordinals, natural range,
degree/dimension, exact homogeneous net and denominator positivity. Positive
original weights plus nonnegative basis give a positive exact denominator on
the natural span. Restrict active ranges using exact de Casteljau, retaining
the affine restriction map. Build a UV rectangle index for tensor roots and an
ordered interval index for curves. Exact position query first finds its root;
cell queries enumerate only intersecting roots. Do not scan all spans per cell.
Local midpoint restrictions use their parent net. Source roots are immutable;
query/restriction children are bounded owner caches, recomputable on eviction.

Homogeneous first/second derivatives use adjacent control differences with
degree/range factors. For S=N/W and coordinate i,

    S_i = (N_i-S*W_i)/W
    S_ij = (N_ij-S_i*W_j-S_j*W_i-S*W_ij)/W.

Evaluate these as enclosed operations with a proved positive denominator.
Exact rational values or interval jets are distinct results; an approximate
jet used for metric proposals is not a bound. Continuity intervals and cached
derivative nets prevent repeated span searches/extraction in refinement.

## 3. Ordinary rational cells: finite restriction, actual residual

For an actual triangle in original UV, query the span index, clip against every
intersecting knot rectangle with exact rational half-plane clipping and
triangulate each nonempty convex intersection as a fan. These are **bound
subcells**, not new topology nodes. Reject/handle lower-dimensional intersections
as zero-area bound pieces while preserving all positive-area coverage. Use one
canonical ownership for coincident span boundaries. Compare every subpiece with
the same original emitted affine XYZ map L; do not evaluate and round substitute
physical corners at clip points. Exact affine combinations of actual output
words define L at those points.

Given span-local triangle vertices `(s_a,t_a)`, write s and t as affine
barycentric polynomials. Construct `B_i^p(s)*B_j^q(t)` by products of those
affine forms, equivalently tensor blossoming into a triangular Bernstein basis.
For two triangular Bernstein polynomials of degrees m,n the coefficient rule is

    C[gamma] = sum_(alpha+beta=gamma)
      multinomial(m,alpha)*multinomial(n,beta)/multinomial(m+n,gamma)
      * A[alpha]*B[beta].

Sum original homogeneous controls against these products. N,W have total degree
at most p+q<=6. Elevate N to degree d+1 by multiplying by the constant1 polynomial;
multiply W by degree1 **actual** L and form R=N-WL. Residual degree<=7 means at
most36 coefficients per coordinate. For a curve/chord the same univariate
construction gives degree<=33 and34 coefficients. Recenter around one exact
anchor algebraically before interval conversion; no rounded subtraction defines
new source controls.

Submit the runtime's `RationalTriangleTask`/`RationalCurveTask`, with exact
nonnegative allowance tau. Acceptance is every exact squared coefficient norm
<=tau²*wstar², where wstar is the minimum positive W coefficient. CPU/P100
intervals filter the same predicate; unresolved rational comparisons use owned
exact fallback. Original extraction roundoff is absent on the exact path and
enclosed on the interval path. Actual XYZ already contains canonical nodal
departures/rounding: never charge them again. A failed conservative comparison
requests refinement; it does not prove an impossible tolerance.

## 4. Analytic formulas, enclosures and whole-trace host bounds

Retain frame O,X,Y,Z as given; it need not be orthonormal. Let
`E=X*cos(u)+Y*sin(u)`, `E'= -X*sin(u)+Y*cos(u)`.

| Carrier | Original formula | Nonzero second derivatives |
| --- | --- | --- |
| Plane | O+uX+vY | none |
| Cylinder | O+rE+vZ | Suu=-rE |
| Cone | O+(r+v sin(a))E+v cos(a)Z | Suu=-(r+v sin(a))E; Suv=sin(a)E' |
| Sphere | O+r cos(v)E+r sin(v)Z | Suu=-r cos(v)E; Suv=-r sin(v)E'; Svv=-r(cos(v)E+sin(v)Z) |
| Torus | O+(R+r cos(v))E+r sin(v)Z | Suu=-(R+r cos(v))E; Suv=-r sin(v)E'; Svv=-r(cos(v)E+sin(v)Z) |

First derivatives follow directly from those expressions. Compile shared frame
Gram bounds once. For the two-column frame A=[X Y], use the enclosing largest
eigenvalue of its exact2x2 Gram matrix, rather than the avoidable factor from
`||X||+||Y||`. A local analytic triangle bound can use

    (Muu*du² + 2*Muv*du*dv + Mvv*dv²)/8
      + max_vertex ||S(uv_i)-actual_XYZ_i||.

The factor follows from barycentric variances<=range²/4 and the Taylor remainder;
it is valid for any triangle contained in that rectangle. Include mixed terms.
This bound charges nodal displacement once; direct actual residual bounds do
not add it again. A source C0 break is partitioned, never crossed by a C2 formula.

Own analytic enclosures: cache rational pi brackets from Machin's identity
`pi=16 atan(1/5)-4 atan(1/239)` using alternating-series remainders. Reduce trig
arguments by enclosed multiples of pi/2, split an ambiguous quadrant, then use
rational Taylor polynomials with explicit remainders on |x|<=pi/4. Enclose the
whole interval, including extrema. Directed host primitives are runtime-owned;
ordinary sin/cos/atan2/Newton results are locator proposals only. Canonical
precision stages, term counts and source interval order are fixed by input and
limits, independent of backend completion. Unresolved angle/range reduction
may use [-1,1], then refine or return precision/resource failure. Symbolic pole
constants below do not become binary64 approximations declared exact.

For a reconstructed affine original-UV trace, exact rational spatial Ce (degree
<=32) and rational S composed with that trace (degree<=6) have a cross-multiplied
common residual degree<=38. Build it on the host by univariate Bernstein
convolution, degree elevation and exact affine composition, preserving positive
denominators. For an authoritative rational pcurve degree<=32, composition with
a degree(3,3) surface has degree<=192 before comparison with the spatial curve;
cross multiplication with a degree32 spatial curve gives degree<=224.
This is a bounded **host composition**, not a36-coefficient GPU task or an
arbitrary symbolic expression engine. Common denominator factors/zero identities
are canceled exactly where established. Account degree, coefficient count,
limb/work and temporary storage before multiplying.

For pcurves crossing surface knot lines, no algebraic preimage is rounded to a
physical parameter node. Subdivide the exact source parameter interval, use its
UV hull to enumerate candidate original spans, and evaluate each candidate span's
polynomial extension on that whole small parameter interval. Maxima over these
extensions cover the actual piecewise carrier wherever that span applies.
Require every extension denominator positive on the bound interval; otherwise
subdivide. At a continuous knot these local extensions agree at the crossing,
so tightening is possible without solving its algebraic parameter exactly.
Terminate under the normal resource/precision limits. Exact affine-map knot
preimages can instead be used as rational bound boundaries directly.

Analytic trace mismatch uses an enclosed **coupled residual**
`r(t)=Ce(t)-S(Psi(gamma(theta(t))))`. Where a second-derivative formula is chosen,
bound `max(||r(a)||,||r(b)||)+sup||r''||*(b-a)²/8` with full chain rule

    (S o gamma)'' = Su*gamma_u'' + Sv*gamma_v''
      + Suu*(gamma_u')² + 2*Suv*gamma_u'*gamma_v' + Svv*(gamma_v')².

Do not replace this with two independent global position hulls; their fixed
width fails to contract with interval length. Symbolically shared affine/line
terms are canceled before enclosures. The checked image profiles next handle
different parameterizations without pretending their same-t residual is zero.

## 5. Selected traces, maps and useful exact geometry profiles

Choose working chart first. Initial charts are original UV, admitted analytic
unwrapped UV with explicit occurrence lift, and the finite physical-plane chart
in section6. Original edges retain one master parameter schedule and canonical
XYZ. The endpoint mapping is literally parameter_range endpoints to vertex_ids;
do not reverse it again using curve_same_sense. Dimension2 pcurve parameter maps
are identity or the retained exact piecewise-affine map. Map kind3 is an
unproved continuation guide in the current reader and remains unsupported until
its own bounded implementation; do not turn it into identity. Map kind4 is a
measured piecewise-linear map with a sampled residual per row (step-source
adapter, non-SameParameter uses); it is refused by name everywhere until a
consumer charges that residual to its error budget, and is never exact.

Choose one dyadic joint per face/source-vertex branch before sampling incoming
uses. Recover candidate chart sites by retained branch guides plus plane dual
coordinates, analytic angle/height inverses, or local NURBS solves. Their finite
binary64 outputs are exact dyadic **chosen sites**, not exact inverse roots.
If a locator fails branch/range/whole-interval discrepancy checks, increase its
precision, request master splits or return unresolved. An all-reconstructed face
domain is the finite chosen polygon in this chart; its E_domain is0 relative to
that named realization. HybridPullback mixed faces instead retain section7's
explicit joint-star domain displacement. Whole original spatial-edge/carrier
mismatch still applies in both policies.

Canonical XYZ is a separate deterministic host operation. Preserve original
vertex words. At a rational original-curve/surface point, evaluate the original
homogeneous expression exactly and correctly round each coordinate once to
nearest binary64, ties-to-even. At an analytic point refine source enclosures
until each selects one rounding cell, using exact special-value identities for
known ties/zeros; return precision failure if the unique rounding cannot be
resolved under limits. Interior plane-chart nodes use the exact planar lift of
their chosen XY, rounded once; the whole height bound charges its source departure
without an implicit UV solve. Pole cut nodes use their original source carrier;
original-edge nodes remain their physical edge owner's Ce evaluations. Approximate
display normals do not enter this canonical point operation.

Default `AffineParameter` maps each master interval affinely onto its chosen
chart segment. Two checked alternatives avoid parameter-speed overrefinement:

* `GeometricLineScalar`: choose a fixed exact scalar covector a, put
  `p(t)=a dot Ce(t)`, and prove p is strictly monotone onto its endpoint range.
  Use `sigma=(p(t)-p(a))/(p(b)-p(a))`, not a requirement that original scalar
  endpoints are already0,1. Bound `Ce-Qa-sigma*(Qb-Qa)` in homogeneous Bernstein
  form, including actual canonical endpoint departures. Rational derivative
  numerator `P'W-PW'` has one proved sign; nonnegative, nonzero polynomial is
  strict except isolated stationary values. A derivative identically zero is
  a constant branch. Exact nonlinear straight curves can have zero transverse
  residual. A coupled trace must separately prove it covers the same geometric
  segment; the two monotone maps may differ. No inverse root is a topology site.
* `CylinderArcOrder`: in the retained carrier dual frame prove a rational curve
  has constant axial coordinate and circular radial locus (or a stated bounded
  tube). Prove the angular derivative numerator `x*y'-y*x'` has one sign and
  the interval lies in a strict radial half-plane; subdivide otherwise. Endpoint
  ray descriptors, original lift and traversal establish the ordered source
  arc. Chosen dyadic angular endpoints generally differ from its true endpoint
  angles. Isolate their endpoint-angle differences by branch-aware rational
  angle bisection with enclosed sin/cos and ray cross/dot signs, never an exact
  atan topology coordinate. The monotone angle correspondence between the two
  endpoint ranges has angular discrepancy bounded by the maximum endpoint
  discrepancy. Charge `||[X Y]||*r*delta_angle` in both directed source/selected-
  trace image bounds, plus any distinct bounded radial/axial departure. Only
  exact endpoint-ray agreement permits calling the arcs identical. Chord sagitta uses
  an enclosing `||[X Y]||*r*(1-cos(delta/2))` for delta<=pi, plus actual endpoint
  departures. Exact circle identity is established from source polynomial
  coefficients; near-circle data does not pass as exact. A tube needs its own
  bounded radial/height error. No exact atan value enters CDT predicates.

Cone generators and arbitrary spatial straight curves use the line profile.
Failed profiles take the valid default correspondence, not a source-name bypass.
Store the finite profile tag and original interval/scalar/branch data in output.
There is no arbitrary expression bytecode or callback map.

## 6. Finite physical-plane inverse-coverage chart

Try this profile before defining reconstructed traces; failure falls back to
original UV. On a convex finite **natural original** rectangle R, choose a fixed
exact projection/preconditioner F=Pi*S. Prove a uniform positive lower bound on
the symmetric Jacobian of F by interval/exact derivative bounds on the source
hierarchy. In2D the exact diagonal/determinant tests or an enclosed eigenvalue
give that lower bound. Prove each projected natural-boundary curve monotonically
covers a straight segment using the line profile. Require four common exact
endpoint images and strict convex Q. Strong monotonicity gives injectivity;
boundary homeomorphism and continuity give F(R)=Q. Store this finite inverse
descriptor Psi=(F restricted to R)^-1; do not solve an implicit UV triangle.

Use exact dyadic XY topology sites and prove each cell/domain lies in Q by
convex half-plane tests. Whole-original-rectangle height control nets bound every
inverse image, including interior height bubbles. Compare the carrier's planar
lift of XY to the actual emitted affine XYZ: the latter's three residual corners
bound its affine difference. Add whole height and projection/XYZ deviations
once, with the exact world frame norm. Reconstructed traces are chosen directly
in this chart; an unnecessary original-UV polygon is never charged first.
Authoritative traces project their original curved domain and retain section7,
using the finite source-point/star evaluation below for nonrational projected
nodes. This is direct source projection, not an inverse UV nodal-star solver.

Original-UV corner display locators may use enclosed inverse brackets and
approximate values; these are consumer fields distinct from Psi. The initial
profile needs no arbitrary local inverse-coverage solver. The nonlinear planar
square `S(u,v)=(u+alpha*u*(1-u),v,0)`, |alpha|<1, fits: two triangles are possible
independent of parameter speed, using line correspondences on its straight
geometric edges. Curved planar trims still require their own boundary density.

## 7. Authoritative and mixed domains: finite strips and nodal stars

Authority is per coedge, not a face-wide Boolean. All-reconstructed faces keep
their chosen polygon reference and identity domain map. All-authoritative faces
retain their original curved reference, mapped by B then T below. A mixed face
uses the named `HybridPullback` reference, constructed as follows.

At a joint incident to an authoritative arc, reference endpoint A is that
original authoritative endpoint; two incident authoritative endpoints must
agree under the existing original closure/branch policy or the input is open/
invalid/unresolved. Pick the one common dyadic Q for the joint and its nodal B
fan A to Q. At a joint with only reconstructed uses, reference endpoint is Q.
For each reconstructed use define its reference trace as
`Gamma_ref=B^-1(Gamma_selected)`, where Gamma_selected is its already chosen
finite polygon. The inverse is explicit on each image fan sector(Q,P_i,P_j):

    B^-1(y)=y+orient(P_i,P_j,y)/orient(P_i,P_j,Q)*(A-Q).

It fixes P's boundary and is identity outside P. Thus the mixed reference is
closed at A without redefining an authoritative arc, welding two original
endpoints or promoting reconstructed guides to source truth. General inverse
roots are not introduced: this is a finite affine/source-point descriptor,
with a rational positive denominator from Q and P. The global inverse follows
the stored fan order reversed; the disjoint fans commute, but no private order
is inferred. First apply all B: reconstructed uses become **exactly** their
selected polygons and authoritative arcs become B(original arc), with common
dyadic Q endpoints throughout.

Preserve original loop order and every per-use authority flag. An unexplained
authoritative gap is invalid/unresolved, not a proximity weld. Rational pcurve samples at dyadic master t may be nondyadic
UV, so their chosen dyadic sites **do not lie on the curve by definition**.
This displacement needs an explicit domain map, not just a point residual.

Use one finite construction for original vertices and displaced internal sample
joints. A source node A is either an exact rational point, the finite descriptor
`OriginalPcurveAtParameter(use,s)`, or
`ProjectedPcurveAtParameter(chart,use,s)`, where s is the original exact mapped
pcurve parameter. The last two name the original source formula, not a rounded
sample: analytic circle/ellipse nodes may be transcendental. Evaluate A by the
owned source enclosure procedure. At A and chosen dyadic Q, choose a rational
convex polygon P containing both strictly inside, disjoint from nonincident
features and other such nodal polygons. The PL fan mapping
`(A,P_i,P_i+1)->(Q,P_i,P_i+1)` fixes P's boundary and is a homeomorphism because
every source and destination fan has positive orientation. Its point displacement
is bounded by ||A-Q||. Explicitly on sector(i,j),
`B(x)=x+lambda_A(x)*(Q-A)` with
`lambda_A(x)=orient(P_i,P_j,x)/orient(P_i,P_j,A)`. Positive denominator/orientation
tests use exact rational arithmetic when A is rational, otherwise its original
source intervals; uncertainty requests tighter A enclosures or failure. P is an
exact rational support value; only dyadic Q is a topology site. A never enters
the CDT. The map is affine in x
with fixed source-defined coefficients. A rational source arc/A stays rational;
the analytic case is a bounded original-source evaluation followed by this
finite affine map. Piece selection uses interval half-plane tests; uncertain
intervals are subdivided, with all possibly applicable affine-piece extensions
enclosed. No algebraic fan-boundary root becomes a mesh node and no arbitrary
expression bytecode is introduced.

Build these neighborhoods by indexed feature exclusion, not a global all-pairs
scan. For rational branches isolate the first exact nonzero homogeneous
endpoint-difference coefficient (deflate its endpoint power). Analytic regular
branches use a derivative interval excluding zero and exact special-point
identities; no unimplemented arbitrary analytic leading-jet search is assumed.
Construct rational separating cones. Verify the entire small branch and its derivative remain in
the chosen cone and have strictly increasing radial projection until the exit.
Source-parameter exit brackets are bounded interval locators. Distinct outgoing
cones give cyclic order; a smooth C1 join has opposite outgoing directions after
reversing its incoming traversal. Exact straight continuation uses monotone line
profiles. A same-direction tangent/cusp with no implemented separating profile
returns `UnresolvedStar`, rather than invoking unspecified higher-order topology.
P must be small enough for the local physical displacement bound and large enough
to contain Q; increase locator precision when those demands conflict.

Compose all disjoint nodal PL fans FIRST, obtaining transformed arcs with exact
dyadic endpoints Q_i,Q_next. T supports are constructed **only for authoritative
arcs**; they must exclude reconstructed selected polygons except declared common
Q endpoints. For each **whole** transformed authoritative arc between these
endpoints choose rational chord coordinates x,y with chord y=g(x), x in[0,X].
Prove strict monotone x by Bernstein derivative signs or analytic interval
subdivision over all possibly applicable fan pieces. Prove a bound m on
`|d(f-g)/dx|` and a whole-arc transverse bound. Choose rational c>m and a cap w0
so the explicit tapered support

    g(x)-w(x) < f(x),g(x) < g(x)+w(x),
    w(x)=min(c*x,c*(X-x),w0)

holds strictly for0<x<X. Prove the capped middle by the whole-arc bound and the
ends by the slope bound. Endpoint cone separation must make these supports
disjoint from other incident supports except the fixed Q; indexed exclusion
handles nonincident transformed features/supports. If those inequalities cannot
be established, request bounded master subdivision/more precise chosen sites,
or return `UnresolvedStrip`/`UnresolvedStar`.

On each vertical fiber define the explicit map with knots
`(g-w,f,g+w)->(g-w,g,g+w)`, affine on each of its two intervals, and identity
outside. It is strictly increasing and extends continuously as the identity
at Q because w vanishes there. Thus every graph/chord strip has a concrete
homeomorphism with fixed tapered boundary; no matching gate or radial exit map
is assumed. Compose the nodal map B then these pairwise-disjoint fiber maps T.
Stars may overlap strips because this order is explicit; strips do not overwrite
one another. Curve/chord coincidence takes the identity branch, avoiding0/0.
Admit the entire mixed boundary, with all original holes, under this common
construction and final exact selected-polygon admission. Since h=T composed
with B is a homeomorphism and maps every authoritative or pulled-back
reconstructed reference arc to its selected boundary, the reference and selected
domains have the same ordered loop topology. A T support that changes a
reconstructed selected polygon is a construction failure, not an extra map step.

These checks establish a directed onto domain deformation with support in the
stored strips/stars. Preserve hole ownership and winding; never drop a thin
hole. For a boundary cell, enclose its inverse T/B image p and its selected
points q, requiring both actual domains inside the admitted convex original
carrier rectangle R. Bound the total UV displacement by the componentwise sums
of the ordered B/T moves; do not take an unjustified maximum of sequential
movements. Query source derivatives only over the convex endpoint hull inside
R, and use `sup||Su||*delta_u + sup||Sv||*delta_v` to bound S(p)-S(q). The straight
segment between the endpoints stays in R even when an auxiliary intermediate
map point does not. Across C0 knots, continuity plus all queried one-sided span
derivative bounds gives the same piecewise Lipschitz inequality; no C2 theorem
or source extrapolation is used. Disjoint branches need only their local maximum.
A conservative carrier-image diameter over this admitted endpoint hull is
allowed when a tighter displacement is unavailable. The direct acceptance is
`E_domain(cell)+E_actual_cell(cell)<=epsilon`; interior E_domain=0. Source
edge-to-chord and original-edge/carrier discrepancies remain separately named
checks. The local allowance is chosen from actual bounded terms, not a global
epsilon/8 split. A whole star diameter that consumes epsilon requests a smaller
star/more precise site or a typed failure, never an enlarged epsilon.

HybridPullback charges B/T displacement on **all** affected cells, including
reconstructed pullback neighborhoods; E_domain is not silently zero there.
Its reconstructed source-edge/carrier reference bound uses the already bounded
Ce to S(Gamma_selected) discrepancy plus the enclosed local carrier displacement
from B^-1. This whole-reference approximation sum must satisfy the definition-
local **positional epsilon**, not the node recovery cap delta_node. The domain/
actual-cell sum separately satisfies that positional allowance; original node
source-recovery checks remain as in section1 at their stored recovery UV values.
These are different named comparisons, not a grant of extra epsilon. Away from
the authoritative stars, B^-1 is identity and reconstructed references retain
their original selected-polygon meaning.

For a physical-plane chart use its direct geometric decomposition instead:
the domain term is the plane-lift operator norm times the composed XY movement,
and the whole-original-rectangle height plus actual affine plane-lift departure
is the actual-cell term. This compares the original authoritative point to the
emitted facet directly, charging the height once; it does not add a second
2*height bound through G(p)-G(q). Original points, every reconstructed reference
arc B^-1(Gamma_selected), and final chosen domains must lie in Q. Check the
inverse fan's possibly applicable affine pieces against all convex Q half-planes
using original source enclosures; an inside selected polygon alone does not
establish this pullback condition. The full closed reference boundary in convex
Q also bounds its enclosed domain there. A failed/unresolved check requests
different Q sites/support precision or a bounded refusal. Auxiliary PL support
polygons can still extend outside Q and be bounded in XY without evaluating
an inverse there. Likewise original-UV P/strip neighborhoods are **ambient map
supports** and may cross a natural carrier boundary. Original/reference and
final domains, recovery points and carrier-evaluated endpoint hulls must remain
in R, checked by affine/source half-plane enclosures before source evaluation.
An original-UV auxiliary support outside R alone is not failure; evaluating the
source there or an actual reference/final domain excursion is unsupported.
For example on S(u,v)=(u,v,v²), R=[0,1]², an authoritative boundary sample
A=(0,1/3) and nearby dyadic Q on u=0 admit a P extending outside R, while all
actual reference/final points and the derivative endpoint hull stay in R.

## 8. Analytic pole/apex: source cap, shared cut and actual facets

Initial finite admission is a declared analytic collapsed pole/apex edge with
one contiguous angular range and its two adjacent **constant-u meridian**
selected boundary segments. General polar trims, an unknown NURBS singularity,
or an unproved quotient are unsupported/unresolved. The same source seam may
give the two meridians one physical identity but separate UV occurrences.

1. Use existing master dyadic samples on both near-pole original physical edges:
   A=(uL,vA), B=(uR,vB). They may have different heights. Do not solve for a new
   nonowned boundary point. Let `g(u)=vA+(vB-vA)*(u-uL)/(uR-uL)`; require both
   heights on one regular side of the analytic pole. The UV cap between the
   pole line and g is convex. Prove it contains no other trim using indexed
   exact selected-domain tests and authoritative strip/star enclosures. If
   necessary request additional master splits and select closer existing nodes.
2. Face owner chooses a power-of-two angular subdivision count and dyadic
   fractions theta_j. Create the cut chain by
   `u_j=uL+theta_j*(uR-uL)`, `v_j=vA+theta_j*(vB-vA)`; do not evaluate g at an
   arbitrary dyadic u, which need not produce a dyadic v. Its endpoints reference those
   existing physical edge nodes; interior nodes have face-generated ancestry and
   common host XYZ. Both the ordinary outside domain/CDT and the pole fan consume
   **this identical chain**, with directed opposite incidence. A full seam may
   have equal physical endpoint keys; use angular pieces with delta<=pi/2 and
   interior cut nodes so every actual facet has nonzero area.
3. For one cut subinterval define normalized dyadic cell coordinates xi,rho:

       u=u_j+xi*(u_j+1-u_j)
       v=v_pole+rho*(g(u)-v_pole)
       Phi=(1-rho)*Qpole+rho*((1-xi)*Qj+xi*Qj+1).

   This covers the complete source cap, including its collapsed rho=0 edge,
   while Phi covers one actual nonzero triangle `(Qpole,Qj,Qj+1)`. It is a
   quotient coverage cell, not a degenerate UV triangle deleted after meshing.
   At rho>0 its map has the stated ordinary source meaning; at rho=0 the entire
   angular edge is the explicit single physical pole identity.
   Its role order is not automatically output orientation: with
   `sJ=sign(delta_u*(g-v_pole))`, the role triangle normal is opposite to
   Phi_xi cross Phi_rho. Keep role order iff `-face.orientation*sJ=+1`, otherwise
   swap the cut roles in the facet. Retain the role-to-facet permutation so the
   source-domain interpolant remains unchanged by an output winding reversal.
4. Sphere `v_pole=+/-pi/2` and cone `v_pole=-r/sin(a)` are analytic constant
   descriptors, generally not dyadics. Predicates operate on xi,rho and chosen
   dyadic cut sites; these constants never enter exact CDT as fake rational
   roots. Require sin(a) nonzero and a consistent cone sheet. Put F=S composed
   with this fixed bilinear Psi. Its exact corner bilinear interpolant has two
   equal ideal-pole corners. Thus a constructive whole-rectangle bound is
   `(sup||F_xixi||+sup||F_rhorho||)/8` plus the maximum of the four corner
   departures from actual Phi. Here
   `F_xixi=Suu*du²+2*Suv*du*rho*dg+Svv*(rho*dg)²` and
   `F_rhorho=Svv*(g-v_pole)²`, with du/dg the angular/cut increments.
   Mixed source derivatives are included in F_xixi. Successive univariate
   interpolation gives this bilinear bound; no triangle Taylor theorem is
   misapplied to the rectangle. Evaluate it using centered analytic pole
   formulas and enclosures. Actual Qpole/Qj departures enter once; tensor
   restriction/subdivision of this fixed formula tightens the bound without
   an arbitrary analytic composition engine.
5. A finite source UV endpoint near pi/2 is not exactly the analytic pole merely
   because a tolerance says so. Retain its original boundary and bound the
   entire finite-to-ideal pole band using original analytic position/derivatives.
   Charge that source-domain realization displacement explicitly. If the
   source does not support the declared collapse within requested bounds, return
   failure. Authoritative noncollapsed tiny edges cannot be erased as poles.

Legal child recipes are part of the geometry return value. Angular bisection
bisects theta, adds a face-owned cut node and replaces one fan with two; ordinary outside
connectivity consumes the same updated cut chain and generation. Radial
refinement chooses a **new dyadic cut line** through closer existing master
samples on the two meridians. Request their owner midpoint splits until both
parameter distances `|vA-v_pole|,|vB-v_pole|` are enclosed at <=3/4 of their old
parameter distances, on the same side with a strict positive gap between cuts;
physical XYZ distance alone does not establish this. Return precision/resource
failure if this cannot be established. Interpolate the new cut at the existing dyadic
angular subdivision fractions. This makes an inner pole fan plus the exact UV
annular quad between two linear cuts, the latter split into two ordinary
triangles with dyadic corners. It is deliberately not the formula rho=1/2 in
original UV: an analytic v_pole would make those sites nondyadic. Side nodes
always come from original edge owners; interior-ray nodes are face-owned. Both
cut chains and all incident annular/fan facets share the same node keys.
New cap widths contract by the proved parameter endpoint inequalities, hence
along the entire linear cut; angular bisection contracts angular width. Source
derivative enclosures supply local bound contraction on the admitted regular
side. A fixed source-to-ideal pole discrepancy may impose a floor: eventual
contraction/success needs the stated strict feasible margin, otherwise return
bounded unresolved rather than promise arbitrarily small caps.
A cut change invalidates the neighboring ordinary face generation; unrelated
faces retain their source caches. No ordinary-UV LEPP/flip primitive edits a
quotient fan.

## 9. Header-level seams and result fields

Geometry headers may be split into `geometry.hpp`, `rational_geometry.hpp`,
`analytic_geometry.hpp` and `correspondence.hpp`; these names do not introduce
separate source models. Public operations have explicit lifetime-borrowed source
and immutable geometry views plus an owner JobContext/cache:

```cpp
Result<GeometryStore> compile_geometry(SourceView, RunResource&, Limits);
Result<WorkingChart> choose_chart(SourceView, GeometryView, FaceRequest,
                                 OwnerGeometryCache&, JobContext&);
Result<JointChoice> choose_joint(SourceView, GeometryView, JointRequest,
                                OwnerGeometryCache&, JobContext&);
Result<SiteChoice> locate_site(SourceView, GeometryView, SiteRequest,
                              OwnerGeometryCache&, JobContext&);
Result<TraceBound> bound_trace(SourceView, GeometryView, TraceRequest,
                              OwnerGeometryCache&, JobContext&);
Result<DomainSupports> build_domain_supports(SourceView, GeometryView,
    AuthoritativeDomainRequest, OwnerGeometryCache&, JobContext&);
Result<CellBoundPlan> prepare_cell_bound(SourceView, GeometryView, ActualCell,
    Allowance, OwnerGeometryCache&, JobContext&);
Result<QuotientPatch> build_quotient(SourceView, GeometryView, PoleRequest,
                                   OwnerGeometryCache&, JobContext&);
Result<QuotientChildren> split_quotient(SourceView, GeometryView,
    QuotientSplitRequest, OwnerGeometryCache&, JobContext&);
```

`CellBoundPlan` is a finite set of runtime rational residual tasks or a named
host analytic comparison with canonical bound constants, plus the domain term.
It never contains a closure/pointer to job scratch. `TraceBound` distinguishes
accepted, split-owner-request, precision/refinement need, and typed failure;
it carries original interval, chosen correspondence, endpoint/whole-interval
discrepancy and generation. Physical XYZ words use the common host policy across
CPU/P100; an enclosure supplies no independently selected node. Approximate
metric jets return separately from proof-bearing bounds.

NativeMesh needs finite tags `UVTriangle`, `PlaneXYTriangle`,
`AnalyticPoleFanQuad`; chart tags `OriginalUV`, `AnalyticUnwrappedUV`,
`PhysicalPlaneConvexQ`; correspondence tags `AffineParameter`,
`GeometricLineScalar`, `CylinderArcOrder`. Plane descriptor retains original
rectangle/projection/Q/inverse meaning; line retains scalar covector and original
p(a),p(b); arc retains dual frame/branch/ray/half-plane data. Pole retains source
collapsed interval, analytic constant kind, cut chain, meridian master nodes,
normalized domain, source map and actual facet association. Authoritative
supports retain their source intervals, nodal-map/strip descriptors and composed
local displacement meaning. Output UV locators/enclosures remain distinguishable
from exact chosen sites and implicit inverse descriptors. Runtime's exact table
owns all cross-owner values; all source/boundary/cell generations are explicit.

Store the **actual selected authoritative domain map** in NativeMesh, rather
than only its bound component. Runtime's `DOMMAPS_` names the ordered nodal-fan B
stage then graph-strip T stage; `SUPPORTS` contains the finite support variants.
A nodal fan stores its original source-point descriptor, chosen Q site and
rational convex P vertices. A graph strip stores the original use/interval,
referenced pre-B maps, Q endpoint sites, rational coordinate origin/covectors,
X,c,w0; original source plus B defines f, its endpoints define chord g, and
w=min(c*x,c*(X-x),w0) defines the two fiber branches. The source-point parameter
is the original **mapped pcurve parameter**, distinct from original edge t.
The face references this domain map with finite policy `AuthoritativeOriginal`
or `HybridPullback`; all-reconstructed identity uses reference0. The original
per-use authority is preserved. HybridPullback defines reconstructed references
by the explicit inverse B above, then uses the same stored B-then-T map; no extra
inverse-map bytecode, private closure or synthetic pcurve is serialized.
Retain original/polygon direction and the inverse-map meaning needed to compare
original points with selected cells. Proof subdivision, interval filter results
and accepted booleans are not serialized. These are selected realization/map
values, not an extra certification artifact.

## 10. Independent RED/oracles and delivery sequence

After whole-plan review, implement in dependency order: shared exact/source
headers; original extraction/queries; actual residual construction and CPU/P100
consumer; analytic/correspondence/chart/support/quotient profiles; then integrated
shared-feature/face construction. Do not report a helper as complete meshing.
Each meaningful RED, implementation/GREEN and review-fix is a real branch commit.
All execution is fleet-only in the new isolated mesher checkout, with bounded
resources. No old watch process or source file is repurposed.

Predetermine independent actual-answer families before product code:

* Extraction: hand-authored clamped and unclamped degree0..3 curves/surfaces,
  unequal positive weights, C0 multispans and discontinuous rejects. A simple
  original Cox/de Boor Fraction evaluator at rational points is independent
  from the Bernstein extraction route. Include wrong rounded saved nets and two
  faces sharing one carrier with different envelope hints.
* Residual: literal affine, bilinear saddle, rational quadratic/cubic and
  large-origin/small-residual patches. Derive exact triangular coefficients by
  independent monomial substitution for bounded degrees. Check actual L, knot
  clipping coverage and squared acceptance/equality; hand wrong-sign, omitted
  mixed coefficient and wrong actual-node cases must fail. CPU/P100 compare
  decisions, not approximate coefficient closeness alone.
* Geometry image: nonlinear straight rational curves with exact zero geometric
  error and nonzero same-t interpolation error; monotone stationary points,
  reversed edges, finite endpoint mismatch, nonmonotone retracing rejects;
  exact rational circle arcs and near-circle failures. UV-linear and spatial
  parameterizations deliberately differ. Use degree32 pcurves plus span crosses
  to exercise host composition, not only ordinary GPU degrees.
  A plane carrying a circular trim distinguishes node recovery from whole
  polyline accuracy: with positional0.005 and node cap0.001, exact recovered
  boundary nodes have zero node residual while a permitted chord/trace sagitta
  may lie strictly between0.001 and0.005. It must not trigger refinement solely
  to satisfy the node cap. Offset recovery UV and source endpoint V/curve
  departures must still fail the node cap even when whole positional bounds pass.
  Use radius R=1/256 and the exact rational quarter circle
  `C(t)=R*((1-t²),2*t,0)/(1+t²)`, t in[0,1], so both sampled endpoints/recovery
  UVs are exact dyadics. Sagitta `R*(1-1/sqrt(2))` is above0.001, while the
  original homogeneous actual-chord residual coefficient bound is at most
  `2*R/3<0.005`. This gives a hand-derived acceptance case rather than a
  tolerance test against an unproved sampled maximum.
* Charts/domains: nonlinear planar square with two-triangle target, interior
  height bubble, folded projection rejection, source guide offset, mismatched
  authoritative endpoints, mixed authority at displaced A/Q joints, small holes,
  displaced nondyadic rational arc nodes,
  smooth opposite-tangent join and unresolved same-direction cusp. Compare
  actual domain/physical distances on analytic families with known extrema;
  sample grids alone never assert whole-domain coverage.
  The mixed literal fixture uses a plane with an authoritative edge at rational
  x=1/3 (homogeneous controls x=1,w=3), nearby chosen dyadic Q endpoints and
  reconstructed remaining edges. Independent Fraction inverse-fan evaluation
  must recover the closed reference at1/3, exact B recovery of the reconstructed
  chosen polygon, preserved use authority and nonzero charged joint displacement.
  Two disagreeing authoritative endpoints must fail the original closure policy.
* Quotients: sphere north/south poles, cone signed slope/apex, unequal meridian
  sample heights, full seam with repeated physical endpoint, finite source pole
  band, cut-chain adjacency and radial/angular child coverage. Analytic normalized
  maps give independently derived source cells/fan images. Require actual
  nonzero facets and complete quotient incidence; deleting a collapsed source
  region must fail inventory/coverage checks.
* Whole outputs: preserve existing independent domain/CDT/physical embedding
  readers and upstream original source fixtures beyond the watch. Compare
  complete source inventories, directed shared boundaries, local bounds and
  actual contacts; NVIDIA/OCCT are differential references with explicitly
  different quality semantics, not universal whole-cell truth.

Performance families vary source span count, degree, knots, parameter speed,
weight condition, origin scale, thin gaps, hole/edge counts and incidence. Record
extraction once, span-query visits, composition degree/work, bound tasks,
fallbacks, repeated evaluations, generated sites and settled whole-operation
time. A planar nonlinear parameterization must not cause tolerance-driven
straight-edge/plane explosion; a local query must not scan all spans. Retain
censored/resource outcomes. Brutal performance testing attacks those mechanisms
and complete consumers, not an arbitrary microsecond target or watch geometry
dispatch. Fault-specific mutation work follows actual uncovered algebra/sign/
ownership faults; it does not substitute for an independent oracle.

Constructor bounds, incidence, coverage maps and actual-contact handling are
the single production mechanism. Independent Fraction/Python actual-output
readers are development research off the hot path. This plan adds no second
native certification/receipt stage.

## Basis and explicit boundaries

The original-source residual/positive Bernstein hull is the selected local
method; the analytic interpolation remainder derives from the derivative-bound
line of [Filip et al.1986](../papers/2026-10-16-whole-mesher-geometry-contract-study.md).
Trim inconsistency and separate parameter/physical meanings follow the acquired
Marussig/Hughes study. Local restriction, rational positivity and domain strips
replace the old global second-derivative density and fixed tolerance split.
The [geometry research and bibliography](../papers/whole-mesher-geometry-contract.bib)
state the primary-source scope and the assumptions not supplied by references.
The procedures above are owned implementation choices, not claims that those
papers prove this particular mesher.

Unresolved authoritative tangent sectors/gates, unavailable parameter maps,
NURBS singular/periodic charts without metadata, out-of-domain NURBS queries,
failed inverse coverage and excessive exact/trig work are named failures. They
cannot become silent geometry omission, a tolerance increase, a Python fallback
or a universal embedding/Hausdorff assertion from finite samples.
