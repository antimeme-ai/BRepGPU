#ifndef BREPGPU_MESHER_ADAPTIVE_SCENE_HPP
#define BREPGPU_MESHER_ADAPTIVE_SCENE_HPP
#include "adaptive_component.hpp"
#include "assembly.hpp"
#include "scene_buffers.hpp"
#include <optional>

namespace bgmesh::adaptive {
enum class SceneScope { complete_assembly, selected_components };
struct AdaptiveSceneRequest final {
  AssemblyStore assembly;
  std::span<const ComponentResult> components;
  SceneScope scope = SceneScope::complete_assembly;
  ScenePresentation presentation;
};
struct AdaptiveSceneVertexSource final {
  uint32_t component, face, site;
  OriginalRootSlot original{invalid_slot};
  std::array<ExactRef, 2> original_uv;
  // Cosmetic pole-normal sector; geometry and original_uv stay unchanged.
  std::optional<uint32_t> mapped_normal_piece{};
};
struct AdaptiveSceneFacetSource final {
  uint32_t component, face, facet;
};
struct AdaptiveSceneData final : SceneBuffers {
  SourceStore source;
  AssemblyStore assembly;
  SceneScope scope;
  std::pmr::vector<ComponentResult> components;
  std::pmr::vector<Slot<DefinitionTag>> original_definitions;
  std::pmr::vector<uint32_t> original_instances;
  std::pmr::vector<PhysicalNodeKey> physical_keys;
  std::pmr::vector<StyleId> material_styles;
  std::pmr::vector<AdaptiveSceneVertexSource> vertex_sources;
  std::pmr::vector<AdaptiveSceneFacetSource> facet_sources;
  AdaptiveSceneData(std::shared_ptr<RunResource>, const AdaptiveSceneRequest &);
};
using AdaptiveScene = std::shared_ptr<const AdaptiveSceneData>;
[[nodiscard]] AdaptiveScene make_adaptive_scene(bgfd::Budget &,
                                                const std::shared_ptr<RunResource> &,
                                                const AdaptiveSceneRequest &);
} // namespace bgmesh::adaptive
#endif
