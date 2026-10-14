# Iteration 3 design: adversarial runtime review

Review, 2026-10-14, against [the integrated design](../docs/ITERATION_3_DESIGN.md),
[runtime design input](2026-10-14-discovery-runtime-design.md), actual
[core](../native/runtime.cpp), [Python adapter](../python/brepgpu/__init__.py),
and private v1/v2 backend headers. This is a design/source review, not execution
of a new operation. No product/test code or dependencies were added.

Disposition: the shared-registry/retained-lifetime/additive-extension architecture
is implementable without breaking existing evaluator or projection APIs. Both medium findings below are resolved after re-reading the integrated
corrections. No runtime/ABI/lifetime design blocker remains.

## R-R1 — medium, blocking: alias/no-write rules omit info descriptors

The creator paragraph rejects written storage overlapping read descriptors or
payloads, and the next paragraph specifies downloads. It does not explicitly
cover written handle versus diagnostic, or the output descriptor versus diagnostic
in `bg3_hierarchy_info` / `bg3_discovery_info`. These are real written regions,
not merely array payloads. The current core's `call()` writes the supplied
diagnostic after the operation; copying a result-info record into aliased
storage then returning BG_OK would be followed by a diagnostic overwrite.
Version/size prefixes alone do not prevent all actual allocated overlap layouts.
Likewise zeroing a handle that overlaps the diagnostic can destroy the latter's
prefix before validation. The previous projection download alias finding
established exactly this class of failure.

Correction: state the rule for **every ABI3 function**, not only downloads:
all written output regions are mutually disjoint and separate from diagnostics;
creator outputs/diagnostics are additionally separate from every borrowed input
record/payload. Source-export option destinations are outputs too. Include
canonical descriptor/diagnostic sizeof regions even when byte capacities are
larger. Run no-write alias preflight before `zero_handle()` or `call()` and
return BG_INVALID_ARGUMENT without writing any aliased region. Apply it to
info, hierarchy/node/root/cache export, compact results and sources. A direct
allocated sentinel case should alias an info destination with a diagnostic;
all bytes must remain unchanged on refusal. Do not change ABI1/2 contracts.

## R-R2 — medium, blocking: exported cache indexing is not fully normative

The design specifies 20 DOP doubles and five blocks of 49 Bernstein interval
pairs per node. It gives DOP direction order and inactive zero padding, but not
the complete exported node/polynomial/coefficient index formula. An exact oracle
or separately compiled backend can read a same-sized cache with a different
u/v coefficient ordering and silently associate coefficients with different
cells. The generic export makes this an API/qualification invariant.

Correction: fix a node-major layout and endpoint order in the design/header.
DOP offset is `node*20 + axis*2 + endpoint`, endpoint0=lower/1=upper, axes in the
listed order. Bernstein offset is `node*490 + polynomial*98 + coefficient*2 +
endpoint`, polynomials [ΣA²,AxW,AyW,AzW,W²]. Choose one explicit
`(u,v)->coefficient` formula matching the owned control indexing; for example
u-major active prefix `coefficient=u*(2q+1)+v`, with u=0..2p and v=0..2q.
Exactly the first K pairs are active and the remaining 49−K are zero. CPU
construction, GPU query and exported exact oracle consume that same ordering.
Do not infer degree/stride from pointer addresses or inactive values.

## Reviewed whole ownership/admission unit

The existing core has one kind-tagged u64 registry, context admission lock,
lookup/promote/recheck admission and process-fatal state. Add hierarchy/query/
result kinds there; keep old table/POD sizes exact. Child-to-parent shared
ownership retains immutable tree/patch/query inputs after public close while
context close invalidates all public IDs. Existing `cleanup_child`,
`publish_owned` and final poison checks already cover completed native tokens
whose registry publication fails, including original-OOM plus fatal cleanup.
Reuse them for node/root/cache tokens rather than introducing parallel registries.

The private v3 table must remain pure, separate from v1/v2 and tied to the exact
base-v1 pointer of its plugin. No v2 negotiation is needed to reuse the compiled
numeric projector internally. Stable unsupported capability may be cached;
transient/fatal factory behavior must propagate through existing normalization,
including fatal+OK or fatal+UNSUPPORTED. Per-thread device select/restore and
synchronous completion apply to upload, query, discover, both hierarchy exports,
compact download and every cleanup callback. Source snapshots use retained host
arrays while honoring current CUDA poison/admission rules.

The complete binary-tree build can retain depth-bounded temporary interval nets.
Degree-(0,0) patches contribute one node; other accepted patches contribute
2^(d+1)−1. Checked totals precede allocation and cannot overflow u32 at the
admitted cap. A failed/unsupported patch refuses the entire unpublished build.
No requested mode or tree depth may be silently downgraded to satisfy storage.

## Three-mode storage admission and practical implementation

Build flags are an enum 0/1/2, not bits; 3 rejects. Derive cache stride by exact
mode equality: 0/20/490 doubles per node. Preserve node80 and buildopts32.
Checked calculations must include u64 products, size_t conversion, vector maxima,
root count, node bytes and cache bytes before allocation. At 1,048,576 nodes:

| Storage | Bytes |
| --- | --- |
| Base nodes | 83,886,080 |
| DOP cache | 167,772,160 |
| Bernstein cache | 4,110,417,920 |
| Bernstein plus base nodes | 4,194,304,000 |

These numbers exclude roots, retained controls, query/result/scratch, allocator
overhead and simultaneous host/device copies. The large maximum is an admitted
request size, not an availability guarantee. Catch host/backend OOM, drain every
unpublished allocation and publish no ID. Full exported cache/node/root data
must be actual resident GPU contents; cached host copies alone do not attack
upload/storage faults. Bernstein padding remains unused by the query.

Build options/mode belong in result provenance. `Hierarchy.info.flags` carries
bound mode, while `Discovery.info.flags` carries existing result profiling flags;
name the separate domains so numeric value1 does not mix DOP with PROFILED.
Mode-specific build/upload/storage and exact export costs stay outside resident
query-only timing but inside reported setup/amortization and full-call measures.
Do not adopt stage queues or persistent cross-block scheduling without measured
need; depth-bounded DFS is the accepted first operation.

## CPU environment, concurrency and worker failures

The design correctly requires caller/actual-worker environment validation and
rejects unsupported modes without mutation. Hierarchy construction validates
CPU state even under a CUDA engine. The plan must implement RN checking for
both supported host control domains, MXCSR FTZ/DAZ clear, masked exceptions and
consistent supported execution; `fegetround()` alone cannot establish that.
A direct environment test should change MXCSR rounding independently as well as
FTZ/DAZ or exception masks and expect batch BG_UNSUPPORTED/out0 before published
numerical results. This is a plan requirement, not a new design blocker.

Noexcept worker wrappers report setup failure into owned synchronized state,
join all started workers and discard unpublished output if any worker fails.
Partial thread-start failure joins existing workers before scratch destruction.
No process-global FP mutation/lock is needed. Define empty patch/query paths as
arithmetic-free: validate descriptors/options and publish empty objects without
worker/environment checks. Single-worker execution checks its actual caller.
The full-N stride/[begin,end) helper and predetermined scratch capacity remain
explicit even when there is only one request.

## Python and compatibility implementation obligations

Lazy ABI3 declaration/size inquiry occurs only at first hierarchy/discovery use.
Old cores still import and run ABI1/2 CPU operations; missing v3 plugins refuse
only the new operation. Source/target/options copying stays private before
native validation/upload and is measured in host-consumed timing. Every creator
uses existing `_create_resource` guarding native invocation, output-handle
extraction and wrapper construction under BaseException cleanup. Results own
plain downloaded data/provenance after native closes; require_gap describes
only the stated squared-bracket criterion, never nearest/stationary truth.

Extend the existing modeled callback seam to cover tree/cache/query/result
publication and cleanup failures, actual alias rejection and cross-family fatal
poison. Retain direct raw kind/foreign/stale tests, concurrency admission-close
checks, exact hierarchy export and original v1/v2 binary/plugin compatibility.
These mechanisms attack different faults; CPU parity or sanitizer success cannot
replace interval geometry qualification. No native implementation is reviewed
or claimed here.

## Disposition record

R-R1 and R-R2 were sent to the root with concrete corrections before plan work.
Append integrated-design dispositions after re-reading the corrections. Other
reviewed runtime/lifetime/mode-storage machinery has no design blocker.

## Integrated correction re-review

Re-read the current integrated design after the root's corrections. R-R1 is
resolved: every ABI3 info/source/download checks descriptor and payload outputs
against diagnostic storage before `call()`; creator handle-diagnostic alias is
explicit; canonical used sizes and unchanged aliased bytes are stated. R-R2 is
resolved: node-major cache offsets, endpoint order, polynomial order,
`a*(2q+1)+b` active coefficient indexing and exact ignored zero padding are
normative for construction/upload/download.

The root also clarified that the fixed depth-12 DFS stack has 13 entries;
overflow is an arithmetic/invariant failure, not a supported budget with a
fallback certificate. Only the visit cap uses the root lower-bound fallback.
That distinction agrees with the reviewed immutable complete-tree admission.
No remaining design blocker in this lane. Plan review must still check concrete
FPenv/worker error paths, cache byte/count validation, empty arithmetic-free
paths, actual export callbacks and existing failure-seam integration.
