#ifndef BREPGPU_MESHER_BOUND_RUNTIME_HPP
#define BREPGPU_MESHER_BOUND_RUNTIME_HPP
#include "bound_tasks.hpp"
#include "geometry_store.hpp"
#include <filesystem>
#include <optional>
#include <stop_token>
#include <variant>

namespace bgmesh {
struct BoundRootSlot final {
  uint32_t value;
};
using BoundRootSelection = std::variant<SurfaceRootSlot, CurveRootSlot>;
struct BoundTaskHeader final {
  BoundTaskKey key;
  uint64_t source_generation, boundary_generation;
  uint32_t bound_revision;
};
struct SurfaceBoundInput final {
  BoundTaskHeader header;
  BoundRootSlot root;
  std::array<ExactRef, 6> original_uv;
  std::array<ExactRef, 9> actual_linear_xyz;
  ExactRef tolerance;
};
struct CurveBoundInput final {
  BoundTaskHeader header;
  BoundRootSlot root;
  std::array<ExactRef, 2> original_parameter;
  std::array<ExactRef, 6> actual_endpoint_xyz;
  ExactRef tolerance;
};
using BoundInput = std::variant<SurfaceBoundInput, CurveBoundInput>;
struct BoundBatchLimits final {
  uint32_t task_capacity, table_capacity;
};
struct BoundRuntimeLimits final {
  uint32_t task_capacity, table_capacity;
  uint64_t child_bytes, child_work, total_work;
};
struct BoundDeviceLimits final {
  int32_t device;
  uint64_t host_bytes, device_bytes;
};
struct BoundRuntimeStats final {
  uint64_t cpu_tasks = 0, device_tasks = 0, exact_tasks = 0, charged_work = 0;
};
struct BoundBankData;
struct BoundBatchData;
struct BoundFilteredData;
struct BoundCompletedData;
struct BoundModuleData;
class BoundBatch;
class BoundRootBank final {
  std::shared_ptr<const BoundBankData> data_;
  explicit BoundRootBank(std::shared_ptr<const BoundBankData> data) : data_(std::move(data)) {}
  friend BoundRootBank prepare_bound_roots(bgfd::Budget &, const std::shared_ptr<RunResource> &,
                                           GeometryStore, std::span<const BoundRootSelection>,
                                           ExactTableKey);
  friend class BoundBatch;
  friend class BoundRuntime;
  friend BoundBatch seal_bound_batch(const std::shared_ptr<RunResource> &, BoundRootBank,
                                     std::span<const ExactTable>, std::span<const BoundInput>,
                                     BoundBatchLimits);

public:
  [[nodiscard]] uint32_t size() const noexcept;
};
[[nodiscard]] BoundRootBank prepare_bound_roots(bgfd::Budget &,
                                                const std::shared_ptr<RunResource> &, GeometryStore,
                                                std::span<const BoundRootSelection>, ExactTableKey);
class BoundBatch final {
  std::shared_ptr<const BoundBatchData> data_;
  explicit BoundBatch(std::shared_ptr<const BoundBatchData> data) : data_(std::move(data)) {}
  friend BoundBatch seal_bound_batch(const std::shared_ptr<RunResource> &, BoundRootBank,
                                     std::span<const ExactTable>, std::span<const BoundInput>,
                                     BoundBatchLimits);
  friend class BoundRuntime;

public:
  [[nodiscard]] std::span<const BoundInput> inputs() const noexcept;
};
[[nodiscard]] BoundBatch seal_bound_batch(const std::shared_ptr<RunResource> &, BoundRootBank,
                                          std::span<const ExactTable>, std::span<const BoundInput>,
                                          BoundBatchLimits);
class BoundFiltered final {
  std::shared_ptr<const BoundFilteredData> data_;
  explicit BoundFiltered(std::shared_ptr<const BoundFilteredData> data) : data_(std::move(data)) {}
  friend class BoundRuntime;

public:
  [[nodiscard]] std::span<const BoundAnswer> answers() const noexcept;
};
class BoundCompleted final {
  std::shared_ptr<const BoundCompletedData> data_;
  explicit BoundCompleted(std::shared_ptr<const BoundCompletedData> data)
      : data_(std::move(data)) {}
  friend class BoundRuntime;

public:
  [[nodiscard]] std::span<const BoundTaskHeader> headers() const noexcept;
  [[nodiscard]] std::span<const BoundAnswer> answers() const noexcept;
};
class BoundCudaModule final {
  std::shared_ptr<const BoundModuleData> data_;
  explicit BoundCudaModule(std::shared_ptr<const BoundModuleData> data) : data_(std::move(data)) {}
  friend BoundCudaModule load_bound_cuda(const std::shared_ptr<RunResource> &,
                                         const std::filesystem::path &);
  friend class BoundRuntime;
};
[[nodiscard]] BoundCudaModule load_bound_cuda(const std::shared_ptr<RunResource> &,
                                              const std::filesystem::path &);
struct BoundTicket final {
  uint64_t runtime, serial;
  auto operator<=>(const BoundTicket &) const = default;
};
// Serialized coordinator/child adapter. It creates no threads and never mutates
// topology. All packets retain immutable exact inputs; exact scratch is local.
class BoundRuntime final {
  struct Impl;
  std::shared_ptr<RunResource> resource_;
  Impl *impl_;

public:
  BoundRuntime(const std::shared_ptr<RunResource> &, BoundRootBank, BoundRuntimeLimits,
               std::stop_token = {});
  ~BoundRuntime();
  BoundRuntime(const BoundRuntime &) = delete;
  BoundRuntime &operator=(const BoundRuntime &) = delete;
  [[nodiscard]] BoundFiltered filter_cpu(const BoundBatch &);
  void open_device(BoundCudaModule, BoundDeviceLimits);
  [[nodiscard]] BoundTicket submit(const BoundBatch &);
  [[nodiscard]] std::optional<BoundFiltered> poll(BoundTicket);
  [[nodiscard]] BoundCompleted resolve(const BoundFiltered &);
  void cancel() noexcept;
  // Failed close retains the live device owner for retry. A final destructor
  // drain failure retains its preregistered record/module/reservation until
  // process exit and emits a diagnostic; it never frees live DMA storage.
  void close();
  [[nodiscard]] BoundRuntimeStats stats() const noexcept;
};
} // namespace bgmesh
#endif
