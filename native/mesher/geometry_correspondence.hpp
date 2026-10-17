#ifndef BREPGPU_MESHER_GEOMETRY_CORRESPONDENCE_HPP
#define BREPGPU_MESHER_GEOMETRY_CORRESPONDENCE_HPP
#include "geometry_boundary.hpp"
namespace bgmesh {
enum class TraceCorrespondence { affine_parameter, geometric_line, cylinder_arc_order };
struct TraceIntervalRequest final {
  Slot<CoedgeTag> use;
  std::array<Rational, 2> source_parameter;
  std::array<Rational, 4> chart_endpoints, original_uv;
  std::array<Rational, 6> actual_endpoint_xyz;
  Rational positional_allowance;
  GeometryBoundPrecision precision{};
  bool allow_geometric_line = true;
};
struct CurveChordPiece final {
  CurveRootSlot root;
  std::array<Rational, 2> original_parameter;
  std::array<Rational, 6> actual_endpoint_xyz;
};
struct GeometricLineMap final {
  std::array<Rational, 3> covector;
  std::array<Rational, 2> curve_scalar, trace_scalar;
};
struct TraceIntervalPlan final {
  TraceCorrespondence correspondence;
  Rational trace_upper;
  std::optional<Rational> chord_upper;
  std::optional<GeometricLineMap> line;
  std::pmr::vector<CurveChordPiece> chord_pieces;
};
// Ordinary rational chord pieces go unchanged to BoundRuntime. Coupled original
// source/selected trace bounds and finite image correspondences are host work.
// Acceptance still needs source-domain maps/protection and actual cell bounds.
[[nodiscard]] TraceIntervalPlan bound_trace_interval(GeometryContext &, const WorkingChart &,
                                                     const TraceIntervalRequest &);
} // namespace bgmesh
#endif
