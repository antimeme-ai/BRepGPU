# Reference implementations by kernel responsibility

Checked 2026-10-13. This is a source-reading map for an owned implementation.
The listed study priorities are project judgments, not benchmark rankings.
The [snapshot catalog](reference-catalog.tsv) identifies which repositories
are already acquired at a fixed revision. Other links below were inspected
through their primary documentation or selected source files; they are not
claimed to be acquired, built, audited or reproduced.

## Curved geometry and B-rep modeling

| Reference | What to learn | Start reading |
| --- | --- | --- |
| [NVIDIA USD BRep / SMLib](https://github.com/NVIDIA-Omniverse/usd-brep) | Mature NURBS modeling, nonmanifold topology, tolerances, intersections, healing, fillets and API boundaries | `source/SMLib/`, `source/SM_API/`, and the [known issues](https://github.com/NVIDIA-Omniverse/usd-brep/blob/main/KNOWN_ISSUES.md); our CPU benchmark is a fixture source, not a correctness oracle |
| [Open CASCADE](https://github.com/Open-Cascade-SAS/OCCT) | Complete Boolean pipeline: intersection data, split pieces, classification, shared result topology and history | [Boolean algorithm documentation](https://occt3d.com/dev/doc/overview/html/specification__boolean_operations.html); `BOPAlgo_PaveFiller`, `IntTools_FaceFace`, `BOPAlgo_Builder`, `BRepCheck`; `Geom`/`Geom2d`/`TopoDS`/`BRep` for geometry/topology boundaries |
| [Truck](https://github.com/ricosjp/truck) | Modular Rust reimplementation of geometry, topology, modeling and interchange | `truck-geometry`, `truck-geotrait`, `truck-topology`, `truck-shapeops`, `truck-stepio`; its WebGPU rendering should not be mistaken for a GPU Boolean backend |
| [SolveSpace](https://github.com/solvespace/solvespace) | A compact independent NURBS/modeling implementation alongside a constraint solver | Native curve/surface and Boolean source, followed by sketch-to-solid operation history; compare declared limits to larger kernels |
| [openNURBS](https://github.com/mcneel/opennurbs) | NURBS evaluation and an explicit B-rep model of faces, edges, loops and parameter-space trims | [NURBS evaluator](https://github.com/mcneel/opennurbs/blob/8.x/opennurbs_evaluate_nurbs.cpp), [B-rep header](https://github.com/mcneel/opennurbs/blob/8.x/opennurbs_brep.h); [McNeel's scope statement](https://developer.rhino3d.com/guides/opennurbs/what-is-opennurbs/) excludes Rhino's Boolean/intersection/meshing machinery |

The first two are the primary whole-kernel references. Truck and SolveSpace
provide independently organized implementations worth comparing with them.
openNURBS is especially useful for representation and evaluation; it does not
make Rhino's full modeling algorithms public.

## Numerical machinery and actual GPU spline code

| Reference | What to learn | Scope and source |
| --- | --- | --- |
| [SISL](https://github.com/SINTEF-Geometry/SISL) | Spline evaluation, rational derivatives, subdivision/intersection outputs and numerical status semantics | [s1421](https://github.com/SINTEF-Geometry/SISL/blob/master/src/s1421.c) computes the position/first/second derivative ordering needed by our first unit; [s1859](https://github.com/SINTEF-Geometry/SISL/blob/master/src/s1859.c) exposes surface-intersection points and parameter-domain curve descriptions; investigate its lower-level discovery machinery rather than taking the routine's stated purpose as a completeness proof |
| [GoTools](https://github.com/SINTEF-Geometry/GoTools) | Closest points, bounded/trimmed surfaces, curve-on-surface relationships, intersections and topology around spline geometry | [SINTEF overview](https://www.sintef.no/projectweb/geometry-toolkits/gotools/) and [coupled intersection-curve interface](https://www.sintef.no/globalassets/upload/ikt/9011/geometri/gotools_core/gotools_core_doc/group__geometry.html); a particularly useful consumer of spline primitives |
| [LNLib](https://github.com/BIMCoderLiang/LNLib) | Readable C++ implementations organized around The NURBS Book | Curve/surface algorithms under `src/LNLib/`; useful for basis evaluation, knot insertion and degree elevation, without treating book correspondence or test coverage as certification |
| [LTCSBezier](https://github.com/valasekg/LTCSBezier) | Linear-time constant-storage evaluation, FMA interpolation choices, arithmetic error and evaluator benchmarking | [bezier_eval.hpp](https://github.com/valasekg/LTCSBezier/blob/main/src/bezier_eval.hpp), [ltcs_fma.hpp](https://github.com/valasekg/LTCSBezier/blob/main/src/ltcs_fma.hpp), `test_precision`, `test_performance`; precision harness uses exact rational truth; [2026 paper](https://doi.org/10.1111/cgf.70403) discusses GPU register pressure |
| [GPU-NURBS](https://github.com/Qiang-Zou/GPU-NURBS) | Actual CUDA evaluation and inverse evaluation, matrix representation, derivative scaling and device work | [Evaluate.cu](https://github.com/Qiang-Zou/GPU-NURBS/blob/main/src/NURBS/Evaluate.cu) contains double-precision device position/derivative routines; [kernel.cu](https://github.com/Qiang-Zou/GPU-NURBS/blob/main/src/kernel/kernel.cu) contains device search/subdivision machinery; README documents GCC 11.4/CUDA 12.2, but we have not built it or established P100 correctness/performance |
| [Original NURBS-Diff](https://github.com/anjanadev96/NURBS_Diff) and [current lab NURBSDiff](https://github.com/idealab-isu/NURBSDiff) | Batched surface evaluation, tensor layouts, fitting and derivatives with respect to model parameters | Original includes C++/CUDA paths; current lab implementation is pure PyTorch. Autodiff with respect to control points/weights/knots is a different contract from our geometric `Su,Sv,Suu,Suv,Svv` output; inspect each rather than assuming equivalence |

GPU-NURBS is a directly relevant additional lead beyond the initial three scout
reports. Its public repository includes a GPL license file while its README
also states academic-only use and bans secondary copying/distribution/business
use. Those statements conflict; record it as restricted/unclear source and study
the associated paper before any proposed reuse. It has not been vendored or
adopted. No P100 support or performance claim follows just from CUDA 12.2 build
instructions.

## Robust arrangements and reusable GPU execution ideas

| Reference | What to learn | Boundary of relevance |
| --- | --- | --- |
| [Geogram](https://github.com/BrunoLevy/geogram) | Exact predicates and constructions, intersection event identity, co-refinement, arrangements and region classification | [CSG paper](https://arxiv.org/abs/2405.12949) plus source; polygonal-input guarantees do not automatically cover curved NURBS |
| [OpenMeshCraft](https://github.com/mangoleaves/OpenMeshCraft) | Localized intersection resolution, indirect predicates and parallel arrangement work | `src/OpenMeshCraft/Arrangements`; author identifies `mesh_arrangements` as original research branch and says Boolean component has not been thoroughly tested |
| [CGAL corefinement/autorefinement](https://doc.cgal.org/latest/PMP_Boolean_operations/group__PMP__corefinement__grp.html) | Mesh splitting and the exact-construction-to-double-output problem | `autorefine_triangle_soup` and iterative snap rounding return failure when the bounded iteration process cannot produce intersection-free output; study its status and partial-result contract |
| [Manifold](https://github.com/elalish/manifold) | Practical Boolean decomposition, spatial filtering, manifold incidence and parallel implementation | `src/boolean*.cpp`, `src/csg_tree.cpp`, algorithm notes and adverse fixtures; manifold topology is a distinct claim from geometric embedding and curved-surface accuracy |
| [Warp](https://github.com/NVIDIA/warp) | GPU BVHs, query organization, buffer management and host/device boundaries | `warp/native/bvh.cu`, `bvh.h`, intersection headers and examples; separate from USD BRep and useful for execution machinery |
| [CCCL / CUB](https://github.com/NVIDIA/cccl) | Prefix scans, compaction, sorting and reductions for variable-size GPU output and work queues | Algorithm studies for later count/scan/fill topology stages. Current support follows the current major CUDA toolkit's architectures; a Pascal-compatible release requires separate selection and verification |
| [Fidget](https://github.com/mkeeter/fidget) / [libfive](https://github.com/libfive/libfive) | Interval pruning, specialized evaluation and hierarchical work over geometry | Implicit representation, useful for execution/region-exclusion ideas; interval bounds for polynomial/rational patches need their own arithmetic derivation |

## Newer architecture experiments and modeling workloads

[Nacre](https://github.com/elgar328/nacre), [Keel](https://github.com/redwoodrhetorica/keel),
[vcad](https://github.com/ecto/vcad), [opencadkernel](https://github.com/HakanSeven12/opencadkernel)
and [Hypermesh](https://github.com/timschmidt/hypermesh) are useful additional
architecture studies: identity, refusal/fallback semantics, tolerance versus
topological decisions, and arrangement reuse. Their published capability claims
remain subjects for source and fixture review.

CadQuery/build123d, brepjs, Dune3D and Chili3D contribute task recipes, API ideas
and interaction workloads. Their existence should not be counted as independent
proofs of the underlying kernel algorithms. The wider
[project list](PROJECTS.md) retains source-snapshot links.

## Proposed reading order

1. **First numerical unit:** compare SISL's derivative contract, openNURBS's
   evaluator and LTCSBezier's arithmetic/precision harness. Study GPU-NURBS's
   matrix approach through its paper and selected public source; inspect both
   NURBSDiff generations for batching and derivative-contract differences.
2. **First resident consumer:** study GoTools projection/trim representation,
   SMLib queries and Warp's device work organization. Qualification still comes
   from analytic/model oracles and bounded unresolved cases.
3. **Intersections and solid operations:** follow OCCT's intersection-to-building
   chain, compare SMLib and Truck, then use Geogram/OpenMeshCraft/CGAL to study
   arrangements, robust decisions and output construction.

The research lesson is to extract and compare complete algorithmic units and
their assumptions. A source read may inspire our design; copying code or adding
a dependency is a separate license and operator decision. No new libraries,
runtimes or external code were installed or adopted for this map.
