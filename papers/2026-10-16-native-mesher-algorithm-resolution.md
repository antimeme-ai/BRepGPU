# Constructive resolution: fixed-metric refinement and fixed-site optimization

2026-10-16. Proposed replacement for B1/B3 in the algorithm review, incorporating
the design owner's selected fixed anisotropic norm and separate optimizer phase.
No product code or tests. This note specifies the algorithm that an implementation
plan can decompose; it does not claim measured performance or a general CAD
convergence theorem.

## 1. One coordinate chart and one fixed anisotropic norm per generation

A face generation owns one exact coordinate chart: original UV for an ordinary
face, the admitted physical-plane coordinates for a physical chart, or the
explicit chart of a separately constructed singular quotient cell. Preserve the
original UV map and source lineage separately. The ordinary triangle algorithm
below does not silently triangulate a singular quotient as an ordinary rectangle.

Compile the face-generation metric at the canonical center of the admitted chart
rectangle, not at an output-dependent centroid. At a knot use the incident source
span of smallest stable span index for this *proposal* evaluation. Let `J` be the
3×2 first derivative, `n` its unit normal and `II=(n·S_ij)` the symmetric 2×2
normal second-derivative matrix. Let `H>0` be the diagonal length of the enclosing
physical face-carrier box, `U,V>0` its admitted chart widths, and
`R=diag(U^-2,V^-2)`. A concrete proposed matrix is

    B = J^T J/H^2 + |II|/(8*epsilon) + 2^-40 R.

Here `|II|` is its spectral absolute value, not entrywise absolute value. The
positive chart floor only regularizes the proposal metric; it is not a geometric
error allowance or minimum-size guarantee. A physical-plane chart has its own
physical derivatives and zero normal curvature. A rank-deficient ordinary chart
must enter an explicitly admitted singular construction or return unsupported;
normalizing a zero normal is not a metric policy. A zero physical carrier box is
likewise not repaired by dividing by a guessed scale.

Use stable scaled 2×2 spectral arithmetic. Approximate its positive square-root
eigenvalues and rotation entries by 53-significand-bit dyadics, retaining separate
exponents within the admitted arithmetic profile. Form a factor `T0` as an exact
dyadic product `diag(s1,s2) * [[c,s],[-s,c]]`, with `s1,s2>0` and `c²+s²>0`.
This representation has positive determinant by construction; still check the
actual expanded dyadic determinant before admission. Keep factor products exact
rather than rounding them into a nearly singular matrix. An unrepresentable
factor is a typed precision/resource result, not a secret switch to an isotropic
ordering. The scalar normalization of the factor can be a power of two, which
does not change length comparisons or triangulation.

Set `M0=T0^T T0` and retain it unchanged for the generation. Exact squared lengths
are `||T0(a-b)||²`. Initial CDT consumes the exact transformed site coordinates
`T0*q`, with unit diagonal metric; original chart coordinates remain attached to
every site. Positive determinant preserves orientation. A midpoint in transformed
coordinates is exactly the transform of the original-chart midpoint. This avoids
silently calling a diagonal-only metric API with a full SPD field.

Local geometry-aware proposals may use a different matrix evaluated at a cell's
canonical centroid by the same formula. That does not change `M0`, LEPP ordering,
or the fallback proof. It also does not establish a locally anisotropic Delaunay
property for the final mesh.

## 2. Nested refinement phase: a real LEPP closure

Each initial triangle starts a lineage with `heuristic_spent=false`. Children
inherit that flag. A heuristic attempt, successful or rejected, sets it true for
all affected children. It **never resets on acceptance within this generation**.
An accepted neighbor split by conformity inherits its own prior flag; a heuristic
edge insertion marks both incident parent lineages spent. Thus every ancestry
path contains at most one heuristic insertion. No geometry flip or vertex motion
occurs during this phase.

An unspent failed cell makes one named geometry-aware site proposal: its metric
circumcenter in the canonical local dyadic SPD metric `M=T^T T`. Solve the two
linear equal-distance equations exactly. Correctly round each exact rational
original-chart coordinate to nearest binary64, ties-to-even, then retain those
words as exact dyadics. This is a candidate-only rounding rule, not an
approximation guarantee. A nonfinite/unrepresentable candidate is rejected;
an arithmetic resource failure remains typed. LEPP midpoints instead retain
arbitrary-precision exact dyadics and do not inherit this candidate rounding.
Accept the rounded site only after exact location establishes strict interior
of the failed target itself, or an unprotected edge of that target whose other
incident lineage is also unspent, plus nonduplication and the protected-feature
policy. Do not insert its circumcenter in an unrelated already-spent triangle:
that would invalidate the one-heuristic-per-ancestry argument. An encroached physical feature becomes an owner
request; an inadmissible candidate marks the lineage spent and proceeds directly
to LEPP. No second residual or off-centre candidate is silently tried. A
successful insertion splits the actual containing triangle or straight edge,
not a new unreviewed constrained cavity. Every child lies inside an old parent.

For a remaining failed target triangle `K`, run terminal-priority LEPP in the
**fixed M0 norm**:

1. Select a longest edge of the current triangle by exact squared `M0` length;
   use ordered endpoint identity only to choose among its ties.
2. At an original physical boundary, issue an interval-owner request as below.
   Do not locally insert a private source-boundary midpoint.
3. At an internal edge, inspect its unique neighbor. If the incoming edge ties
   for the neighbor's longest length, declare this pair terminal immediately;
   do not walk onward merely because the neighbor's ordinary ID tie-break would
   have selected a different equal-length edge.
4. Otherwise continue into that neighbor. Its diameter is strictly larger.
5. Bisect a terminal internal edge at its exact chart midpoint, simultaneously
   replacing both incident triangles. The edge is longest in **both**, so no
   neighbor is forced along an arbitrary nonlongest edge. The local mesh stays
   conforming after this transaction.
6. If target `K` is still live, walk again from `K`. Continue until it is removed
   by a bisection or an original-boundary request ends this face generation.

The walk is bounded by current live triangles; a repeated triangle indicates an
internal topology error. Charge visited edges and actual splits. This is a real
Rivara/LEPP algorithm, not 'bisect a failed triangle and force every neighbor on
whatever side was selected'.

### Why its local progress argument applies

Apply `T0` and work in an ordinary Euclidean plane. Each nonterminal LEPP step
strictly increases diameter, so a walk ends in a finite current mesh. Every
terminal bisection is a longest-edge bisection of every replaced triangle.
The midpoint half-edge has length at most `D/2`; its median has length at most
`sqrt(3)*D/2`. Therefore after two successive longest-edge bisections along any
child ancestry its diameter is at most `sqrt(3)*D/2`. One intervening heuristic
split preserves nonincrease because its site lies inside the parent; the sticky
flag ensures there is at most one on an ancestry path. Subsequent diameters tend
to zero. Fixed invertibility of `T0` makes original-chart diameters tend to zero
as well; the condition number affects constants, not this conclusion.

A single-mark closure cannot indefinitely postpone the unchanged target. While
it remains, every terminal edge is at least as long as its longest edge. New
edges from a terminal pair have length at most `sqrt(3)/2` times that terminal
length. Starting with a finite mesh and its finite maximal length, only a finite
depth of this finite-branching edge-production tree can produce edges above the
unchanged target threshold. Every split removes its old terminal edge. Thus the
closure finishes, unless a physical-boundary request deliberately returns to the
outer scheduler. The current 2026 primary LEPP analysis independently supplies a
formal finite single-mark closure under this terminal-priority convention.

For finite *whole inner refinement*, additionally require a uniform sufficient
chart size `delta>0` below which every actual cell passes its positional/domain
and required local-contact tests. This follows only on admitted compact regular
pieces with strict feasible error margin, controlled boundary discrepancy,
positive weights and the required source separation/protected stars. A failed
target then has diameter at least delta; all closure splits have at least that
diameter. Infinite nested refinement would give an infinite ancestry path in a
finite-branching forest, contradicting diameter convergence. No such sufficient
size is claimed for arbitrary singular, unresolved or infeasible source.

Do not import a cumulative near-linear closure theorem for the *mixed* algorithm
with geometric insertions, face restarts and singular cells. A pure fixed-mesh
LEPP theorem has a narrower hypothesis. Nor does this argument prove optimal
anisotropic output size. The fixed curvature-aware `M0` deliberately reaches for
anisotropy before metrics; the rotated-ridge kappa/epsilon sweep versus an
isotropic control must determine its practical effect before watch tuning.

## 3. Original boundaries: a separate monotone interval algorithm

A face requests refinement of an existing original-edge interval, not permission
to add an arbitrary private sample. The edge owner bisects that interval in its
master dyadic parameter tree, merges duplicate requests, and creates one shared
XYZ/site identity. Original source breakpoints and approved endpoint/star sites
are roots of this interval forest. All incidents consume the same new snapshot.

A new point on a curved source trace is generally off its old chart chord.
Cancel the affected face generation, discard its mutable connectivity and any
optimizer work, rebuild the complete polygon domain/initial CDT from the settled
snapshot, and retain immutable geometry caches. Do not call this a collinear
triangle split. Requests on unrelated edges/faces do not cause a global restart.

The inner proof restarts with that finite new domain. An outer success argument
needs a separate sufficient source-interval size and a request rule that stops
once that requirement is met. Master midpoint trees provide parameter contraction;
monotone generation numbers alone do not. Source offsets/precision floors can
make that requirement infeasible. Report witnessed infeasibility or bounded
unresolved work; do not promise that any sequence of caller-selected dyadics
eventually settles, or that budgets prove successful convergence.

## 4. Exact replacement/update sets

For a strict interior site P of positive `(A,B,C)`, remove that triangle and add
`(A,B,P),(B,C,P),(C,A,P)`. For midpoint P of an internal edge with incident
triangles `(A,B,C),(B,A,D)`, remove both and add
`(A,P,C),(P,B,C),(B,P,D),(P,A,D)`. Require exact positive orientations. Protected
source edges enter the outer scheduler instead. No coordinate-proximity weld.

Delete old triangle/incidence/BVH rows, install child rows and new internal edge
records, and replace the old incident-triangle handle on each unchanged exterior
patch edge. Preserve its original constraint/occurrence record. Update live
incident hints for affected sites, child lineage flags, location anchors and
triangle/edge generations atomically at the face-owner boundary. Remove the
old physical facet bounds and install the actual new ones before contact checks.

Only new triangles need geometric metric/potential/bound evaluation: outside
neighbor coordinates and triangle maps have not changed. Requeue all new cells,
and refresh candidate/contact records for each new facet plus the old/new BVH
neighborhoods whose pair records were invalidated. The queue stores owner,
generation and cell identity; stale rows cannot mutate recycled slots. Numerical
cache updates are not topology changes and do not alter already-defined metrics.
At boundary restart every old face job/result/queue row is stale, even if a site
or carrier happens to have the same coordinates.

## 5. Only then: a finite fixed-site optimizer with an actual potential

Enter optimization only when all face cells and required physical contacts are
accepted. Keep the site positions fixed. For an ordinary triangle in one common
chart, compute the source-derived SPD metric `M_T` at its exact centroid, using
the same stable sampling/regularization rule as above; evaluate in the CPU owner
for both CPU and P100 execution policies. Canonicalize vertex order by rotating
the positive cyclic order to its smallest stable node ID. This is a pure function
of original source/chart and these sites, independent of caches, queue order,
neighbor triangles and previous flips.

For chart edge vectors `d1,d2,d3` and exact positive chart area `A`, choose the
shape-only potential

    Phi(T) = round_binary64((sum_i d_i^T M_T d_i)^2 /
                           (48 det(M_T) A^2)).

Evaluate this entire expression as one exact rational using the retained dyadic
factor `M_T=T^T T`, then correctly round once to nearest binary64, ties-to-even.
In exact arithmetic it is one on a metric-equilateral triangle. Geometry error
is a hard acceptance constraint, not an unspecified weighted term in Phi.
Reject a flip if any required metric/potential computation is nonfinite or lacks
positive determinant/area. Keep the computed finite binary64 potential immutable
for that triangle/site set. Compare `Phi(new1)+Phi(new2)` against the old sum as
exact dyadics; an equal sum does not flip. This avoids an epsilon-dependent
potential tie and says precisely which function terminates the fixed-site loop.

A candidate edge must be internal and unprotected, its two old cells accepted,
and its quad strictly convex in the common exact chart. Require two positive
new cells, no duplicate new diagonal, complete same-domain coverage, both new
actual-cell plus domain bounds accepted, and all actual new-facet contacts
allowed. Only then accept strict potential decrease. Singular fan cells or cells
without this common affine chart are not passed through this flip primitive.

For old `(A,B,C),(B,A,D)`, new cells are `(C,D,B),(D,C,A)` when positive. Replace
two rows, one diagonal and their BVH entries, repair the four exterior-neighbor
handles and refresh exactly the new diagonal/four exterior edge candidates.
Neighbor potentials remain unchanged. If the flip is rejected, no topology or
acceptance state changes. Every operation preserves accepted geometry/contact;
there is no refinement interleaved with this phase. A new boundary generation
discards the optimizer and starts a new face generation.

For fixed finite sites there are finitely many triangulations, and each accepted
flip strictly decreases the global sum of the defined triangle potentials.
Therefore this optimizer ends. It does not claim a global optimum, a minimum
angle, or a variable-metric Delaunay theorem.

## 6. Revised C0/atlas choice resolves the earlier B2 graph blocker

The positional contract may allow triangles across a continuous internal C0
knot; it never promised exact crease output chains. Exactly clip each emitted
chart triangle against every intersecting original knot rectangle, triangulate
the convex clipped polygon (at most seven vertices), and restrict each original
span there. Always restrict the **same actual emitted affine L**, not new
surface-evaluated corners on the clip polygon. Take the maximum piece bound.
Derivative proposals choose one defined incident span and do not use a C2
estimate across the knot. Positional discontinuity requires explicit one-sided
matching or rejection.

This removes the need to invent C0-cut×trim algebraic junctions solely to apply
a derivative bound. It is coherent to admit the existing finite source chart,
explicit analytic seams and actually constructed pole/apex quotients, while
returning typed unsupported for a new atlas, unspecified NURBS periodicity or
unresolved NURBS singularity. Those support exclusions must qualify the broad
surface-family sentence: positive rational coefficients alone do not imply an
admitted regular chart. Any future exact-crease or general-atlas promise needs
its own complete graph ownership/construction, not a silent constructor fallthrough.

## Reference scope

Li and Yang, *Closure complexity of longest-edge bisection for triangular meshes*,
arXiv `2609.04003v2`, 2026-09-04, acquired original hash in
`whole-mesher/acquisition.tsv`: read abstract and Sections1–3, including actual
terminal-priority algorithms and single-mark termination proof. The cumulative
charging proof was not reviewed and is not claimed for this mixed mesher.
Bedregal–Rivara2014 institutional URL returned non-PDF content in this acquisition
attempt; no claim to have read its full text. The elementary transformed-norm
diameter argument above is an owned derivation, not an attributed performance
theorem. All broader literature/refimpl scopes remain in the whole-mesher study.
