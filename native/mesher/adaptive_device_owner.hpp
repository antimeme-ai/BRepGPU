#ifndef BREPGPU_MESHER_ADAPTIVE_DEVICE_OWNER_HPP
#define BREPGPU_MESHER_ADAPTIVE_DEVICE_OWNER_HPP
#include "cuda/adaptive_api.h"
#include "reservations.hpp"
#include <atomic>
#include <cstdio>

namespace bgmesh::adaptive::detail {
// One native ownership mechanism for both typed device APIs. Register before
// create; final failed drains retain this existing record without allocating.
struct ExternalLease final {
  ExternalLease *next = nullptr;
  std::shared_ptr<ResourceReservation> reservation;
};
template <class API> struct DeviceOwner final {
  DeviceOwner *next = nullptr;
  std::shared_ptr<RunResource> resource;
  std::shared_ptr<ResourceReservation> reservation;
  void *module = nullptr;
  const API *api = nullptr;
  DeviceSession *session = nullptr;
  ExternalLease *leases = nullptr;
  explicit DeviceOwner(std::shared_ptr<RunResource> r) : resource(std::move(r)) {}
};
template <class API> void *reserve_external(void *context, uint64_t bytes) noexcept {
  auto &owner = *static_cast<DeviceOwner<API> *>(context);
  ExternalLease *record = nullptr;
  try {
    record = static_cast<ExternalLease *>(
        owner.resource->allocate(sizeof(ExternalLease), alignof(ExternalLease)));
    new (record) ExternalLease;
    record->reservation = reserve_resource(owner.resource, bytes);
    record->next = owner.leases;
    owner.leases = record;
    return record;
  } catch (...) {
    if (record) {
      record->~ExternalLease();
      owner.resource->deallocate(record, sizeof(ExternalLease), alignof(ExternalLease));
    }
    return nullptr;
  }
}
template <class API> void release_external(DeviceOwner<API> &owner, ExternalLease *token) noexcept {
  auto **at = &owner.leases;
  while (*at && *at != token)
    at = &(*at)->next;
  if (!*at)
    return;
  auto *record = *at;
  *at = record->next;
  record->~ExternalLease();
  owner.resource->deallocate(record, sizeof(ExternalLease), alignof(ExternalLease));
}
template <class API> inline std::atomic<DeviceOwner<API> *> poisoned{nullptr};
template <class API> void retain_poison(DeviceOwner<API> *owner) noexcept {
  auto *previous = poisoned<API>.load(std::memory_order_relaxed);
  do {
    owner->next = previous;
  } while (!poisoned<API>.compare_exchange_weak(previous, owner, std::memory_order_release,
                                                std::memory_order_relaxed));
  std::fputs(
      "adaptive CUDA final drain failed: retaining poisoned device owner until process exit\n",
      stderr);
}
inline void device_check(int32_t status, const DeviceError &error) {
  if (status)
    throw Error(status == 2 ? ErrorCode::resource_exhausted : ErrorCode::device_failure,
                error.message[0] ? error.message : "adaptive CUDA failure");
}
} // namespace bgmesh::adaptive::detail
#endif
