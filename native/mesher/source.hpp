#ifndef BREPGPU_MESHER_SOURCE_HPP
#define BREPGPU_MESHER_SOURCE_HPP
#include "resources.hpp"
#include <filesystem>
#include <memory>
#include <memory_resource>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

namespace bgmesh {
struct SourceLimits final {
  uint64_t wire_bytes = uint64_t{1} << 30;
  uint64_t records = 100000000;
  uint64_t work = 1000000000;
};
template <class Tag> struct IdIndex final {
  std::pmr::vector<std::pair<SourceId<Tag>, Slot<Tag>>> entries;
  explicit IdIndex(std::pmr::memory_resource *resource) : entries(resource) {}
  [[nodiscard]] Slot<Tag> find(SourceId<Tag> id) const {
    const auto it = std::lower_bound(entries.begin(), entries.end(), id,
                                     [](const auto &entry, auto key) { return entry.first < key; });
    if (it == entries.end() || it->first != id)
      throw Error(ErrorCode::invalid_source, "unknown source ID " + std::to_string(id.value));
    return it->second;
  }
};
struct Csr final {
  std::pmr::vector<uint64_t> offsets;
  std::pmr::vector<uint32_t> values;
  explicit Csr(std::pmr::memory_resource *resource) : offsets(resource), values(resource) {}
  [[nodiscard]] std::span<const uint32_t> at(uint32_t slot) const;
};
// The builder is private to source.cpp; callers receive const data through
// SourceStore.
struct SourceData final {
  std::shared_ptr<RunResource> resource;
  std::pmr::vector<std::byte> original_bytes;
  SourceDigest source_digest;
  // source_uncertainty_mm: a file-wide maximum of the solids' governing B-rep
  // values, a summary only; a part's own value is its source_uncertainty record.
  double units_mm = 0, source_uncertainty_mm = 0;
  ProductId root_product_id;
  Range<char> adapter_revision;
  std::pmr::vector<Product> products;
  std::pmr::vector<Definition> definitions;
  std::pmr::vector<Usage> usages;
  std::pmr::vector<Vertex> vertices;
  std::pmr::vector<Edge> edges;
  std::pmr::vector<Coedge> coedges;
  std::pmr::vector<Loop> loops;
  std::pmr::vector<Face> faces;
  std::pmr::vector<Carrier> carriers;
  std::pmr::vector<Curve> curves;
  std::pmr::vector<Style> styles;
  std::pmr::vector<Diagnostic> diagnostics;
  std::pmr::vector<DiagnosticValue> diagnostic_values;
  std::pmr::vector<OccurrenceStyle> occurrence_styles;
  std::pmr::vector<UsageId> usage_ids;
  std::pmr::vector<char> strings;
  std::pmr::vector<double> scalars;
  std::pmr::vector<uint32_t> multiplicities;
  std::pmr::vector<uint64_t> source_ids;
  std::pmr::vector<Vec2> parameter_maps;
  std::pmr::vector<FaceId> face_ids;
  std::pmr::vector<EdgeId> edge_ids;
  std::pmr::vector<VertexId> vertex_ids;
  std::pmr::vector<CoedgeId> coedge_ids;
  std::pmr::vector<LoopId> loop_ids;
  std::pmr::vector<SurfaceSpan> surface_spans;
  std::pmr::vector<CurveSpan> curve_spans;
  IdIndex<ProductTag> product_index;
  IdIndex<DefinitionTag> definition_index;
  IdIndex<UsageTag> usage_index;
  IdIndex<VertexTag> vertex_index;
  IdIndex<EdgeTag> edge_index;
  IdIndex<CoedgeTag> coedge_index;
  IdIndex<LoopTag> loop_index;
  IdIndex<FaceTag> face_index;
  IdIndex<CarrierTag> carrier_index;
  IdIndex<CurveTag> curve_index;
  IdIndex<StyleTag> style_index;
  Csr definition_faces, definition_edges, definition_vertices, edge_coedges, edge_faces;
  explicit SourceData(std::shared_ptr<RunResource> owner);
  SourceData(const SourceData &) = delete;
  SourceData &operator=(const SourceData &) = delete;
  [[nodiscard]] std::span<const double> view(Range<double> range) const;
  [[nodiscard]] std::span<const uint32_t> view(Range<uint32_t> range) const;
  [[nodiscard]] std::string_view text(Range<char> range) const;
};
class SourceStore final {
  std::shared_ptr<const SourceData> data_;
  explicit SourceStore(std::shared_ptr<const SourceData> data) : data_(std::move(data)) {}
  friend SourceStore read_source(std::span<const std::byte>, const std::shared_ptr<RunResource> &,
                                 SourceLimits);

public:
  [[nodiscard]] const SourceData &view() const noexcept { return *data_; }
};
// Refuse one face, by name, before a consumer reads its loops: a face refused
// at the source (its status) is never meshed, and this consumer takes
// edge-loop bounds only. A vertex_loop bound (a pole vertex, no uses) is
// carried by the source; a consumer that cannot mesh it refuses here.
inline void require_edge_loops(const SourceData &d, const Face &face) {
  if (face.status != source_status_admitted)
    for (std::size_t i = 0; i < face.loops.count; ++i) {
      const auto &loop = d.loops[d.loop_index.find(d.loop_ids[face.loops.first + i]).value];
      if (loop.status != source_status_admitted)
        throw Error(ErrorCode::unsupported,
                    "face " + std::to_string(face.id.value) + ": refused at the source (loop " +
                        std::to_string(loop.id.value) +
                        ", see its source_face_refused record); no consumer meshes it");
    }
  for (std::size_t i = 0; i < face.loops.count; ++i) {
    const auto &loop = d.loops[d.loop_index.find(d.loop_ids[face.loops.first + i]).value];
    if (loop.vertex)
      throw Error(ErrorCode::unsupported,
                  "face " + std::to_string(face.id.value) + ": vertex_loop bound " +
                      std::to_string(loop.id.value) + " at pole vertex " +
                      std::to_string(loop.vertex.value) +
                      ": this consumer takes edge-loop bounds only");
  }
}
[[nodiscard]] SourceStore read_source(std::span<const std::byte> bytes,
                                      const std::shared_ptr<RunResource> &resource,
                                      SourceLimits limits = {});
[[nodiscard]] SourceStore load_source(const std::filesystem::path &path,
                                      const std::shared_ptr<RunResource> &resource,
                                      SourceLimits limits = {});
} // namespace bgmesh
#endif
