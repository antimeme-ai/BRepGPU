# M4 common brief: rules shared by every gap lane

Each M4 lane has its own brief, `M4-G<n>.md`, which refines one research-gap
stub from `papers/2026-10-17-geometry-lifecycle-map.md` §4.1. That brief and
this one are read together; where they differ, the lane's own brief wins.

**Why M4 exists.** M1 mapped the whole lifecycle of B-rep geometry becoming a
mesh and found 18 research gaps. M5 rewrites the design whole from what the
campaign knows. Each gap is a place where M5 would otherwise guess, so a lane
is done only when its decision can be made from evidence, or when the lane
states plainly that the literature does not say and names the measurement that
would.

**The goal every lane serves.**
- Mesh arbitrary B-rep input faster than a steel-manned NVIDIA USD/BRep
  baseline, at fidelity people can send to machines that cut or form material,
  and look stunning doing it.
- Speed is second only to correctness.
- No overfitting to the OM10 watch.

**Read first, every lane.**
1. `BLACKBIRD.md`: take it literally.
2. `AGENTS.md`.
3. `journal/2026-10-17-fast-mesher-direction.md`. Later entries override
   earlier ones. The gate G1 entry holds the operator decisions (D-*) that
   bind M4, and M2's adopted defaults.
4. `docs/fast-mesher-campaign/PLAN.md`.
5. The lifecycle map in full: your gap's stub, every stage and contradiction it
   cites, and the §1–§3 rows behind them.
6. `docs/FAST_MESHER_DESIGN.md`. It is a working draft: read it for the
   decisions and intent, not as a hardened design.
7. Every earlier paper your brief names, and anything else in `papers/` or the
   shared literature collection (`../papers_proj/` in this workspace) that
   bears on your gap. Build on these papers; do not redo them. Check
   `papers/reference-catalog.tsv` and `QUARANTINE.md` before fetching
   reference code.

**Method.**
- Answer each of your brief's questions with evidence: a paper section, a
  standard clause, a reference file and function at a pinned revision, or a
  measurement. Separate what a source establishes from your own derivation.
- When sources disagree, give both, then the resolution and why.
- Every proposal states a measurable hypothesis and its falsifier, so the arena
  or an oracle can test it.
- Ambiguity that only the operator can resolve stops that question and comes
  back in your return. Never settle it yourself.

**Measurement (only when your brief allows it).**
- Fleet only. Use the fleet SSH alias the dispatcher gives you, with
  `-o BatchMode=yes`.
- Work in your own lane directory under the fleet BRepGPU root,
  `lanes/M4-G<n>/`, and never inside another directory there. Copy tracked
  files with `git ls-files` and `rsync --files-from` if you need code.
- Use the pinned OCCT 8.0.1 and CUDA toolchains the repository already points
  at. Do not install or build new toolchains. A small probe written for the
  lane is fine; it lives in the lane directory, and its source is quoted or
  summarised in the paper.
- The corpus is on the fleet host under the BRepGPU root's `corpus/`, described
  by `corpus/MANIFEST.md`. Read it; never modify it.
- Every timing records:
  - the lane (CPU socket or GPU);
  - load average and the competing non-test load;
  - CPU frequency and governor;
  - SMT and AutoNUMA state;
  - cold or warm.

  Other lanes may be running; say so when it matters.
- No mesher, Boolean or NVIDIA runs unless your brief asks for them.

**Outputs.**
- `papers/2026-10-17-<slug>.md`: the paper. Its slug is named in your brief.
- `papers/2026-10-17-<slug>-sources.tsv`: the existing `*-sources.tsv`
  convention, with read depth per entry (full / method+results / abstract).
- Openly available PDFs into `papers/pdfs/` (gitignored). The laptop has about
  23 GiB free: fetch only what you will read, and stop and report if a source
  is large.
- Unobtainable items go in a section titled "For SHOPPING_LIST.md" at the end
  of the paper.

**Must not.**
- Edit any file other than your outputs.
- Commit or push.
- Build or run anything on the laptop.
- Put hostnames, addresses, absolute workstation paths or private operational
  detail in the outputs. The repository is public.
- Guess where code or literature can be read.
- Accept terms or create accounts.

**Defects.** A bug you notice in our code goes in your return with its file and
line. The dispatcher files it as a bead; the lane does not fix it.

**Return.** Your brief names the specific return. Every return also lists:
- defects found;
- operator questions;
- shopping items.
