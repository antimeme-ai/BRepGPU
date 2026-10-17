#ifndef BREPGPU_MESHER_ADAPTIVE_REGIONS_HPP
#define BREPGPU_MESHER_ADAPTIVE_REGIONS_HPP
#include "adaptive_surface_cache.hpp"
#include "adaptive_trim_types.hpp"
#include "domain_index.hpp"
namespace bgmesh::adaptive {
struct DomainRequest final {
  SharedCurveSnapshot curves;
  FaceConstructionKey key;
  ExactTableKey table_key;
};
struct DomainPreparation final {
  FaceDomainLease domain;
  std::pmr::vector<BoundaryKey> requests;
  explicit DomainPreparation(std::pmr::memory_resource *owner) : requests(owner) {}
};
struct RegionInput final {
  FaceDomainLease domain;
  SurfaceBoundView surface;
  PacketMesh packets;
  FaceConstructionKey key;
  ExactTableKey table_key;
  SurfacePointResult anchors;
  std::span<const uint64_t> anchor_births;
  std::span<const uint64_t> anchor_components;
  FaceConstructionResult previous;
  std::span<const uint64_t> changed_components;
};
struct PacketGridTarget final {
  FacetKey old_facet;
  SurfaceViewKey surface;
  RootSlot root;
  uint32_t leaf = UINT32_MAX;
  std::array<uint64_t, 2> numerator{};
  std::array<uint32_t, 2> depth{};
  uint32_t facet = UINT32_MAX;
};
struct BandAnchorLocator final {
  FacetKey old_facet;
  uint32_t component = UINT32_MAX;
  uint32_t leaf = UINT32_MAX;
  std::array<ExactRef, 2> uv;
  uint64_t birth_horizon = 0;
};
struct ConstructionRefinementData final {
  std::shared_ptr<RunResource> resource;
  FaceConstructionResult construction;
  SurfaceBoundView surface;
  ExactTable values;
  std::pmr::vector<BoundaryKey> boundary_requests;
  std::pmr::vector<LeafKey> splits;
  std::pmr::vector<PacketGridTarget> packet_targets;
  std::pmr::vector<BandAnchorLocator> band_anchors;
  std::pmr::vector<uint64_t> changed_components;
  ConstructionRefinementData(std::shared_ptr<RunResource> owner, FaceConstructionResult old,
                             SurfaceBoundView view, ExactTable exact)
      : resource(std::move(owner)), construction(std::move(old)), surface(std::move(view)),
        values(std::move(exact)), boundary_requests(resource.get()), splits(resource.get()),
        packet_targets(resource.get()), band_anchors(resource.get()),
        changed_components(resource.get()) {}
};
using ConstructionRefinement = std::shared_ptr<const ConstructionRefinementData>;
struct RefinementInput final {
  FaceConstructionResult construction;
  SurfaceFacetResult bounds;
  ExactTableKey table_key;
  uint64_t horizon = 0;
  uint64_t anchor_bits = 1024;
};
[[nodiscard]] DomainPreparation prepare_face_domain(const DomainRequest &, JobContext &);
struct DomainClassifierLimits final {
  uint64_t bytes = uint64_t{1} << 28, work = uint64_t{1} << 36;
};
struct DomainClassifierStats final {
  uint64_t work = 0, peak_bytes = 0;
};
struct PacketFootprintRow final {
  LeafKey prefix;
  SurfaceClass role;
};
struct PacketFootprintData final {
  PacketMesh packets;
  SurfaceFaceIdentity face;
  std::pmr::vector<PacketFootprintRow> rows;
  PacketFootprintData(PacketMesh mesh, SurfaceFaceIdentity identity)
      : packets(std::move(mesh)), face(identity), rows(packets->resource.get()) {}
};
using PacketFootprint = std::shared_ptr<const PacketFootprintData>;
[[nodiscard]] PacketFootprint freeze_packet_footprint(const PacketMesh &, const SurfaceBoundView &,
                                                      bgfd::Budget &);
[[nodiscard]] CandidateClassification apply_footprint(const PacketFootprint &,
                                                      CandidateClassification, bgfd::Budget &);
[[nodiscard]] std::pmr::vector<uint32_t>
packet_leaf_slots(const PacketFootprint &, const SurfaceBoundView &, bgfd::Budget &);
class FaceDomainClassifier final {
  struct Impl;
  std::shared_ptr<RunResource> resource_;
  Impl *impl_;

public:
  FaceDomainClassifier(std::shared_ptr<RunResource>, FaceDomainLease, DomainClassifierLimits,
                       const std::stop_token & = {});
  ~FaceDomainClassifier();
  FaceDomainClassifier(const FaceDomainClassifier &) = delete;
  FaceDomainClassifier &operator=(const FaceDomainClassifier &) = delete;
  [[nodiscard]] CandidateClassification classify(const CandidateBoxes &,
                                                 uint64_t work_allowance = UINT64_MAX);
  [[nodiscard]] std::pmr::vector<SurfaceClass>
  classify_points(const ExactTable &, std::span<const std::array<ExactRef, 2>>,
                  uint64_t work_allowance = UINT64_MAX);
  [[nodiscard]] DomainClassifierStats stats() const noexcept;
};
[[nodiscard]] FaceConstructionResult build_face_regions(const RegionInput &, JobContext &);
[[nodiscard]] FaceConstructionResult rebuild_face_regions(const RegionInput &, JobContext &);
[[nodiscard]] ConstructionRefinement plan_construction_refinement(const RefinementInput &,
                                                                  JobContext &);
} // namespace bgmesh::adaptive
#endif
