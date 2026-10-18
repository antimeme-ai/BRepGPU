# M7 brief: plan tribunal

**Purpose.** Find what is wrong in the M7 plan before M8 executes it:
- `docs/fast-mesher-campaign/M7-plan.md`;
- the first-wave unit plans in `docs/fast-mesher-campaign/plans/`.

Bead: `brepgpu-12s.23`. Gate G3 is delegated: the plan passes once this
tribunal's fixes land.

## Shape
- One round, in the same shape as M6: three reviewers, one fix pass by the
  planner, then a check by the coordinator.
- No reviewer is re-engaged, and no second round runs.
- No more than three agents run at once.
- Narrow findings, which change neither the order nor the interfaces nor what
  gets built, become beads.

## Mandates
1. **Strategy and goal fit.**
   - Does the plan reach a published, steel-manned win over NVIDIA by the
     shortest sound path, with fidelity fit for machines that cut or form
     material and output that looks good?
   - Is any unit ceremony, overbuilt, or missing?
   - Does the plan cover the whole design?
   - Are design gaps G-1 to G-8 (plan §12) handled correctly?
   - Is anything shaped to the watch?
2. **Sequencing, interfaces and executability.**
   - Are the dependencies and the critical path right?
   - Do the interfaces between units agree on names and types?
   - Is the bead map right?
   - Can each first-wave unit plan be executed by an engineer who has seen only
     that plan and the design: exact files, signatures, tests and commands, red
     first, nothing that decides nothing?
3. **Fleet, measurement and the arena.**
   - Does the plan run on the fleet what should run there alongside delivery?
   - Do the M8 measurements the design names come before the defaults that
     rely on them?
   - Do the measurement and arena units follow design §22–§23?
   - Does every GPU test run on both GPUs, with no transfer comparison across
     them?
   - Is the milestone configuration honest?

## Reviewer inputs
- The plan files above.
- `docs/FAST_MESHER_DESIGN.md` (the spec).
- `BLACKBIRD.md` and `AGENTS.md`.
- The journal from "Gate G1" on, and `docs/fast-mesher-campaign/M6-findings.md`.
- The code the plan names.

## Reviewer rules
- Read only. No edits, runs, commits or bead changes.
- Each finding gives:
  - the file and line;
  - what is wrong;
  - the evidence;
  - the fix;
  - its class: **plan** (it changes order, interfaces or what gets built),
    **bead**, or **operator**.
- At most twelve plan findings, most severe first. Bead findings take one line
  each.
- A finding needs evidence.
- No private hostnames, addresses, absolute paths or personal names.

## Fix pass
The planner:
- fixes every valid plan finding in the plan files;
- records each finding's disposition in `docs/fast-mesher-campaign/M7-findings.md`
  as fixed, rejected (with the reason) or bead;
- does not commit and does not touch beads.

## Coordinator check
The coordinator:
- checks the fixes against the plan text;
- files the bead findings and the plan's code defects;
- creates the unit beads;
- journals the round, passes G3 on the delegation, and commits.
