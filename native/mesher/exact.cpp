#include "exact.hpp"
#include <bit>
#include <limits>

namespace {
constexpr uint64_t radix = uint64_t{1} << 32;
bgfd::Integer integer(bgfd::Budget &budget, uint64_t magnitude, int sign = 1) {
  const uint32_t words[]{static_cast<uint32_t>(magnitude), static_cast<uint32_t>(magnitude >> 32)};
  return bgfd::Integer::from_words(budget, magnitude == 0 ? 0 : sign,
                                   std::span(words, magnitude == 0  ? 0
                                                    : words[1] == 0 ? 1
                                                                    : 2));
}
bgfd::Integer signed_integer(bgfd::Budget &budget, int64_t value) {
  const auto magnitude =
      value < 0 ? static_cast<uint64_t>(-(value + 1)) + 1 : static_cast<uint64_t>(value);
  return integer(budget, magnitude, value < 0 ? -1 : 1);
}
bgfd::Integer absolute(const bgfd::Integer &value) {
  return value.sign() < 0 ? value.negative() : bgfd::Integer(value);
}
bool sum_checked(int64_t a, int64_t b, int64_t &result) noexcept {
  if ((b > 0 && a > std::numeric_limits<int64_t>::max() - b) ||
      (b < 0 && a < std::numeric_limits<int64_t>::min() - b))
    return false;
  result = a + b;
  return true;
}
} // namespace

namespace bgfd {
Integer Integer::from_words(Budget &budget, int sign, std::span<const uint32_t> words) {
  if (sign < -1 || sign > 1 || ((sign == 0) != words.empty()) ||
      (!words.empty() && words.back() == 0))
    throw Failure(BGFD_INVALID_INPUT, "noncanonical exact integer words");
  budget.charge(static_cast<uint64_t>(words.size()));
  Integer result(budget);
  result.limbs_.assign(words.begin(), words.end());
  result.sign_ = sign;
  budget.observe(words.size());
  return result;
}
Integer Integer::clone_into(Budget &destination) const {
  return from_words(destination, sign_, limbs_);
}
uint64_t Integer::bit_length() const {
  budget_->charge(1);
  if (limbs_.empty())
    return 0;
  if (limbs_.size() - 1 > (std::numeric_limits<uint64_t>::max() - 32) / 32)
    throw Failure(BGFD_RESOURCE_EXHAUSTED, "integer bit length overflow");
  return static_cast<uint64_t>(limbs_.size() - 1) * 32 +
         static_cast<uint64_t>(32 - std::countl_zero(limbs_.back()));
}
Integer Integer::shift_left(uint64_t bits) const {
  if (sign_ == 0)
    return Integer(*budget_);
  const auto whole = bits / 32;
  const auto partial = static_cast<unsigned>(bits % 32);
  const auto extra = static_cast<uint64_t>(partial != 0);
  if (whole > std::numeric_limits<std::size_t>::max() - limbs_.size() ||
      extra > std::numeric_limits<std::size_t>::max() - limbs_.size() - whole)
    throw Failure(BGFD_RESOURCE_EXHAUSTED, "integer shift storage overflow");
  const auto size = limbs_.size() + static_cast<std::size_t>(whole + extra);
  budget_->charge(static_cast<uint64_t>(size));
  Integer result(*budget_);
  result.limbs_.resize(size);
  uint64_t carry = 0;
  for (std::size_t i = 0; i < limbs_.size(); ++i) {
    const uint64_t word = (static_cast<uint64_t>(limbs_[i]) << partial) | carry;
    result.limbs_[static_cast<std::size_t>(whole) + i] = static_cast<uint32_t>(word);
    carry = word >> 32;
  }
  if (partial != 0)
    result.limbs_.back() = static_cast<uint32_t>(carry);
  result.sign_ = sign_;
  result.trim();
  return result;
}
Integer Integer::shift_right_exact(uint64_t bits) const {
  if (sign_ == 0)
    return Integer(*budget_);
  const auto whole = bits / 32;
  const auto partial = static_cast<unsigned>(bits % 32);
  if (whole >= limbs_.size())
    throw Failure(BGFD_INVALID_INPUT, "inexact integer right shift");
  budget_->charge(static_cast<uint64_t>(limbs_.size()));
  for (uint64_t i = 0; i < whole; ++i)
    if (limbs_[static_cast<std::size_t>(i)] != 0)
      throw Failure(BGFD_INVALID_INPUT, "inexact integer right shift");
  if (partial != 0 &&
      (limbs_[static_cast<std::size_t>(whole)] & ((uint32_t{1} << partial) - 1)) != 0)
    throw Failure(BGFD_INVALID_INPUT, "inexact integer right shift");
  Integer result(*budget_);
  result.limbs_.resize(limbs_.size() - static_cast<std::size_t>(whole));
  for (std::size_t i = 0; i < result.limbs_.size(); ++i) {
    const auto from = i + static_cast<std::size_t>(whole);
    uint32_t word = limbs_[from] >> partial;
    if (partial != 0 && from + 1 < limbs_.size())
      word |= limbs_[from + 1] << (32 - partial);
    result.limbs_[i] = word;
  }
  result.sign_ = sign_;
  result.trim();
  return result;
}
std::pair<Integer, Integer> Integer::divmod_nonnegative(const Integer &dividend,
                                                        const Integer &divisor) {
  dividend.require_same_owner(divisor);
  if (dividend.sign_ < 0 || divisor.sign_ <= 0)
    throw Failure(BGFD_INVALID_INPUT, "nonnegative division requires positive divisor");
  Budget &budget = *dividend.budget_;
  budget.observe_division();
  Integer quotient(budget), remainder(budget);
  if (magnitude_compare(dividend, divisor) < 0)
    return {std::move(quotient), Integer(dividend)};
  const auto n = divisor.limbs_.size(), m = dividend.limbs_.size() - n;
  if (n == 1) {
    budget.charge(static_cast<uint64_t>(dividend.limbs_.size()));
    quotient.limbs_.resize(dividend.limbs_.size());
    uint64_t carry = 0;
    for (std::size_t i = dividend.limbs_.size(); i > 0; --i) {
      const uint64_t value = (carry << 32) | dividend.limbs_[i - 1];
      quotient.limbs_[i - 1] = static_cast<uint32_t>(value / divisor.limbs_[0]);
      carry = value % divisor.limbs_[0];
    }
    quotient.sign_ = 1;
    quotient.trim();
    return {std::move(quotient), integer(budget, carry)};
  }
  // Exact power-of-two divisors avoid normalization and trial division
  // entirely.
  bool power_two = std::has_single_bit(divisor.limbs_.back());
  for (std::size_t i = 0; i + 1 < n; ++i) {
    budget.charge(1);
    power_two = power_two && divisor.limbs_[i] == 0;
  }
  if (power_two) {
    const auto partial = static_cast<unsigned>(std::countr_zero(divisor.limbs_.back()));
    const auto whole = n - 1;
    budget.charge(static_cast<uint64_t>(dividend.limbs_.size()));
    quotient.limbs_.resize(dividend.limbs_.size() - whole);
    for (std::size_t i = 0; i < quotient.limbs_.size(); ++i) {
      uint32_t value = dividend.limbs_[whole + i] >> partial;
      if (partial != 0 && whole + i + 1 < dividend.limbs_.size())
        value |= dividend.limbs_[whole + i + 1] << (32 - partial);
      quotient.limbs_[i] = value;
    }
    remainder.limbs_.assign(dividend.limbs_.begin(),
                            dividend.limbs_.begin() + static_cast<std::ptrdiff_t>(whole));
    if (partial != 0)
      remainder.limbs_.push_back(dividend.limbs_[whole] & ((uint32_t{1} << partial) - 1));
    quotient.sign_ = remainder.sign_ = 1;
    quotient.trim();
    remainder.trim();
    return {std::move(quotient), std::move(remainder)};
  }
  // Knuth Algorithm D, radix 2^32. The leading normalized divisor limb >=2^31.
  // Each product plus carry fits uint64_t; quotient estimation avoids a 65th
  // bit.
  const auto shift = static_cast<unsigned>(std::countl_zero(divisor.limbs_.back()));
  Integer v = divisor.shift_left(shift), u = dividend.shift_left(shift);
  if (dividend.limbs_.size() == std::numeric_limits<std::size_t>::max())
    throw Failure(BGFD_RESOURCE_EXHAUSTED, "division scratch size overflow");
  u.limbs_.resize(dividend.limbs_.size() + 1);
  quotient.limbs_.resize(m + 1);
  for (std::size_t at = m + 1; at > 0; --at) {
    const auto j = at - 1;
    budget.charge(1);
    uint64_t trial = 0, residual = 0;
    if (u.limbs_[j + n] == v.limbs_[n - 1]) {
      trial = radix - 1;
      residual = static_cast<uint64_t>(u.limbs_[j + n - 1]) + v.limbs_[n - 1];
    } else {
      const uint64_t top = (static_cast<uint64_t>(u.limbs_[j + n]) << 32) | u.limbs_[j + n - 1];
      trial = top / v.limbs_[n - 1];
      residual = top % v.limbs_[n - 1];
    }
    while (residual < radix && trial * v.limbs_[n - 2] > (residual << 32) + u.limbs_[j + n - 2]) {
      --trial;
      residual += v.limbs_[n - 1];
      budget.observe_quotient_correction();
    }
    budget.charge(static_cast<uint64_t>(n));
    uint64_t borrow = 0;
    for (std::size_t i = 0; i < n; ++i) {
      const uint64_t product = trial * v.limbs_[i] + borrow;
      const auto low = static_cast<uint32_t>(product);
      const auto before = u.limbs_[j + i];
      u.limbs_[j + i] = before - low;
      borrow = (product >> 32) + static_cast<uint64_t>(before < low);
    }
    const auto high = u.limbs_[j + n];
    u.limbs_[j + n] = static_cast<uint32_t>(high - borrow);
    if (high < borrow) {
      --trial;
      budget.observe_division_addback();
      budget.charge(static_cast<uint64_t>(n));
      uint64_t carry = 0;
      for (std::size_t i = 0; i < n; ++i) {
        const uint64_t sum = static_cast<uint64_t>(u.limbs_[j + i]) + v.limbs_[i] + carry;
        u.limbs_[j + i] = static_cast<uint32_t>(sum);
        carry = sum >> 32;
      }
      u.limbs_[j + n] += static_cast<uint32_t>(carry);
    }
    quotient.limbs_[j] = static_cast<uint32_t>(trial);
  }
  budget.charge(static_cast<uint64_t>(n));
  remainder.limbs_.resize(n);
  for (std::size_t i = 0; i < n; ++i) {
    uint32_t value = u.limbs_[i] >> shift;
    if (shift != 0 && i + 1 < n)
      value |= u.limbs_[i + 1] << (32 - shift);
    remainder.limbs_[i] = value;
  }
  quotient.sign_ = remainder.sign_ = 1;
  quotient.trim();
  remainder.trim();
  return {std::move(quotient), std::move(remainder)};
}
Integer Integer::divide_exact(const Integer &positive_divisor) const {
  require_same_owner(positive_divisor);
  auto result = divmod_nonnegative(absolute(*this), positive_divisor);
  if (result.second.sign_ != 0)
    throw Failure(BGFD_INVALID_INPUT, "inexact integer division");
  result.first.sign_ *= sign_;
  return std::move(result.first);
}
Integer Integer::gcd_nonnegative(const Integer &left, const Integer &right) {
  left.require_same_owner(right);
  if (left.sign_ < 0 || right.sign_ < 0)
    throw Failure(BGFD_INVALID_INPUT, "gcd requires nonnegative operands");
  Integer a(left), b(right);
  Budget &budget = *left.budget_;
  if (a.compare(b) < 0)
    std::swap(a, b);
  while (b.sign_ != 0) {
    if (a.limbs_.size() == 1) {
      auto x = a.limbs_[0], y = b.limbs_[0];
      while (y != 0) {
        budget.charge(1);
        const auto next = x % y;
        x = y;
        y = next;
      }
      return integer(budget, x);
    }
    bool batched = false;
    if (a.limbs_.size() == b.limbs_.size()) {
      const auto size = a.limbs_.size();
      // Common scale 2^(32*(size-2)+1): exact unknown tails both lie in [0,1).
      int64_t x = static_cast<int64_t>(
          ((static_cast<uint64_t>(a.limbs_[size - 1]) << 32) | a.limbs_[size - 2]) >> 1);
      int64_t y = static_cast<int64_t>(
          ((static_cast<uint64_t>(b.limbs_[size - 1]) << 32) | b.limbs_[size - 2]) >> 1);
      int64_t A = 1, B = 0, C = 0, D = 1;
      unsigned steps = 0;
      constexpr int64_t matrix_limit = int64_t{1} << 30;
      while (y > 0) {
        budget.charge(1);
        // Interval bounds on both linear forms certify every quotient,
        // including the discarded limbs. This is deliberately stricter than
        // heuristic agreement on one leading quotient; ambiguity falls back to
        // exact D.
        int64_t xlo = 0, xhi = 0, ylo = 0, yhi = 0;
        if (!sum_checked(x, std::min<int64_t>(0, A) + std::min<int64_t>(0, B), xlo) ||
            !sum_checked(x, std::max<int64_t>(0, A) + std::max<int64_t>(0, B), xhi) ||
            !sum_checked(y, std::min<int64_t>(0, C) + std::min<int64_t>(0, D), ylo) ||
            !sum_checked(y, std::max<int64_t>(0, C) + std::max<int64_t>(0, D), yhi) || xlo < 0 ||
            ylo <= 0)
          break;
        const auto q = xlo / yhi;
        if (q <= 0 || q != xhi / ylo || q > matrix_limit || q > x / y)
          break;
        const auto nextC = A - q * C, nextD = B - q * D;
        if (nextC < -matrix_limit || nextC > matrix_limit || nextD < -matrix_limit ||
            nextD > matrix_limit)
          break;
        const auto nextY = x - q * y;
        x = y;
        y = nextY;
        A = C;
        B = D;
        C = nextC;
        D = nextD;
        ++steps;
      }
      if (steps >= 2) {
        auto nextA =
            a.multiply(signed_integer(budget, A)).add(b.multiply(signed_integer(budget, B)));
        auto nextB =
            a.multiply(signed_integer(budget, C)).add(b.multiply(signed_integer(budget, D)));
        if (nextA.sign_ >= 0 && nextB.sign_ >= 0 && nextA.compare(nextB) >= 0 &&
            nextB.compare(b) < 0) {
          a = std::move(nextA);
          b = std::move(nextB);
          budget.observe_lehmer_batch();
          batched = true;
        }
      }
    }
    if (!batched) {
      auto divided = divmod_nonnegative(a, b);
      a = std::move(b);
      b = std::move(divided.second);
    }
  }
  return a;
}
} // namespace bgfd

namespace bgmesh {
Rational::Rational(bgfd::Integer numerator, bgfd::Integer denominator, Canonical)
    : numerator_(std::move(numerator)), denominator_(std::move(denominator)) {}
Rational::Rational(bgfd::Budget &budget, int64_t value)
    : numerator_(signed_integer(budget, value)), denominator_(integer(budget, 1)) {}
Rational &Rational::operator=(const Rational &other) {
  if (this != &other) {
    require_same_owner(other);
    Rational copy(other);
    numerator_ = std::move(copy.numerator_);
    denominator_ = std::move(copy.denominator_);
  }
  return *this;
}
Rational &Rational::operator=(Rational &&other) {
  if (this != &other) {
    require_same_owner(other);
    numerator_ = std::move(other.numerator_);
    denominator_ = std::move(other.denominator_);
  }
  return *this;
}
void Rational::require_same_owner(const Rational &other) const {
  if (&budget() != &other.budget())
    throw bgfd::Failure(BGFD_INTERNAL_ERROR, "rational arithmetic allocator mismatch");
}
Rational Rational::from_ratio(bgfd::Integer numerator, bgfd::Integer denominator) {
  if (&numerator.budget() != &denominator.budget())
    throw bgfd::Failure(BGFD_INTERNAL_ERROR, "rational constructor allocator mismatch");
  auto &budget = numerator.budget();
  if (denominator.sign() == 0)
    throw bgfd::Failure(BGFD_INVALID_INPUT, "zero rational denominator");
  if (numerator.sign() == 0)
    return Rational(budget);
  if (denominator.sign() < 0) {
    numerator = numerator.negative();
    denominator = denominator.negative();
  }
  auto gcd = bgfd::Integer::gcd_nonnegative(absolute(numerator), denominator);
  return Rational(numerator.divide_exact(gcd), denominator.divide_exact(gcd), Canonical{});
}
Rational Rational::from_words(bgfd::Budget &budget, const ExactWords &words) {
  auto n = bgfd::Integer::from_words(budget, words.sign, words.numerator);
  auto d = bgfd::Integer::from_words(budget, 1, words.denominator);
  auto value = from_ratio(std::move(n), std::move(d));
  const auto nwords = value.numerator().magnitude(), dwords = value.denominator().magnitude();
  if (nwords.size() != words.numerator.size() || dwords.size() != words.denominator.size() ||
      !std::equal(nwords.begin(), nwords.end(), words.numerator.begin()) ||
      !std::equal(dwords.begin(), dwords.end(), words.denominator.begin()))
    throw bgfd::Failure(BGFD_INVALID_INPUT, "noncanonical rational words");
  return value;
}
Rational Rational::clone_into(bgfd::Budget &destination) const {
  return Rational(numerator_.clone_into(destination), denominator_.clone_into(destination),
                  Canonical{});
}
ExactWords Rational::export_words() const {
  budget().charge(static_cast<uint64_t>(numerator_.size() + denominator_.size()));
  return {sign(),
          {numerator_.magnitude().begin(), numerator_.magnitude().end()},
          {denominator_.magnitude().begin(), denominator_.magnitude().end()}};
}
Rational Rational::add(const Rational &other) const {
  require_same_owner(other);
  if (sign() == 0)
    return Rational(other);
  if (other.sign() == 0)
    return Rational(*this);
  auto g = bgfd::Integer::gcd_nonnegative(denominator_, other.denominator_);
  auto a = denominator_.divide_exact(g), b = other.denominator_.divide_exact(g);
  auto n = numerator_.multiply(b).add(other.numerator_.multiply(a));
  if (n.sign() == 0)
    return Rational(budget());
  auto h = bgfd::Integer::gcd_nonnegative(absolute(n), g);
  return Rational(n.divide_exact(h), a.multiply(other.denominator_.divide_exact(h)), Canonical{});
}
Rational Rational::subtract(const Rational &other) const {
  require_same_owner(other);
  return add(other.negative());
}
Rational Rational::multiply(const Rational &other) const {
  require_same_owner(other);
  if (sign() == 0 || other.sign() == 0)
    return Rational(budget());
  auto g = bgfd::Integer::gcd_nonnegative(absolute(numerator_), other.denominator_);
  auto h = bgfd::Integer::gcd_nonnegative(absolute(other.numerator_), denominator_);
  return Rational(numerator_.divide_exact(g).multiply(other.numerator_.divide_exact(h)),
                  denominator_.divide_exact(h).multiply(other.denominator_.divide_exact(g)),
                  Canonical{});
}
Rational Rational::divide(const Rational &other) const {
  require_same_owner(other);
  if (other.sign() == 0)
    throw bgfd::Failure(BGFD_INVALID_INPUT, "division by zero rational");
  if (sign() == 0)
    return Rational(budget());
  auto g = bgfd::Integer::gcd_nonnegative(absolute(numerator_), absolute(other.numerator_));
  auto h = bgfd::Integer::gcd_nonnegative(denominator_, other.denominator_);
  auto n = numerator_.divide_exact(g).multiply(other.denominator_.divide_exact(h));
  auto d = denominator_.divide_exact(h).multiply(other.numerator_.divide_exact(g));
  if (d.sign() < 0) {
    n = n.negative();
    d = d.negative();
  }
  return Rational(std::move(n), std::move(d), Canonical{});
}
int Rational::compare(const Rational &other) const {
  require_same_owner(other);
  if (sign() != other.sign())
    return sign() > other.sign() ? 1 : -1;
  if (sign() == 0)
    return 0;
  auto g = bgfd::Integer::gcd_nonnegative(denominator_, other.denominator_);
  return numerator_.multiply(other.denominator_.divide_exact(g))
      .compare(other.numerator_.multiply(denominator_.divide_exact(g)));
}
Rational Rational::negative() const {
  return Rational(numerator_.negative(), bgfd::Integer(denominator_), Canonical{});
}
Rational Rational::from_double(bgfd::Budget &budget, double value) {
  const auto bits = std::bit_cast<uint64_t>(value);
  const auto raw_exponent = static_cast<unsigned>((bits >> 52) & 2047);
  uint64_t mantissa = bits & ((uint64_t{1} << 52) - 1);
  if (raw_exponent == 2047)
    throw bgfd::Failure(BGFD_INVALID_INPUT, "nonfinite rational binary64 input");
  if (raw_exponent != 0)
    mantissa |= uint64_t{1} << 52;
  if (mantissa == 0)
    return Rational(budget);
  int exponent = raw_exponent == 0 ? -1074 : static_cast<int>(raw_exponent) - 1075;
  if (exponent < 0) {
    const auto common = std::min(static_cast<unsigned>(std::countr_zero(mantissa)),
                                 static_cast<unsigned>(-exponent));
    mantissa >>= common;
    exponent += static_cast<int>(common);
  }
  auto n = integer(budget, mantissa, bits >> 63 != 0 ? -1 : 1), d = integer(budget, 1);
  if (exponent > 0)
    n = n.shift_left(static_cast<uint64_t>(exponent));
  else if (exponent < 0)
    d = d.shift_left(static_cast<uint64_t>(-exponent));
  // The denominator is one, or its remaining power of two is coprime to
  // the now-odd mantissa. No general rational normalization is needed.
  return Rational(std::move(n), std::move(d), Canonical{});
}
namespace {
uint64_t rounded_magnitude(const Rational &value) {
  if (value.sign() == 0)
    return 0;
  const auto n = absolute(value.numerator());
  const auto &d = value.denominator();
  const auto nb = n.bit_length(), db = d.bit_length();
  if (nb > db && nb - db > 1024)
    throw bgfd::Failure(BGFD_RESOURCE_EXHAUSTED, "binary64 nearest result overflows");
  if (db > nb && db - nb > 1076)
    return 0;
  int64_t exponent = nb >= db ? static_cast<int64_t>(nb - db) : -static_cast<int64_t>(db - nb);
  const int leading = exponent >= 0 ? n.compare(d.shift_left(static_cast<uint64_t>(exponent)))
                                    : n.shift_left(static_cast<uint64_t>(-exponent)).compare(d);
  if (leading < 0)
    --exponent;
  if (exponent > 1023)
    throw bgfd::Failure(BGFD_RESOURCE_EXHAUSTED, "binary64 nearest result overflows");
  const auto scale = exponent < -1022 ? int64_t{1074} : 52 - exponent;
  auto dividend = scale >= 0 ? n.shift_left(static_cast<uint64_t>(scale)) : bgfd::Integer(n);
  auto divisor = scale >= 0 ? bgfd::Integer(d) : d.shift_left(static_cast<uint64_t>(-scale));
  auto qr = bgfd::Integer::divmod_nonnegative(dividend, divisor);
  const auto words = qr.first.magnitude();
  if (words.size() > 2)
    throw bgfd::Failure(BGFD_INTERNAL_ERROR, "binary64 quotient width invariant");
  uint64_t significand = words.empty() ? 0 : words[0];
  if (words.size() == 2)
    significand |= static_cast<uint64_t>(words[1]) << 32;
  const auto midpoint = qr.second.shift_left(1).compare(divisor);
  if (midpoint > 0 || (midpoint == 0 && (significand & 1) != 0))
    ++significand;
  if (exponent < -1022)
    return significand; // Includes a rounding carry into the smallest normal.
  if (significand == uint64_t{1} << 53) {
    significand >>= 1;
    ++exponent;
  }
  if (exponent > 1023)
    throw bgfd::Failure(BGFD_RESOURCE_EXHAUSTED, "binary64 nearest result overflows");
  return (static_cast<uint64_t>(exponent + 1023) << 52) | (significand - (uint64_t{1} << 52));
}
} // namespace
double Rational::to_double() const {
  return std::bit_cast<double>(rounded_magnitude(*this) | (sign() < 0 ? uint64_t{1} << 63 : 0));
}
Binary64Bounds Rational::bounds() const {
  auto rounded = rounded_magnitude(*this);
  const auto positive = sign() < 0 ? negative() : Rational(*this);
  const int direction = positive.compare(from_double(budget(), std::bit_cast<double>(rounded)));
  uint64_t lower = rounded, upper = rounded;
  if (direction > 0)
    ++upper;
  else if (direction < 0)
    --lower;
  if (upper >= uint64_t{0x7ff0000000000000})
    throw bgfd::Failure(BGFD_RESOURCE_EXHAUSTED, "binary64 outward endpoint overflows");
  if (sign() < 0)
    return {std::bit_cast<double>(upper | (uint64_t{1} << 63)),
            std::bit_cast<double>(lower | (uint64_t{1} << 63))};
  return {std::bit_cast<double>(lower), std::bit_cast<double>(upper)};
}

namespace {
bgfd::Integer integer_square_root(const bgfd::Integer &value) {
  auto &owner = value.budget();
  if (value.sign() == 0)
    return bgfd::Integer(owner);
  const auto bits = value.bit_length();
  auto x = integer(owner, 1).shift_left(bits / 2 + bits % 2);
  const auto two = integer(owner, 2);
  // x starts above sqrt(value). Integer Newton steps decrease to floor(sqrt),
  // where the next step is equal or larger (including the two-cycle case).
  for (;;) {
    auto quotient = bgfd::Integer::divmod_nonnegative(value, x).first;
    auto next = bgfd::Integer::divmod_nonnegative(x.add(quotient), two).first;
    if (next.compare(x) >= 0)
      return x;
    x = std::move(next);
  }
}
} // namespace

SqrtBounds Rational::square_root_bounds(uint32_t fractional_bits) const {
  if (sign() < 0)
    throw bgfd::Failure(BGFD_INVALID_INPUT, "negative rational square root");
  if (sign() == 0)
    return {Rational(budget()), Rational(budget())};
  auto scaled = numerator_.shift_left(uint64_t{2} * fractional_bits);
  auto qr = bgfd::Integer::divmod_nonnegative(scaled, denominator_);
  auto root = integer_square_root(qr.first);
  const bool exact = qr.second.sign() == 0 && root.multiply(root).compare(qr.first) == 0;
  auto grid = integer(budget(), 1).shift_left(fractional_bits);
  auto lower = from_ratio(bgfd::Integer(root), bgfd::Integer(grid));
  auto upper =
      exact ? Rational(lower) : from_ratio(root.add(integer(budget(), 1)), std::move(grid));
  return {std::move(lower), std::move(upper)};
}
} // namespace bgmesh
