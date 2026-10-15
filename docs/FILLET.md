# One complete box-edge fillet

`brepgpu.fillet.fillet_edge(source,edge_key,radius)` constructs an immutable
closed rounded prism from the actual owned canonical source box. Select one
complete vertical original edgeIDs9..12 with the shared typed EntityKey. Radius
is local, on the exact1/256 grid in[1/16,1/2]. Original bounded Frame is retained;
world radius is local radius times scale. Inputs accept int/Fraction/finite
binary64 excluding bool. Other solids, selections and radii are unsupported.

```python
from fractions import Fraction
from brepgpu.body import make_body
from brepgpu.boolean import EntityKey
from brepgpu.fillet import fillet_edge

source = make_body('box')
key = EntityKey(source.body_id, source.revision, 'edge', 11)
result = fillet_edge(source, key, Fraction(3, 8))
solid = result.solid
answer = solid.classify_point((Fraction(3,4), Fraction(3,8), 0))
assert answer.boundary_faces == (5, 7)
```

The six original plane control/parameter/Frame payloads are unchanged. Two caps
and two sides have changed exact trim domains; one exact rational quarter-cylinder
Patch supplies the band. Its controls are dyadic, weights1,1,2; cap arcs and
source-edge subranges retain exact Fraction coefficients including nonbinary
joins. Retained original vertex/Curve/UVMap binary64 bits remain. New output IDs,
Lineage and namespace are owned even when geometry payloads are shared.

The graph is10 vertices,15 edges,30 coedges,7 loops,7 faces and7 charts. All15
edges have ordinary opposed uses; no periodic seam exists. Band tangency applies
along two side contact rails. The cap joins remain sharp. This is a CPU feature
constructor with separate native CPU/P100 consumers, not GPU fillet construction,
a general rolling-ball solver, interacting fillets or a result mesher.

`classify(slot,u,v)` reads zero-based chart slots, exact submitted values and
returns REJECTED/INTERIOR/BOUNDARY/INVALID. Slots0..5 retain source planes;
band6 has full[0,1]². Nonfinite/out-of-square values are INVALID. Native adapters
copy their actual binary64 query values before this exact decision; no join is
snapped to a rational boundary. `classify_point(worldpoint)` returns exact query,
interior/boundary/exterior and every bounded supporting output face. Unused parts
of the full cylinder and artificial center lines are not boundary supports.

`edge_point(one_based_edge_id,t)` returns exact world XYZ.
`coedge_uv(one_based_coedge_id,t)` returns original/raw chart UV at the same
intrinsic physical-edge parameter. Apply occurrence orientation only when
traversing the loop, never again to this intrinsic map. New `quarter_arc` Curve
and `arc` UV descriptors are operation-owned; original canonical packed tags and
Curve.evaluate guards are unchanged. `topology_input()` emits ABI5 sources and
incidence for the actual solid, with snapshots empty.

`result.history.records` covers all62 original typed entities exactly once:
33 retained,24 modified,5 deleted. All76 output origins are33 retained,
24 modified and19 generated. Four changed-domain charts are modified despite
unchanged plane coefficients. `from_input(key)` returns status/images/generated;
`origin(key)` returns precise parents, role, absolute orientation and exact maps.
Foreign or untyped keys are rejected. Generated descendants can retain deleted
source entities as causal parents without claiming a surviving boundary image.

Every source parameter map names its original edge/coedge owner. A coincident
map is spatial source-curve correspondence; a spine map gives selected-edge axial
correspondence only. Every source UV map names one actual original face/chart,
including contextual original incidence for a coedge. Band interior has no old
plane UV. Map2('affine',sixCoefficients) stores two affine rows;
Map2('arc',(Uc,Vc,R,q)) stores a rational quarter-circle map. `.evaluate(u,v)` is
exact on[0,1]². Origin orientation is absolute: V/E+1, CO traversal, L/F/CH owning
face sense. It is not another sign on parameter maps.

`result.contacts` has incoming/outgoing ContactRails. Each binds the new physical
rail, original support face and deleted original selected-edge coedge, blend face,
blend endpoint u0/1, original/blend UV maps, selected-spine parameter and exact
outward normal. Evaluate each map at(u=t,v=0) for the complete upward rail.
Both world curves and normals agree identically, not merely at displayed samples.

Construction validates original canonical tables/packed/native bits, complete
closed graph/vertex links, exact coefficient-polynomial edge/incident-carrier
identities, complete source maps/contacts and full ancestry before publishing.
Failure preserves original and prior results. Input/solid/result/history and point
answers remain immutable. Timings expose admission, assembly/history, validation
and total CPU constructor costs; measured full consumers include additional work.
Optional foreign geometry comparisons and required external alpha corpora remain
separate qualification mechanisms. Read FILLET_DESIGN.md and FILLET_PLAN.md for
the complete contract and qualification/performance scope.
