# CUDA discovery mode specialization: bounded design proposal

Propose three compile-time-mode instances of the existing discovery kernel,
selected once by the host from the immutable hierarchy mode. Initially change
only `native/cuda_backend.cu`: pass the mode constant to the unchanged,
force-inlined shared numerical helpers. Preserve the actual CPU core and
benchmark binaries. This is a resource/code-generation experiment; it has no
established speedup and does not tighten unresolved brackets.

Source inspected at main `1d0762d`, after the commit-preserving iteration-3
merge. The qualified dynamic kernel was reported as 254 registers, 448 bytes
of stack and zero spill loads/stores. Those observations do not identify the
limiting resource or predict the specialized kernels' performance. No code,
build, native execution, remote operation or performance-source edit accompanies
this proposal.

## Source and smallest change

- [Discovery kernel](../native/cuda_backend.cu#L407): one grid-stride loop,
  patch-slot/root checks, scalar evaluator and eight-double/six-integer output
  writes. The runtime `mode` argument currently reaches every node-bound call.
- [Mode preparation and node bounds](../native/discovery_math.hpp#L170):
  `TargetBounds` holds ten interval dots and an interval norm. Every mode uses
  the AABB calculation; DOP and Bernstein add different calculations.
- [Traversal and refinement](../native/discovery_math.hpp#L250): original-net
  interval witness upper, fixed projector, thirteen-entry DFS stack, work caps,
  branch ordering, equality-preserving pruning and final gap classification.
- [Native callback](../native/cuda_backend.cu#L828): result allocation, device
  selection, optional events, launch, synchronization, restoration and token
  publication. [Hierarchy upload](../native/cuda_backend.cu#L768) already admits
  only modes 0/1/2 and records the mode in its owned token.
- [CPU range](../native/discovery_cpu.cpp#L167) calls the same helpers with a
  runtime mode. Its source and binary remain unchanged in this experiment.

Make `discovery_kernel` a template over an unsigned mode with exactly three
instantiations: AABB, DOP26 and Bernstein. Remove its runtime mode parameter;
pass the template constant to the existing `discover_local` call. Its existing
`__forceinline__` call chain can then expose constant mode conditions to the
compiler without copying or rewriting the arithmetic. Add one host switch at
the existing launch site. Keep 128 threads, the current grid cap of 4,096 blocks,
grid-stride indexing and every output address calculation unchanged.

| Instance | Code/data the compiler can discard | What remains |
| --- | --- | --- |
| AABB | DOP target projections/support loads; Bernstein target norm, five-coefficient interval loop and ratios; unused prepared-target members | AABB, scalar/projector work, original-net upper and DFS |
| DOP26 | Bernstein target norm, coefficient loop/ratios and unused norm member | AABB, ten prepared interval dots and ten support pairs per node, common discovery machinery |
| Bernstein | Ten prepared target dots, direction projection and DOP support loop | AABB, interval target norm, five active coefficient intervals and ratio calculation, common discovery machinery |

Each launch already has one mode, so this removes runtime uniform choices;
it does not eliminate mode divergence between lanes. Geometry, degree and
per-query traversal/refinement work can still diverge. Common projector and
interval-evaluation scratch may dominate register/stack requirements. The
compiler may already shorten unused live ranges in the dynamic kernel.
Constant propagation and reduced resources must be inspected in the emitted
binary, not assumed from the template spelling.

Do not initially add specialized prepared-target types, template patch degrees,
unroll Bernstein coefficients, change launch bounds/register caps, reduce
threads, split queues or specialize refinement budgets. Those changes would
introduce additional numerical/scheduling variables. If constant propagation
does not remove the irrelevant mode paths, report that result and review a
separate explicit compile-time helper design rather than expanding this one
silently.

## Numerical, lifetime and profiling invariants

The public ABI1/2/3, private v1/v2/v3 tables/factories, hierarchy cache layout,
query/result ownership and lazy loading remain unchanged. Dispatch uses the
retained hierarchy token's admitted mode, never a new caller/query field.
Validate the retained private mode before allocation and before the nonempty
launch guard. An unrecognized mode must report an evaluation error through the
existing `Operation`; it must never default to AABB or publish even an empty
result. Public upload admission already prevents that mode.

Exactly one kernel is launched per nonempty operation. Keep the launch inside
the existing `DeviceScope` and context stream, followed by the existing
`cudaGetLastError`, stream finish and explicit restoration before publication.
Keep pending-result drainage, cleanup-error accumulation and process-fatal
propagation unchanged. Empty results skip dispatch, kernel launch and profiling
events; their unprofiled metadata remains zero flags/NaN duration. No kernel
pointer is stored in a public handle or introduced as a new owned resource.

Record optional start/stop events around the selected kernel at the same
locations. `kernel_ms` retains the existing stream-event bracket around that
one launch. The start event is enqueued before the host computes the grid and
dispatches, so its duration may include a submission/idle gap; it is not an
assertion of pure device execution time. Allocation precedes the event bracket;
synchronization/restoration and download retain their existing native scopes.
No event may measure a separate dynamic fallback instead of the selected kernel.

Preserve all operations and their sequence in the shared arithmetic: directed
FP64 primitives, nested additions, negative Bernstein numerator denominator
choice, active `(2p+1)(2q+1)` coefficients, strict `lower > upper` pruning,
`left <= right` ordering, original-net control offset, child-bound visit
reservation and root-lower cap fallback. Keep the same compiler/toolchain and
[CMake numerical flags](../CMakeLists.txt#L43), including disabled FMA/FTZ and
precise division/square root. Degree (0,0) still consumes exactly one active
Bernstein coefficient per polynomial, ignores zero padding and uses the
root-once refinement rule. AABB still accepts a null, zero-extent bound cache.

## Focused qualification before measuring

Preserve the qualified dynamic plugin intact before building. Run the following
against dynamic and specialized plugins on each P100 through the same preserved
core/adapter. Independent exact geometry is the truth oracle; old/new output
identity separately attacks the promised unchanged-operation contract.

1. Reuse the exact original-input discovery corpus and exported-node/cache
   oracle for all three modes, including negative C0, positive denominators,
   inactive padding, original control offsets, subnormals, competing/tied
   minima and capped cases. Require zero certificate violations.
2. Compare all eight returned binary64 fields and six integer slabs between the
   two plugins for these identical cases, excluding profile duration. Require
   bitwise identity by comparing raw double/integer buffer bytes, including
   canonical invalid NaNs and signed zeros, rather than Python float equality. Do not
   replace a difference with CPU parity or an epsilon; investigate it.
3. Extend the existing direct invalid-row and empty/tail cases across all three
   modes: invalid slot plus invalid target must remain BAD_PATCH first; valid
   slot with NaN/Inf/out-of-envelope target must remain BAD_TARGET, eight NaNs
   and zero work. Include counts 0, 1, 127, 128, 129 and 257. The current
   [invalid-row test](../tests/test_discovery.py#L141) uses default AABB; the
   current [tail test](../tests/test_discovery.py#L125) uses Bernstein only.
4. Exercise degree (0,0), mixed degrees/slots, maximum admitted degree/depth,
   `max_refinements=0,1,8,64`, visit budgets 1/2 and full traversal, and both
   GAP-with-cap and unresolved-complete rows. Require existing exact counters,
   flags and original-UV upper inequalities. Reuse
   [case definitions](../python/brepgpu/discovery_fixtures.py#L208) rather than
   inventing another numerical reference.
5. For each mode, compare profile-on/off numerical outputs; nonempty profile
   returns the profiled flag and finite nonnegative duration, and empty profile
   returns no measured duration. Reuse actual per-thread device-restoration,
   parent-close/result-retention and preflight-alias cases. Run discovery paths
   under Compute Sanitizer on both cards. Existing modeled publication/fatal
   cleanup cases cover their unchanged host machinery, not real hardware faults.
6. Smoke the existing ABI1 evaluator and ABI2 projection with the new plugin and
   preserved core. Their tables and entry points have not been redesigned.

New cross-mode boundary cases should run against the preserved dynamic plugin
before implementing specialization. They should pass: this optimization adds
no desired semantic difference. Do not invent an expected functional red or a
timing assertion simply to certify an optimization.

## Controlled old/new comparison and decision

Wait for the current CPU1 trials to finish before any target build/copy or new
job. The coordinator retains the exact CPU1/CPU24 baseline; do not rerun or
recompile CPU work for this GPU-only change. Prepare two isolated variant
directories containing byte-identical core and native benchmark binaries, with
only the sibling CUDA plugin differing. Native plugin discovery follows the
loaded core's directory; Python qualification can use explicit `plugin_path`.
Do not overwrite protected originals. Record source/plugin/core/benchmark and
toolchain identities, plus per-mode ptxas registers/stack/spills and binary
inspection of eliminated paths. Lower resource counts alone are not success.

Use the unchanged native benchmark and seven repetitions per case, after its
excluded warmup, on each P100 separately. An initial compact matrix is:

- Cylinder, depth 8, all modes, counts 1/256/4,096/65,536: launch overhead,
  saturated batches and the existing headline.
- Cylinder-offgrid and axis-offgrid, depth 8, all modes, count 4,096: attained
  widths and unresolved populations alongside cost.
- Diagonal DOP, depths 0/8, and axis-midpoint Bernstein, depths 2/8, count 4,096:
  shallow sufficient versus deeper trees.
- Capped cylinder, depth 8, all modes, count 4,096, refinements 0/8: very short
  and common-projector work. The native corpus keeps visit budget 1.

Alternate dynamic/specialized case order, with no concurrent build, copy,
qualification or other owned performance job. If an apparent gain/regression
is within the observed spread or depends on order, repeat that comparison in
reverse order rather than broadening the whole matrix. Retain full requested
condition keys, all samples, min/median/max, achieved widths, populations and
work histograms. Compare resident, CUDA-event bracket and cached-hierarchy
host-to-host phases separately. Setup, cleanup and first-use phases retain their
existing scopes; no sum of independent medians becomes an end-to-end result.
The bounded repeated-input geometry remains the same; specialization does not
turn the 60-target regular cylinder into an independent-target CAD workload.

Accept specialization only with unchanged qualified results and a useful measured
native-phase benefit without material regressions in the admitted workloads.
If one mode regresses, preserving its dynamic path is a separate explicit
dispatch decision that must be reviewed and measured. Keep the current kernel
if there is no useful benefit. Improved off-grid bounds and shallow hierarchy
selection remain distinct numerical/preprocessing work, as identified by the
[performance review](2026-10-14-discovery-performance-review.md).
