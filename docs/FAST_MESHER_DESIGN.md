# Fast mesher design

Status: draft for operator review, 2026-10-17, branch `research/fast-mesher-study`.
Decisions and their sources are in
[journal/2026-10-17-fast-mesher-direction.md](../journal/2026-10-17-fast-mesher-direction.md).
Evidence comes from seven research reports written this shift:

| Report | What it settles |
|---|---|
| [Owned mesher audit](../papers/2026-10-17-owned-mesher-audit.md) | Where today's 847.7 s goes; what to keep |
| [NVIDIA SmTess study](../papers/2026-10-17-nvidia-smtess-study.md) | How the reference is fast, and where its density rule understates error |
| [Fast tessellation literature](../papers/2026-10-17-fast-tessellation-literature.md) | Density rules, watertightness, GPU/SIMD organization |
| [Test plans: CAD tessellators](../papers/2026-10-17-mesher-test-plans-cad.md) | Field failures and oracle gaps in OCCT, Gmsh, Netgen, FreeCAD and others |
| [Test plans: mesh generation](../papers/2026-10-17-mesher-test-plans-meshgen.md) | Oracle gaps in CGAL, Triangle, TetGen, Manifold and others |
| [Parallel growth clusters](../papers/2026-10-17-parallel-growth-clusters.md) | Splitting work inside and across faces |
| [Tolerant geometry and healing](../papers/2026-10-17-tolerant-geometry-healing.md) | What counts as truth when an edge and its surfaces disagree |

Marking used below: **[operator]** is the operator's decision. **[call]** is the
designer's call, open to override. Everything unmarked follows from those
decisions or from the cited evidence.

## 1. Intent

BRepGPU must beat NVIDIA USD/BRep at every stage, starting with meshing. Speed is
second only to correctness, and correctness without speed does not matter. The
tension between them is where this work is won or lost. **[operator]**

Correctness means fidelity: people can reasonably send the output through other
processing into a machine that cuts or forms real material. Dead serious, not
aerospace-insane, and visually stunning. **[operator]**

Today we lose by a wide margin. Owned meshing of the OM10 watch takes 847.7 s.
NVIDIA's whole preparation plus SDK batch, which contains its meshing, takes
6.1–7.6 s; its summed tessellation is 22.9 CPU-seconds. Two multipliers make the
gap: about 69× more CPU work, and about 1.86 of 12 cores kept busy. Perfect
scheduling of today's work would still take 70–131 s. About 75% of the time
(inferred, band 65–90%) goes to runtime certification: exact rational
acceptance, interval enclosures, per-facet proofs and a hot-path
self-intersection check that found nothing in 107 definitions.

## 2. Goals, non-goals, anti-goals

**Goals**

1. Mesh arbitrary B-rep input extremely fast at the requested tolerance:
   faster than a canonical pinned NVIDIA baseline on every input class of the
   arena corpus (§9), not only on the watch. **[operator]** The operator's aim
   is every stage; this unit's compared scope is the meshing stage: owned
   meshing from decoded source against NVIDIA's `SmApiTessellate` per
   definition, with import, conversion and preparation timed and reported as
   their own stages. The configuration that must win is the shipped default on
   one full lane (its socket plus its GPU); CPU-only results are reported
   beside it. **[call]**
2. Deviation actually within the request: no emitted triangle farther than ε
   from the repaired target solid, by construction from the geometry (§4).
3. Output that slicers and CAM accept without repair: watertight, manifold,
   consistently oriented, indexed, no T-junctions, no unwelded vertices, every
   input face accounted for.
4. Handle stupid input well: no hangs, no unbounded loops, no runaway memory,
   no silent bad meshes, no silently dropped faces.
5. Stunning shading: exact per-corner normals, sharp edges stay sharp, smooth
   joins stay smooth.
6. Run on CPU with SIMD and on GPU. P100 (CUDA 12.x, sm_60, C++20 device code)
   today; newer NVIDIA GPUs and Apple Silicon later. **[operator]**

**Non-goals**

- Runtime certification of any kind. It is excised from the repository, not
  kept as an option. **[operator]**
- Bit-identical output across CPU and GPU, or across runs. Functionally
  identical is the bar: every backend meets tolerance and the topology
  guarantees. **[operator]**
- Preserving today's output format, facet order or IDs. Everything is fair
  game as long as correctness and the topology/history guarantees hold.
  **[operator]**
- Sewing unsewn sheets. If ever needed, it is a separate explicit stage.
  **[call]**
- Mechanism physics, animation and modeling edits. They are on the critical
  path later and must not be boxed in (§11).

**Anti-goals: failure even if it "works"**

- Overfitting to the watch. Admission, dispatch, tolerances and capacities are
  never keyed to watch IDs, names, dimensions or observed counts (AGENTS.md).
  **[operator]**
- A silent accept anywhere: a cap reached and output emitted as valid, an
  n-gon left in place, a collinear ear clipped "anyway", a dropped face.
- A second verification system grown around the first (BLACKBIRD tenet 5).
- Stored B-rep tolerance quietly coarsening the mesh (OCCT's 2× floor).

## 3. Constraints

- **Hardware (fleet host).** 2× Xeon E5-2697 v2: AVX, no AVX2, no FMA, no
  AVX-512. 24 cores / 48 threads, two NUMA nodes of ~387 GB. Two P100-PCIE-16GB:
  GPU0 on node 0 (even CPU ids), GPU1 on node 1 (odd ids). Governor `schedutil`;
  other services share the host. Read 2026-10-17.
- **Toolchain.** This unit builds on the existing pin: Clang 18.1.3 and
  libstdc++ through `tooling/native_cpu_rigor.cmake`, C++20 host code, CUDA 12.8
  with C++20-clean device code. The C++20/C++26 split is the next project and
  follows the verbs repository's Clang stance. New code avoids constructs that
  would block a C++26 lane and keeps device-visible headers C++20-clean.
  **[call]**
- **Floating point.** FMA and contraction are allowed in general code.
  `-ffp-contract=off` and `--fmad=false` are no longer required. Code whose
  sign decisions must be correct (triangulation predicates) controls its own
  arithmetic explicitly; see §6.3.
- **Execution.** All geometry, build, test, render and benchmark execution runs
  on the fleet, never on the laptop.
- **Ingress bridge.** This unit keeps the pinned OCCT 8.0.1 STEP/XDE bridge
  (`native/step_source`) as the reader, extended for the corpus: OCCT pcurve
  reconstruction turned off so missing pcurves reach our own projection;
  refusal per entity instead of per file; units converted to mm at ingress;
  revolution, extrusion, offset and Bézier surfaces passed through as their own
  kinds where owned evaluation exists, otherwise converted to B-spline with the
  conversion error carried as a representation reserve (§6.1). An owned STEP
  reader belongs to the import stage, later. **[call]**
- **Libraries.** Own the machinery. Reference implementations are studied, not
  adopted. Any proposed dependency is discussed with the operator first.

## 4. Fidelity contract

1. **Tolerance.** ε is the caller's requested chord tolerance, expressed in
   world units. It is a request, not a physical absolute. Default for the
   watch stays 0.005 mm.
2. **Deviation.** Error is measured against the desired final position, the
   repaired coherent solid, not against the source's starting gaps.
   **[operator]** Every point of every emitted triangle lies within ε of that
   target. Away from inexact edges the target is the face's own surface. Where
   an edge was resolved (R2–R4), the target for each face is its surface
   displaced along its normal by a field equal to the recorded edge
   displacement at the edge and decaying linearly to zero across the boundary
   band. **[call: the displacement field's shape]** The source's own
   discrepancy d is a metric on starting conditions: it is reported per edge,
   never charged against ε.
3. **Topology.** Each physical edge has one vertex chain shared by ID between
   all incident faces, with one 3D position per ID and one UV per
   (ID, face occurrence). Seams carry two UV occurrences of one chain. Poles are
   one vertex with a fan. Every edge in a closed shell has exactly two uses.
4. **Orientation.** Every triangle is positively oriented in its face's UV chart
   and agrees with the face sense in 3D: the triangle normal against the surface
   normal at its UV centroid.
5. **Accounting.** Every input face yields triangles or an explicit refusal
   record with a reason and its source location. Every input definition and
   occurrence appears exactly once.
6. **Status.** Each face, part and run reports one of: `valid`,
   `valid_goal_unmet` (for example a quality target not reached, with the
   reason), or `refused`. Reaching a cap never produces `valid`.
7. **Shading.** Per corner, the exact surface normal at that corner's UV, per
   face occurrence. Shared smooth (G1) edges may share a normal; creases never do
   (§5.2 continuity rule). Triangles with a 3D angle above about 150° are
   refined away.
8. **Self-intersection.** No pair of emitted triangles intersects except along
   shared mesh edges and vertices. It is established by construction: §5.6.
9. **Invalid inputs people still use: faithful plus reported. [call]**
   - Edges with three or more face uses get one shared chain used by every
     face; the output is reported non-manifold at those edges.
   - Open shells and sheets mesh faithfully and are reported open.
   - Where the source itself self-intersects (within or between bodies of one
     definition), the mesh reproduces it and the run reports the intersecting
     source face pairs; the self-intersection oracle exempts only those
     reported pairs.
   - Refusal is reserved for input that cannot be meshed (R6, ingress refusal).
   - Exports whose format forbids these conditions (3MF requires two uses per
     edge) refuse the affected part with a reason instead of writing an
     invalid file.

## 5. Architecture

The pipeline runs as one global task graph over every definition in the input,
never a serial loop over definitions. Stages within the graph are dependency
edges, not barriers.

### 5.1 Ingress (once per source)

- Decode with the existing `SourceStore` path.
- Refuse non-finite coordinates, knots and weights; zero or inconsistently
  signed weights; broken knot vectors; empty or unclosed loops. Refusal is per
  entity with a record; the rest of the source proceeds.
- Record stored tolerances as metadata only. They never set density.
- Classify each surface: plane, cylinder, cone, sphere, torus, other analytic,
  polynomial B-spline, rational B-spline, other (offset, etc.).
- Classify each edge by use count (1, 2, ≥3, seam) and curve kind.
- Compute and cache per-carrier geometry: radii and frames for analytic
  surfaces; for each B-spline Bézier span, recentred second-difference bounds
  `D_uu, D_uv, D_vv` (§6.1). Cache keyed by source identity, so repeat queries
  and incremental remeshing reuse it.
- Each repeated instance of a definition is meshed once and placed many times.

### 5.2 Edges: sample, measure, classify, resolve

All edges of all definitions form one batch.

1. **Sample count.** The maximum of the curve's own rule and every incident
   face's admissible length along the edge (§6.1). Lines need no interior
   samples. Circles use the closed-form sagitta. Vertex IDs are issued here.
2. **Measure.** At each sample, evaluate the 3D curve and find each incident
   face's UV, from the pcurve when it agrees and by seeded projection otherwise.
   Record each face's distance `d_f`, the surface normals, the dihedral angle θ,
   and the minimum move onto both surfaces,
   `|x| = sqrt(d₁²+d₂²−2d₁d₂cosθ)/sinθ`. Missing pcurves cost one seeded
   projection per sample; no special path.
3. **Classify and resolve** (first match wins; healing report §7.2):

   | Class | Condition | Shared position |
   |---|---|---|
   | R1 exact | max d_f ≤ η | 3D curve |
   | R2 near-exact | max d_f ≤ β | 3D curve; move recorded |
   | R3 transverse | max d_f > β, \|x\| ≤ β everywhere | Two-surface Newton onto S₁∩S₂ |
   | R4 tangent | max d_f > β, \|x\| > β somewhere | Owner face's surface; mismatch recorded on the other face |
   | R5 single-face | one incident face | 3D curve if within β, else the face's own surface |
   | R6 gross | d_f > G, failed or non-monotone projection, or Newton not converged within its geometry-derived bound | Refuse the edge and every face using it, with records |

   Defaults η = ε/16, β = ε/2, G = min(4ε, ¼ local feature size). These are
   flags in the arena, not constants. **[call]** Re-intersection (R3) may move a
   shared boundary off the stored 3D curve; this supersedes the earlier
   "spatial edges are skeleton authority" proposal. **[call]** R4 owner order:
   analytic over freeform (plane frozen first, then lower curvature), then the
   face whose pcurve lies nearest the 3D curve, then larger area, then lower ID.
   **[call]**
4. **Vertices** resolve after their edges: stored point if within β of every
   resolved edge end; else three-surface Newton when the surfaces are pairwise
   transverse; else the centre of the smallest enclosing ball of the edge ends
   (never an average); refuse above G.
5. **Small features** collapse virtually and bottom-up, in the mesh only, never
   in the source: edges inside their end vertices' β balls become one vertex;
   faces lying within β of their boundary mesh from boundary vertices alone.
6. **Continuity** per edge from θ: ≤0.01° throughout is G1 (shared normal
   allowed); ≥1° anywhere is a crease; in between, follow the source's encoded
   regularity, else crease.

An edge chain is final only when items 1–4 are settled. A face consumes no
provisional chain unless the speculative-variant flag (§7) says so.

### 5.3 Faces: density field, interior, band

Every emitted edge, wherever it lies in the face, is sized by the face's local
admissible length. That closes the reference's gap of sizing cells and then
emitting other triangles.

- **Planes.** No interior points. Constrained triangulation of the boundary
  chain only.
- **Cylinders and cones.** θ-slab constraint lines spaced Δθ_max apart; no
  samples along the ruling. Untrimmed faces are strips of 2n triangles.
- **Spheres.** Triangle circumradius ρ with ρ² ≤ 2Rε − ε².
- **Tori and B-splines.** A 2:1-balanced dyadic quadtree over UV, sized from the
  span bounds through the anisotropic Waldron rule (§6.1). Interior leaves
  become conforming quads; hanging sides use the existing packet zipper
  (`adaptive_packets.cpp`). Cells crossed by the trim are deleted; the band
  between the trim chain and the lattice rim is triangulated by constrained
  Delaunay with `direct_cdt`, fed fixed-point integers taken straight from
  doubles. A metric constrained Delaunay generator is the arena alternative.
- **Band rules.** Each connected band component (one per hole) is an
  independent triangulation. Band refinement inserts points only in the
  interior, never on the shared trim chain. There is no "clip anyway", no
  collinear-ear path and no n-gon output; a face that cannot be triangulated is
  refused with a record.
- **Orientation.** Robust orientation in UV, plus each new triangle's 3D normal
  checked against the surface normal at its UV centroid. A fold that is valid in
  UV but reversed in 3D refines or refuses.
- **Termination.** Quadtree depth, band refinement rounds and flip counts are
  bounded by limits derived from ε, the local feature size and the
  floating-point floor. Reaching a bound is `valid_goal_unmet` or `refused`,
  never `valid`.

### 5.4 Splitting large faces

From the growth-cluster study: grow-then-weld is rejected; agree-first on a
shared lattice is the form that works.

- Inside a face, clusters are weighted Hilbert ranges of coarse quadtree blocks.
  Seam vertices are keyed by integer lattice coordinates, so both sides produce
  identical vertices and no weld exists.
- Long band components split at constrained separators that end on existing
  trim samples.
- Correctness oracle: a face's keyed triangle set is identical for every
  cluster count (mesh(K=1) ≡ mesh(K=k)).
- Built behind a flag and included in the arena. Before it is built, the
  per-face predicted work distribution is measured; it tells the arena where
  to expect an effect (batch workloads versus single-part remeshing and the
  GPU), not whether to build it.

### 5.5 Scheduling, memory, output

- **Work estimate before work.** Per item: lattice cells from ∫√det M, band
  triangles from trim samples × layers, edge samples. Over-decompose and
  schedule largest-first across all definitions in one pool, with no barrier
  waves.
- **NUMA.** One pool per socket, memory first-touched on its node. A GPU is
  driven only from its own socket.
- **Memory.** Per-task arenas. No run-wide atomic metering per allocation. A
  per-run RSS and device-memory budget is enforced; exhaustion is a refusal with
  caller and device state recovered.
- **Output size.** A requested ε that implies an absurd triangle count is
  refused up front from the work estimate.
- **Output.** Struct-of-arrays positions, per-corner normals and UVs, indices,
  and per-vertex/per-facet source maps (source edge node or surface root,
  generator, owner, revision, edge provenance). Per-edge records: class, max
  d_f, max |x|, min θ, owner, vertex moves. Per-face status records. Facet
  order is canonical by key, so output does not depend on completion order.
- **Export.** This unit writes the internal schema, binary STL and 3MF
  (flattened assemblies for STL; 3MF with components for instances). USD output
  comes later. **[call]** The validity oracles also run on written bytes,
  because a mesh valid in FP64 can self-intersect once narrowed to 32-bit STL.

### 5.6 Self-intersection by construction

Each face's triangles lie within ε of its own surface. Two faces whose source
surfaces are more than 2ε apart therefore cannot produce intersecting triangles.
The risks are source features thinner than 2ε: thin walls, narrow gaps, and
knife-edge dihedral angles at shared edges. Before meshing, a face-level query
over ε-inflated face bounds finds candidate pairs; for those, the local
tolerance tightens or the faces are refused when the feature is below what the
request can represent. This is a planning input to density, not a check of the
output. The full self-intersection check stays an offline oracle. **[operator:
accepted as default]**

## 6. Numerics

### 6.1 Density rules

- **Waldron (sharp).** Linear interpolation error inside triangle T is at most
  ½·r_min(T)²·M, where r_min is the smallest enclosing disk radius and M bounds
  the second directional derivative. Size by r_min, never by edge midpoints:
  midpoint and centroid checks understate by up to 4/3.
- **Analytic closed forms.** Cylinder `Δθ_max = 2·acos(1 − ε/R)`; cone with the
  largest radius in the slab and an apex pole; sphere as in §5.3; torus by the
  anisotropic Waldron form with `‖S_uu‖ ≤ R_c + r`, `‖S_φφ‖ = r`, `‖S_uφ‖ ≤ r`.
- **Polynomial B-spline span** of degree (p, q):
  `D_uu = p(p−1)·max‖Δ²_u P‖`, `D_uv = pq·max‖Δ_uΔ_v P‖`,
  `D_vv = q(q−1)·max‖Δ²_v P‖`, recentred on the span's bbox centre.
- **Rational span.** Zheng–Sederberg Theorem 4 on homogeneous control points,
  recentred, with the span's own minimum weight. Not Cheng 1992, not
  Piegl–Richard, not the adjacent-weight-ratio bound.
- **Edge curves.** Line none; circle sagitta; ellipse a/b² curvature; B-spline
  ⅛Δt²·p(p−1)·max‖Δ²P‖ per span; rational by recentred Zheng–Sederberg.
- **Representation reserve.** Any approximant (offset surfaces, reconstructed
  pcurves) reserves its own measured error out of ε.

### 6.2 SIMD and GPU batches

- One batch layout serves CPU SIMD lanes and GPU warps: flat SoA job arrays
  (item id, u, v) sorted by surface kind and degree class; control data SoA per
  class (x[], y[], z[], w[]) padded to the class degree.
- SIMD widths vary: AVX on the fleet host (4 doubles, no FMA), AVX2/AVX-512 on
  RunPod hosts, NEON (2 doubles) on Apple. The C++20 lane uses a thin owned
  layer over Clang vector extensions; `std::simd` arrives with the C++26 lane.
  **[call]**
- GPU work is batched evaluation, sizing, lattice emission and normals across
  all faces at once, sized by exclusive scan and written once. Bands stay on the
  CPU unless the arena shows otherwise. No CUDA dynamic parallelism.
- Metal has no `double`, so evaluation is formulated to allow FP32 with
  recentred coordinates from the start.

### 6.3 Predicates

Triangulation orientation and in-circle decisions use Shewchuk-style adaptive
predicates: plain floating point, with extra precision only when a decision is
ambiguous. This is correct arithmetic on the input coordinates, not
certification. nvcc contracts multiply-add into FMA by default, which breaks
these predicates, so predicate code uses explicitly uncontracted operations (or
an FMA-aware formulation) on every device. A plain-double variant runs in the
arena against the robust one. **[operator: arena both]**

## 7. Staged resolution and the background healer

Single pass is not required. **[operator]**

- **Immediate pass.** R1/R2 edges and V1 vertices commit at once. A face whose
  edges and vertices are all committed meshes at once.
- **Deferred queues.** R3 two-surface Newton, vertex three-surface Newton, R4
  owner projection, missing-pcurve continuation, small-feature collapse.
- **Background healer.** A resolver drains the queues concurrently with the
  immediate pass. It resolves items directly or holds them until a SIMD- or
  warp-sized batch accumulates or the immediate pass drains, whichever comes
  first. Each face keeps a pending counter over its unresolved edges and
  vertices and meshes when it reaches zero. No surveyed system runs a
  background healer; it is built and measured anyway, and its data is kept
  whatever it shows. **[operator]**
- **Variants, all flagged:** background queue; inline resolution; single pass;
  speculative (a face meshes against a provisional R2 chain and patches
  boundary positions later, keeping IDs, valid only if every boundary triangle
  stays positively oriented in UV).
- **Provisional state is first-class.** Each item carries its status, its queue
  membership and its dependent faces. Today it is internal, aimed at wall-clock
  time. Later, sharpen-in-place while a user watches becomes a display consumer
  of this state, not a redesign. **[operator]**
- **Incremental remeshing is the same mechanism.** Settling or editing an edge
  invalidates only its incident faces. The complication roadmap uses this path.

## 8. Certification removal

The new path replaces the old one; there is no opt-in mode.

- **Removed from the hot path:** Rational curve parameters and site UVs;
  per-facet certified upper bounds and their Rational compare against τ;
  precise and materialised fallback cascades; certificate-driven horizon
  refinement; interval bound kernels used as acceptance tests; Rational SAH
  costs; per-read GCD; snapshot copies of exact tables; the whole-component
  contact classification (165 s); the stop-token check on every work charge;
  run-wide atomic metering of every allocation; per-query source endpoint
  guards (they move to ingress, once per source).
- **Kept and ported:** the `DefinitionBoundaryForest` model on dyadic paths or
  doubles; identity and provenance types (`PhysicalNodeKey`, `FacetKey`,
  site sources, occurrence and wedge bindings, edge provenance); the
  occurrence and joint logic of `prepare_face_domain` as specification; the
  packet zipper; `direct_cdt` with fixed-point inputs; the numeric kernels and
  CUDA ops; canonical ordering by key; source decode and IO codecs.
- **Moved to the test suite:** the contact BVH and filtered predicates, and the
  independent original-source readers, as offline oracles.
- **Retired:** from this point in the source graph forward, the old mesher is
  no longer part of the system. **[operator]** Its code path leaves the build;
  every consumer (watch render, exploded diagrams, scene/display, cut and
  extrusion result meshers) moves to the new path. Old results, images and
  reports remain, each write-up gaining one line saying it is historical
  information from the previous regime. AGENTS.md and current docs lose the
  certification language that described the old mesher.
- **Other operations.** Certification removal is repository-wide, not only
  the mesher. **[operator]** This unit does the mesher. As soon as it is done,
  the same removal proceeds through the Boolean, classification, arrangement,
  intersection and other operations, in an order chosen then.

## 9. Arena and ablation

### 9.1 Always-on core

Edge-first shared IDs; per-face accounting; analytic closed-form density;
global task graph; certification removed; three-way status.

### 9.2 Flags

Primitive switches read at process start:

| Flag | Levels |
|---|---|
| Triangulation predicates | robust adaptive / plain double |
| Evaluation precision | FP32 recentred / FP64 |
| B-spline face generator | lattice + band / metric CDT |
| In-face splitting | dyadic-lattice clusters / off |
| Staged resolution | background queue / single pass |
| Healing thresholds | η, β, G default / alternate set |
| SIMD path | vector / scalar |
| GPU offload | P100 / CPU only |

Inline resolution and the speculative-chain variant are follow-up levels of
staged resolution, explored after the first arena if the staging effect is
intriguing. **[call]**

### 9.3 Design of the run

- 64 configurations: a 2^(8−2) resolution V fraction of the eight two-level
  flags, which keeps every main effect and every two-way interaction clear of
  each other. Keeping all eight flags two-level is what preserves that; a
  three-level factor coded in two columns would drop the design to resolution
  IV. **[operator: 64; call: two levels per flag]**
- Explore further in configuration space only around something intriguing, and
  then follow it all the way with no time box. **[operator]**
- Permutations run on the fleet as their mechanisms become available, while
  delivery continues; nothing waits for all eight. **[operator]**
- The 64-run result sets provisional defaults, never a law. **[operator]**

### 9.4 Corpus

- ABC: a fixed random sample for speed, a larger sweep for robustness. ABC's
  pipeline filtered out models that needed healing, so it is sloppy, not
  adversarial.
- NIST/CAx-IF STEP fixtures; OCCT regression shapes from the public dataset
  (121 of 210 file-using mesh bug cases have fixtures); step-conformance (CC0,
  exact geometry included, used as corpus, not authority); resources4truck.
- Authored families: poles, seams, tiny features beside huge faces, near-tangent
  trims, wild and zero/negative weights, periodic surfaces, zero-length edges,
  tolerance coarser than features, 1e6:1 scale; plus from the test-plan gaps:
  edge-to-surface offsets 1e-6..1e-2, dihedral sweeps across R3/R4, pcurve-free
  variants, vertices off curves, plane–cylinder tangent steps, slivers at η/2,
  η, 2β; closed faces bounded by two circles with no seam; apex without vertex;
  trim loops crossing the seam; multi-turn faces; shifted periodic ranges; sheet
  bodies and open shells; instanced assemblies; N-holes/N-grooves/N-instances
  scaling families; 100 m offsets; inch files; mirrored and scaled placements.
- A mutation fuzzer over STEP and B-rep parameters, including loop
  permutations, decimal spellings and NaN/Inf injected after ingress (at
  evaluator, projection and sizing outputs). Timeout counts as failure; seeds
  are logged; failures minimise into permanent regressions.
- The watch, always.
- Reports are stratified by input class, so a regression in one class is not
  diluted by corpus-wide rates.

### 9.5 Execution

- Two independent lanes, each pinned to one socket, its memory node and its
  local GPU. One reference configuration repeats on both lanes to detect
  interference.
- A canonical pinned NVIDIA run set under the same pinning, at least two
  repetitions per lane, on the same corpus sample and the watch at the same ε.
  Owned results compare only against it; earlier NVIDIA numbers are
  historical. NVIDIA's output goes through the same fidelity oracles.
  **[operator]**
- Every timing row emits: lane (socket, GPU, CPU set, memory node); competing
  non-test load during the run (load average, non-arena processes above a CPU
  threshold, other GPU processes and utilization, memory pressure); observed CPU
  frequency and governor. **[operator]** The governor is left as found and
  recorded until the operator decides otherwise. **[call: default]**
- Each configuration records per input: wall time, CPU time, work counts, peak
  memory and the full fidelity verdict. Correctness failures are never averaged
  into speed.

### 9.6 Tolerance and scoring **[call]**

- **ε per input.** Fixed before any result is seen. ε is a request, not a
  physical absolute: what people read from their metrology is treated as
  absolute, and that interpretation is where the hardest problems live.
  **[operator]** The arena therefore spans the requests people actually make
  rather than claiming one correct value:
  - inputs with stated real-world units (the watch, NIST/CAx-IF,
    step-conformance, OCCT fixtures with units) run at 0.005 mm and 0.025 mm,
    requests drawn from fine machining and from slicer/printing practice, and
    also at 1e-3 and 1e-4 of their bounding-box diagonal;
  - inputs whose scale is not a reliable physical size (ABC models are user
    sketches at arbitrary scale) run only at the relative levels.
  - The output-size guard applies at every level. **[call]**
- **Steel-man NVIDIA.** The only publishable numbers start from NVIDIA at its
  strongest. **[operator]**
  - NVIDIA reads only `.brep`; a STEP user gets there through OCCT. Its input
    arms are OCCT's default STEP shape processing and our unprocessed read
    (today's references use the unprocessed read), crossed with its two
    healing policies, at its default angles and the requested chord.
  - Per input, NVIDIA's time is its fastest arm that produces a topologically
    valid mesh: no refusal, no hang, no n-gon, no non-manifold output beyond
    the source's own. Its measured deviation is reported beside that time and
    never used to slow it down. **[call]**
  - OCCT healing time lands in NVIDIA's import stage, not its meshing stage.
- **Scoring per input:**

  | Owned | NVIDIA (best arm) | Score |
  |---|---|---|
  | valid | valid | speed ratio; the only publishable comparison |
  | valid | no valid arm | recorded as a finding; not a win, not in the ratio |
  | refused or fails | valid | owned loss, counted against the goal |
  | refused or fails | no valid arm | recorded; excluded from speed |

- The speed goal is judged on ABC, NIST/CAx-IF, the OCCT dataset,
  step-conformance and the watch. Authored adversarial families and fuzz
  mutants count toward robustness only.
- Input classes for stratified reports are fixed before results: dominant
  surface kind (analytic only / mixed / B-spline heavy), size by face count
  (small, medium, large), and whether the source needed healing classes R3–R6.

## 10. Verification stack

Every oracle changes the exit status. Several mature projects ship oracles that
cannot fail; none of ours may.

- **Deviation, two-sided and sound.** Distance to the repaired target of
  §4.2 over the trim domain with a covering-radius correction or a bounded two-sided Hausdorff, so
  dense sampling is a bound, not an estimate. Surface-to-mesh direction finds
  missing surface. Boundary triangles and pole/apex triangles are included,
  where others skip them.
- **Exact per-face UV oracles** on every triangulation, CPU and GPU: every
  constraint segment present; constrained-Delaunay property where claimed;
  orientation independent of topology; coverage with no overlap and no gap;
  Steiner points on their parent constraints.
- **Topology:** shared-edge identity and order across incident faces before any
  consumer joins them; edge-use counts; closed-shell Euler where closed;
  per-face accounting.
- **Self-intersection** (offline): the contact BVH from the old mesher,
  including adjacent and coplanar pairs.
- **Metamorphic:** whole-model scale by 1e±6, far translation and rotation with
  identical topology; mesh(K=1) ≡ mesh(K=k) for splitting; validity across
  thread counts, backends and repeated runs; fresh versus incremental remesh
  equivalence.
- **Forced caps:** tests that force each work, depth, iteration and memory
  budget to fire, then check the status and the returned object.
- **Per-device predicate conformance** before any GPU triangulation result is
  believed.
- **Export:** validity rechecked on written bytes.
- **Negative controls:** each oracle is specified against the degenerate
  configurations it must flag (adjacent fold, coplanar overlap, T-junction,
  position-coincident but identity-distinct vertices, inf coordinate, one
  missing face, boundary and pole excursions). A missing fixture is a failure.
- **Mutation testing comes later**, as its own phase, once this stack has
  settled into the one the project builds on long term. Mutants test the tests,
  not the project. **[operator]**

## 11. Later phases

- **Toolchain split:** C++20 lane for P100 (CUDA 12.x) and a C++26 lane for
  newer GPUs on RunPod, per the verbs Clang stance. Then Apple Silicon (Metal,
  NEON).
- **Perturb-and-permeate:** after principal development, a low-key continuous
  cycle on the fleet host that keeps perturbing these choices and recording results,
  open to any interested partner. **[operator]**
- **Watch critical path:** fast faithful render (this unit), animated
  explosion, correct mechanism physics, explosion while running, a horological
  complication as the alpha showpiece. Constraints today: contact surfaces
  (gear flanks, pallets, pivots) need faithful density; the hairspring flexes;
  incremental remeshing of changed parts is first class.

## 12. Left to the builder

- Data structure details of the task graph, queues and arenas.
- The output schema's byte layout, provided it carries everything in §5.5.
- How the existing `direct_cdt` is adapted to fixed-point input and component
  splitting.
- The exact flag-to-run assignment of the 64-run design.
- Corpus sample size, within a few fleet hours per arena pass.
- Unit decomposition and order inside this design, under BLACKBIRD's
  JSMNTL sequence (sub-plan, red tests, code, green, adversarial review).

## 13. Open questions for the operator

1. May the arena set the CPU governor to `performance` for its runs (needs
   root), or leave it and record frequency (current default)?
