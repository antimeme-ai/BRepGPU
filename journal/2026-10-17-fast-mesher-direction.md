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
  fraction. Correction from the design review: that fraction is resolution V,
  not VII as first written; it still separates main effects and two-way
  interactions, but only while every flag has two levels. Staged resolution
  therefore enters the first arena as background queue versus single pass;
  inline and speculative variants follow if staging is intriguing (my call).
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
- Design draft docs/FAST_MESHER_DESIGN.md written and given a builder check
  by a fresh reviewer. Ten questions: seven settled as marked calls in the
  document (compared scope is the meshing stage; ingress stays on the OCCT
  bridge, extended for the corpus; scoring when outcomes differ; corpus ε at
  1e-3 and 1e-4 of the bounding-box diagonal; invalid-but-used inputs meshed
  faithfully and reported; export to internal schema, STL and 3MF; the
  resolution correction above). Three go to the operator.
- Operator answers after the builder check:
  - Steel-man NVIDIA. NVIDIA being invalid is never our win; it is a finding.
    Publishable numbers start from NVIDIA at its strongest. Checked in code:
    NVIDIA reads only .brep (OcctFileConverter.cpp), and both our readers
    (step_source, reference_source) disable OCCT shape processing. Today's
    references therefore fed NVIDIA unhealed input. Its arms now include OCCT's
    default STEP processing; its time is its fastest valid arm.
  - Do not plan around calendar time. Permutations run on the fleet as they
    become available while delivery continues on the laptop.
  - From this point in the source graph forward the old mesher is no longer
    part of the system. Old artifacts remain, with a line marking them
    historical from the previous regime.
  - Tolerances: 1e-3/1e-4 of the diagonal were a convenient choice, not a
    derived one. Revised (my call): absolute 0.005 and 0.025 mm for inputs with
    trustworthy units; relative 1e-3/1e-4 of the diagonal only for
    arbitrary-scale inputs such as ABC.
  - Boundary-deviation question rephrased; still open.
- Correction: manufacturing tolerance is not absolute. What people read from
  their metrology is treated as absolute, and that interpretation is among the
  densest sources of trouble. ε is a request; the arena spans the requests
  people make (0.005 and 0.025 mm plus 1e-3/1e-4 of the diagonal where units
  are real; relative only for arbitrary-scale inputs).
- Certification removal is repository-wide. The mesher goes first; the other
  operations follow immediately after, in an order chosen then.
- Error is measured against the desired final position (the repaired coherent
  solid), not against the source's starting gap. The source discrepancy is a
  starting-condition metric, reported per edge, never charged against ε.
- Operator: gross starts may warrant a small or a big reconciliation leap, with
  branches for the second leap; two passes remain. Asked whether the desired
  final position comes mostly free from easier, well-conditioned neighbours.
- Process correction: research arrived one paragraph at a time and the design
  was patched per finding. Now: a whole-lifecycle mapping lane (every stage,
  mapped to the design and code, gaps ranked), a parallel lane on deriving
  final positions from well-conditioned neighbours, then one lane per gap,
  then a single synthesis into the design.

## Halt and regrounding

The operator called a halt: "the entire point of blackbird is to attack small
problem surfaces with the wisdom of considering the whole space relative to
goal as mass behind us." A just-launched lifecycle lane was stopped before it
wrote anything. Rereading BLACKBIRD in full against today's work:

- The design never faced an adversarial tribunal (pattern of work step 3); a
  single builder check ran and the draft was then treated as near final.
- The design was patched after each operator answer instead of rebuilt from a
  complete picture ("we never patch, we plan").
- Research lanes launched from chat with no written brief in the repository.
- No SHOPPING_LIST.md existed; lanes' unobtainable items were scattered in
  their reports.
- No B-rep corpus was acquired, though step 1 calls for CAD and TDPs.
- One answer (OCCT default STEP processing) came from memory although the repo
  already held papers/2026-10-15-step-shape-processing-lifecycle.md, which
  confirms it from OCCT source.

docs/fast-mesher-campaign/PLAN.md now governs: written brief per maneuver,
M0 record repair, M1 lifecycle map, M2 neighbour reconciliation and M3 corpus
in parallel, operator triage, M4 gap lanes, M5 whole rewrite of the design,
M6 seven-mandate design tribunal, M7 plan and plan tribunal, M8 execution,
then an after-action assessment in the workspace's autodroit/ directory. The
operator approved the plan.

M0 (brief: docs/fast-mesher-campaign/briefs/M0-repair-record.md): this entry;
FAST_MESHER_DESIGN.md marked as a working draft not to plan from;
SHOPPING_LIST.md created from every lane's not-acquired list, with four items
needing operator decisions (BRL-CAD source, Epic CADKernel, tracker
attachments with unstated licences, non-public OCCT fixtures).
- M0 committed and pushed (research/fast-mesher-study now on origin).
- M1, M2, M3 launched in parallel from committed briefs in
  docs/fast-mesher-campaign/briefs/. Disk read before writing M3: the laptop
  has ~23 GiB free (99% used), the fleet host ~1.5 TB, so corpus data lives on
  the fleet host outside any checkout, with only the manifest tracked.
- M3 returned (corpus/MANIFEST.md, corpus/manifest.tsv; data on the fleet
  host under corpus/, 75 file hashes re-checked there). Acquired: NIST MBE PMI
  models (public domain, 71 STEP), CAx-IF public library (53 STEP, licence
  unstated), five open-hardware printer packages (374 STEP; GPL-3.0 and
  CC BY-NC-SA). Blocked on operator decisions: ABC (download term; selection
  rule fixed before looking, fetch script staged, not run) and Fusion 360
  Gallery (non-commercial licence with indemnity). Four more decisions and two
  retries merged into SHOPPING_LIST.md. No geometry code ran on the corpus.
- M2 returned (papers/2026-10-17-neighbour-reconciliation.md, 59 sources).
  Finding: the final position comes mostly free. Trusted constraints fix the
  directions they determine and a prior fills the rest, at one small solve per
  sample on values the measurement pass already has. Leap size should follow
  conditioning, not distance. Proposals for G1 (not adopted yet): R2 takes the
  linear step; transverse leaps bounded by ¼ local feature size instead of β;
  small tangent gaps mesh as recorded steps; a structural-first branch-choice
  rule; R1–R6/V1–V4 recast as one point solve in three fixed parallel levels.
  Corrections to the healing paper: OCCT's neighbour-intersection gap fix is
  off by default; SMLib's Fix_BadGaps is an empty TODO. Unobtainable items
  merged into SHOPPING_LIST.md.
- M1 returned (papers/2026-10-17-geometry-lifecycle-map.md, 171 sources): 25
  lifecycle stages mapped to design, papers and code; 16 contradictions; 18
  research gaps and 15 operator decisions as stub briefs. Headline findings:
  the OCCT bridge alters geometry even with shape processing off (planar
  pcurves discarded and recomputed, master_representation ignored, vertices
  merged, mirrored placements read as rotations), so the "unprocessed" NVIDIA
  arm is not unprocessed; mature meshers re-split shared edges on face failure
  (the design freezes chains); export to float32 and normal-less formats
  undoes FP64 and shading guarantees unless export is its own stage; instancing
  is only safe for rigid and mirrored placements; no published rule ties chord
  tolerance to GD&T, and 1e-3 of the diagonal exceeds a NIST part's whole
  tolerance band. Four code defects found by reading (BREP_WITH_VOIDS voids
  dropped, invented straight-line t→s map, plane-angle unit unchecked, display
  accepting scaled instances the mesher refuses). Shopping items merged. Next:
  gate G1, operator triage.

## Gate G1: operator triage (2026-10-17)

The operator approved the triage as proposed ("send it"):

- All 18 research gaps (lifecycle map §4.1, G1–G18) go to M4 as research
  lanes, each with its own brief. None is out of scope. G12 waits for G7.
- The 15 operator decisions take the recommended defaults:
  - D-partial-output: mesh the rest and mark the part incomplete, by default.
  - D-placements: rigid and mirrored placements share one mesh; uniform scale
    gets a mesh per scale with ε scaled to match; non-uniform scale and shear
    are refused explicitly.
  - D-units: honour declared units; refuse unit-less input unless the caller
    states units.
  - D-formats: STL; 3MF with separate items, not united components; glTF with
    face ids and normals. USD and AP242 tessellated later.
  - D-consumers: PrusaSlicer, Cura, one glTF renderer and fTetWild as the
    analysis stand-in. PhysX arrives with physics.
  - D-PMI: read PMI to warn when ε is loose against the tightest stated
    tolerance or tighter than the source's own uncertainty. Warn and comply,
    never refuse.
  - D-master: follow the file's declared master representation; with no
    pcurves, recompute them with a recorded residual.
  - D-shapes: main shape representation only; report the others.
  - D-profile: a named BRepGPU data-quality profile seeded from JAMA and
    CAx-IF. Users may tighten it; loosening is recorded.
  - D-determinism: identical output across thread counts on one backend;
    functionally identical across backends; one model may mix backends.
  - D-imprint: none for alpha; matched discretization across declared contacts
    comes with physics.
  - D-features: never drop features above ε; defeaturing is out of scope.
  - D-provisional: provisional meshes are display only; physics consumes only
    final meshes.
  - D-measure: record SMT and AutoNUMA state as context, report cold and warm
    runs, account per definition and per occurrence.
  - D-docs: remove the IGES 5.3 and JT v10 PDFs marked "copying not allowed"
    and list them on SHOPPING_LIST.md.
- M2's five proposals are adopted as defaults, each also an arena flag: one
  point solve for all edge and vertex resolution; R2 takes the linear step;
  transverse leaps bounded by ¼ local feature size; small tangent gaps mesh as
  recorded steps, refused above ¼ local feature size; structural-first branch
  choice. This closes the plan's open question 2 (pass-2 branch choice).

Bug policy (operator): fix every bug. A bug that fits the work in hand is
picked up then; otherwise no bug is carried past the completion of the phase or
task group in which it was found. Work is tracked in beads (bd) from now on;
the GitHub issues and ISSUES.md remain the historical public index. The four
defects M1 found by reading are filed and are fixed in maneuver B1, during M4,
so they close before the research phase does.

Still open: the CPU governor (plan question 1) and the SHOPPING_LIST.md
decisions (ABC download term first, as the priority-1 corpus source).
- B1 and M4 wave 1 (G1–G8) launched in parallel from committed briefs
  (docs/fast-mesher-campaign/briefs/B1-ingress-defects.md, M4-common.md,
  M4-G1.md to M4-G8.md). Wave 2 (G9–G11, G13–G18) launched immediately after,
  since its lanes barely use the fleet. G12 waits for G7.
- M4-G11 returned (papers/2026-10-17-validation-records.md): check outcomes
  pass/fail/not_evaluated with five reasons; face status refused /
  indeterminate / valid_goal_unmet / valid; a deterministic roll-up to
  definition, occurrence and run; exit codes 0/2/3/4 with 1 for tool failure;
  native column-table records in the source snapshot container; profile
  brepgpu-pdq-1 seeded from JAMA V5.1 and CAx-IF, with values JAMA leaves to
  agreement seeded as no_threshold. Corrects the map: usd-brep has 191
  requirements (228 is call sites), and only Parasolid separates "could not
  evaluate" from "invalid". Two defects filed under B2 (brepgpu-12s.37); three
  operator questions filed.
- M4-G17 returned (papers/2026-10-17-contact-interfaces.md): sliding-contact
  physics needs no matched meshes; it needs accuracy tied to clearance and
  exact-surface normals. M5 reserves nine interfaces (per-face tolerance map,
  source back-map, per-instance face variants, extra constraint curves,
  one-sided tolerance, contact record, no merge, final meshes only, a
  contact-fit report). It caught an error in the G15 brief (an inscribed chord
  does not grow a boss); the brief and the running lane were corrected. Three
  operator questions filed, including restating D-imprint.
- M4-G18 returned (papers/2026-10-17-postprocessing-lod.md): in-core operations
  are legal only if their new triangles pass the generator's own checks and
  touch no chain or B-rep vertex; flips, collapses and relaxation are arena
  flags, default off; LOD is re-tessellation at ε_k = 4^k·ε with nested dyadic
  samples; decimation and remeshing stay downstream. Two operator questions
  filed.
- B2 (brepgpu-12s.37) opened to fix defects the M4 lanes report, before M4
  closes. papers/pdfs/ is now ignored as a directory (lanes store .ps.gz and
  .exp files there, which *.pdf did not cover).
- M4-G10 returned (papers/2026-10-17-small-feature-collapse.md): none of the
  four draft collapse rules keeps every triangle within ε as written; each has
  a counterexample family (checked by arithmetic on the fleet) and a cheap
  sound version. Cluster-level tests on the curve, triangles at a merged vertex
  sized to ε − d_f, boundary-only faces still given the density test, merged
  vertex at the smallest enclosing ball centre, six validity checks that
  revert to faithful meshing rather than refuse. Three design-draft errata
  noted on the M5 bead (β must split into a repair threshold and a collapse
  radius). Two operator questions filed.
- M4-G14 returned (papers/2026-10-17-sizing-beyond-chord.md): slicers and
  fTetWild need ε only; the renderer needs ε plus a per-segment angle cap (G13
  sets it); PhysX later needs a maximum edge length. Grading is FEM-only and
  costs about 8× triangles on planes; it also widens edit invalidation, so it
  stays out. Steel-man finding: NVIDIA's SDK default is chord off with 25°
  curve and surface angles, while our NVIDIA driver runs 0/0 angles, so every
  recorded NVIDIA time is a chord-only run although design §9.6 says default
  angles. Operator question filed at priority 1; the owned mesher must carry
  whichever term the steel-man arm uses.
- M4-G16 returned (papers/2026-10-17-source-identity-passthrough.md): the
  reader's Part 21 `#N` keys and the mesher's node classification already
  give Shephard classification; the paper lists the missing fields (revision,
  UUIDs, names, shape_aspect groupings), deterministic v5 UUIDs only, and
  carriage per format (STL by face-grouped ranges in the internal schema; 3MF
  one triangle set per face; glTF `_FACEID` plus a per-face table). Identity as
  range tables stays off the hot path (36.7 KB against 2.19 MB for the watch).
  Found that the draft can emit two mesh edges on one vertex pair and needs a
  sample-count floor (noted on the M5 bead). Two operator questions filed.
- M4-G2 returned (papers/2026-10-17-face-edge-feedback.md): every mature
  mesher stops face-to-edge feedback on a round count (OCCT 5, Gmsh 10+3,
  CGAL 29, NVIDIA 100). Ours needs none. Four segment predicates (UV flatness,
  lens separation, corner cone, 3D band) are hereditary, so splitting at
  dyadic midpoints until each segment passes is order-independent and gives
  the same chain at any thread count; a Ruppert/Shewchuk packing argument over
  Pav–Walkington local feature size bounds samples and depth from geometry.
  M5 answer: settle and protect a sufficient chain; the coupled queue becomes
  an arena arm. Requirement on M2's blend noted on the M5 bead. Cusps (α = 0)
  are the lane's own derivation, to test on family F4. No operator questions.
- M4-G7 returned (papers/2026-10-17-determinism-cache-keys.md): a
  determinism class per output layer (D0 decode identical everywhere; D1
  shared entities with one producer each, chosen by content key; D1 faces and
  records per backend; D2 functions of those bytes; D3 timings never keyed). A
  cache key over dependency output digests gives early cutoff and makes
  cracks from mixed versions impossible. The fleet probe ran 626,493 sizing
  decisions from 478 corpus files on CPU and both P100s, with and without FMA:
  no decision flipped; pure +−×÷√ rules were bit-identical when contraction
  matched; GPU0 and GPU1 byte-identical; differences only from libm and
  --fmad=true, at most 4 ulp. The live risk is round CAD numbers landing
  exactly on integer sample counts, which is why edge chains need one
  producer. A "backend" includes the libm variant, since glibc swaps
  FMA-compiled transcendentals on AVX2+FMA hosts. One display defect filed
  under B2; one operator question (fixed versus load-reactive mixed-backend
  placement) filed at priority 1. G12 launched.
- M4-G15 returned (papers/2026-10-17-pmi-tolerance-requests.md), with the G17
  correction applied: a request-checking rule R-PMI (one-sided margin per
  tolerance kind; consumption c = ε/m before and δ_f/m after meshing; PMI-OUT,
  PMI-LOOSE, UNC-FINE and UNC-EXCEEDED warnings; ε never changed, nothing
  refused). With vertices on the surface a hole's mating size drops by 2δ and a
  boss's is unchanged. Census of the NIST corpus: 17 of 49 files carry PMI
  (618 tolerance records); 11 hole sizes in 5 models sit with nominal on the
  lower limit, so any such mesh is out of tolerance; relative ε of 1e-3 of the
  diagonal violates tolerances on all 17; declared uncertainty reflects the
  exporter (1e-6 to 0.127 mm for one part); the nominal-model flag appears in
  none. One ingress defect filed under B2 (uncertainty maximum across
  contexts); c_max (0.1 or 0.25) filed for the operator.
- M4-G13 returned (papers/2026-10-17-shading-fidelity.md): a cap rule tied to
  Khoury–Shewchuk (repair a cap only when its circumradius exceeds twice its
  smallest enclosing radius and its flat-normal error exceeds the chord-limit
  reference), exact per-face corner normals never averaged across faces, edge
  classification measured per sample (STEP stores no regularity at all), and
  float32 glTF normals by default. A fleet probe on seven exact surfaces
  confirmed the bound in all 504 rows; interpolated exact normals are exact on
  planes, cylinders and spheres. Renders were not run (no reflection-line
  renderer exists yet). Corrects the healing paper and two design-draft
  clauses (noted on the M5 bead). One new operator question; the renderer
  question joins G14's.
- M4-G9 returned (papers/2026-10-17-instancing-semantics.md): a placement
  table (rigid and mirrored share a key; uniform scale keys on ε/σ_max;
  non-uniform, shear and singular refused per occurrence) with emit rules per
  format, tested on the raw STEP placement since OCCT turns mirrors into
  rotations. No mirror or scale exists in the watch or in any of the 500
  corpus STEP files, so those rules are tested on authored families only.
  Accounting rule: one unit of work per mesh key; NVIDIA gets exactly the key
  set, which is its strongest form, since both its own assembly paths
  tessellate per occurrence; emit timed separately. Two colour defects filed
  under B2 (per-occurrence colours dropped silently, including the watch's 9
  and the Voron Trident's 463; innermost-wins precedence); the mirrored-normal
  issue noted on the M8 placements bead. Three operator questions filed.
- M4-G4 returned (papers/2026-10-17-thin-feature-embedding.md): a
  half-clearance lemma gives the draft's 2ε rule as a special case, once
  measured on the repaired target with each face's true localization bound;
  the draft had no rule for faces sharing an edge, which is where meshing at ε
  self-intersects. Exact rational tests on the fleet: on extruded crescent
  knife edges the draft's cylinder slab rule crosses in 59 of 2,016 thick
  configurations and in most thin ones; the proposed shared-edge tilt rule
  crosses in 0 of 5,400, at no more than 7 extra vertices per cross-section;
  tightening only one face made it worse. No construction-time triangle-pair
  test is needed; curved knife edges remain a proof sketch. Two operator
  questions filed.
- M4-G5 returned (papers/2026-10-17-entity-admission-matrix.md): text and
  OCCT censuses over 839 unique STEP files (506 production exports, 333 OCCT
  dataset). Admission matrix by frequency; nearly everything legal in an
  advanced B-rep is admitted, CSG refused, sheets meshed and reported open.
  Today's ingress refuses 6.7% of export files on entity kind alone, mostly
  from the pole-vertex loop, which is legal AP242 (filed under B2; it knocks
  out several NIST fidelity models). Revolutions need native evaluators, since
  rational conversion is not linear in the pcurves' angle parameter
  (correcting map X12). Only 205 of 642,569 export edges carry pcurves, so
  recomputing them is the normal path. More OCCT translation edits found for
  G1. One operator question (tessellated-only parts); ABC now matters for
  offset and extrusion frequency too.
- M4-G6 returned (papers/2026-10-17-units-parameter-semantics.md): OCCT
  already converts every length, angle and admitted pcurve per context, so
  lifting the mm-only refusal needs only the three raw reads fixed (unit
  names, vertex coordinates, uncertainty), not a conversion map. A unit and
  parameter map per surface and curve kind marks where OCCT gets it wrong
  (revolution, extrusion and offset pcurves; parabola trims). Census over 488
  B-rep files: 343 mm, 136 inch, 9 cm; today's gate refuses 151, including 6
  millimetre files named 'MILLIMETRE'; 41 files write the degree so that full
  circles fail OCCT's closure tolerance. Critical for B1, sent to the running
  lane: no angle is read raw, so applying the declared angle factor would
  convert twice. Three further unit defects filed under B2; four operator
  questions filed.
- M4-G12 returned (papers/2026-10-17-edit-invalidation.md): incremental
  remeshing is fresh meshing behind G7's cache, with no mesh patching. Every
  meshing source surveyed patches an old mesh and differs from fresh; the
  incremental-computation literature gets equivalence by construction. Host
  gives content (full resubmission by default, or a tested delta); reuse is a
  key hit only, never deflection (OCCT's 1.1×/0.9× rule rejected; OCCT 8.0.1's
  Modified flag is read by nothing in TKMesh); a bounded halo of two vertex
  rings plus recorded spatial and collapse regions; four global couplings
  forbidden. Four corrections to the draft and M2 noted on the M5 bead. G7's
  placement question answered both ways; no new operator questions.
