#include "predicates.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <cfenv>
#include <cfloat>
#include <cmath>
#include <limits>
#if defined(__x86_64__)
#include <xmmintrin.h>
#endif

namespace bgcdt {
namespace {
static_assert(sizeof(double) == sizeof(uint64_t));
static_assert(std::numeric_limits<double>::is_iec559 && std::numeric_limits<double>::digits == 53);
constexpr uint64_t sign_mask = uint64_t{1} << 63;
constexpr uint64_t fraction_mask = (uint64_t{1} << 52) - 1;

[[nodiscard]] int64_t bits(const bgfd::Integer &value) {
  if (value.sign() == 0)
    return 0;
  const auto magnitude = value.magnitude();
  if (magnitude.size() > static_cast<uint64_t>(std::numeric_limits<int64_t>::max() / 128))
    throw bgfd::Failure(BGFD_RESOURCE_EXHAUSTED, "predicate exponent size exhausted");
  return static_cast<int64_t>(magnitude.size() - 1) * 32 +
         static_cast<int64_t>(32 - std::countl_zero(magnitude.back()));
}

// q has at most 53 bits (or is exactly 2^53). q*2^power is representable;
// building its IEEE bits avoids changing behavior under caller FTZ/rounding.
[[nodiscard]] double exact_scaled_small(uint64_t q, int64_t power) {
  if (q == 0)
    return 0;
  auto high = static_cast<int64_t>(std::bit_width(q) - 1);
  if (high == 53) {
    if (q != uint64_t{1} << 53)
      throw bgfd::Failure(BGFD_INTERNAL_ERROR, "invalid enclosure significand");
    q >>= 1;
    --high;
    ++power;
  }
  const int64_t exponent = power + high;
  if (power < -1074 || exponent > 0)
    throw bgfd::Failure(BGFD_INTERNAL_ERROR, "invalid enclosure scaling");
  uint64_t representation = 0;
  if (exponent >= -1022) {
    representation = (static_cast<uint64_t>(exponent + 1023) << 52) |
                     ((q << static_cast<unsigned>(52 - high)) & fraction_mask);
  } else {
    representation = q << static_cast<unsigned>(power + 1074);
  }
  return std::bit_cast<double>(representation);
}

[[nodiscard]] double adjacent(double value, bool upward) {
  uint64_t representation = std::bit_cast<uint64_t>(value);
  if ((representation & ~sign_mask) == 0)
    return std::bit_cast<double>(upward ? uint64_t{1} : sign_mask | uint64_t{1});
  if ((value > 0) == upward)
    ++representation;
  else
    --representation;
  return std::bit_cast<double>(representation);
}
[[nodiscard]] Interval add(Interval a, Interval b) {
  return {adjacent(a.lo + b.lo, false), adjacent(a.hi + b.hi, true)};
}
[[nodiscard]] Interval subtract(Interval a, Interval b) {
  return {adjacent(a.lo - b.hi, false), adjacent(a.hi - b.lo, true)};
}
[[nodiscard]] Interval multiply(Interval a, Interval b) {
  const std::array products{a.lo * b.lo, a.lo * b.hi, a.hi * b.lo, a.hi * b.hi};
  const auto bounds = std::minmax_element(products.begin(), products.end());
  return {adjacent(*bounds.first, false), adjacent(*bounds.second, true)};
}
[[nodiscard]] Interval cross(Interval ax, Interval ay, Interval bx, Interval by) {
  return subtract(multiply(ax, by), multiply(ay, bx));
}
[[nodiscard]] int interval_sign(Interval value) {
  if (value.lo > 0 && std::isfinite(value.lo) && std::isfinite(value.hi))
    return 1;
  if (value.hi < 0 && std::isfinite(value.lo) && std::isfinite(value.hi))
    return -1;
  return 0;
}
} // namespace

Interval Predicates::enclosure(const bgfd::Integer &value, int64_t power) {
  ++stats_.enclosure_values;
  const int64_t width = bits(value);
  if (width == 0)
    return {0, 0};
  if (power > -width || power < std::numeric_limits<int64_t>::min() + 1074)
    throw bgfd::Failure(BGFD_INTERNAL_ERROR, "predicate normalization outside unit range");
  const int64_t discard = std::max({int64_t{0}, width - 53, -1074 - power});
  const auto magnitude = value.magnitude();
  uint64_t q = 0;
  for (int64_t i = width; i > discard; --i) {
    const auto index = static_cast<std::size_t>((i - 1) / 32);
    const auto offset = static_cast<unsigned>((i - 1) % 32);
    q = (q << 1) | ((magnitude[index] >> offset) & uint32_t{1});
  }
  bool lost = false;
  const auto full = static_cast<std::size_t>(
      std::min<int64_t>(discard / 32, static_cast<int64_t>(magnitude.size())));
  budget_->charge(static_cast<uint64_t>(full) + 1);
  for (std::size_t i = 0; i < full; ++i)
    lost = lost || magnitude[i] != 0;
  if (full < magnitude.size() && discard % 32 != 0) {
    const auto mask = (uint32_t{1} << static_cast<unsigned>(discard % 32)) - 1;
    lost = lost || (magnitude[full] & mask) != 0;
  }
  const int64_t result_power = discard + power;
  const double lo = exact_scaled_small(q, result_power);
  const double hi = exact_scaled_small(q + static_cast<uint64_t>(lost), result_power);
  if (value.sign() < 0)
    return {std::bit_cast<double>(std::bit_cast<uint64_t>(hi) | sign_mask),
            std::bit_cast<double>(std::bit_cast<uint64_t>(lo) | sign_mask)};
  return {lo, hi};
}

Predicates::Predicates(bgfd::Budget &budget, const bgfd_input &input, bool filtering)
    : budget_(&budget), wx_(budget), wy_(budget), points_(&budget), filtering_(filtering) {
  if (input.node_count == 0 || input.node_count > std::numeric_limits<Slot>::max() ||
      input.nodes == nullptr)
    throw bgfd::Failure(BGFD_INVALID_INPUT, "predicates require bounded nonempty point table");
  wx_ = bgfd::Integer::decode(budget, input.x_weight, input.limbs, input.limb_count);
  wy_ = bgfd::Integer::decode(budget, input.y_weight, input.limbs, input.limb_count);
  if (wx_.sign() <= 0 || wy_.sign() <= 0)
    throw bgfd::Failure(BGFD_INVALID_INPUT, "predicates require positive exact metric weights");
  const auto anchor_x =
      bgfd::Integer::decode(budget, input.nodes[0].x, input.limbs, input.limb_count);
  const auto anchor_y =
      bgfd::Integer::decode(budget, input.nodes[0].y, input.limbs, input.limb_count);
  points_.reserve(static_cast<std::size_t>(input.node_count));
  int64_t kx = 0, ky = 0;
  for (uint64_t i = 0; i < input.node_count; ++i) {
    budget.charge(1);
    auto x = bgfd::Integer::decode(budget, input.nodes[i].x, input.limbs, input.limb_count)
                 .subtract(anchor_x);
    auto y = bgfd::Integer::decode(budget, input.nodes[i].y, input.limbs, input.limb_count)
                 .subtract(anchor_y);
    kx = std::max(kx, bits(x));
    ky = std::max(ky, bits(y));
    points_.emplace_back(std::move(x), std::move(y), budget);
  }
  // bits() limits every metadata width to <INT64_MAX/4. These sums and
  // differences therefore fit even when arbitrary input exponents are large.
  const int64_t kl = std::max(bits(wx_) + 2 * kx, bits(wy_) + 2 * ky);
  iwx_ = enclosure(wx_, 2 * kx - kl);
  iwy_ = enclosure(wy_, 2 * ky - kl);
  awx_ = iwx_.lo;
  awy_ = iwy_.lo;
  for (auto &p : points_) {
    p.ix = enclosure(p.x, -kx);
    p.iy = enclosure(p.y, -ky);
    p.ax = p.x.sign() < 0 ? p.ix.hi : p.ix.lo;
    p.ay = p.y.sign() < 0 ? p.iy.hi : p.iy.lo;
    p.lift = wx_.multiply(p.x.multiply(p.x)).add(wy_.multiply(p.y.multiply(p.y)));
    ++stats_.exact_lifts;
  }
}

const Predicates::Point &Predicates::point(Slot slot) const {
  if (slot >= points_.size())
    throw bgfd::Failure(BGFD_INVALID_INPUT, "predicate point slot outside prepared table");
  return points_[slot];
}
bgfd::Integer Predicates::orientation(Slot a, Slot b, Slot c) const {
  const auto &p = point(a), &q = point(b), &r = point(c);
  return q.x.subtract(p.x)
      .multiply(r.y.subtract(p.y))
      .subtract(q.y.subtract(p.y).multiply(r.x.subtract(p.x)));
}
int Predicates::exact_sign(const bgfd::Integer &value) {
  ++stats_.exact_signs;
  if (value.sign() == 0)
    ++stats_.exact_zeros;
  return value.sign();
}
bool Predicates::filters_enabled() {
  if (!filtering_)
    return false;
#if defined(__x86_64__) && FLT_EVAL_METHOD == 0
  if (std::fegetround() == FE_TONEAREST && (_mm_getcsr() & uint32_t{0xE040}) == 0)
    return true;
#endif
  ++stats_.floating_disabled;
  return false;
}
int Predicates::compare_axis(Slot a, Slot b, Axis axis) {
  budget_->charge(1);
  ++stats_.axis_compares;
  const auto &p = point(a), &q = point(b);
  if (axis == Axis::x)
    return p.x.compare(q.x);
  if (axis == Axis::y)
    return p.y.compare(q.y);
  throw bgfd::Failure(BGFD_INVALID_INPUT, "invalid predicate comparison axis");
}
int Predicates::orient(Slot a, Slot b, Slot c) {
  budget_->charge(1);
  ++stats_.orientations;
  const auto &p = point(a), &q = point(b), &r = point(c);
  if (filters_enabled()) {
    budget_->charge(1);
    const double ax = q.ax - p.ax, ay = q.ay - p.ay;
    const double bx = r.ax - p.ax, by = r.ay - p.ay;
    const double value = ax * by - ay * bx;
    if (std::abs(value) > 0x1p-46 && std::isfinite(value)) {
      ++stats_.scalar_signs;
      return value > 0 ? 1 : -1;
    }
    budget_->charge(1);
    const auto result = cross(subtract(q.ix, p.ix), subtract(q.iy, p.iy), subtract(r.ix, p.ix),
                              subtract(r.iy, p.iy));
    const int sign = interval_sign(result);
    if (sign != 0) {
      ++stats_.interval_signs;
      return sign;
    }
  }
  return exact_sign(orientation(a, b, c));
}
int Predicates::incircle(Slot a, Slot b, Slot c, Slot d) {
  budget_->charge(1);
  ++stats_.incircles;
  const auto &p = point(a), &q = point(b), &r = point(c), &s = point(d);
  if (filters_enabled()) {
    budget_->charge(1);
    const double ax = p.ax - s.ax, ay = p.ay - s.ay;
    const double bx = q.ax - s.ax, by = q.ay - s.ay;
    const double cx = r.ax - s.ax, cy = r.ay - s.ay;
    const double la = awx_ * (ax * ax) + awy_ * (ay * ay);
    const double lb = awx_ * (bx * bx) + awy_ * (by * by);
    const double lc = awx_ * (cx * cx) + awy_ * (cy * cy);
    const double value =
        la * (bx * cy - by * cx) + lb * (cx * ay - cy * ax) + lc * (ax * by - ay * bx);
    if (std::abs(value) > 0x1p-39 && std::isfinite(value)) {
      ++stats_.scalar_signs;
      return value > 0 ? 1 : -1;
    }
    budget_->charge(1);
    const auto ix = subtract(p.ix, s.ix), iy = subtract(p.iy, s.iy);
    const auto jx = subtract(q.ix, s.ix), jy = subtract(q.iy, s.iy);
    const auto kx = subtract(r.ix, s.ix), ky = subtract(r.iy, s.iy);
    const auto lift = [this](Interval x, Interval y) {
      return add(multiply(iwx_, multiply(x, x)), multiply(iwy_, multiply(y, y)));
    };
    const auto result = add(add(multiply(lift(ix, iy), cross(jx, jy, kx, ky)),
                                multiply(lift(jx, jy), cross(kx, ky, ix, iy))),
                            multiply(lift(kx, ky), cross(ix, iy, jx, jy)));
    const int sign = interval_sign(result);
    if (sign != 0) {
      ++stats_.interval_signs;
      return sign;
    }
  }
  const auto ax = p.x.subtract(s.x), ay = p.y.subtract(s.y);
  const auto bx = q.x.subtract(s.x), by = q.y.subtract(s.y);
  const auto cx = r.x.subtract(s.x), cy = r.y.subtract(s.y);
  const auto la = p.lift.subtract(s.lift), lb = q.lift.subtract(s.lift);
  const auto lc = r.lift.subtract(s.lift);
  const auto value = la.multiply(bx.multiply(cy).subtract(by.multiply(cx)))
                         .add(lb.multiply(cx.multiply(ay).subtract(cy.multiply(ax))))
                         .add(lc.multiply(ax.multiply(by).subtract(ay.multiply(bx))));
  return exact_sign(value);
}
int Predicates::compare_line_distance(Slot s0, Slot s1, Slot a, Slot b) {
  budget_->charge(1);
  ++stats_.line_distance_compares;
  if (point(s0).x.compare(point(s1).x) == 0 && point(s0).y.compare(point(s1).y) == 0)
    throw bgfd::Failure(BGFD_INVALID_INPUT, "line distance requires distinct segment endpoints");
  auto left = orientation(s0, s1, a), right = orientation(s0, s1, b);
  if (left.sign() < 0)
    left = left.negative();
  if (right.sign() < 0)
    right = right.negative();
  return left.compare(right);
}
} // namespace bgcdt
