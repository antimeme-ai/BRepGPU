#ifndef BREPGPU_MESHER_MESHING_RUN_HPP
#define BREPGPU_MESHER_MESHING_RUN_HPP
#include "face_preparation_job.hpp"
#include "numerical_service.hpp"

namespace bgmesh {
struct MeshingRunLimits final {
  uint64_t coordinator_bytes = 1ULL << 31, coordinator_work = 1ULL << 44;
  FacePreparationLimits preparation{1ULL << 26, 1ULL << 40, {256, 32}};
  FaceOwnerLimits face{1ULL << 26, 1ULL << 40, 128};
  NumericalServiceLimits numerical{
      {256, 32, 1ULL << 26, 1ULL << 36, 1ULL << 44}, {1024, {256, 32}}, 128, {}};
  uint32_t numerical_services = 1, point_batch = 256, contact_batch = 256;
  ContactEpochLimits contacts{10000000};
  uint64_t seed = 0;
  bool allow_plane_profile = true;
  std::optional<BoundCudaModule> cuda;
  std::vector<BoundDeviceLimits> devices;
};
struct MeshingRunStats final {
  uint64_t scheduler_work = 0, scheduler_scratch_peak = 0;
  uint64_t owner_work = 0, owner_peak = 0;
  uint64_t boundary_rounds = 0, contact_rounds = 0, optimizer_proposals = 0, optimizer_flips = 0;
  uint64_t boundary_points = 0, final_sites = 0, final_cells = 0;
  uint64_t numerical_cpu_tasks = 0, numerical_device_tasks = 0, numerical_exact_tasks = 0;
  uint64_t numerical_work = 0, contact_candidates = 0;
  double seconds = 0;
};
struct MeshedRunData final {
  PreparedRun preparation;
  std::pmr::vector<DefinitionBoundarySnapshot> boundaries;
  std::pmr::vector<FaceResult> faces;
  MeshingRunStats stats;
  explicit MeshedRunData(PreparedRun source)
      : preparation(std::move(source)), boundaries(preparation->resource.get()),
        faces(preparation->resource.get()) {}
};
using MeshedRun = std::shared_ptr<const MeshedRunData>;
// The scheduler belongs to this run and must have no outstanding caller tasks.
// Failure stops/drains its children before mutable owners are destroyed. Only
// complete source inventories return; no partial definition is published.
[[nodiscard]] MeshedRun mesh_prepared_run(const PreparedRun &, NativeScheduler &,
                                         const MeshingRunLimits & = {});
} // namespace bgmesh
#endif
