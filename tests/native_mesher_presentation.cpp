#include "../native/mesher/presentation.hpp"
#include <iostream>
int main(int argc, char **argv) {
  try {
    if (argc != 2)
      throw std::runtime_error("expected PNG destination");
    bgfd::Budget budget({1ULL << 20, 1ULL << 20});
    const std::array<uint8_t, 16> image{255, 0, 0,   255, 0,   255, 0,   255,
                                        0,   0, 255, 255, 255, 255, 255, 255};
    bgmesh::write_png(argv[1], 2, 2, image, budget);
    std::vector<uint8_t> wide(8192 * 3 * 4);
    for (uint32_t y = 0; y < 3; ++y)
      for (uint32_t x = 0; x < 8192; ++x) {
        const auto at = 4 * (y * 8192 + x);
        wide[at] = static_cast<uint8_t>(x % 251);
        wide[at + 1] = static_cast<uint8_t>(y);
        wide[at + 2] = static_cast<uint8_t>((x + y) % 251);
        wide[at + 3] = 255;
      }
    bgmesh::write_png(std::string(argv[1]) + ".wide.png", 8192, 3, wide, budget);
    bgmesh::NativePresentation p(&budget);
    p.centers.push_back({0, 0, 0});
    p.explosion.push_back({0, 0, 0});
    bgom10::Request small{};
    small.width = 32;
    small.height = 32;
    std::vector<uint8_t> small_pixels(32 * 32 * 4);
    bool refused = false;
    try {
      bgmesh::annotate_frame(budget, small_pixels, small, p, true, {"TEST", ""});
    } catch (const bgmesh::Error &e) {
      refused = e.code == bgmesh::ErrorCode::resource_exhausted;
    }
    if (!refused)
      throw std::runtime_error("small image must refuse unplaceable occurrence labels");
    std::cout << "PASS\tPNG primary colors/multiblock and finite annotation capacity\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
