#ifndef BREPGPU_RIGID_PLACEMENT_HPP
#define BREPGPU_RIGID_PLACEMENT_HPP
// The one placement admission rule for source usages and displayed instances.
// A row-major 3x4 child-to-parent map is admitted only as a proper rigid
// motion: every Gram entry of its three rotation rows within the tolerance of
// the identity, and a positive determinant. Scale, shear and mirroring refuse
// (widening to mirrored and uniformly scaled placements is a later, separate
// change). python/brepgpu/imported_source.py mirrors this function with the
// same operation order; tests/fixtures/rigid-placements.tsv pins both.
#include <cmath>
#include <cstddef>

namespace brepgpu {
inline constexpr double rigid_placement_tolerance = 1e-9;
enum class PlacementStatus { rigid, nonfinite, nonorthonormal, improper };
inline PlacementStatus rigid_placement_status(const double *m) {
  for (std::size_t i = 0; i < 12; ++i)
    if (!std::isfinite(m[i]))
      return PlacementStatus::nonfinite;
  for (std::size_t row = 0; row < 3; ++row)
    for (std::size_t col = 0; col < 3; ++col) {
      double value = 0;
      for (std::size_t k = 0; k < 3; ++k)
        value += m[4 * row + k] * m[4 * col + k];
      if (!(std::fabs(value - (row == col ? 1.0 : 0.0)) <= rigid_placement_tolerance))
        return PlacementStatus::nonorthonormal;
    }
  const double det = m[0] * (m[5] * m[10] - m[6] * m[9]) - m[1] * (m[4] * m[10] - m[6] * m[8]) +
                     m[2] * (m[4] * m[9] - m[5] * m[8]);
  return det > 0 ? PlacementStatus::rigid : PlacementStatus::improper;
}
} // namespace brepgpu
#endif
