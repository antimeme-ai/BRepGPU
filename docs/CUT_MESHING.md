# Mesh an owned through-bore Boolean result

The dedicated CPU API consumes the actual `CutResult` from
[`difference(box, cutter)`](BOOLEANS.md). It retains that result, its original
operands/sections/arrangements/history, and its actual trimmed `CutSolid`.
This is a bounded result-meshing connection; feature solids, arbitrary Boolean
results and general imported faces are outside this operation's input family.

```python
from fractions import Fraction as F
from brepgpu.body import Frame, make_body
from brepgpu.boolean import difference, EntityKey

box = make_body('box', Frame((0, 0, 0), 1))
cutter = make_body('cylinder', Frame((0, 0, 0), 2))
cut = difference(box, cutter)
mesh = cut.tessellate(tolerance=F(1, 1024))
assert mesh.cut_result is cut
assert mesh.body is cut.solid
assert mesh.history is cut.history
triangle = mesh.triangles[0]
origin = mesh.history.origin(EntityKey(mesh.body.body_id, mesh.body.revision,
                                     'face', triangle.face_id))
```

Equivalent: `brepgpu.cut_meshing.tessellate(cut, *, tolerance,
max_quarter_segments=256)`. Pass the full result, not a bare CutSolid. Existing
canonical body meshing admission is unchanged; no canonical plate is rebuilt.
The result's hole radius is1/2 in box-local units, twice the factory plate's.

Tolerance is a positive int excluding bool, Fraction or finite binary64,
interpreted exactly in world units. Segment budget is an integer power of two
in1..1024. Invalid requests raise TypeError/ValueError; unmet subdivision or
actual rounded-geometry budget raises the existing MeshBudgetError. No partial
output or coarse fallback is returned. Inputs and previous meshes stay intact.

## Actual source identity

CutMesh and all its record tables are immutable, factory-only output. Leaf
records share the existing [mesh schema](MESHING.md): exact/rounded vertices,
face triangles with chart-local UV corners, ordered physical edge polylines,
and directed coedge occurrences with original lifted UV.

Mesh vertex/triangle IDs are dense zero-based slots. Result entity IDs retain
CutSolid's body/revision namespace. A vertex source tuple names a result vertex
or result edge and exact intrinsic parameter; the retained Boolean history maps
those result entities to original operands and actual section/arrangement events.
Both closed-ring endpoint parameters share one physical anchor. Two same-face
wall seam coedges share the physical line while retaining atlas U0 and U4.

Planar charts preserve original box controls/frames. The inward wall preserves
original cutter controls and its twice-box-scale frame, using source V[1/4,3/4].
Wall triangle corners keep this raw V band; they are not reparameterized to0/1.
The cropped seam's intrinsic output t0..1 differs from its original source range
[1/4,3/4], retained separately in history. No coordinate welding establishes
identity and no carrier refitting changes original coefficients.

## Approximation and qualification

Each circle quarter has n dyadic rational intervals in its actual curve
parameterization. At world radius R, `curve_bound=R/(2*n*n)` bounds the complete
arc/chord and cap-domain discrepancy in both directions. Straight cap triangles
include a thin strip inside the analytic void: the output covers its declared
chordal domain, with a bound to the analytic boundary. It does not assert every
triangle interior is material-IN.

Each shared exact vertex is rounded once. `rounding_bound` is the actual maximum
L1 coordinate displacement, which also bounds the whole triangle perturbation
by barycentric interpolation. `error_bound=curve_bound+rounding_bound` must meet
the exact submitted tolerance. Rounded rings, nesting, ordered cap heights,
planes and triangle orientations are checked before publication.

Independent qualification checks original Boolean/source truth, complete exact
and rounded face/chord-strip coverage, all original edge/coedge/source UV maps,
shared mesh-edge incidence, cyclic vertex links, connectedness and genus one.
Counts, volume or a picture cannot replace these checks. External community
qualification remains incomplete and mandatory before alpha.

## Reproduce on the fleet

```sh
PYTHONPATH=python python3 examples/mesh_cut.py --case fine-unit \
  --output-directory results/local-cut-mesh
PYTHONPATH=python python3 benchmarks/cut_meshing/run.py \
  --output-directory results/local-cut-mesh-costs
```

The caller writes actual mesh PLY and SVG, exact/rounded/source tables and the
retained operands/result/history. All construction and indexed/history
consumption is CPU-only; native/CUDA calls and host/device transfers are zero.
Costs distinguish operand preparation, Boolean construction, prepared-result
mesh+indexed/history consumption, independent qualification and export.

[Design](CUT_MESH_DESIGN.md) · [Reviewed delivery plan](CUT_MESH_PLAN.md).
