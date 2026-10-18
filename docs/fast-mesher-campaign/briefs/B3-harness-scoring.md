# B3 brief: stop scoring our failures as NVIDIA's

**Purpose.** The M6 tribunal found a live violation of a standing rule in the
NVIDIA harness: when our own check of NVIDIA's output cannot finish, the harness
records an NVIDIA FAIL. It also found three smaller harness defects that bias
timings. The operator's policy is to fix every bug inside the phase that found
it, so B3 lands before gate G2.

Beads: `brepgpu-12s.25.16` (the scoring defect), `.25.17`, `.25.18`, `.25.19`.
Design reference: `docs/FAST_MESHER_DESIGN.md` §22.1 and §22.5.

**Serves the goal by.** Every NVIDIA comparison rests on this harness. An
input where our check failed, scored as NVIDIA's failure, removes a valid NVIDIA
result and hands us a win we did not earn.

## Defects

1. **`.25.16`: our check failures are scored as NVIDIA FAIL.**
   - `scripts/qualify_om10_nvidia.py:196–202` puts every definition whose
     distance query did not complete (`SOURCE_QUERY_SOLVER_FAILURE`, preparation
     exceptions) into `failed` and sets status `FAIL`, against its own docstring
     at `:69`.
   - `scripts/benchmark_om10_nvidia.py:490` maps any non-zero qualifier exit,
     exceptions included, to `FAIL`; `:417–423` and `:505–506` record that as the
     arm's failed qualification, and `selected_outcomes` then reports "no valid
     arm".
   - Live case: `results/om10-nvidia-complete-2026-10-17/REPORT.md:17–20`
     (definition 1808, "face-edge has no usable trimmed pcurve").
   - Required behaviour (§22.5): a check of NVIDIA's output returns `pass`,
     `fail` (a completed check found a fault in NVIDIA's mesh) or
     `not_evaluated` (our source query, preparation or qualifier failed). A
     `not_evaluated` input exits 5 (ours_failed), is excluded from the ratio and
     listed with its reason, and never makes "no valid arm".
2. **`.25.17`:** about 1 ms of Python pre-fork cost is charged to NVIDIA's timed
   scope (B2-ha §8). Move the fork timestamp so NVIDIA's scope starts at fork.
3. **`.25.18`:** `scripts/measurement_lane.py` counts our own writeback and
   softirq time as foreign load (see its Recorder docstring), which can mark our
   runs loaded unevenly between the sides. Attribute it to our own tree.
4. **`.25.19`:** `measure_quietly` allows at most three tries per estimate.
   Record the discarded loaded runs per side and per run length in the run's
   output.

Out of scope: generalising the harness to arbitrary inventories and per-key
chords (`.25.20`), and replacing its finite-mode quality condition with
achieved fidelity, which needs oracle 1 (§22.2). Both are M8 work.

## Read first
1. `BLACKBIRD.md`, then `AGENTS.md`.
2. Design §22 (`docs/FAST_MESHER_DESIGN.md`).
3. `bd show` for each bead.
4. `results/b2-harness-2026-10-17/REPORT.md` and its tests, and
   `docs/NVIDIA_REFERENCE_PREPARATION.md`.
5. The scripts named above and their tests (`tests/test_om10_nvidia.py`,
   `tests/test_measurement_lane.py`).

## Work, per defect
1. Study the site and its consumers; state the actual failure.
2. Plan in the report before code: mechanism, oracle, fixture.
3. Red test. Fixtures are small and authored; never corpus or OM10 bytes.
4. Fix, in the surrounding code's idiom.
5. Green: the new tests and the affected suites.
6. Self-review against the plan.

The dispatcher then runs one independent review. Its findings are fixed, and
the unit lands. Edge cases that change no result go to beads.

## Where code runs
Edit in the laptop checkout. Run tests only on the fleet host:
- Use the fleet SSH alias the dispatcher gives you, with `-o BatchMode=yes`.
- Create a lane directory `lanes/B3/` under the fleet BRepGPU root, and copy
  tracked files into it with `git ls-files` and `rsync --files-from`.
- Use the toolchains the repository already points at.

## Outputs
- Code and test changes in the laptop checkout. Do not commit.
- `results/b3-harness-scoring-2026-10-19/REPORT.md`: per defect, the study,
  plan, red evidence, fix, green evidence; the commands that ran; suites and
  counts. Fleet-relative paths only.

## Done when
- Each defect has a test that failed before its fix and passes after.
- The affected suites pass.
- The report is complete and has passed a privacy scrub.

## Must not
- Run anything on the laptop except editing and `git` reads.
- Run NVIDIA or owned benchmarks, or touch corpus data or OM10 bytes.
- Weaken a check to make a test pass.
- Commit, push, or touch beads.
- Put hostnames, addresses or absolute workstation paths in tracked files.

## Return
Per defect: cause, fix, test, before and after. Then any further defects
found, each with file and line.
