#ifndef BREPGPU_MESHER_SCENE_VIEW_HPP
#define BREPGPU_MESHER_SCENE_VIEW_HPP
#include "../om10/types.hpp"
#include "geometry_analytic.hpp"
#include "mesh.hpp"
#include "scene_buffers.hpp"
#include <filesystem>

namespace bgmesh {
struct NativeSceneData final : SceneBuffers {
  NativeMesh mesh;
  GeometryStore geometry;
  NativeSceneData(NativeMesh, GeometryStore);
};
using NativeScene = std::shared_ptr<const NativeSceneData>;
[[nodiscard]] NativeScene make_scene(bgfd::Budget &, NativeMesh, GeometryStore,
                                     ScenePresentation = {});
void write_scene(const std::filesystem::path &, const NativeScene &, bgfd::Budget &);
// Borrowed arrays must remain alive through this synchronous validated write.
void write_scene(const std::filesystem::path &, const bgom10::SceneView &, bgfd::Budget &);
void write_views(const std::filesystem::path &, std::span<const bgom10::Request>, bgfd::Budget &);
} // namespace bgmesh
#endif
