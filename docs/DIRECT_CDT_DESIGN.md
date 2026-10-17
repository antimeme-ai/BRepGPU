# Owned direct sampled-domain CDT constructor

Design accepted by root for implementation on 2026-10-16, including the signed-cap refinement below. The operator selected strong algorithms
rather than a native port of the old ear/Lawson loop. This unit implements the
static CPU contender in DIRECT_CDT_STRATEGY.md, while GPU and full constrained
Chew construction remain serious architecture alternatives.

## Contract and boundary

Root owns C ABI, binding, exact input/domain validation and resource aggregation.
The core receives an immutable Predicates owner, shared bgfd::Budget, flattened
uint32 point slots, bgfd_loop ranges, deterministic uint64 seed and CoreStats.
It returns a pmr vector of dense point-slot triangle triples. Root retains and
publishes original uint64 source IDs. Every original site belongs exactly once
to the outer/hole words. Original simple outer CCW, strictly disjoint interior
hole CW, protected collinear samples, constant positive diagonal metric and
no-Steiner constraints remain unchanged. No watch IDs/counts/shape assumptions.
The core defensively checks index/range arithmetic before dereference, but does
not duplicate root's complete geometric domain reader.

The shared predicate interface is bgcdt::Predicates: size(), compare_axis(a,b,
Axis), orient(a,b,c), incircle(a,b,c,d), compare_line_distance(s0,s1,a,b).
All signs use original exact values and actual metric; ghosts never reach these
functions. Predicate implementation is a separate owner. All allocations use
the shared budget resource; checked count products precede allocation. Explicit
stacks and indexed handles avoid call-stack growth proportional to cavity size.
Only D&C recursion remains, with median-balanced logarithmic depth.

## Initial DT: median alternating cuts and indexed quad-edge arena

Use an owned packed quad-edge arena for this construction phase, not individual
heap allocations. One undirected edge owns four consecutive records; rotation
and reversal are index operations. Each record stores origin slot (primal edges)
and origin-next index. Splice swaps two primal and two dual origin rings;
connect joins the destination of one edge to the origin of another; deletion
unsplices both directions and recycles the complete four-record slot. Charge
all walks, comparisons, splices and allocation/high-water work. There is no
coordinate-valued point at infinity or arbitrary supertriangle.

Median partition alternates (X,Y) and (Y,-X) comparison frames. Both are
orientation-preserving; only sort/tangent ordering changes. Exact coordinate
ties use the secondary coordinate, then dense input slot for determinism. Child
hulls are walked to find the extreme outgoing hull handles in the parent's
frame, costing O(h) per child and O(n log n) over all recursion levels. This
avoids fragile assumptions that horizontal cuts inherit vertical extreme handles.
No metric-space coordinate construction is needed: positive diagonal scaling
preserves both axis orderings; orientation/incircle use the exact metric view.

Two-site leaves produce one edge. Three-site leaves produce either a positive
triangle or two consecutive collinear edges, retaining the middle point. Merges
find the lower common tangent, remove locally illegal side candidates by strict
positive incircle, and stitch the dividing chain until both candidates finish.
A constructor-only symbolic-lift tie policy refines exact cocircular zero as described below; raw predicates retain exact zero and original coordinates never change. Collinear sites remain vertices in hull rings and are
never deduplicated. Structural hull/tangent/dividing-chain bounds derived from live edge/site counts additionally reject cycles before a large default work allowance can conceal one. Structural work budgets abort stalled traversal rather than
claiming a completed triangulation.

After the final merge, enumerate primal left-face cycles once, emit only bounded
three-edge positively oriented faces and build the recovery mesh. Exterior cycles
remain exterior; zero/negative-area finite faces are never published as initial
triangles. Account conversion wall/memory separately. Initial construction never
runs a generic global Lawson repair.

## Recovery topology and constraint strip

The recovery mesh uses dense triangle slots, three site slots per triangle,
recycled triangle slots and an owned fixed-capacity open-address index from
oriented endpoint pair to triangle/local edge. Reverse-key lookup supplies the
adjacent triangle. Source-site incident-triangle hints and degree counts support
segment location without scanning unrelated triangles. Deletion removes all
three directed keys, updates hints, and returns the slot; creation verifies no
same-directed duplicate, updates keys/hints, and joins existing reverse uses.
Hash probe and backward-shift deletion work are budgeted. Storage is bounded
by the live planar mesh and explicit cavity capacities, not historical flips.

Process every consecutive original loop pair as a protected segment. Shuffle
segments with a specified seeded generator and unbiased bounded draws; record
the seed. If the segment already exists, mark it protected. Otherwise locate
the intersected endpoint wedge by rotating the star of the endpoint of lower
degree, then walk exact crossed edges through neighboring triangles. Root
validated that no third original site lies in the segment interior and no
protected constraint can be crossed. Such a crossing is an invariant failure.

Record the crossed triangle strip and left/right boundary occurrence chains.
Append a chain vertex only when it changes from the immediately preceding site;
retain nonconsecutive repeated occurrences caused by dangling existing segments.
Reverse the left chain so each cavity has CCW boundary order with the newly
inserted segment as its closing edge. Remove the crossed triangles from the
main mesh only after its complete strip/boundaries are captured. Main mesh may
be temporarily incomplete internally; no partial output is public.

## Actual Shewchuk–Brown cavity algorithm

Use a fresh separate scratch triangulation for each cavity. Its vertices are
**cavity occurrence slots**, mapped to immutable original point slots. Repeated
geometric sites have distinct scratch occurrences; an edge overlapping an edge
outside this scratch does not collide with the main index. Temporary multisheet
subpolygons are not ordinary embedded polygon domains.

For a cavity word v0..v(m-1), protect its endpoints. Build prev/next occurrence
links, then repeatedly choose a uniformly sampled remaining internal occurrence
that is not strictly closer to the segment support line than both neighbors.
The predicate compares absolute exact orientation values, so no square roots or
rounded distances appear. Remove it from the links and save its two neighbors.
The final endpoint/remaining-internal triangle seeds scratch. Reinsert deleted
occurrences in reverse removal order with their saved neighbors.

InsertVertex(u,v,w), implemented with an explicit task stack, looks up the
triangle (w,v,x) on the opposite directed scratch edge. If absent, create
(u,v,w). If present, retain the edge only when incircle(w,v,x,u)<=0 **and**
orient(u,v,w)>0. Otherwise delete (w,v,x), recurse through (u,v,x) and (u,x,w).
If the deleted triangle's incircle<=0, mark u,v,w,x. These are orientation-driven
crossed triangles, not ordinary incircle-driven deletions. Mark only occurrence
slots, never every source site sharing coordinates.

After each top-level insertion, find the fan of current scratch triangles whose
three occurrence vertices are marked. Traverse the new vertex's incident star;
this bounds discovery by its actual degree instead of scanning the whole scratch.
Extract the marked fan's oriented boundary and remove only that fan. Retriangulate
it with **Chew's separate randomized vertex-deletion algorithm**: random cyclic
vertex deletion precomputes insertion neighbors, seed the final three vertices,
then reinsert in reverse order using incircle-driven local deletion. The production helper `detail::signed_cap` owns this algorithm and is also called directly by the native signed-seed fixtures; no test copy or test C ABI exists. It receives original point slots and returns boundary-occurrence indices, preserving local occurrence identity. Each production cap receives one derived seed from the owning seeded stream. Use a fresh secondary scratch index and copy the completed local faces back. This is the
secondary convex-lifting fan procedure in the paper, not recursive CavityCDT or
arbitrary ear clipping. Unmark touched occurrences through a touched list.

The secondary fan's original geometry need not be convex in UV. Its applicability
comes from the crossed-triangle lifting-cone condition proved by Shewchuk–Brown,
not a convex-polygon test. Before a top-level insertion, identify the unique saved
neighbor farther from the segment support line than u. Every orientation-driven
crossed deletion must contain that old fan apex. The marked fan's boundary must
exclude that apex **by original point identity**, not merely occurrence index.
Violation returns an invariant error, never an alternative triangulation.

The secondary scratch stores signed oriented **lifted-cap facets**. Around the
old apex v, translate metric coordinates so lifted height of p is |p-v|²>0.
Central projection onto a positive-height plane turns the cone into a convex
projective polygon. Its facet orientation is the signed incircle determinant
relative to v, not XY orientation. Therefore preserve cyclic boundary words and
raw determinant orientation even for clockwise or XY-collinear seed faces;
never normalize them by an XY sign. Three distinct XY-collinear sites lift to a
noncollinear vertical facet. A chosen seed facing away from the apex is an
invariant error. Chew insertion deletes cap facets by signed determinant visibility
and attaches the new boundary facets; no SB fold/XY-positivity guard is added to
this separate convex-cap algorithm. All-four-XY-collinear comparisons remain
zero and retain the existing facet while attaching the new signed facet.

An original source site may have no incident-triangle hint during an internal delete/add transaction. Main star location runs only after both cavity outputs restore every strip boundary site; scratch star traversal runs only for the fresh inserted vertex after its full insertion. A selected secondary fan shares that new vertex, so its distinct excluded old apex is not an interior fan vertex. No star traversal runs during the deletion window. This is a caller-lifecycle invariant, not a generic guarantee for arbitrary intermediate mesh state.

The raw predicate API continues to return true exact zero. A separately named
constructor topology helper consistently refines zero incircle decisions from
the initial D&C onward by symbolic heights `height_i + epsilon**(rank_i+1)`.
The global rank is the immutable dense original point slot, shared by repeated
occurrences. For rows(a,b,c,d), inspect height cofactors in increasing rank:
`+orient(b,c,d), -orient(a,c,d), +orient(a,b,d), -orient(a,b,c)`. The first nonzero
cofactor chooses a legal cocircular diagonal. Repeated point rows yield zero
as their equal symbolic coefficients cancel; four collinear sites also stay zero.
No orientation zero or coordinate is perturbed.

The lifting argument justifies signed-cap representation. Weak/projectively
repeated degeneracies and finite pseudo-random schedules do not acquire an
unconditional expected-time guarantee merely by citing the paper. Qualification
must exercise their actual bookkeeping; a final positive-orientation assertion
may reveal a valid-input defect requiring diagnosis rather than certify success.
No generic polygon or global Lawson fallback conceals such a failure. Only the
completed original cavity must have strict positive source triangles and exactly
m-2 faces before copying back.

Copy each completed cavity's faces back only after checking original point
mapping and strict positive orientation. Repeated original sites within a final
triangle are an invariant failure. Mark the new source segment protected.
Maintain both sides together internally; an error discards the complete solve.

## Domain selection and publication

The initial/recovered CDT triangulates the convex hull. Remove outside material
without a centroid winding scan per triangle: for each directed original boundary
edge, its right-hand incident face is void (outer exterior or hole interior).
Flood through unprotected edges from all such void seeds, retaining material
faces. Hull sides without an incident triangle are the unbounded void. Original
loop winding makes the same directed rule apply to outer and holes.
Require all original boundary edges present, every original site retained, all
remaining triangle orientations positive and T=N+2H-2 before publication. Root's
independent complete domain/embedding/local-CDT reader remains the oracle; core
checks are invariant defenses, not a second certification format.

CoreStats records merge_steps, crossed_edges, cavity_occurrences, cavity_insertions,
secondary_deletions, max_cavity, quadedge_peak, triangle_peak and constraints. Quadedge peak counts active undirected four-record groups; triangle peak counts active faces in one mesh, not bytes. Predicate stats
remain with Predicates. Expose zero-policy/seed and phase timing at root boundary.
Additional counters record location/hash/secondary work and initial/void face counts. Native stage nanoseconds separate initial construction, conversion, recovery and selection; ordering is an explicit subset of initial construction, not an additive independent phase. Shared resource high-water bytes cover all simultaneous owners. Budget failures are distinct resource errors with no partial triangles; input
geometry errors and internal invariant failures stay distinct under root's ABI.

## Guarantees and limits

Alternating-cut initial DT aims at O(n log n) geometric work. Genuine linear
expected cavity recovery plus random segment order has the literature's expected
O(n log n+n log²k) whole-construction bound under its assumptions. Finite seeded
pseudorandom schedules and exact degeneracy handling require qualification;
no worst-case-linear per-cavity or constant bit-cost claim is made. This hybrid
is not Chew's full O(n log n) constrained strip method. Fixed bit filters, bigint
fallback, normalization, allocation, index probes and source publication are all
real operation costs. This unit does not implement carrier refinement or GPU CDT.

## Sources

Shewchuk–Brown, original 29-page May 4 2015 author manuscript
https://people.eecs.berkeley.edu/~jrs/papers/inccdtj.pdf, Figures6–7, Sections2–4.
Acquired/read for this design; earlier study used the ten-page conference PDF.
Retained PDF SHA-256 fdb5134a3d00d95ba0f5ef3bf60ac6d20eb798b8c47dd8506868b06e568dfed8.
Initial D&C/quad-edge and indexed storage are studied in the retained Triangle,
Guibas–Stolfi descriptions and algorithm-selection report. No source code is
copied or dependency selected.
