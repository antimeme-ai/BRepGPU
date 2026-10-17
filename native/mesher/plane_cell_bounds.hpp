#ifndef BREPGPU_MESHER_PLANE_CELL_BOUNDS_HPP
#define BREPGPU_MESHER_PLANE_CELL_BOUNDS_HPP
#include "cell_bounds.hpp"

namespace bgmesh {
struct PlaneCellProfileView final {
  std::span<const Rational, 3> origin;
  std::span<const Rational, 6> basis;
  std::span<const Rational, 8> quadrilateral;
  const Rational &height_upper;
};
struct PlaneTriangleView final {
  BoundTaskHeader header;
  CellMapKind map_kind;
  std::span<const Rational, 6> chart_xy;
  std::span<const Rational, 9> actual_xyz;
  const Rational &allowance;
};
// Owner-local, borrowed source values must outlive this profile. Validate the
// admitted profile once per batch and retain exact halfplanes for its cells.
// No instance may outlive its Budget or cross a scheduler scratch boundary.
class PlaneCellProfile final {
  bgfd::Budget *budget_;
  PlaneCellProfileView source_;
  std::array<std::array<Rational, 3>, 4> edges_;
  friend CellBoundResult bound_plane_cell(const PlaneCellProfile &, PlaneTriangleView);

public:
  PlaneCellProfile(bgfd::Budget &, PlaneCellProfileView);
};
[[nodiscard]] CellBoundResult bound_plane_cell(const PlaneCellProfile &, PlaneTriangleView);
} // namespace bgmesh
#endif
