#ifndef BREPGPU_MESHER_METRICS_HPP
#define BREPGPU_MESHER_METRICS_HPP
#include "exact_metric.hpp"
#include "geometry_jets.hpp"
#include <optional>
#include <span>

namespace bgmesh {
struct MetricScales final {
  const Rational &physical_diagonal_squared, &chart_width_u, &chart_width_v, &allowance;
};
// Nearest53-significand dyadic sqrt with a separate exact exponent. It never
// underflows a factor into zero merely because binary64 exponent space ends.
[[nodiscard]] std::array<Rational, 4> metric_from_point_jet(const SurfaceJetEnclosure &,
                                                            MetricScales);
[[nodiscard]] std::array<Rational, 4>
original_source_metric(GeometryContext &, const GeometryStore &, Slot<CarrierTag>,
                       std::span<const Rational, 2> point, MetricScales,
                       uint32_t precision_bits = 96);
// Original chart (u0,v0,u1,v1,u2,v2), same owner as the dyadic factor.
// Nonfinite chosen coordinates are infinities for the topology's routine spent
// rejection.
[[nodiscard]] std::array<double, 2> metric_circumcenter(std::span<const Rational, 6>,
                                                        const std::array<Rational, 4> &factor);
[[nodiscard]] std::optional<double> metric_shape_potential(std::span<const Rational, 6>,
                                                           const std::array<Rational, 4> &factor);
} // namespace bgmesh
#endif
