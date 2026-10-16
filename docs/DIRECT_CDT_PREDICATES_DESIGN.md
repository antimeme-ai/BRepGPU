# Native direct CDT predicates

Status: proposed for root review before product code. This implements the
filtered-exact direction in `DIRECT_CDT_STRATEGY.md` and the predicate research
paper, independently of triangulation topology.

## Interface and ownership

`native/direct_cdt/predicates.hpp` declares namespace `bgcdt`:

```
using Slot = uint32_t;
enum class Axis { x, y };
struct PredicateStats {
  uint64_t axis_compares, orientations, incircles, line_distance_compares;
  uint64_t scalar_signs, interval_signs, exact_signs, exact_zeros;
  uint64_t floating_disabled, enclosure_values, exact_lifts;
};
class Predicates {
public:
  Predicates(bgfd::Budget&, const bgfd_input& metric_input);
  size_t size() const noexcept;
  int compare_axis(Slot a, Slot b, Axis);
  int orient(Slot a, Slot b, Slot c);
  int incircle(Slot a, Slot b, Slot c, Slot d);
  int compare_line_distance(Slot s0, Slot s1, Slot a, Slot b);
  const PredicateStats& stats() const noexcept;
};
```

Root owns source-ID mapping, original loop admission and the external ABI. The
predicate object copies/decode points from the supplied node descriptors and
common limb blob, then subtracts point zero exactly as an immutable predicate
anchor. It owns dense centered XY and cached exact weighted lifts, allocated
through the caller's existing `bgfd::Budget`. The two positive integer weights
in this operation's input are explicitly **metric weights** `wx,wy` from the
research derivation, never the face-domain bridge-distance weights. Input
descriptors must remain valid only for construction. No callbacks, JSON,
production dependencies or topology pointers enter this object. It is serial
and noncopyable; independent contexts are independent.

Every method checks slot range and rejects ghost slots. `compare_axis` performs
exact integer order. `compare_line_distance` compares the absolute exact
orientation values against the same supporting segment, which compares the
actual distances because their omitted norm is common positive. A collapsed
supporting segment is invalid input. The constructor/ABI owner never passes
symbolic hull points to numerical predicates.

The existing internal integer class needs one reviewed read-only limb-view
accessor (`std::span<const uint32_t> magnitude() const noexcept`) for exact
enclosures. It exposes no mutable storage and changes no C ABI or arithmetic.
Its budget must outlive the predicates. All vectors and integer temporaries
retain that allocator, with no hidden standard-allocator point/lift copies.

## Exact signs and filters

Orientation uses the integer 2x2 cross product. Incircle uses the cached-lift
3x3 determinant proved in the research paper; source/metric are immutable.
All signs and exact zeros are resolved by this operation, never an epsilon.

Compute integer bit lengths and positive power-of-two axis/weight exponents
from actual centered data, as in the research paper. Convert scaled integers
to enclosing binary64 endpoints by retaining at most 53 leading bits and all
discarded-bit information. Subnormal endpoints are integer multiples of the
least subnormal; a smaller positive value encloses in `[0,least_subnormal]`.
Approximation selects the endpoint toward zero, so its magnitude is at most
one and absolute conversion error is at most `2*u`, where `u=2^-53`.

The scalar fast path evaluates the translated determinant on those approximate
points/weights. Its fixed conservative thresholds are derived below; local
interval evaluation then handles scalar uncertainty. An interval excludes zero
strictly before returning a sign. Otherwise evaluate exact integers. Both
scalar and interval incircle use local translated square lifts; exact fallback
uses cached anchored lifts. This avoids losing filter precision by subtracting
large cached floating lifts.

With each input magnitude at most one and error at most `2*u`, every translated
coordinate approximation has magnitude at most two and error below `8*u`.
The following conservative error bounds include each operation's rounding and
gradual underflow. For an operation with finite exact approximate-operand result
bounded by `M`, use roundoff at most `u*M + sigma`, where `sigma=2^-1074`.

| Expression | Approximate magnitude bound | Absolute error bound |
| --- | ---: | ---: |
| Coordinate difference | 2 | `8*u` |
| Product of two differences | 4 | `40*u` |
| Difference of two such products (orientation/cross) | 8 | `128*u` |
| Weight times squared difference | 4 | `64*u` |
| Sum of two weighted squares | 8 | `160*u` |
| Lift times cross product | 64 | `2560*u` |
| Sum of three lift/cross terms | 192 | `16384*u` |

For multiplication, propagate `|a|*rb+|b|*ra+ra*rb+roundoff`; for addition,
propagate `ra+rb+roundoff`. Substitution gives the bounds above, with generous
strict slack for `u^2` terms and `sigma`. Thus orientation accepts only
`abs(value)>128*u`, and incircle only `abs(value)>16384*u`. These exact powers
of two cannot underflow as error thresholds. They are not near-zero geometry
tolerances; uncertainty goes to a tighter/exact method. A later semi-static
bound may improve hit rate under a separately reviewed derivation.

Interval arithmetic uses outward rounding of each finite endpoint operation
with the adjacent binary64 value, including at zero/subnormal results.
Multiplication checks all four endpoint products; addition/subtraction use
monotone endpoints. The fixed scaled expression bounds prevent overflow.
The implementation may use IEEE binary64 bit adjacency to avoid an external
`nextafter` call per endpoint, with zero and sign cases handled explicitly.

Filters require IEEE binary64, nearest-even rounding, no excess precision,
gradual underflow, no FTZ/DAZ, no reassociation or FMA contraction. The current
native profile already uses `-fno-fast-math -ffp-contract=off`. On the selected
x86-64 profile, check rounding mode plus MXCSR rounding/FTZ/DAZ bits for each
query. If unsupported or changed, skip floating filters and use exact signs;
do not alter the caller's floating environment. Preparing enclosures uses exact
integer bit operations/IEEE construction and therefore does not depend on the
current rounding state. Explicit FMA, checked 128-bit and additional adaptive
stages remain contenders rather than unreviewed additions to this unit.

## Budgets, counters and failures

All allocations use the shared native byte budget. Existing integers charge
limb work before multiplication; this unit additionally charges point decode,
enclosure limb scans, each API query and each filter attempt as documented work
units. Counters distinguish axis/line-distance calls from orientation/incircle,
scalar/interval/exact decisions, exact zeros and environment-disabled filters.
They describe work rather than claim equal cost per counter. Existing budget
peak/max-limb/work statistics stay owned by the caller.

Malformed integer descriptors, nonpositive metric weights, no sites, too many
slots or bad slots throw typed `bgfd::Failure` statuses for the owning ABI to
catch. Work/memory exhaustion propagates as `BGFD_RESOURCE_EXHAUSTED`, never
zero or a fabricated legal result. No partial output topology exists here.

## Direct verification and scope

Fleet-only fixtures use exact hand-derived orientation/incircle signs, both
cocircular diagonals, anisotropic sign reversal, arbitrary wide cancellation,
huge translated exact coordinates, subnormal-scale terms, signed/equal axis
orders, supporting-line distance ties and invalid inputs/budgets. A separate
Python Fraction producer can emit literal integer cases and expected signs
using the full original rational 4x4 elimination oracle; the native fixture
consumes actual predicate outputs. Both easy scalar decisions and uncertain
exact fallback must occur. Changing FP rounding and FTZ/DAZ must disable filters
while preserving signs and restore the fixture's environment afterward.

Build native fixtures under the same Clang release/debug and ASan/UBSan rigor
profiles; no SDK/CUDA link or Python sanitizer host is involved. Report filter
counts and complete batch cost for general independently authored point/metric
families, including degenerate/wide cases. Constructor-level performance and
domain qualification remain the algorithm owner's next consumer obligation;
this predicate unit cannot establish a complete CDT or watch speedup.
