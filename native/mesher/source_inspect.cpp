#include "source.hpp"
#include <chrono>
#include <iostream>

int main(int argc, char **argv) {
  if (argc != 2) {
    std::cerr << "usage: brepgpu-source-inspect source.bgsource\n";
    return 2;
  }
  try {
    const auto start = std::chrono::steady_clock::now();
    auto resource = std::make_shared<bgmesh::RunResource>(uint64_t{4} << 30);
    const auto source = bgmesh::load_source(argv[1], resource);
    const auto &d = source.view();
    std::cout << "products\t" << d.products.size() << "\ndefinitions\t" << d.definitions.size()
              << "\nusages\t" << d.usages.size() << "\nvertices\t" << d.vertices.size()
              << "\nedges\t" << d.edges.size() << "\ncoedges\t" << d.coedges.size() << "\nloops\t"
              << d.loops.size() << "\nfaces\t" << d.faces.size() << "\ncarriers\t"
              << d.carriers.size() << "\ncurves\t" << d.curves.size() << "\nstyles\t"
              << d.styles.size() << "\ndiagnostics\t" << d.diagnostics.size()
              << "\noriginal_bytes\t" << d.original_bytes.size() << "\nsource_read_seconds\t"
              << std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count()
              << "\npeak_reserved_bytes\t" << resource->peak() << '\n';
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
