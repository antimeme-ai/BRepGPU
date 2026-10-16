#include "scheduler.hpp"
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

namespace bgmesh {
struct NativeScheduler::Impl final {
  enum class SlotState { free, queued, running, complete };
  struct Entry final {
    SlotState state = SlotState::free;
    std::shared_ptr<NativeTask> task;
    StepState outcome = StepState::failed;
    std::exception_ptr failure;
    uint64_t work = 0, peak = 0;
  };
  std::shared_ptr<RunResource> resource;
  SchedulerLimits limits;
  std::pmr::vector<Entry> entries;
  std::pmr::vector<uint32_t> ready, complete;
  std::pmr::vector<std::shared_ptr<ResourceReservation>> scratch;
  std::pmr::vector<std::jthread> workers;
  std::mutex mutex;
  std::condition_variable ready_changed, completion_changed;
  std::stop_source stop;
  uint32_t ready_head = 0, ready_tail = 0, ready_count = 0;
  uint32_t complete_head = 0, complete_tail = 0, complete_count = 0, running = 0;

  Impl(std::shared_ptr<RunResource> owner, SchedulerLimits configured)
      : resource(std::move(owner)), limits(configured), entries(resource.get()),
        ready(resource.get()), complete(resource.get()), scratch(resource.get()),
        workers(resource.get()) {
    if (limits.threads == 0 || limits.task_capacity < limits.threads || limits.child_bytes == 0 ||
        limits.child_work == 0)
      throw Error(ErrorCode::invalid_source, "invalid native scheduler limits");
    entries.resize(limits.task_capacity);
    ready.resize(limits.task_capacity);
    complete.resize(limits.task_capacity);
    scratch.reserve(limits.threads);
    workers.reserve(limits.threads);
    for (uint32_t i = 0; i < limits.threads; ++i)
      scratch.push_back(reserve_resource(resource, limits.child_bytes));
    try {
      for (uint32_t i = 0; i < limits.threads; ++i)
        workers.emplace_back([this, i] { worker(i); });
    } catch (...) {
      stop.request_stop();
      ready_changed.notify_all();
      workers.clear();
      throw;
    }
  }
  uint32_t next(uint32_t index) const noexcept {
    return index + 1 == limits.task_capacity ? 0 : index + 1;
  }
  void worker(uint32_t index) noexcept {
    for (;;) {
      uint32_t slot = 0;
      std::shared_ptr<NativeTask> task;
      {
        std::unique_lock lock(mutex);
        ready_changed.wait(lock, [&] { return ready_count != 0 || stop.stop_requested(); });
        if (ready_count == 0)
          return;
        slot = ready[ready_head];
        ready_head = next(ready_head);
        --ready_count;
        ++running;
        entries[slot].state = SlotState::running;
        task = entries[slot].task;
      }
      StepState state = StepState::failed;
      std::exception_ptr failure;
      uint64_t work = 0, peak = 0;
      try {
        JobContext context({limits.child_bytes, limits.child_work}, scratch[index].get(),
                           stop.get_token());
        context.budget.charge(0);
        try {
          state = task->step(context);
          if (state == StepState::failed)
            throw Error(ErrorCode::internal_error, "failed owner step omitted its error");
        } catch (...) {
          failure = std::current_exception();
          state = StepState::failed;
        }
        work = context.budget.work();
        peak = context.budget.peak();
      } catch (...) {
        failure = std::current_exception();
      }
      {
        std::lock_guard lock(mutex);
        auto &entry = entries[slot];
        entry.outcome = state;
        entry.failure = std::move(failure);
        entry.work = work;
        entry.peak = peak;
        entry.state = SlotState::complete;
        complete[complete_tail] = slot;
        complete_tail = next(complete_tail);
        ++complete_count;
        --running;
      }
      completion_changed.notify_all();
    }
  }
};
NativeScheduler::NativeScheduler(const std::shared_ptr<RunResource> &resource,
                                 SchedulerLimits limits) {
  if (!resource)
    throw Error(ErrorCode::internal_error, "null native scheduler resource");
  impl_ = std::allocate_shared<Impl>(RunAllocator<Impl>(resource), resource, limits);
}
NativeScheduler::~NativeScheduler() {
  stop();
  drain();
  impl_->workers.clear();
}
void NativeScheduler::submit(std::shared_ptr<NativeTask> task) {
  if (!task)
    throw Error(ErrorCode::internal_error, "null native owner task");
  auto &p = *impl_;
  std::lock_guard lock(p.mutex);
  if (p.stop.stop_requested())
    throw Error(ErrorCode::cancelled, "native scheduler admission stopped");
  std::optional<uint32_t> free;
  // This scans the bounded outstanding task window, never source/face tables.
  for (uint32_t i = 0; i < p.limits.task_capacity; ++i) {
    const auto &entry = p.entries[i];
    if (entry.state == Impl::SlotState::free) {
      if (!free)
        free = i;
    } else if (entry.task->key.owner == task->key.owner) {
      throw Error(ErrorCode::internal_error, "native owner already queued or completing");
    }
  }
  if (!free)
    throw Error(ErrorCode::resource_exhausted, "native outstanding task window exhausted");
  auto &entry = p.entries[*free];
  entry.task = std::move(task);
  entry.state = Impl::SlotState::queued;
  p.ready[p.ready_tail] = *free;
  p.ready_tail = p.next(p.ready_tail);
  ++p.ready_count;
  p.ready_changed.notify_one();
}
std::optional<StepCompletion> NativeScheduler::take() {
  auto &p = *impl_;
  std::lock_guard lock(p.mutex);
  if (p.complete_count == 0)
    return std::nullopt;
  const auto slot = p.complete[p.complete_head];
  p.complete_head = p.next(p.complete_head);
  --p.complete_count;
  auto &entry = p.entries[slot];
  StepCompletion result{std::move(entry.task), entry.outcome, std::move(entry.failure), entry.work,
                        entry.peak};
  entry.state = Impl::SlotState::free;
  return result;
}
void NativeScheduler::stop() noexcept {
  impl_->stop.request_stop();
  impl_->ready_changed.notify_all();
}
void NativeScheduler::drain() {
  auto &p = *impl_;
  std::unique_lock lock(p.mutex);
  p.completion_changed.wait(lock, [&] { return p.ready_count == 0 && p.running == 0; });
}
std::stop_token NativeScheduler::stop_token() const noexcept { return impl_->stop.get_token(); }
SchedulerLimits NativeScheduler::limits() const noexcept { return impl_->limits; }
bool NativeScheduler::idle() const {
  std::lock_guard lock(impl_->mutex);
  return impl_->ready_count == 0 && impl_->running == 0 && impl_->complete_count == 0;
}
} // namespace bgmesh
