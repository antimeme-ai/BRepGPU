# Determinism contract and cache keys without bit-identity

Research lane M4-G7 of the fast-mesher campaign, 2026-10-17. Brief:
[docs/fast-mesher-campaign/briefs/M4-G7.md](../docs/fast-mesher-campaign/briefs/M4-G7.md)
with [M4-common.md](../docs/fast-mesher-campaign/briefs/M4-common.md). Stub:
[lifecycle map](2026-10-17-geometry-lifecycle-map.md) §4.1 G7 (stages S16, S17;
contradiction X10). Bead `brepgpu-12s.8`. Sources and read depth:
[2026-10-17-determinism-cache-keys-sources.tsv](2026-10-17-determinism-cache-keys-sources.tsv).

One small probe ran on the fleet (question 5, §6): it read corpus STEP text and
evaluated four closed-form density rules on CPU and on both P100s. No mesher,
Boolean or NVIDIA code ran. Nothing ran on the laptop.

Tags: **[src]** a source establishes it; **[code]** read in this repository at
file:line; **[meas]** the §6 probe; **[own]** this lane's derivation;
**[inf]** inferred from a source, not stated by it.

G12 (edit invalidation) reads this paper first. §1, §3 and §5 are written to be
read on their own: §1 defines the backend and the classes, §3 the oracles,
§5 the cache key, with no forward references.

## 0. Answers

**Operator decision D-determinism**: identical output across thread counts on
one backend; functionally identical across backends; one model may mix
backends. It binds everything below.

**Determinism class per output layer** (classes defined in §1.2):

| # | Layer | Class | Producer | Oracle (§3) |
|---|---|---|---|---|
| L1 | Source decode: entity records, topology graph, use counts, source inventory, unit factors | **D0 canonical** | ingress, host, once per source | O-bytes everywhere |
| L2 | Carrier data: analytic radii/frames, per-span second-difference bounds, integer work estimates | **D1 shared** | one producer per carrier | O-bytes per backend; FI across |
| L3 | Thin-feature planning (ε-inflated face-pair query result, local feature size) | **D1 shared** | one producer per definition | O-bytes per backend; FI across |
| L4 | Vertex resolution (V-class, resolved XYZ) | **D1 shared** | one producer per vertex | O-bytes per backend; FI across |
| L5 | Edge chain: R-class, resolved positions, sample parameters and dyadic paths, XYZ, vertex IDs, one UV per face occurrence | **D1 shared** | one producer per physical edge | O-bytes per backend; FI-4 across |
| L6 | Face interior: lattice, band triangulation, interior points, triangles, per-corner normals | **D1** | the face's producer | O-bytes per backend; FI across |
| L7 | Records: per-edge measures, per-face achieved error, statuses | **D1** (values); status is a function of them | with their layer | O-bytes per backend; FI-2 across |
| L8 | Publication: global vertex and triangle numbering, canonical order, source maps | **D2 derived** | host, fixed path | O-bytes given equal inputs |
| L9 | Export bytes (internal schema, STL, 3MF, glTF) | **D2 derived** | host writer | O-bytes given equal L8; G3's reader oracle |
| L10 | Content digests and cache keys | **D2 derived** | host | none needed beyond L1–L9 |
| L11 | Timings, memory peaks, queue depths, first-reported racing error, provisional display meshes | **D3 non-reproducible** | anywhere | never compared, never keyed |

So: edge samples, face triangles and export bytes are all bit-identical per
backend, across thread counts, runs, launch shapes, batch compositions, GPU
instances of one architecture, warm or cold cache, and incremental or fresh
remeshing. Nothing above L1 is bit-identical across backends, and nothing needs
to be, because of the one-producer rule (§4).

**Cache key** (§5): for every task,

```
key = H( key_schema ‖ task_kind ‖ backend_id ‖ request ‖ digest(own source records)
         ‖ for each consumed dependency: role ‖ digest(that dependency's OUTPUT bytes) )
```

`backend_id` names the build and the arithmetic it runs (§1.1). `request` is
every parameter the task reads, ε in definition units included. Dependencies
enter by the digest of what they produced, never by their own keys; that gives
early cutoff and makes Mokhov's "Frankenbuild" impossible. Thread count,
scheduling, GPU instance, cluster count, cache state, file names and entity
numbering are not in any key; they are invariants, tested (§3).

**Mixed backends** (§4): every entity read by work on more than one producer
(vertex, edge chain, cluster-seam vertex, carrier datum, feedback split) has
exactly one producer per run, chosen as a function of the entity's content key
and the declared placement, never of completion order; consumers read its
bytes by ID and never recompute them.

**Probe** (§6): 626,493 sampling decisions from 478 unique corpus files, five
arithmetic arms. Zero decisions changed between CPU and P100. Where the rule
uses only `+ − × ÷ √`, CPU and P100 agreed bit for bit (443,326 of 443,326
values) until contraction differed; libm transcendentals and `--fmad=true`
moved values by at most 2–4 ulp. The hazard is not random rounding but
**round CAD numbers**: 72 cylinder decisions sit exactly on an integer and 16
sit 1 ulp below one, because R = ε and R = 2ε occur in real files.

## 1. Terms

### 1.1 What "one backend" is

D-determinism says "per backend". The word needs a definition the code can
check, because results depend on more than the device.

- **Contraction is a compile-time choice.** nvcc and ptxas both default to
  `--fmad=true`, contracting multiply-add into FMA [src `nvcc --help`,
  `ptxas --help`, CUDA 12.8.93 on the fleet]. Operations written as
  `__dmul_rn`/`__dadd_rn` are never merged [src NVIDIA FP white paper §4.3].
  The fleet CPU has no FMA instruction (journal), so CPU and P100 differ in
  contraction whenever the GPU build contracts.
- **libm is not one thing.** CUDA's double `sin` is within 2 ulp; CPU and GPU
  libraries are independent and not correctly rounded, so "the results will
  often differ slightly" [src NVIDIA FP §5.1]. On the CPU side, glibc 2.39
  (the fleet's) selects its `asin`/`acos`, `sin`/`cos`, `atan`, `atan2`, `exp`,
  `log`, `pow`, `tan` and others at load time through IFUNC: an `_fma` variant,
  compiled with `-mfma -mavx2`, when both FMA and AVX2 are usable, else FMA4,
  else SSE2 [src glibc `sysdeps/x86_64/fpu/multiarch/ifunc-fma4.h`,
  `e_asin-fma.c`, `Makefile` `CFLAGS-e_asin-fma.c = -mfma -mavx2`,
  release/2.39]. The same binary therefore runs different libm code on the
  fleet (Ivy Bridge: SSE2) and on an AVX2 host [inf: the bits can differ; not
  measured here].
- **Mature libraries scope reproducibility the same way.** cuBLAS: bit-wise
  identical results "from a given toolkit version" on GPUs "with the same
  architecture" and "the same number of SMs"; not across toolkit versions; not
  with multiple active streams or with atomics mode [src cuBLAS docs §2.1.4].
  oneMKL CNR: "the number of threads is fixed and constant", per instruction-set
  branch [src oneMKL developer guide, Reproducibility Conditions].
- **Our CUDA build ships SASS only.** `CUDA_ARCHITECTURES 60-real`
  [code `native/mesher/CMakeLists.txt:162,171`]: no PTX, so no driver JIT, so
  the driver version cannot change device arithmetic.

**Definition [own].** `backend_id` = (device class, target architecture, digest
of the exact host binary and device fatbin, the arithmetic dispatch path taken
at run time, and the libm identity and IFUNC level for every libm function an
output-affecting kernel calls). Two runs are on "one backend" exactly when
their `backend_id`s are equal. Two consequences:

1. The thread count, the SM count, the GPU instance (GPU0 or GPU1) and the
   NUMA node are **not** part of `backend_id`. Our kernels must make them
   irrelevant (§2), unlike cuBLAS, which must name the SM count.
2. Two ways keep a CPU build's bits host-independent: own every transcendental
   used in sizing or placement (fast-tessellation §6.3 item 2 already
   recommends it), or put the glibc identity and IFUNC level in `backend_id`.
   Owning them makes `backend_id` small and lets one CPU build share a cache
   across hosts. Recommended [own]; the cost is a handful of polynomial
   kernels for `acos`, `asin`, `sin`, `cos`, `atan2`.

### 1.2 The four classes

- **D0 canonical.** Bytes identical on every backend and every thread count.
  Produced once on a fixed host path from the source, or integer-only. A
  difference anywhere is a bug.
- **D1 backend-reproducible.** For one `backend_id`: bytes identical across
  thread counts, runs, launch configurations, batch compositions, GPU
  instances, cache state and incremental-versus-fresh paths. Across backends:
  functionally identical (§3.2). **D1 shared** marks a D1 entity read by work
  on more than one producer; it obeys the one-producer rule (§4).
- **D2 derived.** A pure function of D0/D1 bytes computed on a fixed host path.
  Identical whenever its inputs are.
- **D3 non-reproducible.** Diagnostics that depend on timing. Stored in their
  own record section; excluded from output bytes, keys and equality oracles.
  Provisional meshes are D3: D-provisional makes them display-only, physics
  never consumes them, and they are never cached as final.

The classes follow Blelloch et al.'s distinction: internal determinism
(every intermediate value fixed for a given input) gives external determinism
(fixed output) and lets any sequential traversal of the dependence graph stand
for every parallel schedule [src Blelloch–Fineman–Gibbons–Shun 2012 §1–2,
Def. 1, Thm 1]. D1 asks for internal determinism of every output-affecting
value per backend; D3 is where external nondeterminism is allowed to live.

## 2. Question 1: bit-identical layers and thread-count invariance

### 2.1 Which layers

Under D-determinism every layer L1–L10 is bit-identical per backend (the table
in §0). The question the stub asked, "the edge-sample layer only?", has a
different answer once one model may mix backends: the edge layer does **not**
need cross-backend identity, because one producer computes each chain and every
consumer reads those bytes (§4). Making the edge layer cross-backend identical
would need owned transcendentals, a fixed contraction policy on every device,
identical operation order and identical iteration counts in every Newton solve
[src NVIDIA FP §5.4; fast-tessellation §6.3]. The probe shows the arithmetic
part is reachable for `+ − × ÷ √` (§6), but iterative solves (R3 two-surface
Newton, V3 point solve, seeded projection) amplify ulp differences into
different iteration counts [inf]. The one-producer rule gets watertightness and
cache soundness without that cost. Cross-backend identity stays a non-goal
(operator).

Only L1 is identical across backends, and only because nothing but the host
ingress produces it.

### 2.2 Mechanisms, cheapest first

Each mechanism below names the fault it removes. The repository already
achieves byte-identical CPU1/CPU4/P100 artifacts with completions sorted by
key [code `native/mesher/adaptive_component.cpp:84-87`; owned-mesher-audit §4],
so these are known to work in this code base; what changes is that FMA and
cross-backend agreement are released.

1. **Pure items.** A work item's output is a function of its declared input
   bundle only: no shared mutable state, no reads of other items' partial
   results, no clock, no global RNG. Cost: none. This is Blelloch's
   "commuting operations" requirement in its strictest, cheapest form [src
   Blelloch 2012 §2, Thm 1].
2. **Batch-composition invariance.** An item's arithmetic must not depend on
   its lane, its batch's size or its neighbours: SIMD tails run the same
   instruction sequence as the body (masked lanes, not a scalar remainder loop
   compiled differently); no cross-lane operations inside per-item results.
   This matters because the design batches by SoA class for SIMD lanes and
   warps (design §6.2) and the background healer forms batches by timing
   (design §7) [own]. Cost: small; masked tails.
3. **Decomposition fixed by data, never by P.** Any decomposition that
   influences results (reduction trees, prefix sizes, cluster splits) is a
   function of the input size and item keys, not of the thread count, SM count
   or grid size. Blelloch's deterministic reservations are internally
   deterministic only "for a fixed schedule of prefix sizes" [src Blelloch
   2012 §5.3], and the GPU reorganizes a reduction when the threads per block
   change [src NVIDIA FP §5.3]. Their reduce and scan stay deterministic for
   non-associative floating-point addition because the tree is fixed [src
   Blelloch 2012 §5.1]. Cost: none at our sizes.
4. **Canonical publication.** Results land in positions fixed by key: either a
   sort by key per wave (current code) or, cheaper, an exclusive scan of
   per-item counts that assigns each item its output range before it writes
   (design §6.2 already plans scan-sized output). Cost: one scan.
5. **No floating-point atomics; no position-allocating atomics.** `atomicAdd`
   on doubles is order-dependent; PyTorch, for example, refuses `index_add_` on
   CUDA under its deterministic mode [src PyTorch randomness notes, 2.14].
   Integer atomics are safe for counts (commutative), not for allocating slots,
   because slot order then follows arrival order. The current topology fill
   allocates slots with `atomicAdd` and documents row order as unspecified
   [code `native/topology_cuda.cu:109`; `docs/ADJACENCY.md:61`]; that is legal
   under its own contract but cannot be carried into an L5/L6 producer (listed
   under defects). Use scan plus a stable fill, or a per-row sort by coedge
   slot.
6. **Exactly associative reductions only.** `min`, `max` and integer sums are
   associative and commutative, so any order gives the same bits (with NaN
   refused at ingress and signed zero canonicalized before `fmax`). Every
   decision quantity is formed from these: bounding boxes, maximum errors,
   integer cell counts. A floating-point sum that feeds a decision either uses
   a tree fixed by item keys (item 3) or is replaced. One exists in the draft:
   "a requested ε that implies an absurd triangle count is refused up front
   from the work estimate" with the estimate from ∫√det M (design §5.5). Sum
   integer per-cell counts instead [own]. Where a true floating-point sum must
   be order-independent across decompositions, ReproBLAS-style binned summation
   does it in one read-only pass and about 9n flops, at under 1.2× cost on large
   parallel sums [src ReproBLAS project page; Demmel–Nguyen 2015, not read in
   full]. No such sum was found in the design's decision path.
7. **Seeded choices.** Random choices (randomized incremental CDT, tie-break
   perturbations) take their seed from the run seed plus the item's content
   key, never the worker number [src whole-mesher-native-runtime item 7]. With
   a fixed random order, a parallel randomized incremental algorithm that
   respects the iteration dependence graph does "the same computation as in
   the sequential order" [src Blelloch–Gu–Shun–Sun 2018 Def. 1], which keeps a
   future intra-face parallel CDT inside D1.
8. **No output-affecting iteration over hash or pointer-ordered containers.**
   build123d chose a merged vertex by memory address, so meshes differed
   between processes [src mesher-test-plans-cad §10, build123d #1492]. CGAL
   tests determinism under a randomizing allocator for this reason
   [src mesher-test-plans-meshgen, `test_mesh_terrain.cpp`].
9. **Fixed arithmetic per build.** Contraction policy, `-ffast-math` absence,
   FTZ off and the libm path are fixed by the build and named in `backend_id`.
   FMA is allowed (operator), and determinism survives because each build
   contracts the same way every run. No runtime ISA dispatch inside
   output-affecting code unless the dispatch path is in `backend_id`.
10. **Tie-breaks by content, not by entity number.** The draft breaks R4 owner
    ties by "lower ID" and M2 breaks reconciliation ties "by source identity"
    (design §5.2; neighbour-reconciliation §4). Entity numbers change when a
    host renumbers a file, so an ID tie-break makes geometry depend on
    numbering and forces numbering into every key. Break ties by content
    digest; if two candidates' content is identical, either choice gives the
    same bytes [own]. This keeps keys free of identity (§5.4).

Cost evidence: Blelloch et al.'s internally deterministic versions of their
benchmark suite ran at speedups of about 12–32× on 32 cores with
hyper-threading; on one thread the
deterministic Delaunay triangulation was about 1.4× slower than the tuned
serial code, and deterministic MIS beat the nondeterministic one [src Blelloch
2012 §6, Tables 2–3]. The literature does not give the cost of items 2–5 on
P100 for our kernels; the arena measures it (§7).

## 3. Question 2: "functionally identical" as an oracle

This section is self-contained for G12.

### 3.1 Per-backend oracles decide validity

Functional identity is not a comparison first. Each backend's output passes
the full validity stack on its own: the sound two-sided deviation oracle
against the repaired target, per-face UV oracles, shared-edge identity and use
counts, closed-shell Euler, orientation, accounting, self-intersection
offline, export re-read (design §10). Those are property and specification
oracles (tiers 5 and 7, BLACKBIRD). A backend that passes them is correct. The
cross-backend comparison adds only what they cannot see.

### 3.2 Equality oracles, by class

- **O-bytes (D0, D1, D2).** Run the same input on one `backend_id` and compare
  output bytes (D3 section excluded) under each of these perturbations; any
  difference fails:
  - thread counts 1, 2, an odd count, 12, 24, 48;
  - schedule perturbation: randomized task order and injected delays, so
    interleavings actually differ;
  - GPU launch shape: block size, grid size and stream count varied;
  - GPU0 versus GPU1;
  - batch composition: items shuffled across SIMD and warp batches, batch sizes
    varied;
  - cold versus warm cache (cache transparency, §5.6);
  - incremental edit versus fresh run of the edited input (G12; §5.7);
  - D0 only: every backend.

  Each run records the worker count and launch shape it actually used, and the
  input must exceed every parallel threshold; otherwise the test cannot fail
  (N4 below).
- **FI, functional identity across backends.** Both outputs already pass §3.1.
  Then:
  - **FI-1 accounting.** The same definitions, faces and occurrences appear,
    counted from the emitted output, each with a status.
  - **FI-2 attributable status and class differences.** Each face status and
    each edge R-class and V-class agree, or the recorded deciding quantity of
    both backends lies within that quantity's cross-backend error bound of its
    threshold. The checker recomputes the quantity in higher precision for every
    disagreement and fails it if the true value is farther from the threshold
    than the bound. An unattributable flip means one backend computed the wrong
    thing.
  - **FI-3 topological invariants.** For every shell, from the mesh welded by
    vertex ID: Euler characteristic, boundary loop count, connected
    components, and the use count of every source edge. These come from the
    B-rep, not from sampling density, so backends must agree exactly.
  - **FI-4 per-edge chains.** Same end vertex IDs, monotone parameters. If the
    sample counts are equal, corresponding positions agree within the derived
    forward-error bound of the producer kernel (below). If they differ, the
    decision that split them passes FI-2.
  - **FI-5 differential agreement on shared keys.** Where both backends emitted
    the same key set (same lattice cell keys, same dyadic paths), positions and
    normals agree within each kernel's forward-error bound, c·u·S, with u the
    unit roundoff, S the recentred coordinate scale and c derived from the
    kernel's operation count. The probe's observed spread is 2–4 ulp per scalar
    (§6); the bound is derived, not fitted to that. This catches a stray FP32
    operation, a missing recentring or a wrong formula that is still inside ε
    on the test input but would not be elsewhere.

### 3.3 Comparisons that can never fail, and must not be used

1. **N1 cross-backend Hausdorff ≤ 2ε** when each backend already passed the
   deviation oracle: the triangle inequality implies it. A second oracle of
   the same kind (BLACKBIRD tenet 5).
2. **N2 watertightness or manifoldness checked after welding by position**
   with a tolerance. It cannot see a mismatch smaller than the weld
   tolerance; mature suites warn of exactly this [src
   mesher-test-plans-meshgen lines 81, 396, 1743]. Check by vertex ID.
3. **N3 counts read from the shared decode.** "Both backends report 9,162
   faces" is a fact about the ingress, which both share. Count emitted faces.
4. **N4 determinism runs that do not vary concurrency.** CGAL Mesh_3 tests its
   parallel determinism only with one thread [src mesher-test-plans-meshgen
   line 363]; small inputs below a parallel threshold run serially whatever
   the requested count. The run must report the worker count and launch shape
   it actually used.
5. **N5 equality between two runs that read the same cache entries.** It
   compares an artifact with itself. Determinism runs are cold, or are the
   explicit cold-versus-warm transparency test.
6. **N6 validity against the mesher's own reported achieved error.**
   Self-certification. The checker computes deviation itself.
7. **N7 statuses compared through a shared record** rather than recomputed per
   backend.

One comparison can fail but is the wrong oracle: **cross-backend byte
equality of D1 layers**. It fails with no bug (§6 shows 23% of cylinder sizing
values differ in the last bits). Never a pass/fail criterion; FI-4 and FI-5
replace it. Regression bands recorded from earlier runs ("64 ≤ n ≤ 72",
CGAL Mesh_2) are tier-3 change detectors, not functional identity, and goldens
never substitute for a bound [src mesher-test-plans-meshgen; -cad H3].

### 3.4 Tolerances

- Deviation: ε, the request, against the repaired target (design §4).
- FI-2: the cross-backend error bound of each deciding quantity, from its
  kernel's forward error.
- FI-4/FI-5: c·u·S per kernel.
- No other tolerance appears. In particular no "close enough" weld radius.

## 4. Question 3: mixing backends in one model

**Rule [own, from src].** For every entity read by work on more than one
producer, exactly one producer computes it in a run. It finishes before its
consumers start (or, under the speculative variant, publishes a provisional
version the consumers record). Every consumer reads its bytes by ID and never
recomputes them. The producer of an entity is a function of the entity's
content key and the run's declared placement, never of completion order.

The basis: every working persistent-mesh system discretizes each physical edge
once, gives each sample a global identity and has every incident face consume
those vertices; with ID sharing, watertightness does not depend on
floating-point reproducibility [src fast-tessellation §3; Kumar–Manocha
ownership of each shared boundary]. The lifecycle map warned that CPU and GPU
estimates near a threshold choose different sample sets, so independently
produced boundaries would not match [src map S17].

Corollaries:

1. **The edge producer owns the whole chain.** Class, resolved positions,
   sample parameters and dyadic paths, XYZ, vertex IDs, and one UV per face
   occurrence (both occurrences of a seam). It also computes the edge's sample
   count from every incident face's admissible length (design §5.2 item 1)
   from L2 carrier bytes, not from face-task results.
2. **Faces never re-evaluate boundary XYZ.** A face may compute
   occurrence-local data from the shared UV (normals, chart orientation); the
   emitted boundary position is the chain's.
3. **Vertices.** One producer per model vertex; edge producers read its
   result. Degenerate-edge aliases stay explicit
   [code `native/mesher/boundary.hpp` `SourceVertexAlias`].
4. **Face-to-edge feedback (G2) crosses backends as integers.** A face asks
   for a split as a dyadic path key on the edge's root interval, which the
   boundary forest already stores [code `native/mesher/boundary.hpp:114-115`
   `sample_paths`, `path_bits`]. The edge producer applies the sorted union of
   requests. Two faces on different backends asking for "the midpoint" ask for
   the same key; float parameters never cross a backend boundary.
5. **Cluster seams inside a face** (design §5.4) are keyed by integer lattice
   coordinates, so identity agrees; positions agree only if one producer writes
   them. Either the seam vertex is owned (lowest cluster key wins), or all
   clusters of one face stay on one backend. The second is simpler and is the
   recommended alpha default [own].
6. **Placement is a function of the item.** The background healer's batches
   form by timing (design §7). If batch size chooses CPU or GPU, the producer
   of an edge depends on timing, and the output changes run to run. Place by
   item, then batch.

**Hypothesis H3.** A run with random per-entity placement over CPU, GPU0 and
GPU1 (several seeds) emits no vertex ID with two positions and no shared edge
whose two uses reference different ID sequences, and each face's bytes equal
those its producer emits in a single-backend run given the same chain bytes.
**Falsifier:** any duplicate-position ID, ID-sequence mismatch, or face whose
bytes depend on another face's backend.

**Open (operator, §9 Q1).** D-determinism does not say whether a mixed run
must be byte-reproducible run to run. It is if placement is a deterministic
function of input, configuration and device inventory (static, HEFT-style
from a fixed cost model). It is not if placement reacts to load
(work stealing between CPU and GPU), though each face stays valid and the
rule above keeps the model watertight. The cache key (§5) and every oracle in
§3 work under either answer; O-bytes for mixed runs requires the first.

## 5. Question 4: cache keys and early cutoff

This section is self-contained for G12.

### 5.1 What the build-systems literature establishes

- A build is **correct** when every non-input key's stored value equals what
  recomputing its task over the final store would give [src Mokhov–Mitchell–
  Peyton Jones 2018 §3.6, Def. 3.1].
- **Verifying traces** record, per key, the hashes of the dependency values it
  was built from, and rebuild when any differs. **Constructive traces**
  also store the value, so a cache can supply it. Both support early cutoff:
  when a rebuilt value equals its previous value, dependents are not rebuilt
  [src §2.3, §4.2.2–4.2.3, Fig. 3].
- **Deep constructive traces** key on terminal inputs only, skipping
  intermediate values. With more than one level skipped they "require the
  tasks to be deterministic", and they lose early cutoff [src §4.2.4].
- **Non-determinism** weakens correctness to "the result contains one possible
  result of recomputing the task"; combining deep traces with a
  non-deterministic task yields a **Frankenbuild**, a stored target
  inconsistent with its stored dependency [src §6.3, §6.4, Fig. 11].
- **Untracked dependencies**: "none [of the sandboxing systems] are likely to
  capture all dependencies right down to the CPU model and microcode version"
  [src §6.3]. Bazel says the same in practice: tools outside the workspace,
  such as a system compiler, are not tracked, and two machines can share a hit
  for different outputs [src Bazel remote caching docs].
- **Content-addressed outputs** let a changed task that produced the same
  output stop the rebuild [src Nix manual, ca-derivations]. **Identity versus
  content**: "cache keys contain zero identity fragments; identity contains
  zero geometry" [src brepjs ADR-0014 via lifecycle map S17]. OCCT's shape hash
  is pointer plus location, not persistent [src map S17].

### 5.2 The key

```
key(task) = SHA-256(
    key_schema_version                       -- bumps when this layout changes
  ‖ task_kind                                -- carrier | thin_features | vertex | edge | face | publish | export
  ‖ backend_id                               -- §1.1; the producer's, for this task
  ‖ request                                  -- every parameter the task reads (below)
  ‖ digest(own source records)               -- decoded bytes, raw IEEE bits
  ‖ sorted by role: role ‖ digest(OUTPUT bytes of each consumed dependency) )
```

- **Digest function.** SHA-256, already owned in the code base
  [code `native/step_source/main.cpp:125-127`, used on original input bytes]. A 256-bit cryptographic
  digest makes a collision not a practical failure mode; brepjs moved off
  `HashCode()` after collisions [src map S16].
- **Canonical bytes = the bytes the producer reads.** Little-endian; doubles as
  raw IEEE bits. No normalization: if the key merged −0.0 with +0.0, or two
  NaN payloads, it would merge inputs the code can tell apart, and a hit would
  be stale. Non-finite values are refused at ingress, so they never reach a
  key.
- **`request`.** ε **in definition units** (so a uniformly scaled placement,
  meshed at ε/s per D-placements, gets its own key automatically, and rigid or
  mirrored placements share one [src map finding 4]); η, β, G and the
  ¼-local-feature-size factor; angular and maximum-length limits; the
  data-quality profile thresholds the task reads; the export format and its
  parameters; every arena flag the task reads. A flag the task does not read
  is not in its key; that is a claim, and the completeness test (§5.6)
  checks it.
- **Dependencies by output digest, never by key.** A face key contains the
  digest of each chain's bytes, not the edge's key. So:
  - **Early cutoff.** An edge recomputed after an edit that produces the same
    bytes leaves every incident face key unchanged; the faces are reused.
  - **No Frankenbuild.** A face built against chain bytes A can only be hit
    when the run's chain is A. Two faces of one edge can never be served
    against two different variants of that edge, which in a mesh is a crack.
- **Per task kind, the inputs** [own, from design §5 and map S17 field list]:

  | Task | Own records | Consumed outputs |
  |---|---|---|
  | carrier | surface or curve record: kind, parameters, knots, control points, weights, frame | — |
  | thin_features | — | carrier outputs of every face of the definition; face bounds |
  | vertex | point record | carrier outputs of incident edges' curves and faces |
  | edge | curve record and range; each coedge's pcurve record and sense | carrier outputs of every incident face; both vertex outputs; thin-feature output for the edge; the sorted set of split-request keys (G2) |
  | face | face sense; ordered loop structure (coedge order, senses, seam pairing) | carrier output; every chain output in loop order; small-feature decisions; thin-feature output for the face |
  | publish | — | every face output of the definition, in canonical key order; placement table |
  | export | — | publish output; format and its parameters |

- **Excluded, because they are invariants tested by O-bytes:** thread count,
  worker IDs, scheduling and timing, NUMA node, GPU instance of one
  architecture, batch composition, intra-face cluster count K (design §5.4
  promises mesh(K=1) ≡ mesh(K=k)), cache state.
- **Excluded, because they are identity, not content:** file path and name,
  STEP entity numbers, product names, colours. They live in a separate
  **identity binding** layer that maps host IDs to content digests (D2, cheap,
  always recomputed). This lets a renumbered or re-exported file reuse every
  face, and it requires tie-breaks by content (§2.2 item 10). If M5 keeps ID
  tie-breaks, the entity numbers involved must enter the keys of the affected
  tasks.

### 5.3 Why a stale reuse is impossible

A hit is stale only if the producer, given the stored key's inputs, would now
produce different bytes. That requires one of:

1. An input the task read that is not in the key. Closed by construction: a
   task receives one immutable input bundle, and the key is computed from that
   bundle. No globals, environment variables, files, clocks or shared caches are
   reachable from task code (Bazel's hermeticity, Mokhov's untracked
   dependencies) [src].
2. A change in the code or arithmetic. Closed by `backend_id`, which contains
   the binary and fatbin digests and the libm path (§1.1). A version string
   bumped by hand is not used, because a forgotten bump is a stale hit [own].
   The cost is that every rebuild starts cold; per-kernel digests can narrow
   that later if measurement says it matters.
3. Nondeterminism in the producer. Closed by D1 per `backend_id`.
4. A digest collision. SHA-256.

### 5.4 Deep keys and cross-backend reuse

- Keys use verifying traces over immediate dependencies (§5.2), which keeps
  early cutoff. A deep key over terminal inputs is allowed only within one
  `backend_id`, where tasks are deterministic [src Mokhov §4.2.4].
- **Cross-backend reuse is off by default [own].** A CPU run could use a chain
  the GPU produced, because any producer's chain is valid. But then a warm run
  would emit different bytes from a cold run on the same backend, breaking
  cache transparency (§5.6) and D-determinism in spirit. If wanted later, it is
  a declared placement ("this edge's producer is GPU0's cached result"), keyed
  with that producer's `backend_id`, and O-bytes then compares against cold
  runs with the same placement.

### 5.5 Storage

Two maps, as Bazel separates its action cache from its CAS: `key → output
digest` and `output digest → bytes`. Eviction removes bytes; a key whose bytes
are gone is a miss [src Bazel; Mokhov §6.4 "eviction"]. Native binary records
only, no JSON (BLACKBIRD internal data formats).

### 5.6 Oracles for the cache

- **Transparency (O-bytes).** Cold run and warm run on one `backend_id` emit
  identical bytes. This is Mokhov's Def. 3.1 made executable.
- **Key completeness, edit sensitivity.** For random single-input
  perturbations (one control point by 1 ulp, a knot multiplicity, ε by 1 ulp,
  one flag, one pcurve), a run warm from the pre-edit cache must equal a cold
  run of the edited input. Any difference names a missing key input.
- **Key minimality, edit insensitivity.** Renumber entities, rename the file,
  change thread count: every task must hit, and bytes must not change. A miss
  names an identity fragment in a key.

### 5.7 Incremental equals fresh (handed to G12)

Under D1 and content keys, incremental remeshing on one backend is the same
computation as a fresh run that happens to hit the cache, so **incremental
bytes equal fresh bytes** on one `backend_id`. That is stronger than the
lifecycle map's "only per face given identical boundary samples" [src map S16
contradiction (c)], which assumed no per-backend determinism. Three conditions
for G12 to keep:

1. Placement is a function of item content (§4 item 6, §9 Q1); otherwise the
   two runs must share one placement map.
2. **Face-to-edge feedback must be history-free.** A chain is a function of its
   current inputs, including the split requests of the faces that exist now.
   A chain that keeps splits requested by a face the edit deleted is
   hysteresis (OCCT's 1.1/0.9 reuse band is the documented case [src map S16])
   and breaks incremental = fresh. The split-request set therefore sits in the
   edge key (§5.2 table).
3. The speculative variant (design §7) emits different bytes from the
   non-speculative one; its flag is in the face key, and incremental = fresh
   holds per variant.

## 6. Question 5: measurement

### 6.1 Method

A small probe in the fleet lane directory; its source is summarized here.

- **Inputs.** `extract.py` read every STEP file under the corpus working
  copies (NIST MBE PMI, CAx-IF library, five open-hardware TDPs; corpus
  MANIFEST): 500 files, 22 byte-identical duplicates dropped by SHA-256, 478
  unique, 228,550 faces. For each unique carrier referenced by an
  `ADVANCED_FACE` it emitted the rule inputs as raw binary64. Faces by carrier:
  71,158 cylinder, 1,585 sphere, 5,004 torus, 19,853 polynomial B-spline (cut
  into 105,835 Bézier patches by Piegl–Tiller knot insertion, checked against
  de Boor evaluation to 4e-14 on 300 random curves); 18,527 cone and 2,500
  rational B-spline faces were counted and not evaluated (cone ρ_max needs the
  trim extent; the rational rule needs Zheng–Sederberg). Planes have no
  interior rule.
- **Requests.** ε = 0.005 mm and 0.025 mm, converted through the length unit of
  the B-rep representation context (16 files had no unique one and got only
  the relative levels), and 1e-3 and 1e-4 of the bounding-box diagonal of all
  `CARTESIAN_POINT`s. Assemblies place points metres apart, so many relative
  requests exceed 2R and are degenerate; those are excluded, not counted.
- **Rules**, one source compiled for both devices (`rules.hpp`); each returns
  the real x whose `ceil` is the decision:
  - cylinder `x = π / acos(1 − ε/R)` (segments per circle);
  - sphere `ρ² = 2Rε − ε²`, `x = π / (2 asin(ρ/R))`;
  - torus `M = λmax([[R_c+r, r],[r, r]])`, `x = π √(M/ε)`;
  - Bézier patch, recentred on its box centre,
    `x = √((D_uu + 2D_uv + D_vv) / 8ε)` with the second-difference bounds of
    design §6.1.
- **Arms.** CPU Clang 18.1.3 `-O2 -ffp-contract=off`, glibc libm (SSE2 path;
  the CPU has no FMA): plain and explicit-`fma()` source. P100 CUDA 12.8.1
  `sm_60`: plain source with `--fmad=false` (GPU0 and GPU1), plain with
  `--fmad=true` (GPU0), explicit-`fma()` with `--fmad=false` (GPU0).
- **Context.** CPU and GPU arms, not a timing study. Load average 7.3 at the
  start and 16.1 at the end, with other lanes running; governor `schedutil`;
  SMT on; AutoNUMA on; both GPUs idle at the start; a single cold run. Timing
  context does not affect the values compared.

### 6.2 Results

Counts are decisions (carrier × request). "bits" = values whose binary64 bits
differ; "ulp" = largest difference; "flip" = `ceil` differs.

| Rule | Decisions | CPU plain vs P100 `--fmad=false` | CPU plain vs P100 `--fmad=true` | CPU `fma()` vs P100 `fma()` | P100 `fmad` false vs true | Flips, any pair |
|---|---:|---|---|---|---|---:|
| cylinder | 178,877 | 42,013 bits, ≤2 ulp | 42,013, ≤2 | 42,013, ≤2 | 0 | 0 |
| sphere | 4,290 | 1,831, ≤2 | 2,028, ≤4 | 1,756, ≤2 | 652, ≤4 | 0 |
| torus | 19,986 | **0** | 11, ≤2 | **0** | 11, ≤2 | 0 |
| Bézier | 423,340 | **0** | 26,140 (6.2%), ≤2 | **0** | 26,140, ≤2 | 0 |

GPU0 and GPU1 outputs were byte-identical for every rule.

Margin to the nearest integer, CPU plain arm, all rules: 626,493 decisions;
88 at or within 4 ulp of an integer, 79 more within 2¹⁰ ulp, 152 within 2²⁰,
all others beyond 2³⁰ ulp. All of the near ones are cylinders. (A further 228
Bézier decisions are exactly 0: flat or linear patches, where B = 0 on every
arm.)

| Count | R in file | ε | x (CPU) | Why |
|---:|---|---|---|---|
| 72 | 0.005 | 0.005 | exactly 2 | R = ε, so acos(0) = π/2 |
| 60 | 0.00500000000000001 | 0.005 | 2 + 5 ulp | R = ε to file precision |
| 144 | 0.005000000000005 | 0.005 | 2 + 2,867 ulp | |
| 16 | 0.049999999999999996 | 0.025 | **3 − 1 ulp** | R = 2ε, acos(½) = π/3 |
| 27 | 0.0499999999999477 … 0.050000000000005 | 0.025 | 3 ± 6 … 3,896 ulp | R = 2ε to file precision |

### 6.3 What it shows

1. **No decision changed** between CPU and P100 on this corpus, with or
   without FMA, for these four closed forms [meas]. That answers question 5 as
   asked.
2. **Where only `+ − × ÷ √` are used, CPU and P100 agree bit for bit** when
   contraction is equal: plain/plain with `--fmad=false`, and `fma()`/`fma()`
   (443,326 torus and Bézier values, zero differences) [meas]. Every value
   difference comes from either libm transcendentals (cylinder, sphere) or
   compiler contraction (`--fmad=true`). This is the white paper's account
   measured on our rules [src NVIDIA FP §5.4].
3. **The tail is not random rounding; it is round numbers.** Designers choose
   R = 0.005 or 0.05 mm, and users request ε = 0.005 or 0.025 mm. The cylinder
   rule has exact algebraic values at ε/R = 1 and ½, so real decisions sit
   exactly on, or 1 ulp from, an integer. Sixteen corpus decisions computed
   2.9999999999999996 where the exact value is 3; a libm that rounded acos(½)
   the other way would emit four segments instead of three. Both libms happened
   to agree. The generic estimate "a flip needs x within a few ulp of an integer,
   probability about 1e-15 per decision" is wrong for CAD data [meas, own].
4. **Consequence for the design.** It does not affect correctness (either count
   meets ε when x is exactly an integer; a mathematically exact x = n means n
   segments achieve exactly ε). It affects cross-backend comparison noise
   (FI-2) and any future cross-backend reuse. It confirms §4: edge chains must
   have one producer, because a 1-ulp acos difference can change a sample count
   on real files.

**Proposal H5 (optional).** Evaluate decisions as `n = ceil(x · (1 + 2⁻⁴⁰))`.
This moves every threshold off the exact algebraic points that round CAD
numbers hit, adds at most one sample exactly where x is an integer (more
samples, so ε still holds), and leaves generic decisions unchanged.
**Hypothesis:** rerunning this probe with the offset gives a minimum margin
above 2²⁰ ulp of the new thresholds on every rule. **Falsifier:** any decision
within 2²⁰ ulp of a shifted threshold, or any cross-backend flip.

## 7. What the literature does not say, and the measurement that would

1. **Flip rates for iterated decisions.** No source measures CPU/GPU
   disagreement for adaptive refinement, Newton projection iteration counts or
   plain-arithmetic CDT predicates in a mesher. The probe covers closed forms
   only. Measurement: after M8, run the new edge and face producers on CPU and
   P100 over the corpus and record per-edge sample-count and per-face
   triangle-count differences with FI-2 attribution.
2. **Cost of D1 mechanisms on P100.** Blelloch et al. measured CPUs. The cost of
   scan-placed output instead of atomic append, masked SIMD tails and fixed
   reduction trees in our kernels is unmeasured. Measurement: arena pair with
   and without each mechanism, outputs compared by O-bytes.
3. **Whether glibc's FMA path changes bits for our arguments.** Inferred from
   glibc's build flags, not measured. Measurement: run the probe's CPU arm on an
   AVX2+FMA host (RunPod) and compare with the fleet bits. This decides whether
   owning transcendentals is required or merely tidier.
4. **Tessellation cache keys in mature meshers.** The map found OCCT's
   parameter-only reuse and brepjs's ADR; no published account of a
   content-keyed tessellation cache with early cutoff was found.

## 8. Consequences for M5

Proposed determinism statement:

> For one `backend_id`, output bytes are identical across thread counts, runs,
> launch configurations, batch compositions, GPU instances, cache state and
> incremental or fresh remeshing. Across backends, outputs are functionally
> identical: each passes the full validity stack, and the FI oracles hold. One
> model may mix backends; every shared entity has exactly one producer per
> run. Diagnostics (D3) are excluded from these guarantees.

Draft contradictions to fix in the rewrite:

- §2 non-goal "bit-identical output across CPU and GPU, **or across runs**":
  D-determinism requires identity across runs and thread counts per backend.
- §5.1 "cache keyed by source identity": key by content (§5.2).
- §5.2 R4 owner order "then lower ID": tie-break by content digest (§2.2
  item 10), or put the IDs in the keys.
- §5.5 absurd-output refusal from a floating-point sum: use integer counts.
- §7 healer batches formed by timing: batch-composition invariance and
  placement by item (§2.2 item 2, §4 item 6).
- §10 "validity across thread counts, backends and repeated runs": per backend,
  byte equality (O-bytes); across backends, FI.
- `-ffp-contract=off`/`--fmad=false` in the build profile (map §3.3): the
  operator released them; once changed, contraction becomes part of
  `backend_id`, nothing else.

## 9. Return items

### Class per layer and cache key

The §0 table and §5.2.

### Defects found

1. `native/topology_cuda.cu:109` (`fill_uses`): CSR slot order comes from
   `atomicAdd` arrival order, so the `members` array differs run to run on CUDA.
   `docs/ADJACENCY.md:61` documents "row order is unspecified", so it is not a
   bug under that unit's own contract, but it violates D-determinism if any new
   output layer carries it (L1 use counts are fine; slot order is not). Replace
   with a stable fill (scan, then a per-row sort by coedge slot) when this code
   is reused. The same pattern in `native/om10/cuda.cu:221` bins display
   triangles; visibility then resolves by depth, so its images may still be
   deterministic; not checked.
2. No other code defect was found; this lane read code only where cited.

### Operator questions

1. **Mixed-backend runs: byte-reproducible run to run, or only functionally
   identical?** Byte-reproducible needs placement to be a deterministic
   function of input, configuration and device inventory (static placement
   from a fixed cost model). Dynamic, load-reactive placement (CPU/GPU work
   stealing) is faster to schedule but makes mixed-run bytes vary. Everything
   else in this paper works either way. This lane does not settle it; G8 and
   G12 depend on the answer.

### Shopping items

None required. Demmel and Nguyen, "Parallel reproducible summation", IEEE TC
2015, was cited through the ReproBLAS project page only; the paper was not
read and no decision here depends on it.

## For SHOPPING_LIST.md

- Demmel J., Nguyen H. D., *Parallel Reproducible Summation*, IEEE
  Transactions on Computers 64(7), 2015 (not acquired; optional, cited through
  the ReproBLAS project page).
