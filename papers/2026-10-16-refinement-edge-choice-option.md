# Deferred option: choose splits by the failing bound

Read-only derivation from the current `_surface_metric`, `_surface_error`,
`_positive_midpoint` and `mesh_imported` split/adjacency code. No product edit,
CAD execution, test or benchmark was performed. **This identifies a performance
possibility, not a geometry correctness defect or measured OM10 attribution.**

## A concrete local cylinder state

Take the standard saved unit-axis cylinder
`S(u,v)=(cos(u),sin(u),v)` with radius1. The current conservative metric is
`mu=2,mv=1`, because it uses `norm(X)+norm(Y)`. Its current surface bound is
`E(T)=du^2/4`, independent of V; `muu=2,muv=mvv=0`.

Consider the positive UV triangle
`A=(0,0), B=(1/8,-64), C=(1/8,64)`. All coordinates are exactly representable
dyadics; its determinant is16. Assume these are interior mesh edges rather than
protected source boundary edges. With zero canonical displacement:

- Its error bound is `1/256=.00390625mm`, exceeding the current selected cell
  allocation `.75*.005=.00375mm`.
- The current length scores are `score(BC)=16384` and
  `score(AB)=score(AC)=4096+1/16`. Therefore BC wins strictly.
- Splitting BC at `(1/8,0)` leaves both child U ranges exactly1/8. Both retain
  the same failing bound1/256. The local failing-triangle count rises from1 to2.
- Splitting AB at `(1/16,-32)` gives one child with U range1/8 and one with
  range1/16. Their bounds are1/256 and1/1024, respectively; the latter passes.
  AC gives the symmetric result. The local failing count remains1.

This calculation uses the existing acceptance bound, not a weakened bound or
sampled residual. It separates scheduling from the actual cylinder geometry:
V is an exactly linear direction, while the chosen error bound depends on U.
The code-derived two-child comparison does not establish a complete mesh's
total construction cost. Nor does it prove this local state arises in the
current initial triangulation. Original boundary sampling, protected edges and
the preceding global Lawson pass can prevent it. Refinement subsequently does
not run Lawson after every split, so initial Delaunay reasoning is not a blanket
exclusion of later states. Actual stage counters must answer reachability and
frequency before an implementation unit is selected.

## A bounded candidate chooser

For each of the failing triangle's at most three eligible nonboundary edges,
read its actual current incident triangles. Use the same shared exact-positive
midpoint routine. Construct prospective UV children without publishing nodes,
physical keys or triangle IDs. Evaluate the unchanged `_surface_error` plus
the correct maximum inherited corner displacement; the proposed newly evaluated
interior point has the same zero computational displacement convention as the
current constructor. Keep all actual original boundaries untouched.

Compare candidates by a deterministic tuple such as:

1. Number of prospective children still exceeding the unchanged allocation,
   adjusted for the number of currently failing incident parents replaced.
2. Summed positive bound excess, adjusted by the replaced parents' excess.
3. Maximum prospective bound, then existing length score and stable edge key
   as deterministic ties.

The exact tuple remains a design choice to measure. **Maximum child bound alone
does not distinguish the cylinder example:** both candidates retain one child
with1/256. A curvature-only score is cheaper, but a prospective-child bound
can account for actual span/knot changes and inherited displacement directly.
At most two incident triangles per ordinary manifold interior edge means at
most four prospective children per candidate; validate that incidence property
rather than silently omitting additional owners. Cache immutable derivative
data, not a bound detached from the candidate's exact coordinates.

No monotonic-progress promise follows for every geometry. If all choices predict
no immediate reduction, retain a deterministic eligible split under the existing
construction/resource limits rather than inventing a tolerance. Forecasting
later descendant benefit is a different, larger search problem.

## Shared adjacency and failure risks

Both sides of an interior edge must receive the **same** midpoint. A candidate
that helps the current triangle can increase its neighbor's failing count, so
score all actual owners. Exact midpoint positivity must hold for every owner;
floating midpoint fallback and strict-zero behavior remain unchanged. Recheck
chosen incidence before publication if a future parallel scheduler permits
concurrent changes. The present serial loop provides a simpler snapshot.

Predicting children must not mutate active incidence, queues, source maps,
physical keys or accounting. If a candidate's bound evaluation fails, report
that failure under an explicit policy; do not swallow an invalid carrier/domain
exception as a low-priority edge. Allocate/publish once after selection; failure
must not leave half a split or reuse speculative keys. Every replacement is
queued and undergoes normal final acceptance even when its prediction passed.

Preserve the original work cap and geometric-resolution guard semantics. Using
a different edge may encounter the existing metric floor earlier; it must not
be bypassed because predicted error looks favorable. There is also no guarantee
of better aspect ratios: long triangles in a genuinely linear direction can
be acceptable for this chord-error contract, while other future consumers may
need independent angle/conditioning requirements.

## Discriminating experiment before selecting work

First measure actual failing triangles, selected-edge scores, selected-child
bound changes, incident-neighbor changes, generated nodes, stale queue storage
and carrier family. Retain a bounded actual case, not only the analytic example.
Compare complete refinement wall, bound-evaluation wall and final vertex/triangle
counts under unchanged source samples, tolerances, caps and physical/source/domain
oracles. A chooser evaluating up to12 child bounds can lose on easy cases even
while reducing splits; report both costs. Censored trials remain censored.

This option follows the current native fixed-domain unit. It is neither part of
its accepted scope nor permission to alter refinement during that delivery.
