#include "face_job.hpp"
#include <algorithm>
#include <memory>

namespace bgmesh {
namespace {
[[noreturn]] void invalid(const char *message) { throw Error(ErrorCode::invalid_source, message); }
NativeTaskKey identity(const FaceOwnerInput &input) {
  if (!input.boundary)
    invalid("face task has no boundary snapshot");
  const auto &b = *input.boundary;
  const auto &s = b.geometry.view().source.view();
  if (b.face.value >= s.faces.size() || b.definition.value >= s.definitions.size() ||
      s.faces[b.face.value].definition_id != s.definitions[b.definition.value].id)
    invalid("face task source ownership mismatch");
  return {
      {OwnerKind::face, b.definition.value, s.faces[b.face.value].id.value}, b.face_generation, 0};
}
bool same_header(const BoundTaskHeader &a, const BoundTaskHeader &b) {
  return a.key == b.key && a.source_generation == b.source_generation &&
         a.boundary_generation == b.boundary_generation && a.bound_revision == b.bound_revision &&
         a.request_revision == b.request_revision && a.face_generation == b.face_generation;
}
template <std::size_t N, std::size_t... I>
std::array<Rational, N> read(bgfd::Budget &budget, const ExactTable &values,
                             const std::array<ExactRef, N> &refs, std::index_sequence<I...>) {
  return {import_exact(budget, values.view(refs[I]))...};
}
} // namespace
struct FaceOwnerJob::Impl final {
  FaceOwnerInput input;
  FaceOwnerLimits limits;
  std::shared_ptr<ResourceReservation> lease;
  bgfd::Budget budget;
  GeometryContext context;
  FaceEngine *engine = nullptr;
  FaceOwnerEvent event;
  std::pmr::vector<CellBoundResult> answers;
  std::optional<FaceCandidateResult> candidate_answer;
  std::pmr::vector<CellResultKey> contacts;
  bool incoming = false, finish = false, usable = true;
  uint64_t reported_work = 0;
  Impl(const std::shared_ptr<RunResource> &resource, FaceOwnerInput source, FaceOwnerLimits cap,
       std::stop_token stop)
      : input(std::move(source)), limits(cap), lease(reserve_resource(resource, cap.bytes)),
        budget({cap.bytes, cap.work}, lease.get(), std::move(stop)), context(budget),
        answers(resource.get()), contacts(resource.get()) {
    if (!cap.bytes || !cap.work || !cap.cell_batch)
      invalid("zero face task limit");
  }
  ~Impl() {
    if (engine) {
      std::destroy_at(engine);
      std::pmr::polymorphic_allocator<FaceEngine>(&budget).deallocate(engine, 1);
    }
  }
  void check() const {
    if (!usable)
      throw Error(ErrorCode::internal_error, "failed face task generation");
  }
  void initialize() {
    const auto factor =
        read(budget, input.metric_values, input.fixed_factor, std::make_index_sequence<4>{});
    const auto scales =
        read(budget, input.metric_values, input.scales, std::make_index_sequence<4>{});
    for (const auto &scale : scales)
      if (scale.sign() <= 0)
        invalid("face metric scale must be positive");
    std::pmr::polymorphic_allocator<FaceEngine> allocator(&budget);
    auto *value = allocator.allocate(1);
    try {
      std::construct_at(value, budget, context, input.boundary,
                        FaceMetricView{factor, {scales[0], scales[1], scales[2], scales[3]}},
                        scales[3], input.seed);
    } catch (...) {
      allocator.deallocate(value, 1);
      throw;
    }
    engine = value;
  }
  void consume() {
    if (!incoming)
      invalid("face continuation has no complete child/contact input");
    if (event.kind == FaceOwnerEventKind::cells) {
      for (const auto &result : answers)
        if (!engine->complete(result))
          invalid("current numerical child rejected by face owner");
      answers.clear();
    } else if (event.kind == FaceOwnerEventKind::candidate) {
      if (!candidate_answer || !engine->complete_candidate(*candidate_answer))
        invalid("current geometry child rejected by face owner");
      candidate_answer.reset();
    } else if (event.kind == FaceOwnerEventKind::settled) {
      for (const auto &cell : contacts)
        if (!engine->mark_contact_failure(cell))
          invalid("contact epoch marks a stale face cell");
      contacts.clear();
    } else
      invalid("face task cannot continue from this horizon");
    event = {};
    incoming = false;
  }
};
FaceOwnerJob::FaceOwnerJob(const std::shared_ptr<RunResource> &resource, FaceOwnerInput input,
                           FaceOwnerLimits limits, std::stop_token stop)
    : NativeTask(identity(input)), resource_(resource), impl_(nullptr) {
  if (!resource_)
    invalid("face task requires run resource");
  std::pmr::polymorphic_allocator<Impl> allocator(resource_.get());
  impl_ = allocator.allocate(1);
  try {
    std::construct_at(impl_, resource_, std::move(input), limits, std::move(stop));
  } catch (...) {
    allocator.deallocate(impl_, 1);
    throw;
  }
}
FaceOwnerJob::~FaceOwnerJob() {
  std::destroy_at(impl_);
  std::pmr::polymorphic_allocator<Impl>(resource_.get()).deallocate(impl_, 1);
}
StepState FaceOwnerJob::step(JobContext &) {
  impl_->check();
  try {
    impl_->budget.charge(1);
    if (impl_->finish) {
      impl_->event = {FaceOwnerEventKind::finished, {}, {}, {}};
      return StepState::done;
    }
    if (!impl_->engine)
      impl_->initialize();
    else
      impl_->consume();
    if (auto cells = impl_->engine->take_cell_work(impl_->limits.cell_batch)) {
      impl_->event = {FaceOwnerEventKind::cells, std::move(*cells), {}, {}};
      return StepState::wait_child;
    }
    const auto next = impl_->engine->advance();
    switch (next.kind) {
    case FaceAdvanceKind::candidate:
      impl_->event = {FaceOwnerEventKind::candidate, {}, next.candidate, {}};
      return StepState::wait_child;
    case FaceAdvanceKind::boundary_request:
      impl_->event = {FaceOwnerEventKind::boundary_horizon, {}, {}, next.boundary};
      return StepState::wait_barrier;
    case FaceAdvanceKind::settled:
      impl_->event = {FaceOwnerEventKind::settled, {}, {}, {}};
      return StepState::wait_barrier;
    case FaceAdvanceKind::unresolved:
      throw Error(ErrorCode::unresolved, "face cell exceeds fixed numerical profile");
    case FaceAdvanceKind::pending:
      throw Error(ErrorCode::internal_error, "face pending cell has no outstanding child packet");
    }
    throw Error(ErrorCode::internal_error, "unknown face advance state");
  } catch (...) {
    impl_->usable = false;
    throw;
  }
}
const FaceOwnerEvent &FaceOwnerJob::event() const {
  impl_->check();
  return impl_->event;
}
void FaceOwnerJob::finish_cells(std::span<const CellBoundResult> answers) {
  impl_->check();
  if (impl_->incoming || impl_->event.kind != FaceOwnerEventKind::cells || !impl_->event.cells)
    invalid("face task is not waiting for numerical results");
  const auto &requested = impl_->event.cells->cells;
  if (answers.size() != requested.size())
    invalid("incomplete face numerical result batch");
  // Sort the bounded reply index once. Packet order already follows the
  // canonical pending stream; arrival order never changes semantic priority.
  std::pmr::vector<uint32_t> order(resource_.get());
  order.reserve(answers.size());
  for (uint32_t i = 0; i < answers.size(); ++i)
    order.push_back(i);
  std::sort(order.begin(), order.end(),
            [&](auto a, auto b) { return answers[a].header.key < answers[b].header.key; });
  for (std::size_t i = 1; i < order.size(); ++i)
    if (answers[order[i - 1]].header.key == answers[order[i]].header.key)
      invalid("duplicate face numerical child key");
  for (const auto &request : requested) {
    const auto found = std::lower_bound(
        order.begin(), order.end(), request.header.key,
        [&](auto index, const auto &key) { return answers[index].header.key < key; });
    if (found == order.end() || !same_header(answers[*found].header, request.header))
      invalid("foreign or stale numerical child packet");
    if (answers[*found].decision != BoundDecision::accept &&
        answers[*found].decision != BoundDecision::bound_fail &&
        answers[*found].decision != BoundDecision::unresolved)
      invalid("invalid numerical child decision");
  }
  impl_->answers.assign(answers.begin(), answers.end());
  impl_->incoming = true;
}
void FaceOwnerJob::finish_candidate(FaceCandidateResult result) {
  impl_->check();
  if (impl_->incoming || impl_->event.kind != FaceOwnerEventKind::candidate ||
      !impl_->event.candidate || result.target != impl_->event.candidate->target ||
      result.ordinal != impl_->event.candidate->ordinal)
    invalid("foreign or stale geometry child packet");
  impl_->candidate_answer = std::move(result);
  impl_->incoming = true;
}
void FaceOwnerJob::mark_contacts(std::span<const CellResultKey> cells) {
  impl_->check();
  if (impl_->incoming || impl_->finish || impl_->event.kind != FaceOwnerEventKind::settled ||
      cells.empty())
    invalid("contact refinement requires settled face horizon");
  const auto &boundary = *impl_->input.boundary;
  const FaceFrontierIdentity expected{key.owner, boundary.face_generation,
                                      boundary.boundary_generation};
  std::pmr::vector<CellResultKey> ordered(cells.begin(), cells.end(), resource_.get());
  for (const auto &cell : ordered)
    if (cell.face != expected || !impl_->engine->topology().is_live(cell.triangle))
      invalid("contact epoch marks foreign face generation");
  std::sort(ordered.begin(), ordered.end());
  if (std::adjacent_find(ordered.begin(), ordered.end()) != ordered.end())
    invalid("contact epoch repeats a cell");
  impl_->contacts = std::move(ordered);
  impl_->incoming = true;
}
void FaceOwnerJob::finish_generation() {
  impl_->check();
  if (impl_->finish || impl_->incoming || impl_->event.kind != FaceOwnerEventKind::settled)
    invalid("only a settled face generation can finish");
  impl_->finish = true;
}
const FaceEngine &FaceOwnerJob::engine() const {
  impl_->check();
  if (!impl_->engine)
    invalid("face engine not initialized");
  return *impl_->engine;
}
FaceOwnerWork FaceOwnerJob::take_owner_work() {
  const auto work = impl_->budget.work();
  const FaceOwnerWork result{work - impl_->reported_work, impl_->budget.peak()};
  impl_->reported_work = work;
  return result;
}
} // namespace bgmesh
