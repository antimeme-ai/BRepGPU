# Native mesher: shared features and face construction implementation plan

Status: proposed implementation plan for review; no product implementation or
tests have started. This is one construction owner inside
`NATIVE_MESHER_DESIGN.md`, coordinated with the geometry and native runtime
plans. It owns source incidence through final accepted facet connectivity,
including actual-facet contact decisions. It does not ship an independent CDT
port or resume the old imported mesher.

## 1. Ownership and typed seams

Place owned code in `native/mesher/` under namespace `bgmesh`. Separate headers
by values and operations, not by an alternate object model. The runtime plan
owns immutable `ExactWords` and job-local exact rational arithmetic. Reuse those
values here; do not introduce another persisted bigint or a second JSON model.

| Record | Contents and owner |
| --- | --- |
| `SourceIncidence` | Definition→face/edge/vertex, face→loop, loop→ordered use, edge→all uses and vertex→incident edge/use CSR. Immutable, compiled once from the original source. |
| `PhysicalNodeKey` | Original vertex, original-edge parameter, or face-generated node key; quotient aliases are explicit source-derived records. This identity is independent of XYZ and chart coordinates. |
| `ChartSite` | Stable key, two exact coordinates, physical-node key, source/branch lineage and face generation. Mesh sites are dyadic; span-clipping subcells may be general rationals without becoming sites. |
| `EdgeInterval` | Original edge, ordered exact original-parameter endpoints, interval-tree key, canonical endpoint nodes and owner generation. |
| `Joint` | One chosen dyadic working-chart endpoint for a face/source-vertex branch, plus all incident uses and original-UV/recovery descriptors. Period/seam branches remain distinct. |
| `UseTrace` | Original use, master interval, branch/lift, chosen chart endpoint sites, source→trace correspondence descriptor and geometry-bound result. |
| `BoundarySnapshot` | Immutable physical nodes/chains, joint/trace records, protected source constraints, quotient descriptors and exact incident-generation vector. |
| `FaceMesh` | One owner: append-only site records; packed live triangle/edge rows; adjacency; lineage tree; queues; exact predicates; local numerical cache. |
| `FacetDelta` | Removed/inserted actual physical facets, node keys, source/cell/boundary generations, bound state and topology contact declarations. |
| `DefinitionContactOwner` | Sole writer of the actual-facet BVH, active pair states and deterministic contact/optimizer commits within a definition. |

Use typed slots with checked conversion and separate generation words. No raw
pointer survives an owner yield, vector growth or arena reset. Site keys retain
their generator identities before runtime assigns final deterministic prefix
IDs. Never derive identity from coordinate hashes or worker completion order.

Physical feature identity is definition-local: vertex key
`(definition,source_vertex)`, edge owner `(definition,source_edge)`, edge sample
`(definition,source_edge,exact_t)`, generated face node
`(definition,face,lineage)`. A source edge/vertex may appear in several definition
inventories; compile its global immutable geometry/index once, then filter its
incident CSR by definition for physical ownership. Those definitions mesh
independently under their own local allowances. All instances of the *same*
definition reuse its one mesh. Do not merge physical nodes across definitions
merely because their source ID or XYZ is equal.

Here epsilon denotes the definition-local positional allowance derived by the
root plan from world tolerance and actual instance matrices. The source-only
residual allowance has its own correspondingly scaled policy. Use these local
allowances consistently in bounds, collars and metric proposals; do not reset
them to the caller's larger world tolerance inside an edge/face job.

The geometry owner supplies typed operations, not callbacks into Python:

- compile a finite admitted chart and its exact source/recovery descriptor;
- choose a common joint from the complete incident branch request;
- locate a candidate chart site and evaluate canonical physical XYZ;
- bound an original-edge interval, a selected use trace, an authoritative
  strip/star, or an actual approximation cell;
- construct the supported singular quotient cells and a legal child recipe.

An ordinary cell contains its three exact chart sites and actual three XYZ
words. A physical-plane cell additionally references the admitted projection
and inverse-coverage descriptor. A singular cell references the source sector/
rectangle, quotient map and its associated nonzero physical facets. Topology
does not fabricate a pole map or reinterpret clipped bound pieces as mesh nodes.

## 2. Compile source incidence and directed physical edges

Validate every source table row, unique typed ID, count/range, owner and all
cross references before jobs run. Build CSR by count/prefix/fill using stable
source order. Preserve all coedge occurrences, including two uses of one edge
on the same face. A vertex's repeated endpoint occurrence on a closed edge is
not a duplicate edge or permission to delete a seam.

**Endpoint mapping is already explicit:** for original edge `e`,

    e.parameter_range[0] <-> e.vertex_ids[0]
    e.parameter_range[1] <-> e.vertex_ids[1].

The parameter range may descend. Do not invert this mapping using
`curve_same_sense`; retain that flag as source metadata. A coedge with positive
sense traverses the canonical chain in that endpoint order; negative sense
traverses it backwards. Parameter-map evaluation consumes the original `t`
once and is not reversed again by the traversal flag.

Original endpoint nodes use the retained vertex XYZ words. Interior edge nodes
use the common host canonical original-curve evaluation. Include original
curve/vertex departures in the geometry bound, without inheriting the old fixed
`tolerance/8` rule. Distinct source vertices remain distinct nodes unless the
explicit admitted quotient construction names their identification. Coordinate
equality alone is not such a construction.

Keep the complete source incidence multiplicities. Before publication match
mesh edge/use incidence to those declared source/quotient occurrences; do not
silently impose two incidents on a seam or erase a nonmanifold occurrence.
Admit finite one-, two- and higher-use incidence as declared, subject to the
same face-domain, quotient and actual-contact rules; an unowned/missing use is
invalid. Retain explicit open/nonmanifold source/result status. The plan does
not promise a closed solid for an originally open or nonmanifold input merely
because triangles were emitted.

## 3. One original-parameter interval forest and common face joints

Each physical edge owner maintains an ordered interval forest in its original
parameter direction. Roots contain its original parameter endpoints, original
spatial-curve breakpoints within the active range, parameter-map source-`t`
breakpoints within that range and approved
dyadic feature/star samples. A request names a live root/leaf interval, never
an arbitrary face-private parameter. The owner bisects it at the exact midpoint,
even when its original range descends. Endpoints and all subsequent samples are
retained exact dyadics; depth/exponent/limb limits are charged, not rounded away.
A declared degenerate/zero-range edge enters the geometry owner's explicit
constant/quotient branch before interval division or midpoint refinement. Its
endpoint departures and source ancestry still participate; a zero range is
not repaired by inventing a positive interval.

Pcurve knot preimages through a piecewise-affine parameter map can be rational.
They are geometry restriction boundaries, not automatically new physical mesh
nodes. Geometry may clip a bound interval there exactly. Do not round a derived
span boundary and claim it is an original source breakpoint.

Before edge sampling, gather all incoming/outgoing uses of each face/source-
vertex branch. The geometry owner chooses the working chart first and proposes
one common dyadic joint from the original vertex, source guides and branch.
Every use of that branch consumes exactly that joint. For an authoritative
pcurve, its original endpoint remains geometry authority; any chosen polygon
joint discrepancy is charged by the endpoint/star correspondence. A seam has
separate lifted chart copies of one physical node. A pole has an explicit
quotient record, not several independently rounded UV corners welded later.

At an edge sample the original analytic inverse/local solve is a locator only.
Its chosen dyadic chart result and retained branch define the reconstructed
polygon. Whole-interval original curve/carrier and chord bounds decide whether
it is acceptable. Native parameter-map routing is explicit: identity, exact
piecewise-affine `(t,s)` evaluation, or an implemented bounded selected-branch
recovery profile. An unknown/unimplemented map cannot become identity. The
geometry owner handles authoritative curved strips/stars before a domain is
admitted; topology consumes their descriptors and source-linked constraints.

Use a complete boundary-request epoch within each definition. From the same
immutable snapshot, every active face runs to its
canonical first boundary-request yield or its local-bound-settled horizon.
The former returns its deterministic bounded request set; the latter returns
its retained current result. Wait for **all** current-generation participants
to reach that horizon before applying any request. A faster CPU/GPU face cannot
cancel a slower face before the slower face produces its deterministic request.
Hard current-generation failures fail the run rather than omit a participant.

At that complete horizon, merge requests by edge/leaf identity, apply them in
stable source order, assign canonical physical nodes, and rebuild every incident
use trace and face boundary snapshot. Mark changed incident face generations
stale, including completed but unpublished ones. A new source-curve point is
generally off an old chart chord: discard affected mutable face connectivity
and restart domain admission plus initial CDT. Retain immutable geometry roots
and source-keyed cache entries. Unrelated faces do not restart.

All incidents must settle the same latest edge schedule before publication.
No old face may publish while another incident uses the new chain. Requests
arriving after cancellation are discarded by run/generation identity. A failure
from a still-current edge/trace cannot be hidden by dropping its face. This is
the selected outer fixed-point algorithm, not a claim that arbitrary sources
eventually become feasible. Precision/work limits return structured unresolved
or witnessed infeasible results without changing epsilon.

## 4. Indexed exact polygon admission, then typed initial CDT

The initial ordinary profile admits one declared outer loop and all its declared
holes in one working chart. More than one declared outer is typed unsupported
with the complete loop inventory; never choose the first and discard the rest.
A later component partition must be a reviewed construction, not an accidental
CDT side effect. Geometry-supplied quotient regions enter their own supported
cell/domain adapter rather than this ordinary-profile assumption.

Concatenate directed use chains with the common joints. Do not duplicate a
joint as a second site at the same loop position. Keep each omitted terminal
copy as an occurrence reference to the retained joint. Admit protected collinear
samples but reject a collapsed chart segment, repeated chart site, wrong loop
winding or a nonincident endpoint touch. Repeated physical nodes at distinct
lifted chart sites are allowed only by the retained seam/quotient mapping.

Own one exact domain admission operation:

1. Compute exact signed loop areas and coordinate order/ranks. Require positive
   outer and negative hole winding without silently changing source traversal.
   Any intentional orientation adapter records its reversal and original use
   order; carrier/face physical orientation remains separate.
2. Build a closed segment-box BVH. Query segment candidates and apply exact
   orientation/on-segment tests. Nonincident contacts and proper intersections
   reject. Adjacent segments are not blanket-exempt: their intersection must
   be precisely their common endpoint, excluding collinear backtracking.
3. Use the same indexed segments for a rightward ray query: prune nodes whose
   half-open y range cannot contain the probe or whose maximum x is left of it.
   At leaves apply the exact half-open crossing rule and orientation. A hole
   vertex queried against **all other loops excluding its own** must have total
   winding one. With noncrossing simple boundaries, positive outer/negative
   holes, this rejects outside and nested holes without an all-hole-pairs loop.
4. Retain exact domain/constraint keys and the source-to-loop occurrence map.

The orientation adapter is concrete. Retain `original_ordered_use_refs` exactly
as in the source loop and never mutate `USES.sense`. Let a be the nonzero exact
area of the concatenated original traversal, and d be +1 for an outer and -1
for a hole. Store `Loop.traversal_adapter=d*sign(a)`, restricted to ±1. For +1
the normalized site cycle is the original cycle; for -1 reverse the cycle while
keeping its first joint as the anchor. Every normalized boundary segment keeps
its original use/parameter interval and has master-chain direction
`original_use.sense*traversal_adapter`. Its source occurrence order is recoverable
without treating the normalized cycle as the original loop. Source face
orientation controls physical facet orientation separately; it is not this
polygon-domain adapter. Readback checks the normalized cycle against the
retained original uses and adapter, rather than merely checking an area sign.

Count sorts, BVH visits, candidate predicates and ray leaves. This avoids the
known explicit quadratic hole-containment scan; adversarial broadphase boxes
can still cause high candidate work, which must be reported rather than hidden
behind an unconditional `N log N` claim.

Initial CDT receives `T0*q` coordinates with one common exact dyadic integer
normalization and unit distance weights. Adapt owned integer descriptors in
memory, not a packed whole-face blob or public C ABI roundtrip. Call the actual
`bgcdt::solve` after the above admission. Do not route through `bgcdt_create`,
which runs the old `bgfd_validate_domain` hole scan again. A typed initializer
for the immutable initial predicate table may share decoding/arithmetic with
the existing API; external ABI behavior remains compatible.

Convert returned dense initial triangles once into `FaceMesh`; build the edge
index and reciprocal neighbor rows in one pass. Check exact positive chart
orientation, each protected segment's expected incident side, and all sites'
incidence while constructing those rows. This is construction, not a second
geometric certification pass. Release initial quad-edge/predicate scratch when
conversion finishes. Initial CDT predicates do not accept appended points.

## 5. Mutable face points, predicates, metric and cell state

Use append-only `ChartSite` storage during a face generation. `FacePredicates`
reads exact dyadic coordinates, with certified interval orientation filters
and owner-local exact fallback. Fix the normalization from the initial admitted
chart bounds for the generation: nested child sites stay in those bounds.
Append caches only the new site's normalized enclosure; it does not renormalize
or rebuild every existing site. A broad
exponent spread may disable a filter and use exact arithmetic. Exact zero stays
zero. This phase does not need dynamic incircle predicates or a fresh CDT after
every insertion. Fixed-M0 edge length uses exact `||T0*(q_b-q_a)||²`.

Implement the accepted canonical metric function once: source chart-rectangle
center for generation `T0`, exact cell centroid for the local proposal metric,
stable source span tie, scaled source first/second derivative evaluation and

    B = J^T J/H^2 + |II|/(8*epsilon) + 2^-40 diag(U^-2,V^-2).

Use the selected nonsingular exact dyadic factor construction from the algorithm
resolution. H and chart extents are source-derived, not current mesh density.
The positive floor is proposal regularization, never an error allocation. Keep
T0 fixed for initial CDT and LEPP. Local metrics do not change the progress norm.
Reject unsupported rank/zero-size cases through the geometry profile; never
normalize a zero normal or silently choose an isotropic replacement.

Each triangle row stores three site slots, three edge/neighbor handles, cell-map
key, immutable lineage key, sticky heuristic flag, generation and numerical/
contact state. Edge rows store unordered site pair, directed incidents,
constraint/occurrence reference, generation and candidate state. A source
constraint is not inferred from its current position. Output orientation uses
the source face/chart orientation descriptor, not a heuristic 3D normal flip.

Bounds are asynchronous values keyed by source, chart, exact sites, actual XYZ,
cell-map/bound revision and all incident boundary generations. `Pending`,
`Accepted`, `NeedsRefine` and typed failure are distinct. A stale result cannot
accept a reused triangle slot. Use a stable max queue ordered by exact M0
diameter squared, then stable cell key; failure reasons are state, not priority.
No GPU enclosure
endpoint or completion time selects the next edit. Numerical work can batch,
while topology mutation remains one face owner's deterministic operation. A
pending cell has its priority before the numerical result arrives: do not edit
a lower-priority ready cell while a higher-priority result remains pending.
Accept/remove ready accepted prefixes and classify the maximal pending cell
before selecting the next failure. This ordered frontier, rather than a queue
of whichever failures completed first, makes the edit sequence backend-stable.

## 6. One heuristic attempt, then terminal-priority LEPP

Before fallback an unspent failed target may propose only its canonical local-
metric circumcenter. Solve the two equal-distance equations as exact rationals,
correctly round each original-chart coordinate once to nearest binary64,
ties-to-even, then retain those words as exact dyadics. Nonfinite, duplicate or
inadmissible candidates spend the attempt and go to fallback. No residual-site
or off-centre routine is tried as an unspecified second strategy.

Require strict interior of that target, or an unprotected target edge whose
other incident lineage is also unspent. A circumcenter in an unrelated or spent
neighbor is rejected. An encroached original feature produces a source owner
request before insertion. Interior insertion makes three child triangles; edge
insertion makes four from its two parents. All affected descendants inherit
`heuristic_spent=true`, including accepted children. Conformity splits preserve
each replaced parent's existing flag. No acceptance resets a flag. This enforces
at most one heuristic split per ancestry path, not merely one per queue entry.

For every remaining failed ordinary target:

1. Pick its longest exact-M0 edge with stable endpoint-key tie breaking.
2. If the edge is an original boundary, request its master interval midpoint and
   end this generation. Other protected quotient boundaries use their declared
   geometry-owner refinement recipe; do not add a private source node.
3. Otherwise cross to the unique neighbor. If the incoming edge is also longest
   there, including a tie, this pair is terminal. Otherwise its diameter is
   strictly larger and the walk continues.
4. Bound the walk by current live triangles. A repeated triangle is an internal
   topology error, not an invitation to spend the entire large work budget.
5. Split only the terminal edge at its arbitrary-precision exact chart midpoint,
   simultaneously in both incidents. Do not round it to binary64 and lose the
   contraction argument. If the marked target remains, walk from it again.

For `(A,B,C),(B,A,D)` the shared-edge split is
`(A,P,C),(P,B,C),(B,P,D),(P,A,D)`. Interior insertion into `(A,B,C)` is
`(A,B,P),(B,C,P),(C,A,P)`. Stage the new site, canonical XYZ, positive children,
edge incidences and required allocations first. Commit removal/replacement only
after the transaction can complete; on resource/cancellation failure return a
typed failed generation, never a half-valid live mesh.

Delete old rows/contact records, repair unchanged exterior-edge neighbor
handles and site incident hints, install child adjacency and bounds jobs, and
invalidate old edge/cell candidate generations. Requeue every child and changed
facet/contact neighborhood. Outside neighbors retain their geometric maps and
bounds; only adjacency/candidate handles change. Preserve all source occurrence
references. Do not repair site hints by repeatedly scanning all face triangles.

No flip or vertex motion occurs in refinement. The transformed-norm diameter
and single-mark closure argument is in the accepted algorithm resolution. This
plan carries its regularity/strict-feasibility qualifications and makes no mixed
algorithm linear-work or optimal-anisotropy theorem claim.

## 7. Exact actual-facet contact operation

Use the **actual emitted binary64 XYZ words as exact dyadics**. A finite facet
must have exact nonzero 3D cross product. A physical degeneracy caused by output
rounding is a precision/unresolved result, not an omitted triangle. Chart area
positivity alone does not decide this.

The definition owner maintains a packed indexed conservative BVH of actual
facet AABBs, including touching boxes. AABB extrema are exact min/max of emitted
words. Initial bulk build uses 16-bin SAH on each axis, leaf capacity four and
stable `(axis,bin)` ties. For each candidate split minimize
`n_left*area(box_left)+n_right*area(box_right)`, with exact dyadic comparison or a
certified filter. Centroid bin assignment uses exact comparisons against bin
boundaries. Empty/degenerate partitions use an exact-centroid median with facet
key ties. Beyond depth `2*ceil(log2(max(2,N)))+8`, use balanced median splits.
These are algorithm constants, not watch-derived capacities. Mutable deltas
refit/remove/insert leaves under the same owner; the insertion descent minimizes
the exact increase of this box-area cost. Rebuild at a contact barrier if height
exceeds the above bound or changed leaves since the last build reach
`max(1,ceil(N/2))`. Count visits/candidates, tree updates and rebuild work. No
all-facet scan after each insertion.

For each candidate pair use this complete exact classification:

1. Compute each vertex's signed determinant against the other plane. A strict
   same-side separation proves disjoint. Keep all exact-zero vertex/edge cases.
2. For nonparallel planes, intersect each closed triangle with the other plane.
   Retain zero-plane vertices and exact rational edge crossings
   `(s_p*q - s_q*p)/(s_p-s_q)`. Project the resulting point/segment onto the
   largest nonzero component of the planes' cross product, with stable axis
   ties. Intersect the two exact closed scalar intervals, reconstructing any
   endpoint as a rational 3D point. This gives empty, point or segment.
3. For coplanar triangles, drop the dominant normal coordinate and exactly clip
   the two convex triangles. Rational Sutherland--Hodgman clipping retains
   boundary equality. Remove consecutive duplicate points; exact signed area
   distinguishes positive-area overlap from a segment/point. Parallel distinct
   planes are disjoint. Bound polygon vertex counts; do not use an epsilon.
4. Determine the allowed common simplex from **declared physical node and edge
   incidence**, including source seam/quotient records. One shared node permits
   only that exact point. Two shared nodes permit their segment only when the
   edge has declared matching incidence. No shared incidence permits no contact.
   Three shared nodes are duplicate facets and forbidden. Coordinate equality,
   same face, shared source vertex elsewhere or adjacent AABBs do not exempt a
   pair. Require the full exact intersection to be a subset of the allowed
   simplex; this catches T-junctions, overlap beyond a shared edge and crossing
   triangles that happen also to share a vertex.

The initial implementation uses certified interval filters for plane/separation
signs and exact rational fallback/classification. It may optimize the rational
intersection representation after qualified profiling; it may not replace it
with a floating epsilon test. This operation proves actual output contact scope,
not discovery of every source self-intersection hidden below epsilon.

Use explicit contact epochs within each definition. First, every face settles
its local positional/domain bounds from the same boundary snapshot. Faces
submit bounded `FacetDelta` batches and yield; the contact owner applies them in
stable definition/source/cell order. Freeze the current live facet tables at
the barrier, validate generations, then query every new-vs-live/new pair once.
Read-only candidate classification batches may run in parallel; fold the entire
epoch's results in stable pair-key order before any contact-driven edit. Retain
unchanged pair decisions under their exact facet generations.

A forbidden pair marks both incident ordinary cells for refinement; quotient
cells use their declared child recipe. The frozen marked set is dispatched to
face owners, which refine it and settle local bounds again before the next
contact epoch. Do not let arrival of the first GPU/contact result decide which
pair causes the next split. An original-edge need goes through its owner and a
coordinator boundary before another epoch; changed incident face generations
restart. Changes invalidate just affected pair rows, and a stale pair cannot
keep a new cell failed or accepted. Contact/geometry precision or source-
separation failure remains structured, bounded unresolved work; do not tune
epsilon to hide it.

## 8. Fixed-site accepted optimizer

Start only after all current cells, shared boundaries and actual facet contacts
settle. No refinement is interleaved. Candidate edges are unprotected internal
edges between ordinary cells in one common affine chart. Both old cells must
be accepted. Require an exactly strictly convex quad, no duplicate new diagonal,
two positive new triangles and identical chart-domain coverage.

Define per-site-set canonical local metric and positive area A as in the design,
and evaluate the entire rational expression

    (sum_edges d^T M_T d)^2 / (48 det(M_T) A^2)

exactly, then round once to nearest binary64, ties-to-even. Keep this finite
potential immutable for that triangle/source/site set. Reject invalid/nonfinite
metric/potential values. Compare sums as exact dyadics and accept only strict
decrease; equality retains the edge. No error term with unspecified weighting.

Prepare replacement cells `(C,D,B),(D,C,A)` for old `(A,B,C),(B,A,D)`. Evaluate
both actual-cell/domain bounds without mutating live connectivity. The definition
contact owner approves their actual contacts against the **current** live
facets, excluding the two replaced facets. Optimizer commits use one stable
per-definition `(face key,edge key)` candidate queue, independent of preparation
completion order; concurrent prepared proposals must be rechecked
after overlapping/current-facet revisions change. Two separately safe proposals
must not both commit against stale opposite meshes.

On approval commit two rows, the diagonal, four exterior neighbor handles and
the BVH delta. Queue the new diagonal and four exterior edge candidates only;
neighbor triangle potentials are unchanged. On rejection discard scratch and
leave the old accepted mesh intact. No ordinary flip handles singular fan maps.
A boundary-generation advance cancels this optimizer, restarts the affected
domain and makes all old prepared edits stale. Strict decrease on the finite
fixed-site triangulations proves this phase ends, not that it finds a global
quality optimum.

## 9. Singular quotient interface and publication

The geometry owner must provide each admitted pole/apex quotient region's
source coverage, collapsed-edge/node identities, noncollapsed boundary chain,
actual nonzero fan facets and a legal local subdivision recipe with progress
conditions. `FaceMesh` stores those cells and their source-to-facet associations;
it does not feed a UV-zero triangle to the ordinary CDT/LEPP/flip primitive.
Ordinary neighboring cells consume the same noncollapsed chain. Any requested
physical-boundary refinement still routes to its original edge owner. A general
polar trim or unknown NURBS singularity has no guessed adapter: return the named
unsupported/unresolved profile.

Pole physical orientation is also explicit. For the role order
`(P_pole,Q_j,Q_j+1)` and the source map
`u=u_j+xi*delta_u`, `v=v_pole+rho*(g(u)-v_pole)`, establish the nonzero regular-
side sign `s_J=sign(delta_u*(g-v_pole))` by the analytic source enclosures.
The role triangle order is negative relative to the `(xi,rho)` parametrization
of Phi: `Phi_xi cross Phi_rho = -rho*N_role`. Therefore with source face
orientation `o_F`, retain role facet order exactly when `-o_F*s_J=+1`; otherwise
swap its two cut roles. This applies to north/south sphere caps and either
admitted cone sheet. Do not use a guessed 3D normal or blindly apply only the
face sign. Cell/interpolant roles remain fixed independently of the facet row:
match PhysicalNode keys to record/reconstruct its role-to-row permutation.

Emit current immutable face results only after all required cells/bounds and
actual contacts settle. Runtime's final `NativeMesh` retains original inventory,
physical nodes/generators, ordered source edge chains, directed coedge/chart
occurrences, actual cells/maps/facets, quotient ancestry and boundary generations.
Publication checks directly assembled incidence and completion state, not an
extra second geometric oracle. No missing face, unsupported part or stale
generation is labeled a complete result. Independent full-mesh readers remain
development oracles, not product callbacks.

## 10. Delivery sequence, direct REDs and performance families

Implement this owner as one integrated conceptual unit after whole-plan review:

1. Incidence, exact physical endpoint mapping, interval owners, common joints
   and immutable snapshots; consume the geometry owner's trace/star contracts.
2. Indexed domain admission and typed transformed initial CDT reuse; build the
   mutable face representation and append-site predicates.
3. Canonical metrics, one heuristic, exact transactional LEPP and generation
   restart; submit actual cells to geometry and deltas to the contact owner.
4. Actual exact facet classification/BVH and settled contact refinement; add
   the accepted fixed-site optimizer and quotient adapters only with their full
   source/cell-map records.
5. Complete result publication through runtime; review and fix actual code;
   then qualified independent families and complete CPU/P100 consumers.

Before product implementation, author focused failing cases that inspect actual
output via the independent exact chart-chain/physical-contact/source readers:

- Descending original edge range with `curve_same_sense=false`, independent
  coedge reversal, closed-edge repeated endpoint and same-face seam occurrences:
  source parameter endpoints must still match the literal vertex IDs once.
- Incoming/outgoing nonlinear guides needing one common joint; a new curved
  edge sample off its old chord; stale in-flight result after edge-generation
  change; all incident faces must consume the identical physical chain.
- Multiple holes, protected collinear samples, nested/outside holes, adjacent
  backtracking and nonincident endpoint touch. Multiple outer inventory must
  produce the explicit current-profile result, not omit a component.
- Appended dyadic sites beyond binary64 midpoint resolution; longest-edge
  incoming ties; a neighbor's longer edge requiring a genuine LEPP walk; spent
  target/neighbor heuristic guards; no flip may undo the refinement forest.
- Literal exact 3D contact answers: disjoint, proper crossing, vertex/edge touch,
  shared vertex plus extra intersection, shared edge plus area overlap,
  coplanar containment, T-junction and coordinate-equal foreign source nodes.
- Equal/adjacent rounding potentials and exact sum ties; rejected bounds or
  contacts leave the old mesh; two prepared flips with mutually conflicting
  new facets cannot both commit from stale pair states.
- Existing admitted sphere/cone seam/pole cases must retain full quotient
  coverage and only nonzero physical facets; unsupported charts remain honest.

Use specification/property/model answers, not status-only triangles/counts.
Resource/cancellation/ASan tests attack their distinct memory/lifetime classes;
do not build another certificate or test the reader with a second reader.

Select independent performance families **before** watch tuning:

- Nonlinear parameterizations of a straight shared edge and the exact planar
  square; compare physical profile against declared original-UV control.
- Rotated ruled ridges over kappa/epsilon sweeps, with fixed anisotropic norm
  and an explicitly isotropic control; record output density, edge samples,
  restriction work and contacts, not just elapsed time.
- Scaling independent holes/collinear constraints and long LEPP chains; report
  domain ray/candidate work separately from initial CDT, append/refinement and
  any boundary restart cost. No hidden repeated quadratic admission.
- Multi-span C0 surfaces, varying positive weights, acute/tangent star families,
  seams/poles and small features near the output precision floor.
- Large separated facet sets and narrow/copanar contact candidates: BVH visits,
  actual exact classifications, refits/rebuilds and memory, including adverse
  broadphase cases. Positive-clearance sweeps separate feasible refinement from
  unresolved/incompatible geometry.

Record complete native CPU/P100 calls and phase/work/live-memory costs with
actual results and failures; there is no numeric throughput SLA or watch-ID
dispatch. Apply suitable pinned external examples with their real semantics.
Only after integrated geometry/review and those general mechanisms work, mesh
the complete watch and render assembled/exploded views. Reference CPU timings
retain their own actual scope and incomplete inventory until resolved.
