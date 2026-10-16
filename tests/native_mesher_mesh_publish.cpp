#include "../native/mesher/face_engine.hpp"
#include "../native/mesher/face_preparation.hpp"
#include "../native/mesher/geometry_candidate.hpp"
#include "../native/mesher/geometry_domain.hpp"
#include "../native/mesher/mesh_io.hpp"
#include "../native/mesher/mesh_publish.hpp"
#include "../native/mesher/numerical_service.hpp"
#include "../native/mesher/plane_cell_bounds.hpp"
#include <iostream>
#include <stdexcept>
namespace {
using namespace bgmesh;
void require(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}
template <std::size_t N, std::size_t... I>
std::array<Rational, N> read(bgfd::Budget &b, const ExactTable &table,
                             const std::array<ExactRef, N> &refs, std::index_sequence<I...>) {
  return {import_exact(b, table.view(refs[I]))...};
}
void authoritative(const std::filesystem::path &fixtures, const std::filesystem::path &out,
                   bool mixed) {
  const auto resource = std::make_shared<RunResource>(uint64_t{1} << 29);
  std::stop_source stop;
  JobContext job({uint64_t{1} << 27, uint64_t{1} << 39}, resource.get(), stop.get_token());
  RunPreparationJob preparation(
      resource,
      {.source = fixtures / (mixed ? "mixed-curved-trim.bgsource" : "authoritative-joint.bgsource"),
       .positional = 1.,
       .node_source = 1e-6});
  require(preparation.step(job) == StepState::done, "authoritative source preparation incomplete");
  const auto prepared = preparation.result();
  GeometryContext context(job.budget);
  DefinitionBoundaryForest forest(job.budget, prepared->geometry.view().source, {0}, resource);
  require(!forest.pending_evaluations(), "authoritative literal endpoints missing");
  const auto &allowance = prepared->assembly.view().definitions[0];
  const auto tau =
      import_exact(job.budget, prepared->assembly.view().values.view(allowance.positional));
  const auto node =
      import_exact(job.budget, prepared->assembly.view().values.view(allowance.node_source));
  FaceBoundaryPreparation face(
      job.budget, context, prepared->geometry, forest.freeze_face_boundary({0}),
      {.face = {0}, .generation = 31, .positional_allowance = tau, .node_source_allowance = node});
  NumericalServiceJob numerical(
      resource, {{OwnerKind::run, 0, 1005}, 1, 0}, prepared->bank,
      {{8, 8, uint64_t{1} << 24, uint64_t{1} << 35, uint64_t{1} << 39}, {64, {8, 8}}, 8, {}},
      stop.get_token());
  uint64_t ordinal = 0;
  const auto execute = [&](NumericalInput input) {
    numerical.begin({{{OwnerKind::run, 0, 1005}, 1, ++ordinal}, std::move(input)});
    while (numerical.step(job) != StepState::done) {
    }
  };
  while (const auto work = face.take_curve_work(prepared->bank, {8, 8})) {
    execute(*work);
    face.complete(std::get<BoundCompleted>(numerical.result().output));
    numerical.release_result();
  }
  require(face.state() == FacePreparationState::ready,
          "actual authoritative curve work incomplete");
  const auto seed = face.snapshot();
  require(seed->domain_map && seed->domain_map->strips.size() == (mixed ? 1U : 0U),
          "actual B/T map absent");
  const auto envelope = prepare_definition_envelope(context, prepared->geometry,
                                                    {.definition = {0}, .generation = 31});
  const auto metric = prepare_face_metric(
      context, seed, {prepared->geometry.view().source, {0}, envelope.xyz}, tau);
  const auto factors =
      read(job.budget, metric.values, metric.factor, std::make_index_sequence<4>{});
  const auto scales = read(job.budget, metric.values, metric.scales, std::make_index_sequence<4>{});
  FaceEngine engine(job.budget, context, seed,
                    {factors, {scales[0], scales[1], scales[2], scales[3]}}, tau, 17);
  while (const auto work = engine.take_cell_work(8)) {
    execute(*work);
    for (const auto &result :
         std::get<std::shared_ptr<const CellBatchResult>>(numerical.result().output)->cells)
      require(result.decision == BoundDecision::accept && engine.complete(result),
              "actual authoritative cell bound failed");
    numerical.release_result();
  }
  require(engine.advance().kind == FaceAdvanceKind::settled, "authoritative face not settled");
  const std::array faces{engine.freeze_result()};
  const std::array boundaries{forest.freeze_definition_boundary()};
  const auto mesh = publish_native_mesh(job.budget, prepared, boundaries, faces, {1., 1e-6, 1});
  const auto &d = mesh.view();
  require(d.domain_maps.size() == 1 && d.supports.size() == (mixed ? 1U : 0U) &&
              d.domain_maps[0].policy == (mixed ? MeshReferencePolicy::hybrid_pullback
                                                : MeshReferencePolicy::authoritative_original),
          "actual domain policy/support descriptors lost");
  for (const auto &profile : d.profiles)
    require(profile.components.count == 1, "actual canonical source/reference trace bound lost");
  for (const auto &sample : d.use_samples) {
    const auto &use = d.uses[static_cast<std::size_t>(sample.use.value - 1)];
    require(sample.raw.unavailable_reason == (use.reconstructed ? 1U : 2U),
            "raw guide source meaning changed");
  }
  std::filesystem::create_directories(out);
  const auto path = out / (mixed ? "mixed.bgnmesh" : "authoritative.bgnmesh");
  write_native_mesh(path, mesh);
  const auto restored = load_native_mesh(path, resource);
  require(restored.view().domain_maps.size() == 1 &&
              restored.view().supports.size() == d.supports.size(),
          "authoritative map wire readback lost semantics");
  numerical.close();
}
void run(const std::filesystem::path &fixtures, const std::filesystem::path &out) {
  auto resource = std::make_shared<RunResource>(uint64_t{1} << 29);
  {
    std::stop_source stop;
    JobContext job({uint64_t{1} << 28, uint64_t{1} << 38}, resource.get(), stop.get_token());
    RunPreparationJob preparation(
        resource,
        {.source = fixtures / "warped-plane.bgsource", .positional = 1e-5, .node_source = 1e-5});
    require(preparation.step(job) == StepState::done, "whole source preparation incomplete");
    const auto prepared = preparation.result();
    const auto source = prepared->geometry.view().source;
    GeometryContext context(job.budget);
    DefinitionBoundaryForest forest(job.budget, source, {0}, resource);
    if (const auto pending = forest.pending_evaluations()) {
      std::pmr::vector<BoundaryEvaluationResult> answers(&job.budget);
      for (const auto &task : pending->tasks()) {
        const auto t = import_exact(job.budget, pending->parameters().view(task.parameter));
        const auto point = canonical_original_curve(context, prepared->geometry, task.curve, t);
        answers.push_back({task, point.dimension, point.xyz});
      }
      static_cast<void>(forest.complete_evaluations(pending->epoch(), answers));
    }
    const auto &allowance = prepared->assembly.view().definitions[0];
    const auto tau =
        import_exact(job.budget, prepared->assembly.view().values.view(allowance.positional));
    const auto node =
        import_exact(job.budget, prepared->assembly.view().values.view(allowance.node_source));
    FaceBoundaryPreparation face(job.budget, context, prepared->geometry,
                                 forest.freeze_face_boundary({0}),
                                 {.face = {0},
                                  .generation = 11,
                                  .positional_allowance = tau,
                                  .node_source_allowance = node});
    require(face.state() == FacePreparationState::ready, "actual plane preparation did not settle");
    const auto seed = face.snapshot();
    const auto envelope = prepare_definition_envelope(context, prepared->geometry,
                                                      {.definition = {0}, .generation = 11});
    const auto metric = prepare_face_metric(context, seed, {source, {0}, envelope.xyz}, tau);
    const auto factors =
        read(job.budget, metric.values, metric.factor, std::make_index_sequence<4>{});
    const auto scales =
        read(job.budget, metric.values, metric.scales, std::make_index_sequence<4>{});
    FaceEngine engine(job.budget, context, seed,
                      {factors, {scales[0], scales[1], scales[2], scales[3]}}, tau, 17);
    const auto &frozen_plane = seed->plane;
    if (!frozen_plane.has_value())
      throw std::runtime_error("plane source profile missing");
    const auto &plane = frozen_plane.value();
    const auto origin = read(job.budget, seed->values, plane.origin, std::make_index_sequence<3>{});
    const auto basis = read(job.budget, seed->values, plane.basis, std::make_index_sequence<6>{});
    const auto quad =
        read(job.budget, seed->values, plane.quadrilateral, std::make_index_sequence<8>{});
    const auto height = import_exact(job.budget, seed->values.view(plane.height_upper));
    PlaneCellProfile profile(job.budget, {origin, basis, quad, height});
    const auto prove = [&](FaceEngine &target) {
      const auto cells = target.take_cell_work(8);
      if (!cells)
        throw std::runtime_error("actual cells absent");
      for (const auto &cell : (*cells)->cells) {
        const auto xy =
            read(job.budget, (*cells)->values, cell.chart_xy, std::make_index_sequence<6>{});
        const auto xyz =
            read(job.budget, (*cells)->values, cell.actual_xyz, std::make_index_sequence<9>{});
        const auto result = bound_plane_cell(profile, {cell.header, cell.map_kind, xy, xyz, tau});
        require(result.decision == BoundDecision::accept && target.complete(result),
                "actual plane proof failed");
      }
    };
    prove(engine);
    require(engine.advance().kind == FaceAdvanceKind::settled, "actual face not settled");
    const std::array results{engine.freeze_result()};
    const std::array boundaries{forest.freeze_definition_boundary()};
    const auto mesh =
        publish_native_mesh(job.budget, prepared, boundaries, results, {1e-5, 1e-5, 1});
    const auto &d = mesh.view();
    require(d.faces.size() == 1 && d.nodes.size() == 4 && d.facets.size() == 2 &&
                d.cells.size() == 2 && d.edges.size() == 4 && d.uses.size() == 4 &&
                d.loops.size() == 1,
            "actual source/result inventory was not fully published");
    require(std::holds_alternative<PhysicalPlaneChart>(d.charts[0].chart) &&
                std::holds_alternative<PlaneTriangleMap>(d.cell_maps[0].map),
            "XY source map lost");
    for (const auto &n : d.nodes) {
      require((n.xyz[0] == 0 || n.xyz[0] == 1) && (n.xyz[1] == 0 || n.xyz[1] == 1) && n.xyz[2] == 0,
              "actual stored unit-square physical words changed");
    }
    require(d.joints.size() == 4 && d.closures.size() == 4 && d.instances.size() == 1 &&
                d.components.size() >= 2,
            "source seams/quality/assembly lost");
    std::filesystem::create_directories(out);
    write_native_mesh(out / "actual.bgnmesh", mesh);
    const auto decoded = load_native_mesh(out / "actual.bgnmesh", resource);
    require(decoded.view().facets.size() == 2 && decoded.view().nodes.size() == 4,
            "actual wire readback failed");
    bool refused = false;
    try {
      static_cast<void>(publish_native_mesh(job.budget, prepared, boundaries, {}, {1e-5, 1e-5, 1}));
    } catch (const Error &e) {
      refused = e.code == ErrorCode::invalid_source;
    }
    require(refused, "incomplete original face inventory published");
    refused = false;
    try {
      static_cast<void>(
          publish_native_mesh(job.budget, prepared, boundaries, results, {1e-12, 1e-5, 1}));
    } catch (const Error &e) {
      refused = e.code == ErrorCode::invalid_source;
    }
    require(refused, "publisher relabeled prepared accuracy as a tighter requested accuracy");
    const auto hostile = [&](const FaceResult &bad) {
      const auto before = resource->live();
      bool failed = false;
      try {
        const std::array input{bad};
        static_cast<void>(
            publish_native_mesh(job.budget, prepared, boundaries, input, {1e-5, 1e-5, 1}));
      } catch (const Error &e) {
        failed = e.code == ErrorCode::invalid_source;
      }
      require(failed && resource->live() == before,
              "invalid publisher input mutated or retained output");
    };
    const auto copy = std::allocate_shared<FaceResultData>(RunAllocator<FaceResultData>(resource),
                                                           seed, results[0]->values);
    copy->sites = results[0]->sites;
    copy->lineages = results[0]->lineages;
    copy->triangles = results[0]->triangles;
    copy->traversal_adapters = results[0]->traversal_adapters;
    ++copy->triangles[0].cell.face.face_generation;
    hostile(copy);
    copy->triangles = results[0]->triangles;
    copy->sites[0].node.xyz[0] = .125;
    hostile(copy);
    bgfd::Budget exhausted({uint64_t{1} << 20, 0}, resource.get());
    refused = false;
    try {
      static_cast<void>(
          publish_native_mesh(exhausted, prepared, boundaries, results, {1e-5, 1e-5, 1}));
    } catch (const Error &e) {
      refused = e.code == ErrorCode::resource_exhausted;
    }
    require(refused, "publisher bypassed caller work limit");
    // A separate face generation deliberately receives a contact-refinement
    // stimulus to exercise real candidate/lineage publication. This does not
    // claim that the clear initial pair has a forbidden physical contact.
    FaceBoundaryPreparation second(job.budget, context, prepared->geometry,
                                   forest.freeze_face_boundary({0}),
                                   {.face = {0},
                                    .generation = 12,
                                    .positional_allowance = tau,
                                    .node_source_allowance = node});
    require(second.state() == FacePreparationState::ready, "second plane generation not prepared");
    FaceEngine refined(job.budget, context, second.snapshot(),
                       {factors, {scales[0], scales[1], scales[2], scales[3]}}, tau, 17);
    prove(refined);
    require(refined.advance().kind == FaceAdvanceKind::settled, "second plane not settled");
    const auto accepted = refined.take_settled();
    require(!accepted.empty() && refined.mark_contact_failure(accepted[0]),
            "contact stimulus absent");
    const auto candidate = refined.advance();
    if (!candidate.candidate)
      throw std::runtime_error("real plane candidate absent");
    const auto evaluated = evaluate_geometry_candidate(context, candidate.candidate);
    require(refined.complete_candidate(evaluated), "real generated point did not commit");
    prove(refined);
    require(refined.advance().kind == FaceAdvanceKind::settled, "generated cells not settled");
    const std::array generated{refined.freeze_result()};
    const auto richer =
        publish_native_mesh(job.budget, prepared, boundaries, generated, {1e-5, 1e-5, 1});
    require(richer.view().nodes.size() == 5 && richer.view().facets.size() == 4 &&
                richer.view().lineage.size() > richer.view().cells.size(),
            "generated physical node/ancestry lost");
    std::size_t created = 0;
    for (const auto &n : richer.view().nodes)
      if (std::holds_alternative<FaceSiteGenerator>(n.generator)) {
        ++created;
        require(n.xyz == Vec3{.5, .5, 0}, "canonical generated physical words changed");
      }
    require(created == 1, "generated node source kind changed");
    // Install a real source-edge midpoint through the sole shared owner, then
    // reject old face output against that newer complete master schedule.
    const auto epoch = forest.begin_epoch();
    const auto intervals = forest.intervals({0});
    require(!intervals.empty(), "source interval absent");
    const std::array request{intervals[0]};
    forest.arrive(epoch, {0}, forest.face_generation({0}), request);
    const auto update = forest.prepare_splits(epoch);
    std::pmr::vector<BoundaryEvaluationResult> evaluated_edges(&job.budget);
    for (const auto &task : update.tasks()) {
      const auto t = import_exact(job.budget, update.parameters().view(task.parameter));
      const auto point = canonical_original_curve(context, prepared->geometry, task.curve, t);
      evaluated_edges.push_back({task, point.dimension, point.xyz});
    }
    static_cast<void>(forest.complete_evaluations(epoch, evaluated_edges));
    const std::array newer_boundary{forest.freeze_definition_boundary()};
    refused = false;
    try {
      static_cast<void>(
          publish_native_mesh(job.budget, prepared, newer_boundary, results, {1e-5, 1e-5, 1}));
    } catch (const Error &e) {
      refused = e.code == ErrorCode::invalid_source;
    }
    require(refused, "publisher accepted a face omitting a newer shared source split");
    FaceBoundaryPreparation split_face(job.budget, context, prepared->geometry,
                                       forest.freeze_face_boundary({0}),
                                       {.face = {0},
                                        .generation = 13,
                                        .positional_allowance = tau,
                                        .node_source_allowance = node});
    require(split_face.state() == FacePreparationState::ready, "split source face not prepared");
    FaceEngine split_engine(job.budget, context, split_face.snapshot(),
                            {factors, {scales[0], scales[1], scales[2], scales[3]}}, tau, 17);
    prove(split_engine);
    require(split_engine.advance().kind == FaceAdvanceKind::settled, "split source cells pending");
    const std::array split_result{split_engine.freeze_result()};
    const auto split_mesh =
        publish_native_mesh(job.budget, prepared, newer_boundary, split_result, {1e-5, 1e-5, 1});
    require(split_mesh.view().nodes.size() == 5 && split_mesh.view().facets.size() == 3,
            "settled shared source midpoint changed output inventory");
    std::size_t edge_nodes = 0;
    for (const auto &n : split_mesh.view().nodes)
      if (std::holds_alternative<OriginalEdgeGenerator>(n.generator)) {
        ++edge_nodes;
        require(n.xyz == Vec3{.625, 0, 0}, "canonical original-curve midpoint changed");
      }
    require(edge_nodes == 1, "original edge sample became a face-generated node");
  }
  require(resource->live() == 0, "publisher retained resource owner after outputs died");
  std::cout << "PASS native complete actual source-to-NativeMesh publisher\n";
}
} // namespace
int main(int argc, char **argv) {
  try {
    require(argc == 3, "expected chart fixtures and output directory");
    run(argv[1], argv[2]);
    authoritative(argv[1], argv[2], false);
    authoritative(argv[1], argv[2], true);
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
