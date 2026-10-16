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

Recovery/joint ASan/UBSan and actual-source tidy GREEN. Strengthened its direct
oracle with the independently authored POWER polynomial at the actual stored
UV, not merely the producer's returned residual; closed authoritative joints
pass and raw authoritative endpoint gaps are rejected. Root review correctly
requires authoritative relocation to remain a proposal until original-domain
B/T maps/protection exist; this module emits no publishable snapshot. Root also
found unselected joint requests were not admitted before owner-affine
comparisons. A foreign-owner second occurrence produced actual fleet RED
`rational arithmetic allocator mismatch`. All occurrences now pass owner,
allowance, precision and original-t/map coverage admission before endpoint
selection or arithmetic; GREEN is running.

Cross-use owner/POWER/authoritative-joint cohort strict GREEN. Root independently
accepted chart/envelope source after the cone correction. During trace RED,
reviewed root's FaceOwnerJob actual scheduling/lifetime code; found optional
empty stop token disconnected persistent arithmetic from scheduler stop.
Root removed the default and requires the submitting scheduler's token.
Scoped ACCEPT is in papers/2026-10-16-native-mesher-face-job-code-review.md.
No geometry correctness is inferred from scheduling tests. Whole trace unit
observed genuine missing geometry_correspondence.hpp RED before implementation.

Trace first strict GREEN: exact original curve-span and selected UV-knot
partition; source rational cross residual N_ce*W_trace-N_trace*W_ce (common
exact anchor, positive product weights, degree<=38); analytic coupled full
second derivative plus original endpoint departures. Rational ordinary chord
pieces retain original root/t and restricted actual affine XYZ for the existing
BoundRuntime; no source-geometry dependency on its consumer was added. Checked
line profiles separately normalize original curve and selected trace scalar
endpoint ranges; both prove strict monotonicity and bounded geometric image
departure, so nonlinear straight parameterizations need no spurious sampling.
Moved that derivative-numerator criterion from charts into one shared profile
helper rather than duplicate it.

The direct oracle includes a nonzero weighted curve against the original
polynomial carrier. Independently authored POWER numerator is
(3/2)t-(9/2)t²+4t³-t⁴; its degree6 Bernstein coefficient maximum is1/4 with
positive weight minimum1. Actual trace upper is exactly1/4; curve homogeneous
scale8 and carrier scale16 preserve it. Identical original compositions give0,
the checked geometric straight image gives0 even with nonlinear speed, and a
selected constant transverse departure1/64 is charged exactly. Current trace
values remain constructor work: B/T supports, whole chosen-domain embedding,
feature protection and quotient cells are still required before publication.

Trace ASan/UBSan and actual-source tidy GREEN; named affine endpoint tuple
removed the sole easily-swappable warning without suppression. Added the
separate face_preparation consumer module per root-reviewed bridge: persistent
owner/context, frozen SourceBoundarySnapshot, original-ordered common joints,
selected sites/use dispositions and whole trace bounds. Existing keyed
CurveBoundInput/BoundBatch numerical work gates snapshot readiness; CPU/P100 use
the same source bank. Source generation comes from GeometryStore's table,
boundary generation from forest and face generation from coordinator quality
request. Final seed table is allocated last; topology owner continued its
packet counter above this ordinal to prevent ExactRef table aliases.

Actual missing-header RED preceded implementation. First strict GREEN uses the
real original forest/canonical point producer and existing BoundRuntime, with
an authored parabolic sheet requiring nontrivial rational chord tasks. Loose
tolerance accepts complete batches to a seed; tight tolerance requests a shared
edge. Outstanding work cannot snapshot, and all sites/uses/loops/trace maps
remain complete. All-reconstructed identity domain is the initial branch;
authoritative nonidentity B/T and quotient coverage are explicitly unsupported
until their full construction lands. This transport is no mesh qualification.

Integrated preparation/metric strict Release GREEN (0.11s), actual-source tidy
GREEN. The actual FaceEngine consumes the complete source seed, continues exact
table ordinals above preparation, and produces its two original-domain cells.
Metric ordinal0 is reserved; H² uses the complete original definition bbox.

Next owned construction subunit: candidate protection uses the original shared
physical segment keys and actual binary64 endpoints. A retained balanced box
index filters conservative L1-radius bounds; the final strict diametral test
is exact dot((p-a),(p-b))<0 in definition XYZ, never a UV circle. All incident
features are considered and the smallest stable BoundaryKey requests a split.
This is the pre-insertion feature policy, not a physical embedding claim or a
replacement for authoritative B/T supports. The current producer explicitly
refuses authoritative domains until those supports are built. Missing retained
protection must refuse candidate work, never imply permission. Local draft files
preceded the fleet consumer RED; the fleet RED below establishes the missing
actual API/integration, not a claim that no local draft existed. Direct square
oracle expects center on four exact circle boundaries to pass; (.5,.25,0)
requests bottom source edge with dot=-3/16; a large off-plane point does not
inherit UV encroachment.

Candidate/protection fleet RED: Release target compilation failed at
native_mesher_face_preparation.cpp:4 with missing geometry_candidate.hpp
(exit2), before uploading the owned implementation. The previous integrated
metric/preparation target was GREEN. Independently reviewed native display
profile math; plane admission incorrectly depended on unused Z. Root replaced
it with the scaled two-axis cross guard; actual-source reread ACCEPT is saved
in papers/2026-10-16-native-display-source-profile-review.md. Root owns its CPU,
both-P100 and API-version checks; no duplicate execution was added.

Candidate/protection strict Release GREEN0.14s and ASan/UBSan/leaks GREEN3.62s.
Actual-source tidy GREEN for all three source files, with only external warnings.
The actual prepared seed retains one immutable source feature index (no seed
cycle); shared BoundaryKey selection is exact and deterministic. The direct
candidate producer refuses absent protection. Authoritative B/T supports and
analytic quotient maps remain separate finite construction branches still to
implement, not capabilities inferred from this ordinary-domain result.

Analytic quotient implementation subplan is docs/native-mesher-pole-unit.md.
This continues the accepted complete mesher geometry lane: actual whole fan
bounds, source-owned meridian cuts, ordinary remainder and native nonzero
facets. No pole constant enters exact topology as rounded rational truth.
Topology owner agreed finite snapshot patch dispatch and face-cut generation
rebuild; ordinary UV topology must not invent quotient coverage.

Plane inverse actual missing-header RED exit1. Parent accepted the selected
strong-monotonicity procedure: cache m=min(det_lower/trace_upper) during the
existing profile walk; no repeated regularity tree. Exact/enclosed conditioned
residual norm/m bounds distance to the unique inverse on convex R. First actual
driver failed to settle at normalized40-bit precision because proposal grid
had only the same40 bits. Reserving16 proposal guard bits produced strict
GREEN against the independent POWER bracket f(u)=(3u-u²)/2, context-history
invariance and outside-Q refusal. Chosen UV is a finite dyadic locator with
real enclosure; it is not claimed equal to an algebraic inverse root.

Inverse + actual plane candidate/preparation ASan/UBSan/leaks GREEN1.25s+4.95s.
Actual-source tidy found one swappable XY/UV helper signature and repeated
shared-pointer optional accesses. Replacing them with a named query tuple and
one locally admitted reserved key; no numerical rule or precision changed.
Pole first direct bound observed missing-header RED, implementation continues
under the accepted source cap/shared cut plan.

Read-only original watch BGSRC source inventory (SHA76d461...6584) has zero
explicitly declared degenerate edges; consequently no distinct-ID declared
pole endpoints. Static parser only, no CAD/evaluation/build workload. TSV is
results/native-source-inventory-2026-10-16/poles.tsv. Generic quotient work
remains selected; this source fact does not justify dropping it or guessing
unmarked poles. Root prioritizes actual authoritative B/T producer integration
next. Pole direct bound ASan/UBSan/leaks GREEN12.67s, inverse1.30s and integrated
preparation5.13s; actual-source pole tidy GREEN. These finite fan bounds are not
claimed to construct source cap/remainder inventory yet.

B/T same-producer concrete implementation proposal is
 docs/native-mesher-domain-map-unit.md. It specializes the accepted taper to
X=1,c=2m,w0=c/2: whole transformed-arc derivative cones plus fixed endpoints
prove support inclusion directly, avoiding a new gate/root procedure. Retained
finite source/fan/strip descriptors feed existing native publication and the
same per-cell allowance tau-E_domain; not a second mesher/checker. Parent
prioritized this source-authority branch while generic quotient cap selection
remains outstanding beyond its actual bounded fan primitive.

Boundary plane enclosure missing-field RED at native_mesher_recovery.cpp31/32
(exit2) preceded code. Accepted inverse is now used at selected boundary XY;
original V/Ce nodecap is rechecked at chosen UV and across common joints. Actual
plane enclosure survives source use/joint/seed transport; ordinary UV remains
a direct source parameter. Optional inverse-profile geometric failures retry
originalUV once before output, preserving cumulative work. Resource/invalid
source failures are not masked. Recovery/preparation ASan GREEN1.73s+5.90s.

B/T actual RED command on neuroses/native-mesher: clang++-18 -std=c++20 -O0
tests/native_mesher_domain_maps.cpp linked against owned ASan geometry/source/
exact libraries with -fsanitize=address,undefined. After correcting the authored
BoundaryKey enum initializer, ld reported undefined construct_domain_map and
bound_domain_displacement (exit1, session12767). The identity-source fixture
checks actual original policy/zero supports/zero allowance; nonlinear fan/strip
cases follow in the same fixture, before full native producer consumption.

### Original authoritative domain map implementation and producer integration

The actual originalUV domain constructor now builds source-point nodal fans B and monotone tapered graph strips T; mixed uses retain the named HybridPullback reference. Exact source samples remain descriptors, including A=1/3 when selected Q is binary64. The map retains immutable original geometry, complete source/face/boundary generations, finite support descriptors and an indexed support query. Physical-plane authoritative use explicitly requests the admitted originalUV fallback; no raw UV is treated as projected XY.

Actual fleet RED: `taskset -c 8-15 cmake --build build/geometry-owner-asan --target brepgpu_native_mesher_domain_maps -j4` followed by `ASAN_OPTIONS=detect_leaks=1 taskset -c 2 build/geometry-owner-asan/brepgpu_native_mesher_domain_maps build/geometry-owner-asan/chart-fixtures` failed `whole original face map must reject omitted complete source-loop inventory` (exit 1). The previous constructor admitted an empty whole-face arc set. Original source loop/coedge cursor admission now requires every use, literal spatial-t range/sense, contiguous partition, source loop incidence and mandatory original t-to-s map partitions. Exact original mapping and partition checks binary-search already monotone t rows, avoiding a known quadratic map scan. Independent review accepted the source inventory and endpoint agreement correction.

The same actual fleet command then GREEN (exit 0): identity authoritative map; nondyadic original rational A=1/3 fan; independently derived POWER displacement; original curved strip with x'=1, y'=(1-2t)/16 and slope 1/16; full-cell physical map bound exactly 1/32; mixed HybridPullback; local unaffected cell upper 0. ASan/UBSan/leak instrumentation was active. This validates this finite admitted mechanism, not universal source trim realizability.

Actual producer RED: affected native face-preparation executable, after adding actual authoritative identity/curved/mixed source cases, failed `authoritative face preparation requires its original-domain B/T map branch` (exit 1). Integration replaces that refusal only with successful named map construction and complete keyed curve-chord fold. The same seed retains domain descriptors and feature protection. Reference trace approximation includes inverse-domain movement; FaceEngine independently charges E_domain once against the actual-cell allowance. Zero-width boundary trace queries now retain derivative motion by enclosing it on its same original span rather than dropping the root.

The same-producer authoritative identity/curved/mixed cases are now ASan/UBSan/leak GREEN, including actual retained map supports and SAME FaceEngine per-cell allowance tau−1/32; focused CTest `native_mesher_face_preparation`14.59s and `native_mesher_domain_maps`1.09s. The previously unexecuted exterior-node fallback case incorrectly assumed no original chord work after switching back to originalUV; it now folds the actual keyed chord runtime before asserting readiness. No geometry policy changed for this test correction.

Small-feature actual RED used the target's exact same CMake ASan flags/static libraries through its retained link command while another newly registered fixture path was not yet uploaded. `ASAN_OPTIONS=detect_leaks=1 taskset -c 2 build/geometry-owner-asan/brepgpu_native_mesher_face_preparation build/geometry-owner-asan/chart-fixtures` failed `tiny nonconstant closed original cubic must refine topology even when chord error is below epsilon` (exit1). The source is an exact degree3 Bezier closed loop with one nondegenerate original edge, one repeated endpoint vertex, no mandatory interior knot, scale2^-10 and positional epsilon1. Preparation now checks the exact selected inventory before topology: count and coordinate alias failures request a canonical source interval only from retained nonzero whole-trace deviation. Real source forest epochs and canonical original evaluation produce a usable >=3-segment hole, retained in SAME direct CDT. The full actual face-preparation executable then ASan GREEN (exit0), including ordinary, plane, authoritative, mixed and small-hole cases. No circle-specific seeding or tolerance welding exists.

Final actual-source tidy of domain/boundary/preparation/candidate/protection found only two optional-access analyzer expressions; both now use an explicitly guarded feature chooser. The final narrow geometry_domain.cpp tidy rerun is GREEN (only non-user-header warnings suppressed by the existing checked-in profile). No suppressions were added. Current source map helpers binary-search original monotone t rows and retain original mapped-s truth; no new numerical backend was introduced. Stored source inventory TSV now also records all51742 uses reconstructed/mapkind1 and the actual analytic/NURBS face counts. This inventory affects expected watch coverage only; generic authoritative B/T delivery is retained.

### Finite analytic cap source/endpoint integration

Root accepted the concrete cap transport and identity-authoritative finite profile. Original source loops/segments stay intact; separate construction cycles may describe an empty ordinary remainder. Pole coverage remains symbolic xi/rho with actual nonzero facets, positive-chart topology roles, and publisher-only face orientation. Forest owns declared distinct-ID components and their exact original min-ID representative; no coordinate weld or geometry-owned second union-find is introduced.

Genuine fleet endpoint API RED: focused ASan face-preparation build failed missing `UseNodeRequest.endpoint` / `UseEndpointRequest` on the new actual source-bound endpoint case. Implemented a retained SourceBoundarySnapshot plus explicit original endpoint side. This distinguishes coincident spatial t endpoints and reads the forest's sorted declared aliases. Recovery still checks original V/Ce against S(storedUV); canonical representative Q supplies a proposal with additional Q-to-originalV/Ce and Q-to-carrier checks under the same source-only node cap. It never substitutes rounded forestXYZ for original truth. Ordinary nonalias requests retain the original path without extra checks.

The same focused native face-preparation executable then ASan/UBSan/leak GREEN21.65s, including endpoint-side mismatch refusal; original pole bound tests GREEN10.30s after exposing the already-computed angular/radial/corner decomposition. This is transport/recovery/primitive closure only; full cap producer/engine/publication remains in progress. New authored source generator covers north/south spheres and cone apex, same/distinct declared IDs and identity-authoritative variants. Its source pcurve interval remains independent from possibly-equal spatial edge endpoints.

### Finite analytic pole producer and source ownership

The SAME FaceEngine specialized-quotient operation reproduced genuine RED in
`context/face-poles-red.log` (`quotient use requires specialized coverage dispatch`)
using the original north analytic source and complete source dispositions. The
existing topology owner implemented that dispatch; no separate mesher was added.
A focused fixed-term RED on the fleet was additionally observed in
`context/pole-fixed-red.log`: the actual primitive fixture required the missing
`PoleFanPlan::pole_departure_upper`. This term is the ideal symbolic pole versus
actual canonical Q departure before the changing cut-corner maximum. The exact
known displaced-pole case requires 1/16. It is distinct from refinable radial and
angular terms and from changing cut corner errors.

The producer admits strict-interior source-master cuts only. A missing master
sample requests its actual meridian source interval, even when its chord error
already fits. This keeps private angular cut nodes away from the opposite
original shared boundary. Empty-remainder authored engine transport retains its
actual source-edge cut keys; continuation of those keys forwards the original
forest request without changing private angular level. Full source occurrences,
including collapsed literal pcurve ranges and declared aliases, remain retained.
The finite identity-authoritative cap path uses a named source-domain map;
nontrivial B/T-cap coupling is explicitly unsupported.

The source-determined base angular partition bounds the finite-source-to-ideal
band independently of extra angular refinement and radial cut heights. Its
upper plus the pole departure may cause typed unresolved inability to certify
within that fixed profile; this is not an asserted lower bound or diagnosis of
true source infeasibility. Full producer semantic tests are still in progress.

Actual first complete producer run failed `settled source chain has invalid
interval inventory` (`context/pole-producer-first.log`). The original forest
correctly retains TWO endpoint occurrences and ZERO splittable intervals for a
declared-degenerate edge. The preparation guard now admits exactly that case;
ordinary nodes=intervals+1 stays unchanged. The collapsed disposition retains
its named source fact and literal pcurve coverage, and coarse-domain retry
never asks the forest to split a declared collapsed edge.

The fixed-pole primitive then passed ASan/UBSan/leak execution, including exact
1/16 departure. The north full-producer ASan run was bounded at120 seconds
without a completed row. A read-only native stack at the limit was in existing
96-bit trig_range/Taylor through analytic_coupled during radial-history
preparation; initial seed construction, same engine settlement, and angular
continuation assertions had already completed. This is censored, not GREEN.
The source-backed breadth/history run is moving to strict Release, with
observable per-case/preparation stages and a selected ASan continuation run.

The finite-to-ideal band math review found a substantive error before closure:
Sv bounded only over the near-pole finite↔ideal strip did not cover the retained
whole affine-rho source correspondence. Parent selected the whole-cap derivative
correction. Genuine actual RED in `context/pole-band-red.log` uses a nonorthogonal
sphere with Z100, finite_v1.5, cut_v.25 and independent analytic xi=rho1/2 source
positions. The corrected bound uses the full convex v hull of ideal pole,
finite source boundary and both cut heights. Fixed source-band upper uses the
complete original prepared source meridian/face envelope and source base u
partition, independently of current radial cuts and extra angular levels.
Release primitive passes (`context/pole-band-release-green.log`).

Actual strict Release full producer/continuation cohort completed EXIT0, all12
families, in `context/pole-producer-release.log`; public rows are retained in
`results/native-poles-2026-10-16/producer.tsv`. Each case checks all six source
preparation stages: initial ownership requests, complete initial cap+ordinary
engine settlement, angular-only generation, and BOTH radial3/4 replacements
across the one-sided intermediate epoch. Final source-only guards additionally
check history table owner/generation, meridian branches and capacity arithmetic.

Final bounded sanitizer closure completed EXIT0:
`ASAN_OPTIONS=detect_leaks=1 taskset -c 2 timeout 180
build/geometry-owner-asan/brepgpu_native_mesher_pole_preparation
context/pole-fixtures north.bgsource`. All six actual preparation stages,
ordinary actual-cell plus host fan settlement, internal angular restart and
one-sided/two-sided radial history passed (`context/pole-final-asan-north.log`).
This is the selected full producer sanitizer path, not all12 variants under
sanitizers. Existing SAME-engine per-kind/same+distinct sanitizer cases remain
the topology owner's separate actual-cell scope.

The corrected primitive passed ASan/UBSan/leaks EXIT0 in
`context/pole-band-asan-green.log`, including nonorthogonal finite-band regression,
exact 1/16 pole departure and unequal cut heights in the existing independent
source/Phi grids. The whole original-source numerator/interval machinery is
reused; no alternate geometry evaluator was introduced.

Actual-source clang-tidy over preparation, poles, source boundary, domain map and
envelope completed with one project finding: request_parameter's optional guard
was enforced inside a called helper, so the analyzer could not establish safety
at the subsequent dereference. An explicit local guard now states that admitted
seam; no suppression or optional.value escape was added. The four other source
units were clean. Only the changed boundary unit is rechecked afterward.

Independent topology/algorithm review accepted the actual corrected whole-cap
band, source-dependent fixed upper, interval history and construction path.
The frozen cap now retains the actual explicit GeometryBoundPrecision and SAME
consumer uses it; nondefault80/112 consumer case passed on that owner's lane.
No more modeling, benchmarks or expanding sanitizer cohort are launched by this
unit after the selected closure.

Changed boundary-unit clang-tidy recheck completed EXIT0, no project findings
(`context/pole-geometry-boundary-tidy.log`). The aggregate five-file run's other
four source units were already clean. All final owned source and fixtures use
the named LLVM/100-column style, uploaded through sibling temporary files and
atomic rename. The only change after successful north sanitizer execution is
the explicit duplicate admission guard and formatting; accepted valid geometry
arithmetic did not change.

Performance follow-up stays with the coherent geometry owner rather than a
watch-specific branch: the retained native stack for the censored sanitizer run
shows `Integer::gcd_nonnegative -> Rational::add -> taylor -> trig_at_stage ->
trig_range -> enclose_original_curve_jet -> analytic_coupled -> prepare_use`.
This supports investigating a source-independent fixed-precision dyadic interval
filter for explicit-profile jets/correspondence (same source formula, deterministic
canonical bound-grid policy and exact fallback), rather than relaxing source or
mesh tolerances. It does not quantify Release whole-watch stage attribution and
no such performance change is implemented in this unit.

A final helper review caught my overly narrow one-cap guard: it rejected ordinary
source-key forwarding for an empty-cap seed, defeating the original no-op
fallthrough. Restored empty-cap source forwarding by rejecting only more than
one cap; unknown face-cut still rejects. An assertion now exercises this direct
public helper seam in the existing ordinary face-preparation fixture. Root's
coordinator guard also avoids unnecessary cap math for ordinary source events.
No cap arithmetic or completed source-case results changed. Root owns the final
affected ordinary fixture rerun; no additional geometry campaign is started.

### Adaptive cached-subdivision research checkpoint

Operator selected cached adaptive subdivision as primary mesher. Research only:
read actual pinned SMLib curve/surface caches and trim-embedding/remaining-polygon
path, current owned geometry/store/residual/interval/point interfaces, and primary
Filip1986 interpolation bounds plus Balazs2004 original source trim treatment.
Saved concrete proposal `papers/native-adaptive-subdivision-geometry.md` for
root/peer review. It selects original analytic harmonic/Gram directional bounds
rather than universally approximating analytic carriers, cached rational
Bernstein subdivision, one original S→actual bilinear B→SAME actual facet L error,
common RN/outward CPU/P100 point/bound profile with charged departures, and
compiled source harmonic cancellation. No old full preparation startup, new
dependency, product code or geometry execution. Remaining finite numerical
primitive/profile decisions are explicit before revised plan acceptance.

Reviewed integrated ADAPTIVE_MESHER_DESIGN/PLAN and saved scoped geometry ACCEPT
in papers/native-adaptive-subdivision-geometry-review.md. One wording correction
sent to root: an inactive axis is proved affine-separable approximation
independence (pure/mixed second derivatives zero), not literal S_v=0; cylinders
have nonzero linear S_v but need no V curvature splitting. The RN-first finite
runtime profile was independently read and accepted; heuristic axis comparisons
do not escalate to exact trig. Typed first geometry-unit plan follows; no code.

Saved finite first geometry/numeric unit plan docs/adaptive-geometry-unit.md for
root/runtime peer review. It selects actual header/SourceCache/POD names,
original evaluator vs prepared root identity, scaled RN sqrt/norm, requested-only
Cox/Bernstein roots, shared analytic formulas and explicit six-coefficient
B−SAME-L actual-facet procedure. No product code or execution yet. Root caller
must integrate actual cached subdivisions/packets; geometry tests are not a
standalone mesher completion claim.

Peer-reviewed actual runtime and packet unit plans; scoped ACCEPT recorded in
papers/native-adaptive-subdivision-unit-peer-review.md. Sent runtime two narrow
completion-gate clarifications: actual physical noncollapse, and exact allowance
comparison via downward conversion/precise ambiguity. Packets retains chart-only
construction scope and runtime owns actual-facet completion. No geometry run.

First adaptive geometry missing-operation RED observed on isolated new fleet:
`c++ -std=c++20 -I native context/adaptive-geometry-red.o native/mesher/source.cpp
-o context/adaptive-geometry-red`; existing actual SourceStore source is linked.
`context/adaptive-geometry-red.log` reports precisely missing prepare_source_cache,
evaluate_sample and bound_actual_facet (exit1). Headers and real fixture preceded
all bodies. Runtime and algorithm peer reviews ACCEPT; numerical owner required
exact original parameters in fallback seam, now explicit Rational spans in
adaptive_geometry.hpp rather than inferring truth from interval endpoints.

Adaptive geometry initial strict Clang18 Release GREEN, independent actual-output
reader GREEN213 observations: original rational grid/precise parameters,
source→actualB dense sample inequalities, actualL threshold equality,129
original-argument trig inputs and subnormal multiplication. Extended the SAME
reader through7 original analytic-source families: plane/cylinder/cone/sphere/
torus with nonorthogonal frames and finite1e200/1e-200 plane frames;25 original
points/family plus source→actualB inequalities all GREEN. First matching
ASan/UBSan/leak path GREEN. These are numerical-unit results, not complete
adaptive patch/wholecomponent publication. Full source→cache→triangles caller
remains runtime integration.

Root numerical source review caught radius-sum nearest rounding on torus,
invalidinterval propagation before min/max and unscaled frame Gram. Fixed all
in the common DAG/preparation. Public precise methods now admit RN environment;
profilepromotedQ uses the SAME RN midpoint of returned outward64 endpoints.
Finite analytic regularity checks reject singular input charts; rational node
bounds prove local immersion from differenced homogeneous control nets in the
SAME adaptive cache, never through another subdivision tree. Root/runtime
agreed actualphysical noncollapse remains a separate final gate and local
immersion does not claim global embedding.

Canonical original knot point ownership now precomputed once using metered
ordered endpoint/corner maps; prepared sample_owners[9] is O(1) at points.
Evaluator-only touching neighbor roots preserve lower-original-span ownership
when a requested rectangle begins on a source knot. No repeated root scan.
Tidy found only three same-type helper argument groups and unnecessary factory
value copies; changed to finite coefficient request records/UVPair and borrowed
constrefs while SourceCache retains owned leases. No suppression added.

Adaptive geometry final expanded Release and ASan/UBSan/leak source reader
GREEN214 main observations +7 analytic families; same reader additionally
checks three weighted extrusion bounds and selected original-coefficient
extreme-weight fallback. Owned-source clang-tidy GREEN, no suppressions. Saved
public-safe results/adaptive-geometry-2026-10-16/REPORT.md and TSV. No wholepatch
or performance claim follows from numerical-unit checks.

Root actual CUDA module strict NVCC needed HD infinity constructed from its IEEE
word rather than host numeric_limits constexpr; common numerical header changed
without changing the represented value. Scoped actual module review found
short rational net admission, missing node source-box containment and ineffective
all-affine failed-bound split action. Root fixed each before final integration;
review saved papers/native-adaptive-device-geometry-review.md.

Actual same independent reader on both P100s GREEN:
`python3 -B tests/native_adaptive_geometry_oracle.py --fixture
build/adaptive-geometry-release/brepgpu_native_adaptive_geometry --source
build/adaptive-geometry-release/original-geometry.bgsource --analytic-fixtures
context/adaptive-analytic-fixtures --cuda-module
build/adaptive-root-cuda/libbrepgpu_mesher_adaptive_cuda.so --device 0`
(and device1). Logs context/adaptive-geometry-p100-0.log and -1.log. Actual device
source sample/node/child restriction/facet operations consumed; scalar trig
stress and selected exact fallback remain explicitly host checks. Module
observed SHA4dc828ac7f701c84c10ce3fec024cb534e456ef87aeb38d300f8f2330d9f11d1.
Stop this bounded numerical qualification cohort; parent owns integrated cache
mesh/caller closure and coherent commits.

Postpacket transfer review found six degree-two residual hull gives twist/2 on
an ordinary half rectangle even when the selected leaf packet bound guarantees
twist/4. Recorded finite closure in docs/adaptive-geometry-unit.md before code;
root/numeric owner approved same-facet covariance bound. Added two hand-derived
same-fixture cases B=(u,v,uv), L=(u,v,v+shift), shift0/1/8, true maximum1/4+shift,
plus precise equality/insufficient allowances and independent Fraction sampling.
Old body genuine RED context/adaptive-facet-transfer-red.log: 'actual quarter-twist
transfer retains displaced-L error' (exit1).

Common HD now returns min(sixcoeff upper, ||D||/4+max nodal residual). Precise path
also compares exact squared-sum criterion r>=0,s=r²-A-B>=0,s²>=4AB; this includes
zero-nodal D²<=16r² and general shifted equality without sqrt-padding ambiguity.
Numeric owner actual-source scoped ACCEPT. Same-source Release reader GREEN216
main observations +7analytic families; affected ASan/UBSan/leaks GREEN216 and
tidy GREEN. Logs context/adaptive-facet-transfer-{green,asan,tidy}.log. No new
subdivision/checker/corpus. Root holding old module/CLI pair through its bounded
8-process cohort; new header/body are stable, pending one affected rebuilt GPU
reader pair after that cohort, not a repeat deep workload campaign.

Root completed the pinned old-revision 2D cohort and now owns the single affected
CMake/CTest closure after rebuilding module+CLI with the covariance transfer.
The registered SAME independent geometry reader now includes both P100s and
analytic fixture generation, so no duplicate device rerun from geometry lane.
Root actual-source covariance/slack review ACCEPT. Geometry code stable and
ready for coherent commit; final rebuilt module hash/device rows will be added
from that registered run rather than conflated with the earlier214 observations.
