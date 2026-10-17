#include "../native/mesher/geometry_analytic.hpp"
#include <array>
#include <bit>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
using bgfd::Budget;
using bgmesh::Rational;
void require(bool ok, const char *message) {
  if (!ok)
    throw std::runtime_error(message);
}
Rational ratio(Budget &b, int64_t n, int64_t d = 1) {
  return Rational(b, n).divide(Rational(b, d));
}
// Independent direct Taylor sums at the ORIGINAL rational argument: no pi,
// range reduction, compressed interval arithmetic or production evaluator.
std::array<Rational, 2> direct_series(const Rational &x, bool cosine) {
  auto &b = x.budget();
  auto term = cosine ? Rational(b, 1) : Rational(x);
  auto total = term;
  const auto xx = x.multiply(x);
  // The original arguments below have |x|<=17. Terms decrease after index10.
  // Two successive sums then enclose the alternating tail exactly.
  for (int64_t k = 1; k <= 128; ++k) {
    const auto d = cosine ? (2 * k - 1) * (2 * k) : (2 * k) * (2 * k + 1);
    term = term.multiply(xx).divide(Rational(b, d)).negative();
    total = total.add(term);
  }
  const int64_t next_d = cosine ? 257 * 258 : 258 * 259;
  const auto next = total.add(term.multiply(xx).divide(Rational(b, next_d)).negative());
  return total.compare(next) < 0 ? std::array<Rational, 2>{total, next}
                                 : std::array<Rational, 2>{next, total};
}
void contains_series(const bgmesh::ExactEnclosure &actual,
                     const std::array<Rational, 2> &reference) {
  require(actual.lower.compare(reference[0]) <= 0 && actual.upper.compare(reference[1]) >= 0,
          "original trig enclosure contains direct rational series bracket");
  require(actual.upper.subtract(actual.lower).compare(ratio(actual.lower.budget(), 1, 1LL << 40)) <
              0,
          "original trig enclosure contracts to requested useful precision");
}
void trig(Budget &b) {
  bgmesh::GeometryContext context(b, {96, 768});
  for (const auto [n, d] : std::array<std::array<int64_t, 2>, 8>{
           {{0, 1}, {1, 2}, {1, 1}, {2, 1}, {-2, 1}, {7, 2}, {-7, 2}, {17, 1}}}) {
    const auto x = ratio(b, n, d);
    const auto answer = bgmesh::enclose_sin_cos(context, x);
    contains_series(answer.sine, direct_series(x, false));
    contains_series(answer.cosine, direct_series(x, true));
  }
}
void source_formulas(Budget &b) {
  bgmesh::GeometryContext context(b, {96, 768});
  // Nonorthogonal source frame; retained dimension2 ignores the frame's Z.
  const bgmesh::Frame frame{3., -2., 5., 2., 1., 7., 1., 3., 11., 0., 2., 4.};
  bgmesh::Curve line{};
  line.dimension = 3;
  line.kind = 1;
  line.frame = frame;
  line.parameter_range = {-2., 2.};
  const auto line_point = bgmesh::evaluate_analytic_curve(context, line, ratio(b, 1, 2));
  const std::array<int64_t, 3> line_n{8, -3, 17};
  for (std::size_t i = 0; i < 3; ++i) {
    require(line_point.coordinates[i].lower.compare(ratio(b, line_n[i], 2)) == 0 &&
                line_point.coordinates[i].upper.compare(ratio(b, line_n[i], 2)) == 0,
            "analytic line original frame evaluates exactly");
  }
  line.dimension = 2;
  line.kind = 3;
  line.radii = {2., 3.};
  const auto ellipse = bgmesh::evaluate_analytic_curve(context, line, ratio(b, 0));
  require(ellipse.dimension == 2 && ellipse.coordinates[0].lower.compare(ratio(b, 7)) == 0 &&
              ellipse.coordinates[1].lower.compare(ratio(b, 0)) == 0,
          "dimension2 ellipse preserves XY frame and exact special value");
  bgmesh::Carrier carrier{};
  carrier.frame = frame;
  carrier.radii = {2., .5};
  carrier.uv_bounds = {-2., 2., -2., 2.};
  const std::array<std::array<int64_t, 3>, 5> expected{
      {{9, 0, 28}, {14, 0, 38}, {14, 0, 38}, {14, 0, 38}, {16, 1, 45}}};
  // Plane(u=.5,v=.5), cylinder/cone/sphere(u=v=0), torus(u=v=0).
  for (uint32_t kind = 1; kind <= 5; ++kind) {
    carrier.kind = kind;
    const auto u = ratio(b, kind == 1 ? 1 : 0, 2), v = ratio(b, kind == 1 ? 1 : 0, 2);
    const auto point = bgmesh::evaluate_analytic_surface(context, carrier, u, v);
    for (std::size_t axis = 0; axis < 3; ++axis) {
      const auto exact = ratio(b, expected[kind - 1][axis], 2);
      // Cone at v0 is independent of semi-angle, no false frame normalization.
      require(point.coordinates[axis].lower.compare(exact) <= 0 &&
                  point.coordinates[axis].upper.compare(exact) >= 0,
              "original analytic surface formula encloses literal source point");
    }
  }
  const auto canonical = bgmesh::canonical_analytic_curve(context, line, ratio(b, 1));
  // x=3+4cos(1)+3sin(1), y=-2+2cos(1)+9sin(1), dimension2.
  const auto cos_one = direct_series(ratio(b, 1), true);
  const auto sin_one = direct_series(ratio(b, 1), false);
  for (std::size_t axis = 0; axis < 2; ++axis) {
    const auto x = Rational::from_double(b, frame[3 + axis]).multiply(ratio(b, 2));
    const auto y = Rational::from_double(b, frame[6 + axis]).multiply(ratio(b, 3));
    const auto lo = Rational::from_double(b, frame[axis])
                        .add(x.multiply(cos_one[0]))
                        .add(y.multiply(sin_one[0]));
    const auto hi = Rational::from_double(b, frame[axis])
                        .add(x.multiply(cos_one[1]))
                        .add(y.multiply(sin_one[1]));
    require(std::bit_cast<uint64_t>(lo.to_double()) == std::bit_cast<uint64_t>(hi.to_double()) &&
                std::bit_cast<uint64_t>(canonical.xyz[axis]) ==
                    std::bit_cast<uint64_t>(lo.to_double()),
            "canonical analytic output equals independent exact-series "
            "rounding cell");
  }
  require(canonical.dimension == 2, "dimension2 canonical point stays dimension2");
  line.dimension = 3;
  line.kind = 1;
  line.frame = {1., 0., 0., 0x1p-53, 0., 0., 0., 0., 0., 0., 0., 0.};
  require(bgmesh::canonical_analytic_curve(context, line, ratio(b, 1)).xyz[0] == 1.,
          "exact source line midpoint rounds ties to even once");
}

void original_dispatch(Budget &b, const char *path) {
  const auto resource = std::make_shared<bgmesh::RunResource>(uint64_t{1} << 26);
  const auto source = bgmesh::load_source(path, resource);
  const auto store =
      bgmesh::compile_geometry(source, resource, {{bgmesh::OwnerKind::geometry, 0, 124}, 0, 0}, b);
  bgmesh::GeometryContext context(b);
  const auto curve = bgmesh::canonical_original_curve(
      context, store, bgmesh::Slot<bgmesh::CurveTag>{0}, ratio(b, 3, 4));
  require(curve.dimension == 2 && curve.xyz[0] == 1.75 && curve.xyz[1] == 1.25,
          "general source dispatch preserves exact original rational curve");
  const auto line = bgmesh::canonical_original_curve(
      context, store, bgmesh::Slot<bgmesh::CurveTag>{1}, ratio(b, 3, 4));
  require(line.dimension == 3 && line.xyz == bgmesh::Vec3{.75, 0., 0.},
          "general source dispatch retains original analytic line");
  const auto surface = bgmesh::canonical_original_surface(
      context, store, bgmesh::Slot<bgmesh::CarrierTag>{0}, ratio(b, 3, 4), ratio(b, 3, 4));
  require(surface.xyz[0] == .75 && surface.xyz[1] == ratio(b, 6, 7).to_double() &&
              surface.xyz[2] == ratio(b, 9, 14).to_double(),
          "general source surface dispatch correctly rounds original rational "
          "position");
}

void exact_special_values_and_limits(Budget &b) {
  bgmesh::GeometryContext context(b);
  bgmesh::Curve line{};
  line.kind = 1;
  line.dimension = 3;
  line.frame = {0., 0., 0., 1., 0., 0., 0., 0., 0., 0., 0., 0.};
  const auto tiny = Rational::from_ratio(Rational(b, -1).numerator(),
                                         Rational(b, 1).numerator().shift_left(1075));
  const auto point = bgmesh::canonical_analytic_curve(context, line, tiny);
  require(std::bit_cast<uint64_t>(point.xyz[0]) == 0x8000000000000000ULL,
          "negative exact source underflow retains IEEE signed zero");
  const auto caught = [&](auto operation, bgmesh::ErrorCode expected) {
    bool rejected = false;
    try {
      operation();
    } catch (const bgmesh::Error &e) {
      rejected = e.code == expected;
    }
    require(rejected, "analytic owner/precision failure has the expected type");
  };
  caught([&] { bgmesh::GeometryContext bad(b, {16, 32}); }, bgmesh::ErrorCode::invalid_source);
  caught([&] { static_cast<void>(context.pi(0)); }, bgmesh::ErrorCode::invalid_source);
  Budget foreign({uint64_t{1} << 20, uint64_t{1} << 20});
  caught([&] { static_cast<void>(bgmesh::enclose_sin_cos(context, ratio(foreign, 1))); },
         bgmesh::ErrorCode::internal_error);
  bgmesh::GeometryContext limited(b, {32, 64});
  const auto huge = Rational::from_ratio(Rational(b, 1).numerator().shift_left(256),
                                         Rational(b, 1).denominator());
  caught([&] { static_cast<void>(bgmesh::enclose_sin_cos(limited, huge)); },
         bgmesh::ErrorCode::unresolved);
  bgmesh::GeometryContext last_stage(b, {32, 128});
  last_stage.refine();
  last_stage.refine();
  require(last_stage.bits() == 95, "last point stage reserves the actual trig guard budget");
  static_cast<void>(bgmesh::enclose_sin_cos(last_stage, ratio(b, 1)));
  caught([&] { last_stage.refine(); }, bgmesh::ErrorCode::unresolved);
}
} // namespace
int main(int argc, char **argv) {
  try {
    Budget budget({uint64_t{1} << 28, uint64_t{1} << 35});
    require(argc == 2, "expected original geometry source fixture");
    trig(budget);
    source_formulas(budget);
    original_dispatch(budget, argv[1]);
    exact_special_values_and_limits(budget);
    std::cout << "PASS\toriginal analytic geometry\twork=" << budget.work()
              << "\tpeak_bytes=" << budget.peak() << '\n';
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "FAIL\t" << e.what() << '\n';
    return 1;
  } catch (const bgfd::Failure &e) {
    std::cerr << "FAIL\t" << e.what() << '\n';
    return 1;
  }
}
