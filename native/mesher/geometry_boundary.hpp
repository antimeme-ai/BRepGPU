#ifndef BREPGPU_MESHER_GEOMETRY_BOUNDARY_HPP
#define BREPGPU_MESHER_GEOMETRY_BOUNDARY_HPP
#include "geometry_charts.hpp"

namespace bgmesh {
struct LocatorLimits final {
  uint32_t iterations = 32, backtracks = 12;
};
struct UseNodeRequest final {
  Slot<CoedgeTag> use;
  Rational source_parameter, node_source_allowance;
  GeometryBoundPrecision precision{};
  LocatorLimits locator_limits{};
};
// A proposal until a common face-vertex branch joint has been selected. These
// owner-local values freeze into the existing FaceBoundarySnapshot, not a new
// source or boundary representation. Source target is original V/Ce, never the
// rounded forest XYZ. pcurve_parameter is mapped s, distinct from spatial t.
struct BoundarySiteChoice final {
  Slot<CoedgeTag> use;
  Rational source_parameter, pcurve_parameter;
  std::array<Rational, 2> chart, original_uv;
  Rational node_carrier_upper, endpoint_curve_upper;
};
struct BoundaryJointChoice final {
  Slot<VertexTag> vertex;
  std::array<Rational, 2> chart, original_uv;
  std::pmr::vector<BoundarySiteChoice> occurrences;
};
[[nodiscard]] BoundarySiteChoice locate_boundary_site(GeometryContext &, const WorkingChart &,
                                                      const UseNodeRequest &);
// All incident occurrences name the same original vertex on one face branch.
// The result supplies ONE chosen UV/working-chart joint to every occurrence.
[[nodiscard]] BoundaryJointChoice choose_boundary_joint(GeometryContext &, const WorkingChart &,
                                                        std::span<const UseNodeRequest>);
} // namespace bgmesh
#endif
