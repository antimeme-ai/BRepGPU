#ifndef BREPGPU_MESHER_FACE_SEED_HPP
#define BREPGPU_MESHER_FACE_SEED_HPP
#include "geometry_charts.hpp"
#include "mesh_values.hpp"
#include "topology.hpp"

namespace bgmesh {
// Transport values, not an additional domain certificate. The producer retains
// its source-to-selected-domain correspondence in the same immutable snapshot.
// All ExactRefs below address values; no owner-local Rational is retained.
struct FaceBoundarySite final {
  std::array<ExactRef, 2> chart, original_uv;
  // If original_parameter is present, it also addresses this snapshot.values.
  PhysicalNode node;
};
struct FaceBoundarySegment final {
  uint32_t site;
  BoundaryKey constraint;
  Slot<CoedgeTag> use;
  // ORIGINAL spatial-edge t endpoints, before use.parameter_map to pcurve s.
  std::array<ExactRef, 2> source_parameter;
  std::array<int32_t, 2> periodic_shift;
};
struct FaceBoundaryLoop final {
  Slot<LoopTag> source_loop;
  bool outer;
  // ORIGINAL source-use traversal. Normalization returns a separate +/-1
  // traversal adapter; neither this order nor original USES.sense is rewritten.
  Range<FaceBoundarySegment> segments;
};
struct FrozenPlaneChart final {
  std::array<ExactRef, 4> natural_rectangle;
  std::array<ExactRef, 3> origin;
  std::array<ExactRef, 6> basis, projection;
  std::array<ExactRef, 8> quadrilateral;
  std::array<ExactRef, 4> preconditioner;
  ExactRef height_upper;
};
struct FaceBoundarySnapshotData final {
  std::shared_ptr<RunResource> resource;
  GeometryStore geometry;
  Slot<DefinitionTag> definition;
  Slot<FaceTag> face;
  uint64_t source_generation, face_generation, boundary_generation;
  WorkingChartKind chart_kind;
  ExactTable values;
  std::optional<FrozenPlaneChart> plane;
  std::pmr::vector<FaceBoundarySite> sites;
  std::pmr::vector<FaceBoundarySegment> segments;
  std::pmr::vector<FaceBoundaryLoop> loops;

  FaceBoundarySnapshotData(std::shared_ptr<RunResource> owner, GeometryStore store,
                           Slot<DefinitionTag> definition_slot, Slot<FaceTag> face_slot,
                           std::array<uint64_t, 3> generations, WorkingChartKind kind,
                           ExactTable table)
      : resource(std::move(owner)), geometry(std::move(store)), definition(definition_slot),
        face(face_slot), source_generation(generations[0]), face_generation(generations[1]),
        boundary_generation(generations[2]), chart_kind(kind), values(std::move(table)),
        sites(resource.get()), segments(resource.get()), loops(resource.get()) {}
  FaceBoundarySnapshotData(const FaceBoundarySnapshotData &) = delete;
  FaceBoundarySnapshotData &operator=(const FaceBoundarySnapshotData &) = delete;
};
// The geometry producer builds once and releases every mutable alias before
// publishing this lease. A face generation retains it through queued work.
using FaceBoundarySnapshot = std::shared_ptr<const FaceBoundarySnapshotData>;
} // namespace bgmesh
#endif
