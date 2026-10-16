# Direct constrained triangulation: select the algorithm before the port

The next connectivity design should construct the desired triangulation directly.
Moving an arbitrary ear mesh through the same global Lawson schedule in C++ is a
useful bounded baseline, but it is not the default architecture merely because we
already have the loop. This is a study and recommendation, not a selected product
implementation or a claim that a reference suite has passed.

## The actual problem

Admitted input is an immutable exact sampled UV polygon: one simple outer loop,
disjoint interior holes, original node/source identities, collinear boundary
samples, and a positive **fixed diagonal metric**. There are no intersecting
constraints, Steiner points, snaps, coordinate perturbations or source changes.
All original consecutive constraint edges must survive individually; replacing a
collinear run by one edge is not equivalent. Existing sampled-face validation
already excludes distinct-loop touches, repeated original slots and nonadjacent
coincident original coordinates. Internal bridge occurrences are an implementation
of ear clipping, not distinct geometric point sites for a new CDT.

A general PSLG package also handles internal dangling constraints, intersections,
overlaps and point merging. Its extra capability does not widen our input contract.
Conversely, our original source IDs are not expendable simply because a package
normally merges coincident vertices. Periodic chart occurrences that do coincide
need an explicit chart/occurrence design; do not silently feed them to an ordinary
point-set triangulator.

For exact coordinates `(X/Du,Y/Dv)` and current positive saved metric `(mu,mv)`,
the map `(u,v) -> (mu*u,mv*v)` is invertible and preserves segments, crossings,
holes and orientation. Ordinary CDT algorithms therefore apply using the exact
weighted incircle predicate already derived in the Lawson proposal. This is a
constant metric per operation, not spatially varying Riemannian triangulation.
Approximate metric-space keys may order work only; they must never decide geometry.
Coordinate ranks preserve order/boxes, not the Delaunay metric.

## What the stronger literature actually offers

**Chew 1989, complete constrained divide and conquer.** This is a deterministic
`O(n log n)` full-CDT algorithm, not simply unconstrained divide/conquer followed
by inserting constraints. It partitions into vertical strips and merges only
vertex-containing regions, avoiding the quadratic number of all strip/constraint
crossings. A sweep finds above/below bounding constraints. Partial CDTs contain
symbolic infinite vertices attached to constraint half-edges; merges remove those
vertices and stitch constrained dividing chains. The paper rotates away equal-X
sites for presentation and uses a unit-cost geometric model. An owned exact
implementation must replace that presentation convenience with a reviewed exact
event-order treatment and count predicate bit complexity. This is a serious
contender if global randomized recovery work is unacceptable. We have read the
complete paper; the symbolic strip-region topology is genuine engineering scope,
not a vague assertion that this method is too theoretical.
[Full paper](https://www.wias-berlin.de/people/si/course/files/Chew89cdt.pdf),
[DOI](https://doi.org/10.1007/BF01553881).

**Shewchuk–Brown 2013, strong incremental constraint recovery.** Their special
cavity algorithm takes expected linear work in crossed edges; random constraint
order yields expected `O(n log n + n log² k)` total construction. Uniform random
constraint order and randomized cavity deletion are different requirements.
CavityCDT applies to cavities evacuated from an existing CDT, not arbitrary
polygons. It handles repeated cavity occurrences, orientation-driven deletions,
and secondary Chew vertex-deletion retriangulation. Replacing that last subroutine
with recursive CavityCDT explicitly loses the known linear bound. Their tests
include long collinear cavities, so degeneracy is not merely ignored. Naive
recursive gift-wrapping can take quadratic work per cavity and cubic total work
when `k` scales with `n`. The full expected theorem concerns combinatorial work;
our exact-integer cost and nonuniform inputs still need separate measurements.
[Author conference paper](https://people.eecs.berkeley.edu/~jrs/papers/inccdt.pdf),
[expanded 2015 journal version](https://doi.org/10.1016/j.comgeo.2015.04.006).
The initial study read the ten-page 2013 version. Constructor design subsequently acquired and read the 29-page May4 2015 author manuscript [inccdtj.pdf](https://people.eecs.berkeley.edu/~jrs/papers/inccdtj.pdf); retained SHA-256 `fdb5134a3d00d95ba0f5ef3bf60ac6d20eb798b8c47dd8506868b06e568dfed8`. Its signed-cap preconditions and clarified Figures6–9 now inform DIRECT_CDT_DESIGN.md.

**Amenta–Choi–Rote 2003, locality with justified randomization.** BRIO partitions
points into geometrically decreasing random rounds, allowing locality ordering
inside rounds while retaining the incremental analysis. Plain Hilbert/Morton
sorting is not BRIO and does not acquire its bound. The paper analyzes an abstract
incremental framework and experiments in 3D; it does not prove that a 2D triangle
walk from the previous insertion has logarithmic location time. A design claiming
an expected bound must include the appropriate location hierarchy/history or
prove its alternative. Locality ordering remains useful without falsely upgrading
a heuristic walk into a theorem. [Author PDF](https://people.eecs.berkeley.edu/~jrs/meshpapers/BRIO.pdf),
[author bibliography](https://page.mi.fu-berlin.de/rote/Papers/abstract/Incremental%2Bconstructions%2Bcon%2BBRIO.html).

**Shewchuk 1996, practical triangle ownership and initial DT.** The controlled
same-author comparison found alternating-cut divide/conquer fastest among its
implemented initial-DT methods. Compact triangle records improved both memory
and execution over quad-edge. Ghost hull triangles represent even isolated edges
and collinear recursive subsets. These findings motivate an initial static
triangle-based D&C contender and dense native topology, not an automatic present-day
speed forecast. Constraints require additional per-edge information. The
experiment does not compare modern BRIO against the selected owned implementation.
[Primary implementation discussion](https://www.cs.cmu.edu/~quake/tripaper/triangle2.html).

**Stronger special-domain and alternative optimal methods.** Chin–Wang show
deterministic `Theta(n)` CDT for a simple polygon, preserving the value of its
supplied boundary order. We verified the publisher's result and bibliographic
record, but did not obtain/read the full algorithm. It is an investigation target,
not an implemented recipe for holes or zero-turn runs. Bridge copies do not make
a polygon-with-holes into the simple polygon required by that theorem.
[SIAM 28(2), 471–486, 1998](https://epubs.siam.org/doi/10.1137/S0097539795285916).
Seidel's 1988 optimal CDT sweep is identified explicitly by the Shewchuk–Brown
paper as a sweep generalizing Fortune; it is not Chew's divide/conquer method.
We have not acquired Seidel's original report, so no unexamined sweep design is
selected from this citation alone.

## What the actual implementations teach

Studied existing Geogram revision `390b14d5b9df949c45d07d5af208700f6be3a616`,
including its retained Triangle 1.6 source, and newly acquired artem-ogre/CDT
revision `53eff288a4e77ad1c0afe1a10f60bd2b57f877b8`.

| Reference | Mechanism read | Implication for an owned design |
| --- | --- | --- |
| Geogram `CDT_2d.cpp` | BRIO point insertion; Sloan-style intersected-edge queue, constraint flips, local re-Delaunayization; dense triangle adjacency | A useful locality/topology model; not evidence that its recovery has Shewchuk–Brown's linear-cavity bound. Its SOS incircle and intersection constructors are different policies. |
| Triangle `triangle.c` | `divconqrecurse`/alternating cuts; `constrainededge`/`delaunayfixup` interleave constraint crossing flips and fan repair, including temporary inverted triangles | It does not establish that all legal source meshes improve efficiently by arbitrary global Lawson flips. Internal temporary inverted triangles must never escape as valid output. |
| artem-ogre/CDT `Triangulation.hpp` | Initial KD-tree breadth-first ordering; crossed-triangle reuse; `triangulatePseudoPolygon` scans with `findDelaunayPoint`, then splits subranges | Explicit stack removes call-stack risk; repeated full subrange scans can still be quadratic. Initial KD ordering is not the random-round BRIO theorem. Good implementation does not make a weaker bound disappear. |

These are source observations, not executions or conformance claims.
[Geogram source](https://github.com/BrunoLevy/geogram/tree/390b14d5b9df949c45d07d5af208700f6be3a616/src/lib/geogram/delaunay),
[CDT source](https://github.com/artem-ogre/CDT/blob/53eff288a4e77ad1c0afe1a10f60bd2b57f877b8/CDT/include/Triangulation.hpp).
No reference code or production dependency is adopted. Triangle's commercial
terms remain separate from Geogram's license; CDT is MPL-2.0.

## Recommendation and decision that the next design must make

Reach for **direct exact CDT with local, output-sensitive recovery**, compact
triangle-neighbor arrays, persistent constraint/source maps, and exact weighted
predicates. Keep the old ear/Lawson producer as an independent small-input model
and measured compatibility baseline. Matching its diagonal sequence is not the
correctness specification for a newly selected direct constructor; exact cocircular
zero admits multiple valid CDTs. Define deterministic seed/tie behavior explicitly.

For a bounded first direct constructor, my preferred candidate is **static
alternating-cut initial DT plus actual Shewchuk–Brown recovery**, with uniformly
randomized constraint order and explicit seed. It avoids first-pass point location
entirely while removing the arbitrary-ear-mesh/global-flip workload. This is a
recommendation based on immutable bulk input, not a claim of stronger total bounds
than Chew. The constraint-recovery algorithm is the same conceptual owner whether
initial points arrive by D&C or incremental insertion.

The competing incremental candidate should be **random-round BRIO plus a real
location mechanism and the same strong cavity recovery**, not plain spatial sort
plus unbounded walk. This contender better matches future local refinement and
point additions. Include all ordering/location structure cost. Neither variant
beats the full constrained Chew asymptotic bound; if repeated constraint recovery
dominates broad independent families, designing Chew's complete strip method is
a more principled response than micro-optimizing gift-wrap or changing source
quality. No universal O(n log n) claim attaches to the recommended hybrid.

Important implementation obligations before selection:

- Admit zero-turn original boundary chains without omitting original nodes.
  A constraint containing a nonendpoint original vertex must be rejected or
  explicitly represented as the original consecutive constraint chain; never
  invent a new source edge silently.
- Use symbolic hull/ghost records or a proved finite enclosure, not an arbitrary
  huge floating supertriangle. Internal auxiliary entities never enter output.
- Keep exact zero distinct from a symbolic ordering device. SOS is a possible
  declared choice for nonunique triangulations; do not copy an index-based SOS
  and claim historical diagonal/ID-permutation invariance.
- For cavity vertex distance ordering, distance from a fixed segment's supporting
  line can be compared through absolute exact orientation; its common norm is
  positive. This removes square roots without changing those comparisons.
- Use owned explicit stacks, local neighbor handles and boundary-occurrence maps;
  account for all cavity/queue capacity and invalidate stale handles. Failure
  returns a resource/error result with no partial valid mesh.
- Cached exact lifts are justified only while points and metric stay fixed.
  Floating filters must be certified for original rational inputs and weighted
  arithmetic; saved-binary64 expansion code does not automatically establish
  signs of rounded rational coordinates. Exact fallback remains mandatory.

GPU construction is a parallel contender under root's separate current-literature
study. This CPU recommendation does not preselect CPU as the eventual winner.
Any GPU constructor must satisfy the same exact-source/constraint/degeneracy
contract and include packing, transfers, repair, and CPU fallback work; a fast
large unconstrained rounded point-set benchmark is not the same operation.

## Performance means eliminating bad work, then exposing the remainder

The ambition is structural: stop manufacturing an arbitrary poor initial mesh;
stop rescanning unrelated topology; avoid quadratic cavity scans; preserve memory
locality without forfeiting the randomization argument; use the cheapest justified
predicate while keeping exact signs. No microsecond deadline establishes those
properties. A finite trial budget only bounds diagnosis.

Select families before tuning: convex/cocircular rings, concave combs, long thin
strips with collinear boundary chains, isotropic irregular loops, multiple separated
holes, long constraints crossing many initial-DT edges, and anisotropic metrics.
Vary node counts and independent seeds; include wide rational/cancellation inputs,
large exact translations and arbitrary source-ID permutations. General PSLG
stress requiring internal constraints remains a separate admitted-operation test,
not disguised simple-loop coverage. Publish each family's work/memory and
whole-call regressions. Watch faces are additional real callers.

Measure normalization/order, initial construction, constraint location, crossed
triangles, cavity construction, exact orientation/incircle calls and fallbacks,
maximum operand widths, triangle allocation/reuse, output, native high-water and
process RSS. Explain scaling with crossed-edge and predicate counts, including
worst cases and failed budgets. Repeated independent seeds test randomized tail
behavior. Boundary/domain/embedding and local weighted-Delaunay readers must
qualify actual output before any speed claim; primitive arithmetic timing cannot
stand in for a complete construction.

## Acquisition and bibliography restoration

PDFs remain ignored in `papers/pdfs/`; original downloaded bytes are unchanged.
The source archive remains intact and extracted reference has no Git metadata.
See the accompanying journal for exact acquisition commands and SHA-256 digests.
Missing full texts: Chin–Wang 1998 and Seidel 1988. Their bibliographic results are
recorded above, but full algorithm acquisition remains outstanding.
