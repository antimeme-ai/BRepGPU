#ifndef BREPGPU_MESHER_DOMAIN_INDEX_HPP
#define BREPGPU_MESHER_DOMAIN_INDEX_HPP
#include "exact.hpp"
#include "values.hpp"
#include <array>
#include <memory_resource>
#include <optional>
#include <span>
namespace bgmesh {
struct MeshSiteTag;
using SiteSlot = Slot<MeshSiteTag>;
struct DomainSegmentLocation final {
  uint32_t loop;
  uint64_t segment;
};
struct DomainBox final {
  std::array<double, 2> lo{}, hi{};
};
struct DomainSegment final {
  SiteSlot a, b;
  DomainSegmentLocation original;
  DomainBox box;
};
struct DomainIndexStats {
  uint64_t bvh_visits = 0, segment_candidates = 0, ray_candidates = 0, ray_queries = 0;
  uint64_t winding_aggregates = 0, event_entries = 0;
};
using DomainPointRef = std::array<const Rational *, 2>;
class DomainSegmentIndex final {
  struct Impl;
  bgfd::Budget *budget_;
  Impl *impl_;

public:
  DomainSegmentIndex(bgfd::Budget &, std::span<const DomainPointRef>, DomainIndexStats &,
                     std::span<const DomainSegment>);
  ~DomainSegmentIndex();
  DomainSegmentIndex(const DomainSegmentIndex &) = delete;
  DomainSegmentIndex &operator=(const DomainSegmentIndex &) = delete;
  [[nodiscard]] std::optional<std::array<uint32_t, 2>> crossing();
  [[nodiscard]] int64_t winding_except(SiteSlot, std::span<const DomainSegment> own);
  [[nodiscard]] int64_t winding(DomainPointRef);
  [[nodiscard]] bool boundary_contact(DomainPointRef);
  [[nodiscard]] bool touches_rectangle(std::span<const Rational, 4>);
};
[[nodiscard]] DomainBox domain_point_box(DomainPointRef);
[[nodiscard]] DomainBox domain_segment_box(DomainPointRef, DomainPointRef);
} // namespace bgmesh
#endif
