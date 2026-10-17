#ifndef BREPGPU_MESHER_CURVE_SOURCE_CACHE_HPP
#define BREPGPU_MESHER_CURVE_SOURCE_CACHE_HPP
#include "adaptive_curve_types.h"
#include "exact.hpp"
#include "source_cache.hpp"
namespace bgmesh::adaptive {
struct CurveRequest {
  Slot<CurveTag> source;
  std::array<double, 2> domain;
};
struct CurveCacheLimits {
  uint32_t roots = 1000000, control_scalars = 64000000;
  uint64_t work = 1000000000;
};
struct CurveSourceCacheData final {
  std::shared_ptr<RunResource> resource;
  SourceStore source;
  uint64_t generation = 1;
  NumericProfile profile;
  std::pmr::vector<CurveRootDescriptor> roots, original_roots;
  std::pmr::vector<Interval> controls;
  CurveSourceCacheData(std::shared_ptr<RunResource>, SourceStore, NumericProfile);
  CurveSourceCacheData(const CurveSourceCacheData &) = delete;
  CurveSourceCacheData &operator=(const CurveSourceCacheData &) = delete;
};
class CurveSourceCache final {
  std::shared_ptr<const CurveSourceCacheData> data_;
  explicit CurveSourceCache(std::shared_ptr<const CurveSourceCacheData> data)
      : data_(std::move(data)) {}
  friend CurveSourceCache prepare_curve_source_cache(const SourceStore &,
                                                     std::span<const CurveRequest>,
                                                     const std::shared_ptr<RunResource> &,
                                                     NumericProfile, CurveCacheLimits,
                                                     bgfd::Budget &);

public:
  const SourceStore &source() const noexcept { return data_->source; }
  uint64_t generation() const noexcept { return data_->generation; }
  NumericProfile profile() const noexcept { return data_->profile; }
  std::span<const CurveRootDescriptor> roots() const noexcept { return data_->roots; }
  std::span<const CurveRootDescriptor> original_roots() const noexcept {
    return data_->original_roots;
  }
  std::span<const Interval> controls(ArithmeticStage stage = ArithmeticStage::rn64) const;
  const CurveSourceCacheData &view() const noexcept { return *data_; }
};
CurveSourceCache prepare_curve_source_cache(const SourceStore &, std::span<const CurveRequest>,
                                            const std::shared_ptr<RunResource> &, NumericProfile,
                                            CurveCacheLimits, bgfd::Budget &);
OriginalCurveRootSlot find_original_curve_root(const CurveSourceCache &, Slot<CurveTag>,
                                               const Rational &);
CurveRootSlot find_prepared_curve_root(const CurveSourceCache &, Slot<CurveTag>,
                                       std::span<const Rational, 2> exact_domain);
} // namespace bgmesh::adaptive
#endif
