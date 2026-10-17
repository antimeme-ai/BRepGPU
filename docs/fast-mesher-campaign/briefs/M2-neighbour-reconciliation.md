# M2 brief: reconciliation from well-conditioned neighbours

**Purpose.** Answer the operator's question: does the desired final position of
inexact or malformed geometry come mostly free from easier calculations on
neighbouring geometry that is closer, better aligned or better conditioned?
And for grossly malformed starts: when is a small reconciliation leap right,
when a big one, and what do the second-pass branches look like?

**Serves the goal by.** Error is measured against the desired final position
(operator decision). If that position is cheap to derive from trusted
neighbours, healing becomes a fast propagation, not an expensive search. It
also decides how the background healer's second pass picks among branches.

**Read first.**
1. `BLACKBIRD.md`.
2. `AGENTS.md`.
3. `journal/2026-10-17-fast-mesher-direction.md` (later entries override earlier
   ones).
4. `docs/fast-mesher-campaign/PLAN.md`, then `docs/FAST_MESHER_DESIGN.md` §5.2
   and §7 (a working draft).
5. `papers/2026-10-17-tolerant-geometry-healing.md` in full. It holds the
   per-edge classes R1–R6, the owner rules, vertex rules V1–V4 and staging.
   Build on it; do not redo it.
6. `papers/2026-10-16-whole-mesher-geometry-contract-study.md`.
7. `papers/2026-10-15-om10-tolerant-incidence-reading.md`.

**Questions.**
1. **Propagation from trusted geometry.** Confidence-ordered resolution,
   region growing from exact or well-conditioned entities, priority queues by
   conditioning, and anchored least-squares or constraint-based repair, where
   trusted entities are fixed and the rest is solved relative to them. How do
   tolerant kernels (Parasolid, ACIS), healers (CADfix, OCCT ShapeFix, CATIA
   healing) and the academic repair literature use neighbouring geometry to
   decide a vertex's or an edge's final position? What do they treat as
   trusted, and why?
2. **Small versus big leaps.** For gross starts, what decides between minimal
   displacement (keep surfaces, close the gap with the least move) and
   reconstruction from the surfaces (re-intersection, re-projection, rebuilding
   an edge or a sliver face)? Are there published decision rules,
   multi-hypothesis repair, or "try the cheap fix, verify, escalate" ladders?
3. **Second-pass branch choice.** How do repair systems choose among candidate
   fixes:
   - a coherence score (closure, least displacement, preservation of analytic
     or larger entities, continuity class, consistency with neighbours);
   - keeping several branches alive;
   - deferring to a user?

   What does the literature say an eye checks when it judges that a solid
   "looks coherent"?
4. **Cost.** Which of these are cheap enough for a background healer that runs
   beside the fast meshing pass, and which batch well for SIMD or GPU?
5. **Fit.** Map the findings onto R1–R6, V1–V4 and the staged resolver.
   Propose a concrete procedure: what is trusted first, how confidence
   propagates, when to take a small or a big leap, how pass 2 branches and
   chooses, and when refusal is the honest outcome. For each proposal, give
   the measurable hypothesis and its falsifier, so the arena can test it.

**Outputs.**
- `papers/2026-10-17-neighbour-reconciliation.md`.
- `papers/2026-10-17-neighbour-reconciliation-sources.tsv`, with read depth
  per entry.
- Openly available PDFs into `papers/pdfs/` (gitignored). The laptop has about
  23 GiB free: fetch only what you will read.
- Unobtainable items go in a section titled "For SHOPPING_LIST.md".

**Done when.** Each question has an evidence-backed answer or an explicit "the
literature does not say", and the procedure in question 5 is concrete enough
for the design rewrite.

**Must not.**
- Edit other files.
- Commit.
- Build or run code or geometry.
- Put hostnames, absolute workstation paths or private operational detail in
  the outputs.
- Guess where a source can be read.

**Return.** A ~300-word summary of the most decision-relevant findings, plus
the proposed branch-choice rule in one paragraph.
