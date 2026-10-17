#ifndef BREPGPU_MESHER_GEOMETRY_PROTECTION_HPP
#define BREPGPU_MESHER_GEOMETRY_PROTECTION_HPP
#include "face_seed.hpp"
namespace bgmesh {
struct ProtectedPhysicalSegment final {
  BoundaryKey feature;
  std::array<double, 6> endpoints, box;
};
struct ProtectionNode final {
  std::array<double, 6> box;
  uint32_t left = 0, right = 0, first = 0, count = 0;
};
struct BoundaryProtectionData final {
  std::shared_ptr<RunResource> resource;
  GeometryStore geometry;
  Slot<DefinitionTag> definition;
  Slot<FaceTag> face;
  uint64_t source_generation, face_generation, boundary_generation;
  std::pmr::vector<ProtectedPhysicalSegment> segments;
  std::pmr::vector<uint32_t> order;
  std::pmr::vector<ProtectionNode> nodes;
  BoundaryProtectionData(const FaceBoundarySnapshotData &);
  BoundaryProtectionData(const BoundaryProtectionData &) = delete;
  BoundaryProtectionData &operator=(const BoundaryProtectionData &) = delete;
};
[[nodiscard]] std::shared_ptr<const BoundaryProtectionData>
prepare_boundary_protection(bgfd::Budget &, const FaceBoundarySnapshotData &);
[[nodiscard]] std::optional<BoundaryKey>
encroached_boundary(bgfd::Budget &, const BoundaryProtectionData &, const Vec3 &actual_xyz);
} // namespace bgmesh
#endif
