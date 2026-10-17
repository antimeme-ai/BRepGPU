#include "adaptive_component_io.hpp"
#include "adaptive_caller_io.hpp"
#include "adaptive_trimmed_io.hpp"
#include <fstream>
#include <iomanip>
#include <unordered_map>

namespace bgmesh::adaptive::caller {
void close_stream(std::ofstream &stream) {
  stream.close();
  if (!stream)
    throw Error(ErrorCode::io_failure, "component output write/close failed");
}
void write_source(const std::filesystem::path &path, const SourceStore &source) {
  std::ofstream out(path, std::ios::binary);
  const auto &bytes = source.view().original_bytes;
  out.write(reinterpret_cast<const char *>(bytes.data()),
            static_cast<std::streamsize>(bytes.size()));
  close_stream(out);
}
void write_exact(std::ostream &out, ExactView value) {
  const auto words = [&](std::span<const uint32_t> limbs) {
    if (limbs.empty()) {
      out << '0';
      return;
    }
    out << std::hex << limbs.back();
    for (std::size_t i = limbs.size() - 1; i > 0; --i)
      out << std::setw(8) << std::setfill('0') << limbs[i - 1];
  };
  out << "r:";
  if (value.sign < 0)
    out << '-';
  words(value.numerator);
  out << '/';
  words(value.denominator);
  out << std::dec << std::setfill(' ');
}
CacheStats surface_totals(std::span<const TrimFaceStats> owners) {
  CacheStats out;
  const auto add = [&](uint64_t &to, uint64_t value) {
    if (value > UINT64_MAX - to)
      throw Error(ErrorCode::resource_exhausted, "component statistic count overflow");
    to += value;
  };
  for (const auto &owner : owners) {
    const auto &from = owner.surface;
    add(out.device_points, from.device_points);
    add(out.device_bounds, from.device_bounds);
    add(out.device_restrictions, from.device_restrictions);
    add(out.point_evaluations, from.point_evaluations);
    add(out.control_restrictions, from.control_restrictions);
    add(out.exact_fallbacks, from.exact_fallbacks);
    add(out.source_triangle_tests, from.source_triangle_tests);
    add(out.cap_facet_tests, from.cap_facet_tests);
    add(out.device_cap_facets, from.device_cap_facets);
    add(out.domain_replacements, from.domain_replacements);
    add(out.uploaded_bytes, from.uploaded_bytes);
    add(out.downloaded_bytes, from.downloaded_bytes);
    add(out.host_reserved_bytes, from.host_reserved_bytes);
    add(out.host_peak_bytes, from.host_peak_bytes);
    add(out.device_reserved_bytes, from.device_reserved_bytes);
    add(out.device_peak_bytes, from.device_peak_bytes);
  }
  return out;
}
void write_component(const std::filesystem::path &directory, const ComponentResult &result,
                     std::ofstream &index, uint64_t query,
                     const std::filesystem::path &relative_directory, bgfd::Budget &budget) {
  if (!std::filesystem::create_directory(directory))
    throw Error(ErrorCode::io_failure, "cannot create component query directory");
  const auto &source = result->source.source().view();
  // Result retains the leases until all writes close. Pointer identity only
  // finds an earlier semantic occurrence; hash iteration never orders output.
  std::pmr::unordered_map<const SharedCurveSnapshotData *, uint32_t> snapshots(
      result->resource.get());
  const auto snapshot = [&](const SharedCurveSnapshot &captured) {
    if (!captured || &captured->source.view() != &source ||
        captured->definition != result->query.definition ||
        captured->query_generation != result->query.generation)
      throw Error(ErrorCode::internal_error, "component export has a foreign boundary snapshot");
    const auto found = snapshots.find(captured.get());
    if (found != snapshots.end())
      return found->second;
    if (snapshots.size() >= UINT32_MAX)
      throw Error(ErrorCode::resource_exhausted, "component snapshot index exhausted");
    const auto id = static_cast<uint32_t>(snapshots.size());
    caller::write_curves(directory / ("snapshot-" + std::to_string(id) + ".bgacrv"), *captured,
                         budget);
    snapshots.emplace(captured.get(), id);
    return id;
  };
  for (const auto &face : result->faces) {
    const auto &key = face->construction->key;
    if (key.definition != result->query.definition || key.query != result->query.generation ||
        &face->construction->source.view() != &source)
      throw Error(ErrorCode::internal_error, "component export has a foreign face");
    const auto id = source.faces.at(key.face.value).id.value;
    const auto file = "face-" + std::to_string(id) + ".bgatrm";
    const auto captured = snapshot(face->boundary);
    caller::write_trimmed_face(directory / file, face, budget);
    index << query << '\t' << source.definitions.at(key.definition.value).id.value << '\t' << id
          << '\t' << relative_directory.generic_string() << '/' << file << '\t'
          << relative_directory.generic_string() << "/snapshot-" << captured << ".bgacrv\t"
          << face->boundary->boundary->epoch.value << '\t' << key.boundary_generation << '\n';
  }
  const auto final = snapshot(result->boundary);
  std::ofstream final_index(directory / "final-boundary.tsv");
  final_index << "snapshot\tepoch\n"
              << "snapshot-" << final << ".bgacrv\t" << result->boundary->boundary->epoch.value
              << '\n';
  close_stream(final_index);
}
} // namespace bgmesh::adaptive::caller
