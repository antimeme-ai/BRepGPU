# CUDA discovery specialization: callback and measurement design review

Reviewed the specialization design against the retained CUDA callback/runtime,
private extension tables and native discovery benchmark. This is source review
only: no product changes, builds, tests, fleet jobs or performance execution.

The proposal is suitable to advance to a plan after the two clarifications
below. No blocker was found in the numerical operation, ABI, ownership graph or
proposed scope of specialization. A speedup remains unestablished.

## R-MODE-1: reject invalid modes independently of nonempty dispatch

The design promises that an unrecognized private retained mode reports an
evaluation error and cannot publish a result. It also correctly requires empty
requests to skip dispatch, launches and profiling events. The current
[discover callback](../native/cuda_backend.cu#L828) publishes an empty result
after its nonempty launch block. Placing the only invalid-mode rejection in the
new launch switch would therefore leave empty requests able to publish.

Validate the retained hierarchy mode before the nonempty launch guard, using
the existing `Operation` at evaluation stage. Keep a defensive non-fallback
default in the dispatch switch. An empty request with an admitted mode must
continue to publish its empty result with zero profile flags and NaN duration;
invalid internal modes must leave the output token null and not create events.
This is a private invariant check, not a new caller field or ABI requirement.
[Hierarchy upload](../native/cuda_backend.cu#L768) already admits only 0/1/2,
so ordinary public construction cannot produce this invalid mode.

## R-MODE-2: qualify the event measurement's exclusion language

The design says host dispatch remains outside event time. The current callback
enqueues the start event before host-side grid calculation and kernel
submission. The proposed switch is also between start-event submission and the
selected launch. Keeping these locations does not establish that host dispatch
delay is excluded: the device can reach the start timestamp before the host
submits the kernel. Describe `kernel_ms` as the CUDA stream-event bracket around
one selected launch, which may include a submission gap, rather than asserting
it is pure device execution time or excludes host dispatch.

Allocation precedes the start event; download follows the callback. Host
synchronization, event elapsed-time readback, event destruction and device
restoration occur after stop-event submission. Their callback wall costs stay
in resident/native timing, separately from the event interval. Preserve those
locations rather than changing timing scope during this optimization.

## Callback, ABI and ownership findings

The smallest proposed change is coherent: one compile-time mode parameter and
one host dispatch, with immutable `Hierarchy::mode` selecting it. The current
mode is uniform across a launch; specialization supplies a compiler constant
rather than removing existing inter-lane mode divergence. It must preserve
invalid-slot precedence, root/source checks, full-count SoA indexing, all six
integer slabs and eight FP64 fields. No scheduler, degree, tolerance, queue,
refinement or shared arithmetic change is justified by this proposal.

[DeviceScope](../native/cuda_backend.cu#L103) records restoration failure in the
same `Operation`; the callback's scope closes before profile/token publication.
[Pending](../native/cuda_backend.cu#L258) consumes unpublished results once,
draining the stream and attempting all frees. Existing finish, event cleanup,
fatal aggregation and restoration must run even when the new switch reports an
error. Avoid returning directly from a dispatch default after event creation.

The public ABI and [v3 private table](../native/discovery_backend_api.h) need no
new entry, layout or capability. The runtime retains the query/hierarchy/patch
graph and plugin base identity independently of which kernel is launched. No
function pointer should become a public entity identity or native handle.
Preserving ABI1 evaluator and ABI2 projector smoke checks is appropriate because
the CUDA translation unit contains all three families even though this change
targets discovery only.

## Qualification and task-value measurement

The design separates direct exact geometry from old/new bit identity correctly.
Reuse the existing independent restriction/cache/original-UV corpus for geometry;
compare copied byte representations for binary64 identity, rather than Python
numeric equality on NaNs. Profile durations are excluded from identity, while
profile metadata rules and profile-on/off numerical slabs remain checked.
All-mode invalid, empty and 127/128/129/tail cases directly attack dispatch and
output indexing. Actual callback restoration and parent-close retention should
continue through every selected mode without inventing GPU fault injection.

The compact performance matrix covers common cylinder work, off-grid unresolved
answers, shallow sufficient trees, cap-limited work and projector costs. Keeping
exact CPU and native benchmark binaries, isolated sibling plugins, sequential
card execution, alternate variant order and all seven samples controls the
specific GPU-only comparison. The native benchmark has separate warmups for
unprofiled and profiled contexts and already emits resident, cached-hierarchy
host-to-host and event phases plus work populations. Resident time includes
native result allocation and completion/restoration; cached-hierarchy
host-to-host includes its existing analytic consume checks. Their medians cannot
be added into a complete caller time.

Per-mode registers/stack/spills and eliminated code paths test whether the
compiler exploited the constant. They do not prove throughput or reduced
memory traffic. Accepting a specialized plugin still requires useful measured
native-phase benefit without material workload regressions and unchanged
precision/work populations. Any dynamic-path retention for a regressing mode
needs the explicit follow-up dispatch decision described in the design.

## Disposition

Advance after R-MODE-1/R-MODE-2 are made explicit. No additional framework,
numerical reference, new public ABI or broad performance matrix is required.
The implementation plan should name the private-mode check location, normal
cleanup path on dispatch failure, byte comparison and preserved timing scopes.

Re-read corrected proposal: R-MODE-1 is resolved by explicit private-mode
validation before allocation and the nonempty guard; R-MODE-2 is resolved by
the stream-event/submission-gap description. Byte comparison is also explicit.
The corrected design has no remaining blocker and is ready for plan review.
