# Complete analytic face intersections

Issue: https://github.com/antimeme-ai/BRepGPU/issues/20. This is the intersection
beachhead of the broad pass. Construction is CPU work. CUDA evaluates supplied
curve samples on both source surfaces using the existing position ABI. Neither
operation establishes general surface intersection or a GPU intersection solver.

## Inputs and completeness

`FaceRef(snapshot, face_id)` retains an owned immutable factory BodySnapshot and
a valid face identity. `intersect_faces(left, right)` accepts exactly one planar
face and one cylindrical wall, in either order. Factory Frames translate and
apply positive power-of-two scale; they do not rotate. Supported plane domains
are the existing closed rectangle, disk and rectangle-minus-open-disk. The
wall is its closed finite-height cylindrical surface with four rational charts.
Coordinates are interpreted as exact binary64 rationals, including subnormals.
Supported Python execution uses round-to-nearest/ties-even and gradual
underflow, as in the qualified CPython/native environment. Integer root/sign/
quantization work does not establish arbitrary FTZ/DAZ environment support;
Python float-to-Fraction decoding and float equality retain this execution
precondition. A future environment-independent decoder is separate work.

The body-facing convenience operation is
`snapshot.intersect_faces(face_id, other_snapshot, other_face_id)`. It constructs
the two FaceRefs and delegates to the same complete operation. Actual callers
can use existing snapshots without rebuilding their source selection. Its
first direct red calls the missing method on healthy existing body/evaluator
objects, then expects the complete known circle; no placeholder implementation
or manufactured import failure is needed.

The operation returns every connected component of the two CLOSED face domains:
circles, clipped arcs, generator segments and isolated points. A tangent
generator remains a segment if its finite domain has positive height. An
isolated trim contact is a point, not a zero-length edge. Boundary-coincident
curves are included. Plane-plane, wall-wall, tilted axes, arbitrary patches and
general coincident surfaces raise UnsupportedIntersection before any partial
result is returned. Empty means the supported pair has no intersection.

No broad-phase prefix can establish complete scene intersections. The bare
face operation requires no broad phase. A caller using pair pages must consume
the full advertised Cartesian domain and deduplicate source face pairs before
construction, including all chart occurrences. Unsupported pairs stay explicit.

## Exact scalar and topology

Own a small quadratic scalar, `a + b*sqrt(d)`, where a/b are Fractions and d is a
nonnegative integer. Normalize rationals and perfect-square radicands. Derive
roots from primitive integer quadratic coefficients; linear and double roots
are separate cases. No tolerance decides roots, emptiness or connectivity.

Comparison does not need factorization or a bounded refinement loop. To sign
`K + u*sqrt(D) + v*sqrt(F)`, first sign z=`K+u*sqrt(D)` by squared magnitudes
when its two terms oppose. If z and v oppose, sign
`z*z-v*v*F = K*K+u*u*D-v*v*F + 2*K*u*sqrt(D)` to choose the larger magnitude;
equality means zero. This also compares algebraic values represented in
different radical fields. Same-field arithmetic and inversion are exact;
unneeded mixed-field arithmetic is rejected. Do not hash noncanonical scalars.

Binary64 conversion uses exact comparisons against ordered finite float values,
then rounds at the exact midpoint with ties to even. A bracket exposes the two
neighboring floats when needed. It never computes a cancellation-prone float
quadratic discriminant. Construction has no precision cap; allocation failure
fails the operation. Sampling may have its own explicit resource/precision cap.

The immutable result retains both FaceRefs, local component/piece/event IDs,
exact geometry, and coupled spatial/UV maps. IDs live in this result; they are
not persistent names or Boolean edges. Components carry closed/open kind and
carrier tangency. Events carry endpoints, named chart joins, source trim/domain
constraints, and incidences. Boundary-coincident constraints are recorded on
pieces. Canonical CCW circle/increasing-height line direction is not a coedge
orientation. Boolean arrangements and topology mutation are later work.

Concrete records: result.left/right FaceRef; components(id, kind, closed,
tangent, piece_ids); pieces(id, component_id, span_id, kind, left_slot, right_slot,
lo, hi, spatial_map, left_map, right_map); events(id, constraints, joins,
incidences). Slots refer to ORIGINAL snapshot charts. ExactMap exposes ascending
power numerator tuples and its own common positive denominator tuple, with
Scalar coefficients, and evaluates exact values. Spatial/UV dimensions are
3/2/2; these three maps need not share a denominator. Piece.position/left_uv/right_uv
evaluate these maps on its common parameter domain. Point pieces have lo==hi
and a single 'point' incidence, never a self-loop or fabricated edge. Wall
quadrant comes from its original chart; atlas lift is quadrant+native u.
IntersectionSet construction is factory-only. Explicit APIs name geometry
and representational subdivisions; they do not imply persistent B-rep topology.

span_id groups overlapping chart occurrences of one geometric span. Physical
graph degree and component closedness use each grouped span once, not each raw
piece occurrence. A seam/cardinal generator's two occurrences share span_id and
the same endpoint events; they remain one open segment, never a two-edge cycle.
Isolated point occurrences share a point span and contribute no graph edge.
Ordinary horizontal interval pieces each have one span; joins connect spans.
Event incidences remain occurrence-specific so both source lifts survive.

## Horizontal planes

For plane z=h and wall center(cx,cy), radius R and height[z0,z1], h outside the
closed height is empty. Otherwise use the exact circle, chart q=0..3:

    c(s) = ((1-s*s)/(1+s*s), 2*s/(1+s*s)), 0 <= s <= 1
    C(s) = (cx,cy,h) + (R*rotate_q(c(s)), 0)

The plane UV map is its exact affine inverse applied to C. Wall UV is
`(s, (h-z0)/(z1-z0))`; its global atlas coordinate is `q+s`.

Rectangle inequalities become degree-at-most-two numerator polynomials after
multiplication by the positive denominator. Disk/hole inequalities simplify
using c.x²+c.y²=1:

    delta.x² + delta.y² + R² - r² + 2*R*delta dot rotate_q(c(s))

with delta the wall center minus the trim center. Disk accepts <=0, hole accepts
>=0, outer rectangle accepts its four closed half spaces. An identically-zero
constraint admits the whole curve on that boundary and is recorded as such.

Enumerate all roots in [0,1], add 0/1, exactly sort/deduplicate. Check each event
itself and one exact rational witness in every root-free open interval. Signs
are constant on these intervals because all roots were enumerated. Keep pieces
split at every admitted constraint root, including a trim tangency inside an
otherwise connected arc. Components join these pieces without losing the
named degree-two trim event. Retain admitted events with no admitted
neighbor as isolated points. This is polynomial cell decomposition, not a
sampling heuristic. Coincident roots from different constraints share an event.

Connect pieces at known atlas joins. a=4 and a=0 are one physical seam event
with distinct UV occurrences. A full circle is one closed component with four
pieces absent additional trim events, and more when those events require
subdivision. An arc crossing the seam is one ordered component, not two arcs. Trim
events and chart joins are exact symbolic incidences, never proximity welding.

## Vertical planes

For x=p (or y=p), use exact D=R²-(p-center)². D<0 is empty, D=0 is one tangent
generator, D>0 gives two generators. Check each constant coordinate against
the planar rectangle's closed span; clip its height against both domains.
An empty height interval is rejected; a singleton is an isolated point.

Locate each generator's exact wall parameter by roots of the same rational
circle equations. Cardinal/seam generators may have two chart occurrences;
those occurrences represent one physical component. Each occurrence has its
own coupled spatial and two UV maps, with common height parameter. Segment
endpoints are shared exact events across occurrences. Source wall orientation
(inner plate wall versus outer cylinder wall) remains source metadata.

Boundary constraints include both source domains: a horizontal circle at wall
z0/z1 is on that entire axial boundary; a generator on a planar lateral bound
is boundary-coincident; clipped height endpoints name every active planar/wall
constraint. Chart joins alone are representation subdivisions, not trim edges.

## Two-sided consumer

`IntersectionSampler(engine, result)` borrows an Engine and owns a PatchSet of
both faces' charts. An immutable prepared request is bound to this exact result.
Each candidate names piece and exact common parameter; both UV maps are
evaluated on CPU. The consumer submits one native order-zero batch containing
two rows per candidate, then verifies actual returned patch slots, UV values,
statuses and full source descriptors. It converts each jet into world space
and scatters both source positions together with the exact curve reference.

Exact endpoints may be irrational. Keep their algebraic values in the request;
rounded native UVs are approximations and need not classify as trim BOUNDARY.
Never infer exact graph connectivity or boundary membership from rounded jets.
Interior samples use rational witnesses inside an inward dyadic interval, so
evaluating maps needs at most one radical field. Failure to find an interior
window within the sampling precision cap raises SamplingPrecisionError and
does not damage the already complete exact result. Point occurrences contribute
one output candidate and two native rows. Resolution is 2..4097 per non-point
piece; the total candidate cap is 2^19. Inward-window refinement is capped at
4096 binary subdivisions; exceeding it raises SamplingPrecisionError before
native work. Prepared candidates are only exact endpoints and rational interior
values, avoiding mixed radical fields. Generic map arithmetic may reject a
mixed-field parameter; the sampler does not generate one. Requests are
factory-only and copied into recursively immutable owned values.

Componentwise world error target is 2^-36 against the exact curve position and
between the two source positions. Existing local order-zero target is 2^-40;
maximum Frame scale is four. The bound includes UV rounding, affine conversion
and both-side evaluation; the independent oracle must attack this derivation,
especially near tangency and tiny intervals. Circle XY component derivatives
are <=2R<=2, planar world spans <=6 and height spans <=4. Nearest UV error on
[0,1] is <=2^-54; affine conversion slack <=2^-48 and two scaled local position
budgets total 2^-37, leaving room under 2^-36. The sampler directly checks finite
actual outputs, each position against the exact curve and their pairwise
component differences before publication. SamplingArithmeticError fails the
whole batch if a grade exceeds the bound; empirical evaluator qualification
does not substitute for these direct model checks. Construction uses no bound.

All child resources close on BaseException, preserving an active primary error
and attaching cleanup failures. A cleanup failure on success fails the whole
operation. Cross-result requests and source mismatches fail before publication;
no partial sample batch is returned. Immutable geometry remains usable after
Engine closure. Empty intersections submit no native rows.

## Independent truth and falsification

Bind known factory controls and frames independently of construction helpers.
Use literal canonical configurations with complete expected components, exact
endpoints and trim constraints. Verify coupled spatial/left/right maps over
whole polynomial pieces, original carrier/trim predicates, exact seam cycles
and graph degree. Do not settle for sampled residuals or CPU/CUDA parity.

Cases include full circle, four clipped arcs, seam-crossing arc, disk lens,
plate hole exclusion, trim tangency point, whole trim boundary, two generators,
tangent generator, empty cases, clipped-height point and both operand orders.
Exercise exact discriminants one subnormal away from tangent, scales/origins,
simultaneous roots and differently represented radicals. Unsupported inputs
must fail explicitly. Sampling independently checks original controls, source
provenance, approximation bounds and failure/cleanup behavior.

First direct red runs on the fleet against a healthy body/evaluator setup and
the missing intersection operation. After implementation use CPU1/CPU24 and
both P100s, Compute Sanitizer on GPU paths, and focused production mutants
attacking omitted components, holes, seam incidence and UV/source coupling.
Unchanged native binaries need no new native qualification campaign.

## Caller and measurement

A real caller reads complete pair pages, constructs a supported planar/wall
face pair, consumes both surface representations, exports TSV and an owned SVG
with graph counts and explicit scope. Benchmarks run construction and sampling
separately, CPU1/CPU24/P1000/P1001, with repeated qualified reused calls and a
cold full path. No overlapping owned fleet jobs during performance trials.

Report CPU construction and request preparation, engine/geometry setup, gather,
query upload, native evaluation host span/event scope, download, world/scatter,
validation, cleanup and whole-path time. CUDA query rows cost 20 bytes upload
and 28 bytes order-zero jet/status download: 40/56 bytes per two-sided candidate.
Geometry upload is counted separately; host metadata is not device transfer.
No curve construction, scene intersection or end-to-end CAD speedup is inferred
from a native evaluator win. Measurements may show CPU construction dominates.

## Studies and return pass

Read papers/2026-10-14-intersection-{topology,oracle,runtime}-study.md and their
primary-source catalogs. OCCT analytic plane/quadric cases and three-curve
representations, SISL intersection objects, Dokken component/parameter topology
and exact/validated intersection literature informed this scope; no reference
implementation is adopted as a dependency.

Round two: general plane orientations/ellipses, wall-wall and spline SSI,
validated continuation/completeness, approximation error budgets, Boolean
arrangements/coedges, persistent names, imported models and GPU construction.
GPU session/residency/buffer reuse and integration polish stay AFTER broad-pass
big rocks; record opportunities in docs/RETURN_PASS.md and the journal.
