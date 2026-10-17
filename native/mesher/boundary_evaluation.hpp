#ifndef BREPGPU_MESHER_BOUNDARY_EVALUATION_HPP
#define BREPGPU_MESHER_BOUNDARY_EVALUATION_HPP
#include "boundary.hpp"
#include "geometry_analytic.hpp"
#include "scheduler.hpp"

namespace bgmesh {
// A bounded child of the shared-boundary owner. It retains immutable original
// geometry and exact parameters, imports into the worker's own Budget, and
// publishes all of its results only after successful completion.
class BoundaryPointJob final : public NativeTask {
  std::shared_ptr<RunResource> resource_;
  GeometryStore geometry_;
  BoundaryEvaluationBatch batch_;
  Range<BoundaryEvaluationTask> range_;
  GeometryPrecision precision_;
  std::pmr::vector<BoundaryEvaluationResult> results_;
  bool finished_ = false, started_ = false;

public:
  BoundaryPointJob(const std::shared_ptr<RunResource> &, const GeometryStore &,
                   const BoundaryEvaluationBatch &, Range<BoundaryEvaluationTask>, NativeTaskKey,
                   GeometryPrecision = {});
  StepState step(JobContext &) override;
  [[nodiscard]] std::span<const BoundaryEvaluationResult> results() const;
};
} // namespace bgmesh
#endif
