#include "../native/mesher/face_engine.hpp"
#include "../native/mesher/face_preparation.hpp"
#include "../native/mesher/geometry_poles.hpp"
#include "../native/mesher/mesh_io.hpp"
#include "../native/mesher/mesh_publish.hpp"
#include "../native/mesher/meshing_run.hpp"
#include "../native/mesher/numerical_service.hpp"
#include "../native/mesher/scene_view.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <dlfcn.h>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>
namespace {
using namespace bgmesh;
void require(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}
FaceBoundarySnapshot bound_backed_seed(bgfd::Budget &budget, GeometryContext &context,
                                       const GeometryStore &geometry,
                                       DefinitionBoundaryForest &forest,
                                       const std::shared_ptr<RunResource> &resource,
                                       uint64_t generation, bool private_boundary_cut = false) {
  const auto &source = geometry.view().source.view();
  const auto original = forest.freeze_face_boundary({0});
  ExactBuilder builder(resource, {{OwnerKind::face, 0, 900}, generation, 1});
  const auto ref = [&](double value) {
    return builder.append(Rational::from_double(budget, value));
  };
  const auto &carrier = source.carriers[0];
  const uint32_t quotient = source.edges[2].degenerate ? 2u : 0u;
  const bool north = quotient == 2;
  const auto kind = carrier.kind == 3 ? AnalyticPoleKind::cone_apex
                    : north           ? AnalyticPoleKind::sphere_north
                                      : AnalyticPoleKind::sphere_south;
  const auto vlo = carrier.uv_bounds[2], vhi = carrier.uv_bounds[3];
  const auto vfar = north ? vlo : vhi, vpole = north ? vhi : vlo;
  const std::array<uint32_t, 2> cuts =
      north ? std::array<uint32_t, 2>{0, 1} : std::array<uint32_t, 2>{3, 2};
  const std::array<Vec2, 5> xy{{{0, vlo}, {1, vlo}, {1, vhi}, {0, vhi}, {.5, vpole}}};
  std::pmr::vector<FaceBoundarySite> sites(resource.get());
  for (uint32_t i = 0; i < 4; ++i) {
    const auto chain = forest.chain({i});
    const auto node = forest.sample(chain.front());
    const std::array point{ref(xy[i][0]), ref(xy[i][1])};
    sites.push_back(
        {point, point, {node.key, node.xyz, ref(source.edges[i].parameter_range[0])}, {}});
  }
  const std::array proxy{ref(.5), ref(vpole)};
  sites.push_back({proxy, proxy, {sites[quotient].node.key, sites[quotient].node.xyz, {}}, {}});
  std::array<BoundaryKey, 4> source_keys{};
  for (uint32_t i = 0; i < 4; ++i) {
    const auto keys = forest.intervals({i});
    source_keys[i] = keys.empty() ? BoundaryKey{source.edges[i].id.value, 0} : keys.front();
  }
  const std::array<Rational, 4> cut{Rational(budget, 0), Rational::from_double(budget, vfar),
                                    Rational(budget, 1), Rational::from_double(budget, vfar)};
  const std::array<double, 9> actual{
      sites[4].node.xyz[0],       sites[4].node.xyz[1],       sites[4].node.xyz[2],
      sites[cuts[0]].node.xyz[0], sites[cuts[0]].node.xyz[1], sites[cuts[0]].node.xyz[2],
      sites[cuts[1]].node.xyz[0], sites[cuts[1]].node.xyz[1], sites[cuts[1]].node.xyz[2]};
  const auto bound = bound_analytic_pole_fan(
      context, carrier,
      {kind,
       cut,
       actual,
       {Rational::from_double(budget, vpole), Rational::from_double(budget, vpole)},
       1,
       {}});
  const BoundaryKey cut_key = private_boundary_cut
                                  ? BoundaryKey{900, 0, BoundaryOwnerKind::face_cut}
                                  : source_keys[north ? 0u : 2u];
  const std::array cut_refs{ref(0), ref(vfar), ref(1), ref(vfar)};
  const auto zero = ref(0), one = ref(1), position = builder.append(bound.position_upper),
             band = builder.append(bound.band_upper);
  const auto angular = builder.append(bound.angular_upper),
             radial = builder.append(bound.radial_upper),
             corner = builder.append(bound.corner_upper);
  const auto pcurve = north ? std::array{one, zero} : std::array{zero, one};
  std::pmr::vector<FaceBoundarySegment> segments(resource.get());
  std::pmr::vector<FaceUseDisposition> uses(resource.get());
  for (uint32_t i = 0; i < 4; ++i) {
    const std::array parameters{ref(source.edges[i].parameter_range[0]),
                                ref(source.edges[i].parameter_range[1])};
    segments.push_back({i, source_keys[i], {i}, parameters, {0, 0}});
    uses.push_back({{i},
                    i == quotient ? FaceUseKind::declared_quotient : FaceUseKind::ordinary_chain,
                    {i, 1},
                    parameters,
                    i == quotient ? std::optional(pcurve) : std::nullopt});
  }
  auto result = std::allocate_shared<FaceBoundarySnapshotData>(
      RunAllocator<FaceBoundarySnapshotData>(resource), resource, geometry, Slot<DefinitionTag>{0},
      Slot<FaceTag>{0},
      std::array<uint64_t, 3>{geometry.view().table.key().generation, generation,
                              original->boundary_generation},
      WorkingChartKind::analytic_unwrapped_uv, builder.seal());
  result->sites.assign(sites.begin(), sites.end());
  result->segments.assign(segments.begin(), segments.end());
  result->uses.assign(uses.begin(), uses.end());
  result->loops.push_back({{0}, true, {0, 4}, {0, 4}});
  result->source_boundary = original;
  result->prepared_original_rectangle = {sites[0].original_uv[0], sites[1].original_uv[0],
                                         sites[0].original_uv[1], sites[2].original_uv[1]};
  for (uint32_t i = 0; i < 4; ++i) {
    const auto parameters = i == quotient ? pcurve : segments[i].source_parameter;
    result->trace_maps.push_back({TraceCorrespondence::affine_parameter, parameters, {}, {}});
  }
  result->construction_domain_present = true;
  FrozenPoleCap cap{};
  cap.kind = kind;
  cap.quotient_use = {quotient};
  cap.meridian_uses = {Slot<CoedgeTag>{3}, Slot<CoedgeTag>{1}};
  cap.consumed_meridians = {Range<FaceBoundarySegment>{3, 1}, Range<FaceBoundarySegment>{1, 1}};
  cap.pcurve_parameter = pcurve;
  cap.pole_site = 4;
  cap.regular_sheet_sign = bound.regular_sheet_sign;
  cap.pieces = {0, 1};
  cap.radial_features = {source_keys[3], source_keys[1]};
  result->pole_caps.push_back(cap);
  // All refs must precede sealing. The original band is the finite source v.
  result->pole_caps.back().original_pole_v = {sites[quotient].original_uv[1],
                                              sites[(quotient + 1) % 4].original_uv[1]};
  result->pole_pieces.push_back(
      {0, cuts, cut_key, cut_refs, bound.facet_roles, position, band, angular, radial, corner});
  return result;
}
template <std::size_t N, std::size_t... I>
std::array<Rational, N> read(bgfd::Budget &budget, const ExactTable &table,
                             const std::array<ExactRef, N> &refs, std::index_sequence<I...>) {
  return {import_exact(budget, table.view(refs[I]))...};
}
struct Consumer final {
  const bgom10::Api *api;
  int32_t device;
};
void consume(const NativeScene &scene, Consumer consumer) {
  using namespace bgom10;
  const auto view = scene->view();
  bgom10::Error error{};
  Times times{};
  void *token = nullptr;
  const auto require_api = [&](bool ok, const char *what) {
    if (!ok)
      throw std::runtime_error(std::string(what) + ": " + error.stage + ": " + error.message);
  };
  require_api(consumer.api->create(&view, 2, consumer.device, &token, &error, &times),
              "pole actual consumer create");
  try {
    std::vector<Jet> jets(scene->vertices.size());
    require_api(consumer.api->evaluate(token, jets.data(), &error, &times),
                "pole actual consumer evaluate");
    for (std::size_t i = 0; i < jets.size(); ++i) {
      const auto &v = scene->vertices[i];
      const auto &c = scene->carriers[v.carrier];
      const long double u = v.uv[0], vparameter = v.uv[1], sine = std::sin(u), cosine = std::cos(u);
      std::array<long double, 3> expected{};
      if (c.tag == 1) {
        expected = {0, 0, 1};
      } else if (c.tag == 4) {
        const auto cv = std::cos(vparameter), sv = std::sin(vparameter);
        const long double sheet = (v.flags & ANALYTIC_NORMAL_LIMIT)
                                      ? ((v.flags & NEGATIVE_RADIAL_SHEET) ? -1.L : 1.L)
                                      : (cv < 0 ? -1.L : 1.L);
        expected =
            (v.flags & ANALYTIC_NORMAL_LIMIT)
                ? std::array<long double, 3>{0, 0, (v.flags & SPHERE_SOUTH) ? -sheet : sheet}
                : std::array<long double, 3>{sheet * cv * cosine, sheet * cv * sine, sheet * sv};
      } else {
        require(c.tag == 3, "pole cohort must be original sphere/cone carriers");
        const long double sa = std::sin(static_cast<long double>(c.r1)),
                          ca = std::cos(static_cast<long double>(c.r1));
        const long double rho = c.r0 + vparameter * sa;
        const long double sheet = (v.flags & ANALYTIC_NORMAL_LIMIT)
                                      ? ((v.flags & NEGATIVE_RADIAL_SHEET) ? -1.L : 1.L)
                                      : (rho < 0 ? -1.L : 1.L);
        expected = {sheet * ca * cosine, sheet * ca * sine, -sheet * sa};
      }
      for (std::size_t axis = 0; axis < 3; ++axis) {
        if (v.flags & REVERSE_NORMAL)
          expected[axis] = -expected[axis];
        require(std::abs(static_cast<long double>(jets[i].normal[axis]) - expected[axis]) < 2e-12L,
                "CPU/P100 pole source normal disagrees with independent analytic answer");
        require(std::bit_cast<uint64_t>(jets[i].used[axis]) == std::bit_cast<uint64_t>(v.xyz[axis]),
                "CPU/P100 pole consumer re-evaluated stored canonical XYZ");
      }
    }
    Request request{};
    request.width = request.height = 128;
    request.supersample = 1;
    request.basis[0] = request.basis[4] = request.basis[8] = 1;
    request.pixel_center[0] = request.pixel_center[1] = 64;
    request.scale = 80;
    request.background[0] = request.background[1] = request.background[2] = .01;
    std::vector<uint8_t> rgba(128 * 128 * 4);
    std::vector<double> depth(128 * 128);
    std::vector<uint64_t> primitive(128 * 128), instance(128 * 128);
    FrameOutput frame{rgba.data(),     depth.data(), primitive.data(),
                      instance.data(), nullptr,      nullptr};
    require_api(consumer.api->render(token, &request, &frame, &error, &times),
                "pole actual consumer render");
    require(std::any_of(primitive.begin(), primitive.end(),
                        [](uint64_t id) { return id != UINT64_MAX; }),
            "actual pole consumer frame has no source facet pixels");
  } catch (...) {
    static_cast<void>(consumer.api->close(token, &error, &times));
    throw;
  }
  require_api(consumer.api->close(token, &error, &times), "pole actual consumer close");
}
void run(const std::filesystem::path &path, const std::filesystem::path &out, bool actual,
         std::span<const Consumer> consumers) {
  const auto resource = std::make_shared<RunResource>(uint64_t{1} << 30);
  std::stop_source stop;
  JobContext job({uint64_t{1} << 28, uint64_t{1} << 40}, resource.get(), stop.get_token());
  RunPreparationJob prepare(resource, {path, 1., 1e-6});
  require(prepare.step(job) == StepState::done, "source preparation incomplete");
  const auto prepared = prepare.result();
  const auto &geometry = prepared->geometry;
  GeometryContext context(job.budget);
  DefinitionBoundaryForest forest(job.budget, geometry.view().source, {0}, resource);
  require(!forest.pending_evaluations(), "analytic literal endpoints unexpectedly pending");
  const auto tau = Rational(job.budget, 1), zero = Rational(job.budget),
             one = Rational(job.budget, 1);
  FaceBoundarySnapshot snapshot;
  if (actual) {
    PoleCutControl control;
    const auto evaluate = [&](const BoundaryEvaluationBatch &batch) {
      std::pmr::vector<BoundaryEvaluationResult> answers(&job.budget);
      for (const auto &task : batch.tasks()) {
        const auto t = import_exact(job.budget, batch.parameters().view(task.parameter));
        const auto p = canonical_original_curve(context, geometry, task.curve, t);
        answers.push_back({task, p.dimension, p.xyz});
      }
      static_cast<void>(forest.complete_evaluations(batch.epoch(), answers));
    };
    for (uint64_t generation = 17; generation < 25 && !snapshot; ++generation) {
      FaceBoundaryPreparation preparation(
          job.budget, context, geometry, forest.freeze_face_boundary({0}),
          {.face = {0},
           .generation = generation,
           .positional_allowance = tau,
           .node_source_allowance = Rational::from_double(job.budget, 1e-6),
           .allow_plane_profile = false},
          control);
      BoundRuntime runtime(resource, prepared->bank,
                           {16, 16, uint64_t{1} << 25, uint64_t{1} << 36, uint64_t{1} << 40});
      while (const auto work = preparation.take_curve_work(prepared->bank, {16, 16}))
        preparation.complete(runtime.resolve(runtime.filter_cpu(*work)));
      runtime.close();
      if (preparation.state() == FacePreparationState::boundary_request) {
        const auto request = preparation.boundary_request();
        require(request.has_value(), "actual cap producer lost source cut request");
        control = preparation.next_pole_control();
        const auto epoch = forest.begin_epoch();
        const std::array requests{*request};
        forest.arrive(epoch, {0}, forest.face_generation({0}), requests);
        evaluate(forest.prepare_splits(epoch));
      } else
        snapshot = preparation.snapshot();
    }
    require(snapshot != nullptr, "actual cap producer did not reach finite cut horizon");
  } else
    snapshot = bound_backed_seed(job.budget, context, geometry, forest, resource, 17);
  const std::array factors{one, zero, zero, one};
  FaceEngine engine(job.budget, context, snapshot,
                    {factors, {Rational(job.budget, 4), one, one, tau}}, tau, 0);
  while (const auto work = engine.take_cell_work(32))
    for (const auto &cell : (*work)->cells) {
      const auto uv =
          read(job.budget, (*work)->values, cell.original_uv, std::make_index_sequence<6>{});
      const auto xyz =
          read(job.budget, (*work)->values, cell.actual_xyz, std::make_index_sequence<9>{});
      const auto allowance = import_exact(job.budget, (*work)->values.view(cell.allowance));
      const auto answer = bound_analytic_cell(
          context, geometry, {cell.header, cell.carrier, cell.map_kind, uv, xyz, allowance});
      require(answer.decision == BoundDecision::accept && engine.complete(answer),
              "actual analytic ordinary remainder bound failed");
    }
  require(engine.advance().kind == FaceAdvanceKind::settled,
          "actual finite cap and remainder not settled");
  const auto result = engine.freeze_result();
  require(result->sites.size() < snapshot->sites.size(),
          "fixture must contain inactive consumed source sites");
  const std::array results{result};
  const std::array boundaries{forest.freeze_definition_boundary()};
  const auto mesh = publish_native_mesh(job.budget, prepared, boundaries, results, {1., 1e-6, 1});
  if (!actual) {
    const auto hostile = [&](const FaceResult &bad) {
      const auto before = resource->live();
      bool refused = false;
      try {
        const std::array bad_results{bad};
        static_cast<void>(
            publish_native_mesh(job.budget, prepared, boundaries, bad_results, {1., 1e-6, 1}));
      } catch (const Error &e) {
        refused = e.code == ErrorCode::invalid_source;
      }
      require(refused && resource->live() == before,
              "invalid pole publication retained output or accepted missing source map");
    };
    const auto bad = std::allocate_shared<FaceResultData>(RunAllocator<FaceResultData>(resource),
                                                          snapshot, result->values);
    bad->sites = result->sites;
    bad->lineages = result->lineages;
    bad->triangles = result->triangles;
    bad->traversal_adapters = result->traversal_adapters;
    hostile(bad);
    bad->source_sites = result->source_sites;
    bad->triangles[0].pole_piece = UINT32_MAX;
    hostile(bad);
  }
  const auto &d = mesh.view();
  require(d.quotients.size() == 1 && d.cells.size() >= snapshot->pole_pieces.size(),
          "published cap/quotient inventory lost");
  require(d.sites.size() == snapshot->sites.size(),
          "complete source occurrence sites were dropped or mistaken for active prefix");
  std::size_t expected_samples = 0;
  for (const auto &edge : boundaries[0]->edges)
    expected_samples += static_cast<std::size_t>(edge.nodes.count);
  require(d.use_samples.size() == expected_samples,
          "complete original settled endpoint occurrences were dropped");
  const auto &q = d.quotients.front();
  const auto &physical = d.nodes[static_cast<std::size_t>(q.node.value - 1)];
  require(physical.quotient.value == 1, "physical pole quotient has no real owner");
  require(boundaries[0]->vertex_aliases.empty()
              ? std::holds_alternative<OriginalVertexGenerator>(physical.generator)
              : std::holds_alternative<QuotientGenerator>(physical.generator),
          "same-ID/distinct declared identity policy changed");
  require(q.source_vertices.count == std::max(std::size_t{1}, boundaries[0]->vertex_aliases.size()),
          "original alias VertexIDs lost");
  std::size_t collapsed = 0;
  for (const auto &use : d.uses) {
    const auto &original =
        geometry.view().source.view().edges
            [geometry.view()
                 .source.view()
                 .edge_index.find(d.edges[static_cast<std::size_t>(use.edge.value - 1)].source)
                 .value];
    if (original.degenerate) {
      ++collapsed;
      require(use.samples.count == 2, "collapsed source endpoints merged");
      const auto a = d.use_sample_refs[static_cast<std::size_t>(use.samples.first)],
                 b = d.use_sample_refs[static_cast<std::size_t>(use.samples.first + 1)];
      const auto &x = d.use_samples[static_cast<std::size_t>(a.value - 1)],
                 &y = d.use_samples[static_cast<std::size_t>(b.value - 1)];
      require(x.edge_sample != y.edge_sample && x.pcurve_parameter && y.pcurve_parameter,
              "spatial collapsed t replaced literal pcurve interval");
      const auto exact = [&](ExactRef ref) {
        const auto table =
            std::find_if(d.exact_tables.begin(), d.exact_tables.end(),
                         [&](const auto &entry) { return entry.key() == ref.table; });
        if (table == d.exact_tables.end())
          throw std::runtime_error("lost semantic exact owner");
        return import_exact(job.budget, table->view(ref));
      };
      if (x.pcurve_parameter && y.pcurve_parameter) {
        const bool north = q.pole == MeshPole::sphere_north;
        require(exact(*x.pcurve_parameter).compare(Rational(job.budget, north ? 1 : 0)) == 0 &&
                    exact(*y.pcurve_parameter).compare(Rational(job.budget, north ? 0 : 1)) == 0,
                "collapsed use lost literal oriented pcurve endpoints");
      }
      require(exact(d.edge_samples[static_cast<std::size_t>(x.edge_sample.value - 1)].parameter)
                          .sign() == 0 &&
                  exact(d.edge_samples[static_cast<std::size_t>(y.edge_sample.value - 1)].parameter)
                          .sign() == 0,
              "collapsed original spatial parameters were fabricated from pcurve traversal");
    }
  }
  require(collapsed == 1, "expected original collapsed edge absent");
  std::size_t pole_cells = 0;
  for (std::size_t i = 0; i < d.cells.size(); ++i) {
    const auto *map = std::get_if<PoleFanMap>(&d.cell_maps[i].map);
    if (!map)
      continue;
    ++pole_cells;
    require(map->quotient.value == 1 && map->cap_cut_chain.count == 2,
            "pole source map missing local role/cut chain");
    require(d.cell_facets[i].interpolant == MeshInterpolant::pole_bilinear_fan,
            "pole interpolant relabeled UV triangle");
    require(d.cells[i].components.count == 2,
            "actual cap and finite-band bounds not both retained");
  }
  require(pole_cells == snapshot->pole_pieces.size(), "pole coverage cell inventory changed");
  std::filesystem::create_directories(out);
  const auto file = out / (path.stem().string() + (actual ? "-actual" : "-authored") + ".bgnmesh");
  write_native_mesh(file, mesh);
  const auto restored = load_native_mesh(file, resource);
  require(restored.view().sites.size() == d.sites.size() && restored.view().quotients.size() == 1,
          "full pole wire roundtrip lost original occurrences");
  const auto restored_geometry = compile_geometry(
      restored.view().source, resource, {{OwnerKind::geometry, 0, UINT64_MAX}, 0, 0}, job.budget);
  const auto scene = make_scene(job.budget, restored, restored_geometry);
  std::size_t limits = 0;
  for (const auto &v : scene->vertices) {
    const auto &node = d.nodes[static_cast<std::size_t>(v.physical - 1)];
    for (std::size_t axis = 0; axis < 3; ++axis)
      require(std::bit_cast<uint64_t>(v.xyz[axis]) == std::bit_cast<uint64_t>(node.xyz[axis]),
              "pole display re-evaluated canonical XYZ");
    if (v.flags & bgom10::ANALYTIC_NORMAL_LIMIT) {
      ++limits;
      require(v.physical == q.node.value, "regular cut corner mislabeled singular normal");
    }
  }
  require(limits == pole_cells, "actual physical pole corners missing limiting normals");
  for (const auto consumer : consumers)
    consume(scene, consumer);
}
struct JointPaths final {
  std::filesystem::path source, output;
};
void joint_run(const JointPaths &paths, std::span<const Consumer> consumers) {
  const auto resource = std::make_shared<RunResource>(uint64_t{1} << 32);
  NativeScheduler scheduler(resource, {4, 16, uint64_t{1} << 27, uint64_t{1} << 40});
  const auto preparation =
      std::allocate_shared<RunPreparationJob>(RunAllocator<RunPreparationJob>(resource), resource,
                                              RunPreparationInput{paths.source, 1., 1e-6});
  scheduler.submit(preparation);
  scheduler.drain();
  const auto completion = scheduler.take();
  if (!completion)
    throw std::runtime_error("joint source preparation completion absent");
  completion->rethrow();
  const auto prepared = preparation->result();
  MeshingRunLimits limits;
  limits.coordinator_bytes = uint64_t{1} << 27;
  limits.numerical_services = 2;
  const auto result = mesh_prepared_run(prepared, scheduler, limits);
  bgfd::Budget budget({uint64_t{1} << 28, uint64_t{1} << 40}, resource.get());
  const auto mesh =
      publish_native_mesh(budget, prepared, result->boundaries, result->faces, {1., 1e-6, 1});
  const auto &d = mesh.view();
  require(d.quotients.size() == 2 && d.faces.size() == 2,
          "shared quotient lost regular face or cap");
  const auto &joint = d.quotients[0], &pole = d.quotients[1];
  require(joint.kind == MeshQuotientKind::joint && joint.pole == MeshPole::none &&
              d.faces[static_cast<std::size_t>(joint.face.value - 1)].source == FaceId{899} &&
              pole.kind == MeshQuotientKind::pole && joint.node == pole.node,
          "face-local regular joint/pole identities were merged");
  const auto &node = d.nodes[static_cast<std::size_t>(joint.node.value - 1)];
  const auto *generator = std::get_if<QuotientGenerator>(&node.generator);
  require(generator && generator->quotient == MQuotient{1} && node.quotient == MQuotient{1},
          "physical component canonical anchor must be first regular source face");
  require(joint.source_vertices.count == 2 && pole.source_vertices.count == 2,
          "regular incident quotient lost original declared VertexIDs");
  require(std::any_of(d.joints.begin(), d.joints.end(),
                      [&](const auto &row) {
                        return row.face == joint.face && row.source_vertex == VertexId{303} &&
                               row.quotient == MQuotient{1};
                      }),
          "regular source joint substituted physical representative for original VertexID");
  const auto file = paths.output / "north-joint.bgnmesh";
  write_native_mesh(file, mesh);
  const auto restored = load_native_mesh(file, resource);
  const auto geometry = compile_geometry(restored.view().source, resource,
                                         {{OwnerKind::geometry, 0, UINT64_MAX}, 0, 0}, budget);
  const auto scene = make_scene(budget, restored, geometry);
  std::size_t pole_limits = 0;
  for (const auto &v : scene->vertices) {
    if (v.flags & bgom10::ANALYTIC_NORMAL_LIMIT) {
      ++pole_limits;
      require(v.physical == pole.node.value && scene->carriers[v.carrier].id == 9,
              "regular incident face received limiting pole normal");
    }
  }
  require(pole_limits != 0, "cap corner used regular node anchor instead of face-local pole");
  for (const auto consumer : consumers)
    consume(scene, consumer);
}
} // namespace
int main(int argc, char **argv) {
  try {
    require(
        argc >= 3 && argc <= 5,
        "expected pole fixture directory/output directory/[display CUDA module]/[--authored-only]");
    const std::filesystem::path input(argv[1]), output(argv[2]);
    std::vector<Consumer> consumers{{bgom10::cpu_api_v2(), 0}};
    void *module = nullptr;
    bool authored_only = false;
    const char *module_path = nullptr;
    for (int i = 3; i < argc; ++i)
      if (std::string_view(argv[i]) == "--authored-only")
        authored_only = true;
      else {
        require(module_path == nullptr, "duplicate CUDA module argument");
        module_path = argv[i];
      }
    if (module_path) {
      module = dlopen(module_path, RTLD_NOW | RTLD_LOCAL);
      require(module != nullptr, "cannot load explicit CUDA display module");
      const auto factory =
          reinterpret_cast<const bgom10::Api *(*)()>(dlsym(module, "brepgpu_om10_cuda_v2"));
      require(factory != nullptr, "CUDA source-profile v2 factory absent");
      consumers.push_back({factory(), 0});
      consumers.push_back({factory(), 1});
    }
    if (authored_only) {
      for (const char *name : {"north", "north-distinct", "south", "south-distinct", "cone",
                               "cone-distinct", "north-reversed"})
        run(input / (std::string(name) + ".bgsource"), output, false, consumers);
    } else {
      run(input / "north.bgsource", output, false, consumers);
      for (const char *name :
           {"north", "north-distinct", "north-authoritative", "south", "south-distinct",
            "south-authoritative", "cone", "cone-distinct", "cone-authoritative", "north-reversed",
            "south-reversed", "cone-reversed"})
        run(input / (std::string(name) + ".bgsource"), output, true, consumers);
      joint_run({input / "north-joint.bgsource", output}, consumers);
    }
    if (module)
      require(dlclose(module) == 0, "CUDA display module close failed");
    std::cout << "PASS native complete pole/source quotient publication and actual consumers\n";
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
