#ifndef BREPGPU_MESHER_ADAPTIVE_ASSEMBLY_HPP
#define BREPGPU_MESHER_ADAPTIVE_ASSEMBLY_HPP
#include "assembly.hpp"

namespace bgmesh::adaptive {
struct AdaptiveAssemblyKeys final {
  ExactTableKey envelopes, allowances;
};
struct AdaptiveAssemblyPrecision final {
  AssemblyPrecision transport{};
};
struct AdaptiveAssemblyLimits final {
  uint64_t source_items = 100000000, work = uint64_t{1} << 38;
  AssemblyLimits transport{};
};
struct AdaptiveDefinitionEnvelope final {
  Slot<DefinitionTag> source;
  std::array<ExactRef, 6> xyz;
};
struct AdaptiveCarrierEnvelope final {
  Slot<CarrierTag> source;
  // Exact dyadic rectangle corresponding to cache_domain. Rational surfaces
  // retain their original natural rectangle, never the saved UV display hint.
  std::array<ExactRef, 4> uv;
  std::array<double, 4> cache_domain;
};
struct AdaptiveFaceEnvelope final {
  Slot<FaceTag> source;
  uint32_t carrier_envelope;
};
struct AdaptiveAssemblyData final {
  std::shared_ptr<RunResource> resource;
  SourceStore source;
  ExactTable envelope_values;
  AssemblyStore assembly;
  std::pmr::vector<AdaptiveDefinitionEnvelope> definitions;
  std::pmr::vector<AdaptiveCarrierEnvelope> carriers;
  std::pmr::vector<AdaptiveFaceEnvelope> faces;
  AdaptiveAssemblyData(std::shared_ptr<RunResource>, SourceStore, ExactTable, AssemblyStore);
  AdaptiveAssemblyData(const AdaptiveAssemblyData &) = delete;
  AdaptiveAssemblyData &operator=(const AdaptiveAssemblyData &) = delete;
};
using AdaptiveAssembly = std::shared_ptr<const AdaptiveAssemblyData>;
// Every subsequent source/cache/recovery profile must stay inside the retained
// carrier rectangle. A profile expansion requires new assembly preparation.
[[nodiscard]] AdaptiveAssembly
prepare_adaptive_assembly(bgfd::Budget &, const SourceStore &, const std::shared_ptr<RunResource> &,
                          const RequestedQuality &, AdaptiveAssemblyKeys,
                          AdaptiveAssemblyPrecision = {}, AdaptiveAssemblyLimits = {});
} // namespace bgmesh::adaptive
#endif
