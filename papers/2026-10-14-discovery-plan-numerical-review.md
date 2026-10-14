# Iteration 3 implementation-plan numerical review

2026-10-14. Reviewed [implementation plan](../docs/ITERATION_3_PLAN.md) against
the reviewed [design](../docs/ITERATION_3_DESIGN.md),
[numerical algorithm input](2026-10-14-discovery-numerical-design.md),
[three-mode arithmetic/experiment input](2026-10-14-discovery-strong-bounds-design.md),
canonical existing evaluator/projection helper contracts and scalar adapter code.
No product or test implementation was written during this review. Reviewed
plan SHA-256: `204f1a084b487a2f1a4f64bdf5809315cd89aab7d6eb0ebf52f0e5387ed84e34`;
integrated design SHA-256:
`d047b4b146042cba32e60a9cbf04c3639796a2b3517817c148ade38027502ee0`.

**Disposition: accepted; no remaining numerical plan blocker.** Numerical
construction, interval arithmetic, failure/counter invariants and shared bounded
query decisions remain one ownership unit. Runtime owns the resource graph,
actual-worker environment admission and CUDA callback/kernel wrapper; it
consumes the shared algorithm rather than copying its decisions. All three
cache modes are a concrete part of this delivery before default selection.

## Handoff and failure review

The CPU builder's false return means an enclosure/tree invariant failure.
Allocation exceptions propagate so runtime can preserve OOM identity and clean
unpublished vectors/tokens. Do not convert an allocation exception to a generic
false or publish whatever prefix was built. Runtime performs checked node/byte
admission for0/20/490 doubles per node first, and numerical construction checks
the actual interval/index invariants. A valid zero-patch set performs no
arithmetic. Allocation and arithmetic failure use distinct existing batch paths.

`discovery_environment_supported()` inspects the actual executing thread without
mutating its state. Runtime calls it for nonempty CPU construction (including
CUDA hierarchy construction) and every actual CPU numerical worker. Unsupported
workers are joined and the operation publishes no result, rather than emitting
apparently certified rows. Empty discovery ranges do no numerical work.
Compile-time/platform checks and RN/MXCSR/gradual-underflow/exception-mask checks
remain explicit. No unsupported Mac or generic-C++17 support claim follows
from local editing or successful source parsing.

`discover_cpu_range` receives the full output count as SoA stride plus a separate
`[begin,end)` range, just like existing helpers. Original patch offsets remain
record offsets, not per-selected-patch rebased assumptions. Node/root/cache
arrays stay immutable. The runtime validates complete extents/ownership before
these private helper calls; numerical code validates mode/index/state invariants
needed for safe local traversal. CUDA wraps original device controls and the
shared helper with its independent tensor evaluator.

Suggested exact shared declaration, delivered to runtime before kernel wiring:

```cpp
namespace brepgpu {
template<class Evaluator>
DiscoveryCandidate discover_local(Evaluator& evaluator,
    const double* original_net, unsigned p, unsigned q,
    const bg_hierarchy_node* nodes, uint64_t node_count, uint32_t root,
    const double* bounds, uint32_t mode, const double* target,
    const bg_discovery_options& options) noexcept;
}
```

`DiscoveryCandidate` holds fields[8] plus the six scalar integer outputs in the
plan. Root/child patch identity is checked against root.patch_slot inside local
traversal; runtime/kernel callers also confirm the selected root belongs to the
requested patch slot before invoking it. The canonical public/private headers
are runtime-owned and arrive before dependent implementation. This declaration
is a handoff clarification, not a changed public ABI.

## Algorithms and bounded resources

The plan preserves outward homogeneous midpoint restriction, original-input
point enclosures and precise upward witness/downward bound operations. DOP
uses interval target projections, exact integer norms and maximum separating
slab bounds; Bernstein uses enclosed exact integer-quotient product factors,
negative C0 intervals, positive active C4 coefficients and ignores padding.
Its query streams coefficients; the sign-aware one-division lower rule does
not require storing a49-coefficient scratch array in each GPU thread.

Root scalar display evaluation plus projector callbacks have the count bound
53313. Valid interval witnesses cost1+refinements separately. Node lower bounds
are calculated once, stored with IDs, ordered near-first/tie-left and reserve
two visits before expansion. Fixed stack13 is sufficient for admitted depth12;
overflow is arithmetic failure. Complete traversal and root-lower visit-cap
fallback retain the exact-input bracket invariant. Strict LB>U preserves equal
minimizers; root-leaf duplicate refinement is skipped. Gap satisfaction stays
independent of complete coverage and withheld-refinement flags.

Starting128 threads/block and grid-stride rows are concrete first choices,
not an assumed optimal schedule. Shared header inlining can combine solver,
interval witness and mode arithmetic lifetimes; ptxas register/stack/spill
inspection must precede optimization claims. Bernstein cache memory and actual
allocation/synchronization belong to whole-operation measurements. The plan
does not impose query-time subdivision, growing frontiers or coefficient nets
that would undo the bounded-residency design.

## Direct qualification and measured sequence

The direct red cases exercise the real missing capability/helper contract before
implementation. Exact power-basis restriction independently checks every node's
homogeneous-derived Cartesian controls, supports and quadratic coefficients;
exact original-input UV evaluation checks U without an epsilon. Analytic global
cases independently check L<=d*²<=U. Equality-pruning mutation needs a genuine
exact-zero CUDA row or a direct production predicate case because CPU nextafter
can make exact-operation intervals wider; this does not require an artificial
test of an oracle. All numerical/native execution and mutations stay on the
qualified fleet as the plan requires.

The complete three-mode qualification happens before timing. Comparisons include
depths0/2/4/8, diagonal/axis planes, supplied rational cylinders, competing/flat
minima and caps, with same targets/budgets/meaning. Publish setup/cache/upload,
full resident call and host-consumption measurements, actual work counts,
achieved bracket precision and amortization rather than choosing from pruning
counts or a single kernel sample. CPU1/24 and both P100s are named separately.
The seven-sample plan and artifact-producing caller/figures are concrete and
bounded; existing suites rerun once after shared-boundary source changes.

Actual old ABI1/2 binaries/adapters/plugins, resource faults and caller sentinels
exercise compatibility/publication failure as a distinct mechanism from exact
geometry oracles. Neither parity nor memory sanitization substitutes for the
enclosure claim. No additional generic proof framework, dependency or ceremony
is required before this implementation sequence.
