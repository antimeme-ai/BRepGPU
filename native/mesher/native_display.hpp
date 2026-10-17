#ifndef BREPGPU_MESHER_NATIVE_DISPLAY_HPP
#define BREPGPU_MESHER_NATIVE_DISPLAY_HPP
#include "presentation.hpp"
#include <optional>

namespace bgmesh {
class NativeDisplayModule final {
  void *handle_ = nullptr;
  const bgom10::Api *api_ = nullptr;

public:
  // No path selects the actual CPU consumer, without loading a CUDA module.
  explicit NativeDisplayModule(std::optional<std::filesystem::path> module = {});
  ~NativeDisplayModule();
  NativeDisplayModule(const NativeDisplayModule &) = delete;
  NativeDisplayModule &operator=(const NativeDisplayModule &) = delete;
  [[nodiscard]] const bgom10::Api &api() const noexcept { return *api_; }
};
struct NativeDisplayState;
class NativeDisplay final {
  std::shared_ptr<NativeDisplayState> state_;
  bool close_raw(bgom10::Error &, bgom10::Times &);

public:
  NativeDisplay(const std::shared_ptr<RunResource> &, std::shared_ptr<NativeDisplayModule>,
                std::shared_ptr<const SceneBuffers>, const NativePresentation &);
  ~NativeDisplay();
  NativeDisplay(const NativeDisplay &) = delete;
  NativeDisplay &operator=(const NativeDisplay &) = delete;
  [[nodiscard]] const bgom10::SceneView &view() const;
  void open(uint32_t threads, int32_t device, bgom10::Times &);
  void evaluate(bgom10::Times &);
  void render(const bgom10::Request &, bgom10::Times &);
  [[nodiscard]] std::span<uint8_t> pixels();
  void close(bgom10::Times &);
};
} // namespace bgmesh
#endif
