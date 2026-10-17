#ifndef BREPGPU_MESHER_DOMAIN_HPP
#define BREPGPU_MESHER_DOMAIN_HPP
#include "../direct_cdt/core.hpp"
#include "domain_index.hpp"
#include "topology.hpp"
#include <span>

namespace bgmesh {
struct DomainLoopView final {
  bool outer;
  std::span<const SiteSlot> sites;
  std::span<const BoundaryKey> constraints;
};
struct DomainStats final : DomainIndexStats {
  bgcdt::CoreStats cdt;
};
enum class DomainFailureMode { reject, report_geometry };
// Deferred construction still admits every CURRENT site through its cycles.
// The same topology owner may then append finite mapped cells before one seal.
enum class DomainInitialSeal { complete, deferred };
enum class DomainIssueKind { too_few_sites, zero_area, boundary_contact, hole_placement };
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
                                                   DomainFailureMode = DomainFailureMode::reject,
                                                   DomainInitialSeal = DomainInitialSeal::complete);
} // namespace bgmesh
#endif
