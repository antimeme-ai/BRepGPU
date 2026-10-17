#ifndef BREPGPU_MESHER_ADAPTIVE_TRIM_TYPES_HPP
#define BREPGPU_MESHER_ADAPTIVE_TRIM_TYPES_HPP
#include "adaptive_curves_types.hpp"
#include "adaptive_packets.hpp"
#include "adaptive_surface_view.hpp"
#include <limits>
#include <optional>
#include <variant>
namespace bgmesh::adaptive {
struct FaceConstructionKey final {
  uint64_t source_generation = 0, query = 0, boundary_generation = 0;
  uint64_t chart_revision = 0, profile_revision = 0, construction_revision = 0;
  Slot<DefinitionTag> definition;
  Slot<FaceTag> face;
  uint64_t assignment_epoch = 0;
  auto operator<=>(const FaceConstructionKey &) const = default;
};
enum class DomainPolicy : uint32_t { reconstructed_selected = 1, authoritative_exact_image = 2 };
struct WholeUseLineImage final {
  Slot<CoedgeTag> use;
  Slot<CurveTag> raw_curve;
  std::array<ExactRef, 4> endpoints;
  std::array<ExactRef, 2> covector, scalar_endpoints;
  Range<UseCurveSite> chosen_sites;
  int32_t raw_traversal = 1;
};
struct FaceDomainUse final {
  Slot<CoedgeTag> use;
  DomainPolicy policy = DomainPolicy::reconstructed_selected;
  std::optional<WholeUseLineImage> image;
};
struct SourceOccurrenceBinding final {
  uint32_t shared_site, construction_site;
};
struct DomainBoundarySite final {
  uint32_t shared_site, boundary_node;
  std::array<ExactRef, 2> uv;
};
struct DomainBoundaryEdge final {
  std::array<uint32_t, 2> sites;
  BoundaryKey provenance;
};
struct DomainBoundaryLoop final {
  Slot<LoopTag> source;
  bool positive = false;
  int32_t traversal_adapter = 1;
  Range<DomainBoundaryEdge> edges;
};
struct FaceDomainData final {
  std::shared_ptr<RunResource> resource;
  SourceStore source;
  SharedCurveSnapshot curves;
  FaceConstructionKey key;
  ExactTable values;
  std::pmr::vector<FaceDomainUse> uses;
  std::pmr::vector<DomainBoundarySite> sites;
  std::pmr::vector<SourceOccurrenceBinding> source_bindings;
  std::pmr::vector<DomainBoundaryEdge> edges;
  std::pmr::vector<DomainBoundaryLoop> loops;
  FaceDomainData(std::shared_ptr<RunResource> owner, SourceStore original,
                 SharedCurveSnapshot boundary, FaceConstructionKey identity, ExactTable exact)
      : resource(std::move(owner)), source(std::move(original)), curves(std::move(boundary)),
        key(identity), values(std::move(exact)), uses(resource.get()), sites(resource.get()),
        source_bindings(resource.get()), edges(resource.get()), loops(resource.get()) {
    if (!resource || source.view().resource != resource || !curves ||
        &curves->source.view() != &source.view() || curves->resource != resource ||
        values.resource() != resource || key.definition != curves->definition ||
        key.query != curves->query_generation)
      throw Error(ErrorCode::invalid_source, "trimmed face domain source/owner mismatch");
  }
};
using FaceDomainLease = std::shared_ptr<const FaceDomainData>;
enum class FacetGenerator : uint32_t { packet = 1, band = 2 };
// Qualified by ONE retained FaceConstructionKey in the containing work/result.
struct FacetKey final {
  FacetGenerator generator = FacetGenerator::band;
  uint64_t owner = 0, revision = 0;
  uint32_t ordinal = 0;
  auto operator<=>(const FacetKey &) const = default;
};
struct BoundarySiteSource final {
  uint32_t boundary_node = UINT32_MAX;
};
struct SurfaceSiteSource final {
  OriginalRootSlot original;
  ArithmeticStage stage = ArithmeticStage::rn64;
  bool anchor = false;
  uint64_t birth_horizon = 0;
  double departure_upper = std::numeric_limits<double>::infinity();
};
struct ConstructionSite final {
  std::array<ExactRef, 2> uv;
  std::array<double, 3> actual_xyz;
  uint64_t constructed_revision = 0;
  std::variant<BoundarySiteSource, SurfaceSiteSource> source;
};
// Original occurrence rows may share a settled loop joint, never a UV seam
// sector.

struct RegionFacet final {
  FacetKey key;
  std::array<uint32_t, 3> sites;
  uint32_t component = UINT32_MAX;
  uint32_t leaf = UINT32_MAX;
};
struct RimIntervalKey final {
  uint64_t footprint_side = 0, revision = 0;
  uint32_t ordinal = 0;
  auto operator<=>(const RimIntervalKey &) const = default;
};
struct ProtectedRegionEdge final {
  std::array<uint32_t, 2> sites;
  std::variant<BoundaryKey, RimIntervalKey> provenance;
};
struct RegionLoop final {
  bool positive = false;
  Range<uint32_t> sites;
  Range<ProtectedRegionEdge> edges;
  std::optional<Slot<LoopTag>> source_loop;
  int32_t traversal_adapter = 1;
};
struct RegionComponent final {
  uint64_t id = 0, revision = 0;
  Range<RegionLoop> loops;
  Range<RegionFacet> facets;
};
struct FaceConstructionResultData final {
  std::shared_ptr<RunResource> resource;
  SourceStore source;
  SourceCache surface_source;
  SharedCurveSnapshot curves;
  FaceDomainLease domain;
  SurfaceBoundView surface;
  PacketMesh packets;
  FaceConstructionKey key;
  ExactTable values;
  std::array<ExactRef, 4> fixed_metric_factor;
  std::pmr::vector<ConstructionSite> sites;
  std::pmr::vector<SourceOccurrenceBinding> source_bindings;
  std::pmr::vector<uint32_t> loop_sites;
  std::pmr::vector<ProtectedRegionEdge> edges;
  std::pmr::vector<RegionLoop> loops;
  std::pmr::vector<RegionComponent> components;
  std::pmr::vector<RegionFacet> facets;
  std::pmr::vector<uint32_t> changed_facets;
  std::pmr::vector<uint64_t> anchor_components;
  FaceConstructionResultData(std::shared_ptr<RunResource> owner, SourceStore original,
                             SourceCache roots, SharedCurveSnapshot boundary,
                             FaceDomainLease policy, SurfaceBoundView cover, PacketMesh interior,
                             FaceConstructionKey identity, ExactTable exact)
      : resource(std::move(owner)), source(std::move(original)), surface_source(std::move(roots)),
        curves(std::move(boundary)), domain(std::move(policy)), surface(std::move(cover)),
        packets(std::move(interior)), key(identity), values(std::move(exact)),
        sites(resource.get()), source_bindings(resource.get()), loop_sites(resource.get()),
        edges(resource.get()), loops(resource.get()), components(resource.get()),
        facets(resource.get()), changed_facets(resource.get()), anchor_components(resource.get()) {
    if (!resource || source.view().resource != resource || values.resource() != resource ||
        !curves || !domain || !surface || !packets || curves->resource != resource ||
        domain->resource != resource || &curves->source.view() != &source.view() ||
        &surface_source.source().view() != &source.view())
      throw Error(ErrorCode::invalid_source, "trimmed construction source/owner mismatch");
  }
};
using FaceConstructionResult = std::shared_ptr<const FaceConstructionResultData>;
} // namespace bgmesh::adaptive
#endif
