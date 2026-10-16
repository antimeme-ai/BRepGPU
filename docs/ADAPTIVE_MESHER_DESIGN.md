# Cached adaptive B-rep mesher

Status: operator-selected architecture; constructive design accepted in scoped
algorithm, geometry and runtime review after the resolutions below. Product
units require their finite interface/RED/review procedures. This supersedes the
direct-refinement framework in NATIVE_MESHER_DESIGN.md. The first regular
untrimmed surface operation is implemented and measured in
[the patch report](../results/adaptive-patch-2026-10-16/REPORT.md); the complete
curve/trim/component mesher remains under implementation.
The dependency draft is [ADAPTIVE_MESHER_PLAN.md](ADAPTIVE_MESHER_PLAN.md).

## Purpose and selected architecture

Build an owned native counterpart to the CPU USD/BRep tessellation operation,
with one cached adaptive subdivision algorithm executed on parallel CPU or
CPU+P100. Study NVIDIA's implementation and the wider literature for mechanisms
and assumptions; do not inherit its pointer layout, stopping rules, splitting
policy, approximations or numerical choices without evaluating alternatives.

The complete operation is:

    original source and assembly
      -> source-root geometry and reusable curve/surface caches
      -> shared source-edge sampling and chosen trace/domain maps
      -> adaptive surface leaves and face trim classification
      -> regular interior packets plus connected boundary regions
      -> conforming actual facets with original-source error bounds
      -> physical contacts, complete semantic mesh and source display.

The adaptive cache owns approximation and refinement. Direct CDT is available
for boundary-region connectivity; it does not drive whole-face LEPP refinement.
An anisotropic cache or alternate chart may reduce unnecessary subdivision.
Rotated ridges, distorted parameterizations, thin trims and many holes remain
independent measured workloads, not arguments for silently restoring the old
constructor. No watch names, source IDs, dimensions or sample counts select an
algorithm, tolerance or resource limit.

The first untrimmed patch checkpoint admits local source immersion, separately
from actual-facet noncollapse. Analytic roots check their finite frame/radius/
pole conditions; rational nodes use conservative derivative/cofactor bounds on
the same restricted controls. Ambiguity drives that cache's refinement or a
typed finite failure. This is not a global injectivity or embedded-solid proof.
Shared trims, finite quotients and complete component publication follow their
explicit later checkpoints.

## Source and error meaning

Original binary64 source coefficients, frames, topology, pcurves and assembly
placements remain authoritative and inspectable. Source extraction stays the
isolated existing STEP adapter. Production geometry and scheduling are native.
Reconstructed pcurves guide branch selection; authoritative ones retain their
full declared domain meaning. Preserve original vertices, edge/use occurrences,
holes, seams, declared quotients, placements and material labels.

The positional allowance applies to the whole actual emitted piecewise-affine
surface after the original assembly transforms. Retain the existing definition
allowances derived from composed source transforms and their storage error.
The source-node allowance compares the ORIGINAL vertex or original curve point
against its selected original face-carrier point (and retains the original
endpoint vertex/curve nodal guard). It does not compare a newly rounded mesh
point in their place or become an extra discretionary chord/cell allowance.
Source tolerance remains metadata.

For a cached leaf, let B be its retained bilinear map constructed from its
actual four corner XYZ values, and e bound the original S versus B over the
whole leaf. Every final triangle retains its actual affine map L. On each
triangle/leaf overlap require

    e + upper(norm(B-L)) + charged_domain_displacement <= local_allowance.

Use the SAME emitted L on every overlapping leaf/span; restriction does not
substitute newly sampled triangle corners. Positive rational originals admit
cached Bernstein bounds on N-WB divided by a positive lower W. Analytic source
formulas admit original-frame directional second-derivative bounds on S-B;
actual bilinear twist controls B-L. The geometry review supplies constructive
numerics and the tighter fallback choices. No sampled flatness check alone
establishes this whole-cell bound.

Correct-nearest transcendental XYZ was an internal policy of the previous
constructor, not a prerequisite for accurate source-linked output. The new
design may explicitly use deterministic bounded point construction, with its
original-source departure retained and charged once against the actual
curve-chord and face positional bounds, rather than paid from the source-node
allowance. Original source vertices
remain their original XYZ. A source edge has ONE canonical physical sample
sequence used by every incident face; coordinate equality never welds keys.
CPU/P100 construction must use a specified common arithmetic/evaluation policy;
unspecified libm and execution order cannot choose different mesh geometry.
Any changed construction policy increments the semantic geometry policy.

## Cache ownership and identity

Use packed root/node/control/sample arrays and stable source-derived identities,
not a polymorphic heap object per leaf. Separate immutable source compilation,
mutable adaptive cache storage, and a quality/trim query's selected leaves.
Quality changes may reuse source bounds and parent/child restrictions without
re-extracting source. A source edit invalidates only dependent roots; a trim
generation changes face classification and boundary regions, not unrelated
surface geometry. Assembly allowance changes reselect/refine required leaves.
Source, chart, arithmetic profile, cache generation and query identity are
explicit. A stale sample or cache result cannot settle a current query.

Root domains retain original analytic coordinates or original rational knot
spans. Original knot/domain endpoints need not be dyadic. Subdivision coordinates
are exact dyadic fractions OF the original root interval; packed representations
retain their exact meaning. Common sides and shared knot endpoints have one
canonical chart-point identity and evaluation owner, independently of which
leaf/root reaches them first. Cache-allocation offsets are not semantic IDs.
Adjacent original spans share knot points only after original C0 continuity is
established. A discontinuous source unsupported by the format/policy is refused;
do not infer a new branch or weld merely because rounded coordinates agree.

Select packed binary kd nodes with SoA metadata, degree-bucket control slabs,
two scan/compaction frontiers, a canonical PointPool and a separate QueryView.
The detailed records/procedures are in the
[runtime design](../papers/native-adaptive-subdivision-runtime.md). Retain parent
metadata and summaries; reclaim replayable split-parent controls only after the
epoch barrier. Replaying an evicted net follows the SAME axis/child path from a
retained checkpoint, rather than restricting by a different arithmetic history.

SourceCache compiles only live requested roots from SourceStore. Use interval
original-coefficient extraction, with exact selected-root extraction as a lazy
fallback. Direct analytic descriptors need no rational extraction. The old eager
GeometryStore/PreparedRun cannot be a mandatory constructor merely to satisfy
publisher types; adapt source/envelope/publication ownership to the new roots.
Original source admission and complete inventory remain mandatory.

Each task identifies source, cache, query and arithmetic-profile generations,
root, complete split path, operation and request ordinal. A current slot alone
does not establish identity. Bound root-relative dyadic/path storage with explicit
configured limits and checked allocation; depth exhaustion is unresolved, never
wrapped coordinates or forced acceptance. Reserve/check the complete canonical
epoch's required node/control/frontier/ambiguity/output capacity BEFORE mutation.
Cache history is distinct from the current quality-selected leaf mask.
Preserve base and promoted numerical/control/point tiers separately. Each new
query follows the SAME base-to-escalation decision policy as a cold query; a
previous tighter query cannot silently supply a tighter bound or different Q
that changes subsequent loose-query topology. Reuse a cached tier when the
current policy actually requests it. Promotion invalidates constructed-record
generations, side/physical-node/contact users and bounds at the shared authority;
logical original curve/parameter identity remains separate from that generation.

Use cheap deterministic directional scores from cached analytic curvature plus
twist, or rational directional control/residual variation. These are heuristic
split proposals, not additional geometric truth. Compare their canonical upper
values with a fixed U tie; do not invoke exact transcendental evaluation just
to choose between nearly equal axes. Every fourth split on a failing lineage
instead bisects its longest normalized ORIGINAL-root active direction (U tie).
An axis is inactive only after checked exact affine-separable approximation
dependence: its pure second derivative AND mixed derivative are zero, allowing
linear translation such as cylinder V. A bilinear saddle keeps both axes active.
Numeric roundoff uses precision promotion rather than invented curvature.
This supplies fair shrinking without sacrificing the exact
one-direction cylinder/extrusion path. Retain/count actual chosen restrictions;
trial restrictions/evaluations are not free work or cache reuse.

Select numerical profile `rn64_anchor32_p128_v1`: shared RN operations and
identical IEEE-word outward padding, no reassociation/contraction, converted
existing owned P128 Chebyshev coefficient/anchor intervals and retained uniform
remainder. Use the runtime report's finite bounded quadrant locator, whole
reduced-domain/anchor checks, fixed reconstruction DAG and deterministic
midpoint construction. This is ordinary CPU/P100 point evaluation. Escalate
unresolved numerical width/reduction to fixed128,192,256 and finally owned exact
evaluation. No ordinary libm sine/cosine supplies a geometric enclosure.

The source/numerical profile chooses Q independently of visiting face or local
epsilon. If a tighter request needs a better Q, the PointPool authority promotes
the sorted common requests at a generation barrier and invalidates all dependent
bounds and incident construction. A face cannot privately replace shared XYZ.
Numerical floors request promotion/tighter source bounds, not endless geometric
subdivision. Source construction error remains charged to epsilon as above.

## Shared boundaries and trim embedding

Curve caches retain accepted original parameter intervals, physical endpoint
keys, actual XYZ, chord bounds and source-to-selected-trace correspondence.
Reusing an accepted interval must retain its original domain and quality meaning.
Compile/cancel source expressions before enclosure where their checked finite
geometry permits it. Do not run the old expensive analytic trace preparation
unchanged in front of the new cache and describe that as accelerated meshing.

Independently clipping every leaf to a selected polygon can create new private
vertices on a physical source edge. Mapping such a crossing back to original t
can move the recovered UV away from the claimed crossing. Therefore do not use
private leaf/trim intersections as physical boundary samples.

The leading construction keeps the settled original source-edge chains intact,
emits accepted wholly-interior leaf packets, and triangulates the remaining
connected boundary regions between source chains and packet rims. Build the
trim spatial index once per face boundary generation. Hole/outer winding and
contacts use the existing exact filtered predicates. A leaf touched by a trim
belongs to the boundary region rather than being declared wholly interior.

Interior packet sides consume a common ordered schedule of all neighboring
leaf endpoints. This supports hanging sides without forcing global2:1 balance.
Triangulation must preserve every side sample and emit positive cells, without
creating a new center point merely for convenience. Side/rim events retain
source-independent chart identities and canonical physical nodes.

Select the [algorithm report's](../papers/native-adaptive-subdivision-algorithm.md)
exact collinear-side event sweep and no-center two-half zipper. Each rectangle
with k side samples produces k-2 positive triangles. Both incident sides consume
the same ordered atomic intervals. Inspect alternating packet/band sectors at
rim vertices and monotonically demote incident packets until the simple-loop
region constructor's hypotheses hold; demotion changes no source feature.

Packets touching only at a vertex can produce weak boundary-region cycles.
Resolve such cases explicitly before giving cycles to a simple-loop constructor;
monotone demotion of the affected packets into the boundary region is a finite
candidate. Dense/thin trims may make a large boundary region. Measure that
behavior and its work; do not drop features or invent unsupported success.

Boundary triangles are checked through the source cache against their actual L.
Tighten a loose bound on the SAME L before geometric changes. Consume remaining
surface failures in a complete canonical region horizon, refine implicated cache
leaves and install at least one NEW point strictly inside each failed triangle.
Choose a positive leaf/triangle overlap piece, use its exact rational centroid
as a locator, and refine dyadic rounding until exact predicates prove strict
interiority. Deduplicate anchors canonically and rebuild each affected connected
region once per horizon. An anchor outside/on the old facet, or cache restriction
alone, does not count as construction progress. Apply the same fair split rule
and cumulative memory/work/precision limits; no universal termination/shape-quality
claim follows. Domain/source limitations request their actual feature authority.
The previous whole-face refinement engine is not a hidden fallback.

For the owned integer direct-CDT boundary adapter, select exact common-denominator
clearing per connected region, cached for that unchanged site set. Use one
positive D for BOTH axes, preserving orientation/incircle signs and the declared
metric. Retain the original exact coordinates in semantic sites; the scaled
integers are predicate inputs only. Count/check denominator bits, work and bytes
before use. Common dyadic source coordinates reduce to a shared power of two.
The rational-predicate adapter is an explicit alternative if denominator growth
becomes material; no coordinate is rounded to appease the integer API.

Finite source quotient caps retain their actual symbolic source maps, ordinary/
fan cut identity, original aliases and positional/band components. They use the
cache's geometry service and current source-cut protocol; proxy coordinates are
not ordinary UV coverage. The existing one-cap sector profile is not generalized
by assertion. Add an explicit adaptive-subdivision lineage/root-path operation
and update the semantic codec/readers; do not describe cache sites as LEPP or
circumcenter births.

## CPU and P100 execution

Use the same packed tasks, source geometry and decisions on both paths.
Independent roots/leaves form batches grouped by geometry family and degree.
The GPU performs real restriction/evaluation/bound and adaptive-frontier work.
Host exact predicates and unresolved numerical cases remain explicit tasks.
Transfers do not reconstruct a source tree on each cell or each iteration.

Stable scan/compaction assigns child slots from canonical frontier order;
completion order cannot choose split axes, points, topology or failure identity.
Use owned bounded scratch and persistent root/control/sample pools. Device loss,
cancellation, capacity exhaustion and source mutation must release or retain
leases correctly without publishing a partial successful mesh.

CPU/P100 floating interval widths can differ when one uses nextafter padding
and the other exact directed instructions. Pick a common numerical decision
profile or an explicit common ambiguity decision; do not assume current filter
implementation details already produce identical adaptive leaves.

## Independent comparisons

Compare NVIDIA USD/BRep CPU, owned CPU1/parallel CPU and owned P100 on the same
original source and explicitly matched requested quality/operation scopes.
Document differences in actual guarantees and failed/unsupported outcomes.
Measure source compilation, cache build/refinement, boundary work, output
construction, validation, transfers, display and process lifetime separately.
Report complete cold operation and declared warm-cache reuse, including retained
host/device bytes and invalidations. A warm result cannot stand in for cold cost.

Use independently authored analytic, rational, C0, distorted planar, rotated
ridge, shared-edge, periodic/seam, pole, thin-trim and many-hole families. Actual
complete components and the full watch supplement them. The old four-face
component observation28.3305s mesh is retained as a baseline, not a performance
target or evidence for a new implementation. The comparison tests whether and
where the GPU helps; GPU advantage is not presumed.

## Research and delivery status

The pinned reference source studies and bibliography remain under papers/ and
papers/whole-mesher/. Focused reports cover
[algorithm/trim ownership](../papers/native-adaptive-subdivision-algorithm.md),
[original geometry and numerical caches](../papers/native-adaptive-subdivision-geometry.md),
and [native/GPU runtime](../papers/native-adaptive-subdivision-runtime.md).
The selected procedures above integrate their constructive choices. Scoped
reviews are recorded in
[runtime review](../papers/native-adaptive-runtime-review.md) and
[geometry review](../papers/native-adaptive-subdivision-geometry-review.md),
with algorithm acceptance in the algorithm report/journal. Finite product-unit
plans still require actual interface review and direct RED before code.
Implement complete
conceptual units with direct oracles and scoped runtime/static checks, commit
each material milestone, and keep the whole-mesher PR unfinished until the
selected operation is actually delivered.
