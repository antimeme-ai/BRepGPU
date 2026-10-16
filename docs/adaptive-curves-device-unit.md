# Typed resident curve operations on P100

Finite device unit for review before implementation. Root owns the versioned
CurveCudaAPI export and curve-mode operations in the existing CUDA module.
Geometry owns adaptive_curve_types.h and the shared RN numerical kernel; runtime
owns AdaptiveBoundarySession and exact physical/source identities.

Grounding: SmCurveCache original span/restriction reuse, Filip's vector chord
bound, original Bernstein residual identities and the focused curve geometry
study select the geometry. Schwarz/Stamminger 2009 shared boundary/dataflow and
Blelloch's work-efficient scan select canonical parallel allocation. The existing
adaptive device unit supplies actual tested buffer/reservation/ticket/drain
procedures. We reuse that owned lifetime machinery; no second allocator/failed-
drain implementation or imported GPU library is introduced.

## Typed API

Add cuda/adaptive_curve_api.h with curve_cuda_abi=1 and export
brepgpu_adaptive_curve_cuda_v1 from the same shared library. Existing surface API
and its version remain intact. A DeviceSession has an explicit surface/curve
mode. A call for the wrong mode refuses before reading typed descriptor memory.
Curve creation consumes DeviceConfig, original/prepared CurveRootDescriptor
banks and original interval controls; all borrowed input is copied/owned before
return. Original roots and requested restrictions remain distinct.

Use these finite packets, with exact spelling reconciled to the geometry header:

- CurveDeviceSample: destination, CurveSampleInput, optional supplied demanded-
  tier CurveSampleResult and a checked supplied flag. Ordinary samples execute
  the shared original-root evaluator. fixed_actual is an explicit source-vertex
  or chosen-point override; its departure from original Ce is recomputed, never
  borrowed from a different Q. Source/profile/tier identity is native-qualified.
- CurveDeviceRestriction: prepared root, resident parent range and distinct left/
  right destination ranges. Both children are produced together. Shape, read
  coverage, all write ranges and read/write overlap are checked before launch.
- CurveDeviceNode: node destination, CurveNodeInput, two resident sample slots,
  downward-rounded tolerance, explicit eligible image policy and optional
  demanded-tier cached CurveNodeBounds. Ordinary node work consumes the actual
  resident endpoint values/departures. A cached bound is admitted only by its
  native full source/path/point-revision/tier/profile key.
- CurveDeviceNodeAnswer: CurveNodeBounds, accept/split/numerically-unresolved
  action and canonical exclusive split prefix. Parameter/image bounds remain
  separate. Image acceptance requires both the geometry proof and the query's
  explicit compatible correspondence policy.

The function table provides create, samples, paired_children, nodes, poll, stats
and destroy. Reuse existing DeviceConfig, DeviceError, DeviceStats and the
DeviceSession token. Poll capacity is RESULT RECORD COUNT; restrictions have no
result payload. One operation may be pending per session. Inputs are copied to
owned pinned staging before asynchronous work. Pending/retired allocations and
reservation tokens are registered before acquisition/free, with retryable drain
and final poisoned-owner retention exactly as the owned surface path.

## Concrete kernels and checks

Degree32 spatial controls have132 interval scalars. Sample evaluation uses the
same explicit RN operation order as CPU and canonical original roots. Analytic
samples use the shared bounded trig; supplied precise values are separately
counted native fallback/import, not GPU evaluations.

For paired children one warp stages a parent, computes one midpoint pyramid in
two shared slabs and writes both endpoint sequences. Lane-strided work and warp
barriers preserve every old-level read before the next level overwrites it.
This is O(p²), unlike recomputing every coefficient's blossom or duplicating the
whole child pyramid. CPU uses the identical scalar DAG and interval padding.

Rational chord bounds use cooperative coefficient work and a canonical maximum;
p32 produces34 residual coefficients, so each lane loops by32 where needed.
Analytic bounds use original-frame cached derivative constants. No private
132-scalar net is copied into every residual thread. Ready-but-large bounds
request geometric splitting; genuine unresolved numeric status requests the
common native precision path. No parameter-size override forces acceptance.

Mark and scan in original task order, then compact actual split requests. Stable
prefix ranks feed the native exact-key child owner; device completion timing
cannot select source point identity or partial physical request horizons.
Persistent roots, controls, samples and node buffers survive queries. Count
actual command/root/result host-device traffic and owned device/pinned memory.
Do not treat D2D relocation as host traffic or CUDA context memory as owned pools.

Direct RED precedes product bodies. The direct geometry reader must consume real
P100 curve results (analytic, rational, degree32 and paired restrictions), not a
host substitute. Protocol tests attack malformed mode/shape/range, duplicate
destinations, parent overlap, stale/consumed tickets, output capacity and pending
destroy. Include enough marked nodes to cross scan tiles and check every prefix.
Scoped CUDA memcheck/leak and race checks cover the new mechanisms; reusing the
one complete-operation geometry reader avoids another certifier. Existing surface
caller checks run once after this shared-module change to catch compatibility
regressions. All builds, tests and performance run on the fleet.
