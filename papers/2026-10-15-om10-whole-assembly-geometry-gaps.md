# Whole OM10 assembly: actual carrier and trimming gaps

2026-10-15. Bounded static source inspection of the privately retained operator-supplied OM10 MAN VA 01.3 STEP, following [the component/source inventory](2026-10-15-om10-step-source-study.md). Original bytes remain untouched. Counts below come from entity types, scalar fields, knot lists and original references; no CAD import, product parser, geometry evaluator, renderer, tests or dependency ran. This is a scope study, not an implementation or validity claim.

The operator now wants the **whole actual movement**, followed by an exploded/labeled assembly, with CPU and GPU paths and clearly scoped timings in the graphics. A single clamp remains a useful development slice, but delivering that slice would not satisfy the requested whole-movement result.

## Actual B-spline surfaces

All 635 B-spline surface records are nonrational `B_SPLINE_SURFACE_WITH_KNOTS`; none is already a single tensor Bézier span. Degrees are:

| U degree / V degree | Source surfaces |
|---|---:|
| 2 / 1 | 624 |
| 3 / 3 | 10 |
| 3 / 1 | 1 |

All are clamped at both ends in both directions: first and last multiplicities equal degree plus one. All three source flags for U closure, V closure and self-intersection are `.F.`; these are source declarations, not an independent geometry check.

| Nonzero U spans / V spans | Source surfaces |
|---|---:|
| 4 / 1 | 469 |
| 2 / 1 | 111 |
| 8 / 1 | 29 |
| 6 / 1 | 21 |
| 1 / 87 | 2 |
| 1 / 103 | 1 |
| 1 / 102 | 1 |
| 1 / 4 | 1 |

There are **2,839 tensor span rectangles** across those surfaces. That is an extraction inventory, not a mesh-patch count or runtime measurement. Knot insertion/span extraction could produce degree-at-most-three Bézier patches for the existing surface evaluator without degree reduction. It must preserve source knot domains, adjacent-span boundaries, control identity/source maps and a declared floating-point conversion budget. One cannot feed the entire multispan control net to the current single-span API and expect equivalent geometry.

Current [interchange support](../docs/INTERCHANGE.md) accepts a single clamped tensor span of degree 0–3 in either direction and rejects multispan carriers. Consequently **all 635 of these spline carriers fall outside the present importer contract**, even though their degrees can fit the native evaluator after a correct conversion.

## Curves introduce a separate degree gap

The file has 2,262 simple nonrational B-spline curve records: 625 degree two, 1,630 degree three and **seven degree five**. Their source knot domains contain one to 103 nonzero spans. A further 73 complex STEP entities describe rational quadratic B-spline curves; all 73 are single-span/clamped examples. Counting only statements that begin `B_SPLINE_CURVE_WITH_KNOTS` misses these complex rational entities.

The seven degree-five curve carriers cannot in general become exact cubic curves by ordinary knot insertion. A faithful boundary pipeline needs a curve evaluator supporting them, or an explicitly bounded approximation whose topology, seam identity and shared edge sampling remain controlled. Existing surface first-jet acceleration is not by itself a general imported-curve evaluator.

Other stored carrier records include 14,423 lines, 8,901 circles and 212 ellipses. Those counts include definitions, not unique geometric loci or expanded assembly instances.

## Analytic surfaces also require conversion

The earlier inventory found 3,688 planes, 4,431 cylinders, 341 cones, 49 tori and 18 spheres, in addition to the 635 spline surfaces. The current USD experiment does not import these STEP analytic carriers as solids. Analytic-to-rational span construction and original-parameter/source-domain mapping would be needed to feed the existing CPU/P100 rational-patch evaluator. Sphere poles, cone apices, angular periodicity and face trims must be handled explicitly where present; carrier type counts do not establish which source faces meet a degeneracy.

The absence of open shells or void-B-rep entity types in this inventory is not evidence that every solid is closed, oriented or intersection-free. The source labels 107 solids and closed shells; that is the source's assertion.

## Trims are topology plus spatial curves, without saved pcurves

The file contains **zero** `PCURVE`, `SURFACE_CURVE`, `SEAM_CURVE`, `TRIMMED_CURVE` or `COMPOSITE_CURVE` entities. Its advanced faces refer to `FACE_OUTER_BOUND`/`FACE_BOUND` records, `EDGE_LOOP`, `ORIENTED_EDGE`, `EDGE_CURVE` and vertices. EDGE_CURVE carries a spatial curve and sense; its endpoint/topological use supplies trimming information even though it is not a `TRIMMED_CURVE` entity.

The actual inventory is 9,162 faces, 9,768 loops, 25,871 edge records and 51,742 oriented edge uses. Of the faces, 8,787 have one loop, 314 have two, 28 have three, and the remaining faces have more; the maximum is **55 loops on one face**. One face contains **965 edge uses** across its loops. Convex straight-sided caps and one interior circular hole do not cover these faces.

There are **608 distinct edges reused twice on the same source face**, found by following actual face/loop/oriented-edge references. They are seam-like topological uses that must retain both occurrences; absence of `SEAM_CURVE` records does not mean absence of seams. The source provides no explicit corresponding two-sided UV curves. Their placement in the surface parameter domain must be reconstructed from the actual carrier, original spatial edge, loop traversal and periodic chart choice, with errors and ambiguity made explicit. For analytic carriers this may be direct; B-spline boundary inversion is a further problem.

A whole-assembly mesher therefore needs shared boundary discretization, UV/chart recovery, arbitrary trimmed-face domains with multiple holes and nonconvex boundaries, curved-boundary error control, periodic seams, face orientation and adjacency. Existing canonical/convex-result meshers cannot be broadened merely by supplying more source triangles or relabeling the incoming body.

## Assembly placements and exploded labels

The source has 196 `ITEM_DEFINED_TRANSFORMATION` entities and 196 complex `REPRESENTATION_RELATIONSHIP_WITH_TRANSFORMATION` entities. Transformations refer to pairs of `AXIS2_PLACEMENT_3D` records; 17,975 such axis placements also serve the geometry. There is one `MAPPED_ITEM` and one `REPRESENTATION_MAP`. No Cartesian transformation-operator entity was found in the inspected type inventory.

A faithful reader must distinguish geometry carrier frames from assembly instance frames and compose the actual occurrence chain. The single root expands to 166 leaf occurrences from 107 unique solid definitions. Exploding and labeling should operate on those preserved identities/placements, not reinterpret 196 usage records as 196 solid parts. An exploded offset is an explicit display transform in addition to the source assembly transform; the assembled scene remains the fixed comparison.

## Implication for the requested CPU/GPU graphic

There is a plausible selective-acceleration path: faithfully recover/mesh the complete source once, evaluate admitted rational carrier patches and first jets on CPU/P100 using the same source parameters, and render the matched geometry. It requires actual whole-source coverage; the currently supported clamp/extrusion path and a few successful imported patches are insufficient. Full trimmed meshing and rendering remain separate CPU work unless independently implemented and measured elsewhere.

For a credible X graphic, report at least the evaluated operation's CPU/P100 cost, upload/download/setup scope, and the complete caller/render wall cost. A faster native first-jet stage should be labeled that way. Whole rendering acceleration cannot be inferred from faster surface sampling while preprocessing, meshing, UV recovery, labels or display dominate the run. No timings, speedups, source import success or whole-assembly reconstruction are established by this static study.
