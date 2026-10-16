# Complete component and numerical strategy checkpoint

The operator asked whether the new native path meshes a real single component,
and challenged accepting the cost of one numerical strategy as a C++ limitation.
The answer is yes for one observed complete OM10 component; current performance
is poor. No replacement numerical implementation is selected by this checkpoint.

## Actual complete component observation

An external diagnostic input script retains complete original component rows
from the existing source snapshot: every face, original edge, vertex, carrier,
loop, coedge and referenced spatial/pcurve. It preserves source IDs and geometry
values, selects that component's product as the root, and removes assembly uses.
This is source selection at the test boundary, not geometry reconstruction or a
production Python callback. The first selection is the definition with fewest
faces; a second five-face pin includes conical geometry.

The qualified native CLI at commit f4d56d9 ran CPU4 with positional0.005mm and
source-node0.001mm, original source values, and two960x720 images:

| Complete source definition | Faces | Observation |
| --- | ---: | --- |
|1795 Cheville de plateau|4|37nodes,70facets,1instance; mesh28.330530113s; required outputs through close28.600637855s; process wall28.60s|
|1722 Goupille050-100|5|Stopped by diagnostic timeout at35.00s; no completed output; status124|

The first result contains complete semantic mesh, source scene, original labels,
both views and both images. Source preparation took0.067080927s, and assembled
display took0.043514968s. It performed13boundary rounds and reported
1,638,587,209owner work units. The image was inspected. These are single
observations, not uncontended benchmark estimates, external-oracle qualification,
or a whole-watch result. No matched-quality reference comparison is established.
Raw source, images and operational transcripts remain ignored in context/.

## NVIDIA prepared-component observation

During the adaptive-architecture study, the existing pinned NVIDIA reference
worker consumed the corresponding complete original1795/1722 BinTools source
definitions, requested chord0.005mm, healing0, strict no-partial output. Both
returned complete meshes with closed/manifold report status. In one shared fresh
worker, context/plugin setup took0.005177606s. Per-component observations:

| Definition | Actual physical vertices / triangles | Tessellation stage | Conversion/import/tessellation/output/part teardown |
| --- | ---: | ---: | ---: |
|1795|26/48|0.009416878s|0.036027589s|
|1722|128/252|0.020337965s|0.024774508s|

Combined native process wall was0.10s. This begins with prepared original
component B-reps, not STEP decoding. It includes the official OCCT→USD conversion
and import in the larger per-part scope. Environment limits constrain PXR/OMP/
BLAS but do not by themselves prove a TBB worker count. There is no reference
display time or warm-tree reuse measurement here. Requested chord values alone
do not establish the same whole-cell/source correspondence guarantee or actual
quality, so no matched-quality speedup ratio is calculated. Both outcomes and
their counts remain, rather than selecting only the smallest/fastest part.

## What the references actually offer

- [OCCT BRepMesh architecture](https://github.com/Open-Cascade-SAS/OCCT/blob/master/dox/user_guides/mesh/mesh.md)
  separates original-edge discretization, face-local construction and postprocess;
  dispatches surface-specific range splitters; caches reusable discretizations;
  and runs independent edges/faces in parallel. Its additional
  [deflection controller](https://github.com/Open-Cascade-SAS/OCCT/blob/master/src/ModelingAlgorithms/TKMesh/BRepMesh/BRepMesh_DelaunayDeflectionControlMeshAlgo.hxx)
  uses sampled checks and bounded passes, not our whole-cell error contract.
- [NVIDIA SMLib inspected revision](https://github.com/NVIDIA-Omniverse/usd-brep/blob/c9c5979f04cea8c38213004d4f04df0dcfb40d05/source/SMLib/src/SmSurfaceCache.cpp)
  uses cached adaptive surface trees, cheap flat/control-net tests and more
  expensive geometry only where needed. Its
  [tessellator](https://github.com/NVIDIA-Omniverse/usd-brep/blob/c9c5979f04cea8c38213004d4f04df0dcfb40d05/source/SMLib/src/SmTess.cpp)
  embeds trims into the adaptive subdivision. Existing detailed source reading
  is in2026-10-16-whole-mesher-algorithms.md; no fresh reference timing is inferred.
- [CGAL lazy exact numbers](https://doc.cgal.org/latest/Number_types/classCGAL_1_1Lazy__exact__nt.html)
  evaluate approximations first and materialize the exact representation only
  when needed. Its
  [exact-predicate/inexact-construction kernel](https://doc.cgal.org/latest/Kernel_23/classCGAL_1_1Exact__predicates__inexact__constructions__kernel.html)
  explicitly separates robust discrete decisions from construction arithmetic.
  This is a numerical pattern, not a complete imported B-rep tessellator.
- [Shewchuk adaptive predicates](https://www.cs.cmu.edu/~quake/robust.html)
  establish another concrete implementation of escalating only when the sign
  cannot yet be decided. Triangle's predicate machinery is C; the numerical
  lesson is applicable to our owned C++ machinery without adopting its code.

## Current avoidable work and serious alternatives

geometry_analytic.cpp::trig_range calls rational trig_at_stage for interval
midpoints; geometry_jets.cpp uses it for analytic positions and first/second
derivatives. The point-only fixed-limb evaluator does not serve this path.
The earlier whole-watch profile exposes the resulting exact GCD/allocation cost.

The present analytic_curve_chord_upper constructs a full interval jet and two
full endpoint jets before applying an already-implemented source Gram curvature
bound. For circle/ellipse C(t)=O+Acos(t)+Bsin(t), the source Gram operator norm
bounds ||C''|| globally. The chord estimate M*dt^2/8 plus actual endpoint
departure needs no interior trigonometry. Cache this original-frame bound and
endpoint departures; retain local tighter bounds when the global bound cannot
decide. Original finite axes need not be orthogonal or normalized.

The other contenders are a shared outward fixed/dyadic or binary64 interval
evaluation ladder for explicit jets; and compiling analytic edge-minus-carrier
residuals before bounding so shared harmonics cancel instead of subtracting two
wide independent intervals. Expensive exact/high-precision paths remain available
for unresolved decisions. A residual compiler must declare and check its finite
geometry/parameter-map preconditions. The numerical owner is recording the
source-backed comparison separately. These are changes to representation,
precision policy and avoided work; a faster generic GCD alone does not answer
the operator's performance concern. No new dependency is adopted here.
