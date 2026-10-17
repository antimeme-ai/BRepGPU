#ifndef BREPGPU_MESHER_GEOMETRY_POLYNOMIAL_HPP
#define BREPGPU_MESHER_GEOMETRY_POLYNOMIAL_HPP
#include "geometry.hpp"

namespace bgmesh::geometry_detail {
struct PolynomialShape final {
  uint32_t degree;
  std::size_t stride;
};
// The same de Casteljau engine used by original geometry evaluation. Derivative
// homogeneous polynomials may have signed/zero weight channels, so this private
// owner seam validates shape/ownership without interpreting a rational surface.
[[nodiscard]] std::pmr::vector<Rational>
evaluate_homogeneous_polynomial(std::span<const Rational>, PolynomialShape,
                                const Rational &normalized_parameter);
} // namespace bgmesh::geometry_detail
#endif
