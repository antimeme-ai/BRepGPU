#ifndef BREPGPU_MESHER_ADAPTIVE_TYPES_HPP
#define BREPGPU_MESHER_ADAPTIVE_TYPES_HPP
#include "exact_store.hpp"
#include "source_cache.hpp"
#include <array>
#include <memory>
#include <memory_resource>
#include <vector>

namespace bgmesh::adaptive {
struct PointSlot final {
  uint32_t value = 0;
  auto operator<=>(const PointSlot &) const = default;
};
// RootSlot is qualified by PatchSelection.source. Path words contain two bits
// per step (axis then child), earliest step in the least significant bits.
// Range is into PatchSelection.path_words; depth, never a hash, bounds equality.
struct LeafKey final {
  RootSlot root;
  uint32_t depth = 0;
  Range<uint64_t> path;
};
struct PointKey final {
  OriginalRootSlot original;
  std::array<ExactRef, 2> parameter;
  ArithmeticStage stage = ArithmeticStage::rn64;
  uint64_t constructed_generation = 0;
};
struct PointRecord final {
  PointKey key;
  // Construction coordinates are distinct from original-UV display/recovery
  // locators on an admitted alternate chart. They coincide for ordinary UV.
  std::array<ExactRef, 2> chart_xy, original_uv;
  std::array<double, 3> actual_xyz;
  double departure_upper = 0;
};
struct SelectedLeaf final {
  LeafKey key;
  std::array<ExactRef, 4> box; // xmin,xmax,ymin,ymax in construction chart
  std::array<PointSlot, 4> corners; // 00,10,01,11
  double source_to_bilinear_upper = 0, twist_upper = 0;
  ArithmeticStage stage = ArithmeticStage::rn64;
};
struct SelectionKey final {
  uint64_t session = 0, query = 0, cache_revision = 0, profile_revision = 0;
  auto operator<=>(const SelectionKey &) const = default;
};
// Producer fills, validates and publishes as shared_ptr<const PatchSelection>.
// Points and leaves are in canonical semantic-key order, independent of cache
// allocation/history. Consumers retain this lease through all borrowed refs.
struct PatchSelection final {
  std::shared_ptr<RunResource> resource;
  SourceCache source;
  ExactTable values;
  SelectionKey key;
  std::pmr::vector<PointRecord> points;
  std::pmr::vector<SelectedLeaf> leaves;
  std::pmr::vector<uint64_t> path_words;
  PatchSelection(std::shared_ptr<RunResource> owner, SourceCache roots, ExactTable exact,
                 SelectionKey identity = {})
      : resource(std::move(owner)), source(std::move(roots)), values(std::move(exact)), key(identity),
        points(resource.get()), leaves(resource.get()), path_words(resource.get()) {
    if (!resource || source.source().view().resource != resource || values.resource() != resource)
      throw Error(ErrorCode::internal_error, "adaptive selection resource mismatch");
  }
  PatchSelection(const PatchSelection &) = delete;
  PatchSelection &operator=(const PatchSelection &) = delete;
};
} // namespace bgmesh::adaptive
#endif
