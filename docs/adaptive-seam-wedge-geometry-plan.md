# Quotient source geometry and actual cap bounds

2026-10-17. Literal first geometry unit PLAN, accepted by root for implementation;
runtime shared-key review incorporated below. The selected design is
[the seam-wedge study](../papers/native-adaptive-seam-wedges-geometry.md).
This unit owns one invariant: a quotient-capable ORIGINAL source bank permits
singular source evaluation, ordinary nodes prove local regularity, and explicit
mapped-cap tasks bound the whole finite source map against SAME actual L.
Construction/cut control, runtime continuation, CUDA adapter and publication have
their own named owners below; their shared contracts are part of this plan.

## Grounding and scope

Pinned USD/BRep `c9c5979f04cea8c38213004d4f04df0dcfb40d05`,
`SmTess.cpp::sms_FillGapUVWithPolyEdge` around1440–1595 and `MakePolygon`
around1610–1760 explicitly retain one 3D vertex with two UV sectors and an extra
POLYGON edge. `TessellateEdge`/`Phase1SetupBrep` supply shared physical chains.
Pinned OCCT8.0.1 `b8f597c677811d1f9f4d8a97f5ae2825c0353a42`,
`ShapeAnalysis_Surface::DegeneratedValues/ProjectDegenerated` and
`BRepMesh_EdgeDiscret::Tessellate3d/Tessellate2d` supply the singular-sector and
shared-chain reference mechanisms. We do not inherit their repair tolerances.
Filip–Magedson–Markot1986 §2 Theorems2/4 supply the vector interpolation
inequality; the cap application is our derivation already reviewed in
`geometry_poles.cpp::bound_analytic_pole_fan` and
papers/2026-10-16-native-mesher-cap-algorithm-review.md.

Finite profile: nonzero-radius sphere north/south or nonzero-angle cone, finite
independent original frame, one checked shared-original-vertex seam wedge, one
outer loop, repeated nondegenerate physical meridian, regular opposite side,
reconstructed selected meridians with admitted constant-U proposals and strictly
monotone selected V. No new source edge, coordinate weld, general inverse, old
GeometryStore/PreparedRun or arbitrary authoritative B/T map. The common math
also supports the old finite source-band shape, but this unit does not pretend
an original collapsed use exists in a three-use wedge.

## Literal source-bank and ordinary-node changes

Own `source_cache.hpp/.cpp`, `adaptive_numeric.hpp`, `adaptive_geometry.cpp`
for these bounded changes, after a coherent shared-file ownership window:

```cpp
enum class SourceCachePurpose : uint32_t {
  regular_patch, trace_geometry, quotient_trim
};
SourceCache prepare_quotient_trim_source_cache(
  const SourceStore &, std::span<const CarrierRequest>,
  const std::shared_ptr<RunResource> &, NumericProfile,
  SourceCacheLimits, bgfd::Budget &);

// In adaptive::numeric, CUDA17-compatible, SAME body on CPU/P100:
BGAD_HD bool analytic_node_regularity_rn64(
  const RootDescriptor &, const Interval *actual_box4);
```

The new factory retains the complete supplied ORIGINAL rectangle unchanged and
the existing deterministic angular-panel partition. It never recomputes hints,
unions a second guide envelope, truncates a pole or makes a new bank per cut.
It preserves source/resource/profile/generation/finite frame/radius admission and
all positive rational natural-domain/continuity rules. For cone/sphere it permits
original evaluation where the regular-sheet test fails; it does not return a
regularity claim. Existing regular/trace factories keep their existing scope.
Other analytic families follow their existing checks. One full original analytic
evaluator remains the common point owner across prepared panels.

No RootDescriptor layout flag is added. `frame_f` is explicitly the scaled FULL
three-column norm for cone too; currently cone stores `frame_a` there and no
old cone consumer uses `frame_f`. Existing cone ordinary bounds still read A,
sin(alpha),cos(alpha). The new cap Sv bound reads F. The device capability below
binds this semantic addition without changing packed root layout.

`bound_node_rn64` calls the new regularity helper for ALL analytic nodes, including
old-purpose banks; it no longer assigns regularity1 unconditionally. Plane,
cylinder and separated ring torus keep their admitted source proof. Cone requires
`r0+V*sin(alpha)` and `cos(alpha)` separated from zero over ACTUAL node V. Sphere
requires both endpoint cosine enclosures have the same strict sign and V width
strictly less than a conservative pi lower bound. This suffices because distinct
same-sign cosine intervals separated by a zero pair are at least pi apart.
An invalid/wide RN argument returns no proof, never a guessed regular sign.

`bound_node_precise` performs the same local source test with its exact box and
owned explicit-precision trig/radius enclosure; it cannot restore unconditional
analytic regularity. Positional bounds may be ready with regularity0; ordinary
acceptance still requires both. A singular/mixed box must classify as mapped cap,
subdivide or end unresolved. No per-node exact trig on the normal RN path.

## Packed cap math API

New owned headers `adaptive_cap_types.h`, `adaptive_cap_numeric.hpp` are CUDA17
compatible and include the existing source POD/numerical headers only:

```cpp
enum class AnalyticCapKind : uint32_t { sphere_north, sphere_south, cone_apex };
struct CapSourceDescriptor {
  OriginalRootSlot original;
  AnalyticCapKind kind;
  ArithmeticStage stage = ArithmeticStage::rn64;
  Interval pole_v;
  Interval ideal_xyz[3];
};
struct CapFacetInput {
  OriginalRootSlot original;
  Interval cut_uv[4];       // leftU,leftV,rightU,rightV, ORIGINAL coordinates
  Interval finite_v[2];     // two exact finite connector endpoint enclosures
  double actual_xyz[9];     // roles: fixed pole Q, left cut Q, right cut Q
  double allowance_down;
};
struct CapFacetBounds {
  NumericStatus status = NumericStatus::needs_wider;
  double position_upper=0, band_upper=0, total_upper=0;
  double angular_upper=0, radial_upper=0, corner_upper=0;
  double pole_departure_upper=0, uncertainty_upper=0;
  int32_t regular_sheet_sign=0;
  uint32_t positive_chart_roles[3]{};
};
BGAD_HD CapFacetBounds bound_cap_facet_rn64(
  const RootDescriptor &, const CapSourceDescriptor &, const CapFacetInput &);
```

CapSourceDescriptor is derived from ORIGINAL source, not an input claim about a
new curve/carrier. Host preparation binds it to the exact cache lease. Root kind,
original slot and family must match at every host/device operation. All finite
coordinates refer to the retained original source domain, except `pole_v` and
`ideal_xyz`, which are explicitly AUXILIARY analytic comparison values and may
lie just outside it. They do not enlarge the source profile or placement B0.
The descriptor carries no generated pole point: actual pole Q is supplied from
the fixed original vertex in the mapped-domain record.

Host header/body `adaptive_cap_geometry.hpp/.cpp`:

```cpp
// Private constructor, friend prepare_cap_source; no mutable descriptor access.
class PreparedCapSource {
public:
  const SourceCache &source() const noexcept;
  const CapSourceDescriptor &numeric() const noexcept;
};
PreparedCapSource prepare_cap_source(
  const SourceCache &, OriginalRootSlot, AnalyticCapKind, bgfd::Budget &);
CapFacetBounds bound_cap_facet(const PreparedCapSource &, const CapFacetInput &,
                               bgfd::Budget &);

// Source-only construction admission: no fake actual XYZ and no Q sampler.
struct CapMapPlan {
  ExactEnclosure pole_parameter;
  int32_t regular_sheet_sign;
  std::array<uint32_t,3> positive_chart_roles;
};
CapMapPlan admit_cap_map(const PreparedCapSource &,
  std::span<const Rational,4> cut_uv, std::span<const Rational,2> finite_v,
  GeometryBoundPrecision, bgfd::Budget &);
enum class CapProgressState { proved, not_contracted, unresolved };
std::array<CapProgressState,2> cap_cut_progress(const PreparedCapSource &,
  std::span<const Rational,4> old_cut, std::span<const Rational,4> new_cut,
  GeometryBoundPrecision, bgfd::Budget &);

// Owner-local exact output: Rational fields belong to the caller's Budget.
struct PreciseCapResult { PoleFanPlan bounds; bool certified; };
PreciseCapResult compare_cap_facet_precise(
  const PreparedCapSource &, std::span<const Rational,4> exact_cut_uv,
  std::span<const Rational,2> exact_finite_connector_v,
  std::span<const double,9> actual_xyz, const Rational &allowance,
  GeometryBoundPrecision, bgfd::Budget &);
```

Preparation computes sphere pole ±pi/2 and exact O±rZ, or cone
`vp=-r0/sin(alpha)` and O+vp*cos(alpha)Z, using the common RN profile once.
An unresolved source-only descriptor may use the bounded fixed/exact ladder once,
then store outward binary64 intervals; it does not evaluate exact trig per facet.
Zero cone angle is unsupported as an isolated apex, while the old regular
zero-angle cone remains valid. Frame norm compile uses the existing scaled Gram
machinery. Every persistent descriptor retains its source lease; no Budget-affine
Rational escapes preparation.

Host bound validates original/prepared profile containment of finite/cut values,
positive angular width<=pi/2 lower, one strict regular cut side, positive reference
height, finite actual XYZ and actual physical nonzero via the SAME existing
`actual_facet_nonzero` helper. The HD kernel checks the corresponding interval
shape/source/family preconditions. Physical nonzero is a host exact/filter
admission, not a raw floating cross-product test on device. A source-bound-only
task does not sample or change physical Q.

## Finite bound procedure

Use the exact Fsrc, Fideal and emitted bilinear fan L in the accepted study.
Set `H=max|g-vp|` over both cut endpoints and the source pole interval,
`du=u1-u0`, `dg=g1-g0`, and `D=max|vf-vp|` over the finite endpoints. For sphere:

    Muu <= |r|*A*min(1,H),   Muv <= |r|*A,
    Mvv <= |r|*F,            Mv <= |r|*F.

Here |cos(v)|<=min(1,|v-vp|) on the ideal cap hull. For cone, the EXACT source
identity `r0+v*sin(alpha)=sin(alpha)*(v-vp)` avoids cancellation at the apex:

    Muu <= |sin(alpha)|*A*H, Muv <= |sin(alpha)|*A,
    Mvv = 0,                Mv <= F.

The full cone coefficient vector for Sv has Euclidean norm1 by
sin²(alpha)+cos²(alpha)=1, so the full frame norm is valid for nonorthogonal axes
and large Z. Use directed common interval operations throughout:

    angular = (du²*Muu + 2*|du*dg|*Muv + dg²*Mvv)/8,
    radial  = H²*Mvv/8,
    position = angular + radial + maximum actual corner departure,
    band = D*Mv,
    total = position + band.

This Mv covers the WHOLE finite/ideal/cut V hull; it does not use a near-pole
strip derivative. Source-linked constants/intervals make that explicit. Compute
actual corner departure from original ideal pole and the two original source
cut evaluations against the SAME three actual XYZ roles. Cut evaluations use the
common bounded source formula, not a new sampler or old preparation controller.
The ideal pole departure is retained separately from the changing cut departures.
The unused source square corner repeats the same fixed pole Q.

Source-only admission precedes actual cut-point sampling. It checks finite
`g-vf` fixed sign separately from ideal cuts on one regular sheet. For du>0 the
source Jacobian is `du*(g-vf)`, while emitted barycentrics
`(rho*(1-xi),rho*xi)` have Jacobian `-rho`: positive chart roles are [0,2,1]
when g>vf and [0,1,2] otherwise. Never derive roles from proxy UV or assume the
finite and ideal side signs agree. Cut progress compares BOTH symbolic-pole
endpoint gaps by `4*newgap<=3*oldgap`. Pole interval ambiguity is an explicit
unresolved state, not a false contraction or Q-distance heuristic. These helpers
reuse produced source intervals and use precise source trig only on uncertainty.

Return the directional terms for geometry-owned angular/radial cut selection.
The exact fallback calls the reviewed store-free original pole primitive with
the same cut, finite connector range, actual XYZ and explicit precision; it
normalizes finite endpoints to their min/max range without changing Fsrc.
Compare its ORIGINAL rational upper to exact allowance. A successful exact
threshold can be retained as the accepted tau at freeze; do not round it upward
and convert that acceptance back to failure. A conservative upper above tau is
undecided/refinable, never proof of true infeasibility. Fixed source/Q floors stay
separate; original V is not numerically promotable. No claim of automatic
termination at every equality is made.

## Shared domain and runtime contracts, later owners

Construction owner will add `adaptive_cap_domain.hpp` without an include cycle
through adaptive_trim_types. Its immutable source/map rows are qualified by

```cpp
CapDomainKey { source_generation, query, definition, face,
               boundary_generation, chart_revision, domain_revision,
               assignment_epoch, profile_revision };
```

`CapDomainData` retains SAME SourceCache/SharedCurveSnapshot, sealed ExactTable,
source joint pair/vertex, finite connector endpoints, cap pieces and cut control.
`FaceDomainData` gains a trailing `CapDomainLease caps`. Cap source/definition/
face/generation/profile/resource identity must match that whole face domain.
The pooled assignment epoch must equal the full enclosing FaceConstructionKey
and SurfaceFaceIdentity: the same face/query/boundary can be reassigned to a new
owner with reused local revision seeds. A lease from an older assignment cannot
settle that newer operation.
It is a retained PRODUCED map, not a caller boolean. Original loops/use/site
inventory stays complete; the connector is construction-only.

`SurfaceQuery` gains a trailing FaceDomainLease. A quotient_trim operation requires
non-null matching domain+cap map at begin/classify; ordinary PatchSession refuses
quotient_trim. A regular face in a multi-carrier quotient-capable bank may use an
explicit empty cap inventory under the ordinary classifier and local-node
regularity requirement. An outside-D singular portion of its broad source box
need not be made regular. It cannot borrow an unrelated face's cap to bypass
operation admission.
Common source samples remain available independently of ordinary mesh admission.

Extend SurfaceClass with mapped_cap, with a per-candidate cap ordinal or explicit
typed role row. Classify a material intersection entirely in a cap as mapped_cap
(it may ALSO have an outside-D portion), not fake OUT. A candidate containing
both ordinary and mapped material is mixed. It subdivides until the retained
ordinary node's ACTUAL whole box is locally regular; it may then remain an
ordinary boundary-band leaf straddling the cut. Do not demand that a slanted cut
ever align with the rectangular tree. Construction clips its ordinary material
against the shared cut; cap coverage remains separate. Outside stays actual
outside-D. Ordinary retained nodes must prove local regularity. Mapped region
coverage/metadata survives minting the view and footprint overlays; generic
regular packets cannot claim that region. This source partition is retained in
the domain/view and original-output artifact.

Append `domain_revision` to SurfaceFaceIdentity and FaceConstructionKey. A local
cap cut changes this revision and face_generation, with unchanged physical
boundary_generation. Chart revision changes when branch/chart proposals change.
Runtime adds explicit `replace_domain(expected_view,new_domain)` continuation
(full literal request can include exact values/root allowances) rather than
calling replace_boundary or fabricating a forest generation. Preflight checks
same source/query/face/boundary, advancing face/domain revisions, captured old
view, idle/drained operations and unchanged exact quality. Preserve numerical
point stages and monotone table ordinals; invalidate old cover/classifier,
footprints, cap bounds and construction. No old job settles the new horizon.
Physical boundary replacement remains its distinct existing transition.

Construction adds FacetGenerator::mapped_cap and an explicit cap piece/source-role
reference. A positive topology proxy does not enter ordinary UV bounds, metric
refinement or source-normal lookup. Both seam endpoint source rows retain their
different UV sectors while sharing one original vertex/Q. Shared cut sites and
keys are bit-identical across cap/remainder; original orientation applies once.
Root coordinator routes source-edge requests through the complete definition
horizon and face-cut requests through this domain transition. Cut history must
survive one-sided radial epochs and require BOTH endpoint gap contractions.
Semantic export carries maps/partition/generator; display has explicit symbolic
pole/radial-sheet corner descriptors. These are not implemented in this first
numerical source unit and must land before complete cap publication is claimed.

## CUDA capability and ownership

Root owns a versioned mapped-cap extension getter on the SAME DeviceSession,
resident original source bank, poll/ticket/stat/allocator owner. Its finite
operation consumes `{CapSourceDescriptor,CapFacetInput}` records and returns
CapFacetBounds through `bound_cap_facet_rn64`; one scalar task per cap piece is
sufficient, with no large local control arrays. The extension also handshakes
the quotient_trim/local-regularity/frame_f semantics. A quotient session must
refuse an old base-only module even though RootDescriptor layout is unchanged.
Base surface, curve and trim ABIs remain unchanged. Root supplies the literal
extension API after reviewing these PODs. No private device points or second
device session are introduced.

## RED, implementation and bounded qualification

1. Review this literal plan and the shared field contracts. Publish a direct
   authored source-backed fixture calling the missing quotient factory/cap
   preparation/bound operations; observe actual missing-operation RED. Fixtures
   have three original uses and one shared original pole vertex, not a fabricated
   collapsed coedge. First fixture does not need a fake SurfaceBoundView.
2. Implement coherent source capability and local RN/precise regularity together.
   Add true cone/sphere pole-crossing box refusal by ordinary regularity, regular
   retained child proof, old-purpose factory refusal and unchanged original
   evaluator/panel closure. Regular PatchSession refusal and typed FaceTrim begin
   guard are consumed by runtime owner's next unit, not left implicit.
3. Publish actual cap POD/host declarations, implement prepared source constants,
   common RN bound and rare precise adapter. Parent wires target
   `brepgpu_mesher_adaptive_cap_geometry` from adaptive_cap_geometry.cpp, linking
   existing adaptive_geometry and owned geometry/pole implementation. Native
   fixture `brepgpu_native_adaptive_cap_geometry` also links dl for the same
   module extension. No CMake edits from geometry owner.
4. Extend the SAME independent original-geometry reader with `--cap-fixture` and
   retain existing CLI behavior. The native fixture emits original source/map
   refs, actual XYZ, every bound term and local regularity. Reader constructs
   Fsrc from ORIGINAL source, subtracts SAME L, and uses Fraction Taylor interval
   subdivision of the complete xi/rho square; it does not reuse the production
   directional inequality. Direct nonorthogonal/large-Z interior witnesses
   attack the old near-pole-only band defect. Source-fixed node offsets attack
   ideal-point substitution. A degenerate physical facet is explicitly refused.
5. Preselect north/south/cone, reversed source orientation roles, a variable-U
   reconstructed guide source (inventory retained), finite connector offsets,
   large nonorthogonal Z and fixed-Q floor cases. Test exact cache identity,
   source-domain containment, zero cone angle and finite resource refusal.
   Regularity oracle uses independent analytic Jacobian/cofactor signs on
   authored boxes, not the production proved flag.
6. After root extension exists, SAME native fixture/reader on CPU and both P100s;
   source oracle decides geometry, byte equality decides deterministic DAG.
   One focused host ASan/UBSan/leak path, actual-source tidy/format and one direct
   cap extension memory check cover the new fault classes. No deep whole-source
   campaign or work-cap increase. Actual-source numerical and lifetime reviews
   close the first unit; root then integrates construction/runtime/display and
   the existing complete-component oracle before the first real source trial.

Build/run only on fleet in isolated adaptive-cap-geometry-release/asan trees.
Shared headers are formatted in temporary files and atomically replaced.
Announce coherent header/body windows so other owners never build mixed snapshots.
