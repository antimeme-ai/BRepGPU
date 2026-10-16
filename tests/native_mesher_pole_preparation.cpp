#include "../native/mesher/face_engine.hpp"
#include "../native/mesher/face_preparation.hpp"
#include "../native/mesher/geometry_domain.hpp"
#include "../native/mesher/geometry_pole_control.hpp"
#include <filesystem>
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
void run(const std::filesystem::path &path) {
  auto resource = std::make_shared<RunResource>(uint64_t{1} << 30);
  const auto source = load_source(path, resource);
  bgfd::Budget b({uint64_t{1} << 29, uint64_t{1} << 40}, resource.get());
  const auto geometry =
      compile_geometry(source, resource, {{OwnerKind::geometry, 0, 759}, 3, 0}, b);
  GeometryContext ctx(b);
  DefinitionBoundaryForest forest(b, source, {0}, resource);
  PoleCutControl control;
  const auto evaluate = [&](const BoundaryEvaluationBatch &batch) {
    std::pmr::vector<BoundaryEvaluationResult> answers(&b);
    for (const auto &task : batch.tasks()) {
      const auto t = import_exact(b, batch.parameters().view(task.parameter));
      const auto p = canonical_original_curve(ctx, geometry, task.curve, t);
      answers.push_back({task, p.dimension, p.xyz});
    }
    static_cast<void>(forest.complete_evaluations(batch.epoch(), answers));
  };
  if (const auto initial = forest.pending_evaluations())
    evaluate(*initial);
  const auto split = [&](BoundaryKey key) {
    const auto epoch = forest.begin_epoch();
    const std::array keys{key};
    forest.arrive(epoch, {0}, forest.face_generation({0}), keys);
    evaluate(forest.prepare_splits(epoch));
  };
  const auto tau = Rational(b, 1), zero = Rational(b), one = Rational(b, 1);
  uint64_t generation = 10;
  const auto prepare = [&]() {
    std::cerr << "PREPARE\t" << path.filename().string() << "\t" << generation + 1 << std::endl;
    FaceBoundaryPreparation prep(b, ctx, geometry, forest.freeze_face_boundary({0}),
                                 {.face = {0},
                                  .generation = ++generation,
                                  .positional_allowance = tau,
                                  .node_source_allowance = one.divide(Rational(b, 1000000)),
                                  .allow_plane_profile = false},
                                 control);
    const auto roots = prep.curve_roots();
    const auto bank =
        prepare_bound_roots(b, resource, geometry, roots,
                            {{OwnerKind::geometry, 0, 759}, 3, static_cast<uint32_t>(generation)});
    BoundRuntime runtime(resource, bank,
                         {64, 16, uint64_t{1} << 26, uint64_t{1} << 36, uint64_t{1} << 38});
    while (const auto work = prep.take_curve_work(bank, {64, 16}))
      prep.complete(runtime.resolve(runtime.filter_cpu(*work)));
    runtime.close();
    if (prep.state() == FacePreparationState::boundary_request) {
      const auto request = prep.boundary_request();
      require(request.has_value(), "cap producer lost actual source feature request");
      control = prep.next_pole_control();
      split(*request);
      return FaceBoundarySnapshot{};
    }
    return prep.snapshot();
  };
  FaceBoundarySnapshot seed;
  uint32_t source_rounds = 0;
  for (; source_rounds < 8 && !seed; ++source_rounds)
    seed = prepare();
  require(seed && source_rounds >= 3,
          "producer must obtain BOTH strict-interior master cuts before face publication");
  require(seed->pole_caps.size() == 1 && !seed->pole_pieces.empty() &&
              seed->construction_domain_present && seed->construction_loops.size() == 1,
          "same producer failed complete finite cap plus ordinary remainder transport");
  const auto &cap = seed->pole_caps.front();
  require(cap.fixed_pole_upper && cap.fixed_band_upper,
          "actual producer omitted canonical fixed source terms");
  require(seed->uses.size() == 4 && seed->source_boundary &&
              seed->trace_maps.size() == seed->segments.size(),
          "pole preparation discarded original occurrence/source trace inventory");
  const bool auth = !source.view().coedges[0].reconstructed;
  require(auth ? seed->domain_map && seed->domain_map->fans.empty() &&
                     seed->domain_map->strips.empty()
               : !seed->domain_map,
          "authoritative identity cap must retain actual named source map");
  const std::array<Rational, 4> factor{one, zero, zero, one};
  FaceEngine engine(b, ctx, seed, {factor, {Rational(b, 8), one, one, tau}}, tau, 37);
  while (const auto work = engine.take_cell_work(64)) {
    const auto &batch = work.value();
    for (const auto &cell : batch->cells) {
      const auto uv = read(b, batch->values, cell.original_uv, std::make_index_sequence<6>{});
      const auto xyz = read(b, batch->values, cell.actual_xyz, std::make_index_sequence<9>{});
      const auto allowance = import_exact(b, batch->values.view(cell.allowance));
      const auto answer = bound_analytic_cell(
          ctx, geometry, {cell.header, cell.carrier, cell.map_kind, uv, xyz, allowance});
      require(answer.decision == BoundDecision::accept && engine.complete(answer),
              "actual original analytic remainder failed common whole-cell bound");
    }
  }
  require(engine.advance().kind == FaceAdvanceKind::settled &&
              engine.stats().host_pole_cells == seed->pole_pieces.size(),
          "same engine did not settle actual ordinary+full singular coverage maps");
  const auto result = engine.freeze_result();
  require(result->source_sites.size() == seed->sites.size() && !result->triangles.empty(),
          "complete source-to-active mapping missing from full result");
  // An angular-only edit changes the internal dyadic chain, not forest nodes.
  control = continue_pole_cut(b, seed, seed->pole_pieces.front().cut);
  require(control.angular_level == 1 && control.revision == 1,
          "internal face-cut continuation did not retain angular history");
  const auto angular = prepare();
  require(angular && angular->pole_pieces.size() == 2 * seed->pole_pieces.size() &&
              angular->boundary_generation == seed->boundary_generation,
          "face-owned angular edit changed source boundary or lost shared cut chain");
  // Radial continuation retains BOTH old parameter gaps across one-sided epochs.
  control = continue_pole_cut(b, angular, angular->pole_caps.front().radial_features[0]);
  split(angular->pole_caps.front().radial_features[0]);
  const auto partial = prepare();
  require(!partial, "one contracted endpoint cannot publish a replacement two-sided cut");
  const auto radial = prepare();
  require(radial && radial->pole_control.history == nullptr,
          "both endpoint contraction did not complete preserved cut history");
  std::cout << "PASS\t" << path.filename().string() << "\t" << seed->pole_pieces.size() << "\t"
            << result->triangles.size() << '\n';
}
} // namespace
int main(int argc, char **argv) {
  try {
    std::cout.setf(std::ios::unitbuf);
    if (argc != 2 && argc != 3)
      throw std::runtime_error("pole source fixture directory required");
    if (argc == 3) {
      run(std::filesystem::path(argv[1]) / argv[2]);
      return 0;
    }
    for (const auto *name :
         {"north", "south", "cone", "north-distinct", "south-distinct", "cone-distinct",
          "north-authoritative", "south-authoritative", "cone-authoritative", "north-reversed",
          "south-reversed", "cone-reversed"})
      run(std::filesystem::path(argv[1]) / (std::string(name) + ".bgsource"));
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "FAIL\t" << e.what() << '\n';
    return 1;
  } catch (const bgfd::Failure &e) {
    std::cerr << "FAIL\t" << e.what() << '\n';
    return 1;
  }
}
