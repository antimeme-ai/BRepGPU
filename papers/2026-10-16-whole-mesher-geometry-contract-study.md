# Whole imported-BRep mesher: geometry, error and independent answers

Research proposal for the global design review, 2026-10-16. No product, tests or
meshing executions accompany this study. Runtime and meshing algorithms have
other owners; this paper specifies what their output must mean. Bibliography
and acquisition/read scopes are in `whole-mesher-geometry-contract.bib` and
`whole-mesher-geometry-contract-sources.tsv`. Studying these implementations does
not adopt a dependency.

## 1. The input is not necessarily one exact watertight point set

Read current `imported_source.py`, `imported_meshing.py`, `imported_oracles.py`,
the original cylinder trim diagnosis, existing mesh/oracle designs and retained
reference-case catalog. The source contains original spatial curves, vertices,
face carriers, oriented coedge occurrences, pcurves, parameter maps and source
tolerances. These are different geometric representations, not interchangeable
samples of an assumed exact intersection.

OCCT's SameParameter means agreement within the edge's tolerance; it does not
establish agreement at a smaller requested meshing tolerance. Its documented
curve-on-surface access can also construct a planar pcurve rather than return
one that was stored. Preserve that distinction. [OCCT topology training](https://dev.opencascade.org/sites/default/files/pdf/Topology.pdf),
[BRep_Tool](https://occt3d.com/dev/doc/refman/html/class_b_rep___tool.html).

Marussig/Hughes §§3.2–3.4 explain how approximate intersections produce gaps,
overlaps and competing trimmed descriptions. A topological edge does not imply
that all associated carrier traces coincide exactly. Geometry remodelling is a
separate operation with approximation consequences. This is a reason to state
our reference geometry explicitly, not a reason to discard source topology.
[Primary report](https://www.oden.utexas.edu/media/reports/2017/1703.pdf).

The existing cylinder example is decisive: a reconstructed foreign pcurve
deviated by a measured 6.61 micrometres while the original spatial curve's
measured distance to the original cylinder was about .141 micrometres. Scalar
reparameterization could not fix the foreign trace. Those are measured values,
not whole-interval bounds. They demonstrate why a reconstructed pcurve cannot
silently become the exact original trim-domain oracle. See
`2026-10-15-om10-cylinder-trim-recovery-study.md`.

**Proposed source meaning:** retain original spatial edges/vertices as physical
skeleton authority and face carriers as surface authority. Every occurrence
also retains its original raw pcurve, source parameter/map, reconstruction
status and branch. When a supplied pcurve defines an authoritative original
trim domain, any recovered-domain displacement must enter the approximation
claim. When it is only a reconstructed branch/topology guide, explicitly name
the selected toleranced face realization constructed from the spatial skeleton.
Do not call that realization an unchanged exact original trimmed solid.

A valid complete result must approximate every original entity promised by its
declared policy. It cannot obtain success by dropping a difficult loop, changing
the source carrier, welding unrelated close vertices, averaging adjacent faces'
boundary coordinates, or raising an internal tolerance. The global design must
choose and document this policy once, before implementation.

## 2. One approximation contract with explicit components

Let `S_f` be an original face carrier, `C_e(t)` an original spatial edge and
`gamma_h` a selected continuous chart trace for coedge occurrence `h`. Preserve
the map from edge parameter `t` to that occurrence's original pcurve parameter,
raw UV, recovered UV and period. A non-SameParameter edge needs a justified
monotone correspondence; numerical proximity at unrelated parameters is not
such a correspondence.

The output has one canonical physical key/XYZ per sampled edge parameter and
original endpoint, shared by all incidents. Different occurrences may have
different UV coordinates, period lifts, orientation and face normals. Distinct
source edges never merge just because their coordinates happen to be close.
Retain the same-edge seam occurrences even when they belong to the same face.

Use world-space distance in the declared unit system. Positional tolerance,
normal-angle quality, triangle shape, maximum size and topology are separate
requirements. A chord tolerance is not an angle bound. A correct exact
orientation sign is not a positional-error bound. Optional angle/size criteria
must name their scope and admissibility; small input angles preclude universal
minimum-angle promises. [Triangle paper](https://www.cs.cmu.edu/~quake/tripaper/triangle1.html)
and [Shewchuk thesis abstract](https://csd.cs.cmu.edu/academics/doctoral/degrees-conferred/jonathan-richard-shewchuk).

For an edge interval `I=[a,b]` with actual emitted endpoints `Q_a,Q_b`, use its
parameter-matched chord `L_e(t)`. Bound the *whole interval*, not just its
midpoint: `sup_I ||C_e(t)-L_e(t)|| <= E_edge(I)`. Endpoint incidence offsets are
included if the original vertex differs from the curve endpoint. For each
incident chart trace also bound `||C_e(t)-S_f(gamma_h(t))||` over the interval,
or include that discrepancy directly in the cell/trim-strip residual. A source
edge tolerance is retained metadata, not a substitute for this calculation.

For a UV triangle `T`, let `L_T` be its affine map to the **actual emitted XYZ**
corners, and let `P_f` be the union of its polygonal chart cells. A uniform bound
on `||S_f(v)-L_T(v)||` controls only `S_f(P_f)`. It does not cover a curved trim
region omitted or added between the original trace and its polygonal chord.

To extend the claim to a declared original domain `D_f`, require a continuous,
onto, orientation-preserving correspondence `h_f:D_f -> P_f`, compatible with
all boundary occurrence maps and quotient identities. Establish either a
domain collar construction with bounded ambient displacement or equivalent
direct curved boundary-strip bounds. Then the triangle inequality gives

```
||S_f(u) - L_T(h_f(u))||
 <= ||S_f(u)-S_f(h_f(u))|| + ||S_f(h_f(u))-L_T(h_f(u))||
 <= E_domain + E_cell.
```

For a partition, take the maximum applicable total bound, not the sum over all
cells. Onto correspondence gives both directed distance bounds for that face;
without it, a pointwise triangle residual cannot be renamed two-sided
Hausdorff distance to the original trimmed face. At seams/poles the maps are
defined on the appropriate atlas/quotient rather than one globally injective
rectangle. If a toleranced realization replaces an unavailable exact original
domain, name that reference and bound its discrepancies to retained originals.

`E_cell` may already include canonical nodal displacement, output rounding,
extraction and evaluation uncertainty. Count each once. If instead it is computed
for exact carrier-evaluated vertices, add `max_i ||Q_i-S_f(UV_i)||` and a
separate arithmetic allowance once. The global design should choose one form;
it must not mix both ledgers and double-charge the same displacement.

No theorem requires the current fixed boundary `tolerance/8`, face `.75*tolerance`
and UV-rounding `tolerance/16` allocations. The contract is the total bound.
Choose local work to satisfy it, accounting first for source discrepancy and
arithmetic floors. Bound tightening, local restriction, changing a split and
different valid connectivity can reduce work without relaxing the requested
tolerance. A hard pointwise parametric residual is a sufficient geometric bound;
it may over-refine tangential motion. A cheaper genuinely geometric bound is a
candidate only when paired with complete domain coverage/correspondence.

### Constructing the trace and covering the curved boundary strip

The notation `gamma_h`/`h_f` is an obligation, not an oracle that can be wished
into existence. The following constructive route should be evaluated by the
algorithm design; an implementation claiming the whole domain must implement
an applicable route or explicitly refuse the unresolved source interval.

First attempt the retained source pcurve with its actual parameter map, provided
the policy admits its authority and a whole-interval curve/carrier residual
meets the request. For plane/cylinder/cone, owned analytic inversion supplies
an explicit candidate from the canonical source generator, with the actual
branch, axis/signed-radius conditions and continuity bounds. For a general
NURBS recovery, pointwise closest-point samples are insufficient: a selected
branch must be continued over the source interval with a bounded residual and
no jumps. A local inverse/stationary system with a root-isolation enclosure is
one candidate for regular charts; endpoint/natural-boundary constraints and
rank failures need their actual constrained cases. It must not be selected as
a general successful solver without that machinery in the reviewed plan.

For the chosen continuous trace, split into arcs monotone in a local projection
coordinate `x`; bound the relevant derivative/control projections rather than
sampling monotonicity. Represent each arc as `y=f(x)` and its sampled chord as
`y=g(x)`. Enclose the region between them, including any crossings of f and g,
and bound world-space carrier displacement to the corresponding original or
polygonal boundary point. A local transverse derivative bound times the strip
width is sufficient; a direct residual/enclosure can be tighter.

Protect these strips from all other source features except their declared
junctions. Directed source-to-chord chain replacement then leaves winding
unchanged outside the strip union, so `D_f` symmetric-difference `P_f` is inside
that union. Every such point has an explicit boundary projection with the
computed physical displacement bound. Together with complete cell coverage this
gives both directed geometric-distance bounds; no unevaluated global h is needed
for this alternative. Original curve and chart parameter association is retained.

For a regular disjoint graph collar of width w, with |g-f|<w, an explicit local
ambient deformation shifts y by `s*chi((y-f(x))/w)*(g(x)-f(x))`. Choose chi with
chi(0)=1, zero at the collar boundary and Lipschitz constant<=1. Its transverse
map is strictly increasing because |g-f|/w<1, and it is identity outside the
collar. This supplies a constructive local isotopy. Junction neighborhoods must
be handled jointly, not by overlapping arbitrary independent shifts. Tangential
common endpoints have zero feature clearance and need their own established
protected-star/sector ordering; this regular-collar argument does not prove
their termination. The final algorithm/plan must resolve that case explicitly.

## 3. Local bounds starting from the original coefficients

Filip/Magedson/Markot derive vector curve and right-triangle surface interpolation
bounds from uniform second derivatives. Their §3.1 also warns that independently
accurate surfaces can exceed the requested tolerance after shared-boundary
snapping. Their use of half-tolerances is a sufficient composition strategy,
not evidence that full tolerances remain valid empirically. §4 discusses
derivative bounds. [Primary paper](https://www.ljll.fr/~frey/papers/meshing/Filip%20D.%2C%20Magedson%20R.%2C%20Markot%20R.%2C%20Surface%20algorithms%20using%20bounds%20on%20derivatives.pdf).

For arbitrary UV triangles, an explicit independent derivation is available:
expand `S` about `u=sum lambda_i*u_i`; linear Taylor terms cancel. Half the
weighted second-order remainder is bounded by coordinate variances and their
absolute covariance. If `Muu,Muv,Mvv` bound derivative-vector norms over the
relevant convex region and `du,dv` are coordinate ranges, then

```
E_interp <= (Muu*du^2 + 2*Muv*du*dv + Mvv*dv^2)/8.
```

Here `sum lambda_i*(u_i-u)^2 <= du^2/4`, and Cauchy bounds the mixed absolute
term. This states the arbitrary-triangle extension rather than citing a theorem
whose published domain is only right triangles. The argument requires suitable
regularity over the triangle. Split genuine C0 derivative jumps; do not apply a
single C2 formula across one. C1 piecewise-C2 use requires a valid uniform
piecewise remainder argument. Use local restricted derivative bounds, not
whole-model derivative maxima.

The algorithm owner's stronger candidate directly compares rational geometry
with the emitted affine triangle. In a common positive Bernstein basis on the
triangle, write the original carrier `S=N/W`, `W>=w_min>0`, and
`R=N-W*L_T`. Then

```
sup ||S-L_T|| <= max_j ||R_j|| / min_j W_j.
```

This follows from positivity/partition of unity of the basis and does not need
an assumed curvature value. Product coefficients and degree elevation must use
the same basis. Exact triangular restriction avoids constructing an ill-scaled
global affine inverse for a very thin UV triangle. A containing rectangle is
safe if the bound covers it, but may waste work on affine extension outside T.
For bilinear leaf output the two physical triangles also need the bilinear-to-
triangular twist term; a bilinear patch is not generally a planar quadrilateral.

**Extraction is part of the arithmetic:** authoritative data are original
knots, multiplicities, homogeneous controls and frames. Existing binary64
extracted spans cannot become exact truth merely by reinterpretation as
rationals. Own source-to-span extraction with exact operations or enclosing
arithmetic, propagate its uncertainty through restriction/products, and bound
the residual relative to original geometry. If enclosing coefficient values
are used, numerator upper and denominator lower bounds must be directed.
Unenclosed round-to-nearest norms and denominator minima are not hard bounds.
Recentering an algebraically identical residual can reduce translation-driven
interval width; it must preserve the original map and count arithmetic once.

Balázs/Guthe/Klein §§4–5 independently approximate trims and surfaces, use
adaptive binary partitioning and a bilinear/twist bound. Their composed-trim
construction uses a sampled solve and epsilon degree reduction; their loop
correction reconstructs intersecting loops. Neither is automatically compatible
with exact original-coefficient authority and source-preserving rejection.
Their approximate-error mode must not substitute for a hard-error mode.
[Primary institutional PDF](https://dspace.zcu.cz/bitstreams/4b28dac2-142f-41fb-8277-c234696e71c8/download).

## 4. Shared refinement and trim topology

Initialize protected original vertices/junctions, period cuts, singularities and
necessary continuity partitions before geometric refinement. Protect close
distinct features using local curve enclosures/collars and directed boundary
order. Mere small chord error does not ensure that two nearby curved trims
have noncrossing polygonal approximations. Endpoint-adjacent tangential joins
need their own sidedness/order argument; a positive feature-distance argument
for disjoint objects does not apply at their common endpoint.

Each physical edge accepts monotone split proposals in original edge parameter
from every incident chart/cell. Merge/deduplicate exact parameter identities,
evaluate canonical XYZ once, propagate the same ordered schedule to all uses,
and settle affected face cells before publishing. Work queues/generations must
prevent stale cells from escaping after a new edge split. A face may propose a
split, not create a private unmatched edge vertex. Do not repair cracks by
post-hoc proximity welding or special-case circle schedule rewriting.

Projection or inverse recovery needs a unique selected branch over each
interval and continuation through original incidence. A nearest point may be
nonunique, jump sheets or cease to be monotone. Preserve those failure cases;
do not select the closest root independently at each sample. Analytic inverses
and rational interval isolation are algorithm choices under this same contract.

For periodic analytic and NURBS carriers, use the original parameter period,
not a universal `2*pi` assumption. Preserve integer lifts and both seam sides;
do not reduce each point modulo the period. If a face cannot use one simple
lifted chart, partition its atlas while retaining original face ownership and
matching the cut copies. C0 span lines must be respected by bounds or split;
continuity assumptions must follow actual degree/multiplicity and weights.

That is the general theory, not a claim about the current format. BGSRC001
contains analytic period shifts but **no explicit NURBS periodic flags or period
values**. Initial source admission must treat those NURBS as finite original
parameter charts and cannot invent periodic continuation from their bounds or
from `2*pi`. Explicit same-edge occurrences at finite chart boundaries retain
their given UVs without requiring an inferred period. General periodic NURBS
continuation needs declared source metadata/format support before it is offered.

At a declared sphere pole/cone apex, multiple UV occurrences may identify one
original physical vertex. Coverage cells need not be triangles and need not
correspond one-to-one with physical triangles. For example, a UV rectangle with
one edge collapsed to a pole can map to the actual one-triangle fan by

```
Phi(xi,rho)=(1-rho)*Ppole + rho*((1-xi)*P2+xi*P3),  0<=xi,rho<=1.
```

The rectangle is fully covered, its pole edge has rank collapse, and the image
is the one nonzero physical fan triangle. A residual `N-W*Phi` over the rectangle
bounds that whole cell. Generalized cell-domain/map/quotient records must refer
to the actual physical facet; no zero-area physical triangle is required. The
current positive-UV-cell/marked-collapse representation is another honest option
only if its physically omitted cells retain complete source-domain coverage.
Neither option permits deleting coverage or classifying arbitrary repeated
physical keys as poles. Check the quotient's actual fan/link. Singular normals
need a stated per-face limit or an explicit unsupported result; topology alone
supplies no normal. Display UV triples are not by themselves a complete record
of the rectangular pole map; an export adapter must state its approximation.

## 5. Physical embedding belongs to mesh construction

Watertight incidence/closed vertex links and exact positive UV-domain coverage
do **not** establish a non-self-intersecting physical mesh. Even complete cells
with positional error <=epsilon can cross when the original feature clearance
is smaller than twice epsilon. Reducing numeric comparison tolerances does not
solve this geometric problem.

Li et al. §§3–4 separate curve/piece hulls before triangulation and constrain
triangles to separated critical regions. The useful implication requires
disjoint patches with positive separation, and their boundary equations assume
exact common geometry. Adjacent, tangent, singular and toleranced inputs need
additional treatment. [Publisher article](https://academic.oup.com/jcde/article/13/1/239/8383411).

Adopt the *obligation*, not an unqualified theorem: maintain conservative
within-definition candidate bounds and classify contact of actual emitted
triangles robustly. Nonincident triangles must be disjoint; incident triangles
may intersect only in their declared common vertex/edge. Do not skip an entire
pair simply because it shares a vertex: the triangles can also overlap away
from it. Coplanar overlap, endpoint-on-edge/T-junction, zero-area cells and
non-topological tangency require explicit classifications.

Integrate this with refinement. A forbidden emitted contact triggers local
cell/boundary refinement when source separation is established, then reevaluates
affected contacts before publication. If source geometry itself intersects or
cannot be resolved within bounds/resources, return the corresponding bounded
failure. Original source-patch hulls need enlargement for canonical vertices
off the carrier, or use bounds of actual emitted triangles; otherwise hull
separation does not imply output separation. This phase is part of constructing
an embedded mesh, not an additional post-hoc certification campaign.

Collision checks are within each solid definition. Different assembly instances
may legitimately touch or overlap; this operation is not an assembly collision
detector. For a valid embedded closed oriented mesh with correct per-face
domains/shared features, complete source topology and link checks establish the
intended boundary complex. Euler characteristic alone cannot identify it.

## 6. Admission, infeasibility and bounded failure

The global design must publish an admission matrix: supported analytic carriers,
finite valid original knot vectors/degrees, positive rational weights,
nonzero denominators, permissible charts and manifold source incidence. Current
surface-degree<=3 and curve-degree<=32 guards are existing implementation limits,
not mathematical limits of this research. Do not quietly widen them before
corresponding machinery is implemented. Negative/zero weights or a denominator
crossing zero do not inherit positive-weight convex-hull bounds.

A regular chart needs a justified nonvanishing rank/orientation over admitted
cells, with identified boundary singularities handled by explicit quotient
rules. A sample Jacobian is not that justification. Self-crossing original
trim loops, unexplained contacts and ambiguous source sheets are not repaired
silently. Their status must identify the unresolved/invalid source interval.

Differentiate four outcomes in the eventual operation:

* A concrete lower-bound witness makes the request infeasible under the declared
  source/authority policy. For example, a forced canonical source endpoint is
  farther from a required incident carrier than its entire allowed displacement.
* A conservative upper bound above epsilon is merely unresolved; tighten it or
  refine. It does not prove geometric infeasibility.
* An unavailable carrier/map/singularity is unsupported, not a numeric success.
* Work, memory, arithmetic precision or cancellation limits produce a bounded
  incomplete outcome, with no complete mesh published.

The source's tolerance metadata alone proves neither feasibility nor
infeasibility at a new requested epsilon. Refining cannot remove a fixed source
representation mismatch. Nor does an original-pcurve discrepancy prove that
every valid recovered chart trace is impossible. A simultaneous source-policy
change/heal would be a separate operator-visible operation.

## 7. One direct independent output reader, with explicit fault classes

Normal product execution is native end to end: local bounds, shared-schedule
settlement, physical contact resolution and complete actual-output invariants
belong to construction before atomic publication. Do not introduce a second
native implementation that recomputes every bound merely to certify native
ownership. A final inventory/link/ownership check attacks actual publication
faults; it is not another geometry-certificate system. Python callbacks or a
Python qualification gate are not required on the product path.

The independent reader described below is a development/research oracle against
actual emitted output and original source, off the normal product path. Reuse
the existing reader for that purpose; strengthening a missing claim replaces
its inadequate mechanism rather than adding another layer of the same kind.

Reuse and strengthen the existing original-value reader rather than adding
certificates of its certificates. Current `qualify_mesh` independently checks
original generators/occurrences, exact positive UV boundary chains and physical
links, but its four interior probes per triangle are finite observations and it
does not perform a complete physical triangle-intersection test. It therefore
cannot alone underwrite the proposed whole-cell/original-domain/embedded claim.

The direct reader should consume original source and actual emitted records:

| Claim / fault | Direct independent answer |
| --- | --- |
| Missing/reordered owners, forged mappings, seam aliasing | Complete original inventories/ordered coedges; exact source IDs/parameters/period sides; original vertex/curve/carrier evaluation |
| Hole omission, reversed cells, nonconforming UV edge splits | Existing exact positive complete directed boundary-chain checker, with the admitted simple original/surrogate domains and quotient records |
| Whole-cell positional error | Analytic extrema for authored families; for general retained NURBS, independent original-knot rational evaluation/enclosures on actual coverage cells/maps, not producer leaf/error flags |
| Missed curved boundary strip or changed material | Original trace-to-polyline correspondence and full protected-strip/partition coverage; same source policy as product |
| Cracks, pinches, swapped sheet | Shared physical key/parameter chains, exact oriented edge incidence, vertex links and connected components |
| Physical intersection | Independently enumerate conservative candidates and classify actual binary64 triangles using exact rational determinant/contact logic; test only within definitions |
| Source extraction/arithmetic loss | Original-knot/source coefficient correspondence, exact rational authored answers and interval bounds, wide-value/weight/endpoint cases |

The reference triangle intersection implementation must be independent of the
native predicate expression/cache. Broadphase must be conservative and bounded;
do not hide quadratic reference enumeration inside a claimed product timing.
Retain actual qualification cost separately. Replace inadequate finite-only
error coverage for the new claim; do not call the old probes a whole-cell oracle.
If an independent interval computation cannot settle a case within its budget,
record that outcome rather than declare PASS from sampled points.

CGAL Mesh_3's documented `facet_distance` measures a specific circumcenter to
surface-Delaunay-ball-center distance; it is not automatically our whole-cell
Hausdorff metric. Its feature protection and separate topology controls are
valuable references. A foreign library's successful mesh and a manifold report
are differential inputs, not the definition of our geometry contract.
[Official manual](https://doc.cgal.org/latest/Mesh_3/index.html).

## 8. Predetermined authored and external families

Choose these before implementation; vary exact parameters and orientation/UV
representation without targeting the watch. This is a design cohort, not a
claim that the current product supports every member.

1. Plane with nonconvex outer loop, disjoint holes and protected collinear
   samples; include very thin valid gaps, near-tangent joins and actual invalid
   crossings/touches. Literal rational domain/area is an independent answer.
2. Shared cylinder/disk edges, reversed source traversal, multiple incident
   occurrences, tilted/translated frames and an artificial pcurve disagreement.
   Exact analytic radial/plane formulas distinguish source mismatch from mesh
   chord error. Include feasible and witnessed infeasible source policies.
3. Sphere poles, cone apex and doubly periodic torus seams; preserve atlas sides
   and original quotient identities under face and edge permutations. Analytic
   reference/physical links expose false seam welding and lost pole coverage.
4. Polynomial saddle/bilinear twist, anisotropic ruled patch and a nonplanar
   positive rational tensor patch with exact controls/nonunit domains. Include
   repeated C0/C1 knots, endpoint cells and nonuniform positive weights. Literal
   rational evaluations and an independent residual bound expose omitted twist,
   denominator or extraction errors.
5. Two nearby curved sheets in one solid, sliver face and thin curved channel:
   epsilon larger than feature separation must still preserve embedding or fail
   explicitly. Also retain deliberately invalid self-intersecting source and
   non-topological tangency as rejection/ambiguity cases.
6. Native-type, byte/point/work/precision/cancellation limits; repeated geometries
   and source IDs, exact UVs between adjacent binary64 values, large translations
   and unit/UV reparameterizations. These attack ownership and numeric floors.

Actual upstream inputs include the already pinned NVIDIA complete trim recipe
and cone face-normal/glue recipes, Manifold polygon corpus and OCCT mesh bug
scripts/data availability catalog. Use originals with their actual fill,
tolerance, orientation and degeneracy semantics. Known upstream success-only
placeholders supply no oracle. OCCT cases that skip bad wires conflict with
our complete-source contract and belong to explicit rejection tests. Missing
external data stays missing until separately acquired; no fabricated fixture.
See `2026-10-16-native-meshing-reference-cases.tsv` for exact source revisions.

Use retained watch main plate/captured difficult faces only after the authored
and actual external families. NVIDIA/OCCT output is useful differential geometry
and work-density evidence; do not require identical diagonals, vertex counts
or raw tolerance flags. Compare actual compatible scopes with our own reader.
Existing NVIDIA sphere tolerance witnesses remain explicit external disagreements,
not proof that all its outputs violate the requested accuracy.

## 9. Performance faults to expose, without speculative latency targets

Measure complete native import/preparation/mesh/destruction and independent
reading separately. Record output density and per-stage work so fewer triangles
cannot masquerade as a faster implementation under a different error policy.
The operation should expose meaningful work for local bounds, edge samples,
recovery, cell refinement, contact candidates and exact fallbacks, not a new
telemetry framework.

Predetermine sparse and adverse families: local curvature versus global maxima;
strong anisotropy/ruled directions; many tiny holes; long recovered constraints;
thin-feature contact refinement; large coordinates with small local features;
highly varying rational weights; many coedge occurrences sharing a skeleton;
periodic/singular charts; and bounded adversarial all-overlap candidates. Rotate,
translate, scale units with epsilon, permute IDs and reparameterize the same
geometry. A translation or harmless UV choice should not explode density or
exact-arithmetic work except where the actual output precision floor changes.

Faults include repeated full-source/span scans, recomputed frame inverses,
per-face duplicate edge evaluation, conservative bounds that do not tighten,
global uniform refinement caused by one curved feature, stale split propagation,
exact fallbacks on ordinary cases, leaking memory on failed attempts, and
refining already-discarded outside regions. Triangle's implementation discussion
shows that refinement ordering and early domain removal can materially change
work/output count; those are algorithmic decisions, not just language changes.
[Primary discussion](https://www.cs.cmu.edu/~quake/tripaper/triangle4.html).

Exact predicates, source-map ownership and bounded arithmetic can all be native.
Python is not a mathematical requirement. Conversely translating the present
work generation into C++ does not establish efficient density or a sound global
error/embedding contract. Root's global theory→design→plan review must select
the algorithms and delivery stages before any product or test implementation.
