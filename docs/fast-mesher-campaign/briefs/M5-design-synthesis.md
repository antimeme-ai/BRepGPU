# M5 brief: rewrite the fast mesher design whole

**Purpose.** Write `docs/FAST_MESHER_DESIGN.md` again from the beginning, as one
coherent design for BRepGPU's fast mesher. The fast mesher has to beat a
steel-manned NVIDIA USD/BRep on arbitrary B-rep input. Its output must be fit
for machines that cut or form material, and it must look good.

This is a rewrite, not a patch. The old draft (613 lines) is one input among
many. Its errors are listed in bead `brepgpu-12s.20`'s notes.

Bead: `brepgpu-12s.20`. Plan: `PLAN.md` M5.

## Read first
1. `BLACKBIRD.md`, then `AGENTS.md`.
2. `journal/2026-10-17-fast-mesher-direction.md`, whole. It holds the gate G1
   decisions (D-partial-output … D-docs), M2's five adopted proposals, every
   lane outcome, and the 2026-10-19 operator answers.
3. `docs/fast-mesher-campaign/M4-decisions.md`, including "Operator answers".
   Rows not answered take their recommended default.
   - `.53`: a mixed-backend run must be functionally equivalent each time, not
     byte-identical.
   - `.68`: GPU0's x8 link is the slot itself. Every test runs on both GPUs,
     and transfer comparisons between the GPUs are forbidden.
4. `bd show brepgpu-12s.20`. Its notes carry each M4 lane's required design
   corrections. Every one must be resolved in the new design.
5. The lifecycle map, `papers/2026-10-17-geometry-lifecycle-map.md`, and the
   neighbour reconciliation, `papers/2026-10-17-neighbour-reconciliation.md`.
6. The 18 M4 papers. Their files are listed in
   `docs/fast-mesher-campaign/briefs/M4-G*.md`; each paper is
   `papers/2026-10-17-<topic>.md`.
7. `corpus/MANIFEST.md`, and the B1 and B2 reports in `results/b1-*` and
   `results/b2-*`. The ingress as it now stands is the design's starting
   point: raw-entity authority, declared corrections, and named refusals.
8. The old `docs/FAST_MESHER_DESIGN.md`, and `papers/2026-10-17-owned-mesher-audit.md`.

## What the design must be
- **Organised by the lifecycle.** Each section names its stage in the lifecycle
  map, the evidence behind it (paper section, measurement, or reference
  function), and any operator decision it carries. Decisions are carried
  unchanged.
- **Complete.** It covers every stage from source decoding to output and
  display. It covers placement on CPU and GPU, measurement against NVIDIA, the
  arena and its flags, failure and refusal semantics, and what is deferred to
  physics, edits and LOD.
- **Concrete enough to plan from.** It gives data layouts, contracts, numeric
  bounds and oracles. A sound plan must be writable from it without returning
  to the papers.
- **Honest about open questions.** Where the evidence does not settle a
  choice, name the choice, the arena arm that settles it, and its default.
- **Free of ceremony.** No certification layers, and no tests for tests. Every
  oracle must be able to fail. No JSON for internal data.
- **General, not watch-shaped.** Nothing is keyed to OM10 IDs, names, sizes or
  counts. Specialisations carry checked geometric preconditions and keep a
  general path.
- **Steel-manned against NVIDIA.**
  - NVIDIA is measured at its strongest arm.
  - Every timing states its lane and its competing load.
  - An invalid NVIDIA result is never our win.
  - Our own failures are never scored as NVIDIA's.
- **Library adoption.** If the design needs a library, it says so as a
  proposal for the operator, with benefit, fit, cost and the owned
  alternative. It does not adopt one.

## Rules
- Writing only: no builds, tests or fleet runs. If a claim needs a
  measurement, the design names it as an arena or M8 measurement.
- Do not modify any file other than `docs/FAST_MESHER_DESIGN.md`. If you find
  a defect in code, list it with file and line in your return.
- Keep private hostnames, addresses, absolute paths and personal names out of
  the document.
- Do not commit, and do not touch beads.

## Return
- The design is written in place.
- A short list:
  - each bead-`.20` note, with the section that resolves it;
  - every open question, with its default and the arena arm that settles it;
  - any library proposal;
  - any code defect found.
