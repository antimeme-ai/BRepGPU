#ifndef BREPGPU_MESHER_ADAPTIVE_CAP_TYPES_H
#define BREPGPU_MESHER_ADAPTIVE_CAP_TYPES_H
#include "adaptive_geometry_types.h"

namespace bgmesh::adaptive {
enum class AnalyticCapKind : uint32_t { sphere_north, sphere_south, cone_apex };
// Bound-only auxiliary values derived from the SAME original evaluator root.
// They do not construct a physical pole Q or enlarge its original source box.
struct CapSourceDescriptor {
  OriginalRootSlot original;
  AnalyticCapKind kind = AnalyticCapKind::sphere_north;
  ArithmeticStage stage = ArithmeticStage::rn64;
  Interval pole_v{};
  Interval ideal_xyz[3]{};
};
struct CapFacetInput {
  OriginalRootSlot original;
  Interval cut_uv[4]{};   // left U,V then right U,V, ORIGINAL coordinates
  Interval finite_v[2]{}; // affine finite connector endpoint V enclosures
  double actual_xyz[9]{}; // fixed original pole Q, left cut Q, right cut Q
  double allowance_down = 0.;
};
struct CapFacetBounds {
  NumericStatus status = NumericStatus::needs_wider;
  double position_upper = 0., band_upper = 0., total_upper = 0.;
  double angular_upper = 0., radial_upper = 0., corner_upper = 0.;
  double pole_departure_upper = 0., uncertainty_upper = 0.;
  int32_t regular_sheet_sign = 0;
  // Positive ORIGINAL source-chart roles, before original face orientation.
  uint32_t positive_chart_roles[3]{};
};
} // namespace bgmesh::adaptive
#endif
