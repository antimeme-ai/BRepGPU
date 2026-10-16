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
enum class DomainFailureMode { reject, report_geometry };
enum class DomainIssueKind { too_few_sites, zero_area, boundary_contact, hole_placement };
struct DomainSegmentLocation final {
  uint32_t loop;
  uint64_t segment;
};
// Locations address the ORIGINAL occurrence order supplied by DomainLoopView,
// never the normalized cycle or an inferred original-curve parameter.
struct DomainIssue final {
  DomainIssueKind kind;
  std::array<DomainSegmentLocation, 2> segments{};
  uint32_t segment_count = 0;
};
struct InitialDomain final {
  std::pmr::vector<int32_t> traversal_adapters;
  DomainStats stats;
  std::optional<DomainIssue> issue;
  explicit InitialDomain(std::pmr::memory_resource *resource) : traversal_adapters(resource) {}
};
// Initial sites already belong to this one mutable owner. Every site occurs
// exactly once in the original cycles. Source use order/sense are untouched;
// adapter -1 reverses a cycle anchored at its original first joint. On failure
// the face generation must be destroyed, never partially published.
[[nodiscard]] InitialDomain initialize_face_domain(bgfd::Budget &, MutableFaceTopology &,
                                                   std::span<const DomainLoopView>, uint64_t seed,
                                                   DomainFailureMode = DomainFailureMode::reject);
} // namespace bgmesh
#endif
