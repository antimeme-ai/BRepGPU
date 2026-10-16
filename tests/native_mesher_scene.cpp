#include "../native/mesher/mesh_io.hpp"
#include "../native/mesher/scene_view.hpp"
#include "../native/om10/scene.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}
} // namespace
int main(int argc, char **argv) {
  try {
    using namespace bgmesh;
    require(argc == 3, "expected actual native mesh and output directory");
    const auto resource = std::make_shared<RunResource>(1ULL << 29);
    const auto mesh = load_native_mesh(argv[1], resource);
    bgfd::Budget budget({1ULL << 27, 1ULL << 38}, resource.get());
    const auto geometry = compile_geometry(mesh.view().source, resource,
                                           {{OwnerKind::geometry, UINT32_MAX, 71}, 0, 0}, budget);
    const auto scene = make_scene(budget, mesh, geometry);
    require(scene->triangles.size() == mesh.view().facets.size() &&
                scene->definitions.size() == mesh.view().definitions.size() &&
                scene->instances.size() == mesh.view().instances.size(),
            "native source display lost geometry/assembly inventory");
    for (const auto &vertex : scene->vertices) {
      require((vertex.flags & 33U) == 33U && vertex.physical > 0 &&
                  vertex.physical <= mesh.view().nodes.size(),
              "all native display vertices must retain physical key and stored/cofactor flags");
      const auto &point = mesh.view().nodes[static_cast<std::size_t>(vertex.physical - 1)].xyz;
      for (std::size_t axis = 0; axis < 3; ++axis)
        require(std::bit_cast<uint64_t>(vertex.xyz[axis]) == std::bit_cast<uint64_t>(point[axis]),
                "native display recomputed a stored physical coordinate");
    }
    for (std::size_t i = 0; i < scene->instances.size(); ++i)
      require(scene->instances[i].reserved == mesh.view().instances[i].appearance.value,
              "source occurrence appearance does not reach the display override");
    const auto view = scene->view();
    require(view.nplaced_vertices == scene->vertex_refs.size() &&
                view.nprimitives == scene->primitive_refs.size(),
            "native display flat placement references missing");
    const std::filesystem::path directory(argv[2]);
    std::filesystem::create_directories(directory);
    write_scene(directory / "source.bgscene", scene, budget);
    write_views(directory / "source.bgview", {}, budget);
    const bgom10::Scene decoded((directory / "source.bgscene").string());
    require(decoded.vertices.size() == scene->vertices.size() &&
                decoded.triangles.size() == scene->triangles.size(),
            "direct source scene is rejected or changed by shared file boundary");
    bool refused = false;
    try {
      bgom10::Request invalid{};
      write_views(directory / "invalid.bgview", std::span<const bgom10::Request>(&invalid, 1),
                  budget);
    } catch (const std::exception &) {
      refused = true;
    }
    require(refused, "native request writer emitted a request rejected by its reader");
    std::ifstream input(directory / "source.bgscene", std::ios::binary);
    std::array<char, 16> header{};
    input.read(header.data(), static_cast<std::streamsize>(header.size()));
    require(input && std::string_view(header.data(), 8) == "BGMESH01" && header[8] == 1 &&
                header[12] == 1,
            "source profile feature/version not selected in actual scene output");
    std::cout << "PASS\tactual native mesh to stored-XYZ source scene\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  } catch (const bgfd::Failure &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
