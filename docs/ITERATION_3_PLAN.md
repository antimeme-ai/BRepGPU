# Iteration 3 implementation and qualification plan

Reviewed plan, 2026-10-14. The three design lanes accepted the
[integrated design](ITERATION_3_DESIGN.md). This plan implements one complete
resident discovery operation, including the operator-requested three-mode
literature comparison. No new dependency, pointer-defined public identity,
trimmed-face proximity claim or implicit backend fallback is introduced.

[Numerical](../papers/2026-10-14-discovery-plan-numerical-review.md),
[runtime](../papers/2026-10-14-discovery-plan-runtime-review.md) and
[oracle](../papers/2026-10-14-discovery-plan-oracle-review.md) review accepted
this plan. The runtime empty-query clarification is incorporated below;
implementation may proceed in the stated whole-unit ownership lanes.

## Whole implementation units and file ownership

1. **Numerical owner:** `native/discovery_math.hpp`,
   `native/discovery_cpu.cpp`, `native/discovery_cpu.h`, native direct numerical
   check. Own all interval operators, complete CPU hierarchy/cache construction,
   exact dyadic IDs, CPU environment inspection, shared allocation-free bounded
   query algorithm and CPU scalar adapter. The shared algorithm uses the owned
   projector and original-patch witness, with AABB/DOP/Bernstein modes. It owns
   numerical failure/counter/flag semantics as one invariant. No runtime or
   CUDA-backend file edits by this owner.
2. **Runtime owner:** public/private discovery headers, `native/runtime.cpp`,
   `native/cuda_backend.cu`, `python/brepgpu/__init__.py`, existing fault backend,
   native/Python lifecycle fault tests and CMake integration. Own the additive
   ABI3 shared-registry graph, snapshots, checked admission, actual worker guards,
   CUDA tokens/uploads/kernels/downloads/cleanup, lazy Python declarations and
   native-publication/wrapper cleanup. The CUDA query wraps the shared numeric
   algorithm with the existing independent `ScalarCUDA` evaluator; it does not
   duplicate the optimization or exclusion decisions.
3. **Oracle owner:** `python/brepgpu/discovery_fixtures.py`,
   `scripts/qualify_discovery.py`, `tests/test_discovery.py`. Own independent
   exact polynomial restriction/coefficient/support/witness qualification and
   analytic global answers, public contracts/caps/provenance checks and actual
   caller data. No numerical product or runtime file edits by this owner.
4. **Root:** `benchmarks/discovery/`, example seed-free caller, owned SVG
   figures, results/report, README/operation guide/journal/issue/PR integration.
   Own full-operation comparisons and final cross-review integration. Only
   root creates commits, pushes, performs source deployment and merges.

Stage within each unit; numerical and runtime implementation can run concurrently
after this plan's review because the shared declarations below are fixed. The
oracle first writes direct red cases; unrelated runtime fault extensions may
proceed while the numeric implementation is incomplete. Each owner returns
reviewable files and direct checks rather than a grab bag of tasks.

## Fixed shared handoff

The runtime owner writes `brepgpu_discovery.h` and
`discovery_backend_api.h` first, using the accepted design and runtime input.
Canonical public APIs include `bg3_hierarchy_bound_download`; compact results
use eight double fields and six separate u32 arrays, not a new binary serialization.
The private v3 table has nine callbacks and exact base-v1 identity. Modes are
enum values 0/1/2, cache strides 0/20/490 doubles/node, exported node-major.

The numeric C++ header declares:

```cpp
bool discovery_environment_supported() noexcept;
bool build_discovery_hierarchy(const bg_patch_desc* patches, uint64_t patch_count,
    const double* controls, const bg_hierarchy_options& options,
    std::vector<bg_hierarchy_node>& nodes, std::vector<uint32_t>& roots,
    std::vector<double>& bounds);
void discover_cpu_range(const bg_patch_desc* patches, uint64_t patch_count,
    const double* controls, const bg_hierarchy_node* nodes, uint64_t node_count,
    const uint32_t* roots, const double* bounds, uint32_t mode,
    const uint32_t* slots, const double* targets, uint64_t count,
    const bg_discovery_options* options, uint64_t begin, uint64_t end,
    double* fields, uint32_t* statuses, uint32_t* flags, uint32_t* visits,
    uint32_t* pruned, uint32_t* refinements, uint32_t* evaluations) noexcept;
```

Builder false means enclosure/invariant failure; allocation exceptions propagate
to existing runtime normalization rather than losing OOM identity. Runtime
checks admission/node/byte products first; numeric construction checks its own
interval/tree invariants. It accepts empty patches without arithmetic. The
numeric owner supplies supported actual-thread environment inspection; runtime
calls it before nonempty builder work and inside every actual CPU worker.
No source mutation or environment mutation is needed.
For count=0, explicitly skip both worker creation and the single-worker range
path/environment check. Still validate handles/options and publish the empty
result. Do not copy ABI2's `workers<=1` branch unconditionally: that branch
would run a newly guarded empty range. Validate tolerance's finite [0,1] domain
from copied binary64 bits, avoiding floating comparisons on signaling NaNs or
subnormals before an unsupported exception environment can be refused. This
also keeps empty structural validation arithmetic-free under altered modes.

The shared device-capable math header provides `DiscoveryCandidate` with
fields[8], status,flags,visits,pruned,refinements,evaluations, and a templated
`discover_local(evaluator, original_net, p, q, nodes, node_count, root,
bounds, mode, target, options)`; the numeric owner confirms the exact namespace
and signature to runtime before kernel wiring. Device/host interval arithmetic
uses explicit compile-target policies while the control-flow recipe is shared.
The CPU adapter references original patch control offsets correctly. Old
evaluator/projector helpers remain behaviorally unchanged.

## Implementation order and direct red checks

- Declare the ABI/header and Python POD handoff. Oracle writes exact restriction,
  all-node cache and witness cases against the missing capability; runtime writes
  alias/environment/retention/failure cases against its existing direct seam.
  Record the genuine missing-capability/native failure; no artificial failing
  test whose only purpose is a red-test receipt.
- Implement interval primitives and CPU hierarchy. Qualify actual exported boxes,
  integer paths and moment/support coefficients with exact binary-input models.
  Include all degrees, negative C0, asymmetric weights and subnormal midpoint
  loss. Add DOP and Bernstein construction as owned modes before GPU qualification.
- Implement bounded query/witness arithmetic and CPU range. Direct root-only,
  depth-zero, refinement-zero, ties, rank refusal, competing minima, caps and
  four gap/coverage combinations exercise its actual outputs/counters.
- Wire core/Python and complete CUDA token/array/kernel paths. Upload the exact
  CPU-generated cache; export actual device storage. Query kernel begins with
  128 threads/block, grid-stride rows; change this only after resource/timing
  measurements. Fixed stack13 and streamed Bernstein coefficients are the
  starting scratch strategy; no query-time subdivision or persistent queue.
- Run exact and public-contract qualification on CPU and both P100s. The oracle
  checks returned UV upper without allowance, analytic global truth, active
  coefficient intervals and fallback lower. Approximate XYZ has separately
  stated empirical evaluator error allowances; it never validates certificates.
- Cross-review the complete numerical/runtime/oracle code. Fix each valid
  finding, document rejected findings, and repeat the affected direct checks.

## Fleet execution and failure classes

Native execution, sanitizers and mutations use the qualified Linux x86-64
target. Preserve previous ABI1/2 actual cores/plugins/adapters before builds;
those baselines already exist in private context. Deploy only owned source to
the isolated project checkout, preserving unrelated archives/context/toolchains.
Local editing/read-only source study does not establish Mac native support.

Release configuration builds CUDA sm_60 and test hooks in isolated build dirs;
ASan/UBSan is a separate CPU-only build. Inspect ptxas register/stack/spill output
for each mode's query kernel. Run Compute Sanitizer for actual CUDA operations,
including hierarchy/cache exports and close, on both P100s. It attacks memory
and resource faults rather than geometric enclosure.

Extend the existing modeled backend seam for discovery capability refusal/base/
size/transient/fatal normalization, unpublished hierarchy/query/result cleanup,
partial downloads, cross-family poison, and admission-close. Direct allocated
sentinels attack creator/info/download/source alias and no-write behavior. CPU
tests alter real rounding, FTZ/DAZ and masks then restore them; unsupported
environment must fail before publication. No fault injector changes unrelated
production behavior. Both native resource and Python BaseException guards must
preserve the original error while draining the publication.

Fresh-process compatibility probes use actual old ABI1/2 binaries/plugins,
new adapters and preserved old adapters with new core/plugin. Old operations
remain useful; missing v3 refuses only new work. Run existing product suites
once after source changes because runtime/CUDA/Python boundaries are shared.
Broaden/repeat only for new source changes or unresolved faults.

Mutation checks target the admitted fault classes on the fleet: no outward
expansion, incorrect sign/norm, negative-C0 clipping, padding inclusion, rounded
XYZ upper, incomplete lower after cap and >= pruning. Use direct production
numeric helper/public outputs; do not create an oracle for the oracle or a
perpetual certification matrix. A real constant-point CUDA row with exact
zero upper/root lower exposes equality pruning through missing retained-leaf
failure; direct native comparison can cover the predicate where CPU interval
widening prevents exact equality.

## Measured delivered value and graphics

Root implements a stdlib/native benchmark over plane, diagonal plane, supplied
rational cylinder unique/axis targets, parabola normal/twin minima, and capped
queries. Native corpus global distances use dyadic analytic constructions where
the exact known minimum is representable, so every timed row is checked against
that truth without an epsilon on L<=d*²<=U. Exact Fraction corpus/caller checks
each witness and cache separately. No timing of invalid-row rejection is
reported as discovery throughput.

Use CPU1/24 and each P100, at least seven samples after excluded warm-up for the
published comparisons. First compare all three modes at common depth8 and
N=1,256,4096,65536; compare depths0/2/4/8 on diagonal and cylinder-axis precision
cases. Include a large repeated batch when the initial measurements justify
its cost. Record raw timings/counters, not only speedup medians. Native timing
separates build/upload, query snapshot/upload, resident output allocation+
traversal+refinement+sync, compact download, complete host consumption and cleanup.
Profile CUDA kernel scopes separately. Python caller adds copying/provenance/
wrapper/export costs. Preprocessing amortization and cache memory participate
in choosing default versus opt-in modes; report cases without a break-even.

The seed-free caller exports target/witness UV/XYZ, exact bracket fields,
source identity and flags/counters for actual rows. Owned SVG figures draw
cached enclosures and returned witnesses, label unresolved results, and explain
the three bounds. Without a traversal trace do not label drawn boxes as actual
visited/pruned nodes. Benchmark graphics read retained samples and show timing
scope, counts and achieved precision. README links diagrams, raw results and
reproducible commands; no decorative figure substitutes for geometry evidence.

## Checkpoints and completion

Commit/push reviewed design, reviewed plan/direct cases, numerical/cache work,
runtime/CUDA/Python work, qualification fixes, measured comparisons and public
guide/graphics as coherent substantive milestones. Root integrates ownership
lanes, opens a reviewable PR, merges preserving commits and retains the branch.
Issue #8 closes only after the operation, direct qualification, measurements and
usable caller/docs are delivered. No social posting or upstream messaging is
implied. The journal records actual actions/findings and public artifacts omit
private operational identifiers and paths.
