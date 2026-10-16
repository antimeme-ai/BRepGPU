#include "../native/mesher/cell_bounds.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {
using namespace bgmesh;
void require(bool condition, const char *message) {
  if (!condition)
    throw std::runtime_error(message);
}
uint32_t word(std::istream &in) {
  uint32_t value = 0;
  for (unsigned i = 0; i < 4; ++i) {
    const auto c = in.get();
    require(c >= 0, "truncated cell oracle");
    value |= static_cast<uint32_t>(c) << (8 * i);
  }
  return value;
}
Rational value(std::istream &in, bgfd::Budget &budget) {
  ExactWords words;
  words.sign = std::bit_cast<int32_t>(word(in));
  const auto n = word(in), d = word(in);
  require(n < 1000 && d > 0 && d < 1000, "cell oracle integer size");
  words.numerator.resize(n);
  words.denominator.resize(d);
  for (auto &x : words.numerator)
    x = word(in);
  for (auto &x : words.denominator)
    x = word(in);
  return Rational::from_words(budget, words);
}
template <std::size_t N, std::size_t... I>
std::array<Rational, N> values(std::istream &in, bgfd::Budget &budget, std::index_sequence<I...>) {
  return {((void)I, value(in, budget))...};
}
template <class Function> void rejected(Function f, ErrorCode code) {
  bool caught = false;
  try {
    f();
  } catch (const Error &e) {
    caught = e.code == code;
  }
  require(caught, "cell consumer error classification");
}
constexpr ActualCellLimits limits{64, {2, 1}};
void diagonal(const std::shared_ptr<RunResource> &resource, const GeometryStore &geometry,
              bgfd::Budget &budget) {
  constexpr uint32_t n = 16;
  const auto &carrier = geometry.view().carriers[3];
  std::pmr::vector<BoundRootSelection> roots(&budget);
  roots.reserve(carrier.roots.count);
  for (uint64_t i = 0; i < carrier.roots.count; ++i)
    roots.push_back(SurfaceRootSlot{static_cast<uint32_t>(carrier.roots.first + i)});
  const auto bank =
      prepare_bound_roots(budget, resource, geometry, roots, {{OwnerKind::geometry, 0, 0}, 7, 2});
  const Rational zero(budget), one(budget, 1);
  const auto far = one.subtract(one.divide(Rational(budget, 8 * n)));
  const std::array uv{zero, zero, one, far, far, one};
  const std::array xyz{zero, zero, zero, one, far, zero, far, one, zero};
  const ActualTriangleView cell{
      {{1, 999, 5, 0}, 7, 9, 1}, {3}, CellMapKind::uv_triangle, uv, xyz, zero};
  const auto prepared = prepare_actual_cell(budget, resource, bank, cell,
                                            {{OwnerKind::face, 0, 999}, 5, 0}, {8 * n, {2, 1}});
  const auto stats = prepared.stats();
  std::cout << "thin_diagonal_bbox=" << n * n << " column_entries=" << stats.column_entries
            << " rectangle_candidates=" << stats.root_candidates << '\n';
  require(stats.column_entries + stats.root_candidates <= 8 * n,
          "thin diagonal cell visits full bbox grid instead of occupied knot columns");
}
void analytic(const GeometryStore &geometry, bgfd::Budget &budget) {
  GeometryContext context(budget);
  const Rational zero(budget), one(budget, 1);
  const std::array uv{zero, zero, one, zero, zero, one};
  const std::array xyz{Rational::from_double(budget, 2),   Rational::from_double(budget, -1),
                       Rational::from_double(budget, .75), Rational::from_double(budget, 4),
                       Rational::from_double(budget, -1),  Rational::from_double(budget, .75),
                       Rational::from_double(budget, 3),   Rational::from_double(budget, 2),
                       Rational::from_double(budget, .75)};
  const auto quarter = one.divide(Rational(budget, 4)), eighth = one.divide(Rational(budget, 8));
  const BoundTaskHeader parent{{1, 99, 5, 0}, 7, 9, 1, 13, 17};
  const ActualTriangleView plane{parent, {1}, CellMapKind::uv_triangle, uv, xyz, quarter};
  const auto plane_result = bound_analytic_cell(context, geometry, plane);
  require(plane_result.header.request_revision == 13 && plane_result.header.face_generation == 17,
          "analytic parent result lost request/face metadata");
  require(plane_result.decision == BoundDecision::accept,
          "actual analytic plane endpoint departure equality must accept");
  const ActualTriangleView plane_fail{parent, {1}, CellMapKind::uv_triangle, uv, xyz, eighth};
  require(bound_analytic_cell(context, geometry, plane_fail).decision == BoundDecision::bound_fail,
          "analytic plane cannot hide actual off-source XYZ departure");
  const ActualTriangleView rational{parent, {0}, CellMapKind::uv_triangle, uv, xyz, quarter};
  rejected([&] { static_cast<void>(bound_analytic_cell(context, geometry, rational)); },
           ErrorCode::unsupported);
  const std::array curved_uv{zero, zero, eighth, zero, zero, eighth};
  std::pmr::vector<Rational> curved_xyz(&budget);
  curved_xyz.reserve(9);
  for (std::size_t i = 0; i < 3; ++i) {
    const auto point =
        canonical_original_surface(context, geometry, {2}, curved_uv[2 * i], curved_uv[2 * i + 1]);
    for (const auto x : point.xyz)
      curved_xyz.push_back(Rational::from_double(budget, x));
  }
  const auto broad = one.divide(Rational(budget, 16)), narrow = one.divide(Rational(budget, 10000));
  const ActualTriangleView curve_ok{parent,
                                    {2},
                                    CellMapKind::uv_triangle,
                                    curved_uv,
                                    std::span<const Rational, 9>(curved_xyz.data(), 9),
                                    broad};
  const ActualTriangleView curve_fail{parent,
                                      {2},
                                      CellMapKind::uv_triangle,
                                      curved_uv,
                                      std::span<const Rational, 9>(curved_xyz.data(), 9),
                                      narrow};
  // Unit-cylinder ||Suu||=1, du=1/8, mixed/vv derivatives0. The
  // sufficient Hessian term is at least1/512 and below1/16, with tiny
  // canonical nodal rounding. These decisions need no libm oracle.
  require(bound_analytic_cell(context, geometry, curve_ok).decision == BoundDecision::accept,
          "original curved analytic triangle bound should accept broad tolerance");
  require(bound_analytic_cell(context, geometry, curve_fail).decision == BoundDecision::bound_fail,
          "original curved analytic triangle bound should refine narrow tolerance");
}
struct FixturePaths final {
  const char *oracle, *module;
};
void run(const std::shared_ptr<RunResource> &resource, BoundRootBank bank, FixturePaths paths) {
  std::ifstream input(paths.oracle, std::ios::binary);
  std::string magic(8, '\0');
  input.read(magic.data(), 8);
  require(magic == "BGMCELL1", "cell oracle magic");
  const auto count = word(input);
  require(count == 8, "cell oracle case count");
  for (uint32_t row = 0; row < count; ++row) {
    bgfd::Budget budget({1ULL << 24, 1ULL << 32}, resource.get());
    const auto uv = values<6>(input, budget, std::make_index_sequence<6>{});
    const auto xyz = values<9>(input, budget, std::make_index_sequence<9>{});
    const auto tau = value(input, budget);
    const auto piece_count = word(input);
    const BoundTaskHeader parent{{1, row, 5, 0}, 7, 9, 1, 13, 17};
    ActualTriangleView view{parent, {0}, CellMapKind::uv_triangle, uv, xyz, tau};
    const auto prepared = prepare_actual_cell(budget, resource, bank, view,
                                              {{OwnerKind::face, 0, row}, 5, 0}, limits);
    if (piece_count > 1) {
      rejected(
          [&] {
            static_cast<void>(prepare_actual_cell(budget, resource, bank, view,
                                                  {{OwnerKind::face, 0, row}, 5, 2}, {1, {2, 1}}));
          },
          ErrorCode::resource_exhausted);
    }
    rejected([&] { CellBoundFold null(nullptr, prepared); }, ErrorCode::internal_error);
    require(prepared.pieces().size() == piece_count,
            "clipping piece count disagrees with literal polygons");
    for (uint32_t i = 0; i < piece_count; ++i) {
      const auto root = word(input), expected = word(input);
      const auto expected_uv = values<6>(input, budget, std::make_index_sequence<6>{});
      const auto expected_xyz = values<9>(input, budget, std::make_index_sequence<9>{});
      const auto &piece = prepared.pieces()[i];
      require(piece.root.value == root, "source root identity changed");
      for (std::size_t k = 0; k < 6; ++k)
        require(import_exact(budget, prepared.values().view(piece.original_uv[k]))
                        .compare(expected_uv[k]) == 0,
                "exact clipped UV disagrees");
      for (std::size_t k = 0; k < 9; ++k)
        require(import_exact(budget, prepared.values().view(piece.actual_linear_xyz[k]))
                        .compare(expected_xyz[k]) == 0,
                "virtual corner resampled source instead of SAME actual affine L");
      BoundRuntime runtime(resource, bank, {2, 1, 1ULL << 20, 1ULL << 30, 1ULL << 38});
      const auto answer = runtime.resolve(runtime.filter_cpu(prepared.batch({i, 1})));
      require(static_cast<uint32_t>(answer.answers()[0].decision) == expected,
              "independent POWER residual oracle disagrees");
    }
    const auto expected = word(input);
    BoundRuntime cpu(resource, bank, {2, 1, 1ULL << 20, 1ULL << 30, 1ULL << 38});
    CellBoundFold fold(resource, prepared);
    rejected([&] { static_cast<void>(fold.result()); }, ErrorCode::internal_error);
    for (uint32_t first = 0; first < piece_count; first += 2) {
      const auto result =
          cpu.resolve(cpu.filter_cpu(prepared.batch({first, std::min(2u, piece_count - first)})));
      if (first == 0) {
        auto newer = view;
        ++newer.header.request_revision;
        const auto revised = prepare_actual_cell(budget, resource, bank, newer,
                                                 {{OwnerKind::face, 0, row}, 5, 3}, limits);
        CellBoundFold revised_fold(resource, revised);
        rejected([&] { revised_fold.add(result); }, ErrorCode::invalid_source);
        auto rebuilt_view = view;
        ++rebuilt_view.header.face_generation;
        const auto rebuilt = prepare_actual_cell(budget, resource, bank, rebuilt_view,
                                                 {{OwnerKind::face, 0, row}, 5, 4}, limits);
        CellBoundFold rebuilt_fold(resource, rebuilt);
        rejected([&] { rebuilt_fold.add(result); }, ErrorCode::invalid_source);
      }
      fold.add(result);
      rejected([&] { fold.add(result); }, ErrorCode::invalid_source);
    }
    require(fold.result().header.key == parent.key &&
                fold.result().header.request_revision == parent.request_revision &&
                fold.result().header.face_generation == parent.face_generation &&
                static_cast<uint32_t>(fold.result().decision) == expected,
            "full parent fold disagrees");
    auto wrong = view;
    wrong.map_kind = CellMapKind::plane_xy_triangle;
    rejected(
        [&] {
          static_cast<void>(prepare_actual_cell(budget, resource, bank, wrong,
                                                {{OwnerKind::face, 0, row}, 5, 1}, limits));
        },
        ErrorCode::unsupported);
    if (paths.module) {
      const auto cuda = load_bound_cuda(resource, paths.module);
      for (int32_t device = 0; device < 2; ++device) {
        BoundRuntime gpu(resource, bank, {2, 1, 1ULL << 20, 1ULL << 30, 1ULL << 38});
        gpu.open_device(cuda, {device, 1ULL << 20, 1ULL << 20});
        CellBoundFold device_fold(resource, prepared);
        // Reverse batch order: final conjunction must be completion independent.
        for (uint32_t end = piece_count; end > 0;) {
          const auto first = end >= 2 ? end - 2 : 0;
          const auto ticket = gpu.submit(prepared.batch({first, end - first}));
          std::optional<BoundFiltered> filtered;
          while (!filtered)
            filtered = gpu.poll(ticket);
          device_fold.add(gpu.resolve(*filtered));
          end = first;
        }
        require(static_cast<uint32_t>(device_fold.result().decision) == expected,
                "P100 actual cell fold differs");
        gpu.close();
      }
    }
  }
  require(input.get() == std::char_traits<char>::eof(), "extra cell oracle data");
}
} // namespace
int main(int argc, char **argv) {
  try {
    require(argc == 3 || argc == 4, "source oracle [CUDA module] arguments required");
    auto resource = std::make_shared<RunResource>(1ULL << 28);
    {
      const auto source = load_source(argv[1], resource);
      bgfd::Budget scratch({1ULL << 24, 1ULL << 34}, resource.get());
      const auto geometry =
          compile_geometry(source, resource, {{OwnerKind::geometry, 0, 0}, 7, 0}, scratch);
      const std::array<BoundRootSelection, 2> roots{SurfaceRootSlot{0}, SurfaceRootSlot{1}};
      const auto bank = prepare_bound_roots(scratch, resource, geometry, roots,
                                            {{OwnerKind::geometry, 0, 0}, 7, 1});
      run(resource, bank, {argv[2], argc == 4 ? argv[3] : nullptr});
      analytic(geometry, scratch);
      diagonal(resource, geometry, scratch);
    }
    require(resource->live() == 0, "actual cell consumer retained healthy resources");
    std::cout << "PASS C0 source clipping SAME affine actual-cell bounds\n";
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
