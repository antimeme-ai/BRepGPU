#ifndef BREPGPU_MESHER_ADAPTIVE_CACHE_HPP
#define BREPGPU_MESHER_ADAPTIVE_CACHE_HPP
#include "adaptive_packets.hpp"
#include "scheduler.hpp"
#include <filesystem>
#include <span>
#include <stop_token>

namespace bgmesh::adaptive {
struct RootAllowance final {
  RootSlot root;
  ExactRef positional;
};
// The span is borrowed only through begin(). The immutable exact table and all
// source/result leases are retained until the query and its output finish.
struct PatchQuery final {
  uint64_t generation;
  SourceCache source; // must be the actual session cache lease
  ExactTable values;
  std::span<const RootAllowance> allowances;
};
struct CacheLimits final {
  uint32_t nodes = 1u << 18, points = 1u << 20, control_scalars = 1u << 24;
  uint32_t frontier = 1u << 18, ambiguities = 4096, max_axis_depth = 64;
  uint32_t cpu_threads = 1;
  uint64_t host_bytes = uint64_t{1} << 30, device_bytes = uint64_t{1} << 30;
  uint64_t child_bytes = uint64_t{1} << 26, child_work = uint64_t{1} << 34;
  uint64_t total_work = uint64_t{1} << 44;
};
struct CacheDevice final {
  int32_t device;
  std::filesystem::path module;
};
struct CacheStats final {
  uint64_t queries = 0, epochs = 0, nodes_created = 0, nodes_reused = 0;
  uint64_t point_evaluations = 0, point_hits = 0, control_restrictions = 0, exact_fallbacks = 0;
  uint64_t device_points = 0, device_bounds = 0, device_restrictions = 0, device_epochs = 0;
  uint64_t uploaded_bytes = 0, downloaded_bytes = 0;
  uint64_t host_reserved_bytes = 0, host_peak_bytes = 0;
  uint64_t device_reserved_bytes = 0, device_peak_bytes = 0;
  uint64_t packet_rounds = 0, actual_facet_tests = 0, actual_facet_failures = 0;
};
class PatchSession final {
  struct Impl;
  std::shared_ptr<RunResource> resource_;
  Impl *impl_;

public:
  PatchSession(std::shared_ptr<RunResource>, SourceCache, CacheLimits,
               const std::stop_token & = {});
  ~PatchSession();
  PatchSession(const PatchSession &) = delete;
  PatchSession &operator=(const PatchSession &) = delete;
  void open_device(const CacheDevice &);
  void begin(const PatchQuery &);
  // Coordinator only. CPU batches have private scheduler contexts. A pending
  // device/worker completion returns ready so no executor blocks on a future.
  StepState step(JobContext &);
  [[nodiscard]] PacketMesh result() const;
  [[nodiscard]] std::shared_ptr<const PatchSelection> selection() const;
  [[nodiscard]] CacheStats stats() const noexcept;
  void release_result();
  void cancel() noexcept;
  // Failed drain retains the live owner for retry; a final failing destructor
  // retains its preregistered poisoned owner and reservations until exit.
  void close();
};
} // namespace bgmesh::adaptive
#endif
