#ifndef BREPGPU_MESHER_GEOMETRY_BOUNDARY_HPP
#define BREPGPU_MESHER_GEOMETRY_BOUNDARY_HPP
#include "boundary.hpp"
#include "geometry_charts.hpp"

namespace bgmesh {
struct LocatorLimits final {
  uint32_t iterations = 32, backtracks = 12;
};
struct UseEndpointRequest final {
  SourceBoundarySnapshot boundary;
  uint32_t original_endpoint;
};
struct UseNodeRequest final {
  Slot<CoedgeTag> use;
  Rational source_parameter, node_source_allowance;
  GeometryBoundPrecision precision{};
  LocatorLimits locator_limits{};
  std::optional<UseEndpointRequest> endpoint{};
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
  std::optional<std::array<Rational, 4>> original_uv_enclosure{};
  std::optional<Rational> canonical_source_upper{}, canonical_carrier_upper{};
};
struct BoundaryJointChoice final {
  Slot<VertexTag> vertex;
  std::array<Rational, 2> chart, original_uv;
  std::pmr::vector<BoundarySiteChoice> occurrences;
  std::optional<std::array<Rational, 4>> original_uv_enclosure{};
};
struct OriginalUsePoint final {
  Slot<CoedgeTag> use;
  const Rational &source_parameter;
};
[[nodiscard]] Rational original_quotient_parameter(bgfd::Budget &, const SourceStore &,
                                                   Slot<CoedgeTag>, uint32_t original_endpoint);
[[nodiscard]] Rational map_original_use_parameter(GeometryContext &, const WorkingChart &,
                                                  OriginalUsePoint);
void require_authoritative_point_agreement(GeometryContext &, const WorkingChart &,
                                           std::array<OriginalUsePoint, 2>,
                                           GeometryBoundPrecision = {});
[[nodiscard]] BoundarySiteChoice locate_boundary_site(GeometryContext &, const WorkingChart &,
                                                      const UseNodeRequest &);
// All incident occurrences name the same original vertex on one face branch.
// The result supplies ONE chosen UV/working-chart joint to every occurrence.
[[nodiscard]] BoundaryJointChoice choose_boundary_joint(GeometryContext &, const WorkingChart &,
                                                        std::span<const UseNodeRequest>);
} // namespace bgmesh
#endif
