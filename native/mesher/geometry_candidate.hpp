#ifndef BREPGPU_MESHER_GEOMETRY_CANDIDATE_HPP
#define BREPGPU_MESHER_GEOMETRY_CANDIDATE_HPP
#include "face_engine.hpp"
#include "geometry_protection.hpp"
namespace bgmesh {
[[nodiscard]] FaceCandidateResult evaluate_geometry_candidate(GeometryContext &,
                                                              const FaceCandidateWork &);
} // namespace bgmesh
#endif
