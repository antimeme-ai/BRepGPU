# Iteration 3 plan: whole runtime unit review

Review, 2026-10-14, against [the accepted design](../docs/ITERATION_3_DESIGN.md),
[implementation plan](../docs/ITERATION_3_PLAN.md), actual core/CUDA/Python and
CMake files. No product/test implementation or Git mutation in this review.

The ownership split is a manageable whole invariant. The runtime owner owns
canonical ABI/table declarations, shared-registry lifetimes, CUDA token resource
paths, Python native-publication cleanup and guarded fault tooling together.
The numeric owner owns the shared arithmetic/control flow and CPU builder/range,
with no competing edits to CUDA. Root owns measurements/docs; the oracle owner
consumes canonical declarations and checks exported data independently.
No framework or dependency adoption is needed.

## P-R1 — medium clarification: explicitly skip empty query worker admission

The plan explicitly makes the empty patch builder arithmetic-free. It should
make the empty discovery execution branch equally explicit. Existing projection
runtime uses `workers=min(threads,N)` then invokes `range(0,N)` when workers<=1,
including N=0. Copying that branch and putting the new environment check in the
range would reject an empty batch under altered floating-point settings even
though no arithmetic needs qualification.

Correction: after ordinary descriptor/options/handle admission and empty result
allocation, N=0 skips worker construction, the caller range, environment checking
and numeric callbacks. It still publishes a valid empty result and retains its
sources/options. Nonempty single-worker execution checks its actual caller;
nonempty multithread execution checks each actual worker. A direct empty-query
case under a changed rounding/FTZ setting (then restored) attacks this boundary.
The root was notified; integration disposition follows below. This clarification
does not require changing ABI1/2 empty behavior.

## Shared handoff feasibility

Public node80/buildopts32/discoveropts32/hierarchy-info40/result-info48 sizes and
six distinct u32 result slabs match the accepted design. The generic
`bg3_hierarchy_bound_download` carries exact mode-derived extent0/20/490*Nnodes;
node-major cache formulas and padding are fixed. Runtime publishes these
canonical headers before consumers or math/kernel integration. The private v3
factory/table has nine callbacks and exact v1 identity; it is independent of
v2 negotiation. Core diagnostics and close remain ABI1.

The numeric builder returns false only for enclosure/invariant failure;
allocation exceptions preserve OOM identity through existing `call()` and
publication cleanup. The runtime first checks options/degree-aware total node
admission, cache products/size_t/vector bounds and actual caller environment,
then invokes the builder under the same context admission that retains original
patch controls. All invalid/unsupported geometry refuses the entire hierarchy.
Plan implementation should map builder refusal to a clear native diagnostic
without pretending it is a successful empty tree. No descriptor/control capacity
is inferred from device addresses.

CPU range uses full N as field stride plus separate [begin,end). Worker guard
must be noexcept and use owned synchronized failure state; no exception crosses
std::thread. Join all started workers on environment rejection or partial thread
construction failure before private result/scratch destruction. Validate host
rounding domains, FTZ/DAZ and exception masks on actual threads; do not mutate
caller FP state. The math owner supplies inspection and the runtime owns its
admission placement. CPU builder work also exists under a CUDA context.

## Resource and compatibility sequence

Inspecting existing CUDA `Operation`, `Pending<T>`, device-selection/restoration,
copy/allocation helpers and typed cleanup shows that the new tree/cache/query/
result callbacks can extend the owned machinery directly. Add typed cleanup for
all partial node/root/cache/output allocations. Every upload/query/result callback
returns only after stream completion and device restore, publishes no token on
failure, and propagates cleanup fatality without replacing the primary cause.
Actual resident node/root/cache exports run those same checked device/stream
paths. Native hierarchy retains patch, query retains hierarchy, result retains
query/options; backend raw references are safe only under that retained graph.

Extend the core kind registry and plugin's independent v3 state rather than
altering old kinds/tables/callback semantics. Reuse Admission, device_children,
cleanup_child, publish_owned and final process-poison publication check. Public
parent close removes IDs while private retention preserves resource/provenance;
context close invalidates all children and drains them. Probe/refusal/fatal
behavior crosses old/new operation families in the existing fault seam. Do not
catch a transient factory error as stable unsupported capability.

Lazy Python ABI3 symbol declaration/inquiry permits old-core CPU import and
old evaluator/projection usage. New methods snapshot arrays/options before
native call and use `_create_resource` across invocation/handle conversion/
wrapper construction. Sources/downloads own their copied metadata after closes.
Native direct alias sentinels include info descriptors, handle/diagnostic pairs,
node/root/cache exports and source option destinations, with no writes on
preflight refusal. Old ABI1/2 binaries/plugins/adapters are compared in fresh
processes, rather than inferred compatible from header names.

## Three-mode cost and scheduling boundaries

AABB/DOP26/Bernstein enum validation and exact per-node strides participate in
admission. Bernstein plus nodes can request 4,194,304,000 bytes at the node cap,
before roots/control/query/result and simultaneous host/device copies. Node
admission is not an available-memory guarantee. Host/backend OOM drains the
whole unpublished hierarchy; no mode downgrade or combined cache is permitted.
Build/upload/export is reported separately from resident repeated queries and
inside full-operation/amortization comparisons.

The initial 128-thread grid-stride CUDA query wraps the shared numeric algorithm
and independent ScalarCUDA evaluator. Fixed stack13 and streamed coefficient
reads are implementable starting points. ptxas stack/register/spill output and
whole-call timing decide later block-size or staging changes. No persistent
queue or unrelated scheduling framework is required by this plan. Preserve
strict CPU/CUDA compilation, explicit sm_60 cubin and optional CPU-only loading.

The numerical handoff's exact templated evaluator/net/query signature must be
confirmed before kernel wiring. Runtime does not duplicate candidate selection,
pruning, witness bounds or flag/counter semantics. Root's benchmarks consume the
same public ABI and admission rules as ordinary callers; independent exact
cache/witness and analytic-global checks qualify timed geometry. Sanitizers,
modeled faults and CPU parity cannot replace that geometry oracle.

## Execution and review scope

The direct missing-capability red, canonical declaration milestone, actual CPU
integration, actual uploaded-cache/kernel qualification on both P100s and
whole-code cross-review form a coherent sequence. Guarded modeled tests remain
separate from real device-fault claims. Release hooks and ASan/UBSan builds use
isolated target directories. Root performs deployments/builds and records exact
outcomes; this source/plan review asserts no execution outcome. Existing product
suites run after the shared boundary changes; further runs require new changes,
failures or unresolved concerns. No Mac native-support claim is inferred.

No remaining ABI/lifetime/resource-owner design issue was found. Plan acceptance
requires the explicit P-R1 empty-query branch; other notes are implementation
obligations already implied by the reviewed design rather than new blockers.

## Integrated correction re-review and acceptance

Re-read the updated plan: count0 explicitly skips workers, single-worker range
and environment checks while validating/publishing an empty result. P-R1 is
resolved. The plan additionally requires tolerance validation from copied
binary64 bits, preventing signaling-NaN or subnormal comparisons before rejecting
an unsupported exception environment and keeping empty validation arithmetic-free.
The numerical owner confirmed the exact `brepgpu::DiscoveryCandidate` and
`discover_local` signature, including root ownership checks. No remaining
runtime-unit plan blocker. This lane accepts the plan for implementation after
root opens implementation.
