# Original adaptive envelopes and exact assembly allowances

2026-10-17. Finite implementation subplan, reviewed and implemented. This unit owns original source bounds
and the existing AssemblyStore transport for the cached mesher. It does not mesh,
prepare old GeometryStore/PreparedRun, create a bound bank, recover nodes or
implement curved domain maps/caps.

## Purpose and source contract

The component coordinator needs local positional and original-node caps BEFORE
opening the definition forest or face sessions. `prepare_assembly` already owns
exact usage composition, once-rounded instance matrices, kappa/eta transport and
minimum definition allowances. It requires every original definition's finite
XYZ envelope. The new producer supplies those envelopes directly from SourceStore
and retains the finite UV profile that subsequent source/cache/recovery preparation
must respect. This closes the otherwise circular assumption that a later expanded
guide/recovery rectangle still fits an earlier transform-storage bound.

Retain the complete original SourceStore and all original definitions, including
unplaced definitions. An explicit selected-component coordinator may use one row;
it cannot filter the source to impersonate whole-assembly completeness. Spatial
values are already millimetres; units_mm is retained source metadata, not another
geometry scale.

## Grounding and alternatives

Read actual owned `geometry_envelopes.cpp::{mapped_range,edge_box,
prepare_face_envelope,prepare_definition_envelope}`, `assembly.cpp::{transport,
prepare_assembly}`, source/cache readers and literal analytic formulas in
`geometry_analytic.cpp::evaluate_surface_impl`. Reuse their source semantics,
not their mandatory eager GeometryStore layout.

Pinned OCCT 8.0.1 `b8f597c677811d1f9f4d8a97f5ae2825c0353a42`,
`GeomBndLib_BSplineSurface.cxx::Box` around 75–151 explicitly uses influencing
Cartesian poles when the requested parameters lie in the natural domain.
`GeomBndLib_BezierSurface.cxx::Box` uses all poles for the full surface and switches
to sampled evaluation for restricted boxes. We select the full positive control
hull and EXACT domain admission; the sampled/out-of-domain fallbacks are not
selected. Restoration/hashes are in QUARANTINE.md and papers/bounds-oracle-sources.tsv.
The rational derivative sources already read in native-adaptive-curves-geometry.md
(Floater 1992 §§1–3; Bez/Bez 2013 introduction and Lemma 2.1) assume positive weights;
the simpler hull used here follows directly from the nonnegative normalized basis.

For original homogeneous controls `(N_i,w_i)` with `w_i>0`,
`S=sum(B_i*N_i)/sum(B_i*w_i)=sum(lambda_i*(N_i/w_i))`, where
`lambda_i=B_i*w_i/sum(B_j*w_j)>=0` and `sum lambda_i=1` on the admitted original
natural domain. Thus coordinate min/max of the EXACT ratios enclose the entire
curve/tensor surface. This applies to original nonclamped B-spline natural domains
as well as Bezier [0,1]; adapter saved span controls are not input authority.

Compared alternatives: sampled extrema are insufficient; exact derivative-root
isolation is tighter but unnecessary compile-once work; exact knot extraction and
restricted control hulls are useful later if broad original hulls materially
reduce the transform allowance. An eager all-source GeometryStore duplicates new
cache state. The selected first unit uses original controls and global analytic
angular bounds, with exact arithmetic. This is
linear source scanning, not per-query/per-node exact trigonometry.

## Literal native interface

New files owned here: `native/mesher/adaptive_assembly.hpp/.cpp`,
`tests/native_adaptive_assembly.cpp`, `tests/native_adaptive_assembly.py`, and one
authored fixture writer if the existing source-writer helper is insufficient.
Root owns CMake. Header includes assembly.hpp, not old geometry/store/cache APIs.

Namespace `bgmesh::adaptive`:

```cpp
struct AdaptiveAssemblyKeys {
  ExactTableKey envelopes, allowances; // distinct keys, same caller generation
};
struct AdaptiveAssemblyPrecision {
  AssemblyPrecision transport{};
};
struct AdaptiveAssemblyLimits {
  uint64_t source_items = 100000000, work = uint64_t{1} << 38;
  AssemblyLimits transport{};
};
struct AdaptiveDefinitionEnvelope {
  Slot<DefinitionTag> source;
  std::array<ExactRef,6> xyz; // lo,hi per axis in envelope_values
};
struct AdaptiveCarrierEnvelope {
  Slot<CarrierTag> source;
  std::array<ExactRef,4> uv; // actual ADMITTED finite cache rectangle
  std::array<double,4> cache_domain;
};
struct AdaptiveFaceEnvelope {
  Slot<FaceTag> source;
  uint32_t carrier_envelope; // retained carrier row, no unqualified owner escape
};
struct AdaptiveAssemblyData {
  std::shared_ptr<RunResource> resource;
  SourceStore source;
  ExactTable envelope_values;
  AssemblyStore assembly;
  std::pmr::vector<AdaptiveDefinitionEnvelope> definitions;
  std::pmr::vector<AdaptiveCarrierEnvelope> carriers;
  std::pmr::vector<AdaptiveFaceEnvelope> faces;
};
using AdaptiveAssembly = std::shared_ptr<const AdaptiveAssemblyData>;
AdaptiveAssembly prepare_adaptive_assembly(
    bgfd::Budget&, const SourceStore&, const std::shared_ptr<RunResource>&,
    const RequestedQuality&, AdaptiveAssemblyKeys,
    AdaptiveAssemblyPrecision = {}, AdaptiveAssemblyLimits = {});
```

Data construction is private to the producer; returned containers are immutable
and RunResource-accounted. Rational scratch does not escape. Keys must be distinct
and source/resource/quality owners must agree before mutation. Limits use the
existing Budget scoped_work guard across the full preparation. Charge scans,
indexing and exact operations; source item count and all vector/index conversions
are checked. Failed preparation returns no partial assembly. The existing exact
transport stays untouched unless actual source review discovers a separate defect.

Root target: `brepgpu_mesher_adaptive_assembly`, sources the new cpp, links
`brepgpu_mesher_assembly` and its existing source/exact dependencies. No CUDA code:
these compile-once source bounds do not benefit from moving source control scans
or exact transform composition to P100.
The resulting EXACT caps are used by the same CPU/P100 meshing kernels.

## Deterministic construction

1. Check source/resource and arithmetic ownership, keys, positive quality and
   finite precision/limits. Enter the local work scope before scans. Use SourceData's
   admitted indices/definition CSR; never rescan all usages per path or all source
   curves per face. Memoize whole original curve hulls by typed CurveSlot, separate
   dimensions; source curves shared by several uses are bounded once.
2. Bound every original definition vertex by its literal binary64 words. Bound
   every original spatial edge over its literal source interval, including
   descending and equal endpoint intervals. Endpoint vertices remain included
   even if the edge curve disagrees within an allowed source guard. A missing
   curve/unsupported source profile is typed refusal, never dropped geometry.
3. Original analytic line bounds use exact affine endpoint evaluation. Circle and
   ellipse bounds use EACH literal radius and original frame column: per coordinate
   `O +/- (abs(r0*X)+abs(r1*Y))`, valid over any source angular interval. Rational
   curves use all positive original homogeneous control ratios. Admit spatial and
   guide requested parameters inside their original natural domain; Bezier [0,1],
   NURBS expanded knots at degree/control_count, analytic declared parameter range.
   No recursive basis expansion, cached saved spans or sampling is needed.
4. For each original face, begin an analytic UV envelope with its finite declared
   box, then union the whole ORIGINAL pcurve hulls of all its occurrences. Map kind1
   uses original spatial t; map kind2 intersects the literal edge t interval with
   every original linear map piece and unions its exact mapped endpoint images.
   Descending t and nonmonotone mapped s remain covered. Declared collapsed uses
   retain their explicit pcurve range. Kind3 is named unsupported. The first finite
   profile permits zero lift only; a nonzero lift is typed unsupported rather than
   rounded 2*pi. Existing lifted trace support can be integrated later through its
   actual enclosure service, without changing original source bytes.
5. Rational face UV is the exact original natural rectangle, NEVER carrier
   uv_bounds. Its positive control hull encloses that full rectangle. Original raw
   reconstructed guides outside natural R do not expand rational source authority;
   subsequent source-node/domain admission still decides whether a chosen locator
   exists. Authoritative out-of-domain use is not silently clipped. The producer
   records the fixed natural profile, not a theorem about guide correspondence.
6. Before freezing analytic UV profiles, expand plane UV by the exact dual projection
   of the included original physical edge/vertex boxes when rank(X,Y)=2. Expand
   cylinder V by the exact height dual, when available. These are the existing finite locator
   directions, not a general inverse solver. If an optional inverse is unproved,
   no inverse expansion is claimed; a later locator must stay in the prepared
   rectangle or refuse. This does not omit physical XYZ: vertex/edge boxes are
   already included independently.
7. Union per-face analytic profiles for any shared carrier before computing face
   spatial bounds. The same original carrier then has one immutable request
   rectangle across definitions/faces; no duplicate carrier request or hidden
   post-assembly enlargement. Round each analytic rectangle OUTWARD to finite
   binary64 endpoints once, convert those stored endpoints back to exact Rational,
   and use THAT rectangle for both B0 and cache requests. A nonfinite outward
   endpoint is typed unrepresentable/unresolved. Rational natural endpoints are
   original binary64 knots and do not expand beyond their natural domain.
8. Compute face spatial bounds with the formulas below, union all face/edge/vertex
   bounds per definition, then freeze their ExactRefs. A definition with no physical
   geometry is named unsupported rather than a made-up zero box. All definitions
   and original face rows appear exactly once. Carrier/face envelopes constrain
   preparation; they do not certify trim embedding, immersion, joint agreement or
   source nodal guards.
9. Call existing prepare_assembly synchronously with borrowed same-Budget XYZ6
   views, requested exact quality and allowance key. Retain the resulting
   AssemblyStore plus sealed original envelopes in AdaptiveAssembly. Caller imports
   caps and exact profile values into its owner; no Budget-affine source state
   survives this call.

Whole original guide control hulls can be loose. If they cause an otherwise valid
regular cache profile to be unresolved, the coherent stronger alternative is
requested exact original curve restriction (existing common source extraction)
at preparation time, followed by the SAME profile construction. Do not increase
quality thresholds or claim that a loose box proves a singular source. This first
unit records conservative preparation, not universal profile admission.

## Analytic XYZ formulas, including pre-cap source

For coordinate a, original frame columns are X,Y,Z and origin O. Let
`A=abs(X_a)+abs(Y_a)` and `V=max(abs(vlo),abs(vhi))` for the ACTUAL admitted profile.
All input binary64 words become exact dyadic Rational before arithmetic.

* Plane: exact interval `O_a + X_a*[ulo,uhi] + Y_a*[vlo,vhi]`; rank is not needed
  for this bound, although later regular patch admission still requires it.
* Cylinder: `O_a +/- (abs(r)*A + V*abs(Z_a))`.
* Cone: `O_a +/- ((abs(r)+V)*A + V*abs(Z_a))`, using only |sin|,|cos|<=1.
  This covers the WHOLE original finite radial profile including an apex crossing.
* Sphere: `O_a +/- abs(r)*(A+abs(Z_a))`, over all U,V, including ideal poles.
* Torus: `O_a +/- ((abs(R)+abs(r))*A + abs(r)*abs(Z_a))`, over all U,V.

These formulas preserve nonorthogonal/unnormalized finite source frames. They
make no local-regularity or global-injectivity claim. Exact extrema/Gram-axis
bounds could tighten them, but rowwise absolute sums avoid sqrt and overflow
and are adequate conservative transform-storage inputs.

No intermediate ideal-apex/pole extent or trig evaluation is needed here.
`assembly.cpp::transport` computes eta on ORIGINAL B0 expanded per coordinate by
requested positional epsilon. Its kappa is at least one; returned local tau is
`floor((epsilon-eta)/kappa)<=epsilon`. The accepted SAME source-map to actual-L
bound supplies, for every output point L, an original point x in retained B0 with
`||L-x||<=tau`; therefore L is inside exactly that expanded box. This is the box
needed for `(A_exact-A_stored)*L`, while `A_exact*(x-L)` is charged by kappa*tau.
The complete cap source→ideal→actual fan estimate already pays any intermediate
ideal excursion. A cap or nonidentity map must reference original source points
inside the retained finite profile; it cannot silently enlarge that profile.
No optional ideal-cap permission/flag/span or cone-angle arithmetic belongs to
this unit. This also avoids pessimizing regular nearly cylindrical cones through
a distant irrelevant mathematical apex.

## Direct RED, independent oracle and completion

One authored original-source fixture contains multiple definitions/placements and
all five analytic carrier families, positive unequal-weight rational surface/curve,
nonclamped rational natural knots, a misleading saved span/uv hint, original vertex
outside carrier control hull, shared-carrier faces with differing guide envelopes,
descending line/edge intervals and an unplaced definition. Literal nonorthogonal
frames prevent accidental orthonormal assumptions. A second small input variant
changes one weight or period/map profile to attack refusal; this is not a source
corpus expansion.

The C++ consumer writes exact envelope/cap refs and stored instance matrices in
the existing direct fixture TSV style. The Python external oracle reads the SAME
original fixture. It derives expected rational Cartesian control extrema using
Fraction, exact affine plane/line endpoints and analytic global row bounds from
the written literal source, not a production cache. Require the output to cover those whole-domain formulas and all original
vertices/spatial edges. Include an ordinary near-cylindrical cone with a distant
ideal apex: finite original V, rather than that apex, determines its returned B0.
Demand the natural knot rectangle despite deliberately false hints;
whole source inventory and shared-carrier profile union must be exact. The oracle
also checks original geometric model points on authored rational/analytic families
against returned boxes, as counterexamples in addition to the whole-domain formulas.

For the SAME nested shear/nonuniform-scale placement, compose matrices independently
with Fraction and bound actual rounded placement error at all eight expanded B0
corners. Verify returned local tau/delta satisfy world positional/node caps with
the retained source/instance paths. Reuse the existing assembly fixture's exact
path and min-allowance checks; do not create a second transport certifier.
Wrong resource/owner/key, unsupported nonzero lift/map3, nonpositive weights,
nonfinite outward endpoint and work/item caps must fail without a partial result.

Order after review: write fixture/header; observe missing prepare_adaptive_assembly
operation RED; implement the whole producer and exact transport call; run the ONE
independent authored oracle on the fleet, a bounded ASan/UBSan/leak path and strict
format/tidy; request actual-source peer review; fix concrete findings; make a
coherent checkpoint. Root registers library/fixture once files exist. No OM10 run,
GPU geometry campaign or whole-watch source admission belongs to this unit.
