#ifndef BREPGPU_MESHER_RESERVATIONS_HPP
#define BREPGPU_MESHER_RESERVATIONS_HPP
#include "resources.hpp"
#include <cassert>

namespace bgmesh {
// A prepaid capacity, retained through all PMR blocks or external device/pinned
// allocations it covers. Metadata is separately charged through RunAllocator.
// Child Budgets remain single-owner even though admission is thread safe.
class ResourceReservation final : public std::pmr::memory_resource {
  std::shared_ptr<RunResource> resource_;
  uint64_t capacity_;
  std::atomic<uint64_t> live_{0};
  void *do_allocate(std::size_t bytes, std::size_t alignment) override {
    const auto amount = static_cast<uint64_t>(bytes);
    auto previous = live_.load(std::memory_order_relaxed);
    for (;;) {
      if (amount > capacity_ - previous)
        throw Error(ErrorCode::resource_exhausted, "prepaid memory reservation exhausted");
      if (live_.compare_exchange_weak(previous, previous + amount, std::memory_order_relaxed))
        break;
    }
    try {
      return std::pmr::new_delete_resource()->allocate(bytes, alignment);
    } catch (...) {
      live_.fetch_sub(amount, std::memory_order_relaxed);
      throw;
    }
  }
  void do_deallocate(void *value, std::size_t bytes, std::size_t alignment) override {
    std::pmr::new_delete_resource()->deallocate(value, bytes, alignment);
    live_.fetch_sub(static_cast<uint64_t>(bytes), std::memory_order_relaxed);
  }
  bool do_is_equal(const std::pmr::memory_resource &other) const noexcept override {
    return this == &other;
  }

public:
  // Construct through reserve_resource so the shared control block is metered.
  ResourceReservation(std::shared_ptr<RunResource> resource, uint64_t capacity)
      : resource_(std::move(resource)), capacity_(capacity) {
    if (!resource_)
      throw Error(ErrorCode::internal_error, "null reservation resource");
    resource_->acquire(capacity_);
  }
  ~ResourceReservation() override {
    assert(live() == 0 && "reservation must outlive all of its allocations");
    resource_->release(capacity_);
  }
  ResourceReservation(const ResourceReservation &) = delete;
  ResourceReservation &operator=(const ResourceReservation &) = delete;
  [[nodiscard]] uint64_t capacity() const noexcept { return capacity_; }
  [[nodiscard]] uint64_t live() const noexcept { return live_.load(std::memory_order_relaxed); }
};
[[nodiscard]] inline std::shared_ptr<ResourceReservation>
reserve_resource(const std::shared_ptr<RunResource> &resource, uint64_t bytes) {
  if (!resource)
    throw Error(ErrorCode::internal_error, "null run resource");
  return std::allocate_shared<ResourceReservation>(RunAllocator<ResourceReservation>(resource),
                                                   resource, bytes);
}
} // namespace bgmesh
#endif
