# Adaptive cached geometry and source error

Status: concrete geometry proposal for the operator-selected adaptive architecture;
not product code. Original-source and positional contracts remain, while
correctly-nearest transcendental output is deliberately replaced by bounded,
deterministic physical point construction. No dependency adoption is proposed.

## Actual reference reading and choices

Read pinned NVIDIA USD/BRep revision
`c9c5979f04cea8c38213004d4f04df0dcfb40d05`, specifically:

- `source/SMLib/src/SmCurveCache.cpp:1210–1320`: contiguous span/control allocation;
  original rational spans versus approximated other curves. `:1322–1410` splits
  approximate cubics by consulting original midpoint position/derivative, rather
  than inheriting an unchanged approximation remainder indefinitely.
- `SmCurveCache.cpp:1700–1854`: accepted leaf properties, original interval,
  continuity and ordered output; exact rational subdivision versus reconstruction
  from original curves. Minimum-parameter/zero-size acceptance can override
  failed tests; we must return bounded failure instead of importing that rule.
- `SmSurfaceCache.cpp:3500–3655`: cached pseudobox/control-net directional checks.
  Non-B-spline branches may sample original geometry, so these acceptance values
  are not automatically hard whole-cell bounds.
- `SmSurfaceCache.cpp:3706–3950,4030–4140`: iso-curve/block shortcuts, diagonal
  warning and minimum-size/partition-culling decisions. Harvest localization and
  reuse, not those stopping conditions.
- `SmTess.cpp:13215–13395`: reuses an existing tree, exact B-spline subdivision,
  approximate cubic starter blocks for other types, then implants trims.
  `:12023–12125` emits inside quads and triangulates remaining polygons. This is
  the architectural reference, not an instruction to approximate every analytic
  carrier by a cubic or rerun the original expensive source checks afterward.

Primary mathematical material reread: Filip–Magedson–Markot1986, Theorems2/4 and
Section3, original acquired text/PDF; Balázs–Guthe–Klein2004, Sections3–8, original
acquired text/PDF. Filip supplies vector second-derivative interpolation bounds
and explicitly warns that independently shifted planar approximants can crack.
Balázs separates approximate rendering mode from guaranteed error, constructs
trim composition across source Bézier spans, and exposes composition degree cost.
Neither sampled checks nor an approximant's flatness replaces original-source
error. The formulas below are our explicit construction, not copied code.

## One leaf map and one actual-facet inequality

A regular source leaf has an original-UV rectangle R and FOUR actual stored
corner points Q00,Q10,Q01,Q11. B is their bilinear interpolant. Cache e_R bounding
`sup_R ||S-B||` against ORIGINAL S. For every emitted actual affine triangle L,
partition its domain by overlapping original span/cache leaf rectangles and require

    e_R + upper_piece ||B-L|| + E_domain(piece) <= epsilon_definition.

L remains the SAME emitted XYZ map on every virtual partition. A virtual clip
vertex carries its exact original barycentric weights; its L is the affine
combination of the original THREE emitted points, not a new evaluation of S.
B-L is at most quadratic on a triangular overlap piece. Its degree2 Bernstein
coefficients come directly from the bilinear coefficients and L; convex hull
of coefficient norm bounds gives a tiny cheap CPU/P100 test. A usual two-triangle
rectangle can instead use `||Q00-Q10-Q01+Q11||/4` for this twist term. Hanging-side
and boundary-region triangles use their actual piece residual, not that shortcut.

A conservative e_R must not be mistaken for actual geometric failure. On an
undecided/failing cheap bound, tighten the original bound on the SAME facet first.
If numerical/source-cache looseness dominates, increase the numerical tier or
restrict the bound representation. If a real facet needs improvement, install
new construction sites/rebuild its region; merely splitting a bound cache cannot
change an unchanged L. Source/domain limitation requests the actual shared edge
or finite source feature, not arbitrary private boundary points.

## Packed geometry records

Use existing immutable SourceStore as original truth. Do not call full old
compile_geometry/FaceBoundaryPreparation before this path. Select live carrier,
spatial-curve and actually queried pcurve roots from the requested source closure.
Direct analytic records need no rational extraction. Positive rational original
knot spans compile lazily into interval Bernstein roots; exact original extraction
is retained as lazy fallback, not performed eagerly for every unused guide.

Proposed finite records, with integer slots/flat ranges rather than per-leaf
polymorphic objects:

    SourceGeometryKey(source_generation,kind,original_source_slot,original_span)
    RootRecord(key,original_parameter_box,map_kind,degree,control_range,
               analytic_record,arithmetic_profile,root_revision)
    NodeRecord(root,split_path,parent,children,original_box,control_range,
               corner_sample_keys[4],bilinear_coeffs,e_upper,error_parts,
               curvature_ranges,chart_aabb,source_aabb,revision)
    PointSample(key,actual_xyz,source_position_enclosure,departure_upper,policy)
    CurveLeaf(root,path,original_t_interval,endpoint_sample_keys,
              actual_chord,error_upper,correspondence_ref,revision)

Exact original boxes/parameters remain represented by immutable source references
and exact dyadic fractions of those intervals. Floating boxes are search aids.
Original knot endpoints can be represented exactly even if a future source format
permits nondyadic values. The existing dyadic-only topology adapter must then be
explicitly upgraded or the construction sites must remain checked dyadics; a
rounded root coordinate is not exact source identity. Virtual clipping pieces
may be rational without becoming physical sites.

A source carrier can serve several faces: root truth never uses the first face's
uv_bounds as its natural rectangle. Per-face chosen envelopes/charts and trim
queries reference the same original roots. Immutable roots and point values may
be retained across queries; adaptive nodes belong to their explicit cache owner.

## Analytic source algebra: no per-leaf rational trigonometry

Write `E(u)=X cos(u)+Y sin(u)` using original, possibly nonorthogonal axes.
Let A be an upper bound for operator norm `[X Y]`, F for `[X Y Z]` and ZN for
`||Z||`. Compile these ONCE from original frame Gram entries using common
outward arithmetic, scaled to avoid intermediate overflow; unresolved constants
use a fixed/wide/exact fallback once per root. Do not assume unit/orthogonal axes.
A cheap rigorous upper is sqrt(max absolute Gram row sum); a tighter fixed-size
spectral enclosure can improve anisotropic cases without changing the formula.

Cache local sin/cos RANGE enclosures from endpoints plus contained extrema.
These use bounded polynomial/range-reduction arithmetic and integer critical-point
classification, not `GeometryContext::trig_range` rational Taylor per leaf.
A critical-point ambiguity uses [-1,1] or a precise local fallback; it never
silently omits an extremum. Global range [-1,1] is sufficient to begin meshing.

Useful directional norm bounds (Muu,Mvv,Muv):

| Original carrier | Bounds |
|---|---|
| plane O+uX+vY | 0,0,0 |
| cylinder O+rE(u)+vZ | abs(r)A,0,0 |
| cone O+(r+v sin(a))E(u)+v cos(a)Z | max_v abs(r+v sin(a))A,0,abs(sin(a))A |
| sphere O+r(cos(v)E(u)+sin(v)Z) | abs(r)maxabs(cos(v))A,abs(r)F,abs(r)maxabs(sin(v))A |
| torus O+(R+r cos(v))E(u)+r sin(v)Z | maxabs(R+r cos(v))A,abs(r)F,abs(r)maxabs(sin(v))A |

Sin/cos of the cone's original semiangle are compiled source constants with
original-value enclosures. Scaled/nonorthogonal source frames remain original.
For a rectangle of widths hu,hv, repeated 1D linear interpolation gives

    e_R <= Muu*hu^2/8 + Mvv*hv^2/8 + max corner_departure.

There is no mixed term here because B is a BILINEAR interpolant. The mixed
term reappears through B-L/twist, especially on cones. A cylinder's v direction
therefore needs no curvature-driven splitting. This is stronger and cheaper
than generic full jets, and does not first approximate the cylinder by a cubic.
For an arbitrary triangle directly inside a C2 rectangle, the alternative safe
bound `(Muu hu²+2Muv hu hv+Mvv hv²)/8 + max vertex_departure` follows from
barycentric weighted variances; the bilinear cache residual is normally tighter.

Root-local sphere/torus bounds can tighten F using local angular enclosures;
only do that when the global constant cannot decide. Split-axis proposals compare
predicted directional error reduction and actual twist, with deterministic ties.
They do not repeatedly recompute derivatives through generic source evaluation.

## Positive rational originals and approximant alternatives

Original nonzero knot spans retain p/q, positive homogeneous W, and exact source
parameter ownership. Subdivide their INTERVAL homogeneous Bernstein nets with
shared explicit-operation de Casteljau. Cache child controls, W lower bound,
centered anchor and ranges. For actual bilinear B, bound `R=N-WB` in its tensor
Bernstein basis and divide by positive Wmin. This includes original coefficient
conversion and every interval restriction roundoff. No additional free
"coefficient tolerance" is introduced. Degree elevation/linear multiplication
uses fixed combinatorics already studied in bound_tasks; p,q<=3 gives a small
(p+1,q+1) residual. Higher curve degree<=32 remains a declared separate bucket.

If interval width dominates, rebuild the affected child from original exact
roots or wider fixed arithmetic and retry the SAME bound. Repeated subdivision
of interval controls alone can accumulate rounding floors. A cache may keep a
floating affine anchor without changing actual stored Q or original truth.
Rational C0 knots partition source bounds; never apply C2 derivative bounds
across them. Shared sides consume one schedule/point owner.

Bounded analytic Bernstein approximants are a contender for difficult local
leaves or shared GPU buckets, not the universal first step. Construct a local
Taylor/Chebyshev polynomial in ORIGINAL u/v with coefficient enclosures and
explicit uniform remainder; then charge
`source_to_approximant + approximant_to_B + B_to_actual_L + domain`.
A restricted approximant retains its old remainder; restriction alone cannot
make a global approximation floor disappear. Recenter/rebuild from original
harmonics or increase polynomial order when that floor dominates. Do not use
a tangent-half-angle rational conic and claim unchanged original angular UV:
that requires an explicit nonlinear parameter correspondence. The direct
analytic constants above avoid this complication and unnecessary tensor degree.

The tight unresolved rational fallback may directly compare original `N-WL` on
the same clipped triangle, bypassing e_R+B-L looseness. Analytic fallback uses
local harmonic range refinement, or a bounded polynomial with retained remainder.
Exact original rational/trig routines remain last-resort tasks with measured
counts/work, not the default startup or repeated per-cell path.

## Bounded deterministic point construction

Select a COMMON CPU/P100 arithmetic profile: explicit RN multiply/add operations
with contraction disabled, the same polynomial coefficients and operation order,
and the same IEEE-word outward-neighbor padding after every enclosure operation.
Do not use current host one-ULP padding but device RD/RU for adaptive decisions:
they can produce different enclosures/leaves. Use the same RN-plus-padding model
on both. Check host nearest mode, FTZ/DAZ/evaluation model; require strict sm60
build. Overflow, unsupported argument reduction or insufficient positivity returns
unresolved to a shared higher tier, never an architecture-specific split.

For sin/cos, select a fixed finite range-reduction profile and owned polynomial
with rigorously retained approximation AND coefficient AND arithmetic error.
The existing owned point-trig Chebyshev work supplies the coefficient/proof
source, but its host bigint canonical-nearest wrapper is not the new per-point
hot path. GPU and CPU execute the same compact polynomial. Wider fixed-point
range reduction/interval evaluation handles ambiguous large phases; original
exact source evaluation is final fallback. Argument range/profile caps are based
on arithmetic representability, not model names or tolerances. This numerical
primitive and coefficient/reduction error bound must be concretely reviewed
before implementation; no unspecified libm sin/cos is permitted.

Q is the deterministic constructed point. Cache an enclosure for TRUE original
Ce(t) or S(u,v) and bound its departure from actual Q. Original vertices remain
their original source XYZ. Every shared edge sample is constructed ONCE by its
edge authority; all faces reuse that identical physical key/XYZ. Root corners,
midpoints and common sides share canonical sample keys independent of visit order.
A cache allocation offset is not a sample identity.

Counting error precisely:

- Source-node cap remains original V/Ce(t) versus original S at the stored chosen
  UV, plus original endpoint V versus original Ce. Q is absent from this test.
- Curve actual chord bound uses original Ce versus the actual chord(Qa,Qb).
  With an analytic second-derivative shortcut it includes max endpoint
  departures; with original rational residual it includes them inherently.
- Leaf e_R uses original S versus actual B(Qcorners), so corner Q departure is
  already paid there. B-L uses actual B and actual L, with no extra rounding
  allowance added again. E_domain is paid once for the named domain realization.
- A boundary Q supplied by Ce rather than S can have a larger carrier departure;
  e_R/actual facet residual measures that actual difference. Do not additionally
  subtract node_source allowance as if it were output error.

## Shared curve caches and source correspondence

Analytic line chord error is max actual endpoint departure. Circle/ellipse
`O+A cos(t)+B sin(t)` has a cached M2 upper `||[A B]||`, so chord error is
`M2 dt²/8 + max endpoint departures` with no interior trig. Tighten with local
sagitta/harmonic bounds only if needed. Positive rational curve spans use cached
`N-W actual_chord` residuals and existing degree buckets. Full closed nonconstant
curves still need enough distinct source-master samples for topology even when
radius<epsilon; never drop/tolerance-weld the loop. Degenerate declared source
edges retain two endpoint occurrences, zero splittable intervals and separate
literal pcurve coverage.

Compile finite source relations BEFORE enclosure. An affine selected original-UV
trace on an analytic carrier yields finite harmonic terms (cone permits affine
amplitudes). Products of sin/cos become sum/difference frequencies. Compare that
compiled expression to original analytic Ce and combine equal frequency/phase
terms before bounding. Matched cylinder circles/generators, sphere meridians,
torus iso-circles and planes then avoid independent wide Ce and S(trace) hulls.
The compiler must check original frames, literal t range, mapped-s pieces,
phase/scale and use branch/lift. No curve_same_sense reinversion on ordinary t.
Exact equal algebraic terms cancel; near-equal rounded source frames leave their
actual bounded residual. This is a general geometric profile, never source-ID
or watch dispatch. Nonlinear monotone line/angle correspondences retain their
named map; same-parameter residual is not substituted for image correspondence.

General rational pcurve/carrier composition restricts to original span pieces
and uses Bernstein composition or an endpoint+whole second-derivative residual
bound with cache jets; its high-degree work stays HOST until explicit device
buckets exist. Degree32 pcurve with bicubic carrier can produce degree192 before
combining an independent degree32 spatial curve. Do not force every guide through
that composition: reconstructed guides are locators; their selected finite trace
is authoritative only through the retained chosen-domain policy.

Recovery uses fast cached positions/derivative enclosures and floating Newton
PROPOSALS, then selects a common exact dyadic joint and checks original nodal cap
with source enclosures. Authoritative original endpoints remain source-point
descriptors for B/T stars, not rounded convenience UV. Existing finite B/T map,
plane inverse and pole quotient semantics remain, but their source point/range
operations dispatch directly through the new cache/numeric profile. Calling the
old full FaceBoundaryPreparation first is explicitly outside this design.

## Singularities, planar geometry and invalidation

Planes have zero interpolation curvature. Geometrically planar rational carriers
can use the existing admitted finite physical-plane convex-Q profile, proved
from original coefficient/derivative cache bounds; do not let nonlinear UV
parameterization force dense planar triangles. Its finite inverse chart locators
remain distinct from actual physical XY truth. It is prepared directly on the
new root/cache path, not by running the old mesher.

An admitted sphere/cone pole leaf is a quotient coverage cell, not two ordinary
UV triangles with a degenerate physical triangle deleted. Retain the symbolic
pole, original finite boundary, actual shared pole node and full xi/rho fan map.
Use the SAME cheap compiled analytic derivatives in normalized coordinates;
charge finite-to-ideal band over the WHOLE cap, as corrected in the previous
unit. Face-owned internal cut points never become private original-edge nodes.
Unsupported general singular NURBS/atlas/tangent stars remain explicit.

Source edits invalidate dependent root generations, coefficients/constants,
point samples, trace compilers and source geometry bounds. A changed arithmetic
profile invalidates constructed Q and every bound depending on those values.
Trim/use generation invalidates chosen traces/domain maps, classifications and
boundary regions; reusable carrier restrictions survive. A tighter epsilon
reselects/refines leaves and edge intervals; source geometry/cache truth survives.
A looser epsilon can select ancestors only after their actual point/map bounds
and current conforming schedules are valid. Assembly allowance changes affect
query selection, not original source compilation. Eviction/reconstruction cannot
change a sample's semantic key or deterministic Q; persisted profile/version
must be enough to recreate it.

## Minimal first real delivery and direct oracles

First delivery must construct actual mesh, not just expose a subdivision class:
original native source patch -> lazy original root/cache -> CPU/P100 point and
adaptive-bound epochs -> conforming positive actual facets -> existing semantic
publisher/display with original mappings. The first untrimmed/rectangular patch
slice should include plane, cylinder/cone, sphere/torus and positive rational
multi-span examples so it exercises BOTH analytic and rational kernels. Compare
cold compile/build/output and warm tighter/looser epsilon reuse; count exact
fallback and point-cache misses. This is the first executable slice of the whole
operation, followed immediately by SAME-path complete original shared boundaries,
curved/hole trims, finite maps, singular cells and complete definitions. It must
not advertise whole imported mesher delivery before those connections exist.

Independent direct oracles: exact polynomial/rational plane/saddle/bubble roots,
analytic cylinder/cone/sphere/torus source formulas with original nonorthogonal
frames; displaced actual Q; original-vs-actual-chord residual; C0 span boundaries;
periodic seams/poles; many holes/tiny closed nonconstant trim; nonlinear planar
parameterization; rotated/localized rational ridge; large translation and small
weights; deliberate cache generation/profile edits. Check actual full-cell
original-source bounds independently on bounded cases, complete source inventory,
shared-node identities and final contact/embedding behavior. CPU/P100 byte parity
is a determinism check, not the original geometry oracle. Legacy exact machinery
is an independent development reference/rare fallback, not a required expensive
second production validation pass.

The review-closure sections below select the shared polynomial/range-reduction
profile, deterministic error comparison direction, lazy extraction and finite
trace compiler procedures. Typed unit APIs and direct RED are next before product
code. These choices are not a claim that tested product implementations already
exist.

## Review closure: finite RN profile and lazy compilation procedure

The runtime report's `rn64_anchor32_p128_v1` is now the selected common first
point/range profile. Scoped numerical review ACCEPTS its checked reduction:
P128-derived outward binary64 pi/2, anchors and sine15/cosine16 coefficients;
ties-even k locator with abs(k)<=2^30; ENTIRE reduced interval inside [-1,1];
ties-even j/32 anchor with ENTIRE delta inside [-1/32,1/32]; parity Horner with
retained uniform remainder; fixed anchor/quadrant reconstruction; identical RN
operation order/padding; midpoint Q and endpoint-based original departure.
Any integer k is valid after that inclusion check; no nearest-quadrant theorem
is assumed. Failed checks select fixed128/192/256 then original precise fallback.
Original coefficient/source intervals are never replaced by exact rounded literals.

Axis selection is a performance heuristic. It does NOT need exact trigonometric
work to establish the truly best axis. Compare common canonical child PACKET
upper scores `e_child + twist_child/4` (and known side-node corrections), not e
alone: a pure bilinear saddle has e=0 with nonzero facet twist. Use deterministic
U ties/interval overlap and the construction owner's finite stagnation/fairness
rule. Escalate numerical/source acceptance and point-floor uncertainty, not the
optimization of an axis heuristic. Geometry review requested this correction.

PointPool promotion is an authority transaction. A face may request promotion
when a numerical floor blocks epsilon, but cannot privately overwrite Q. Sort
and apply the complete promoted set, increment profile generations, invalidate
all Q-dependent leaf/chord/map bounds and incident face results, then restart
selection from roots. Old leases remain valid only for old results. Q is not
chosen from visit order or whichever epsilon happened to arrive first.

Lazy source compilation is constructive:

1. Build a metadata catalog/index directly from retained SourceStore: source IDs,
   family, original natural nonzero span intervals, control/weight ranges and
   source generation. Original dimensional strides remain dimension+1. Analytic
   rows compile their finite frame/radius/harmonic constants without exact
   Bernstein extraction. This replaces eager global compile_geometry startup.
2. On a demand for original span i, initialize its degree0 Cox basis as1 and the
   other locally supported degree0 basis functions as0. Apply the same finite
   Cox recurrence used by the existing exact extractor, expressed in local span
   Bernstein coefficients. Knot-equality/zero-denominator branches are decided
   from ORIGINAL source values, not rounded interval near-zero tests. Only the
   p+1 local support functions are needed. Tensor surfaces apply it in U/V.
3. Perform those coefficient operations first in common outward RN intervals;
   combine original homogeneous controls with those basis coefficient intervals.
   This encloses exact original source extraction, including conversion and all
   arithmetic. Positive original weights remain mandatory. Cache the source
   span's controls and min-weight information once, then restrict children by
   common de Casteljau. Do not independently normalize an interval net to force
   partition of unity or force control positivity by clamping.
4. If weight/width/residual acceptance is undecided, reconstruct only that SAME
   demanded source span from original coefficients with wider fixed arithmetic
   or the existing exact recurrence. Seal a qualified per-root exact lease for
   reuse. No fallback first compiles every pcurve/carrier in the assembly.
5. Root-demand and point keys are semantic original IDs/spans, while batched
   canonical allocation selects flat pool slots. Existing GeometryStore,
   BoundRootBank and publisher interfaces need a SourceStore+lazy-root lease
   adapter: obtaining their old structs must not secretly trigger eager exact
   extraction. This is part of checkpoint1, not a later startup optimization.

Cheap source bounds for initial catalog/prepared envelopes also avoid trig.
For each coordinate, ellipses use sqrt(A_axis²+B_axis²); a whole sphere uses
abs(r)*sqrt(X_axis²+Y_axis²+Z_axis²); a torus uses
abs(R)*sqrt(X_axis²+Y_axis²)+abs(r)*sqrt(X_axis²+Y_axis²+Z_axis²). Plane/cylinder/
cone include their original finite axial interval linearly. Positive rational
originals use the convex hull of projected original homogeneous controls.
These are conservative original bounds, not sampled fits. Local angular bounds
can tighten them after demand. Source/UV envelope expansion retains the existing
transaction that recomputes affected original B0, placement error and allowances
before accepting a new generation; it never expands a NURBS natural rectangle.

## Review closure: finite trace compiler procedure

Initial analytic cancellation profile admits original line/circle/ellipse Ce,
analytic carriers plane/cylinder/cone/sphere/torus, and a selected affine UV trace
on one literal original t-map interval. Compile u=a*t+b and v=c*t+d from exact
selected endpoints and exact original t endpoints ONCE. Product-to-sum expands
sphere/torus harmonic products to frequencies a, c and a±c. Cones permit affine
amplitudes from v. Group equal exact rational frequencies and syntactically equal
phase/source factors BEFORE enclosing coefficients. Absorb other phase offsets
using one cached certified sin/cos pair per constant, not per leaf. Near-equal
rounded original frames leave their real bounded residual; they do not cancel
by tolerance. Original ordinary t already carries source curve direction and is
never inverted again through curve_same_sense.

The residual has the finite form

    P0+t*P1 + sum_k ((A_k+t*B_k)cos(w_k*t+phi_k)
                    +(C_k+t*D_k)sin(w_k*t+phi_k)).

On an interval centered at m with halfwidth h, rebase amplitudes to m. Its
second derivative norm is bounded by the sum of

    w_k²*(||A_k+mB_k||+h||B_k||+||C_k+mD_k||+h||D_k||)
      +2abs(w_k)*(||B_k||+||D_k||).

Endpoint residual enclosures plus this M_R2 times full_dt²/8 bound the WHOLE
Ce-versus-S(selected trace) interval. The affine polynomial part contributes no
second derivative. Local harmonic sin/cos ranges may tighten the direct bound;
restriction/rebase operations and constant coefficients are cached. No two
independent wide source-position hulls are subtracted as the main method.

For a reconstructed affine UV trace on a positive bicubic original, clip its
parameter interval at source span lines, preserving original t adapters. Compose
that linear UV map with the degree(p,q) homogeneous source net, degree<=p+q,
and compare the original spatial-curve span through the common residual basis.
Virtual source-span crossing parameters restrict the bound only; they do not
create private source-edge nodes. Interval Bernstein convolution first, precise
same-expression fallback only when unresolved. An authoritative degree32 pcurve
can require degree192 carrier composition and degree224 combined residual;
retain an explicit bounded host task rather than pretending it fits ordinary
P100 degree buckets. These high-degree tasks are not run for reconstructed guides.

Line IMAGE correspondence is a separate finite profile: prove a common line by
original coefficient residuals, transverse bounds and monotone scalar projections
with covered endpoints for BOTH original curves. Normalize each original scalar
by its OWN endpoints. Then a named monotone map aligns their images without
per-point inverse roots. Same-parameter residual must not defeat a nonlinear
parameterization of an exactly straight geometric line. Angular image profiles
likewise retain source endpoint-ray departure and branch/lift; they are not
silently obtained by rounding an atan value to an exact angle. Unrecognized
relations use the general cached original composition/range path under limits,
not an unchecked cancellation or old full preparation call.
