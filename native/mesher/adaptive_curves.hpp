#ifndef BREPGPU_MESHER_ADAPTIVE_CURVES_HPP
#define BREPGPU_MESHER_ADAPTIVE_CURVES_HPP
#include "adaptive_curves_types.hpp"
#include "scheduler.hpp"
#include <span>
#include <stop_token>

namespace bgmesh::adaptive {
class CurveSourceCache;
struct CacheDevice;
struct FaceCurveAllowance final {
  Slot<FaceTag> face;
  ExactRef positional, node_source;
};
struct CurveBoundaryQuery final {
  uint64_t generation;
  ExactTable values;
  ExactRef default_positional, default_node_source;
  std::span<const FaceCurveAllowance> overrides;
};
struct CurveLimits final {
  uint32_t nodes = 1U << 18, points = 1U << 20, control_scalars = 1U << 24;
  uint32_t frontier = 1U << 18, requests = 1U << 18, ambiguities = 4096;
  uint32_t max_depth = 64, cpu_threads = 1;
  uint64_t host_bytes = uint64_t{1} << 30, device_bytes = uint64_t{1} << 30;
  uint64_t child_bytes = uint64_t{1} << 26, child_work = uint64_t{1} << 34;
  uint64_t total_work = uint64_t{1} << 44;
};
struct CurveStats final {
  uint64_t queries = 0, epochs = 0, nodes_created = 0, nodes_reused = 0;
  uint64_t point_evaluations = 0, point_hits = 0, control_restrictions = 0, chord_tests = 0;
  uint64_t trace_tests = 0, exact_fallbacks = 0, requested_splits = 0, installed_splits = 0;
  uint64_t invalidated_faces = 0, device_points = 0, device_bounds = 0, device_restrictions = 0;
  uint64_t uploaded_bytes = 0, downloaded_bytes = 0;
  uint64_t host_reserved_bytes = 0, host_peak_upper_bytes = 0;
  uint64_t device_reserved_bytes = 0, device_peak_bytes = 0;
};
class AdaptiveBoundarySession final {
  struct Impl;
  std::shared_ptr<RunResource> resource_;
  Impl *impl_;

public:
  AdaptiveBoundarySession(std::shared_ptr<RunResource>, SourceStore, Slot<DefinitionTag>,
                          CurveSourceCache, CurveLimits, const std::stop_token & = {});
  ~AdaptiveBoundarySession();
  AdaptiveBoundarySession(const AdaptiveBoundarySession &) = delete;
  AdaptiveBoundarySession &operator=(const AdaptiveBoundarySession &) = delete;
  void open_device(const CacheDevice &);
  void begin(const CurveBoundaryQuery &);
  StepState step(JobContext &);
  [[nodiscard]] SharedCurveSnapshot result() const;
  [[nodiscard]] CurveStats stats() const noexcept;
  void release_result();
  void cancel() noexcept;
  void close();
};
} // namespace bgmesh::adaptive
#endif
