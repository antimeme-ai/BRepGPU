# Complete trim-aware canonical meshes

The owned CPU mesher constructs a complete indexed triangle shell for the
existing box, cylinder or one-hole plate. Physical boundary vertices are shared
by source identity; face/chart/coedge occurrences remain distinct. Original
loops, hole boundaries and both sides of the periodic seam are retained.
This is the narrow approximation/meshing beachhead, with no new native ABI or
third-party dependency. General imported faces and arbitrary trims are unsupported.

## Use

```python
from fractions import Fraction
from brepgpu.body import make_body

body = make_body('plate')
mesh = body.tessellate(tolerance=Fraction(1, 1024))
# mesh.body is body; mesh.vertices and mesh.triangles are immutable tuples.
# Each triangle references three shared vertex indices and original face_id.
```

The equivalent function is `brepgpu.meshing.tessellate(body, *, tolerance,
max_quarter_segments=256)`. Tolerance is a positive finite int (excluding bool),
Fraction or binary64 value interpreted exactly in **world units**. Frames retain
the existing scale/origin admission. The segment budget must be a power of two
in1..1024. Invalid requests raise TypeError/ValueError; an unmet subdivision or
rounding budget raises MeshBudgetError. No partial output or silent coarse
fallback is returned. Existing bodies and previous meshes stay unchanged.

```sh
PYTHONPATH=python python3 examples/mesh_body.py --shape plate \
  --tolerance 1/1024 --output-directory results/local-mesh
PYTHONPATH=python python3 benchmarks/meshing/run.py \
  --output-directory results/local-mesh-costs
```

Product callers run on the fleet. The mesher itself is exact CPU construction
and does not load a native library or CUDA. The entry test separately exercises
existing native geometry before entering the new API.

## What the bound means

Straight triangles approximate the circular trims. An inscribed cylinder-cap
polygon omits a narrow circular rim; the corresponding plate-hole polygon adds
a narrow strip inside the analytic void. The mesh covers the complete declared
chordal domain and preserves hole topology. It does not classify all triangle
interiors as belonging to the exact curved trim.

Each original circle quarter uses dyadic rational s=j/n in its actual rational
parameterization, not equal angles. At world radius R, every chord's sagitta is
at most `R/(2*n*n)`. The same bound controls the wall's extruded chord strips
and both cap-domain discrepancies, in both directions. The box has zero source
approximation error.

`mesh.curve_bound` is that exact rational geometric bound. Each MeshVertex has
exact Fraction `position` and one shared binary64 `rounded_position`. The
actual maximum L1 coordinate displacement is `mesh.rounding_bound`, a conservative
Euclidean displacement bound. Barycentric interpolation bounds every rounded
triangle by the same vertex displacement. `mesh.error_bound` is their sum and
must be <= the submitted tolerance. Half the tolerance is reserved for rounding
when selecting n. Actual rounded triangle signs, ring ordering, nesting and
extrusion prerequisites are checked; rounding cannot silently drop a facet.

No Delaunay, minimum-angle, aspect-ratio or finite-element-quality claim applies.
No GPU mesh construction or complete CAD speedup is established by this API.

## Source identity and output

IDs for mesh vertices and triangles are dense zero-based slots. Original entity
IDs retain their typed body/revision namespace; Edge.start/end in source records
are zero-based original vertex slots, distinct from original Vertex.id.

- MeshVertex: `id`, exact `position`, `rounded_position`, typed `source` tuple.
- MeshTriangle: `id`, three `vertices`, original `face_id`, three `corners`.
- MeshCorner: original `chart_id` and exact **chart-local** `uv`.
- EdgePolyline: original `edge_id`, increasing exact `parameters`, shared
  `vertices`; a closed ring repeats the same anchor index at t=0 and t=1.
- BoundaryUse: original `coedge_id`, `face_id`, `edge_id`, `orientation`, directed
  `parameters`/`vertices` and original-map `uv`. Wall uses retain the **lifted**
  atlas0..4, including separate seam occurrences at0 and4.

Quarter-chart joins reuse physical vertices while keeping corner-local0/1
occurrences. They do not invent B-rep edges. Normals for rendering belong to
face/corner occurrences; the caller exports geometric facet normals, without
smoothing source sharp edges. No coordinate welding supplies any identity.

The actual caller emits triangle PLY, a top-face triangle view and all-face
wire view, exact/rounded vertices, triangles, corners, original physical edge
parameters/coedge UVs, bounds, original body/controls and qualification. The PLY
and images use the actual shared rounded array. Wire views have no hidden-line
removal; source/data tables remain the geometric record.

## Qualification and scope

The independent qualifier reads original source data. Strict triangle orientation
and the complete integer directed boundary chain establish coverage multiplicity
one inside each chordal face domain and zero outside, including holes. Walls
use actual affine chord coordinates rather than pretending source arc parameter
is affine. Exact and actual rounded domains are checked separately. Shared edge
incidence, cyclic vertex links, connected shell and original seam occurrences
establish the supported manifold topology. Exact original arc inequalities and
actual rounded displacements bound geometry; counts and renders do not do so.

Applicable external community qualification remains incomplete and is mandatory
before alpha. The owned canonical meshes do not substitute for those cases.
[Release gate](ALPHA_QUALIFICATION.md) · [Design](MESHING_DESIGN.md) ·
[Reviewed delivery plan](MESHING_PLAN.md).
