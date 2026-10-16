#ifndef BREPGPU_MESHER_GEOMETRY_HPP
#define BREPGPU_MESHER_GEOMETRY_HPP

#include "exact.hpp"
#include "values.hpp"
#include <array>
#include <cstdint>
#include <memory_resource>
#include <span>
#include <vector>

namespace bgmesh {
// Original binary64 source values; knots are expanded using their original
// multiplicities. Controls are homogeneous, dimension+1 values per point.
struct BSplineCurveView final {
  uint32_t dimension, degree;
  std::span<const double> knots, controls;
};
struct BSplineSurfaceView final {
  uint32_t p, q, nu, nv;
  std::span<const double> knots_u, knots_v, controls;
};

// Owner-local working values. Their Budget must outlive them; they never cross
// worker boundaries or become shared GeometryStore roots without explicit
// freeze.
struct CurveShape final {
  uint32_t dimension, degree;
};
struct BezierCurve final {
  uint32_t dimension, degree;
  Rational begin, end;
  std::pmr::vector<Rational> controls;
  BezierCurve(bgfd::Budget &, CurveShape, const std::array<Rational, 2> &domain);
  BezierCurve(const BezierCurve &other);
  BezierCurve(BezierCurve &&) noexcept = default;
  BezierCurve &operator=(const BezierCurve &) = delete;
  BezierCurve &operator=(BezierCurve &&) = delete;
};
struct BezierSurface final {
  uint32_t p, q;
  std::array<Rational, 4> bounds;      // u0,u1,v0,v1 in original source coordinates
  std::pmr::vector<Rational> controls; // U-major, V-fast, homogeneous stride4
  BezierSurface(bgfd::Budget &, uint32_t p_value, uint32_t q_value,
                const std::array<Rational, 4> &bounds_value);
  BezierSurface(const BezierSurface &other);
  BezierSurface(BezierSurface &&) noexcept = default;
  BezierSurface &operator=(const BezierSurface &) = delete;
  BezierSurface &operator=(BezierSurface &&) = delete;
};
struct BezierTriangle final {
  uint32_t degree;
  // Homogeneous stride4. For i=0..degree,j=0..degree-i the exponents are
  // (degree-i-j,i,j); corner roles0/1/2 match the supplied UV order.
  std::pmr::vector<Rational> controls;
  BezierTriangle(bgfd::Budget &budget, uint32_t degree_value)
      : degree(degree_value), controls(&budget) {}
  BezierTriangle(const BezierTriangle &other)
      : degree(other.degree), controls(other.controls, other.controls.get_allocator().resource()) {}
  BezierTriangle(BezierTriangle &&) noexcept = default;
  BezierTriangle &operator=(const BezierTriangle &) = delete;
  BezierTriangle &operator=(BezierTriangle &&) = delete;
};

[[nodiscard]] std::pmr::vector<BezierCurve> extract_curve(bgfd::Budget &, BSplineCurveView);
[[nodiscard]] std::pmr::vector<BezierSurface> extract_surface(bgfd::Budget &, BSplineSurfaceView);
[[nodiscard]] std::pmr::vector<Rational> evaluate_curve(const BezierCurve &, const Rational &t);
[[nodiscard]] std::array<Rational, 3> evaluate_surface(const BezierSurface &, const Rational &u,
                                                       const Rational &v);
[[nodiscard]] BezierCurve restrict_curve(const BezierCurve &, const Rational &begin,
                                         const Rational &end);
[[nodiscard]] BezierSurface restrict_surface(const BezierSurface &,
                                             const std::array<Rational, 4> &bounds);
// Restrict source values, not a topology/coverage admission operation. UV is
// u0,v0,u1,v1,u2,v2, in the original root domain; no virtual XYZ is evaluated.
[[nodiscard]] BezierTriangle restrict_to_triangle(const BezierSurface &,
                                                  std::span<const Rational, 6> original_uv);
} // namespace bgmesh
#endif
