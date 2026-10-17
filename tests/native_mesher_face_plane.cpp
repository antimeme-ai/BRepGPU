#include "../native/mesher/assembly.hpp"
#include "../native/mesher/face_engine.hpp"
#include "../native/mesher/face_preparation.hpp"
#include "../native/mesher/geometry_candidate.hpp"
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
void complete_cells(FaceEngine &engine, const FaceCellWork &work,
                    const std::shared_ptr<NumericalServiceJob> &service, NativeScheduler &scheduler,
                    NativeTaskKey child) {
  service->begin({child, work});
  scheduler.submit(service);
  while (true) {
    const auto completion = scheduler.take();
    if (!completion)
      continue;
    completion->rethrow();
    if (completion->state == StepState::done)
      break;
    require(completion->state == StepState::ready, "plane numerical child blocked unexpectedly");
    scheduler.submit(service);
  }
  require(service->result().key == child, "plane numerical service lost request identity");
  const auto results = std::get<std::shared_ptr<const CellBatchResult>>(service->result().output);
  require(results->cells.size() == work->cells.size(), "plane numerical service dropped cells");
  for (const auto &result : results->cells) {
    require(result.decision == BoundDecision::accept && engine.complete(result),
            "nonlinear flat actual cells did not pass same physical plane bound");
  }
  service->release_result();
}
void run(const std::filesystem::path &root) {
  auto resource = std::make_shared<RunResource>(uint64_t{1} << 29);
  const auto source = load_source(root / "warped-plane.bgsource", resource);
  bgfd::Budget budget({uint64_t{1} << 28, uint64_t{1} << 38}, resource.get());
  const auto geometry =
      compile_geometry(source, resource, {{OwnerKind::geometry, 0, 7}, 3, 0}, budget);
  GeometryContext context(budget);
  DefinitionBoundaryForest forest(budget, source, {0}, resource);
  if (const auto pending = forest.pending_evaluations()) {
    std::pmr::vector<BoundaryEvaluationResult> answers(&budget);
    for (const auto &task : pending->tasks()) {
      const auto t = import_exact(budget, pending->parameters().view(task.parameter));
      const auto p = canonical_original_curve(context, geometry, task.curve, t);
      answers.push_back({task, p.dimension, p.xyz});
    }
    (void)forest.complete_evaluations(pending->epoch(), answers);
  }
  const auto tau = Rational(budget, 1).divide(Rational(budget, 100000));
  FaceBoundaryPreparation preparation(budget, context, geometry, forest.freeze_face_boundary({0}),
                                      {.face = {0},
                                       .generation = 11,
                                       .positional_allowance = tau,
                                       .node_source_allowance = tau,
                                       .allow_plane_profile = true});
  require(preparation.state() == FacePreparationState::ready,
          "geometric straight boundary did not settle without unrelated chord work");
  const auto seed = preparation.snapshot();
  require(seed->chart_kind == WorkingChartKind::physical_plane_convex_q && seed->plane,
          "nonlinear flat source did not retain actual admitted PlaneXY chart");
  const auto envelope =
      prepare_definition_envelope(context, geometry, {.definition = {0}, .generation = 11});
  const auto metric = prepare_face_metric(context, seed, {source, {0}, envelope.xyz}, tau);
  const std::array factor{import_exact(budget, metric.values.view(metric.factor[0])),
                          import_exact(budget, metric.values.view(metric.factor[1])),
                          import_exact(budget, metric.values.view(metric.factor[2])),
                          import_exact(budget, metric.values.view(metric.factor[3]))};
  const std::array scales{import_exact(budget, metric.values.view(metric.scales[0])),
                          import_exact(budget, metric.values.view(metric.scales[1])),
                          import_exact(budget, metric.values.view(metric.scales[2])),
                          import_exact(budget, metric.values.view(metric.scales[3]))};
  FaceEngine engine(budget, context, seed, {factor, {scales[0], scales[1], scales[2], scales[3]}},
                    tau, 17);
  require(engine.topology().triangles().size() == 2,
          "plane constructor changed exact boundary triangulation");
  const auto pending = engine.take_cell_work(8);
  if (!pending)
    throw std::runtime_error("plane packet absent");
  const auto &cells = *pending;
  require(cells->chart_source == seed && cells->cells.size() == 2,
          "plane packet lost admitted source profile or bounded actual cells");
  for (const auto &cell : cells->cells)
    require(cell.map_kind == CellMapKind::plane_xy_triangle &&
                cell.chart_xy[0].table == cells->values.key(),
            "plane XY was mislabeled original UV");
  if (!seed->plane)
    throw std::runtime_error("admitted plane profile absent");
  const std::array<BoundRootSelection, 1> roots{SurfaceRootSlot{0}};
  const auto bank =
      prepare_bound_roots(budget, resource, geometry, roots, {{OwnerKind::geometry, 0, 93}, 3, 0});
  NativeScheduler scheduler(resource, {1, 4, uint64_t{1} << 26, uint64_t{1} << 36});
  const NumericalServiceLimits limits{
      {8, 8, uint64_t{1} << 24, uint64_t{1} << 34, uint64_t{1} << 38}, {64, {8, 8}}, 8, {}};
  auto service = std::allocate_shared<NumericalServiceJob>(
      RunAllocator<NumericalServiceJob>(resource), resource,
      NativeTaskKey{{OwnerKind::run, 0, 93}, 0, 0}, bank, limits, scheduler.stop_token());
  const OwnerKey face_owner{OwnerKind::face, 0, source.view().faces[0].id.value};
  complete_cells(engine, cells, service, scheduler, {face_owner, 11, 1});
  require(engine.advance().kind == FaceAdvanceKind::settled &&
              engine.topology().triangles().size() == 2,
          "tight nonlinear planar source was refined for parameter interpolation error");
  // An authored contact mark stimulates the plane transaction; it is not a
  // claim that the two clear initial facets have an actual forbidden contact.
  const auto accepted = engine.take_settled();
  require(accepted.size() == 2 && engine.mark_contact_failure(accepted.front()),
          "accepted plane cells missing from the same frontier");
  const auto next = engine.advance();
  require(next.kind == FaceAdvanceKind::candidate && next.candidate,
          "plane refinement did not stage an owned chart candidate");
  const auto result = evaluate_geometry_candidate(context, next.candidate);
  const auto &reserved_key = next.candidate->result_table_key;
  require(result.decision == FaceCandidateDecision::admitted && result.locator && reserved_key &&
              result.locator->values.key() == *reserved_key,
          "plane source inverse did not use the reserved child table");
  ExactBuilder foreign_values(resource, {{OwnerKind::run, 0, 92}, 0, 0});
  const auto zero = foreign_values.append(Rational(budget)),
             one = foreign_values.append(Rational(budget, 1));
  auto foreign = result;
  foreign.locator = CandidateLocator{foreign_values.seal(), {zero, zero}, {zero, one, zero, one}};
  bool rejected = false;
  try {
    (void)engine.complete_candidate(foreign);
  } catch (const Error &error) {
    rejected = error.code == ErrorCode::invalid_source;
  }
  require(rejected && engine.topology().stats().sites == 4,
          "foreign inverse reply changed plane topology");
  require(engine.complete_candidate(result), "current same-source plane candidate did not commit");
  const auto children = engine.take_cell_work(8);
  if (!children)
    throw std::runtime_error("plane children absent");
  complete_cells(engine, *children, service, scheduler, {face_owner, 11, 2});
  require(engine.advance().kind == FaceAdvanceKind::settled,
          "proved plane children remained pending");
  const auto frozen = engine.freeze_result();
  const auto &center = frozen->sites.back();
  for (const auto &site : frozen->sites)
    require(site.original_uv_enclosure.has_value(),
            "plane boundary/candidate source enclosure was lost at export");
  const auto x = import_exact(budget, frozen->values.view(center.chart[0])),
             u = import_exact(budget, frozen->values.view(center.original_uv[0]));
  require(center.node.xyz == Vec3{.5, .5, 0} &&
              x.compare(Rational(budget, 1).divide(Rational(budget, 2))) == 0 &&
              u.compare(x) != 0 && center.original_uv_enclosure &&
              frozen->triangles.front().map_kind == CellMapKind::plane_xy_triangle,
          "plane output substituted chart XY for its nonlinear original-UV locator");
  require(service->stats().requests == 2 && service->stats().plane_cells == 6,
          "same retained numerical service did not handle both plane generations of cells");
  service->close();
  std::cout << "native plane face: actual physical bound2tris/inverse transaction PASS\n";
}
} // namespace
int main(int argc, char **argv) {
  try {
    require(argc == 2, "expected authored chart fixture directory");
    run(argv[1]);
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
