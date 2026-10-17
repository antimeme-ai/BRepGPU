#ifndef BREPGPU_MESHER_ADAPTIVE_TRIMMED_IO_HPP
#define BREPGPU_MESHER_ADAPTIVE_TRIMMED_IO_HPP
#include "adaptive_trimmed.hpp"
#include <filesystem>

namespace bgmesh::adaptive::caller {
void write_trimmed_face(const std::filesystem::path &, const FaceTrimResult &,
                        bgfd::Budget &);
} // namespace bgmesh::adaptive::caller
#endif
