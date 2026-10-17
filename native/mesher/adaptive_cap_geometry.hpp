#ifndef BREPGPU_MESHER_ADAPTIVE_CAP_GEOMETRY_HPP
#define BREPGPU_MESHER_ADAPTIVE_CAP_GEOMETRY_HPP
#include "adaptive_cap_types.h"
#include "adaptive_geometry.hpp"
#include "geometry_poles.hpp"

namespace bgmesh::adaptive {
class PreparedCapSource final {
  SourceCache source_;
  CapSourceDescriptor numeric_;
  PreparedCapSource(SourceCache source, CapSourceDescriptor numeric)
      : source_(std::move(source)), numeric_(numeric) {}
  friend PreparedCapSource prepare_cap_source(const SourceCache &,
                                              OriginalRootSlot, AnalyticCapKind,
                                              bgfd::Budget &);

public:
  [[nodiscard]] const SourceCache &source() const noexcept { return source_; }
  [[nodiscard]] const CapSourceDescriptor &numeric() const noexcept {
    return numeric_;
  }
};
struct PreciseCapResult final {
  PoleFanPlan bounds;
  bool certified;
};
struct CapMapPlan final {
  ExactEnclosure pole_parameter;
  int32_t regular_sheet_sign;
  std::array<uint32_t, 3> positive_chart_roles;
};
enum class CapProgressState : uint32_t { proved, not_contracted, unresolved };
[[nodiscard]] PreparedCapSource prepare_cap_source(const SourceCache &,
                                                   OriginalRootSlot,
                                                   AnalyticCapKind,
                                                   bgfd::Budget &);
[[nodiscard]] CapFacetBounds bound_cap_facet(const PreparedCapSource &,
                                             const CapFacetInput &,
                                             bgfd::Budget &);
[[nodiscard]] CapMapPlan
admit_cap_map(const PreparedCapSource &,
              std::span<const Rational, 4> exact_cut_uv,
              std::span<const Rational, 2> exact_finite_connector_v,
              GeometryBoundPrecision, bgfd::Budget &);
[[nodiscard]] std::array<CapProgressState, 2>
cap_cut_progress(const PreparedCapSource &,
                 std::span<const Rational, 4> old_cut,
                 std::span<const Rational, 4> new_cut, GeometryBoundPrecision,
                 bgfd::Budget &);
// Owner-local Rational outputs; they must be sealed before asynchronous
// handoff.
[[nodiscard]] PreciseCapResult compare_cap_facet_precise(
    const PreparedCapSource &, std::span<const Rational, 4> exact_cut_uv,
    std::span<const Rational, 2> exact_finite_connector_v,
    std::span<const double, 9> actual_xyz, const Rational &allowance,
    GeometryBoundPrecision, bgfd::Budget &);
} // namespace bgmesh::adaptive
#endif
