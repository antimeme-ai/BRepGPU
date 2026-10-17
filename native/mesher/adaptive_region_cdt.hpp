#ifndef BREPGPU_MESHER_ADAPTIVE_REGION_CDT_HPP
#define BREPGPU_MESHER_ADAPTIVE_REGION_CDT_HPP
#include "../direct_cdt/core.hpp"
#include "exact.hpp"
#include <span>
namespace bgmesh::adaptive {
struct RegionCycleView final {
  std::span<const uint32_t> sites;
};
struct RegionCDT final {
  std::pmr::vector<bgcdt::TriangleWord> triangles;
  std::pmr::vector<uint32_t> components, loop_components;
  uint32_t component_count = 0;
  uint64_t denominator_bits = 0;
  bgcdt::CoreStats stats;
  explicit RegionCDT(std::pmr::memory_resource *owner)
      : triangles(owner), components(owner), loop_components(owner) {}
};
[[nodiscard]] RegionCDT construct_region_cdt(bgfd::Budget &,
                                             std::span<const std::array<Rational, 2>> sites,
                                             std::span<const RegionCycleView> cycles,
                                             const std::array<Rational, 4> &factor, uint64_t seed,
                                             bool filter_nonmaterial_anchors = false);
} // namespace bgmesh::adaptive
#endif
