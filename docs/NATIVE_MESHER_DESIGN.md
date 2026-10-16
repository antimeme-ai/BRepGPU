# One native, feature-conforming B-rep mesher

Status: whole-system design accepted for implementation planning by independent
algorithm, geometry/source and runtime/numerical reviews on2026-10-16.
Implementation is underway against the accepted whole plan. This replaces selecting the next local change to the old imported
mesher. The three [research reports](#research-and-alternatives) supply the
literature, inspected reference revisions, assumptions and acquisition gaps.

Reviews and constructive resolutions are retained in
[algorithm review](../papers/2026-10-16-native-mesher-design-algorithm-review.md),
[geometry review](../papers/2026-10-16-native-mesher-design-geometry-review.md),
[runtime review](../journal/2026-10-16-native-mesher-design-runtime-review.md),
[algorithm construction](../papers/2026-10-16-native-mesher-algorithm-resolution.md)
and [numerical/runtime construction](../papers/2026-10-16-whole-mesher-runtime-resolution.md).
They preserve the rejected gaps as well as their resolutions. Accepted design
is not measured implementation or complete-watch success.

## Purpose and complete operation

Construct a complete, source-linked, conforming triangle boundary from the owned
BGSRC001 source. One C++20 application owns decoding, geometry compilation,
shared boundaries, face meshing, refinement, physical embedding and publication.
CPU-only and CPU+P100 are execution policies of that application. Python is an
optional external binding and an independent development oracle; no production
geometry, predicate, inverse-map, refinement or scheduling callbacks cross it.

The operation approximates the supplied geometry at a requested world-space
positional tolerance. It preserves original entity and occurrence identities,
holes, seams, singular quotient vertices and assembly definitions. It neither
heals the original B-rep nor supplies minimum-angle, analysis-element, normal-
angle, volume-error or collision-free-assembly guarantees by implication.
No incomplete part inventory may become a successful mesh.

The requested positional tolerance applies after the retained assembly
placements. Their source matrices are approximately rigid binary64 values,
not exact isometries by assertion. Compose source paths exactly, bound their
actual linear operator norms, and charge rounding of the stored composed
instance matrix. Derive a safe definition-local allowance for all its instances
before meshing; retain that allowance and original path. Raster/projection
rounding remains the separate display-consumer contract.

The selected framework is **geometry-aware constrained refinement with local
approximation bounds and one global feature authority**. Geometry subdivision
trees supply bounds, local charts, span/trim lookup and sizing information; they
do not impose an axis-aligned output grid. Point placement seeks anisotropic
elements; exact local topology and whole-cell bounds decide admissibility.

## Research and alternatives

- [Algorithms and inspected implementations](../papers/2026-10-16-whole-mesher-algorithms.md):
  NVIDIA/OCCT/JIGSAW, local subdivision/trim embedding, anisotropic and frontal
  refinement, WATER and concrete parameterization/axis-alignment countercases.
- [Geometry and error contract](../papers/2026-10-16-whole-mesher-geometry-contract-study.md):
  source realization, curved trims, local residuals, feature protection,
  singularities, physical intersections and independent original-value answers.
- [Native runtime and CPU/P100 placement](../papers/2026-10-16-whole-mesher-native-runtime.md):
  immutable source/cache ownership, task graph, arenas, publication and reuse.

An output grid with local trim clipping is a serious alternative with excellent
regular batching. It is rejected as the universal output structure because a
rotated ruled ridge needs asymptotically more axis-aligned cells than aligned
strips. A hard same-UV interpolation estimate alone is also insufficient as the
only chart: nonlinear parameterization of a plane can provoke subdivision even
though two spatial triangles are exact. Both counterfamilies are mandatory
before tuning. Closed-smooth-surface refinement theorems are not transferred to
trimmed, sharp and toleranced source without their hypotheses.

The existing direct CDT earns reuse for initial constrained connectivity and
its exact filtered predicates. It does not determine geometry refinement,
point placement, error meaning or the native object model. Its fixed-site ABI
is not called through serialization inside each native face job.

## Source meaning and admission

Retain the original snapshot bytes, source digest, coefficients, frames,
tolerances and all typed topology. They are immutable. Compile from original
knots/homogeneous coefficients; floating extracted spans are derived hints, not
automatically exact equivalent geometry. Own enclosing or exact extraction and
restriction. All analytic carriers retain their original UV and independent
axes; do not convert them to rational charts while silently retaining old UVs.

Initial BGSRC001 admission covers its existing finite analytic surface families,
positive rational tensor surfaces through degree three, and spatial/pcurves
through degree32. Degrees, counts and arithmetic are explicitly bounded. An
unknown parameter-map kind is unsupported, not identity. Periodic analytic
charts retain integer lifts. This format contains no NURBS periodic flags or
periods; do not invent those from an interval or assume2π. Supporting additional
source periodicity requires explicit source metadata and tests.

Original spatial edges and vertices own physical boundary identity. Original
carriers own face geometry. Original authoritative pcurves retain their domain
meaning. Reconstructed pcurves guide branch selection; they are not promoted
to exact original geometry. The result names this policy and every selected
trace/discrepancy. A source tolerance is metadata, not permission to enlarge
the requested mesh tolerance.

For reconstructed uses the selected trace is an explicit finite object, not an
implicit inverse equation that a triangulator cannot represent:

1. Choose the working chart **before** defining the selected domain. Usually it
   is original UV with source map Psi=identity; the admitted physical-plane
   profile below instead chooses projected XY and Psi as its proved original-
   projection inverse. The face owner selects one dyadic working-chart joint
   for each source vertex occurrence branch before sampling incident uses.
   Incoming/outgoing endpoints agree exactly there. Periodic seam copies retain
   separate lifted joints; pole/apex copies retain an explicit quotient. Equal
   canonical XYZ alone does not establish a closed chart loop.
2. At dyadic original edge parameters, original analytic inverses or local NURBS
   solves propose dyadic sites in that chosen chart near the retained branch;
   physical-plane sites use the original spatial projection. Plane dual
   coordinates, angle/height inverses and projected solves are distinct locator
   algorithms. Original pcurves/maps/lifts and the original-UV recovery descriptor
   remain attached. Predicates see chosen exact dyadics, never an approximate
   locator value misrepresented as an exact inverse root.
3. Gamma_h is the finite polygonal trace through those working-chart sites and
   fixed joints. Its default correspondence to original t is piecewise affine.
   A checked monotone profile may reparameterize the **same polygon**, storing
   the map/profile, endpoint coverage and whole-interval error. For example,
   original projected scalar position or cylinder angle can supply the
   correspondence. No unrepresented inverse roots become topology sites.
4. Bound the whole original-geometry relationship under the actually selected
   correspondence: compare C_e(t) with S_f(Psi(Gamma_h(theta_h(t)))), or use a
   proved geometric-image profile establishing both directed distances and
   monotone endpoint coverage. The default original-UV, same-t case uses a
   rational Bernstein composition residual or enclosed analytic formulas.
   Geometrically identical straight traces need not have identical same-t
   parameterizations; never call their parameter-matched residual zero. Bound
   C_e against the actual emitted spatial chord independently, using the same
   distinction between parameter-matched and checked geometric profiles.
   Canonical interior nodes evaluate C_e; endpoints use original vertices.
5. Require simple, correctly oriented selected loops with every declared hole
   and original use, valid branch/lift choices and bounded whole-interval source
   discrepancies. Invalid polygon pairs request coordinated original-parameter
   splits before face construction. Trace changes advance the boundary
   generation and restart affected face domains. A fixed-point loop has explicit
   work/precision limits; difficult geometry cannot become a success by omission.

Thus all-reconstructed faces have E_domain=0 relative to their named selected
polygon **in the chosen working chart**, realized through S_f composed with Psi;
the whole-interval original edge/carrier discrepancy is still required. A
physical-plane realization is not first defined as an original-UV polygon and
then charged for projecting that unnecessarily chosen polygon. These traces
are not claimed identical to an unknown original curved
trim. Authoritative original pcurves instead retain their curved-domain meaning:
their graph strips and joint neighborhoods must establish bounded correspondence
to the polygon, charged below. An undefined/open authoritative domain is reported
explicitly. Degenerate edges take a declared constant/quotient branch and never
enter a formula dividing by a zero parameter range.

Authority is per use, not a face-wide boolean. Mixed faces use the finite
`HybridPullback` realization: a joint touching an authoritative arc retains its
original endpoint A, with a common chosen polygon joint Q. A local nodal fan B
maps A to Q. Reconstructed reference arcs are B^-1 of their selected polygons;
authoritative arcs remain original. Apply all nodal B maps, then tapered strip
maps T only on authoritative arcs, excluding reconstructed polygons except their
fixed common endpoints. This closes the full reference boundary and maps it to
the selected polygon. Bound and charge B/T displacement on every affected cell
and inverse movement in original-edge/reference-trace comparisons. It is not
free domain error on reconstructed arcs near a mixed joint. Store the finite
selected maps and reference-policy tag. The [geometry plan](native-mesher-geometry-plan.md)
specifies the affine fan inverse, support construction, bounds and unresolved cases.

Thus the reference is a named toleranced face realization plus bounds to the
retained originals. We do not promise distance to an unspecified exact solid
that the input representations do not define.

## One approximation rule

An approximation cell K records a source-domain map Psi_K, an actual emitted
piecewise affine physical map L_K, its source/quotient ownership and a bound
for every point of the cell. This includes ordinary UV triangles, certified
physical-plane charts and singular fan cells. Cell-domain coverage and physical
triangle incidence are separate; one UV triangle need not equal one physical
triangle at a pole.

For an ordinary rational cell S=N/W, restrict the **original** homogeneous map
to its barycentric triangle. Let L use the actual emitted binary64 XYZ. Form
R=N−WL in one triangular Bernstein basis. Positive weights give

    sup_K ||S−L|| <= max_i ||R_i|| / min_j W_j.

Tensor degree(p,q) becomes total degree at most p+q; residual degree is at most
p+q+1. Use proper Bernstein multiplication and degree elevation. Subtract an
identical nearby anchor algebraically before coefficient formation to avoid
large-origin cancellation. Source-to-span extraction, restriction and arithmetic
are exact or enclosed. Canonical nodal displacement and XYZ rounding already
enter L; do not add them a second time.

For crossed knot spans, query the source-span hierarchy and exactly clip the
dyadic chart triangle against original rational knot rectangles. Triangulate
each nonempty convex intersection and restrict its original span there, always
comparing against the same emitted affine L_K. Take the maximum over all pieces.
No per-triangle scan over all carrier spans. A containing-rectangle bound is a
filter; triangular restriction tightens uncertain cases. No C2 formula crosses
a C0 knot. The positional contract preserves original B-rep feature chains; it
does not promise an exact output crease chain at every internal C0 knot, so no
unconstructed cut×trim algebraic intersection is required. Reject positional
discontinuities (internal multiplicity degree+1 unless matching one-sided source
limits are established); C0 continuity is not C-1 continuity.

Analytic cells use local, enclosed original-formula bounds, including bilinear
twist/mixed behavior. Cache reusable frame and expression data. Use enclosing
trigonometric range reduction/evaluation or conservative nontrigonometric
bounds; CPU/CUDA libm output by itself is not an enclosure.

An affine UV residual controls only the polygonal chart domain. For authoritative
or transformed curved traces, compile each interval into a regular monotone graph strip, with an ordered chord and
a bound on displacement of its carrier image. Prove that the curved/polygonal
domain symmetric difference is contained in the strips and protected vertex
stars. Their directed, noncrossing order provides an onto domain deformation.
On a boundary cell accept only when

    E_domain(K) + E_actual_cell(K) <= requested_tolerance.

Interior cells have E_domain=0. The maxima are local; do not sum bounds for
unrelated cells. Physical source-edge chords independently satisfy the requested
edge bound against C_e, with original endpoint departures included. Source
uncertainty remains reported separately from computed discrepancies.

The retained source-recovery setting is a nodal cap: original V or C_e(t_i)
versus original S at the stored chosen recovery UV. It is not a whole-curve
closest-carrier guarantee. Whole curve/selected-trace approximation uses the
positional allowance; a plane's curved trim must not inherit a finer chord
tolerance merely because its nodal recovery cap is smaller. Actual XYZ rounding
is already included in cell/chord bounds and is not charged again here.

The default edge correspondence is parameter-matched linear interpolation.
A checked geometric-line profile instead projects C_e onto the endpoint line,
proves the scalar map is monotone and covers the chord, and bounds the transverse
residual and endpoint departures. It records that scalar correspondence. This
admits nonlinear parameterizations of a straight edge without pretending that
parameter-matched interpolation error vanishes. Local graph profiles may use the
same explicit correspondence interface after their own bounds are implemented.

There is no fixed global1/8 edge allocation or3/4 face allocation. An initial
edge may consume available local tolerance; a face requests additional shared
splits if its actual bound cannot close. Refining cannot remove an unavoidable
source discrepancy or output precision floor. Distinguish a lower-bound witness
of infeasibility from a conservative upper bound that merely remains unresolved.

## Geometry compiler and local charts

SourceStore owns typed source tables, ID-to-slot maps and incident-feature CSR.
GeometryStore compiles original curves/carriers into immutable roots and owned
local restriction trees. Cache dual frames, original continuity intervals,
restricted positive homogeneous nets, derivative/normal bounds and compiled
analytic expression profiles. Child nets are obtained from parents; do not
re-evaluate every ancestor or reconstruct frame inverses at every point.

Compile nearly affine chart maps as bounded tubes around a proposed affine map
when source algebra/enclosures prove it. For instance an angular trace near an
affine angle may have a small certified residual; it need not pay a loose norm
bound that discards derivative cancellation. This is a general checked geometric
profile, not coordinate snapping or a source-name dispatch.

The initial physical-plane profile is finite and constructive. On the convex
original UV rectangle, prove a uniform positive lower bound on the symmetric
Jacobian of a fixed preconditioned source projection F. Prove that its four
projected natural-boundary curves each monotonically cover a straight segment,
forming the boundary of a nondegenerate convex quadrilateral Q. Continuity,
injectivity and that boundary correspondence give F(rectangle)=Q. Require every
working-chart domain/cell to lie in Q. Psi is the inverse of this retained
source projection on Q, represented by its source/chart descriptor rather than
algebraic UV coordinates passed to topology predicates.

Select reconstructed polygonal domains directly in this XY chart. Checked
geometric-line/trace profiles establish boundary coverage without demanding
same-t equality of nonlinear parameterizations. Authoritative original domains
still transform their actual curved traces into this chart and require the
corresponding strip/star bounds; their domain meaning is not redefined.

A whole-original-rectangle control-net height enclosure supplies a valid bound
on every inverse-image cell without solving an arbitrary implicit inverse-cell
problem. Include the actual emitted nodes' off-plane displacement and projected
coordinate/XYZ rounding in the world-space cell bound, once. The enclosure must
include interior controls: a height bubble can vanish along every boundary.
Consumer UV corners may use approximate inverse locators with enclosures; they
do not redefine Psi or the chosen XY domain. Exact coplanarity and the admitted
nonlinear planar-square counterfamily can then need only two spatial triangles,
independent of source parameter speed. More general inverse-coverage profiles
remain future contenders; a failed profile falls back to valid original-UV
cells. There is no universal two-triangle claim for arbitrary planar trims.

At a sphere pole/cone apex prepare an explicit quotient star. A radial/angular
cell can map to a physical fan triangle through

    Phi(xi,rho)=(1−rho)P_pole + rho((1−xi)P_a+xi P_b).

Record the full source-cell map and quotient identity; bound S∘Psi−Phi over the
cell. Emit nonzero physical fan facets and retain all source-domain coverage,
including the collapsed pole boundary. General polar trim branches need
monotone star maps, not an assumption that every trim is a meridian. A source
with an unresolved singular chart returns a typed unresolved result.

## Shared feature authority and trim topology

Each original physical edge has one owner, one ordered exact dyadic parameter schedule
and one canonical XYZ sequence. Each use has a separate chart/period/normal
record. Original vertex identity, edge identity and coordinate equality are
different concepts. Initial chart admission uses the existing finite source
chart, explicit periodic seam occurrences and the implemented analytic pole/apex
quotients. General new atlases, NURBS singularities and unspecified NURBS
periodicity are typed unsupported cases until their own complete construction
is delivered. Do not invent interior atlas cuts or imply the initial CDT accepts
an unimplemented internal feature graph. Bounds can cross C0 spans as above.

Before ordinary refinement of authoritative curved domains, establish vertex
stars and protected feature neighborhoods. Use source curve/trace enclosures to exclude nonincident features
and isolate regular outgoing branches. At an acute or tangent junction choose
a common physical collar radius, isolate first branch exits, and shrink the
radius/parameter brackets until interval orientation/leading-jet tests establish
the required cyclic order of the chord rays. Canonical endpoint consistency is
part of this construction. Distinct coincident branches require an explicit
source explanation (such as a seam); otherwise report invalid/unresolved input.
A positive-clearance argument for disjoint arcs is not used at their common
vertex. Ordinary well-separated corners need no artificial tiny collar.

For authoritative curved domains, outside the stars subdivide traces until each piece is a regular monotone
graph, its protected strip is separated from nonincident strips, and its local
geometric error is bounded. Conservative curve/strip indices prune candidate
pairs; uncertain pairs use local subdivision/exact decisions. Chord insertion
must preserve loop winding, hole ownership and original cyclic uses. No post-hoc
circle-only rewrite, dropped small hole or proximity weld repairs the result.

Faces read an immutable BoundarySnapshot. They propose splits in original edge
parameters; only the edge owner applies them. Requests merge by exact parameter
identity and source order, advance the schedule generation and invalidate the
incident face generation. Inserting a new source-curve point generally moves
the polygonal boundary off its old UV chord; this is not an ordinary collinear
triangle-edge split. Restart the affected face domain and initial connectivity
from the new snapshot, retaining immutable geometry caches. All incident faces
consume the new chain before publication. Grid/chord intersections never create private physical boundary
vertices. Generated nodes have explicit curve/face/cell ancestry.

## Face construction and refinement

Each face generation uses one fixed positive metric for initial connectivity
and its progress algorithm. At the canonical chart-rectangle center, evaluate
the original first derivative J and normal second derivative matrix II. Let H
be the positive enclosing physical carrier-box diagonal, U,V the positive chart
widths, and R=diag(U^-2,V^-2). The selected proposal matrix is

    B = J^T J/H^2 + |II|/(8*epsilon) + 2^-40 R.

The absolute value is spectral, not entrywise. Compile a nonsingular dyadic
factor T0 from a stable scaled spectral evaluation; M0=T0^T T0 is then exact
and fixed. The factor construction/regularization and zero-rank cases are given
in the [algorithm resolution](../papers/2026-10-16-native-mesher-algorithm-resolution.md).
These constants guide work, never accept geometry or consume an error allowance.
The initial direct CDT reads exact transformed coordinates T0*q with unit
diagonal metric; every site retains its original chart coordinates. This reaches
for anisotropic connectivity without misusing a diagonal-metric API. Positive
determinant preserves orientation. Ordinary rank-deficient charts do not enter
this path; they need an implemented quotient construction or typed refusal.

The refinement and optimization phases are deliberately separate:

1. Construct the complete exact polygon domain from the settled snapshot and
   admitted star/quotient records. Use the native direct constructor through
   typed in-memory values. No whole-face binary pack/redecode or Python callback.
2. Process failed cells using a common host-derived proposal priority and stable
   cell key; GPU interval tightness/completion order cannot choose priorities.
   An initial triangle has a sticky `heuristic_spent=false` lineage flag. It may
   try exactly one local-metric circumcenter: solve its 2x2 equations exactly in
   the source-defined dyadic SPD metric at its canonical centroid, then round
   each coordinate to nearest binary64, ties to even. Exact location, positive
   children, nonduplication and feature protection decide whether this chosen
   dyadic site can be inserted. A rejection spends the attempt and enters
   fallback; there is no second unspecified candidate routine.
3. An admitted candidate lies strictly inside its failed target triangle, or on
   its unprotected internal edge with both incident lineages still unspent.
   Reject a circumcenter in an unrelated triangle or a spent neighbor. Insertion
   splits the target into three, or that internal edge's two incidents into four.
   Children stay inside parents. Set affected lineage flags true, inherited by every descendant;
   acceptance never resets them during the generation. No cavity reconnection,
   flip or vertex motion occurs during refinement.
4. Remaining failures use terminal-priority longest-edge propagation (LEPP) in
   the fixed exact M0 norm. Walk across the selected longest edge. If it is also
   a longest edge of the neighbor, including a tie, stop and bisect that common
   edge at its exact chart midpoint in both incidents. Otherwise the next
   triangle has strictly larger diameter; continue. Tie endpoint identities are
   stable, but an equal-longest incoming edge terminates instead of walking on.
   Repeat from the unchanged failed target until it is removed. An original
   boundary encounter requests a master interval midpoint and ends that face
   generation; it is never a private collinear split.
5. Every transaction removes old triangle/edge/BVH rows, repairs exterior
   neighbor handles and site incidence, installs positive children, and queues
   their bounds and affected actual-facet contact neighborhoods. Outside cells
   retain their maps and bounds. Slot generations reject stale queued work.
   The resolution specifies exact replacement triangles and update sets.
6. Settle bounds and actual contacts before optimization. With fixed sites,
   optimize only ordinary affine-chart pairs: both old and replacement cells
   must pass their actual positional/domain bounds and contacts, the common
   quad must be strictly convex, and the new diagonal must be unprotected and
   nonduplicate. For canonical triangle order, source-derived local M_T and
   exact positive chart area A, the selected shape potential is

       Phi(T) = round_binary64((sum_edges d^T M_T d)^2 /
                              (48*det(M_T)*A^2)).

   Compute the rational expression once and round nearest, ties to even.
   Nonfinite/nonpositive exceptional values reject the flip. Compare old/new
   sums of the finite binary64 values as exact dyadics; only strict decrease
   flips. Equal sums retain the old edge. Potentials depend only on stable
   source geometry and fixed sites, not mutable cache refinement. No refinement
   is interleaved with optimization; a boundary change discards that optimizer
   and starts a new face generation. Singular fan cells are not flipped by this
   ordinary-chart primitive.

In the linear T0 image every terminal split is an ordinary longest-edge
bisection in every replaced triangle. After two such descendants the diameter
contracts by at least sqrt(3)/2; the at-most-one heuristic on each ancestry path
does not defeat shrinkage. Finite LEPP walks/closure and the sufficient-size
argument are spelled out in the resolution. Boundary interval trees supply a
separate outer contraction; a face-generation restart is not part of the inner
forest proof. Success remains conditional on regularity, strict feasible error
margin, controlled boundary discrepancy and required feature separation.
No universal CAD termination, mixed-algorithm linear-complexity, minimum-angle
or optimal anisotropic-output theorem is asserted. Fixed-site strict potential
decrease does prove that the separate optimizer ends. Finite budgets/cancellation
bound unresolved work; actual complete-watch success remains required for delivery.

## Physical embedding and publication

Maintain a conservative within-definition BVH of actual facet bounds. Exact
binary64 contact predicates distinguish disjoint, declared vertex/edge contact,
T-junction, coplanar overlap and forbidden intersection. Sharing a source vertex
does not exempt the rest of a pair. A forbidden contact requests local geometry/
boundary refinement. Each selected patch has a finite contact-refinement budget;
unresolved contact returns a structured failure. The claim is an embedded
realization with bounded per-entity approximation. It is not a claim that every
input self-intersection below epsilon has been discovered: output contact tests
cannot establish that stronger statement. No original entity may be omitted.
Assembly instances are outside this collision rule; they may touch/overlap.

Publication requires complete original inventory, current boundary generations,
accepted local approximation/strip bounds, conforming chains and source/quotient
incidence, and no pending/stale actual-facet contact decisions for final node/
boundary generations. It publishes one immutable NativeMesh or a structured failure. These
are direct construction invariants, not a second certification framework.
Independent original-value readers remain development/qualification tools off
the product path and inspect actual output rather than trusting producer flags.

NativeMesh retains source, physical nodes, generators/parameters, ordered edge
chains, coedge chart occurrences, face cells/maps, actual facets, quotient maps,
quality and derived-feature ancestry. BGMESH01 is a derived display view. A
separate explicit little-endian full-mesh format embeds the source snapshot and
counted length-delimited semantic sections; no dumped structs, JSON or pickle.
Decoders reject overflow, foreign IDs, unowned rows and incomplete references.

## Native execution and P100 placement

MeshingRun owns SourceStore, GeometryStore, BoundarySnapshot generations,
bounded worker scheduler, aggregate reservations, worker arenas and optional
device sessions. Original/span geometry roots are shared immutable values.
Mutable restriction nodes, connectivity and scratch have one face/edge owner;
no unassigned lazy shared cache mutation occurs. Edge/feature owners and face
jobs mutate disjoint state.
Barrier application of ordered requests gives deterministic IDs independent of
worker count. Stable prefix ranges publish output. Exact arithmetic budgets are
per owner; the current non-thread-safe face budget is never shared between jobs.
Count actual simultaneous reserved host/device buffers, including retained arenas.

Independent curves/spans/edges/faces expose CPU parallelism. Batch tiny work,
let large face owners submit bounded numerical batches and yield continuations,
and steal ready tasks without sharing topology mutation. Workers must not all
block behind their own queued child work. Before admitting a parent, reserve its
bounded active scratch and permitted outstanding child/staging capacity; suspend
fewer parents or release disposable caches rather than hold the full memory
budget while children wait for allocations. Inputs/results outlive completions;
no view points into an arena reused by the next job. Task-boundary exception
handling completes dependency bookkeeping on failure. Cancellation stops new
submissions, invalidates dependent generations and drains CPU tasks/owned device
streams before buffers are freed or arenas reset. Device errors poison that
session. Retain the previous immutable published result; no partially written
complete artifact appears.
Explicit work/memory/precision limits and user cancellation replace an arbitrary
short whole-watch shell deadline. Progress emits named stage walls and work
counts through one channel; no second telemetry/receipt system.

Select two initial certified P100/sm60 task kinds. `RationalTriangleResidual`
restricts an original rational surface to the actual cell/subcell map and forms
its residual against the actual emitted affine facet. `RationalCurveResidual`
restricts an original rational curve interval and forms its residual against
the actual emitted chord. The parameter-matched chord primitive is not the
universal curve algorithm: proved monotone geometric-line/graph specializations
still avoid nonlinear straight-curve overrefinement. Surface residual degree is
at most7 (36 triangular coefficients per coordinate); degree32 curve residual
degree is at most33 (34 coefficients). Original curve/selected-trace mismatch,
feature/chart selection and topology initially remain host work.

For these tasks the defined exact original-coefficient residual is R=N-WL,
w_*=min_j W_j>0, and acceptance is precisely every coefficient satisfying

    sum_axis R_i_axis^2 <= tau^2 * w_*^2.

Equality accepts. The host supplies the same exact nonnegative rational local
allowance tau in both policies, deriving a canonical safe lower allowance where
other bounded error terms require it. A failed comparison means this conservative
bound does not accept, not that the geometry is inherently infeasible. Exact
extraction/restriction/elevation/multiplication define coefficients; approximate
cached nets do not become exact input. Directed interval filters enclose this
same comparison on CPU/P100. An overlap, uncertain weight positivity or exceptional
arithmetic returns to the owned host exact rational/integer evaluation, clearing
only known-positive denominators. Thus interval tightness changes fallback work,
not accepted splits. Exact arithmetic remains under per-owner resource limits.

Analytic bound decisions remain host-authoritative unless the host compiler has
produced canonical certified rational/dyadic constants for a named implemented
analytic bound formula; then the same filtered comparison can be batched on the
device. This selects no general GPU inverse/trigonometric solver. Existing
approximate first-jet/display kernels retain their numerical consumer contract.

The numerical profile is IEEE binary64, explicit CUDA directed FP64 intrinsics
and proved outward host operations, with stated rounding, contraction and
subnormal handling. Host filter workers require nearest rounding, no excess
precision and no FTZ/DAZ; verify or disable filters without changing other workers'
FP environment. No fast math or unproved reassociation/FMA substitution applies.
Nonfinite or unrepresentable enclosures and invalid denominator intervals are
unresolved tasks, never approximate acceptance. Correctly enclosed subnormals
are permitted. Exact source fallback and analytic unresolved handling remain
explicit; ordinary CPU/CUDA libm is not an enclosure.

P100 sessions retain immutable source/span control tables and bounded staging/
batch buffers, bucketed by kind/degree. Task records carry exact input ranges,
actual output-point/allowance references, source/cell IDs, bound revision and
source/boundary generations. They return proved accept, proved bound failure or
unresolved with matching identity. Late results cannot enter a newer snapshot.
Canonical candidate sites, physical XYZ words and proposal/priority values come
from the common host policy, not GPU interval endpoints or completion order.
A device evaluation may supply a canonical mesh word only when its enclosure
proves the common rounding decision; otherwise use host canonical evaluation.
Successful NativeMesh geometry is therefore identical across CPU/P100 policies
regardless filter tightness. Approximate display jets have a separate numerical
comparison scope.

GPU memory/initialization failure is a typed run failure unless an explicitly
selected and reported fallback policy requeues the identical task on the host;
never label silently CPU-only work accelerated. Include packing, upload/download,
synchronization, fallback and teardown in whole-call costs. Existing display
kernels alone do not establish accelerated certified meshing. The
[runtime resolution](../papers/2026-10-16-whole-mesher-runtime-resolution.md)
specifies the task/value/lifetime seams for the implementation plan.

## Delivery and independent evidence

The implementation plan follows review of this complete design. Its conceptual
units are the geometry compiler/bounds, feature/face meshing authority and native
runtime/result integration, all sharing the specified cell/feature contracts.
No independent port is delivered as the new whole mesher. Review actual code,
fix findings, and run relevant host/device instrumentation on the fleet.

Before tuning, direct oracles cover original-value curve/surface answers,
nonlinear planar maps, rotated anisotropic ridges, bilinear twist, varying
positive weights, C0/C1 knots, protected holes/tangent stars, seams/poles,
source discrepancy, precision floors, physical contact and ownership failures.
Use applicable actual pinned NVIDIA/Manifold/OCCT inputs with their semantics.
Retain unresolved external alpha cases. Mutation work is selected by uncovered
fault classes, not a ceremony preceding implementation.

Then exercise the actual complete watch and native CPU/P100 consumers at the
same declared source/quality. Report complete calls and useful stage/work/memory
costs, output density, unfavorable cases and failures. Compare actual NVIDIA
CPU outcomes without treating its missing definitions or tolerance flags as
success. After integrated geometry passes, produce the required assembled and
exploded labeled graphics. External README-baseline backfill follows the watch
as already requested. No alpha or broader kernel capability is inferred.
