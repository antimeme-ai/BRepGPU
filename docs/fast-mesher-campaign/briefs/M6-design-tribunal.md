# M6 brief: design tribunal

**Purpose.** Find what is wrong in `docs/FAST_MESHER_DESIGN.md` before the
operator reviews it at G2 and before a plan is written from it. Seven
reviewers read the design in parallel, one per mandate. The designer then fixes
every valid finding in one pass. The coordinator checks the fixes against the
findings list. Then the design goes to the operator.

Bead: `brepgpu-12s.21`. Plan: `PLAN.md` M6.

## Shape (operator, 2026-10-18)
- One round: seven reviews, one fix pass, one coordinator check.
- No reviewer is re-engaged after the fix pass, and no second round of review
  runs.
- Findings that only touch contrived edge cases, with no effect on the corpus,
  the watch or the design's structure, become beads for the phase that owns
  them. They do not hold the design back.

## Mandates
1. **Geometry and numerics.** Are the surface, curve, placement and tolerance
   rules correct? Are the numeric bounds sound, and do they hold for
   arbitrary B-rep input?
2. **Fidelity and oracles.** Are the oracle tiers right? Can every oracle
   fail? Are there tests for tests, or oracles that only restate the code?
3. **Performance and architecture.** Does the design fit the hardware: NUMA,
   AVX-only CPUs, P100 GPUs, GPU0 on an x8 link? Does it fit the NVIDIA cost
   model? Where will time go, and does the design know?
4. **Robustness, pathology and termination.** Does every loop end? Does every
   refusal have a name? What does malformed or hostile input do?
5. **Measurement honesty.**
   - Is NVIDIA steel-manned, at its strongest arm?
   - Is the arena's statistical design sound?
   - Is load and frequency context recorded with every timing?
   - Are our failures never scored as NVIDIA's?
   - Are transfer comparisons across the two GPUs excluded?
6. **Doctrine and ceremony.** The "is this bullshit?" pass:
   - BLACKBIRD tenet 5;
   - no JSON for internal data;
   - no certification layers;
   - no over-testing.
   Is anything in the design there for show?
7. **Overfitting and roadmap fit.**
   - Is anything shaped to the watch (OM10) rather than to arbitrary input?
   - Does the design leave room for physics, incremental remeshing and
     sharpen-in-place?

## Reviewer inputs
- `docs/FAST_MESHER_DESIGN.md`, whole.
- `BLACKBIRD.md` and `AGENTS.md`.
- The decisions it must carry unchanged:
  - `journal/2026-10-17-fast-mesher-direction.md`;
  - `docs/fast-mesher-campaign/M4-decisions.md`, including "Operator answers".
  A finding that contradicts an operator decision is not a finding. If the
  decision itself looks wrong, say so as a question for the operator.
- Whatever papers, results and code the mandate needs, read from the repo.

## Reviewer rules
- Read only. Do not edit any file, run builds, tests or fleet jobs, commit, or
  touch beads.
- Stay inside the mandate. Leave other mandates' findings to their reviewers.
- Each finding gives:
  - the section and line;
  - what is wrong;
  - the evidence (a paper section, code line, measurement or standard clause);
  - the fix proposed;
  - its class: **design** (it changes what gets built), **bead** (a real but
    narrow case), or **operator** (only the operator can decide it).
- Report at most fifteen design findings, most severe first. Report bead
  findings in one line each.
- A finding needs evidence. "This could be clearer" is not a finding.
- No private hostnames, addresses, absolute paths or personal names in the
  return.

## Designer fix pass
- Inputs: the compiled findings in `docs/fast-mesher-campaign/M6-findings.md`.
- Fix every valid design finding in `docs/FAST_MESHER_DESIGN.md`. Write the
  fixes into the design as it is, not as appended errata.
- For each finding, record its disposition in `M6-findings.md`: fixed (with
  the section), rejected (with the reason), or bead or operator.
- Do not modify other files, commit, or touch beads.

## Coordinator check
- Check each "fixed" finding against the design text.
- Check the rejection reasons.
- File the bead findings.
- Bring the operator findings to G2.
- Journal the tribunal, then commit and push.

## Done when
- Every finding has a disposition.
- The design carries the fixes.
- Bead findings are filed.
- The journal entry is written, and the work is committed and pushed.
