#ifndef BREPGPU_MESHER_FACE_SEED_HPP
#define BREPGPU_MESHER_FACE_SEED_HPP
#include "geometry_charts.hpp"
#include "geometry_correspondence.hpp"
#include "mesh_values.hpp"
#include "topology.hpp"

namespace bgmesh {
struct BoundaryProtectionData;
struct DomainMapData;
// Transport values, not an additional domain certificate. The producer retains
// its source-to-selected-domain correspondence in the same immutable snapshot.
// All ExactRefs below address values; no owner-local Rational is retained.
struct FaceBoundarySite final {
  std::array<ExactRef, 2> chart, original_uv;
  // If original_parameter is present, it also addresses this snapshot.values.
  PhysicalNode node;
  // Plane inverse enclosure; ordinary UV is already an exact source parameter.
  std::optional<std::array<ExactRef, 4>> original_uv_enclosure{};
};
struct FaceBoundarySegment final {
  uint32_t site;
  BoundaryKey constraint;
  Slot<CoedgeTag> use;
  // ORIGINAL spatial-edge t endpoints, before use.parameter_map to pcurve s.
  std::array<ExactRef, 2> source_parameter;
  std::array<int32_t, 2> periodic_shift;
};
struct FrozenGeometricLineMap final {
  std::array<ExactRef, 3> covector;
  std::array<ExactRef, 2> curve_scalar, trace_scalar;
};
struct FaceTraceMap final {
  TraceCorrespondence kind;
  // Original mapped pcurve s endpoints; raw source curve/map/lift remains in
  // retained SourceStore, including authoritative or reconstructed provenance.
  std::array<ExactRef, 2> mapped_pcurve_parameter;
  std::optional<FrozenGeometricLineMap> line;
  // Canonical host source-edge/image versus named reference trace upper.
  // Absent only in authored integration transports; never a retry witness.
  std::optional<ExactRef> trace_position_upper{};
};
enum class FaceUseKind { ordinary_chain, declared_quotient };
struct FaceUseDisposition final {
  Slot<CoedgeTag> source_use;
  FaceUseKind kind;
  Range<FaceBoundarySegment> segments;
  std::array<ExactRef, 2> original_parameter;
};
struct FaceBoundaryLoop final {
  Slot<LoopTag> source_loop;
  bool outer;
  // ORIGINAL source-use traversal. Normalization returns a separate +/-1
  // traversal adapter; neither this order nor original USES.sense is rewritten.
  Range<FaceBoundarySegment> segments;
  Range<FaceUseDisposition> uses;
};
struct FrozenPlaneChart final {
  std::array<ExactRef, 4> natural_rectangle;
  std::array<ExactRef, 3> origin;
  std::array<ExactRef, 6> basis, projection;
  std::array<ExactRef, 8> quadrilateral;
  std::array<ExactRef, 4> preconditioner;
  ExactRef height_upper;
  ExactRef inverse_monotonicity_lower;
};
struct FaceBoundarySnapshotData final {
  std::shared_ptr<RunResource> resource;
  GeometryStore geometry;
  Slot<DefinitionTag> definition;
  Slot<FaceTag> face;
  uint64_t source_generation, face_generation, boundary_generation;
  WorkingChartKind chart_kind;
  ExactTable values;
  // NURBS true natural rectangle; finite prepared source envelope for analytic
  // carriers. All values address this snapshot.values table.
  std::array<ExactRef, 4> prepared_original_rectangle{};
  std::optional<FrozenPlaneChart> plane;
  std::shared_ptr<const BoundaryProtectionData> protection;
  std::shared_ptr<const DomainMapData> domain_map;
  std::pmr::vector<FaceBoundarySite> sites;
  std::pmr::vector<FaceBoundarySegment> segments;
  std::pmr::vector<FaceBoundaryLoop> loops;
  std::pmr::vector<FaceUseDisposition> uses;
  // One finite selected correspondence per segment in identical index order.
  std::pmr::vector<FaceTraceMap> trace_maps;

  FaceBoundarySnapshotData(std::shared_ptr<RunResource> owner, GeometryStore store,
                           Slot<DefinitionTag> definition_slot, Slot<FaceTag> face_slot,
                           std::array<uint64_t, 3> generations, WorkingChartKind kind,
                           ExactTable table)
      : resource(std::move(owner)), geometry(std::move(store)), definition(definition_slot),
        face(face_slot), source_generation(generations[0]), face_generation(generations[1]),
        boundary_generation(generations[2]), chart_kind(kind), values(std::move(table)),
        sites(resource.get()), segments(resource.get()), loops(resource.get()),
        uses(resource.get()), trace_maps(resource.get()) {}
  FaceBoundarySnapshotData(const FaceBoundarySnapshotData &) = delete;
  FaceBoundarySnapshotData &operator=(const FaceBoundarySnapshotData &) = delete;
};
// The geometry producer builds once and releases every mutable alias before
// publishing this lease. A face generation retains it through queued work.
using FaceBoundarySnapshot = std::shared_ptr<const FaceBoundarySnapshotData>;
} // namespace bgmesh
#endif
