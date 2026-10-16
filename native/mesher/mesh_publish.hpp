#ifndef BREPGPU_MESHER_MESH_PUBLISH_HPP
#define BREPGPU_MESHER_MESH_PUBLISH_HPP
#include "boundary.hpp"
#include "face_result.hpp"
#include "mesh.hpp"
#include "run_preparation.hpp"
namespace bgmesh {
// Whole-run construction adapter. Inputs may arrive in completion order; the
// output is ordered by original source identities. No geometry is recomputed.
[[nodiscard]] NativeMesh publish_native_mesh(bgfd::Budget &, const PreparedRun &,
                                             std::span<const DefinitionBoundarySnapshot>,
                                             std::span<const FaceResult>, MeshQuality);
} // namespace bgmesh
#endif
