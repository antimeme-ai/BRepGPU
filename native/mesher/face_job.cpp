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
  std::stop_token stop;
  bgfd::Budget budget;
  GeometryContext context;
  FaceEngine *engine = nullptr;
  FaceOwnerEvent event;
  std::pmr::vector<CellBoundResult> answers;
  std::optional<FaceCandidateResult> candidate_answer;
  std::pmr::vector<CellResultKey> contacts;
  enum class Command { none, begin_optimizer, prepare_flip, reject_flip, finish_flip };
  Command command = Command::none;
  EdgeHandle selected_edge;
  std::optional<std::array<CellBoundResult, 2>> flip_answers;
  std::optional<ContactApproval> flip_approval;
  FaceResult output;
  bool incoming = false, finish = false, usable = true;
  uint64_t reported_work = 0;
  Impl(const std::shared_ptr<RunResource> &resource, FaceOwnerInput source, FaceOwnerLimits cap,
       std::stop_token token)
      : input(std::move(source)), limits(cap), lease(reserve_resource(resource, cap.bytes)),
        stop(std::move(token)), budget({cap.bytes, cap.work}, lease.get(), stop), context(budget),
        answers(resource.get()), contacts(resource.get()) {
    if (!cap.bytes || !cap.work || !cap.cell_batch || !stop.stop_possible())
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
  FaceHorizon snapshot_contacts() {
    const auto &b = *input.boundary;
    const auto &source = b.geometry.view().source.view();
    const FaceFrontierIdentity face{
        {OwnerKind::face, b.definition.value, source.faces[b.face.value].id.value},
        b.face_generation,
        b.boundary_generation};
    auto result = std::allocate_shared<FaceHorizonData>(RunAllocator<FaceHorizonData>(b.resource),
                                                        b.resource, face);
    for (const auto &edit : engine->take_changes())
      for (uint32_t i = 0; i < edit.removed_count; ++i)
        result->removed.push_back(edit.removed[i]);
    for (const auto &key : engine->take_settled()) {
      budget.charge(1);
      const auto triangle = engine->topology().triangle(key.triangle);
      std::array<PhysicalNode, 3> nodes;
      for (std::size_t i = 0; i < 3; ++i) {
        const auto &node = engine->node(triangle.sites[i]);
        // Contact work carries physical identity/words, no unleased parameter.
        nodes[i] = {node.key, node.xyz, {}};
      }
      result->inserted.push_back({key, nodes, engine->declared_edges(key.triangle)});
    }
    return result;
  }
  std::shared_ptr<FaceOptimizerReplyData> optimizer_reply(std::span<const EdgeHandle> edges) {
    auto result = std::allocate_shared<FaceOptimizerReplyData>(
        RunAllocator<FaceOptimizerReplyData>(input.boundary->resource), input.boundary->resource);
    result->edges.reserve(edges.size());
    for (const auto edge : edges) {
      budget.charge(1);
      result->edges.push_back({edge, engine->optimizer_edge_key(edge)});
    }
    std::sort(result->edges.begin(), result->edges.end(),
              [](const auto &a, const auto &b) { return a.key < b.key; });
    return result;
  }
  StepState optimize() {
    const auto selected = command;
    command = Command::none;
    if (selected == Command::prepare_flip) {
      auto work = engine->prepare_flip(selected_edge);
      event = {};
      if (work) {
        event.kind = FaceOwnerEventKind::optimizer_proposal;
        event.optimization = std::move(*work);
        return StepState::wait_child;
      }
      event.kind = FaceOwnerEventKind::optimizer_queue;
      event.optimizer = optimizer_reply({});
      return StepState::wait_barrier;
    }
    std::shared_ptr<FaceOptimizerReplyData> reply;
    if (selected == Command::begin_optimizer) {
      const auto edges = engine->begin_optimizer();
      reply = optimizer_reply(edges);
    } else if (selected == Command::reject_flip) {
      engine->reject_flip(event.optimization->ordinal);
      reply = optimizer_reply({});
    } else if (selected == Command::finish_flip) {
      if (!flip_answers || !flip_approval)
        invalid("optimizer continuation has no accepted numerical/contact replies");
      const auto committed =
          engine->complete_flip(event.optimization->ordinal, *flip_answers, *flip_approval);
      reply = optimizer_reply(committed.affected_edges);
      reply->committed = committed.cells;
      flip_answers.reset();
      flip_approval.reset();
    } else
      invalid("optimizer continuation has no command");
    event = {};
    event.kind = reply->committed ? FaceOwnerEventKind::optimizer_committed
                                  : FaceOwnerEventKind::optimizer_queue;
    event.optimizer = std::move(reply);
    return StepState::wait_barrier;
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
  if (!resource_ || resource_ != input.boundary->resource ||
      resource_ != input.metric_values.resource() ||
      resource_ != input.boundary->geometry.view().source.view().resource)
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
StepState FaceOwnerJob::step(JobContext &job) {
  impl_->check();
  try {
    if (job.stop != impl_->stop)
      invalid("face owner requires its scheduler stop token");
    impl_->budget.charge(1);
    if (impl_->finish) {
      impl_->output = impl_->engine->freeze_result();
      impl_->event = {FaceOwnerEventKind::finished, {}, {}, {}};
      return StepState::done;
    }
    if (impl_->command != Impl::Command::none)
      return impl_->optimize();
    if (!impl_->engine)
      impl_->initialize();
    else
      impl_->consume();
    for (;;) {
      impl_->budget.charge(1);
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
      impl_->event.horizon = impl_->snapshot_contacts();
      return StepState::wait_barrier;
    case FaceAdvanceKind::unresolved:
      throw Error(ErrorCode::unresolved, "face cell exceeds fixed numerical profile");
    case FaceAdvanceKind::pending: {
      auto cells = impl_->engine->take_cell_work(impl_->limits.cell_batch);
      if (!cells)
        continue; // Host domain decisions enter the same canonical frontier.
      impl_->event = {FaceOwnerEventKind::cells, std::move(*cells), {}, {}};
      return StepState::wait_child;
    }
    }
      throw Error(ErrorCode::internal_error, "unknown face advance state");
    }
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
  if (impl_->incoming || impl_->finish || impl_->command != Impl::Command::none ||
      impl_->event.kind != FaceOwnerEventKind::settled || cells.empty())
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
  if (impl_->finish || impl_->incoming || impl_->command != Impl::Command::none ||
      (impl_->event.kind != FaceOwnerEventKind::settled &&
       impl_->event.kind != FaceOwnerEventKind::optimizer_queue &&
       impl_->event.kind != FaceOwnerEventKind::optimizer_committed))
    invalid("only a settled face generation can finish");
  impl_->finish = true;
}
void FaceOwnerJob::begin_optimizer() {
  impl_->check();
  if (impl_->finish || impl_->incoming || impl_->command != Impl::Command::none ||
      impl_->event.kind != FaceOwnerEventKind::settled)
    invalid("optimizer requires the settled face horizon");
  impl_->command = Impl::Command::begin_optimizer;
}
void FaceOwnerJob::prepare_flip(EdgeHandle edge) {
  impl_->check();
  if (impl_->finish || impl_->incoming || impl_->command != Impl::Command::none ||
      (impl_->event.kind != FaceOwnerEventKind::optimizer_queue &&
       impl_->event.kind != FaceOwnerEventKind::optimizer_committed))
    invalid("optimizer proposal requires an idle optimizer owner");
  impl_->selected_edge = edge;
  impl_->command = Impl::Command::prepare_flip;
}
void FaceOwnerJob::reject_flip() {
  impl_->check();
  if (impl_->command != Impl::Command::none ||
      impl_->event.kind != FaceOwnerEventKind::optimizer_proposal || !impl_->event.optimization)
    invalid("optimizer rejection requires current proposal");
  impl_->command = Impl::Command::reject_flip;
}
void FaceOwnerJob::finish_flip(const std::array<CellBoundResult, 2> &answers,
                               ContactApproval approval) {
  impl_->check();
  if (impl_->command != Impl::Command::none ||
      impl_->event.kind != FaceOwnerEventKind::optimizer_proposal || !impl_->event.optimization)
    invalid("optimizer completion requires current proposal");
  const auto &work = *impl_->event.optimization;
  for (std::size_t i = 0; i < 2; ++i)
    if (!same_header(answers[i].header, work.cells->cells[i].header) ||
        answers[i].decision != BoundDecision::accept)
      invalid("optimizer completion requires its two accepted current bounds");
  if (!contact_approval_matches(approval, {work.ordinal, work.old_cells, work.nodes, work.edges}))
    invalid("optimizer completion requires matching contact approval");
  impl_->flip_answers = answers;
  impl_->flip_approval = std::move(approval);
  impl_->command = Impl::Command::finish_flip;
}
FaceResult FaceOwnerJob::result() const {
  impl_->check();
  if (!impl_->output)
    invalid("face generation has not published its final result");
  return impl_->output;
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
