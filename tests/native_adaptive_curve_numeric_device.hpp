#ifndef BREPGPU_TEST_ADAPTIVE_CURVE_NUMERIC_DEVICE_HPP
#define BREPGPU_TEST_ADAPTIVE_CURVE_NUMERIC_DEVICE_HPP
#include "../native/mesher/adaptive_curve_geometry.hpp"
#include "../native/mesher/cuda/adaptive_curve_api.h"
#include "../native/mesher/reservations.hpp"
#include <chrono>
#include <dlfcn.h>
#include <iostream>
#include <stdexcept>
#include <thread>

namespace adaptive_test {
using namespace bgmesh::adaptive;
class CurveDeviceFixture final {
  struct AllocationLease {
    std::shared_ptr<bgmesh::RunResource> resource;
    std::shared_ptr<bgmesh::ResourceReservation> reservation;
  };
  std::shared_ptr<bgmesh::RunResource> resource_;
  static void *reserve(void *context, uint64_t bytes) noexcept {
    try {
      auto &self = *static_cast<CurveDeviceFixture *>(context);
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
  const CurveCudaAPI *api_ = nullptr;
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
  CurveDeviceFixture(const CurveSourceCache &cache, const char *path, int32_t device)
      : resource_(cache.view().resource) {
    module_ = ::dlopen(path, RTLD_NOW | RTLD_LOCAL);
    if (!module_)
      throw std::runtime_error(::dlerror());
    auto factory = reinterpret_cast<const CurveCudaAPI *(*)()>(
        ::dlsym(module_, "brepgpu_adaptive_curve_cuda_v1"));
    if (!factory)
      throw std::runtime_error("adaptive CUDA factory missing");
    api_ = factory();
    if (!api_ || api_->abi != curve_cuda_abi || api_->size != sizeof(CurveCudaAPI))
      throw std::runtime_error("adaptive CUDA ABI mismatch");
    DeviceConfig config{};
    config.abi = curve_cuda_abi;
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
  CurveDeviceFixture(const CurveDeviceFixture &) = delete;
  CurveDeviceFixture &operator=(const CurveDeviceFixture &) = delete;
  ~CurveDeviceFixture() {
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
  CurveSampleResult sample(const CurveSampleInput &input, uint32_t destination = 0) {
    CurveDeviceSample command{};
    command.destination = destination;
    command.input = input;
    uint64_t ticket = 0;
    DeviceError error{};
    checked(api_->samples(session_, &command, 1, &ticket, &error), error);
    CurveSampleResult result;
    poll(ticket, &result, 1);
    return result;
  }
  CurveNodeBounds node(CurveNodeInput input) {
    CurveDeviceNode command{};
    command.input = input;
    command.slot = 0;
    command.tolerance_down = 100.;
    command.points[0] = 0;
    command.points[1] = 1;
    uint64_t ticket = 0;
    DeviceError error{};
    checked(api_->nodes(session_, &command, 1, &ticket, &error), error);
    CurveDeviceNodeAnswer result;
    poll(ticket, &result, 1);
    return result.bounds;
  }
  void children(const CurveDeviceRestriction &command) {
    uint64_t ticket = 0;
    DeviceError error{};
    checked(api_->paired_children(session_, &command, 1, &ticket, &error), error);
    poll(ticket, nullptr, 0);
  }
  CurveNodeBounds trace(const AffineTraceBoundInput &command, std::span<const Interval> controls) {
    uint64_t ticket = 0;
    DeviceError error{};
    checked(api_->trace_bounds(session_, &command, 1, controls.data(),
                               static_cast<uint32_t>(controls.size()), &ticket, &error),
            error);
    CurveNodeBounds result;
    poll(ticket, &result, 1);
    return result;
  }
};
} // namespace adaptive_test
#endif
