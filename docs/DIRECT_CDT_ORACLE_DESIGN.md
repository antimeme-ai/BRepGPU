# Direct constrained Delaunay: independent output oracle

Status: proposed for design review. This document selects checks and inputs before
implementation or tuning. It does not claim native CDT exists or that the current
watch pipeline is complete. No geometry was executed to write this proposal.

The claim is a complete, fixed-site constrained Delaunay triangulation of the
admitted sampled UV domain, under the actual caller's positive diagonal metric.
The claim is neither parity with the existing ear/Lawson sequence nor a minimum
triangle-angle guarantee. Source sampling and surface approximation remain
separate obligations of the complete imported-mesh consumer.

## Admitted geometry and exact values

The first unit retains the current face-domain scope: one simple positive outer
loop and zero or more disjoint, strictly contained negative hole loops. Every
input node occurs once in those loop words. Sites have distinct exact UVs and
arbitrary original uint64 IDs, including zero. Collinear consecutive boundary
samples are allowed and protected; zero-length segments, overlapping or touching
nonadjacent segments, coincident distinct sites and nested/touching holes are
invalid. No Steiner sites, internal PSLG constraints, or free interior sites are
introduced by this unit. If the product design widens that scope, update this
contract and its incidence/count checks before testing the new scope.

Interpret every supplied UV and metric value as its exact rational value. For a
binary64 metric this means `Fraction(actual_binary64)`, not parsing a shortened
decimal display or recomputing a carrier-derived metric. Require finite strictly
positive `mu,mv`. Geometry in metric space is `(mu*u,mv*v)`. This is a constant
diagonal change of coordinates, not a spatially varying anisotropic mesher or
additively weighted regular triangulation. Source UVs and IDs remain unchanged.

Each consecutive pair in each source loop is an individual constraint. Replacing
two collinear source segments by one longer segment violates this contract even
if the longer segment has identical geometric support. No boundary interpolation,
snapping, epsilon merging, or dropped protected site is permitted.

## What the existing oracles actually establish

- `imported_oracles._simple_domains` independently reads original loop UVs and
  checks exact winding, nonadjacent crossings/touches, hole containment and hole
  separation. Its pair enumeration is intentionally simple; its cost is not the
  product triangulator's measured cost.
- `imported_oracles.qualify_domain` reads strictly positive exact triangles and
  checks their entire oriented boundary chain against the original domain. It
  does not check original node IDs, manifold incidence, embedding, or Delaunay
  connectivity by itself.
- `tests/test_face_domain_external.py::qualify_external` adds actual original-ID
  inventory, each protected directed boundary segment, opposite interior edge
  uses, planar Euler triangle count, and bounded direct crossing/T-junction and
  overlap checks. These are useful existing mechanisms to reuse, not duplicate
  in a second parallel certification layer. Its current embedding enumeration
  is quadratic and suited to bounded semantic cases.
- Existing imported-meshing Lawson tests check a hand-derived diagonal change,
  unchanged constraints/concave input/cocircular input, one anisotropic change,
  and thin-strip span plus boundary incidence. They do not check a general CDT
  postcondition. Existing Python Lawson uses related integer normalization and
  translated lifts; exact agreement with it is differential evidence only.
- Physical-shell and original-carrier/curve quality readers attack later 3D
  shell/source-distance errors. A correct CDT alone does not establish either.

The missing mechanism is an independent local constrained-Delaunay postcondition
on actual returned source-ID triangles. Add it to the same semantic reader.

## One output reader, distinct obligations

Read the actual output, not native topology flags or a copied initial mesh.

1. Reuse original-domain validation. Read triangles by original source IDs;
   reject unknown IDs, repeated IDs, repeated triangles, nonpositive UV area,
   and omission of any original site. Require exactly the original site set.
2. Build edge incidence directly from those triangles. Each source boundary
   segment has its original directed single occurrence. Every other edge has
   exactly two opposite uses by distinct triangles. There are no other boundary
   edges. For N protected boundary sites and H holes, require
   `T = N + 2*H - 2`. Source-slot incidence, not equal-coordinate cancellation,
   determines preservation of protected collinear samples.
3. Reuse positive boundary-chain coverage and bounded direct embedding checks.
   Edges may meet only at their common original endpoint; reject crossings,
   nonshared endpoint-on-edge contacts, collinear overlap, and triangle interior
   overlap. These preconditions prevent a fake local-circle answer from blessing
   a disconnected, overlapping or multiply covered soup.
4. For each unconstrained interior edge perform the exact local test below.
   Constraints are exempt from the local circle test but not from incidence or
   preservation checks. Require at least one local edge check on cases intended
   to exercise connectivity, so an empty/overconstrained test is not a CDT test.

The reader returns counts useful to explain scope: original sites/constraints,
triangles, unconstrained interior edges checked, exact positive violations and
zero determinants. It raises on violations. These are observations from the
direct reader, not a new certificate format or second validation pipeline.

### Independent exact determinant

For an interior edge orient its two positive triangles as `(a,b,c)` and
`(b,a,d)`. Check four distinct IDs and the exact positive orientation of both
triangles; do not obtain those facts from native adjacency or cached predicates.
Create fresh metric-space rational coordinates for these original sites and
the four rows

```
[mu*u_i, mv*v_i, (mu*u_i)**2 + (mv*v_i)**2, 1]
```

for `i=(a,b,c,d)`. Compute the complete 4x4 determinant with a generic rational
Gaussian elimination routine: find a nonzero pivot in each remaining column,
swap rows while recording sign, multiply pivots, eliminate lower rows through
exact rational division, and return zero if no pivot exists. Do not call the
native bigint API, reproduce its normalized integer weights, expand its cached
lifts, or call Python `_improve_connectivity`. Using the full unshifted rational
matrix deliberately differs from the native optimized expression and arithmetic.

Because `(a,b,c)` is positive and the metric has positive determinant, `D > 0`
means d is inside its circumcircle: the unconstrained edge fails. `D <= 0`
passes, including exact cocircular zero. No approximate tolerance is involved.
The result is independent of which positive incident triangle was chosen first.

A positive local violation in a valid straight-line triangulation implies the
two-triangle quadrilateral is strictly convex and its other diagonal can replace
this edge. For product updates, check strict positivity of both proposed
triangles and absence of the proposed diagonal before mutating topology. The
oracle does **not** skip a positive determinant because the implementation said
the edge was nonflippable. Nonconvex or straight-angle containing quadrilaterals
are already locally legal; their determinants cannot create an excuse to leave
an actual positive violation. Structural failures remain structural failures.

The constrained local-Delaunay criterion applies after valid planar embedding
and all constraints have been established. Do not apply the ordinary all-sites
empty-circle condition to every triangle: hole and boundary constraints can
block visibility, and such a condition rejects valid constrained triangulations.
There is no separate global visibility implementation in this first oracle.

### Cocircularity and comparison policy

For exact zero, either embedded diagonal satisfying constraints is legal. The
oracle compares semantics rather than triangle sequence. A direct constructor
may legitimately disagree with old ear/Lawson output, a reference constructor,
or another insertion permutation on those diagonals. A product promise of
repeatability for an identical serialized input is tested separately and does
not imply canonical connectivity under source renaming or input permutation.

Hand-derived four-site examples include one strict violation, its legal opposite
diagonal, an exact square with both legal diagonals, and a metric that reverses
the legal diagonal. Actual illegal output must raise even when coverage and
incidence alone pass. Missing/merged collinear boundary samples and a crossed
triangle soup exercise different obligations of this same output reader.
These are direct semantic fault cases, not a mutation/certification campaign.

## Predetermined independent workload cohort

Select these families before optimizing against any captured watch face. The
small semantic cohort runs the complete simple embedding reader. Size sweeps
exercise product performance separately, with semantic qualification outside the
timed product call; report qualification cost rather than burying it in solve.

| Family | Construction and purpose |
| --- | --- |
| Strict convex | Exact rational circle parameterization, without trig rounding; small and many-site rings test construction without an inherited ear fan. |
| Collinear strips | Two protected parallel rational sample chains and end segments; varied length/width, offsets and unequal station spacing exercise protected samples and long thin domains. |
| Concave comb and spiral | Independently authored simple orthogonal notches and a non-self-intersecting corridor; test location/recovery across many intervening features, without watch-derived carrier data. |
| One and many holes | Nonuniform independently placed rectangular and triangular holes in a concave outer; narrow corridors and staggered boundaries test constraint recovery and domain removal. |
| Nearly degenerate | Exact rational almost-collinearity and almost-cocircularity with both signs, large translations and unrelated denominators; test cancellation and exact fallback. |
| Exact degeneracy | Square/cocircular rational sites, straight protected runs, and repeated coordinate ranks; legal zeros must not oscillate, delete samples or force a unique diagonal. |
| Metric anisotropy | Both isotropic and strongly unequal positive binary64 metric scales on the same independently authored domains; include a hand-derived diagonal reversal. |
| Input order | Original, reverse insertion, alternating distant sites, seeded shuffles, cyclic loop rotations and hole-word permutations; test location, recovery and source maps. Loop winding remains valid. |
| External original domains | All five already pinned NVIDIA/Manifold cases, described below; do not count their transforms as additional independent geometries. |
| CAD regression | Captured watch exact points/constraints/metric/source pins, reported as its own workload; it never controls admission or capacities. |

Use geometric size series (for example 32, 128, 512, 2048, 8192 boundary sites)
where a family's construction admits those sizes. Small exact-degeneracy cases
are not inflated merely to populate a chart. Predetermine a bounded measured
cohort in the benchmark plan with explicit seeds/counts before tuning. A timeout
or resource rejection remains visible; do not discard that row or change the
input after observing it. Record actual per-family time, work and peak memory.

Domain metamorphisms include rational translations, a 180-degree rotation,
source-ID bijections (including zero/high IDs and order reversal), cyclic loop
rotation and hole reordering. Exact positive power-of-two axis scales with
compensating inverse metric scales preserve metric geometry when both actual
binary64 metric values remain finite/nonzero; verify this exact precondition.
General rational coordinate scaling is still a useful semantic case, but cannot
be called metric-equivalent if its recomputed binary64 metric rounds differently.
Axis exchange is also valid if coordinates and metric axes are exchanged and
all loop/triangle windings are corrected for the orientation reversal.

## Reference inputs and implementations

The five retained original-domain inputs are NVIDIA's complete planar L outer,
triangle hole and declared 32-point rational circle schedule, plus Manifold's
SimpleHole, Eberly, Concave2 and AlignedHoles cases. Read original fixture bytes
through their existing adapters. Keep source revisions and fixture hashes in
`tests/fixtures/face-domain/PROVENANCE.md`. A sampled circle's chordal polygon is
the exact domain here; it is not the analytic disk. Upstream counts and no-crash
tests remain diagnostics, not the expected CDT answer.

CGAL and Triangle are reference algorithms and sources to study, not production
dependencies selected by this proposal. If isolated executable comparisons are
added, use identical site coordinates, protected segment inventory, hole regions
and constant diagonal metric. Use no quality/area/refinement switches, no
Steiner insertion or segment splitting, and no decimal-rounding adapter that
silently changes exact rational geometry. Triangle's `-p` constructs a CDT while
`-q/-a/-u` initiate refinement and change the comparison scope. Triangle uses
binary floating-point site input; rational cases it cannot represent exactly
must be explicitly excluded from exact-input differential claims, or classified
as a separately changed-input comparison. Do not call that disagreement a native
defect without examining both actual exact domains.

Feed each actual foreign output through the same source inventory, domain and
local CDT reader after restoring original source IDs. Compare admissible geometry
and local postconditions, not triangle order or cocircular diagonal equality.
Reference execution cannot replace the owned output oracle. A reference suite's
own build or green upstream test count does not establish our output correctness.

## Literature and exact implications

- [CGAL 6.2.1 constrained Delaunay class reference](https://doc.cgal.org/latest/Triangulation_2/classCGAL_1_1Constrained__Delaunay__triangulation__2.html),
  read 2026-10-16: constraints block visibility, and the constrained-circle
  property differs from ordinary empty circles. This underpins the exemption
  for source constraints and rejects a global unconstrained-circle oracle.
- [Shewchuk, *Delaunay Refinement Mesh Generation*, CMU-CS-97-137, 1997](https://www.cs.cmu.edu/~glmiller/Publications/ShewPHD.pdf),
  Chapter 2 local Delaunay discussion/Lemma 2: a locally illegal edge is
  flippable; nonconvex adjacency does not justify skipping an illegal edge.
- [Shewchuk, *General-Dimensional Constrained Delaunay and Constrained Regular
  Triangulations, I: Combinatorial Properties*, author bibliography](https://www.cs.cmu.edu/~jrs/jrspapers.html):
  the author's description states the constrained Delaunay lemma in terms of
  every codimension-one simplex being locally regular or constrained. Our use
  is its planar constant-metric specialization after checking embedding.
- [Triangle definitions](https://www.cs.cmu.edu/~quake/triangle.defs.html) and
  [Triangle Delaunay usage](https://www.cs.cmu.edu/~quake/triangle.delaunay.html),
  read 2026-10-16: distinguish fixed-site constraints from refinement and
  conforming meshes; this defines the no-Steiner reference comparison scope.

These sources justify the postconditions, not a promised wall-clock target or
particular insertion/recovery complexity. The algorithm study must separately
justify its constructor, location strategy, predicate filters, topology layout
and fallback bounds. Timing results then decide which studied design earns
production use across the predetermined families.
