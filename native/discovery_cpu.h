#ifndef BREPGPU_DISCOVERY_CPU_H
#define BREPGPU_DISCOVERY_CPU_H
#include "discovery_backend_api.h"
#include <vector>
namespace brepgpu {
bool discovery_environment_supported() noexcept;
/* Inputs are owned validated snapshots; false is an enclosure/tree failure.
   Partial output vectors remain unpublished. Allocation exceptions propagate. */
bool build_discovery_hierarchy(const bg_patch_desc* patches,uint64_t patch_count,
    const double* controls,const bg_hierarchy_options& options,
    std::vector<bg_hierarchy_node>& nodes,std::vector<uint32_t>& roots,
    std::vector<double>& bounds);
/* Environment inspected by runtime on every actual worker before entry.
   Full count is the output SoA stride; begin/end partition only the rows. */
void discover_cpu_range(const bg_patch_desc* patches,uint64_t patch_count,
    const double* controls,const bg_hierarchy_node* nodes,uint64_t node_count,
    const uint32_t* roots,const double* bounds,uint32_t mode,
    const uint32_t* slots,const double* targets,uint64_t count,
    const bg_discovery_options* options,uint64_t begin,uint64_t end,
    double* fields,uint32_t* statuses,uint32_t* flags,uint32_t* visits,
    uint32_t* pruned,uint32_t* refinements,uint32_t* evaluations) noexcept;
}
#endif
