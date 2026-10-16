#include "face_preparation_job.hpp"
#include "geometry_candidate.hpp"
#include <memory>

namespace bgmesh {
namespace {
[[noreturn]] void invalid(const char *message) { throw Error(ErrorCode::invalid_source, message); }
NativeTaskKey identity(const FacePreparationInput &input) {
  if (!input.run || !input.boundary)
    invalid("face preparation requires complete run and boundary inputs");
  const auto &b = *input.boundary;
  const auto &source = input.run->geometry.view().source.view();
  if (&b.source.view() != &source || b.definition.value >= source.definitions.size() ||
      b.face.value >= source.faces.size() ||
      source.faces[b.face.value].definition_id != source.definitions[b.definition.value].id)
    invalid("face preparation source ownership mismatch");
  return {{OwnerKind::face, b.definition.value, source.faces[b.face.value].id.value},
          input.generation,
          0};
}
NativeTaskKey candidate_identity(const FaceCandidateWork &input) {
  if (!input)
    invalid("candidate task requires immutable geometry work");
  auto owner = input->target.face.owner;
  owner.kind = OwnerKind::geometry;
  return {owner, input->target.face.face_generation, input->ordinal};
}
template <std::size_t... I>
std::array<Rational, 6> read_box(bgfd::Budget &budget, const ExactTable &values,
                                 const std::array<ExactRef, 6> &refs, std::index_sequence<I...>) {
  return {import_exact(budget, values.view(refs[I]))...};
}
} // namespace
struct FacePreparationJob::Impl final {
  FacePreparationInput input;
  FacePreparationLimits limits;
  std::shared_ptr<ResourceReservation> lease;
  std::stop_token stop;
  bgfd::Budget budget;
  GeometryContext context;
  FaceBoundaryPreparation *preparation = nullptr;
  FacePreparationEvent event;
  std::optional<BoundCompleted> answer;
  std::optional<FaceOwnerInput> output;
  bool usable = true;
  uint64_t reported_work = 0;
  Impl(const std::shared_ptr<RunResource> &resource, FacePreparationInput source,
       FacePreparationLimits cap, std::stop_token token)
      : input(std::move(source)), limits(cap), lease(reserve_resource(resource, cap.bytes)),
        stop(std::move(token)), budget({cap.bytes, cap.work}, lease.get(), stop), context(budget) {
    if (!cap.bytes || !cap.work || !cap.batch.task_capacity || !cap.batch.table_capacity ||
        !stop.stop_possible())
      invalid("invalid face preparation limits or cancellation token");
  }
  ~Impl() {
    if (preparation) {
      std::destroy_at(preparation);
      std::pmr::polymorphic_allocator<FaceBoundaryPreparation>(&budget).deallocate(preparation, 1);
    }
  }
  void check() const {
    if (!usable)
      throw Error(ErrorCode::internal_error, "failed face preparation generation");
  }
  const DefinitionAllowance &allowance() const {
    const auto &definitions = input.run->assembly.view().definitions;
    const auto slot = input.boundary->definition;
    if (slot.value >= definitions.size() || definitions[slot.value].definition != slot)
      invalid("missing whole-run definition quality");
    return definitions[slot.value];
  }
  Rational positional() {
    return import_exact(budget, input.run->assembly.view().values.view(allowance().positional));
  }
  void initialize() {
    const auto &a = allowance();
    const FaceChartRequest request{
        .face = input.boundary->face,
        .generation = input.generation,
        .positional_allowance = positional(),
        .node_source_allowance =
            import_exact(budget, input.run->assembly.view().values.view(a.node_source)),
        .precision = limits.precision,
        .profile_limits = limits.profile,
        .allow_plane_profile = input.allow_plane_profile};
    std::pmr::polymorphic_allocator<FaceBoundaryPreparation> allocator(&budget);
    auto *value = allocator.allocate(1);
    try {
      std::construct_at(value, budget, context, input.run->geometry, input.boundary, request,
                        input.pole_control);
    } catch (...) {
      allocator.deallocate(value, 1);
      throw;
    }
    preparation = value;
  }
  void publish() {
    const auto seed = preparation->snapshot();
    const auto slot = seed->definition;
    const auto &definitions = input.run->definitions;
    if (slot.value >= definitions.size() || definitions[slot.value].definition != slot)
      invalid("missing whole-run definition envelope");
    const auto xyz = read_box(budget, input.run->envelopes, definitions[slot.value].xyz,
                              std::make_index_sequence<6>{});
    const auto metric =
        prepare_face_metric(context, seed, {input.run->geometry.view().source, slot, xyz},
                            positional(), limits.precision);
    output = FaceOwnerInput{seed, metric.values, metric.factor, metric.scales, input.seed};
  }
};
FacePreparationJob::FacePreparationJob(const std::shared_ptr<RunResource> &resource,
                                       FacePreparationInput input, FacePreparationLimits limits,
                                       std::stop_token stop)
    : NativeTask(identity(input)), resource_(resource), impl_(nullptr) {
  if (!resource_ || resource_ != input.run->resource || resource_ != input.boundary->resource)
    invalid("face preparation requires its run resource");
  std::pmr::polymorphic_allocator<Impl> allocator(resource_.get());
  impl_ = allocator.allocate(1);
  try {
    std::construct_at(impl_, resource_, std::move(input), limits, std::move(stop));
  } catch (...) {
    allocator.deallocate(impl_, 1);
    throw;
  }
}
FacePreparationJob::~FacePreparationJob() {
  std::destroy_at(impl_);
  std::pmr::polymorphic_allocator<Impl>(resource_.get()).deallocate(impl_, 1);
}
StepState FacePreparationJob::step(JobContext &job) {
  impl_->check();
  try {
    if (job.stop != impl_->stop)
      invalid("face preparation requires its scheduler stop token");
    impl_->budget.charge(1);
    if (impl_->event.kind == FacePreparationEventKind::unstarted)
      impl_->initialize();
    else if (impl_->event.kind == FacePreparationEventKind::curves && impl_->answer) {
      impl_->preparation->complete(*impl_->answer);
      impl_->answer.reset();
    } else
      invalid("face preparation cannot continue without its numerical child");
    switch (impl_->preparation->state()) {
    case FacePreparationState::curve_work: {
      auto work = impl_->preparation->take_curve_work(impl_->input.run->bank, impl_->limits.batch);
      if (!work)
        throw Error(ErrorCode::internal_error, "face curve state has no numerical work");
      impl_->event = {FacePreparationEventKind::curves, std::move(work), {}};
      return StepState::wait_child;
    }
    case FacePreparationState::boundary_request:
      impl_->event = {FacePreparationEventKind::boundary_horizon,
                      {},
                      impl_->preparation->boundary_request(),
                      impl_->preparation->next_pole_control()};
      if (!impl_->event.boundary)
        throw Error(ErrorCode::internal_error, "face preparation lost shared boundary request");
      return StepState::wait_barrier;
    case FacePreparationState::ready:
      impl_->publish();
      impl_->event = {FacePreparationEventKind::ready, {}, {}};
      return StepState::done;
    }
    throw Error(ErrorCode::internal_error, "unknown face preparation state");
  } catch (...) {
    impl_->usable = false;
    throw;
  }
}
const FacePreparationEvent &FacePreparationJob::event() const {
  impl_->check();
  return impl_->event;
}
void FacePreparationJob::finish_curves(BoundCompleted answer) {
  impl_->check();
  if (impl_->event.kind != FacePreparationEventKind::curves || impl_->answer)
    invalid("face preparation is not waiting for curve results");
  impl_->answer = std::move(answer);
}
const FaceOwnerInput &FacePreparationJob::result() const {
  impl_->check();
  if (!impl_->output)
    invalid("face preparation has not published a complete seed and metric");
  return *impl_->output;
}
FaceOwnerWork FacePreparationJob::take_owner_work() {
  const auto work = impl_->budget.work();
  const FaceOwnerWork result{work - impl_->reported_work, impl_->budget.peak()};
  impl_->reported_work = work;
  return result;
}
CandidatePointJob::CandidatePointJob(FaceCandidateWork work)
    : NativeTask(candidate_identity(work)), input_(std::move(work)) {}
StepState CandidatePointJob::step(JobContext &job) {
  if (started_)
    invalid("geometry candidate started twice");
  started_ = true;
  GeometryContext context(job.budget);
  output_ = evaluate_geometry_candidate(context, input_);
  return StepState::done;
}
const FaceCandidateResult &CandidatePointJob::result() const {
  if (!output_)
    invalid("geometry candidate is incomplete");
  return *output_;
}
} // namespace bgmesh
