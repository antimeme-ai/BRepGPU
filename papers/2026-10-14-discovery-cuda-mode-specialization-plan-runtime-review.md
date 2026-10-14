# CUDA mode-specialization plan: runtime and measurement review

Reviewed docs/CUDA_DISCOVERY_MODE_PLAN.md against the corrected design and
current CUDA callback, cleanup, ABI and native timing sources. Accepted with
no blockers. This review writes only the report and matching journal; it runs
no product code, build, test, copy, target or performance job.

The permitted delta remains one CUDA kernel mode template and host selection.
The optional small templated launch helper removes repetitive argument lists
without adding ownership or changing arithmetic. Shared helpers, CPU sources,
core/adapter ABI and binaries, hierarchy layout, numerical flags, launch size
and native benchmark stay unchanged. The plan's early retained-mode admission
is before pending-result construction and the empty guard, resolving the empty
publication issue from design review. Valid empty calls still create neither
events nor a launch. A defensive switch failure must record its error without
returning past finish, event cleanup, scope restoration or Pending drainage.
Source review rather than a production corruption hook is the appropriate
check for the unreachable public invalid-token invariant.

The boundary clients attack actual differences introduced by selection: every
mode, tails around128 lanes, zero cache extent, canonical invalid fields and
precedence, original control offsets, active coefficients and retained children.
Profile-on/off byte equality, empty metadata and caller/worker device restoration
cover the existing callback obligations through the selected kernels. Keeping
actual backend unavailability as failure prevents accidental CPU qualification.
Fresh-process capture on the preserved dynamic plugin before the product delta
establishes an actual operation baseline without inventing a functional red.

The identity client compares slab bytes and deterministic case/options/source
metadata. It must treat selected plugin identity as recorded provenance, not
metadata required to equal across variants: that identity deliberately changes.
Core and benchmark identities must equal. NaN payloads and signed zeros are
compared as bytes, while duration is excluded and profile rules remain checked.
The independent Fraction qualifier supplies geometry truth separately; the
identity client need not recreate a second splitter, traversal or solver.
One full qualifier run per new plugin/card and focused new sanitizer cases are
proportionate to this CUDA-only change; no fresh CPU/mutation/fault campaign is
implied for unchanged machinery. ABI1/ABI2 smoke calls cover translation-unit
integration without extending the task into another feature.

Staging preserves the actual old plugin rather than rebuilding old source and
uses isolated sibling directories with identical core and benchmark bytes.
This matches current core-relative plugin discovery. Coordinator ownership and
the explicit wait for the scalar queue prevent build/copy/qualification jobs
from contaminating baseline times. Isolated directories and fresh outputs also
preserve each variant's source/binary identity for interpretation.

The28 matched conditions per variant/card cover overhead, large batches,
off-grid wide brackets, shallow sufficient bounds and capped/refinement work.
Seven samples after existing warmups, sequential cards, alternate variant order
and selective reverse-order repeats are sufficient for the proposed bounded
experiment. Work and precision populations remain visible so shorter latency
cannot be credited to changed stopping/quality. Resident, stream-event bracket
and cached-hierarchy host-to-host phases stay distinct; the event bracket may
include submission delay, and host-to-host retains its analytic consume cost.
No sum of independent medians is presented as caller wall time.

The proposed5% large-batch threshold, both-card reverse-order confirmation and
small-batch absolute/relative investigation rule make the retain/reject decision
concrete. They are experimental thresholds rather than CI promises. Resource
reductions or event-time gains alone do not satisfy native task value. A mixed
dynamic/specialized mode policy remains a separately reviewed decision if any
mode regresses, so it cannot creep into the initial delta.

Disposition: the plan implements the accepted design corrections and is ready
for coordinated implementation. No remaining callback, ABI, ownership,
measurement-scope or staging blocker was found. Actual emitted code and target
execution determine whether specialization is worth retaining.
