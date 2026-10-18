# B2-harness: NVIDIA measured at its strongest

This is unit B2-harness of maneuver B2
([brief](../../docs/fast-mesher-campaign/briefs/B2-lane-defects.md)), bead
`brepgpu-12s.37.17`.

The operator's rule: NVIDIA is steel-manned, and NVIDIA being invalid is never our
win. Every timing records its lane and the competing load.

**Grounding:**
- [G8](../../papers/2026-10-17-heterogeneous-placement-measurement.md) §7, the
  measurement protocol;
- [G9](../../papers/2026-10-17-instancing-semantics.md) §0.2, per-key accounting;
- [G1](../../papers/2026-10-17-occt-ingress-translation-edits.md) §5, the NVIDIA
  ingress arms.

**How it ran:**
- Code was edited in the laptop checkout. Builds and tests ran only on the fleet
  host, against the pinned OCCT 8.0.1 and the NVIDIA USD/BRep 1.0.0 package.
- The final runs used binaries built from the combined tree, which includes
  B2-ingress (adapter v6, reference exporter v3).
- No published benchmark was re-run.
- The lane's returns were consolidated by the dispatcher.

**Review.** An independent adversarial reviewer read the change five times. Five
fix rounds followed. The last review asked for one fix, and it was made.

## 1. What was wrong

**Workers.**
- `examples/render_om10_nvidia.py` assigned definitions to NVIDIA workers by
  source face count. G8 §7.11 shows this misorders cost.
- No worker was pinned and no memory was bound, under AutoNUMA.
- One worker count was run, and no context rows were written.

**The published heal0 "NVIDIA complete" wall.** At least 21.7 s of its 43.67 s was
our own work:

| Our work inside the wall | Time |
|---|---|
| Decode, admission and scene pack | 19.51 s, of which physical admission was 16.74 s |
| Display | 1.27 s |
| View writes | 0.95 s |

Ingress export plus the NVIDIA batch wall came to 10.17 s.

**Ingress.** NVIDIA reads only `.brep`. It was fed OCCT translation without
ShapeProcess. The candidate strongest input is OCCT's default STEP read (G1).

**Owned watch runs.** They had one-socket affinity only: memory unbound, no context
rows.

## 2. Final design

### 2.1 NVIDIA's time

`nvidia_headline` (the driver's `complete_seconds`) has two parts.

**Ingress process wall.** The minimal NVIDIA ingress
(`native/nvidia_ingress/brepgpu-nvidia-ingress`), timed from fork to exit. It does
a `STEPControl_Reader` read and transfer, then writes one BinTools `.brep` per
`MANIFOLD_SOLID_BREP`. There is no XDE, no hashing and none of our checks. Its
files are what NVIDIA consumes.

**NVIDIA-only worker makespan.** The worst worker's total of:
- producer start, from launch to `main` (`CLOCK_MONOTONIC`, cross-checked against
  `/proc/self/stat`);
- context setup;
- conversion; in prepared mode only each `ConvertOcctFileToUsd` span counts;
- import and heal, and tessellation, including NVIDIA's rectangle-triangulation
  call;
- per-definition object teardown;
- producer exit.

An `…_excluding_ingress` variant is reported beside the headline.

**Listed, never added.** Each item has its own row in `scopes.tsv`:
- our input preparation, flagged "requires our preparation" only when it changed
  NVIDIA's input bytes;
- the as-run total and the measured batch wall;
- NVIDIA's failed conversion attempt before our repair;
- our converter-service overhead, and its idle time, which is charged to no one;
- our XDE export, extraction and writes;
- physical admission;
- emit (G9 §0.2 rule 3), display and views;
- context capture and pin hashing.

**Symmetry rule.** Written in `docs/NVIDIA_REFERENCE_PREPARATION.md`:
- Both headlines count ingress from the same STEP bytes, timed the same way.
- The owned headline excludes its own admission, scene construction, emit and
  display.
- Owned ingress timing is filed for M8 (`brepgpu-12s.25.9`).

### 2.2 Exit codes

| Exit | Meaning | Status |
|---|---|---|
| 0 | Complete | `COMPLETE_UNQUALIFIED` (transport), `PASS`/`FAIL` (finite) |
| 1 | NVIDIA's batch, or its admitted output, failed | `FAIL` |
| 3 | Harness or settings problem | `HARNESS_REFUSED`; stops the campaign |
| 4 | Our ingress tooling failed, or NVIDIA's input failed our checks | `INGRESS_REFUSED`; that arm only |
| 5 | Our work failed, or our converter service failed twice | `OURS_FAILED` |

An `OURS_FAILED` arm keeps NVIDIA's headline only when the batch and admission
passed and, in finite mode, geometry quality is PASS.

A preflight refuses harness problems before any arm runs.

### 2.3 Ingress arms and checks

| Arm | Status |
|---|---|
| `occt-no-shape-process` | today's arm and the default, never called the steel-man. The exporter must declare `translation-only` (or the older `none`). |
| `occt-default-read` | candidate strongest. Refuses until an XDE exporter declares `default` (`brepgpu-12s.37.19`). |
| `owned-projected-pcurves` | labelled slot, refused until an operator decision. |

`check_nvidia_input` refuses the input (exit 4) unless:
- the read policy matches the arm;
- the solids are exactly the source definitions;
- every root is an identity-located, forward solid;
- for the translation-only arm without declared raw-entity corrections, every
  file is byte-identical to our XDE export.

Both readers' OCCT libraries are pinned. On a byte mismatch, a geometry diff is
written before the refusal.

### 2.4 Converter service (prepared mode)

- One long-lived `brepgpu-nvidia-convert --serve FD` per worker, so the
  first-call setup is paid once per worker, as in-process use pays it. Replies come
  on a dedicated descriptor, never stdout.
- A dead, broken or silent server is replaced and the job retried once. A second
  failure is ours (exit 5).
- Served output is byte-identical to one-shot output (§6).

### 2.5 Calibration

- LPT assignment is weighted by the headline's own per-definition CPU.
- Every arm runs a calibration pass first.
- 24-worker arms calibrate from the 12-worker pinned pass.
- Calibrations must match source, settings, workers, placement and pins, and
  every worker must have exited 0 with every definition PASS.
- An uncalibrated arm is never named fastest.

### 2.6 Lanes and placement

- One NUMA node per lane, CPUs ordered by physical core first, bound with
  `numactl --physcpubind --membind`.
- Callers are launched already bound.
- Arm axes: ingress × preparation × heal × workers {12, 24} × placement {pinned,
  floating}.

### 2.7 Quiet policy, shared by owned and NVIDIA runs

`measure_quietly` in `scripts/measurement_lane.py`:
- Settle checks run immediately before the timed region.
- Classification uses foreign CPU only: host busy minus our own process tree, at
  every check, over the window and during the run (for NVIDIA, its own ingress and
  batch rows).
- Up to 3 attempts are made. The first quiet attempt represents the arm, else the
  first attempt, labelled. The minimum is never taken.
- Arm order and environment padding are seeded and recorded.
- Every row says n = 1. A headline needs the median of at least 10 fresh
  processes (G8 §7.5).

### 2.8 Page cache

Immediately before each ingress, NVIDIA's and owned alike, the STEP file's pages
are dropped with `posix_fadvise(POSIX_FADV_DONTNEED)`. Residency of the STEP file,
the tool and its OCCT libraries is recorded. Library pages are recorded, not
dropped.

### 2.9 Context rows

One row per timing, captured outside the timed interval. Capture never raises.

Each row records:
- the lane and binding;
- foreign CPU, unthresholded and above 5%;
- our own tree;
- load average;
- delivered clock and CPU share (`native/measurement/brepgpu-delivered-clock`);
- governor, SMT, AutoNUMA and THP;
- free memory per node and PSI;
- GPU use and process counts.

Foreign processes appear only as counts and CPU, never by name.

## 3. Review rounds

| Review | What was found and fixed |
|---|---|
| 1 (4 blockers) | NVIDIA's time included our admission and emit → owner scopes. Harness refusals were scored as NVIDIA failures → exit 3 and a preflight. Face-count assignment → calibration. Re-exec was charged to NVIDIA → callers launched bound. Should-fixes: floating arm, calibration rules, load class, seeded order, competing-load accounting, capture safety, replay context, retired old driver, probe CPU share, tests, privacy. |
| 2 (bias *for* NVIDIA) | The headline omitted NVIDIA's process start, exit and teardown → measured and included. Our failures were scored as NVIDIA's → exits 4 and 5. Our preparation was in the headline → listed instead. Ingress was our XDE read → minimal ingress plus the symmetry rule. The quiet gate counted our own load → foreign-only. LPT weight and w24 calibration fixed. |
| 3 | Converter process start and exit were charged to NVIDIA → only the conversion span counts. Exit 5 skipped geometry quality → quality must be PASS. Ingress frame, orientation, byte and policy checks added. Per-arm exit 4. Owned runs got the same quiet gate. One estimator for every arm. |
| 4 | First-call setup was repeated 107 times → one long-lived converter per worker. Page cache equalised. Byte check made conditional, with libraries pinned and a geometry diff on mismatch. Settle and run load scoped to NVIDIA's timed region. |
| 5 | A converter-server failure read as an NVIDIA refusal → retry once, then ours. Protocol moved off stdout. Served-equals-one-shot proven. |

## 4. Evidence

**Red runs.** Logs are in the fleet lane directory.
- **Pre-fix harness:** workers had affinity to all 48 CPUs and default memory
  policy; assignment was by face count; there were no context rows.
- **Round 1:** refused arms were recorded as NVIDIA `FAIL`; 29 context rows named
  foreign processes (counted, then deleted).
- **Round 2:** a fixture worker measured 46 ms without its start and exit, against
  84 ms with them.
- **Round 3:** a converter process took 65 ms, of which the conversion itself was
  13 ms.
- **Old prepared suite:** it blamed geometry for a missing plugin path.

**Green runs, final.**
- Fleet, combined-tree binaries, 0 build warnings.
- `tests.test_measurement_lane`: 37/37.
- `test_om10_nvidia`, `test_nvidia_prepared_pipeline`, `test_om10_posters`,
  `test_nvidia_reference_controls` and `test_reference_periodic`: 25/25.

## 5. Measured first-call gap

The in-process producer converted three authored inputs three times each, five
repetitions, on one pinned CPU.

| Call | Time |
|---|---|
| First `convert_occt_to_usd` in a process | 12.6–13.3 ms |
| Later calls | 2.4–2.8 ms |

The excess is about 10.2 ms per first call. That is why prepared mode keeps one
converter per worker.

## 6. Served against one-shot conversion, all 107 watch definitions

The OM10 source was read in place on the fleet, read-only. Derived files were
deleted afterwards, and only counts are reported.

| Outcome | Definitions |
|---|---|
| Converted by both, byte-identical | 105 |
| Converted by both, bytes differ | 0 |
| Refused by both (the two periodic cases) | 2 |
| Outcome differs | 0 |
| Server failures | 0 |

## 7. Published figures marked

One line was added after the title of each report below. It states:
- the pre-fix harness conditions;
- that the "complete" wall included our admission, decode and scene packing;
- that NVIDIA read the no-shape-processing arm, not the candidate strongest
  ingress;
- "whether the fixed harness or the default-read arm is faster is not yet
  measured".

The reports:
- [om10-nvidia-complete](../om10-nvidia-complete-2026-10-17/REPORT.md), NVIDIA
  figures only;
- [om10-watch](../om10-watch-2026-10-17/REPORT.md), its NVIDIA times, and the
  owned and replay times, which had socket affinity only, unbound memory and no
  context rows;
- [om10-exploded](../om10-exploded-2026-10-17/REPORT.md), its NVIDIA walls and
  replay times.

## 8. Measurement limits

- Every row is n = 1. The G8 §7.5 repetition is not implemented. Choosing arms by
  the minimum of single runs favours NVIDIA, which the steel-man intends.
- Foreign-load accounting:
  - interrupt, softirq and writeback time counts as foreign;
  - processes that start and exit inside a window appear only in the host total;
  - CPU time has 10 ms tick resolution.
- About 1 ms of Python pre-fork cost is charged to NVIDIA.
- Where no CPU column exists, wall time stands in for CPU time.
- `posix_fadvise` does nothing on tmpfs.
- SMT contention is untested: the 2-definition fixture keeps only two workers busy
  at any worker count.
- The "source read in the previous 60 s" field is not implemented.
- The exploded replay has not been executed, because it needs the watch artifacts.
- Finite-mode qualification is tested only at unit level.

## 9. Open items

| Item | Bead |
|---|---|
| Default-read XDE exporter. Its face map must be built under FixShape when the arm becomes runnable. | `brepgpu-12s.37.19` |
| Mapped-item exporter coverage | `brepgpu-12s.37.25` |
| Owned ingress timing (M8) | `brepgpu-12s.25.9` |
| Angle arm (operator). Either runs by explicit setting; defaults are unchanged and recorded. | `brepgpu-12s.48` |
| `owned-projected-pcurves` arm (operator) | `brepgpu-12s.70` |

## 10. Process note

In one round, numbers already measured on the fleet were parsed with a short
script on the laptop rather than on the fleet. It was text parsing only; no code
ran against project data.

## 11. Files

**Changed:**
- `examples/render_om10_nvidia.py`
- `examples/render_om10_exploded_reference.py`
- `scripts/benchmark_om10_nvidia.py`
- `scripts/benchmark_om10.py` (retired; refuses unless given
  `--historical-reproduction`)
- `scripts/prepare_nvidia_reference_worker.py`
- `native/om10_nvidia/main.cpp`
- `native/om10_nvidia/convert.cpp`
- `tests/test_nvidia_prepared_pipeline.py`
- `docs/NVIDIA_REFERENCE_PREPARATION.md`
- the three report marks

**New:**
- `scripts/measurement_lane.py`
- `scripts/build_measurement_probe.sh`
- `native/measurement/`
- `native/nvidia_ingress/`
- `tests/test_measurement_lane.py`
- `tests/fixtures/mapped-item-solid.step`
