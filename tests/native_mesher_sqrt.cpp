#include "../native/mesher/exact.hpp"
#include <bit>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>

namespace {
void require(bool condition, const char *message) {
  if (!condition) {
    std::cerr << message << '\n';
    std::exit(1);
  }
}
uint32_t word(std::istream &in) {
  uint32_t result = 0;
  for (unsigned i = 0; i < 4; ++i) {
    const auto byte = in.get();
    require(byte != std::char_traits<char>::eof(), "truncated sqrt vector");
    result |= static_cast<uint32_t>(byte) << (8 * i);
  }
  return result;
}
bgmesh::Rational rational(std::istream &in, bgfd::Budget &budget) {
  bgmesh::ExactWords words;
  words.sign = std::bit_cast<int32_t>(word(in));
  const auto n = word(in), d = word(in);
  require(n < 10000 && d > 0 && d < 10000, "invalid sqrt vector size");
  words.numerator.resize(n);
  words.denominator.resize(d);
  for (auto &x : words.numerator)
    x = word(in);
  for (auto &x : words.denominator)
    x = word(in);
  return bgmesh::Rational::from_words(budget, words);
}
void check(const char *path) {
  bgfd::Budget errors({1ULL << 20, 10000});
  bool negative = false;
  try {
    static_cast<void>(bgmesh::Rational(errors, -1).square_root_bounds(0));
  } catch (const bgfd::Failure &failure) {
    negative = failure.status == BGFD_INVALID_INPUT;
  }
  require(negative, "negative rational square root admitted");
  bool exhausted = false;
  try {
    static_cast<void>(
        bgmesh::Rational(errors, 2).square_root_bounds(std::numeric_limits<uint32_t>::max()));
  } catch (const bgfd::Failure &failure) {
    exhausted = failure.status == BGFD_RESOURCE_EXHAUSTED;
  }
  require(exhausted, "sqrt precision bypassed work/storage budget");
  std::ifstream in(path, std::ios::binary);
  std::string magic(8, '\0');
  in.read(magic.data(), 8);
  require(magic == "BGMSQRT1", "invalid sqrt vector header");
  const auto count = word(in);
  require(count > 0 && count < 1000, "invalid sqrt vector count");
  uint64_t work = 0, peak = 0, max_limbs = 0;
  for (uint32_t row = 0; row < count; ++row) {
    bgfd::Budget budget({1ULL << 26, 1ULL << 34});
    const auto value = rational(in, budget);
    const auto bits = word(in);
    const auto lower = rational(in, budget), upper = rational(in, budget);
    const auto actual = value.square_root_bounds(bits);
    require(&actual.lower.budget() == &budget && &actual.upper.budget() == &budget,
            "sqrt result owner changed");
    require(actual.lower.compare(lower) == 0 && actual.upper.compare(upper) == 0,
            "independent integer-isqrt bounds disagree");
    require(actual.lower.multiply(actual.lower).compare(value) <= 0 &&
                actual.upper.multiply(actual.upper).compare(value) >= 0,
            "sqrt bounds do not enclose original rational");
    work += budget.work();
    peak = std::max(peak, budget.peak());
    max_limbs = std::max(max_limbs, budget.max_limbs());
  }
  require(in.get() == std::char_traits<char>::eof(), "extra sqrt vector bytes");
  std::cout << "sqrt_vectors=" << count << " charged_work=" << work << " max_limbs=" << max_limbs
            << " peak_bytes=" << peak << '\n';
}
} // namespace
int main(int argc, char **argv) {
  require(argc == 2, "usage: native_mesher_sqrt vector-file");
  check(argv[1]);
}
