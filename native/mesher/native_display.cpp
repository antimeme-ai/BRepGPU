#include "native_display.hpp"
#include <algorithm>
#include <dlfcn.h>
#include <iostream>

namespace bgmesh {
namespace {
void display_check(bool ok, const bgom10::Error &error) {
  if (!ok)
    throw Error(ErrorCode::device_failure, std::string(error.stage) + ": " + error.message +
                                               " row=" + std::to_string(error.row));
}
} // namespace
NativeDisplayModule::NativeDisplayModule(std::optional<std::filesystem::path> module) {
  if (!module) {
    api_ = bgom10::cpu_api_v2();
    return;
  }
  if (module->empty())
    throw Error(ErrorCode::invalid_source, "empty requested display module path");
  handle_ = ::dlopen(module->c_str(), RTLD_NOW | RTLD_LOCAL);
  if (!handle_)
    throw Error(ErrorCode::device_failure,
                "cannot load display module: " + std::string(::dlerror()));
  const auto factory =
      reinterpret_cast<const bgom10::Api *(*)()>(::dlsym(handle_, "brepgpu_om10_cuda_v2"));
  if (factory)
    api_ = factory();
  if (!api_ || api_->version != 2 || api_->size < sizeof(bgom10::Api) || !api_->create ||
      !api_->evaluate || !api_->render || !api_->close) {
    ::dlclose(handle_);
    handle_ = nullptr;
    throw Error(ErrorCode::device_failure, "display module has no complete v2 source API");
  }
}
NativeDisplayModule::~NativeDisplayModule() {
  if (handle_ && ::dlclose(handle_) != 0)
    std::cerr << "cleanup\tdisplay_module\tdlclose failed\n";
}
struct NativeDisplayState final {
  std::shared_ptr<RunResource> resource;
  std::shared_ptr<NativeDisplayModule> module;
  std::shared_ptr<const SceneBuffers> source;
  std::pmr::vector<bgom10::Instance> instances;
  std::pmr::vector<uint8_t> rgba;
  std::pmr::vector<double> depth;
  std::pmr::vector<uint64_t> primitive, occurrence;
  bgom10::SceneView view{};
  uint32_t width = 0, height = 0;
  void *token = nullptr;
  // The existing owner record self-retains only on failed final device drain.
  std::shared_ptr<NativeDisplayState> poisoned_keepalive;
  NativeDisplayState(std::shared_ptr<RunResource> r, std::shared_ptr<NativeDisplayModule> m,
                     std::shared_ptr<const SceneBuffers> s)
      : resource(std::move(r)), module(std::move(m)), source(std::move(s)),
        instances(resource.get()), rgba(resource.get()), depth(resource.get()),
        primitive(resource.get()), occurrence(resource.get()) {}
};
bool NativeDisplay::close_raw(bgom10::Error &error, bgom10::Times &times) {
  if (!state_->token)
    return true;
  void *token = state_->token;
  state_->token = nullptr;
  const bool ok = state_->module->api().close(token, &error, &times);
  if (!ok) {
    state_->poisoned_keepalive = state_;
    std::cerr
        << "cleanup\tdisplay_owner\tretained poisoned module/input/output until process exit\n";
  }
  return ok;
}
NativeDisplay::NativeDisplay(const std::shared_ptr<RunResource> &resource,
                             std::shared_ptr<NativeDisplayModule> module,
                             std::shared_ptr<const SceneBuffers> source,
                             const NativePresentation &presentation) {
  if (!resource || !module || !source || source->resource != resource ||
      presentation.explosion.size() != source->instances.size())
    throw Error(ErrorCode::invalid_source, "display source/resource/presentation mismatch");
  state_ = std::allocate_shared<NativeDisplayState>(RunAllocator<NativeDisplayState>(resource),
                                                    resource, std::move(module), std::move(source));
  state_->instances = state_->source->instances;
  for (std::size_t i = 0; i < state_->instances.size(); ++i)
    std::copy(presentation.explosion[i].begin(), presentation.explosion[i].end(),
              state_->instances[i].explosion);
  state_->view = state_->source->view();
  state_->view.instances = state_->instances.data();
  state_->width = presentation.views[0].width;
  state_->height = presentation.views[0].height;
  if (!state_->width || !state_->height || presentation.views[1].width != state_->width ||
      presentation.views[1].height != state_->height)
    throw Error(ErrorCode::invalid_source, "display frame dimension mismatch");
  const uint64_t pixels = uint64_t{presentation.views[0].width} * presentation.views[0].height;
  if (pixels > SIZE_MAX / 4)
    throw Error(ErrorCode::resource_exhausted, "display image allocation overflow");
  state_->rgba.resize(static_cast<std::size_t>(pixels * 4));
  state_->depth.resize(static_cast<std::size_t>(pixels));
  state_->primitive.resize(static_cast<std::size_t>(pixels));
  state_->occurrence.resize(static_cast<std::size_t>(pixels));
}
NativeDisplay::~NativeDisplay() {
  if (state_->token) {
    bgom10::Error error{};
    bgom10::Times times{};
    if (!close_raw(error, times))
      std::cerr << "cleanup\t" << error.stage << '\t' << error.message << '\n';
  }
}
const bgom10::SceneView &NativeDisplay::view() const { return state_->view; }
void NativeDisplay::open(uint32_t threads, int32_t device, bgom10::Times &times) {
  if (state_->token || state_->poisoned_keepalive)
    throw Error(ErrorCode::invalid_source, "display owner already opened or failed drain");
  bgom10::Error error{};
  display_check(
      state_->module->api().create(&state_->view, threads, device, &state_->token, &error, &times),
      error);
}
void NativeDisplay::evaluate(bgom10::Times &times) {
  if (!state_->token)
    throw Error(ErrorCode::invalid_source, "display evaluation without an open owner");
  bgom10::Error error{};
  display_check(state_->module->api().evaluate(state_->token, nullptr, &error, &times), error);
}
void NativeDisplay::render(const bgom10::Request &request, bgom10::Times &times) {
  if (!state_->token || request.width != state_->width || request.height != state_->height)
    throw Error(ErrorCode::invalid_source, "display render owner/dimension mismatch");
  bgom10::FrameOutput output{state_->rgba.data(),
                             state_->depth.data(),
                             state_->primitive.data(),
                             state_->occurrence.data(),
                             nullptr,
                             nullptr};
  bgom10::Error error{};
  display_check(state_->module->api().render(state_->token, &request, &output, &error, &times),
                error);
}
std::span<uint8_t> NativeDisplay::pixels() { return state_->rgba; }
void NativeDisplay::close(bgom10::Times &times) {
  bgom10::Error error{};
  display_check(close_raw(error, times), error);
}
} // namespace bgmesh
