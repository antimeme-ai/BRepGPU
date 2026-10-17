#ifndef BREPGPU_MESHER_CONTACT_HPP
#define BREPGPU_MESHER_CONTACT_HPP
#include "cuda/bound_api.h"
#include "exact.hpp"
#include "mesh_values.hpp"
#include <array>
#include <memory_resource>
#include <span>
#include <vector>

namespace bgmesh {
struct ContactFacet final {
  uint32_t definition;
  std::array<NodeSlot, 3> nodes;
  std::array<Vec3, 3> xyz;
};
enum class SimplexDimension { none, vertex, edge };
struct AllowedSimplex final {
  SimplexDimension dimension = SimplexDimension::none;
  std::array<NodeSlot, 2> nodes{};
};
enum class ContactDimension { empty, point, segment, area };
struct FacetContact final {
  ContactDimension dimension;
  bool permitted;
};
struct ContactPredicateStats final {
  uint64_t filtered_separation = 0, filtered_shared_edge = 0;
  uint64_t filtered_shared_vertex = 0, filtered_projected = 0, exact_pairs = 0;
};
// Retain one immutable preparation per actual facet revision. Its enclosures
// certify these same binary64 XYZ words, independently of pair-query order.
class PreparedContactFacet final {
  ContactFacet facet_;
  std::array<std::array<bgmb_interval, 3>, 3> edges_{};
  std::array<bgmb_interval, 3> normal_{};
  bool filtered_ = false;
  PreparedContactFacet() = default;
  friend PreparedContactFacet prepare_contact_facet(bgfd::Budget &, const ContactFacet &);
  friend FacetContact classify_contact(bgfd::Budget &, const PreparedContactFacet &,
                                       const PreparedContactFacet &, AllowedSimplex,
                                       ContactPredicateStats &);

public:
  [[nodiscard]] const ContactFacet &facet() const noexcept { return facet_; }
};
[[nodiscard]] PreparedContactFacet prepare_contact_facet(bgfd::Budget &, const ContactFacet &);
[[nodiscard]] FacetContact classify_contact(bgfd::Budget &, const PreparedContactFacet &,
                                            const PreparedContactFacet &, AllowedSimplex,
                                            ContactPredicateStats &);
[[nodiscard]] FacetContact classify_contact(bgfd::Budget &, const ContactFacet &,
                                            const ContactFacet &, AllowedSimplex);
struct FacetBox final {
  FacetHandle facet;
  Vec3 lower, upper;
};
[[nodiscard]] FacetBox facet_box(bgfd::Budget &, FacetHandle, const ContactFacet &);
[[nodiscard]] FacetBox facet_box(FacetHandle, const PreparedContactFacet &);
struct BVHStats final {
  uint64_t live_facets = 0, box_tests = 0, visits = 0, rebuilds = 0, updates = 0;
  uint64_t exact_costs = 0;
};
// One definition owner; query results are canonical handle order. Broadphase
// boxes are closed and never imply permission for a physical contact.
class FacetBVH final {
  struct Impl;
  bgfd::Budget *budget_;
  Impl *impl_;

public:
  explicit FacetBVH(bgfd::Budget &);
  ~FacetBVH();
  FacetBVH(const FacetBVH &) = delete;
  FacetBVH &operator=(const FacetBVH &) = delete;
  void build(std::span<const FacetBox>);
  void insert(const FacetBox &);
  void erase(FacetHandle);
  [[nodiscard]] std::pmr::vector<FacetHandle> query(const FacetBox &) const;
  [[nodiscard]] BVHStats stats() const;
};
} // namespace bgmesh
#endif
