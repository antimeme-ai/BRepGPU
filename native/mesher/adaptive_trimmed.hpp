#ifndef BREPGPU_MESHER_ADAPTIVE_TRIMMED_HPP
#define BREPGPU_MESHER_ADAPTIVE_TRIMMED_HPP
#include "adaptive_cap_domain.hpp"
#include "adaptive_surface_cache.hpp"

namespace bgmesh::adaptive {
struct FaceContinuationData final {
  std::shared_ptr<RunResource> resource;
  SourceCache source;
  CapDomainKey origin;
  ExactTable values;
  ExactRef positional, node_source;
  CapCutControlLease control;
  FaceContinuationData(std::shared_ptr<RunResource> owner, SourceCache bank, CapDomainKey captured,
                       FacetAllowances quality, ExactRef delta, CapCutControlLease history)
      : resource(std::move(owner)), source(std::move(bank)), origin(captured),
        values(std::move(quality.values)), positional(quality.face_position), node_source(delta),
        control(std::move(history)) {}
};
using FaceContinuation = std::shared_ptr<const FaceContinuationData>;
struct TrimFaceQuery final {
  uint64_t generation;
  Slot<DefinitionTag> definition;
  Slot<FaceTag> face;
  SharedCurveSnapshot boundary;
  ExactTable values;
  ExactRef positional, node_source;
  uint64_t assignment = 0; // zero selects the legacy strictly increasing-query API
  FaceContinuation continuation{};
};
struct TrimDomainReplacement final {
  SurfaceBoundView expected;
  FaceDomainLease domain;
  ExactTable values;
  ExactRef positional, node_source;
};
struct TrimFaceLimits final {
  CacheLimits surface;
  TrimFacetLimits facets;
  uint64_t regions = uint64_t{1} << 20, anchors = uint64_t{1} << 20;
  uint64_t exact_bits = uint64_t{1} << 20, construction_work = uint64_t{1} << 42;
  bool trace_progress = false;
};
enum class TrimFaceProgress : uint32_t { progressed, pending, needs_boundary, complete };
struct TrimFaceStats final {
  CacheStats surface;
  uint64_t classifications = 0, packet_rounds = 0, region_builds = 0;
  uint64_t band_anchors = 0, packet_targets = 0, boundary_requests = 0;
  uint64_t facet_horizons = 0, bound_pieces = 0, direct_source_tests = 0;
  double classification_seconds = 0, packet_seconds = 0, region_seconds = 0;
  double bound_seconds = 0, anchor_seconds = 0;
};
struct FaceTrimResultData final {
  std::shared_ptr<RunResource> resource;
  FaceConstructionResult construction;
  SharedCurveSnapshot boundary;
  SurfaceBoundView surface;
  ExactTable bound_values;
  // One conservative accepted threshold for ALL facets, not a measured maximum.
  ExactRef positional_upper, requested_node_source;
  FaceTrimResultData(std::shared_ptr<RunResource> owner, FaceConstructionResult face,
                     SurfaceBoundView cover, const TrimFaceQuery &quality)
      : resource(std::move(owner)), construction(std::move(face)), boundary(quality.boundary),
        surface(std::move(cover)), bound_values(quality.values),
        positional_upper(quality.positional), requested_node_source(quality.node_source) {}
};
using FaceTrimResult = std::shared_ptr<const FaceTrimResultData>;
class FaceTrimSession final {
  struct Impl;
  std::shared_ptr<RunResource> resource_;
  Impl *impl_;

public:
  FaceTrimSession(std::shared_ptr<RunResource>, SourceCache, TrimFaceLimits,
                  const std::stop_token & = {});
  ~FaceTrimSession();
  FaceTrimSession(const FaceTrimSession &) = delete;
  FaceTrimSession &operator=(const FaceTrimSession &) = delete;
  void open_device(const CacheDevice &);
  void begin(const TrimFaceQuery &);
  TrimFaceProgress step(JobContext &);
  [[nodiscard]] std::span<const BoundaryKey> boundary_requests() const;
  void replace_boundary(SharedCurveSnapshot);
  void replace_domain(const TrimDomainReplacement &);
  [[nodiscard]] FaceContinuation continuation() const;
  [[nodiscard]] FaceTrimResult result() const;
  [[nodiscard]] TrimFaceStats stats() const noexcept;
  void release_result();
  void retire_assignment(); // pooled complete/needs_boundary; captured host leases survive
  void cancel() noexcept;
  void close();
};
} // namespace bgmesh::adaptive
#endif
