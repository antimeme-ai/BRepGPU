#ifndef BREPGPU_MESHER_GEOMETRY_POLE_CONTROL_HPP
#define BREPGPU_MESHER_GEOMETRY_POLE_CONTROL_HPP
#include "face_seed.hpp"
namespace bgmesh {
// Geometry owns cut continuation. The coordinator forwards these immutable
// values and routes source-edge versus face-cut epochs without evaluating CAD.
[[nodiscard]] PoleCutControl continue_pole_cut(bgfd::Budget &, const FaceBoundarySnapshot &,
                                               BoundaryKey);
} // namespace bgmesh
#endif
