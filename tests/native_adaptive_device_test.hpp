#ifndef BREPGPU_TEST_ADAPTIVE_DEVICE_HPP
#define BREPGPU_TEST_ADAPTIVE_DEVICE_HPP
#include "../native/mesher/adaptive_geometry.hpp"
#include "../native/mesher/cuda/adaptive_api.h"
#include "../native/mesher/cuda/adaptive_trim_api.h"
#include "../native/mesher/reservations.hpp"
#include <chrono>
#include <dlfcn.h>
#include <iostream>
#include <stdexcept>
#include <thread>

namespace adaptive_test {
using namespace bgmesh::adaptive;
class DeviceFixture final {
  struct AllocationLease {
    std::shared_ptr<bgmesh::RunResource> resource;
    std::shared_ptr<bgmesh::ResourceReservation> reservation;
  };
  std::shared_ptr<bgmesh::RunResource> resource_;
  static void *reserve(void *context, uint64_t bytes) noexcept {
    try {
      auto &self = *static_cast<DeviceFixture *>(context);
      auto lease = bgmesh::reserve_resource(self.resource_, bytes);
      bgmesh::RunAllocator<AllocationLease> allocator(self.resource_);
      auto *token = allocator.allocate(1);
      std::construct_at(token, AllocationLease{self.resource_, std::move(lease)});
      return token;
    } catch (...) {
      return nullptr;
    }
  }
  static void release(void *, void *opaque) noexcept {
    if (!opaque)
      return;
    auto *token = static_cast<AllocationLease *>(opaque);
    auto resource = token->resource;
    std::destroy_at(token);
    bgmesh::RunAllocator<AllocationLease>(resource).deallocate(token, 1);
  }
  void *module_ = nullptr;
  const AdaptiveCudaAPI *api_ = nullptr;
  DeviceSession *session_ = nullptr;
  static void checked(int32_t status, const DeviceError &error) {
    if (status != 0)
      throw std::runtime_error(std::string("adaptive device: ") + error.message);
  }
  void poll(uint64_t ticket, void *output, uint32_t count) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
    for (;;) {
      uint32_t ready = 0;
      DeviceError error{};
      checked(api_->poll(session_, ticket, &ready, output, count, &error), error);
      if (ready)
        return;
      if (std::chrono::steady_clock::now() > deadline)
        throw std::runtime_error("adaptive device fixture deadline");
      std::this_thread::yield();
    }
  }

public:
  DeviceFixture(const SourceCache &cache, const char *path, int32_t device)
      : resource_(cache.view().resource) {
    module_ = ::dlopen(path, RTLD_NOW | RTLD_LOCAL);
    if (!module_)
      throw std::runtime_error(::dlerror());
    auto factory = reinterpret_cast<const AdaptiveCudaAPI *(*)()>(
        ::dlsym(module_, "brepgpu_adaptive_cuda_v1"));
    if (!factory)
      throw std::runtime_error("adaptive CUDA factory missing");
    api_ = factory();
    if (!api_ || api_->abi != adaptive_cuda_abi || api_->size != sizeof(AdaptiveCudaAPI))
      throw std::runtime_error("adaptive CUDA ABI mismatch");
    DeviceConfig config{};
    config.abi = adaptive_cuda_abi;
    config.size = sizeof(config);
    config.device = device;
    config.points = 512;
    config.controls = 65536;
    config.nodes = 512;
    config.frontier = 512;
    config.device_bytes = uint64_t{1} << 28;
    config.host_bytes = uint64_t{1} << 28;
    config.work = uint64_t{1} << 32;
    config.reservation_context = this;
    config.reserve = reserve;
    config.release = release;
    DeviceError error{};
    checked(api_->create(&config, cache.roots().data(), static_cast<uint32_t>(cache.roots().size()),
                         cache.original_roots().data(),
                         static_cast<uint32_t>(cache.original_roots().size()),
                         cache.controls().data(), static_cast<uint32_t>(cache.controls().size()),
                         &session_, &error),
            error);
  }
  DeviceFixture(const DeviceFixture &) = delete;
  DeviceFixture &operator=(const DeviceFixture &) = delete;
  ~DeviceFixture() {
    if (session_) {
      DeviceError error{};
      if (api_->destroy(session_, &error) != 0) {
        // Failed drain retains the actual module/token for process lifetime.
        // No replacement poison allocation or dlclose follows a failed drain.
        std::cerr << "adaptive device final drain failed: " << error.message << '\n';
        return;
      }
    }
    if (module_)
      ::dlclose(module_);
  }
  void close() {
    if (session_) {
      DeviceError error{};
      checked(api_->destroy(session_, &error), error);
      session_ = nullptr;
    }
    if (module_) {
      if (::dlclose(module_) != 0)
        throw std::runtime_error("adaptive module close failed");
      module_ = nullptr;
    }
  }
  SampleResult sample(const SampleInput &input, uint32_t destination = 0) {
    DeviceSample command{};
    command.destination = destination;
    command.input = input;
    uint64_t ticket = 0;
    DeviceError error{};
    checked(api_->samples(session_, &command, 1, &ticket, &error), error);
    SampleResult result;
    poll(ticket, &result, 1);
    return result;
  }
  NodeBounds node(const SourceCache &cache, const NodePredicate &input) {
    const auto &root = cache.roots()[input.root.value];
    for (uint32_t i = 0; i < 4; ++i) {
      SampleInput point;
      point.original = root.sample_owners[(i % 2 == 0 ? 0 : 2) + 3 * (i / 2 == 0 ? 0 : 2)];
      point.uv[0] = input.box[i % 2];
      point.uv[1] = input.box[2 + i / 2];
      // Interior child corners must use the original evaluator root; the
      // prepared side owner differs only at original source knot endpoints.
      const bool root_corner = (point.uv[0].lower == root.prepared_box[i % 2].lower) &&
                               (point.uv[1].lower == root.prepared_box[2 + i / 2].lower);
      if (!root_corner)
        point.original = root.original;
      const auto value = sample(point, i);
      if (value.status != NumericStatus::ready)
        throw std::runtime_error("device corner source unresolved");
    }
    DeviceNode command{};
    command.slot = 0;
    command.input = input;
    for (uint32_t i = 0; i < 4; ++i)
      command.points[i] = i;
    command.tolerance_down = 10.;
    command.fair_axis = 2;
    uint64_t ticket = 0;
    DeviceError error{};
    checked(api_->nodes(session_, &command, 1, &ticket, &error), error);
    DeviceNodeAnswer answer{};
    poll(ticket, &answer, 1);
    return answer.bounds;
  }
  void restrict_node(const DeviceRestriction &command) {
    uint64_t ticket = 0;
    DeviceError error{};
    checked(api_->restrict_nodes(session_, &command, 1, &ticket, &error), error);
    poll(ticket, nullptr, 0);
  }
  ActualFacetBounds facet(const ActualFacetInput &input) {
    uint64_t ticket = 0;
    DeviceError error{};
    checked(api_->facets(session_, &input, 1, &ticket, &error), error);
    ActualFacetBounds result;
    poll(ticket, &result, 1);
    return result;
  }
  SourceTriangleBounds source_triangle(const SourceTriangleInput &input) {
    auto factory = reinterpret_cast<const TrimCudaAPI *(*)()>(
        ::dlsym(module_, "brepgpu_adaptive_trim_cuda_v1"));
    if (!factory)
      throw std::runtime_error("adaptive trim CUDA factory missing");
    const auto *trim = factory();
    if (!trim || trim->abi != adaptive_trim_cuda_abi || trim->size != sizeof(TrimCudaAPI) ||
        !trim->source_triangles)
      throw std::runtime_error("adaptive trim CUDA ABI mismatch");
    uint64_t ticket = 0;
    DeviceError error{};
    checked(trim->source_triangles(session_, &input, 1, &ticket, &error), error);
    SourceTriangleBounds result;
    poll(ticket, &result, 1);
    return result;
  }
};
} // namespace adaptive_test
#endif
