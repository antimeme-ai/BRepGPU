#include "../native/mesher/scheduler.hpp"
#include <array>
#include <atomic>
#include <iostream>
#include <stdexcept>
#include <thread>

namespace {
void require(bool condition, const char *message) {
  if (!condition)
    throw std::runtime_error(message);
}
struct OriginalStep final : bgmesh::NativeTask {
  std::atomic<uint32_t> &entered;
  std::atomic<bool> &release;
  uint32_t calls = 0;
  OriginalStep(uint64_t identity, std::atomic<uint32_t> &count, std::atomic<bool> &gate)
      : NativeTask({{bgmesh::OwnerKind::face, 0, identity}, 0, 0}), entered(count), release(gate) {}
  bgmesh::StepState step(bgmesh::JobContext &context) override {
    ++calls;
    entered.fetch_add(1);
    while (!release.load()) {
      context.budget.charge(1);
      std::this_thread::yield();
    }
    return calls == 1 ? bgmesh::StepState::wait_barrier : bgmesh::StepState::done;
  }
};
void transitions() {
  auto run = std::make_shared<bgmesh::RunResource>(uint64_t{1} << 24);
  std::atomic<uint32_t> entered{0};
  std::atomic<bool> release{false};
  {
    bgmesh::NativeScheduler scheduler(run, {2, 4, 65536, uint64_t{1} << 40});
    const auto first = std::make_shared<OriginalStep>(1, entered, release);
    const auto second = std::make_shared<OriginalStep>(2, entered, release);
    scheduler.submit(first);
    scheduler.submit(second);
    while (entered.load() != 2)
      std::this_thread::yield();
    bool duplicate = false;
    try {
      scheduler.submit(first);
    } catch (const bgmesh::Error &error) {
      duplicate = error.code == bgmesh::ErrorCode::internal_error;
    }
    require(duplicate && !scheduler.take(), "no duplicate mutable owner or premature completion");
    scheduler.submit(std::make_shared<OriginalStep>(3, entered, release));
    scheduler.submit(std::make_shared<OriginalStep>(4, entered, release));
    bool full = false;
    try {
      scheduler.submit(std::make_shared<OriginalStep>(5, entered, release));
    } catch (const bgmesh::Error &error) {
      full = error.code == bgmesh::ErrorCode::resource_exhausted;
    }
    require(full, "ready plus running plus completed storage is one bounded window");
    release.store(true);
    scheduler.drain();
    uint32_t horizon = 0;
    while (auto result = scheduler.take()) {
      result->rethrow();
      require(result->state == bgmesh::StepState::wait_barrier,
              "worker must yield without waiting for a barrier participant");
      ++horizon;
    }
    require(horizon == 4 && first->calls == 1 && second->calls == 1,
            "complete barrier arrives once before coordinator resubmission");
    scheduler.submit(second);
    scheduler.submit(first);
    scheduler.drain();
    uint32_t done = 0;
    while (auto result = scheduler.take()) {
      result->rethrow();
      require(result->state == bgmesh::StepState::done, "continuation retains owner state");
      ++done;
    }
    require(done == 2, "both owners resume independently of completion order");
  }
  require(run->live() == 0, "workers and completion buffers release their reservations");
}
void cancellation() {
  auto run = std::make_shared<bgmesh::RunResource>(uint64_t{1} << 24);
  std::atomic<uint32_t> entered{0};
  std::atomic<bool> release{false};
  {
    bgmesh::NativeScheduler scheduler(run, {1, 2, 65536, uint64_t{1} << 40});
    auto task = std::make_shared<OriginalStep>(1, entered, release);
    scheduler.submit(task);
    while (entered.load() == 0)
      std::this_thread::yield();
    auto queued = std::make_shared<OriginalStep>(2, entered, release);
    scheduler.submit(queued);
    scheduler.stop();
    scheduler.drain();
    uint32_t cancelled = 0;
    while (auto result = scheduler.take()) {
      try {
        result->rethrow();
      } catch (const bgfd::Cancelled &) {
        ++cancelled;
      }
    }
    require(cancelled == 2 && queued->calls == 0,
            "exact arithmetic stops and queued work completes without running");
    bool refused = false;
    try {
      scheduler.submit(task);
    } catch (const bgmesh::Error &error) {
      refused = error.code == bgmesh::ErrorCode::cancelled;
    }
    require(refused && !scheduler.take(), "stop closes admission and drains exactly once");
  }
  require(run->live() == 0, "cancel releases all native storage after drain");
}
} // namespace
int main() {
  try {
    transitions();
    cancellation();
    std::cout << "PASS\tnative owner continuation scheduler\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << "FAIL\t" << error.what() << '\n';
    return 1;
  }
}
