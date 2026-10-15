# Whole OM10: independent geometry, assembly and display qualification

2026-10-15. Research and design input for the operator's complete assembled and
exploded/labeled OM10 CPU/P100 delivery. No production implementation, product
execution, library adoption or performance claim is made here. The scope is the
supplied OM10 MAN VA01.3 source:107 solid definitions,9,162 distinct source faces
and166 expanded leaf occurrences. Those three inventories measure different
things and must remain distinct in output and graphics.

## Source material actually read

Read the shared Blackbird doctrine, repository instructions, delivery brief,
source inventory and whole-assembly geometry gaps, existing interchange exact
Cox–de Boor/Bernstein oracle, and the feature-bore full-domain/physical-manifold
qualifier. These establish the existing machinery rather than assuming a CAD
kernel's success means the requested geometry is correct.

Primary references read for this study:

- [Shewchuk, robust predicates](https://www.cs.cmu.edu/~quake/robust.html):
  orientation/incircle signs can be wrong near zero under ordinary floating-point
  arithmetic. Our independent checker can use exact rational values of saved
  binary64 coordinates; production can use owned filtered/exact predicates.
- [Shewchuk, Triangle](https://www.cs.cmu.edu/~quake/triangle.html): constrained
  domain triangulation is distinct from refinement and mesh quality. This is a
  reference, not a proposed dependency or imported B-rep oracle.
- [Giesen, Triangle rasterization in practice,2013](https://fgiesen.wordpress.com/2013/02/08/triangle-rasterization-in-practice/):
  integer edge predicates, subpixel coordinates, overflow limits and half-open
  edge ownership matter independently. Biased coverage predicates must not be
  mistaken for unbiased interpolation weights.
- [NVIDIA CUDA Best Practices, timing](https://docs.nvidia.com/cuda/cuda-c-best-practices-guide/index.html#timing):
  asynchronous launch time is not completed-work time. Synchronized host walls
  and GPU events answer different timing questions. The page read is current
  documentation; the delivered binary/compiler remain explicitly pinned to the
  project's actual CUDA12/P100 build.
- [OCCT BRepCheck_Analyzer reference](https://occt3d.com/dev/doc/refman/html/class_b_rep_check___analyzer.html)
  and [BRepTools reference](https://occt3d.com/dev/doc/refman/html/class_b_rep_tools.html):
  foreign shape checks, UV ranges and stored triangulations can provide useful
  diagnostics. They do not independently establish the owned output's source
  correspondence, complete face coverage or approximation error.

The recommendations below are design reasoning from those mechanisms and the
actual source inventory. No unacquired paper is presented as read.

## One direct mechanism per claim

Use four independent checks because they address different faults, not four
receipts for one success:

1. Original source records and definition/occurrence paths determine completeness
   and placement.
2. Original carriers/curves and independently evaluated parameters determine
   coordinate, derivative, conversion and trim-recovery accuracy.
3. Positive full boundary chains and physical triangle incidence determine
   polygonal-domain coverage and shell topology.
4. A tiny scalar pixel model determines native renderer visibility and primitive
   ownership; actual CPU/CUDA outputs must also agree within their stated rule.

There is no need for a mutation campaign, coverage target, second mesh certificate,
or fresh universal numerical certification project. Existing external alpha
qualification remains incomplete and separate. Each failed source record is a
delivery failure; a renderer cannot silently omit it and call the watch complete.

## Native interchange values the checker needs

The source bridge should emit an owned versioned binary representation, not JSON
as an internal model. Preserve the original source hash and entity references,
translation correspondence and foreign-reader revision/settings separately from
owned geometric output. The independent checker needs:

- Definition IDs, original solid/face/wire/coedge/edge/vertex IDs and every nested
  usage path. Definitions are reusable; occurrences are individually placed.
- Original length units and model tolerances, distinct from chosen approximation
  quality. The source's0.01mm uncertainty is not the mesher tolerance.
- Every original analytic carrier frame, kind, dimensions and original UV domain;
  spline degrees, knot multiplicities/values and homogeneous control records.
- Spatial edge curve and original parameter interval/sense; supported degree-five
  curves must remain degree-five rather than silently becoming cubic.
- Per-coedge UV curve, recovered original parameter relation, traversal sense,
  periodic shift and occurrence ID. Original STEP has no saved pcurves; recovery
  must be identified as recovery and retain its measured residuals/bounds.
- Physical edge sample IDs and original curve parameters. Triangle face IDs and
  all per-corner UV/chart occurrences must survive native packing. Same-face
  seams have distinct UV corners sharing the same physical edge samples.
- Actual output arrays, finite status/error results and error-budget components.
  A display-only PLY cannot carry all of these obligations.

Source adaptation may repair or generate representation data. Emit each change,
including increased tolerance, recovered pcurve and replaced topology; never
overwrite the original inventory and describe the result as unchanged source.

## Carrier and curve truth

For spline conversion, reuse the independent original-knot Cox–de Boor approach
in spirit: evaluate original data, not the production span-extraction helper.
At original knots, use declared one-sided first derivatives where a knot makes
the derivative discontinuous. Endpoints, both sides of interior knots, interior
parameters, and nonunit domains are deliberate cases. Compare original source
positions/first derivatives with native CPU and each promised P100. Positive
homogeneous weights and active-domain checks are required.

For analytic carriers, independent original-frame formulas and derivative
identities cover planes, cylinders, cones, spheres and tori. Include nontrivial
frames, opposite face orientation and periodic translations. A sphere pole or
cone apex can have singular UV derivatives; preserve its topological identity
and define its normal handling explicitly. Do not normalize a zero cross product.

Spatial edge→pcurve→surface agreement must use the actual physical point emitted
to the mesh. If a point is chosen from the original spatial edge, its recovered
UV may not reproduce that point exactly on either incident face. Add this
residual to the world approximation budget; checking only the surface's own
sample would hide an edge/face crack. Direct finite probes measure residuals at
those probes. They are not an all-parameter maximum.

Where an all-triangle bound is claimed, use a justified derivative/control-hull
bound over the triangle's entire UV extent, including every knot span it meets,
plus boundary recovery and actual output rounding. A center/edge-midpoint probe
alone cannot certify flatness. Bound UV-boundary chord error separately from
interior surface error. A result may honestly report measured residuals without
claiming a universal bound that was not established.

## Full domains and physical solids

Independently reconstruct each sampled UV domain from original ordered source
loops and actual coedge chains. Check loop joining, occurrence sense, periodic
lift, self-intersection, hole separation/containment and completeness. Original
nonconvex and55-loop faces must be admitted if valid, rather than narrowed to a
convex fixture after observing them. A UV inversion with ambiguous disconnected
solutions must be resolved from source chart/continuation information or fail.

For each triangle, take exact rational values of its stored binary64 UV corners.
Require positive oriented area in its source domain. Sum its three directed
edges with integer multiplicity and cancel reverse pairs. The remaining chain
must equal the complete oriented domain boundary. Positive orientation plus
boundary winding equality checks full polygonal coverage without duplicating
the production hole bridges, ear selection or refinement logic. Do not reduce
counts to sets/parity or use area equality as an overlap oracle.

If span/chart constraints subdivide a face, apply that check to the actual
subdomains and require all artificial cuts in opposite pairs. Conforming
subdivision must be explicit; the checker should not epsilon-weld a T-junction.
Periodic chart cuts are representation boundaries, not new physical edges.
UV triangles near analytic singularities may contain distinct corner occurrences
that identify the same physical source vertex. Account for that quotient while
rejecting collapsed physical output triangles. Degenerate source edges require
explicit degeneracy metadata rather than arbitrary duplicate-coordinate removal.

Independently build physical triangle-edge incidence per solid: two opposed
uses per ordinary edge, closed connected vertex links, connected shells as
declared, finite noncollapsed triangles and consistent orientation. Do not weld
different definitions or touching assembly parts into one global manifold.
Genus follows the actual source topology; do not require every solid genuszero.
Closed incidence alone does not establish lack of geometric self-intersection,
correct trims or source fidelity, which the separate domain/source checks address.

Approximate straight boundary chords can cross a thin strip of analytic material
or void. State that approximation explicitly; do not call every triangle interior
exact material-IN. Retaining every source face and loop is compatible with a
bounded display mesh; discarding a tiny hole merely because its pixel size is
small is not the same delivery contract.

## Assembly, explosion and labels

Build expected occurrence placement independently from the original two-frame
transform records and usage graph, including representation mapping. Confirm
all166 expanded leaf paths,31 assembly definitions and107 solid definitions;
do not mistake196 usage records for196 physical parts. Verify rigid transforms,
millimetre conversion and selected world landmarks on known repeated parts.

Test nested noncommuting rotation/translation fixtures. An implementation which
only adds translations will pass a flat trivial fixture and fail the real claim.
Per-occurrence color/name overrides follow explicit precedence while source
names and IDs remain available. A missing style can use a documented material
fallback; missing geometry cannot.

Explosion is a second authored transform, retaining original placements. At
explode0, return the original placement directly rather than recomputing it with
unnecessary arithmetic; require identical assembled values. At explode1 and an
intermediate value, expected authored subassembly/leaf offsets compose in the
documented coordinate frame. The exploded scene uses the same unique geometry
and166 occurrence identities. This is an illustration, not a collision-free
disassembly or validated mate/kinematic model.

Every displayed part number resolves to a source occurrence or an explicit
group/quantity list. Leader endpoints project the actual transformed anchor;
labels/legend remain inside the graphic and avoid each other under the fixed
layout contract. A complete BOM can list every instance while the hero uses
grouped callouts; no requirement to cram166 unreadable names into one panel.

## Native display: independent synthetic answer model

The proposed CPU/CUDA display uses orthographic projection,1/256-pixel XY
quantization, integer half-open top-left coverage, FP64 depth, stable primitive
IDs, and interpolated source-face first-jet normals. Normals are interpolated
only inside each original face; duplicated display corners preserve sharp
boundaries. The original geometry remains unquantized. Explicit
range checks must prevent edge-function integer overflow before a launch.

The independent oracle reads source geometry and camera directly and uses exact
rational scalar predicates for tiny images, not the production tile bins or
raster helper. It checks IDs/depth in addition to RGB so attractive lighting
cannot conceal visibility mistakes. Contract cases:

- A triangle with analytically known inside/outside/edge-center pixels.
- Two triangles forming a rectangle, including diagonal pixel centers: exactly
  one owns the shared edge, with unbiased barycentric interpolation.
- Front/back overlapping triangles, reversed submission order and exact equal
  depth with stable lowest-primitive-ID ownership.
- Two instances of one triangle, including noncommuting placement and an
  offscreen instance. Changing only an instance transform updates the image.
- Thin/subpixel, negative/offscreen coordinates, projection-degenerate triangles,
  invalid index/NaN/overflow rejection, empty scene and all-background output.
- A nonaxis geometric normal rotated by a rigid instance; a known light direction
  establishes brightness/orientation independently. Zero normals fail explicitly.

Select depth with a documented arithmetic order. FP64CPU/CUDA may differ when
one backend fuses arithmetic; either specify explicit matching operations or
bound depth error and enforce the exact expected winner away from the bound.
Near-equal values must have a declared deterministic rule. Whole-image parity
is useful integration evidence after these direct answers, not their replacement.

## Matched timing and graphics

Measure CPU1 and a useful multicore native baseline against each promised P100;
do not compare Python per-triangle loops with CUDA. Use identical complete
source, mesh quality, camera, material, explosion, label layout, image dimensions
and output format. Name each GPU operation actually performed. A GPU renderer
does not imply GPU STEP reading, UV recovery or triangulation.

Record repeated cold source-to-image process walls, including source input,
adapter/conversion, trim/mesh, native initialization, uploads, geometry/placement,
render, downloads, labels/output and shutdown. Source preprocessing used equally
by both modes is still part of the cold wall; if measured once and added to
repeated native runs, report that composition instead of calling it repeated
whole cold runs. File-cache state and retained machine/device state are explicit.

Record reused assembled/exploded/camera update walls separately, including
transform upload, work completion, image download and identical label/export.
Retained CPU/GPU geometry must be symmetric. Native CUDA events may report a
kernel stage; synchronized host wall reports the native call; external process
wall includes teardown. Overlapping rows are not summed twice. Independent
qualification is separate from useful work and reported within any named
qualified caller wall. Do not omit it from a displayed time whose caption says
qualified complete operation.

Prefer a small fixed repeated comparison with alternating mode order, median
and min/max; inspect variance before any claim. Do not add a performance gate
or keep rerunning in search of a speedup. The final image's timing inset must
name cold or reused scope, CPU thread count, P100 device and geometry/render
stage where applicable. Publish the measured slowdown if the complete GPU wall
is slower. The model and X image remain useful regardless of the headline.

## Bounded red-test and review subplan

Before production, write direct failing tests against the reviewed APIs for one
coherent family at a time:

1. Source/geometry fixture with nested placements, nonunit knot domain and
   degree-five edge, periodic seam, opposite orientation and a multi-hole
   nonconvex trimmed face. Independent expected values and boundary chains must
   predate the implementation; exercise invalid/ambiguous recovery errors.
2. Source mesh fixtures deliberately exposing missing hole, reversed/duplicate
   triangle, seam UV collapse, wrong original parameter and unmatched physical
   edge as direct error-raising cases. These are malformed actual outputs, not
   a campaign which merely tests whether a checker can test itself.
3. Tiny native display fixtures above with scalar specification answers. Run
   same fixtures CPU1/multicore/P100 and relevant sanitizer/memory checks for
   new native ownership; no historical whole-repository GPU rerun by default.
4. Actual source qualification checks every source definition/face/occurrence
   and emitted geometry. Save failures and fix causes rather than widening
   tolerances or silently reducing advertised whole-assembly scope.
5. Independent code review fixes actual findings; one matched bounded timing
   run and actual artifact inspection finish the delivery. No added certification
   lane after these direct obligations pass.

Concrete API names and selected approximation/normal budgets remain the owning
design's decision. This study supplies acceptance obligations and required data,
not a parallel undocumented product contract.
