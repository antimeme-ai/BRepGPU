#ifndef BREPGPU_MESHER_DOMAIN_HPP
#define BREPGPU_MESHER_DOMAIN_HPP
#include "../direct_cdt/core.hpp"
#include "topology.hpp"
#include <span>

namespace bgmesh {
struct DomainLoopView final {
  bool outer;
  std::span<const SiteSlot> sites;
  std::span<const BoundaryKey> constraints;
};
struct DomainStats final {
  uint64_t bvh_visits = 0, segment_candidates = 0, ray_candidates = 0, ray_queries = 0;
  uint64_t winding_aggregates = 0, event_entries = 0;
  bgcdt::CoreStats cdt;
};
struct InitialDomain final {
  std::pmr::vector<int32_t> traversal_adapters;
  DomainStats stats;
  explicit InitialDomain(std::pmr::memory_resource *resource) : traversal_adapters(resource) {}
};
// Initial sites already belong to this one mutable owner. Every site occurs
// exactly once in the original cycles. Source use order/sense are untouched;
// adapter -1 reverses a cycle anchored at its original first joint. On failure
// the face generation must be destroyed, never partially published.
[[nodiscard]] InitialDomain initialize_face_domain(bgfd::Budget &, MutableFaceTopology &,
                                                   std::span<const DomainLoopView>, uint64_t seed);
} // namespace bgmesh
#endif
