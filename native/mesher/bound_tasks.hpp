#ifndef BREPGPU_MESHER_BOUND_TASKS_HPP
#define BREPGPU_MESHER_BOUND_TASKS_HPP

#include "cuda/bound_api.h"
#include "geometry.hpp"
#include <cstdint>
#include <span>

namespace bgmesh {
struct BoundTaskKey final {
  uint64_t owner = 0, cell = 0, generation = 0, ordinal = 0;
  auto operator<=>(const BoundTaskKey &) const = default;
};
enum class BoundDecision : uint32_t { accept, bound_fail, unresolved };
struct BoundAnswer final {
  BoundTaskKey key;
  BoundDecision decision;
};
// Degree-d simplex coefficients, homogeneous XYZ/W, exponent order(d-i-j,i,j).
// Values are local to one child Budget. L is the ACTUAL output facet restricted
// to the same subcell, with XYZ at its three vertices; no virtual XYZ rounding.
struct TriangleResidualView final {
  uint32_t degree;
  std::span<const Rational> homogeneous;
  std::span<const Rational, 9> linear_xyz;
  const Rational &tolerance;
};
[[nodiscard]] BoundAnswer triangle_residual_exact(BoundTaskKey, TriangleResidualView);
[[nodiscard]] BoundAnswer surface_bound_exact(BoundTaskKey, const BezierSurface &,
                                              std::span<const Rational, 6> original_uv,
                                              std::span<const Rational, 9> linear_xyz,
                                              const Rational &tolerance);
[[nodiscard]] BoundAnswer curve_bound_exact(BoundTaskKey, const BezierCurve &,
                                            const Rational &begin, const Rational &end,
                                            std::span<const Rational, 6> endpoint_xyz,
                                            const Rational &tolerance);
// Packing translates homogeneous XYZ by the exact root anchor N0/W0, and L by
// the same anchor; W, residual R and decisions are unchanged. No per-cell host
// source restriction is performed, and canonical physical XYZ are never changed.
[[nodiscard]] bgmb_root pack_bound_root(const BezierSurface &, std::span<bgmb_interval>,
                                        uint64_t offset = 0);
[[nodiscard]] bgmb_root pack_bound_root(const BezierCurve &, std::span<bgmb_interval>,
                                        uint64_t offset = 0);
[[nodiscard]] bgmb_task pack_bound_task(BoundTaskKey, uint32_t root, const BezierSurface &,
                                        std::span<const Rational, 6> original_uv,
                                        std::span<const Rational, 9> linear_xyz,
                                        const Rational &tolerance);
// Curve task parameters increase; directed adapters swap parameters/XYZ together.
[[nodiscard]] bgmb_task pack_bound_task(BoundTaskKey, uint32_t root, const BezierCurve &,
                                        const Rational &begin, const Rational &end,
                                        std::span<const Rational, 6> endpoint_xyz,
                                        const Rational &tolerance);
// Prepared immutable banks already validated original controls/weights. Import
// only this small exact frame into a child when packing another task.
struct PreparedBoundFrame final {
  uint32_t kind;
  std::span<const Rational> domain;
  std::span<const Rational, 3> anchor;
};
[[nodiscard]] bgmb_task pack_prepared_bound_task(BoundTaskKey, uint32_t root, PreparedBoundFrame,
                                                 std::span<const Rational> parameters,
                                                 std::span<const Rational> linear_xyz,
                                                 const Rational &tolerance);
[[nodiscard]] BoundAnswer filter_bound_cpu(bgfd::Budget &, const bgmb_root &,
                                           std::span<const bgmb_interval>, const bgmb_task &);
} // namespace bgmesh
#endif
