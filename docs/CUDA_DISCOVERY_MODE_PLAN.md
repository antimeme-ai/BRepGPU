# CUDA discovery mode experiment: implementation plan

Purpose: test whether making the existing hierarchy mode a compile-time CUDA
constant improves the qualified discovery operation while preserving its exact
outputs, counters, ABI and arithmetic. The
[design](../papers/2026-10-14-discovery-cuda-mode-specialization-design.md),
[numerical review](../papers/2026-10-14-discovery-cuda-mode-specialization-numerical-review.md)
and [runtime review](../papers/2026-10-14-discovery-cuda-mode-specialization-runtime-review.md)
are accepted. This plan incorporates their raw-byte identity, early private-mode
validation and stream-event timing corrections. Implementation awaits plan
review. No fleet work starts until the coordinator finishes the scalar queue.

## Ownership and permitted delta

One implementation owner owns the CUDA selection and its boundary/identity
clients as one experiment. Files: `native/cuda_backend.cu`, new
`tests/test_discovery_modes.py`, new `scripts/check_discovery_mode_identity.py`,
and an implementation journal. Independent reviewers attack numerical/result
identity and callback/resource integration. The root coordinator owns target
staging, builds, qualification, measurements, reports, commits and publication.

Keep shared math, CPU sources/core binary, Python adapter, public/private ABI,
cache construction/layout, numerical flags, CMake and native benchmark/runner
unchanged. No dependencies, degree/budget templates, queues, register limits or
block-size changes. The preserved dynamic plugin is an actual binary, not a
rebuild of old source.

## 1. Add direct boundary and identity clients before product changes

Use existing public constructors and fixtures. Explicit CUDA selection must
error if the requested card is unavailable; do not skip requested qualification.
Add focused public tests for all three modes:

- Counts 0/1/127/128/129/257, preserving full-count SoA stride. Mix valid rows
  with invalid slot plus invalid target, and valid slot with NaN, infinity or
  out-of-envelope target. Check precedence, eight canonical NaNs and zero work
  for invalid rows. AABB has zero cache extent.
- Degree (0,0) zero and rational one-third fixtures, a mixed-source-offset batch
  including degree (3,3), and one depth-12 traversal fixture. Check root-once
  refinement and active coefficient/padding behavior through the existing
  expected cases. Do not create a degree/depth Cartesian campaign.
- Refinement caps 0/1/8/64 and visits 1/2/full on a small selected fixture set.
  Include gap-with-cap, unresolved-complete and complete root-leaf cases. Use
  independently specified counters where supplied; otherwise check actual
  bounded counts and preserved output identity.
- Profile on/off numerical bytes and metadata for each mode. Nonempty profile
  requires profiled flags and finite nonnegative duration; empty profile has
  zero flags and NaN duration. Close public parents before downloading a retained
  child in one case per mode; exercise prior-device restoration on caller and
  worker threads. No new modeled failure/lifetime framework.

These tests use simple analytic answers or existing case assertions, not a new
per-row Fraction campaign for repeated tail rows. Implement one stdlib identity
client with fresh-process capture and compare commands. Capture the existing
309-row discovery corpus plus the focused boundary cases against each selected
plugin/core. Persist each case's eight-double and six-integer slab bytes in an
owned binary file; a TSV index records exact case/options/profile/count keys,
slab extents, input/control identity and selected binary identities. Compare
bytes directly, preserving signed zeros and NaN payloads; report the first
different field/slab/index. Compare deterministic metadata/options/provenance;
exclude only `kernel_ms`, whose shape/finite/flag rules remain tested. Do not
serialize float tuples as the identity mechanism or compare only hashes.

The identity capture is an execution/result compatibility client; it does not
recompute geometry truth. The unchanged exact qualifier supplies that separate
oracle once per new plugin/card. Retained baseline qualification applies only
when its actual binary identity matches.

After the scalar queue finishes, the coordinator runs the focused tests and
captures raw dynamic-plugin outputs on both P100s **before the product delta**.
These baselines should pass. There is no desired functional red: behavior is
unchanged. A baseline failure stops integration and is investigated; no expected
timing assertion or source-pattern test is invented to make a red.

## 2. Apply the minimal CUDA change

Template the existing `discovery_kernel` on unsigned `Mode`, instantiate exactly
AABB/DOP26/Bernstein, remove its runtime mode argument, and pass the constant
into the unchanged `discover_local` helper. Keep slot/root precedence, original
control offset, all output writes, 128 threads and the 4,096-block grid cap.

In `discover`, after existing context/count checks, read the retained hierarchy
mode and validate 0/1/2 **before pending-result construction/allocation and the
empty guard**. An invalid private mode reports an evaluation error, leaves the
token null and profile unset, and creates no events. Public upload cannot create
that token; source review must inspect this defensive path. Do not introduce a
production token-corruption hook or claim public invalid-option tests execute it.

Dispatch inside the current device/stream scope. Use three explicit template
launches with identical arguments, or one small private templated host launch
helper over the existing context/query/result/options objects to keep one kernel
argument list. Add no owned resource or public pointer. A defensive switch
default records an `Operation` evaluation error; it must not return past event
cleanup, bypass stream completion/restoration, fall back to AABB or publish.

Retain `cudaGetLastError`, finish, event destruction, pending-result drainage,
fatal aggregation and restoration-before-publication. Valid empty requests still
skip launch/events. Keep the event positions: the bracket includes any intervening
submission/idle gap and is not asserted to be pure device instruction time.

## 3. Qualify and independently review the changed unit

The coordinator builds only the new CUDA plugin with the pinned toolchain and
unchanged numerical flags. Record all three instances' ptxas registers, stack,
spill loads/stores and inspect emitted mode paths. Reduced resource counts are
an observation, not acceptance.

On each P100: run focused boundary/profile/device checks, capture new raw outputs
and require exact identity with the pre-change capture. Run the unchanged
309-row independent Fraction qualifier once against the new plugin, including
actual exported caches and original-UV uppers. Run the focused new discovery
tests under Compute Sanitizer on both cards. Smoke the existing ABI1 evaluator
and ABI2 projector using the preserved core/new plugin. Any missing backend,
nonzero violation, slab/metadata difference, sanitizer error or leak blocks
measurement; do not waive it with CPU parity or epsilon.

Review the whole CUDA delta and new clients against the design: dispatch,
validation precedence, numerical recipe, raw identity, all-mode indexing,
profile scope, cleanup and device binding. Fix and re-review concrete findings.
Unchanged registry/factories/shared math do not require another full mutation,
CPU benchmark, 309-row CPU run or broad modeled-fault matrix.

## 4. Measure the GPU-only comparison

Stage isolated dynamic/specialized directories with byte-identical preserved
core and benchmark binaries; only the sibling plugin differs. Verify identities,
do not overwrite baseline inputs, and use fresh output directories. Core-relative
plugin discovery makes these native calls select the intended binary. Retain the
completed CPU1/CPU24 measurements; do not rebuild or rerun them.

Run 28 matched native cases per variant/card, seven samples after each existing
excluded warmup, with default tolerance `2^-24` and visits 8,191 unless capped:

| Corpus | Count | Mode | Depth | Refinement cap | Cases |
| --- | --- | --- | --- | --- | ---: |
| Cylinder | 1, 256, 4,096, 65,536 | all three | 8 | 8 | 12 |
| Cylinder-offgrid, axis-offgrid | 4,096 | all three | 8 | 8 | 6 |
| Diagonal | 4,096 | DOP26 | 0, 8 | 8 | 2 |
| Axis-midpoint | 4,096 | Bernstein | 2, 8 | 8 | 2 |
| Capped cylinder | 4,096 | all three | 8 | 0, 8; visits 1 | 6 |

Alternate variant order by case; execute cards sequentially with no concurrent
owned copy/build/qualification/performance job. Retain all seven raw samples,
full requested condition keys, quality populations/widths and work histograms.
Compare resident, event-bracket and cached-hierarchy host-to-host phases
separately. Setup/first use/cleanup retain their existing scopes; do not add
independent medians into a synthetic caller time. Investigate changed precision
or work counts before interpreting timing. Reverse-order repeat only apparent
benefits/regressions that depend on order or overlap observed spread.

## 5. Decide and publish, or retain the dynamic kernel

Correctness and compatibility differences are unconditional blockers. Proposed
performance decision: retain specialization only if a count-at-least-4,096 case
shows at least 5% resident or cached-host median improvement, confirmed in a
reverse-order comparison on both cards, with no confirmed greater-than-5%
regression in either native phase of the other large-batch cases. For counts
1/256, investigate a confirmed regression exceeding both 5% and 0.05 ms. These
are experiment decision thresholds, not CI timing assertions. Event/resource
improvements without native-phase value do not suffice.

If findings or measurements reject the change, leave the qualified dynamic
plugin as the product and retain the experiment report. If one mode alone
regresses, mixed dynamic/specialized dispatch requires a separate reviewed
decision; do not silently introduce it. Publication records actual gains,
regressions, widths, repeated-input limits and cost scopes. No CAD modeling,
precision improvement or arbitrary independent-target claim follows.
