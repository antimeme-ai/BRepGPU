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
  uint64_t constructed_revision = 0;
};
struct BoundaryPointRevision final {
  BoundarySampleSlot sample;
  PhysicalNodeKey expected_key;
  uint64_t expected_constructed_revision;
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
  std::pmr::vector<BoundarySampleSlot> new_samples, revised_samples;
  std::pmr::vector<BoundaryKey> removed_intervals, inserted_intervals;
  explicit BoundaryUpdate(std::pmr::memory_resource *resource)
      : changed_faces(resource), new_samples(resource), revised_samples(resource),
        removed_intervals(resource), inserted_intervals(resource) {}
};
struct SourceBoundaryEdge final {
  Slot<EdgeTag> edge;
  // Literal Edge.parameter_range[0] -> [1] direction, not curve_same_sense.
  Range<PhysicalNode> nodes;
  Range<BoundaryKey> intervals;
};
// Only explicit declared-degenerate source edges join vertices. Every member
// of a distinct-ID component is retained; the minimum source VertexID provides
// physical identity and original XYZ, without erasing any original endpoint.
struct SourceVertexAlias final {
  Slot<VertexTag> original, representative;
};
struct SourceVertexAliasData final {
  std::shared_ptr<RunResource> resource;
  std::pmr::vector<SourceVertexAlias> rows;
  SourceVertexAliasData(std::shared_ptr<RunResource> owner,
                        std::span<const SourceVertexAlias> aliases)
      : resource(std::move(owner)), rows(aliases.begin(), aliases.end(), resource.get()) {}
};
struct DefinitionBoundarySnapshotData {
  std::shared_ptr<RunResource> resource;
  SourceStore source;
  Slot<DefinitionTag> definition;
  BoundaryEpoch epoch;
  ExactTable parameters;
  std::pmr::vector<SourceBoundaryEdge> edges;
  std::pmr::vector<PhysicalNode> nodes;
  std::pmr::vector<uint64_t> constructed_revisions;
  std::pmr::vector<BoundaryKey> intervals;
  // One range per nodes row. Directions are literal root first/second child,
  // from that original root to the interval whose midpoint created the sample.
  std::pmr::vector<Range<uint8_t>> sample_paths;
  std::pmr::vector<uint8_t> path_bits;
  // Sorted by original VertexSlot, independent of exported edge subset.
  std::shared_ptr<const SourceVertexAliasData> vertex_alias_owner;
  std::span<const SourceVertexAlias> vertex_aliases;
  DefinitionBoundarySnapshotData(std::shared_ptr<RunResource> owner, SourceStore original,
                                 Slot<DefinitionTag> definition_slot, BoundaryEpoch identity,
                                 ExactTable table)
      : resource(std::move(owner)), source(std::move(original)), definition(definition_slot),
        epoch(identity), parameters(std::move(table)), edges(resource.get()), nodes(resource.get()),
        constructed_revisions(resource.get()), intervals(resource.get()),
        sample_paths(resource.get()), path_bits(resource.get()) {}
  DefinitionBoundarySnapshotData(const DefinitionBoundarySnapshotData &) = delete;
  DefinitionBoundarySnapshotData &operator=(const DefinitionBoundarySnapshotData &) = delete;
  DefinitionBoundarySnapshotData(DefinitionBoundarySnapshotData &&) = default;
};
using DefinitionBoundarySnapshot = std::shared_ptr<const DefinitionBoundarySnapshotData>;
struct SourceBoundarySnapshotData final : DefinitionBoundarySnapshotData {
  Slot<FaceTag> face;
  uint64_t boundary_generation;
  SourceBoundarySnapshotData(DefinitionBoundarySnapshotData &&rows, Slot<FaceTag> original_face,
                             uint64_t generation)
      : DefinitionBoundarySnapshotData(std::move(rows)), face(original_face),
        boundary_generation(generation) {}
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
  [[nodiscard]] std::shared_ptr<DefinitionBoundarySnapshotData>
      freeze_rows(std::span<const Slot<EdgeTag>>) const;

public:
  DefinitionBoundaryForest(bgfd::Budget &, SourceStore, Slot<DefinitionTag>,
                           std::shared_ptr<RunResource>, std::optional<OwnerKey> table_owner = {});
  ~DefinitionBoundaryForest();
  DefinitionBoundaryForest(const DefinitionBoundaryForest &) = delete;
  DefinitionBoundaryForest &operator=(const DefinitionBoundaryForest &) = delete;
  [[nodiscard]] std::optional<BoundaryEvaluationBatch> pending_evaluations() const;
  [[nodiscard]] BoundaryUpdate complete_evaluations(BoundaryEpoch,
                                                    std::span<const BoundaryEvaluationResult>);
  [[nodiscard]] BoundaryEpoch begin_epoch();
  void arrive(BoundaryEpoch, Slot<FaceTag>, uint64_t generation,
              std::span<const BoundaryKey> requests);
  // Owner-origin failures also cover wires; every current face still arrives.
  void request_owner_splits(BoundaryEpoch, std::span<const BoundaryKey>);
  // Staged only. The same complete_evaluations commits revisions plus splits.
  void stage_owner_promotions(BoundaryEpoch, std::span<const BoundaryPointRevision>);
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
  [[nodiscard]] DefinitionBoundarySnapshot freeze_definition_boundary() const;
  [[nodiscard]] std::pmr::vector<uint8_t> sample_path(Slot<EdgeTag>, BoundarySampleSlot) const;
};
} // namespace bgmesh
#endif
