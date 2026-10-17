// Standalone entry to the unmodified official reference converter.
#include <SmApiGeneral.h>
#include <SmApiUsd.h>
#include <chrono>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>

namespace occt {
bool ConvertOcctFileToUsd(const std::string &, const std::string &, std::string &);
}
namespace fs = std::filesystem;
void require(bool value, const char *message) {
  if (!value) throw std::runtime_error(message);
}
int main(int argc, char **argv) {
  fs::path temporary;
  bool own_temporary = false;
  try {
    if (argc == 2 && std::string(argv[1]) == "--help") {
      std::cout << "Official NVIDIA CPU OCCT-to-USD conversion only; no tessellation.\n"
                   "INPUT.brep NEW_OUTPUT.usdc\n";
      return 0;
    }
    require(argc == 3, "usage: INPUT.brep NEW_OUTPUT.usdc");
    const fs::path input = argv[1], output = argv[2];
    require(!fs::exists(output), "output already exists");
    require(fs::is_regular_file(input), "input is not a regular file");
    temporary = output.string() + ".pending.usdc";
    require(!fs::exists(temporary), "temporary output already exists");
    const auto begin = std::chrono::steady_clock::now();
    SmApiCreateContext();
    require(SmApiUsdEnsurePluginRegistered() == SM_SUCCESS, "reference plugin unavailable");
    std::string message;
    own_temporary = true;
    if (!occt::ConvertOcctFileToUsd(input.string(), temporary.string(), message))
      throw std::runtime_error("official OCCT-to-USD bridge: " + message);
    require(fs::is_regular_file(temporary), "converter did not emit a USD file");
    fs::rename(temporary, output);
    temporary.clear();
    std::cout << std::setprecision(17) << "converted_seconds="
              << std::chrono::duration<double>(std::chrono::steady_clock::now()-begin).count()
              << '\n';
    return 0;
  } catch (const std::exception &error) {
    if (own_temporary && !temporary.empty()) {
      std::error_code ignored;
      fs::remove(temporary, ignored);
    }
    std::cerr << error.what() << '\n';
    return 1;
  }
}
