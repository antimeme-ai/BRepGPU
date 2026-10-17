#ifndef BREPGPU_MESHER_ADAPTIVE_PACKETS_HPP
#define BREPGPU_MESHER_ADAPTIVE_PACKETS_HPP
#include "adaptive_types.hpp"
#include <memory>
#include <memory_resource>
#include <span>
#include <vector>

namespace bgmesh::adaptive {
// Axis is the varying construction coordinate; constant is perpendicular.
struct PacketLine final {
  uint32_t axis;
  ExactRef constant;
  Range<PointSlot> events;
};
struct PacketSide final {
  uint32_t line, first, count;
  bool reverse;
};
struct PacketTriangle final {
  std::array<PointSlot, 3> points;
  uint32_t leaf;
};
struct PacketRecord final {
  uint32_t leaf;
  Range<PacketSide> sides;
  Range<PacketTriangle> triangles;
};
struct PacketRim final {
  uint32_t leaf;
  std::array<PointSlot, 2> points;
};
struct PacketStats final {
  uint64_t comparisons = 0, side_events = 0, demotion_rounds = 0, demoted_packets = 0;
  uint64_t dyadic_comparisons = 0, rational_comparisons = 0;
};
struct PacketMeshData final {
  std::shared_ptr<RunResource> resource;
  std::shared_ptr<const PatchSelection> selection;
  std::pmr::vector<PacketLine> lines;
  std::pmr::vector<PointSlot> line_points;
  std::pmr::vector<PacketSide> sides;
  std::pmr::vector<PacketRecord> packets;
  std::pmr::vector<PacketTriangle> triangles;
  std::pmr::vector<PacketRim> rims;
  std::pmr::vector<uint32_t> demoted_leaf_slots;
  PacketStats stats;
  PacketMeshData(std::shared_ptr<RunResource> owner,
                 std::shared_ptr<const PatchSelection> retained_selection)
      : resource(std::move(owner)), selection(std::move(retained_selection)), lines(resource.get()),
        line_points(resource.get()), sides(resource.get()), packets(resource.get()),
        triangles(resource.get()), rims(resource.get()), demoted_leaf_slots(resource.get()) {}
};
using PacketMesh = std::shared_ptr<const PacketMeshData>;

// The immutable selection owns geometry values. No source evaluation or private
// point construction occurs here. Demoted leaves remain unmeshed band material.
[[nodiscard]] PacketMesh build_packet_mesh(bgfd::Budget &scratch,
                                           const std::shared_ptr<RunResource> &result_resource,
                                           const std::shared_ptr<const PatchSelection> &selection,
                                           std::span<const uint32_t> enabled_leaf_slots);
} // namespace bgmesh::adaptive
#endif
