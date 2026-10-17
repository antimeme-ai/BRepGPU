#include "../../native/mesher/point_trig.hpp"
#include <bit>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <optional>
#include <string_view>

namespace {
using namespace bgmesh;
enum class Mode { exact, taylor, chebyshev, selected };
template <class Evaluate> CanonicalPoint reference(GeometryContext &context, Evaluate evaluate) {
  for (;;) {
    const auto proof = evaluate();
    Vec3 xyz{};
    bool resolved = true;
    for (uint32_t axis = 0; axis < proof.dimension; ++axis) {
      const auto lo = proof.coordinates[axis].lower.to_double(),
                 hi = proof.coordinates[axis].upper.to_double();
      resolved = resolved && std::bit_cast<uint64_t>(lo) == std::bit_cast<uint64_t>(hi);
      xyz[axis] = lo;
    }
    if (resolved)
      return {proof.dimension, xyz};
    context.refine();
  }
}
struct Observation final {
  uint64_t checksum;
  double milliseconds;
  uint64_t work, peak;
};
template <class Evaluate>
Observation measure(std::string_view name, std::string_view mode, Evaluate evaluate) {
  bgfd::Budget budget({uint64_t{1} << 29, uint64_t{1} << 38});
  GeometryContext context(budget);
  const auto start = std::chrono::steady_clock::now();
  uint64_t checksum = 0;
  constexpr int64_t count = 1024;
  for (int64_t i = 0; i < count; ++i) {
    const auto t = Rational(budget, i).divide(Rational(budget, count));
    const auto point = evaluate(context, t);
    for (uint32_t axis = 0; axis < point.dimension; ++axis)
      checksum = std::rotl(checksum, 1) ^ std::bit_cast<uint64_t>(point.xyz[axis]);
  }
  const auto ms =
      std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
  std::cout << name << '\t' << mode << '\t' << count << '\t' << ms << '\t' << budget.work() << '\t'
            << budget.peak() << '\t' << checksum << std::endl;
  return {checksum, ms, budget.work(), budget.peak()};
}
template <class Evaluate> void family(std::string_view name, Evaluate evaluate) {
  const auto baseline = measure(
      name, "exact_reference", [&](auto &c, const auto &t) { return evaluate(c, t, Mode::exact); });
  for (const auto mode : {Mode::taylor, Mode::chebyshev, Mode::selected}) {
    const auto label = mode == Mode::taylor      ? "taylor"
                       : mode == Mode::chebyshev ? "chebyshev"
                                                 : "selected";
    const auto observation =
        measure(name, label, [&](auto &c, const auto &t) { return evaluate(c, t, mode); });
    if (observation.checksum != baseline.checksum)
      throw std::runtime_error("point backend changed source word checksum");
  }
}
CanonicalPoint curve(GeometryContext &c, const Curve &source, const Rational &t, Mode mode) {
  if (mode == Mode::selected)
    return canonical_analytic_curve(c, source, t);
  if (mode != Mode::exact)
    for (const uint32_t p : {128U, 192U, 256U})
      if (const auto point = point_detail::try_curve(
              c, source, t,
              {p, c.maximum_bits(),
               mode == Mode::taylor ? point_detail::PolynomialFamily::taylor
                                    : point_detail::PolynomialFamily::chebyshev}))
        return *point;
  return reference(c, [&] { return evaluate_analytic_curve(c, source, t); });
}
CanonicalPoint surface(GeometryContext &c, const Carrier &source, const Rational &u,
                       const Rational &v, Mode mode) {
  if (mode == Mode::selected)
    return canonical_analytic_surface(c, source, u, v);
  if (mode != Mode::exact)
    for (const uint32_t p : {128U, 192U, 256U})
      if (const auto point = point_detail::try_surface(
              c, source, u, v,
              {p, c.maximum_bits(),
               mode == Mode::taylor ? point_detail::PolynomialFamily::taylor
                                    : point_detail::PolynomialFamily::chebyshev}))
        return *point;
  return reference(c, [&] { return evaluate_analytic_surface(c, source, u, v); });
}
} // namespace
int main() {
  try {
    std::cout << std::setprecision(17)
              << "family\tbackend\tpoints\twall_ms\twork\tpeak_bytes\tword_checksum\n";
    Curve circle{};
    circle.kind = 2;
    circle.dimension = 3;
    circle.radii = {1, 1};
    circle.frame = {0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1};
    family("circle_0_1",
           [&](auto &c, const auto &t, auto mode) { return curve(c, circle, t, mode); });
    auto ellipse = circle;
    ellipse.kind = 3;
    ellipse.radii = {1.5, .75};
    ellipse.frame = {2, -3, .5, 1, .25, .5, .125, 2, -.5, 0, 0, 1};
    family("ellipse_nonorthogonal_0_6", [&](auto &c, const auto &t, auto mode) {
      return curve(c, ellipse, t.multiply(Rational(c.budget(), 6)), mode);
    });
    Carrier torus{};
    torus.kind = 5;
    torus.radii = {2, .25};
    torus.frame = ellipse.frame;
    family("torus_nonorthogonal", [&](auto &c, const auto &t, auto mode) {
      return surface(c, torus, t.multiply(Rational(c.budget(), 6)),
                     t.negative().divide(Rational(c.budget(), 3)), mode);
    });
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
