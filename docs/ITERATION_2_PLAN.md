# Implement resident seeded projection

Implementation plan, 2026-10-13, following the reviewed
[design](ITERATION_2_DESIGN.md). The purpose is a useful bounded numerical
consumer: refine one provided seed on one supported local carrier without
shipping intermediate jets to Python. Return a first-order candidate with
honest status/rank/curvature diagnostics and measure complete native calls.

## Whole ownership units

| Owner | Concept | Files |
| --- | --- | --- |
| Runtime lane | One registry, additive ABI and resource lifetime boundary | `include/brepgpu_projection.h`, `native/projection_backend_api.h`, `native/runtime.cpp`, `python/brepgpu/__init__.py`, `CMakeLists.txt`, native callback-model faults/compatibility tooling |
| Numerical lane | Complete bounded CPU/GPU solver and private CUDA resources | `native/projection_solver.hpp`, `native/projection_cpu.cpp`, `native/cuda_backend.cu`, direct native math cases if needed |
| Independent oracle lane | Spec/analytic fixtures and public caller conformance | `python/brepgpu/projection_fixtures.py`, `tests/test_projection.py`, `scripts/qualify_projection.py` |
| Integrator | Native benchmark, actual Python cloud caller, target runs and publication | `benchmarks/projection/`, `examples/project_cloud.py`, final docs/results/journal/issue updates |

The runtime owner publishes canonical public/private declarations before any
dependent numerical/caller implementation. Header handoff includes exact names,
size/version values, result field order, flags/statuses, options limits and
internal CPU range signature. Avoid parallel duplicate POD definitions or a
second registry. Coordinate interface changes through that owner.

## Sequence

1. Define the canonical additive ABI. bg1 remains byte-compatible. Add ABI2
   options/info checking separately from existing ABI1 helpers and diagnostics;
   pure v2 factory negotiation preserves the same v1 table/token domain.
2. Write spec-derived fixture expectations and public red cases before the
   operation exists. The initial missing projection API is a real caller red;
   native solver red directly exercises required analytic geometry/curvature.
   Do not count a successful import or CPU/GPU parity as geometry qualification.
3. Implement the complete numerical unit using the exact bounded algorithm in
   the design. Reuse the existing CPU Bernstein helper with stack-sized single
   queries or expose an owned scalar helper without changing bg1 arithmetic.
   Reuse CUDA homogeneous tensor jets inside each thread. Keep decision math
   shared while these evaluator strategies remain independent.
4. Implement new handle kinds/snapshots, project and result APIs, lazy plugin
   extension and lazy Python declarations. Preserve old-core evaluator use and
   old-plugin evaluator use. CPU-only builds never require CUDA or v2 loading.
5. Run direct numerical/caller green on the supported Linux host, both P100s,
   old evaluator qualification and relevant native/device memory checks. Cross
   review the numerical implementation, native/Python boundary and callers.
   Integrate all valid findings and repeat affected checks.
6. Build the native benchmark and off-surface point-cloud caller after header
   handoff. Verify all timed rows against independent geometric expectations
   outside their timers. Measure all declared outcomes and native/Python costs.
7. Publish code, exact source identity, raw results, source studies and actual
   caller outputs. Push meaningful branch commits during this sequence, create
   a PR and preserve commits on merge. Close issue #6 only when required work
   and the reported operation are complete.

## Numerical implementation constraints

All iteration predicates, pivot scaling, step caps, trial arithmetic, status
precedence and classification are as specified in the design. Add a shared
host/device solver with a tiny evaluator callback returning the local full jet
or position. It allocates no dynamic memory and touches no global registry.
The CPU range entry takes full batch N separately from [begin,end); output SoA
stride is never the worker chunk length. Partial thread starts join before an
unpublished result is discarded, as in the existing evaluator runtime.

The CUDA owner also owns its private projection query/result allocations,
extension callbacks, event handling and cleanup. New query tokens retain the
v1-created private patch/context; the core ensures their dependency lifetime.
Save/select/restore the calling thread's device, synchronize owned stream work
and publish tokens only after successful restoration. Four integer slabs plus
seven FP64 planes are independent distinct result allocations or checked slices
of owned allocations; no result aliases a later call's scratch.

Use no fast math or implicit contraction. Solver dots/trials use the reviewed
operation order; the evaluator keeps its explicit FP64 interpolation FMA.
Inspect sm_60 register/stack/spill reports, but do not introduce scheduling or
memory-layout machinery merely to chase an occupancy number.

## Runtime/failure and compatibility checks

Extend the existing relevant native callback model instead of duplicating a
fault framework. Direct fresh-process cases must exercise:

- Projection query/result cleanup after public parent closure; context close
  with promoted projection admission blocked before its lock.
- Wrong-kind projection/evaluator calls, exact byte/count/alignment rejection,
  pairwise output-overlap preflight and unchanged destination sentinels.
- Pure extension refusal/malformed base preserving evaluator use; factory
  runtime/fatal diagnostics propagated and transient failure not cached.
- Projection publication OOM plus fatal cleanup: primary error, zero handle,
  token drain and cross-family CUDA poison. Partial projection downloads publish
  no host batch; completed prior host arrays/CPU remain usable.
- Actual caller-thread device restoration through all new callbacks on both
  available P100s. No modeled error is described as induced hardware failure.

The integrator preserved the actual iteration-one libbrepgpu/core and CUDA
plugin before any product modification, privately outside build output. Test
new Python plus old core on evaluator operations, and new core plus old plugin
on evaluator operations with explicit unsupported projection. Also test old
evaluator ABI calls against the new core/plugin and retain ordinary iteration-one
exact qualification. Preserve compiled baseline artifacts and all unrelated
host data; deployment touches only this project's owned source/build directory.

## Direct oracle and rounding allowances

Use the oracle study's exact plane, cylinder, parabolic, weak-corner, conditioned
and degenerate fixtures. Targets and seeds are independently constructed; no
solver-generated inputs or expected iteration path are the geometric truth.
An explicit exhausted-search case uses S=((2s-1)^3,t,0), target
(2^-10,.5,0), seed (33/64,.5), max_backtracks=0: its indefinite Newton
Hessian invokes GN, whose full trial increases distance. Expect LINESEARCH_LIMIT,
the unchanged seed, zero accepted updates and two evaluations. For nonlinear
throughput, parabolic targets X=(a-2*a*lambda,t,a*a+lambda), a in
{-.5,-.25,.25,.5}, lambda=1/16, have independently known unique solutions
s=(a+1)/2 and distance squared lambda²*(4*a²+1). The scalar Hessian is
positive throughout the rectangle because X_z<=5/16.
The actual provided binary64 controls, target and returned UV define exact
Fraction expectations. Derive point/distance/gradient/Hessian directly at that
UV and check flags/status against the specified finite fixture margins.

Reuse empirical evaluator grades E0=2^-40 and E1=2^-30 as finite-corpus budgets,
not universal enclosures. For exact r=S-X and jet components D_i, a distance
allowance is sum_j(2*|r_j|*E0+E0²) plus 64 ulps at max(1,distance_squared).
A gradient component allowance is sum_j(|r_j|*E1+|D_ij|*E0+E0*E1) plus
64 ulps at max(1,sum_j|r_j*D_ij|). Box projection is nonexpansive; use the maximum
gradient allowance plus 64 ulps at 1 for rho. These are corpus acceptance
allowances conditioned on its evaluator grades, not runtime error guarantees.

Every finite reported point must meet E0 at the returned UV; distance/rho must
agree within the respective allowance. A stationary row's reported rho must
be <= requested tolerance; exact rho must be <= tolerance plus that allowance.
Known well-conditioned affine/cylinder/parabolic fixture basins also check the
expected answer, with UV tolerance 8*requested_tolerance/min(known reduced
distance-Hessian eigenvalue,1) plus a stated oracle rounding allowance, and
position/distance allowances propagated from the known analytic map. Do not
apply that bound to conditioned/flat/degenerate fixtures or arbitrary carriers.

Check final exact squared distance against exact seed squared distance with
allowances from both endpoint evaluations and their arithmetic budget. This is
finite-corpus nonincrease qualification, not a certified exact monotone trace.
Curvature expectation is independent exact matrix geometry, not another copy of
the solver's branch logic; include exact threshold equality, weak positive
multiplier, forbidden full-H eigenvector, regular saddle and quartic ambiguity.
The seeded conditioned-chart witness explicitly expects stationary at the
non-nearest seed, demonstrating the contract's UV-accuracy limit.

Counters obey direct necessary bounds, actual zero-update/initial-solution
cases and result provenance. Do not invent a second iterative solver to predict
every accepted-update count. Invalid rows have NaN fields and zero counters;
finite unresolved rows have CANDIDATE_VALID and accurate final diagnostics.

## Benchmark and actual caller

The native benchmark compares CPU1/CPU24/CUDA0/CUDA1 through public bg2 calls,
same bits/options/required result fields, one warmup and seven measured samples.
Use plane, tilted-boundary, nonlinear radial-cylinder and mixed seeded cases.
Default counts 1,32,256,4096,65536; include 1,048,576 where a complete qualified
run is practical. Record stationary/unresolved/rank/curvature counts and update/
evaluation distributions. A deliberate adverse corpus is reported separately;
do not filter slow/unresolved rows from the timing population.

Generation stays outside prepared native host timers. Report snapshot/upload,
resident project with fresh result allocation and publication, download into
prepared host arrays, prepared host-to-host with required status consumption,
cleanup and first context use. Optional events use a separate profiling context
and bracket the actual submission; no subtraction from another run's wall time.
No other owned build/test job overlaps the published timing matrix.

The Python caller bulk-projects off-surface cylinder and polynomial targets,
exports target/seed/candidate/flag records and a projected-segment view. It must
check finite fields before maxima and use analytic/spec-derived position and
stationarity expectations. Time preparation, query packing/upload, project,
download/provenance, qualification and file/visual export separately with exact
names. Native versus Python scopes remain distinct. Imported CAD export,
global search, topology and automatic CPU fallback are absent from this caller.

Store private operational logs under ignored context; publish sanitized
hardware/toolchain/settings, raw samples/qualification witnesses, matching
source revision/hashes and actual caller outputs. No new geometry/binding/test
library is adopted. The resulting operation remains a seeded local-candidate
stage toward complete B-rep operations.

## Review disposition

[Runtime review](../papers/2026-10-13-projection-plan-runtime-review.md),
[numerical review](../papers/2026-10-13-projection-plan-numerical-review.md) and
[independent oracle review](../papers/2026-10-13-projection-plan-oracle-review.md)
reported no blocker. The oracle review added the explicit exhausted-search
witness and independently checked the parabolic normal-target throughput family;
both are incorporated above. The plan is accepted for implementation. Code,
supported-host checks, code reviews and measured caller output remain required.
