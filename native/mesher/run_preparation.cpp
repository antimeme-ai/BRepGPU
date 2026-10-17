#include "run_preparation.hpp"
#include <cmath>

namespace bgmesh {
namespace {
using Clock = std::chrono::steady_clock;
double seconds(Clock::time_point start) {
  return std::chrono::duration<double>(Clock::now() - start).count();
}
constexpr OwnerKey run_owner{OwnerKind::run, UINT32_MAX, 0};
} // namespace
PreparedRunData::PreparedRunData(std::shared_ptr<RunResource> owner, GeometryStore original,
                                 AssemblyStore instances, ExactTable boxes, BoundRootBank roots,
                                 MeshQuality quality)
    : resource(std::move(owner)), geometry(std::move(original)), assembly(std::move(instances)),
      envelopes(std::move(boxes)), bank(std::move(roots)), requested(quality), definitions(resource.get()) {}
RunPreparationJob::RunPreparationJob(const std::shared_ptr<RunResource> &resource,
                                     RunPreparationInput input)
    : NativeTask({run_owner, 0, 0}), resource_(resource), input_(std::move(input)) {
  if (!resource_ || !std::isfinite(input_.positional) || !std::isfinite(input_.node_source) ||
      input_.positional <= 0 || input_.node_source <= 0)
    throw Error(ErrorCode::invalid_source, "invalid native run source quality/resource");
}
StepState RunPreparationJob::step(JobContext &job) {
  if (started_)
    throw Error(ErrorCode::internal_error, "native source preparation started twice");
  started_ = true;
  RunPreparationTimes times;
  job.budget.charge(1);
  auto start = Clock::now();
  const auto source = load_source(input_.source, resource_, input_.source_limits);
  times.source_seconds = seconds(start);
  start = Clock::now();
  const auto geometry =
      compile_geometry(source, resource_, {{OwnerKind::geometry, UINT32_MAX, 0}, 0, 0}, job.budget);
  times.geometry_seconds = seconds(start);
  GeometryContext context(job.budget);
  std::pmr::vector<DefinitionEnvelope> envelopes(&job.budget);
  envelopes.reserve(source.view().definitions.size());
  start = Clock::now();
  for (uint32_t i = 0; i < source.view().definitions.size(); ++i) {
    job.budget.charge(1);
    envelopes.push_back(
        prepare_definition_envelope(context, geometry, {.definition = {i}, .generation = 0}));
  }
  times.envelopes_seconds = seconds(start);
  std::pmr::vector<DefinitionEnvelopeView> views(&job.budget);
  views.reserve(envelopes.size());
  for (const auto &envelope : envelopes)
    views.push_back({source, envelope.definition, envelope.xyz});
  start = Clock::now();
  const auto assembly =
      prepare_assembly(job.budget, source, resource_, views,
                       {Rational::from_double(job.budget, input_.positional),
                        Rational::from_double(job.budget, input_.node_source)},
                       {run_owner, 0, 2}, input_.assembly_precision, input_.assembly_limits);
  times.assembly_seconds = seconds(start);
  ExactBuilder boxes(resource_, {run_owner, 0, 1});
  std::pmr::vector<PreparedDefinitionEnvelope> frozen(&job.budget);
  frozen.reserve(envelopes.size());
  for (const auto &envelope : envelopes) {
    PreparedDefinitionEnvelope item{envelope.definition, {}};
    for (std::size_t i = 0; i < item.xyz.size(); ++i)
      item.xyz[i] = boxes.append(envelope.xyz[i]);
    frozen.push_back(item);
  }
  std::pmr::vector<BoundRootSelection> roots(&job.budget);
  const auto &g = geometry.view();
  if (g.surface_roots.size() > UINT32_MAX || g.curve_roots.size() > UINT32_MAX ||
      g.curve_roots.size() > roots.max_size() ||
      g.surface_roots.size() > roots.max_size() - g.curve_roots.size())
    throw Error(ErrorCode::resource_exhausted, "source root bank count overflow");
  roots.reserve(g.surface_roots.size() + g.curve_roots.size());
  for (uint32_t i = 0; i < g.surface_roots.size(); ++i)
    roots.emplace_back(SurfaceRootSlot{i});
  for (uint32_t i = 0; i < g.curve_roots.size(); ++i)
    if (g.curves[g.curve_roots[i].source.value].dimension == 3)
      roots.emplace_back(CurveRootSlot{i});
  start = Clock::now();
  const auto bank = prepare_bound_roots(job.budget, resource_, geometry, roots, {run_owner, 0, 3});
  times.bank_seconds = seconds(start);
  auto result = std::allocate_shared<PreparedRunData>(
      RunAllocator<PreparedRunData>(resource_), resource_, geometry, assembly, boxes.seal(), bank,
      MeshQuality{input_.positional, input_.node_source, 1});
  result->definitions.assign(frozen.begin(), frozen.end());
  result->times = times;
  output_ = std::move(result);
  return StepState::done;
}
PreparedRun RunPreparationJob::result() const {
  if (!output_)
    throw Error(ErrorCode::internal_error, "native source preparation is incomplete");
  return output_;
}
} // namespace bgmesh
