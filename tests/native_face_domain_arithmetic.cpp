#include "exact.hpp"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <vector>

namespace {
[[nodiscard]] uint32_t word() {
  std::array<unsigned char, 4> bytes{};
  if (std::fread(bytes.data(), 1, 4, stdin) != 4)
    throw bgfd::Failure(BGFD_INVALID_INPUT, "truncated arithmetic vector");
  return static_cast<uint32_t>(bytes[0]) | (static_cast<uint32_t>(bytes[1]) << 8) |
         (static_cast<uint32_t>(bytes[2]) << 16) | (static_cast<uint32_t>(bytes[3]) << 24);
}
[[nodiscard]] bgfd::Integer integer(bgfd::Budget &budget) {
  const uint32_t sign = word(), count = word();
  if (sign > 2 || count > 1024)
    throw bgfd::Failure(BGFD_INVALID_INPUT, "invalid arithmetic vector size");
  std::vector<uint32_t> limbs;
  limbs.reserve(count);
  for (uint32_t i = 0; i < count; ++i)
    limbs.push_back(word());
  return bgfd::Integer::decode(budget, {0, count, static_cast<int32_t>(sign) - 1}, limbs.data(),
                               count);
}
} // namespace
int main() {
  try {
    const uint32_t count = word();
    if (count > 10000)
      return 2;
    bgfd::Budget budget({.bytes = 64U << 20, .work = 1ULL << 30});
    for (uint32_t i = 0; i < count; ++i) {
      const uint32_t operation = word();
      auto a = integer(budget), b = integer(budget), expected = integer(budget);
      auto actual = operation == 0 ? a.add(b) : operation == 1 ? a.subtract(b) : a.multiply(b);
      if (operation > 2 || actual.compare(expected) != 0) {
        std::fprintf(stderr, "arithmetic vector %u differs\n", i);
        return 1;
      }
    }
    if (std::fgetc(stdin) != EOF)
      return 2;
    std::printf("%u exact arithmetic vectors PASS\n", count);
    return 0;
  } catch (const bgfd::Failure &failure) {
    std::fprintf(stderr, "%s\n", failure.what());
    return 2;
  }
}
