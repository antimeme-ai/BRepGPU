# Whole mesher: owned native geometry, task ownership and CPU/P100 placement

Research and architecture proposal, 2026-10-16. No product implementation,
test, CAD execution or benchmark accompanies this note. This is input to the
whole-mesher design review, not authorization for disconnected component ports.

## Decision

Build one C++20 meshing application that reads the owned source snapshot,
settles shared physical boundaries, constructs and refines complete faces,
publishes the full mesh with its source maps, and derives the existing native
display scene. CPU-only and CPU+P100 are execution policies of this application.
Neither path calls Python for geometry, refinement, scheduling or publication.
Python can remain an optional binding and an independent research/oracle reader.

The important change is the geometry/approximation algorithm and its ownership,
not a translation of the present Python loops. The algorithm research lane is
studying local control-net approximation, actual-cell residual bounds, regular
interior cells and exact connectivity near trims/transitions. This note specifies
the runtime seams those choices must satisfy. It does not settle the remaining
boundary-cell construction mathematics by naming an API.

## What exists, and what must change

| Owned component | Reuse | Required architectural change |
| --- | --- | --- |
| `native/step_source/main.cpp`, `BGSRC001` | Isolated STEP extraction; original source IDs, analytic frames, knots, homogeneous controls, extracted spans, source topology and assembly | Add a strict native reader and immutable typed source store. Keep OCCT outside the mesher. Original coefficients remain available alongside every derived cache. |
| `native/source_uv/` | Source-adapter reconstruction/branch guards and recorded conversion semantics | This component contains OCCT handles and query-shape work. It is not a reusable owned runtime evaluator. Native recovery must operate on the owned snapshot, preserving the original/recovered distinction. |
| `native/face_domain/exact.hpp`, `native/direct_cdt/` | Owned arbitrary integers, certified predicate filters, exact domain admission, direct fixed-site CDT and indexed topology | Use native typed adapters/internal interfaces; do not serialize and re-decode every face through a Python/limb C ABI. Retain the ABI for external callers. Current common-axis denominator normalization is a candidate control, not the inevitable exact representation. |
| `native/evaluator.hpp`, `native/om10/math.hpp` | Rational quotient-jet identities, analytic carrier formulas, CPU/CUDA first jets | Consolidate a common source-geometry kernel with original-span lookup, curve support, bounds and recovery. Existing display math is binary64 evaluation, not a certified control-net restriction/bound engine. |
| `native/om10/{cpu.cpp,cuda.cu,types.hpp}` | Native display API, resident scene and jets, owned stream, checked teardown and CPU display tasks | Feed an in-memory immutable scene view from native mesh output. Keep strict lifetime ownership. Extend typed error/status and resource accounting rather than reuse the simple display pool for throwing geometry jobs. |
| `BGMESH01` | Existing complete display scene interchange and independent reader | It loses edge schedules, raw/recovered coedge UV, closures, joint recovery, collapsed UV cells and quality components. It cannot be the sole full mesh artifact. |
| `tooling/native_cpu_rigor.cmake` | C++20, pinned host profile, strict FP, warnings, sanitizer/fuzz profiles | Apply to all new host units and concurrency paths. Keep the established CUDA/C++17 translation boundary; a C++20 host does not require changing the CUDA build dialect. |

Current source support includes analytic surfaces, positive rational tensor
surfaces of degrees at most three, and curves up to degree 32. A new format or
GPU specialization must not silently lower the admitted curve degree. Degree
buckets may have an explicitly checked general host path. Positive weights are
a mathematical precondition, not a reason to assume a numerically computed
weight lower bound remains positive after restriction.

`BGSRC001` already stores both original spline data and extracted spans. Their
floating coefficients are not automatically exact equivalent representations.
Native preparation must derive its own extraction/restriction enclosure from
the original coefficient words, or admit the saved spans against that source
with an explicit discrepancy budget. Treating saved extracted controls as exact
original truth would create the second geometry theory this design must avoid.

## Native values and lifetime

Use strong source IDs and dense slots with distinct types: definition, physical
vertex, physical edge, coedge occurrence, face, carrier, curve and mesh-corner.
No pointer identity, approximate XYZ weld, or UV hash defines physical identity.
An immutable `SourceStore` owns decoded values and compact adjacency ranges;
all jobs hold stable slot/range views into its lifetime. The source digest and
adapter revision accompany the store. Duplicate source IDs, bad references,
nonfinite words, section arithmetic overflow, malformed UTF-8 and unowned source
rows fail at admission. Binary parsing reads fields explicitly; native struct
layout/padding is never a disk schema.

`GeometryStore` is a derived immutable or single-owner cache, not a second source.
It contains original-span indices, exact dyadic coefficient views, bounded
extracted/restricted homogeneous nets, derivative/residual coefficients and
conservative search bounds. Preserve separate exact values, outward intervals
and approximate evaluation values in the types. Never allow an approximate jet
or cached control hull to satisfy an exact topology predicate by conversion.

Cache identity includes source identity, curve/carrier slot, exact interval or
cell restriction, chart/period branch, operation/bound revision and metric where
relevant. Boundary-dependent entries additionally include the edge-schedule
generation. A quality-specific acceptance result is separate from reusable
geometry bounds. Hashes accelerate lookup; full keys decide equality. Avoid a
global hash table of every restricted net: original-span arrays plus child
indices/path words give better locality and bounded lifetime. Store only useful
levels; evict/recompute derived data under explicit budgets.

Per-face connectivity/cell trees have one mutable owner and compact indexed
records. Recycle dead slots with generations whenever a queued handle can outlive
reuse. Packed adjacency and free lists already work in the CDT core. Reuse
high-water scratch between sequential jobs, releasing all face-local data after
copying its immutable result. A monotonic arena is appropriate for genuinely
epoch-local data, not indefinitely growing refinement queues or exact temporaries
whose intermediate allocations otherwise remain live until face completion.

The current `bgfd::Budget` is deliberately single-owner, not thread-safe. Do not
share one instance across parallel faces. Each job receives a budgeted resource;
a run-level allocator accounts actual simultaneously reserved blocks, including
arena retention and device buffers. Checked reservations throttle admission of
new jobs. The runtime must not launch all large faces and discover aggregate
memory exhaustion after duplicating their control trees.

Proposed native interface seams, to be finalized in the reviewed design:

| Interface | Input and owner | Output/failure |
| --- | --- | --- |
| `read_source(bytes, limits)` | Owned byte span/file mapping; no foreign handles | Immutable `SourceStore` or typed source/decode error |
| `prepare_geometry(source, resource, stop)` | Original coefficient words and typed source ranges | `GeometryStore` with indexed span roots and enclosures |
| `settle_boundaries(source, geometry, quality, policy)` | Canonical edge owners, incident occurrence ranges, explicit job budgets | Immutable `BoundarySnapshot` and generation, or structured failed task |
| `build_face(face, boundary, geometry, scratch)` | One face owner; immutable boundary generation | Complete `FaceResult`, canonical `EdgeSplitRequest` set, or failure |
| `bound_batch(session, tasks, destination)` | Owned task buffer with exact cell/chord/triangle identity and request generation | Bounds/certified decisions and unresolved-lane IDs; completion owns buffer lifetime |
| `publish_mesh(source, boundary, face_results)` | Stable source order; only complete current-generation results | Immutable full `MeshResult` with all provenance, counted output ranges and quality data |
| `make_scene(mesh, assembly, palette)` | Complete constructed mesh and original rigid usage tree | Owned `SceneStore`/`SceneView`, with optional BGMESH01 serialization |

`MeshingRun` owns source/cache lifetimes, task scheduler, cancellation source,
global reservations and optional device sessions. A task result owns its memory
until consumed or discarded; no span points into a worker's next reused scratch
epoch. C ABI handles can wrap these objects later. The native CLI must already
perform the whole operation with Python absent before the binding is useful.

## One native task graph

1. Decode/admit source once; build sorted dense ownership and incident-edge CSR.
   Resolve source-joint/period branch choices before dependent edge work. Cache
   immutable original geometry once per definition; assembly instances do not
   cause repeated tessellation.
2. Prepare original carrier/curve spans and geometric hierarchy roots. Independent
   carriers/curves can run concurrently. Mandatory continuity/chart/feature cuts
   belong to preparation and remain explicit source events.
3. Give each physical edge one schedule owner. It combines requests from every
   incident occurrence, produces one ordered source-parameter schedule and one
   canonical XYZ sequence, and records per-use UV/branch and mismatch bounds.
   Parallel edges do not mutate faces or neighboring schedules directly.
4. Coordinate coupled schedule constraints in connected dependency components.
   This includes the present circular-joint behavior; it is not independent
   merely because edges have separate IDs. Apply ordered, deduplicated requests
   at barriers, then invalidate only affected incident boundary cells. Preserve
   mandatory samples; a removal/coarsening operation requires separate proven
   semantics and must not be mixed into a supposedly monotone split closure.
5. Construct faces concurrently against an immutable boundary snapshot. Each job
   owns its local approximation hierarchy, trim index, exact connectivity and
   refinement queue. Emit either a complete face result, deterministic edge
   split requests, or an explicit failure. No per-face worker appends globally
   visible physical vertices or repairs another face's topology.
6. If the chosen mathematics requires source-edge splits, apply a canonical
   request round and redo only invalidated boundary work. Every accepted split
   must advance a finite checked source interval hierarchy, respect sampling/
   work limits and preserve all incidents. A work cap is an honest refusal, not
   a convergence proof. The algorithm lane must choose between this mechanism
   and boundary cells that leave the canonical schedule fixed. Arbitrary grid/
   trim intersections may not become private per-face physical edge vertices.
7. Publish by source order and canonical local keys, using counted ranges and
   checked prefix sums. Original vertices, edge samples, face-interior sites,
   corners and triangles get deterministic namespaces/ranges. Workers write
   disjoint assigned ranges. Scheduling order never determines IDs. Local
   random choices derive from run seed plus immutable face/source slot, not a
   shared RNG or worker number.
8. Constructor invariants, approximation-bound checks and complete source
   inventory checks determine construction success before atomic publication.
   Assembly transforms and explosion/labels consume the same definition meshes.
   Independent research qualification reads the actual output and original
   source off the product path; it need not run inside every CLI invocation.

For initial delivery prefer one owned bounded task scheduler over nested pools.
Queue jobs with stable IDs and size hints from actual span/boundary counts; batch
tiny homogeneous jobs, let large face jobs yield bulk bound/evaluation tasks,
and allow idle workers to steal ready work without sharing mutable geometry.
NUMA placement should keep a definition's immutable cache and face scratch near
its workers; shared source pages can be read-only. This is a locality hypothesis,
not a measured benefit. Do not pin a pathological large face permanently to one
worker merely to claim definition-level parallelism.

## P100 is part of the geometry design

The initial CPU+P100 policy retains host exact topology/domain/CDT, source branch
decisions and publication. It sends bulk regular numerical work to sm_60:
restricted positive-rational curve/surface control nets; flatness and actual-cell
homogeneous residual coefficients; interval geometric bounds; source position/
first-jet queries; and existing display evaluation/projection/raster work.
Batch by operation, degree, net shape and source span, with stable task IDs.
Do not upload one Python triangle or launch one kernel for each recursive split.

Device-resident original/span coefficient tables and batched child buffers belong
to a native `GeometrySession`. A bounded pool of pinned staging buffers and owned
streams overlaps independent face/edge batches. Launch records reference immutable
source/cell IDs and generations; returned flags cannot be applied to a newer
schedule. The coordinator owns topology updates and queue compaction after
completion. Multi-P100 jobs partition task ownership; they do not race to mutate
one cavity or implicitly duplicate the source cache without budget accounting.

For rational residuals, define the actual mathematical certificate first. For
example, on a cell with `S=N/W`, compare an enclosed Bernstein representation of
`N-W*L`, where `L` is the actual triangle/chord interpolant, against an admitted
positive lower bound for `W`. Include frame, restriction, canonical XYZ and UV
rounding effects. The residual over a rectangle is not automatically the residual
over a clipped transition triangle; the algorithm must provide the actual-cell
domain or a valid covering bound. A zero plane-distance test alone cannot certify
coverage, orientation, folds or tangential error of a nonlinear planar map.

GPU filtering accepts only when an outward enclosure proves the same defined
certificate inequality as the host path. Uncertain, nonfinite, overflowing,
underflow-sensitive or nonpositive-denominator lanes return to the owned host
interval/exact path. For polynomial/rational certificate comparisons, clear
positive denominators and use exact integer/rational fallback as appropriate;
do not put arbitrary rational inputs through dyadic-double expansion arithmetic.
Avoid global LCM normalization across thousands of unrelated rational sites:
per-point homogeneous or indirect predicates remain the serious contender.

CUDA 12.8 documents directed binary64 addition, multiplication, division and
square root intrinsics; these provide building blocks for outward interval
evaluation on the target. They do not by themselves prove a geometric bound.
See the [CUDA double intrinsics reference](https://docs.nvidia.com/cuda/archive/12.8.1/cuda-math-api/cuda_math_api/group__CUDA__MATH__INTRINSIC__DOUBLE.html).
The numeric unit needs explicit endpoint formulas, exceptional-value handling,
compiler contraction/FTZ assumptions and independently checked exact inputs.
CPU intervals must similarly state rounding/environment assumptions without
changing a shared process FP environment behind other threads.

The existing CPU/CUDA analytic evaluator calls ordinary sin/cos. That is useful
display/evaluation code, not an interval proof. Analytic bound kernels can use
certified conservative geometric formulas where valid; any tighter trig-based
decision requires an owned range-reduction/enclosure mechanism or host-prepared
certified constants. Substituting a rational-circle parameter for the original
analytic parameter requires an explicit source map and approximation budget.

To make thread-count/backend comparisons meaningful, exact certificate decisions
and deterministic tie policies should select the same splits whenever feasible.
An interval algorithm's accidental looseness must not silently define topology.
If a justified backend policy produces different valid meshes, disclose counts
and the scope difference; do not claim bitwise parity or divide unmatched times.
CPU-only and CPU+P100 must both execute the native source-to-output operation.
Report source preparation, upload, allocation, batch launches, bound work, exact
fallback, synchronization, download and publication as components of that wall.
Warm residency is an additional observation, never a replacement for cold cost.

## Errors, cancellation and publication

Use typed statuses for unsupported source, invalid domain, numerical ambiguity,
resource exhaustion, cancellation, I/O failure, device failure and internal error.
Attach stage, source ID, task/generation, processed work and live/peak resources.
Worker exceptions are captured into result slots and trigger cooperative stop;
they must not escape a thread entry or leave the scheduler's barrier incomplete.
Poll cancellation at bounded work units, not only once per completed face.
In-flight GPU work is drained before reclaiming buffers or source views. A failed
CUDA session becomes unusable until explicit reconstruction; never continue with
partially updated buffers. RAII performs cleanup on every path.

Geometry results can be deterministic without promising that simultaneous
resource exhaustion chooses the same first error under every interleaving.
Define the reported error ordering and cancellation semantics honestly. Retain
per-job counters, wall intervals and output counts; distinguish critical-path
wall, summed overlapping worker wall, and CPU time. Progress names current stage
and task rather than suggesting a face percentage is a runtime percentage.

Write to a run-owned temporary destination, complete the constructor's direct
invariants and bound checks, close successfully, then publish with an atomic
same-filesystem rename. On failure
leave diagnostics and explicitly incomplete scratch only, never a valid-looking
mesh header or success status. No complete assembly artifact may omit a failed
definition or replace it with its bounding box.

## Full mesh versus display binary

Propose a versioned sectioned full-mesh binary alongside unchanged `BGMESH01`.
Its sections retain source digest/revision and quality; complete definition/face
inventory; canonical physical vertex provenance; ordered edge source parameters;
per-coedge raw pcurve parameter/UV and owned UV/recovery/period branch; boundary
loops; corner UV/carrier/orientation; triangles and collapsed UV cells; chart
closures/joint recoveries; and bounded error components. Exact rational values
use canonical signed-limb descriptors where binary64 is insufficient. Use
explicit counts/offsets, checked section lengths and reserved fields.

Retain coverage-cell domain/map, singular quotient identity and the referenced
actual physical facet. Do not assume one UV triangle corresponds to one XYZ
triangle: at a pole, a rectangular UV cell can map to one physical fan triangle.
The source/cell/physical relation must express that quotient directly rather than
invent a degenerate displayed triangle to satisfy an overly narrow schema.

The independent research reader consumes the actual stored mesh and original
source; producer status or producer bounds alone do not establish research
qualification. The existing Python/Fraction reader can retain that role off the
product path. Native ownership does not create a need for a second native checker
or for a new certification stage before every publication. Constructor invariants,
bound checks and atomic publication remain the production mechanism. A separate
native checker would require a distinct fault class and an actual consumer need,
neither established by this architecture proposal.

Source maps, approximation components and original parameter/branch values carry
the semantics needed to use and independently inspect the mesh. They are not a
receipt/certificate protocol. Construction success and independently observed
qualification remain distinct claims. The design review must settle the semantic
schema before implementation rather than retrofit missing source associations
after rendering.

`BGMESH01` remains a derived GPU-friendly scene: definition ranges, instances,
display vertices/corners, triangles, carrier views, controls and materials. Its
intentional corner duplication preserves different UVs/normals at shared physical
vertices. Extract its scene construction into a native view builder; do not
duplicate every placed definition's geometry or reread a scene from disk solely
to hand it to a renderer already in the same native application.

## Reference implementation lessons read for this proposal

**NVIDIA USD BRep / SMLib**, pinned source
`c9c5979f04cea8c38213004d4f04df0dcfb40d05`, `SmTess.cpp` Phase1SetupBrep,
DoTessellation, Phase2CreateFacePolygons and Phase3DeleteFacePolygons;
`SmCurveCache.cpp` BuildTree/Tessellate; shipped 1.0.0 `SmTess.h`,
`SmCurveCache.h`, `SmApiGeneral.h` and `SmApiUsd.h` were read. The implementation
settles shared edges against surface breakpoints/node sizes and polyline
crossings, maintains face-local trim/surface caches, and can emit/free one face's
polygons at a time. Curve caches retain Bezier spans and block-managed tree data.
Learn phase boundaries and cache lifetime; do not copy its mutable source-BRep
splitting, approximate acceptance or partial-success policy. The shipped API
warns that arbitrary calls share an unlocked global context; process workers
are appropriate for that reference, not an argument that an owned runtime must
use processes. Scene APIs can report success while skipping failed members.
Read [pinned source](https://github.com/NVIDIA-Omniverse/usd-brep/blob/c9c5979f04cea8c38213004d4f04df0dcfb40d05/source/SMLib/src/SmTess.cpp).

**OCCT 8.0.1**, pinned
`b8f597c677811d1f9f4d8a97f5ae2825c0353a42`: BRepMesh_Context establishes model,
edge, healer/preprocessing, face and postprocessing stages. BRepMesh_EdgeDiscret
parallelizes edges and updates all incident pcurves; BRepMesh_FaceDiscret creates
face-specific algorithm objects, parallelizes faces and records per-face failure/
user-break. Learn parallel ownership barriers, not OCCT meshing or healing as a
production dependency. [Pinned face stage](https://github.com/Open-Cascade-SAS/OCCT/blob/b8f597c677811d1f9f4d8a97f5ae2825c0353a42/src/ModelingAlgorithms/TKMesh/BRepMesh/BRepMesh_FaceDiscret.cxx).

**Triangle 1.6**, retained Geogram third-party `triangle.c`: memorypool,
poolinit/restart/alloc/dealloc and triangle allocation/traversal were read.
Block allocation, free-item reuse and per-mesh pools improve locality without a
general pointer graph. Restart retains blocks: count reserved storage, not only
live records. Traversal visits dead pool entries unless explicitly excluded.
Do not adopt pointer-bit packing or licensing as an owned dependency. Shewchuk's
[primary implementation discussion](https://www.cs.cmu.edu/~quake/tripaper/triangle2.html)
reports favorable compact triangle-record results for his implementations;
it does not predict a modern speedup factor for this mesher.

**Robust numerical literature:** the existing predicate study reads Shewchuk's
adaptive predicates, Attene's indirect predicates and Bartels/Fisikopoulos/Weiser
floating filters. Their relevant lesson is a filtered exact decision derived for
the actual represented values, with explicit FP assumptions. They do not justify
turning arbitrary rational UV into binary64 and calling an expansion predicate.
See [the retained predicate study](2026-10-16-direct-cdt-predicate-design.md).

No new runtime library, foreign code or dependency is selected here. Existing
snapshot restoration manifests remain the source acquisition record.

## Review blockers before the coherent implementation plan

- Settle boundary-cell construction and any shared-edge split closure. Grid
  conformity cannot be delegated to a runtime without geometric semantics.
- Define original-coefficient extraction/restriction enclosures and the actual
  cell/trim residual contract, including nonlinear planar maps and poles.
- Select exact rational/indirect predicate representation and a CPU/GPU bound
  decision contract that can be implemented under the actual compiler profile.
- Set the full-mesh semantic schema, complete source inventory, common
  display-ready comparison scope and the existing independent research reader's
  actual-output obligations. Do not add a second qualification mechanism merely
  because production ownership becomes native.
- Select independent workload families before tuning: highly nonuniform spans,
  nonlinear flat parameterizations, varying weights, rotated thin trims, many
  holes, seams/poles, C0 cuts, tiny features, large faces and skewed assemblies.
  Measure work/bit growth, output density, memory and whole-operation cost;
  keep regressions/censors, not a numeric SLA masquerading as algorithm ambition.

These are design obligations, not reasons to start another isolated port. The
native runtime, stronger meshing algorithm, output semantics and CPU/P100
placement must meet at one reviewed source-to-output contract; independent
research qualification attacks its actual results without becoming a hidden
production dependency.
