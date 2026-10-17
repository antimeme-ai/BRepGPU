#include "../native/mesher/face_preparation.hpp"
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
template <class F> void rejected(F action, ErrorCode code) {
  bool failed = false;
  try {
    action();
  } catch (const Error &e) {
    failed = e.code == code;
  }
  require(failed, "plane cell refusal type");
}
void literal(bgfd::Budget &budget) {
  const Rational zero(budget), one(budget, 1), two(budget, 2), three(budget, 3);
  const auto eighth = one.divide(Rational(budget, 8)), quarter = one.divide(Rational(budget, 4));
  const std::array origin{two, Rational(budget, -1), one.divide(two)};
  const std::array basis{two, one, zero, three, one, zero};
  const std::array q{zero, zero, one, zero, one, one, zero, one};
  const PlaneCellProfile profile(budget, {origin, basis, q, eighth});
  const std::array xy{zero, zero, one, zero, zero, one};
  // Exact departures=(3/8,4/8,0), norm5/8. Nonorthogonal frame.
  const auto dx = three.multiply(eighth), dy = one.divide(two);
  const std::array xyz{two.add(dx),
                       Rational(budget, -1).add(dy),
                       one.divide(two),
                       Rational(budget, 4).add(dx),
                       Rational(budget, -1).add(dy),
                       three.divide(two),
                       three.add(dx),
                       two.add(dy),
                       one.divide(two)};
  const auto tau = three.multiply(quarter);
  const BoundTaskHeader header{{900, 4, 5, 0}, 7, 9, 1, 13, 17};
  PlaneTriangleView view{header, CellMapKind::plane_xy_triangle, xy, xyz, tau};
  const auto answer = bound_plane_cell(profile, view);
  require(answer.decision == BoundDecision::accept && answer.header.key == header.key &&
              answer.header.face_generation == 17 && answer.header.request_revision == 13,
          "exact h+5/8 equality/metadata");
  const auto smaller = tau.subtract(one.divide(Rational(budget, 1048576)));
  require(bound_plane_cell(profile, {header, CellMapKind::plane_xy_triangle, xy, xyz, smaller})
                  .decision == BoundDecision::bound_fail,
          "just below exact threshold should fail");
  const auto tiny = one.divide(Rational(budget, 16));
  require(
      bound_plane_cell(profile, {header, CellMapKind::plane_xy_triangle, xy, xyz, tiny}).decision ==
          BoundDecision::bound_fail,
      "allowance below source height should fail");
  auto outside = xy;
  outside[2] = two;
  rejected(
      [&] {
        static_cast<void>(
            bound_plane_cell(profile, {header, CellMapKind::plane_xy_triangle, outside, xyz, tau}));
      },
      ErrorCode::invalid_source);
  const std::array reversed{zero, zero, zero, one, one, zero};
  rejected(
      [&] {
        static_cast<void>(bound_plane_cell(
            profile, {header, CellMapKind::plane_xy_triangle, reversed, xyz, tau}));
      },
      ErrorCode::invalid_source);
  rejected(
      [&] {
        static_cast<void>(
            bound_plane_cell(profile, {header, CellMapKind::uv_triangle, xy, xyz, tau}));
      },
      ErrorCode::unsupported);
  auto badq = q;
  badq[6] = two;
  rejected([&] { PlaneCellProfile invalid(budget, {origin, basis, badq, eighth}); },
           ErrorCode::invalid_source);
  const auto negative = eighth.negative();
  rejected([&] { PlaneCellProfile invalid(budget, {origin, basis, q, negative}); },
           ErrorCode::invalid_source);
  auto bad_header = header;
  bad_header.key.ordinal = 1;
  rejected(
      [&] {
        static_cast<void>(
            bound_plane_cell(profile, {bad_header, CellMapKind::plane_xy_triangle, xy, xyz, tau}));
      },
      ErrorCode::invalid_source);
}
template <std::size_t N, std::size_t... I>
std::array<Rational, N> read(bgfd::Budget &budget, const ExactTable &table,
                             const std::array<ExactRef, N> &refs, std::index_sequence<I...>) {
  return {import_exact(budget, table.view(refs[I]))...};
}
FaceCellWork packet(const std::shared_ptr<RunResource> &resource, const FaceBoundarySnapshot &seed,
                    bgfd::Budget &budget) {
  const auto &plane = seed->plane;
  if (!plane.has_value())
    throw std::runtime_error("actual source plane profile missing");
  const auto &p = plane.value();
  const auto origin = read(budget, seed->values, p.origin, std::make_index_sequence<3>{});
  const auto basis = read(budget, seed->values, p.basis, std::make_index_sequence<6>{});
  const auto q = read(budget, seed->values, p.quadrilateral, std::make_index_sequence<8>{});
  const auto height = import_exact(budget, seed->values.view(p.height_upper));
  require(height.sign() == 0, "warped flat plane height must be exactly zero");
  PlaneCellProfile profile(budget, {origin, basis, q, height});
  const auto tau = Rational(budget, 1).divide(Rational(budget, 100000));
  ExactBuilder builder(resource, {{OwnerKind::run, 0, 701}, 1, 0});
  std::array<FaceCellInput, 2> cells{};
  constexpr std::array<std::array<std::size_t, 3>, 2> triangles{{{0, 1, 2}, {0, 2, 3}}};
  const auto &source = seed->geometry.view().source.view();
  const auto face = source.faces[seed->face.value];
  for (std::size_t t = 0; t < 2; ++t) {
    const auto &roles = triangles[t];
    std::array<Rational, 6> xy{q[2 * roles[0]],     q[2 * roles[0] + 1], q[2 * roles[1]],
                               q[2 * roles[1] + 1], q[2 * roles[2]],     q[2 * roles[2] + 1]};
    std::pmr::vector<Rational> xyz(&budget);
    xyz.reserve(9);
    auto &cell = cells[t];
    cell.header = {
        {face.id.value, t, 5, 0}, seed->source_generation, seed->boundary_generation, 1, 13,
        seed->face_generation};
    cell.carrier = source.carrier_index.find(face.carrier_id);
    cell.map_kind = CellMapKind::plane_xy_triangle;
    cell.allowance = builder.append(tau);
    for (std::size_t i = 0; i < 3; ++i) {
      cell.chart_xy[2 * i] = builder.append(xy[2 * i]);
      cell.chart_xy[2 * i + 1] = builder.append(xy[2 * i + 1]);
      // Source UV corners equal these XY corner numbers; their *interior*
      // source interpolation differs. They remain recovery-only values.
      cell.original_uv[2 * i] = cell.chart_xy[2 * i];
      cell.original_uv[2 * i + 1] = cell.chart_xy[2 * i + 1];
      for (std::size_t axis = 0; axis < 3; ++axis) {
        const auto point = origin[axis]
                               .add(basis[2 * axis].multiply(xy[2 * i]))
                               .add(basis[2 * axis + 1].multiply(xy[2 * i + 1]));
        xyz.push_back(Rational::from_double(budget, point.to_double()));
        cell.actual_xyz[3 * i + axis] = builder.append(xyz.back());
      }
    }
    require(bound_plane_cell(profile, {cell.header, cell.map_kind, xy,
                                       std::span<const Rational, 9>(xyz.data(), 9), tau})
                    .decision == BoundDecision::accept,
            "nonlinear flat original source should retain its physical plane triangle");
  }
  auto work = std::allocate_shared<FaceCellWorkData>(
      RunAllocator<FaceCellWorkData>(resource), resource, seed->geometry, builder.seal(), seed);
  work->cells.assign(cells.begin(), cells.end());
  return work;
}
void exercise(const std::shared_ptr<RunResource> &resource, const std::filesystem::path &directory,
              const char *module) {
  const auto source = load_source(directory / "warped-plane.bgsource", resource);
  bgfd::Budget budget({1ULL << 26, 1ULL << 35}, resource.get());
  literal(budget);
  const auto geometry =
      compile_geometry(source, resource, {{OwnerKind::geometry, 0, 0}, 7, 0}, budget);
  GeometryContext context(budget);
  DefinitionBoundaryForest forest(budget, source, {0}, resource);
  if (const auto pending = forest.pending_evaluations()) {
    std::pmr::vector<BoundaryEvaluationResult> answers(&budget);
    for (const auto &task : pending->tasks()) {
      const auto parameter = import_exact(budget, pending->parameters().view(task.parameter));
      const auto point = canonical_original_curve(context, geometry, task.curve, parameter);
      answers.push_back({task, point.dimension, point.xyz});
    }
    static_cast<void>(forest.complete_evaluations(pending->epoch(), answers));
  }
  const auto tau = Rational(budget, 1).divide(Rational(budget, 100000));
  FaceBoundaryPreparation preparation(budget, context, geometry, forest.freeze_face_boundary({0}),
                                      {.face = {0},
                                       .generation = 11,
                                       .positional_allowance = tau,
                                       .node_source_allowance = tau,
                                       .allow_plane_profile = true});
  require(preparation.state() == FacePreparationState::ready,
          "real plane boundary preparation did not settle");
  const auto seed = preparation.snapshot();
  require(seed->chart_kind == WorkingChartKind::physical_plane_convex_q,
          "real nonlinear source did not select plane profile");
  const auto work = packet(resource, seed, budget);
  const std::array<BoundRootSelection, 1> selections{SurfaceRootSlot{0}};
  const auto bank = prepare_bound_roots(budget, resource, geometry, selections,
                                        {{OwnerKind::geometry, 0, 0}, 7, 1});
  {
    const auto &cell = work->cells[0];
    const auto uv = read(budget, work->values, cell.original_uv, std::make_index_sequence<6>{});
    const auto xyz = read(budget, work->values, cell.actual_xyz, std::make_index_sequence<9>{});
    // Independent literal Sx(1/2)-Lx(1/2)=1/8. Treating XY as UV must
    // request refinement even though the actual physical-plane error is zero.
    const auto prepared = prepare_actual_cell(
        budget, resource, bank, {cell.header, cell.carrier, CellMapKind::uv_triangle, uv, xyz, tau},
        {{OwnerKind::run, 0, 702}, 1, 0}, {64, {4, 4}});
    BoundRuntime runtime(resource, bank, {4, 4, 1ULL << 22, 1ULL << 30, 1ULL << 35});
    CellBoundFold fold(resource, prepared);
    fold.add(runtime.resolve(
        runtime.filter_cpu(prepared.batch({0, static_cast<uint32_t>(prepared.pieces().size())}))));
    require(fold.result().decision == BoundDecision::bound_fail,
            "same-UV control did not distinguish nonlinear parametrization");
    runtime.close();
  }
  for (int32_t device = -1; device < (module ? 2 : 0); ++device) {
    NativeScheduler scheduler(resource, {1, 4, 1ULL << 24, 1ULL << 33});
    auto service = std::allocate_shared<NumericalServiceJob>(
        RunAllocator<NumericalServiceJob>(resource), resource,
        NativeTaskKey{{OwnerKind::run, 0, 800 + static_cast<uint64_t>(device + 1)}, 1, 0}, bank,
        NumericalServiceLimits{{4, 4, 1ULL << 22, 1ULL << 30, 1ULL << 35}, {64, {4, 4}}, 8, {}},
        scheduler.stop_token());
    if (device >= 0)
      service->open_device(load_bound_cuda(resource, module), {device, 1ULL << 20, 1ULL << 20});
    const NativeTaskKey child{{OwnerKind::face, 0, 900}, 11, 77};
    auto stale = std::allocate_shared<FaceCellWorkData>(RunAllocator<FaceCellWorkData>(resource),
                                                        resource, geometry, work->values, seed);
    stale->cells.assign(work->cells.begin(), work->cells.end());
    ++stale->cells[0].header.face_generation;
    rejected([&] { service->begin({child, FaceCellWork(stale)}); }, ErrorCode::invalid_source);
    service->begin({child, work});
    scheduler.submit(service);
    for (;;) {
      const auto completion = scheduler.take();
      if (!completion)
        continue;
      completion->rethrow();
      if (completion->state == StepState::done)
        break;
      require(completion->state == StepState::ready, "plane numerical job blocked");
      scheduler.submit(service);
    }
    require(service->result().key == child, "plane coordinator identity changed");
    const auto results = std::get<std::shared_ptr<const CellBatchResult>>(service->result().output);
    require(results->cells.size() == 2, "plane service changed two-triangle coverage");
    for (const auto &answer : results->cells)
      require(answer.decision == BoundDecision::accept && answer.header.face_generation == 11 &&
                  answer.header.request_revision == 13,
              "plane actual-cell metadata/decision changed");
    const auto stats = service->stats();
    require(stats.plane_cells == 2 && stats.runtime.device_tasks == 0 &&
                stats.runtime.cpu_tasks == 0,
            "plane path fabricated residual/GPU tasks");
    service->release_result();
    service->close();
  }
}
} // namespace
int main(int argc, char **argv) {
  try {
    require(argc == 2 || argc == 3, "chart directory [CUDA module] required");
    const auto resource = std::make_shared<RunResource>(1ULL << 29);
    exercise(resource, argv[1], argc == 3 ? argv[2] : nullptr);
    require(resource->live() == 0, "plane consumer retained owner storage");
    std::cout << "PASS exact physical plane actual-cell source bound\n";
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
