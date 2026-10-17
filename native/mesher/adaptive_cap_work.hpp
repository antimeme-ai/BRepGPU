#ifndef BREPGPU_MESHER_ADAPTIVE_CAP_WORK_HPP
#define BREPGPU_MESHER_ADAPTIVE_CAP_WORK_HPP
#include "adaptive_cap_geometry.hpp"
#include "adaptive_trimmed_geometry.hpp"

namespace bgmesh::adaptive {
// Native packing, independent of the optional CUDA operation table.
struct CapFacetTask final {
  CapSourceDescriptor source;
  CapFacetInput input;
};
struct CapFacetRecord final {
  uint32_t construction_facet, cap, piece, prepared_source;
  std::array<ExactRef, 4> cut_uv;
  std::array<ExactRef, 2> finite_v;
  std::array<double, 9> actual_xyz; // SOURCE roles POLE/LEFT/RIGHT, never locator UV order
  std::array<uint32_t, 3> positive_chart_roles;
  int32_t regular_sheet_sign;
  bool physical_nonzero;
  GeometryBoundPrecision precision;
};
struct CapPreparationStats final {
  uint64_t facets = 0, prepared_sources = 0, exact_entries = 0;
};
struct CapFacetData;
class CapFacetWork final {
  std::shared_ptr<const CapFacetData> data_;
  explicit CapFacetWork(std::shared_ptr<const CapFacetData> data) : data_(std::move(data)) {}
  friend CapFacetWork prepare_cap_facet_work(JobContext &, SurfaceBoundView, FaceConstructionResult,
                                             FaceDomainLease, FacetAllowances,
                                             std::span<const uint32_t>, ExactTableKey,
                                             TrimFacetLimits);
  friend struct CapWorkAccess;

public:
  [[nodiscard]] const TrimFacetWorkKey &key() const;
  [[nodiscard]] const ExactTable &values() const;
  [[nodiscard]] std::span<const CapFacetRecord> facets() const;
  [[nodiscard]] CapPreparationStats stats() const;
};
struct CapFacetBatch final {
  CapFacetWork work;
  std::pmr::vector<uint32_t> indices;
  std::pmr::vector<CapFacetTask> inputs;
  CapFacetBatch(CapFacetWork retained, std::pmr::memory_resource *resource)
      : work(std::move(retained)), indices(resource), inputs(resource) {}
};
[[nodiscard]] CapFacetWork prepare_cap_facet_work(JobContext &, SurfaceBoundView,
                                                  FaceConstructionResult, FaceDomainLease,
                                                  FacetAllowances, std::span<const uint32_t>,
                                                  ExactTableKey, TrimFacetLimits = {});
[[nodiscard]] CapFacetBatch pack_cap_facets(JobContext &, const CapFacetWork &,
                                            std::span<const uint32_t> work_facets);
// The caller consumes/seals the returned Rational fields in this JobContext.
// Only nonzero captured facets may enter precise comparison.
[[nodiscard]] PreciseCapResult compare_cap_work_facet_precise(JobContext &, const CapFacetWork &,
                                                              uint32_t);
} // namespace bgmesh::adaptive
#endif
