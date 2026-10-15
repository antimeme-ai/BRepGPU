# Actual extrusion → bore → mesh

`brepgpu.feature_bore.difference(feature, cutter)` consumes an actual owned
`ExtrusionResult` and a factory cylinder `BodySnapshot`. It returns an immutable
`BoreResult`, with the original feature/cutter objects, a new solid, immediate
history and ordered history paths back to the original profile and cutter.

```python
from fractions import Fraction as F
from brepgpu.body import make_body
from brepgpu.extrusion import make_profile, extrude
from brepgpu.feature_bore import difference

profile = make_profile(((-F(1,2), -F(1,2)), (F(1,2), -F(1,2)),
                        (F(1,2), F(1,2)), (-F(1,2), F(1,2))))
feature = extrude(profile, F(1,2))
cutter = make_body('cylinder')
result = difference(feature, cutter)
mesh = result.tessellate(tolerance=F(1,256))
assert result.feature is feature
assert result.cutter is cutter
assert mesh.history is result.history
assert mesh.composed_history is result.composed_history
```

The supported profile is the existing 3–32 vertex strictly convex CCW lattice
family. Source Frames have the existing uniform scale and translation. The
actual cutter must share the extrusion's Z origin, strictly exceed its world
height, and have its entire circle strictly inside every original profile edge.
Off-center XY cuts are supported. Exact positive/squared support tests reject
tangent, exterior and crossing circles; short or cap-contact cutters also fail.
Unsupported geometry raises `UnsupportedBore`; wrong input types raise
`TypeError`. Inputs and earlier results survive failed calls.

No canonical-box relabeling supplies the result. Original outer entities and
carrier coefficients/Frames remain; cap loops acquire real circular holes.
For N profile vertices, topology has 2N+2 vertices, 3N+3 edges, 6N+6 coedges,
N+5 loops, N+3 faces and N+6 charts. The inward wall keeps the actual cutter's
original V band. Its seam has distinct U=0 and U=4 occurrences.

`result.history` retains all immediate input dispositions and output origins for
vertices, edges, coedges, loops, faces and charts. `result.composed_history`
retains every original profile/cutter root, including deleted inputs and roots
of generated output. Each output origin has `HistoryPath(root, steps)` entries.
Steps are the actual immediate-cut and extrusion `Origin` objects, in that
order; a cutter path has only its actual cut step. Source ranges, affine/axial
maps, domains and owners stay separate. No unlike maps are flattened into one
fabricated parameterization. Typed key lookups reject foreign/stale keys.

`brepgpu.feature_bore_meshing.tessellate(result, *, tolerance,
max_quarter_segments=256)` builds the complete closed mesh. Original outer
edges stay unsplit. For M=4q circle intervals the mesh has 2N+2M shared vertices
and 4N+4M outward triangles. Exact cap-hole triangulation uses a visible bridge
and ear clipping, with duplicate boundary occurrences sharing physical indices.
Caps, outer sides and the cropped inward wall retain original corner UVs.

The positive world tolerance accepts int excluding bool, Fraction or finite
binary64. The subdivision budget is a power of two in 1..1024. The first dyadic
q satisfying R/(2q²) ≤ tolerance/2 bounds source approximation. Actual maximum
binary64 vertex displacement is recorded separately; their sum must meet the
submitted tolerance. Rounded layers, ring ordering, nesting and outward facets
must retain the product embedding. Unmet budgets/embedding raise
`MeshBudgetError`, without widening tolerance. Circular chords can enter the
thin analytic void strip; exact material-IN for every triangle is not claimed.

Independent qualification reads original source expressions, complete immediate
and ordered root ancestry, all supporting material faces, whole positive
exact/rounded domain chains, source corners, shared physical incidence, cyclic
vertex links and actual error. This is a CPU operation and CPU mesher; arbitrary
edit graphs, multiple/interacting cuts, general Booleans and GPU construction
remain separate work. External community alpha qualification remains incomplete.

On the fleet:

```sh
PYTHONPATH=python python3 -m unittest discover -s tests -p 'test_feature_bore*.py' -v
PYTHONPATH=python python3 examples/render_watch.py \
  --output-directory results/local-watch
```

[Target notes](WATCH_TARGET.md) · [Design](WATCH_CHAIN_DESIGN.md) ·
[Plan](WATCH_CHAIN_PLAN.md).
