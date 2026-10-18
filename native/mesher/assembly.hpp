#ifndef BREPGPU_MESHER_ASSEMBLY_HPP
#define BREPGPU_MESHER_ASSEMBLY_HPP
#include "exact_store.hpp"
#include "source.hpp"

namespace bgmesh {
// Borrowed only for this synchronous preparation call. Geometry owns the full
// original envelope; XYZ stores lo/hi per axis and uses the caller's Budget.
struct DefinitionEnvelopeView final {
  SourceStore source;
  Slot<DefinitionTag> definition;
  std::span<const Rational, 6> xyz;
};
struct RequestedQuality final {
  Rational positional, node_source;
};
struct AssemblyPrecision final {
  uint32_t norm_fractional_bits = 96, allowance_fractional_bits = 128;
};
struct AssemblyLimits final {
  uint64_t instances = 10000000;
};
struct DefinitionAllowance final {
  Slot<DefinitionTag> definition;
  ExactRef positional, node_source;
};
struct OccurrenceFaceStyle final {
  FaceId face;
  StyleId style;
};
// Style of one occurrence under CAx-IF Model Styling v1.10 precedence (4.2.1,
// 4.3, 5): an explicit occurrence style (a usage style, or an occurrence-style
// row on a prefix of the path; the deepest wins, the row on a tie) beats every
// face style; else the outermost assembly-node product style does; else the
// part's own style applies and its faces keep theirs (G9 5.2 call, recorded by
// the adapter). Rows with a face restyle that face of exactly this path.
struct OccurrenceStyleResult final {
  StyleId style;
  bool over_faces = false;
};
[[nodiscard]] OccurrenceStyleResult
occurrence_style(const SourceData &, std::span<const UsageId> path, bgfd::Budget &,
                 std::pmr::vector<OccurrenceFaceStyle> *faces = nullptr);
struct NativeInstance final {
  ProductId product;
  Slot<DefinitionTag> definition;
  Range<UsageId> path;
  Frame matrix;
  StyleId style;
  // The occurrence style beats the definition's face styles.
  bool style_over_faces = false;
  Range<OccurrenceFaceStyle> face_styles;
};
struct AssemblyData final {
  std::shared_ptr<RunResource> resource;
  SourceStore source;
  ExactTable values;
  std::pmr::vector<DefinitionAllowance> definitions;
  std::pmr::vector<NativeInstance> instances;
  std::pmr::vector<UsageId> paths;
  std::pmr::vector<OccurrenceFaceStyle> face_styles;
  AssemblyData(std::shared_ptr<RunResource>, SourceStore, ExactTable);
};
class AssemblyStore final {
  std::shared_ptr<const AssemblyData> data_;
  explicit AssemblyStore(std::shared_ptr<const AssemblyData> data) : data_(std::move(data)) {}
  friend AssemblyStore prepare_assembly(bgfd::Budget &, const SourceStore &,
                                        const std::shared_ptr<RunResource> &,
                                        std::span<const DefinitionEnvelopeView>,
                                        const RequestedQuality &, ExactTableKey, AssemblyPrecision,
                                        AssemblyLimits);

public:
  [[nodiscard]] const AssemblyData &view() const noexcept { return *data_; }
};
[[nodiscard]] AssemblyStore prepare_assembly(bgfd::Budget &, const SourceStore &,
                                             const std::shared_ptr<RunResource> &,
                                             std::span<const DefinitionEnvelopeView>,
                                             const RequestedQuality &, ExactTableKey,
                                             AssemblyPrecision = {}, AssemblyLimits = {});
} // namespace bgmesh
#endif
