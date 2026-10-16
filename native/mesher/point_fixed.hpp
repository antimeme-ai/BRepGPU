#ifndef BREPGPU_MESHER_POINT_FIXED_HPP
#define BREPGPU_MESHER_POINT_FIXED_HPP
#include "exact.hpp"
#include <algorithm>
#include <array>

namespace bgmesh::point_detail {
// Signed magnitude at a profile's common scale 2^fractional_bits. Ten words
// suffice for 288 fractional bits plus32 integer bits. No heap or FP operation.
struct FixedLiteral final {
  int sign;
  std::array<uint32_t, 10> words;
};
struct FixedIntervalLiteral final {
  FixedLiteral lower, upper;
};
enum class Directed { down, up };
struct FixedOverflow final {};
struct FixedScale final {
  uint32_t fractional_bits;
};
struct DyadicLiteral final {
  int32_t value;
  uint32_t divide_power;
};
class Fixed final {
  std::array<uint32_t, 10> words_{};
  int sign_ = 0;
  void normalize() {
    if (std::all_of(words_.begin(), words_.end(), [](auto v) { return v == 0; }))
      sign_ = 0;
  }

public:
  Fixed() = default;
  explicit Fixed(FixedLiteral literal) : words_(literal.words), sign_(literal.sign) { normalize(); }
  [[nodiscard]] int sign() const noexcept { return sign_; }
  [[nodiscard]] std::span<const uint32_t> words() const noexcept { return words_; }
  [[nodiscard]] Fixed negative() const {
    auto r = *this;
    r.sign_ = -r.sign_;
    return r;
  }
  [[nodiscard]] static Fixed integer(int32_t value, FixedScale scale) {
    const auto bits = scale.fractional_bits;
    Fixed r;
    r.sign_ = (value > 0) - (value < 0);
    const auto magnitude = value < 0 ? static_cast<uint32_t>(-static_cast<int64_t>(value))
                                     : static_cast<uint32_t>(value);
    r.words_[bits / 32] = magnitude;
    return r;
  }
  [[nodiscard]] static Fixed dyadic(DyadicLiteral literal, FixedScale scale) {
    const auto value = literal.value;
    const auto bits = scale.fractional_bits, divide_power = literal.divide_power;
    Fixed r;
    r.sign_ = (value > 0) - (value < 0);
    const auto magnitude = value < 0 ? static_cast<uint32_t>(-static_cast<int64_t>(value))
                                     : static_cast<uint32_t>(value);
    const auto offset = bits - divide_power, word = offset / 32, part = offset % 32;
    const uint64_t shifted = uint64_t{magnitude} << part;
    r.words_[word] = static_cast<uint32_t>(shifted);
    if (word + 1 < r.words_.size())
      r.words_[word + 1] = static_cast<uint32_t>(shifted >> 32);
    else if ((shifted >> 32) != 0)
      throw FixedOverflow{};
    return r;
  }
  [[nodiscard]] static Fixed from_integer(const bgfd::Integer &value, uint32_t bits) {
    Fixed r;
    const auto words = value.magnitude();
    const auto capacity = bits / 32 + 1;
    if (words.size() > capacity)
      throw FixedOverflow{};
    std::copy(words.begin(), words.end(), r.words_.begin());
    r.sign_ = value.sign();
    return r;
  }
  [[nodiscard]] bgfd::Integer as_integer(bgfd::Budget &budget) const {
    auto size = words_.size();
    while (size && words_[size - 1] == 0)
      --size;
    return bgfd::Integer::from_words(budget, sign_, std::span<const uint32_t>(words_).first(size));
  }
  [[nodiscard]] int compare(const Fixed &other) const {
    if (sign_ != other.sign_)
      return sign_ > other.sign_ ? 1 : -1;
    for (std::size_t i = words_.size(); i-- > 0;)
      if (words_[i] != other.words_[i])
        return sign_ * (words_[i] > other.words_[i] ? 1 : -1);
    return 0;
  }
  [[nodiscard]] Fixed add(bgfd::Budget &budget, const Fixed &other, uint32_t bits) const {
    const auto n = bits / 32 + 1;
    budget.charge(n);
    Fixed r;
    if (sign_ == 0)
      return other;
    if (other.sign_ == 0)
      return *this;
    if (sign_ == other.sign_) {
      uint64_t carry = 0;
      for (uint32_t i = 0; i < n; ++i) {
        const uint64_t sum = uint64_t{words_[i]} + other.words_[i] + carry;
        r.words_[i] = static_cast<uint32_t>(sum);
        carry = sum >> 32;
      }
      if (carry)
        throw FixedOverflow{};
      r.sign_ = sign_;
    } else {
      const bool left = compare(other.negative()) * sign_ >= 0;
      const auto &a = left ? *this : other;
      const auto &b = left ? other : *this;
      uint64_t borrow = 0;
      for (uint32_t i = 0; i < n; ++i) {
        const uint64_t sub = uint64_t{b.words_[i]} + borrow;
        r.words_[i] = static_cast<uint32_t>(uint64_t{a.words_[i]} - sub);
        borrow = uint64_t{a.words_[i]} < sub ? 1U : 0U;
      }
      if (borrow)
        throw FixedOverflow{};
      r.sign_ = a.sign_;
    }
    r.normalize();
    return r;
  }
  [[nodiscard]] Fixed subtract(bgfd::Budget &b, const Fixed &other, uint32_t bits) const {
    return add(b, other.negative(), bits);
  }
  [[nodiscard]] Fixed multiply(bgfd::Budget &budget, const Fixed &other, uint32_t bits,
                               Directed direction) const {
    const auto n = bits / 32 + 1, shift = bits / 32;
    budget.charge(uint64_t{n} * n);
    std::array<uint32_t, 20> product{};
    Fixed r;
    for (uint32_t i = 0; i < n; ++i) {
      uint64_t carry = 0;
      for (uint32_t j = 0; j < n; ++j) {
        const uint64_t v = uint64_t{words_[i]} * other.words_[j] + product[i + j] + carry;
        product[i + j] = static_cast<uint32_t>(v);
        carry = v >> 32;
      }
      product[i + n] = static_cast<uint32_t>(carry);
    }
    for (uint32_t i = shift + n; i < 2 * n; ++i)
      if (product[i])
        throw FixedOverflow{};
    for (uint32_t i = 0; i < n; ++i)
      r.words_[i] = product[i + shift];
    r.sign_ = sign_ * other.sign_;
    bool discarded = false;
    for (uint32_t i = 0; i < shift; ++i)
      discarded = discarded || product[i] != 0;
    if (discarded && ((r.sign_ > 0 && direction == Directed::up) ||
                      (r.sign_ < 0 && direction == Directed::down))) {
      uint64_t carry = 1;
      for (uint32_t i = 0; i < n && carry; ++i) {
        const uint64_t v = uint64_t{r.words_[i]} + carry;
        r.words_[i] = static_cast<uint32_t>(v);
        carry = v >> 32;
      }
      if (carry)
        throw FixedOverflow{};
    }
    r.normalize();
    return r;
  }
  // Nearest integer to value*2^scale, ties away from0. This is a locator only;
  // the caller verifies the resulting exact residual interval domain.
  [[nodiscard]] int32_t nearest(uint32_t bits, uint32_t scale) const {
    const auto shift = bits - scale, word = shift / 32, part = shift % 32;
    uint64_t magnitude = words_[word] >> part;
    if (part && word + 1 < words_.size())
      magnitude |= uint64_t{words_[word + 1]} << (32 - part);
    const auto half = shift - 1;
    if (((words_[half / 32] >> (half % 32)) & 1U) != 0)
      ++magnitude;
    if (magnitude > INT32_MAX)
      throw FixedOverflow{};
    return sign_ * static_cast<int32_t>(magnitude);
  }
};
struct FixedInterval final {
  Fixed lower, upper;
};
class FixedArithmetic final {
  bgfd::Budget &budget_;

public:
  const uint32_t bits;
  FixedArithmetic(bgfd::Budget &budget, uint32_t fractional) : budget_(budget), bits(fractional) {}
  [[nodiscard]] FixedInterval literal(const FixedIntervalLiteral &value) const {
    return {Fixed(value.lower), Fixed(value.upper)};
  }
  [[nodiscard]] FixedInterval constant(int32_t value) const {
    const auto v = Fixed::integer(value, {bits});
    return {v, v};
  }
  [[nodiscard]] FixedInterval negative(const FixedInterval &a) const {
    return {a.upper.negative(), a.lower.negative()};
  }
  [[nodiscard]] FixedInterval add(const FixedInterval &a, const FixedInterval &b) const {
    return {a.lower.add(budget_, b.lower, bits), a.upper.add(budget_, b.upper, bits)};
  }
  [[nodiscard]] FixedInterval subtract(const FixedInterval &a, const FixedInterval &b) const {
    return add(a, negative(b));
  }
  [[nodiscard]] FixedInterval multiply(const FixedInterval &a, const FixedInterval &b) const {
    // Exact endpoint signs select monotone extrema. Only two intervals
    // crossing zero need four candidate products; no approximate sign.
    std::array<const Fixed *, 4> endpoints{};
    if (a.lower.sign() >= 0) {
      if (b.lower.sign() >= 0)
        endpoints = {&a.lower, &b.lower, &a.upper, &b.upper};
      else if (b.upper.sign() <= 0)
        endpoints = {&a.upper, &b.lower, &a.lower, &b.upper};
      else
        endpoints = {&a.upper, &b.lower, &a.upper, &b.upper};
    } else if (a.upper.sign() <= 0) {
      if (b.lower.sign() >= 0)
        endpoints = {&a.lower, &b.upper, &a.upper, &b.lower};
      else if (b.upper.sign() <= 0)
        endpoints = {&a.upper, &b.upper, &a.lower, &b.lower};
      else
        endpoints = {&a.lower, &b.upper, &a.lower, &b.lower};
    } else if (b.lower.sign() >= 0)
      endpoints = {&a.lower, &b.upper, &a.upper, &b.upper};
    else if (b.upper.sign() <= 0)
      endpoints = {&a.upper, &b.lower, &a.lower, &b.lower};
    else {
      const auto l = a.lower.multiply(budget_, b.upper, bits, Directed::down),
                 r = a.upper.multiply(budget_, b.lower, bits, Directed::down),
                 u = a.lower.multiply(budget_, b.lower, bits, Directed::up),
                 v = a.upper.multiply(budget_, b.upper, bits, Directed::up);
      return {l.compare(r) < 0 ? l : r, u.compare(v) > 0 ? u : v};
    }
    return {endpoints[0]->multiply(budget_, *endpoints[1], bits, Directed::down),
            endpoints[2]->multiply(budget_, *endpoints[3], bits, Directed::up)};
  }
  [[nodiscard]] FixedInterval square(const FixedInterval &a) const {
    const auto l = a.lower.multiply(budget_, a.lower, bits, Directed::up),
               r = a.upper.multiply(budget_, a.upper, bits, Directed::up);
    const auto hi = l.compare(r) > 0 ? l : r;
    if (a.lower.sign() <= 0 && a.upper.sign() >= 0)
      return {Fixed(), hi};
    const auto ll = a.lower.multiply(budget_, a.lower, bits, Directed::down),
               rr = a.upper.multiply(budget_, a.upper, bits, Directed::down);
    return {ll.compare(rr) < 0 ? ll : rr, hi};
  }
};
} // namespace bgmesh::point_detail
#endif
