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
struct NativeInstance final {
  ProductId product;
  Slot<DefinitionTag> definition;
  Range<UsageId> path;
  Frame matrix;
  StyleId style;
};
struct AssemblyData final {
  std::shared_ptr<RunResource> resource;
  SourceStore source;
  ExactTable values;
  std::pmr::vector<DefinitionAllowance> definitions;
  std::pmr::vector<NativeInstance> instances;
  std::pmr::vector<UsageId> paths;
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
