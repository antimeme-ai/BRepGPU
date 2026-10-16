# Cached adaptive B-rep mesher

Status: operator-selected architecture; constructive design under independent
algorithm, geometry and runtime review. This supersedes the direct-refinement
framework in NATIVE_MESHER_DESIGN.md. It is not an implemented mesher claim.

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
The source-node allowance bounds a physical source node's departure from the
original vertex/curve versus its selected face carrier. It does not become an
extra discretionary chord or cell allowance. Source tolerance remains metadata.

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
original-source departure retained and charged once. Original source vertices
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

The splitting policy, packed layout, canonical point algorithm and reclamation
procedure remain concrete review decisions before product implementation.

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

Packets touching only at a vertex can produce weak boundary-region cycles.
Resolve such cases explicitly before giving cycles to a simple-loop constructor;
monotone demotion of the affected packets into the boundary region is a finite
candidate. Dense/thin trims may make a large boundary region. Measure that
behavior and its work; do not drop features or invent unsupported success.

Boundary triangles are checked through the source cache against their actual L.
Failure requests a geometrically defined cache/region refinement, with one
owner and an explicit progress/resource rule. Do not secretly invoke the old
whole-face refinement engine. Exact proposed rules for region refinement and
mapped poles remain review items in the algorithm/geometry reports.

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
papers/whole-mesher/. Current focused reports will cover algorithm/trim ownership,
original geometry and numerical caches, and native/GPU runtime. Integrate their
findings, resolve the remaining concrete decisions, obtain design review, then
write/review the implementation plan before product code. Implement complete
conceptual units with direct oracles and scoped runtime/static checks, commit
each material milestone, and keep the whole-mesher PR unfinished until the
selected operation is actually delivered.
