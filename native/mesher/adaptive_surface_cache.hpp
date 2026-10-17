#ifndef BREPGPU_MESHER_ADAPTIVE_SURFACE_CACHE_HPP
#define BREPGPU_MESHER_ADAPTIVE_SURFACE_CACHE_HPP
#include "adaptive_cache.hpp"
#include "adaptive_surface_view.hpp"
#include "adaptive_trimmed_geometry.hpp"

namespace bgmesh::adaptive {
struct PacketGridTarget;
struct SurfacePointPromotion final {
  PointKey point; // exact refs address the request's captured view selection
  ArithmeticStage demanded = ArithmeticStage::rn64;
};
struct SurfaceQuery final {
  uint64_t generation;
  SurfaceFaceIdentity face;
  SourceCache source;
  ExactTable values;
  std::span<const RootAllowance> allowances; // copied by begin; nonempty root subset
};
struct SurfaceRefineRequest final {
  SurfaceBoundView view;
  std::span<const LeafKey> splits;
  std::span<const PacketGridTarget> packet_targets;
  std::span<const SurfacePointPromotion> promotions;
  FaceConstructionResult construction; // required for packet target facet guards
};
struct SurfacePointInput final {
  std::array<ExactRef, 2> original_uv;
  ArithmeticStage demanded = ArithmeticStage::rn64;
};
struct SurfacePointRequest final {
  SurfaceBoundView view;
  ExactTable values;
  uint64_t ordinal;
  std::span<const SurfacePointInput> points;
};
struct SurfacePointResultData final {
  std::shared_ptr<RunResource> resource;
  SurfaceBoundView view;
  uint64_t ordinal;
  ExactTable values;
  std::pmr::vector<PointRecord> points;
  SurfacePointResultData(std::shared_ptr<RunResource> owner, SurfaceBoundView captured,
                         uint64_t request, ExactTable exact)
      : resource(std::move(owner)), view(std::move(captured)), ordinal(request),
        values(std::move(exact)), points(resource.get()) {}
};
using SurfacePointResult = std::shared_ptr<const SurfacePointResultData>;
struct SurfaceFacetRequest final {
  SurfaceBoundView view;
  FaceConstructionResult construction;
  FaceDomainLease domain;
  FacetAllowances allowances;
  std::span<const uint32_t> selected_facets;
  uint64_t ordinal;
  TrimFacetLimits limits;
};
struct SurfaceFacetAnswer final {
  uint32_t facet;
  std::optional<ExactRef> positional_upper;
  bool accepted = false;
};
struct SurfaceFacetResultData final {
  std::shared_ptr<RunResource> resource;
  TrimFacetWork work;
  ExactTable values;
  uint64_t ordinal;
  std::pmr::vector<SurfaceFacetAnswer> facets;
  std::pmr::vector<TrimPointFloor> point_floors;
  SurfaceFacetResultData(std::shared_ptr<RunResource> owner, TrimFacetWork prepared,
                         ExactTable exact, uint64_t request)
      : resource(std::move(owner)), work(std::move(prepared)), values(std::move(exact)),
        ordinal(request), facets(resource.get()), point_floors(resource.get()) {}
};
using SurfaceFacetResult = std::shared_ptr<const SurfaceFacetResultData>;
enum class SurfaceProgress : uint32_t {
  progressed,
  pending,
  needs_classification,
  selected,
  points_complete,
  facets_complete
};
class SurfaceCacheSession final {
  struct Impl;
  std::shared_ptr<RunResource> resource_;
  Impl *impl_;

public:
  SurfaceCacheSession(std::shared_ptr<RunResource>, SourceCache, CacheLimits,
                      const std::stop_token & = {});
  ~SurfaceCacheSession();
  SurfaceCacheSession(const SurfaceCacheSession &) = delete;
  SurfaceCacheSession &operator=(const SurfaceCacheSession &) = delete;
  void open_device(const CacheDevice &);
  void begin(const SurfaceQuery &);
  SurfaceProgress step(JobContext &);
  [[nodiscard]] CandidateBoxes candidates() const;
  void classify(CandidateClassification);
  [[nodiscard]] SurfaceBoundView selection() const;
  void refine(const SurfaceRefineRequest &);
  void sample(const SurfacePointRequest &);
  [[nodiscard]] SurfacePointResult point_result() const;
  void bound(const SurfaceFacetRequest &);
  [[nodiscard]] SurfaceFacetResult facet_result() const;
  [[nodiscard]] CacheStats stats() const noexcept;
  void release_operation();
  void release_query();
  void retire_assignment(); // drained selected/idle owner; retains fixed source/service bank
  void cancel() noexcept;
  void close();
};
} // namespace bgmesh::adaptive
#endif
