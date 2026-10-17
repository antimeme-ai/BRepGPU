#ifndef BREPGPU_MESHER_SCENE_BUFFERS_HPP
#define BREPGPU_MESHER_SCENE_BUFFERS_HPP
#include "../om10/types.hpp"
#include "resources.hpp"
#include <span>
#include <vector>

namespace bgmesh {
struct ScenePresentation final {
  std::span<const Vec3> explosion;
};
struct SceneBuffers {
  std::shared_ptr<RunResource> resource;
  std::pmr::vector<bgom10::Definition> definitions;
  std::pmr::vector<bgom10::Instance> instances;
  std::pmr::vector<bgom10::Vertex> vertices;
  std::pmr::vector<bgom10::Triangle> triangles;
  std::pmr::vector<bgom10::Carrier> carriers;
  std::pmr::vector<bgom10::Material> materials;
  std::pmr::vector<bgom10::Control> controls;
  std::pmr::vector<bgom10::VertexRef> vertex_refs;
  std::pmr::vector<bgom10::PrimitiveRef> primitive_refs;
  explicit SceneBuffers(std::shared_ptr<RunResource> owner)
      : resource(std::move(owner)), definitions(resource.get()), instances(resource.get()),
        vertices(resource.get()), triangles(resource.get()), carriers(resource.get()),
        materials(resource.get()), controls(resource.get()), vertex_refs(resource.get()),
        primitive_refs(resource.get()) {}
  [[nodiscard]] bgom10::SceneView view() const {
    return {definitions.data(),    definitions.size(),   instances.data(),   instances.size(),
            vertices.data(),       vertices.size(),      triangles.data(),   triangles.size(),
            carriers.data(),       carriers.size(),      materials.data(),   materials.size(),
            controls.data(),       controls.size(),      vertex_refs.data(), vertex_refs.size(),
            primitive_refs.data(), primitive_refs.size()};
  }
};
} // namespace bgmesh
#endif
