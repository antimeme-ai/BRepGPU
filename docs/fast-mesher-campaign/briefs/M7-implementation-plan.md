# M7 brief: implementation plan

**Purpose.** Turn `docs/FAST_MESHER_DESIGN.md` (G2 passed 2026-10-19) into a plan
that M8 executes. Bead: `brepgpu-12s.23`. Plan: `PLAN.md` M7.

**Serves the goal by.** The design says what to build. The plan fixes the order,
the units and the interfaces between them, so the fastest path to a published,
steel-manned win over NVIDIA is the path M8 walks. The plan also fixes what runs
on the fleet while delivery continues.

## Shape of the plan
The design spans several subsystems, so M7 writes two layers:

1. **Master plan:** `docs/fast-mesher-campaign/M7-plan.md`. It holds no
   bite-sized steps. It contains:
   - **Units.** Each unit is one conceptual unit, and the list covers the
     whole design. Per unit:
     - its purpose;
     - the design sections it implements;
     - the beads it closes;
     - the files it creates or modifies;
     - the interfaces it consumes and produces (names and types);
     - the oracle tier of its red tests (§24);
     - the M8 measurements it needs first;
     - its fleet runs;
     - its done criteria.
   - **Sequencing and critical path,** with these constraints from the design:
     - the owned STEP reader (`.25.7`) is on the critical path, and no headline
       is published before it (§5.6, §22.3);
     - oracle 1, its trim-domain builder and oracle 10 are built before the code
       they judge (§24, §25);
     - the M8 measurements the design names come before the defaults that rely
       on them (§5.6, §12.6, §13.4, §22.6, §27);
     - gate lifts `.25.6`, `.25.13`, `.25.12` and `.25.5` come before any arena
       run that adopts defaults (§23.4);
     - the old mesher is retired only once the new path serves every consumer
       (§25).
   - **Waves.** No more than three units run at once, because the operator caps
     concurrent agents at three. Name the first wave explicitly.
   - **Fleet work alongside delivery:** measurements and arena permutations as
     their mechanisms land (§23).
   - **Bead map.** Every open bead under `brepgpu-12s.25`, plus `.37.14`, is
     either assigned to a unit or deferred with a reason.
   - **Milestones that matter to the goal:**
     - the first owned end-to-end mesh of a corpus part, checked by the oracles;
     - the first steel-manned headline on the watch's own stratum;
     - the first corpus-stratum claim.
2. **Unit plans** for the first wave only:
   `docs/fast-mesher-campaign/plans/<unit>.md`, in the writing-plans format
   (header, Global Constraints, Review Focus, tasks with Files, Interfaces and
   checkbox steps, red test first). Later units get their unit plans just in
   time in M8, from the master plan.

## Read first
1. `BLACKBIRD.md`, then `AGENTS.md`.
2. `docs/FAST_MESHER_DESIGN.md`, whole: the spec.
3. `docs/fast-mesher-campaign/PLAN.md`, `M6-findings.md`, and the journal
   `journal/2026-10-17-fast-mesher-direction.md` from "Gate G1" on.
4. `bd show` on each bead under `brepgpu-12s.25`.
5. The code the plan builds on:
   - `native/` (`step_source`, `reference_source`, `direct_cdt`, `mesher`,
     `om10`, `projection_solver.hpp`, evaluators, `measurement`);
   - `python/brepgpu/` (`imported_oracles.py` and its kin);
   - `scripts/` (measurement lane, NVIDIA harness);
   - `tests/`, and the CMake and tooling files (`tooling/native_cpu_rigor.cmake`).
   Name real paths. Extend existing patterns rather than inventing parallel ones.

## Global constraints (copy into each unit plan's header)
- Toolchain:
  - C++20 on the pinned Clang 18.1.3 with libstdc++;
  - CUDA 12.8, `sm_60`, SASS only;
  - new code stays C++26-lane-clean, and device-visible headers stay C++20.
- **No new libraries.** Any need for one becomes a proposal for the operator.
  Pinned OCCT 8.0.1 is the existing bridge.
- **No JSON for internal data:** use the tagged-table binary container. TSV and
  JSON appear only at presentation, on request.
- **Fleet only:** every build, test, render and benchmark runs on the fleet; the
  laptop edits and reads git. Every GPU test runs on both GPUs. Transfer figures
  are never compared across GPU0 (x8) and GPU1 (x16).
- **Original bytes stay private:** OM10 and corpus bytes are never copied into
  tests. Fixtures are small and authored.
- **No overfitting:** nothing keyed to OM10 IDs, names, sizes or counts.
- **Bounded review:** each unit gets one independent review, its findings fixed,
  then it lands. Edge cases go to beads.
- **Bugs:** fix every bug in the phase it is found, and file a bead.
- Mutation testing waits until the verification stack settles.
- **Privacy:** no hostnames, addresses, absolute paths or personal names in
  tracked files. Commits end with the Co-Authored-By line used in this
  repository.

## Rules
- Writing only: no builds, tests or fleet runs.
- Modify only the plan files named above. Do not commit, and do not touch
  beads.
- Where the design leaves a choice to the builder (§29), the plan makes it and
  marks it [call].
- Run the writing-plans self-review (spec coverage, step scan, type consistency,
  Review Focus, proportion) on the master plan and on each unit plan.

## Return (short)
- The units, in order, with the first wave named.
- The bead map, with any bead deferred and its reason.
- Any design gap found, with its section.
- Any library proposal.
- Any code defect, with file and line.
