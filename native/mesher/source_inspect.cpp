#include "source.hpp"
#if defined(BREPGPU_SOURCE_GEOMETRY)
#include "geometry_store.hpp"
#endif
#include <algorithm>
#include <chrono>
#include <iostream>
#include <map>

int main(int argc, char **argv) {
  if (argc != 2 && !(argc == 3 && std::string_view(argv[2]) == "--geometry")) {
    std::cerr << "usage: brepgpu-source-inspect source.bgsource [--geometry]\n";
    return 2;
  }
  try {
    const auto start = std::chrono::steady_clock::now();
    auto resource = std::make_shared<bgmesh::RunResource>(uint64_t{4} << 30);
    const auto source = bgmesh::load_source(argv[1], resource);
    const auto &d = source.view();
    // Ordinals are contiguous per definition, so its shell count is max + 1.
    std::map<uint64_t, uint32_t> definition_shells;
    for (const auto &face : d.faces) {
      auto &count = definition_shells[face.definition_id.value];
      count = std::max(count, face.shell + 1);
    }
    uint64_t shells = 0;
    for (const auto &entry : definition_shells)
      shells += entry.second;
    std::cout << "shells\t" << shells << "\nvoid_shells\t" << shells - definition_shells.size()
              << '\n';
    std::cout << "vertex_loops\t"
              << std::count_if(d.loops.begin(), d.loops.end(),
                               [](const auto &loop) { return loop.vertex.value != 0; })
              << "\noccurrence_styles\t" << d.occurrence_styles.size() << '\n';
    std::cout << "refused_faces\t"
              << std::count_if(d.faces.begin(), d.faces.end(),
                               [](const auto &f) { return f.status == bgmesh::source_status_refused; })
              << "\nrefused_loops\t"
              << std::count_if(d.loops.begin(), d.loops.end(),
                               [](const auto &l) { return l.status == bgmesh::source_status_refused; })
              << "\nincomplete_definitions\t"
              << std::count_if(d.definitions.begin(), d.definitions.end(), [](const auto &v) {
                   return v.status == bgmesh::definition_status_incomplete;
                 })
              << '\n';
    std::cout << "products\t" << d.products.size() << "\ndefinitions\t" << d.definitions.size()
              << "\nusages\t" << d.usages.size() << "\nvertices\t" << d.vertices.size()
              << "\nedges\t" << d.edges.size() << "\ncoedges\t" << d.coedges.size() << "\nloops\t"
              << d.loops.size() << "\nfaces\t" << d.faces.size() << "\ncarriers\t"
              << d.carriers.size() << "\ncurves\t" << d.curves.size() << "\nstyles\t"
              << d.styles.size() << "\ndiagnostics\t" << d.diagnostics.size()
              << "\noriginal_bytes\t" << d.original_bytes.size() << "\nsource_read_seconds\t"
              << std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count()
              << "\npeak_reserved_bytes\t" << resource->peak() << '\n';
    if (argc == 3) {
#if defined(BREPGPU_SOURCE_GEOMETRY)
      const auto geometry_start = std::chrono::steady_clock::now();
      bgfd::Budget budget({uint64_t{2} << 30, uint64_t{1} << 40}, resource.get());
      const auto geometry = bgmesh::compile_geometry(
          source, resource, {{bgmesh::OwnerKind::geometry, 0, 0}, 0, 0}, budget);
      std::cout << "original_curve_roots\t" << geometry.view().curve_roots.size()
                << "\noriginal_surface_roots\t" << geometry.view().surface_roots.size()
                << "\ngeometry_prepare_seconds\t"
                << std::chrono::duration<double>(std::chrono::steady_clock::now() - geometry_start)
                       .count()
                << "\ngeometry_exact_work\t" << budget.work() << "\ngeometry_scratch_peak_bytes\t"
                << budget.peak() << "\ncomplete_prepare_peak_reserved_bytes\t" << resource->peak()
                << '\n';
#else
      throw bgmesh::Error(bgmesh::ErrorCode::unsupported, "original geometry module is not built");
#endif
    }
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
#if defined(BREPGPU_SOURCE_GEOMETRY)
  catch (const bgfd::Failure &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
#endif
}
