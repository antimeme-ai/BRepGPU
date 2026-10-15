# Meshing the actual through-bore result: independent oracle study

2026-10-15. Scope: the exact admitted `boolean.difference` result, not a widened
canonical plate or a general Boolean triangulator. This study selects no library,
runs no geometry locally, and writes no product code. Tests and actual product
qualification run on the fleet after design and plan review.

## What the existing sources actually require

Read `boolean.py`, `boolean_oracles.py`, `meshing.py`, `meshing_oracles.py`,
`tests/test_meshing.py`, `tests/test_meshing_entry.py`, Boolean construction tests,
`docs/MESHING.md`, `docs/POST_BREADTH_ROADMAP.md`, and the existing canonical mesh
oracle/semantics studies. Existing independent Boolean qualification already
checks original operands, sections, arrangement selection, all source carrier
bits/frames, exact maps and complete 85-input/81-output history. Reuse that direct
mechanism once when qualifying a result mesh; do not reproduce Boolean history
validation inside another mesh-specific checker.

The result resembles the canonical plate topologically but differs materially:
its local hole radius is **1/2**, not 1/4. It has 10 original output vertices,
15 physical edges, 30 coedge occurrences, 9 loops, 7 faces and 10 charts. Its
four wall charts preserve the cutter's controls and **twice-box-scale Frame**;
the used source V is **[1/4,3/4]**, not [0,1]. Physical result curves use the
result/box Frame. Its seam uses two distinct `line` UV maps at atlas U=0 and4,
with V interpolating 1/4..3/4. Mapping the wall to a rescaled canonical carrier
would falsify its actual retained source correspondence even if the picture
looked right.

Proposed public result has `.cut_result` retaining the actual `CutResult`, `.body`
identical to its actual `CutSolid`, and `.history` reading its original history.
The immutable leaf records may use the existing mesh schema. Generator tuples
`('vertex', id)` and `('edge', id, t)` are scoped by that retained result body;
face/chart/coedge IDs likewise refer to the output body's namespace. Original
operand ancestry is reached through the exact `EntityKey` and retained history,
not inferred from coincident positions or copied to a second mutable map.

## Literature and reference basis

[Shewchuk's primary Triangle paper, domain construction](https://www.cs.cmu.edu/~quake/tripaper/triangle3.html)
was read again for its distinction between points, constrained segments and
holes. Sampling vertices does not establish the material domain. No angle,
Delaunay or general PSLG claim is needed for this bounded mesher.

[Erickson's winding-number notes](https://jeffe.cs.illinois.edu/teaching/comptop/2023/notes/02-winding-number.html)
were read again for signed orientation, edge cancellation and winding
additivity. They motivate the existing complete positive-triangle chain oracle;
we do not need an approximate winding-number evaluator or a second clipping
oracle to check the same partition property.

The earlier [canonical oracle study](2026-10-15-meshing-oracles-study.md) records
actual pinned OCCT mesh harness reading and external failure families. Its
[complete source inventory](2026-10-15-meshing-oracles-sources.tsv) and bibliography
remain applicable. An attempted fresh browser open of the pinned OCCT GitHub
harness returned an internal error; this study does not claim a fresh upstream
harness execution or renewed code reading through that failed request.

## One source-first mesh qualifier

Add `cut_meshing_oracles.qualify_mesh(mesh)` as a separate bounded adapter. Do
not call the canonical `qualify_mesh`: its factory-body assumptions, radius,
source V range and expected source types are wrong for the actual cut result.
Elementary exact helpers for rational patch evaluation, original curves/UV maps,
integer chains, ring embedding and radial minima can be reused because they do
not call mesher/refinement/triangulation helpers or manufacture expected answers
from output adjacency.

1. Check exact public types, retained result/body/history object identity and
   call the existing independent Boolean qualifier on the actual retained input.
   Every record table must be an immutable tuple with dense native integer IDs;
   reject bool/float identity aliases, wrong numeric types and mutable tables.
2. Derive the complete source generator inventory from actual result vertices
   and actual circle edges. Every original vertex appears once and each interior
   circle parameter appears once. No face-center vertex is needed for a cap
   annulus. Require every physical mesh vertex to reproduce its original exact
   world generator, with distinct global identity/position and no epsilon weld.
3. Independently derive each physical edge's complete parameter chain: line
   endpoints 0,1 and full-circle dyadic rational quarters. Both circle endpoints
   refer to the same original anchor identity. Adjacent charts, cap/wall rims
   and all incident faces consume the same IDs, including the seam anchor.
4. Read every actual original coedge and apply its own orientation/UV map to
   the directed physical edge chain. Require all 30 occurrences once. The two
   seam uses reverse one shared physical chain but retain U=0 versus4 and the
   original [1/4,3/4] V endpoints. Rim wall UV uses the same original V band.
5. For each triangle corner, evaluate the retained actual source patch using
   exact ratios of its original binary64 coefficients and Frame. It must equal
   the authoritative exact mesh position. Wall triangle corners retain a single
   source quadrant and V in {1/4,3/4}. This independently detects the tempting
   wrong-frame/reparameterized-wall adaptation.

A source-pointer identity check in the actual invocation test is essential:
`mesh.cut_result is requested_result`, `mesh.body is requested_result.solid`, and
`mesh.history is requested_result.history`. With implicit body-scoped local IDs,
a checker alone cannot identify an entire same-geometry result deliberately
substituted after publication; the factory is responsible for retaining the
actual passed object and the invocation test checks that behavior directly.

## Complete domains and material orientation

For each non-wall face, reconstruct directed boundary words from its actual
original loops/coedges and independently checked physical polylines. For cap
faces this is a square with a circular polygon hole, with original bottom/top
orientation respected. Verify expected ring simplicity, strict convexity,
angular order and strict containment inside the outer square.

Project each triangle into the source planar chart's oriented exact 2D plane.
Require strictly positive material orientation after applying the face sense.
Accumulate its directed sides as an **integer multiset**, cancelling only
opposite full segments with equal shared physical IDs. The remainder must equal
the independently reconstructed original boundary words. Positive triangles
plus this complete chain imply winding multiplicity one over the chordal
material domain and zero over its hole/exterior: no positive-area overlap,
missing region or extra filled region. Reject T-junctions rather than silently
repairing the mesh in the checker.

For the wall, use each actual bottom/top source circle interval's physical chord
rectangle. Project every corner to exact (linear chord fraction, world Z), check
it lies in that physical chord plane and the original cap-height range, apply
the inward material orientation, and require complete positive coverage of that
rectangle through its full directed chain. A proof only in nonlinear rational
curve parameter and V would not prove coverage of the emitted spatial triangle.

An independent global incidence dictionary requires exactly two distinct
oppositely directed triangle uses per physical mesh edge, one connected shell,
every mesh vertex used, and one cycle in every vertex link. Euler=0/genus1 is a
readable consequence, not the complete manifold oracle. All inward wall faces
and sharp planar outward orientations must match original material; a completely
reversed but watertight mesh fails.

## Exact and actual rounded geometry budgets

Use actual world radius R=result.frame.scale/2, derived from original geometry.
For every source circle interval, independently check its exact radius/minor-arc
orientation and square-root-free sagitta inequality

    (R*R + a dot b)/2 >= (R - curve_bound)^2

where a,b are source endpoints relative to the original world center. The
proposed conservative dyadic bound R/(2*n*n), with half the requested budget
reserved for approximation, remains suitable. Exact first-admitted subdivision
checks detect local/world radius confusion and incorrect threshold selection.
Rational-quarter samples are not uniform angles.

For each wall/cap triangle, compute the exact minimum XY radius over its whole
triangle using rational edge projections and origin containment. Require its
whole inward deviation to be bounded, rather than trusting corner/centroid
samples. Hole-cap chord triangles can enter a thin part of the true void: the
contract is a bounded approximation of the complete trimmed material boundary,
not exact material-IN membership of all triangle interiors.

For every actual binary64 output coordinate, derive its exact Fraction and L1
displacement from the authoritative exact coordinate. The maximum rho bounds
whole-triangle perturbation by barycentric convexity. Require reported rounding
bound=rho and total bound=curve_bound+rho<=requested tolerance. Repeat physical
ring angular order/strict convexity/containment, cap-height separation, triangle
noncollapse/orientation and complete chain checks on the actual rounded values.
Global source IDs alone do not establish an embedded rounded surface.

## Direct fleet test matrix and falsification

Write a red test before implementation that constructs a real admitted Boolean
then calls the proposed result-mesh API. A useful coarse literal result is
N=4 circle intervals, V=2N+8=16 vertices and T=4N+16=32 triangles; its all-face,
all-coedge, same-seam and full original-history checks must also pass. These
literal counts check the selected construction, while the full chain/domain
mechanism remains the geometry oracle.

Bounded functional cases:

- All four admitted left scales 1/4,1/2,1,2, paired with actual twice-scale
  cutters; zero and translated Frames, including subnormal origin components.
- Very coarse tolerances still retain the complete through-hole/genus1;
  exact first-level threshold tolerance=world radius, its immediately lower
  float, and a fine but bounded case exercise exact subdivision selection.
- Actual circle parameter cardinal endpoints, all wall quarter corners, both
  original source V band endpoints and both seam lifts.
- Fraction/int/finite-float tolerance semantics; zero, negatives, nonfinite,
  bool, Decimal/string/None; bad budgets and tiny unreachable tolerances fail
  without changing operands, result, history or previously published mesh.
- Bare CutSolid, canonical bodies, another feature and arbitrary objects reject;
  factory-only CutMesh and recursively immutable leaf/tables remain immutable.
- Convenience method (if selected) and direct function preserve actual object
  identity and deterministic geometry/source records.

Use test-only altered copies of **real completed meshes** for direct sensitivity:
missing cap triangle; duplicate another cap triangle; filled-hole extra triangle;
one or all triangle winding reversed; a seam physical identity split; wrong rim
source parameter; wall corner V=0 or1; seam U=0 substituted for4; stale/foreign
chart or face ID; a moved generator; a claimed local-radius/error bound; actual
rounded coordinate changed without a corresponding honest displacement; and
bool/float source-ID aliases. These check materially distinct requirements.
Tests should assert the intended diagnostic where stable, so accidental schema
failure does not impersonate geometric fault detection. No broad mutation
coverage claim follows from these selected cases; mutation execution, if selected,
remains fleet-only.

## External qualification and limits

No external adapter or upstream case passes are produced by this oracle study.
The applicable pinned meshing/Boolean inventory must remain visible and non-green
until adapters inspect BRepGPU's actual outputs under a reconciled tolerance
contract. An upstream successful triangulation, aggregate volume match or our
attractive rendering does not replace full source-domain qualification. Alpha
policy and existing revolution/offset reference disagreements stay unchanged.
