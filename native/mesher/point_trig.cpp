#include "point_trig.hpp"
#include "point_fixed.hpp"
#include "point_trig_tables.hpp"

namespace bgmesh::point_detail {
namespace {
bgfd::Integer unit(bgfd::Budget &b) {
  const std::array<uint32_t, 1> one{1};
  return bgfd::Integer::from_words(b, 1, one);
}
bgfd::Integer absolute(const bgfd::Integer &value) {
  return value.sign() < 0 ? value.negative() : bgfd::Integer(value);
}
Fixed rounded(const bgfd::Integer &numerator, const bgfd::Integer &denominator, uint32_t bits,
              Directed direction) {
  auto &b = numerator.budget();
  auto [q, r] =
      bgfd::Integer::divmod_nonnegative(absolute(numerator).shift_left(bits), denominator);
  if (r.sign() != 0 && ((numerator.sign() > 0 && direction == Directed::up) ||
                        (numerator.sign() < 0 && direction == Directed::down)))
    q = q.add(unit(b));
  if (numerator.sign() < 0)
    q = q.negative();
  return Fixed::from_integer(q, bits);
}
const TrigTable &coefficients(uint32_t bits) {
  if (bits == 128)
    return table128;
  if (bits == 192)
    return table192;
  if (bits == 256)
    return table256;
  throw Error(ErrorCode::invalid_source, "unknown canonical point trig profile");
}
FixedInterval polynomial(const FixedArithmetic &a, const FixedInterval &x, const FixedInterval &xx,
                         const PolynomialTable &table, bool sine) {
  if (!table.count || table.count > table.coefficients.size())
    throw Error(ErrorCode::internal_error, "invalid point polynomial table");
  auto result = a.literal(table.coefficients[table.count - 1]);
  for (uint32_t i = table.count - 1; i-- > 0;)
    result = a.add(a.multiply(result, xx), a.literal(table.coefficients[i]));
  if (sine)
    result = a.multiply(result, x);
  const Fixed error(table.error);
  return a.add(result, {error.negative(), error});
}
ExactEnclosure exported(bgfd::Budget &b, const FixedInterval &x, uint32_t bits) {
  const auto denominator = unit(b).shift_left(bits);
  return {Rational::from_ratio(x.lower.as_integer(b), denominator),
          Rational::from_ratio(x.upper.as_integer(b), denominator)};
}
} // namespace
std::optional<SinCosEnclosure> try_sin_cos(bgfd::Budget &b, const Rational &x, Profile profile) {
  if (&x.budget() != &b)
    throw Error(ErrorCode::internal_error, "point trig exact owner mismatch");
  const auto &table = coefficients(profile.bits);
  if (profile.polynomial != PolynomialFamily::taylor &&
      profile.polynomial != PolynomialFamily::chebyshev)
    throw Error(ErrorCode::invalid_source, "unknown canonical point polynomial");
  b.charge(1);
  if (x.sign() == 0)
    return SinCosEnclosure{ExactEnclosure(Rational(b)), ExactEnclosure(Rational(b, 1))};
  const auto n = absolute(x.numerator());
  if (n.compare(x.denominator().shift_left(128)) > 0)
    return std::nullopt;
  const auto nb = n.bit_length(), db = x.denominator().bit_length();
  const auto magnitude_bits = nb > db ? nb - db + 1 : 1;
  const auto fractional = profile.bits + 32;
  const uint64_t required = uint64_t{fractional} + magnitude_bits + 8;
  if (required > 512 || required > profile.working_cap)
    return std::nullopt;
  try {
    // pi_floor512/2^512 < pi < (pi_floor512+1)/2^512. Prefix
    // truncation gives the same rigorous bracket at the requested precision.
    const auto pi_full = bgfd::Integer::from_words(b, 1, pi_floor512);
    auto [pi_low, unused] =
        bgfd::Integer::divmod_nonnegative(pi_full, unit(b).shift_left(512 - required));
    static_cast<void>(unused);
    const auto pi_high = pi_low.add(unit(b));
    const auto denominator = x.denominator().multiply(pi_low);
    auto [k, remainder] =
        bgfd::Integer::divmod_nonnegative(n.shift_left(required + 1), denominator);
    if (remainder.shift_left(1).compare(denominator) >= 0)
      k = k.add(unit(b));
    if (x.sign() < 0)
      k = k.negative();
    const auto kd = k.multiply(x.denominator()), scaled = x.numerator().shift_left(required + 1);
    const auto lower = scaled.subtract(kd.multiply(k.sign() < 0 ? pi_low : pi_high));
    const auto upper = scaled.subtract(kd.multiply(k.sign() < 0 ? pi_high : pi_low));
    const auto common = x.denominator().shift_left(required + 1);
    const FixedInterval reduced{rounded(lower, common, fractional, Directed::down),
                                rounded(upper, common, fractional, Directed::up)};
    const FixedArithmetic a(b, fractional);
    const auto one = a.constant(1), minusone = a.constant(-1);
    if (reduced.lower.compare(minusone.lower) < 0 || reduced.upper.compare(one.upper) > 0)
      return std::nullopt;
    const auto midpoint_sum = reduced.lower.add(b, reduced.upper, fractional);
    const auto anchor_index = midpoint_sum.nearest(fractional, 4);
    if (anchor_index < -32 || anchor_index > 32)
      return std::nullopt;
    const auto anchor = Fixed::dyadic({anchor_index,5}, {fractional});
    const auto delta = a.subtract(reduced, {anchor, anchor});
    const auto limit = Fixed::dyadic({1,5}, {fractional});
    if (delta.lower.compare(limit.negative()) < 0 || delta.upper.compare(limit) > 0)
      return std::nullopt;
    const auto square = a.square(delta);
    const auto offset = profile.polynomial == PolynomialFamily::chebyshev ? 2U : 0U;
    const auto sine = polynomial(a, delta, square, table.polynomials[offset], true);
    const auto cosine = polynomial(a, delta, square, table.polynomials[offset + 1], false);
    const auto index = static_cast<std::size_t>(anchor_index < 0 ? -anchor_index : anchor_index);
    const auto anchor_sine =
        anchor_index < 0 ? a.negative(a.literal(table.sine[index])) : a.literal(table.sine[index]);
    const auto anchor_cosine = a.literal(table.cosine[index]);
    auto s = a.add(a.multiply(anchor_sine, cosine), a.multiply(anchor_cosine, sine));
    auto c = a.subtract(a.multiply(anchor_cosine, cosine), a.multiply(anchor_sine, sine));
    uint32_t quadrant = k.sign() == 0 ? 0 : k.magnitude()[0] & 3U;
    if (k.sign() < 0)
      quadrant = (4 - quadrant) & 3U;
    if (quadrant == 1) {
      const auto old = s;
      s = c;
      c = a.negative(old);
    } else if (quadrant == 2) {
      s = a.negative(s);
      c = a.negative(c);
    } else if (quadrant == 3) {
      const auto old = s;
      s = a.negative(c);
      c = old;
    }
    const auto clip = [&](FixedInterval value) {
      if (value.lower.compare(minusone.lower) < 0)
        value.lower = minusone.lower;
      if (value.upper.compare(one.upper) > 0)
        value.upper = one.upper;
      if (value.lower.compare(value.upper) > 0)
        throw Error(ErrorCode::internal_error, "empty canonical trig enclosure");
      return value;
    };
    return SinCosEnclosure{exported(b, clip(s), fractional), exported(b, clip(c), fractional)};
  } catch (const FixedOverflow &) {
    return std::nullopt;
  }
}
} // namespace bgmesh::point_detail
