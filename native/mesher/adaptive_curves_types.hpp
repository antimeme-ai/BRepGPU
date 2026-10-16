#ifndef BREPGPU_MESHER_ADAPTIVE_CURVES_TYPES_HPP
#define BREPGPU_MESHER_ADAPTIVE_CURVES_TYPES_HPP
#include "adaptive_curve_types.h"
#include "boundary.hpp"
#include <array>
#include <memory>
#include <optional>

namespace bgmesh::adaptive {
enum class CurveTraceAuthority : uint32_t {
  reconstructed_selected = 1,
  authoritative_original = 2
};
struct GeometricLineRecord final {
  std::array<ExactRef, 3> covector;
  std::array<ExactRef, 2> curve_scalar, trace_scalar, original_t;
  int32_t traversal = 1;
};
struct CylinderAffineIdentityRecord final {
  Slot<CurveTag> spatial;
  Slot<CarrierTag> carrier;
  std::array<ExactRef, 2> original_t;
  std::array<ExactRef, 4> chosen_uv;
  ExactRef constant_v;
  int32_t orientation = 1;
};
struct PhysicalCurveInterval final {
  BoundaryKey boundary_key;
  std::array<uint32_t, 2> boundary_nodes;
  std::array<ExactRef, 2> original_t;
  ExactRef parameter_chord_upper;
  std::optional<ExactRef> image_chord_upper;
  CurveCorrespondence selected_correspondence{};
  std::optional<GeometricLineRecord> image_map;
};
// One complete current physical-bound horizon, shared by every face trace.
// Its exact refs address values, while boundary owns source parameters/nodes.
struct PhysicalCurveBoundsData final {
  std::shared_ptr<RunResource> resource;
  DefinitionBoundarySnapshot boundary;
  ExactTable values;
  std::pmr::vector<PhysicalCurveInterval> intervals;
  PhysicalCurveBoundsData(std::shared_ptr<RunResource> owner, DefinitionBoundarySnapshot physical,
                          ExactTable exact)
      : resource(std::move(owner)), boundary(std::move(physical)), values(std::move(exact)),
        intervals(resource.get()) {
    if (!resource || !boundary || boundary->resource != resource || values.resource() != resource)
      throw Error(ErrorCode::internal_error, "physical curve bounds owner mismatch");
  }
};
using PhysicalCurveBounds = std::shared_ptr<const PhysicalCurveBoundsData>;
struct UseCurveSite final {
  uint32_t boundary_node = UINT32_MAX;
  Slot<LoopTag> loop;
  uint32_t junction_occurrence = UINT32_MAX;
  ExactRef original_t, mapped_s;
  std::array<ExactRef, 2> chosen_original_uv, chart_xy;
  std::array<int32_t, 2> original_periodic_shift{}, selected_branch{};
  // Max ORIGINAL V/Ce/S and declared-alias guards; never generated Q residual.
  ExactRef node_source_upper;
};
struct UseCurveInterval final {
  BoundaryKey boundary_key;
  std::array<uint32_t, 2> sites;
  CurveTraceAuthority authority = CurveTraceAuthority::reconstructed_selected;
  CurveCorrespondence correspondence{};
  ExactRef carrier_trace_to_actual_chord_upper, source_to_selected_trace_upper;
  std::array<ExactRef, 2> uv_chord_component_upper;
  std::optional<ExactRef> authoritative_domain_upper;
  std::optional<GeometricLineRecord> image_map;
  std::optional<CylinderAffineIdentityRecord> cylinder_identity;
};
struct UseCurveTrace final {
  Slot<CoedgeTag> use;
  Slot<FaceTag> face;
  uint64_t face_generation = 0, boundary_generation = 0;
  CurveTraceAuthority authority = CurveTraceAuthority::reconstructed_selected;
  Range<UseCurveSite> sites;
  Range<UseCurveInterval> intervals;
};
struct SharedCurveSnapshotData final {
  std::shared_ptr<RunResource> resource;
  SourceStore source;
  Slot<DefinitionTag> definition;
  uint64_t query_generation;
  ExactTable values;
  DefinitionBoundarySnapshot boundary;
  std::pmr::vector<PhysicalCurveInterval> physical_intervals;
  std::pmr::vector<UseCurveTrace> uses;
  std::pmr::vector<UseCurveSite> sites;
  std::pmr::vector<UseCurveInterval> intervals;
  SharedCurveSnapshotData(std::shared_ptr<RunResource> owner, SourceStore original,
                          Slot<DefinitionTag> part, uint64_t generation, ExactTable exact,
                          DefinitionBoundarySnapshot physical)
      : resource(std::move(owner)), source(std::move(original)), definition(part),
        query_generation(generation), values(std::move(exact)), boundary(std::move(physical)),
        physical_intervals(resource.get()), uses(resource.get()), sites(resource.get()),
        intervals(resource.get()) {
    if (!resource || source.view().resource != resource || values.resource() != resource ||
        !boundary || boundary->resource != resource || &boundary->source.view() != &source.view() ||
        boundary->definition != definition)
      throw Error(ErrorCode::internal_error, "shared curve snapshot owner/source mismatch");
  }
};
using SharedCurveSnapshot = std::shared_ptr<const SharedCurveSnapshotData>;
} // namespace bgmesh::adaptive
#endif
