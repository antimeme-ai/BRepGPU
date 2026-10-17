#include "../native/mesher/assembly.hpp"
#include "../native/mesher/face_engine.hpp"
#include "../native/mesher/face_preparation.hpp"
#include "../native/mesher/geometry_domain.hpp"
#include "../native/mesher/numerical_service.hpp"
#include <filesystem>
#include <iostream>
#include <stdexcept>

namespace {
using namespace bgmesh;
void require(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}
struct Fixture final {
  std::shared_ptr<RunResource> resource;
  SourceStore source;
  bgfd::Budget budget;
  GeometryStore geometry;
  GeometryContext context;
  DefinitionBoundaryForest forest;
  BoundRootBank bank;
  NativeScheduler scheduler;
  std::shared_ptr<NumericalServiceJob> service;
  uint64_t serial = 0;
  static BoundRootBank roots(bgfd::Budget &budget, const std::shared_ptr<RunResource> &resource,
                             const GeometryStore &geometry) {
    std::pmr::vector<BoundRootSelection> selected(&budget);
    for (uint32_t i = 0; i < geometry.view().surface_roots.size(); ++i)
      selected.emplace_back(SurfaceRootSlot{i});
    for (uint32_t i = 0; i < geometry.view().curve_roots.size(); ++i)
      if (geometry.view().curves[geometry.view().curve_roots[i].source.value].dimension == 3)
        selected.emplace_back(CurveRootSlot{i});
    return prepare_bound_roots(budget, resource, geometry, selected,
                               {{OwnerKind::geometry, 0, 151}, 3, 1});
  }
  explicit Fixture(const std::filesystem::path &path)
      : resource(std::make_shared<RunResource>(uint64_t{1} << 30)),
        source(load_source(path, resource)),
        budget({uint64_t{1} << 28, uint64_t{1} << 39}, resource.get()),
        geometry(compile_geometry(source, resource, {{OwnerKind::geometry, 0, 150}, 3, 0}, budget)),
        context(budget), forest(budget, source, {0}, resource),
        bank(roots(budget, resource, geometry)),
        scheduler(resource, {1, 4, uint64_t{1} << 26, uint64_t{1} << 36}) {
    const NumericalServiceLimits limits{
        {32, 16, uint64_t{1} << 24, uint64_t{1} << 34, uint64_t{1} << 39}, {128, {32, 16}}, 32, {}};
    service = std::allocate_shared<NumericalServiceJob>(
        RunAllocator<NumericalServiceJob>(resource), resource,
        NativeTaskKey{{OwnerKind::run, 0, 151}, 0, 0}, bank, limits, scheduler.stop_token());
    if (const auto initial = forest.pending_evaluations())
      evaluate(*initial);
  }
  ~Fixture() { service->close(); }
  void evaluate(const BoundaryEvaluationBatch &batch) {
    std::pmr::vector<BoundaryEvaluationResult> answers(&budget);
    for (const auto &task : batch.tasks()) {
      const auto t = import_exact(budget, batch.parameters().view(task.parameter));
      const auto point = canonical_original_curve(context, geometry, task.curve, t);
      answers.push_back({task, point.dimension, point.xyz});
    }
    static_cast<void>(forest.complete_evaluations(batch.epoch(), answers));
  }
  NumericalOutput calculate(NumericalInput input) {
    const NativeTaskKey key{{OwnerKind::run, 0, 152}, 0, ++serial};
    service->begin({key, std::move(input)});
    for (;;) {
      scheduler.submit(service);
      scheduler.drain();
      auto completion = scheduler.take();
      if (!completion)
        throw std::runtime_error("actual numerical service completion absent");
      completion->rethrow();
      if (completion->state == StepState::done)
        break;
      require(completion->state == StepState::ready, "numerical service lost its continuation");
    }
    require(service->result().key == key, "actual numerical reply has foreign identity");
    auto result = service->result().output;
    service->release_result();
    return result;
  }
  FaceBoundarySnapshot prepare(uint64_t generation) {
    FaceBoundaryPreparation preparation(
        budget, context, geometry, forest.freeze_face_boundary({0}),
        {.face = {0},
         .generation = generation,
         .positional_allowance = Rational(budget, 1),
         .node_source_allowance = Rational(budget, 1).divide(Rational(budget, 1000000)),
         .allow_plane_profile = false});
    while (preparation.state() == FacePreparationState::curve_work) {
      const auto work = preparation.take_curve_work(bank, {32, 16});
      if (!work)
        throw std::runtime_error("actual authoritative chord work absent");
      preparation.complete(std::get<BoundCompleted>(calculate(*work)));
    }
    require(preparation.state() == FacePreparationState::ready,
            "actual authoritative source preparation did not settle");
    const auto seed = preparation.snapshot();
    require(seed->domain_map && seed->domain_map->strips.size() == 1,
            "source-backed authoritative strip was not retained");
    return seed;
  }
  FrozenFaceMetric metric(const FaceBoundarySnapshot &seed, const Rational &tau) {
    const auto envelope = prepare_definition_envelope(
        context, geometry, {.definition = {0}, .generation = seed->face_generation});
    return prepare_face_metric(context, seed, {source, {0}, envelope.xyz}, tau);
  }
  void split_verticals(bool lower_half) {
    const auto left = source.view().edge_index.find(EdgeId{403});
    const auto right = source.view().edge_index.find(EdgeId{401});
    const auto a = forest.intervals(left), b = forest.intervals(right);
    const std::array requests{lower_half ? a.back() : a.front(), b.front()};
    const auto epoch = forest.begin_epoch();
    forest.arrive(epoch, {0}, forest.face_generation({0}), requests);
    evaluate(forest.prepare_splits(epoch));
  }
};
template <std::size_t N, std::size_t... I>
std::array<Rational, N> read(bgfd::Budget &budget, const ExactTable &table,
                             const std::array<ExactRef, N> &refs, std::index_sequence<I...>) {
  return {import_exact(budget, table.view(refs[I]))...};
}
void run(const std::filesystem::path &root) {
  Fixture fixture(root / "authoritative-curved-trim.bgsource");
  const auto seed = fixture.prepare(11);
  const Rational tau(fixture.budget, 1);
  const auto metric = fixture.metric(seed, tau);
  const auto factor =
      read(fixture.budget, metric.values, metric.factor, std::make_index_sequence<4>{});
  const auto scales =
      read(fixture.budget, metric.values, metric.scales, std::make_index_sequence<4>{});
  FaceEngine engine(fixture.budget, fixture.context, seed,
                    {factor, {scales[0], scales[1], scales[2], scales[3]}}, tau, 17);
  const auto work = engine.take_cell_work(32);
  if (!work)
    throw std::runtime_error("actual domain-adjusted packet absent");
  for (const auto &cell : (*work)->cells) {
    const auto xy =
        read(fixture.budget, (*work)->values, cell.chart_xy, std::make_index_sequence<6>{});
    const auto domain = bound_domain_displacement(fixture.context, seed->domain_map, xy);
    const auto remaining = import_exact(fixture.budget, (*work)->values.view(cell.allowance));
    require(remaining.compare(tau.subtract(domain.upper)) == 0 &&
                domain.upper.compare(tau.divide(Rational(fixture.budget, 32))) == 0,
            "same cell independently granted full tolerance to domain and interpolant");
  }
  const auto numerical = std::get<std::shared_ptr<const CellBatchResult>>(fixture.calculate(*work));
  for (const auto &cell : numerical->cells)
    require(cell.decision == BoundDecision::accept && engine.complete(cell),
            "actual domain-adjusted cells did not pass common numerical runtime");
  require(engine.advance().kind == FaceAdvanceKind::settled,
          "actual authoritative domain-adjusted face failed to settle");

  // Reuse the actual geometric construction under a stricter refinement
  // allowance. This generation cannot publish: its domain remainder is empty.
  const auto tight = tau.divide(Rational(fixture.budget, 64));
  const auto tight_seed = fixture.prepare(12);
  const auto tight_metric = fixture.metric(tight_seed, tight);
  const auto tight_factor =
      read(fixture.budget, tight_metric.values, tight_metric.factor, std::make_index_sequence<4>{});
  const auto tight_scales =
      read(fixture.budget, tight_metric.values, tight_metric.scales, std::make_index_sequence<4>{});
  FaceEngine exhausted(
      fixture.budget, fixture.context, tight_seed,
      {tight_factor, {tight_scales[0], tight_scales[1], tight_scales[2], tight_scales[3]}}, tight,
      17);
  require(!exhausted.take_cell_work(32), "host-only domain failure dispatched numerical work");
  const auto request = exhausted.advance();
  require(request.kind == FaceAdvanceKind::boundary_request && request.boundary &&
              request.boundary->owner == 400 && exhausted.stats().domain_failures == 2,
          "host-only domain decisions did not reach canonical boundary horizon");

  fixture.split_verticals(false);
  fixture.split_verticals(true);
  const auto queued_seed = fixture.prepare(13);
  const auto queued_metric = fixture.metric(queued_seed, tight);
  const auto queued_factor = read(fixture.budget, queued_metric.values, queued_metric.factor,
                                  std::make_index_sequence<4>{});
  const auto queued_scales = read(fixture.budget, queued_metric.values, queued_metric.scales,
                                  std::make_index_sequence<4>{});
  FaceEngine queued(
      fixture.budget, fixture.context, queued_seed,
      {queued_factor, {queued_scales[0], queued_scales[1], queued_scales[2], queued_scales[3]}},
      tight, 17);
  const auto pending = queued.take_cell_work(32);
  require(pending && queued.stats().domain_failures > 0,
          "source subdivision did not produce mixed host/numerical cells");
  require(queued.advance().kind == FaceAdvanceKind::pending,
          "lower-priority host domain failure bypassed higher pending cell");
  std::cout << "PASS source-backed domain allowance and canonical host frontier\n";
}
} // namespace
int main(int argc, char **argv) {
  try {
    require(argc == 2, "expected actual source fixture directory");
    run(argv[1]);
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
  } catch (const bgfd::Failure &error) {
    std::cerr << error.what() << '\n';
  }
  return 1;
}
