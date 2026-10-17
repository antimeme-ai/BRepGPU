#ifndef BREPGPU_MESHER_FACE_FRONTIER_HPP
#define BREPGPU_MESHER_FACE_FRONTIER_HPP
#include "exact_store.hpp"
#include "topology.hpp"
#include <optional>
#include <span>

namespace bgmesh {
struct FaceFrontierIdentity final {
  OwnerKey owner;
  uint64_t face_generation, boundary_generation;
  auto operator<=>(const FaceFrontierIdentity &) const = default;
};
struct CellResultKey final {
  FaceFrontierIdentity face;
  TriangleHandle triangle;
  uint32_t request_revision;
  auto operator<=>(const CellResultKey &) const = default;
};
enum class CellNumericalState { pending, accepted, needs_refine, unresolved };
struct CellFront final {
  CellResultKey key;
  CellNumericalState state;
  bool contact_failure;
};
struct FaceFrontierStats final {
  uint64_t tracked_cells = 0, heap_cells = 0, comparisons = 0, stale_results = 0,
           edit_transactions = 0;
};
// Geometric priority/state only, never a worker executor or connectivity owner.
// Consumes the SAME topology's ordered edit stream after a local transaction.
class FaceCellFrontier final {
  struct Impl;
  bgfd::Budget *budget_;
  Impl *impl_;

public:
  FaceCellFrontier(bgfd::Budget &, MutableFaceTopology &, FaceFrontierIdentity);
  ~FaceCellFrontier();
  FaceCellFrontier(const FaceCellFrontier &) = delete;
  FaceCellFrontier &operator=(const FaceCellFrontier &) = delete;
  void initialize();
  [[nodiscard]] CellResultKey key(TriangleHandle) const;
  [[nodiscard]] bool complete(CellResultKey, CellNumericalState);
  [[nodiscard]] CellResultKey retry(TriangleHandle);
  [[nodiscard]] bool mark_contact_failure(CellResultKey);
  [[nodiscard]] std::optional<CellFront> next();
  [[nodiscard]] std::pmr::vector<CellResultKey> take_settled();
  [[nodiscard]] std::pmr::vector<CellResultKey> take_pending(uint32_t maximum);
  void apply_changes(std::span<const TopologyEdit>);
  [[nodiscard]] FaceFrontierStats stats() const;
};
} // namespace bgmesh
#endif
