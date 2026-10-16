#include "geometry_analytic.hpp"
#include "geometry_enclosure.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>

namespace bgmesh {
namespace {
[[noreturn]] void invalid(const char *message) { throw Error(ErrorCode::invalid_source, message); }
[[noreturn]] void unresolved(const char *message) { throw Error(ErrorCode::unresolved, message); }
void owner(GeometryContext &context, const Rational &value) {
  if (&context.budget() != &value.budget())
    throw Error(ErrorCode::internal_error, "original analytic geometry owner mismatch");
}
bgfd::Integer absolute(const bgfd::Integer &x) {
  return x.sign() < 0 ? x.negative() : bgfd::Integer(x);
}
Rational integer_value(bgfd::Integer n) {
  auto &b = n.budget();
  return Rational::from_ratio(std::move(n), Rational(b, 1).numerator());
}
// Exact floor/ceil at a dyadic grid. No floating environment or approximate
// quotient influences enclosure endpoints.
Rational dyadic(const Rational &x, uint32_t bits, bool upward) {
  const auto magnitude = absolute(x.numerator()).shift_left(bits);
  auto [q, r] = bgfd::Integer::divmod_nonnegative(magnitude, x.denominator());
  if (r.sign() != 0 && ((x.sign() > 0 && upward) || (x.sign() < 0 && !upward)))
    q = q.add(Rational(x.budget(), 1).numerator());
  if (x.sign() < 0)
    q = q.negative();
  return Rational::from_ratio(std::move(q), Rational(x.budget(), 1).numerator().shift_left(bits));
}
ExactEnclosure compressed(const Rational &lo, const Rational &hi, uint32_t bits) {
  if (bits == 0)
    return {lo, hi};
  return {dyadic(lo, bits, false), dyadic(hi, bits, true)};
}
ExactEnclosure add(const ExactEnclosure &a, const ExactEnclosure &b, uint32_t bits) {
  return compressed(a.lower.add(b.lower), a.upper.add(b.upper), bits);
}
ExactEnclosure negative(const ExactEnclosure &a) {
  return {a.upper.negative(), a.lower.negative()};
}
ExactEnclosure subtract(const ExactEnclosure &a, const ExactEnclosure &b, uint32_t bits) {
  return add(a, negative(b), bits);
}
ExactEnclosure multiply(const ExactEnclosure &a, const ExactEnclosure &b, uint32_t bits) {
  const std::array products{a.lower.multiply(b.lower), a.lower.multiply(b.upper),
                            a.upper.multiply(b.lower), a.upper.multiply(b.upper)};
  std::size_t low = 0, high = 0;
  for (std::size_t i = 1; i < products.size(); ++i) {
    if (products[i].compare(products[low]) < 0)
      low = i;
    if (products[i].compare(products[high]) > 0)
      high = i;
  }
  return compressed(products[low], products[high], bits);
}
ExactEnclosure scale(const ExactEnclosure &a, const Rational &b, uint32_t bits) {
  return multiply(a, ExactEnclosure(b), bits);
}
ExactEnclosure divide_positive(const ExactEnclosure &a, const Rational &b, uint32_t bits) {
  if (b.sign() <= 0)
    invalid("analytic enclosure division requires positive denominator");
  return compressed(a.lower.divide(b), a.upper.divide(b), bits);
}
ExactEnclosure square(const ExactEnclosure &a, uint32_t bits) {
  const auto left = a.lower.multiply(a.lower), right = a.upper.multiply(a.upper);
  const auto hi = left.compare(right) > 0 ? left : right;
  const auto lo = a.lower.sign() <= 0 && a.upper.sign() >= 0 ? Rational(a.lower.budget(), 0)
                  : left.compare(right) < 0                  ? left
                                                             : right;
  return compressed(lo, hi, bits);
}
struct AtanRequest final {
  int64_t divisor;
  uint32_t bits;
};
ExactEnclosure atan_reciprocal(bgfd::Budget &b, AtanRequest request) {
  const auto [divisor, bits] = request;
  const auto d = Rational(b, divisor), d2 = d.multiply(d);
  auto power = d;
  Rational total(b, 0);
  const uint32_t n = bits / 4 + 8;
  for (uint32_t k = 0; k < n; ++k) {
    const auto term =
        Rational(b, 1).divide(power.multiply(Rational(b, 2 * static_cast<int64_t>(k) + 1)));
    total = (k % 2 == 0) ? total.add(term) : total.subtract(term);
    power = power.multiply(d2);
  }
  const auto next =
      Rational(b, 1).divide(power.multiply(Rational(b, 2 * static_cast<int64_t>(n) + 1)));
  return n % 2 == 0 ? compressed(total, total.add(next), bits + 16)
                    : compressed(total.subtract(next), total, bits + 16);
}
bgfd::Integer nearest_integer(const Rational &x) {
  auto [q, r] = bgfd::Integer::divmod_nonnegative(absolute(x.numerator()), x.denominator());
  const auto comparison = r.shift_left(1).compare(x.denominator());
  if (comparison > 0 || (comparison == 0 && q.sign() != 0 && (q.magnitude()[0] & 1U) != 0))
    q = q.add(Rational(x.budget(), 1).numerator());
  return x.sign() < 0 ? q.negative() : std::move(q);
}
ExactEnclosure taylor(const ExactEnclosure &x, bool cosine, uint32_t bits) {
  auto &b = x.lower.budget();
  const auto xx = square(x, bits);
  const uint32_t n = bits / 4 + 8;
  std::optional<ExactEnclosure> term, total;
  auto *term_value = &term.emplace(cosine ? ExactEnclosure(Rational(b, 1)) : ExactEnclosure(x));
  auto *total_value = &total.emplace(*term_value);
  for (uint32_t k = 1; k < n; ++k) {
    const auto twice = 2 * static_cast<int64_t>(k);
    const auto divisor = cosine ? (twice - 1) * twice : twice * (twice + 1);
    auto next =
        negative(divide_positive(multiply(*term_value, xx, bits), Rational(b, divisor), bits));
    term_value = &term.emplace(std::move(next));
    auto sum = add(*total_value, *term_value, bits);
    total_value = &total.emplace(std::move(sum));
  }
  // Taylor/Lagrange bound on |x|<=1. The omitted odd/even zero coefficient
  // permits order2n+1 for sine and order2n for cosine.
  Rational remainder(b, 1);
  const uint32_t order = cosine ? 2 * n : 2 * n + 1;
  for (uint32_t k = 2; k <= order; ++k)
    remainder = remainder.divide(Rational(b, k));
  return compressed(total_value->lower.subtract(remainder), total_value->upper.add(remainder),
                    bits);
}
uint32_t work_bits(const GeometryContext &context) {
  if (context.bits() > std::numeric_limits<uint32_t>::max() - 32)
    unresolved("analytic precision overflow");
  return context.bits() + 32;
}
ExactEnclosure exact_double(bgfd::Budget &b, double value) {
  if (!std::isfinite(value))
    invalid("nonfinite original analytic coefficient");
  return ExactEnclosure(Rational::from_double(b, value));
}
void validate_frame(const Frame &frame) {
  for (const auto value : frame)
    if (!std::isfinite(value))
      invalid("nonfinite original analytic frame");
}
template <class Evaluate> CanonicalPoint canonical(GeometryContext &context, Evaluate evaluate) {
  for (;;) {
    const auto point = evaluate();
    Vec3 output{};
    bool resolved = true;
    for (uint32_t axis = 0; axis < point.dimension; ++axis) {
      const auto low = point.coordinates[axis].lower.to_double();
      const auto high = point.coordinates[axis].upper.to_double();
      if (std::bit_cast<uint64_t>(low) != std::bit_cast<uint64_t>(high))
        resolved = false;
      output[axis] = low;
    }
    if (resolved)
      return {point.dimension, output};
    context.refine();
  }
}
} // namespace

ExactEnclosure::ExactEnclosure(Rational lo, Rational hi)
    : lower(std::move(lo)), upper(std::move(hi)) {
  if (&lower.budget() != &upper.budget() || lower.compare(upper) > 0)
    throw Error(ErrorCode::internal_error, "invalid original geometry enclosure");
}
ExactEnclosure::ExactEnclosure(const Rational &value) : lower(value), upper(value) {}
GeometryContext::GeometryContext(bgfd::Budget &budget, GeometryPrecision policy)
    : budget_(budget), policy_(policy), bits_(policy.initial_bits), pi_cache_(&budget) {
  if (policy.initial_bits < 32 || policy.initial_bits > policy.maximum_bits ||
      policy.maximum_bits > 16384)
    invalid("invalid original geometry precision limits");
}
const ExactEnclosure &GeometryContext::pi(uint32_t required_bits) {
  if (required_bits < 32)
    invalid("invalid analytic pi precision request");
  if (required_bits > policy_.maximum_bits)
    unresolved("analytic range reduction precision exhausted");
  for (const auto &cached : pi_cache_)
    if (cached.bits == required_bits)
      return cached.enclosure;
  {
    const auto a = atan_reciprocal(budget_, {.divisor = 5, .bits = required_bits});
    const auto b = atan_reciprocal(budget_, {.divisor = 239, .bits = required_bits});
    auto next = subtract(scale(a, Rational(budget_, 16), required_bits + 16),
                         scale(b, Rational(budget_, 4), required_bits + 16), required_bits + 16);
    pi_cache_.push_back({required_bits, std::move(next)});
  }
  return pi_cache_.back().enclosure;
}
void GeometryContext::refine() {
  const auto cap = policy_.maximum_bits > guard_bits_ ? policy_.maximum_bits - guard_bits_ : 0;
  if (bits_ >= cap)
    unresolved("canonical original geometry rounding unresolved");
  bits_ = bits_ > cap / 2 ? cap : 2 * bits_;
}
namespace {
uint64_t argument_bits(const Rational &x) {
  const auto numerator_bits = x.numerator().bit_length(),
             denominator_bits = x.denominator().bit_length();
  return numerator_bits > denominator_bits ? numerator_bits - denominator_bits + 1 : 1;
}
SinCosEnclosure trig_at_stage(GeometryContext &context, const Rational &x, uint32_t stage) {
  owner(context, x);
  if (stage < 32 || stage > 16384)
    invalid("invalid analytic enclosure precision stage");
  auto &b = context.budget();
  if (x.sign() == 0)
    return {ExactEnclosure(Rational(b, 0)), ExactEnclosure(Rational(b, 1))};
  const auto integer_bits = argument_bits(x);
  const auto required = static_cast<uint64_t>(stage) + 32 + integer_bits;
  if (required > context.maximum_bits())
    unresolved("analytic angle magnitude exceeds precision policy");
  const auto &pi = context.pi(static_cast<uint32_t>(required));
  const auto mid = pi.lower.add(pi.upper).divide(Rational(b, 2));
  const auto k = nearest_integer(x.multiply(Rational(b, 2)).divide(mid));
  const auto kvalue = integer_value(k);
  const auto bits = stage + 32;
  const auto reduced =
      subtract(ExactEnclosure(x), scale(pi, kvalue.divide(Rational(b, 2)), bits), bits);
  if (reduced.lower.compare(Rational(b, -1)) < 0 || reduced.upper.compare(Rational(b, 1)) > 0)
    unresolved("analytic angle reduction did not isolate a bounded quadrant");
  auto sine = taylor(reduced, false, bits), cosine = taylor(reduced, true, bits);
  uint32_t quadrant = k.sign() == 0 ? 0 : k.magnitude()[0] % 4;
  if (k.sign() < 0)
    quadrant = (4 - quadrant) % 4;
  if (quadrant == 0)
    return {std::move(sine), std::move(cosine)};
  if (quadrant == 1)
    return {std::move(cosine), negative(sine)};
  if (quadrant == 2)
    return {negative(sine), negative(cosine)};
  return {negative(cosine), std::move(sine)};
}
} // namespace
SinCosEnclosure enclose_sin_cos(GeometryContext &context, const Rational &x) {
  auto answer = trig_at_stage(context, x, context.bits());
  context.guard_bits_ = std::max(context.guard_bits_, static_cast<uint32_t>(32 + argument_bits(x)));
  return answer;
}
EnclosedPoint evaluate_analytic_curve(GeometryContext &context, const Curve &curve,
                                      const Rational &t) {
  owner(context, t);
  validate_frame(curve.frame);
  if ((curve.dimension != 2 && curve.dimension != 3) || curve.kind < 1 || curve.kind > 3)
    invalid("invalid original analytic curve kind/dimension");
  const auto bits = work_bits(context);
  auto &b = context.budget();
  std::optional<SinCosEnclosure> trig;
  if (curve.kind != 1) {
    if (!std::isfinite(curve.radii[0]) || !std::isfinite(curve.radii[1]))
      invalid("nonfinite original analytic curve radii");
    trig.emplace(enclose_sin_cos(context, t));
  }
  const auto make = [&](std::size_t axis) -> ExactEnclosure {
    const auto origin = exact_double(b, curve.frame[axis]);
    if (axis >= curve.dimension)
      return ExactEnclosure(Rational(b, 0));
    if (curve.kind == 1) {
      const auto value = Rational::from_double(b, curve.frame[axis])
                             .add(t.multiply(Rational::from_double(b, curve.frame[3 + axis])));
      return ExactEnclosure(value);
    }
    const auto x = scale(trig->cosine,
                         Rational::from_double(b, curve.radii[0])
                             .multiply(Rational::from_double(b, curve.frame[3 + axis])),
                         bits);
    const auto y = scale(trig->sine,
                         Rational::from_double(b, curve.radii[1])
                             .multiply(Rational::from_double(b, curve.frame[6 + axis])),
                         bits);
    return add(origin, add(x, y, bits), bits);
  };
  return {curve.dimension, {make(0), make(1), make(2)}};
}
EnclosedPoint evaluate_analytic_surface(GeometryContext &context, const Carrier &carrier,
                                        const Rational &u, const Rational &v) {
  owner(context, u);
  owner(context, v);
  validate_frame(carrier.frame);
  if (carrier.kind < 1 || carrier.kind > 5)
    invalid("invalid original analytic carrier kind");
  if (!std::isfinite(carrier.radii[0]) || !std::isfinite(carrier.radii[1]))
    invalid("nonfinite original analytic carrier radii");
  auto &b = context.budget();
  const auto bits = work_bits(context);
  if (carrier.kind == 1) {
    const auto component = [&](std::size_t axis) {
      const auto value = Rational::from_double(b, carrier.frame[axis])
                             .add(u.multiply(Rational::from_double(b, carrier.frame[3 + axis])))
                             .add(v.multiply(Rational::from_double(b, carrier.frame[6 + axis])));
      return ExactEnclosure(value);
    };
    return {3, {component(0), component(1), component(2)}};
  }
  const auto trig_u = enclose_sin_cos(context, u);
  const auto radius = Rational::from_double(b, carrier.radii[0]);
  std::optional<ExactEnclosure> radial, axial;
  if (carrier.kind == 2) {
    radial.emplace(radius);
    axial.emplace(v);
  } else if (carrier.kind == 3) {
    const auto angle = Rational::from_double(b, carrier.radii[1]);
    const auto trig_angle = enclose_sin_cos(context, angle);
    radial.emplace(add(ExactEnclosure(radius), scale(trig_angle.sine, v, bits), bits));
    axial.emplace(scale(trig_angle.cosine, v, bits));
  } else {
    const auto trig_v = enclose_sin_cos(context, v);
    const auto minor = carrier.kind == 4 ? radius : Rational::from_double(b, carrier.radii[1]);
    auto r = scale(trig_v.cosine, minor, bits);
    radial.emplace(carrier.kind == 4 ? std::move(r) : add(ExactEnclosure(radius), r, bits));
    axial.emplace(scale(trig_v.sine, minor, bits));
  }
  const auto component = [&](std::size_t axis) {
    const auto e =
        add(scale(trig_u.cosine, Rational::from_double(b, carrier.frame[3 + axis]), bits),
            scale(trig_u.sine, Rational::from_double(b, carrier.frame[6 + axis]), bits), bits);
    const auto z = scale(*axial, Rational::from_double(b, carrier.frame[9 + axis]), bits);
    return add(exact_double(b, carrier.frame[axis]), add(multiply(*radial, e, bits), z, bits),
               bits);
  };
  return {3, {component(0), component(1), component(2)}};
}
CanonicalPoint canonical_analytic_curve(GeometryContext &context, const Curve &curve,
                                        const Rational &t) {
  return canonical(context, [&] { return evaluate_analytic_curve(context, curve, t); });
}
CanonicalPoint canonical_analytic_surface(GeometryContext &context, const Carrier &carrier,
                                          const Rational &u, const Rational &v) {
  return canonical(context, [&] { return evaluate_analytic_surface(context, carrier, u, v); });
}
EnclosedPoint enclose_original_curve(GeometryContext &context, const GeometryStore &store,
                                     Slot<CurveTag> slot, const Rational &t) {
  owner(context, t);
  const auto &curves = store.view().source.view().curves;
  if (slot.value >= curves.size())
    throw Error(ErrorCode::internal_error, "original curve slot out of bounds");
  const auto &curve = curves[slot.value];
  if (curve.kind <= 3)
    return evaluate_analytic_curve(context, curve, t);
  const auto root = import_curve_root(context.budget(), store, find_curve_root(store, slot, t));
  const auto point = evaluate_curve(root, t);
  return {root.dimension,
          {ExactEnclosure(point[0]), ExactEnclosure(point[1]),
           ExactEnclosure(root.dimension == 3 ? point[2] : Rational(context.budget(), 0))}};
}
EnclosedPoint enclose_original_surface(GeometryContext &context, const GeometryStore &store,
                                       Slot<CarrierTag> slot, const Rational &u,
                                       const Rational &v) {
  owner(context, u);
  owner(context, v);
  const auto &carriers = store.view().source.view().carriers;
  if (slot.value >= carriers.size())
    throw Error(ErrorCode::internal_error, "original carrier slot out of bounds");
  const auto &carrier = carriers[slot.value];
  if (carrier.kind <= 5)
    return evaluate_analytic_surface(context, carrier, u, v);
  const auto root =
      import_surface_root(context.budget(), store, find_surface_root(store, slot, u, v));
  const auto point = evaluate_surface(root, u, v);
  return {3, {ExactEnclosure(point[0]), ExactEnclosure(point[1]), ExactEnclosure(point[2])}};
}
CanonicalPoint canonical_original_curve(GeometryContext &context, const GeometryStore &store,
                                        Slot<CurveTag> slot, const Rational &t) {
  return canonical(context, [&] { return enclose_original_curve(context, store, slot, t); });
}
CanonicalPoint canonical_original_surface(GeometryContext &context, const GeometryStore &store,
                                          Slot<CarrierTag> slot, const Rational &u,
                                          const Rational &v) {
  return canonical(context, [&] { return enclose_original_surface(context, store, slot, u, v); });
}
namespace geometry_detail {
ExactEnclosure sum(const ExactEnclosure &a, const ExactEnclosure &b) { return add(a, b, 0); }
ExactEnclosure negate(const ExactEnclosure &a) { return negative(a); }
ExactEnclosure difference(const ExactEnclosure &a, const ExactEnclosure &b) {
  return subtract(a, b, 0);
}
ExactEnclosure product(const ExactEnclosure &a, const ExactEnclosure &b) {
  return multiply(a, b, 0);
}
ExactEnclosure times(const ExactEnclosure &a, const Rational &b) { return scale(a, b, 0); }
ExactEnclosure quotient(const ExactEnclosure &a, const ExactEnclosure &positive) {
  if (positive.lower.sign() <= 0)
    invalid("source enclosure denominator not proved positive");
  const Rational one(a.lower.budget(), 1);
  return product(a, ExactEnclosure(one.divide(positive.upper), one.divide(positive.lower)));
}
ExactEnclosure squared(const ExactEnclosure &a) { return square(a, 0); }
SinCosEnclosure trig_range(GeometryContext &context, const Rational &begin, const Rational &end,
                           uint32_t precision_bits) {
  owner(context, begin);
  owner(context, end);
  if (begin.compare(end) > 0)
    invalid("reversed original analytic parameter interval");
  if (begin.compare(end) == 0)
    return trig_at_stage(context, begin, precision_bits);
  auto &b = context.budget();
  const auto radius = end.subtract(begin).divide(Rational(b, 2));
  const auto clip = [&](const ExactEnclosure &v) {
    const auto lo = v.lower.subtract(radius), hi = v.upper.add(radius);
    return ExactEnclosure(lo.compare(Rational(b, -1)) < 0 ? Rational(b, -1) : lo,
                          hi.compare(Rational(b, 1)) > 0 ? Rational(b, 1) : hi);
  };
  if (radius.compare(Rational(b, 2)) >= 0)
    return {ExactEnclosure(Rational(b, -1), Rational(b, 1)),
            ExactEnclosure(Rational(b, -1), Rational(b, 1))};
  const auto midpoint = begin.add(end).divide(Rational(b, 2));
  const auto center = trig_at_stage(context, midpoint, precision_bits);
  // Both derivatives have magnitude<=1 globally. This includes all extrema,
  // needs no rounded critical-angle root, and contracts with this interval.
  return {clip(center.sine), clip(center.cosine)};
}
} // namespace geometry_detail
} // namespace bgmesh
