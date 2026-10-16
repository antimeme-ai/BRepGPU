#ifndef BREPGPU_MESHER_CELL_BOUNDS_HPP
#define BREPGPU_MESHER_CELL_BOUNDS_HPP
#include "bound_runtime.hpp"
#include "geometry_jets.hpp"
#include "mesh_values.hpp"

namespace bgmesh {
// The three XYZ triples have exactly the same positive original-UV role order
// as original_uv, independently of emitted facet winding. Allowance is the
// remaining definition-local positional allowance; actual departure is already
// included by comparing this same affine output map, not charged a second time.
struct ActualTriangleView final {
  BoundTaskHeader header;
  Slot<CarrierTag> carrier;
  CellMapKind map_kind;
  std::span<const Rational, 6> original_uv;
  std::span<const Rational, 9> actual_xyz;
  const Rational &allowance;
};
struct ActualCellLimits final {
  uint32_t max_pieces;
  BoundBatchLimits batches;
};
struct CellPieceRange final {
  uint32_t first, count;
};
struct CellPreparationStats final {
  uint64_t column_entries = 0, positive_columns = 0, root_candidates = 0;
};
struct CellBoundResult final {
  BoundTaskHeader header;
  BoundDecision decision;
};
struct PreparedCellData;
class PreparedCellBounds final {
  std::shared_ptr<const PreparedCellData> data_;
  explicit PreparedCellBounds(std::shared_ptr<const PreparedCellData> data)
      : data_(std::move(data)) {}
  friend PreparedCellBounds prepare_actual_cell(bgfd::Budget &,
                                                const std::shared_ptr<RunResource> &, BoundRootBank,
                                                ActualTriangleView, ExactTableKey,
                                                ActualCellLimits);
  friend class CellBoundFold;

public:
  [[nodiscard]] const BoundTaskHeader &parent() const noexcept;
  [[nodiscard]] const ExactTable &values() const noexcept;
  [[nodiscard]] std::span<const SurfaceBoundInput> pieces() const noexcept;
  [[nodiscard]] CellPreparationStats stats() const noexcept;
  [[nodiscard]] BoundBatch batch(CellPieceRange) const;
};
[[nodiscard]] PreparedCellBounds prepare_actual_cell(bgfd::Budget &,
                                                     const std::shared_ptr<RunResource> &,
                                                     BoundRootBank, ActualTriangleView,
                                                     ExactTableKey, ActualCellLimits);
[[nodiscard]] CellBoundResult bound_analytic_cell(GeometryContext &, const GeometryStore &,
                                                  ActualTriangleView, GeometryBoundPrecision = {});
class CellBoundFold final {
  std::shared_ptr<RunResource> resource_;
  PreparedCellBounds prepared_;
  std::pmr::vector<uint8_t> seen_;
  uint32_t completed_ = 0;
  bool failed_ = false;

public:
  CellBoundFold(const std::shared_ptr<RunResource> &, PreparedCellBounds);
  void add(const BoundCompleted &);
  [[nodiscard]] bool complete() const noexcept;
  [[nodiscard]] CellBoundResult result() const;
};
} // namespace bgmesh
#endif
