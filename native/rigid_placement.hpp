#ifndef BREPGPU_RIGID_PLACEMENT_HPP
#define BREPGPU_RIGID_PLACEMENT_HPP
// The one placement admission rule for source usages and displayed instances.
// A row-major 3x4 child-to-parent map is admitted only as a proper rigid
// motion: every Gram entry of its three rotation rows within the tolerance of
// the identity, and a positive determinant. Scale, shear and mirroring refuse
// (widening to mirrored and uniformly scaled placements is a later, separate
// change).
//
// What admission guarantees. Let M be the 3x3 part and tau the tolerance.
// Gershgorin on G = M M^T (|G_ij - delta_ij| <= tau) puts every eigenvalue
// sigma^2 in [1 - 3 tau, 1 + 3 tau], so
//   |sigma - 1| <= 3 tau / (1 + sqrt(1 - 3 tau)) = 1.5 tau (1 + 0.75 tau + ...),
//   |det M - 1| <= (1 + 1.5 tau)^3 - 1 = 4.5 tau + 6.75 tau^2 + 3.375 tau^3,
// and a normal carried by rotate(M, n) and renormalised (om10/math.hpp:177-178,
// legacy and raw profiles) differs in direction from the exact cofactor normal
// M^-T n by an angle theta with tan(theta) <= 3 tau / (1 - 3 tau). At
// tau = 1e-9: lengths change by at most 1.5e-9 relative, |det - 1| <= 4.5e-9,
// theta <= 3e-9 rad. Every consumer applies M itself, so positions are exact
// as declared; the tolerance only decides membership of the rigid class
// (one definition mesh shared by its rigid placements).
//
// Consumers checked for that (none uses M^T as an inverse): the native reader
// (mesher/source.cpp), native assembly compose and its transport bound, which
// scales each definition's allowance by the actual sqrt(max row sum |M^T M|)
// (mesher/assembly.cpp), the display projection (om10/math.hpp project, CPU
// and CUDA), presentation bounds (mesher/presentation.cpp), the raw reference
// display (om10_reference), and in Python imported_assembly (compose,
// transform_point, placed_transform), om10_scene and the oracles'
// qualify_native_scene, which recompute M p and M n. A tighter rule anywhere
// downstream would refuse what the readers admit without protecting any of
// these bounds, so every placement consumer applies this one predicate.
//
// Callers: mesher/source.cpp (reader), om10/validation.hpp (source and legacy
// display profiles, CPU and CUDA), om10_reference/raw.hpp (raw reference).
// Mirror: python/brepgpu/imported_source.py rigid_placement_status, the same
// operations in the same order; python/brepgpu/imported_assembly.py
// rigid_transform (usages, composed occurrences, reference_source usages)
// calls that mirror. No other copy of the rule exists.
// tests/fixtures/rigid-placements.tsv pins all of them
// (tests/native_placement_admission.cpp,
// tests/native_reference_placement_admission.cpp, tests/test_step_ingress.py,
// tests/test_placement_oracles.py).
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
