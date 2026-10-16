#ifndef BREPGPU_MESHER_FACE_RESULT_HPP
#define BREPGPU_MESHER_FACE_RESULT_HPP
#include "contact_epoch.hpp"
#include "face_frontier.hpp"
#include "face_seed.hpp"
namespace bgmesh {
struct FaceResultSite final {
  SiteSlot site;
  std::array<ExactRef, 2> chart, original_uv;
  PhysicalNode node;
  std::optional<std::array<ExactRef, 4>> original_uv_enclosure{};
};
struct FaceResultTriangle final {
  CellResultKey cell;
  CellMapKind map_kind;
  std::array<SiteSlot, 3> sites;
  uint64_t lineage;
  std::array<DeclaredContactEdge, 3> edges;
};
struct FaceResultData final {
  FaceBoundarySnapshot boundary;
  ExactTable values;
  std::pmr::vector<FaceResultSite> sites;
  std::pmr::vector<TriangleLineage> lineages;
  std::pmr::vector<FaceResultTriangle> triangles;
  std::pmr::vector<int32_t> traversal_adapters;
  FaceResultData(FaceBoundarySnapshot seed, ExactTable table)
      : boundary(std::move(seed)), values(std::move(table)), sites(boundary->resource.get()),
        lineages(boundary->resource.get()), triangles(boundary->resource.get()),
        traversal_adapters(boundary->resource.get()) {}
};
using FaceResult = std::shared_ptr<const FaceResultData>;
} // namespace bgmesh
#endif
