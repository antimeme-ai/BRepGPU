#include "boundary_evaluation.hpp"

namespace bgmesh {
namespace {
std::pmr::memory_resource *required_resource(const std::shared_ptr<RunResource> &resource) {
  if (!resource)
    throw Error(ErrorCode::internal_error, "null original boundary evaluation resource");
  return resource.get();
}
} // namespace
BoundaryPointJob::BoundaryPointJob(const std::shared_ptr<RunResource> &resource,
                                   const GeometryStore &geometry,
                                   const BoundaryEvaluationBatch &batch,
                                   Range<BoundaryEvaluationTask> range, NativeTaskKey identity,
                                   GeometryPrecision precision)
    : NativeTask(identity), resource_(resource), geometry_(geometry), batch_(batch), range_(range),
      precision_(precision), results_(required_resource(resource)) {
  if (!resource_ || range.first > batch_.tasks().size() ||
      range.count > batch_.tasks().size() - range.first ||
      identity.generation != batch_.epoch().value)
    throw Error(ErrorCode::internal_error, "invalid original boundary evaluation range");
  if (&batch_.source().view() != &geometry_.view().source.view())
    throw Error(ErrorCode::internal_error, "boundary parameters belong to a foreign source");
  results_.reserve(static_cast<std::size_t>(range.count));
}
StepState BoundaryPointJob::step(JobContext &context) {
  if (started_)
    throw Error(ErrorCode::internal_error, "original boundary child executed more than once");
  started_ = true;
  GeometryContext geometry_context(context.budget, precision_);
  for (uint64_t i = 0; i < range_.count; ++i) {
    context.budget.charge(1);
    const auto &task = batch_.tasks()[static_cast<std::size_t>(range_.first + i)];
    if (task.epoch != batch_.epoch())
      throw Error(ErrorCode::internal_error, "stale original boundary evaluation task");
    const auto parameter = import_exact(context.budget, batch_.parameters().view(task.parameter));
    const auto point = canonical_original_curve(geometry_context, geometry_, task.curve, parameter);
    if (point.dimension != 3)
      throw Error(ErrorCode::invalid_source,
                  "original spatial edge curve is not three-dimensional");
    results_.push_back({task, point.dimension, point.xyz});
  }
  finished_ = true;
  return StepState::done;
}
std::span<const BoundaryEvaluationResult> BoundaryPointJob::results() const {
  if (!finished_)
    throw Error(ErrorCode::internal_error, "original boundary results are incomplete");
  return results_;
}
} // namespace bgmesh
