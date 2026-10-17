# Cached shared curves: occurrence-trace construction

Status: concrete first-checkpoint proposal; report and API only, no product code.
The operation returns one physical chain per original edge and all incident
chosen occurrence traces. It does not yet emit trimmed faces or claim an onto
face-domain homeomorphism. Read with docs/adaptive-shared-curves-unit.md and the
geometry/runtime reports. The old eager GeometryStore/face preparation is not
run as a hidden prerequisite.

## Grounding actually read

NVIDIA USD/BRep revision c9c5979f04cea8c38213004d4f04df0dcfb40d05,
source/SMLib/src/SmTess.cpp:

- SmCurveTessDriver::TessellateCurve,1320–1438, first tries curve-specific
  bisection, then SmCurveCache; evaluator/equally-spaced alternatives are real
  separate branches. They are not the only possible general algorithm.
- ClampAndSplitEdge,2884–2987, deletes proposed samples near endpoint vertices
  under source tolerances, then splits the mutable B-rep edge. We retain original
  endpoints but do NOT inherit tolerance-driven feature deletion.
- TessellateEdge,3009–3066, discretizes the physical curve interval and modifies
  the edge before face construction. Phase1SetupBrep around11490–11585 also
  inserts iso-surface compatibility splits. Shared topology propagates splits;
  faces do not independently invent boundary XYZ.
- SubdivideManifoldEdge,10843–11054, explicitly updates radial partners. Its
  disabled midpoint-dropping alternatives discuss singularity collisions and
  expensive searches; those are not a justified generic inverse policy.
- SmCurveCache.cpp1210,1511–1570,1710–1858: ordered span blocks, cached accepted
  leaves and binary subdivision. Reuse these mechanisms, not pointer ownership
  or minimum-size forced acceptance.

OCCT8.0.1 revision b8f597c677811d1f9f4d8a97f5ae2825c0353a42,
src/ModelingAlgorithms/TKMesh/BRepMesh:

- BRepMesh_EdgeDiscret.cxx performInternal/process,80–191: parallelize physical
  edges; optionally reuse the best existing polygon; otherwise create one curve
  discretization. Tessellate3d,216–282, fixes source vertex endpoints.
- Tessellate2d,286–339: walk EVERY pcurve and project the common 3D schedule into
  it. This is the useful physical-owner/occurrence distinction.
- BRepMesh_EdgeParameterProvider.hxx Init/Parameter,90–182: SameParameter uses
  the stored parameter directly; otherwise scale a predictor and run local
  projection, with a monotonicity guard against period/order reversal. This is
  a cheap proposal mechanism, not a whole-interval correspondence theorem.

Archives/revisions are in QUARANTINE.md and papers/reference-catalog.tsv. No
reference code or dependency is adopted. Exact line scopes refer to retained
files, not the separately packaged official NVIDIA binary's build settings.

Filip/Magedson/Markot1986 §2.1 Theorem2, full acquired PDF/text reread, proves
vector curve linear interpolation error <=h² sup||C''||/8. Its parameter-speed
sensitivity is real; use it for genuine curves, not as the sole straight-image
criterion. Balazs/Guthe/Klein2004 §§4–6, full acquired, separates trimming curves
from surface subdivision but composes/reduces curves numerically; that conversion
is not original-coefficient authority. Ma/Hewitt2002 §§4.1–4.5, full acquired,
and Marussig/Hughes2017's trimmed-representation discussion motivate retaining
both spatial and parameter curves rather than assuming they agree. The recent
Li etal JCDE13(1),239–258, boundary mapping assumes exact shared intersections;
toleranced source gaps do not automatically satisfy that hypothesis.

Lutterkort/Peters TR99-004 (1999), institutional metadata only: its advertised
spline/control-polygon envelopes are a useful stronger-bound candidate, but the
primary PDF returned403 via browser and fleet curl. Not counted as full read or
selected machinery. Existing positive-rational residual and exact image tests
already make this first operation constructive; no acquisition wait blocks it.

## Selected finite scope and source truth

Admit original spatial/pcurve lines, circles, ellipses and positive rational
splines within the geometry service's degree32 contract. Surface traces use
current analytic plane/cylinder/cone/sphere/torus or admitted rational tensor
SourceCache roots. Mandatory reconstructed recovery fast paths are analytic
planes and cylinder iso uses. Generic rational/other analytic carriers accept
source-guide-derived chosen sites when the ORIGINAL source guard passes; when
that fails, return named unsupported recovery rather than claim a general
closest-point inverse. Later inverse support is a separate finite extension.

All definition edges and incident uses are inventoried. Unknown map/curve/chart
profiles fail complete publication, never disappear. Ordinary t follows literal
Edge.parameter_range[0]→[1], already associated with vertex_ids[0]→[1]. Do not
apply curve_same_sense again. Occurrence USE.sense controls traversal of that
chain. Map kind1 is original s=t; kind2 is its original monotone piecewise affine
t→s map. Partition at map-t pieces and spatial original knots. Pcurve-knot
preimages partition numerical work; they do not automatically become physical
mesh samples. Map kind3 is the existing unproved-continuation profile and is explicitly
unsupported; never substitute identity for it or another unknown kind.

Nonzero periodic_shift is admitted only on original analytic periodic axes:
u for cylinder/cone/sphere/torus, v for torus. Retain the symbolic original
2*k*pi lift in the source-point descriptor; its chosen dyadic approximation
carries enclosure/departure. Nonperiodic shifts and unknown NURBS periodicity
are unsupported. Distinct literal seam coordinates remain distinct even when
both shifts are zero. Authoritative joint equality requires the same original
pcurve descriptor, mapped-s and lift, or exact endpoint equality after cancelling
a common lift. A separately implemented symbolic analytic identity may establish
other equality. Overlapping enclosures or tolerance do not establish equality;
proved unequal joints are invalid source, unresolved equality is unsupported.

Original V/Ce versus S(chosenUV) is the source-node guard. At endpoints retain
original V versus original Ce as another source obligation. Rounded actual Q
is absent from those comparisons. Q departure is counted in actual chord/trace
approximation. Original source tolerance is metadata, not permission to enlarge
the requested node cap or epsilon.

## One concrete procedure

1. Compile live original curve/pcurve roots and selected surface roots lazily.
   Immutable source caches share numerical work; physical keys remain owned by
   (definition,edge), independently of how many edges reuse a CurveSlot.
2. The sole edge authority selects exact root-relative dyadic t intervals and
   constructs ONE canonical Q per original parameter under the common numeric
   profile. Original endpoint vertices remain fixed. Retain original closed-edge
   endpoint occurrences even when their physical key/XYZ is equal.
3. For every original face loop, identify joints by LOOP-ADJACENT OCCURRENCE
   sector, not merely (face,VertexID,periodic_shift). Recover one chosen UV for
   each actual joint and use it on both incident trace intervals. Equal physical
   node keys may legitimately have different UV sites on the same face.
4. Analytic plane recovery uses the exact dual of its original basis:
   G=B^T B, uv=G^-1 B^T(Ce−O), with checked detG>0. This is not an invented
   orthonormal frame. Bound/round the ORIGINAL target to chosen dyadic UV, then
   check original Ce/V against original S there. A reconstructed guide locates
   the branch but does not impose its speed. Authoritative pcurve endpoints
   remain original(use,mapped_s) descriptors; round them with departure bounds,
   and refuse inconsistent authoritative joint source points.
5. Cylinder iso recovery retains its original unwrapped constant-u or constant-v
   pcurve occurrence. Checked source circle/vertical-line/frame identities supply
   the source correspondence; a local guide proposal never normalizes the angle
   to another turn. Generic fallback samples the original mapped guide, chooses
   a deterministic dyadic UV and checks the same source guard. A failed generic
   guard returns unsupported recovery in this first scope.
6. Bound every actual physical chord and EVERY incident selected affine UV trace
   over the WHOLE interval by the selected tests below. Add authoritative
   original-pcurve→chosen-chord displacement as a separate retained bound.
7. Collect all failed interval/numerical-promotion requests over the COMPLETE
   definition horizon, union by physical owner/key, then split/promote once.
   Rebuild only incident occurrence traces. A source-fixed discrepancy does not
   shrink merely because t is split: unresolved bounds/precision/resource floors
   yield typed failure, not minimum-size success or tolerance inflation.
8. Publish immutable chains, occurrence sites, explicit correspondence and bounds
   only after every original use settles. This output is sufficient for later
   trim classification; it is not a claim that an authoritative curved domain
   has already been mapped onto a polygonal face mesh.

## Selected tests: cancellation and image first, cached bounds next

For interval I with normalized lambda and actual chord L=(1−lambda)Q0+lambda Q1,
use cached rational residual N−WL (degree p+1), anchored and divided by positive
Wmin. Analytic fallback is h²M2/8 plus maximum original endpoint departure to Q.
Line and constant expressions cancel before interval enclosure.

The mandatory nonlinear-speed fast path checks a spatial STRAIGHT IMAGE from
original homogeneous controls. For original endpoints A,B and d=B−A, choose a
nonzero component j and check exactly

    (N_k−A_k W)d_j − (N_j−A_j W)d_k ==0, for every k/control.

Let r=(N_j−A_jW)/(d_jW). Check its rational derivative numerator in Bernstein
form: nonnegative coefficients with one positive imply positive derivative in
the open interval, allowing stationary endpoints such as t³. Reverse sign for
opposite traversal. Proof-only restrictions can establish this property without
adding physical samples. A nonmonotone line cannot silently use this profile.

For a plane selected UV chord, its carrier image is a physical segment D0D1.
Reparameterize BOTH source and selected segments by normalized image scalar mu,
not source t. Then

    source→actual Q chord <=max(||A−Q0||,||B−Q1||),
    source→selected carrier trace <=max(||A−D0||,||B−D1||).

This removes speed-only refinement even with a differently parameterized guide.
Retain the named scalar image map, not an invented affine t correspondence.
The checked circle/cylinder iso profile similarly retains angular/order data;
source angular parameter versus rational conic parameter is never assumed equal.

Otherwise selected gamma(lambda) is affine in chosen UV. The retained source
service indexes ORIGINAL span rectangles intersected by gamma, partitions at
exact crossing parameters, and directly restricts S to this affine segment.
For admitted bicubic rational roots its homogeneous trace degree is <=6;
N_gamma−W_gamma*the SAME actual L has degree <=7. Cache the source restriction
and bound anchored coefficients/Wmin. Analytic carriers use directional
h²M2/8 plus actual original-carrier endpoint departures to that SAME L.
The service batches all uses on CPU/P100; it MUST NOT create/run PatchSession,
perform full 2D refinement or prepare the old GeometryStore per occurrence.
Combine Ce−L and S(gamma)−L as a safe source/trace bound; checked identical
expressions and coupled cancellation tighten only when needed (spatial degree32
and affine bicubic trace give degree<=38). No degree32 GUIDE composition into
bicubic carrier (up to192) is the default. Virtual span crossings are numerical
pieces, never private physical-edge nodes. A loose upper is not an observed
source mismatch. Higher carrier degrees require the geometry service's declared
finite composition bound or named unsupported, not an assumed bicubic limit.

For authoritative pcurve P(s(t)), separately bound each component of
P(s(t))−gamma(t) from original pcurve controls, with map-piece restrictions.
The source chart displacement bound is <=Mu*delta_u+Mv*delta_v over a containing
natural-domain hull, using cached one-sided derivatives across C0 spans. This
is a displacement allowance consumed once by later domain/cell construction,
not a complete face-map theorem and not another full epsilon granted privately.
Original pcurve points and selected endpoints stay in the admitted original
chart. Tangencies/cusps or unavailable branches report finite unsupported rather
than inventing a regular isotopy. Full B/T-domain maps remain the next unit.

## Public finite seam; trace owner versus runtime owner

Runtime owns SharedCurveSnapshotData with resource, SourceStore, definition,
query_generation, ExactTable values and DefinitionBoundarySnapshot boundary.
It stores UseCurveTrace/UseCurveSite/UseCurveInterval in packed PMR arrays.
All refs address the retained exact table; no Rational/Budget escapes.

UseCurveTrace: original use+face, face_generation+boundary_generation, ranges of
sites and intervals, original authority policy. UseCurveSite: shared boundary
node index, original_t, mapped_s, chosen_original_uv[2], chart_xy[2], original
periodic_shift[2], LOOP+junction occurrence identity, source-node upper.
UseCurveInterval: endpoint site indices, original BoundaryKey, correspondence
kind (affine_parameter/monotone_line_image/cylinder_iso_order), finite scalar or
angular map refs, source_to_selected_trace_upper, UV-chord component uppers[2]
and optional authoritative_domain_upper. Null authoritative bound is permitted only under reconstructed selected
policy; authoritative policy with a missing bound fails.

Trace construction owns native/mesher/adaptive_traces.hpp/.cpp after plan review.
One FaceTraceConstruction owns the ordered original loop/use joints for a face;
its immutable request retains source, both source caches, current physical
boundary snapshot, allowances and source/query/face/boundary generations. It
has no mutable forest access, evaluator callback or private split authority.
The proposed finite staged seam is:

    prepare_trace_sites(request, original_curve_samples, JobContext&)
        -> TraceSiteWork{chosenUV refs, carrier_sample_requests, retained values}
    complete_trace_sites(work, original_carrier_samples, JobContext&)
        -> TraceSites{accepted rows OR precision/refusal outcome}
    prepare_trace_intervals(sites, JobContext&)
        -> TraceIntervalWork{affine_trace_pieces, raw_pcurve_chord_work,
                             checked image/cylinder identities, retained values}
    complete_trace_intervals(work, ordered numerical bounds, JobContext&)
        -> UseTraceResult{rows OR canonical BoundaryKey requests}

Runtime batches original spatial/pcurve samples, then carrier samples and trace
pieces across faces, calls these host constructors in source order and folds
all results at its ONE complete owner horizon. Each phase checks the full request
identity and retains exact tables; Rational/Budget values never cross phases.
Mandatory plane recovery uses the exact dual above; cylinder iso uses its checked
original identity/branch; otherwise a deterministic guide-derived candidate must
pass the original guard or refuse. No general Newton inverse is required here.
A future optional bounded Newton proposal cannot bypass the same source guard.
TraceIntervalWork calls geometry's prepare_affine_trace(cache,request,JobContext)
which returns sealed original-span pieces; bound_affine_trace_piece_rn64 shares
the rational actual-chord/analytic directional kernel. CylinderAffineIdentity retains original curve/carrier slots, source-t endpoints,
chosenUV endpoints, constant-v and orientation. Require exactly
Ocurve=Ocarrier+v*Z, rxcurve*Xcurve=r*Xcarrier,
rycurve*Ycurve=r*Ycarrier, and selected u=t literally. A general arc-order tag
is not emitted without its actual finite map producer.

UseCurveInterval stores BOTH carrier_trace_to_actual_chord_upper (positional
acceptance) and source_to_selected_trace_upper (Ce↔Sselected relationship).
Ce→actualQ is separately accepted at physical owner. These quantities are not
interchangeable; Ce↔Sselected never uses the original-node cap. Acceptance is
Ce→Q<=tau and carrier-trace→Q+authoritative-domain<=tau. The retained Ce↔Sselected
sum may reach2tau; it has no separate same-tau cap. It may instead use a
compatible tighter coupled bound. Snapshot semantics name which
correspondence each bound uses. Original FaceBoundarySnapshot remains later
transport, never an eager GeometryStore prerequisite for this operation.

Geometry report and runtime implementation unit received scoped ACCEPT after
these service/ownership distinctions. The current root unit plan and runtime unit plan receive scoped ACCEPT after
actual reread: the separate actual-carrier-trace bound is incorporated;
implementation still starts from direct original-output RED, not this report.

## Direct original-output adversaries

- Two planes share one nonlinear-speed rational straight edge: original cubic
  scalar controls (0,0,0,1), differently parameterized reconstructed line guide.
  Whole image is a segment. Require two physical endpoints when no other
  topology need demands splits; compare exact original formulas and guards.
- Cylinder rectangle uses ONE physical vertical edge twice at u=0 and stored
  binary64 2pi, both shift0. UV chains remain distinct; physical keys/Q match.
- One circle shared by plane and cylinder: plane UV chord has sagitta/domain
  displacement, cylinder selected iso trace may be exact. Same physical chain.
- Whole-interval mismatch vanishes at sample endpoints but bulges internally;
  nodal-only acceptance must fail. A fixed source offset must not be hidden by Q.
- Positive-weight high-degree/nonclamped/C0 knots, reversed t, map-piece breaks,
  misleading adapter spans, stationary endpoints and nonmonotone straight image.
- Tiny one-edge closed hole with repeated vertex and radius below epsilon:
  preserve source interval/occurrence inventory. Topology must later request
  enough distinct noncollinear sites; no radius-based deletion or fixed circle
  sample count. This boundary checkpoint must not call a collapsed polygon a
  complete face domain.
- Declared degenerate edges/poles retain explicit quotient/pcurve coverage and
  original alias guards. First unsupported pole/cap construction refuses complete
  publication; equal coordinates never create a quotient. Finite sphere/cone
  cap recipes remain named later specialization, not an ordinary trace fallback.

Use original rational/analytic formulas plus exact interval residuals on actual
output, source inventory, common-node/key equality and separate UV seam values.
CPU1/best parallel CPU/both P100 compare actual semantic bytes and fresh-query
history. Counters identify real curve evaluations/restrictions/bounds. Resource,
stale generation and ownership tests attack separate fault classes; do not add
a second validator for the same geometry claim.
