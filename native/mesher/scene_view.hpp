#ifndef BREPGPU_MESHER_SCENE_VIEW_HPP
#define BREPGPU_MESHER_SCENE_VIEW_HPP
#include "geometry_analytic.hpp"
#include "mesh.hpp"
#include "../om10/types.hpp"
#include <filesystem>

namespace bgmesh {
struct ScenePresentation final {
  // Empty retains the assembly's stored presentation. Otherwise one offset
  // per original instance; these are presentation coordinates, never geometry.
  std::span<const Vec3> explosion;
};
struct NativeSceneData final {
  NativeMesh mesh;
  GeometryStore geometry;
  std::pmr::vector<bgom10::Definition> definitions;
  std::pmr::vector<bgom10::Instance> instances;
  std::pmr::vector<bgom10::Vertex> vertices;
  std::pmr::vector<bgom10::Triangle> triangles;
  std::pmr::vector<bgom10::Carrier> carriers;
  std::pmr::vector<bgom10::Material> materials;
  std::pmr::vector<bgom10::Control> controls;
  std::pmr::vector<bgom10::VertexRef> vertex_refs;
  std::pmr::vector<bgom10::PrimitiveRef> primitive_refs;
  NativeSceneData(NativeMesh, GeometryStore);
  [[nodiscard]] bgom10::SceneView view() const;
};
using NativeScene = std::shared_ptr<const NativeSceneData>;
[[nodiscard]] NativeScene make_scene(bgfd::Budget &, NativeMesh, GeometryStore,
                                      ScenePresentation = {});
void write_scene(const std::filesystem::path &, const NativeScene &, bgfd::Budget &);
void write_views(const std::filesystem::path &, std::span<const bgom10::Request>, bgfd::Budget &);
} // namespace bgmesh
#endif
