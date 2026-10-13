# Iteration 1 plan review: numerical evaluator and CUDA backend

2026-10-13. Reviewed [implementation plan](../docs/ITERATION_1_PLAN.md) against the [reviewed design](../docs/ITERATION_1_DESIGN.md), numerical source study and resolved numerical design review. Scope is the complete CPU/CUDA numerical unit and the private plugin resource/error boundary. No product code has been written in this review lane.

**Assessment:** the sequence, file ownership, direct oracles and measurement regions are coherent. Two shared-interface details require explicit wording before canonical headers are published. The runtime owner has agreed to the contracts below; their integration resolves the findings without expanding scope. No numerical algorithm redesign is needed.

## P-N1 — medium: multicore partitioning must retain the full SoA stride

The plan permits explicit CPU worker counts and per-call start/join costs, but the numerical helper description initially lists query arrays and output storage without a request-range/stride contract. With component-major output, calling a chunk evaluator using a chunk count as the stride and merely offsetting the output pointer produces the wrong layout and can make workers overwrite one another.

Required interface: `evaluate_cpu(..., query_count, begin, end, order, jets, statuses)` retains the **full** request count as output stride; a worker writes only request indices `[begin,end)`. The core partitions disjoint ranges and owns worker creation/join. The helper is stateless and uses bounded per-call/per-request scratch. Empty ranges write nothing.

Direct oracle: the same asymmetric shuffled mixed-degree batch evaluated with one worker and several explicit worker counts must return the same ordered components/statuses within the independent numeric grades. Include a batch count not divisible by worker count and compare expected full-stride vector positions, rather than only reducing all outputs to one checksum.

Runtime owner disposition: agreed to this canonical full-stride range interface. If worker creation throws after earlier workers start, the runtime joins those workers before releasing unpublished output. This preserves native buffer lifetime and avoids destruction of joinable thread objects during exception unwinding. No extra numerical thread pool or evaluator cache is required.

## P-N2 — medium: cleanup callbacks must report their error/restoration outcomes

The plan's private backend table exports close operations and promises classified errors, cleanup preservation and device restoration before publication. A `void` cleanup callback or an unreportable destructor cannot express cleanup/restoration failure to the core, especially when an earlier operation already has a primary diagnostic.

Required interface: backend destroy operations return a fixed-width status and use the canonical caller-owned diagnostic. Private context lifetime remains independent of whether its public handle is closed; child allocations can be cleaned while retained native ownership keeps that context alive. The plugin saves/selects/restores the calling thread's device for all operations, including cleanup. It never exports public IDs or resets the CUDA device. The core owns process-wide poison and preserves the original operation failure while recording cleanup outcomes separately.

Direct failure oracle: a test-build failure at an actual cleanup/restoration boundary cannot publish a successful creation/evaluation token, mask a preceding failure, or discard the mandatory status. Fatal-state cases run in fresh processes. Ordinary successful cleanup is also exercised after public parent closure while dependent results still retain storage.

Runtime owner disposition: agreed that destroy callbacks return status plus diagnostic and that the private context/plugin stay alive through child cleanup. The exact enum/diagnostic/token signatures belong in the canonical runtime-owned headers; the numerical owner will consume them unchanged.

## Numerical choices and direct checks accepted

The plan carries the reviewed local cube, weight interval `[1,256]`, all degree pairs `0..3`, closed normalized parameter domain and three cumulative output orders. CPU Bernstein basis/derivative sums and CUDA tensor de Casteljau use different evaluation structures. Both apply explicit quotient recovery, direct division and binary64 arithmetic; implicit contraction/unsafe math are disabled while CUDA's deliberate two-FMA interpolation remains enabled.

The scalar-channel CUDA schedule and grid-stride query loop are reasonable first choices. A bad patch slot is checked before patch/control memory access. Degree-zero missing derivatives become exact homogeneous zeros. Every failed query fills every requested Cartesian field with NaN and emits its canonical status, even if earlier components were finite. Successful arithmetic still does not certify topology, normal regularity or a conservative enclosure.

Exact represented-input rational truth and analytic polynomial/cylinder/bilinear/singular fixtures directly attack the actual claimed outputs. The sixteen degree pairs, three orders, endpoint neighbors, weight extremes, cancellation and shuffled/tail cases qualify CPU and each P100 independently. Cumulative `E0/E1/E2` semantics remain explicit. Metamorphic inputs must stay inside the supported envelope or expect refusal. The bounded transform and rejected `2^600` witness have independent expected outcomes.

The canonical header-first order prevents parallel owners inventing incompatible structures. CPU-only loading remains independent of CUDA. Profiling is optional and off in normal public timings; event components are separately labeled and use an equivalent CPU prepared-output comparison. Distinct public result allocation and required runtime work stay in resident-call latency. The actual batch sampler and fair host-to-host measurement establish the first useful delivery without claiming Boolean acceleration.

## Disposition

With P-N1/P-N2 recorded in the canonical interface decisions, this plan is ready for numerical implementation. The numerical owner will implement only `native/evaluator.hpp`, `native/evaluator_cpu.cpp` and `native/cuda_backend.cu`, plus directly necessary native checks if integration requires them. No runtime/Python/oracle/benchmark files are owned by this lane. Product code awaits the integrator's explicit acceptance of the complete plan.
