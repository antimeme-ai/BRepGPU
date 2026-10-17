#include "adaptive_cache_detail.hpp"

namespace bgmesh::adaptive {
using namespace detail::surface;
struct PatchSession::Impl final : SurfaceCacheCore {
  using SurfaceCacheCore::SurfaceCacheCore;
};

PatchSession::PatchSession(std::shared_ptr<RunResource> resource, SourceCache cache,
                           CacheLimits limits, const std::stop_token &stop)
    : resource_(std::move(resource)), impl_(nullptr) {
  check(resource_ != nullptr, ErrorCode::internal_error, "null adaptive run resource");
  auto *memory = resource_->allocate(sizeof(Impl), alignof(Impl));
  try {
    impl_ = new (memory) Impl(resource_, std::move(cache), limits, stop);
  } catch (...) {
    resource_->deallocate(memory, sizeof(Impl), alignof(Impl));
    throw;
  }
}
PatchSession::~PatchSession() {
  if (!impl_)
    return;
  try {
    close();
  } catch (...) {
    if (impl_->device) {
      retain_poison(impl_->device);
      impl_->device = nullptr;
    }
  }
  impl_->~Impl();
  resource_->deallocate(impl_, sizeof(Impl), alignof(Impl));
}
void PatchSession::open_device(const CacheDevice &request) { open_core(*impl_, request); }
void PatchSession::begin(const PatchQuery &request) { begin_core(*impl_, request, false); }
StepState PatchSession::step(JobContext &context) { return step_core(*impl_, context); }
PacketMesh PatchSession::result() const {
  check(impl_->phase == Impl::Phase::done && impl_->mesh != nullptr, ErrorCode::invalid_source,
        "adaptive result not complete");
  return impl_->mesh;
}
std::shared_ptr<const PatchSelection> PatchSession::selection() const { return impl_->frozen; }
CacheStats PatchSession::stats() const noexcept { return stats_core(*impl_); }
void PatchSession::release_result() {
  auto &p = *impl_;
  check(p.phase == Impl::Phase::done, ErrorCode::invalid_source, "adaptive result not complete");
  p.mesh.reset();
  p.frozen.reset();
  p.query_values.reset();
  p.phase = Impl::Phase::idle;
}
void PatchSession::cancel() noexcept {
  impl_->cancelled.store(true, std::memory_order_relaxed);
  impl_->cancellation.request_stop();
  if (impl_->workers)
    impl_->workers->stop();
}
void PatchSession::close() { close_core(*impl_); }
} // namespace bgmesh::adaptive
