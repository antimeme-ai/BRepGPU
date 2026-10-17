#include "../native/mesher/exact.hpp"
#include <bit>
#include <cfenv>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory_resource>
#include <string>
#include <string_view>
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
uint32_t word(std::istream &in) {
  uint32_t result = 0;
  for (unsigned i = 0; i < 4; ++i) {
    const auto byte = in.get();
    require(byte != std::char_traits<char>::eof(), "truncated exact vector");
    result |= static_cast<uint32_t>(byte) << (8 * i);
  }
  return result;
}
uint64_t wide(std::istream &in) {
  const auto low = word(in);
  return low | (static_cast<uint64_t>(word(in)) << 32);
}
bgmesh::ExactWords words(std::istream &in) {
  bgmesh::ExactWords result;
  result.sign = std::bit_cast<int32_t>(word(in));
  const auto numerator_count = word(in), denominator_count = word(in);
  require(numerator_count < 10000 && denominator_count > 0 && denominator_count < 10000,
          "invalid exact vector size");
  result.numerator.resize(numerator_count);
  result.denominator.resize(denominator_count);
  for (auto &limb : result.numerator)
    limb = word(in);
  for (auto &limb : result.denominator)
    limb = word(in);
  return result;
}
void same(const bgmesh::Rational &actual, const bgmesh::ExactWords &expected, const char *message) {
  const auto exported = actual.export_words();
  require(exported.sign == expected.sign && exported.numerator == expected.numerator &&
              exported.denominator == expected.denominator,
          message);
}
template <typename Function>
void failure(Function function, bgfd_status status, const char *message) {
  bool caught = false;
  try {
    function();
  } catch (const bgfd::Failure &error) {
    caught = error.status == status;
  }
  require(caught, message);
}
class Upstream final : public std::pmr::memory_resource {
  void *do_allocate(std::size_t bytes, std::size_t alignment) override {
    if (reject)
      throw std::bad_alloc();
    void *result = std::pmr::new_delete_resource()->allocate(bytes, alignment);
    live += bytes;
    return result;
  }
  void do_deallocate(void *pointer, std::size_t bytes, std::size_t alignment) override {
    live -= bytes;
    std::pmr::new_delete_resource()->deallocate(pointer, bytes, alignment);
  }
  bool do_is_equal(const std::pmr::memory_resource &other) const noexcept override {
    return this == &other;
  }

public:
  std::size_t live = 0;
  bool reject = false;
};
void budget_scopes() {
  bgfd::Budget budget({10000, 100});
  budget.charge(7);
  {
    auto outer = budget.scoped_work(5);
    budget.charge(2);
    {
      auto inner = budget.scoped_work(1);
      budget.charge(1);
      failure([&] { budget.charge(1); }, BGFD_RESOURCE_EXHAUSTED,
              "nested work cap applies at individual operation");
    }
    budget.charge(2);
    failure([&] { budget.charge(1); }, BGFD_RESOURCE_EXHAUSTED,
            "inner scope restores outer remaining cap");
  }
  require(budget.work() == 12, "scoped cap erased cumulative charged work");
  budget.charge(88);
  failure([&] { budget.charge(1); }, BGFD_RESOURCE_EXHAUSTED,
          "scope did not preserve original run cap");
  bgfd::Budget exceptional({10000, 32});
  exceptional.charge(3);
  failure(
      [&] {
        auto scope = exceptional.scoped_work(0);
        exceptional.charge(1);
      },
      BGFD_RESOURCE_EXHAUSTED, "zero scoped cap stops before work");
  exceptional.charge(29);
  require(exceptional.work() == 32, "exception did not restore prior work cap");
  bgfd::Budget overflow({10000, 100});
  overflow.charge(9);
  {
    auto scope = overflow.scoped_work(UINT64_MAX);
    overflow.charge(91);
    failure([&] { overflow.charge(1); }, BGFD_RESOURCE_EXHAUSTED,
            "overflow-sized scope cannot widen parent cap");
  }
  bgfd::Budget owner({10000, 10000});
  const bgmesh::Rational held(owner, 17);
  failure(
      [&] {
        auto scope = owner.scoped_work(0);
        (void)held.add(held);
      },
      BGFD_RESOURCE_EXHAUSTED, "same-owner rational respects scoped cap");
  require(held.to_double() == 17., "scope changed exact owner/value lifetime");
}
void basic() {
  bgfd::Budget budget({1ULL << 24, 1ULL << 30});
  using bgmesh::Rational;
  Rational minimum(budget, std::numeric_limits<int64_t>::min());
  require(minimum.to_double() == -9223372036854775808.0, "signed minimum constructor");
  require(std::bit_cast<uint64_t>(Rational::from_double(budget, -0.0).to_double()) == 0,
          "canonical rational zero");
  failure(
      [&] {
        static_cast<void>(Rational::from_double(budget, std::numeric_limits<double>::infinity()));
      },
      BGFD_INVALID_INPUT, "infinity rejected");
  failure(
      [&] {
        static_cast<void>(Rational::from_double(budget, std::numeric_limits<double>::quiet_NaN()));
      },
      BGFD_INVALID_INPUT, "NaN rejected");
  failure([&] { static_cast<void>(Rational(budget, 1).divide(Rational(budget, 0))); },
          BGFD_INVALID_INPUT, "zero divisor rejected");
  failure([&] { static_cast<void>(Rational::from_words(budget, {1, {2}, {4}})); },
          BGFD_INVALID_INPUT, "noncanonical rational rejected");
  failure([&] { static_cast<void>(Rational::from_words(budget, {0, {}, {2}})); },
          BGFD_INVALID_INPUT, "noncanonical zero rejected");
  failure([&] { static_cast<void>(Rational::from_words(budget, {1, {1, 0}, {1}})); },
          BGFD_INVALID_INPUT, "high zero word rejected");
  auto six = bgfd::Integer::from_words(budget, 1, std::vector<uint32_t>{6});
  auto four = bgfd::Integer::from_words(budget, 1, std::vector<uint32_t>{4});
  failure([&] { static_cast<void>(six.divide_exact(four)); }, BGFD_INVALID_INPUT,
          "inexact integer division rejected");
  failure([&] { static_cast<void>(six.shift_right_exact(2)); }, BGFD_INVALID_INPUT,
          "inexact integer shift rejected");
  require(six.shift_left(97).shift_right_exact(97).compare(six) == 0, "cross-word shift");
  require(six.bit_length() == 3, "integer bit length");
  auto reduced = Rational::from_ratio(six.negative(), four.negative());
  same(reduced, {1, {3}, {2}}, "signed denominator canonicalization");
  bgfd::Budget other({1ULL << 24, 1ULL << 30});
  auto copied = reduced.clone_into(other);
  same(copied, {1, {3}, {2}}, "explicit owner clone");
  failure([&] { static_cast<void>(reduced.add(copied)); }, BGFD_INTERNAL_ERROR,
          "mixed-owner rational rejected");
  failure([&] { static_cast<void>(six.add(copied.numerator())); }, BGFD_INTERNAL_ERROR,
          "mixed-owner integer rejected");
  {
    bgfd::Budget tiny({4, 10000});
    failure([&] { static_cast<void>(Rational(tiny, 3)); }, BGFD_RESOURCE_EXHAUSTED,
            "memory limit applied to value");
  }
  {
    bgfd::Budget tiny({10000, 1});
    failure([&] { static_cast<void>(Rational::from_words(tiny, {1, {3}, {2}})); },
            BGFD_RESOURCE_EXHAUSTED, "work limit applied to value");
  }
  Upstream upstream;
  {
    bgfd::Budget nested({10000, 10000}, &upstream);
    {
      auto value = Rational(nested, 42);
      require(upstream.live > 0, "upstream allocation used");
    }
    require(upstream.live == 0, "upstream released");
    upstream.reject = true;
    bool rejected = false;
    try {
      auto value = Rational(nested, 7);
    } catch (const std::bad_alloc &) {
      rejected = true;
    }
    require(rejected, "upstream failure propagated");
    upstream.reject = false;
    auto value = Rational(nested, 7);
    require(value.to_double() == 7, "failed allocation did not poison budget");
  }
  require(upstream.live == 0, "upstream all storage released");
}
void vectors(const char *path) {
  std::ifstream in(path, std::ios::binary);
  std::string magic(8, '\0');
  in.read(magic.data(), 8);
  require(magic == "BGEXACT1", "wrong exact vector magic");
  const auto count = word(in), integer_count = word(in);
  require(count < 10000 && integer_count < 10000, "oversized exact vectors");
  bgfd::Budget budget({1ULL << 28, 1ULL << 36});
  for (uint32_t row = 0; row < count; ++row) {
    auto a = bgmesh::Rational::from_words(budget, words(in));
    auto b = bgmesh::Rational::from_words(budget, words(in));
    same(a.add(b), words(in), "Fraction add");
    same(a.subtract(b), words(in), "Fraction subtract");
    same(a.multiply(b), words(in), "Fraction multiply");
    const auto quotient = words(in);
    if (b.sign() != 0)
      same(a.divide(b), quotient, "Fraction divide");
    require(a.compare(b) == std::bit_cast<int32_t>(word(in)), "Fraction comparison");
    const auto overflow = word(in);
    const auto nearest = wide(in);
    if (overflow != 0)
      failure([&] { static_cast<void>(a.to_double()); }, BGFD_RESOURCE_EXHAUSTED,
              "binary64 nearest overflow");
    else
      require(std::bit_cast<uint64_t>(a.to_double()) == nearest, "Fraction nearest ties-even");
    const auto enclosure_overflow = word(in);
    const auto lower = wide(in), upper = wide(in);
    if (enclosure_overflow != 0)
      failure([&] { static_cast<void>(a.bounds()); }, BGFD_RESOURCE_EXHAUSTED,
              "binary64 enclosure overflow");
    else {
      const auto bounds = a.bounds();
      require(std::bit_cast<uint64_t>(bounds.lower) == lower &&
                  std::bit_cast<uint64_t>(bounds.upper) == upper,
              "Fraction outward neighbors");
    }
    // Exact IEEE decoding has a different independent Python integer-ratio
    // oracle.
    const auto input_bits = wide(in);
    same(bgmesh::Rational::from_double(budget, std::bit_cast<double>(input_bits)), words(in),
         "IEEE binary64 exact decoding");
  }
  for (uint32_t row = 0; row < integer_count; ++row) {
    auto n = bgmesh::Rational::from_words(budget, words(in));
    auto d = bgmesh::Rational::from_words(budget, words(in));
    const auto q = words(in), r = words(in), g = words(in);
    auto divided = bgfd::Integer::divmod_nonnegative(n.numerator(), d.numerator());
    same(bgmesh::Rational::from_ratio(
             std::move(divided.first),
             bgfd::Integer::from_words(budget, 1, std::vector<uint32_t>{1})),
         q, "Python integer quotient");
    same(bgmesh::Rational::from_ratio(
             std::move(divided.second),
             bgfd::Integer::from_words(budget, 1, std::vector<uint32_t>{1})),
         r, "Python integer remainder");
    auto gcd = bgfd::Integer::gcd_nonnegative(n.numerator(), d.numerator());
    same(bgmesh::Rational::from_ratio(
             std::move(gcd), bgfd::Integer::from_words(budget, 1, std::vector<uint32_t>{1})),
         g, "Python integer gcd");
  }
  require(in.get() == std::char_traits<char>::eof(), "extra exact vector bytes");
  require(budget.quotient_corrections() > 0, "Algorithm D trial correction exercised");
  require(budget.division_addbacks() > 0, "Algorithm D addback exercised");
  require(budget.lehmer_batches() > 0, "certified Lehmer batches exercised");
  std::cout << "rational_vectors=" << count << " integer_vectors=" << integer_count
            << " work=" << budget.work() << " max_limbs=" << budget.max_limbs()
            << " peak_bytes=" << budget.peak() << " divisions=" << budget.divisions()
            << " trial_corrections=" << budget.quotient_corrections()
            << " addbacks=" << budget.division_addbacks()
            << " lehmer_batches=" << budget.lehmer_batches() << '\n';
}
} // namespace
int main(int argc, char **argv) {
  if (argc == 2 && std::string_view(argv[1]) == "--budget-only") {
    budget_scopes();
    std::cout << "scoped exact budget PASS\n";
    return 0;
  }
  budget_scopes();
  require(argc == 2, "usage: exact_test vector-file");
  basic();
  const int rounding = std::fegetround();
  require(std::fesetround(FE_DOWNWARD) == 0, "set downward rounding");
#if defined(__x86_64__)
  const auto mxcsr = _mm_getcsr();
  _mm_setcsr(mxcsr | uint32_t{0x8040});
#endif
  vectors(argv[1]);
#if defined(__x86_64__)
  _mm_setcsr(mxcsr);
#endif
  require(std::fesetround(rounding) == 0, "restore rounding");
}
