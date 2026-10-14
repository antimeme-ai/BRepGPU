# Discovery runtime and numerical-boundary code review

2026-10-14. Adversarial source review of runtime.cpp, cuda_backend.cu and the
Python adapter after their ABI3 integration, with the canonical discovery
headers and numerical helper contract. This review does not replace target
fault, exact-output or device-memory qualification. No runtime-owned product
files were edited by this reviewer.

Reviewed source SHA-256 checkpoints:

| File | SHA-256 |
| --- | --- |
| native/runtime.cpp | 9a69b064a19959ac448f6813b671ceceac3d4a36ce84ed47d27a718d65dd339a |
| native/cuda_backend.cu | 3da09294aa1d291218344fa004ea0db638f331681ceb0fe633811e2430cda4ca |
| python/brepgpu/__init__.py | f4e657f4608feb52721238d4a6051d6750fb86c012803a6df195e09e722f570e |

**Disposition: no numerical/runtime implementation blocker found.** One
low-severity public preflight wording discrepancy was sent to the runtime and
integration owners for resolution, described below. Fault-seam extension and
qualified CPU/P100 execution were still in progress at this checkpoint.

## Concrete finding

R3-NR1, low severity: `discovery_alias` treats an unrepresentable address/count
span as rejection before `zero_handle`, preserving all output bytes. The header
describes no-write alias rejection but says other creator failures zero a
disjoint valid output. Thus a huge target count/extent with an allocated handle
sentinel can return invalid argument and leave the sentinel unchanged without
a concrete representable overlap. This is safe behavior; forcing zeroing while
an input span cannot be assessed would weaken the no-write guarantee.

Recommended disposition: explicitly include invalid/unrepresentable spans in
the no-write preflight rejection contract, matching implementation. Use an
allocated-sentinel public/native case if not already covered. Actual valid-span
overlap is checked before descriptor/handle/diagnostic mutation. This finding
does not question conservative geometry or require a new approval flow.

Resolved disposition: runtime owner updated the public header to explicitly
include alias or unrepresentable-span preflight rejection in the no-write
contract. No core arithmetic or resource behavior changed. This preserves the
safe implementation and removes the creator-zeroing wording contradiction.
There is no remaining finding blocking qualification at this reviewed checkpoint.

## CPU admission, snapshots and helper use

Hierarchy admission computes exact complete node counts using each patch's
degrees and build depth, checks total max_nodes and all0/20/490 cache extents,
then performs owned CPU arithmetic. Invalid complete-cache construction never
publishes a prefix. Nonempty construction checks the actual caller environment,
including for requested CUDA operation, and revalidates retained binary controls
under that admitted environment. Builder output counts are checked against
admission before upload/publication; allocation exceptions retain existing
normalization and unpublished cleanup.

CPU discovery checks every actual numerical worker before its helper range.
An atomic failure flag is read only after joining all workers; unsupported
workers cause batch unsupported/no result publication. Thread-start failure
joins already-started workers before unwinding. Empty query batches do no
worker/numerical arithmetic. The options tolerance is screened by copied bits,
avoiding sNaN/subnormal comparisons before environment admission. Invalid target
classification occurs in the admitted helper, with uniform NaNs/zero work.

The CPU range gets fullN SoA stride, original descriptor/control offset and
retained root/cache arrays. It checks root membership for the requested slot
and preserves separate statuses/flags/visits/pruned/refinements/evaluations.
No temporary normalized patch is silently substituted for the original carrier.
The shared numerical routine owns gap/cap semantics; runtime does not reinterpret
UNRESOLVED/rank evidence or label stationarity as nearest-point success.

## CUDA wrapper, residency and completion

The CUDA kernel validates patch slot, root range and root.patch_slot, constructs
ScalarCUDA over the original record offset and invokes the same discover_local
control-flow algorithm. Its target triplets and all eight/six output slabs use
the canonical full count. Mode lower-bound and witness arithmetic live in the
shared device-capable header, with directed intrinsics selected by device
compilation; no host-rounded projector distance supplies the certificate.

Hierarchy upload checks mode and exact cache count, retains the same existing
patch token and uploads actual nodes/roots/cache once. Export copies actual
resident arrays. Checked device allocation has partial cleanup under Pending;
query/result buffers remain owned until completion and device restoration
before publication. Discovery returns fresh resident output, not shared mutable
scratch. The128-thread grid-stride wrapper is a measured starting point;
register/stack facts and full-operation timing remain qualification outputs.

Callbacks use inherited Operation/DeviceScope/Pending/stream completion and
bounded diagnostics. Upload/evaluation/download failures cannot publish valid
tokens or partial successful host outputs; result cleanup consumes its token
once. Masked device exceptions require explicit numerical invalid-interval
checks, which the shared arithmetic supplies. Plugin extension factory is pure
and has exact base-v1 identity; missing v3 does not require or alter v2.

## Lifetime, capability and Python publication

Hierarchy retains patch, query retains hierarchy and result retains query/options.
Their token cleanup precedes parent release; core context ownership remains
alive through all child cleanup. Same registry/admission and publish_owned
helpers preserve earlier publication/cleanup failure identity and fatal flags.
Discovery extension negotiation caches stable refusal/success, not transient or
fatal failures. Existing backend_status normalization propagates fatal+OK and
fatal+unsupported anomalies through process poison across operation families.

Python declares ABI3 symbols lazily and checks canonical sizes/counts; old cores
can still import and perform old operations. Creators use the existing
BaseException-aware native-create/wrap-or-close helper. Native source snapshots
and downloaded Python arrays outlive public parents; no hidden native pointer
identity becomes a product patch identifier. Node/cache/export layouts and all
ctypes argument lists match their canonical headers. require_gap checks only
the stated status/gap flag and carries no stationarity/global-body claim.

## Qualification still required

Run actual environment alterations and publication/cleanup/capability/alias
fault cases, retained source close and Python constructor interruption cases,
plus preserved ABI1/2 core/plugin/adapter combinations. Exact cache and witness
oracles establish geometry independently of this source review. Compute
Sanitizer checks actual node/cache/compact-output storage on both P100s; CPU
ASan/UBSan checks owned memory/lifetime. Mode workloads and preprocessing differ,
so full resident/host-consumed timing and useful counters/bracket widths are
needed before default selection. These checks bear on separate concrete fault
classes; no extra generic proof or oracle-for-oracle layer is recommended.
