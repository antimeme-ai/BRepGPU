#ifndef BREPGPU_MESHER_RESOURCES_HPP
#define BREPGPU_MESHER_RESOURCES_HPP
#include "values.hpp"
#include <algorithm>
#include <atomic>
#include <limits>
#include <memory>
#include <memory_resource>

namespace bgmesh {
class ResourceReservation;
// This resource counts reserved allocation blocks, including container
// capacity. A RunResource outlives every arena and sealed value allocated from
// it.
class RunResource final : public std::pmr::memory_resource {
  friend class ResourceReservation;
  uint64_t limit_;
  std::atomic<uint64_t> live_{0}, peak_{0};
  void acquire(uint64_t amount) {
    auto previous = live_.load(std::memory_order_relaxed);
    for (;;) {
      if (amount > limit_ - previous)
        throw Error(ErrorCode::resource_exhausted, "run memory reservation exhausted");
      if (live_.compare_exchange_weak(previous, previous + amount, std::memory_order_relaxed))
        break;
    }
    const auto now = previous + amount;
    auto peak = peak_.load(std::memory_order_relaxed);
    while (peak < now && !peak_.compare_exchange_weak(peak, now, std::memory_order_relaxed)) {
    }
  }
  void release(uint64_t amount) noexcept { live_.fetch_sub(amount, std::memory_order_relaxed); }
  void *do_allocate(std::size_t bytes, std::size_t alignment) override {
    const auto amount = static_cast<uint64_t>(bytes);
    acquire(amount);
    try {
      return std::pmr::new_delete_resource()->allocate(bytes, alignment);
    } catch (...) {
      release(amount);
      throw;
    }
  }
  void do_deallocate(void *value, std::size_t bytes, std::size_t alignment) override {
    std::pmr::new_delete_resource()->deallocate(value, bytes, alignment);
    release(static_cast<uint64_t>(bytes));
  }
  bool do_is_equal(const std::pmr::memory_resource &other) const noexcept override {
    return this == &other;
  }

public:
  explicit RunResource(uint64_t bytes) : limit_(bytes) {}
  [[nodiscard]] uint64_t live() const noexcept { return live_.load(std::memory_order_relaxed); }
  [[nodiscard]] uint64_t peak() const noexcept { return peak_.load(std::memory_order_relaxed); }
};
// allocate_shared keeps a copy of this allocator through control-block release;
// the resource therefore also outlives deallocation of the containing object.
template <class T> class RunAllocator final {
public:
  using value_type = T;
  std::shared_ptr<RunResource> resource;
  explicit RunAllocator(std::shared_ptr<RunResource> owner) : resource(std::move(owner)) {}
  template <class U> RunAllocator(const RunAllocator<U> &other) : resource(other.resource) {}
  [[nodiscard]] T *allocate(std::size_t count) {
    if (count > std::numeric_limits<std::size_t>::max() / sizeof(T))
      throw Error(ErrorCode::resource_exhausted, "run allocation size overflow");
    return static_cast<T *>(resource->allocate(count * sizeof(T), alignof(T)));
  }
  void deallocate(T *value, std::size_t count) noexcept {
    resource->deallocate(value, count * sizeof(T), alignof(T));
  }
  template <class U> bool operator==(const RunAllocator<U> &other) const noexcept {
    return resource == other.resource;
  }
};
} // namespace bgmesh
#endif
