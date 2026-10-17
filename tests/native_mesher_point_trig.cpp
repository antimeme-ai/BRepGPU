#include "../native/mesher/point_fixed.hpp"
#include "../native/mesher/point_trig.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <stdexcept>
namespace {
using namespace bgmesh;
void require(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}
uint32_t word(std::ifstream &input) {
  std::array<unsigned char, 4> bytes{};
  input.read(reinterpret_cast<char *>(bytes.data()), 4);
  require(bool(input), "truncated point vector");
  uint32_t value = 0;
  for (uint32_t i = 0; i < 4; ++i)
    value |= uint32_t{bytes[i]} << (8 * i);
  return value;
}
Rational rational(bgfd::Budget &budget, std::ifstream &input) {
  const int sign = std::bit_cast<int32_t>(word(input));
  const auto nc = word(input), dc = word(input);
  require(nc < 1024 && dc < 1024 && dc > 0, "invalid point vector shape");
  std::pmr::vector<uint32_t> n(&budget), d(&budget);
  for (uint32_t i = 0; i < nc; ++i)
    n.push_back(word(input));
  for (uint32_t i = 0; i < dc; ++i)
    d.push_back(word(input));
  return Rational::from_ratio(bgfd::Integer::from_words(budget, sign, n),
                              bgfd::Integer::from_words(budget, 1, d));
}
void run(const char *path) {
  bgfd::Budget budget({uint64_t{1} << 29, uint64_t{1} << 40});
  for (const uint32_t bits : {160U, 224U, 288U}) {
    using namespace point_detail;
    const FixedScale scale{bits};
    const auto tiny = Fixed::dyadic({1, bits}, scale), half = Fixed::dyadic({1, 1}, scale);
    const auto pd = tiny.multiply(budget, half, bits, Directed::down),
               pu = tiny.multiply(budget, half, bits, Directed::up);
    const auto nd = tiny.negative().multiply(budget, half, bits, Directed::down),
               nu = tiny.negative().multiply(budget, half, bits, Directed::up);
    require(pd.sign() == 0 && pu.compare(tiny) == 0 && nd.compare(tiny.negative()) == 0 &&
                nu.sign() == 0,
            "signed discarded product must enclose half grid unit");
    FixedLiteral maximum{1, {}};
    for (uint32_t i = 0; i <= bits / 32; ++i)
      maximum.words[i] = UINT32_MAX;
    const Fixed largest(maximum), one = Fixed::integer(1, scale);
    require(largest.multiply(budget, one, bits, Directed::down).compare(largest) == 0,
            "full-limb carry multiplication changed exact identity");
    bool overflow = false;
    try {
      static_cast<void>(largest.add(budget, tiny, bits));
    } catch (const FixedOverflow &) {
      overflow = true;
    }
    require(overflow, "fixed addition silently wrapped headroom");
    overflow = false;
    try {
      static_cast<void>(largest.multiply(budget, largest, bits, Directed::up));
    } catch (const FixedOverflow &) {
      overflow = true;
    }
    require(overflow, "fixed product silently wrapped headroom");
    const FixedArithmetic arithmetic(budget, bits);
    constexpr std::array<std::array<int32_t, 2>, 3> signed_intervals{{{-3, -1}, {-2, 3}, {1, 4}}};
    for (const auto &left : signed_intervals)
      for (const auto &right : signed_intervals) {
        const auto product =
            arithmetic.multiply({Fixed::integer(left[0], scale), Fixed::integer(left[1], scale)},
                                {Fixed::integer(right[0], scale), Fixed::integer(right[1], scale)});
        const std::array literal_products{left[0] * right[0], left[0] * right[1],
                                          left[1] * right[0], left[1] * right[1]};
        const auto [lo, hi] = std::minmax_element(literal_products.begin(), literal_products.end());
        require(product.lower.compare(Fixed::integer(*lo, scale)) == 0 &&
                    product.upper.compare(Fixed::integer(*hi, scale)) == 0,
                "monotone fixed product selected wrong signed endpoints");
      }
    const FixedInterval a{Fixed::integer(-1, scale), Fixed::integer(2, scale)},
        b{Fixed::integer(-3, scale), Fixed::integer(4, scale)};
    const auto cross = arithmetic.multiply(a, b), squared = arithmetic.square(a);
    require(cross.lower.compare(Fixed::integer(-6, scale)) == 0 &&
                cross.upper.compare(Fixed::integer(8, scale)) == 0 && squared.lower.sign() == 0 &&
                squared.upper.compare(Fixed::integer(4, scale)) == 0,
            "cross-zero fixed interval arithmetic lost an extremum");
  }
  std::ifstream input(path, std::ios::binary);
  std::array<char, 8> magic{};
  input.read(magic.data(), 8);
  require(std::string_view(magic.data(), 8) == "BGPTV001", "point vector magic");
  const auto count = word(input);
  for (uint32_t i = 0; i < count; ++i) {
    const auto bits = word(input);
    const auto x = rational(budget, input);
    const std::array expected{rational(budget, input), rational(budget, input),
                              rational(budget, input), rational(budget, input)};
    for (const auto family :
         {point_detail::PolynomialFamily::taylor, point_detail::PolynomialFamily::chebyshev}) {
      const auto result = point_detail::try_sin_cos(budget, x, {bits, 4096, family});
      if (!result.has_value())
        throw std::runtime_error("in-profile fixed trig unexpectedly unresolved");
      const auto &answer = result.value();
      const bool contained = answer.sine.lower.compare(expected[0]) <= 0 &&
                             answer.sine.upper.compare(expected[1]) >= 0 &&
                             answer.cosine.lower.compare(expected[2]) <= 0 &&
                             answer.cosine.upper.compare(expected[3]) >= 0;
      if (!contained)
        throw std::runtime_error("fixed trig oracle case" + std::to_string(i) + " bits" +
                                 std::to_string(bits));
    }
    if (bits == 128) {
      Curve circle{};
      circle.kind = 2;
      circle.dimension = 3;
      circle.radii = {1, 1};
      circle.frame = {0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1};
      GeometryContext context(budget);
      const auto canonical = canonical_analytic_curve(context, circle, x);
      const auto same = [](double a, double b) {
        return std::bit_cast<uint64_t>(a) == std::bit_cast<uint64_t>(b);
      };
      require(same(expected[0].to_double(), expected[1].to_double()) &&
                  same(expected[2].to_double(), expected[3].to_double()),
              "independent oracle did not isolate a nearest word");
      require(canonical.dimension == 3 && same(canonical.xyz[0], expected[2].to_double()) &&
                  same(canonical.xyz[1], expected[0].to_double()) && same(canonical.xyz[2], 0),
              "canonical POINT changed independent correct-nearest source result");
    }
  }
  require(input.peek() == std::char_traits<char>::eof(), "trailing point vectors");
  const auto outside = Rational::from_ratio(Rational(budget, 1).numerator().shift_left(129),
                                            Rational(budget, 1).numerator());
  require(!point_detail::try_sin_cos(budget, outside,
                                     {128, 4096, point_detail::PolynomialFamily::chebyshev}),
          "out-of-profile angle admitted");
  require(!point_detail::try_sin_cos(budget, Rational(budget, 1),
                                     {128, 128, point_detail::PolynomialFamily::chebyshev}),
          "filter ignored total working cap");
  std::cout << "PASS directed point trig " << count << " independent cases\n";
}
} // namespace
int main(int argc, char **argv) {
  try {
    require(argc == 2, "expected point vectors");
    run(argv[1]);
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
