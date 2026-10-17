# Fast mesher direction and research lanes

New shift. Read docs/HANDOFF_2026-10-17.md and the recorded whole-watch phase
files. No geometry, build or benchmark ran; laptop work was reading only.

## What the recorded phases already show

- Owned meshing is 847.712s CPU12 / 845.785s P1001. NVIDIA preparation plus
  SDK batch, which contains its meshing, is 6.130/7.636s. The meshing gap is
  therefore at least ~110x, not the ~20x end-to-end ratio (875 vs 44s).
- adaptive_model_main.cpp:398 meshes the 107 definitions one at a time; the
  per-definition complete_component times sum exactly to complete_meshing.
  Parallelism exists only inside a component (NativeScheduler). NVIDIA runs
  12 worker processes across definitions.
- Stage sums: faces 541s, contacts 165s, boundary 140s. Definition 1713 alone
  is 139s; the 12 slowest definitions are ~414s.
- The P100 does not touch the meshing cost; it serves bounds and display.

## Operator decisions

- Goal: beat NVIDIA USD/BRep at every stage; meshing first. Speed is second
  only to correctness, and correctness without speed does not matter.
- Correctness means fidelity sufficient for people to send the output through
  downstream processing into machines that cut or form real material. Dead
  serious, not aerospace-insane, and visually stunning.
- Certification is excised from the repository, not kept as an opt-in mode.
  (An earlier statement in this shift said opt-in; the operator corrected it
  once the term was clear.) Runtime proof machinery goes: exact rational
  acceptance, interval enclosures, per-facet proof obligations, exact
  self-intersection checks in the hot path. In the operator's words, "the way
  we certify is by understanding how to correctly implement geometry in our
  code." Correctness comes from correct geometry in the algorithm and strong
  test oracles.
- Toolchain: P100s are the only owned GPUs, so a C++20 device lane stays
  (CUDA 12.x, sm_60). Add a C++26 lane for newer GPUs tested on RunPod;
  Apple Silicon follows. Compiler family is Clang, per the verbs repository
  (design/SPEC.md section 8: upstream LLVM 23 with -std=c++2c; reflection and
  contracts absent upstream; Mull stops at LLVM 22).
- Sequence: meshing performance first, toolchain split second, Apple third.
- Watch critical path, for perspective only: stage 0 fast faithful render,
  then animated explosion, correct mechanism physics, explosion while running,
  and a horological complication as the alpha showpiece. Meshing design must
  not box these in (contact fidelity, incremental remeshing of changed parts).

Recorded in-session note: the current code is not uniformly C++17. The mesher
is C++20 via tooling/native_cpu_rigor.cmake (pinned Clang 18.1.3, libstdc++,
-Werror) and already uses std::jthread and std::pmr; the root library,
topology, om10, step_source, reference tools and CUDA modules are C++17.

## Research lanes launched

Three independent lanes, each writing one report to papers/:

- NVIDIA SmTess/NL_Tessellate end to end, including where its chord estimate
  can understate deviation: papers/2026-10-17-nvidia-smtess-study.md.
- Honest audit of the owned mesher's cost structure and which machinery is
  certification versus construction: papers/2026-10-17-owned-mesher-audit.md.
- Literature on cheap non-understating error bounds, GPU trimmed tessellation,
  crack-free shared edges, parameter-space CDT, and manufacturing mesh
  requirements: papers/2026-10-17-fast-tessellation-literature.md.

Design follows synthesis of the three; no implementation unit is selected.

## Later operator decisions this shift

- Certification removal and the new tessellator are one unit: the new path
  chooses density from carrier geometry and replaces each proof-gated decision
  as it lands. A deletion pass first would leave a mesher with no stopping rule.
- Everything about current output is fair game (facet order, IDs, on-disk
  format), provided the mesher becomes extremely fast at correctly meshing
  arbitrary inputs. Shared-edge, seam and history guarantees stay.
- No overfitting to the watch. The watch is one benchmark; design and
  measurement must cover broad independent inputs.
- Coverage bootstrap accepted: ABC sample (speed) and larger sweep
  (robustness), NIST/CAx-IF STEP fixtures, OCCT meshing regression shapes,
  authored adversarial families, a mutation fuzzer over STEP and B-rep
  parameters, and the watch. Every input is timed against NVIDIA and checked
  against exact geometry offline; refusals and failures are retained.
- ABC's original pipeline filtered out models needing shape healing and
  over-refined meshes (Koch et al. 2019), so it is real-world sloppy, not
  adversarial. Pathology handling is designed in: non-finite and malformed
  values refused at ingress, every loop bounded by geometry-derived limits,
  budgets as a reported backstop, never a silent accept.
- Open, not answered by the operator: whether a garbage face refuses the whole
  part or the rest meshes with that face reported. Design default (my call):
  mesh the rest, mark the part incomplete, and report the face explicitly.
- Two more lanes study the full test plans of mature public meshers as a
  backstop against blindness: CAD/B-rep tessellators
  (papers/2026-10-17-mesher-test-plans-cad.md) and mesh-generation and
  robustness libraries (papers/2026-10-17-mesher-test-plans-meshgen.md).
- Lean into SIMD where appropriate. Design must lay out batched work so CPU
  SIMD lanes and GPU warps consume the same structure-of-arrays batches.
  Open design points: std::simd exists only in the C++26 lane, so the C++20
  lane needs an owned thin layer (Clang vector extensions or intrinsics); the
  fleet CPU ISA is not yet recorded; the current -ffp-contract=off policy
  blocks FMA and was chosen for exact CPU/GPU agreement, which needs
  re-deciding once certification is gone.
- Fleet CPU read over SSH (lscpu, /proc/cpuinfo): 2x Xeon E5-2697 v2 (Ivy
  Bridge EP), 24 cores / 48 threads, 2 NUMA nodes. Flags include AVX but no
  AVX2, no FMA, no AVX-512. Fleet SIMD is 256-bit float/double AVX with
  128-bit integer ops and no fused multiply-add. Both owned and NVIDIA watch
  runs used 12 threads/workers of the 24 physical cores.
- Bit-identical CPU/GPU output is NOT a goal (operator says this was told to
  the previous agent too). Functionally identical is the bar: each backend
  meets tolerance and the topology guarantees. FMA and fp-contract are free
  to use; --fmad=false / -ffp-contract=off are no longer required.
- Operator idea, now its own research lane
  (papers/2026-10-17-parallel-growth-clusters.md): grow N mesh clusters in
  parallel from optimizer-chosen seeds. Connectivity inside a cluster is free;
  pay for welds only at cluster seams, and cheapen or remove those welds with a
  shared coordinate system (e.g. a dyadic UV lattice or edge-parameter paths)
  so seam vertices coincide by integer key. It applies inside large faces
  (GPU-scale parallelism) as well as between faces. Net gain is a hypothesis
  to measure.
- Literature lane returned (papers/2026-10-17-fast-tessellation-literature.md).
- Self-intersection default accepted: each face stays within its ε tube, so
  only source features thinner than 2ε (thin walls, gaps, knife-edge dihedral
  angles at shared edges) can intersect. A face-level ε-inflated box query
  over the source finds them before meshing; tighten local tolerance there or
  refuse explicitly. The full self-intersection check stays an offline oracle.
- Robust predicates in the triangulator: arena both variants (with and
  without Shewchuk-style adaptive predicates) against each other on the full
  adversarial corpus. Measured correctness and speed outcomes decide.
- Mutation testing comes only after the verification stack has settled into
  the one the project builds on long term. Mutants test the tests, not the
  project. Early units use direct oracles that change exit status.
- Inexact B-reps: there is no single truth rule. The operator's bar: choose
  what an eye would instantly see preserves the solid's coherence. Research
  lane on tolerant modeling and healing heuristics
  (papers/2026-10-17-tolerant-geometry-healing.md).
- Operator idea: not single pass. Mesh the easy, unambiguous work at once;
  bring expensive cases (heals, hard welds, inexact trims, near-degenerate
  features) close cheaply; track them; background logic resolves them directly
  or plans a batched second pass as they accumulate. Covered by the same lane.
- Staged meshing is internal for now, aimed at wall-clock time. Many future
  uses include sharpen-in-place while a user watches, so provisional results
  are first-class state (status, queue membership, dependent faces), not
  hidden scaffolding.
- My call, recorded as such: the arena is a standing mechanism for contested
  choices the literature does not settle (predicates, FP32 vs FP64
  evaluation, grid-plus-band vs metric CDT for B-splines, in-face splitting).
- First unit: build the optional mechanisms behind primitive feature flags and
  ablate them in a broad-corpus plus watch arena, time-boxed (hours, not
  days), to learn which mechanisms help and in which combinations.
- Ablation size: 64 configurations. Explore further in configuration space
  only around something intriguing. Eight two-level flags fit a 2^(8-2)
  fraction (resolution VII), which separates main effects and two-way
  interactions. Three-level flags (staged resolver) take a mixed-level
  assignment inside the same 64 runs (my call).
  When something intriguing appears, follow it all the way: no time box on
  that follow-up exploration.
- Fleet topology read over SSH (nvidia-smi, numactl): two P100-PCIE-16GB.
  GPU0 is attached to NUMA node 0 (even CPU ids), GPU1 to node 1 (odd ids);
  each node has 12 cores / 24 threads and ~387 GB. The arena runs two
  independent lanes, each pinned to one socket, its memory and its local GPU.
  A shared reference configuration repeats on both lanes to detect
  cross-lane interference.
- Fairness: a canonical pinned NVIDIA run set under the same per-socket lane
  pinning, with at least two repetitions on each lane, over the same corpus
  sample and the watch. Owned results compare only against this pinned set.
- Every timing row emits its lane (socket, GPU, CPU set, memory node) and the
  competing non-test load during the run: load average, per-process CPU of
  non-arena processes above a threshold, other GPU processes and utilization,
  memory pressure, and observed CPU frequency. Read at handoff time over SSH:
  governor schedutil (dynamic frequency), load average ~3.8, with other
  services active. Frequency and background load are therefore real
  timing variables on this host.
- Healing lane returned (papers/2026-10-17-tolerant-geometry-healing.md).
  Defaults accepted: per-edge classification (exact / re-intersect where
  surfaces cross / owner face where tangent, analytic first / single-face /
  refuse), thresholds as flags, re-intersection may move a boundary off the
  stored 3D curve, sewing out of mesher scope.
- A background healer is built and tried even though no surveyed system runs
  one. "Nobody does it" is not an engineering argument; we find out why and
  keep all the data.
- One ablation pass is not a law. Defaults from the 64-run arena are
  provisional. After principal development, set up a low-key perturb-and-
  permeate cycle on the fleet host that keeps perturbing these choices over time,
  open to any interested partner.
- Not yet answered by the operator: whether measured source error counts
  inside ε or is reported on top; whether the arena may set the CPU governor
  to performance. Design defaults until answered: mesh held to ε against the
  chosen truth with source error reported alongside; governor left alone and
  frequency recorded.
