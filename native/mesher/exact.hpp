#ifndef BREPGPU_MESHER_EXACT_HPP
#define BREPGPU_MESHER_EXACT_HPP

#include "../face_domain/exact.hpp"
#include <cstdint>
#include <vector>

namespace bgmesh {
// Canonical, transferable value: no allocator or mutable worker state is
// retained. Public words are validated on import; zero is sign=0, numerator={},
// denominator={1}.
struct ExactWords final {
  int sign = 0;
  std::vector<uint32_t> numerator;
  std::vector<uint32_t> denominator{1};
};

struct Binary64Bounds final {
  double lower, upper;
};

struct SqrtBounds;

class Rational final {
  bgfd::Integer numerator_, denominator_;
  struct Canonical {};
  Rational(bgfd::Integer numerator, bgfd::Integer denominator, Canonical);
  void require_same_owner(const Rational &) const;

public:
  explicit Rational(bgfd::Budget &, int64_t value = 0);
  Rational(const Rational &) = default;
  Rational(Rational &&) noexcept = default;
  Rational &operator=(const Rational &);
  Rational &operator=(Rational &&);
  [[nodiscard]] static Rational from_ratio(bgfd::Integer numerator, bgfd::Integer denominator);
  [[nodiscard]] static Rational from_double(bgfd::Budget &, double);
  [[nodiscard]] static Rational from_words(bgfd::Budget &, const ExactWords &);
  [[nodiscard]] Rational clone_into(bgfd::Budget &) const;
  [[nodiscard]] ExactWords export_words() const;
  [[nodiscard]] const bgfd::Integer &numerator() const noexcept { return numerator_; }
  [[nodiscard]] const bgfd::Integer &denominator() const noexcept { return denominator_; }
  [[nodiscard]] bgfd::Budget &budget() const noexcept { return numerator_.budget(); }
  [[nodiscard]] int sign() const noexcept { return numerator_.sign(); }
  [[nodiscard]] Rational add(const Rational &) const;
  [[nodiscard]] Rational subtract(const Rational &) const;
  [[nodiscard]] Rational multiply(const Rational &) const;
  [[nodiscard]] Rational divide(const Rational &) const;
  [[nodiscard]] int compare(const Rational &) const;
  [[nodiscard]] Rational negative() const;
  // Integer-only conversion, independent of ambient rounding/FTZ/DAZ.
  // Nonzero negative underflow rounds to -0; the rational value zero rounds to
  // +0. A nonfinite nearest result, or a nonfinite outward endpoint, throws
  // Failure.
  [[nodiscard]] double to_double() const;
  [[nodiscard]] Binary64Bounds bounds() const;
  // Nonnegative input, enclosed on the absolute dyadic grid 2^-fractional_bits.
  // Both endpoints retain this value's owner; arithmetic is integer-only.
  [[nodiscard]] SqrtBounds square_root_bounds(uint32_t fractional_bits) const;
};

struct SqrtBounds final {
  Rational lower, upper;
};
} // namespace bgmesh
#endif
