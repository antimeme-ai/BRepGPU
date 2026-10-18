# Fast mesher design

Status: M5 design of the fast-mesher campaign
([plan](fast-mesher-campaign/PLAN.md), bead `brepgpu-12s.20`). It replaces the
613-line working draft whole. Next: the M6 seven-mandate tribunal, then gate G2.
Plan from this document after G2, not before.

## 0. Reading this document

**What it is.** One design for BRepGPU's fast mesher: arbitrary B-rep in, a
mesh people can send to machines that cut or form material out, faster than a
steel-manned NVIDIA USD/BRep, and good to look at. Sections follow the geometry
lifecycle of the [lifecycle map](../papers/2026-10-17-geometry-lifecycle-map.md)
(stages S1–S25). Each section opens with one line naming its stage, its
evidence and the operator decisions it carries.

**Markers.**

- **[operator]** an operator decision, carried unchanged. G1 decisions are named
  `D-…`; M4 decision-sheet rows are named by bead suffix (`.48` is
  `brepgpu-12s.48`). Sources:
  [journal](../journal/2026-10-17-fast-mesher-direction.md) and
  [M4-decisions](fast-mesher-campaign/M4-decisions.md).
- **[call]** a designer's choice the evidence does not force. Open to override.
- **[open]** a choice the evidence does not settle. §27 names its default and the
  arena arm or measurement that settles it.
- Unmarked text follows from the cited evidence.

**Evidence short names.** All papers are `papers/2026-10-17-<name>.md`.

| Short | Paper or report | Short | Paper or report |
|---|---|---|---|
| Map | geometry-lifecycle-map | G10 | small-feature-collapse |
| M2 | neighbour-reconciliation | G11 | validation-records |
| G1 | occt-ingress-translation-edits | G12 | edit-invalidation |
| G2 | face-edge-feedback | G13 | shading-fidelity |
| G3 | export-precision-validity | G14 | sizing-beyond-chord |
| G4 | thin-feature-embedding | G15 | pmi-tolerance-requests |
| G5 | entity-admission-matrix | G16 | source-identity-passthrough |
| G6 | units-parameter-semantics | G17 | contact-interfaces |
| G7 | determinism-cache-keys | G18 | postprocessing-lod |
| G8 | heterogeneous-placement-measurement | Heal | tolerant-geometry-healing |
| G9 | instancing-semantics | FT | fast-tessellation-literature |
| SM | nvidia-smtess-study | PC | parallel-growth-clusters |
| Audit | owned-mesher-audit | Corpus | `corpus/MANIFEST.md` |
| B1 | `results/b1-ingress-defects-2026-10-17/` | B2-in | `results/b2-ingress-2026-10-17/` |
| B2-ad | `results/b2-reader-adapter-2026-10-18/` | B2-ex | `results/b2-reader-export-2026-10-18/` |
| B2-ha | `results/b2-harness-2026-10-17/` | B2-or | `results/b2-oracles-2026-10-17/` |

**Symbols used throughout.**

| Symbol | Meaning | Default |
|---|---|---|
| ε | the caller's chord request, world units | per §6 and §23 |
| ε_def, ε_f | ε in definition units for one mesh key; the per-face request | ε/σ_max; ≤ ε_def |
| ε_loc | the local budget a triangle must meet (§3.4) | ≤ ε_f |
| η | R1 "exact" gate | ε/16 |
| β | R2 small-move gate (reconciliation only) | ε/2 |
| r_c | collapse radius, charged against ε | ε/2 |
| η_s | sliver width for sliver collapse | ε/16 |
| θ_cut | truncation angle of the point solve | 8.1° [open] |
| lfs | 3D local feature size of a definition, over entities above collapse scale, floored at 2r_c | — |
| τ₀ | UV tangent-variation cap in closure | π/3 |
| u_ctx | the governing uncertainty of a representation context | read per context |
| δ(R) | float32 rounding move for coordinates of magnitude ≤ R: (√3/2)·2^(⌊log₂R⌋−23) | 26 nm for R ≤ 512 mm |

## 1. Intent and the decisions that bind it

**Stage** all · **Evidence** journal; Audit §0; SM §4–6; B2-ha §1 · **Decisions**
listed below.

**Goal.** Beat NVIDIA USD/BRep at every stage, meshing first. Speed is second
only to correctness, and correctness without speed does not matter. Correctness
means fidelity sufficient for people to send the output through downstream
processing into machines that cut or form material: dead serious, not
aerospace-insane, and visually stunning. [operator]

**Where we start.** Owned meshing of the OM10 watch took 847.7 s CPU12. About
1,578 CPU-s against NVIDIA's 22.86 CPU-s of per-definition `tessellate`: 69× the
work, with 1.86 of 12 cores busy (Audit §0). In the published NVIDIA heal0 run,
ingress export plus NVIDIA's batch wall was 10.17 s; its 43.67 s "complete"
figure held at least 21.7 s of our own work (B2-ha §1). Perfect scheduling of the
old work would still take 70–131 s. Per-unit cost must fall more than an order of
magnitude, whatever the scheduling. The old mesher is retired (§25).

**Decisions that shape the whole design.** [operator]

- Certification is excised repository-wide, not kept as a mode. Correctness
  comes from implementing geometry correctly plus strong offline oracles.
- Bit-identical CPU/GPU output is not a goal. Each backend meets tolerance and
  the topology guarantees (D-determinism, `.53`).
- Everything about current output is fair game (facet order, IDs, on-disk
  format). Shared-edge, seam and history guarantees stay.
- No overfitting to the watch. Admission, dispatch, tolerances and capacities are
  never keyed to OM10 IDs, names, sizes or counts. Specialisations carry checked
  geometric preconditions and keep a general path.
- Pathology is designed in: non-finite and malformed values refused at ingress,
  every loop bounded by geometry-derived limits, budgets a reported backstop,
  never a silent accept.
- Single pass is not required. Easy work meshes at once; hard items are brought
  close cheaply and resolved by a background healer, built and measured even
  though no surveyed system has one ("nobody does it" is not an argument).
- Provisional results are first-class state (status, queue membership,
  dependent faces). Sharpen-in-place will consume them later.
- Error is measured against the desired final position, the repaired coherent
  solid. The source's starting discrepancy is reported per edge and never
  charged against ε.
- Toolchain sequence: meshing performance first, C++20/C++26 split second, Apple
  third. The P100 C++20 device lane stays.
- The arena is a standing mechanism for contested choices; one ablation pass is
  not a law; a perturb-and-permeate cycle follows principal development.
- Watch critical path, for perspective only: fast faithful render, animated
  explosion, mechanism physics, explosion while running, a complication. The
  mesher must not box these in.

**Non-goals.**

- Runtime certification of any kind. [operator]
- Bit-identical output across backends. [operator] (Identical bytes across
  thread counts and runs on one backend *is* required, §4.2.)
- Sewing unsewn sheets. If ever needed it is a separate, visible stage. [call;
  Heal §7.2]
- Defeaturing features above ε. [D-features]
- Matched discretization across contacts (§26.1; `.43`).
- LOD alternatives written into files for alpha (`.41`); renderer decimation
  below the topology floor (`.42`).

**Anti-goals: failure even if it "works".**

- A silent accept: a cap reached and output marked valid, an n-gon left in place,
  a collinear ear clipped "anyway", a dropped face, a swallowed trim crossing.
- A second verification system of the same kind grown around the first
  (BLACKBIRD tenet 5).
- Stored B-rep tolerance setting density (OCCT's 2× floor).
- A reuse band on deflection (OCCT's 1.1×/0.9×).
- An NVIDIA failure scored as our win, or our failure scored as NVIDIA's.

## 2. Constraints

**Stage** S18 · **Evidence** G8 §2; G7 §1.1; journal · **Decisions** toolchain,
FMA released, `.27`, `.67`, `.68`.

- **Fleet host.** 2× Xeon E5-2697 v2 (Ivy Bridge EP): AVX, no AVX2, no FMA, no
  AVX-512. 12 cores / 24 threads per socket; node 0 even CPU ids, node 1 odd;
  about 387 GB per node. Remote latency 1.6–1.7× local; remote socket read
  bandwidth about 65% of local. SMT on, AutoNUMA on, THP `madvise`, governor
  `schedutil` (left as found, `.27`), AutoNUMA left on and neutralised by memory
  binding (`.67`). Other services share the host.
- **GPUs.** Two P100-PCIE-16GB. GPU0 on node 0 in a Gen3 x8 slot (the slot
  itself, `.68`); GPU1 on node 1, Gen3 x16. Pinned transfer: GPU0 about 5.7 GB/s,
  GPU1 9–10.5 GB/s. Launch plus sync 7.4–7.8 µs; smallest offload round trip
  16–18 µs; CUDA context creation 0.21–0.29 s per process. FP64 4.1–4.6 TFLOP/s.
  Every test runs on both GPUs. Transfer numbers are never compared between the
  two GPUs (`.68`).
- **Toolchain.** C++20 host code on the pinned Clang 18.1.3 with libstdc++
  (`tooling/native_cpu_rigor.cmake`). CUDA 12.8, `sm_60`, SASS only (`60-real`),
  so the driver cannot change device arithmetic. The C++26 lane (upstream LLVM
  23, `-std=c++2c`) for newer GPUs comes next; new code avoids constructs that
  block it and keeps device-visible headers C++20-clean.
- **Floating point.** FMA and contraction are allowed [operator]. The build
  profile still sets `-ffp-contract=off` and `--fmad=false` (bead `.25.2`).
  Predicate code controls its own arithmetic (§21.3); everything else may
  contract. Whatever contraction a build uses is part of its `backend_id`
  (§4.2).
- **Execution.** Geometry, builds, tests, renders and benchmarks run on the
  fleet only. Arena permutations run as their mechanisms land, while delivery
  continues; nothing is planned around calendar time. [operator]
- **Libraries.** Own the machinery. The pinned OCCT 8.0.1 bridge is the existing,
  discussed STEP reader; this design proposes no new library. NVIDIA's package is
  a measurement target, never a dependency.
- **Internal data.** Owned native values and binary tagged-table containers.
  JSON appears only at an external presentation boundary on request.
- **Privacy.** No hostnames, addresses, absolute paths or personal names in
  artifacts; foreign processes appear in context rows as counts and CPU only.

## 3. Fidelity contract

**Stage** S2, S12, S21 · **Evidence** journal; M2 §5.3; G3 §1; G4 §3; G10 §2;
G13 §0; G15 §6; G16 §3.2 · **Decisions** D-features, D-partial-output, `.46`,
`.47`, `.60`.

### 3.1 The request

- ε is a request, not a physical absolute. What people read from their
  metrology is treated as absolute, and that interpretation is the densest
  source of trouble. [operator]
- ε is stated in world units with explicit units (D-units). Each mesh key uses
  ε_def = ε/σ_max in definition units (§16).
- A per-face request map ε_f ≤ ε_def may tighten faces (contact density, §26.1;
  thin features, §12). It is part of the mesh key (G17 I1).
- The default output promises ε only. The angle term exists in the render
  profile only (`.49`; §6.2).

### 3.2 The target

- The target is the repaired coherent solid. Face carriers are never modified.
  Edges and vertices take their resolved positions (§8).
- Where a resolved chain point is not on face f's carrier (a truncated
  direction, a recorded tangent step), f's target near that edge is its carrier
  displaced along its normal by the recorded displacement at the edge,
  decaying linearly to zero over the 3D length ℓ₃ of the adjacent chain
  segment, measured inward along the face [call; M2 §5.3]. The decay distance
  comes from the chain bytes, so the deviation oracle rebuilds the same target.
- Distances are to the trimmed target faces, never to untrimmed carriers.
- The source discrepancy d_e is a starting-condition metric, reported per edge,
  never charged against ε. [operator]

### 3.3 Deviation

- Every point of every emitted triangle, **as written** (after float32
  rounding), lies within ε_loc of the target. Every target point lies within
  ε_loc of the mesh, except collapsed geometry, which lies within r_c of its
  survivor (§9).
- Collapse displacement counts against ε (`.46`). Export rounding counts against
  ε (§3.4).

### 3.4 The ε ledger

Every emitted triangle t of face f meets

  e(t) + r_trim(t) + r_coll(t) + δ_exp ≤ ε_loc(t)

- **e(t)** is the triangle's interpolation bound against f's target on the
  evaluator the generator uses (§10.3, §13). For Waldron-form rules,
  e(t) = ½·r_min'(t)² in the face's scaled chart (FT §1.3).
- **r_trim(t)** = L_S·w̄(s) when t has an edge on chain segment s, else 0. w̄(s)
  is the segment's UV lens half-width (§11) and L_S bounds ‖DS‖ there. It covers
  the sliver where a UV chord leaves a concave trim: such points map onto the
  carrier extension, within L_S·w̄ of the edge curve [call; own derivation from
  G4 §3.1 and FT §1.4]. It is small (w̄ ≈ κ_uv·ℓ²/4) and costs one multiply.
- **r_coll(t)** = d_f when t uses a collapse survivor p* (§9.1); w/2 at a
  symmetric sliver chain, w when one neighbour absorbs it (§9.2).
- **δ_exp** = δ(R) of the frame the item is written in (§18.1): ≤ 6.6 nm for
  coordinates under 64 mm, ≤ 26 nm under 512 mm. The mesh is built to ε − δ_exp,
  so the written bytes meet ε (G3 §4.3).
- **ε_loc(t)** = min(ε_f, clearance budget, tilt budget) from §12.

Not in the ledger: the source discrepancy and every reconciliation move. They
move the target; they are reported (§6.4: b_f = δ_f + r_f).

### 3.5 Topology

- One sample chain per physical edge, shared by key by every incident face.
  Physical node identity is a key, never a coordinate weld. A key carries one
  3D position (Audit §6 item 1).
- One UV per (key, face occurrence). A seam's two occurrences of one chain carry
  two UVs. A pole or apex is one vertex with a fan. A spindle or horn torus has
  interior pole lines (§13.1).
- Every edge of a closed shell has exactly two uses, in opposite directions.
- No two distinct mesh edges join the same pair of vertex keys (the sample-count
  floor, §10.4). No T-junctions.

### 3.6 Orientation

- Every triangle is positive in its face's UV chart.
- **A0 fold rule:** its facet normal, oriented by face sense, makes an angle
  below 90° with the exact normal at each of its corners (G13 Rule A0). A
  triangle that fails refines or the face refuses.

### 3.7 Embedding

- No two triangles of one definition intersect except along shared mesh edges
  and vertices. Construction guarantees it with clearance budgets (§12); no
  emitted triangle pair is tested in construction.
- Contacts and self-intersections present in the source are reproduced and
  reported as pairs. The offline oracle exempts only reported pairs.
- Between occurrences, nothing is guaranteed. Separately meshed parts in
  nominal contact can interpenetrate by up to ε. The contact fit report states it
  (§18.4; G17 I8).

### 3.8 Accounting

- Every input face has one disposition: **meshed** (a triangle range),
  **absorbed** (an empty range plus a collapse record, legal only below r_c,
  §9), or **refused** (a record). (G16 §3.3)
- Every definition, mesh key and occurrence appears exactly once.

### 3.9 Status

Face: `valid` < `valid_goal_unmet` < `indeterminate` < `refused`. Part and above
add `incomplete`. Reaching a cap never yields `valid`. §7 defines the lattice.

### 3.10 Shading

Each corner of each face occurrence carries that face's exact normal at its own
UV, never averaged across faces at any dihedral. At singular parameters the
corner carries the one-sided limit normal along its own triangle (G13 Rule B).
Caps are repaired by interior means (§15.1).

### 3.11 Invalid inputs people still use: faithful plus reported [call]

- Edges with three or more face uses get one shared chain; the part is reported
  non-manifold there.
- Open shells and sheets mesh faithfully and are reported open (G5 §6).
- A solid whose outer shell is open is meshed and reported invalid.
- Source self-intersections and contacts within a definition are reproduced and
  reported (§3.7).
- Refusal is reserved for what cannot be meshed (§8.5 rung 5, ingress refusals).
- A format that forbids a condition refuses the affected part for that format
  only, with a record (3MF `model` objects need two uses per edge).

### 3.12 What ε does not cover

- Consumer re-processing: Cura's 30 µm vertex meld (`.73`), fTetWild's default
  envelope (1e-3 of the diagonal), PrusaSlicer's facet removal. Reported as
  consumer limits; never coarsened for (G3 §3.5).
- Feature-of-size bias of vertices-on-surface meshes. Reported per feature (§6.3).
- Tessellated-only parts: their own triangles pass through, labelled "deviation
  unknown", outside fidelity claims and speed ratios (`.61`).

## 4. Architecture

**Stage** S7, S17, S18 · **Evidence** G7 §0–§5; G12 §3.1; G2 §3; Audit §4 ·
**Decisions** D-determinism, `.53`, D-provisional.

### 4.1 The task graph

The pipeline is one global task graph over every definition of the input. No
serial loop over definitions; no barrier waves. Stages are dependency edges.

| Task | Per | Reads (the complete input bundle) | Coupling |
|---|---|---|---|
| T0 decode | source record | source bytes | none |
| T1 carrier | surface or curve | own record | none |
| T1b feature size | definition | T1 bounds of every entity above collapse scale | spatial (recorded region) |
| T2 measure | edge | curve record and range; pcurves and masters; T1 of incident faces; request | incident faces |
| T3 L1 solve | edge sample; full-rank vertex | T2 | incident faces |
| T4 L2 vertex | rank-deficient vertex | T3 end samples of incident edges | vertex star |
| T5 L3 edge | edge | T3; T3/T4 of both end vertices | edge |
| T5′ deferred rungs | deferred item | T5; other trims of both faces; T1b lfs | faces + spatial |
| T6 collapse | collapse cluster | T5/T5′ of members; request (ε, r_c) | component |
| T7 clearance | face pair, edge, vertex, face self-proximity | T1 and bounds; T5/T5′ displacement μ; request | spatial (recorded region) |
| T8 closure | edge | T5/T5′ arcs per occurrence; **source-derived** arcs of every other feature of each incident face; T1 of incident faces; T6 slice; request | incident faces |
| T9 face | face | sense; ordered loop structure; T1; T8 chains in loop order; T6 slice; T7 slice; request ε_f, flags | edge-incident faces |
| T10 publish | definition | every T9 output in canonical key order; identity binding | definition, D2 |
| T11 place | occurrence | T10; raw placement | D2 |
| T12 export | file | T11; format and parameters | D2 |

- T8 reads source-derived arcs, never other chains. That keeps the graph acyclic
  and the edit halo finite (G12 §3.1).
- T7 and T1b record their query regions for invalidation (§19.3).
- Each task's output is a pure function of its input bundle. No globals, clocks,
  environment, shared caches or reads of other tasks' partial results are
  reachable from task code (G7 §2.2 item 1, §5.3).

### 4.2 Layers and determinism

| Layer | Content | Class | Producer |
|---|---|---|---|
| L1 | decoded records, topology, use counts, unit factors | **D0**: identical bytes on every backend | host ingress, once |
| L2 | carrier data and span bounds (integer work estimates derive from them at graph build, D2) | **D1 shared** | one producer per carrier |
| L3 | feature size, clearance results | **D1 shared** | one producer per definition item |
| L4 | resolved vertices | **D1 shared** | one producer per vertex |
| L5 | edge chains: class, positions, parameters, dyadic paths, UV per occurrence | **D1 shared** | one producer per physical edge |
| L6 | face interiors, triangles, corner normals | **D1** | the face's producer |
| L7 | records, statuses | **D1** | with their layer |
| L8 | publication: global numbering, canonical order, maps | **D2**: pure function of D0/D1 bytes, fixed host path | host |
| L9 | export bytes | **D2** | host writer |
| L10 | content digests and cache keys | **D2** | host |
| L11 | timings, memory peaks, queue depths, provisional display meshes | **D3**: never compared, never keyed | anywhere |

**Determinism statement** (G7 §8, with `.53`) [operator + lane]:

> For one `backend_id`, output bytes are identical across thread counts, runs,
> launch configurations, batch compositions, GPU instances, cache state and
> incremental or fresh remeshing. Across backends, outputs are functionally
> identical: each passes the full validity stack on its own, and FI-1 to FI-5
> hold (§24.2). One model may mix backends; every shared entity has exactly one
> producer per run. A mixed-backend run gives functionally equivalent output
> every time; its bytes are a function of the input and its recorded placement
> log (§20.4). D3 is excluded.

**`backend_id`** = (device class, target architecture, digest of the exact host
binary and device fatbin, the arithmetic dispatch path taken at run time, and
the libm identity and IFUNC level of every libm function an output-affecting
kernel calls) (G7 §1.1). Thread count, SM count, GPU instance and NUMA node are
not in it; the kernels make them irrelevant.

**Owned transcendentals** [call; G7 §1.1]: `acos`, `asin`, `sin`, `cos`, `atan2`
used in sizing or placement are owned polynomial kernels. glibc swaps FMA-built
variants on AVX2+FMA hosts, so a libm call would tie bytes to the host's ISA.
Owning them keeps `backend_id` small and lets one CPU build share a cache across
hosts.

**Mechanisms that make D1 hold** (G7 §2.2), each removing a named fault:

1. Pure items (above).
2. Batch-composition invariance: SIMD tails run masked, with the body's
   instruction sequence; no cross-lane operations inside per-item results.
3. Decompositions that affect results are functions of input size and item keys,
   never of thread, SM or grid counts.
4. Canonical publication: an exclusive scan of per-item counts assigns output
   ranges before writes.
5. No floating-point atomics; no slot-allocating atomics. Integer atomics only
   for counts.
6. Only exactly associative reductions (min, max, integer sums) feed decisions.
   Work estimates sum integer counts, never ∫√det M in floating point.
7. Seeds from (run seed, item content key), never worker numbers.
8. No output-affecting iteration over hash- or pointer-ordered containers.
9. Fixed arithmetic per build; contraction, FTZ and libm path in `backend_id`.
10. Tie-breaks by content digest, never by entity number.

### 4.3 The one-producer rule

Every entity read by work on more than one producer (vertex, chain, carrier
datum, cluster-seam vertex, closure split) has exactly one producer per run,
chosen by the entity's content key and the placement rule, never by completion
order. Consumers read its bytes by key and never recompute them. Faces never
re-evaluate boundary XYZ (G7 §4). Closure splits cross backends only as integer
dyadic-path keys.

### 4.4 No run-global integers inside D1 outputs

A chain names its samples by (role, dyadic path) and its ends by role. A face
names boundary vertices by (loop position, coedge, sample path) and interior
vertices by face-local index. Global vertex and triangle numbering happens only
at T10 from canonical keys. Otherwise one inserted edge renumbers every chain,
every digest changes, and early cutoff never fires (G12 §3.5). Identity (file
names, `#N` entity numbers, product names) lives in a separate identity-binding
layer, never in keys.

### 4.5 Staging, the background healer and provisional state

- **Immediate.** Items that commit at rung 0 of §8.5 resolve inline. A face
  meshes as soon as its pending counter reaches zero.
- **Deferred.** Rung 1–4 items (Newton, transverse leaps, small leaps,
  alternative priors), missing-pcurve continuation, collapse clusters and
  closure splits on deferred edges go to typed queues.
- **Background healer.** Drains the queues concurrently with the immediate pass.
  It resolves an item directly, or holds it until a SIMD- or warp-sized batch
  accumulates or the immediate pass drains. Items are placed by item, then
  batched; batch composition never changes an item's bytes (§4.2 items 2, 3).
- **Pending counters cover two hops** (G2 §4). A face meshes when its own edges
  are resolved and every face incident to each of its edges has its edges
  resolved, because closure of its edges reads those faces' arcs.
- **Variants** (arena, §23): background queue (default), single pass; inline
  resolution and speculative meshing as follow-up arms.
- **Speculative meshing is provisional.** A face meshed against an unsettled
  chain is D3, display-only, and is always re-meshed against the final chain.
  Whether a face was meshed speculatively depends on timing, so its bytes can
  never be final (G12 §5.1 condition 5).
- **Provisional records** carry status, queue membership, dependent faces,
  achieved bound and generation. No final record may carry `pending`; a run that
  ends with a pending item has an internal error (G11 §3.3).

### 4.6 Core data layouts

All are struct-of-arrays per definition, FP64 positions and parameters, raw IEEE
bits, no run-global integers above L8. Field lists are binding; byte layout is the
builder's (§29).

| Record | Layer | Fields |
|---|---|---|
| Carrier | L2 | content digest; kind (`plane`, `cylinder`, `cone`, `sphere`, `torus`, `bspline`, `revolution`, `extrusion`); flags (rational, periodic u/v, R ≤ r, degenerate torus, `select_outer`, offset rewritten); frame (origin, three axes); radii; cone semi-angle; extrusion vector with raw magnitude; profile-curve reference for swept kinds; singular loci (poles, apex, interior pole lines v*). B-spline: degrees (p, q); per Bézier span: domain (u₀, u₁, v₀, v₁), control-point offset, recentring centre, D_uu, D_uv, D_vv, minimum weight, box, a reserved σ̲(DS) slot. Bounds only: carriers carry nothing that depends on ε |
| Work estimate | D2, at graph build | per item integer counts (edge samples, lattice cells, band trims × layers) from carrier bounds and ε_f; summed only as integers |
| Edge measurement | T2 output | per sample: dyadic path (bits, depth), t, P; per face occurrence: (u, v), d_f, n_f; per sample θ, \|x\|; flags (projection failed, non-monotone) |
| Chain | L5 | edge content digest; end roles (start, end vertex); per sample: dyadic path, t, resolved xyz, rung, residual; per face occurrence: (u, v) (a seam carries two occurrences); per segment: continuity class, closure depth; record references |
| Vertex | L4 | vertex content digest; resolved xyz; level (L1, L2); move from the stored point; rung |
| Face output | L6 | face digest; sense; disposition; status; δ_f; r_f; face vertex block [model vertices, chain samples per coedge use in loop order, interior vertices], boundary entries naming (loop position, coedge, path), interior entries holding (u, v) and xyz; one exact normal per block entry (seam entries twice, one per lift); triangles as block-local index triples; generator tag ranges (lattice, band, fan, strip) |
| Publication | L8 | unique positions by physical key; face table (triangle range, block range); block → global vertex map; normals per block entry; classification rows per coedge use; sparse tables (quotients, collapse survivors, second seam lifts); occurrence table (NAUO path, raw A and t, kind, key); the record tables of §7.3 |

## 5. Ingress

**Stage** S3, S1 · **Evidence** G1 §0–§6; G5 §0–§6; G6 §0–§5; G16 §2; B1; B2-in;
B2-ad; B2-ex · **Decisions** D-master, D-shapes, D-units, D-placements, `.61`–
`.66`, `.69`, `.71`.

### 5.1 The authority rule

1. The raw Part 21 entity is authoritative for everything the file declares:
   topology and identity, vertex points, carriers and their parameterisation,
   units and uncertainty per context, placements, representation selection, and
   for every surface curve the declared `master_representation` (G1 §4).
2. OCCT's translated shape is an evaluator. A translated value is consumed only
   where it agrees with the raw value within the context's resolution. Where they
   disagree, raw wins and a typed per-entity record holds the edit class, the
   magnitude and both values.
3. Where raw declares a master and its partner disagrees beyond the uncertainty,
   the master wins. The partner's discrepancy is a starting-condition metric
   (§3.2). [D-master]
4. Where the file declares nothing (no pcurve; unspecified trim master), our own
   construction supplies the value with a recorded residual. OCCT's
   reconstructions are never an authority. [D-master]

**What G1 measured** (915 files): 98.7% of non-planar coedges carry no file
pcurve; OCCT does not recompute missing pcurves on the no-processing path and
merged 0 of 459,061 vertex records. Where pcurves exist, OCCT overrules every
planar declared master (8,309 dropped) and 1,817 of 8,377 non-planar ones. Only
205 of 642,569 export edges carry pcurves (G5 §0.2), so "recompute with a recorded
residual" is the normal path, not the exception.

### 5.2 The ingress as it stands (the starting point)

B1 and B2 landed this in `native/step_source` and `native/reference_source`
(source store BGSRC001 version 2):

- **Raw authority with declared corrections.** The label names every correction
  class that can fire (`raw-entity-corrections=P1,P2,G5,G6,S1,S2`); each
  correction has a typed record; every undecidable case is refused by name
  (B2-ad).
  - Placements read raw (`native/step_placements.hpp`); OCCT's value is written
    only where `placements::agree` holds (2⁻³² relative, translation within the
    contexts' resolution, no point of the child's extent moved by more).
  - Closed B-spline carriers, curves and file pcurves that OCCT makes periodic are
    written as the file's own clamped nets.
  - Projected UV curves that overshoot a B-spline's knot rectangle are corrected
    into it, the change charged to the chord allocation, with a record.
  - Face orientation decided per face and per edge-connected shell part: S1
    (a face's same_sense contradicts its own bound), S2 (a whole part inside out),
    fold and contact tests for two-region and two-pole faces, refusal reason 7
    otherwise. OM10: 6 S1 corrections in one definition.
- **Units and uncertainty per context** (`native/step_units.hpp`, UR-1, -2, -4,
  -6, -7): factors from unit entities, never names; vertices scaled by their own
  context; per-context governing uncertainty with a `usable` flag (a value below
  1e-15 of the sound diagonal from `native/shape_extent.hpp` makes no claim).
  Admission stays mm-only for now (`.25.6`).
- **Structural refusal per face.** `LOOPS___`, `FACES___` and `SOLIDS__` carry
  statuses. A face is refused only through a refused loop; a definition is
  incomplete iff a face is refused. Vertex loops are carried and admitted under
  five checks with reason codes 1–6 (B2-in §2).
- **Voids** walked in file order with composed orientation; `.T.` voids refused.
- **Styles** read from every styled item, CAx-IF precedence, per-occurrence
  styles in `OCCSTYLE`.
- **Typed diagnostics.** `DIAGS___` rows carry typed (name, kind, value) fields;
  no magnitude lives in message text.
- **Measured parameter maps** (non-SameParameter coedges) are kind 4 with a
  sampled residual. Every old consumer refuses kind 4 by name until it charges
  the residual.

**Corpus result** (500 files, adapter): 232 accepted, 268 refused: non-mm unit
155, C/S residual 44, nonplanar multiple-wire faces 39, leaf-product binding 14,
closed/periodic spline chart with edge loops 12, conic chart grade 2,
natural-domain correction 1, unowned solid 1; 0 faces refused inside accepted
files. Exporter: 327/173 in both modes. OM10: 107 definitions, 9,162 faces,
196 usages, 166 occurrences, 0 refused faces.

### 5.3 What the new mesher's ingress changes

The new mesher consumes the adapter's source store directly. M8 lifts the
remaining gates, each as its own unit:

| Gate today | Files | Lift | Bead |
|---|---|---|---|
| mm-only unit gate | 155 | UR-1–UR-10 (§5.5); 86 Rat Rig inch/mm twins as the oracle | `.25.6` |
| C/S residual (fixed 1e-3 mm grade in `source_uv`) | 44 | Not an ingress refusal in the new path: edge measurement projects every sample and reconciliation resolves the gap (§8); the grade, where any remains, derives from u_ctx (G6 H4) | new path (§8) |
| nonplanar multiple-wire UV conversion | 39 | Faces with several loops on curved carriers | `.25.13` |
| leaf-product binding (one solid per leaf) | 14 | D-shapes main-representation selection on raw entities (§5.4 row 11) | `.25.5` |
| closed/periodic B-spline faces with edge loops | 12 | Lifted chart on closed B-spline carriers | `.25.12` |
| solids placed by `MAPPED_ITEM` | — | Publish as occurrences (the exporter already does) | `.25.11` |
| rigid-only placements | 0 in corpus | D-placements table (§16.1) | `.25.4` |
| revolution and extrusion carriers absent | 17 OCCT-dataset files | Native swept evaluators (§13.1) | `.25.10` |
| per-file refusal anywhere downstream | — | Per-entity refusal, rest of the part meshed | `.25.1` |

The `source_uv` reconstruction of missing pcurves (OCCT ShapeConstruct, 17-point
check) is not used for geometry by the new mesher. A face's UV per sample comes
from seeded projection in edge measurement (§8.1), with the residual recorded
per coedge (D-master). Today `source_uv` keeps only per-solid maxima
(`SourceUVStats`); the per-coedge residual is new (G1 §7).

### 5.4 Admission matrix

Frequencies are files / instances, exports (506 files) and OCCT dataset (333)
apart, never pooled (G5 §0.1). "Exact" means evaluated as the file defines it,
with no representation reserve.

| # | Kind | Exports | OCCT set | Decision | Representation |
|---|---|---|---|---|---|
| 1 | plane, line, circle, ellipse | 490 / 114,075 faces | 258 | admit | native, exact |
| 2 | cylinder, cone, sphere, ring torus | 464 / 77,043 cyl. | 140 | admit | native, exact |
| 3 | B-spline curve and surface, any degree ≤ 25 | 155 / 77,878 edges; 111 / 22,909 faces (8 / 157 of degree 4–6) | 54 / 1,802 faces of degree 4–12 | admit | native; one generic-degree batch class beside p, q ≤ 3 |
| 4 | manifold solid B-rep, closed shell | 493 / 3,190 solids | 223 | admit | closure computed from edge uses, never trusted |
| 5 | `brep_with_voids` | 5 / 16 solids, 86 voids | 17 | admit | every shell meshed; void orientation `.F.` |
| 6 | `vertex_loop` bound (pole, apex) | 14 / 141 faces | 8 | admit (B2) | pole vertex with fan |
| 7 | open shell, sheet, surface model | 15 / 37 | 72 / 755 | admit; report open | free edges are single-face edges (R5) |
| 8 | closed shell inside a surface model | 0 | 21 | admit; "closed sheet", no solid asserted | as 7 |
| 9 | surface of linear extrusion | 1 / 9 | 39 / 683 | admit | native swept evaluator; raw vector magnitude |
| 10 | surface of revolution | 1 / 2 | 16 / 516 | admit | native swept evaluator, angle-linear u |
| 11 | geometric sets, constructive, alternative, tessellated beside the shape | 51 files | — | report, not meshed | D-shapes; record each |
| 12 | torus with R ≤ r as `toroidal_surface` | 4 / 354 | 7 / 52 | admit | native torus, flag R ≤ r, interior pole lines |
| 13 | `degenerate_toroidal_surface` | 0 | 1 / 4 | admit | as 12; `select_outer` recorded and checked against the face's lobe; mismatch reported, never repaired |
| 14 | torus with R < 0 | 0 | 2 surfaces | refuse face with record | `.69` |
| 15 | hyperbola, parabola edges | 0 | 3 files | admit | native conic evaluators |
| 16 | polyline edge | 0 | 0 | admit | degree-1 B-spline, knots 0..n−1 |
| 17 | faceted B-rep (`poly_loop`) | 0 | 0 | admit | planar polygons, exact |
| 18 | non-advanced `face_surface` in surface models | 0 | 13 / 312 | admit | per-kind geometry checks |
| 19 | offset of an elementary base | 0 | 0 | admit | exact rewrite to the same kind (cone: radius R + d/cos α, axial shift, affine v) |
| 20 | offset of another base | 0 | 0 | refuse `offset_general_not_built` until a corpus shows one | native Ŝ = S + dN with bound M̂ when built [call] |
| 21 | offset through a singularity | 0 | 0 | refuse `offset_singular` | — |
| 22 | tessellated-only part | 1 | 4 | pass its own triangles through, "deviation unknown"; excluded from fidelity claims and ratios | `.61` |
| 23 | CSG | 0 | 2 / 4 | refuse `not_a_brep` | — |
| 24 | swept surfaces with reference curves; `offset_curve_3d` | 0 | 0 | refuse `semantics_unavailable` until Part 42 is read | — |
| 25 | composite-curve edge | 0 | 0 | refuse `illegal_edge_geometry` | — |
| 26 | external-reference assembly without local geometry | 11 | 10 | record `external_reference_unresolved` | resolution is assembly work |
| 27 | wireframe-only | 0 | 20 | record `no_surface_geometry` | — |

Every refusal record carries the file SHA-256, the `#id`, the entity type, a
reason code, AP legality with the WHERE rule, affected entities, what still
meshed and the part status (G5 §5).

**Why revolution and extrusion stay native.** A rational parameterisation cannot
be linear in angle, so converting a revolution to a rational B-spline forces a
non-linear, non-polynomial remap of every angle-unit pcurve (G5 §4.1; corrects
Map X12). The native evaluator S(u,v) = C + Rot_z(u)(λ(v) − C) keeps the file's
parameterisation; extrusion S(u,v) = λ(u) + v·V keeps the raw magnitude |V|.

**NVIDIA and offsets.** NVIDIA's importer header suggests offsets reach the SDK
as their basis surface. Before any offset face is compared, the steel-man is
checked on it (G5 §8).

### 5.5 Units and parameter maps

- **UR-1 Scope.** Units and uncertainty per representation context. No
  file-level unit. Pcurve parameters take the units of the face's
  representation context; each placement is read in its own context.
- **UR-2 Resolution** from the entity graph; names never set factors; a name that
  disagrees with its factor is a finding.
- **UR-3 Exact constants** (`.65`, default yes): a resolved length factor within
  1e-9 relative of 25.4, 304.8, 0.0254, 2.54e-5, 1, 10, 1,000 or 1e6 mm uses that
  constant; an angle factor within 2e-9 of π/180 uses the double nearest π/180;
  both values recorded. This closes the 41 files whose 0.0174532925 degree makes
  full circles fall 7.18e-9 rad short of 2π. Not yet in `native/step_units.hpp`.
- **UR-4 Refuse per context** (never per file): no length unit unless the caller
  states one (D-units); an unresolvable unit; two units of one kind; no
  plane-angle unit where an angle-bearing value is read (`.66`, applied by B1).
- **UR-5 Accept** inch, foot, mil, micro-inch, every SI prefix, mixed units across
  contexts.
- **UR-6 One conversion per value.** Raw reads use their own context's factor;
  OCCT-derived values are never multiplied again (the PrePoMax double
  conversion).
- **UR-7 Uncertainty** per context, governed by the named measure; absence
  recorded, never defaulted for coincidence decisions.
- **UR-8 Cyclic parameters** reduced with period 360·A after conversion; values
  outside one period are legal.
- **UR-9 Kinds OCCT maps wrongly** (revolution v, extrusion and offset pcurves and
  rectangular trims; parabola trims; parabola and hyperbola pcurves): while OCCT
  is the bridge, use recomputed UVs with a recorded residual (§8.1) or refuse;
  the owned reader applies the G6 §1 maps itself.
- **UR-10 Other formats** for later readers: IGES empty unit flag is inches by the
  standard and treated as declared, recorded (`.62`); Parasolid XT's metre
  convention treated as declared, recorded (`.63`).
- **Caller unit against file unit:** refuse unless an explicit override is
  passed, which is recorded (`.64`).
- **PMI or supplemental geometry with no reachable unit context** takes the main
  representation's units with a record; PMI is warn-only (`.71`).

### 5.6 The owned reader

OCCT's Part 21 parse is 71% of the bridge's read and transfer (89.9 s of 127 s
over 844 MB, 9.4 MB/s, single-threaded). A lexing-and-number-conversion lower
bound runs at about 300 MB/s on one core, about 32× faster; the raw walk the
overrides need costs 2.7% of the parse (G1 §6). OCCT's default processing
multiplies transfer by 18×. On the watch: bridge 3.1 s, default read 13.9 s.

- The owned headline counts ingress from STEP bytes (§22.3). Beating NVIDIA's
  ingress therefore needs an owned reader; more work on the translation layer
  cannot do it.
- **Decision** [call; G1 recommendation]: keep the bridge with raw overrides now;
  bring the owned STEP reader forward as an ingress-speed unit (`.25.7`), with
  parallel record splitting. The bridge stays as the evaluator cross-check and
  the reference exporter's path.
- The owned reader's oracle: byte-identical source store against the bridge on
  every accepted corpus file, where the bridge agrees with raw; on disagreements,
  the raw value (the G1 falsifiable form, §24.1).

### 5.7 Identity read at ingress (S1)

One pass over the parsed model collects, into side tables keyed by `#N`, off the
mesher's hot path (G16 §2.7, §5.1):

- input SHA-256, `FILE_NAME`, `FILE_SCHEMA`, `FILE_DESCRIPTION` recommended-practice
  strings;
- product: `#product_definition`, `product.id`, name, revision
  (`product_definition_formation.id`), UUIDs, nominal-model flag (tri-state:
  declared or not declared; it gates nothing, G15 §0 item 6);
- occurrence: `#NAUO` path from the root, name, raw transform, UUIDs;
- face and edge names; UUID set memberships (`v4_`/`v5_uuid_attribute`, read as
  `StepData_UndefinedEntity` records); `uuid_relationship`, `uuid_provenance`,
  `uuid_context` kept verbatim;
- `shape_aspect` groups (one hole across two faces) with GISU/IIRU members and
  SAR parents.

`#N` with the source digest is exact provenance for one input; it is not identity
across edits. Files without UUIDs (OM10 has none) get exact per-input provenance,
never a claimed persistent identity. BRepGPU never mints a UUID for a source
entity; UUIDs it writes (3MF Production) are v5 over source digest and path.

## 6. The request

**Stage** S2, S19 · **Evidence** Map S2; G14 §5; G15 §0, §4–§6; G17 §2–§4 ·
**Decisions** D-PMI, `.48`, `.49`, `.54`, `.39`.

### 6.1 Units and scale

- Absolute requests are in mm (or caller units, recorded). Relative requests
  (fractions of a diagonal) use the diagonal of the root's world box from the
  sound `shape_extent` bounds, computed once per input; the request is then an
  absolute ε for every key [call].
- Relative requests suit only inputs whose scale is not a physical size (ABC's
  user sketches). §23.4 gives the arena's request set.

### 6.2 Sizing profiles

The admissible length at a point is the minimum of the terms the profile enables.
Every profile keeps the same topology contract; profiles change only density
(G14 §5).

| Profile | Consumers | Terms | Not used, and why |
|---|---|---|---|
| **Fabricate** (default) | PrusaSlicer, Cura (STL, 3MF) | ε | No L_max (slicing is per facet), no grading, no minimum angle. Report edges under 0.03 mm and loops under 1 mm for Cura targets; never coarsen |
| **Render** | a rasterizer (three.js / Filament class, `.50`) | ε + θ_max = 25° on curves and on surface normal turning | No L_max: exact normals interpolate exactly along rulings and planes. No grading |
| **Analyse** | fTetWild | ε; report ε + ε_env (fTetWild's envelope) | fTetWild simplifies inside 0.8·ε_env and sizes its own output |
| **Compare** | the NVIDIA arm | the arm's own terms: chord only (0/0 angles), or ε + 25°/25° | quality must match (§22.2) |
| **Simulate** (later) | PhysX | ε + L_max ≤ 500·`PxTolerancesScale::length`; welded; no zero-area triangles | contact density is §26.1 |

- **No grading anywhere.** Grading at FEM slopes costs about (4/√3)/g triangles per
  boundary sample (7.7 at g = 0.3; about 8× plane triangle counts), buys nothing
  the named consumers use, and spreads influence across faces, which breaks local
  invalidation under edits and sharpen-in-place (G14 §3 item 6, §4.3).
- **No minimum size as a stop.** A geometry-derived floor may exist only as a
  termination bound; reaching it yields `valid_goal_unmet` or `refused`.
- **The angle term on analytic carriers is closed form**: Δθ = min(2·acos(1 − ε/R),
  θ_max); it binds only for R < ε/(1 − cos(θ_max/2)), 42.2ε at 25° (0.21 mm at
  ε = 0.005 mm). Sphere: ρ ≤ min(√(2Rε − ε²), R·sin(θ_max/2)). Torus:
  h ≤ θ_max/κ_max with κ_max = max(1/r, |cos φ/(R_c + r cos φ)|).
- **On freeform carriers** a 3D normal-turning bound needs a per-span lower bound
  on the singular values of DS and principal-curvature bounds; no source gives one
  for rational B-splines (G14 §4.1) [open]. Default until derived: refine each
  emitted triangle until the spread of its exact corner normals is ≤ θ_max. That
  is a construction test on the emitted triangle, as Waldron is, measured at
  corners only; the per-span bound replaces it when it exists. Near poles the
  formulation is 3D normal turning, never a parameter-space normal bound.

### 6.3 PMI and the request-checking rule R-PMI

The mesher reads semantic PMI on exact geometry to warn when ε is loose against
the tightest stated tolerance or finer than the source's own uncertainty. It warns
and complies; it never refuses and never changes ε. [D-PMI]

- **PMI subset read** (G15 §3): geometric tolerances with magnitude, modifiers and
  unequal disposition; sizes and locations with bounds, ranges and ISO 286
  classes; toleranced-feature bindings; each context's units and uncertainty; the
  nominal-model flag; the dimensioning standard; PMI validation properties (reader
  self-check); general-tolerance notes verbatim. Not read: presentation, datums,
  saved views, PMI on tessellated geometry.
- **Margin** m_t: the one-sided distance along the normal from the nominal to the
  zone edge on the side where the chord bias falls. Vertices on the surface make
  every feature of size smaller: a hole's mating size falls by 2δ; a boss's
  mating size is unchanged and its two-point sizes fall by up to 2δ (G15 §4;
  corrects the "bosses larger" premise). A size tolerance fails exactly when
  2δ > −ei, under envelope and independency alike. Planar features, flatness,
  straightness and tangent-plane orientation get no margin.
- **Consumption** c_t = ε/m_t before meshing; δ_f/m_t after.
- **Warnings**, each with m_t, c_t, the tolerance's `#id` and the ε that would
  bring c_t to c_max:
  - PMI-OUT: m_t = 0 or c_t ≥ 1 (a part made to the mesh leaves the zone). 11
    NIST hole sizes have nominal on the lower limit, so any vertices-on-surface
    mesh of them is out of tolerance at any ε > 0.
  - PMI-LOOSE: c_t > c_max = 0.1 (`.54`), in the named profile; loosening
    recorded.
  - UNC-FINE: ε < u_ctx of the context holding the meshed B-rep.
  - UNC-EXCEEDED (after measurement): a measured edge discrepancy exceeds u_ctx.
- **Record codes**: B-RQ-PM (PMI-OUT, PMI-LOOSE), B-RQ-UN (UNC-FINE,
  UNC-EXCEEDED) (§7.3).
- **Declared uncertainty describes the exporter, not the part**: the same NIST
  part declares 1e-6 to 0.127 mm across exports (G15 §0 item 4). Only measured
  edge discrepancy says anything about geometry.
- **Sender validation-property mismatch**: warn and comply (`.39`).

### 6.4 What is reported per face and per feature

- δ_f: the achieved chord bound against the target (≤ ε, 0 inside planes).
- r_f: the largest distance between f's target and its own source surface (0 when
  every edge resolved at rung 0 with zero residual).
- b_f = δ_f + r_f, a linear sum of worst-case bounds; signed where the side is
  known.
- u_ctx beside b_f, never added to it.
- Per PMI feature of size: mating size, two-point range, least-squares size from
  the inscribed-chord geometry, their deviations from nominal, m_t and c_t.

## 7. Validation, records and status

**Stage** S4, S15 · **Evidence** G11 §3–§6; Map S4, S15; B2-in §4 · **Decisions**
D-partial-output, D-profile, `.38`, `.39`, `.40`.

### 7.1 Two axes

- **Check outcome**, one per check per subject: `pass`, `fail`, or
  `not_evaluated` with a reason: `check_failed`, `prerequisite_failed`, `budget`,
  `unsupported`, `no_threshold`.
- **Rule A (gating checks are conservative):** a check whose verdict selects a
  construction branch never leaves a decision open; `not_evaluated` takes the
  refusal branch. Only non-gating checks can leave a subject `indeterminate`.
- **Entity status**, derived from the fidelity-contract clauses that apply.
  Hard clauses: deviation, shared-edge topology, orientation, embedding,
  production. Soft goals: cap rule, requested quality targets, any soft cap that
  fired.

| Face status | Rule |
|---|---|
| `refused` | no triangles: a hard clause failed, or Rule A took the refusal branch; carries reason and finding |
| `indeterminate` | triangles exist; no hard clause failed; a hard clause is `not_evaluated` |
| `valid_goal_unmet` | every hard clause passed; a soft goal failed or was not evaluated |
| `valid` | every clause passed |

`indeterminate` is rare by intent: a carrier kind with no stated density bound
meshed by a fallback rule (`unsupported`); a clearance query out of budget
(`budget`, though §12.5 refuses instead); a non-gating input check that broke
while backing a hard clause.

### 7.2 Roll-up and exit status

- **Part:** `refused` if no face produced or a definition-level refusal;
  `incomplete` if any face refused (D-partial-output); else the maximum status
  over produced faces. An incomplete part also reports its worst produced status.
  Order: `valid` < `valid_goal_unmet` < `indeterminate` < `incomplete` < `refused`.
- **Occurrence:** the maximum of its definition's status and its own clauses
  (placement admission, scaled ε).
- **Run:** counts per definition and per occurrence (D-measure), and the same
  mapping over occurrences. The reduction is a commutative monoid over a
  canonically keyed multiset, so it is identical at any thread count.
- **Exit codes** (`.40`, confirmed default):

| Exit | Run status |
|---|---|
| 0 | every occurrence `valid` or `valid_goal_unmet` |
| 2 | output written; some occurrence `indeterminate`; none incomplete |
| 3 | output written; `incomplete` |
| 4 | nothing produced: input refused |
| 1 | tool failure: internal error, I/O, device, resource exhaustion of the run |

- **Indeterminate faces in exports** (`.38`): their triangles go to the internal
  schema and glTF only; the part counts as incomplete for STL and 3MF.

### 7.3 Native record tables

Owned SoA tables in the existing tagged-table binary container (eight-byte tag,
row count, byte size). Enums are `enum class : uint16_t`, append-only. Strings only
for external names, in one pool. No number inside text; human text is generated
from records at presentation time (G11 §4).

| Table | One row per | Fields |
|---|---|---|
| `RUN_____` | run | input SHA-256; tool build id; backend set and `backend_id`s; thread count; ε requests (value, unit, absolute or relative); units honoured; profile id and digest; every non-default parameter and arena flag; start/end; OS; host class (never a hostname); STEP header part information |
| `CRITERIA` | profile row in force | criterion id; PDQ code or owned `B-…` code; Part 59 entity name; measure variant; assessment kind; polarity; unit; seed and active value; extra parameters; provenance; origin (seed, tightened, loosened); action (report, warn, gate) |
| `MEASURE_` | inspected element, dense | per edge: class, rung, max d_f, max \|x\|, min θ, owner face, sample count, method, per-end vertex move; per face: area, triangle count, δ_f, r_f |
| `FINDINGS` | (phase, criterion, subject[, second subject]) failed or not evaluated | key; phase; criterion; subjects; owner; outcome and reason; measured value; threshold copy; method; accuracy; one or two extreme `Location`s; Part 59 class (erroneous, inapt) |
| `ACTIONS_` | thing done to make the mesh | key; kind; findings answered; entities; before (P1) and after (P2) values; maximum displacement; carrier chosen; M2 rung; surviving alternatives and the branch-rule step that decided |
| `STATUS__` | face, definition, occurrence, run | status; reason; per-clause outcome (2 bits per clause); disposition; triangle count; finding range; counts per status |
| `SUMMARY_` | (phase, criterion) | inspected, violating and not-evaluated counts; worst value and location; truncated flag |

- **Phases:** P0 raw entities; P1 as translated (pre-heal, on what the bridge hands
  over: the name says the bridge already edited it); P2 resolved; P3 mesh; P4
  export (written bytes); P5 offline oracle. The same criteria run at P1 and P2
  ("before and after healing"), because healing can create tiny elements
  (VDA 4955 §3.1.6.1).
- **Action kinds:** M2 rungs 0–5; vertex levels L1/L2; collapse (E′, S′, absorbed);
  missing-pcurve recompute with residual; ε tightening near a thin feature; tilt
  refinement at a knife edge; orientation flip; closure split; algorithm fallback;
  refusal. Face- and body-level silent actions are recorded too.
- **Keys and order.** Finding key (phase, criterion, subject, second subject);
  action key (kind, primary entity). Sorted at the end of a run; ids are indices
  after sorting.
- **No merging and no cap** on records. If a budget ever caps findings,
  `SUMMARY_.truncated` is set and counts are labelled lower bounds.
- The edit-disposition record (§19.4) and D3 timings live in their own sections,
  outside every equality oracle.
- `brepgpu report` formats the tables at the boundary: text (default), TSV, JSON
  on request, Quality Stamp XML. Nothing internal parses its own output.

### 7.4 The profile `brepgpu-pdq-1`

Seeded from JAMA V5.1 and CAx-IF (D-profile). A row is (criterion, measure,
value, polarity, unit, provenance). Users may tighten; loosening is recorded and
the profile digest changes. The profile holds data-quality criteria only, not
the mesher's ε-derived gates (η, β, r_c, θ_cut), which are request parameters and
arena flags recorded in `RUN_____`.

| Code | Criterion | Seed | Action |
|---|---|---|---|
| G-SH-LG | gap between faces at an edge (3D-curve and pcurve measures) | 0.01 mm | report |
| G-FA-EG / G-FA-VG / G-LO-LG | edge–surface, vertex–surface, adjacent-edge gaps | 0.01 mm | report |
| — | vertex–edge gap | 0.01 mm (own, by analogy) | report |
| G-ED-TI / G-FA-TI | short edge / small face | 0.01 mm / 1e-4 mm² | report |
| G-FA-NA | entirely narrow face | 0.01 mm | report |
| G-SU-DC | nearly degenerate surface boundary | 0.01 mm | report |
| G-SH-IS, G-LO-IS, G-FA-IS | self-intersection classes | 0.01 mm; α unset (`no_threshold`) | report |
| G-SH-NT | G1 discontinuity | smooth ≤ 0.01°, crease ≥ 0.2° | report (mesher classification is §15.2) |
| G-SH-NM / G-SH-FR / G-SH-OU | non-manifold edge / free edge / over-used vertex (3) | logical | report; 3MF `model` gate; part open |
| G-CU-HD, G-SU-HD | degree above 5 | 5 | report only; the mesher accepts ≤ 25 |
| B-VP-SV | sender volume/area; centroid | 0.5%; 0.02 mm + 0.1% of diagonal above 20 mm | warn and comply (`.39`) |
| B-ME-VP | mesh volume against exact | \|V_mesh − V_exact\| ≤ 2·ε·A | goal |
| B-ME-DV / B-ME-WT / B-ME-OR / B-ME-SI | deviation / watertightness / orientation / self-intersection | ε / logical | hard clauses |
| B-RQ-UN, B-RQ-PM | request against uncertainty / PMI | §6.3 | warn and comply |
| B-ED-CL | edge needed rungs 1–5 | logical | report |
| A-TR-MA, A-TR-AS | min angle, aspect | none seeded | goal, off unless requested |

Values JAMA leaves to agreement are seeded `no_threshold`, not invented. The
mesh-volume check never uses CAx-IF percentage bands: an honest Ø1 mm pin at
ε = 0.005 mm can differ by about 4% (G11 §0 item 4, §5.3).

### 7.5 Human view

Generated from the tables. Header first (input and digest, tool, profile and
loosened rows, ε, run status and exit code in words); failures before successes
(every refused face, then every indeterminate face, with entity ids, reason, value
and location); then summaries per criterion.

## 8. Edge measurement and reconciliation

**Stage** S5, S5b · **Evidence** Heal §7.2; M2 §1.5, §5; G2 §3; G12 §3.5; G15 §0
item 5 · **Decisions** M2's five proposals adopted as defaults, each an arena flag;
error against the final position; D-master; D-partial-output.

### 8.1 Measurement (T2)

All edges of all definitions form one batched stage (CPU SIMD or GPU, §20).

1. **Initial sampling** at the dyadic parameters of §10 (the chain's own curve
   rule plus every incident face's admissible length). If there is no 3D curve,
   the owner pcurve's lift supplies P.
2. At each sample t, evaluate P = C(t) from the prior representation (§8.2).
3. For each incident face occurrence f: if f's pcurve is the prior
   representation, u_f = p_f(t) and d_f = 0; else if f's file pcurve maps within
   η of P (through its measured parameter map where the edge is not
   SameParameter), u_f = p_f(t); otherwise seeded projection of P onto S_f (seed
   from p_f or the previous sample, monotone along the edge, lifted across the
   period, snapped to a pole within vertex uncertainty). Record
   d_f = |P − S_f(u_f)| and the unit normal n_f.
4. With two faces: θ = ∠(n₁, n₂) and the minimum move onto both,
   |x| = √(d₁² + d₂² − 2d₁d₂cos θ)/sin θ.
5. Per edge: max d_f, max |x|, min θ, failed or non-monotone projections.

Missing pcurves cost one seeded projection per sample; there is no special path.
This replaces the `source_uv` reconstruction for geometry (§5.3).

### 8.2 The prior

- **Edge samples.** If the file declares a master, the master's point (G1 rule 3).
  If it declares none and offers one representation (the normal export case: a
  bare 3D curve), that representation. If it offers several with no master,
  the one with the smallest measured maximum residual to the incident surfaces
  (OCCT, SMLib, openNURBS, Parasolid practice; M2 §1.1).
- **Vertices.** The stored point if it is within β of every incident edge's
  L1-resolved end; otherwise the smallest-enclosing-ball centre of those ends
  (never an average).

### 8.3 One point solve (L1)

For every edge sample and every vertex (M2 §5.1):

- One row per incident surface: n_i·x = −d_i, from stage-1 values.
- Weights by declared trust tier: plane hard (eliminated, as planegcs does);
  other analytic a high finite weight; freeform 1. Ties within a tier by content
  digest (never by entity id).
- Truncation: solve singular direction j iff σ_j ≥ σ_cut or the move it needs,
  |u_jᵀb|/σ_j, is ≤ β; otherwise the prior stands in that direction. σ_cut is set
  as an angle θ_cut [open; default 8.1°, sweep {3.6°, 8.1°, 10°}].
- Linear step x = VΣ⁺Uᵀb. Predicted residual r_i ≈ κ_i(|x| + |d_i|)²/2.
- Commit at rung 0 if every r_i ≤ η and the move is ≤ β; else queue for rung 1
  seeded at p₀ + x.

For an R2-sized discrepancy (|d|, |x| ≤ β) the predicted residual is within η
whenever the curvature radius is ≥ 8ε (0.04 mm at ε = 0.005 mm). The directions the
surfaces cannot determine are, to first order, those in which the error does not
change, out to √(2η/Δκ) along a tangent slide (M2 §1.5). That is why the final
position comes "mostly free" from stage-1 data.

### 8.4 Three acyclic levels

| Level | Work | Reads |
|---|---|---|
| L1 | every edge sample; every full-rank vertex: §8.3 | T2 only |
| L2 | rank-deficient vertices: prior from L1-resolved ends, then §8.3 | L1 |
| L3 | each edge: end samples replaced by its vertices; each end's correction Δ₀, Δ₁ spread as (1 − s)Δ₀ + sΔ₁ and projected onto each sample's null space | L1, L2 |

- **s is the normalised curve parameter of the sample (or the source curve's arc
  length), never the polyline's arc length.** Inserting a sample then moves no
  other sample, which closure locality and incremental = fresh require (G2 §3).
- No level reads its own outputs: no priority queue, no iteration.
- On a transverse edge the free subspace is the tangent, so L3 only
  reparameterises; on a tangent edge it carries the slide from the vertices.
- L3 replaces last-segment absorption (arena flag `l3_blend`, M2 H7).

### 8.5 The ladder per deferred item

| Rung | When | What | Verified by |
|---|---|---|---|
| 0 | linear-step residual ≤ η and move ≤ β | commit | chain monotone |
| 1 | residual > η, full rank | Newton seeded at the linear step, iteration bound from the geometry | converged; chain monotone; UV loop simple; ends on resolved vertices |
| 2 (big) | transverse, move > β | Newton to the intersection at any distance up to ¼ lfs | as rung 1, plus no crossing of another trim of either face |
| 3 (small) | near-tangent (σ₂ < σ_cut, move > β), or rungs 1–2 failed | truncated weighted solve; slide from the prior; the residual gap sits on the lower-trust face as a recorded step | chain monotone; step ≤ ¼ lfs |
| 4 | representations disagree by more than β | rungs 0–3 from the alternative prior or owner | as above |
| 5 | nothing verified | refuse the item and every face using it, with a record | — |

- A rung that fails verification falls to the next rung (copy-check-rollback),
  never tolerance inflation.
- Rungs 2–4 run in the background healer, batched by kind.
- **Bridge geometry is never invented**: no bridge curve, no sliver face.
- **The gross gate is structural, not a distance.** The draft's
  G = min(4ε, ¼ lfs) is retired. Senders legally declare gaps above 4ε (NIST
  FTC-07 declares 0.127 mm against a 0.02 mm G at ε = 0.005 mm; G15 §0 item 5).
  Such a gap now goes to the intersection (transverse) or becomes a recorded step
  (tangent) whenever it is below ¼ lfs, and UNC-EXCEEDED fires only if it is above
  u_ctx. Whether real gaps track u_ctx is G15's measurement H5 (§27 #30).

### 8.6 Branch choice

When an item has several verified candidates (M2 §5.5; structural-first adopted
at G1):

1. discard any candidate that breaks structure: shared-key closure, monotone
   chains, simple consistently oriented UV loops, ends on resolved vertices;
2. prefer no move of a higher-trust carrier beyond η;
3. then the preserved continuity class measured from the source;
4. then the least maximum boundary deviation over all incident faces;
5. then the least kink against neighbours **as resolved at L1/L2 of the current
   run** (a fixed level, never "whatever resolved first", never a previous run;
   G12 §3.5);
6. then the lower content digest.

Two candidates stay alive only when steps 1–3 tie and step 4 differs by less than
η; both are recorded, the deterministic winner is emitted. The mesher never
waits for a user.

### 8.7 Refusal is honest when

No candidate passes step 1; two same-tier top carriers (two planes, two analytic
surfaces of one tier) meet near-tangentially with a gap that must move one beyond
η; the move or gap exceeds ¼ lfs; Newton fails its bound and the small leap leaves
a gap above ¼ lfs. Refusal is per item, with every face using it refused and the
part marked incomplete.

### 8.8 Feature size

lfs is the 3D local feature size of the definition, over entities above collapse
scale, floored at 2r_c, so a sub-ε sliver cannot shrink it and then refuse leaps
beside it (G10 §7). Collapse candidates are found on source positions with margin
r_c plus the edge's measured discrepancy. T1b records its query regions (§19.3).

## 9. Small features

**Stage** S6 · **Evidence** G10 §0–§6; G16 §3.3 · **Decisions** D-features,
`.46` (collapse counts against ε), `.47` (no topology change by default).

Correctness never depends on collapse: a sub-ε feature can always be meshed
faithfully within ε, because density depends on curvature, not feature size.
Collapse is an optimisation (triangle count, needles, caps, near-degenerate CDT
input). A failed check reverts the cluster to faithful meshing; it never refuses a
face (G10 §0 decision 2). Collapse is arena flag F6 (§23.2; G10 H3); if no stratum
gains ≥ 1% in triangles or time, E′ and S′ leave the design and F′ stays as a
seeding shortcut.

### 9.1 Edge collapse E′ and the merged vertex

1. Clusters are connected components (union-find) of edges whose **curve
   geometry**, not just samples, fits the ball test on resolved positions. A
   cluster K is accepted only if its curve-ball radius r_K ≤ r_c; otherwise none
   of it collapses (all-or-nothing keeps it order-independent).
2. The merged vertex p* goes to the multi-surface point (V2) if the surfaces are
   pairwise transverse and the collapsed geometry stays within r_c of it; else
   the smallest-enclosing-ball centre (V3), with d_f ≤ r_K.
3. Triangles using p* in face f are sized to e(t) ≤ ε_loc − d_f (§3.4). The last
   segment of a surviving edge ending at K carries the same reserve.

Counterexamples to the draft's per-edge, per-sample, unreserved rule: a chain of
1.5β edges deletes a 15β hole at N = 64; an arc's apex escapes both β balls once
its radius exceeds β/√2; unreserved triangles reach 1.09–1.19ε (G10 §2.1).

### 9.2 Sliver collapse S′

A face narrower than η_s between two **distinct** neighbours maps its long
boundaries onto one chain C* (union of samples, nearest-pair midpoints). Each
neighbour moves by ≤ w/2 (symmetric) or ≤ w (one neighbour absorbs it), charged
as r_coll. Dispersal: trust order when the neighbours' tiers differ (plane
frozen), symmetric when they are equal (`tangent_split`, shared with M2). A sliver
whose long sides border the same face is absorbed into that face if its target
passes within the reserve, otherwise meshed faithfully; never a dangling chain.

### 9.3 Boundary-only faces F′

Boundary-only meshing is the first attempt for a narrow face, never a licence:
every boundary-only triangle passes the face's own density test (§13) on the
triangle actually emitted, including the twist term. Failures get interior points
or a boundary split. A priori sufficient: the face fits a scaled-chart disk of
radius R_face with ½R_face² ≤ ε_loc. Boundary CDT runs in the carrier metric
(G10 §2.2; the twisted-strip counterexample reaches 1.35ε under the width
premise alone).

### 9.4 Checks, in order

1. curve-ball radius;
2. contiguity: K's edges form one contiguous run in every loop (no pinch, no
   inner–outer loop merge);
3. same shell (never merge across shells or bodies);
4. UV loop simplicity after p* gets its per-face UV;
5. Euler χ per shell before and after (a topology change, such as filling a
   through-pinhole or closing a sub-ε gap, is refused by default; a flag allows it
   with a record, `.47`);
6. A0 orientation at p*.

Any failure reverts the cluster. Thresholds: r_c = ε/2 (alternative ε/4),
η_s = ε/16, both arena values. β stays the R2 gate only (G10 decision 1).

### 9.5 Records and accounting

- One flat record per cluster or sliver, from union-find components (order-free):
  survivor key, absorbed `#N`s with their dimension, op (degenerate-edge→vertex,
  small-edge→vertex, degenerate-face→edge, small-face→vertex), parent record. The
  records form Shephard's survivor tree.
- Survivor choice: lowest-dimension original entity with the minimum content
  digest [call: content, not source id, per §4.2 item 10].
- A face whose triangles all collapse is **absorbed**: empty range plus its
  collapse record (§3.8). F′ faces are not collapses.
- Collapse is ε-dependent: its slice enters the face key with ε, so a tightened ε
  re-runs it.

## 10. Shared boundaries and edge sampling

**Stage** S7, S8, S19 · **Evidence** FT §1.4; SM §3; G2 §2; G10 §2.4; G14 §4;
G16 §3.2; G18 §5.2; G7 §6 · **Decisions** shared-edge guarantees stay.

### 10.1 The chain model

Kept from the old code as a model, re-stored on doubles and dyadic paths
(Audit §4): `DefinitionBoundaryForest` (physical node keys, per-edge interval keys,
root-relative `sample_paths`/`path_bits`, vertex aliases from declared-degenerate
edges). `PhysicalNodeKey{definition, generator, source, generation, ordinal}` with
generator ∈ {original_vertex, original_edge, face_site, declared_quotient} is the
classification map (§17).

### 10.2 Dyadic sampling

A chain is a cut of a dyadic tree of parameter intervals on the edge's root
interval [t₀, t₁] (after units). A segment splits at its dyadic midpoint when a
test fails; tests are hereditary bounds, so the coarse sample set is a subset of
every finer one (nesting for LOD and geomorphs, G18 §5.2), and the result is
order-free. Closed-form uniform counts (the circle's n = ⌈π/acos(1 − ε/R)⌉) become
"split until each segment's sag ≤ budget", costing at most 2× samples on those
edges.

**Ties at round CAD numbers.** Real files put R = ε and R = 2ε on exact algebraic
values; 16 corpus cylinder decisions computed 3 − 1 ulp where the exact value is 3
(G7 §6). Either side meets ε, but a 1-ulp libm difference changes a count; that is
why chains have one producer. The arena arm `biased_split` tests "split if
sag > budget·(1 − 2⁻⁴⁰)", the dyadic form of ceil(x(1 + 2⁻⁴⁰)) (G7 H5).

### 10.3 Sample tests

A segment s passes when **all** of these hold:

1. **Curve rule** (sag of the chord against the 3D curve):
   - line: 0;
   - circle radius R over angle Δ: R(1 − cos(Δ/2));
   - ellipse, hyperbola, parabola: κ_max·ℓ²/8, with κ_max = a/b² (ellipse,
     a ≥ b; hyperbola at its vertex) and 1/(2f) (parabola);
   - polynomial B-spline span of degree p, sub-interval fraction h of the span:
     ⅛h²·p(p − 1)·max‖Δ²P‖ (span-local, after Bézier extraction);
   - rational span: Zheng–Sederberg Theorem 4 on homogeneous control points,
     recentred on the span's box centre, with the span's own minimum weight
     (D·δ² ≤ 8ε·min w). Not Cheng 1992, Piegl–Richard or adjacent-weight ratios;
   - no endpoint-Hermite proxy (SM §3 row 3).
2. **Face rule** for every incident face occurrence: closure predicate P4 (§11),
   the segment's UV arc length in the face's **scaled chart** is ≤ 2√(2ε_b) with
   ε_b = ε_loc − L_S·w̄(s). In the scaled chart a plane and a cylinder's ruling
   direction have zero scale, so straight edges there need no samples; a straight
   edge on a twisted or ruled face (bilinear patch, hyperbolic paraboloid, ruled
   B-spline) gets samples from the twist term. **Corrects the draft's "lines need
   no interior samples"** (G10 §2.4) and keeps every boundary triangle within ε
   (G2 P4).
3. **Profile terms**: the render angle term (§6.2) on curves; L_max for the
   simulate profile.
4. **Closure predicates P1–P3** (§11).

The draft's "an edge shorter than 4β emits no interior samples" is deleted: by arc
length it adds nothing, and by chord it is wrong (a 350° arc with ends 4β apart)
(G10 §2.4).

### 10.4 The sample-count floor

No two distinct mesh edges may join one pair of vertex keys (G16 §3.2):

- a closed edge (start = end vertex) has at least three segments;
- among edges sharing both end vertices (a lens, a split hole's two halves), at
  most one has a single segment; the others get at least two. Which one keeps a
  single segment is decided by content digest.

These cases occur only for features near or below ε, so the floor meets §9: either
the floor applies or the feature collapses with a record, never neither. The
classification oracle (§24.1 row 4, check 3) falsifies it on authored small-circle
and lens families at ε ≥ R.

### 10.5 Seams, poles, quotients

- A seam is two occurrences of one chain in one face: two UV lifts one period
  apart, bound to the same physical keys.
- A pole or apex is a `declared_quotient` vertex with a fan; each corner carries
  the one-sided limit normal (§15.1). Interior pole lines of a spindle or horn
  torus at v* = ±arccos(−R/r) are quotients inside the chart; a face's domain may
  touch v* only at those lines (G5 §4.4).
- Degenerate-edge aliases stay explicit (`SourceVertexAlias`).

## 11. Boundary closure: face-to-edge feedback

**Stage** S20, S7, S9 · **Evidence** G2 §0–§5; Heal §7.4 · **Decisions**
D-determinism, D-partial-output, D-provisional.

**Policy** [lane, adopted]: settle and protect a sufficient chain. "Sufficient"
means every segment passes four hereditary predicates in every incident face
occurrence. The chain is frozen after closure. Every mature mesher re-splits
shared edges on face trouble and stops on a count (OCCT 5, Gmsh 10 + 3, CGAL 29,
SmTess 100); this one needs no count.

### 11.1 Predicates on (segment s, face occurrence F)

Notation: ℓ(s) UV arc length; τ(s) total UV tangent variation; ℓ₃ 3D arc length;
D(s) a lower bound on the UV distance from arc(s) to every feature of F other than
e's own arc and the parts inside s's corner balls, computed from **source** arcs.

- **P1 flatness:** τ(s) ≤ τ₀ (π/3).
- **P2 separation:** w̄(s) = ½ℓ(s)·tan(τ(s)/2) < ½D(s). w̄ bounds the lens
  half-width (Pav–Walkington Lemma 1).
- **P3 corner:** at each endpoint vertex v with UV corner angle α_v, inside the
  corner ball B_v (largest R_v with τ₁(R_v) + τ₂(R_v) < α_v/4), w̄(s) <
  dist(s, v)·sin(α_v/4), and τ(s) < α_v/4 for the segment touching v. A segment
  crossing ∂B_v splits until inside or outside.
- **P4 band:** the scaled-chart UV arc length is ≤ 2√(2ε_b) (§10.3 item 2).

**Lemma 1 (sufficiency).** If every leaf passes P1–P3 in F, F's polygonal loops are
simple, nest exactly as the curved loops do, and no curve point of another loop
lies in any chord's cap. P4 lets every boundary triangle meet ε_b with an apex
about ℓ₃/2 away.

**Cusps (α_v = 0).** Two arcs with a common tangent and curvatures κ₁ ≠ κ₂ separate
as |κ₁ − κ₂|r²/2; the first chords meet only at v; later segments pass P2 with
D ≈ |κ₁ − κ₂|r²/2, a geometric grading. If |κ₁ − κ₂| is not measurable above the
floor, the face refuses "trim curves tangent-coincident at vertex". This is the
lane's own derivation, tested on family F4.

### 11.2 Stop rule and bounds

Each predicate holds once ℓ(s) ≤ L*(s):

  L*(s) = min( τ₀/κ̄, √(2·D̲/κ̄), α_v/(4κ̄) inside corner balls, P4's length )

with κ̄ the UV curvature bound on the arc and D̲ the lower bound of D. A segment
splits only when it fails, so no arc shorter than (1 − q)·L*_min is created,
q = ρ/(1 + ρ), ρ the parameter speed ratio. Samples per occurrence ≤
∫ds/((1 − q)L*(s)) + 1; depth ≤ ⌈log_{1/q}(ℓ_edge/L*_min)⌉ + 1. No round count.

**Stop without passing** (each a refusal record):

1. arithmetic: the midpoint is not representable apart from the ends, or a UV
   chord is shorter than the CDT's fixed-point separation; doubles bound depth by
   about 52;
2. a premise broke: D̲ at or below the arc-separation floor (the source trim
   self-intersects or loops touch), κ̄ or M̄ unbounded, α_v = 0 without a cusp
   certificate;
3. depth exceeds the packing bound from the measured L* (a bound was wrong; the
   record names it);
4. the predicted count exceeds the request's budget: refused before refinement
   (whole request or the face's own estimate, §20.5).

### 11.3 Determinism and cost

- The split set is N = {n : every proper ancestor of n fails some predicate in
  some occurrence}: a pointwise definition, identical for every order, schedule,
  thread count and occurrence permutation. Requests merge by maximum depth per
  node, a commutative idempotent join.
- **Purity:** D(s) comes from source arcs through each feature's own dyadic lens
  hierarchy, never from other edges' current chords. The first level is a box
  tree over lens boxes (OCCT FaceChecker's cost).
- **Floating point:** arc length and variation by fixed-order quadrature per
  segment; minima for D.
- Per new sample: L1 solve, L3 blend, one UV per occurrence, the children's
  predicates. Nothing else.
- **UV-arc bounds** κ̄ and D̲ come from the pcurve where one is master or agrees;
  for projected occurrences the arc is the foot-point curve of the resolved edge,
  and its curvature bound needs the same per-span first-derivative floor σ̲(DS)
  as the freeform angle term (§6.2) [open]. Default until derived: closed forms on
  analytic carriers; on freeform carriers, κ̄ from the sampled foot points with a
  2× margin, and the residual detector (§11.4) refuses a face whose estimate was
  wrong.

### 11.4 Residual detector

After triangulation, one pass per face looks for chord crossings, containment of
other loops, 3D folds and boundary-triangle ε. It is not a feedback trigger. If it
fires on input that met the premises, a bound was wrong: the face refuses and the
record names the predicate. It exists because premises are checked, not assumed.

### 11.5 Interactions

- Closure runs after an edge's resolution settles. A later move (rung 1–3) changes
  the arc; closure for that edge re-runs from scratch.
- The lfs in P2 is each face's UV trim lfs, not M2's 3D lfs.
- New closure samples on deferred edges become deferred items.
- Lattice clusters and band separators are built after closure; C4-aligned
  edges force their lattice level (§13.4).
- **Arm B** [arena, `boundary_closure`]: OCCT-style crossing-plus-nesting checks
  and splits in synchronous rounds per definition, requests of round k merged
  before k + 1, splits limited by the same floors, no count. Asynchronous rounds
  are order-dependent and not allowed.

### 11.6 Refusal record

Face id, edge id, occurrence (sense, seam side), definition and occurrence; segment
key, t interval, UV and 3D ends; predicate and stop case; ℓ, ℓ₃, τ, w̄, D(s) with
the feature that bounded it, α_v, κ̄, M̄, L*, the floor, predicted and observed
depth; the witness point. The face is `refused`, the part `incomplete`; the chain
stays valid for the other faces. Where the lfs query could not resolve above the
floor, the face is `indeterminate`, never `valid`.

## 12. Embedding: clearance budgets in construction

**Stage** S21 · **Evidence** G4 §0, §3–§7; G3 §1, §4.3; Map X16 · **Decisions**
D-features, `.59`, `.60`.

Each emitted point stays strictly on its own face's side of the medial surface
between faces. Construction guarantees it with three budgets computed from source
geometry and from the triangle being built. No emitted triangle pair is tested in
construction. The full self-intersection check stays an offline oracle.

### 12.1 Quantities

- **Localization** ε̂_f = sup over f's triangles of the distance to f's own trimmed
  face: ≤ ε_loc elsewhere, ≤ 2ε_loc + s_e in concave-trim slivers (s_e the edge
  chord sagitta).
- **Displacement** μ_f: the largest distance between f's target and its source
  face (rung moves, recorded steps, collapse moves); clearance is computed on
  the target, or on the source minus μ_f + μ_g.
- **Clearance** c(q) = min over other faces g of dist(q, F_g); same-face
  self-proximity uses remote pieces of F_f.
- **Wedge** at a shared edge e: polar openings Δψ_in(ρ), Δψ_out(ρ) in the normal
  plane, with Δψ(0) = θ measured at every sample (§8.1) and
  Δψ(ρ) ≥ θ − ½(κ_f + κ_g)ρ; Δψ_min(e) over ρ ∈ [0, r*_e],
  r*_e = 2(ε̂ + μ)/sin Δψ_min.
- **Tilt** α(t, v) = ∠(aff t, T_v S_f) for a star triangle t at chain vertex v;
  τ_v the chord-to-tangent angle at v (≈ κ_e ℓ/2).

### 12.2 The lemmas behind the budgets

- **A (half-clearance).** If every point of M_f deviates from its footprint by less
  than half the clearance to F_g, and conversely, M_f ∩ M_g = ∅. The draft's "2ε"
  is the special case, stated on the target with ε̂ and μ.
- **B (one face).** M_f is embedded if its UV polygon is simple and positively
  triangulated, deviation stays below the carrier's local reach (1/κ_max and half
  the distance to remote pieces of F_f), and every triangle normal is within π/2
  of the carrier normal (Khoury–Shewchuk Triangle Normal Lemma).
- **C (the wedge).** Within the zone of e, M_f ∩ M_g is exactly the shared chain if
  (C1) every star triangle of each face satisfies α(t, v) + τ_v ≤ ½ sin Δψ_min − m;
  (C2) ρ₀ = 2s_e/(sin Δψ_min − 2 max α) is below both faces' first-row depth and
  the segment lengths; (C3) every non-star triangle in the zone satisfies
  Lemma A. Straight edges are checked numerically; curved and doubly-curved edges
  are a proof sketch tested by families K4–K5.
- **D (vertices).** Faces sharing only a vertex are separated by a plane through v
  if the tangent-cone gap exceeds α_f + α_g + m.

### 12.3 The mechanism

- **E0 constants:** ε̂_f, μ_f per face from §8 records.
- **E1 candidates:** (a) face pairs whose boxes inflated by ε̂ + μ overlap and that
  share no edge or vertex (the existing CPU/CUDA AABB pair-discovery path); (b)
  every shared edge; (c) every vertex with three or more faces, or faces sharing
  only it; (d) per face, non-neighbour span pairs whose inflated boxes overlap, and
  chord boxes of different loops or remote stretches of one loop, in UV.
- **E2 clearance lower bounds:** closed forms for analytic pairs (parallel planes,
  coaxial cylinders and cones, sphere–plane, sphere–sphere, cylinder–plane);
  otherwise recursive span-pair box distance with the Bézier convex hull until the
  bound clears ε̂ + μ, the pieces are small enough to set a local budget, or a work
  budget runs out ("could not establish", never "clear"). For edges, Δψ_min from
  measured θ minus curvature terms.
- **E3 budgets** join the ε ledger (§3.4) as ε_loc:
  - non-adjacent and self-proximity: ε_loc < ½(c − μ_f − μ_g)·(1 − m) on both faces
    over the zone; planes need nothing;
  - at shared edges: the C1 tilt limit per star triangle, then a budget growing as
    ½ρ·sin Δψ up to ε_loc at ρ = r*_e. For a cylinder strip along a ruling the star
    budget is R(1 − cos(½ sin Δψ_min)) ≈ R·Δψ_min²/8;
  - at vertices: Lemma D against the tangent-cone gap;
  - in UV: chord deviation below half the UV clearance to other loops and tilt
    below half the UV angle at loop junctions; a failure re-splits the shared
    chain through closure (§11).
  - For analytic strips the budgets are closed forms; for bands they are checked
    per triangle in the same pass as A0.
  - Margin (1 − m) = 0.9 [call; the G4 probe's factor].
- **Budgets are per face, with no credit taken from the neighbour.** Tightening one
  face can create the knife-edge failure: refining the concave face of the K1
  crescent raised failures from 2/24 to 11/24 in one sweep (G4 §3.4). Signed
  credit (A3, signed C1: a convex face beside a wall, a tube's inner face) is an
  arena flag, default off.

### 12.4 Thin walls and gaps

A wall or gap of width w > ε is a feature that must not be dropped. It is meshed
with ε_loc < w/2 on both faces, never refused (D-features). Planes need no change.
A gap narrower than the source's stated uncertainty is meshed the same way (`.60`).

### 12.5 Outcomes

- **Clearance at or below the floor** c_floor: the faces touch or intersect in the
  source. Mesh faithfully, report the pair, exempt it in the oracle. c_floor =
  2δ_exp + the clearance bound's own resolution [call; G4 leaves the floor
  unvalued] [open].
- **A budget that cannot be met, or clearance not established within its work
  budget:** tighten local ε first; if that fails, the affected faces are `refused`
  with a record, never `valid` (`.59`). A floating-point triangle-pair test
  confined to that region is an arena flag (`local_pair_test`), default off.
- **Export** shrinks every clearance by up to 2δ_exp; the budgets reserve it
  (G3 §1, C2–C3).

### 12.6 Evidence and cost

- The draft's cylinder slab rule crosses in 59 of 2,016 crescent configurations
  thicker than ε (up to 3.62ε) under a fixed angular lattice, and in 2,157 of 3,168
  sub-ε ones; the per-face rule crossed in 0 of 5,400, at 0–7 extra vertices per
  face cross-section (G4 §3.8). The old hot-path check's "0 forbidden pairs" on the
  watch is no evidence for the new path (H5 of G4).
- The edge rule is a few flops per sample and per star triangle, from θ already
  measured. Freeform clearance runs as an irregular span-pair worklist on the CPU.
  Candidate counts on the corpus are unmeasured; G4 H4 (under 2% of meshing time,
  under 5% of triangles) is the arena measurement.

## 13. Face construction

**Stage** S9 · **Evidence** FT §1; SM §6; PC §5; G2 §4; G4 §3; G10 §2.2; G13 §0;
G18 §2; Audit §4 · **Decisions** predicates in the arena [operator]; no silent
accepts.

Every emitted edge, interior, boundary or band, is sized by the face's local
admissible length. That closes NVIDIA's gap of sizing cells and emitting other
triangles (SM §3).

### 13.1 Generators per carrier

| Carrier | Interior | Rule |
|---|---|---|
| plane | none: constrained triangulation of the chain polygon in the plane's chart | zero deviation; only profile terms (L_max) add points |
| cylinder, cone | θ-slab constraint lines on the face's dyadic angular lattice; no samples along rulings | Δθ ≤ 2acos(1 − ε_loc/R) (cone: largest radius in the slab; apex a pole); untrimmed faces are strips of 2n triangles; any re-triangulation inside one slab keeps the bound |
| extrusion | slab lines in u only | ruled: S_uv = S_vv = 0; D_uu = max‖λ''‖ |
| sphere | dyadic lattice in (u, v) with pole fans, plus band | circumradius ρ² ≤ 2Rε_loc − ε_loc², rechecked on every new triangle (not monotone under containment) |
| torus (ring, horn, spindle), revolution, B-spline (polynomial, rational, any degree), elementary offsets after rewrite | 2:1-balanced dyadic quadtree over UV sized by the anisotropic Waldron rule per span; interior leaves become conforming quads; hanging sides by the packet zipper | D bounds below |

**Second-derivative bounds** (scaled chart: u ↦ u√(D_uu + D_uv),
v ↦ v√(D_vv + D_uv); a triangle is within e if ½r_min'² ≤ e):

- torus: ‖S_uu‖ ≤ R_c + r, ‖S_φφ‖ = r, ‖S_uφ‖ ≤ r; spindle tori add the interior
  pole lines;
- revolution: D_uu = ρ_max, D_uv = max‖λ'_⊥‖, D_vv = max‖λ''‖ (profile rules);
- polynomial span (p, q): D_uu = p(p − 1)max‖Δ²_uP‖, D_uv = pq·max‖Δ_uΔ_vP‖,
  D_vv = q(q − 1)max‖Δ²_vP‖, recentred on the span's box centre;
- rational span: Zheng–Sederberg on homogeneous control points, recentred, with the
  span's own minimum weight;
- general offsets (when built): M̂ ≤ D_ww + |d|(max‖D²n‖/min‖n‖ + 3max‖Dn‖²/min‖n‖²).

Size by r_min, never by edge midpoints: midpoint and centroid checks understate
by up to 4/3 (FT §1.3).

### 13.2 The trim band

- Lattice cells crossed by the trim are deleted. The band between the trim chain
  and the lattice rim is triangulated by constrained Delaunay with `direct_cdt`,
  fed fixed-point integers taken straight from doubles in the carrier's scaled
  chart (never Rational sites).
- Each connected band component (about one per hole) is an independent
  triangulation. Long components split at constrained separators from a
  lattice-front vertex to an **existing** trim sample (PC C2).
- Band refinement inserts points only in the interior, never on a chain. Chains
  change only through closure (§11).
- Every band triangle meets the ε ledger (§3.4) including r_trim, the clearance
  and tilt budgets (§12.3) and A0; violators refine.
- Metric CDT generation of the whole face is the arena alternative
  (`face_generator`).
- No "clip anyway", no collinear-ear path, no n-gon output.

### 13.3 Orientation and termination

- UV orientation and in-circle decisions use the predicates of §21.3.
- A0 (§3.6) on every new triangle, with exact corner normals already evaluated.
  A fold valid in UV but reversed in 3D refines or refuses. This removes NVIDIA's
  inside-out fillet class and the 1808 duplicate-triangle class by construction
  (SM §5).
- Quadtree depth, band refinement rounds and flip counts are bounded by limits
  from ε_loc, the face's L* and the floating-point floor. Reaching a bound is
  `valid_goal_unmet` (soft clause) or `refused` (hard clause), never `valid`.

### 13.4 Splitting large faces

Agree first on a shared lattice; never grow then weld (PC §0).

- **C1** lattice clusters: weighted Hilbert ranges of coarse quadtree blocks;
  seam vertices keyed by integer lattice coordinates; one owner per key; a
  read-only ghost layer of neighbour levels. All clusters of one face stay on one
  backend [call; G7 §4 item 5].
- **C2** band components and separators (above).
- **C4** iso-line edges sampled on the lattice's dyadic levels where the edge lies
  on an iso-line; the adjacent lattice refines to that level.
- **Split rule** (G8 §5): split face f when e_f + w_f > τ_split = 0.1·W/(P − 1)
  (W the lane's estimated work, P its workers); never below g_min ≈ 100·o
  (o ≈ 1 µs per-task overhead, so ≈ 0.1 ms) or 10× the seam cost. On a 12-core
  lane at NVIDIA-class work for the watch, τ_split ≈ 0.21 s.
- **Oracle:** a face's keyed triangle set is identical for every cluster count and
  assignment, mesh(K = 1) ≡ mesh(K = k).
- Before C1/C2 ship in the default, the per-face predicted work distribution is
  measured on the watch, the corpus and families; if no face exceeds τ_split in the
  batch workloads, C1/C2 serve the single-part latency case and the GPU (PC §6).

## 14. Post-processing legality

**Stage** S11 · **Evidence** G18 §0, §2, §4 · **Decisions** D-features,
D-provisional.

An in-core operation is legal only if every triangle it creates passes the
generator's own predicates by the generator's own evaluator (UV orientation, the
face's density rule, A0, the operation's topology condition) and it changes no
chain vertex, no B-rep vertex and no identity. It adds no second verification
layer.

| Operation | Verdict |
|---|---|
| interior split at the target evaluator; chain split at the exact parameter by the edge stage only | allowed |
| flip of a non-constraint interior edge; half-edge collapse of an interior vertex onto an existing one; tangential relaxation in (u, v) with exact re-evaluation | allowed under check; arena flags, **default off** |
| canonical reorder, identity weld at export, attribute evaluation, packing | allowed |
| flip of a chain, trim, seam or slab line; face-local move of a chain or B-rep vertex; QEM placement; smoothing projected on the mesh; positional or nearby weld; hole filling, overlap union, orientation or T-junction repair, deleting degenerate triangles; cross-face simplification; uncapped plain-epsilon flips | forbidden in-core |
| decimation, global remeshing, convex decomposition, physics cooking, slicer repair | downstream only |

"Exact re-evaluation" inside a band near a displaced target means the target's
evaluator, displacement included (G18 §2 item 4). Flips, collapses and relaxation
are kept only if a measured quality target cannot be met by refinement alone
(G18 H1).

## 15. Normals, edge classes and LOD

**Stage** S12 · **Evidence** G13 §0–§4; G18 §5; Khoury–Shewchuk 2019 ·
**Decisions** `.41`, `.42`, `.49`, `.50`, `.55`.

### 15.1 Normals and the cap rule

- **Rule B normals**: every corner of every face occurrence carries its face's exact
  normal at its UV, normalised in binary64. Never averaged across faces at any
  dihedral. The rendered normal jump at every edge sample equals the true jump θ.
- **Shading-exact carriers**: Phong-interpolated exact vertex normals reproduce the
  true normal field exactly on planes, cylinders and spheres for any triangle
  shape (G13 §1.3). On cones, caps raise interpolated error to 5ε-equivalent, a
  second reason for A1.
- **Limit normals** at poles, apexes and collapsed edges: the one-sided limit along
  the corner's own triangle. Freeform collapsed-edge limit normals fail explicitly
  today (`native/om10/math.hpp:126,151`); M8 implements them.
- **A1 cap rule**: if R > c·r (R the 3D circumradius, r the min-containment radius)
  and θ(T) > max(√(2ε·κ(T)), 1e-9 rad), with θ(T) the largest angle between the
  facet normal and a corner's exact normal, repair by interior means only (move or
  delete an interior apex, insert an interior Steiner point); never split a chain.
  Default c = 2 (largest angle 150°); arena c ∈ {1.155, 1.414, 2, off}. An
  unrepairable cap is `valid_goal_unmet`, never a refusal. The θ gate leaves caps
  on planes and along rulings alone.
- **A2 display bound** θ(T) ≤ θ_display: optional, off by default, for callers whose
  consumer flat-shades.
- Acceptance renderer: a rasterizer (three.js / Filament class) (`.50`).

### 15.2 Edge classification (records, not normals)

Measured at every resolved edge sample, because STEP stores no edge regularity
(corrects the draft's "follow the source's encoded regularity" and "G1 edges share
a normal"):

- smooth: max θ ≤ 0.01°;
- crease: min θ ≥ 0.2°;
- vanishing: θ crosses between the two along the edge;
- defect band: anything else, reported as a G-SH-NT finding;
- feature line: θ ≥ 1° (CAx-IF) or three or more face uses.

Thresholds are profile data. Defect-band steps are never hidden in display
(`.55`).

### 15.3 LOD

- LOD is re-tessellation at ε_k = 4^k·ε, cached under the mesh key. Dyadic chains
  make coarse samples a subset of fine ones, so mixed per-face levels stay
  crack-free by chain identity (a chain's level is the finest among its incident
  faces) and sharpen-in-place gets ancestor maps for geomorphs. Band triangles have
  no ancestors and swap atomically per invalidation component (§19.5).
- Switching level k to k − 1 moves a surface point by at most 1.25·ε_k.
- The curved part of a full pyramid costs at most 4/3 of the finest level.
- At coarse levels more thin features fall under §12; display levels tighten
  locally rather than refuse.
- The topology floor (about V − 2 + 2h triangles per face; ≥ 9,162 on the watch) is
  as coarse as re-tessellation goes. Below it is renderer work (`.42`).
- LOD alternatives are in-process display only for alpha (`.41`). A LOD mesh is
  final for its ε_k, not provisional.

## 16. Assembly and instancing

**Stage** S10 · **Evidence** G9 §0–§5; G17 §2; B2-or; B1 §4 · **Decisions**
D-placements, D-measure, `.56`, `.57`, `.58`.

### 16.1 Placement table

A is the 3×3 linear part of the composed raw placement (root to leaf, definition
units after D-units), σ_max ≥ σ_min its singular values (closed form from AᵀA),
κ = σ_max/σ_min. τ = 1e-6 [call; G9 H5 checks no real placement falls in
(1 + 1e-9, 1 + 1e-3)].

| Kind | Test | Admitted | Key | STL | 3MF (one build item per occurrence) | glTF (one node per occurrence) | Normals |
|---|---|---|---|---|---|---|---|
| rigid | det > 0, κ ≤ 1 + τ, \|σ − 1\| ≤ τ | yes | (definition, ε_def, request lengths, versions) | x′ = Ax + t | item → key object, [A\|t] | node TRS | n′ = A n |
| mirrored | det < 0, else rigid | yes, same key | unchanged | x′ = Ax + t, **reverse each triangle** | default: item → the key's opposite-hand object (reflected once by a fixed M, re-wound) with transform A·M (det > 0); spec path (det < 0 item) behind a flag | negative-determinant TRS; no re-wind (consumer reverses) | n′ = A n, never the cofactor |
| uniform scale | κ ≤ 1 + τ, \|σ − 1\| > τ | yes | ε and every length request ÷ σ_max; angles unchanged | as rigid; reverse iff det < 0 | that scale's object, [A\|t] | uniform-scale TRS | normalise(A n) |
| non-uniform, shear | κ > 1 + τ | refused per occurrence with record | — | — | — | — | — |
| singular, non-finite | σ_min ≤ τ, or non-finite | refused per occurrence | — | — | — | — | — |

- Placements are classified on the raw STEP placement, never OCCT's location
  (OCCT reads mirrors as rotations, inverts scale and defaults an omitted axis3
  wrongly; corpus frequency 0, read raw anyway; B2-ad `.37.20`).
- One mesh per distinct scale value; grouping nearby scales is an arena flag
  (`.57`; G9 H2 with r = 2^(1/8)).
- Today one rigid predicate (`native/rigid_placement.hpp`, Gram within 1e-9) serves
  readers and display; M8 widens it to this table (`.25.4`). The display's cofactor
  normal transform (`native/om10/math.hpp:177-178`) yields inward normals under
  det < 0 and must change when mirrors are admitted.

### 16.2 The mesh key and ownership

- Key = (definition content identity: source digest and `#id`, UUIDs where present;
  effective request in definition units: ε_def, every length request ÷ σ_max, the
  per-face map ε_f; ingress interpretation: thresholds, master and units policy;
  producer identity: mesher version, flags, `backend_id`).
- **The per-face minimum runs within one scale class** (corrects G17's formula):
  ε_f = min over that class's occurrences and their declared contacts of
  ε_contact/σ_max. Running it over all occurrences would collapse "mesh per scale"
  into "mesh at the tightest scale".
- One mesh task per key, created when the graph is built. Occurrences hold
  read-only references to the finished key result; no occurrence-level locks
  (OCCT's 2012 race has no analogue). Colour, visibility, occurrence id and
  explosion offsets never enter the key.
- Reserved slot for per-occurrence face variants (G17 I6): (definition, ε-map
  digest, constraint-set digest).

### 16.3 Per-occurrence attributes

- Occurrence id = the NAUO path from the root, plus UUIDs.
- Colour precedence (as B2-in implemented): explicit occurrence style (deepest
  wins); then the outermost coloured node style; then the part style with faces
  keeping their own colours (face over solid is a recorded deviation from CAx-IF
  §4.2.1, G9 H4 counts the ambiguous case).
- A colour variant costs a duplicated object in 3MF (items cannot carry
  properties) and a duplicated mesh object sharing accessors in glTF.
- Hidden occurrences are written to STL and 3MF and reported (`.58`).

### 16.4 Accounting

- The unit of meshing work is one mesh key. Emission (transform, re-wind, colour
  variants, writing items, nodes or flattened facets) is a separate stage, timed
  separately, through the same owned emitter for owned and NVIDIA meshes.
- Every comparison reports definitions, keys, occurrences, occurrences per
  placement kind, and per-key and per-occurrence triangle totals.
- Refused placements are excluded from the speed ratio and listed as coverage gaps
  in their own table (`.56`).

## 17. Classification map and identity carriage

**Stage** S23, S1 · **Evidence** G16 §2–§5; Shephard 1998 · **Decisions**
D-formats, `.51`, `.52`.

### 17.1 Classification of mesh entities

- **Vertex on a model vertex:** `#vertex_point`; position = resolved vertex; its
  move from the source point is a recorded fact.
- **Vertex on a model edge:** `#edge_curve`, dyadic path key, FP64 t, one (u, v) per
  incident face occurrence (two on seams). Rung-3 edges name their owner face.
- **Vertex in a face:** face, FP64 (u, v), generator through `FacetKey`.
- **Quotient** (poles, apexes, interior pole lines): the model vertex when the
  source has one there, else the face with the UV locus.
- **Mesh edge** on model edge E iff it joins two consecutive samples of E's chain;
  otherwise classified in the face.
- **Collapse:** single classification against the virtual entity; the survivor
  tree of §9.5 links back to the source.

### 17.2 Layout: ranges, not lanes

- Triangles are emitted face-contiguous in canonical order; triangle → face is one
  offset per face (F + 1 u32). On the watch: 36.7 KB against 2.19 MB of
  per-triangle face ids.
- Each face's vertex block is ordered [model vertices, chain samples per coedge use
  in loop order, interior vertices]; classification is one row per coedge use
  (edge `#N`, start, count, direction) plus one interior range per face.
- Sparse side tables keyed by vertex index for quotients, collapse survivors and
  second seam UVs.
- SIMD and GPU batches carry identity once per batch descriptor; lanes carry
  geometry only.

### 17.3 Carriage per format

| Field | Internal | STL | 3MF | glTF |
|---|---|---|---|---|
| triangle → face | range per face | internal schema records each face's range in the written STL | core triangle set, one `refrange` per face | `_FACEID` FLOAT per vertex (exact to 2²⁴; a definition with more faces refuses glTF export by name) |
| face → source | face table | — | set name and identifier; metadata | `extras` face table |
| groups (UUID sets, `shape_aspect`) | group table | — | extra triangle sets before face sets | `extras` groups |
| vertex classification | key + t / (u, v) FP64 | — | — | — |
| occurrence | tables | — | `partnumber`, metadata, **3MF Production UUIDs (v5)** (`.51`) | node `extras`, names |
| collapse history | record tree | — | — | — |

No draft glTF extensions in alpha (`EXT_mesh_primitive_edge_visibility`,
`EXT_mesh_features`) (`.52`). USD (`GeomSubset`) and AP242 tessellated (native
`geometric_link`, `topological_link`) come later.

## 18. Export and consumers

**Stage** S13, S14 · **Evidence** G3 §0–§6; G13 §4; G14 §1; G9 §0.1 ·
**Decisions** D-formats, D-consumers, `.38`, `.51`, `.52`, `.58`, `.72`–`.74`.

Plain float32 rounding of a stand-in producer's clean solids broke 3.4% / 2.4% of
items at 0.025 / 0.005 mm in the file frame, nearly all through sub-picometre
slivers and sub-nanometre contacts; at +100 m, 18.6% / 32.3% with real moves
above ε (G3 §3.3). The lifecycle map's "12%" measured something else (already
self-intersecting Thingi10K models rounded to doubles; G3 §7).

### 18.1 Quantization-aware construction

- Stored positions are chosen at construction in the frame the file will use: the
  item frame for glTF, the **world frame for 3MF** (PrusaSlicer bakes a
  single-instance foreign item into world float32), the source frame for STL.
- Three local conditions keep every FP64 guarantee through plain rounding:
  - **C1** every triangle's altitude onto its longest edge > 2δ (enforced by the
    quality pass; refine or collapse needles);
  - **C2** every non-adjacent or vertex-adjacent pair more than 2δ apart (the
    clearance budgets of §12 add 2δ);
  - **C3** no knife-edge fold within 2δ/h⊥ (a first-order rotation bound).
  No element that met C1–C3 failed in any of G3's five placements and five
  emulations.
- Deviation grows by at most δ by convexity; the mesh is built to ε − δ_exp (§3.4).
- **Not snap rounding.** It repairs soups that already intersect, outputs doubles,
  re-triangulates and does not guarantee success; its float32 variant fails 42% of
  the time in its authors' tests.

### 18.2 Per format

| | STL (binary) | 3MF (separate items) | glTF (face ids, normals) |
|---|---|---|---|
| Frame | source frame; recentre with a record only when δ(R) exceeds the budget (`.74`) | world frame for δ; vertices written item-local under the item transform | item frame; positions in mm under a 0.001 node scale, or in m; node translation carries placement |
| Values | binary float32, exact | shortest round-tripping decimal of each float32 (lib3mf truncates to 6 decimals by default; our writer does not) | float32 as stored; accessor `min`/`max` from stored floats |
| Welding | none at write; identity-welded values (exact-equality readers recover the identity weld because C2 keeps distinct vertices distinct) | shared indices, manifold by index | per-face and per-normal split vertices from one stored value |
| Normals | facet normal = unit winding normal of the rounded triangle | none (core has none) | NORMAL FLOAT VEC3, one exact normal per corner per face occurrence, mandatory; no byte-quantized normals; SHORT only on request; no TANGENT |
| Open parts | written; reported open | `surface` object, never `model` | double-sided material |
| Indeterminate faces | excluded; part incomplete for STL (`.38`) | excluded; incomplete (`.38`) | included with face id |

- Producer-side positional welding is forbidden.
- Mirrored and scaled emission per §16.1.
- Hidden occurrences written and reported (`.58`).

### 18.3 3MF assembly policy

The assembly is written in place as separate items (`.72`). The 3MF spec makes
overlapping items and components alike united by the device, and lets consumers
move items. The writer never turns FP64 contact into overlap: inter-item pairs
within 2δ_world are found by a box query on stored values, and a pair that is
disjoint in FP64 but overlaps as stored refuses that 3MF export of the pair with a
record (part incomplete for 3MF). Directed rounding away from the partner is an
arena arm [call]. Contacts are reported (§18.4).

### 18.4 Contact fit report

Per declared or detected inter-occurrence contact pair: clearance after faceting
(c − δ_hole − d_consumer) and any facet-induced penetration. Inscribed faceting of
a nominal pin and hole guarantees FP64 interpenetration up to ε; it is reported,
never silently accepted (G17 I8). This is an offline oracle and a report, not a
construction guarantee.

### 18.5 Consumers and their limits

| Consumer | What it does | Our response |
|---|---|---|
| PrusaSlicer 2.9.6 | float32; exact facet merge, nearby merge only if unmatched; removes facets with two equal vertices; flips all if volume negative; `slice_closing_radius` 0.049 mm; G-code resolution 0.0125 mm | quantization-aware construction; nothing to repair |
| Cura 5.11–5.13 | integer µm; 30 µm vertex meld that drops faces (removes faces in about half of clean items); unions overlaps; 0.15 mm deliberate overlap of touching meshes | accepted and reported as Cura's limit; never coarsen (`.73`) |
| lib3mf 2.5 | decimal → double → float; no geometric checks | exact decimals |
| three.js r186 | float32 positions; float64 node matrices; flat shading when NORMAL absent | NORMAL always written |
| fTetWild | grid weld at 1e-8·diag; drops faces with area/2 ≤ 1e-8·diag; default envelope 1e-3·diag | report ε + ε_env; callers pass `-e` near ε |

## 19. Caching, edits and provisional state

**Stage** S16, S17, S24 · **Evidence** G7 §5; G12 §0–§5; G18 §5.4 · **Decisions**
D-determinism, D-provisional, `.53`.

### 19.1 The cache key

```
key(task) = SHA-256( key_schema_version ‖ task_kind ‖ backend_id ‖ request
                     ‖ digest(own source records, raw IEEE bits)
                     ‖ sorted by role: role ‖ digest(OUTPUT bytes of each consumed dependency) )
```

- Dependencies enter by the digest of what they produced, never by their keys:
  early cutoff, and no "Frankenbuild" (two faces of one edge served against two
  variants of it, which in a mesh is a crack).
- `request` = every parameter the task reads: ε in definition units, η, β, r_c,
  θ_cut, the ¼-lfs factor, angle and length limits, profile thresholds the task
  reads, export format, every arena flag it reads.
- Not in any key, because they are tested invariants: thread count, worker ids,
  scheduling, NUMA node, GPU instance, batch composition, cluster count, cache
  state. Not in any key because they are identity: file names, entity numbers,
  product names, colours.
- **Per task** (G7 §5.2): carrier (own record); clearance (carriers and bounds of
  the definition's faces, μ); vertex (point record; incident carriers); edge (curve
  record and range, coedge pcurves and senses; incident carriers; vertex outputs;
  clearance slice; the closure split set, so feedback is history-free); face
  (sense; ordered loop structure; carrier; chains in loop order; collapse slice;
  clearance slice); publish (face outputs in canonical order; placement table);
  export (publish output; format).
- **T8 key is static and coarse:** the digest of every other feature's source arc
  in each incident face. Downstream invalidation is the same as a dynamic read-set
  key because the chain digest cuts off (G12 §3.1; H7 there tests it).
- Storage: `key → output digest` and `output digest → bytes`, native binary; an
  evicted digest is a miss.
- Cross-backend reuse is off by default; it would make warm runs differ from cold
  runs on one backend (G7 §5.4).

### 19.2 Incremental remeshing is fresh meshing behind the cache

No code path patches an old mesh. Every source surveyed that patches reports or
implies that incremental differs from fresh (G12 §0).

- **Rule C, change description.** The host gives content. Channel F (default):
  the whole new model. Channel Δ (trusted in-process hosts: animation, physics,
  owned feature operations): new bytes of every changed record, deleted ids,
  created entities, changed transforms and requests; history relations optional,
  used only for the old-to-new correspondence. Channel Δ must equal channel F on
  every generated edit (tested).
- **Rule I, invalidation.** A task recomputes exactly when its key misses. The
  static upper bound (the halo):
  - topological: changed faces F₀, faces sharing a vertex with them (R₁), and faces
    sharing a vertex with R₁ (R₂, reached only through a closure split caused by a
    moved arc); resolve and closure work on edges of F₀ ∪ R₁. Without vertex moves
    R₁ shrinks to edge-adjacent faces;
  - spatial: every task whose recorded query region (clearance, feature size,
    collapse candidates) meets the old **or** new bounds of a changed entity;
  - component: every collapse cluster touching a changed entity in the old or the
    new model.
  This replaces the draft's "editing an edge invalidates only its incident faces",
  which misses vertex stars, closure reads, spatial queries and collapse
  components.
- **Rule R, reuse.** A key hit and nothing else. Achieved or requested deflection is
  never a reuse criterion; OCCT's 1.1×/0.9× band is rejected. Serving a finer mesh
  for a coarser request is not a cache behaviour.
- **Rule E, equivalence.** E1: on one `backend_id` with the same placement,
  incremental bytes equal a cold fresh run of the edited model, every D0–D2 layer,
  for any edit sequence, eviction state and thread count; undo restores the
  original bytes with every task a hit. Under load-reactive placement: incremental
  equals fresh replayed with the incremental run's placement log. Across backends:
  FI-1 to FI-5.
- **Forbidden global couplings** (each would make the halo the model): run-global
  integers in D1 outputs; tie-breaks by entity number; an output-size guard that
  picks which faces to refuse; a global placement optimizer for shared entities.
- Sub-face (band component) reuse is an option only if single-part latency shows
  large faces dominating (G12 §4.2).

### 19.3 Spatial queries record their regions

A spatial query certifies absences ("no feature within r"). Each records its
query region (inflated box or ball) in its trace. A box-tree query of changed
entities' old and new bounds against recorded regions finds the invalidated tasks:
one query per changed entity.

### 19.4 The edit-disposition record

Per face: reused / re-meshed / new / deleted, the correspondence rule used, old and
new content keys. Correspondence priority: equal persistent identifier (CAx-IF PID
UUID, Parasolid identifier, our `EntityKey`); host history relation; equal content
key unique within the definition; none (ambiguity reported). Per run: hits and
misses per task kind, |halo|, |invalidation set|. D2, but outside every equality
oracle.

### 19.5 Display between edit and remesh; sharpen-in-place

- Invalidated faces form components linked by shared changed chains. Each
  component swaps atomically, all old to all new; until then its old meshes stay on
  display, marked stale.
- Physics never sees stale or provisional meshes (D-provisional; G17 I7).
- Sharpen-in-place is a sequence of request edits with ε_f decreasing; the final
  mesh equals a fresh mesh at the final ε by purity.
- Rigid and mirrored transform changes touch only the placement table: animation
  and exploded views are remesh-free. A uniform scale change changes ε/s and so the
  keys. A flexing hairspring re-meshes its faces each frame; display shows
  provisional meshes in between.

## 20. Placement, scheduling and memory

**Stage** S18 · **Evidence** G8 §2–§6; G7 §4; G12 §5.3; PC C3 · **Decisions**
`.53`, `.67`, `.68`.

### 20.1 Lane mode (the shipped configuration that must win)

One socket, its memory node and its local GPU.

1. **Tasks and estimates.** Each task has a class (edge-sampling batch, edge
   resolution batch, lattice range, band component, healer item, publication, GPU
   batch of stage s) and integer a-priori work units (samples, cells, band trims ×
   layers). Priority is HEFT's upward rank, in this shallow graph "face work
   remaining after this task".
2. **CPU workers.** One per physical core the lane owns (minus the core whose SMT
   sibling hosts the GPU driver thread). Per-worker deques seeded in LPT order of
   (priority, estimate); owners pop the highest-priority local task; thieves steal
   the oldest, largest task. Over-decompose so no task exceeds τ_split. Never more
   workers than cores.
3. **Device allocation**, HeteroPrio at batch granularity: each eligible stage-class
   batch gets α = t_cpu/t_gpu from the cost model; the GPU pulls from the high-α
   end, CPUs from the low-α end; classes below break-even never go to the GPU; an
   idle GPU may spoliate a running or queued CPU batch it can finish sooner, the CPU
   result dropped and never merged.
4. **GPU-eligible classes:** B-spline and rational span sizing and cell
   classification; B-spline lattice evaluation, emission and normals; seeded
   projection onto freeform carriers in edge measurement; display. **Never**:
   analytic-surface evaluation (PCIe costs more per item than 11 cores need), bands,
   CDT, topology.
5. **GPU driver thread** pinned to the SMT sibling of a lane core; double-buffered
   pinned staging on the GPU's node; context creation starts at process start,
   overlapped with ingress; the GPU joins the pool when ready.
6. **Memory.** Every arena, staging and output buffer bound with
   `mbind(MPOL_BIND)` to the lane's node; read-mostly source and carrier data bound
   too. First touch alone is not a placement under AutoNUMA (2 GiB migrated in about
   4.3 s, read bandwidth falling to 1.9 GB/s meanwhile).

### 20.2 Whole-host mode

Socket pre-partition of definitions (or face groups of huge definitions) by LPT on
estimates; each socket runs the lane rule with its own GPU; a socket with empty
deques steals from the other only tasks ≥ 10× the cross-socket steal overhead
(about 1 µs); outputs go to the thief's arena; read-mostly source data replicated
per node (about 0.4 ms per 10 MB). Compared only with NVIDIA's two-socket arm.

### 20.3 Cost model and fallback

- CPU: t_cpu(κ) = a_κ + b_κ·u. GPU: t_gpu = t0_d + u·max(g, β_s/B_d), with t0_d the
  16–18 µs round trip plus about 3.6 µs per extra kernel and B_d each device's own
  pinned bandwidth. Calibrated per (class, device, host, binary digest) by
  least squares from the previous run's per-task timings, persisted beside the
  binary; first-run defaults from G8's microbenchmarks. GPU0 and GPU1 are separate
  devices with separate models (`.68`).
- **Fallback to CPU-only priority work stealing, GPU off,** when a class's model is
  uncalibrated, its median measured/predicted ratio leaves [0.5, 2], the context is
  not ready, or the predicted saving is below the remaining context time. Logged per
  run.

### 20.4 Mixed backends

- Placement is per item, then batch (§4.5). Load-reactive placement (HeteroPrio
  with spoliation) is the default under `.53`; fixed placement from a fixed cost
  model is an arena arm for edit locality [call].
- The placement log (task key → `backend_id`) is a D2 record of the run; a run's
  bytes are a function of (input, placement log).
- Cache lookup tries each `backend_id` of the inventory in a fixed order; the first
  hit fixes that task's placement. Misses are placed by load, preferring the backend
  that produced the task's previous version (sticky placement), so early cutoff
  survives.
- The one-producer rule holds either way (§4.3).

### 20.5 Budgets

- Per-task arenas; no run-wide atomic metering of allocations; a per-run RSS and
  device-memory budget; exhaustion of the run is a tool failure (exit 1) with caller
  and device state recovered.
- A face whose work exceeds a budget derived from its **own** estimate refuses with
  `budget` (never `valid`). A request whose predicted output exceeds the run budget
  is refused as a whole before refinement (exit 4). No guard picks faces across the
  run (§19.2).

### 20.6 The GPU is a hypothesis

At NVIDIA's 2.5 ms per face, per-face offload buys nothing. Bicubic jets break even
near N* ≈ 830–1,900 items per batch on GPU1 and about twice that on GPU0. At watch
scale vertex evaluation is about 0.07 CPU-s even if every vertex were a B-spline jet,
against NVIDIA's 22.9 CPU-s, so sizing, subdivision tests, trims and triangulation
dominate. Hypotheses (G8 §4.3): G0 the GPU arm equals CPU-only within CI on the
watch at 0.005 mm; G1 lattice evaluation beats 11 cores above N*; G2 span sizing
profits when per-span CPU cost exceeds about 600 ns on GPU1; G3 every analytic class
has α < 1; G4 visible cold cost is max(0, context − ingress); G5 FP32 is not needed
for speed on the P100.

## 21. Numerics and batch layout

**Stage** S8, S9, S18 · **Evidence** FT §1, §6; G7 §1.1, §6; journal ·
**Decisions** FMA released; predicates in the arena; SIMD where appropriate.

### 21.1 One batch layout for SIMD lanes and warps

- Flat SoA job arrays (item id, u, v) sorted by carrier kind and degree class
  (p, q ≤ 3 specialised; one generic class up to 25).
- Control data SoA per class (x[], y[], z[], w[]) padded to the class degree,
  recentred on the span's box centre.
- Identity once per batch descriptor (§17.2).
- Masked tails with the body's instruction sequence.
- GPU kernels size outputs by exclusive scan and write once; no dynamic
  parallelism.

### 21.2 SIMD

AVX on the fleet (4 doubles, no FMA), AVX2/AVX-512 on rented hosts, NEON (2 doubles)
on Apple. The C++20 lane uses a thin owned layer over Clang vector extensions;
`std::simd` arrives with the C++26 lane [call].

### 21.3 Predicates

UV orientation and in-circle decisions in triangulation use Shewchuk-style adaptive
predicates: plain floating point, extra precision only when a decision is ambiguous.
This is correct arithmetic on the input coordinates, not certification. nvcc
contracts multiply-add into FMA by default, which breaks these predicates, so
predicate code uses explicitly uncontracted operations (`__dmul_rn`/`__dadd_rn`) or
an FMA-aware formulation on every device. `direct_cdt` already decides its
predicates by an interval filter with an exact fallback, on fixed-point integer
input (Audit §4); that is the robust level. A plain-double variant runs in the
arena against the robust one [operator]. Per-device predicate conformance is checked before any GPU
triangulation result is believed.

### 21.4 Precision

FP64 by default. FP32 with recentred coordinates is an arena arm: Metal has no
double, and memory-bound kernels might gain; the P100's half-rate FP64 (15–17
sockets' worth) means FP32 is not needed for speed there (G8 G5).

## 22. Measurement against NVIDIA

**Stage** S18 · **Evidence** G8 §7; G9 §0.2; G1 §5; G14 §0; B2-ha · **Decisions**
steel-man NVIDIA [operator]; D-measure; `.27`, `.48`, `.56`, `.61`, `.67`, `.68`,
`.70`.

### 22.1 Principles [operator]

- NVIDIA is measured at its strongest arm. Publishable numbers start there.
- An invalid NVIDIA result is never our win; it is a finding.
- Our failures are never scored as NVIDIA's (harness exits 3, 4, 5).
- Every timing states its lane and its competing load.

### 22.2 NVIDIA's arms

| Axis | Levels |
|---|---|
| Ingress | OCCT default STEP read (FixShape), the candidate strongest; OCCT translation without ShapeProcess (labelled, never called the steel-man); our projected pcurves in the `.brep` (labelled "our projected pcurves", never the headline, `.70`) |
| Preparation | none; documented plane-range and periodic preparations labelled "requires our preparation" |
| Heal policy | 0, 1 |
| Workers per lane | 12, 24 single-threaded processes |
| Worker placement | pinned to physical cores of the lane's socket, memory bound; floating |
| Assignment | LPT on NVIDIA's own measured per-definition `tessellate` times from a calibration pass (never face count) |
| Angle arm | chord-only (0/0 angles); SDK default 25°/25° plus the chord |
| Path | per-definition `SmApiTessellate`; the USD `tbb::parallel_for` path as an extra arm if it produces valid output |

- NVIDIA's time per input and angle arm = its fastest valid arm. "Valid" means it
  passes the same admission as owned output: every edge two opposite distinct uses,
  shells disjoint with correct signs, Euler over shells, finite points (B2-ex).
  NVIDIA's mesh of the correct `void-block.step` fails in all 8 configurations
  tried; that is recorded as a finding.
- **Headline** (`.48`): the matched-fidelity arm, where both sides meet ε plus the
  25° angle term (owned in the render profile, §6.2); the chord-only arm beside it.
- NVIDIA gets exactly the key set: one definition-frame `.brep` per mesh key with
  chord ε/σ_max, angles unchanged (G9 §0.2). Both NVIDIA assembly paths tessellate
  per occurrence, so this is its strongest form.

### 22.3 Scopes and the headline

- **NVIDIA headline** = minimal ingress process wall (STEP read and transfer, one
  `.brep` per solid, fork to exit) + NVIDIA-only worker makespan (process start,
  context, conversion span, import and heal, tessellation, teardown, exit).
- **Owned headline**, by the symmetry rule: ingress from the same STEP bytes, timed
  the same way, plus per-key meshing; it excludes admission, scene construction,
  emit and display, exactly as NVIDIA's does (`.25.9`).
- Listed beside, never added: our preparation, physical admission, emit, display,
  views, context capture.
- Stage times reported separately: ingress, conversion and preparation, meshing,
  emission, export. The meshing-stage ratio is reported beside the headline.
- **Cold** = a fresh process: CUDA context, module load, decode, empty caches, the
  STEP file's pages dropped with `posix_fadvise(POSIX_FADV_DONTNEED)` before each
  ingress (owned and NVIDIA alike), library residency recorded. **Warm** = a second
  query in the same process after a full drain, retained caches named. Reported side
  by side; warm never replaces cold.
- Accounting per mesh key (D-measure): Σ keys mesh + Σ occurrences emit. Per-scale
  meshes are listed and charged as extra key work.

### 22.4 Protocol

1. ≥ 10 fresh processes per (input, configuration, lane) for headline numbers:
   median, rank-based 95% CI of the median, min and p90. Arena corpus runs ≥ 3
   processes per input. Corpus summaries are sums of costs, never averaged ratios.
   Microbenchmarks of deterministic kernels report the minimum.
2. Configurations interleaved in seeded random order, environment padded by a random
   0–4 KiB per repetition; binary digests recorded.
3. Each run is its own process under `numactl --cpunodebind=N --membind=N` with
   `CUDA_VISIBLE_DEVICES` set to the lane's GPU; never two arena runs on one lane at
   once.
4. **Quiet gate:** three checks a minute apart before the run, 1-minute load
   ≤ 4.5, foreign processes ≤ 150% CPU, no foreign GPU process; classification uses
   foreign CPU only (host busy minus our own tree). Headlines come from quiet runs;
   loaded runs are labelled and never mixed into a quiet summary.
5. **Context row** before and after every run, captured outside the timed interval:
   lane (CPU set, node, GPU), load average, foreign CPU (unthresholded and above 5%),
   our own tree, delivered clock from a 50 ms dependent-add probe (never
   `scaling_cur_freq`), governor, SMT, AutoNUMA, THP, free memory per node, PSI, GPU
   use and process counts, cold or warm. Governor left at `schedutil` (`.27`);
   AutoNUMA left on, neutralised by binding, state recorded (`.67`).
6. **Timers:** `CLOCK_MONOTONIC` wall and `CLOCK_PROCESS_CPUTIME_ID` CPU per stage
   (CPU/wall gives parallelism per stage); CUDA events for device spans; timer
   overhead below 5% of the interval.
7. **Both GPUs.** Every GPU test and arena configuration runs on both lanes. Transfer
   numbers are never compared between GPU0 (x8) and GPU1 (x16) (`.68`). A reference
   configuration repeated on both lanes detects interference by comparing each lane
   with its own history; cross-lane comparison of whole-run times is made only for
   CPU-only configurations [call].
8. Hardware fairness: the owned lane (socket + GPU) against NVIDIA on the same
   socket; NVIDIA has no GPU path, which is its limitation; CPU-only owned results
   reported beside.

### 22.5 Scoring per input

| Owned | NVIDIA (best arm) | Score |
|---|---|---|
| valid | valid | speed ratio; the only publishable comparison |
| valid | no valid arm | a finding; not a win; not in the ratio |
| refused or failed | valid | owned loss, counted against the goal |
| refused or failed | no valid arm | recorded; excluded from speed |

Exclusions, listed in their own tables: refused placements (non-uniform, shear)
(`.56`); tessellated-only parts (`.61`). NVIDIA's measured deviation is reported beside
its time and never used to slow it down. The speed goal is judged on the real-input
strata (§23.4); authored families and fuzz mutants count toward robustness only.

### 22.6 What has to fall

847 s over 24 cores is 35 s at perfect scaling against NVIDIA's 6–8 s, so per-face
cost must fall about 5× just to tie on wall time and about 37× to match NVIDIA's
22.9 CPU-s. NVIDIA's own efficiency is low (22.9 CPU-s over 12 processes is 1.9 s
ideal against 6–8 s observed), so its wall time is beatable once CPU-seconds are
competitive (Map S18). T∞, the largest single face, bounds any schedule. These are
the targets the arena measures against; no outcome is claimed.

## 23. The arena

**Stage** all · **Evidence** journal; M2 §5.7; G2 §6; G4 §6; G7 H5; G8 §3.6; G10 §6;
G13 H3; G18 H1 · **Decisions** 64 configurations; resolution V; follow what is
intriguing with no time box; permutations as mechanisms land; one ablation is not a
law [operator].

### 23.1 Always-on core

Global task graph; edge-first shared keys; closure arm A; per-face accounting;
analytic closed-form density; the ε ledger; clearance budgets; A0; certification
removed; the status lattice; per-key accounting.

### 23.2 The first arena: eight two-level flags, 2^(8−2), resolution V

| # | Flag | Default level | Other level | Basis |
|---|---|---|---|---|
| F1 | `predicates` | robust adaptive | plain double | [operator] |
| F2 | `face_generator` (torus, revolution, B-spline) | lattice + band CDT | metric CDT | FT §4; journal |
| F3 | `in_face_split` | clusters C1/C2 | off | PC §5; G8 §5 |
| F4 | `staging` | background healer | single pass | [operator] |
| F5 | `reconciliation` | M2 bundle (point solve, `r2_linear_step`, ¼-lfs leaps, recorded tangent steps, structural-first choice) | healing-paper classes R1–R6 with distance split | G1 adoption; M2 H4 |
| F6 | `collapse` | E′/S′ on | off (F′ only) | G10 H3 |
| F7 | `eval_precision` | FP64 | FP32 recentred | journal; G8 G5 |
| F8 | `gpu` | lane GPU on | CPU only | G8 G0–G2 |

Generators F7 = F1·F2·F3·F4 and F8 = F1·F2·F5·F6; defining relation
I = 12347 = 12568 = 345678. Every word has length ≥ 5, so main effects and two-way
interactions are clear of each other while every flag stays two-level [call: the
flag set; operator: 64 and the fraction]. The draft's SIMD vector/scalar flag leaves
the factorial: the scalar path is a reference, measured as a microbenchmark pair
[call]. The draft's thresholds flag moves to §23.3 now that β and r_c are separate.

### 23.3 Follow-up arms

Run as one-factor arms against the default, or as a new fraction around anything
intriguing:

| Arm | Levels | Hypothesis |
|---|---|---|
| `thresholds` | (β, r_c) = (ε/2, ε/2) / (ε/4, ε/4) | G10 §6 |
| `theta_cut` | 3.6°, 8.1°, 10° | M2 H9 |
| `r2_linear_step`, `transverse_leap_by_conditioning`, `tangent_split`, `l3_blend` | on / off each | M2 H4, H6, H7 |
| `boundary_closure` | A hereditary / B synchronous check-and-split | G2 H5, H6 |
| `split_point`; `tau0` | dyadic / bisector-snapped; π/3 / π/6 | G2 §6 |
| `biased_split` | off / threshold × (1 − 2⁻⁴⁰) | G7 H5 |
| `signed_clearance` | off / on | G4 H3 |
| `local_pair_test` | off / on where clearance is not established | `.59` |
| `cap_c` | 2 / 1.155 / 1.414 / off | G13 H3 |
| `postprocess` | none / flips / collapses / relaxation | G18 H1 |
| `scale_buckets` | off / r = 2^(1/8) | G9 H2; `.57` |
| `mirror_3mf` | baked opposite-hand / det < 0 item | G9 H3 |
| `inline_resolution`, `speculative` | follow-ups of F4 | journal |
| `placement` | load-reactive / fixed | G12 §5.3 |
| `smt_workers` | 12 / 24 per socket | G8 §6 |
| `gpu_driver_core` | SMT sibling / dedicated core | G8 §8 |
| `cross_socket_steal` | on / separate pools | G8 P3 |
| `simd` | vector / scalar | microbenchmark |
| `topology_change` | refuse / allow with record | `.47` |
| `directed_rounding_3mf` | off / on | §18.3 |

Defaults from the arena are provisional; a perturb-and-permeate cycle keeps
perturbing them after principal development [operator].

### 23.4 Corpus, strata and requests

- **Strata, never pooled:** production exports (NIST MBE PMI 49 unique, CAx-IF 53,
  five open-hardware TDPs 376, step-conformance 27, the watch); the OCCT dataset
  (333, bug reproductions that over-represent pathology); ABC when acquired (fixed
  selection: 3 chunks, 30,000-slot robustness sweep, 1,000-model speed sample; the
  download term is open, `.28`); Gmsh/FreeCAD/Netgen/resources4truck held sets.
  Fusion 360 Gallery is out permanently (`.29`). BRL-CAD from a mirror into
  quarantine as reference only (`.30`). A second OM10 revision waits (`.36`).
- **Report classes,** fixed before results: dominant surface kind (analytic only,
  mixed, B-spline heavy), size by face count, whether the source needed rungs 1–5.
  A regression in one class is never diluted by corpus-wide rates. Correctness,
  time, work and memory outcomes are kept per family, regressions and censored cases
  included.
- **Requests per input,** fixed before any result: inputs with trustworthy units at
  0.005 and 0.025 mm and at 1e-3 and 1e-4 of the diagonal; arbitrary-scale inputs
  (ABC) at the relative levels only [operator]. **PMI-bearing inputs** (17 NIST
  models) are meshed at the absolute levels only in scored runs, because 1e-3 of the
  diagonal violates their tolerances on all 17 (worst 84×) and 1e-4 still on 11; the
  warnings the relative levels would raise are computed from the request and
  recorded (G15 §8 item 1) [open: confirmation that this narrowing of the request
  set on one input class is acceptable].
- The output-size guard applies at every level.

### 23.5 Authored families

Parametric, generated, none keyed to the watch. Each family states its parameters,
the branch it forces and its expected outcome.

| Group | Families |
|---|---|
| Closure (G2 §5.1) | F1 disk with off-centre hole, gap/sagitta 10 → 0.01; F1b hole inside a chord's cap (containment without crossing); F2 S-bent UV pcurve on a clustered-knot B-spline; F3 two inner circles; F4 crescent cusp, Δκ → 0; F5 wedge α 60° → 0.1°; F6 saddle z = κxy beside planes; F7 cone trimmed near and through the apex; F8 crossing source trims; F9 3D neck thinner than 2ε; F10 seam-crossing, multi-turn, shifted periodic; F11 non-manifold edge and same-face seam with different depths; F12 strip of k faces with alternating inner loops; F14 speed ratio ρ to 10³ |
| Embedding (G4 §6) | K1 extruded crescent (θ 0.5–20°, R_B/R_A 1.2–10, even and lattice slabs, concave face ×1 and ×5); K2 lens; K3 chisel and material cusp; K4 doubly-curved knives; K5 curved knife edges (countersink lip); K6 reflex slot; T1 thin tube w/ε 0.25–2.5; T2 slab with a boss; T3 parallel planes (no change expected); T4 thin spherical shells; S1 B-spline hairpin and helical face; U1 hole near a convex arc and loops at 0.5–10° (straddles OCCT's 5° exemption); U2 concave-trim sliver; V1 pyramid apex and needle tips; R1 faces 2ε + η apart after a resolution move; C1 source contacts |
| Collapse (G10 §5) | A1 planar sliver width sweep around η and 2β; A2 curved slivers; A3 twisted strip k = 0–10; A4 bilinear patch; A5 L, crescent, serpentine, dumbbell; A6 short line and arc; A7 polygonal hole N = 8–512; A8 cut corner; A9 merged vertex leaving a convex face; A10 shallow step; A11 through-pinhole; A12 hourglass; A13 island sliver; A14 plane–freeform sliver; A15 touching shells; A16 collapse beside a thin wall |
| Reconciliation (Heal §7.5; M2 §5.7) | edge–surface offsets 1e-6–1e-2; dihedral sweeps across the conditioning cutoff; pcurve-free variants; vertices off curves; plane–cylinder tangent step; fillet tangent to a plane with controlled Δκ and stored-curve slide (M2 H2) |
| Entities (G5 §4) | revolution profiles (B-spline, conic, line, touching the axis); extrusion; elementary and bicubic offsets d = 0.01–0.9 of the curvature radius; spindle and horn tori (all 410 corpus faces); high-degree B-splines; voids (`void-block.step` and its void-free twin, signed volumes 936 / 1000); apex cones; sheets |
| Identity (G16 §3.2) | small closed circles and lenses at ε ≥ R |
| Placement (G9 §8) | mirrored chiral part (helical gear); one curved definition at 32 scales over [0.5, 2]; authored CTO fixtures |
| Shading (G13 §5) | the seven exact surfaces of G13's probe |
| Contact (G17 §5) | pin-in-hole c ∈ {1, 2, 5, 10} µm, δ_h/c ∈ {1, 0.5, 0.2, 0.1} (physics phase) |
| Carried regressions | the cylinder-seam trimmed family; OM10 fixtures named in B2 |
| Scale and far placement | whole-model scale 1e±6; 100 m offsets; N-holes, N-grooves, N-instances scaling |

**Fuzzer:** mutations over STEP and B-rep parameters (loop permutations, decimal
spellings, NaN/Inf injected after ingress at evaluator, projection and sizing
outputs). Timeout counts as failure; seeds logged; failures minimise into permanent
regressions.

### 23.6 Per-run record

Per input and configuration: wall and CPU per stage, work counts (samples, cells,
band triangles, deferred items per rung, closure splits, collapse clusters, clearance
candidates), peak RSS and device memory, the full oracle verdict, and the context
row. Correctness failures are never averaged into speed. The first arena is
time-boxed in hours, not days; anything intriguing is followed with no time box
[operator].

## 24. Verification stack

**Stage** S25 · **Evidence** Map S25; G7 §3; G3 §4.5; G16 §3.4; G13 §6; G11 §5.3;
G15 §7; BLACKBIRD testing · **Decisions** mutants after the stack settles
[operator].

Every oracle changes exit status. Each runs offline on written outputs against the
original source, never trusting a producer status. Known-bad inputs belong to each
oracle's own corpus and run in the same pass; nothing tests an oracle separately.

### 24.1 Oracles by fault class

| # | Oracle | Fault class | Tier | Fails when |
|---|---|---|---|---|
| 1 | Two-sided sound deviation against the target of §3.2, over the trim domain, with a covering-radius correction or bounded Hausdorff; boundary, pole and apex triangles included; collapse geometry checked against r_c | deviation | spec | any point beyond ε_loc in either direction |
| 2 | Exact per-face UV oracles: every constraint segment present; coverage with no overlap or gap; orientation independent of topology; Steiner points on their parents; CDT property where claimed | triangulation | spec | any violation |
| 3 | Topology by vertex key (never by position): shared-edge identity and order across incident faces; edge-use counts; per-shell Euler with face genus χ = 2 − 2g − b − p; accounting of every face, definition, key and occurrence | watertightness | property | any mismatch |
| 4 | Classification map (G16 §3.4): vertex on model vertex within the request; vertex on E at its t with matching (u, v) on every incident face; consecutive chain samples form mesh edges used by exactly E's faces and no other mesh edge joins two samples of one chain (checks §10.4); triangles classified on F or its boundary or a survivor; every source face has a range, absorbed or refused record; every UUID read reaches output; per-format readback | identity | spec | any check |
| 5 | Offline self-intersection with the retained contact BVH and filtered predicates, adjacent and coplanar pairs included; exempts only reported source pairs | embedding | property | any other forbidden pair |
| 6 | Normals: interpolated corner normals against the exact normal at the nearest carrier point (planes, cylinders, spheres ≤ 1e-12 rad); facet error ≤ arcsin(√3·R·κ); glTF written normals reproduce the source dihedral within 1e-5 rad, unit within 1e-6 | shading | spec/property | any triangle or corner outside |
| 7 | Reader-equivalent export oracle per format and named consumer: parse; rebuild what PrusaSlicer, Cura, lib3mf, three.js and fTetWild hold; check degeneracy, orientation, manifoldness by index, pinched vertices, exact self-intersection against the FP64 set, displacement ≤ δ; 2.4–5.2 µs per triangle | export | model | any new defect |
| 8 | Ingress: authored fixtures per override class must produce the raw value and a record naming the class; the meshed boundary is checked against the raw master; CAx-IF and NIST validation properties against exact properties of the geometry as read; 86 Rat Rig inch/mm twins equal ×25.4 within 1e-9; the owned reader byte-identical to the bridge where the bridge agrees with raw | reading | spec/metamorphic | any mismatch |
| 9 | PMI reader (G15 §7): tolerance counts equal the sender's validation properties (STC-10's composite counting a known finding); values against NIST drawings; mm→inch rewrite leaves every margin unchanged; nominal-on-limit holes give PMI-OUT at every ε and mesh mating diameter D − 2δ_f | request | spec/metamorphic | any mismatch |
| 10 | Mesh volume within 2·ε·A of exact volume (offline exact integration) | deviation (global) | property | any part beyond |
| 11 | O-bytes per `backend_id` (§24.2) | determinism | metamorphic | any byte difference |
| 12 | FI-1 to FI-5 across backends (§24.2) | backend correctness | metamorphic/differential | an unattributable difference |
| 13 | Incremental = fresh via an edit generator (§24.3), which is also the cache key completeness test | caching | metamorphic | any byte difference, or a case that does not both hit and miss |
| 14 | Metamorphic geometry: whole-model scale by 2^k with ε and every declared length scaled gives the same keyed triangle sets, positions scaled exactly (every threshold is relative or scales with ε, so a difference names a hidden absolute constant); scale 1e±6, far translation and rigid rotation keep topology and pass oracle 1; mesh(K = 1) ≡ mesh(K = k); chain sets identical across thread counts and shuffled face orders | invariance | metamorphic | any difference |
| 15 | Forced caps: each work, depth, iteration and memory bound forced to fire; the status and the returned object are checked | silent accept | spec | a cap yields `valid` or a missing record |
| 16 | Per-device predicate conformance | arithmetic | spec | any sign error on the conformance set |

### 24.2 Determinism oracles

- **O-bytes** (D0–D2): same input, one `backend_id`, under thread counts 1, 2, an odd
  count, 12, 24, 48; randomized task order and injected delays; varied GPU launch
  shapes and streams; GPU0 against GPU1; shuffled batch compositions; cold against
  warm cache; incremental against fresh. D0 also across every backend. Each run
  records the worker count and launch shape it actually used, and the input exceeds
  every parallel threshold, or the test cannot fail.
- **FI across backends:** FI-1 accounting from emitted output; FI-2 status and class
  differences only where both backends' deciding quantity lies within its
  cross-backend error bound of the threshold, recomputed in higher precision; FI-3
  topological invariants from the mesh welded by key; FI-4 per-edge chains (same end
  keys, monotone parameters, positions within the producer kernel's forward-error
  bound c·u·S when counts match); FI-5 positions and normals on shared keys within
  each kernel's forward-error bound.
- **Comparisons that cannot fail and are not used:** cross-backend Hausdorff ≤ 2ε
  after both passed deviation; watertightness after positional welding; counts read
  from the shared decode; determinism runs that do not vary concurrency; two runs
  reading the same cache entries; validity against the mesher's own reported error;
  statuses compared through a shared record. Cross-backend byte equality of D1 layers
  is the wrong oracle (it fails with no bug).

### 24.3 Incremental = fresh

Metamorphic: two computations that must agree, the cold one taking no cached path.
An edit generator applies random single-input perturbations (one control point by
1 ulp, a knot multiplicity, ε by 1 ulp, one flag, one pcurve, renumbering, file
rename, thread count). Each case must both hit and miss somewhere, or it is
discarded as vacuous; the warm result must equal a cold run of the edited input, and
pure renumbering must hit everywhere with unchanged bytes. One oracle covers key
completeness, key minimality and edit equivalence (G7 §5.6, G12 §5.4).

### 24.4 Mutation testing

Comes later, on the fleet, once this stack has settled into the one the project
builds on. Mutants test the tests, not the project [operator].

## 25. Certification removal and the old mesher

**Stage** all · **Evidence** Audit §3–§6 · **Decisions** certification excised;
the old mesher leaves the source graph [operator].

- **Removed from the hot path:** Rational curve parameters and site UVs; per-facet
  certified bounds and their Rational compare; precise and materialised fallback
  cascades; certificate-driven horizon refinement; interval kernels used as
  acceptance tests; Rational SAH costs; per-read GCD; snapshot copies of exact
  tables; the 165 s whole-component contact classification; the stop-token check on
  every work charge; run-wide atomic allocation metering; per-query source endpoint
  guards (moved to ingress, once per source).
- **Kept and ported:** `DefinitionBoundaryForest` on doubles and dyadic paths;
  `PhysicalNodeKey`, `FacetKey`, site sources, occurrence and wedge bindings, edge
  provenance; `prepare_face_domain`'s occurrence and joint logic as specification;
  the packet zipper; `direct_cdt` on fixed-point input; the numeric kernels and CUDA
  ops; canonical ordering by key; source decode and IO codecs.
- **Moved to the test suite:** the contact BVH and filtered predicates; the
  independent original-source readers.
- **Retired:** the old mesher leaves the build; every consumer (watch render,
  exploded diagrams, scene and display, cut and extrusion result meshers) moves to
  the new path. Old results, images and reports remain, each gaining one line saying
  it is historical information from the previous regime. Repository docs lose the
  certification language (`.25.3`).
- **Other operations** (Boolean, classification, arrangement, intersection) follow
  immediately after the mesher, in an order chosen then [operator].
- **Python** may remain an optional boundary and research oracle; no production
  geometry callbacks. Whether Python belongs in the delivered system is a deferred
  architecture study.

## 26. Deferred and roadmap

**Stage** S22, S16, S12 · **Evidence** G17; G12; G18; journal · **Decisions**
D-imprint as restated (`.43`), D-provisional, `.44`, `.45`.

### 26.1 Contact and physics

Sliding mechanism contact needs no matched discretization; every formulation and
engine read works on non-matching meshes (G17 §0). D-imprint is restated (`.43`):
per-face density tied to clearance, plus exact-surface normals. Watch physics is
joint-level first, with contact only at the escapement (`.44`). Clearances come from
a declared contact record supplied with the model; a horological default table later
(`.45`).

Interfaces this design carries now or reserves (G17 §3):

| # | Interface | State in this design |
|---|---|---|
| I1 | per-face request map (ε_f, θ_f, ℓ_f) in the request and the key | built (§3.1, §16.2) |
| I2 | declared contact record (sides, kind, clearance, ε_contact, θ_contact) | input reserved; reduces into I1 |
| I3 | classification map to (face, u, v) / (edge, t) / vertex | built (§17) |
| I4 | bodies never merged; each occurrence a closed body | built (§3.11, §18.3) |
| I5 | extra constraint curves in face meshing, keyed like edge samples | later: a "virtual edge" kind in the band constraint set |
| I6 | key slot for per-occurrence face variants | reserved (§16.2) |
| I7 | final-only status for physics | built (§4.5, §19.5) |
| I8 | contact fit report between occurrences | built as offline report (§18.4) |
| I9 | one-sided tolerance per face (ε⁺, ε⁻) | later: the contract and deviation oracle need a signed band |

Contact-density constraint for the physics phase: on a face of a declared contact
pair, ε_f ≤ c/k with k = 10 and θ_f ≤ 10° (arena flags; M-1 measures them); planar
contact faces need nothing more; contact normals and closest points come from the
exact surface through I3.

### 26.2 Edits, LOD, sharpen-in-place

Covered by §19 and §15.3; nothing more is deferred except sub-face reuse keys
(§19.2) and LOD in files (`.41`).

### 26.3 Toolchain, platforms, perturb-and-permeate

C++26 lane for newer GPUs on rented hosts, then Apple Silicon (Metal without double,
NEON): §21.2, §21.4 keep both open. The perturb-and-permeate cycle runs on the fleet
after principal development, open to any interested partner [operator].

## 27. Open questions

Each names the choice, its default and what settles it. Questions for the operator
are marked **(operator)**.

| # | Choice | Default | Settled by |
|---|---|---|---|
| 1 | truncation angle θ_cut | 8.1° | arena `theta_cut` (M2 H9) |
| 2 | reconciliation: M2 bundle vs healing classes | M2 bundle | first arena F5 (M2 H4) |
| 3 | M2 sub-proposals separately | each on | follow-up arms (M2 H4, H6, H7) |
| 4 | small-feature collapse at all | on | F6 (G10 H3: drop E′/S′ if no stratum gains ≥ 1%) |
| 5 | (β, r_c) | (ε/2, ε/2) | arm `thresholds` |
| 6 | face generator | lattice + band | F2 |
| 7 | predicates | robust adaptive | F1 [operator: arena both] |
| 8 | evaluation precision | FP64 | F7 |
| 9 | in-face clusters in the default | on only where a face exceeds τ_split | F3 plus the per-face work-distribution measurement (PC §6) |
| 10 | staging | background healer | F4; inline and speculative follow-ups |
| 11 | GPU in meshing | on, by cost model | F8; G8 G0–G5 |
| 12 | closure arm | A hereditary | `boundary_closure` (G2 H5, H6) |
| 13 | split point, τ₀ | dyadic, π/3 | follow-up arms |
| 14 | ties at round CAD numbers | unbiased | `biased_split` (G7 H5) |
| 15 | signed clearance credit | off | `signed_clearance` (G4 H3) |
| 16 | clearance not established | tighten, then refuse | `local_pair_test` (`.59`) |
| 17 | clearance floor c_floor | 2δ_exp + bound resolution | families C1, T1, K3 under oracle 5 |
| 18 | cap factor c | 2 | `cap_c` (G13 H3: ≤ 2% triangles, ≤ 2.1 θ_ref) |
| 19 | post-processing operations | none | `postprocess` (G18 H1) |
| 20 | scale bucketing | off | `scale_buckets` (G9 H2) |
| 21 | 3MF mirrored items | baked opposite-hand | slice both variants in PrusaSlicer and Cura (G9 H3) |
| 22 | placement | load-reactive | arm `placement` for edit locality (G12 §5.3) |
| 23 | SMT, driver core, cross-socket stealing | 12 workers, sibling, stealing on | arms (G8 §6, §8, P3) |
| 24 | owned transcendentals | owned | the G7 probe on an AVX2+FMA host decides required vs tidier |
| 25 | freeform angle term | measured corner-normal spread per emitted triangle | owned per-span σ̲(DS) and principal-curvature bound, with a dense normal-turning oracle (G14 §6) |
| 26 | UV-arc bounds for projected occurrences | closed forms on analytic carriers; sampled with 2× margin on freeform plus the residual detector | the same σ̲(DS) derivation; G2 H2 detector counts |
| 27 | target displacement field shape | linear decay over the adjacent chain segment's ℓ₃ | M2 H3 model oracle (global solve) and oracle 1 |
| 28 | trim-sliver reserve r_trim | L_S·w̄ | oracle 1 on U2 and concave-trim families; drop if it never binds |
| 29 | placement τ | 1e-6 | G9 H5 over every raw placement in the corpus |
| 30 | gaps against declared uncertainty | gross gate structural (¼ lfs) | G15 H5: do measured gaps track u_ctx across export triples |
| 31 | PMI warning threshold c_max | 0.1 (`.54`) | G15 H4 offline conformance-flip measurement |
| 32 | PMI-bearing inputs at relative ε | absolute only in scored runs; warnings recorded | **(operator)** confirmation at G2 (§23.4) |
| 33 | face-over-solid plain style | face wins, recorded | G9 H4 corpus count |
| 34 | contact k and θ_f | 10, 10° | M-1 pin-in-hole family (physics phase) |
| 35 | identity carriage cost | ranges | G16 H1–H4 (< 1% time, < 2% bytes) |
| 36 | clearance cost | budgets on | G4 H4 (< 2% time, < 5% triangles) |
| 37 | angle-term cost | render profile only | G14 H-angle census of radii below 42ε |
| 38 | grading | none | G14 H-no-grade slicer and fTetWild measurements |
| 39 | native revolution and extrusion | native | G5 H1, H2 |
| 40 | general offsets | refused until a corpus shows one | ABC census once `.28` is answered **(operator, open)** |
| 41 | 3MF directed rounding | off (refuse the pair instead) | `directed_rounding_3mf` |
| 42 | governor | `schedutil`, delivered clock recorded | revisit only if variance hides effects (`.27`) |

## 28. Corrections carried from the M4 lanes

Each correction recorded on bead `brepgpu-12s.20`, and where this design resolves it.

| Lane | Correction | Section |
|---|---|---|
| G10 | lines need samples on ruled and twisted faces | §10.3 item 2 |
| G10 | "edge shorter than 4β" clause deleted | §10.3 |
| G10 | β split into the R2 gate and the collapse radius r_c; thresholds flag changes | symbols table; §9.4; §23.3 |
| G14 | per-span principal-curvature bound absent for the freeform angle term | §6.2; §27 #25 |
| G14 | grading conflicts with sharpen-in-place and incremental remeshing | §6.2 (no grading) |
| G16 | sample-count floor | §10.4; oracle §24.1 #4 |
| G16 | `absorbed` face outcome | §3.8; §9.5 |
| G16 | identity as face-contiguous ranges off the hot path | §17.2 |
| G2 | settle and protect with P1–P4, packing stop, arm B as arena | §11; §23.3 |
| G2 | L3 blend weights from the curve parameter | §8.4 |
| G2 | healing paper's stop rule replaced by lens width against separation | §11.1–§11.2 |
| G7 | determinism classes D0–D3 | §4.2 |
| G7 | `backend_id` includes libm; owned transcendentals | §4.2 |
| G7 | cache key over dependency output digests | §19.1 |
| G7 | draft wording "bit-identical ... or across runs" contradicted D-determinism | §1 non-goals; §4.2 |
| G7 | round-number ties: biased threshold arm | §10.2; §23.3 |
| G15 | PMI inputs at absolute ε only | §23.4; §27 #32 |
| G15 | nominal-on-limit holes give PMI-OUT | §6.3; §24.1 #9 |
| G15 | declared uncertainty reflects the exporter | §6.3 |
| G15 | G = min(4ε, ¼ lfs) refuses sender-legal gaps | §8.5 (gate structural); §27 #30 |
| G13 | Rule A cap rule | §15.1 |
| G13 | Rule B exact normals, never averaged; classification measured (draft §5.2 item 6 and §4.7 wrong) | §3.10; §15.1; §15.2 |
| G13 | Rule C per-format normals | §18.2 |
| G13 | freeform collapsed-edge limit normals | §15.1 |
| G9 | per-key accounting; NVIDIA given one definition-frame `.brep` per key; emit timed separately; counts per placement kind | §16.4; §22.2 |
| G9 | G17's per-face minimum within one scale class | §16.2 |
| G9 | τ = 1e-6 | §16.1 |
| G4 | §5.6 becomes clearance budgets in construction | §12 |
| G4 | ε̂ ≤ 2ε + s_e plus μ | §12.1 |
| G4 | shared-edge tilt rule; vertex cone rule; UV half-clearance | §12.2–§12.3 |
| G4 | budgets never split between faces | §12.3 |
| G4 | thin walls wider than ε meshed at ε_loc < w/2, never refused | §12.4 |
| G4 | the draft's cylinder slab rule crosses (59/2016) | §12.6; §13.1 |
| G5 | admission matrix; 6.7% of exports refused on kind | §5.4; §5.3 |
| G5 | revolution and extrusion native (corrects Map X12) | §5.4; §13.1 |
| G5 | recomputation is the normal path (205 of 642,569 edges carry pcurves) | §5.1; §8.1 |
| G5 | carrier table flags R ≤ r and degenerate tori; interior pole lines | §5.4 rows 12–13; §10.5; §13.1 |
| G5 | degree up to 25 | §5.4 row 3; §21.1 |
| G5 | OCCT translation edits feed the authority rule | §5.1–§5.2 |
| G5 | NVIDIA may resolve offsets to their basis | §5.4 |
| G6 | lifting mm-only needs the three raw reads, not a map | §5.2; §5.3 |
| G6 | UR-1–UR-10 | §5.5 |
| G6 | OCCT maps revolution, extrusion, offset pcurves wrongly (UR-9) | §5.5 |
| G6 | 86 Rat Rig twins as oracle | §5.3; §24.1 #8 |
| G6 | 41 files write the degree as 0.0174532925 | §5.5 UR-3 |
| G12 | incremental = fresh behind the cache, no patching | §19.2 |
| G12 | speculative meshing provisional unless content-ruled | §4.5 |
| G12 | invalidation includes vertex moves, closure reads, spatial queries, collapse components | §19.2; §19.3 |
| G12 | vertex numbering at publication | §4.4 |
| G12 | M2 branch step 5 at a fixed level | §8.6 |
| G12 | four forbidden global couplings | §19.2 |
| G8 | per-socket work stealing plus HeteroPrio; per-device models; CPU-only fallback | §20.1; §20.3 |
| G8 | split threshold τ = 0.1·W/(P − 1), floor ≈ 0.1 ms | §13.4 |
| G8 | GPU hypothesis; analytic never offloads; break-even about 10³ | §20.6 |
| G8 | protocol: ≥ 10 processes, median and rank CI, interleaving, numactl, quiet gate, strongest arm | §22.4 |
| G1 | raw entity authoritative; OCCT an evaluator; master over partner; gap never charged | §5.1 |
| G1 | M1 corrected: missing pcurves stay missing; 0 vertex merges | §5.1 |
| G1 | NVIDIA steel-man ingress is OCCT's default read; today's arm mislabelled | §22.2 |
| G1 | OCCT CTO defects read raw | §16.1 |
| G3 | float32-safe construction in the written frame, C1–C3, δ bound | §18.1; §3.4 |
| G3 | identity weld only; exact decimals | §18.2 |
| G3 | offline reader-equivalent oracle | §24.1 #7 |
| G3 | not snap rounding; corrects the "12%" figure and the PrusaSlicer claim | §18.1; §18 opening; §18.5 |

## 29. Left to the builder

- Byte layouts of the internal output container and the cache store, provided they
  carry every field named in §7.3, §17 and §19.
- Task-graph, deque and arena data structures, within §4 and §20.
- How `direct_cdt` is adapted to fixed-point scaled-chart input and component
  splitting.
- The exact flag-to-run assignment of the 2^(8−2) design, from the generators in
  §23.2.
- Corpus sample sizes within a few fleet hours per arena pass.
- Unit decomposition and order under JSMNTL (sub-plan, red tests at the right oracle
  tier, code, green, one independent review, fixes), in the M7 plan.
