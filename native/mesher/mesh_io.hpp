#ifndef BREPGPU_MESHER_MESH_IO_HPP
#define BREPGPU_MESHER_MESH_IO_HPP
#include "mesh.hpp"
#include <filesystem>

namespace bgmesh {
struct MeshIOLimits final {
  uint64_t bytes = uint64_t{16} << 30, rows = 100000000, work = uint64_t{1} << 40;
};
void write_native_mesh(const std::filesystem::path &, const NativeMesh &, MeshIOLimits = {});
[[nodiscard]] NativeMesh load_native_mesh(const std::filesystem::path &,
                                          const std::shared_ptr<RunResource> &, MeshIOLimits = {});
} // namespace bgmesh
#endif
