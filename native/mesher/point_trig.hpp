#ifndef BREPGPU_MESHER_POINT_TRIG_HPP
#define BREPGPU_MESHER_POINT_TRIG_HPP
#include "geometry_analytic.hpp"
namespace bgmesh::point_detail {
enum class PolynomialFamily { taylor, chebyshev };
struct Profile final {
  uint32_t bits, working_cap;
  PolynomialFamily polynomial = PolynomialFamily::chebyshev;
};
[[nodiscard]] std::optional<SinCosEnclosure> try_sin_cos(bgfd::Budget &, const Rational &, Profile);
// Internal point-only entry points share the original source formula body.
// They are used for profile selection/benchmarks; explicit jets never call them.
[[nodiscard]] std::optional<CanonicalPoint> try_curve(GeometryContext &, const Curve &,
                                                      const Rational &, Profile);
[[nodiscard]] std::optional<CanonicalPoint>
try_surface(GeometryContext &, const Carrier &, const Rational &, const Rational &, Profile);
} // namespace bgmesh::point_detail
#endif
