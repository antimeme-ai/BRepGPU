#ifndef BREPGPU_MESHER_NUMERICAL_SERVICE_HPP
#define BREPGPU_MESHER_NUMERICAL_SERVICE_HPP
#include "face_engine.hpp"
#include "scheduler.hpp"

namespace bgmesh {
using NumericalInput = std::variant<BoundBatch, FaceCellWork>;
struct NumericalRequest final {
  NativeTaskKey key;
  NumericalInput input;
};
struct CellBatchResult final {
  std::shared_ptr<RunResource> resource;
  std::pmr::vector<CellBoundResult> cells;
  explicit CellBatchResult(std::shared_ptr<RunResource> owner)
      : resource(std::move(owner)), cells(resource.get()) {}
};
using NumericalOutput = std::variant<BoundCompleted, std::shared_ptr<const CellBatchResult>>;
struct NumericalResult final {
  NativeTaskKey key;
  NumericalOutput output;
};
struct NumericalServiceLimits final {
  BoundRuntimeLimits runtime;
  ActualCellLimits cells;
  uint32_t cell_capacity;
  GeometryBoundPrecision analytic;
};
struct NumericalServiceStats final {
  BoundRuntimeStats runtime;
  uint64_t requests = 0, prepared_cells = 0, analytic_cells = 0;
  uint64_t plane_cells = 0;
  uint64_t batches = 0, mixed_cell_batches = 0;
};
// One serialized service owns one persistent numerical runtime/session. Only
// the coordinator calls begin/release/open/close, between completed steps.
// READY is a continuation boundary, including nonblocking device polls.
class NumericalServiceJob final : public NativeTask {
  struct Impl;
  std::shared_ptr<RunResource> resource_;
  Impl *impl_;

public:
  NumericalServiceJob(const std::shared_ptr<RunResource> &, NativeTaskKey service, BoundRootBank,
                      NumericalServiceLimits, std::stop_token);
  ~NumericalServiceJob() override;
  void open_device(BoundCudaModule, BoundDeviceLimits);
  void begin(NumericalRequest);
  StepState step(JobContext &) override;
  [[nodiscard]] const NumericalResult &result() const;
  void release_result();
  [[nodiscard]] NumericalServiceStats stats() const noexcept;
  // Terminal. Failed drain keeps all owned state for retry, as BoundRuntime
  // does; final failure retains its registered device owner until process exit.
  void close();
};
} // namespace bgmesh
#endif
