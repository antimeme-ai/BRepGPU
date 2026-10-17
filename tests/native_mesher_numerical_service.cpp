#include "../native/mesher/numerical_service.hpp"
#include <iostream>
#include <stdexcept>

namespace {
using namespace bgmesh;
void require(bool condition, const char *message) {
  if (!condition)
    throw std::runtime_error(message);
}
template <class Function> void rejected(Function f, ErrorCode code) {
  bool caught = false;
  try {
    f();
  } catch (const Error &e) {
    caught = e.code == code;
  }
  require(caught, "numerical service failure classification");
}
template <std::size_t N, std::size_t... I>
std::array<Rational, N> read(bgfd::Budget &budget, const ExactTable &table,
                             const std::array<ExactRef, N> &refs, std::index_sequence<I...>) {
  return {import_exact(budget, table.view(refs[I]))...};
}
constexpr NumericalServiceLimits limits{
    {4, 4, 1ULL << 22, 1ULL << 31, 1ULL << 38}, {64, {4, 4}}, 8, {}};
const NativeTaskKey child{{OwnerKind::face, 0, 900}, 17, 42};
BoundTaskHeader header(uint64_t cell) { return {{900, cell, 5, 0}, 7, 9, 1, 13, 17}; }
BoundBatch curves(const std::shared_ptr<RunResource> &resource, const BoundRootBank &bank,
                  bgfd::Budget &budget) {
  ExactBuilder builder(resource, {{OwnerKind::boundary, 0, 400}, 9, 0});
  const auto zero = builder.append(Rational(budget)), one = builder.append(Rational(budget, 1));
  // Original side is Ce(t)=(1,t,1). Exact endpoint interpolation has zero error.
  const auto side = bank.geometry().view().source.view().curve_index.find(CurveId{2001});
  const auto root = find_curve_root(bank.geometry(), side, Rational(budget));
  const CurveBoundInput good{
      header(90), bank.find(root), {zero, one}, {one, zero, one, one, one, one}, zero};
  auto bad = good;
  bad.header = header(91);
  bad.actual_endpoint_xyz[2] = zero;
  const std::array<BoundInput, 2> inputs{good, bad};
  const std::array tables{builder.seal()};
  return seal_bound_batch(resource, bank, tables, inputs, limits.cells.batches);
}
FaceCellWork cells(const std::shared_ptr<RunResource> &resource, const GeometryStore &geometry,
                   bgfd::Budget &budget) {
  ExactBuilder builder(resource, {{OwnerKind::face, 0, 900}, 17, 0});
  const auto value = [&](int64_t n, int64_t d = 1) {
    return builder.append(Rational(budget, n).divide(Rational(budget, d)));
  };
  const auto zero = value(0), one = value(1), half = value(1, 2), quarter = value(1, 4);
  const std::array uv{zero, zero, one, zero, zero, one};
  // S=(u,v,max(0,2u-1)); L=(u,v,u) has exact maximal residual1/2.
  const std::array xyz{zero, zero, zero, one, zero, one, zero, one, zero};
  const std::array plane{value(2),    value(-1), value(3, 4), value(4),   value(-1),
                         value(3, 4), value(3),  value(2),    value(3, 4)};
  const auto eighth = value(1, 8);
  const auto table = builder.seal();
  auto data = std::allocate_shared<FaceCellWorkData>(RunAllocator<FaceCellWorkData>(resource),
                                                     resource, geometry, table);
  data->cells.push_back({header(1), {0}, CellMapKind::uv_triangle, uv, xyz, quarter});
  data->cells.push_back({header(0), {0}, CellMapKind::uv_triangle, uv, xyz, half});
  data->cells.push_back({header(3), {1}, CellMapKind::uv_triangle, uv, plane, eighth});
  data->cells.push_back({header(2), {1}, CellMapKind::uv_triangle, uv, plane, quarter});
  return data;
}
void execute(NativeScheduler &scheduler, const std::shared_ptr<NumericalServiceJob> &job) {
  scheduler.submit(job);
  for (;;) {
    const auto completion = scheduler.take();
    if (!completion)
      continue;
    completion->rethrow();
    if (completion->state == StepState::done)
      return;
    require(completion->state == StepState::ready, "numerical service blocks on child work");
    scheduler.submit(job);
  }
}
void exercise(const std::shared_ptr<RunResource> &resource, BoundRootBank bank,
              const char *module) {
  const auto baseline = resource->live();
  for (int32_t device = -1; device < (module ? 2 : 0); ++device) {
    {
      NativeScheduler scheduler(resource, {1, 4, 1ULL << 24, 1ULL << 33});
      const auto service_key = NativeTaskKey{{OwnerKind::run, 0, 100 + uint64_t(device + 1)}, 1, 0};
      auto service = std::allocate_shared<NumericalServiceJob>(
          RunAllocator<NumericalServiceJob>(resource), resource, service_key, bank, limits,
          scheduler.stop_token());
      if (device >= 0)
        service->open_device(load_bound_cuda(resource, module), {device, 1ULL << 20, 1ULL << 20});
      bgfd::Budget scratch({1ULL << 22, 1ULL << 31}, resource.get());
      auto curve = curves(resource, bank, scratch);
      service->begin({child, std::move(curve)});
      rejected([&] { service->begin({child, cells(resource, bank.geometry(), scratch)}); },
               ErrorCode::internal_error);
      execute(scheduler, service);
      require(service->result().key == child, "service lost coordinator request identity");
      const auto curve_result = std::get<BoundCompleted>(service->result().output);
      require(curve_result.answers().size() == 2 &&
                  curve_result.answers()[0].decision == BoundDecision::accept &&
                  curve_result.answers()[1].decision == BoundDecision::bound_fail,
              "original line chord decisions disagree");
      service->release_result();
      auto request = cells(resource, bank.geometry(), scratch);
      service->begin({{child.owner, child.generation, child.ordinal + 1}, request});
      request.reset(); // service owns immutable values, not caller scratch or handle
      execute(scheduler, service);
      require(service->result().key ==
                  NativeTaskKey{child.owner, child.generation, child.ordinal + 1},
              "reused service returned a previous child identity");
      const auto retained =
          std::get<std::shared_ptr<const CellBatchResult>>(service->result().output);
      require(retained->cells.size() == 4, "service lost actual cells");
      constexpr std::array<uint64_t, 4> order{1, 0, 3, 2};
      for (uint64_t i = 0; i < 4; ++i) {
        const auto &answer = retained->cells[i];
        require(answer.header.key == header(order[i]).key && answer.header.face_generation == 17 &&
                    answer.header.request_revision == 13 && answer.header.bound_revision == 1 &&
                    answer.header.source_generation == 7 && answer.header.boundary_generation == 9,
                "full actual-cell envelope changed");
        require(answer.decision ==
                    (order[i] % 2 ? BoundDecision::bound_fail : BoundDecision::accept),
                "C0/analytic actual-cell decisions disagree with exact fixture geometry");
      }
      require(service->stats().mixed_cell_batches > 0, "mixed-cell batch path was not exercised");
      require(service->stats().runtime.exact_tasks > 0,
              "exact equality fallback was not exercised");
      service->release_result();
      require(retained->cells[0].decision == BoundDecision::bound_fail,
              "released service invalidated retained immutable results");
      service->close();
      rejected([&] { service->begin({child, cells(resource, bank.geometry(), scratch)}); },
               ErrorCode::internal_error);
    }
    require(resource->live() == baseline, "healthy persistent service leaked run allocations");
  }
}
void failures(const std::shared_ptr<RunResource> &resource, BoundRootBank bank,
              bgfd::Budget &scratch) {
  const NativeTaskKey service_key{{OwnerKind::run, 0, 500}, 1, 0};
  rejected([&] { NumericalServiceJob invalid(resource, service_key, bank, limits, {}); },
           ErrorCode::invalid_source);
  NativeScheduler scheduler(resource, {1, 4, 1ULL << 24, 1ULL << 33});
  auto service = std::allocate_shared<NumericalServiceJob>(
      RunAllocator<NumericalServiceJob>(resource), resource, service_key, bank, limits,
      scheduler.stop_token());
  rejected([&] { static_cast<void>(service->result()); }, ErrorCode::internal_error);
  auto stale = std::const_pointer_cast<FaceCellWorkData>(cells(resource, bank.geometry(), scratch));
  ++stale->cells.front().header.source_generation;
  rejected([&] { service->begin({child, FaceCellWork(stale)}); }, ErrorCode::invalid_source);
  const auto foreign = compile_geometry(bank.geometry().view().source, resource,
                                        {{OwnerKind::geometry, 0, 88}, 7, 0}, scratch);
  rejected([&] { service->begin({child, cells(resource, foreign, scratch)}); },
           ErrorCode::invalid_source);
  auto duplicate =
      std::const_pointer_cast<FaceCellWorkData>(cells(resource, bank.geometry(), scratch));
  duplicate->cells[1].header = duplicate->cells[0].header;
  rejected([&] { service->begin({child, FaceCellWork(duplicate)}); }, ErrorCode::invalid_source);
  {
    const auto work = cells(resource, bank.geometry(), scratch);
    const auto &cell = work->cells.front();
    const auto uv = read(scratch, work->values, cell.original_uv, std::make_index_sequence<6>{});
    const auto xyz = read(scratch, work->values, cell.actual_xyz, std::make_index_sequence<9>{});
    const auto tau = import_exact(scratch, work->values.view(cell.allowance));
    const auto prepared = prepare_actual_cell(
        scratch, resource, bank, {cell.header, cell.carrier, cell.map_kind, uv, xyz, tau},
        {{OwnerKind::run, 0, 600}, 1, 0}, limits.cells);
    require(prepared.pieces().size() == 3, "atomic fold fixture lost C0 pieces");
    auto stale_piece = prepared.pieces()[1];
    ++stale_piece.header.face_generation;
    const std::array<BoundInput, 2> inputs{prepared.pieces()[0], stale_piece};
    const std::array tables{prepared.values()};
    const auto batch = seal_bound_batch(resource, bank, tables, inputs, limits.cells.batches);
    BoundRuntime runtime(resource, bank, limits.runtime);
    const auto completed = runtime.resolve(runtime.filter_cpu(batch));
    CellBoundFold fold(resource, prepared);
    rejected([&] { fold.add(completed, {0, 0}); }, ErrorCode::invalid_source);
    rejected([&] { fold.add(completed, {1, 2}); }, ErrorCode::invalid_source);
    rejected([&] { fold.add(completed, {0, 2}); }, ErrorCode::invalid_source);
    // A valid first answer in a rejected range must stay unseen.
    fold.add(completed, {0, 1});
    rejected([&] { fold.add(completed, {0, 1}); }, ErrorCode::invalid_source);
    require(!fold.complete(), "range fold ignored missing clipped pieces");
    runtime.close();
  }
  auto small = limits;
  small.cell_capacity = 1;
  {
    NumericalServiceJob bounded(resource, {{OwnerKind::run, 0, 501}, 1, 0}, bank, small,
                                scheduler.stop_token());
    rejected([&] { bounded.begin({child, cells(resource, bank.geometry(), scratch)}); },
             ErrorCode::invalid_source);
    bounded.close();
  }
  // Private scratch exhaustion is a typed failure, never a geometric result.
  service->begin({child, cells(resource, bank.geometry(), scratch)});
  JobContext tiny({1, 1ULL << 30}, resource.get(), scheduler.stop_token());
  rejected([&] { static_cast<void>(service->step(tiny)); }, ErrorCode::resource_exhausted);
  rejected([&] { static_cast<void>(service->result()); }, ErrorCode::internal_error);
  service->close();
  {
    NumericalServiceJob cancelled(resource, {{OwnerKind::run, 0, 502}, 1, 0}, bank, limits,
                                  scheduler.stop_token());
    cancelled.begin({child, curves(resource, bank, scratch)});
    scheduler.stop();
    JobContext job({1ULL << 22, 1ULL << 31}, resource.get(), scheduler.stop_token());
    rejected([&] { static_cast<void>(cancelled.step(job)); }, ErrorCode::cancelled);
    cancelled.close();
  }
}
} // namespace
int main(int argc, char **argv) {
  try {
    require(argc == 2 || argc == 3, "source [CUDA module] arguments required");
    const auto resource = std::make_shared<RunResource>(1ULL << 29);
    {
      const auto source = load_source(argv[1], resource);
      bgfd::Budget scratch({1ULL << 24, 1ULL << 34}, resource.get());
      const auto geometry =
          compile_geometry(source, resource, {{OwnerKind::geometry, 0, 0}, 7, 0}, scratch);
      const auto side = find_curve_root(geometry, source.view().curve_index.find(CurveId{2001}),
                                        Rational(scratch));
      const std::array<BoundRootSelection, 3> roots{SurfaceRootSlot{0}, SurfaceRootSlot{1}, side};
      const auto bank = prepare_bound_roots(scratch, resource, geometry, roots,
                                            {{OwnerKind::geometry, 0, 0}, 7, 1});
      exercise(resource, bank, argc == 3 ? argv[2] : nullptr);
      failures(resource, bank, scratch);
    }
    require(resource->live() == 0, "persistent service retained owners after close");
    std::cout << "PASS persistent curve/C0/analytic numerical service\n";
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
