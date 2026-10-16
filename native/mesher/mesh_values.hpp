#ifndef BREPGPU_MESHER_MESH_VALUES_HPP
#define BREPGPU_MESHER_MESH_VALUES_HPP
#include "exact_store.hpp"
#include "values.hpp"
#include <optional>

namespace bgmesh {
struct PhysicalNodeTag;
struct PhysicalFacetTag;
using NodeSlot = Slot<PhysicalNodeTag>;
using FacetSlot = Slot<PhysicalFacetTag>;
enum class NodeGenerator : uint32_t {
  original_vertex,
  original_edge,
  face_site,
  declared_quotient
};
struct PhysicalNodeKey final {
  uint32_t definition = 0;
  NodeGenerator generator = NodeGenerator::original_vertex;
  uint64_t source = 0;
  uint64_t generation = 0;
  uint64_t ordinal = 0;
  auto operator<=>(const PhysicalNodeKey &) const = default;
};
// Identity is source/owner-derived. Equality of XYZ never aliases two nodes.
// Original edge sample ordinals are assigned by its canonical owner transaction,
// persist across boundary rounds, and are not positions in a newly sorted list.
// Generated face keys include the face generation, which invalidates old cells.
struct PhysicalNode final {
  PhysicalNodeKey key;
  Vec3 xyz;
  std::optional<ExactRef> original_parameter;
};
struct FacetHandle final {
  FacetSlot slot;
  uint64_t generation = 0;
  auto operator<=>(const FacetHandle &) const = default;
};
struct PhysicalFacet final {
  FaceId source_face;
  uint32_t definition = 0;
  std::array<NodeSlot, 3> nodes;
};
} // namespace bgmesh
#endif
