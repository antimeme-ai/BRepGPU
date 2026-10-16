#ifndef BREPGPU_MESHER_GEOMETRY_INVERSE_HPP
#define BREPGPU_MESHER_GEOMETRY_INVERSE_HPP
#include "geometry_charts.hpp"
namespace bgmesh {
struct FaceBoundarySnapshotData;
struct PlaneLocatorLimits final {
  uint32_t iterations = 32, backtracks = 16, normalized_fractional_bits = 40;
};
struct PlaneLocator final {
  std::array<Rational, 2> chosen;
  std::array<Rational, 4> enclosure;
};
[[nodiscard]] PlaneLocator recover_plane_chart_locator(GeometryContext &, const WorkingChart &,
                                                       const std::array<Rational, 2> &xy,
                                                       GeometryBoundPrecision = {},
                                                       PlaneLocatorLimits = {});
[[nodiscard]] PlaneLocator recover_plane_chart_locator(GeometryContext &,
                                                       const FaceBoundarySnapshotData &,
                                                       const std::array<Rational, 2> &xy,
                                                       GeometryBoundPrecision = {},
                                                       PlaneLocatorLimits = {});
} // namespace bgmesh
#endif
