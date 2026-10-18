# Fast mesher design

Status: M5 design of the fast-mesher campaign
([plan](fast-mesher-campaign/PLAN.md), bead `brepgpu-12s.20`), with the M6
tribunal's fixes written in ([findings](fast-mesher-campaign/M6-findings.md)).
G2 passed 2026-10-19 on the operator's delegation ("just make sane choices"). Plan from this document.

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
| ε_def, ε_f | ε in definition units for one mesh key; the per-face request | §16.1; ≤ ε_def |
| ε_ref | reference length of the repaired target of one definition (§3.2): sets η, β and the lfs floor, never the density | min ε_def over the definition's keys in the session |
| ε_loc | the local budget a triangle must meet (§3.4) | ≤ ε_f |
| η | R1 "exact" gate | ε_ref/16 |
| β | R2 small-move gate (reconciliation only) | ε_ref/2 |
| r_c | collapse radius, charged against ε | ε/2 |
| η_s | sliver width for sliver collapse | ε/16 |
| θ_cut | truncation angle of the point solve | 8.1° [open] |
| lfs | 3D local feature size of a definition (§8.8), over entities above collapse scale, floored at ε_ref | — |
| ℓ_f | decay length of face f's target displacement (§3.2) | ½·min lfs over f's loops |
| τ₀ | UV tangent-variation cap in closure | π/3 |
| m_α, m_c | angular margin of the wedge rule (rad); clearance factor (1 − m_c) (§12.3) | 10× the bound on θ's evaluation error; 0.1 |
| σ̲, σ̄, M_S | per-span lower and upper bounds on the singular values of DS; upper bound on ‖D²S‖ (§13.1) | from span data |
| u_ctx | the governing uncertainty of a representation context | read per context |
| δ(R) | float32 rounding move for coordinates of magnitude ≤ R: (√3/2)·2^(⌊log₂R⌋−23) | 3.3 nm for R < 64 mm; 26 nm for R < 512 mm |
| R_max, δ_exp | construction frame bound of a key and its export reserve (§18.1) | binade of the unexploded root box; per §18.1 |

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
- The owned STEP reader (`.25.7`) is on the critical path: no headline is
  published until owned ingress fits a budget no larger than NVIDIA's minimal
  ingress (§5.6, §22.3).
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
  ε_def in definition units (§16.1).
- A per-face request map ε_f ≤ ε_def may tighten faces (contact density, §26.1;
  thin features, §12). It is part of the mesh key (G17 I1).
- The request is validated before anything else runs: ε finite and > 0, units
  resolved, a relative request resolvable (§6.1). Anything else refuses the
  request by name (exit 4).
- The default output promises ε only. The angle term exists in the render
  profile only (`.49`; §6.2).

### 3.2 The target

The target is the repaired coherent solid. Face carriers are never modified;
edges and vertices take their resolved positions (§8). The target does not
depend on the request's ε, the dyadic cut, the backend or the LOD level, so
LOD levels, sharpen-in-place steps and per-scale keys mesh one solid.

- **One target per definition.** Reconciliation gates (η, β) and the lfs floor
  derive from ε_ref, a request field fixed per definition for the session:
  default the smallest ε_def over the definition's keys; a session declaring an
  LOD ladder or a sharpen sequence declares its finest ε. A later request below
  ε_ref re-resolves the target, which is a recorded key miss, never a silent
  change. Collapse, the dyadic cut and the budgets stay ε-dependent.
- **The resolved edge** x_e(t) is a pointwise function of t: stage-1 data at t,
  the L1/L2 vertex results and the L3 blend, all under ε_ref gates (§8.4).
  Chains sample it; a sample's position is the same at every ε, so chains at
  other ε reuse samples by dyadic path.
- **Boundary displacement.** For each face occurrence f of e,
  g_f(t) = n_f·(x_e(t) − S_f(u_f(t))), signed, n_f the unit normal oriented by
  face sense, u_f(t) the foot measured after L3 (§8.4). At a vertex g is the
  vertex's own offset, so g is continuous around each loop. Plane rows are hard
  (§8.3), so g = 0 on planes, except where an overdetermined hard set leaves a
  residual (≤ η) or two near-coplanar planes share a symmetric step (§8.7).
- **The field.** f's target is S_f(u) + φ_f(u)·n_f(u) with
  φ_f(p) = g_f(π(p))·h(δ(p)/ℓ_f) for δ(p) < ℓ_f and 0 beyond. δ(p) is the 3D
  distance from S_f(p) to f's boundary, π(p) the nearest boundary point,
  h(s) = 1 − 3s² + 2s³ (C¹: h′(0) = h(1) = h′(1) = 0), and
  ℓ_f = ½·min lfs over f's loops (§8.8), a function of source geometry and
  ε_ref only. ℓ_f is recorded in the face record. [call; replaces the linear
  decay over the adjacent chain segment, whose kinks made the interpolation
  error first-order and whose length moved with ε; M2 §5.3 leaves the shape
  open]
- **Away from reflex corners** φ_f is C¹ with
  M_φ ≤ 6d/ℓ_f² + 3d′/ℓ_f + d″, d, d′, d″ bounds on |g| and its first two arc-
  length derivatives along the boundary within ℓ_f (d from the recorded chain
  maximum; d′ and d″ from sample differences, an M8-measured default that
  oracle 1 can falsify). Across the bisector of a reflex corner φ_f can jump by
  up to 2d; where a reflex corner's incident edges carry d > ε_loc/4, the
  bisector segment of length ℓ_f is a constraint line in the band (§13.2), so no
  triangle straddles it.
- **Normals.** Rule B corners carry the carrier normal (§3.10, §15.1). The
  target's tilt against it is at most ψ_f = atan(1.5d/ℓ_f + d′), and A0 is
  checked with that margin (§3.6).
- Distances are to the trimmed target faces, never to untrimmed carriers.
- The source discrepancy d_e is a starting-condition metric, reported per edge,
  never charged against ε. [operator]

### 3.3 Deviation

- Every point of every emitted triangle, **as written** (after float32
  rounding), lies within ε_f of the target, and every target point lies within
  ε_f of the mesh, except collapsed geometry, which lies within r_c of its
  survivor (§9). The construction meets this by building every triangle to its
  ledger (§3.4); the oracle checks it with thresholds taken from inputs only
  (ε_f, δ_exp of the key, r_c; §24.1 row 1).
- Collapse displacement counts against ε (`.46`). Export rounding counts against
  ε (§3.4).

### 3.4 The ε ledger

Every emitted triangle t of face f meets

  e(t) + r_disp(t) + r_trim(t) + r_coll(t) + δ_exp ≤ ε_loc(t)

- **e(t)** is the triangle's interpolation bound against f's carrier on the
  evaluator the generator uses (§10.3, §13). For Waldron-form rules,
  e(t) = ½·r_min'(t)² in the face's scaled chart (FT §1.3), with D the maximum
  over every span the triangle's UV hull meets (§13.1).
- **r_disp(t)** = min(½·M_φ·R(t)², 2d_t) for t within ℓ_f of f's boundary, else
  0; R(t) the triangle's 3D smallest-containing radius, d_t the largest |g|
  within ℓ_f of t (§3.2). It is the target displacement's own interpolation
  term, so residual and step displacements are charged, never assumed away.
- **r_trim(t)** = L_S·w̄(s) when t has an edge on chain segment s, else 0. w̄(s)
  is the segment's UV lens half-width (§11) and L_S bounds ‖DS‖ there. It covers
  the sliver where a UV chord leaves a concave trim: such points map onto the
  carrier extension, within L_S·w̄ of the edge curve [call; own derivation from
  G4 §3.1 and FT §1.4]. It is small (w̄ ≈ κ_uv·ℓ²/4) and costs one multiply.
- **r_coll(t)** = d_f when t uses a collapse survivor p* (§9.1); w/2 at a
  symmetric sliver chain, w when one neighbour absorbs it (§9.2).
- **δ_exp** is the key's export reserve (§18.1): the largest rounding move of
  any written frame of the key, computed from its construction frame bound
  R_max, never from one occurrence. The mesh is built to ε_f − δ_exp, so the
  written bytes meet ε (G3 §4.3).
- **ε_loc(t)** = min(ε_f, clearance budget, tilt budget) from §12.
- Every subtraction is checked positive before it is used: a budget at or below
  zero takes its stage's conservative branch (collapse reverts, §9.4; clearance
  refuses, §12.5; export recentres or refuses the format, §18.1), never a
  refinement towards a negative target.

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
  below 90° − ψ_f with the exact carrier normal at each of its corners (G13
  Rule A0; ψ_f the target tilt of §3.2, 0 outside displacement zones). A
  triangle that fails refines or the face refuses.

### 3.7 Embedding

- No two triangles of one definition intersect except along shared mesh edges
  and vertices. Construction guarantees it with clearance budgets (§12); no
  emitted triangle pair is tested in construction.
- Contacts and self-intersections present in the source are reproduced and
  reported as pairs. The offline oracle recomputes each reported pair's source
  clearance itself and exempts it only if that check confirms it (§24.1 row 5).
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
add `incomplete`. Reaching a cap never yields `valid`. Status is per (subject,
format) where a format imposes its own clause. §7 defines the lattice.

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
- Refusal is reserved for what cannot be meshed (§8.5 rung 5, the ingress
  catalog of §5.8).
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
serial loop over definitions; no barrier waves. Stages are dependency edges, and
every edge points forward: no task reads a task that reads it.

| Task | Per | Reads (the complete input bundle) | Coupling |
|---|---|---|---|
| T0 decode | source record | source bytes | none |
| T1 carrier | surface or curve | own record | none |
| T1b feature size | definition | T1 bounds of every entity above collapse scale; ε_ref | spatial (recorded region) |
| T2 measure | (edge, face occurrence) | curve record and range; that occurrence's pcurve and master; T1 of the face; ε_ref; the sampling request | incident face |
| T2o orientation | face | T2 UVs of its loops | face |
| T3 L1 solve | edge sample; full-rank vertex | T2 of every occurrence at that sample | incident faces |
| T4 L2 vertex | rank-deficient vertex | T3 end samples of incident edges | vertex star |
| T5 L3 edge | edge | T3; T3/T4 of both end vertices; one seeded projection per sample and occurrence (measured residual, g) | edge |
| T5′ deferred rungs | deferred item | T5; source trims of both faces; T1b lfs | faces + spatial |
| T6 collapse | collapse cluster | T5/T5′ of members; request (ε, r_c) | component |
| T7 clearance | face pair, edge, vertex, face self-proximity | T1 and bounds; T1b lfs and collapse candidates; T3 move classes and bounds μ̄ (§8.1, §12.1); request | spatial (recorded region) |
| T8 closure | edge | T5/T5′ of the edge; T2/T3 of every other edge of each incident face (source arcs and move bounds μ̄); T1 of incident faces; T6 slice; T7 slice; request | incident faces, one hop, through T2/T3 only |
| T9 face | face | T2o sense; ordered loop structure; T1; T8 chains in loop order; T6 slice; T7 slice; request ε_f, flags | edge-incident faces |
| T10 publish | definition | every T9 output in canonical key order; T2o and shell sense; identity binding | definition, D2 |
| T11 place | occurrence | T10; raw placement | D2 |
| T12 export | file | T11; format and parameters | D2 |

- **Readiness is mechanical.** Each task holds one pending counter over its
  declared reads above and runs when it reaches zero. There is no other
  readiness rule; in particular no two-hop counter (§4.5).
- **Why the graph is acyclic.** T8 reads its own edge's resolved arc and only
  T2/T3-level data of other features (source arcs, move bounds), never other
  chains; T7 reads T3 move bounds, never resolved positions; T9 never re-splits
  a chain (a budget it cannot meet is a refusal, §12.5). That keeps the edit
  halo finite (G12 §3.1).
- **Critical path:** T0 → T1 → T2 → T3 → T5 (→ T5′) → T8 → T9 → T10, with T1b
  and T7 beside T3–T5 and joining at T5′ and T8.
- **Remaining global barriers,** each named: whole-store ingress while the OCCT
  bridge is the reader (a definition's tasks start when its records are decoded
  once the owned reader lands, §5.6); T12, which waits for every T11 of its
  file. Graph construction is per definition, after its T1.
- **Refusal is terminal and releases dependents.** A refused entity's task
  completes with its refusal record; counters waiting on it decrement. An
  evaluable refused carrier still feeds its neighbours' L1 rows, closure arcs
  and clearance, so their bytes do not change; a non-evaluable one drops its
  rows with a record. Oracle 3 classifies the edges a refused face leaves
  single-used as `open_by_refusal`.
- T7 and T1b record their query regions for invalidation (§19.3).
- Each task's output is a pure function of its input bundle. No globals, clocks,
  environment, shared caches or reads of other tasks' partial results are
  reachable from task code (G7 §2.2 item 1, §5.3).
- **Finiteness.** Each task output of T2–T5′, T9 and T10 is checked finite
  before it is published; a non-finite value refuses that entity with
  `nonfinite_evaluation` naming the task and entity. Non-finite values are
  rejected before any min or max reduction (std::max is order-dependent on NaN).
  Every gate passes iff `value ≤ bound` (or `<`), so a NaN that escapes fails
  closed into Rule A's conservative branch (§7.1).

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

> For one placement function (every entity's `backend_id` fixed by input,
> configuration and device classes), output bytes are identical across thread
> counts, runs, launch configurations, batch compositions, GPU instances of one
> device class, cache state and incremental or fresh remeshing. Across backends,
> outputs are functionally equivalent (`.53`): each passes the validity stack on
> its own, and FI-1 and FI-2 hold (§24.2). One model may mix backends; every
> shared entity has exactly one producer per run. A load-reactive mixed run
> gives functionally equivalent output every time; replaying its recorded
> placement log (§20.4) reproduces its bytes. D3 is excluded.

**`backend_id`** = (device class, target architecture, digest of the exact host
binary and device fatbin, the arithmetic dispatch path taken at run time, and
the libm identity and IFUNC level of every libm function an output-affecting
kernel calls) (G7 §1.1). Thread count, SM count, GPU instance and NUMA node are
not in it; the kernels make them irrelevant.

**Transcendentals** come from libm, whose identity and IFUNC level are already in
`backend_id`. glibc swaps FMA-built variants on AVX2+FMA hosts; that changes the
`backend_id`, which is all D-determinism needs, and cross-backend cache reuse is
off by default (§19.1). Owned polynomial kernels return only if cross-host cache
sharing becomes a requirement.

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

Produced entities are fine enough that every GPU-eligible piece of work is its
own entity with its own producer: the per-occurrence measurement of an edge
(T2 runs per (edge, face occurrence)), and a face's lattice interior (lattice
vertices, cell classification and interior triangles, keyed by integer lattice
coordinates, rim vertices included; §13.4). The chain's producer reads the
occurrence measurements by key, and the band reads the lattice interior by key,
as faces read chains. The placement log records one `backend_id` per entity.

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
  alternative priors), missing-pcurve continuation and collapse clusters go to
  typed queues. New closure samples on a deferred edge are evaluated inside
  that edge's T8 by the edge's own resolution (§11.4), not queued again.
- **Background healer.** Drains the queues concurrently with the immediate pass.
  Items are placed by item, then batched; batch composition never changes an
  item's bytes (§4.2 items 2, 3).
- **Batch formation** (no stage-wide barrier). Each work class has a staging
  buffer that fills as items become ready. An idle CPU worker takes any
  non-empty buffer and runs it with masked tails. A GPU takes a buffer only when
  it holds at least that device's own break-even N* (§20.3). A healer item whose
  blocked downstream work (its upward rank, §20.1) exceeds the class's split
  threshold dispatches at once on a CPU without waiting for a batch. T2 forms
  batches per definition as each definition's T1 completes, never over all
  edges of the input. Deferred-item counts per class (`B-ED-CL`) are measured in
  M8 before batch sizes are tuned.
- **One hop, not two.** Closure of edge e in face F reads only T2/T3-level data
  of F's other edges: source arcs and move bounds (§11.1). It never waits for their
  resolution. (G2 §4 needed two hops because its closure read neighbours'
  resolved arcs.)
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
| Carrier | L2 | content digest; kind (`plane`, `cylinder`, `cone`, `sphere`, `torus`, `bspline`, `revolution`, `extrusion`); flags (rational, periodic u/v, R ≤ r, degenerate torus, `select_outer`, offset rewritten); frame (origin, three axes); radii; cone semi-angle; extrusion vector with raw magnitude; profile-curve reference for swept kinds; singular loci (poles, apex, interior pole lines v*). B-spline: degrees (p, q); knot multiplicities and the knot lines of multiplicity ≥ p (interior creases, §13.1); per Bézier span: domain (u₀, u₁, v₀, v₁), control-point offset, recentring centre, D_uu, D_uv, D_vv, M_S, σ̲, σ̄, normal cone (axis, half-angle; or `irregular` when its hull contains the origin), minimum weight, box. Bounds only: carriers carry nothing that depends on ε |
| Work estimate | D2, at graph build | per item integer counts (edge samples, lattice cells, band trims × layers) from carrier bounds and ε_f; summed only as integers |
| Edge measurement | T2 output, per (edge, face occurrence) | per sample: dyadic path (bits, depth), t, P, (u, v), signed d_f, n_f; per edge (joined at T3): θ, \|x\| where σ₂ ≥ σ_cut, the move class and its bound μ̄ (§12.1); flags (projection failed, non-monotone) |
| Chain | L5 | edge content digest; ε_ref; end roles (start, end vertex); per sample: dyadic path, t, resolved xyz, rung, measured residual; per face occurrence: (u, v) and g_f (a seam carries two occurrences); per segment: continuity class, closure depth; a reserved signed normal offset per sample and occurrence, default 0 (I9, §26.1); record references |
| Vertex | L4 | vertex content digest; ε_ref; resolved xyz; level (L1, L2); move from the stored point; rung |
| Face output | L6 | face digest; sense; disposition; status per format; δ_f; r_f; ℓ_f; face vertex block [model vertices, chain samples per coedge use in loop order, interior vertices], boundary entries naming (loop position, coedge, path), interior entries holding (u, v) and xyz; one exact normal per block entry (seam and knot-crease entries twice, one per side); the reserved signed normal offset per entry, default 0; triangles as block-local index triples; generator tag ranges (lattice, band, fan, strip) |
| Publication | L8 | unique positions by physical key; face table (triangle range, block range); block → global vertex map; normals per block entry; classification rows per coedge use; sparse tables (quotients, collapse survivors, second seam lifts); occurrence table (NAUO path, raw A and t, kind, key); the record tables of §7.3 |

Expected peak bytes, from these field lists (FP64 positions, u32 indices): about
12 B per triangle, 24 B position + 24 B normal + 16 B UV per block entry, and
per chain sample 8 B t + 24 B xyz + 24 B UV and offset per occurrence + 8 B path.
M8 measures peak RSS per triangle and per sample against this arithmetic (§20.5).

### 4.7 Loops and their bounds

Every loop has a bound computed from the geometry, with a count backstop, a stop
name and a status. Oracle 15 forces each row to fire (§24.1).

| Loop | Bound (count backstop) | Stop name | Status on reaching it |
|---|---|---|---|
| edge and closure splitting (§10.3, §11) | packing bound from L* (§11.2); arithmetic floor: midpoint not representable apart from the ends, or a UV chord below the CDT fixed-point separation | `closure_stop` with its case | refused (deviation is hard) |
| seeded projection (T2, T5) | 16 iterations [call] with a residual-decrease test; `projection_solver` statuses `ITERATION_LIMIT`, `STAGNATION`, `LINESEARCH_LIMIT`, `RANK_UNRESOLVED` each map to "projection failed" | `projection_failed` | the sample is deferred; its ladder continues |
| rung 1–2 Newton (§8.5) | 8 steps [call], each halving the residual, else stagnation (M2 names a geometry-derived bound but derives none) | `newton_limit`, `newton_stagnation` | next rung (copy-check-rollback) |
| rung ladder (§8.5) | rungs 0–5 once; rung 4 runs rungs 0–3 once per alternative representation and never recurses | rung 5 | refused |
| quadtree refinement (§13.1) | depth ≤ ⌈log₂(span width / fixed-point quantum)⌉ per axis | `lattice_floor` | refused |
| band refinement and A0 (§13.2–§13.3) | Steiner points ≤ 4A/(π·r_floor²) + boundary samples, A the band's scaled-chart area, r_floor = √(2·ε_loc,min) | `band_floor` | refused |
| Lawson flips and constraint recovery (`direct_cdt`) | n(n − 1)/2 flips for n sites; with robust predicates the bound never fires | `flip_limit` | refused; under the plain-double arm (F1) it is the non-termination backstop |
| A1 cap repair (§15.1) | two attempts per cap (move or delete the apex; one interior Steiner point) [call] | `cap_unrepaired` | `valid_goal_unmet` |
| clearance recursion E2 (§12.3) | depth ≤ ⌈log₂(piece extent / bound resolution)⌉ per side; the pair's work budget is that depth's node count | `clearance_unestablished` | tighten once to the computed budget, then refused (`.59`) |
| face work (§20.5) | 16× the face's own a-priori estimate [call; M8 measures the estimate's error] | `face_budget` | refused |
| run memory (§20.5) | per-run RSS and device-memory budgets | `run_budget` | exit 1, records written |
| NAUO path expansion (§16) | path count by dynamic programming over the product DAG before expanding, against the request's occurrence budget | `occurrence_budget` | refused at ingress (§5.8) |

### 4.8 Cancellation, deadlines and device faults

- **Cancellation** is cooperative and checked at task dequeue, never on every
  work charge (§25 removed that). A cancelled run stops dequeuing, lets running
  tasks finish, writes the records of everything that completed and exits 6
  (`cancelled`). Completed results are pure, so they may be cached and reused by
  key. A superseding request (sharpen-in-place, a per-frame remesh) cancels the
  old run's unstarted tasks. A deadline is cancellation at a time.
- **Fuzzer timeout** = 8× the input's integer work estimate, converted through
  the calibrated cost model [call]; reaching it is a failure (§23.5).
- **Device fault** (allocation, launch, ECC): the failed batch's items are
  re-placed on CPU and the device leaves the run, with a `device_fallback`
  record. The output is functionally equivalent (`.53`). Exit 1 only if the CPU
  path also fails.

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
| orientation from `source_uv` pcurves (OCCT classifier, Gauss integration) | — | Owned decision in the mesher: S1 per face, S2 per shell (below) | new path (T2o, T10) |
| refusals thrown as whole-file text, exit 1 | every refusal | Typed records per the catalog of §5.8; exit 4 for a file-level refusal | `.25.1` |

The `source_uv` reconstruction of missing pcurves (OCCT ShapeConstruct, 17-point
check) is not used by the new mesher, for geometry or for orientation. A face's
UV per sample comes from seeded projection in edge measurement (§8.1), with the
residual recorded per coedge (D-master). Today `source_uv` keeps only per-solid
maxima (`SourceUVStats`); the per-coedge residual is new (G1 §7).

**Orientation without `source_uv`.** Today's S1/S2 verdicts run OCCT's classifier
and Gauss integration on `source_uv`'s projected pcurves
(`native/step_source/main.cpp:666–675`, `:1070–1072`). The new path decides:

- **S1 per face** in T2o: the signed area of each loop's UV polygon from T2's
  per-sample UVs (an owned Green's sum, seams lifted) against the loop's
  declared role and the face's `same_sense`. A contradiction is corrected with a
  record; a sum smaller than its own rounding bound refuses the face with reason
  7.
- **S2 per shell** at T10: the signed volume of the shell's mesh, summed in FP64.
  A negative volume reverses the shell at publication (winding and normals),
  with a record; |V| below its rounding bound refuses the shell with reason 7.
- The adapter's own verdicts are kept as diagnostics and compared offline
  (oracle 8). Its two-region and two-pole fold and contact tests leave the new
  path.
- Budgets that read the material side (signed credit at material cusps, §12.3)
  read the declared sense. A shell that S2 reverses at T10 after such a budget
  was used refuses those faces with a record rather than re-running them; M8
  counts how often this happens.

**Bounding the bridge.** OCCT's read, transfer and remaining checks run in the
adapter process under CPU-time and address-space rlimits; exceeding either
refuses the file as `ingress_budget` (exit 4). (B2-ad measured single contact
tests at 0.37–24 s per fixture, and a mapped-item cycle overflowed the stack.)

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
| 28 | owned-operation result (cut, extrusion, revolution, fillet, offset, later Booleans) | — | — | admit through the native source record (below) | the operation's own representation is the declared master |

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

**The native source record** (row 28). Owned operations write their results into
the source store as native B-rep records, the same tables a Part 21 ingress
fills, so the mesher has one input path. The operation's representation is the
declared master of every curve and surface it writes. Exact rationals are
rounded once to binary64, with the rounding residual recorded per entity. The
operation declares its u_ctx (its own construction tolerance), which takes the
place of a representation context's uncertainty for UNC-FINE, pole snapping and
coincidence decisions. Content digests cover these records as they cover Part 21
records (§16.2), so an owned result has a mesh key. Oracle 8 carries fixtures
for each operation kind.

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
  cannot do it. The adapter that feeds the new path today takes 25.0 s on the
  watch (B2-ad), of which only the bridge's
  3.06 s read and transfer is split out (G1 §6); the rest (raw walk, placements,
  units, orientation, `source_uv`, store writing) has never been timed by phase.
- **Decision** [operator: the owned reader precedes the first headline]: the
  owned STEP reader (`.25.7`), with parallel record splitting, is on the critical
  path. No headline is published until owned ingress, from STEP bytes to a
  decoded definition, fits a budget no larger than NVIDIA's minimal ingress
  measured on the same input and lane (§22.3). Until then the bridge with raw
  overrides feeds development and the arena. The bridge stays as the evaluator
  cross-check and the reference exporter's path. (The 25.0 s above compares with
  about 17 s before the B2-reader changes.)
- **What the owned path keeps** from the adapter: the raw walk, placements,
  units and uncertainty per context, styles, identity side tables and the
  correction records. **What leaves the headline path:** `source_uv`, OCCT's
  classifier and contact tests (orientation moves to T2o/T10, §5.3), the OCCT
  transfer itself.
- **Streaming** [call]: the owned reader emits each definition's records as they
  are decoded, and that definition's T1 and graph start at once; no task waits
  for the whole store. With the bridge, the store is complete before the graph
  is built.
- **M8 measurement:** the adapter's phase split on the watch and the corpus
  (the phases above, each wall and CPU), so the owned reader's budget is set
  per phase before it is written.
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

### 5.8 Refusal catalog

Checks run in the order syntax → topology → consistency → geometry (Map S4), each
once per entity at ingress or at T1. Every row writes a typed record (§5.4) with
its reason code. An entity refusal refuses the faces that use it and marks the
part `incomplete`; a definition refusal marks the part `refused`; only a
file-level refusal stops the run (exit 4). Exit 1 is reserved for internal, I/O
and device faults (§7.2). Every refusal the adapter throws today maps to a row
(today all of them exit 1 with text: `native/step_source/main.cpp:1279–1280`,
`native/mesher/source.cpp:88`).

| Order | Check | Scope | Reason code | Today |
|---|---|---|---|---|
| syntax | non-finite scalar | entity | `nonfinite_value` | whole file (`main.cpp:217`, `source.cpp:88`) |
| syntax | malformed record, unresolved reference, wrong arity | entity | `malformed_record` | whole file |
| syntax | unit unresolvable, two units of one kind, missing angle unit where needed (UR-4) | context | `unit_unresolved` | per context (B1) |
| syntax | request: ε not finite or ≤ 0; relative ε over an unbounded or empty box | request | `invalid_request` | — |
| topology | translated shells or faces differ from the raw solid | definition | `translation_mismatch` | whole file (`main.cpp:665`) |
| topology | edge range empty after units (t₀ ≥ t₁, non-periodic) | edge | `empty_edge_range` | — |
| topology | two entities with identical content where one is required (duplicate faces, edges or vertices) | entity pair | `coincident_source_entities` | coordinator refuses today |
| topology | mapped-item or NAUO cycle | occurrence subtree | `placement_cycle` | per file (B2) |
| topology | occurrence path count, by dynamic programming over the product DAG, above the request's occurrence budget | file | `occurrence_budget` | — |
| consistency | raw knot vector differs from OCCT's edited one | entity | lifted: the owned evaluator reads the raw net | whole file (`main.cpp:341–380`) |
| consistency | periodic file pcurve with no raw match | coedge | lifted: treated as a missing pcurve (§8.1) | whole file (`main.cpp:955`) |
| geometry | radius ≤ 0 (cylinder, sphere, cone base, torus minor), torus R < 0 (`.69`) | face | `nonpositive_radius` | torus only |
| geometry | cone semi-angle outside (0, π/2) | face | `cone_angle_out_of_range` | — |
| geometry | zero-length or parallel frame axes | entity | `degenerate_frame` | — |
| geometry | extrusion with \|V\| = 0 | face | `zero_extrusion` | — |
| geometry | ellipse or hyperbola semi-axis ≤ 0, parabola focal length ≤ 0 | edge | `nonpositive_axis` | — |
| geometry | rational weight ≤ 0 (ISO 10303-42) | entity | `nonpositive_weight` | — |
| geometry | B-spline degree > 25 | entity | `degree_above_reader_limit` (G5 §5) | — |
| geometry | knots decreasing, multiplicity > degree + 1, net size inconsistent with knots and degree | entity | `invalid_knots` | — |
| geometry | a sub-span whose normal cone still contains the origin at the subdivision floor, at a point not declared singular (§13.1) | face | `undeclared_singularity`, until the limit-normal route of §15.1 covers it | `native/om10/math.hpp:151` returns false |
| budget | adapter CPU time or address space exceeded (§5.3) | file | `ingress_budget` | stack overflow guarded only for one recursion |

## 6. The request

**Stage** S2, S19 · **Evidence** Map S2; G14 §5; G15 §0, §4–§6; G17 §2–§4 ·
**Decisions** D-PMI, `.48`, `.49`, `.54`, `.39`.

### 6.1 Units and scale

- Absolute requests are in mm (or caller units, recorded). Relative requests
  (fractions of a diagonal) use the diagonal of the unexploded root world box
  from the sound `shape_extent` bounds. The relative request resolves **once per
  session** into an absolute ε, recorded in the request and in every key, and is
  never recomputed when an edit grows the box or a transform moves a part
  (explosion, motion), unless the caller asks [call]. Re-resolving it per input
  would couple every key to the whole model (§19.2). An unbounded or empty box
  refuses the request by name (§5.8).
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
| **Compare** | the NVIDIA arm | the owned side runs Fabricate (chord-only arm) or Render (25° arm) | both sides must achieve the request as measured by the oracles; NVIDIA's settings are tightened until its output does (§22.2) |
| **Simulate** (later) | PhysX | ε + L_max ≤ 500·`PxTolerancesScale::length`; welded; no zero-area triangles | contact density is §26.1 |

- **No grading anywhere.** Grading at FEM slopes costs about (4/√3)/g triangles per
  boundary sample (7.7 at g = 0.3; about 8× plane triangle counts), buys nothing
  the named consumers use, and spreads influence across faces, which breaks local
  invalidation under edits and sharpen-in-place (G14 §3 item 6, §4.3).
- **No minimum size as a stop.** A geometry-derived floor may exist only as a
  termination bound; reaching it yields `valid_goal_unmet` or `refused`.
- **The angle term on analytic carriers is closed form**: Δθ = min(Δθ_ε, θ_max)
  with Δθ_ε = 4·asin(√(ε/(2R))) for ε < 2R and π otherwise (the same value as
  2·acos(1 − ε/R) inside its domain, without its NaN beyond it or its
  cancellation as ε/R → 0). It binds only for R < ε/(1 − cos(θ_max/2)), 42.2ε at
  25° (0.21 mm at ε = 0.005 mm). Sphere: ρ ≤ min(ρ_ε, R·sin(θ_max/2)) with
  ρ_ε = √(ε(2R − ε)) for ε < R and R otherwise. Torus: h ≤ θ_max/κ_max with
  κ_max = max(1/r, |cos φ/(R_c + r cos φ)|).
- **On freeform carriers** the angle term uses the Bernstein normal cone of each
  sub-span (FT §1.5 names it as an owned candidate). The span's polynomial
  normal, S_u × S_v for a polynomial span and
  w·(H_u × H_v) − w_u·(H × H_v) − w_v·(H_u × H) for a rational one (parallel to
  S_u × S_v, H the homogeneous numerator), has Bernstein coefficients whose cone
  contains every normal of the sub-span, and the cone is hereditary under
  subdivision. Normal turning inside any triangle within the sub-span is at most
  the cone's full angle, so the lattice refines a sub-span until that angle is
  ≤ θ_max. The same coefficients give σ̲ ≥ min|N|/σ̄ (min|N| the distance from
  the origin to their hull), the first-derivative floor that §11.1 uses. A cone
  whose hull contains the origin (a pole or an irregular point) subdivides; one
  still containing it at the subdivision floor is a declared quotient if the
  carrier declares one there, else refused `undeclared_singularity` (§5.8).
  Near poles the formulation is 3D normal turning, never a parameter-space
  normal bound.

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
- Per toleranced PMI feature: m_t, c_t and the four warnings of §6.3. Mating,
  two-point and least-squares sizes need feature recognition and fitting, which
  is metrology beyond "warn and comply" (D-PMI); they are not computed.

## 7. Validation, records and status

**Stage** S4, S15 · **Evidence** G11 §3–§6; Map S4, S15; B2-in §4 · **Decisions**
D-partial-output, D-profile, `.38`, `.39`, `.40`.

### 7.1 Outcomes and status

- **Check outcome**, for every check that runs: `pass`, `fail`, or
  `not_evaluated` with a reason: `prerequisite_failed`, `budget`, `unsupported`,
  `no_threshold`, `nonfinite`.
- **Rule A (gating checks are conservative):** a check whose verdict selects a
  construction branch never leaves a decision open; `fail` and `not_evaluated`
  both take that gate's conservative branch, which is refusal for a hard clause
  and revert for a collapse (§9.4).
- **Hard clauses** (deviation, shared-edge topology, orientation, embedding) are
  carried by construction: nothing at run time evaluates them a second time, and
  no record claims they passed. Offline oracles check them (§24). Status
  therefore comes from **construction events**, never from per-clause pass bits.

| Event during the face's construction | Kind | Face status |
|---|---|---|
| a gating check failed or was not evaluated (rung 5, closure stop, clearance not established after tightening, face budget, non-finite value, ingress catalog row) | hard, gating | `refused`, with reason and finding |
| the face was built by a rule whose hard-clause bound this design does not state (an arena arm such as plain-double predicates or `local_pair_test`; an `unsupported` fallback generator) | hard, non-gating | `indeterminate` |
| a soft cap fired or a soft goal was not met (A1 cap unrepaired, a requested quality target, a soft termination bound) | soft | `valid_goal_unmet` |
| none of the above | — | `valid` |

The worst event decides. `indeterminate` does not occur in the default
configuration; it exists for arena arms and future fallbacks.

- **Per format.** A format that imposes its own clause (3MF `model` needs two
  uses per edge, §3.11; the 3MF pair refusal, §18.3; a chain-bounded needle,
  §18.1; `.38` for indeterminate faces) gives the subject a status per format.

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
- **Exit codes** (`.40`, confirmed default; 6 added by the operator at M6):

| Exit | Run status |
|---|---|
| 0 | every occurrence `valid` or `valid_goal_unmet` in every requested format |
| 2 | output written; some occurrence `indeterminate`; none incomplete |
| 3 | output written; `incomplete` in some requested format |
| 4 | nothing produced: input or request refused |
| 6 | cancelled; the records of what completed are written (§4.8) |
| 1 | tool failure: internal error, I/O, a device fault the CPU could not absorb, resource exhaustion of the run |

- The exit code is the worst over occurrences and requested formats, in the
  order 0 < 2 < 3 < 4 < 6 < 1. Records are written on every exit, 1 included;
  meshes are written only for 0, 2 and 3.
- **Indeterminate faces in exports** (`.38`): their triangles go to the internal
  schema and glTF only; the part counts as incomplete for STL and 3MF.

### 7.3 Native record tables

Owned SoA tables in the existing tagged-table binary container (eight-byte tag,
row count, byte size). Enums are `enum class : uint16_t`, append-only. Strings only
for external names, in one pool. No number inside text; human text is generated
from records at presentation time (G11 §4).

| Table | One row per | Fields |
|---|---|---|
| `RUN_____` | run | input SHA-256; tool build id; backend set and `backend_id`s; thread count; ε requests (value, unit, absolute or relative, the resolved absolute value); ε_ref per definition; R_max per key; units honoured; profile id and digest; the profile toggle (runtime set or full); every non-default parameter and arena flag; calibration file digest; cache directory state (empty, verified); start/end; OS; host class (never a hostname); STEP header part information |
| `CRITERIA` | profile row in force | criterion id; PDQ code or owned `B-…` code; measure variant; assessment kind; polarity; unit; seed and active value; extra parameters; provenance; origin (seed, tightened, loosened); action (report, warn, gate); evaluated (runtime, on request) |
| `MEASURE_` | inspected element, dense | per edge: class, max signed d_f, max \|x\|, min θ, owner face, sample count, method, per-end vertex move (the rung lives once, in the chain); per face: area, triangle count, δ_f, r_f |
| `FINDINGS` | (phase, criterion, subject[, second subject]) failed or not evaluated | key; phase; criterion; subjects; owner; outcome and reason; measured value; threshold copy; method; accuracy; one or two extreme `Location`s |
| `ACTIONS_` | thing done to make the mesh | key; kind; findings answered; entities; before (P1) and after (P2) values; maximum displacement; carrier chosen; surviving alternatives and the branch-rule step that decided |
| `STATUS__` | (face, definition, occurrence or run; format) | status; the deciding event and reason; disposition; triangle count; finding range; counts per status |
| `SUMMARY_` | (phase, criterion) | inspected, violating and not-evaluated counts; worst value and location; truncated flag |

- **Phases:** P0 raw entities; P1 as translated (pre-heal, on what the bridge hands
  over: the name says the bridge already edited it), measured by T2; P2 resolved;
  P3 mesh; P4 export (written bytes); P5 offline oracle. Re-running the source
  criteria at P2 ("before and after healing", VDA 4955 §3.1.6.1) is a P5 or
  on-request run (§7.4), not part of a default run.
- **Action kinds:** M2 rungs 1–5 (rung 0 is the default and is visible in the
  chain); vertex level L2; collapse (E′, S′, absorbed); missing-pcurve recompute
  with residual; ε tightening near a thin feature; tilt refinement at a knife
  edge; orientation flip (S1, S2); algorithm fallback; device fallback;
  refusal. Face- and body-level silent actions are recorded too.
- **Keys and order.** Finding key (phase, criterion, subject, second subject);
  action key (kind, primary entity). Sorted at the end of a run; ids are indices
  after sorting.
- **No merging and no cap** on records. If a budget ever caps findings,
  `SUMMARY_.truncated` is set and counts are labelled lower bounds.
- The edit-disposition record (§19.4) and D3 timings live in their own sections,
  outside every equality oracle.
- `brepgpu report` formats the tables at the boundary: text (default), TSV, JSON
  on request. Part 59 and Quality Stamp XML emission waits until a consumer asks
  for it. Nothing internal parses its own output.
- Record emission is inside the owned headline's timed scope (§22.3).

### 7.4 The profile `brepgpu-pdq-1`

Seeded from JAMA V5.1 and CAx-IF (D-profile). A row is (criterion, measure,
value, polarity, unit, provenance). Users may tighten; loosening is recorded and
the profile digest changes. The profile holds data-quality criteria only, not
the mesher's ε-derived gates (η, β, r_c, θ_cut), which are request parameters and
arena flags recorded in `RUN_____`.

**Toggle** [operator, M6]: by default a run evaluates only the rows marked
*runtime*, each a value the mesher already computes. The rows marked *on
request* (the self-intersection classes, the other whole-source checks and the
P2 re-run) run offline at P5 or when the caller asks for the full profile. No
construction step reads any profile row.

| Code | Criterion and its runtime measure | Seed | Evaluated | Action |
|---|---|---|---|---|
| G-SH-LG | gap between faces at an edge: max \|x\| over the edge's samples (T2) | 0.01 mm | runtime | report |
| G-FA-EG | edge–surface gap: max \|d_f\| per (edge, face) (T2) | 0.01 mm | runtime | report |
| G-FA-VG | vertex–surface gap: distance from the stored vertex point to each incident carrier (T3 rows) | 0.01 mm | runtime | report |
| — | vertex–edge gap: distance from the stored vertex point to the curve end C(t₀) or C(t₁) (T2) | 0.01 mm (own, by analogy) | runtime | report |
| G-LO-LG | adjacent-edge gap: distance between consecutive coedges' curve ends at a vertex (T2) | 0.01 mm | runtime | report |
| G-ED-TI / G-FA-TI | short edge: 3D arc-length upper bound (§10.3) / small face: meshed area (T9) | 0.01 mm / 1e-4 mm² | runtime | report |
| G-SH-NT | G1 discontinuity: θ per sample (§15.2) | smooth ≤ 0.01°, crease ≥ 0.2° | runtime | report |
| G-SH-NM / G-SH-FR / G-SH-OU | non-manifold edge / free edge / over-used vertex (3): use counts | logical | runtime | report; 3MF `model` gate; part open |
| G-CU-HD, G-SU-HD | degree above 5: record field | 5 | runtime | report only; the mesher accepts ≤ 25 |
| B-RQ-UN, B-RQ-PM | request against uncertainty / PMI (§6.3) | §6.3 | runtime | warn and comply |
| B-ED-CL | edge needed rungs 1–5: chain rung | logical | runtime | report |
| G-FA-NA | entirely narrow face | 0.01 mm | on request | report |
| G-SU-DC | nearly degenerate surface boundary | 0.01 mm | on request | report |
| G-SH-IS, G-LO-IS, G-FA-IS | self-intersection classes | 0.01 mm; α unset (`no_threshold`) | on request | report |
| B-VP-SV | sender volume/area; centroid, against exact properties of the geometry as read | 0.5%; 0.02 mm + 0.1% of diagonal above 20 mm | on request | warn and comply (`.39`) |
| A-TR-MA, A-TR-AS | min angle, aspect | none seeded | when requested | goal |

Values JAMA leaves to agreement are seeded `no_threshold`, not invented. The hard
clauses (deviation, watertightness, orientation, self-intersection) are not
profile rows: construction carries them and the offline oracles of §24 check
them. Mesh volume against exact volume is not a criterion: it follows from the
deviation bound (G11 §5.3) and would only re-check oracle 1.

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

T2 runs per (edge, face occurrence), each its own produced entity (§4.3),
batched per work class as each definition's T1 completes (§4.5), on CPU SIMD or
GPU (§20).

1. **Initial sampling** at the dyadic parameters of §10 (the chain's own curve
   rule plus every incident face's admissible length). If there is no 3D curve,
   the owner pcurve's lift supplies P.
2. At each sample t, evaluate P = C(t) from the prior representation (§8.2).
3. For the face occurrence f: if f's pcurve is the prior representation,
   u_f = p_f(t) and d_f = 0; else if f's file pcurve maps within η of P (through
   its measured parameter map where the edge is not SameParameter),
   u_f = p_f(t); otherwise seeded projection of P onto S_f (seed from p_f or the
   previous sample, monotone along the edge, lifted across the period, snapped to
   a pole within vertex uncertainty). Record the **signed** distance
   d_f = n_f·(P − S_f(u_f)), with n_f the unit normal at the foot oriented by
   face sense.
4. T3 joins the occurrences at each sample. With two faces: θ = ∠(n₁, n₂); where
   the pair is full rank (σ₂ ≥ σ_cut, §8.3) the minimum move onto both is
   |x| = √(d₁² + d₂² − 2d₁d₂cos θ)/sin θ with the signed d_i; below the cutoff,
   |x| is the truncated minimum-norm move of §8.3 and no full-rank value is
   recorded (the formula divides by sin θ → 0 at tangent edges).
5. Per edge: max |d_f|, max |x|, min θ, failed or non-monotone projections, and
   the edge's **move class**: `small` when every sample has |x| ≤ β and each end
   vertex's stored point lies within β of its incident edges' measured ends,
   with move bound μ̄_e = 2β (one β from the sample's L1 move, one from the L3
   vertex correction); otherwise `deferred`, with μ̄_e = ¼·lfs_e. T7 and T8 read
   μ̄ instead of resolved positions (§4.1). A resolved move that exceeds its
   edge's μ̄ refuses the faces that used the bound (`move_bound_exceeded`), never
   a re-run; rung verification already caps deferred moves at ¼ lfs, so this can
   fire only on a `small` edge, and M8 counts it.

Missing pcurves cost one seeded projection per sample; there is no special path.
This replaces the `source_uv` reconstruction for geometry and orientation (§5.3).

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

- One row per incident surface: n_i·x = −d_i, from stage-1 values (unit
  normals, signed distances; M2 §1.5).
- **Hard block first.** Plane rows are hard (eliminated, as planegcs does). The
  plane rows are reduced by a truncated SVD with the same angle cutoff, so
  coplanar split faces (common in exports) give one row and near-coplanar ones
  truncate instead of being refused (planegcs refuses a rank-deficient hard set,
  M2 L276). Overdetermined hard rows (four planes at a vertex) are solved by
  least squares; the solution stands if its residual is ≤ η, else the item goes
  to rung 3.
- **Conditioning is classified on unit rows.** After hard elimination, the
  remaining rows' unit normals, projected onto the hard null space, decide which
  directions are full rank: σ_j ≥ σ_cut, σ_cut set as an angle θ_cut [open;
  default 8.1°, sweep {3.6°, 8.1°, 10°}] (M2 L339–344 derives the cutoff for unit
  rows). Weights never enter this decision; a high analytic weight scales σ by
  √w and would pass near-tangent junctions as full rank.
- Weights by declared trust tier enter only the solution in the full-rank
  directions: other analytic a high finite weight; freeform 1. Ties within a
  tier by content digest (never by entity id).
- Truncation: solve direction j iff it is full rank or the move it needs,
  |u_jᵀb|/σ_j, is ≤ β; otherwise the prior stands in that direction.
- Linear step x = VΣ⁺Uᵀb. The predicted residual r_i ≈ κ_i(|x| + |d_i|)²/2 is
  triage only: an item whose prediction already exceeds η goes straight to
  rung 1, seeded at p₀ + x.
- Rung 0 commits on the **measured** residual after L3 (§8.4): every |r_i| ≤ η
  and the move ≤ β. Otherwise the item goes to rung 1.

For an R2-sized discrepancy (|d|, |x| ≤ β) the predicted residual is within η
whenever the curvature radius is ≥ 8ε_ref (0.04 mm at ε_ref = 0.005 mm). The
directions the surfaces cannot determine are, to first order, those in which the
error does not change, out to √(2η/Δκ) along a tangent slide (M2 §1.5). That is
why the final position comes "mostly free" from stage-1 data.

### 8.4 Three acyclic levels

| Level | Work | Reads |
|---|---|---|
| L1 | every edge sample; every full-rank vertex: §8.3 | T2 only |
| L2 | rank-deficient vertices: prior from L1-resolved ends, then §8.3 | L1 |
| L3 | each edge: end samples replaced by its vertices; each end's correction Δ₀, Δ₁ spread as (1 − s)Δ₀ + sΔ₁ and projected onto each sample's null space | L1, L2 |

- **s is the normalised curve parameter of the sample (or the source curve's arc
  length), never the polyline's arc length.** Inserting a sample then moves no
  other sample, which closure locality and incremental = fresh require (G2 §3).
- **Pointwise in t.** A sample's resolved position is a function of (edge
  content, t, ε_ref) only: L1 at t reads stage-1 values at t, L3's weights come
  from the curve parameter and its corrections from L1/L2 vertex results. A
  sample inserted later (closure, a finer LOD level) is one evaluation of the
  same function; it changes nothing else.
- **Measured residual.** After L3, one seeded projection per sample and face
  occurrence (closed form on analytic carriers) measures the resolved point's
  signed offset g_f(t) from each incident carrier (§3.2). L3's slide along a
  linearised tangent can add κβ²/2 off the surface, above η wherever the edge
  radius is below 8ε_ref, so the commit reads this measured value, never the
  prediction.
- No level reads its own outputs: no priority queue, no iteration.
- On a transverse edge the free subspace is the tangent, so L3 only
  reparameterises; on a tangent edge it carries the slide from the vertices.
- L3 replaces last-segment absorption (arena flag `l3_blend`, M2 H7).

### 8.5 The ladder per deferred item

| Rung | When | What | Verified by |
|---|---|---|---|
| 0 | measured residual ≤ η and move ≤ β | commit | measured residual (§8.4); chain monotone |
| 1 | residual > η, full rank | Newton seeded at the linear step, at most 8 halving steps (§4.7) | converged; measured residual ≤ η; chain monotone; UV loop simple; ends on resolved vertices |
| 2 (big) | transverse, move > β | Newton to the intersection at any distance up to ¼ lfs | as rung 1, plus no crossing of another trim of either face |
| 3 (small) | near-tangent (σ₂ < σ_cut, move > β), or rungs 1–2 failed | truncated weighted solve; slide from the prior; the residual gap sits on the lower-trust face as a recorded step; between carriers of one tier it is split symmetrically, half on each face (`tangent_split`, as §9.2) | chain monotone; step ≤ ¼ lfs |
| 4 | representations disagree by more than β | rungs 0–3 once from each alternative prior or owner, never recursing | as above |
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

No candidate passes step 1; the move or gap exceeds ¼ lfs; Newton fails its bound
and the small leap leaves a gap above ¼ lfs. Refusal is per item, with every face
using it refused and the part marked incomplete.

Two same-tier carriers (two near-coplanar planes, two analytic surfaces of one
tier) meeting near-tangentially with a gap above η are not refused: they take the
symmetric recorded step of rung 3, reported in each face's b_f. That follows the
adopted M2 proposal (small tangent gaps mesh as recorded steps, refused only
above ¼ lfs) and §3.11 (refusal only for what cannot be meshed). Refusing them is
the arena arm `equal_tier_tangent`; M8 counts the class on the corpus.

### 8.8 Feature size

lfs(p) at a point p of an edge or vertex x is the radius of the smallest ball
centred at p that meets an entity disjoint from the closure of x (sharing no
vertex with it), over entities above collapse scale. This is the PSLG-style
feature size G2 uses for UV (G2 L363–369, L410–411), lifted to 3D: medial-axis
lfs is zero at every sharp edge, exactly where it is used, and would empty
rung 2's range and refuse every gap above ε/4. lfs is floored at ε_ref, so a
sub-ε sliver cannot shrink it and then refuse leaps beside it (G10 §7). T1b
computes a lower bound per edge, lfs_e, by a box-tree query over T1 bounds of the
entities disjoint from each edge's closure, and records its query regions
(§19.3). Collapse candidates are found on source positions with margin r_c plus
the edge's measured discrepancy.

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
2. The merged vertex p* goes to the multi-surface point (V2: the §8.3 point
   solve over every surface incident to the cluster) if the surfaces are
   pairwise transverse and the collapsed geometry stays within r_c of it; else
   the smallest-enclosing-ball centre (V3), with d_f ≤ r_K.
3. Triangles using p* in face f are sized to e(t) ≤ ε_loc − d_f (§3.4). The last
   segment of a surviving edge ending at K carries the same reserve. If
   ε_loc − d_f ≤ 0 on any incident face (a clearance budget beside the cluster
   can put ε_loc below r_c), the cluster reverts to faithful meshing (§9.4).

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
6. A0 orientation at p*;
7. the survivor reserve ε_loc − d_f > 0 on every incident face.

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
order-free. Closed-form uniform counts (the circle's n = ⌈2π/Δθ_ε⌉, §6.2) become
"split until each segment's sag ≤ budget", costing up to 2× samples on those
edges (about 1.4× on average), paid for LOD nesting, which is in-process display
only for alpha (`.41`). The arena arm `closed_form_counts` (§23.3) measures it:
dyadic; counts rounded up to base·2^j with an odd base fixed per edge from the
finest ε (nesting kept on the 4^k ladder; LOD paper §5.2, §8); plain closed form.
Reported per surface-kind class.

**Ties at round CAD numbers.** Real files put R = ε and R = 2ε on exact algebraic
values; 16 corpus cylinder decisions computed 3 − 1 ulp where the exact value is 3
(G7 §6). Either side meets ε, but a 1-ulp libm difference changes a count; that is
why chains have one producer. The arena arm `biased_split` tests "split if
sag > budget·(1 − 2⁻⁴⁰)", the dyadic form of ceil(x(1 + 2⁻⁴⁰)) (G7 H5).

### 10.3 Sample tests

A segment s passes when **all** of these hold:

1. **Curve rule** (sag of the chord against the 3D curve):
   - line: 0;
   - circle radius R over angle Δ: 2R·sin²(Δ/4) (= R(1 − cos(Δ/2)) without the
     cancellation);
   - ellipse, hyperbola, parabola: κ_max·ℓ²/8, with ℓ the segment's arc length
     or an upper bound on it (the chord understates it, by about 7% on a 60°
     arc), κ_max = a/b² (ellipse, a ≥ b; hyperbola at its vertex) and 1/(2f)
     (parabola);
   - polynomial B-spline span of degree p, sub-interval fraction h of the span:
     ⅛h²·p(p − 1)·max‖Δ²P‖ (span-local, after Bézier extraction);
   - rational span: Zheng–Sederberg Theorem 4, an a-priori parameter-step rule
     for rational curves, used here for what it states (§13.1 does not use it
     for surfaces): with the span's homogeneous control points (w_iP_i, w_i) recentred
     on the span's box centre c, r = max‖P_i − c‖, and parameter step h,
     h²·D ≤ 8·ε_b·min w with D = max‖Δ²(wP)‖ + (r − ε_b)·max|Δ²w| (FT §1.2,
     L148). Not Cheng 1992, Piegl–Richard or adjacent-weight ratios;
   - no endpoint-Hermite proxy (SM §3 row 3).
2. **Face rule** for every incident face occurrence: closure predicate P4 (§11),
   the segment's UV arc length in the face's **scaled chart** is ≤ 2√(2ε_b) with
   ε_b = ε_loc − δ_exp − r_coll − r_trim(s) − r_disp, the ledger's residual
   budget (§3.4), checked positive first. In the scaled chart a plane and a cylinder's ruling
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
occurrence, evaluated on sound bounds. Closure runs once per edge, on the edge's
settled resolution, and the chain is frozen after it. Every mature mesher
re-splits shared edges on face trouble and stops on a count (OCCT 5, Gmsh 10 + 3,
CGAL 29, SmTess 100); this one needs no count, and no later stage re-splits a
chain.

### 11.1 Predicates on (segment s, face occurrence F)

Notation: ℓ̄(s) an upper bound on the UV arc length; τ̄(s) an upper bound on the
total UV tangent variation; ℓ₃ the 3D arc length; D̲(s) a lower bound on the UV
distance from arc(s) to every feature of F other than e's own arc and the parts
inside s's corner balls. D̲ is computed from the other features' **source** arcs
and reduced by each feature's UV move margin μ̄_g/σ̲ (its T3 move bound, §8.1),
so their resolution, which closure does not wait for, cannot close the gap.

- **P1 flatness:** τ̄(s) ≤ τ₀ (π/3).
- **P2 separation:** w̄(s) = ½ℓ̄(s)·tan(τ̄(s)/2) < ½D̲(s). w̄ bounds the lens
  half-width (Pav–Walkington Lemma 1).
- **P3 corner:** at each endpoint vertex v with UV corner angle α_v > 0, inside
  the corner ball B_v (largest R_v with τ₁(R_v) + τ₂(R_v) < α_v/4),
  w̄(s) < dist(s, v)·sin(α_v/4), and τ̄(s) < α_v/4 for the segment touching v. A
  segment crossing ∂B_v splits until inside or outside.
- **P4 band:** the scaled-chart UV arc length is ≤ 2√(2ε_b) (§10.3 item 2).

**The bounds are sound and order-free** (fixed-order quadrature, the earlier
choice, was order-free but could miss turning between nodes; G2 L614–616):

- **On a file pcurve** that is master or agrees: τ̄ from the angular spread of
  the hodograph's Bernstein coefficients over s's parameter interval (for a
  rational pcurve, of the homogeneous hodograph numerator w·p′ − w′·p), which is
  hereditary under subdivision; ℓ̄ from the sub-segment's control-polygon
  length; closed forms on lines, circles and conics.
- **On a projected occurrence** (no pcurve, the normal case: §5.1), the arc is
  the foot-point curve of the resolved edge on S_F. With σ̲, σ̄ and M_S of the
  face's spans over the segment's UV box (§13.1) and κ̄_C the 3D curve's
  curvature bound from its own span data (§10.3): ℓ̄ = ℓ̄₃/σ̲ and τ̄ = κ̄_uv·ℓ̄
  with κ̄_uv = (σ̄²·κ̄_C + M_S)/σ̲. (From C = S∘u: C″ = DS·u″ + D²S[u′, u′], so
  |u″| ≤ (|C″| + M_S|u′|²)/σ̲ and |u′| ≥ |C′|/σ̄.) The resolved edge lies within
  μ̄ of S_F; σ̲ is taken over the μ̄-offset band, which covers the foot-point map.
- Bounds combine by max and min only.

**Lemma 1 (sufficiency).** If every leaf passes P1–P3 in F, F's polygonal loops are
simple, nest exactly as the curved loops do, and no curve point of another loop
lies in any chord's cap. P4 lets every boundary triangle meet ε_b with an apex
about ℓ₃/2 away.

**Cusps (α_v = 0).** Two arcs with a common tangent and curvatures κ₁ ≠ κ₂ separate
as |Δκ|r²/2 at distance r from v. When |Δκ| is above its floor (8× the bound on
κ's evaluation error [call]), P3 is waived at v, and (G2 §2.3, L525–541):

- the innermost segment, which touches v, is exempt from cusp splitting: its
  chords already meet only at v;
- every later segment at distance r ≥ ℓ_first from v passes P2 with
  D ≈ |Δκ|r²/2, so its length is bounded by L*(r) = r·√(2|Δκ|/κ̄), a geometric
  grading of at most ⌈log(ℓ_edge/ℓ_first)/log(1 + √(2|Δκ|/κ̄))⌉ + 1 segments.

If |Δκ| is at or below the floor, the face refuses "trim curves tangent-coincident
at vertex". This is the lane's own derivation, tested on family F4.

### 11.2 Stop rule and bounds

Each predicate holds once ℓ̄(s) ≤ L*(s):

  L*(s) = min( τ₀/κ̄, √(2·D̲/κ̄), α_v/(4κ̄) inside corner balls with α_v > 0,
  the cusp grading L*(r) at cusp vertices, P4's length )

with κ̄ the UV curvature bound on the arc and M̄ the scaled-chart metric bound
P4 reads (√(D_uu + D_uv), √(D_vv + D_uv) over the segment's UV box, §13.1). A
segment splits only when it fails, so no arc shorter than (1 − q)·L*_min is
created, q = ρ/(1 + ρ), ρ the parameter speed ratio. Samples per occurrence ≤
∫ds/((1 − q)L*(s)) + 1; depth ≤ ⌈log_{1/q}(ℓ_edge/L*_min)⌉ + 1. No round count.

**Stop without passing** (each a refusal record):

1. arithmetic: the midpoint is not representable apart from the ends, or a UV
   chord is shorter than the CDT's fixed-point separation. The fixed-point floor
   is the binding limit; near t = 0 subnormal midpoints would allow about 1,075
   levels, so dyadic path storage covers the floor's depth;
2. a premise broke: D̲ at or below the arc-separation floor (the source trim
   self-intersects or loops touch), κ̄ or M̄ unbounded (σ̲ = 0: a span whose normal
   cone contains the origin, §13.1), α_v = 0 with |Δκ| at or below its floor;
3. depth exceeds the packing bound from the measured L* (a bound was wrong; the
   record names it);
4. the predicted count exceeds the request's budget: refused before refinement
   (whole request or the face's own estimate, §20.5).

### 11.3 Determinism and cost

- The split set is N = {n : every proper ancestor of n fails some predicate in
  some occurrence}: a pointwise definition, identical for every order, schedule,
  thread count and occurrence permutation. Requests merge by maximum depth per
  node, a commutative idempotent join.
- **Purity:** D̲(s) comes from source arcs through each feature's own dyadic lens
  hierarchy and the features' T3 move bounds, never from other edges' current
  chords. The first level is a box tree over lens boxes (OCCT FaceChecker's
  cost).
- Per new sample: one evaluation of the edge's resolution at t (§8.4: L1 solve,
  L3 blend, measured residual, the edge's rung where its gates require one), one
  UV per occurrence, the children's predicates. Nothing else.

### 11.4 Interactions

- Closure runs once per edge, after the edge's resolution has settled (T5 and
  any T5′ items). A new closure sample is one evaluation of x_e at its t by the
  edge's own rule (§8.4), run inline in T8. A new sample whose evaluation fails
  its rung's verification, or whose move exceeds the edge's μ̄ (the margin other
  edges' closure assumed), refuses the edge's faces with a record. Closure never
  re-runs.
- The lfs in P2 is each face's UV trim lfs, not the 3D lfs of §8.8.
- Lattice clusters and band separators are built after closure; C4-aligned
  edges force their lattice level (§13.4).
- No residual detector runs after triangulation. The predicates read sound
  bounds; the offline oracles 2 (exact UV) and 5 (self-intersection) catch a
  wrong bound, and G2 H2's counts come from oracle 2 on the corpus and families.
- **Arm B** [arena, `boundary_closure`]: OCCT-style crossing-plus-nesting checks
  and splits in synchronous rounds per definition, requests of round k merged
  before k + 1, splits limited by the same floors, no count. Asynchronous rounds
  are order-dependent and not allowed.

### 11.5 Refusal record

Face id, edge id, occurrence (sense, seam side), definition and occurrence; segment
key, t interval, UV and 3D ends; predicate and stop case; ℓ̄, ℓ₃, τ̄, w̄, D̲(s)
with the feature that bounded it, α_v, |Δκ|, κ̄, M̄, L*, the floor, predicted and
observed depth; the witness point. The face is `refused`, the part `incomplete`;
the chain stays valid for the other faces. Where the lfs query cannot resolve
above the floor, Rule A refuses the face (stop case 2), never `indeterminate`.

## 12. Embedding: clearance budgets in construction

**Stage** S21 · **Evidence** G4 §0, §3–§7; G3 §1, §4.3; Map X16 · **Decisions**
D-features, `.59`, `.60`.

Each emitted point stays strictly on its own face's side of the medial surface
between faces. Construction guarantees it with three budgets computed from source
geometry, the T3 move bounds and the triangle being built. No emitted triangle
pair is tested in construction. The full self-intersection check stays an
offline oracle.

### 12.1 Quantities

- **Localization** ε̂_f = sup over f's triangles of the distance to f's own trimmed
  target face: ≤ ε_loc, concave-trim slivers included, since r_trim is in the
  ledger (§3.4).
- **Displacement** μ_f(Z) for a clearance zone Z: the supremum of |φ_f| (§3.2)
  over Z's footprint on f, bounded from the T3 move bounds μ̄ of the edges whose
  decay band (within ℓ_f) meets Z, plus r_c for each collapse candidate T1b found
  in Z; zero outside every decay band and candidate. Clearance is computed on source geometry minus
  μ_f(Z) + μ_g(Z). A face-wide maximum would let one rung-2 move anywhere on a
  face tighten every clearance on it.
- **Clearance** c(q) = min over other faces g of dist(q, F_g); same-face
  self-proximity uses remote pieces of F_f.
- **Wedge** at a shared edge e: θ_s, the normal angle signed by
  (n_f × n_g)·t_e with t_e along f's coedge, so convex and concave edges differ.
  The two polar openings in the normal plane are Δψ_in(0) = π − θ_s (the
  dihedral: π at a tangent edge) and Δψ_out(0) = 2π − Δψ_in(0), with
  Δψ(ρ) ≥ Δψ(0) − ½(κ_f + κ_g)ρ per opening (the curvature correction applies to
  the opening, never to θ). Δψ_min(e) is the smaller opening's minimum over
  ρ ∈ [0, r*_e]. With s(Δψ) = sin Δψ for Δψ ≤ π/2 and 1 above (a wider opening
  is never harder; G4 L405), r*_e = 2(ε̂ + μ)/s(Δψ_min). A tangent edge has
  Δψ_min ≈ π, s = 1 and no wedge hazard (G4 L557).
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
  (C1) every star triangle of each face satisfies
  α(t, v) + τ_v ≤ ½·s(Δψ_min) − m_α; (C2) ρ₀ = 2s_e/(s(Δψ_min) − 2 max α) is
  below both faces' first-row depth and the segment lengths; (C3) every non-star
  triangle in the zone satisfies Lemma A. Straight edges are checked
  numerically; curved and doubly-curved edges are a proof sketch tested by
  families K4–K5.
- **D (vertices).** Faces sharing only a vertex are separated by a plane through v
  if the tangent-cone gap exceeds α_f + α_g + m_α.

### 12.3 The mechanism

- **E0 constants:** ε̂_f per face; μ per zone from T3 move bounds and T1b's
  collapse candidates (§12.1).
- **E1 candidates:** (a) face pairs whose boxes inflated by ε̂ + μ overlap and that
  share no edge or vertex (the existing CPU/CUDA AABB pair-discovery path); (b)
  every shared edge; (c) every vertex with three or more faces, or faces sharing
  only it; (d) per face, non-neighbour span pairs whose inflated 3D boxes
  overlap (self-proximity). UV separation of loops is closure's job alone (P2,
  P3; §11.1).
- **E2 clearance lower bounds:**
  - closed forms for analytic pairs: parallel planes, coaxial cylinders and
    cones, sphere–plane, sphere–sphere, cylinder–plane with the axis parallel to
    the plane;
  - generic analytic bounds for every other analytic pair (non-coaxial
    cylinders, cone–cylinder, torus pairs, a torus as a tube of radius r about
    its centre circle): the distance between the pieces' axis segments or centre
    curves minus their radii, then recursive subdivision of each piece's
    parameter box with closed-form patch boxes per kind;
  - B-spline pieces subdivide with the Bézier convex hull;
  - recursion stops when the bound clears ε̂ + μ, the pieces are small enough to
    set a local budget, or the depth bound of §4.7 is reached ("could not
    establish", never "clear");
  - for edges, Δψ_min from the measured signed θ minus curvature terms.
- **E3 budgets** join the ε ledger (§3.4) as ε_loc:
  - non-adjacent and self-proximity: ε_loc < ½(c − μ_f(Z) − μ_g(Z))·(1 − m_c)
    on both faces over the zone; planes need nothing;
  - at shared edges: the C1 tilt limit per star triangle, then a budget growing
    as ½ρ·s(Δψ) up to ε_loc at ρ = r*_e. For a cylinder strip along a ruling the
    star budget is R(1 − cos(½·s(Δψ_min))) ≈ R·s(Δψ_min)²/8;
  - at vertices: Lemma D against the tangent-cone gap;
  - for analytic strips the budgets are closed forms; for bands they are checked
    per triangle in the same pass as A0.
- **When budgets are known.** T7 computes every budget from source geometry and
  the T3 move bounds, before closure. Closure's P4 reads the resulting ε_loc
  (§10.3), so every boundary row can meet its budget without a chain split, and
  T9 never re-splits a chain (§4.1).
- **Margins** [call]: m_c = 0.1, the G4 probe's factor 0.9; m_α = 10× the bound on
  θ's evaluation error, so the sign of the C1 budget is decided correctly (G4
  L394 asks only m > 0). One symbol for both, as the draft had it, would subtract
  0.1 rad from ½·s(Δψ_min) and refuse every K1 crescent below about 11.5°.
- **Budgets are per face, with no credit taken from the neighbour.** Tightening one
  face can create the knife-edge failure: refining the concave face of the K1
  crescent raised failures from 2/24 to 11/24 in one sweep (G4 §3.4).
- **Signed credit** (A3, signed C1) is on by default only at **material cusps**,
  edges where the faces are tangent with material between them, classified at
  T7 from the signed θ and the curvature terms. There no finite unsigned budget
  exists (G4 L555), so without credit every such edge's faces would refuse.
  Elsewhere signed credit is the arena flag `signed_clearance`, default off.

### 12.4 Thin walls and gaps

A wall or gap of width w > ε is a feature that must not be dropped. It is meshed
with ε_loc < w/2 on both faces, never refused (D-features). Planes need no change.
A gap narrower than the source's stated uncertainty is meshed the same way (`.60`).

### 12.5 Outcomes

- **Clearance at or below the floor** c_floor: the faces touch or intersect in the
  source. Mesh faithfully, report the pair, exempt it in oracle 5 once the
  oracle's own source clearance confirms it (§24.1 row 5). c_floor =
  2δ_exp + the clearance bound's own resolution [call; G4 leaves the floor
  unvalued]; its value is settled by that source-side measurement, never by how
  many pairs oracle 5 then passes.
- **A budget that cannot be met, or clearance not established within its work
  budget:** tighten local ε once to the computed budget (interior refinement
  only; chains are never re-split); if that fails, the affected faces are
  `refused` with a record, never `valid` (`.59`). A floating-point triangle-pair
  test confined to that region is an arena flag (`local_pair_test`), default off.
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
- T7 is the largest unmeasured cost. The old regime's contact stage, a
  whole-component self-intersection check rather than T7, took 164.9 s (19.5%)
  at 64–67 µs per candidate pair over 2.2 M pairs (Audit §0.3). **M8 measurement,
  before T7 is built:** E1 candidate counts per class and closed-form coverage on
  the watch and the corpus, from the existing AABB pair-discovery path. The
  per-pair cost target is in §22.6; G4 H4 (under 2% of meshing time, under 5% of
  triangles) is the arena measurement.

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
| plane | none: constrained triangulation of the chain polygon in the plane's chart; many-hole faces split into components at separators as bands do (§13.2) | zero deviation against the carrier; r_disp only where g ≠ 0 (§3.2); otherwise only profile terms (L_max) add points |
| cylinder, cone | θ-slab constraint lines on the face's dyadic angular lattice; no samples along rulings | Δθ ≤ Δθ_ε(ε_b, R) of §6.2 (cone: largest radius in the slab; apex a pole); untrimmed faces are strips of 2n triangles; any re-triangulation inside one slab keeps the bound |
| extrusion | slab lines in u only | ruled: S_uv = S_vv = 0; D_uu = max‖λ''‖ |
| sphere | dyadic lattice in (u, v) with pole fans, plus band | circumradius ρ ≤ ρ_ε(ε_b, R) of §6.2, rechecked on every new triangle (not monotone under containment) |
| torus (ring, horn, spindle), revolution, B-spline (polynomial, rational, any degree), elementary offsets after rewrite | 2:1-balanced dyadic quadtree over UV sized by the anisotropic Waldron rule; interior leaves become conforming quads; hanging sides by the packet zipper | D bounds below |

**Second-derivative bounds** (scaled chart: u ↦ u√(D_uu + D_uv),
v ↦ v√(D_vv + D_uv); a triangle is within e if ½r_min'² ≤ e). A triangle's D is
the maximum over every Bézier span its UV hull meets; the face's chart for one
band CDT uses the maximum over the face's spans, and each triangle's own test
uses its local D.

- torus: ‖S_uu‖ ≤ R_c + r, ‖S_φφ‖ = r, ‖S_uφ‖ ≤ r; spindle tori add the interior
  pole lines;
- revolution: D_uu = ρ_max, D_uv = max‖λ'_⊥‖, D_vv = max‖λ''‖ (profile rules);
- polynomial span (p, q): D_uu = p(p − 1)max‖Δ²_uP‖, D_uv = pq·max‖Δ_uΔ_vP‖,
  D_vv = q(q − 1)max‖Δ²_vP‖, recentred on the span's box centre;
- rational span, by the quotient rule on recentred homogeneous data
  H_c = w·(S − c), c the span's box centre and r = max‖P_i − c‖ (≥ ‖S − c‖ by
  the convex hull, since w > 0): per direction d ∈ {uu, uv, vv},
  D_d ≤ (‖∂²_d H_c‖ + 2·‖∂w‖·L_S + r·‖∂²_d w‖)/min w with
  L_S ≤ (‖DH_c‖ + r·‖Dw‖)/min w, every norm bounded by the Bernstein differences
  of the homogeneous control points with their degree factors. (Zheng–Sederberg
  Theorem 4 is a parameter-step rule for curves; using it inside the
  arbitrary-triangle Waldron rule would be an unproved extension.) The bound is
  falsified on the high-weight-variation family of §23.5;
- general offsets (when built): M̂ ≤ D_ww + |d|(max‖D²n‖/min‖n‖ + 3max‖Dn‖²/min‖n‖²).

The same span pass writes M_S, σ̲, σ̄ and the Bernstein normal cone of each span
(§6.2; σ̲ ≥ min|N|/σ̄), which closure (§11.1) and the freeform angle term read.

**Knot lines.** A knot line of multiplicity ≥ p (C⁰ inside the face) is an
interior crease: a constraint line in the lattice and the band, with two normals
per vertex on it (one per side, as a face-internal seam), classified as an edge
class of §15.2. At multiplicity p − 1 the second derivative jumps but stays
bounded, and the maximum over spans covers it.

Size by r_min, never by edge midpoints: midpoint and centroid checks understate
by up to 4/3 (FT §1.3).

### 13.2 The trim band

- Lattice cells crossed by the trim are deleted. The band between the trim chain
  and the lattice rim is triangulated by constrained Delaunay with `direct_cdt`,
  fed fixed-point integers taken straight from doubles in the carrier's scaled
  chart (never Rational sites).
- **Loop nesting comes from topology and closure**, not from a pairwise check:
  the loops' roles are read from the source, and Lemma 1 (§11.1) makes the
  polygonal loops simple and nested as the curved loops are. `direct_cdt`'s
  pairwise hole-containment validation (12.95 s of a 13.18 s call on 8,104 sites
  in separated square holes; DIRECT_CDT.md L70) is dropped on this path; where a
  containment check is still wanted it is an O(n log n) sweep. This is a plan
  requirement, not a builder's choice (§29).
- Each connected band component (about one per hole) is an independent
  triangulation. Long components split at constrained separators from a
  lattice-front vertex to an **existing** trim sample (PC C2). Many-hole planar
  faces split the same way.
- Extra constraint lines: knot creases (§13.1), and the reflex-corner bisectors
  of §3.2 where the target displacement would otherwise jump inside a triangle.
- Band refinement inserts points only in the interior, never on a chain. Chains
  change only through closure (§11).
- Every band triangle meets the ε ledger (§3.4), the clearance and tilt budgets
  (§12.3), A0 and C1 of §18.1 (altitude > 2δ_exp); violators refine.
- Metric CDT generation of the whole face is the arena alternative
  (`face_generator`).
- No "clip anyway", no collinear-ear path, no n-gon output.

### 13.3 Orientation and termination

- UV orientation and in-circle decisions use the predicates of §21.3.
- A0 (§3.6) on every new triangle, with exact corner normals already evaluated.
  A fold valid in UV but reversed in 3D refines or refuses. This removes NVIDIA's
  inside-out fillet class and the 1808 duplicate-triangle class by construction
  (SM §5).
- Quadtree depth, band refinement and flips are bounded by §4.7. Reaching a bound
  is `valid_goal_unmet` (soft clause) or `refused` (hard clause), never `valid`.

### 13.4 Splitting large faces and placing their parts

Agree first on a shared lattice; never grow then weld (PC §0).

- **The lattice interior is its own produced entity** (§4.3): lattice vertices,
  cell classification and interior triangles, keyed by integer lattice
  coordinates with the rim vertices included. The band reads the rim by key, as
  faces read chains. So lattice and cell work can run on a GPU while the band
  runs on a CPU, each entity with one producer and one `backend_id`. Cell
  classification is conservative on every device: a cell stays in the lattice
  only if it lies inside the trim with a margin larger than the classifier's
  own error bound; an ambiguous cell goes to the band. A wrong classification
  can only move work into the band, and oracle 2 checks the result.
- **C1** lattice clusters: weighted Hilbert ranges of coarse quadtree blocks;
  seam vertices keyed by integer lattice coordinates; one owner per key; a
  read-only ghost layer of neighbour levels. One backend per produced entity.
- **C2** band components and separators (above).
- **C4** iso-line edges sampled on the lattice's dyadic levels where the edge lies
  on an iso-line; the adjacent lattice refines to that level.
- **Split rule** (G8 §5): split face f when e_f + w_f > τ_split = 0.1·W/(P − 1)
  (W the lane's estimated work, P its workers); never below g_min ≈ 100·o
  (o ≈ 1 µs per-task overhead, so ≈ 0.1 ms) or 10× the seam cost. τ_split
  converts integer work units to seconds through the calibrated cost model
  (§20.3), or through G8's defaults when the class is uncalibrated.
- A face's keyed triangle set is identical for every cluster count and
  assignment (oracle 14).
- **M8 measurements before C1/C2 ship in the default:** the per-face predicted
  work distribution and the share of freeform faces with an empty band, on the
  corpus strata, the families and the watch separately. If no face exceeds
  τ_split in the batch workloads, C1/C2 serve the single-part latency case and
  the GPU (PC §6).

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
- Switching level k to k − 1 moves a surface point by at most 1.25·ε_k, because
  every level meshes the one target of §3.2 (ε_ref ≤ ε ≤ ε_k) and lies within
  its own ε of it (LOD paper §5.2).
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
units after D-units), σ_max ≥ σ_min its singular values from AᵀA by the
deviatoric (Smith) form of the symmetric eigenproblem, which stays accurate at a
near-triple eigenvalue (the cubic's coefficients err by about u^(1/3) ≈ 5e-6,
above τ), κ = σ_max/σ_min. τ = 1e-6 [call; G9 H5 checks no real placement falls
in (1 + 1e-9, 1 + 1e-3)].

| Kind | Test | Admitted | Key | STL | 3MF (one build item per occurrence) | glTF (one node per occurrence) | Normals |
|---|---|---|---|---|---|---|---|
| rigid | det > 0, κ ≤ 1 + τ, \|σ − 1\| ≤ τ | yes | (definition, ε_def = ε/(1 + τ), request lengths, versions): one canonical value for every rigid occurrence | x′ = Ax + t | item → key object, [A\|t] | node TRS | normalise(A n) |
| mirrored | det < 0, else rigid | yes, same key | unchanged | x′ = Ax + t, **reverse each triangle** | default: item → the key's opposite-hand object (reflected once by a fixed M, re-wound) with transform A·M (det > 0); spec path (det < 0 item) behind a flag | negative-determinant TRS; no re-wind (consumer reverses) | normalise(A n), never the cofactor |
| uniform scale | κ ≤ 1 + τ, \|σ − 1\| > τ | yes | scale class σ_k: σ_max rounded to the lattice of relative step τ; ε_def = ε/(σ_k(1 + τ)) and every length request ÷ σ_k; angles unchanged | as rigid; reverse iff det < 0 | that scale's object, [A\|t] | uniform-scale TRS | normalise(A n) |
| non-uniform, shear | κ > 1 + τ | refused per occurrence with record | — | — | — | — | — |
| singular, non-finite | σ_min ≤ τ, or non-finite | refused per occurrence | — | — | — | — | — |

- Placements are classified on the raw STEP placement, never OCCT's location
  (OCCT reads mirrors as rotations, inverts scale and defaults an omitted axis3
  wrongly; corpus frequency 0, read raw anyway; B2-ad `.37.20`).
- **Why a canonical ε_def.** Rigid placements have σ_max = 1 within a few ulps,
  different per occurrence. Keying on ε/σ_max would split every rigid instance
  into its own mesh; sharing one mesh built at ε/σ_max of one occurrence would
  let another's world deviation exceed ε by up to (1 + τ). ε/(1 + τ) is the
  largest value safe for every rigid occurrence, and the same for all of them.
- One mesh per distinct scale value (`.57`). Scales are distinct when they differ
  by more than the placement tolerance τ, the same tolerance that defines
  "rigid" [call]; grouping scales further apart is the arena flag `scale_buckets`
  (G9 H2 with r = 2^(1/8)).
- Today one rigid predicate (`native/rigid_placement.hpp`, Gram within 1e-9) serves
  readers and display; M8 widens it to this table (`.25.4`). The display's cofactor
  normal transform (`native/om10/math.hpp:177-178`) yields inward normals under
  det < 0 and must change when mirrors are admitted.

### 16.2 The mesh key and ownership

- Key = (definition content identity: the Merkle digest of the definition's
  source records by content, raw IEEE bits, with no `#id`, file digest, name or
  UUID in it; effective request in definition units: ε_def, every length request
  ÷ σ_k, the per-face map ε_f, ε_ref, the R_max class of §18.1; ingress
  interpretation: thresholds, master and units policy; producer identity: mesher
  version, flags, `backend_id`). `#id`, the file digest and UUIDs live in the
  identity layer and the §19.4 correspondence only (§4.4), so renumbering or
  renaming hits every key, a channel-F edit misses only what it changed, and an
  owned operation's result (§5.4 row 28) has a key like any other definition.
- **The per-face minimum runs within one scale class** (corrects G17's formula):
  ε_f = min over that class's occurrences and their declared contacts of
  ε_contact/(σ_k(1 + τ)). Running it over all occurrences would collapse "mesh
  per scale" into "mesh at the tightest scale".
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
| triangle → face | range per face | internal schema records each face's range in the written STL | core triangle set, one `refrange` per face | `_FACEID` FLOAT per vertex, exact to 2²⁴; a definition with more faces is written as several primitives of at most 2²⁴ faces each, with the face-id offset per primitive in `extras` |
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

- The frames the file will use: the item frame for glTF, the **world frame for
  3MF** (PrusaSlicer bakes a single-instance foreign item into world float32),
  the source frame for STL.
- **One reserve per key.** One key serves many occurrences at different world
  positions, so the reserve never comes from one occurrence. δ_exp(key) is the
  largest, over the requested formats, of:
  - glTF: δ(R_def), R_def the definition's own extent (item-frame values);
  - STL: δ(R_max);
  - 3MF as a baking consumer holds it: σ_max·δ(R_def) + δ(R_max) + the transform
    parse term, since the item-local float32 values are rounded again by the bake
    (G3 §2.1 L197–198, §4.4 L738).
- **R_max** is a request field in the key: by default the binade (power of two)
  of the largest world-coordinate magnitude of any occurrence of the key in the
  unexploded root box. δ is constant within a binade, so keys stay shared;
  explosion, motion and other transforms never change R_max, so they stay
  remesh-free (§19.5).
- **At export,** an occurrence whose written coordinates leave R_max (an exploded
  view, a far placement) is recentred with a record where the format allows it
  (STL, `.74`), written where its frame keeps it inside R_max (glTF's item frame
  always does), and otherwise refused for that format with a record. It is never
  rebuilt silently.
- **A reserve too large to build to.** If δ_exp > ε_f/2 [call], the key's STL is
  recentred and any other affected format refused for that key before meshing,
  with a record. (At +100 m, δ ≈ 6.8 µm exceeds ε = 5 µm.)
- Three local conditions keep every FP64 guarantee through plain rounding, each
  with the key's δ_exp:
  - **C1** every triangle's altitude onto its longest edge > 2δ_exp. This is a T9
    predicate on every emitted triangle (there is no separate quality pass):
    a lattice or band triangle that fails refines in the interior. A needle all of
    whose vertices are chain or model vertices (two chains meeting at a sharp
    corner) has no interior remedy; it refuses, for that part, each format whose
    δ makes it degenerate, with a record (status per format, §7.1);
  - **C2** every non-adjacent or vertex-adjacent pair more than 2δ_exp apart (the
    clearance budgets of §12 add 2δ_exp);
  - **C3** no knife-edge fold within 2δ_exp/h⊥ (a first-order rotation bound).
  No element that met C1–C3 failed in any of G3's five placements and five
  emulations.
- Deviation grows by at most δ_exp by convexity; the mesh is built to
  ε_f − δ_exp (§3.4).
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
record (part incomplete for 3MF). Directed rounding away from the partner is not
built unless M8 finds such refusals on the corpus (it counts them). Contacts are
reported (§18.4).

### 18.4 Contact fit report

Per declared or detected inter-occurrence contact pair: clearance after faceting
(c − δ_hole − d_consumer) and any facet-induced penetration. Inscribed faceting of
a nominal pin and hole guarantees FP64 interpenetration up to ε; it is reported,
never silently accepted (G17 I8). This is a report, computed offline; it has no
fail condition and is not a construction guarantee.

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
- `request` = every parameter the task reads: ε in definition units, ε_ref, η,
  β, r_c, θ_cut, the ¼-lfs factor, R_max, angle and length limits, profile
  thresholds the task reads, export format, every arena flag it reads.
  Reconciliation (T1b, T3–T5′) reads ε_ref and not ε, so its keys hit across
  request ε, LOD levels and scale classes; a resolved sample is found again by
  (edge, dyadic path, ε_ref).
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
- Storage: `key → output digest` and `output digest → bytes`, native binary
  (the tagged-table container); an evicted digest is a miss.
- **Hits are verified.** A hit re-hashes the bytes it serves; a mismatch is a
  miss with a record, never an accept. Dependencies enter keys by output digest,
  so an unverified torn entry would propagate into every downstream key.
  Entries are written to a temporary file and renamed into place, so concurrent
  writers (two lanes, whole-host mode) never expose a partial entry.
- **Cost.** Content digests of source records are computed once at T0 and serve
  tie-breaks and identity; keys and output digests are computed only when the
  store is on. Hashing reads its input in place (today's owned SHA-256 copies its
  whole input first, `native/step_source/main.cpp:244`). Ivy Bridge has no SHA
  extensions, so digest time is a line of the stage budget (§22.6).
- **The persistent store is opt-in** (sessions and in-process hosts turn it on).
  The cold headline runs with it off and an empty cache directory, verified and
  recorded (§22.3); writing the store is measured separately and happens after
  publication, off the critical path.
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
  functional equivalence, FI-1 and FI-2 (§24.2).
- **Forbidden global couplings** (each would make the halo the model): run-global
  integers in D1 outputs; tie-breaks by entity number; an output-size guard that
  picks which faces to refuse; a global placement optimizer for shared entities;
  a relative ε re-resolved per input (§6.1); an export reserve taken from one
  occurrence's position (§18.1); a target that depends on the request (§3.2).
- Sub-face (band component) reuse is an option only if single-part latency shows
  large faces dominating (G12 §4.2).

### 19.3 Spatial queries record their regions

A spatial query records that no feature lies within r of something. Each records
its query region (inflated box or ball) in its trace. A box-tree query of changed
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
  and exploded views are remesh-free, with an occurrence that leaves R_max
  recentred or refused for that format at export (§18.1). A uniform scale change
  changes the scale class and so the keys. A flexing hairspring re-meshes its
  faces each frame; display shows provisional meshes in between.
- Sharpen-in-place and LOD steps reuse every reconciliation result, since the
  target does not depend on ε (§3.2); a step below ε_ref re-resolves it, as a
  recorded miss.

## 20. Placement, scheduling and memory

**Stage** S18 · **Evidence** G8 §2–§6; G7 §4; G12 §5.3; PC C3 · **Decisions**
`.53`, `.67`, `.68`.

### 20.1 Lane mode (the shipped configuration that must win)

One socket, its memory node and its local GPU.

1. **Tasks and estimates.** Each task has a class (edge-measurement batch, edge
   resolution batch, lattice-interior range, band component, healer item,
   publication, GPU batch of stage s) and integer a-priori work units (samples,
   cells, band trims × layers). Priority is HEFT's upward rank, in this shallow
   graph "face work remaining after this task".
2. **CPU workers,** pinned one per physical core of the lane. CPU-only: 12
   workers. GPU on: 11 workers, and the GPU driver thread pinned to the twelfth
   core, with CUDA's blocking host synchronisation so it sleeps rather than spins
   [call]. The GPU-on configuration therefore gives up one core, and arena flag
   F8's effect includes that cost by design; the arm `gpu_driver_core` (driver
   on the SMT sibling of a worker core, 12 workers) measures the alternative.
   Per-worker deques seeded in LPT order of (priority, estimate); owners pop the
   highest-priority local task; thieves steal the oldest, largest task.
   Over-decompose so no task exceeds τ_split. Never more workers than cores.
3. **Device allocation**, HeteroPrio at batch granularity: each eligible
   stage-class batch gets α = t_cpu/t_gpu from the cost model; the GPU pulls from
   the high-α end, CPUs from the low-α end; classes below break-even never go to
   the GPU; an idle GPU may spoliate a running or queued CPU batch it can finish
   sooner, the CPU result dropped and never merged.
4. **GPU-eligible classes** are decided per class and device by the cost model,
   never by a fixed list of surface kinds: B-spline and rational span sizing;
   lattice-interior entities (cell classification, lattice evaluation, emission
   and normals, §13.4); seeded projection onto freeform carriers (per-occurrence
   T2 entities, §4.3); analytic lattice evaluation and emission, which G8 expects
   never to pay over PCIe (hypothesis G3, tested in the arena rather than written
   into dispatch); display. **Excluded:** bands, CDT and topology, because no
   device CDT exists and building one is outside this design.
5. **Bytes that cross the bus.** β_s counts only bytes that must cross given
   where the consumer is. A mesh whose consumer is GPU-resident (render,
   explosion, LOD swaps, sharpen-in-place, physics on the roadmap) pays no
   download.
6. **Device residency.** Per definition, after its T1, the span tables (control
   nets, D bounds, M_S, σ̲, normal cones) upload once and stay resident; trim
   polygons upload once per face for cell classification. A batch sends
   (span id u32, u, v f64) = 20 B per item and receives fixed-size results
   (position and normal 48 B; with first derivatives 96 B), plus one descriptor
   per batch; that arithmetic is β per class. The per-lane device-memory plan
   holds the resident tables, the trim polygons and two staging buffers within
   16 GB; a definition's tables are evicted after its last T9, least recently
   used first when over budget, never in the middle of its batches. (The old
   regime uploaded about 2.7 KB per surface point and 766 B per curve point,
   Audit §4 item 7; 1M jets took 6.1 ms resident against 73–85 ms host to host,
   G8 §4.1.)
7. **GPU driver thread:** double-buffered pinned staging on the GPU's node,
   allocated under the lane's bound memory policy; context creation starts at
   process start, overlapped with ingress; the GPU joins the pool when ready.
8. **Memory.** Each worker owns scratch slabs allocated once on the lane's node,
   bound with `mbind(MPOL_BIND)` and `madvise(MADV_HUGEPAGE)`, and bump-reset per
   task: no system call per task (G8 §6: arena sets per node, scratch from the
   worker's node; 4 KiB pages cost TLB misses inside the 125–128 ns local
   latency, G8 §2.3). Task outputs go into per-definition chunk lists drawn from
   the worker's slab pool, compacted at T10 by the exclusive scan, so tasks of
   unknown output size (closure grows chains) need neither a barrier nor a
   shared allocator. Read-mostly source and carrier data are bound too. First
   touch alone is not a placement under AutoNUMA (2 GiB migrated in about 4.3 s,
   read bandwidth falling to 1.9 GB/s meanwhile). Slab sizes come from the M8
   per-task peak measurement against §4.6's arithmetic (the old regime reserved
   256 MiB per thread, 7.2–8.4 GB against a 2 GB RSS; Audit §4 item 8).

### 20.2 Whole-host mode

Socket pre-partition of definitions (or face groups of huge definitions) by LPT on
estimates, each socket weighted by its own calibrated capacity (the two sockets'
GPUs differ: x8 and x16); each socket runs the lane rule with its own GPU; a
socket with empty deques steals from the other only tasks ≥ 10× the cross-socket
steal overhead (about 1 µs); outputs go to the thief's chunk lists; read-mostly
source data replicated per node (about 0.4 ms per 10 MB). Compared only with
NVIDIA's two-socket arm (§22.2).

### 20.3 Cost model and fallback

- CPU: t_cpu(κ) = a_κ + b_κ·u. GPU: t_gpu = t0_d + u·max(g, β_s/B_d), with t0_d the
  16–18 µs round trip plus about 3.6 µs per extra kernel and B_d each device's own
  pinned bandwidth. GPU0 and GPU1 are separate devices with separate models
  (`.68`). Each device's break-even batch size N* comes from its own model.
- **Calibration is an explicit pre-pass.** A calibration run, its own process on
  the lane, fits the models by least squares per (class, device, host class,
  binary digest) and writes one file per device in the tagged-table container.
  Measured runs read it read-only and record its digest in `RUN_____`; no
  measured run writes it, so the processes of a series are independent and two
  lanes never write one file. A rebuild needs a new calibration pass before it is
  measured; G8's microbenchmark defaults serve only outside measurement.
- On read, every coefficient must be finite and positive; otherwise the defaults
  apply, with a record.
- **Fallback to CPU-only priority work stealing, GPU off,** when a class's model is
  uncalibrated, its median measured/predicted ratio leaves [0.5, 2], the context is
  not ready, or the predicted saving is below the remaining context time. Logged per
  run.

### 20.4 Mixed backends

- Placement is per produced entity, then batch (§4.3, §4.5). Load-reactive
  placement (HeteroPrio with spoliation) is the default; its output is
  functionally equivalent run to run (`.53`). Fixed placement from a fixed cost
  model is an arena arm for edit locality [call].
- The placement log (entity key → `backend_id`, tagged-table container) is a D2
  record of the run; replaying it reproduces the run's bytes (§24.2).
- Cache lookup tries each `backend_id` of the inventory in a fixed order; the first
  hit fixes that task's placement. Misses are placed by load, preferring the backend
  that produced the task's previous version (sticky placement), so early cutoff
  survives.
- The one-producer rule holds either way (§4.3).

### 20.5 Budgets

- Per-worker slabs (§20.1); no run-wide atomic metering of allocations; a per-run
  RSS and device-memory budget; exhaustion of the run is a tool failure (exit 1)
  with every record written and caller and device state recovered.
- A face whose work exceeds 16× its **own** a-priori estimate refuses with
  `budget` (§4.7; never `valid`). A request whose predicted output exceeds the run
  budget is refused as a whole before refinement (exit 4). No guard picks faces
  across the run (§19.2).
- Device faults fall back to CPU (§4.8).

### 20.6 The GPU is a hypothesis

At NVIDIA's 2.5 ms per face, per-face offload buys nothing. Bicubic jets break even
near N* ≈ 830–1,900 items per batch on GPU1; GPU0's N* comes from its own model and
is never set beside GPU1's (`.68`). At watch scale vertex evaluation is about
0.07 CPU-s even if every vertex were a B-spline jet, against NVIDIA's 22.9 CPU-s,
so sizing, subdivision tests, trims and triangulation dominate. Hypotheses (G8
§4.3): G0 the GPU arm equals CPU-only within CI, stated per report class (analytic
only, mixed, B-spline heavy) and per lane, never on the watch alone; G1 lattice
evaluation beats 11 cores above N*; G2 span sizing profits when per-span CPU cost
exceeds about 600 ns on GPU1; G3 every analytic class has α < 1 once consumer
residency is counted; G4 visible cold cost is max(0, context − ingress); G5 FP32
is not needed for speed on the P100.

## 21. Numerics and batch layout

**Stage** S8, S9, S18 · **Evidence** FT §1, §6; G7 §1.1, §6; journal ·
**Decisions** FMA released; predicates in the arena; SIMD where appropriate.

### 21.1 One batch layout for SIMD lanes and warps

- **The lane axis is stated per kernel**, because the fleet's Ivy Bridge AVX has
  no gather (VGATHERDPD is AVX2) and only 128-bit integer SIMD:
  - sizing and flatness kernels (span D bounds, M_S, normal cones): lanes over
    spans, SoA across spans (x[], y[], z[], w[] of the same control-point index
    of 4 spans side by side), branch-free max-norm reductions (FT §6.2);
  - evaluation kernels (lattice points, projections, residuals): lanes over
    points of one span, its control points broadcast, jobs sorted by span so a
    batch walks one control net at a time;
  - GPU: one block per span, its net in shared memory, threads over points.
- Flat SoA job arrays (span id, u, v) sorted by carrier kind, degree class and
  span.
- Degree classes: p, q ≤ 3 specialised; higher degrees bucketed by degree (4–5,
  6–8, 9–12, 13–25), each padded only to its bucket's top degree, so a degree-4
  span is not padded to 25 (that would cost about (26/5)² ≈ 27× its work).
  Control data recentred on the span's box centre.
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
arena against the robust one [operator]. No device runs an orientation or
in-circle predicate today (bands, CDT and topology stay on CPU, §20.1); device
cell classification is conservative by construction (§13.4). A device predicate
test joins the stack with the first arm that runs one on a device.

### 21.4 Precision

FP64 by default. FP32 with recentred coordinates is an arena arm: Metal has no
double, and memory-bound kernels might gain; the P100's half-rate FP64 (15–17
sockets' worth) means FP32 is not needed for speed there (G8 G5).

## 22. Measurement against NVIDIA

**Stage** S18 · **Evidence** G8 §7; G9 §0.2; G1 §5; G14 §0; SM §3–§4; B2-ha ·
**Decisions** steel-man NVIDIA [operator]; matched means achieved and NVIDIA as
shipped [operator, M6]; D-measure; `.27`, `.48`, `.56`, `.61`, `.67`, `.68`, `.70`.

### 22.1 Principles [operator]

- NVIDIA is measured as its shipped binary, at its strongest arm. Publishable
  numbers start there. Rebuilding it from source with its compiled-out options
  (its TBB loops under `SM_USE_TBB`, its scalable allocator under
  `SM_USE_TBB_SCALABLE`; SM §4) is a **named exclusion**, stated with every
  result.
- An invalid NVIDIA result is never our win; it is a finding.
- Our failures are never scored as NVIDIA's. A check of NVIDIA output has three
  outcomes, and "our check could not decide" is never NVIDIA's fail (§22.5;
  harness exits 3, 4, 5).
- Every timing states its lane, its competing load, its delivered clock and its
  GPU link state (§22.4).
- Transfer-bearing figures from GPU0 (x8) and GPU1 (x16) are never set side by
  side (`.68`).

### 22.2 NVIDIA's arms

| Axis | Levels |
|---|---|
| Ingress | OCCT default STEP read (FixShape), the candidate strongest; OCCT translation without ShapeProcess (labelled, never called the steel-man); our projected pcurves in the `.brep` (labelled "our projected pcurves", never the headline, `.70`) |
| Preparation | none; documented plane-range and periodic preparations labelled "requires our preparation" |
| Heal policy | 0, 1 |
| Workers | per lane: 12, 24 single-threaded processes; two-socket (for whole-host mode only): 24, 48, half pinned per socket |
| Worker placement | pinned to physical cores of the lane's socket, memory bound; floating |
| Assignment | static LPT on NVIDIA's own measured per-definition `tessellate` times from a calibration pass (never face count); dynamic pull from a queue sorted by those times |
| Start | after the whole ingress; streamed (each worker takes each `.brep` as it lands) |
| Angle | chord-only (0/0 angles); SDK default 25°/25° plus the chord |
| Chord | the key's ε_def, tightened until the output achieves ε (below) |
| Path | per-definition `SmApiTessellate`; the USD `tbb::parallel_for` path as an extra arm if it produces valid output |
| Build | the shipped binary only (source rebuilds are the named exclusion) |

Until the streamed and dynamic-pull arms exist, NVIDIA's ideal-overlap bound,
max(ingress, makespan), is reported beside its measured ingress + makespan,
because the owned graph overlaps decode with meshing and context creation with
ingress (§4.1, §20.1).

- **Validity** is topology and finite points: every edge with two opposite
  distinct uses, shells disjoint with correct signs, Euler over shells, finite
  points (B2-ex). It is the same admission as owned output. NVIDIA's mesh of the
  correct `void-block.step` fails it in all 8 configurations tried; that is
  recorded as a finding.
- **Achieved fidelity** [operator, M6: "matched" means achieved]. An arm achieves
  the request when its measured output passes oracle 1 (deviation against the
  source, NVIDIA's input discrepancy d_e allowed, §24.1) and, for the angle arm,
  oracle 16 (normal and curve turning). NVIDIA's arm for the headline is its
  fastest valid arm whose measured output achieves ε and the angle term, with its
  chord tightened as needed. An NVIDIA arm that misses ε is not our win; it is
  re-run tighter: next chord = chord·max(½, 0.9·ε/δ_measured) [call], each step
  its own recorded arm. (SM §3 shows NVIDIA's chord estimate understating
  deviation: sphere witnesses 0.01 → 0.0145.) The owned side achieves by
  construction and is measured by the same oracles.
- **Headline** (`.48`, with the M6 answer): the matched-fidelity arm, both sides
  achieving ε plus the 25° angle term (owned in the Render profile, §6.2); the
  chord-only arm, both achieving ε, beside it. Every ratio carries NVIDIA's
  measured δ, its final chord setting and both sides' triangle counts.
- NVIDIA gets exactly the key set: one definition-frame `.brep` per mesh key with
  the key's chord, angles unchanged (G9 §0.2). Both NVIDIA assembly paths
  tessellate per occurrence, so this is its strongest form.
- **Screening protocol.** The arms multiply (about 64 per angle arm, before chord
  steps). Per stratum, a seeded calibration subset of inputs runs every arm at
  n = 1; the best k = 3 arms per input [call] then run at full n in fresh
  processes, and NVIDIA's time is the best of those by median. Choosing by
  minimum over noisy screens biases the result toward NVIDIA; that bias is the
  steel-man's, and it is stated with every result.
- **Harness.** Before any corpus speed claim the NVIDIA harness takes arbitrary
  inventories and per-key chords. Today it hard-codes the watch's 107/9,162/166
  inventory and a 0.005 mm chord (`scripts/benchmark_om10_nvidia.py:258, 324,
  418–420`), and its finite mode makes geometry quality a validity condition
  (`:87–99`), which this section replaces with validity plus achieved fidelity.

### 22.3 Scopes and the headline

- **NVIDIA headline** = minimal ingress process wall (STEP read and transfer, one
  `.brep` per solid, fork to exit) + NVIDIA-only worker makespan (process start,
  context, conversion span, import and heal, tessellation, teardown, exit).
- **Owned headline**, by the symmetry rule, runs fork to exit: process start,
  CUDA context creation, ingress from the same STEP bytes timed the same way,
  per-key meshing and record emission (§7.3), teardown and exit. Its meshing ends
  when every key's L6 and L7 output is resident in memory. It excludes admission,
  scene construction, emit, display and the persistent store write (§19.1),
  exactly as NVIDIA's excludes our work (`.25.9`).
- **One declared configuration.** The owned headline is one configuration (flag
  set, binary digest, calibration file digest), the same for every input of a
  stratum, recorded before the headline series. It is never selected per input.
  Defaults adopted from arena data are confirmed in fresh processes on held-out
  inputs where the stratum is large enough (§23.6).
- **Ingress gate** [operator, M6]: no headline is published until owned ingress
  fits a budget no larger than NVIDIA's minimal ingress on the same input and
  lane (§5.6).
- Listed beside, never added: our preparation, physical admission, emit, display,
  views, context capture.
- **Stage times.** NVIDIA's stages are its process spans. The owned graph has no
  stage boundaries, so its stage cost is CPU-seconds per task class (§22.6). The
  meshing-stage ratio uses a separate owned meshing-only run on already decoded
  source (T1 onward) against NVIDIA's tessellation span; it is reported beside
  the headline, never as it.
- **Cold** = a fresh process: CUDA context, module load, decode, an empty cache
  directory verified and recorded per row, the store off, the calibration file
  read-only, the STEP file's pages dropped with `posix_fadvise(POSIX_FADV_DONTNEED)`
  before each ingress (owned and NVIDIA alike), library residency recorded. A STEP
  file or `.brep` intermediate on tmpfs is refused for cold runs, since
  `posix_fadvise` does nothing there (B2-ha §8).
- **Warm.** Owned: a second query in the same process after a full drain, with
  the retained caches named; mesh-output cache hits are excluded from warm
  meshing ratios. NVIDIA: a second query in the same long-lived workers, ingress
  done. Warm compares only with warm, is reported beside cold, and never
  replaces it.
- Accounting per mesh key (D-measure): Σ keys mesh + Σ occurrences emit. Per-scale
  meshes are listed and charged as extra key work.

### 22.4 Protocol

1. ≥ 10 fresh processes per (input, configuration, lane) for headline numbers:
   median, the rank interval of the median with its actual coverage (ranks 2–9
   at n = 10 cover about 97.9%), min and p90. Arena runs follow §23.6.
   Microbenchmarks of deterministic kernels report the minimum.
2. **One schedule per lane.** Owned and NVIDIA headline runs form one seeded
   schedule per lane, alternating owned and NVIDIA within blocks, under one gate
   and one other-lane state, so host drift lands on both sides. Configurations
   are interleaved in seeded random order, the environment padded by a random
   0–4 KiB per repetition; binary digests recorded.
3. Each run is its own process under `numactl --cpunodebind=N --membind=N` with
   `CUDA_VISIBLE_DEVICES` set to the lane's GPU; never two arena runs on one lane at
   once.
4. **Quiet gate, per lane.** Foreign CPU on the lane's CPUs and their SMT
   siblings, from per-CPU `/proc/stat` over the minute before and the whole run:
   at most 5% of one CPU per lane CPU [call; M8 measures the idle noise floor
   first]. No foreign GPU process on the lane's GPU, sampled during the run.
   Host-wide load average and foreign CPU are recorded as covariates, never as a
   gate. The other lane's activity (idle, owned, NVIDIA, foreign) is a recorded
   column and a blocking factor; an owned/NVIDIA headline pair shares one
   other-lane state. Headlines come from quiet runs; loaded runs are labelled and
   never mixed into a quiet summary.
5. **Context row, inside the timed interval.** Per lane CPU: cycles and wall
   across the interval (APERF/MPERF or `perf_event` cycles), giving the delivered
   clock during the run; the dependent-add probe before and after stays as a
   cross-check (never `scaling_cur_freq`). Per GPU, sampled during the run: SM
   clock, throttle reasons, current PCIe generation and width. Also: lane (CPU set,
   node, GPU), load average, foreign CPU (unthresholded and above 5%), our own
   tree, governor, SMT, AutoNUMA, THP, free memory per node, PSI, GPU use and
   process counts, cold or warm, the filesystem type of the STEP file and of
   NVIDIA's `.brep` intermediates. Governor left at `schedutil` (`.27`); AutoNUMA
   left on, neutralised by binding, state recorded (`.67`).
6. **Timers:** `CLOCK_MONOTONIC` wall and `CLOCK_PROCESS_CPUTIME_ID` CPU per stage
   (CPU/wall gives parallelism per stage); CUDA events for device spans; timer
   overhead below 5% of the interval.
7. **Both GPUs, both lanes.** Every GPU test and arena configuration runs on both
   lanes. NVIDIA is measured on each lane too. Each owned lane's ratio is paired
   with the same lane's NVIDIA result; both are reported and neither is picked.
   GPU-on effects are estimated per lane with the lane as a block, never pooled.
   The reporting tool refuses any table or estimate that puts transfer-bearing
   figures recorded at different link widths together (`.68`). Whole-run times
   are compared across lanes only for CPU-only configurations [call].
8. Hardware fairness: the owned lane (socket + GPU) against NVIDIA on the same
   socket; NVIDIA has no GPU path, which is its limitation; CPU-only owned results
   reported beside.

### 22.5 Scoring per input

A check of NVIDIA's output returns `pass` (valid, and achieving where the arm
claims it), `fail` (a completed check found a fault in NVIDIA's mesh) or
`not_evaluated` (our check could not decide: our source query, our preparation or
our qualifier failed; the harness exits 5). Today's harness scores the last as
NVIDIA's FAIL (`scripts/qualify_om10_nvidia.py:196–202`,
`scripts/benchmark_om10_nvidia.py:490, 505–506`), against the rule.

| Owned | NVIDIA (best arm) | Score |
|---|---|---|
| valid, achieved | valid, achieved | speed ratio; the only publishable comparison |
| valid | no valid achieving arm | a finding; not a win; not in the ratio |
| any | `not_evaluated` | ours: the input is excluded and listed with the reason, never "no valid arm" |
| refused or failed | valid | owned loss, counted against the goal |
| refused or failed | no valid arm | recorded; excluded from speed |
| censored (past the harness budget) | any | an owned loss in coverage; the budget recorded |
| any | censored | neither NVIDIA-invalid nor our win; the budget recorded; re-run with a longer budget before a claim |

**The claim per stratum:**

- the ratio of summed paired costs (NVIDIA over owned) over inputs where both
  sides are valid and achieve, with a bootstrap interval resampling inputs and
  runs;
- the fraction of inputs won, with its interval, and the per-input ratio
  distribution (median, quartiles, worst), because the sum is dominated by the
  largest inputs and hides small-part losses (Audit §2: median 32.5×, worst 279×
  on a 5-face part);
- a **win** only when the ratio's interval lies wholly above 1 and owned coverage
  on the stratum is no lower than NVIDIA's; otherwise "not distinguishable", or
  "faster where both are valid, lower coverage" (G8 §7.5).

Exclusions, listed in their own tables: refused placements (non-uniform, shear)
(`.56`); tessellated-only parts (`.61`). The speed goal is judged on the
real-input strata (§23.4), the watch in its own stratum; authored families and
fuzz mutants count toward robustness only.

### 22.6 What has to fall

NVIDIA figures below are from the pre-fix harness at n = 1 and are labelled so
until the fixed harness has run.

- **NVIDIA on the watch:** 22.86 CPU-s of per-definition `tessellate`; its largest
  definition takes 1.995 s; its busiest worker took 2.75 s under face-count
  assignment (SM §4); ingress export plus batch wall 10.17 s (B2-ha §1). With 12
  pinned workers and LPT on measured times, its meshing makespan on one lane is at
  least max(22.86/12, 1.995) ≈ 2.0 s.
- **Owned today:** about 1,578 CPU-s on the watch, 69× NVIDIA's work (Audit §0).
  On 11 workers that is at least 1,578/11 ≈ 143 s even under perfect scheduling.
- **Target:** to beat the lane makespan, owned meshing work must fall below about
  11 × 2.0 ≈ 22 CPU-s on the watch, a per-unit cut of about 70×, and the largest
  single face (T∞) must stay below NVIDIA's largest definition. These are targets
  the arena measures against; no outcome is claimed.
- **Stage budget** for that target, ≤ 20 CPU-s on 11 workers at 0.005 mm [call:
  allocations to revise from M8's first measurements]. Every line is a per-unit
  cost, so it applies to any input; it is checked per report class, never on the
  watch alone.

| Stage | Share | Per-unit form | Counter |
|---|---|---|---|
| T0–T1 decode, carriers, span bounds and cones | 5% | CPU-s per span and per curve | spans, curves |
| T1b feature size, T7 clearance | 15% | CPU-s per E1 candidate (the per-pair target once E1 counts exist, §12.6) | candidates per class, closed-form share |
| T2–T5′ measurement and reconciliation | 15% | CPU-s per (sample, occurrence) | samples, deferred items per rung |
| T8 closure | 10% | CPU-s per segment test | segments, closure splits |
| T9 lattice, band CDT, A0 and the ledger | 40% | CPU-s per triangle, plus a fixed cost per face | triangles, faces, band sites, flips |
| T10 publication, records, digests | 10% | CPU-s per triangle and per record | bytes hashed, records |
| scheduling | 5% | CPU-s per task | tasks, steals, batches |

  At 20 CPU-s over the watch's faces the whole budget is about 2.2 ms per face
  against NVIDIA's 2.5 ms; 92% of those faces are analytic at about 30 triangles
  each, so the fixed per-face part (graph node, records, keys) must stay in the
  tens of µs.

## 23. The arena

**Stage** all · **Evidence** journal; M2 §5.7; G2 §6; G4 §6; G7 H5; G8 §3.6, §7; G10 §6;
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
| F8 | `gpu` | lane GPU on (11 workers + driver) | CPU only (12 workers) | G8 G0–G2 |

Generators F7 = F1·F2·F3·F4 and F8 = F1·F2·F5·F6; defining relation
I = 12347 = 12568 = 345678. Every word has length ≥ 5, so main effects and two-way
interactions are clear of each other while every flag stays two-level [call: the
flag set; operator: 64 and the fraction]. F8's effect is estimated per lane with
the lane as a block, never pooled across GPU0 and GPU1 (`.68`). SIMD is not an
arena flag: there is no maintained scalar product path, and vector kernels are
compared with their scalar reference only as kernel microbenchmarks. The draft's
thresholds flag moves to §23.3 now that β and r_c are separate.

### 23.3 Follow-up arms

Run as one-factor arms against the default, or as a new fraction around anything
intriguing:

| Arm | Levels | Hypothesis |
|---|---|---|
| `thresholds` | (β, r_c) = (ε_ref/2, ε/2) / (ε_ref/4, ε/4) | G10 §6 |
| `theta_cut` | 3.6°, 8.1°, 10° | M2 H9 |
| `r2_linear_step`, `transverse_leap_by_conditioning`, `tangent_split`, `l3_blend` | on / off each | M2 H4, H6, H7 |
| `equal_tier_tangent` | symmetric recorded step / refuse | §8.7; counts the class |
| `boundary_closure` | A hereditary / B synchronous check-and-split | G2 H5, H6 |
| `split_point`; `tau0` | dyadic / bisector-snapped; π/3 / π/6 | G2 §6 |
| `biased_split` | off / threshold × (1 − 2⁻⁴⁰) | G7 H5 |
| `closed_form_counts` | dyadic / base·2^j / plain closed form | §10.2; LOD paper §5.2 |
| `signed_clearance` | material cusps only / everywhere | G4 H3 |
| `local_pair_test` | off / on where clearance is not established | `.59` |
| `cap_c` | 2 / 1.155 / 1.414 / off | G13 H3 |
| `postprocess` | none / flips / collapses / relaxation | G18 H1 |
| `scale_buckets` | off / r = 2^(1/8) | G9 H2; `.57` |
| `inline_resolution`, `speculative` | follow-ups of F4 | journal |
| `placement` | load-reactive / fixed | G12 §5.3 |
| `smt_workers` | 12 / 24 per socket | G8 §6 |
| `gpu_driver_core` | dedicated twelfth core / SMT sibling of a worker core | G8 §8 |
| `cross_socket_steal` | on / separate pools | G8 P3 |

Not arena arms: `topology_change` is a user option of the output contract
(`.47`), not a choice to score; the 3MF mirrored-item form is settled by one
consumer check in the placements unit (slice both variants in PrusaSlicer and
Cura; no corpus file has a mirror, G9); directed 3MF rounding waits for observed
pair refusals (§18.3).

Defaults from the arena are provisional; a perturb-and-permeate cycle keeps
perturbing them after principal development [operator].

### 23.4 Corpus, strata and requests

- **Strata, never pooled:** production exports (NIST MBE PMI 49 unique, CAx-IF 53,
  five open-hardware TDPs 376, step-conformance 27); **the watch, its own stratum**;
  the OCCT dataset (333, bug reproductions that over-represent pathology); ABC
  when acquired (fixed selection: 3 chunks, 30,000-slot robustness sweep,
  1,000-model speed sample; the download term is open, `.28`);
  Gmsh/FreeCAD/Netgen/resources4truck held sets. Fusion 360 Gallery is out
  permanently (`.29`). BRL-CAD from a mirror into quarantine as reference only
  (`.30`). A second OM10 revision waits (`.36`).
- Every arena default and speed claim is stated with and without the watch.
- **Admission before defaults.** Today's ingress refuses 268 of 500 corpus files
  (non-mm units 155, C/S residual 44, multi-wire curved faces 39, leaf binding 14,
  periodic spline charts 12) and admits the watch whole, so the admitted set is
  watch-shaped. Defaults are adopted only from an arena run after the gate lifts
  `.25.6`, `.25.13`, `.25.12` and `.25.5` have landed. Arena runs before then are
  exploratory and labelled "admitted set at T"; each adopted default is re-run,
  not perturbed, as each later gate lifts. Every arena verdict carries the
  admitted fraction per stratum and its report class.
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
  recorded (G15 §8 item 1) [call; operator delegated at G2].
- The output-size guard applies at every level.

### 23.5 Authored families

Parametric, generated, none keyed to the watch. Each family states its parameters,
the branch it forces, its expected outcome and **the record that shows the branch
fired** (a rung in the chain, a collapse op, a closure split, a tilt refinement, a
named refusal, a signed-credit edge). A family run whose expected record is absent
fails: a family that never reaches its branch tests nothing.

| Group | Families |
|---|---|
| Closure (G2 §5.1) | F1 disk with off-centre hole, gap/sagitta 10 → 0.01; F1b hole inside a chord's cap (containment without crossing); F2 S-bent UV pcurve on a clustered-knot B-spline; F3 two inner circles; F4 crescent cusp, Δκ → 0; F5 wedge α 60° → 0.1°; F6 saddle z = κxy beside planes; F7 cone trimmed near and through the apex; F8 crossing source trims; F9 3D neck thinner than 2ε; F10 seam-crossing, multi-turn, shifted periodic; F11 non-manifold edge and same-face seam with different depths; F12 strip of k faces with alternating inner loops; F14 speed ratio ρ to 10³ |
| Embedding (G4 §6) | K1 extruded crescent (θ 0.5–20°, R_B/R_A 1.2–10, even and lattice slabs, concave face ×1 and ×5); K2 lens; K3 chisel and material cusp; K4 doubly-curved knives; K5 curved knife edges (countersink lip); K6 reflex slot; K7 tangent (G1) edges, fillet to plane and cylinder to cylinder (no wedge refinement expected); T1 thin tube w/ε 0.25–2.5; T2 slab with a boss; T3 parallel planes (no change expected); T4 thin spherical shells; S1 B-spline hairpin and helical face; U1 hole near a convex arc and loops at 0.5–10° (straddles OCCT's 5° exemption); U2 concave-trim sliver; V1 pyramid apex and needle tips; R1 faces 2ε + η apart after a resolution move; C1 source contacts; X1 non-coaxial cylinder pairs (cross holes, pins), cone–cylinder and torus pairs |
| Collapse (G10 §5) | A1 planar sliver width sweep around η and 2β; A2 curved slivers; A3 twisted strip k = 0–10; A4 bilinear patch; A5 L, crescent, serpentine, dumbbell; A6 short line and arc; A7 polygonal hole N = 8–512; A8 cut corner; A9 merged vertex leaving a convex face; A10 shallow step; A11 through-pinhole; A12 hourglass; A13 island sliver; A14 plane–freeform sliver; A15 touching shells; A16 collapse beside a thin wall |
| Reconciliation (Heal §7.5; M2 §5.7) | edge–surface offsets 1e-6–1e-2; dihedral sweeps across the conditioning cutoff, with analytic weights high and low; coplanar and near-coplanar split planes; four planes at a vertex; pcurve-free variants; vertices off curves; plane–cylinder tangent step; equal-tier near-tangent pairs with gaps across (η, ¼ lfs); fillet tangent to a plane with controlled Δκ and stored-curve slide (M2 H2); edges with radius below 8ε_ref (L3 slide residual) |
| Entities (G5 §4) | revolution profiles (B-spline, conic, line, touching the axis); extrusion; elementary and bicubic offsets d = 0.01–0.9 of the curvature radius; spindle and horn tori (all 410 corpus faces); high-degree B-splines; multi-span B-splines with knot multiplicities p − 1 and p; rational patches with high weight variation (falsifies §13.1's quotient bound); irregular B-spline points (S_u × S_v = 0 away from any declared pole); voids (`void-block.step` and its void-free twin, signed volumes 936 / 1000); apex cones; sheets |
| Identity (G16 §3.2) | small closed circles and lenses at ε ≥ R |
| Placement (G9 §8) | mirrored chiral part (helical gear); one curved definition at 32 scales over [0.5, 2]; rigid instances whose σ differs by ulps (one key expected); authored CTO fixtures |
| Shading (G13 §5) | the seven exact surfaces of G13's probe |
| Contact (G17 §5) | pin-in-hole c ∈ {1, 2, 5, 10} µm, δ_h/c ∈ {1, 0.5, 0.2, 0.1} (physics phase) |
| Malformed values (§5.8) | one fixture per catalog row: each must refuse exactly its entity with its reason code, and the rest of the part must mesh |
| Carried regressions | the cylinder-seam trimmed family; OM10 fixtures named in B2 |
| Scale and far placement | whole-model scale 1e±6; 100 m offsets; N-holes, N-grooves, N-instances scaling; many-hole plates (N = 8–4,096, the `direct_cdt` pairwise-cost family) |

**Fuzzer:** mutations over STEP and B-rep parameters (loop permutations, decimal
spellings, NaN/Inf injected after ingress at evaluator, projection and sizing
outputs). Its oracle: every output passes the full stack, or carries refusal
records for exactly the entities whose computation read an injected value; no
face that read one may be `valid` (§4.1 finiteness). A run past its timeout (§4.8) is a
failure. Seeds logged; failures minimise into permanent regressions.

### 23.6 Sampling plan, adoption rule and per-run record

- **Replicate across inputs, not processes.** One process per (configuration,
  input, lane), with inputs as blocks. Each stratum's input sample is seeded,
  sized from the time budget and recorded before any result. The first arena is
  time-boxed in hours, not days, and the sample size follows from that box
  [operator].
- **Named before the first run:** the primary response, log of the stratum's
  summed cost; the correctness gates (every oracle green, per report class); the
  secondary responses (triangles, peak memory).
- **Adoption.** A default changes only if its effect's interval on the primary
  response excludes zero with the correctness gates held, and a fresh
  confirmation at ≥ 10 processes on held-out inputs reproduces it. Everything
  else is exploratory: followed with no time box if intriguing [operator], never
  adopted on the screen alone.
- **Per-run record** (tagged-table container; TSV only when presented), per input
  and configuration: wall and CPU per stage, work
  counts (samples, cells, band triangles, deferred items per rung, closure splits,
  collapse clusters, clearance candidates), peak RSS and device memory, the full
  oracle verdict, and the context row. Correctness failures are never averaged
  into speed.

## 24. Verification stack

**Stage** S25 · **Evidence** Map S25; G7 §3; G3 §4.5; G12 §5.4; G13 §6; G14 §6;
G15 §7; G16 §3.4; M2 §5.7; BLACKBIRD testing · **Decisions** mutants after the
stack settles [operator].

- **Outcomes.** Every oracle returns `pass`, `fail`, or `not_evaluated` with a
  reason (a distance query out of its budget, a trim domain the oracle could not
  build). `not_evaluated` is non-green for exit status and is counted separately
  in the arena; it never reads as a pass (ALPHA_QUALIFICATION: skipped or not-run
  cases are never passes).
- **Independence.** Each oracle runs offline on written outputs against the
  original source. It never trusts a producer status and never takes a target, a
  threshold, a UV, a budget or an exemption from what the producer wrote:
  thresholds come from inputs only (ε_f, the key's δ_exp and R_max, r_c), and
  every producer claim an oracle uses is re-derived from source first (oracle 10
  verifies the chains before oracle 1 measures faces against them; oracle 5
  re-checks every exemption).
- **Which outputs exercise the fail branches.** A correct mesher refuses bad
  input rather than emitting a bad mesh, so known-bad inputs exercise only the
  refusal paths. Real defective outputs exercise the output oracles in the same
  pass: NVIDIA's recorded invalid outputs (`void-block.step` in 8 configurations;
  the duplicate-triangle class of definition 1808; the inside-out fillet class;
  SM §5), scored by these same oracles (§22.5), and the outputs of arena arms that
  drop a guarantee (plain-double predicates). Hand-made defective outputs (a
  vertex pushed 1.01ε, two face ranges swapped) are mutants of the output; they
  wait for mutation testing (§24.4). Nothing tests an oracle separately.
- **Build units.** Oracle 1, its trim-domain builder and oracle 10 do not exist
  today; they are build units of the plan, before the code they judge (§25).

### 24.1 Oracles by fault class

| # | Oracle | Fault class | Tier | Fails when |
|---|---|---|---|---|
| 1 | Two-sided sound deviation against the target of §3.2, rebuilt by the oracle from source and the chains oracle 10 has verified, over the trim domain the oracle builds itself (below), with a covering-radius correction or bounded Hausdorff; boundary, pole and apex triangles included; collapse geometry checked against r_c. Run on the FP64 output at ε_f − δ_exp of the key, and on each written format, read back in its own frame, at ε_f. On a foreign producer's output (NVIDIA): against the source faces, with each edge's input discrepancy d_e allowed within ℓ_f of that edge | deviation | property | any point beyond its threshold in either direction |
| 2 | Exact per-face UV oracles against the oracle's own trim domain: every constraint segment present; coverage with no overlap or gap; orientation independent of topology; Steiner points on their parents; CDT property where claimed | triangulation | property | any violation |
| 3 | Topology by vertex key (never by position): shared-edge identity and order across incident faces; edge-use counts, with edges left single-used by a refused face classified `open_by_refusal`; per-shell Euler with face genus χ = 2 − 2g − b − p; accounting of every face, definition, key and occurrence | watertightness | property | any mismatch |
| 4 | Classification map (G16 §3.4): vertex on model vertex within the request; vertex on E at its t, its (u, v) on every incident face matching the oracle's own projection of the written position, its normal offset (I9, default 0) honoured; consecutive chain samples form mesh edges used by exactly E's faces and no other mesh edge joins two samples of one chain (checks §10.4); triangles classified on F or its boundary or a survivor; every source face has a range, absorbed or refused record; every UUID read reaches output; per-format readback | identity | property | any check |
| 5 | Offline self-intersection with the retained contact BVH and filtered predicates, adjacent and coplanar pairs included. A reported source pair is exempt only after the oracle recomputes its source clearance: the exemption fails if that clearance exceeds c_floor, or if the pair's mesh intersection lies farther than ε_f + μ from the source contact set | embedding | property | any other forbidden pair, or a failed exemption |
| 6 | Normals: (a) on planes, cylinders and spheres, interpolated corner normals equal the exact normal at the nearest carrier point within 1e-12 rad (G13 §1.3); (b) on every carrier kind, each written corner normal lies within 1e-6 rad of the exact normal at the oracle's own projection of the written position, unit within 1e-6 (G13 H5), and at quotient vertices the one-sided limit normal along the corner's own triangle; (c) A0 on every triangle; the facet bound arcsin(√3·R·κ) only where R·κ ≤ 1/√3 (G13 H2's precondition), closed-form strip bounds on cylinders and cones; (d) glTF written normals reproduce the source dihedral within 1e-5 rad | shading | spec/property | any triangle or corner outside |
| 7 | Reader-equivalent export oracle per format and named consumer: parse; rebuild what PrusaSlicer, Cura, lib3mf, three.js and fTetWild hold; check degeneracy, orientation, manifoldness by index, pinched vertices, exact self-intersection against the FP64 set, displacement ≤ the key's δ_exp; 2.4–5.2 µs per triangle | export | model | any new defect |
| 8 | Ingress: authored fixtures per override class must produce the raw value and a record naming the class; one fixture per §5.8 catalog row refuses exactly its entity with its reason; the native source record per owned operation kind (§5.4 row 28); the meshed boundary is checked against the raw master; owned S1/S2 verdicts compared with the adapter's; CAx-IF and NIST validation properties computed by us against exact properties of the geometry as read, sender mismatches recorded (warn and comply, `.39`) and never failed; 86 Rat Rig inch/mm twins equal ×25.4 within 1e-9; the owned reader byte-identical to the bridge where the bridge agrees with raw | reading | spec/metamorphic | any mismatch in our own computation |
| 9 | PMI reader (G15 §7): tolerance counts equal the sender's validation properties (STC-10's composite counting a known finding); mm→inch rewrite leaves every margin unchanged; nominal-on-limit holes give PMI-OUT at every ε | request | spec/metamorphic | any mismatch |
| 10 | Reconciliation against source: for every chain sample and resolved vertex, the oracle's own projection onto each incident source carrier gives the distance and the move from the source position; fails on a hard-tier (plane) move > η, a rung-0 sample with residual > η or move > β, any move > ¼ lfs with lfs recomputed by the oracle from exact distance queries, or a move beyond the edge's recorded μ̄. Model tier (M2 H3): a global solve with the same weights over the definition, offline and slow, on corpus items | reconciliation | property; model | any violation; any position differing from the global solve by > η |
| 11 | O-bytes per placement function (§24.2) | determinism | metamorphic | any byte difference |
| 12 | FI-1 and FI-2 across backends and between mixed runs (§24.2) | backend correctness | metamorphic/differential | a status or class difference FI-2 does not excuse |
| 13 | Incremental = fresh via the edit generator (§24.3), which is also the cache key completeness test | caching | metamorphic | any byte difference; any case that breaks its class's hit/miss expectation; a recomputed task outside the halo |
| 14 | Metamorphic geometry: whole-model scale by 4^k (exact in binary and under square roots) with ε and every declared length scaled gives the same keyed triangle sets, positions scaled exactly (every threshold is relative or scales with ε, so a difference names a hidden absolute constant); scale 1e±6, far translation and rigid rotation keep topology and pass oracle 1; mesh(K = 1) ≡ mesh(K = k) for every cluster count and assignment | invariance | metamorphic | any difference |
| 15 | Forced caps: each row of §4.7's table forced to fire; the status and the named stop record are checked against that row | silent accept | spec | a status other than the row's, or a missing record |
| 16 | Angle term: per emitted triangle, normal turning by dense evaluation of the exact normal over the triangle's UV preimage, with a covering bound between samples from the span's normal cone; per chain segment, curve turning. Run on owned Render-profile output and on NVIDIA's angle arm before it is called matched (§22.2) | angle fidelity | property | any triangle or segment beyond θ_max |
| 17 | Counts on untrimmed analytic families (cylinder strips, sphere lattices, full circles, planes with no Steiner points): chain samples and triangles equal the closed form of the rule in force (dyadic: 2^⌈log₂ n_cf⌉ with the §10.4 floor; 2n triangles per strip of n slabs) | over-refinement | spec | any difference |

**The oracle's trim domain.** For every face the oracle builds its own trim
domain by projecting the source 3D edge curves onto the carrier with its own
seeded solver at its own density, independent of §8.1's projection and never
reading producer UVs. Where the file has a master pcurve, the oracle uses it.

### 24.2 Determinism and functional-equivalence oracles

- **O-bytes** (D0–D2): same input, one placement function (CPU-only, or fixed
  placement with the GPU-eligible classes on the lane's GPU), under thread counts
  1, 2, an odd count, 12, 24, 48; randomized task order and injected delays;
  varied GPU launch shapes and streams; GPU0 against GPU1 (one device class, one
  `backend_id`); shuffled batch compositions; cold against warm cache. D0 across
  separately built host binaries. Each run records the worker count and launch
  shape it actually used, and the input exceeds every parallel threshold, or the
  test cannot fail.
- **Replay:** the default load-reactive configuration with a recorded placement
  log forced, under every perturbation above, gives equal bytes.
- **FI** (`.53`), across backends and between two load-reactive runs with
  different placement seeds: each output passes oracles 1–10 and 14–17 on its
  own; **FI-1** accounting identical (dispositions, keys, occurrences); **FI-2**
  status and classification identical (face status, edge class, collapse
  membership, rung), except where the two runs' values of the deciding quantity
  lie on opposite sides of its threshold and its higher-precision recomputation
  lies between them; such a difference is excused and recorded. Nothing else is
  compared across backends: no position tolerance and no per-kernel
  forward-error bound, since identical positions are not a goal.
- **Comparisons that cannot fail and are not used:** cross-backend Hausdorff ≤ 2ε
  after both passed deviation (G7 N1); watertightness after positional welding;
  counts read from the shared decode; D0 compared across backends inside one run;
  determinism runs that do not vary concurrency; two runs reading the same cache
  entries; incremental against fresh inside O-bytes (oracle 13 owns it); validity
  against the mesher's own reported error (G7 N6); statuses compared through a
  shared record. Cross-backend byte equality of D1 layers is the wrong oracle (it
  fails with no bug).

### 24.3 Incremental = fresh

Metamorphic: two computations that must agree, the cold one taking no cached path.
The edit generator carries G12 §5.4 whole. Edit classes: identity (renumbering,
file rename, thread count); transform (rigid, within R_max); scale; request;
carrier interior; carrier boundary; vertex; pcurve or parameterisation; topology;
spatial (an edit within 2ε of another entity, then one away: two edits); collapse
(shorten below r_c, then lengthen); sequence (ten random edits, then undo all).

- Identity and transform classes expect zero misses in T1–T9 with unchanged
  bytes. Every other class must both hit and miss. A case that breaks its class's
  expectation fails; none is discarded.
- The warm result equals a cold run of the edited input; a sequence's undo
  restores the original bytes with every task a hit.
- Every recomputed task lies in the static halo B(E) of §19.2 (G12 §5.4 step 5).
- Channel Δ equals channel F on every generated edit.

One oracle covers key completeness, key minimality and edit equivalence (G7 §5.6,
G12 §5.4).

### 24.4 Mutation testing

Comes later, on the fleet, once this stack has settled into the one the project
builds on. Mutants test the tests, not the project [operator]. Hand-made
defective outputs belong here.

## 25. Certification removal and the old mesher

**Stage** all · **Evidence** Audit §3–§6 · **Decisions** certification excised;
the old mesher leaves the source graph [operator].

- **Removed from the hot path:** Rational curve parameters and site UVs; per-facet
  certified bounds and their Rational compare; precise and materialised fallback
  cascades; certificate-driven horizon refinement; interval kernels used as
  acceptance tests; Rational SAH costs; per-read GCD; snapshot copies of exact
  tables; the 165 s whole-component contact classification; the stop-token check on
  every work charge (replaced by cancellation at task dequeue, §4.8); run-wide
  atomic allocation metering; per-query source endpoint
  guards (moved to ingress, once per source).
- **Kept and ported:** `DefinitionBoundaryForest` on doubles and dyadic paths;
  `PhysicalNodeKey`, `FacetKey`, site sources, occurrence and wedge bindings, edge
  provenance; `prepare_face_domain`'s occurrence and joint logic as specification;
  the packet zipper; `direct_cdt` on fixed-point input; the numeric kernels and CUDA
  ops; canonical ordering by key; source decode and IO codecs.
- **Moved to the test suite:** the contact BVH and filtered predicates (oracle 5);
  the independent original-source readers, rewritten for the new contract. As
  kept they encode the old one and would fail correct output: every vertex on its
  source generator within 2⁻³² (`python/brepgpu/imported_oracles.py:2140`), every
  edge sample on the original curve (`:2171`), a 2D pcurve required on every
  coedge (`:1399`), while the new mesher moves vertices on purpose (§3.2) and
  98.7% of non-planar coedges have no pcurve. Their on-source checks are replaced
  by oracle 10, the mandatory-pcurve rule is lifted, and their distance query
  (which today refuses a face edge with no usable trimmed pcurve) gives way to
  oracle 1's own trim domain (§24.1). Oracle 1, the trim-domain builder and
  oracle 10 are build units of the plan.
- **Retired:** the old mesher leaves the build; every consumer (watch render,
  exploded diagrams, scene and display, cut and extrusion result meshers) moves to
  the new path, the result meshers through the native source record (§5.4 row
  28). Old results, images and reports remain, each gaining one line saying
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
| I9 | one-sided tolerance per face (ε⁺, ε⁻) | slot reserved: a signed normal offset per vertex and occurrence, default 0, in chain and face-output records and classification rows (§4.6); oracles 1 and 4 measure against S(u, v) + offset·n. A shared chain's offset must satisfy both incident faces' bands. The signed contract itself comes with physics |

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
are marked **(operator)**. "M8" means a measurement the plan schedules before the
mechanism's default is relied on.

| # | Choice | Default | Settled by |
|---|---|---|---|
| 1 | truncation angle θ_cut | 8.1° | arena `theta_cut` (M2 H9) |
| 2 | reconciliation: M2 bundle vs healing classes | M2 bundle | first arena F5 (M2 H4) |
| 3 | M2 sub-proposals separately | each on | follow-up arms (M2 H4, H6, H7) |
| 4 | small-feature collapse at all | on | F6 (G10 H3: drop E′/S′ if no stratum gains ≥ 1%) |
| 5 | (β, r_c) | (ε_ref/2, ε/2) | arm `thresholds` |
| 6 | face generator | lattice + band | F2 |
| 7 | predicates | robust adaptive | F1 [operator: arena both] |
| 8 | evaluation precision | FP64 | F7 |
| 9 | in-face clusters in the default | on only where a face exceeds τ_split | F3 plus M8's per-face work distribution and empty-band share (PC §6) |
| 10 | staging | background healer | F4; inline and speculative follow-ups |
| 11 | GPU in meshing | on, by cost model, per class and device | F8 per lane and per report class; G8 G0–G5 |
| 12 | closure arm | A hereditary | `boundary_closure` (G2 H5, H6) |
| 13 | split point, τ₀ | dyadic, π/3 | follow-up arms |
| 14 | ties at round CAD numbers | unbiased | `biased_split` (G7 H5) |
| 15 | signed clearance credit | material cusps only | `signed_clearance` (G4 H3) |
| 16 | clearance not established | tighten once, then refuse | `local_pair_test` (`.59`) |
| 17 | clearance floor c_floor | 2δ_exp + bound resolution | oracle 5's own source clearance on families C1, T1, K3 |
| 18 | cap factor c | 2 | `cap_c` (G13 H3: ≤ 2% triangles, ≤ 2.1 θ_ref) |
| 19 | post-processing operations | none | `postprocess` (G18 H1) |
| 20 | scale bucketing | off (distinct beyond τ) | `scale_buckets` (G9 H2) |
| 21 | 3MF mirrored items | baked opposite-hand | one consumer check in the placements unit: slice both variants in PrusaSlicer and Cura (G9 H3) |
| 22 | placement | load-reactive | arm `placement` for edit locality (G12 §5.3) |
| 23 | workers, driver core, cross-socket stealing | CPU-only 12 pinned; GPU on 11 pinned + driver on the twelfth core, blocking sync; stealing on | arms `smt_workers`, `gpu_driver_core`, `cross_socket_steal` (G8 §6, §8, P3) |
| 24 | target displacement field | C¹ smoothstep over ℓ_f = ½ min lfs, nearest-boundary-point extension, reflex bisectors as constraints (§3.2) | oracle 10's model tier (M2 H3 global solve) and oracle 1 on the reconciliation families |
| 25 | freeform angle term and σ̲ | Bernstein normal cone per sub-span (§6.2) | oracle 16 on the B-spline families; the cone's refinement cost per report class |
| 26 | displacement derivative bounds d′, d″ | from sample differences | oracle 1 on families with large rung-2/3 moves; replaced by a bound if it ever fails |
| 27 | edge move bounds μ̄ | 2β for `small` edges, ¼ lfs_e otherwise | M8: `move_bound_exceeded` count and the clearance tightening the ¼ lfs bound causes |
| 28 | trim-sliver reserve r_trim | L_S·w̄ | kept: its derived magnitude (w̄ ≈ κ_uv·ℓ²/4) is what decides it, not whether oracle 1 ever sees it bind |
| 29 | placement τ | 1e-6 | G9 H5 over every raw placement in the corpus |
| 30 | gaps against declared uncertainty | gross gate structural (¼ lfs) | G15 H5: do measured gaps track u_ctx across export triples |
| 31 | PMI warning threshold c_max | 0.1 (`.54`) | G15 H4 offline conformance-flip measurement |
| 32 | PMI-bearing inputs at relative ε | absolute only in scored runs; warnings recorded | decided at G2 [call; operator delegated] (§23.4) |
| 33 | face-over-solid plain style | face wins, recorded | G9 H4 corpus count |
| 34 | contact k and θ_f | 10, 10° | M-1 pin-in-hole family (physics phase) |
| 35 | identity carriage cost | ranges | G16 H1–H4 (< 1% time, < 2% bytes) |
| 36 | clearance cost | budgets on | M8 E1 candidate counts and closed-form share; G4 H4 (< 2% time, < 5% triangles) |
| 37 | angle-term cost | render profile only | G14 H-angle census of radii below 42ε |
| 38 | grading | none | G14 H-no-grade slicer and fTetWild measurements |
| 39 | native revolution and extrusion | native | G5 H1, H2 |
| 40 | general offsets | refused until a corpus shows one [call; operator delegated at G2] | a corpus census (ABC if its download term is ever accepted, `.28`) |
| 41 | equal-tier near-tangent pairs | symmetric recorded step | arm `equal_tier_tangent`; M8 corpus count of the class |
| 42 | governor | `schedutil`, delivered clock recorded during the run | revisit only if variance hides effects (`.27`) |
| 43 | dyadic versus closed-form counts | dyadic | arm `closed_form_counts`, per surface-kind class |
| 44 | count backstops (projection 16, Newton 8, face budget 16×, cap repair 2, fuzz timeout 8×), Δκ floor 8×, m_α 10× | as listed (§4.7, §11.1, §12.3) | M8 distributions of the measured counts against each bound |
| 45 | owned ingress budget and streaming | ≤ NVIDIA's minimal ingress [operator]; stream per definition | M8 adapter phase split; the owned reader unit `.25.7` |
| 46 | stage budget allocations | §22.6 table | M8 CPU-s per task class on the strata, the watch separately |
| 47 | quiet-gate threshold | 5% of one CPU per lane CPU | M8 idle noise floor per lane |
| 48 | NVIDIA chord tightening and screening | chord·max(½, 0.9ε/δ); best k = 3 arms at full n | stated with every result; revisited if a stratum's ranking changes with k |
| 49 | export reserve limit | δ_exp ≤ ε_f/2, else recentre or refuse the format | the 100 m family's counts |
| 50 | degree buckets above 3 | 4–5, 6–8, 9–12, 13–25 | M8 kernel microbenchmarks per bucket |

## 28. Corrections carried from the M4 lanes

Each correction recorded on bead `brepgpu-12s.20`, and where this design resolves it.
Where the M6 tribunal changed a resolution, the row says so; every M6 finding's
disposition and section is in [M6-findings](fast-mesher-campaign/M6-findings.md).

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
| G7 | `backend_id` includes libm (M6: transcendentals stay libm; owned kernels bought nothing by default) | §4.2 |
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
| G4 | ε̂ and μ in the half-clearance lemma (M6: ε̂ ≤ ε_loc now that r_trim is in the ledger; μ per zone, not per face) | §12.1 |
| G4 | shared-edge tilt rule; vertex cone rule (M6: openings from the signed dihedral; UV half-clearance is closure's P2/P3 alone) | §12.1–§12.3; §11.1 |
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
| G8 | GPU hypothesis; break-even about 10³ (M6: analytic offload is the arena's G3 test, not a dispatch rule) | §20.1; §20.6 |
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
- How `direct_cdt` is adapted to fixed-point scaled-chart input. Dropping its
  pairwise hole-containment check and splitting many-hole faces are plan
  requirements (§13.2), not builder's choices.
- The exact flag-to-run assignment of the 2^(8−2) design, from the generators in
  §23.2.
- Corpus sample sizes within a few fleet hours per arena pass, seeded and recorded
  before any result (§23.6).
- Slab, chunk and staging-buffer sizes, from M8's per-task measurements (§20.1).
- Unit decomposition and order under JSMNTL (sub-plan, red tests at the right oracle
  tier, code, green, one independent review, fixes), in the M7 plan.
