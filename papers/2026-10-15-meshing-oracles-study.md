# Trim-aware meshing: independent geometric truth and qualification

A complete first family can cover the actual owned box, cylinder and one-hole
plate, including their original loops, shared edges and same-face wall seam.
Use exact CPU mesh construction and explicit rounded-export bounds if selected
by the design; no duplicate GPU consumer is necessary to establish this breadth
beachhead. This study selects no dependency and implements no product code.

The important contract is **complete coverage of a source-derived chordal domain,
with a certified distance from the true trimmed boundary/surface**. Finite straight
triangles cannot reproduce an exact circular trim. An inscribed disk cap omits a
thin true rim; an inscribed polygon hole adds a thin strip inside the true void.
Neither is an unqualified exact-material tessellation. Preserve the hole and its
source correspondence and bound that strip by the requested world tolerance.

## Sources read and what they contribute

Read actual `body.py`, `body_oracles.py`, the body/trim/table contracts, the alpha
qualification policy and pinned OCCT meshing parameters/tests. Body scales remain
powers of two in[1/4,4], finite origin components in[-16,16]. Primitive dimensions
are box/plate half-width3/4, half-height1/2, circular radius1/4 before Frame.
Circle parameter t traverses four rational quarters; wall U traverses0..4.
The original seam has two coedge occurrences on the same face, with distinct UV
lifts but shared physical edge/vertices. Global position identity and per-corner
face/UV occurrence identity must therefore be separate.

Piegl/Richard derive parametric triangle-size bounds from surface second
partials and distinguish surface/interior error from trimming-boundary error.
Their p.20 point-selection discussion also discards a trim smaller than its
sampling step: this is a concrete example of a published simplification we must
**not** inherit when promising preserved holes. Coarse tolerance still retains
the plate's inner boundary/genus. [Piegl and Richard, 1995](https://doi.org/10.1016/0010-4485(95)90749-6),
[original paper text](https://bojianwu.github.io/gsoc2016/week_6/NURBS_tess.pdf)

Ng/Tan's published abstract emphasizes topology during trimline insertion/removal
and boundary merging. That supports studying shared boundary ownership separately
from local triangle generation; only the available abstract/preview was read,
not the complete paper. [Ng and Tan, 2000](https://doi.org/10.1016/S0010-4485(99)00089-5)

Shewchuk's Triangle material separates constrained polygon triangulation and exact
predicates from refinement/quality concerns. CGAL's Mesh_2 manual likewise defines
a mesh as a partition of a bounded PSLG domain whose boundary segments are
constraints. We need that partition claim, rather than successful point sampling.
They are references, not dependencies or curved B-rep answer oracles.
[Shewchuk, 1996](https://www.cs.cmu.edu/~quake/tripaper/triangle0.html),
[CGAL Mesh_2 manual](https://doc.cgal.org/latest/Mesh_2/index.html)

The directed-boundary completeness argument below uses elementary winding
additivity and strict positive orientation. Erickson's computational-topology
notes explain the signed crossing/winding formulation and cancellation of
interior triangle contributions. No approximate angle sum or generalized-winding
threshold is needed in our exact rational bounded case.
[Erickson, Winding Numbers](https://jeffe.cs.illinois.edu/teaching/comptop/2023/notes/02-winding-number.html)

## Source-first data and shared identities

The oracle should independently read original body vertices, curves, coedges,
loops, face orientation, charts and homogeneous controls. Expected answers must
not come from the mesher's fan/strip construction, triangulation helper, private
cache, supplied normal, reported error or output adjacency builder.

Require every triangle to retain its original body/revision/face and all three
actual global mesh vertex IDs. Each corner additionally retains exact source UV;
sharp edges and the wall seam cannot use one global UV or normal per position.
Boundary occurrence records should bind original coedge/edge and exact directed
source-parameter subintervals. Validate against original rational maps, including
both wall seam lifts and the parameter reversal of oppositely used boundaries.

Source edge parameter chains cover exactly[0,1] in order with no omitted/duplicate
intervals. Closed circle t=0 and t=1 share the original physical endpoint ID;
quad-boundary samples share identities across charts. The top/bottom cap and wall
consume the same ring IDs. Adjacent original line edges meet at the same source
vertex ID, not independently rounded coincident coordinates. No epsilon weld
supplies these identities. Mesh interior vertices, if supported, need explicit
source-face semantics; do not silently infer their lineage from a coordinate.

## Complete face coverage without copying the triangulator

Independently reconstruct the simple oriented boundary of each polygonal face:
original rectangle boundary; sampled-circle CCW disk boundary; or rectangular
outer boundary plus CW sampled-circle hole. Establish simplicity, orientation,
strict containment of the hole and complete original parameter coverage from
source records. Do not accept the product's declared boundary as expected truth.

A cheaper sufficient oracle than pairwise triangle clipping is the following
bounded chain/winding argument, accepted after root's independent proposal:

1. Project each triangle to an oriented exact 2D face frame. Require a strictly
   positive determinant for every triangle; zero or reversed triangles fail.
2. Sum its three directed edges as an **integer multiset**, preserving every
   occurrence. Internal reverse pairs cancel. The complete remaining chain must
   equal the independently reconstructed CCW outer and CW hole boundary chain.
   No set, parity reduction, loss of duplicates or floating-coordinate welding.
3. Winding is additive in directed chains. At any point off the finite edge set,
   each positive triangle contributes1 if it contains that point and0 otherwise.
   Their sum equals the expected domain boundary's winding:1 in material,0 in
   the exterior/hole. This proves neither gaps nor positive-area overlaps nor
   extra material. Since finite closed triangles and the target polygonal domain
   are regular closed sets, the equality extends to complete closed coverage.

This is a complete domain argument, not just Euler/area/count or centroid
membership. Preserve geometric endpoint equality and explicit incidence identity
when cancelling; reject nonconforming subdivided boundaries/T-junctions instead
of silently splitting edges in the checker. Area equality can remain a readable
consequence/diagnostic; it is not a second copy of the completeness mechanism.

For cylindrical walls, use each actual source circle interval's **chord plane**.
Independently check its XY endpoints against the original radius/parameter and
express every triangle corner in exact(local chord fraction lambda, world z).
Require its XY position equals A+lambda(B-A), with lambda in[0,1]. Apply the
positive-triangle/boundary-chain proof to the full chord-by-height rectangle.
Original rational curve parameter s is nonlinear in chord fraction: covering
linear(s,z) alone does not prove a supplied spatial triangle covers that chord
rectangle. Periodic seam gluing occurs through original physical IDs after these
local geometric domains have been checked; distinct UV occurrences remain.

## Closed orientable manifold and actual material orientation

Build an independent triangle incidence dictionary, not a call to existing CSR.
Every undirected global mesh edge has exactly two distinct incident triangles
with opposite directed uses. Require one connected shell, every vertex used,
nondegenerate triangles and a **single cycle in each vertex link**. Two uses per
edge alone permits pinched/bow-tie vertices with disconnected links.

Original face IDs and outward orientation must agree: box/plate outer planes,
cylinder outer radial normals, and the plate hole's inward radial material
normal. Derive signs from original face orientation and exact cross products,
not product normals. An entirely reversed watertight shell must fail. Check
source-face border chains against all actual original coedge occurrences,
including the same-face seam, rather than interpreting every seam as a free edge.

Expected Euler characteristics2 for box/cylinder and0 for plate are useful
consequences of these specific canonical solids, not sufficient manifold or
material evidence. Equal area/genus can hide a moved patch, duplicated-and-omitted
triangles, wrong face tags or a filled hole plus an unrelated manufactured hole.

## Analytic whole-triangle world error

Let a,b be consecutive source circle endpoint vectors about its center, with
squared radius R² and an ordered source interval contained in one original
quarter. The maximum arc-to-chord error is the sagitta

    delta = R - sqrt((R² + a·b)/2).

For a requested exact world budget e with0<e<R, the equivalent square-root-free
condition is

    (R² + a·b)/2 >= (R-e)².

For e>=R the condition is trivially satisfied, but still retain at least the
four source quarters/hole topology. Evaluate this independently for every actual
interval after Frame. Uniform rational t increments are **not uniform angles**;
a regular-N-gon cos(pi/N) formula does not qualify arbitrary rational-quarter
samples. Domain coverage plus all interval bounds controls the missing cap rim
and the source-to-mesh wall direction, not merely vertices on the source.

Also derive a whole-triangle inward-radius bound independently. The exact minimum
squared XY distance to the center occurs at the origin if contained, otherwise
at a triangle edge's clamped orthogonal projection (including vertices). These
use rational dot products and inequalities. On a wall or holed planar cap,
min_radius² >= (R-e)² for e<R bounds every triangle point's radial discrepancy;
this catches a long diagonal whose centroid looked acceptable. Wall vertex radii
and convexity give max_radius<=R. Flat outer rectangular faces have zero carrier
error; hole-cap error is distance to the **trimmed material**, not zero merely
because its plane equation is satisfied. Disk-cap triangles lie in their exact
carrier disk by convexity; the omitted true rim is covered by the sagitta bound.

The chord rectangles and polygonal caps supply a source-coupled homeomorphic
approximation with the same hole topology. The resulting world-space Hausdorff
claim is for this entire narrow admitted family. It is not a derivative bound for
general NURBS or certification of arbitrary imported meshes.

If float coordinates are exported, compute actual displacement from authoritative
exact coordinates using the actual exported binary values. Let rho be the maximum
vertex L1 displacement. Barycentric convexity bounds displacement of **every**
point on each triangle by rho; geometric_error+rho bounds total geometric error.
Reserve/check a total budget explicitly. Shared IDs alone preserve combinatorics,
not embedding: verify the rounded export has no collapsed triangles, unwanted
vertex collisions or orientation reversal under the admitted coordinate range.
Do not present the exact mesh's stronger guarantee as a float-export guarantee.

## Bounded tests and meaningful injected faults

Literal coarse meshes should exercise every original face and signed boundary:
box8 vertices/12 triangles if unrefined; cylinder ringN plus2 centers gives
V=2N+2,T=4N; the proposed plate four corner fans plus four side bridge triangles
per cap gives V=2N+8,T=4N+16. These are construction-specific literal tests, not
universal validity conditions. Full source-domain coverage remains authoritative
when a supported triangulation legitimately subdivides differently.

Cover all five admitted Frame scales, translated origins, coarse tolerance that
must retain the hole, exact/neighboring-float chord thresholds, source endpoint
and seam lifts, original face/corner UV permutations where API permits, and fine
but bounded circle subdivisions. Use tolerance0/negative/nonfinite/bool, failed
iterators or invalid source records only where the selected API admits them.
Tiny positive tolerances beyond count/resource limits must reject explicitly,
never silently return an unqualified coarse mesh or allocate without bounds.

Candidate actual production mutations should attack distinct direct assertions:

- Omit a cap triangle; duplicate one elsewhere; fill the plate hole; reverse one
  triangle or the whole body. Strict orientation and full chain multiplicity
  distinguish missing/overlapping/reversed material.
- Duplicate wall seam vertices despite identical positions; use a different cap
  ring; collapse top and bottom IDs; unwrap a seam corner to the wrong UV lift.
  Physical incidence, source binding and vertex links reject these faults.
- Drop a source parameter interval or reverse a coedge's parameter sense. Original
  boundary-chain completeness and UV/spatial correspondence reject the change.
- Coarsen actual circle chords while claiming the requested budget; use local
  radius instead of world radius; incorrectly apply the regular-polygon bound.
  Whole-interval/triangle rational inequalities supply direct witnesses.
- Reverse the hole-wall material normal; falsify a float displacement budget or
  perturb an exported position. Original material sign and actual-byte exact
  displacement checks must fail.

As required by alpha policy, kills establish only these chosen fault sensitivities.
Do not claim broad mutation coverage, renderer truth or CPU/GPU parity as geometry.

## External meshing cases and alpha debt

Pinned OCCT 8.0.1 source at commit
b8f597c677811d1f9f4d8a97f5ae2825c0353a42 contains immediately useful actual cases:
`tests/bugs/mesh/bug25586_1..3` (closed extruded faces/holes), `bug31853` (holes with
large deflection), `bug25287` (out-of-tolerance mesh), `bug31449_1..3` (free nodes/
loopback face), `bug32471` (planar-face deflection recomputation) and the standard/
advanced incremental/parallel mesh grids. These are candidates to inventory,
not cases passed by BRepGPU in this study.

Actual `tests/mesh/end` executes `triarea` and `tricheck`, reporting absent face
triangulations, free links, cross-face errors, asynchronous edges and free nodes.
It also generates TODO exceptions from case variables. Those assertions attack
useful external fault families, but aggregate area and upstream BAD/TODO do not
replace the complete source-domain oracle or become alpha passes. Actual
IMeshTools_Parameters distinguishes boundary/interior deflection and angle, shape
size-relative mode, minimum size, and analytic-surface control switches; translate
the intended parameter contract rather than assume a scalar tolerance matches.
[OCCT pinned mesh harness](https://github.com/Open-Cascade-SAS/OCCT/blob/b8f597c677811d1f9f4d8a97f5ae2825c0353a42/tests/mesh/end)

Under `docs/ALPHA_QUALIFICATION.md`, required applicable upstream cases must be
selected against advertised mesh operations before observing passing results.
Missing external data, unported operation/output adapters, required unsupported
geometry, unresolved tolerance semantics and upstream known BAD remain visibly
non-green until resolved. Self-contained canonical tests can qualify this narrow
beachhead during development; they cannot erase general imported trimmed-B-rep
meshing release debt. NIST exchange/validation properties and a reference build
supply neither complete triangle coverage nor per-triangle deviation answers.
No external suite/reference was built or executed and no library adopted here.
