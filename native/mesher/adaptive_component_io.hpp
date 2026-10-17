#ifndef BREPGPU_MESHER_ADAPTIVE_COMPONENT_IO_HPP
#define BREPGPU_MESHER_ADAPTIVE_COMPONENT_IO_HPP
#include "adaptive_component.hpp"
#include <fstream>

namespace bgmesh::adaptive::caller {
void close_stream(std::ofstream &);
void write_source(const std::filesystem::path &, const SourceStore &);
void write_exact(std::ostream &, ExactView);
[[nodiscard]] CacheStats surface_totals(std::span<const TrimFaceStats>);
// relative_directory is the artifact path beneath the publication root. Keep
// nested model/query/definition paths in the index rather than just a basename.
void write_component(const std::filesystem::path &, const ComponentResult &, std::ofstream &,
                     uint64_t query, const std::filesystem::path &relative_directory,
                     bgfd::Budget &);
} // namespace bgmesh::adaptive::caller
#endif
