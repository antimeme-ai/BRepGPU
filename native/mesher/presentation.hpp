#ifndef BREPGPU_MESHER_PRESENTATION_HPP
#define BREPGPU_MESHER_PRESENTATION_HPP
#include "scene_view.hpp"
#include <string_view>
namespace bgmesh {
struct NativePresentation final {
  std::pmr::vector<Vec3> explosion, centers;
  std::array<bgom10::Request, 2> views{};
  uint32_t axial_direction = 2;
  double strength = 0;
  explicit NativePresentation(std::pmr::memory_resource *r) : explosion(r), centers(r) {}
};
[[nodiscard]] NativePresentation make_presentation(bgfd::Budget &, const NativeScene &,
                                                   uint32_t width, uint32_t height,
                                                   double strength);
[[nodiscard]] NativePresentation make_presentation(bgfd::Budget &, const bgom10::SceneView &,
                                                   uint32_t width, uint32_t height,
                                                   double strength);
struct FrameCaption final {
  std::string_view heading, timing;
};
void annotate_frame(bgfd::Budget &, std::span<uint8_t> rgba, const bgom10::Request &,
                    const NativePresentation &, bool exploded, FrameCaption caption);
void write_png(const std::filesystem::path &, uint32_t width, uint32_t height,
               std::span<const uint8_t> rgba, bgfd::Budget &);
} // namespace bgmesh
#endif
