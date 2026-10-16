#ifndef BREPGPU_MESHER_RUN_PREPARATION_HPP
#define BREPGPU_MESHER_RUN_PREPARATION_HPP
#include "assembly.hpp"
#include "bound_runtime.hpp"
#include "geometry_charts.hpp"
#include "mesh_values.hpp"
#include "scheduler.hpp"
#include <chrono>

namespace bgmesh {
struct PreparedDefinitionEnvelope final {
  Slot<DefinitionTag> definition;
  std::array<ExactRef, 6> xyz;
};
struct RunPreparationTimes final {
  double source_seconds = 0, geometry_seconds = 0, envelopes_seconds = 0, assembly_seconds = 0,
         bank_seconds = 0;
};
struct PreparedRunData final {
  std::shared_ptr<RunResource> resource;
  GeometryStore geometry;
  AssemblyStore assembly;
  ExactTable envelopes;
  BoundRootBank bank;
  MeshQuality requested;
  std::pmr::vector<PreparedDefinitionEnvelope> definitions;
  RunPreparationTimes times;
  PreparedRunData(std::shared_ptr<RunResource>, GeometryStore, AssemblyStore, ExactTable,
                  BoundRootBank, MeshQuality);
};
using PreparedRun = std::shared_ptr<const PreparedRunData>;
struct RunPreparationInput final {
  std::filesystem::path source;
  double positional, node_source;
  SourceLimits source_limits{};
  AssemblyPrecision assembly_precision{};
  AssemblyLimits assembly_limits{};
};
class RunPreparationJob final : public NativeTask {
  std::shared_ptr<RunResource> resource_;
  RunPreparationInput input_;
  PreparedRun output_;
  bool started_ = false;

public:
  RunPreparationJob(const std::shared_ptr<RunResource> &, RunPreparationInput);
  StepState step(JobContext &) override;
  [[nodiscard]] PreparedRun result() const;
};
} // namespace bgmesh
#endif
