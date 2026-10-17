#ifndef BREPGPU_MESHER_ADAPTIVE_COMPONENT_HPP
#define BREPGPU_MESHER_ADAPTIVE_COMPONENT_HPP
#include "adaptive_curves.hpp"
#include "adaptive_trimmed.hpp"
#include "contact_epoch.hpp"
#include "curve_source_cache.hpp"

namespace bgmesh::adaptive {
struct ComponentLimits final {
  TrimFaceLimits face;
  CurveLimits curves;
  uint32_t face_owners = 4, contact_batch = 256;
  uint64_t maximum_boundary_rounds = 256;
  ContactEpochLimits contacts{1U << 24};
};
struct ComponentQuery final {
  uint64_t generation;
  Slot<DefinitionTag> definition;
  CurveSourceCache curves;
  ExactTable values;
  ExactRef positional, node_source;
};
struct ComponentStats final {
  uint64_t assignments = 0, face_steps = 0, boundary_rounds = 0;
  uint64_t requested_intervals = 0, retained_faces = 0, facets = 0;
  double boundary_seconds = 0, face_seconds = 0, contact_seconds = 0;
  double owner_setup_seconds = 0, boundary_setup_seconds = 0, boundary_close_seconds = 0;
  CurveStats curves;
  ContactEpochStats contacts;
};
struct ComponentResultData final {
  std::shared_ptr<RunResource> resource;
  SourceCache source;
  ComponentQuery query;
  SharedCurveSnapshot boundary;
  std::pmr::vector<FaceTrimResult> faces;
  // Cumulative fixed-owner snapshots; callers can subtract prior query values.
  std::pmr::vector<TrimFaceStats> owners;
  ComponentStats stats;
  ComponentResultData(std::shared_ptr<RunResource>, SourceCache, const ComponentQuery &);
};
using ComponentResult = std::shared_ptr<const ComponentResultData>;
// Source authority for one site in the immutable final component horizon.
struct ComponentSiteSlot final {
  uint32_t face, site;
};
[[nodiscard]] PhysicalNodeKey component_site_key(const ComponentResultData &, ComponentSiteSlot);
class ComponentMesher final {
  struct Impl;
  std::shared_ptr<RunResource> resource_;
  Impl *impl_;

public:
  ComponentMesher(std::shared_ptr<RunResource>, SourceCache, ComponentLimits,
                  std::optional<CacheDevice> = {}, const std::stop_token & = {});
  ~ComponentMesher();
  ComponentMesher(const ComponentMesher &) = delete;
  ComponentMesher &operator=(const ComponentMesher &) = delete;
  [[nodiscard]] ComponentResult mesh(const ComponentQuery &, NativeScheduler &, JobContext &);
  void cancel() noexcept;
  void close();
};
} // namespace bgmesh::adaptive
#endif
