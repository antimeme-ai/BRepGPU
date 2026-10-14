# Conservative bounds and candidate discovery

Study synthesis, 2026-10-14. The operator authorized continuing after the measured
resident projector and explicitly requested acquisition/study of Wafer's
GPU-mode repository. Product code follows the source studies, reviewed design
and reviewed implementation plan.

The next useful capability is to locate promising parts of supported carriers
without supplied seeds. Conservative subdivision/bounds must preserve original
patch and UV identity and exclude a region only when its entire exact surface
cannot improve the declared query result. Budget/capacity exhaustion must retain
unresolved coverage. A stationary local candidate alone does not establish a
global nearest point or a conservative surface-distance upper bound.

## Whole research units

- Numerical conservatism: positive-weight homogeneous subdivision, outward
  binary64 control/surface enclosures, convex-hull distance lower bounds and
  independently justified candidate upper bounds on CPU/P100.
- Resident execution: packed hierarchy and source identity, preprocessing vs
  per-query work, bounded queues/frontiers, overflow semantics, lifetimes and
  optional capability compatibility. Acquire and study the requested Wafer
  repository as a reference; determine its actual hardware/precision fit.
- Independent oracles: exact dyadic rational subdivision, enclosure/exclusion
  witnesses, complete domain coverage, analytic distances including boundaries,
  multiple minima and degenerate carriers; actual useful caller/timing scopes.

## Completed studies and recommendation

The requested reference is Wafer's
[GPU performance resource index](https://github.com/wafer-ai/gpu-perf-engineering-resources),
which explicitly links [GPU MODE lectures](https://github.com/gpu-mode/lectures).
Both are acquired at pinned revisions with intact archives, clean extractions,
source catalogs and restoration commands. The
[actual-code study](../papers/2026-10-14-wafer-gpu-mode-study.md) covers selected
profiling, divergence, shared-memory and reduction examples. An independent
source review confirmed its boundary and work-ownership counterexamples. These
are read-only analytic findings, not reproduced performance measurements.

The [numerical study](../papers/2026-10-14-bounds-numerical-study.md),
[oracle study](../papers/2026-10-14-bounds-oracle-study.md),
[runtime study](../papers/2026-10-14-bounds-runtime-study.md) and
[GPU traversal study](../papers/2026-10-14-bounds-gpu-study.md) support the next
design direction:

1. Build an immutable binary hierarchy of dyadic subpatch rectangles on CPU.
   Outward homogeneous subdivision and dehomogenization bound the original
   binary-input carrier; ordinary rounded subdivision plus final padding is
   insufficient. Upload packed boxes and integer cell/source identity once.
2. Run bounded resident queries naming one patch and a target in its local
   metric. Traversal supplies automatic UV candidates and conservative distance
   lower bounds. The existing projector can improve a candidate without a
   Python iteration loop.
3. Evaluate a candidate UV on the original patch with outward arithmetic to
   obtain a valid upper distance bound. Ordinary projector XYZ/distance is
   descriptive and cannot certify exclusion.
4. Return the candidate, squared-distance bracket, work counters and explicit
   unresolved reasons. A visit/candidate/output cap must retain the lower-bound
   contribution of every uncovered region. One best candidate does not mean
   all minimizers were enumerated.
5. Qualify bounds against exact binary-input rational arithmetic and qualify
   domain coverage with integer cell identities. Measure setup/amortization,
   resident filtering/refinement, transfer, host consumption and unresolved
   populations on CPU and each P100.

This scope removes manual seed selection and supplies bounds machinery needed
by later proximity and intersection operations. Cross-patch selection needs an
explicit common metric; trims need valid trim-domain witnesses. Neither is
silently inherited from the current local patch metadata.

The Wafer/GPU MODE scheduling lessons support measuring a simple bounded
traversal before adding compaction or persistent queues. Nsight Compute does
not support our GP100 target; the source study records current and original
release support. Existing CUDA-event/host timers and compiler/binary inspection
remain the usable measurement path.

## Decisions still needed for the reviewed design

Resolve fixed hierarchy depth/cache admission, CPU arithmetic environment
rejection versus save/restore, visit and refinement budgets, output capacity,
coverage fields and whether the first API offers a squared-gap completion
criterion. The numerical lane proposes total binary depth 8 initially; this is
a resource choice, not an accuracy guarantee. The oracle lane also identifies
closed-radius filtering as a useful operation on the same hierarchy. Keep its
predicate distinct from nearest-preserving candidate exclusion.

Adversarial design review and then implementation-plan review precede product
code. No new API, bounds qualification or speedup is claimed by this synthesis.

The existing degree/weight/local-frame envelope and shared CPU/CUDA/Python
boundary are the starting point. Keep packed entity identities and original UV
rectangles; avoid pointer-defined geometry or a parallel resource registry.
Separate preprocessing, resident traversal, native complete calls, Python costs
and unresolved populations when measuring value. Owned C++17/CUDA and Python
remain the default; studying Wafer or another reference does not adopt it.

[Issue #8](https://github.com/antimeme-ai/BRepGPU/issues/8) tracks this delivery.
Work proceeds on feat/conservative-discovery with coherent pushed checkpoints
and commit-preserving PR merges for complete study and product milestones.
