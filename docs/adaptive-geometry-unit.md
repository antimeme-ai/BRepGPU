# Cached adaptive geometry and common numerical unit

2026-10-16. Implements the first regular-patch checkpoint of
ADAPTIVE_MESHER_DESIGN.md and ADAPTIVE_MESHER_PLAN.md. This unit owns original
source compilation, the CPU/P100 numerical DAG and its bounds. Runtime owns
resident caches/epochs; packets owns conforming connectivity; root owns the
complete caller and CMake. This is not a separate evaluator delivery.

## Files and public interface

Namespace `bgmesh::adaptive`. Owned files:

- `adaptive_geometry_types.h`: CUDA17 PODs below; no SourceStore, Rational,
  Budget, PMR, exceptions or device pointer stored in a record.
- `adaptive_numeric.hpp`: shared explicit-RN/padded interval operations,
  analytic source formulas, rational coefficient routines and actual-facet DAG.
- `adaptive_rn_tables.hpp`: generated binary64 word literals derived from the
  existing owned P128 anchors/Chebyshev tables and their uniform remainders.
- `source_cache.hpp/.cpp`: immutable original SourceStore lease, requested
  original/prepared roots and flat coefficient pools; C++20 host only.
- `adaptive_geometry.hpp/.cpp`: checked host preparation/dispatch and precise
  selected-root fallback, without an eager GeometryStore prerequisite.
- `tests/native_adaptive_geometry.cpp` and one independent Python source/vector
  generator/reader. Operational outputs are TSV.

The following names/fields are selected for runtime integration. They are
actual intended declarations, not an additional competing runtime model:

```cpp
// adaptive_geometry_types.h; fixed underlying integer widths
auto constexpr invalid_slot = UINT32_MAX;
struct RootSlot { uint32_t value; };       // prepared query root, cache-qualified
struct OriginalRootSlot { uint32_t value; };
struct Interval { double lower, upper; };
struct ScalarRange { uint32_t first, count; }; // flat Interval scalar pool

enum class GeometryFamily : uint32_t {
  plane, cylinder, cone, sphere, torus, rational_tensor
};
enum class ArithmeticStage : uint32_t { rn64, p128, p192, p256, precise };
enum class NumericStatus : uint32_t { ready, needs_wider, unsupported, invalid };
enum class SplitAxis : uint32_t { u, v };
struct RootDescriptor {
  GeometryFamily family;
  OriginalRootSlot original;
  uint32_t source_slot; uint64_t source_id;
  uint32_t span_u, span_v;
  uint32_t degree_u, degree_v; // rational p,q<=3 in this delivery
  ScalarRange controls;       // original homogeneous u-major/v-minor: 4 channels
  Interval original_box[4];   // u0,u1,v0,v1, exact endpoints enclosed
  Interval prepared_box[4];
  double frame[12], radii[2]; // original binary64 values, never orthogonalized
  Interval frame_a, frame_f, cone_sin, cone_cos;
  uint32_t affine_separable_axes; // bit0 U, bit1 V, proved not heuristic
};
struct SampleInput {
  OriginalRootSlot original;
  Interval uv[2];             // encloses exact ORIGINAL parameters
  ArithmeticStage stage;
};
struct SampleResult {
  NumericStatus status;
  Interval original_xyz[3];
  double actual_xyz[3];       // one deterministic agreed construction
  double departure_upper;    // original source -> actual_xyz, not source delta
};
struct NodePredicate {
  RootSlot root;
  Interval box[4];            // exact original UV box, not normalized UV
  ScalarRange controls;      // current restricted H net, empty for analytic
  double actual_corners[12];  // 00,10,01,11, xyz within corner
  double corner_departures[4];
  ArithmeticStage stage;
};
struct NodeBounds {
  NumericStatus status;
  double source_to_bilinear_upper;
  double twist_upper;
  double packet_upper;
  double score_u, score_v;    // heuristic only; stable U tie
  double numerical_upper;    // identifies need for arithmetic promotion
};
struct ActualFacetInput {
  double actual_corners[12];  // SAME actual B as NodePredicate
  Interval uv[6];             // normalized-to-leaf triangle vertices
  Interval affine_xyz[9];     // SAME actual emitted L at these virtual vertices
  double source_to_bilinear_upper;
};
struct ActualFacetBounds {
  NumericStatus status;
  double bilinear_to_facet_upper, total_upper;
};
```

`original_box`/`prepared_box` intervals are kernel enclosures, never exact point
identity. Host roots also retain exact original/prepared endpoint bit patterns
and original source/span ordinals. RootSlot is meaningful only with its retained
SourceCache lease; identical source IDs in two SourceStores are not identity.
Runtime owns NodeSlot/PointSlot/LeafKey and richer `adaptive_types.hpp` values.

```cpp
// source_cache.hpp, C++20
struct CarrierRequest {
  Slot<CarrierTag> source;
  std::array<double,4> domain; // finite positive rectangle, original UV
};
struct NumericProfile { uint32_t revision = 1; }; // rn64_anchor32_p128_v1
struct SourceCacheLimits {
  uint32_t roots = 1000000, control_scalars = 64000000;
  uint64_t work = 1000000000;
};
struct SourceCacheData;
class SourceCache {
  std::shared_ptr<const SourceCacheData> data_;
 public:
  const SourceStore& source() const noexcept;
  uint64_t generation() const noexcept;
  NumericProfile profile() const noexcept;
  std::span<const RootDescriptor> roots() const noexcept;
  std::span<const RootDescriptor> original_roots() const noexcept;
  std::span<const Interval> controls(ArithmeticStage) const;
  const SourceCacheData& view() const noexcept;
};
SourceCache prepare_source_cache(SourceStore,
  std::span<const CarrierRequest>, std::shared_ptr<RunResource>,
  NumericProfile, SourceCacheLimits, bgfd::Budget&);
```

The constructor seals only requested carrier/root closure. It retains original
source, resource-accounted flat original/prepared root records and controls;
no mutable Rational/Budget escapes. Preparation and point dispatch may import
original exact coefficients temporarily into the passed owner. `view()` exposes
const data only; caller needs are not permission to mutate source roots.

Host wrappers in `adaptive_geometry.hpp` select the exact same kernel DAG as
CPU/P100 and validate shape/owner/stage/range before indexing:
`evaluate_sample`, `bound_node`, `bound_actual_facet`.
POD HD functions have suffix `_rn64` and explicit borrowed original/control
views. Rational restriction exposes `restricted_coefficient_rn64(parent,
axis, child, output_index)` and `residual_coefficient_rn64(...)`; external
runtime workspace stages nets. No thread allocates a whole bicubic/net pair.
Precise selected-root/point fallback accepts SourceCache plus original typed
key and private Budget; a fallback cannot replace a shared Q privately.

## Closed numerical procedures

Primary profile is `rn64_anchor32_p128_v1` exactly as selected in the runtime
paper. CPU and P100 use identical RN operation order and IEEE-word outward
padding, no FMA/reassociation; old legacy interval.hpp is unchanged. Host
nearest mode/FTZ admission and FLT_EVAL_METHOD==0 are explicit. Any nonfinite/unsupported arithmetic
returns needs_wider, not a guessed finite bound.

Trig: convert owned P128 anchors, sin15/cos16 Chebyshev coefficients, pi/2 and
uniform remainders outward into generated word literals. Ties-even k locator
requires |k|<=2^30; interval reduction must lie inside [-1,1]. Ties-even j/32
anchor locator requires j in [-32,32] and entire residual within [-1/32,1/32].
Interval Horner, retained uniform error, anchor addition and k modulo4 enclose
the original function. The reduction checks carry the error; k need not be the
mathematically nearest quadrant. Wider fixed128/192/256 and precise original
fallback handle only unresolved requests. Bound/score ties never invoke exact
trig. Profile changes are sorted shared-authority promotion epochs.

Q coordinate is RN(RN(lo*0.5)+RN(hi*0.5)), computed zero canonical +0. It need
not lie in the interval; padded distances to BOTH endpoints give its departure.
Original vertices keep original XYZ. This numerical departure belongs to mesh
positional error, never to delta. Delta still compares original V/Ce against
original S at stored chosen UV, plus endpoint original V/Ce checks.

Norm upper: scale by max absolute component upper m, compute interval sum of
squares of components/m, correctly-rounded RN hardware sqrt plus one IEEE-word
outward step, then padded multiplication by m. CPU's admitted IEEE binary64
sqrt and device __dsqrt_rn obey the same operation. Zero is exact; overflow,
nonfinite or subnormal scaling that cannot retain a finite enclosure promotes.
Independent exact squared vectors attack this primitive. No rational integer
sqrt occurs per ordinary leaf. Exact squared comparisons remain available at
precise fallback.

Analytic roots compile frame Gram upper constants A>=||[X Y]||, F>=||[X Y Z]||
from scaled padded Gram row sums once. Cone sin/cos alpha compile once. Pure
second derivative norm bounds on each source interval are the formulas in
papers/native-adaptive-subdivision-geometry.md, including original nonorthogonal
frames. `e=Mu*hu^2/8+Mv*hv^2/8+max corner_departure` bounds original S to actual
bilinear B. No perleaf rational Taylor. Root summaries/local extrema can tighten
without changing original formula. Compile constants retain BASE stage too;
warm queries cannot silently start at a tighter cached stage.

Rational controls are already homogeneous original source values. Interval
Cox-to-Bernstein recurrence compiles requested natural spans once, verifies
positive W and positional C0 at shared source knots with selected precise
fallback if needed. Original full-span controls evaluate canonical point keys;
prepared intersection roots restrict only leaf construction/bounds. A query
endpoint may need a non-dyadic virtual restriction fraction despite binary64
source endpoints; outward exact-to-interval ratio handles it. Topology sites
remain exact dyadic original coordinates generated from prepared endpoints.

Restriction is coefficient/lane-strided: left coefficient i is
sum(k=0..i) binom(i,k)*2^-i*P[k]; right i is
sum(k=0..p-i) binom(p-i,k)*2^-(p-i)*P[i+k], likewise V. Parent controls and stage
are retained/replayed through the SAME path. For residual N-WB, elevate N to
(p+1,q+1) and form W times degree1,1 actual B using Bernstein product ratios;
maximum25 coefficient vectors. Wmin>0 and maximum residual coefficient norm
bound e. Anchor N'=N-W*A with deterministic binary64 A and B'=B-A to reduce
translation cancellation, enclosing every operation. Degree/shape/positive
weights are checked before workspace indexing.

Final actual-facet kernel uses leaf-normalized u,v and SAME actual affine L.
Let B=c00+u*A+v*C+uv*D. Degree2 triangular Bernstein diagonals are B(pi)-Li;
offdiagonal ij is c00+(ui+uj)A/2+(vi+vj)C/2+
(ui*vj+uj*vi)D/2-(Li+Lj)/2. Max of six coefficient norms bounds B-L over the
piece. Add e_leaf ONCE. Virtual clipping interpolates the actual L, never source
samples. Domain displacement is a separate allowance term handled by caller;
first regular rectangular checkpoint has identity domain. A conservative upper
exceeding tau is a failure to certify, never a witnessed lower/infeasibility
claim; precise fallback preserves that distinction. This exact finite
kernel is required before hanging packets can complete.

## RED, delivery and direct oracles

After peer acceptance, author native fixture against missing SourceCache and
actual geometry functions; run its missing-operation RED on the NEW fleet
checkout before implementation. Independent reader/generator derives original
POWER/rational values and exact squared/nodal residual inequalities, not native
bound-helper parity. Include plane, bilinear saddle, positive rational bubble,
C0 two-span, nonorthogonal cylinder/cone/sphere/torus, translation, finite weights,
selected actual off-source Q and a hanging-facet counterexample. Include shape,
range/foreign owner, resource/cancellation and unsupported singular rectangle
failures needed by the actual seam. Continuity guards use original coefficients.

Implement common RN primitives/trig first, source closure and original point
construction, rational/analytic leaf procedures, actual-facet procedure. Runtime
integrates these with resident roots/points and packets immediately. Root's
source→cache→complete actual triangles caller is the checkpoint; direct geometry
fixture is a numerical unit oracle, not delivered mesher by itself. Scoped
strict build, independent vector reader, host sanitizer, same CPU/P100 arithmetic
and actual-source review close this unit. No watch run or separate certification
framework. Actual cold/prepared/warm complete caller costs include preparation.

CMake requested: `brepgpu_mesher_adaptive_geometry` library over source+exact
(and resource dependencies), fixture `brepgpu_native_adaptive_geometry`, generated
RN header dependency; CUDA17 sees only adaptive_geometry_types.h,
adaptive_numeric.hpp and adaptive_rn_tables.hpp. Runtime CUDA target consumes
these inline bodies and external workspace; this library has no CUDA session.
All builds/tests use cached-adaptive-mesher/build/adaptive-geometry-{release,asan}.

## Reviewed implementation seam clarifications

The implemented factory borrows `const SourceStore&` and `const
shared_ptr<RunResource>&`, then retains owned copies inside SourceCache.
Regular means local immersion on the requested untrimmed rectangle; this first
operation does not promise global source injectivity or a closed B-rep.
Analytic planes prove rank[X,Y]=2 (unused Z may be zero), other admitted families
prove full original frame rank, nonzero relevant radii and no source pole/apex
inside the finite chart. Sphere cosine endpoint sign plus width<pi proves the
pole exclusion; ring-torus radius separation is an explicit initial profile.
Rational `NodeBounds.regularity_proved` uses the SAME restricted homogeneous
net derivative hull/cofactor; failure refines the SAME cache or reports bounded
unresolved, never positional success by itself.

Prepared descriptors retain `sample_owners[9]` for low/interior/high U×V cases.
Metered ordered source endpoint/corner indexes fill these once; evaluator-only
closed neighboring roots preserve lex-lower original ownership at query-boundary
knots. No per-point source-root scan occurs.

Exact fallback takes exact original UV/box Rational spans, never infers truth
from an interval. It reconstructs only the selected original root from source
coefficients, reuses existing owned original analytic formulas on rare fallback,
and compares exact residual norms/squared thresholds to the exact allowance.
A conservative upper exceeding tolerance is not a lower/infeasibility claim.
All constructed-stage Q use the same RN midpoint of outward64 source endpoints;
the exact original source relationship remains distinct from Q departure.

HD coefficient request records (`RestrictionCoefficient`, `TensorCoefficient`)
avoid same-type positional-argument swaps. A supplied warp-reduced maximum
residual may bypass only the serial coefficient loop of `bound_node_rn64`;
Wmin, regularity, division and the packet fold remain the identical function.

Rational axis scores use anchored homogeneous first/second control differences
and denominator first variation, with degree factors and Wmin normalization;
actual bilinear twist contributes to both active axes. They do not multiply one
scalar e by U/V widths. This cheap heuristic needs no per-control quotient/sqrt
or repeated full residual pass. A q=1 original support proves affine-separable V
when W_i0=W_i1 and (N_i1-N_i0)/W_i is the same vector across U support; exact
cross products check this once per requested original root. The p=1 dual proves
U. This is the source identity S(u,v)=C(u)+vD on that span, not literal S_v=0.
The original source coefficients supply the proof, not rounded extracted values.
