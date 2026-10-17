#ifndef BREPGPU_MESHER_TOPOLOGY_HPP
#define BREPGPU_MESHER_TOPOLOGY_HPP
#include "exact.hpp"
#include "values.hpp"
#include <array>
#include <cstdint>
#include <memory_resource>
#include <optional>
#include <vector>

namespace bgmesh {
struct MeshSiteTag;
using SiteSlot = Slot<MeshSiteTag>;
struct TriangleHandle final {
  uint32_t slot = UINT32_MAX;
  uint64_t generation = 0;
  auto operator<=>(const TriangleHandle &) const = default;
};
struct EdgeHandle final {
  uint32_t slot = UINT32_MAX;
  uint64_t generation = 0;
  auto operator<=>(const EdgeHandle &) const = default;
};
enum class CoordinateAxis { x, y };
class MutableFaceTopology;
class HeuristicTicket final {
  const void *owner_;
  uint64_t serial_;
  HeuristicTicket(const void *owner, uint64_t serial) : owner_(owner), serial_(serial) {}
  friend class MutableFaceTopology;
};
enum class BoundaryOwnerKind { source_edge, face_cut };
class FlipTicket final {
  const void *owner_;
  uint64_t serial_;
  FlipTicket(const void *owner, uint64_t serial) : owner_(owner), serial_(serial) {}
  friend class MutableFaceTopology;
};
struct BoundaryKey final {
  uint64_t owner = 0, interval = 0;
  BoundaryOwnerKind kind = BoundaryOwnerKind::source_edge;
  auto operator<=>(const BoundaryKey &) const = default;
};
struct TriangleView final {
  std::array<SiteSlot, 3> sites;
  std::array<EdgeHandle, 3> edges;
  std::array<std::optional<TriangleHandle>, 3> neighbors;
  uint64_t lineage;
  bool heuristic_spent, accepted;
};
struct EdgeView final {
  std::array<SiteSlot, 2> sites;
  std::array<TriangleHandle, 2> incidents;
  uint32_t incident_count;
  std::optional<BoundaryKey> constraint;
};
enum class LineageOperation { initial, heuristic, bisection, flip };
struct TriangleLineage final {
  std::array<SiteSlot, 3> sites;
  uint64_t parent = UINT64_MAX;
  LineageOperation operation = LineageOperation::initial;
  bool heuristic_spent = false;
  uint64_t secondary_parent = UINT64_MAX;
};
struct TopologyStats final {
  uint64_t sites = 0;
  uint64_t orientations = 0, filtered_orientations = 0, exact_orientations = 0;
  uint64_t edge_probes = 0, walk_steps = 0, edge_splits = 0, interior_splits = 0;
  uint64_t live_triangles = 0, live_edges = 0, triangle_peak = 0, edge_peak = 0;
};
enum class RefineKind { target_bisected, boundary_request };
struct RefineResult final {
  RefineKind kind;
  std::optional<EdgeHandle> boundary;
  uint64_t splits;
};
struct TopologyEdit final {
  std::array<TriangleHandle, 2> removed{};
  std::array<TriangleHandle, 4> inserted{};
  uint32_t removed_count = 0, inserted_count = 0;
  std::optional<SiteSlot> new_site;
};

// One owner and one Budget for the generation. No method is thread safe.
// Rejected geometry leaves connectivity intact. A failure during commit poisons
// the generation, whose only permitted subsequent operations are destruction
// and is_usable(); no partial mutable mesh can be published.
class MutableFaceTopology final {
  struct Impl;
  bgfd::Budget *budget_;
  Impl *impl_;

public:
  explicit MutableFaceTopology(bgfd::Budget &, std::array<double, 4> transform = {1, 0, 0, 1});
  MutableFaceTopology(bgfd::Budget &, const std::array<Rational, 4> &transform);
  ~MutableFaceTopology();
  MutableFaceTopology(const MutableFaceTopology &) = delete;
  MutableFaceTopology &operator=(const MutableFaceTopology &) = delete;
  MutableFaceTopology(MutableFaceTopology &&) = delete;
  MutableFaceTopology &operator=(MutableFaceTopology &&) = delete;
  [[nodiscard]] bool is_usable() const noexcept;
  [[nodiscard]] SiteSlot append_site(std::array<double, 2>);
  [[nodiscard]] SiteSlot append_site(const std::array<Rational, 2> &);
  [[nodiscard]] TriangleHandle add_initial_triangle(std::array<SiteSlot, 3>);
  void seal_initial();
  [[nodiscard]] bool is_live(TriangleHandle) const;
  [[nodiscard]] bool is_live(EdgeHandle) const;
  [[nodiscard]] std::pmr::vector<EdgeHandle> internal_edges() const;
  [[nodiscard]] TriangleView triangle(TriangleHandle) const;
  [[nodiscard]] EdgeView edge(EdgeHandle) const;
  [[nodiscard]] std::optional<EdgeHandle> find_edge(SiteSlot, SiteSlot) const;
  [[nodiscard]] std::pmr::vector<TriangleHandle> triangles() const;
  [[nodiscard]] const Rational &coordinate(SiteSlot, CoordinateAxis) const;
  [[nodiscard]] const Rational &metric_coordinate(SiteSlot, CoordinateAxis) const;
  [[nodiscard]] const Rational &diameter_squared(TriangleHandle) const;
  [[nodiscard]] int compare_axis(SiteSlot, SiteSlot, CoordinateAxis) const;
  [[nodiscard]] int orient(std::array<SiteSlot, 3>) const;
  [[nodiscard]] const TriangleLineage &lineage(uint64_t) const;
  [[nodiscard]] uint64_t lineage_count() const;
  [[nodiscard]] TopologyStats stats() const;
  [[nodiscard]] std::pmr::vector<TopologyEdit> take_changes();
  void protect_edge(EdgeHandle, BoundaryKey);
  void mark_accepted(TriangleHandle);
  [[nodiscard]] std::optional<SiteSlot> insert_heuristic(TriangleHandle, std::array<double, 2>);
  [[nodiscard]] std::optional<SiteSlot> insert_heuristic(TriangleHandle,
                                                         const std::array<Rational, 2> &);
  // A ticket spends the attempt but publishes no site/triangle. The owner may
  // evaluate canonical XYZ before commit. Cancel preserves the sticky flag.
  [[nodiscard]] std::optional<HeuristicTicket> stage_heuristic(TriangleHandle,
                                                               std::array<double, 2>);
  [[nodiscard]] std::optional<HeuristicTicket> stage_heuristic(TriangleHandle,
                                                               const std::array<Rational, 2> &);
  [[nodiscard]] const Rational &heuristic_coordinate(HeuristicTicket, CoordinateAxis) const;
  [[nodiscard]] SiteSlot commit_heuristic(HeuristicTicket);
  void cancel_heuristic(HeuristicTicket);
  [[nodiscard]] SiteSlot bisect_internal_edge(EdgeHandle);
  // LEPP selection only: lets the same owner stage canonical XYZ before commit.
  [[nodiscard]] EdgeHandle next_terminal(TriangleHandle);
  [[nodiscard]] std::optional<FlipTicket> stage_flip(EdgeHandle);
  [[nodiscard]] std::array<std::array<SiteSlot, 3>, 2> flip_sites(FlipTicket) const;
  [[nodiscard]] std::array<TriangleHandle, 2> flip_cells(FlipTicket) const;
  void cancel_flip(FlipTicket);
  void commit_flip(FlipTicket);
  [[nodiscard]] RefineResult refine_longest(TriangleHandle);
};
} // namespace bgmesh
#endif
