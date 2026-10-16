#ifndef BREPGPU_MESHER_SOURCE_CACHE_HPP
#define BREPGPU_MESHER_SOURCE_CACHE_HPP
#include "../face_domain/exact.hpp"
#include "adaptive_geometry_types.h"
#include "source.hpp"
#include <array>
#include <memory>
#include <span>

namespace bgmesh::adaptive {
struct CarrierRequest {
  Slot<CarrierTag> source;
  std::array<double, 4> domain;
};
struct NumericProfile {
  uint32_t revision = 1;
};
struct SourceCacheLimits {
  uint32_t roots = 1000000, control_scalars = 64000000;
  uint64_t work = 1000000000;
};
struct SourceCacheData final {
  std::shared_ptr<RunResource> resource;
  SourceStore source;
  uint64_t generation = 1;
  NumericProfile profile;
  std::pmr::vector<RootDescriptor> roots, original_roots;
  std::pmr::vector<Interval> controls;
  SourceCacheData(std::shared_ptr<RunResource>, SourceStore, NumericProfile);
  SourceCacheData(const SourceCacheData &) = delete;
  SourceCacheData &operator=(const SourceCacheData &) = delete;
};
class SourceCache final {
  std::shared_ptr<const SourceCacheData> data_;
  explicit SourceCache(std::shared_ptr<const SourceCacheData> data) : data_(std::move(data)) {}
  friend SourceCache prepare_source_cache(const SourceStore &, std::span<const CarrierRequest>,
                                          const std::shared_ptr<RunResource> &, NumericProfile,
                                          SourceCacheLimits, bgfd::Budget &);

public:
  [[nodiscard]] const SourceStore &source() const noexcept { return data_->source; }
  [[nodiscard]] uint64_t generation() const noexcept { return data_->generation; }
  [[nodiscard]] NumericProfile profile() const noexcept { return data_->profile; }
  [[nodiscard]] std::span<const RootDescriptor> roots() const noexcept { return data_->roots; }
  [[nodiscard]] std::span<const RootDescriptor> original_roots() const noexcept {
    return data_->original_roots;
  }
  [[nodiscard]] std::span<const Interval>
  controls(ArithmeticStage stage = ArithmeticStage::rn64) const;
  [[nodiscard]] const SourceCacheData &view() const noexcept { return *data_; }
};
[[nodiscard]] SourceCache prepare_source_cache(const SourceStore &, std::span<const CarrierRequest>,
                                               const std::shared_ptr<RunResource> &, NumericProfile,
                                               SourceCacheLimits, bgfd::Budget &);
} // namespace bgmesh::adaptive
#endif
