#ifndef BREPGPU_MESHER_GEOMETRY_POLES_HPP
#define BREPGPU_MESHER_GEOMETRY_POLES_HPP
#include "geometry_jets.hpp"
namespace bgmesh {
enum class AnalyticPoleKind {sphere_north,sphere_south,cone_apex};
struct PoleFanRequest final {
 AnalyticPoleKind kind;
 // u0,v0,u1,v1 along the exact dyadic linear cut. Source pole is symbolic.
 std::array<Rational,4> cut_uv;
 // Actual role order: pole, left cut, right cut. Not necessarily facet winding.
 std::array<double,9> actual_xyz;
 std::array<Rational,2> original_pole_v;
 int32_t face_orientation;
 GeometryBoundPrecision precision{};
};
struct PoleFanPlan final {
 ExactEnclosure pole_parameter;
 Rational position_upper,band_upper;
 std::array<uint32_t,3> facet_roles;
};
[[nodiscard]] PoleFanPlan bound_analytic_pole_fan(GeometryContext &,const Carrier &,const PoleFanRequest &);
} // namespace bgmesh
#endif
