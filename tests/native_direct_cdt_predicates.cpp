#include "../native/direct_cdt/predicates.hpp"
#include <bit>
#include <cfenv>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <vector>
#if defined(__x86_64__)
#include <xmmintrin.h>
#endif

namespace {
void require(bool condition, const char *message) {
  if (!condition) {
    std::cerr << message << '\n';
    std::exit(1);
  }
}
struct Input {
  std::vector<uint32_t> limbs;
  std::vector<bgfd_node> nodes;
  bgfd_input input{};
  bgfd_integer integer(int64_t value) {
    const int sign = value < 0 ? -1 : value > 0 ? 1 : 0;
    const uint64_t magnitude =
        value < 0 ? static_cast<uint64_t>(-(value + 1)) + 1 : static_cast<uint64_t>(value);
    const auto offset = static_cast<uint64_t>(limbs.size());
    if (magnitude != 0)
      limbs.push_back(static_cast<uint32_t>(magnitude));
    if (magnitude >> 32 != 0)
      limbs.push_back(static_cast<uint32_t>(magnitude >> 32));
    return {offset, static_cast<uint32_t>(limbs.size() - offset), sign};
  }
  void point(int64_t x, int64_t y) {
    nodes.push_back({static_cast<uint64_t>(nodes.size()), integer(x), integer(y)});
  }
  void finish(int64_t wx = 1, int64_t wy = 1) {
    input.x_weight = integer(wx);
    input.y_weight = integer(wy);
    input.nodes = nodes.data();
    input.node_count = nodes.size();
    input.limbs = limbs.data();
    input.limb_count = limbs.size();
  }
};
uint32_t read_word(std::istream &stream) {
  uint32_t result = 0;
  for (unsigned i = 0; i < 4; ++i) {
    const int byte = stream.get();
    require(byte != std::char_traits<char>::eof(), "truncated predicate vectors");
    result |= static_cast<uint32_t>(byte) << (8 * i);
  }
  return result;
}
bgfd_integer read_integer(std::istream &stream, Input &input) {
  const int32_t sign = std::bit_cast<int32_t>(read_word(stream));
  const uint32_t count = read_word(stream);
  require(count < 100000, "oversized predicate fixture integer");
  const auto offset = static_cast<uint64_t>(input.limbs.size());
  for (uint32_t i = 0; i < count; ++i)
    input.limbs.push_back(read_word(stream));
  return {offset, count, sign};
}
void batch(const char *path) {
  std::ifstream stream(path, std::ios::binary);
  std::string magic(8, '\0');
  stream.read(magic.data(), 8);
  require(magic == "BGCDTP01", "wrong predicate fixture magic");
  const uint32_t count = read_word(stream);
  require(count < 10000, "oversized predicate fixture batch");
  uint64_t scalar = 0, interval = 0, exact = 0, peak = 0;
  const auto begin = std::chrono::steady_clock::now();
  for (uint32_t i = 0; i < count; ++i) {
    Input input;
    input.input.x_weight = read_integer(stream, input);
    input.input.y_weight = read_integer(stream, input);
    for (uint32_t j = 0; j < 4; ++j) {
      const auto x = read_integer(stream, input), y = read_integer(stream, input);
      input.nodes.push_back({j, x, y});
    }
    const int32_t expected_orientation = std::bit_cast<int32_t>(read_word(stream));
    const int32_t expected_circle = std::bit_cast<int32_t>(read_word(stream));
    input.input.nodes = input.nodes.data();
    input.input.node_count = input.nodes.size();
    input.input.limbs = input.limbs.data();
    input.input.limb_count = input.limbs.size();
    bgfd::Budget budget({1ULL << 26, 1ULL << 32});
    for (const bool filtering : {true, false}) {
      bgcdt::Predicates predicates(budget, input.input, filtering);
      require(predicates.orient(0, 1, 2) == expected_orientation, "rational orientation vector");
      require(predicates.orient(1, 0, 2) == -expected_orientation, "orientation permutation");
      require(predicates.incircle(0, 1, 2, 3) == expected_circle, "rational metric circle vector");
      require(predicates.incircle(1, 0, 2, 3) == -expected_circle, "incircle permutation");
      scalar += predicates.stats().scalar_signs;
      interval += predicates.stats().interval_signs;
      exact += predicates.stats().exact_signs;
    }
    peak = std::max(peak, budget.peak());
  }
  require(stream.get() == std::char_traits<char>::eof(), "extra predicate vector bytes");
  require(scalar > 0 && interval > 0 && exact > 0, "all predicate stages exercised");
  const auto ns =
      std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - begin)
          .count();
  std::cout << "vectors=" << count << " scalar=" << scalar << " interval=" << interval
            << " exact=" << exact << " peak_bytes=" << peak << " whole_ns=" << ns << '\n';
}
} // namespace

int main(int argc, char **argv) {
  Input square;
  square.point(0, 0);
  square.point(1, 0);
  square.point(1, 1);
  square.point(0, 1);
  square.finish();
  bgfd::Budget budget({1ULL << 24, 1ULL << 26});
  bgcdt::Predicates predicates(budget, square.input);
  require(predicates.size() == 4, "point count");
  require(predicates.orient(0, 1, 2) == 1, "positive orientation");
  require(predicates.orient(0, 2, 1) == -1, "negative orientation");
  require(predicates.orient(0, 0, 1) == 0, "exact collinear zero");
  require(predicates.incircle(0, 1, 2, 3) == 0, "exact cocircular zero");
  require(predicates.compare_axis(0, 1, bgcdt::Axis::x) == -1, "exact axis order");
  require(predicates.compare_axis(0, 3, bgcdt::Axis::x) == 0, "exact axis tie");
  require(predicates.compare_line_distance(0, 1, 2, 3) == 0, "line distance tie");
  require(predicates.compare_line_distance(0, 1, 0, 2) == -1, "line distance order");
  Input anisotropic;
  anisotropic.point(0, 0);
  anisotropic.point(4, 0);
  anisotropic.point(4, 2);
  anisotropic.point(1, 4);
  anisotropic.finish();
  bgcdt::Predicates isotropic(budget, anisotropic.input);
  require(isotropic.incircle(0, 1, 2, 3) == -1, "isotropic outside");
  anisotropic.finish(4, 1);
  bgcdt::Predicates weighted(budget, anisotropic.input);
  require(weighted.incircle(0, 1, 2, 3) == 1, "metric reverses incircle sign");
  require(predicates.stats().scalar_signs > 0, "scalar filter exercised");
  require(predicates.stats().exact_zeros > 0, "exact fallback exercised");
  const int rounding = std::fegetround();
  require(std::fesetround(FE_DOWNWARD) == 0, "fixture changes rounding");
  bgcdt::Predicates downward_prepared(budget, anisotropic.input);
  require(downward_prepared.incircle(0, 1, 2, 3) == 1, "prepare under changed rounding");
  require(weighted.incircle(0, 1, 2, 3) == 1, "changed rounding exact fallback");
  require(std::fesetround(rounding) == 0, "fixture restores rounding");
#if defined(__x86_64__)
  const auto mxcsr = _mm_getcsr();
  _mm_setcsr(mxcsr | uint32_t{0x8040});
  bgcdt::Predicates ftz_prepared(budget, anisotropic.input);
  require(ftz_prepared.incircle(0, 1, 2, 3) == 1, "prepare under FTZ/DAZ");
  require(weighted.incircle(0, 1, 2, 3) == 1, "FTZ/DAZ exact fallback");
  _mm_setcsr(mxcsr);
#endif
  require(weighted.stats().floating_disabled >= 1, "floating environment detected");
  bool invalid_slot = false, invalid_line = false, exhausted = false, invalid_weight = false;
  try {
    static_cast<void>(weighted.orient(0, 1, std::numeric_limits<uint32_t>::max()));
  } catch (const bgfd::Failure &error) {
    invalid_slot = error.status == BGFD_INVALID_INPUT;
  }
  try {
    static_cast<void>(weighted.compare_line_distance(0, 0, 1, 2));
  } catch (const bgfd::Failure &error) {
    invalid_line = error.status == BGFD_INVALID_INPUT;
  }
  try {
    bgfd::Budget small({64, 10});
    bgcdt::Predicates rejected(small, square.input);
  } catch (const bgfd::Failure &error) {
    exhausted = error.status == BGFD_RESOURCE_EXHAUSTED;
  }
  bool work_exhausted = false;
  try {
    bgfd::Budget small({1ULL << 24, 0});
    bgcdt::Predicates rejected(small, square.input);
  } catch (const bgfd::Failure &error) {
    work_exhausted = error.status == BGFD_RESOURCE_EXHAUSTED;
  }
  square.finish(0, 1);
  try {
    bgcdt::Predicates rejected(budget, square.input);
  } catch (const bgfd::Failure &error) {
    invalid_weight = error.status == BGFD_INVALID_INPUT;
  }
  require(invalid_slot && invalid_line && exhausted && work_exhausted && invalid_weight,
          "typed predicate errors");
  if (argc == 2)
    batch(argv[1]);
  std::cout << "native direct CDT predicates PASS\n";
}
