#ifndef BREPGPU_MESHER_BOUNDARY_HPP
#define BREPGPU_MESHER_BOUNDARY_HPP
#include "exact_store.hpp"
#include "mesh_values.hpp"
#include "source.hpp"
#include "topology.hpp"
#include <memory>
#include <optional>
#include <span>

namespace bgmesh {
struct BoundarySampleTag;
using BoundarySampleSlot = Slot<BoundarySampleTag>;
struct BoundaryEpoch final {
  uint64_t value = 0;
  auto operator<=>(const BoundaryEpoch &) const = default;
};
struct BoundaryEvaluationTask final {
  BoundaryEpoch epoch;
  BoundarySampleSlot sample;
  Slot<CurveTag> curve;
  ExactRef parameter;
  auto operator<=>(const BoundaryEvaluationTask &) const = default;
};
struct BoundaryEvaluationResult final {
  BoundaryEvaluationTask task;
  uint32_t dimension;
  Vec3 xyz;
};
struct BoundaryEvaluationData final {
  std::shared_ptr<RunResource> resource;
  BoundaryEpoch epoch;
  SourceStore source;
  Slot<DefinitionTag> definition;
  ExactTable parameters;
  std::pmr::vector<BoundaryEvaluationTask> tasks;
  BoundaryEvaluationData(std::shared_ptr<RunResource>, BoundaryEpoch, SourceStore,
                         Slot<DefinitionTag>, ExactTable, std::pmr::vector<BoundaryEvaluationTask>);
};
class BoundaryEvaluationBatch final {
  std::shared_ptr<const BoundaryEvaluationData> data_;
  friend class DefinitionBoundaryForest;
  explicit BoundaryEvaluationBatch(std::shared_ptr<const BoundaryEvaluationData> data)
      : data_(std::move(data)) {}

public:
  [[nodiscard]] BoundaryEpoch epoch() const noexcept { return data_->epoch; }
  [[nodiscard]] const SourceStore &source() const noexcept { return data_->source; }
  [[nodiscard]] Slot<DefinitionTag> definition() const noexcept { return data_->definition; }
  [[nodiscard]] const ExactTable &parameters() const noexcept { return data_->parameters; }
  [[nodiscard]] std::span<const BoundaryEvaluationTask> tasks() const noexcept {
    return data_->tasks;
  }
};
struct BoundarySampleView final {
  PhysicalNodeKey key;
  Vec3 xyz;
};
struct BoundaryIntervalView final {
  Slot<EdgeTag> edge;
  std::array<BoundarySampleSlot, 2> samples;
  bool leaf;
};
struct FaceBoundaryGeneration final {
  Slot<FaceTag> face;
  uint64_t old_generation, generation;
};
struct BoundaryUpdate final {
  std::pmr::vector<FaceBoundaryGeneration> changed_faces;
  std::pmr::vector<BoundarySampleSlot> new_samples;
  std::pmr::vector<BoundaryKey> removed_intervals, inserted_intervals;
  explicit BoundaryUpdate(std::pmr::memory_resource *resource)
      : changed_faces(resource), new_samples(resource), removed_intervals(resource),
        inserted_intervals(resource) {}
};
struct SourceBoundaryEdge final {
  Slot<EdgeTag> edge;
  // Literal Edge.parameter_range[0] -> [1] direction, not curve_same_sense.
  Range<PhysicalNode> nodes;
  Range<BoundaryKey> intervals;
};
struct SourceBoundarySnapshotData final {
  std::shared_ptr<RunResource> resource;
  SourceStore source;
  Slot<DefinitionTag> definition;
  Slot<FaceTag> face;
  BoundaryEpoch epoch;
  uint64_t boundary_generation;
  ExactTable parameters;
  std::pmr::vector<SourceBoundaryEdge> edges;
  std::pmr::vector<PhysicalNode> nodes;
  std::pmr::vector<BoundaryKey> intervals;
  SourceBoundarySnapshotData(std::shared_ptr<RunResource> owner, SourceStore original,
                             Slot<DefinitionTag> definition_slot, Slot<FaceTag> face_slot,
                             BoundaryEpoch identity, uint64_t generation, ExactTable table)
      : resource(std::move(owner)), source(std::move(original)), definition(definition_slot),
        face(face_slot), epoch(identity), boundary_generation(generation),
        parameters(std::move(table)), edges(resource.get()), nodes(resource.get()),
        intervals(resource.get()) {}
  SourceBoundarySnapshotData(const SourceBoundarySnapshotData &) = delete;
  SourceBoundarySnapshotData &operator=(const SourceBoundarySnapshotData &) = delete;
};
using SourceBoundarySnapshot = std::shared_ptr<const SourceBoundarySnapshotData>;
struct BoundaryStats final {
  uint64_t epochs = 0, requests = 0, splits = 0, invalidated_faces = 0, samples = 0,
           live_intervals = 0;
};
// Sole mutable owner for one definition. An evaluation batch retains sealed
// parameter words and may be read by child jobs; no Rational/Budget crosses
// owners. Stale epoch/generation results never contribute to a current horizon.
class DefinitionBoundaryForest final {
  struct Impl;
  bgfd::Budget *budget_;
  Impl *impl_;

public:
  DefinitionBoundaryForest(bgfd::Budget &, SourceStore, Slot<DefinitionTag>,
                           std::shared_ptr<RunResource>);
  ~DefinitionBoundaryForest();
  DefinitionBoundaryForest(const DefinitionBoundaryForest &) = delete;
  DefinitionBoundaryForest &operator=(const DefinitionBoundaryForest &) = delete;
  [[nodiscard]] std::optional<BoundaryEvaluationBatch> pending_evaluations() const;
  [[nodiscard]] BoundaryUpdate complete_evaluations(BoundaryEpoch,
                                                    std::span<const BoundaryEvaluationResult>);
  [[nodiscard]] BoundaryEpoch begin_epoch();
  void arrive(BoundaryEpoch, Slot<FaceTag>, uint64_t generation,
              std::span<const BoundaryKey> requests);
  [[nodiscard]] bool horizon_complete(BoundaryEpoch) const;
  [[nodiscard]] BoundaryEvaluationBatch prepare_splits(BoundaryEpoch);
  [[nodiscard]] uint64_t face_generation(Slot<FaceTag>) const;
  [[nodiscard]] std::pmr::vector<BoundarySampleSlot> chain(Slot<EdgeTag>) const;
  [[nodiscard]] std::pmr::vector<BoundaryKey> intervals(Slot<EdgeTag>) const;
  [[nodiscard]] BoundaryIntervalView interval(BoundaryKey) const;
  [[nodiscard]] BoundarySampleView sample(BoundarySampleSlot) const;
  [[nodiscard]] const Rational &parameter(BoundarySampleSlot) const;
  [[nodiscard]] BoundaryStats stats() const;
  [[nodiscard]] SourceBoundarySnapshot freeze_face_boundary(Slot<FaceTag>) const;
};
} // namespace bgmesh
#endif
