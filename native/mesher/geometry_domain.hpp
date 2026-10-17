#ifndef BREPGPU_MESHER_GEOMETRY_DOMAIN_HPP
#define BREPGPU_MESHER_GEOMETRY_DOMAIN_HPP
#include "geometry_charts.hpp"
#include "topology.hpp"
namespace bgmesh {
// All input rows are original-use traversal, including literal descending edge
// ranges. pcurve_parameter is mapped s; it is never treated as spatial t.
struct DomainArcInput final {
  Slot<CoedgeTag> use;
  BoundaryKey feature;
  uint32_t begin_site, end_site;
  std::array<Rational, 2> source_parameter, pcurve_parameter;
  std::array<Rational, 4> selected;
  bool declared_quotient = false;
};
enum class DomainReferencePolicy { authoritative_original, hybrid_pullback };
struct FrozenDomainFan final {
  Slot<CoedgeTag> use;
  ExactRef parameter;
  uint32_t chosen_site;
  bool projected = false;
  std::array<ExactRef, 8> polygon;
  // Original-source enclosure is a construction cache, not replacement A.
  std::array<ExactRef, 4> source_enclosure;
  BoundaryKey feature;
};
struct FrozenDomainStrip final {
  Slot<CoedgeTag> use;
  Range<FrozenDomainFan> pre_fans;
  std::array<ExactRef, 2> source_parameter;
  std::array<uint32_t, 2> endpoints;
  std::array<ExactRef, 2> origin, x_covector, y_covector;
  ExactRef extent, c, w0, slope_upper;
  BoundaryKey feature;
};
struct DomainSupportNode final {
  std::array<double, 4> box;
  uint32_t left = 0, right = 0, first = 0, count = 0;
};
struct DomainMapData final {
  std::shared_ptr<RunResource> resource;
  GeometryStore geometry;
  Slot<DefinitionTag> definition;
  Slot<FaceTag> face;
  std::array<uint64_t, 3> generations;
  DomainReferencePolicy policy;
  GeometryBoundPrecision precision;
  Slot<CarrierTag> carrier;
  WorkingChartKind chart_kind;
  std::array<ExactRef, 4> original_rectangle{};
  ExactTable values;
  std::pmr::vector<FrozenDomainFan> fans;
  std::pmr::vector<FrozenDomainStrip> strips;
  // Unified support ordinal: fans first then strips; no shared mutable Budget.
  std::pmr::vector<uint32_t> support_order;
  std::pmr::vector<DomainSupportNode> support_index;
  DomainMapData(std::shared_ptr<RunResource>, GeometryStore, Slot<DefinitionTag>, Slot<FaceTag>,
                std::array<uint64_t, 3>, DomainReferencePolicy, GeometryBoundPrecision, ExactTable);
  DomainMapData(const DomainMapData &) = delete;
  DomainMapData &operator=(const DomainMapData &) = delete;
};
using DomainMap = std::shared_ptr<const DomainMapData>;
struct DomainMapLimits final {
  uint32_t curve_subdivisions = 32, star_shrinks = 24;
  uint64_t interval_pieces = 65536;
};
struct DomainMapResult final {
  DomainMap map;
  std::optional<BoundaryKey> boundary_request;
};
struct DomainMapRequest final {
  const WorkingChart &chart;
  Slot<DefinitionTag> definition;
  std::array<uint64_t, 3> generations;
  std::span<const DomainArcInput> arcs;
  std::shared_ptr<RunResource> resource;
  ExactTableKey table;
  DomainMapLimits limits{};
  GeometryBoundPrecision precision{};
};
[[nodiscard]] DomainMapResult construct_domain_map(GeometryContext &, const DomainMapRequest &);
struct DomainDisplacement final {
  Rational upper;
  std::optional<BoundaryKey> feature;
};
[[nodiscard]] DomainDisplacement
bound_domain_displacement(GeometryContext &, const DomainMap &,
                          std::span<const Rational, 6> selected_triangle);
struct FaceBoundarySnapshotData;
struct DomainIssue;
// Source-aware classification of the SAME exact selected-domain validator's
// issue. Positive canonical source/reference deviation permits a bounded master
// split. Missing/declared-chart ambiguity is unresolved, never guessed valid.
[[nodiscard]] BoundaryKey
classify_coarse_domain_issue(bgfd::Budget &, const FaceBoundarySnapshotData &, const DomainIssue &);
[[nodiscard]] std::optional<BoundaryKey>
protected_domain_point(bgfd::Budget &, const DomainMap &, const std::array<Rational, 2> &selected);
} // namespace bgmesh
#endif
