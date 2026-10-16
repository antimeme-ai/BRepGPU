#ifndef BREPGPU_MESHER_SCHEDULER_HPP
#define BREPGPU_MESHER_SCHEDULER_HPP
#include "exact_store.hpp"
#include "reservations.hpp"
#include <exception>
#include <optional>
#include <stop_token>

namespace bgmesh {
enum class StepState { ready, wait_child, wait_barrier, done, failed };
struct NativeTaskKey final {
  OwnerKey owner;
  uint64_t generation, ordinal;
  auto operator<=>(const NativeTaskKey &) const = default;
};
struct JobContext final {
  bgfd::Budget budget;
  std::stop_token stop;
  JobContext(bgfd::Limits limits, std::pmr::memory_resource *resource, std::stop_token token)
      : budget(limits, resource, token), stop(std::move(token)) {}
};
// One mutable owner at a time. A step returns at a child or barrier boundary;
// only the coordinator submits its continuation. No task waits for queued work.
// Scratch values from JobContext must die before step returns. Retained values
// use the owner's own prepaid arena or immutable run-owned output tables.
class NativeTask {
public:
  const NativeTaskKey key;
  explicit NativeTask(NativeTaskKey identity) : key(identity) {}
  virtual ~NativeTask() = default;
  NativeTask(const NativeTask &) = delete;
  NativeTask &operator=(const NativeTask &) = delete;
  virtual StepState step(JobContext &) = 0;
};
struct StepCompletion final {
  std::shared_ptr<NativeTask> task;
  StepState state;
  std::exception_ptr failure;
  uint64_t exact_work = 0, scratch_peak = 0;
  void rethrow() const {
    if (failure)
      std::rethrow_exception(failure);
  }
};
struct SchedulerLimits final {
  uint32_t threads, task_capacity;
  uint64_t child_bytes, child_work;
};
// Bounded ready/completion rings. Worker completion order is deliberately not
// semantic order; the coordinator folds task keys at complete owner horizons.
class NativeScheduler final {
  struct Impl;
  std::shared_ptr<Impl> impl_;

public:
  NativeScheduler(const std::shared_ptr<RunResource> &, SchedulerLimits);
  ~NativeScheduler();
  NativeScheduler(const NativeScheduler &) = delete;
  NativeScheduler &operator=(const NativeScheduler &) = delete;
  void submit(std::shared_ptr<NativeTask>);
  [[nodiscard]] std::optional<StepCompletion> take();
  void stop() noexcept;
  void drain(); // coordinator only; completed records remain available to take
  [[nodiscard]] std::stop_token stop_token() const noexcept;
};
} // namespace bgmesh
#endif
