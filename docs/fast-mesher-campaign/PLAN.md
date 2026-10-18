# Fast mesher campaign plan

Written 2026-10-17 after the operator called a halt to regroup. Every maneuver
below, large or small, gets a written brief in `briefs/` before it starts and a
journal entry after it ends. Nothing launches from a chat message alone.

## What this campaign is for

BRepGPU must mesh arbitrary B-rep input faster than a steel-manned NVIDIA
USD/BRep baseline, at fidelity people can send to a machine that cuts or forms
material, and look stunning doing it. Certification is excised repo-wide;
correctness comes from implementing geometry correctly plus strong offline
oracles. The mesher is first; the other operations follow. The authoritative
record of operator decisions is
[journal/2026-10-17-fast-mesher-direction.md](../../journal/2026-10-17-fast-mesher-direction.md).

## Where we are against BLACKBIRD's pattern of work

| Step | State |
|---|---|
| 1. Frumentarii acquire literature, refimpls, CAD/TDPs; gaps to SHOPPING_LIST | Partly done. Seven research lanes, ~150 PDFs, 28 refimpls. **No B-rep corpus acquired. No SHOPPING_LIST.md.** |
| 2. Deep research toward a complete, holistic design | Partial. Research came one question at a time; the whole lifecycle was never mapped. |
| 3. Adversarial design tribunal | **Not done.** A single builder check ran instead. |
| 4. Plan, then adversarial plan review | Not started. |
| 5. Code under JSMNTL | Not started. |

[docs/FAST_MESHER_DESIGN.md](../FAST_MESHER_DESIGN.md) is a working draft. It
carries the operator's decisions correctly but was patched per finding. M5
rewrites it whole.

## Rules for every maneuver

- A brief in `briefs/` states purpose, how it serves the goal, inputs, outputs,
  what "done" means, and what it must not do. The brief gets the "is this
  bullshit?" pass before launch.
- One brief is one conceptual unit (BLACKBIRD: delegation units are conceptual
  units). Independent units run in parallel.
- Every maneuver reads what the repo already knows first: papers/, docs/,
  journal/ and code.
- No geometry, build, test, render or benchmark on the laptop. Fleet only.
- Before any commit, scrub hostnames, absolute workstation paths and private
  operational detail; the repository is public.
- Ambiguity that only the operator can resolve stops the maneuver and goes to
  the operator.
- A journal entry follows every maneuver: what ran, what it emitted, what
  changed.

## Maneuvers

### M0. Repair the record (small; laptop; no compute)

- Journal the halt: what went wrong today and what this plan changes.
- Mark FAST_MESHER_DESIGN.md as a working draft pending M5.
- Create SHOPPING_LIST.md from every 2026-10-17 lane's not-acquired list
  (paywalled papers, ISO 10303-59, SASIG PDQ Attachment C, Parasolid PK pages,
  BRL-CAD from an official source, Epic CADKernel, mmg3d corpus, Thingi10K
  meshes, TetWild papers, tracker attachments with unstated licences), with an
  operator decision column where one is needed (BRL-CAD).
- Commit and push the branch.
- Done when: committed, pushed, nothing running.

### M1. Whole-lifecycle map (one research lane)

- Brief: `briefs/M1-lifecycle-map.md`.
- Purpose: the complete life of B-rep geometry as it becomes a mesh and lives
  on, from authoring intent through exchange, reading, validation, healing,
  simplification and virtual topology, shared boundaries, edge and face
  meshing, assembly and instancing, post-processing, normals and LOD, export
  validity, downstream consumers, error reporting, incremental remeshing,
  caching and provenance, and performance at scale. Each stage's purpose,
  decisions and failure modes come from literature and mature systems.
- Maps each stage to the design, the 2026-10-17 papers, the 2026-10-16
  whole-mesher studies, the 2026-10-15 STEP shape-processing study and the
  code. Ranked gaps, each written as a stub brief for one gap lane.
- Output: `papers/<date>-geometry-lifecycle-map.md` plus sources TSV.

### M2. Reconciliation from well-conditioned neighbours (one research lane, parallel with M1)

- Brief: `briefs/M2-neighbour-reconciliation.md`.
- Purpose: answer the operator's question of whether the desired final
  position of inexact or malformed geometry comes mostly free from easier,
  well-conditioned nearby geometry. Topics:
  - confidence-ordered propagation and region growing from trusted entities;
  - anchored least-squares and constraint-based repair;
  - how tolerant kernels and healers resolve vertices and edges from adjacent
    geometry;
  - small versus big reconciliation leaps, multi-hypothesis second passes, and
    how a branch gets chosen;
  - the coherence criteria an eye applies.
- Output: `papers/<date>-neighbour-reconciliation.md` plus sources TSV.

### M3. Corpus acquisition (frumentarii; parallel with M1, M2)

- Brief: `briefs/M3-corpus.md`.
- Purpose: the inputs every speed and fidelity claim rests on.
- Sources:
  - ABC, as a reproducible sample selection with its licence;
  - Fusion 360 Gallery B-rep data;
  - NIST CAx-IF and NIST MBE/PMI test models;
  - public TDPs with native and STEP files (open hardware), recording missing
    TDP components explicitly as BLACKBIRD requires.
  - Already held: the OCCT dataset, step-conformance, resources4truck and the
    Gmsh/FreeCAD test STEP files.
- Output: data in quarantine; a tracked corpus manifest (source, revision,
  licence, hashes, restoration commands); SHOPPING_LIST entries for anything
  unobtainable.

### Gate G1. Operator triage

The operator reviews the M1 gap list and the M2 findings and says which gaps
are research, which are their decisions, and which are out of scope.

Done 2026-10-17. All 18 gaps are research lanes; the 15 decisions and M2's five
proposals take the recommended defaults (journal, "Gate G1"). Work is tracked in
beads from here (epic `brepgpu-12s`).

### B1. Ingress and admission defects (code; fleet; parallel with M4)

- Brief: `briefs/B1-ingress-defects.md`.
- The four defects M1 found by reading. Operator policy: no bug is carried
  past the phase that found it, so B1 closes before M4 does.

### M4. Gap lanes (one lane per triaged gap; parallel)

- One brief per gap, refined from the M1 stubs: `briefs/M4-G1.md` to
  `briefs/M4-G18.md`, with the shared rules in `briefs/M4-common.md`.
- Each writes one paper. Two waves: G1–G8 with B1, then G9–G18; G12 starts
  after G7's paper.
- Done when every triaged gap has a paper or an operator decision.

Done 2026-10-19. All 18 papers are delivered. B1 and B2 (the defects M4 and B1
found) have landed. The operator decisions raised are collected in
`M4-decisions.md`.

### M5. Holistic design synthesis (designer; laptop)

- Rewrite FAST_MESHER_DESIGN.md in one pass from the lifecycle map, M2, M4,
  the corpus and every earlier paper, with no patching of the old draft.
- Every section names its stage in the lifecycle, its evidence and any
  operator decision. Operator decisions are carried from the journal
  unchanged.

Done 2026-10-19 (d13d63b).

### M6. Design tribunal (parallel adversarial reviewers)

- Brief: `briefs/M6-design-tribunal.md`.
- One reviewer per mandate:
  1. Geometry and numerics correctness.
  2. Fidelity and oracle soundness: oracle tiers, no oracle that cannot fail,
     no tests for tests.
  3. Performance and architecture against the hardware (NUMA, AVX-only
     CPUs, P100) and the NVIDIA cost model.
  4. Robustness, pathology and termination.
  5. Measurement honesty: steel-manned NVIDIA, the arena's statistical design,
     load and frequency context.
  6. Doctrine and ceremony: "is this bullshit?", tenet 5, JSON-internals rule,
     over-testing.
  7. Overfitting and roadmap fit: the watch, arbitrary inputs, physics,
     incremental remeshing, sharpen-in-place.
- The designer fixes every valid finding; incorrect findings are recorded in
  the journal with the reason.

Done 2026-10-19. One round as the operator set it: seven reviews, one fix pass,
one coordinator check. Findings and dispositions:
[M6-findings.md](M6-findings.md).

### Gate G2. Operator review of the design

### M7. Implementation plan and plan tribunal

- Write the plan with the writing-plans skill. Its units are conceptual units,
  each with its JSMNTL sub-plan.
- Plan tribunal: holistic strategy, sequencing, and what can run on the fleet
  while delivery continues.

### Gate G3. Operator approval of the plan

### M8. Execution

- JSMNTL per unit: written sub-plan, red tests at the right oracle tier, code
  grounded in literature and refimpls, green, adversarial code review, fixes.
- Arena permutations run on the fleet as mechanisms land; delivery continues
  on the laptop. Mutation testing waits until the verification stack settles.

## Open operator questions carried forward

1. The CPU governor during arena runs: set to `performance` (needs root), or
   leave it and record frequency (default).
2. ~~How pass 2 chooses among reconciliation branches.~~ Settled at G1: M2's
   structural-first rule is the default and an arena flag.

## After-action assessment

When the campaign ends, however it goes, the designer writes an after-action
assessment into the workspace's `autodroit/` directory (outside this
repository): what was intended, what happened, what went well, what went
wrong and why, and what changes next time. **[operator]**
