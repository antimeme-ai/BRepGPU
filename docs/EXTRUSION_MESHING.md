# Mesh an actual extrusion result

`brepgpu.extrusion_meshing.tessellate(feature, *, tolerance)` and the lazy
`feature.tessellate(tolerance=...)` consume the existing strict convex-profile
ExtrusionResult. Mesh output retains that exact feature, its original profile,
actual solid and complete source history. Other feature families and bare solids
are outside this API's input contract; existing mesher guards stay unchanged.

```python
from fractions import Fraction as F
from brepgpu.extrusion import make_profile, extrude

profile = make_profile(((-F(1,2), -F(1,4)), (F(1,2), -F(1,4)),
                        (F(1,4), F(1,2))))
feature = extrude(profile, F(3,4))
mesh = feature.tessellate(tolerance=F(1,1048576))
assert mesh.feature_result is feature
assert mesh.body is feature.solid
assert mesh.history is feature.history
```

For N original profile vertices, the complete mesh has2N shared vertices and
4N−4 outward triangles. It reads actual directed face loops, source edges and
coedge maps, retaining every original occurrence and each corner's exact chart UV.
The original profile polygon trims the cap carrier; the full carrier square is
not filled. Typed history maps result entities to original profile IDs and
source affine/axial maps. No coordinate welding supplies identity.

The faces are planar, so exact triangulation has zero source approximation
error. Every shared world vertex is rounded once to binary64. `rounding_bound`
is the actual maximum L1 vertex displacement; corresponding barycentric points
bound the whole rounded shell in both directions. `error_bound` equals that
value and must meet the exact submitted world tolerance. Rounded output must
remain a strictly convex XY polygon extruded between ordered common Z levels.
An oblique rounded side plane may differ slightly from its exact source plane.

Tolerance is a positive int excluding bool, Fraction or finite binary64. Invalid
requests raise TypeError/ValueError; insufficient rounding budget or rounded
embedding failure raises existing MeshBudgetError. Failure preserves inputs and
prior meshes, with no partial output or tolerance widening. ExtrusionMesh and
all record tables are immutable factory output. Existing mesh value records and
indexed readers are reused; constant curve_bound=0 and quarter_segments=0 mean
there are no curved quarters. No subdivision knob is exposed.

Independent direct qualification checks whole exact/rounded face coverage,
original patch/UV and source boundaries, closed opposite triangle-edge uses,
cyclic vertex links, connectedness, convex-prism embedding and actual error.
The bounded delivery stops after these checks, affected compatibility, actual
callers and independent review pass. External community alpha qualification
remains separate and incomplete.

On the fleet:

```sh
PYTHONPATH=python:tests python3 -m unittest -v test_extrusion_meshing
PYTHONPATH=python python3 examples/mesh_extrusion.py \
  --output-directory results/local-extrusion-mesh
```

The caller saves original profile/result/control/history records, actual indexed
PLY/SVG, exact/rounded/source mesh tables and scoped CPU costs. Feature creation,
prepared-result mesh+indexed/history consumption, independent checking and export
are separate. No native/GPU mesher, general triangulation/quality guarantee or
performance speedup claim applies.

[Design](EXTRUSION_MESH_DESIGN.md) · [Plan](EXTRUSION_MESH_PLAN.md).
