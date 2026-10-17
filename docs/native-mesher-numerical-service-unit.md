# Persistent numerical service

Proposed integration seam for parent review before code. It consumes the
existing FaceCellWork and BoundBatch values under the accepted numerical
contract; the point filter is independent of this integration.

## Public native API

Add `native/mesher/numerical_service.hpp/.cpp`:

```cpp
using NumericalInput = std::variant<BoundBatch, FaceCellWork>;
struct NumericalRequest {
  NativeTaskKey key; // coordinator's child identity, separate from service key
  NumericalInput input;
};
struct CellBatchResult {
  std::shared_ptr<RunResource> resource;
  std::pmr::vector<CellBoundResult> cells; // original packet order
};
using NumericalOutput = std::variant<BoundCompleted,
                                    std::shared_ptr<const CellBatchResult>>;
struct NumericalResult {
  NativeTaskKey key;
  NumericalOutput output;
};
struct NumericalServiceLimits {
  BoundRuntimeLimits runtime;
  ActualCellLimits cells;
  uint32_t cell_capacity;
  GeometryBoundPrecision analytic;
};
class NumericalServiceJob final : public NativeTask {
public:
  NumericalServiceJob(const std::shared_ptr<RunResource>&, NativeTaskKey service,
                      BoundRootBank, NumericalServiceLimits, std::stop_token);
  void open_device(BoundCudaModule, BoundDeviceLimits); // once, before requests
  void begin(NumericalRequest); // idle only, coordinator between steps
  StepState step(JobContext&) override;
  const NumericalResult& result() const; // complete only
  void release_result(); // returns to idle; retained caller handles survive
  NumericalServiceStats stats() const noexcept;
  void close(); // drain/release persistent device owner; terminal
};
```

The coordinator owns service assignment. A service has one request at a time,
one retained immutable bank and one BoundRuntime; no executor or nested future.
CPU worker services use `filter_cpu`; a device service opens one session and
reuses it across both boundary curve and face cell requests. The caller loads
the module once and can share its immutable handle. Service NativeTask identity
is stable; NumericalRequest identity is echoed so reused services cannot route
an old result to a new child.

## Step machine and ownership

`begin` validates configured capacities, nonempty input, same run resource and
same retained GeometryStore as the bank before any source-slot use. Existing
BoundBatch ownership checks remain in BoundRuntime. All full source/face/cell/
boundary/request metadata remain unchanged through the result.

For FaceCellWork, import one bounded cell at a time into the current
JobContext Budget. Same-positive-UV-role actual XYZ is preserved. Analytic
carriers call the existing explicit-precision bound_analytic_cell. Rational
carriers call prepare_actual_cell, freezing its clipped UV and actual affine L
in run-owned ExactTables. No Rational or pointer to JobContext/face-owner Budget
survives a step. PreparedCellBounds and CellBoundFold retain only immutable
tables/run-owned state. CellMapKind plane/pole remain explicit unsupported until
their specialized consumer exists; they never enter the ordinary UV path.

Once preparation has bounded results/pieces, pack consecutive rational pieces
across cells into batches capped by both runtime task and table capacities.
Sort preparation records by parent task key, retaining original packet indices:
the frontier priority order need not satisfy the runtime's sorted-key contract.
Keep each cell's piece range contiguous inside a batch; a cell may span batches.
This supplies useful GPU batches without per-cell submit/session setup. Retain
only one outstanding device ticket. A CPU step filters/resolves one batch; a
GPU submit step returns READY, later poll steps return READY while incomplete,
then resolve only unresolved pieces through the existing private-child exact
fallback. The coordinator resubmits READY services; no worker waits for CUDA or
child work. All pieces are folded before publishing any CellBatchResult. Results
remain in the original face packet order, independent of completion timing.

Add the narrow fold overload `CellBoundFold::add(const BoundCompleted&,
CellPieceRange response_range)` to validate/fold a contiguous portion of a
mixed-cell response. Existing `add(completed)` delegates over the entire range.
This reuses the current complete-header, duplicate and decision validation;
there is no second fold mechanism. Validate every selected response before
mutating its fold state. Direct BoundBatch requests return their existing full
BoundCompleted unchanged.

Current JobContext scratch uses the scheduler's prepaid worker capacity. The
existing BoundRuntime child reservation is separate and includes its own exact
fallback scratch/work; it is not borrowed from the face or scheduler Budget.
The coordinator configures both capacities before admission, accounting for
their possible overlap. Bounded prepared/output vectors and exact tables are
charged to RunResource. There is no memory wait, unbounded queue or global cache.
StepCompletion reports preparation/analytic scratch work; BoundRuntimeStats
reports runtime model/exact work separately, so the coordinator does not count
either twice.

NumericalServiceStats contains the existing runtime counters plus requests,
prepared/analytic cells, batches and mixed-cell batches. These are observational
work counters, not additional geometry acceptance records. The required stop
token must equal the current scheduler JobContext token. Preparation tables use
the distinct service run owner and a monotone table ordinal across all requests;
they never consume a face owner's table-identity sequence.

Cancellation fails the current request, cancels/drains the retained runtime and
publishes no partial result. Numeric resource failure propagates rather than
becoming bound_fail. Explicit close follows the reviewed BoundRuntime retry and
exceptional poisoned-owner retention policy; healthy close releases all device
and pinned reservations. Failed service requests cannot be silently restarted.

## Focused implementation qualification

First author a missing-header/API RED using actual GeometryStore roots and
existing source fixtures. Reuse one service across a real CurveBoundInput batch,
a multi-cell packet crossing C0 roots and an analytic packet. Compare full
headers and independently expected decisions on CPU and both P100s. Ensure the
mixed-cell batch path executes, all pieces are required, stale identity is
rejected, and a second request reuses rather than recreates the device session.
Check caller input/table destruction after begin, immutable output lifetime,
busy/release/cancel/resource behavior and healthy close returning RunResource
live bytes to the pre-service baseline. Strict Release, host ASan/UBSan/leaks,
scoped actual device memcheck and source format/tidy suffice. No broad rerun,
driver fault campaign or whole-watch claim belongs to this unit.
