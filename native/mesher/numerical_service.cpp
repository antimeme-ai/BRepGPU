#include "numerical_service.hpp"
#include "plane_cell_bounds.hpp"
#include <algorithm>
#include <limits>

namespace bgmesh {
namespace {
[[noreturn]] void invalid(const char *message) { throw Error(ErrorCode::invalid_source, message); }
[[noreturn]] void internal(const char *message) { throw Error(ErrorCode::internal_error, message); }
template <std::size_t N, std::size_t... I>
std::array<Rational, N> read(bgfd::Budget &budget, const ExactTable &table,
                             const std::array<ExactRef, N> &refs, std::index_sequence<I...>) {
  return {import_exact(budget, table.view(refs[I]))...};
}
void plane_source(const FaceCellWorkData &work, const std::shared_ptr<RunResource> &resource) {
  const auto &seed = work.chart_source;
  if (!seed || seed->resource != resource || &seed->geometry.view() != &work.geometry.view() ||
      seed->chart_kind != WorkingChartKind::physical_plane_convex_q || !seed->plane)
    invalid("plane numerical packet lacks its admitted source profile");
  const auto &source = work.geometry.view().source.view();
  if (seed->face.value >= source.faces.size() ||
      seed->definition.value >= source.definitions.size() ||
      seed->source_generation != work.geometry.view().table.key().generation)
    invalid("plane numerical source identity mismatch");
  const auto &face = source.faces[seed->face.value];
  if (face.definition_id != source.definitions[seed->definition.value].id)
    invalid("plane numerical face definition mismatch");
  const auto carrier = source.carrier_index.find(face.carrier_id);
  for (const auto &cell : work.cells)
    if (cell.map_kind != CellMapKind::plane_xy_triangle || cell.carrier != carrier ||
        cell.header.key.owner != face.id.value || cell.header.key.ordinal != 0 ||
        cell.header.bound_revision != 1 ||
        cell.header.source_generation != seed->source_generation ||
        cell.header.boundary_generation != seed->boundary_generation ||
        cell.header.face_generation != seed->face_generation)
      invalid("plane numerical cell escaped its source face/generations");
}
} // namespace
struct NumericalServiceJob::Impl final {
  enum class Phase { idle, preparing, dispatch, waiting, complete, failed, closed };
  struct Prepared final {
    PreparedCellBounds packet;
    CellBoundFold fold;
    uint32_t cell;
    Prepared(const std::shared_ptr<RunResource> &resource, PreparedCellBounds input, uint32_t index)
        : packet(std::move(input)), fold(resource, packet), cell(index) {}
  };
  struct Route final {
    uint32_t prepared;
    CellPieceRange response;
  };
  std::shared_ptr<RunResource> resource;
  NativeTaskKey identity;
  BoundRootBank bank;
  NumericalServiceLimits limits;
  BoundRuntime runtime;
  std::stop_token stop;
  Phase phase = Phase::idle;
  bool device = false;
  uint64_t request_ordinal = 0;
  uint32_t table_ordinal = 0, next_cell = 0, next_prepared = 0, next_piece = 0;
  std::optional<NumericalRequest> request;
  std::optional<NumericalResult> result;
  std::optional<BoundBatch> pending;
  std::optional<BoundTicket> ticket;
  std::shared_ptr<CellBatchResult> cell_results;
  std::pmr::vector<Prepared> prepared;
  std::pmr::vector<Route> routes;
  NumericalServiceStats counters;

  Impl(std::shared_ptr<RunResource> owner, NativeTaskKey service, BoundRootBank roots,
       NumericalServiceLimits cap, std::stop_token token)
      : resource(std::move(owner)), identity(service), bank(std::move(roots)), limits(cap),
        runtime(resource, bank, cap.runtime, token), stop(std::move(token)),
        prepared(resource.get()), routes(resource.get()) {
    if (!stop.stop_possible() || identity.owner.kind != OwnerKind::run || !cap.cell_capacity ||
        !cap.cells.max_pieces || !cap.cells.batches.task_capacity ||
        !cap.cells.batches.table_capacity ||
        cap.cells.batches.task_capacity > cap.runtime.task_capacity ||
        cap.cells.batches.table_capacity > cap.runtime.table_capacity)
      invalid("invalid numerical service ownership/capacities");
    prepared.reserve(cap.cell_capacity);
    routes.reserve(std::min(cap.cell_capacity, cap.cells.batches.task_capacity));
  }
  void idle() const {
    if (phase != Phase::idle)
      internal("numerical service is not idle");
  }
  const NumericalRequest &current() const {
    if (!request)
      internal("numerical service lost active request");
    return *request;
  }
  ExactTableKey next_table() {
    if (table_ordinal == std::numeric_limits<uint32_t>::max())
      throw Error(ErrorCode::resource_exhausted, "numerical service table ordinals exhausted");
    return {identity.owner, identity.generation, ++table_ordinal};
  }
  void prepare(JobContext &job) {
    const auto &work = *std::get<FaceCellWork>(current().input);
    const auto &cell = work.cells[next_cell];
    if (cell.map_kind == CellMapKind::plane_xy_triangle) {
      prepare_plane(job, work);
      return;
    }
    const auto uv = read(job.budget, work.values, cell.original_uv, std::make_index_sequence<6>{});
    const auto xyz = read(job.budget, work.values, cell.actual_xyz, std::make_index_sequence<9>{});
    const auto allowance = import_exact(job.budget, work.values.view(cell.allowance));
    const ActualTriangleView view{cell.header, cell.carrier, cell.map_kind, uv, xyz, allowance};
    if (bank.geometry().view().carriers[cell.carrier.value].kind <= 5) {
      GeometryContext context(job.budget);
      cell_results->cells[next_cell] =
          bound_analytic_cell(context, bank.geometry(), view, limits.analytic);
      ++counters.analytic_cells;
    } else {
      auto packet =
          prepare_actual_cell(job.budget, resource, bank, view, next_table(), limits.cells);
      prepared.emplace_back(resource, std::move(packet), next_cell);
      ++counters.prepared_cells;
    }
    ++next_cell;
    if (next_cell == work.cells.size()) {
      // Frontier packet order is geometric priority, not task-key order. The
      // runtime requires sorted keys; retain original cell indices for output.
      std::sort(prepared.begin(), prepared.end(), [](const auto &a, const auto &b) {
        return a.packet.parent().key < b.packet.parent().key;
      });
      phase = Phase::dispatch;
    }
  }
  void prepare_plane(JobContext &job, const FaceCellWorkData &work) {
    // The source lease was bound at admission. Import this constant profile
    // once into the same step Budget as the entire bounded cell batch.
    const auto &seed = work.chart_source;
    if (!seed)
      internal("plane service lost its source profile");
    const auto &plane = seed->plane;
    if (!plane.has_value())
      internal("plane service lost its source profile");
    const auto &frozen = plane.value();
    const auto origin =
        read(job.budget, seed->values, frozen.origin, std::make_index_sequence<3>{});
    const auto basis = read(job.budget, seed->values, frozen.basis, std::make_index_sequence<6>{});
    const auto q =
        read(job.budget, seed->values, frozen.quadrilateral, std::make_index_sequence<8>{});
    const auto height = import_exact(job.budget, seed->values.view(frozen.height_upper));
    const PlaneCellProfile profile(job.budget, {origin, basis, q, height});
    for (std::size_t i = 0; i < work.cells.size(); ++i) {
      const auto &cell = work.cells[i];
      const auto xy = read(job.budget, work.values, cell.chart_xy, std::make_index_sequence<6>{});
      const auto xyz =
          read(job.budget, work.values, cell.actual_xyz, std::make_index_sequence<9>{});
      const auto allowance = import_exact(job.budget, work.values.view(cell.allowance));
      cell_results->cells[i] =
          bound_plane_cell(profile, {cell.header, cell.map_kind, xy, xyz, allowance});
      ++counters.plane_cells;
    }
    result.emplace(
        NumericalResult{current().key, std::shared_ptr<const CellBatchResult>(cell_results)});
    phase = Phase::complete;
  }
  void pack(bgfd::Budget &scratch) {
    std::pmr::vector<BoundInput> inputs(&scratch);
    std::pmr::vector<ExactTable> tables(&scratch);
    inputs.reserve(limits.cells.batches.task_capacity);
    tables.reserve(limits.cells.batches.table_capacity);
    routes.clear();
    while (next_prepared < prepared.size() && inputs.size() < limits.cells.batches.task_capacity &&
           tables.size() < limits.cells.batches.table_capacity) {
      const auto &p = prepared[next_prepared].packet;
      const auto count = static_cast<uint32_t>(std::min<std::size_t>(
          p.pieces().size() - next_piece, limits.cells.batches.task_capacity - inputs.size()));
      const auto first = static_cast<uint32_t>(inputs.size());
      tables.push_back(p.values());
      for (uint32_t i = 0; i < count; ++i)
        inputs.push_back(p.pieces()[next_piece + i]);
      routes.push_back({next_prepared, {first, count}});
      next_piece += count;
      if (next_piece == p.pieces().size()) {
        next_piece = 0;
        ++next_prepared;
      }
    }
    if (inputs.empty())
      internal("numerical service made empty rational batch");
    pending = seal_bound_batch(resource, bank, tables, inputs, limits.cells.batches);
    if (routes.size() > 1)
      ++counters.mixed_cell_batches;
  }
  StepState finish(BoundCompleted completion) {
    const auto &active = current();
    if (std::holds_alternative<BoundBatch>(active.input)) {
      result.emplace(NumericalResult{active.key, std::move(completion)});
      pending.reset();
      phase = Phase::complete;
      return StepState::done;
    }
    for (const auto &route : routes) {
      auto &p = prepared[route.prepared];
      p.fold.add(completion, route.response);
      if (p.fold.complete())
        cell_results->cells[p.cell] = p.fold.result();
    }
    pending.reset();
    routes.clear();
    phase = Phase::dispatch;
    return StepState::ready;
  }
  StepState dispatch(JobContext &job) {
    const auto &active = current();
    if (std::holds_alternative<FaceCellWork>(active.input) && !pending) {
      if (next_prepared == prepared.size()) {
        for (const auto &p : prepared)
          if (!p.fold.complete())
            internal("numerical service published incomplete cell fold");
        result.emplace(
            NumericalResult{active.key, std::shared_ptr<const CellBatchResult>(cell_results)});
        phase = Phase::complete;
        return StepState::done;
      }
      pack(job.budget);
    }
    if (!pending)
      internal("numerical service lost dispatch packet");
    const auto packet = *pending;
    ++counters.batches;
    if (device) {
      ticket = runtime.submit(packet);
      phase = Phase::waiting;
      return StepState::ready;
    }
    return finish(runtime.resolve(runtime.filter_cpu(packet)));
  }
};
NumericalServiceJob::NumericalServiceJob(const std::shared_ptr<RunResource> &resource,
                                         NativeTaskKey service, BoundRootBank bank,
                                         NumericalServiceLimits limits, std::stop_token stop)
    : NativeTask(service), resource_(resource), impl_(nullptr) {
  if (!resource_)
    internal("null numerical service resource");
  std::pmr::polymorphic_allocator<Impl> allocator(resource_.get());
  impl_ = allocator.allocate(1);
  try {
    std::construct_at(impl_, resource_, service, std::move(bank), limits, std::move(stop));
  } catch (...) {
    allocator.deallocate(impl_, 1);
    throw;
  }
}
NumericalServiceJob::~NumericalServiceJob() {
  std::destroy_at(impl_);
  std::pmr::polymorphic_allocator<Impl>(resource_.get()).deallocate(impl_, 1);
}
void NumericalServiceJob::open_device(BoundCudaModule module, BoundDeviceLimits limits) {
  impl_->idle();
  if (impl_->device || impl_->counters.requests)
    internal("device must open once before numerical service requests");
  try {
    impl_->runtime.open_device(std::move(module), limits);
    impl_->device = true;
  } catch (...) {
    impl_->runtime.cancel();
    impl_->phase = Impl::Phase::failed;
    throw;
  }
}
void NumericalServiceJob::begin(NumericalRequest request) {
  auto &p = *impl_;
  p.idle();
  if (p.stop.stop_requested())
    throw Error(ErrorCode::cancelled, "numerical service admission cancelled");
  if (p.request_ordinal == std::numeric_limits<uint64_t>::max())
    throw Error(ErrorCode::resource_exhausted, "numerical service requests exhausted");
  if (const auto *batch = std::get_if<BoundBatch>(&request.input)) {
    if (batch->inputs().empty() || batch->inputs().size() > p.limits.runtime.task_capacity)
      invalid("numerical service batch capacity exceeded");
    p.pending = *batch;
    p.phase = Impl::Phase::dispatch;
  } else {
    const auto &work = std::get<FaceCellWork>(request.input);
    if (!work || work->resource != resource_ ||
        &work->geometry.view() != &p.bank.geometry().view() || work->cells.empty() ||
        work->cells.size() > p.limits.cell_capacity)
      invalid("numerical service foreign geometry/cell capacity");
    std::pmr::vector<BoundTaskKey> keys(resource_.get());
    keys.reserve(work->cells.size());
    for (const auto &cell : work->cells) {
      if (cell.carrier.value >= p.bank.geometry().view().carriers.size() ||
          cell.header.source_generation != p.bank.geometry().view().table.key().generation)
        invalid("numerical service cell source mismatch");
      keys.push_back(cell.header.key);
    }
    std::sort(keys.begin(), keys.end());
    if (std::adjacent_find(keys.begin(), keys.end()) != keys.end())
      invalid("numerical service repeated cell key");
    if (std::any_of(
            work->cells.begin(), work->cells.end(),
            [](const auto &cell) { return cell.map_kind == CellMapKind::plane_xy_triangle; }) ||
        (work->chart_source &&
         work->chart_source->chart_kind == WorkingChartKind::physical_plane_convex_q))
      plane_source(*work, resource_);
    auto answers =
        std::allocate_shared<CellBatchResult>(RunAllocator<CellBatchResult>(resource_), resource_);
    answers->cells.resize(work->cells.size());
    p.cell_results = std::move(answers);
    p.phase = Impl::Phase::preparing;
  }
  p.request.emplace(std::move(request));
  ++p.request_ordinal;
  ++p.counters.requests;
}
StepState NumericalServiceJob::step(JobContext &job) {
  auto &p = *impl_;
  try {
    if (job.stop != p.stop)
      internal("numerical service requires its scheduler stop token");
    job.budget.charge(0);
    if (p.stop.stop_requested())
      throw Error(ErrorCode::cancelled, "numerical service cancelled");
    switch (p.phase) {
    case Impl::Phase::preparing:
      p.prepare(job);
      return p.phase == Impl::Phase::complete ? StepState::done : StepState::ready;
    case Impl::Phase::dispatch:
      return p.dispatch(job);
    case Impl::Phase::waiting: {
      if (!p.ticket)
        internal("numerical service lost device ticket");
      auto filtered = p.runtime.poll(*p.ticket);
      if (!filtered)
        return StepState::ready;
      p.ticket.reset();
      return p.finish(p.runtime.resolve(*filtered));
    }
    default:
      internal("numerical service step has no active request");
    }
  } catch (const bgfd::Cancelled &) {
    p.runtime.cancel();
    p.phase = Impl::Phase::failed;
    throw Error(ErrorCode::cancelled, "numerical service scratch cancelled");
  } catch (const bgfd::Failure &failure) {
    p.runtime.cancel();
    p.phase = Impl::Phase::failed;
    throw Error(failure.status == BGFD_RESOURCE_EXHAUSTED ? ErrorCode::resource_exhausted
                                                          : ErrorCode::internal_error,
                failure.what());
  } catch (...) {
    p.runtime.cancel();
    p.phase = Impl::Phase::failed;
    throw;
  }
}
const NumericalResult &NumericalServiceJob::result() const {
  if (impl_->phase != Impl::Phase::complete || !impl_->result)
    internal("numerical service result requested before completion");
  return *impl_->result;
}
void NumericalServiceJob::release_result() {
  auto &p = *impl_;
  if (p.phase != Impl::Phase::complete)
    internal("numerical service has no completed result to release");
  p.result.reset();
  p.request.reset();
  p.cell_results.reset();
  p.prepared.clear();
  p.routes.clear();
  p.next_cell = p.next_prepared = p.next_piece = 0;
  p.phase = Impl::Phase::idle;
}
NumericalServiceStats NumericalServiceJob::stats() const noexcept {
  auto counters = impl_->counters;
  counters.runtime = impl_->runtime.stats();
  return counters;
}
void NumericalServiceJob::close() {
  auto &p = *impl_;
  if (p.phase == Impl::Phase::closed)
    return;
  if (p.phase != Impl::Phase::idle && p.phase != Impl::Phase::complete)
    p.runtime.cancel();
  p.runtime.close();
  p.ticket.reset();
  p.pending.reset();
  p.request.reset();
  p.result.reset();
  p.cell_results.reset();
  p.prepared.clear();
  p.routes.clear();
  p.phase = Impl::Phase::closed;
}
} // namespace bgmesh
