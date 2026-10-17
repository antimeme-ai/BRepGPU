#include "exact_metric.hpp"
#include "values.hpp"
#include <array>
#include <bit>
#include <limits>
namespace bgmesh {
namespace {
[[noreturn]] void invalid(const char *m) { throw bgfd::Failure(BGFD_INVALID_INPUT, m); }
[[noreturn]] void internal(const char *m) { throw bgfd::Failure(BGFD_INTERNAL_ERROR, m); }
Rational power_two(bgfd::Budget &b, int64_t exponent) {
  const std::array<uint32_t, 1> word{1};
  auto one = bgfd::Integer::from_words(b, 1, word);
  const auto magnitude =
      exponent < 0 ? static_cast<uint64_t>(-(exponent + 1)) + 1 : static_cast<uint64_t>(exponent);
  return exponent < 0 ? Rational::from_ratio(one, one.shift_left(magnitude))
                      : Rational::from_ratio(one.shift_left(magnitude), one);
}
int64_t logarithm(const Rational &r) {
  if (r.sign() == 0)
    invalid("metric logarithm of zero");
  const auto nb = r.numerator().bit_length(), db = r.denominator().bit_length();
  const auto difference = nb >= db ? nb - db : db - nb;
  if (difference > static_cast<uint64_t>(INT64_MAX / 4))
    throw Error(ErrorCode::resource_exhausted, "metric exponent range exhausted");
  int64_t exponent =
      nb >= db ? static_cast<int64_t>(difference) : -static_cast<int64_t>(difference);
  const auto n = r.sign() < 0 ? r.numerator().negative() : bgfd::Integer(r.numerator());
  const int compare = exponent >= 0
                          ? n.compare(r.denominator().shift_left(static_cast<uint64_t>(exponent)))
                          : n.shift_left(static_cast<uint64_t>(-exponent)).compare(r.denominator());
  if (compare < 0)
    --exponent;
  return exponent;
}
Rational rounded_dyadic53_impl(const Rational &r) {
  if (r.sign() == 0)
    return Rational(r.budget(), 0);
  const auto exponent = logarithm(r);
  const auto normalized = r.multiply(power_two(r.budget(), -exponent));
  return Rational::from_double(r.budget(), normalized.to_double())
      .multiply(power_two(r.budget(), exponent));
}
} // namespace
Rational rounded_dyadic53(const Rational &r) { return rounded_dyadic53_impl(r); }
Rational rounded_sqrt53(const Rational &value) {
  if (value.sign() < 0)
    invalid("negative metric square root");
  if (value.sign() == 0)
    return Rational(value.budget(), 0);
  auto &b = value.budget();
  const auto exponent = logarithm(value);
  auto root_exponent = exponent / 2;
  if (exponent < 0 && exponent % 2 != 0)
    --root_exponent;
  const auto normalized = value.multiply(power_two(b, -2 * root_exponent));
  const auto bounds = normalized.square_root_bounds(64);
  const double lower = bounds.lower.to_double(), upper = bounds.upper.to_double();
  double chosen = lower;
  if (lower != upper) {
    const auto lo = std::bit_cast<uint64_t>(lower), hi = std::bit_cast<uint64_t>(upper);
    if (hi != lo + 1)
      internal("normalized metric sqrt bracket width invariant");
    const auto midpoint =
        Rational::from_double(b, lower).add(Rational::from_double(b, upper)).divide(Rational(b, 2));
    const int side = normalized.compare(midpoint.multiply(midpoint));
    chosen = side < 0 ? lower : (side > 0 ? upper : ((lo & 1) == 0 ? lower : upper));
  }
  return Rational::from_double(b, chosen).multiply(power_two(b, root_exponent));
}
} // namespace bgmesh
