#ifndef BREPGPU_MESHER_ADAPTIVE_TRIMMED_GEOMETRY_HPP
#define BREPGPU_MESHER_ADAPTIVE_TRIMMED_GEOMETRY_HPP
#include "adaptive_curve_geometry.hpp"
#include "adaptive_geometry.hpp"
#include "adaptive_trim_types.hpp"
#include "adaptive_trimmed_types.h"
#include "curve_source_cache.hpp"
#include "scheduler.hpp"
#include <optional>

namespace bgmesh::adaptive {
struct FacetAllowances final {
  ExactTable values;
  ExactRef face_position;
};
struct TrimFacetLimits final {
  uint64_t candidate_visits = 10000000, pieces = 1000000, exact_entries = 16000000;
  uint64_t work = 1000000000;
  uint32_t batch_records = 4096, precise_tasks = 4096;
};
enum class TrimBoundMethod : uint32_t { cached_bilinear, original_source };
struct TrimFacetWorkKey final {
  SurfaceViewKey surface;
  FaceConstructionKey construction;
  ExactTableKey values;
  auto operator<=>(const TrimFacetWorkKey &) const = default;
};
struct TrimPieceRecord final {
  uint32_t parent, leaf;
  OriginalRootSlot original;
  Range<ExactEntry> original_uv, affine_xyz;
};
struct TrimFacetRecord final {
  uint32_t construction_facet;
  Range<TrimPieceRecord> pieces;
  bool original_plane = false;
  bool physical_nonzero = false;
  OriginalRootSlot original;
};
struct TrimPreparationStats final {
  uint64_t candidate_visits = 0, clip_vertices = 0, pieces = 0, plane_facets = 0;
};
struct TrimFacetData;
class TrimFacetWork final {
  std::shared_ptr<const TrimFacetData> data_;
  explicit TrimFacetWork(std::shared_ptr<const TrimFacetData> data) : data_(std::move(data)) {}
  friend TrimFacetWork prepare_trim_facet_work(JobContext &, SurfaceBoundView,
                                               FaceConstructionResult, FaceDomainLease,
                                               FacetAllowances, std::span<const uint32_t>,
                                               ExactTableKey, TrimFacetLimits);

public:
  [[nodiscard]] const TrimFacetWorkKey &key() const;
  [[nodiscard]] const ExactTable &values() const;
  [[nodiscard]] std::span<const TrimFacetRecord> facets() const;
  [[nodiscard]] std::span<const TrimPieceRecord> pieces() const;
  [[nodiscard]] TrimPreparationStats stats() const;
  [[nodiscard]] const TrimFacetData &data() const;
};
template <class Input> struct TrimBatch final {
  TrimFacetWork work;
  std::pmr::vector<uint32_t> indices;
  std::pmr::vector<Input> inputs;
  TrimBatch(TrimFacetWork retained, std::pmr::memory_resource *resource)
      : work(std::move(retained)), indices(resource), inputs(resource) {}
};
using TrimBaseBatch = TrimBatch<ActualFacetInput>;
using TrimSourceBatch = TrimBatch<SourceTriangleInput>;
using TrimPlaneBatch = TrimBatch<SourceTriangleInput>;
struct TrimPreciseResult final {
  SourceTriangleBounds bounds;
  bool certified = false;
};
enum class TrimFloorKind : uint32_t { surface_site, bilinear_corner, source_boundary };
struct TrimPointFloor final {
  TrimFloorKind kind;
  uint32_t facet, authority;
  ArithmeticStage current, demanded;
};
struct TrimPointFloors final {
  TrimFacetWork work;
  std::pmr::vector<TrimPointFloor> requests;
  TrimPointFloors(TrimFacetWork retained, std::pmr::memory_resource *resource)
      : work(std::move(retained)), requests(resource) {}
};
[[nodiscard]] TrimFacetWork prepare_trim_facet_work(JobContext &, SurfaceBoundView,
                                                    FaceConstructionResult, FaceDomainLease,
                                                    FacetAllowances, std::span<const uint32_t>,
                                                    ExactTableKey, TrimFacetLimits = {});
[[nodiscard]] TrimBaseBatch pack_base_pieces(JobContext &, const TrimFacetWork &,
                                             std::span<const uint32_t>);
[[nodiscard]] TrimSourceBatch pack_source_pieces(JobContext &, const TrimFacetWork &,
                                                 std::span<const uint32_t>);
[[nodiscard]] TrimPlaneBatch pack_plane_facets(JobContext &, const TrimFacetWork &,
                                               std::span<const uint32_t>);
[[nodiscard]] TrimPreciseResult compare_trim_piece_precise(JobContext &, const TrimFacetWork &,
                                                           uint32_t, TrimBoundMethod);
[[nodiscard]] TrimPreciseResult compare_trim_plane_precise(JobContext &, const TrimFacetWork &,
                                                           uint32_t);
// Only returns proved actual source-point departures above the positional cap.
// Boundary rows are unpromotable source/precision failures in the first profile.
[[nodiscard]] TrimPointFloors trim_point_floor_requests(JobContext &, const TrimFacetWork &,
                                                        std::span<const uint32_t> failed_facets);
[[nodiscard]] SourceTriangleBounds bound_source_triangle(const SourceCache &,
                                                         const SourceTriangleInput &);
[[nodiscard]] SourceTriangleBounds bound_original_plane(const SourceCache &,
                                                        const SourceTriangleInput &);
[[nodiscard]] TrimPreciseResult
compare_original_triangle_precise(const SourceCache &, OriginalRootSlot,
                                  std::span<const Rational, 6>, std::span<const Rational, 9>,
                                  const Rational &allowance, bgfd::Budget &);
} // namespace bgmesh::adaptive
#endif
