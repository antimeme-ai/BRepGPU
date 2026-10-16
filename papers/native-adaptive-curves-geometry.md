# Cached original curves and selected trace geometry

Status: selected finite numerical/source contract for checkpoint2 of
ADAPTIVE_MESHER_PLAN.md. Research and interface proposal only; no product changes
or geometry execution in this study. The root implementation unit still needs
review of these concrete procedures and interfaces before RED and code. No
library adoption is proposed. The existing direct-refinement geometry is a
mathematical/reference control, not a preparation prerequisite.

## Source reading and decisions

Pinned NVIDIA USD/BRep is commit
`c9c5979f04cea8c38213004d4f04df0dcfb40d05`, restored by QUARANTINE.md.
The following actual source was read in the retained tree:

| File/function | Reading and consequence |
|---|---|
| `source/SMLib/src/SmCurveCache.cpp`, `sm_CreateBezierSegmentBlock`, lines1210–1316 | Allocates span records/controls in contiguous blocks; true B-spline spans retain their degree while other types start as approximate cubics. Select packed original-root storage, without approximating our analytic originals by uncharged cubics. |
| Same, `sm_SplitSegment`,1322–1410 | Nonrational cubic children consult original midpoint position and derivative. A parent approximant's nominal flatness alone does not establish original-source error. Our analytic descriptors avoid the approximant entirely. |
| Same, `sm_ComputeConstants`,858–1142 | Checks control polygon distance to a baseline/turning, and tests approximant-to-original extrema by20 steps and local solving. This is a useful fast rendering implementation, not a universal whole-interval source-error oracle. Do not import sampled extrema as our acceptance mechanism. |
| Same, `SmCurveCache::Tessellate`,1454–1860, especially1738–1794 | Ordered original span subdivision and retained trees are useful. Null-curve skipping and forced acceptance below minimum parameter size are rejected choices: a tiny closed hole can require more topological sites even below positional allowance. |
| `source/SMLib/src/SmTess.cpp`, `SmCurveTessDriver::TessellateCurve`,1269–1430 | Chooses bisection, curve cache or evaluator-based schemes. Its own comment warns evaluator checks can miss spikes. This does not establish which internal branch the separately distributed reference binary used for every input. |

Read file SHA256: SmCurveCache.cpp
`7203a983fb6482729af95246842b409551ee99198b29c8630cbc1eb1cfe59271`;
SmTess.cpp `f2832e236559753b8755365a06c0036f71b404bd7b7c784dd086144f6de7fa59`.
Public source: [pinned SMLib](https://github.com/NVIDIA-Omniverse/usd-brep/tree/c9c5979f04cea8c38213004d4f04df0dcfb40d05/source/SMLib).

Owned actual sources read: `source.cpp` controls/knots/admission;
`source_cache.cpp` original/prepared root separation and point ownership;
`geometry.cpp` exact original extraction; `geometry_analytic.cpp` original curve
formulas; `geometry_jets.cpp` analytic chord bounds;
`geometry_correspondence.cpp` coupled rational residual/line-image map;
`geometry_boundary.cpp` original parameter mapping/endpoint/node guards;
`boundary.cpp` sole source-edge owner and all-use horizon. Their source contracts
are reusable; eager GeometryStore extraction, per-point exact trig, full jet
reconstruction and per-use face-preparation machinery are not selected here.

Primary literature actually read for this proposal:

* Filip, Magedson, Markot, *Surface algorithms using bounds on derivatives*,
  CAGD3(1986),295–311, §2.1 Theorem2 and its proof, §3 and derivative discussion.
  It supplies the vector interpolation bound used below. Acquired original PDF
  remains `papers/whole-mesher-2026-10-16/filip-magedson-markot-1986.pdf`.
  [Original article](https://doi.org/10.1016/0167-8396(86)90005-1).
* Floater, *Derivatives of rational Bézier curves*, CAGD9(1992),161–174;
  author manuscript revised November1991, §§1–3, equations7–19 and hodograph
  argument. Positive homogeneous weights give useful tangent-direction bounds;
  differentiating homogeneous coordinates and projecting is not itself the
  rational derivative. [Author PDF](https://www.mn.uio.no/math/english/people/aca/michaelf/papers/bez.pdf).
* Karavelas, Kaklis, Kostas, *Bounding the distance between2D parametric Bézier
  curves and their control polygon*, Computing72(2004),117–128; acquired author
  text, §§1–2 including Theorems2.1–2.4, and §3 orientation procedure. These give
  sharper polynomial/control-polygon enclosures; that polygon uses Greville
  parameterization and is not automatically our physical endpoint chord.
  [Author PDF](https://users.math.uoc.gr/~mkaravel/files/papers/bounds.pdf).
* Bez and Bez, *New minimal bounds for the derivatives of rational Bézier paths
  and rational rectangular Bézier surfaces*, AMC225(2013),475–479; acquired
  author text, introduction, Theorems1.1–1.2 and Lemma2.1 proof. Möbius
  reparameterization can improve derivative bounds, but changes the parameter
  relationship we must retain. It is an explicit alternative, not free original-t
  preservation. [Institutional author PDF](https://pure-oai.bham.ac.uk/ws/portalfiles/portal/12291200/MinimalBounds_revisedsubmitted.pdf).
* Balázs/Guthe/Klein2004 §§3–8 and Marussig/Hughes2017 §§3.2–3.4,5.2 were
  reread from the previously acquired originals: trim composition, original
  competing spatial/parametric representations, and source gaps matter. They
  do not turn shared physical XYZ into exact incidence on every carrier.

New intact PDFs, hashes and restoration URLs are in
`papers/native-adaptive-curves/acquisition.tsv`; bibliography is alongside it.
Nairn/Peters/Lutterkort1999 is a useful control-polygon lead, but the Purdue
download returned403. Do not describe its full text as read. Its accessible
descendant above supplies the limited polynomial-enclosure comparison used here.

## The quantities must stay different

An original edge names a spatial curve Ce(t), literal endpoint order/range and
original vertices V0,V1. Each original use independently names a2D pcurve P(s),
its original piecewise map s=m(t), traversal, analytic period lift/branch and
authoritative/reconstructed disposition. A single physical owner provides the
same stored Q(t) and key to every incident use. Source vertices retain their
original binary64 XYZ (declared quotient representatives keep their explicit
separate alias/source obligations). Equal coordinates never merge IDs.

Keep four separate relationships:

1. `NodeEndpointCurve`: original V versus original Ce at an endpoint.
2. `NodeCarrier`: original V at endpoint, or original Ce(t) inside, versus
   original S at the stored chosen exact UV. Both nodal relationships use the
   requested original-node cap, not a mesh construction error allowance.
3. `ChordPosition`: original Ce image/parameter map versus the SAME actual chord
   through Q0,Q1. Numerical construction Q−Ce is part of this positional bound,
   exactly once. It cannot be spent from the nodal cap.
4. `TracePosition`: original carrier on the selected affine UV trace versus that
   SAME actual chord, with an explicit same-parameter or image correspondence.
   Authoritative original pcurve flattening/domain displacement is retained
   separately. No whole Ce−S(selected-polyline) test uses the nodal cap: that
   would mistake ordinary circular sagitta for source-node recovery error.

These are definition-local quantities. Assembly transport uses the established
original-transform bounds in the later whole caller. Source tolerances and
source.units_mm are retained metadata; existing coordinates are already mm.
This checkpoint produces complete edge/use schedules and selected interval
traces. It does not assert a complete face-domain homeomorphism or finished
trimmed-face mesh merely because its boundaries have positional bounds.

## Original one-dimensional roots and polynomial-time extraction

Select dedicated typed CurveSourceCache, never a surface RootDescriptor with a
fake zero-width axis. Source reader admits dimensions2/3, analytic line/circle/
ellipse kinds1–3 and positive rational original degree0..32 (source.cpp around
491–508). This is an existing finite source limit, not a curve-degree estimate
from one model. Original homogeneous stride is dimension+1:3 for UV and4 for
spatial curves. The maximum p32 spatial slab has132 scalar intervals and a
degree33 chord residual has34 vector coefficients. GPU warp loops therefore
use lane+32, not a single-lane-per-coefficient assumption.

Compile only requested original spans from original knots/multiplicities and
controls. Saved adapter span arrays are hints, never truth. Nonclamped natural
domain is expanded knot U[p]..U[n+1]. Retain source endpoint evaluation-only
neighbor roots and select the lexicographically lower closed original span.
At full-multiplicity knots compare exact one-sided endpoint limits before
sharing t identity; a true jump is typed unsupported, never an XYZ weld.

The degree<=3 surface helper's recursive basis expansion must not be extended
naively to32. Select a local Cox-to-Bernstein dynamic program: on one span
[a,b], degree0 has only N_span,0=1. At level k compute the k+1 active functions
from the previous level with the original Cox recurrence. Its linear factors
are represented by their values at a,b; multiplication of a degree(k−1)
Bernstein net by an affine factor uses the two neighboring coefficients and
weights (k−j)/k and j/k. O(k²) work per level yields O(p³) work and O(p²)
scratch per original span, followed by the original homogeneous-control sum.
Repeated knots use the exact zero-denominator omission, not a tiny denominator.
Interval operations account for original coefficient/extraction rounding.
If finite positive W cannot be established, rebuild only that original span
with the existing owned exact extractor and freeze outward intervals.

Original evaluator roots and prepared query-restricted roots remain distinct.
Initial clipping is a counted one-time root restriction; later midpoint splits
use ONE paired de Casteljau pyramid, O(p²), writing both child nets. At every
level use the common RN interval `lerp=.5*a+.5*b` DAG; CPU and GPU perform the
same arithmetic per coefficient. GPU uses two shared132-interval slabs, stages
the parent before any child write, and writes both endpoint sequences. Do not
recompute a blossom independently for every coefficient/child (O(p³)). Parent
path/profile is semantic cache identity, and eviction replays the same DAG.

## Actual-chord bounds and stronger finite profiles

Let u∈[0,1] be the normalized ORIGINAL interval and L(u)=(1−u)Q0+uQ1. For an
original positive rational Bézier curve C=N/W of degree p, form R=N−WL of degree
p+1. With N degree-elevated once, its k-th coefficient is

    R_k = [(p+1−k)N_k + k N_(k−1)]/(p+1)
          −[(p+1−k)W_k Q0 + k W_(k−1)Q1]/(p+1),

with absent terms zero at the ends. Then `sup||C−L|| <= max_k||R_k||/min_i W_i`.
Anchor N by W*A and Q by A before this residual, preserving the same exact
identity while avoiding large world-translation cancellation. This includes
fixed original vertices and constructed endpoint departures already: do not
add those errors again. Restriction plus positive W supplies the convex bound
for all degrees<=32; no low-degree adjacent-weight derivative constant is
silently generalized to high degree. Numerical uncertainty promotes before
spatial splitting if the retained source/point interval floor dominates.

For original analytic C(t)=O+tX, M2=0. For circle/ellipse
C(t)=O+rx X cos(t)+ry Y sin(t), set M2>=||[rx X,ry Y]|| by the existing scaled
Gram bound using original frame/radius products enclosed outward. Nonorthogonal
axes remain original. Filip's vector theorem gives

    parameter_upper = M2*(b−a)²/8 + max_endpoint||original C−actual Q||.

It avoids cubic conversion and per-leaf rational trig. The point evaluator uses
the already selected shared bounded RN trig/profile; exact source evaluation
is only the selected ambiguity fallback. For a circle image with span<=pi, a
separate angular/sagitta correspondence may improve constants; do not substitute
it into a same-t result without a retained map. General conic-image map is not
required in the first finite implementation.

The first stronger profile IS a checked line image. Choose a retained coordinate
projection p(t)=a·C(t) with nonzero original endpoint difference. Prove source
collinearity and monotone p (nonnegative derivative/control direction plus
strict endpoint progress; stationary endpoints such as t³ are allowed). Define

    sigma(t) = (p(t)−p(a))/(p(b)−p(a)).

Bound C(t)−[Q0+sigma(t)(Q1−Q0)], keeping the original scalar endpoints rather
than assuming p(a)=a·Q0. For rational source this residual has degree p, not
p+1, after clearing W and the scalar endpoint denominator. A zero residual
accepts any speed on that straight image. Continuity, monotonicity and endpoint
progress ensure all chord points are covered. Store the original source/endpoint
descriptors that define sigma, projection and orientation; rounded scalar
brackets are bounds, not an invented exact map. Parameter_upper and image_upper
are distinct fields. The runtime may select image acceptance only when every
incident selected trace carries a compatible explicit image policy. An
unlabelled min of two differently parameterized bounds is invalid.

For positive rational curves, monotone control-polygon scalar order is a cheap
sufficient condition via the positive tangent-cone property (Floater); a stricter
coefficient check of p' numerator is available only on ambiguity. Source line
collinearity is checked once/root from original values; do not rediscover it
through exact per-leaf symbolic arithmetic. Failure of this optional profile
falls back to the parameter bound, not source invalidity.

## Selected trace service: direct restriction, no per-use surface mesher

For each selected UV segment gamma(u)=UV0+u(UV1−UV0), the construction owner
asks the geometry service to bound original S(gamma(u)) against SAME Q chord.
Use retained SourceCache, not a PatchSession per use or the old eager
GeometryStore. Partition gamma only at indexed original surface span boundaries;
these exact rational subparameters are virtual bound pieces, not new physical
edge samples. Keep L restricted by its original u on EVERY piece; never resample
S to replace a virtual L endpoint.

For a rational surface root of degree p,q<=3, affine-segment restriction gives
a positive homogeneous Bernstein curve of degree<=p+q<=6, using the same
Bernstein/blossom affine convolution as the existing triangle restriction.
Apply N−WL/Wmin directly. On analytic carriers use the directional Muu,Muv,Mvv
compiled from original frames and a,b UV rates:

    M2_trace <= Muu*du² + 2*Muv*abs(du*dv) + Mvv*dv².

Add the two original carrier-at-selected-UV to actual-Q endpoint departures and
apply normalized interval²/8. All ordinary constants and samples use the shared
outward RN numeric path. The original finite chart/regularity admission remains;
no hidden second2D adaptive controller is created just to process one trace.

Retain separate spatial-chord and carrier-trace-to-chord bounds. Their sum is
a safe Ce↔Sgamma bound with an explicit common parameter map, but can be loose.
On ambiguity/profile demand tighten the SAME trace: rational Ce degree<=32 and
rational Sgamma degree<=6 have coupled numerator
Nc*Ws−Ns*Wc of degree<=38. This cancels exact source agreement before enclosure.
For an exact cylinder circle/meridian harmonic identity, compare the original
carrier and curve descriptor algebra/parameters: same original harmonic frame,
same height and selected angle literally equal to spatial t. Retain this
`CylinderAffineIdentity` descriptor; zero-angle/height differences must follow
from exact source equality, not a tolerance. An arbitrary phase, different
angular parameterization or literal binary64 2pi is NOT symbolic equality.
The generic parameter bound still works when this optional identity is absent.
No general CylinderArcOrder/image tag is admitted without an implemented map.

Authoritative raw pcurve flattening additionally bounds original lifted P(m(t))
versus selected UV chord using the same2D curve routine and selected endpoints.
A carrier-J upper over their admitted convex support transports that UV bound
to a positional component; this avoids composing degree32 pcurves into degree6
surfaces (potential degree192, then224 with Ce cross multiplication). Keep this
domain component distinct from the node-source cap and future face-domain map.
At C0 source spans use partitioned position/J bounds, never claim a global C2
derivative. Constructive star/strip topology remains the next whole-domain unit.

Reconstructed pcurves are only branch/recovery proposals. First try the cached
raw pcurve point converted by the declared deterministic dyadic-UV policy, and
admit only after the original-node guard. The first finite unit additionally
implements analytic-plane dual-frame recovery and cylinder branch-preserving
angle/height recovery; their finite locator policy belongs to the construction
unit plan. Other carrier kinds admit guide-derived sites only when the same
original-node guard passes, otherwise return typed unsupported/unresolved.
Generic damped Newton is a possible explicit later extension, not an unstated
general inverse prerequisite. Original-node residual admission
uses original Ce/V enclosure versus original S(chosen exact UV), not either Q.
Any chosen UV satisfying the node cap on the retained source branch is valid;
there is no need to isolate a unique closest/exact inverse at every node.
Poor guides/finite locator conditioning may return bounded unresolved.
NURBS natural-domain exit is unsupported; analytic prepared-envelope exit asks a
new source-root closure transaction, never an uncounted extrapolation.

Common face-vertex joints choose ONE UV per face/declared branch before traces.
Two authoritative endpoints need the same original descriptor/parameter, exact
rational equality or an explicitly checked analytic identity. Interval overlap
or distance below a tolerance is not equality. Literal same-face seam occurrences
at u=0 and stored binary64 u=2pi remain separate source UVs even when both shifts
are zero; nonzero period shifts require a supported analytic axis/retained true
period descriptor. No period is inferred for NURBS. Map kinds1(identity) and2
(declared piecewise affine) are the first finite admission; unknown kind3 is
typed unsupported, not silently identity. Split schedules seed every original
map-t breakpoint; s may backtrack across pieces, so do not infer coverage from
the two s endpoints of an unsplit interval. Ordinary traversal follows literal
spatial-t endpoints and use sense, never a second curve_same_sense reversal.

## Exact proposed native seam

Namespace `bgmesh::adaptive`; these are proposed declarations, not source stubs.
Use existing `Interval`, `ArithmeticStage`, `ScalarRange`, numeric profile and
source/RunResource leases. New CUDA17 POD header: `adaptive_curve_types.h`.
New common HD implementation: `adaptive_curve_numeric.hpp`; source compilation/
checked host/precise functions: `curve_source_cache.hpp/.cpp` and
`adaptive_curve_geometry.hpp/.cpp`. No old geometry module object is constructed
to obtain a handle. Existing owned exact arithmetic/extraction is lazy fallback.

    CurveRootSlot { uint32_t value; }
    OriginalCurveRootSlot { uint32_t value; }
    CurveFamily { line, circle, ellipse, rational }
    CurveCorrespondence { affine_parameter, monotone_line_image,
                          cylinder_affine_identity }

    CurveRootDescriptor {
      CurveFamily family; uint32_t dimension, degree;
      OriginalCurveRootSlot original;
      uint32_t source_slot, original_span; uint64_t source_id;
      ScalarRange controls; // stride dimension+1; interval homogeneous values
      Interval original_t[2], prepared_t[2];
      double frame[12], radii[2];
      Interval second_derivative_upper;
      uint32_t line_axis, line_profile; // optional checked original identity
    }
    CurveSampleInput {
      OriginalCurveRootSlot original; Interval t; ArithmeticStage stage;
      uint32_t fixed_actual; double actual_xyz[3]; // original V when fixed
    }
    CurveSampleResult {
      NumericStatus status;
      Interval original_xyz[3]; double actual_xyz[3]; double departure_upper;
    } // dim2 has exact third0; no fabricated source z component
    CurveNodeInput {
      CurveRootSlot root; Interval t[2]; ScalarRange controls;
      double actual_endpoints[6], endpoint_departures[2]; ArithmeticStage stage;
    }
    CurveNodeBounds {
      NumericStatus status;
      double parameter_upper, image_upper, numerical_upper;
      uint32_t image_available; CurveCorrespondence correspondence;
    }

CurveSourceCache owns original SourceStore+resource and flat immutable original/
prepared descriptor/net arrays. `CurveRequest{Slot<CurveTag> source; double t[2]}`
names the source interval; a physical edge range still belongs to its source edge,
not just a CurveSlot. Source/profile/stage generations qualify all borrowed slots.
Factory:

    prepare_curve_source_cache(const SourceStore&, span<const CurveRequest>,
       const shared_ptr<RunResource>&, NumericProfile, CurveCacheLimits, Budget&)

Checked host functions and common HD counterparts:

    evaluate_curve_sample(cache, CurveSampleInput) -> CurveSampleResult
    bound_curve_node(cache, CurveNodeInput, optional borrowed net) -> CurveNodeBounds
    numeric::evaluate_curve_sample_rn64(original_descriptor, net, input)
    numeric::bound_curve_node_rn64(prepared_descriptor, net, input)
    numeric::curve_midpoint_pair(root, parent, left, right, caller_workspace)

The last routine specifies ONE pyramid/explicit interval lerp DAG; CUDA cooperates
on the same levels using shared double buffers. A separate coefficient-residual
helper lets the GPU reduce34 coefficient norms with lane-strided iterations.
Exact sample/bound fallbacks take exact original parameter spans plus actual
endpoints and exact allowance; they do not recover truth from float interval
endpoints. Forced fixed V recomputes its true Ce departure rather than borrowing
an interior generated point's departure. An unresolved bound is not witnessed
infeasibility. Result values are immutable POD; no Budget/host pointer escapes.

Selected trace seam (host compiles virtual span pieces; batchable POD executes
their numerical bounds on the SAME source service):

    AffineTraceRequest {
      SourceCache carrier_roots; CarrierSlot carrier;
      exact UV[4]; exact actual_chord_xyz[6]; exact original_interval_u[2];
      source/profile/query generations;
    }
    AffineTracePiece {
      source_root; exact subinterval_u[2]; interval restricted_H_range;
      interval actual_chord_xyz[6]; // affine restriction of same actual L
    }
    TraceBounds {
      carrier_trace_to_actual_chord_upper;
      source_curve_to_selected_trace_upper; // explicit sum or tighter SAME-map residual
      optional compatible image_upper;
      correspondence; root/piece identity; numeric/source components;
    }

The optional exact cylinder identity payload is finite:

    CylinderAffineIdentityRecord {
      Slot<CurveTag> curve; Slot<CarrierTag> carrier;
      exact source_t[2], chosen_uv[4], constant_v;
      traversal_orientation;
    }

It requires original Ocurve=Ocarrier+vZ,
rx_curve*Xcurve=r_carrier*Xcarrier and
ry_curve*Ycurve=r_carrier*Ycarrier, chosen u literally equal to original t,
fixed v and admitted source intervals. Exact rational products compare original
binary64 values; equal approximate axes are insufficient. This records a
same-parameter identity, not a general circle-image/phase correspondence.
Unknown phases, signed/shifted angle identities and a stored binary64 full turn
fall back to generic bounds until their finite maps are actually implemented.

`prepare_affine_trace(cache,request,JobContext&)` seals exact piece coordinates
and typed work; `bound_affine_trace_piece_rn64` shares rational chord residual
or analytic directional formula; exact fallback uses the retained exact values.
No per-use PatchSession. Construction owns the complete per-use transport:
original use/edge/face, original t and mapped s, period/branch, chosenUV and
node-source bounds, selected trace correspondence plus domain approximation
component. Root/runtime own packet/scheduler details, retaining all originals.

Select the SAME owned CUDA module implementation/lifetime with a separate
versioned CurveCudaAPI export and curve-mode token/pools. It uses typed curve
descriptors and paired restriction destinations, not fake surface roots or a
second ad-hoc allocator/drain implementation. Parent owns API/device wrapper;
runtime owns CurveSession and the sole DefinitionBoundaryForest authority.
The authoritative source/curve algebra and HD arithmetic remain this geometry
unit's invariant. Exact public C ABI names are finalized by those owners from
these finite numerical PODs before product code.

## Cohesive first delivery, direct oracles and failure classes

The next delivery constructs every edge of one requested definition and every
incident selected trace, through original SourceStore→requested curve/carrier
roots→one source-edge forest→CPU/P100 actual samples/restrictions/chord/trace
bounds→immutable complete boundary snapshot. Fresh query forests preserve
cold/warm coarse reselection while numerical caches persist. All uses must settle
at the complete physical horizon; one easy face cannot publish a private chain.
This is a boundary/chosen-trace operation, not a polyline evaluator demo and not
yet an onto face-domain map or complete trimmed component.

Preselect these independent direct families before implementation:

* Exact POWER/positive rational line/quadratic/cubic and degree32/degree-elevated
  originals; nonclamped knots, C0 joins, misleading adapter spans; a real jump
  must refuse. Fraction Cox recursion/power formulas independently check roots,
  original sample enclosures and Bernstein residual coefficients/whole-chord
  inequalities, not CPU-versus-GPU alone.
* Analytic circles/ellipses with nonorthogonal frames, small/large translation,
  fixed endpoint V offset and multiple turns. Independent high-precision trig
  or exact rational conics with their retained parameter map check source/chord
  relationships. Whole analytic interpolation theorem is tested independently
  of sparse visual flatness.
* Straight image with t³ stationary endpoint and positive varying weights,
  differently parameterized UV guide/nonlinear planar carrier. Image profile
  must avoid speed-only source splits and retain a map, not relabel parameter
  error as zero. Perturbed transverse source component must break zero residual.
* One physical edge shared by two faces, opposite use traversal, descending edge
  interval and false curve_same_sense; same-face cylinder seam u=0/stored2pi,
  authoritative versus reconstructed guides and unknown map kind. Check original
  CSR occurrence inventory, identical Q/key/t chain and distinct UV branch facts.
* Tiny closed single cubic with repeated endpoint and no interior knot: below-
  epsilon chord acceptance cannot delete its hole; source topology must request
  enough distinct samples. Curve cache returns bounds, not topological permission.
* Source Ce/carrier mismatch straddling the nodal cap, even if generated Q happens
  to lie on S. Endpoint V/Ce guard is independently checked. Whole selected-trace
  sagitta follows positional allowance, not the nodal cap.

CPU/P100 byte semantics checks attack execution determinism separately from
original-source geometry. Scoped sanitizers/lifetime/refusal tests cover source
leases, stage generation, capacity, cancellation and failed drain. Measure
degree buckets, point/residual/restriction work and coarse/fine/cold/warm complete
operation costs; retain censors/regressions. No full-watch retry, copied source,
new dependency or mutation/certification campaign is required to close this unit.
