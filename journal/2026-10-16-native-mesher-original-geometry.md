# Native original rational geometry — 2026-10-16

Root accepted the whole implementation plan and assigned the original geometry
owner unit. First conceptual subunit is original-coefficient Cox-to-Bernstein
extraction, exact homogeneous evaluation and de Casteljau restriction. Read the
actual values/source/exact headers. Coordinate Rational ownership directly with
the numerical owner: one bgfd::Budget, owner-local PMR nets, explicit later
ExactWords freeze; no second bigint or shared captured Budget.

Authored tests/native_mesher_geometry.cpp before geometry implementation. Its
literal model/spec answers cover weighted quadratic, unclamped natural ranges,
C0 knots and matching/differing rational one-sided limits, degree32 t^32,
constant curves, rational quarter circle, tensor weighted bilinear/unclamped/
cubic mixed monomials, exact restrictions and invalid original inputs.

Genuine RED on the isolated fleet checkout:

    scp tests/native_mesher_geometry.cpp neuroses:BRepGPU/native-mesher/tests/native_mesher_geometry.cpp
    ssh neuroses 'cd ~/BRepGPU/native-mesher && taskset -c 2 /usr/bin/clang++-18 -std=c++20 -fsyntax-only tests/native_mesher_geometry.cpp'

Exit1: tests/native_mesher_geometry.cpp:1:10 fatal error,
`../native/mesher/geometry.hpp` file not found. The required native API does not
exist. This is the intended missing-operation RED; no local native execution
occurred. Implementing the actual unit next; root owns CMake and commits.

Implemented actual owner-local PMR geometry.hpp/.cpp using the numerical owner's
Rational and bgfd Budget. First strict Clang18 debug build/run on fleet CPU2:

    /usr/bin/clang++-18 -std=c++20 -O0 -g -Wall -Wextra -Wpedantic -Werror -Wconversion -Wsign-conversion -Wshadow -fno-fast-math -ffp-contract=off tests/native_mesher_geometry.cpp native/mesher/geometry.cpp native/mesher/exact.cpp -o build/native-mesher-geometry/check
    taskset -c 2 build/native-mesher-geometry/check

GREEN: original rational geometry, work3211313, peak_bytes228432. This is the
extraction/evaluation/restriction prerequisite, not a completed mesher.

The real numerical CPU/P100 task consumer needs tensor-to-triangle restriction.
Added independent literal monomial coefficient tests before implementing it.
Second genuine RED on CPU2 with the same syntax command: exit1, no member
restrict_to_triangle in namespace bgmesh (two actual call sites). Implemented
the finite affine Bernstein composition/convolution, with original UV corner
order mapped to exponent order(d-i-j,i,j). Rebuilt strict debug and ran actual
fixture: GREEN work3266004, peak_bytes228432. Weighted bilinear coefficients and
cubic u^3v^3 coefficient1/20, elevated u^3/v^3 and weight partition unity match
their hand-derived answers. Numerical owner notified actual API is available.

Continuing original-source GeometryStore and whole mesher integration. No
shared root retains this temporary Rational/Budget; root's actual immutable
ExactTable PMR limb pool will store compiled coefficients.

Review found that publicly constructible malformed local nets could reach
restriction indexing before shape/owner checks. The curve with one removed
homogeneous control produced an actual AddressSanitizer heap-buffer-overflow
RED. Shared narrow admission now guards evaluation and every restriction entry;
the same source fixture passes ASan/UBSan (work3266239, peak228432). Copy
construction preserves the PMR owner. Unused assignment operators are deleted:
partial assignment must not mutate degree/domain before a foreign-owner
Rational assignment rejects.

Added an independently written direct original homogeneous de Boor model for
nonuniform/repeated-knot weighted cubic curves and tensor degree(2,3) surfaces.
Exact extracted evaluations match that model at five samples per curve span and
nine surface span samples. Strict debug GREEN: work3899079, peak228432. These
counts describe this fixture's primitive arithmetic Budget, not whole mesher
time, memory or source compilation.

GeometryStore missing-header RED preceded its implementation. It now retains
immutable SourceStore and one sealed exact table, with flat PMR root records,
contiguous local coefficient indices and ordered tensor-axis queries. The real
source writer/reader fixture compiles inside a temporary Budget, drops that
Budget and the caller's source handle, then imports/evaluates roots in a fresh
owner. It also verifies analytic formula inventory, original natural domains
despite deliberately wrong saved controls/tiny adapter hints, two-by-two span
lookup and closed knot contacts. No SourceStore constructor backdoor was added.
All retained RunResource allocations release with the Store.

Root's full-coverage query review identified silent clipping outside the natural
rectangle. Added the actual partial-outside query regression first: strict fleet
debug exit1, `FAIL partial outside source-cell query must not silently clip
coverage`. The interval index now rejects partial/full outside source-cell
queries with typed unsupported before enumeration. Inside closed knot boxes
still return all four contacting roots. Full fixture ASan/UBSan GREEN on the
fleet CPU2, work3899079, peak228432; no local geometry execution occurred.

Scoped independent reviews of root ExactTable pool ownership and the numerical
owner's residual/filter core found no defect. Borrowed exact views require a
retained Store/Table lease. The common exact anchor N0/W0 preserves residual
identity: (N-W*A)-W*(L-A)=N-W*L. Source coefficients and canonical actual XYZ are
unchanged; interval root packing now uses this identity to avoid origin-sized
cancellation. Original tensor blossom partition weights, positive minimum-W
intervals and squared residual decisions match the accepted geometry meaning.

Next original-geometry subunit supplies the shared boundary owner with original
analytic positions and canonical points, then full-interval derivative/chord
tasks. Add `geometry_analytic.hpp/.cpp` to the same geometry owner. A local
GeometryContext retains its one Budget, deterministic precision policy and
Machin-pi enclosure; source/root storage remains immutable. Rational enclosure
endpoints use the existing exact arithmetic, with integer-only outward dyadic
compression to bound denominator growth. Circle/ellipse and the five source
analytic carrier formulas retain arbitrary frames and dimension2 XY semantics.
Rational line/plane/Bezier special values remain exact. Canonical points require
identical nearest-even words at both enclosure ends, otherwise increase the
fixed precision stage or return bounded unresolved. No stdlib trigonometric
result defines source truth. Root and numerical owner received the concrete
API; numerical owner accepted this arithmetic seam. Test independent direct
alternating-series brackets at rational arguments across quadrants, analytic
formula identities, original rational dispatch and rounding/owner limits before
implementation. This is prerequisite geometry for whole mesher boundary/cell
tasks, not a separate primitive delivery.

Analytic actual missing-header RED: fleet syntax check exited1 at
`geometry_analytic.hpp` absent. Implemented the original finite formulas,
Machin/Taylor rational enclosures, integer quadrant selection and exact endpoint
IEEE-cell isolation. Tests use an independent 128-term direct rational series
at original arguments0,1/2,1,2,-2,7/2,-7/2,17 (no pi or range reduction), literal
nonorthogonal source-frame identities, dimension2, exact source-line ties,
negative-underflow signed zero, real Store rational/analytic dispatch and typed
owner/precision failures. Focused full ASan/UBSan GREEN: work16469872,
peak17568 (fixture Budget, including independent reference arithmetic). Numerical
owner's scoped core review ACCEPT is in
`papers/2026-10-16-native-mesher-analytic-numerics-review.md`. This does not
establish bulk source-node throughput; a generic curve-family measurement is
required before the watch consumer.

Root accepted the interval-jet/chord interface. Same geometry-owner enclosure
arithmetic feeds full original first/second derivative bounds. Rational jets
restrict one true original root and cancel a common exact spatial anchor before
quotient differentiation; no C0 knot is silently treated C2. Analytic full
trigonometric ranges use the derivative<=1 enclosure about their enclosed
midpoint, clipped to[-1,1], so extrema are included and the range contracts with
subdivision. Rational chords retain the runtime's actual residual task, rather
than introducing a geometry-to-runtime dependency cycle. Analytic chord
interpolation will add actual endpoint departure once. The numerical owner owns
the dyadic sqrt primitive needed for scalar Euclidean bounds.

Authored literal original curve/tensor derivative and translated-rational
regressions before implementing jets. Genuine RED: fleet syntax check exit1,
`geometry_jets.hpp` missing. New helper header only exposes the same exact
enclosure operations between geometry-owner translation units; it does not
define another source/evaluator or numerical backend.

Jets strict debug GREEN368123work/21528peak before adding the weighted quotient
model; full weighted tensor/mixed-partial grid ASan/UBSan GREEN412447work/21528peak.
Point precision cleanup then made the total working cap explicit, reserved the
observed range-reduction guard on the last stage, and keyed stable PMR pi-cache
entries by exact requested precision. Analytic jets take an explicit profile
precision and therefore do not change when a previous canonical point needed
more precision. Cache-history/point-stage invariance regression passes. The
numerical owner reread these changes and added a scoped ACCEPT addendum.

Analytic actual-chord fixtures first produced a genuine missing-symbol link RED
for `analytic_curve_chord_upper`. Implemented M2*width²/8 plus the maximum source
endpoint-to-actual-word departure once. M2 takes the tighter local derivative
hull or largest eigenvalue of the exact two-column frame Gram matrix. This
supports nonorthogonal frames without an axis-norm sum. The numerical owner's
independently tested Rational dyadic sqrt supplies scalar upper endpoints;
precision grids are explicit request values. Literal radius2 curvature/chord,
known1/8 endpoint perturbation counted once, and nonorthogonal ellipse Gram
cases pass strict debug (3295616work/21528peak). Root independently reads jets
derivative formulas; final scoped review/ASan remains pending for the chord add.

Root's integrated original whole-source preparation measured source read0.213s
and exact compilation48.136s, with80339 curve roots and2839 surface roots.
These are source preparation only. Avoiding unused guide extraction and
parallelizing independent original owners remain whole-Run performance
questions; no tuning branch or watch meshing run follows from these numbers.

Core Clang-tidy reported seven actual signature issues. Replaced swappable
dimension/degree, control count/stride, span begin/end and binomial arguments
with named shapes/domain tuples; compile_geometry now borrows the shared
resource handle by const reference. No warning suppression or math change.
The numerical owner authorized updating its single constructor fixture caller.
Core original fixture strict debug GREEN3899255work/228432peak after these
explicit tuple copies. Root owns the narrow tidy rerun and coherent commit.

Root accepted chosen-chart/envelope ownership: geometry prepares finite original
face envelopes, original occurrence maps/lifts and the optional checked plane
profile. Default UV remains admitted when the optional profile fails; source and
resource errors do not become a fallback. No root coordinator infers the
carrier's adapter hint as a natural domain. WorkingChart/FaceEnvelope retain
GeometryStore and exact owner-local values; later cross-owner serialization
freezes them through the shared exact table. Native recovered node/use/trace
records follow this chart choice, before defining reconstructed polygons.

Authored actual writer fixtures for a nonlinear regular planar square and the
same natural-boundary square with an interior height bubble. The latter prevents
boundary-only planarity from establishing a full original-image height bound.
Genuine fleet syntax RED: geometry_charts.hpp absent. This subunit will implement
source envelopes and finite plane-profile procedures, not another evaluator.

Performed root-requested narrow independent scheduler source review during the
geometry build wait. Queue capacity/owner exclusion, cancellation publication,
partial construction/destruction joins and retained reservations are coherent;
ACCEPT in `papers/2026-10-16-native-mesher-scheduler-code-review.md`. Concrete
steps must respect temporary JobContext ownership and the coordinator must sum
per-step work. No additional execution or certification campaign was added.

Actual-source analytic/jets Clang-tidy initially found swappable atan/norm/tensor
parameters and unchecked optional access. Named request/degree tuples now carry
these arguments; Taylor obtains stored references directly from successful
emplace calls. A narrow norm-span admission closes the analyzer's potential
coordinate-index escape. No warning suppression was added. Both actual source
translation units now pass the repository's native-cpu Clang-tidy profile and
the named 100-column format profile. Final pre-cleanup jets/chord ASan/UBSan
GREEN was3295656 work,21528 peak; only signatures/storage references and the
internal span guard changed for these code checks. Root owns integrated builds
and the reviewed geometry checkpoint.

The common metric consumer needs first/second source jets at a single original
parameter. Authored an actual-source weighted-quotient derivative fixture first;
genuine fleet RED was missing `enclose_original_surface_point_jet` (16 cascading
compile errors). Added that narrow dispatch with stable lower original-span
ties. Original rational derivative nets use original span widths and the same
homogeneous de Casteljau engine, exposed through a shape/owner/unit-domain
checked internal polynomial view. Analytic point jets use the same explicit
request precision and finite source formulas. No finite differences, artificial
zero-width restriction or separate evaluator was introduced. Focused strict
debug GREEN on exact position/first/second/mixed rational answers and a source
knot tie; topology owner can consume the actual API. Focused sanitizers and
actual-source tidy are running before this continuation's review.

Root explicitly requested a bounded bulk canonical-point cost observation before
the watch consumer. Added one native client: 1024 exact dyadic parameters i/1024
per original line, unit circle and positive rational curve, one reused context
per family, source load/compile measured separately. It records wall time,
exact work, private Budget peak and a checksum of actual output words. Line and
rational use the real typed Store dispatch; circle uses the same original
analytic row evaluator directly (no separate source-row lookup). Source fixture
is authored and already covered by exact geometry tests; this client does not
claim full mesh qualification or a CPU/reference speedup. Bound the whole run
to60 seconds, retain any censored outcome; no point rewrite follows silently.

Bulk point observation completed uncensored on CPU2: line1024=6.773 ms,
193155 exact work/552B peak; circle1024=4307.041 ms,216541769 work/1820B;
positive rational1024=33.560 ms,1007524 work/2492B. Source preparation separately
1.517 ms. Results and limitations are in results/native-point-costs-2026-10-16.
Circle canonical evaluation is material at4.206 ms/node. Record an owned fixed
precision/double-double enclosure filter with exact fallback as a performance
candidate; keep nearest-original semantics and continue whole geometry
integration before attempting a point rewrite.

Original image-range, source-curve root-index and explicit norm helpers are
stable for actual-cell consumers; focused strict Debug GREEN includes a C0
whole-curve image, closed knot intervals, outside-domain rejection and exact
3-4-5 norm. The new envelope fixture observed genuine missing-link RED for
prepare_face_envelope/prepare_definition_envelope before implementation.
First strict Debug GREEN now covers true natural bounds despite tiny hints,
whole original interior bubble, piecewise parameter-map interior extrema and
unsupported continuation/period metadata. ASan/UBSan and actual-source tidy
follow before claiming the source-envelope continuation complete.

Envelope ASan/UBSan GREEN and actual-source Clang-tidy GREEN. The finite plane
chart uses the original natural rectangle, exact corner-plane dual projection,
fixed center inverse-Jacobian preconditioner and positive symmetric-Jacobian
interval tests on bounded original-root subdivisions. Each natural boundary
must lie exactly on its projected chord and have nonnegative/nonzero rational
derivative-numerator Bernstein coefficients. Whole positive-weight Cartesian
control hulls bound perpendicular height, including interior controls. No
inverse UV samples establish image coverage. First strict Debug GREEN accepts
the nonlinear warped planar square and rejects its interior bubble at the
requested positional allowance. Added two further direct geometric refusals:
folded planar projection and curved projected natural boundary. Focused
sanitizers/tidy are running on this completed cohort. Optional geometric profile
failure falls back to UV; no resource or source error is swallowed.

All four plane chart cases pass ASan/UBSan and actual-source tidy. Root review
found a declared cone apex could be silently omitted when fixed-precision sine
straddles zero. Added source fixtures for angle0 and2^-200 and observed genuine
old-code failure. Fixed angle0 to typed unsupported and uncertain nonzero sine
to unresolved; the ideal cap is never skipped. ASan/UBSan GREEN follows.
Cosine inverse expansion is preparation-only when invertibility is proved;
later recovery cannot escape the prepared envelope without expansion/refusal.

Boundary node recovery missing-header RED was observed on fleet before adding
the module. First strict Debug GREEN covers nonlinear source parameterization,
physical-plane XY choice before UV interpolation, and a deliberately wrong
mapped reconstructed guide used only as Newton locator seed. Target is original
V/exact original Ce, never rounded forest XYZ. Approximate Newton proposals are
read again through original exact/enclosed surface jets; source-only node cap
and endpoint V-to-Ce checks admit them, with branch/envelope/resource limits.
Added common face-vertex joint selection: choose one stored UV then verify all
incident uses at exactly that UV; authoritative raw endpoint identities remain
distinct from chosen dyadic sites. The topology-owned frozen source boundary
snapshot supplies the same physical identities/XYZ and exact master t.
