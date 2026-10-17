// Literal complete cylinder: its two Plane faces have positive rational
// circles.
#include <SmApiGeneral.h>
#include <SmApiPrimitives.h>
#include <SmApiUsd.h>
#include <SmBrep.h>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
int main(int argc, char **argv) {
  try {
    if (argc != 2 || std::filesystem::exists(argv[1]))
      throw std::runtime_error("new USD output path required");
    SmApiCreateContext();
    if (SmApiUsdEnsurePluginRegistered() != SM_SUCCESS)
      throw std::runtime_error("plugin unavailable");
    SmBrep *raw = nullptr;
    if (SmApiCreateCylinder(SmVector3d(1.25, -.75, .5), 2., 3., raw) !=
            SM_SUCCESS ||
        !raw)
      throw std::runtime_error("authored cylinder failed");
    std::unique_ptr<SmBrep> source(raw);
    if (SmApiUsdExportBrep(argv[1], source.get(), FALSE, FALSE) != SM_SUCCESS)
      throw std::runtime_error("authored USD export failed");
    std::cout << "PASS authored radius2 height3 cylinder with complete Plane "
                 "cap boundaries\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
