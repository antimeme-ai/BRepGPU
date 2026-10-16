#include "metrics.hpp"
#include <bit>
#include <cmath>
#include <limits>

namespace bgmesh {
namespace {
struct Symmetric final {
  Rational a, b, c;
};
[[noreturn]] void invalid(const char *message) { throw bgfd::Failure(BGFD_INVALID_INPUT, message); }
[[noreturn]] void internal(const char *message) {
  throw bgfd::Failure(BGFD_INTERNAL_ERROR, message);
}
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
Rational rounded53(const Rational &r) {
  if (r.sign() == 0)
    return Rational(r.budget(), 0);
  const auto exponent = logarithm(r);
  const auto normalized = r.multiply(power_two(r.budget(), -exponent));
  return Rational::from_double(r.budget(), normalized.to_double())
      .multiply(power_two(r.budget(), exponent));
}
Rational selected(const ExactEnclosure &value) {
  return rounded53(value.lower.add(value.upper).divide(Rational(value.lower.budget(), 2)));
}
std::array<Rational, 3> selected_axes(const std::array<ExactEnclosure, 3> &values) {
  return {selected(values[0]), selected(values[1]), selected(values[2])};
}
Rational dot(const std::array<Rational, 3> &a, const std::array<Rational, 3> &b) {
  return a[0].multiply(b[0]).add(a[1].multiply(b[1])).add(a[2].multiply(b[2]));
}
std::array<Rational, 3> cross(const std::array<Rational, 3> &a, const std::array<Rational, 3> &b) {
  return {a[1].multiply(b[2]).subtract(a[2].multiply(b[1])),
          a[2].multiply(b[0]).subtract(a[0].multiply(b[2])),
          a[0].multiply(b[1]).subtract(a[1].multiply(b[0]))};
}
Rational determinant(const Symmetric &m) { return m.a.multiply(m.c).subtract(m.b.multiply(m.b)); }
Symmetric absolute(const Symmetric &m) {
  const auto det = determinant(m), trace = m.a.add(m.c);
  if (det.sign() >= 0)
    return trace.sign() >= 0 ? Symmetric{m.a, m.b, m.c}
                             : Symmetric{m.a.negative(), m.b.negative(), m.c.negative()};
  const auto difference = m.a.subtract(m.c);
  const auto gap = rounded_sqrt53(
      difference.multiply(difference).add(m.b.multiply(m.b).multiply(Rational(m.a.budget(), 4))));
  return {m.a.multiply(m.a).add(m.b.multiply(m.b)).subtract(det).divide(gap),
          m.b.multiply(trace).divide(gap),
          m.c.multiply(m.c).add(m.b.multiply(m.b)).subtract(det).divide(gap)};
}
Symmetric gram(const std::array<Rational, 4> &factor) {
  const auto det = factor[0].multiply(factor[3]).subtract(factor[1].multiply(factor[2]));
  if (det.sign() <= 0)
    invalid("metric factor has nonpositive determinant");
  return {factor[0].multiply(factor[0]).add(factor[2].multiply(factor[2])),
          factor[0].multiply(factor[1]).add(factor[2].multiply(factor[3])),
          factor[1].multiply(factor[1]).add(factor[3].multiply(factor[3]))};
}
std::array<Rational, 4> factorize(const Symmetric &m) {
  auto &b = m.a.budget();
  const auto det = determinant(m);
  if (m.a.sign() <= 0 || det.sign() <= 0)
    internal("source proposal matrix is not SPD");
  const auto difference = m.a.subtract(m.c);
  const auto gap = rounded_sqrt53(
      difference.multiply(difference).add(m.b.multiply(m.b).multiply(Rational(b, 4))));
  const auto large = m.a.add(m.c).add(gap).divide(Rational(b, 2)), small = det.divide(large);
  const auto s1 = rounded_sqrt53(large), s2 = rounded_sqrt53(small);
  Rational cosine(b, 1), sine(b, 0);
  if (m.b.sign() == 0) {
    if (m.a.compare(m.c) < 0) {
      cosine = Rational(b, 0);
      sine = Rational(b, 1);
    }
  } else {
    const auto twice = m.b.multiply(Rational(b, 2));
    const auto x = difference.sign() >= 0 ? gap.add(difference) : twice;
    const auto y = difference.sign() >= 0 ? twice : gap.subtract(difference);
    const auto norm = rounded_sqrt53(x.multiply(x).add(y.multiply(y)));
    cosine = rounded53(x.divide(norm));
    sine = rounded53(y.divide(norm));
  }
  std::array<Rational, 4> factor{s1.multiply(cosine), s1.multiply(sine),
                                 s2.multiply(sine).negative(), s2.multiply(cosine)};
  (void)gram(factor);
  return factor;
}
std::optional<double> finite_round(const Rational &r) {
  const auto nb = r.numerator().bit_length(), db = r.denominator().bit_length();
  if (nb >= db && nb - db >= 1023) {
    if (nb - db > 1024)
      return std::nullopt;
    const auto threshold = power_two(r.budget(), 1024).subtract(power_two(r.budget(), 970));
    const auto magnitude = r.sign() < 0 ? r.negative() : Rational(r);
    if (magnitude.compare(threshold) >= 0)
      return std::nullopt;
  }
  return r.to_double();
}
} // namespace

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
std::array<Rational, 4> metric_from_point_jet(const SurfaceJetEnclosure &jet, MetricScales scales) {
  auto &b = scales.allowance.budget();
  for (const auto *value : {&scales.physical_diagonal_squared, &scales.chart_width_u,
                            &scales.chart_width_v, &scales.allowance})
    if (value->sign() <= 0)
      throw Error(ErrorCode::unsupported, "nonpositive source metric extent/allowance");
  const auto du = selected_axes(jet.du), dv = selected_axes(jet.dv), normal = cross(du, dv);
  const auto normal_squared = dot(normal, normal);
  if (normal_squared.sign() == 0)
    throw Error(ErrorCode::unsupported, "zero chosen source normal needs a quotient profile");
  const auto normal_length = rounded_sqrt53(normal_squared);
  const Symmetric second{dot(normal, selected_axes(jet.duu)).divide(normal_length),
                         dot(normal, selected_axes(jet.duv)).divide(normal_length),
                         dot(normal, selected_axes(jet.dvv)).divide(normal_length)};
  const auto curvature = absolute(second);
  const auto curvature_scale = Rational(b, 8).multiply(scales.allowance), floor = power_two(b, -40);
  const Symmetric proposal{
      dot(du, du)
          .divide(scales.physical_diagonal_squared)
          .add(curvature.a.divide(curvature_scale))
          .add(floor.divide(scales.chart_width_u.multiply(scales.chart_width_u))),
      dot(du, dv).divide(scales.physical_diagonal_squared).add(curvature.b.divide(curvature_scale)),
      dot(dv, dv)
          .divide(scales.physical_diagonal_squared)
          .add(curvature.c.divide(curvature_scale))
          .add(floor.divide(scales.chart_width_v.multiply(scales.chart_width_v)))};
  return factorize(proposal);
}
std::array<Rational, 4> original_source_metric(GeometryContext &context, const GeometryStore &store,
                                               Slot<CarrierTag> carrier,
                                               std::span<const Rational, 2> point,
                                               MetricScales scales, uint32_t precision_bits) {
  return metric_from_point_jet(enclose_original_surface_point_jet(context, store, carrier, point[0],
                                                                  point[1], precision_bits),
                               scales);
}
std::array<double, 2> metric_circumcenter(std::span<const Rational, 6> uv,
                                          const std::array<Rational, 4> &factor) {
  auto &b = uv[0].budget();
  const auto m = gram(factor);
  const auto ux = uv[2].subtract(uv[0]), uy = uv[3].subtract(uv[1]), vx = uv[4].subtract(uv[0]),
             vy = uv[5].subtract(uv[1]);
  const auto a = m.a.multiply(ux).add(m.b.multiply(uy)), c = m.b.multiply(ux).add(m.c.multiply(uy));
  const auto d = m.a.multiply(vx).add(m.b.multiply(vy)), e = m.b.multiply(vx).add(m.c.multiply(vy));
  const auto ru = ux.multiply(a).add(uy.multiply(c)).divide(Rational(b, 2)),
             rv = vx.multiply(d).add(vy.multiply(e)).divide(Rational(b, 2));
  const auto det = a.multiply(e).subtract(c.multiply(d));
  if (det.sign() == 0)
    invalid("circumcenter triangle is collinear");
  const auto x = uv[0].add(ru.multiply(e).subtract(c.multiply(rv)).divide(det));
  const auto y = uv[1].add(a.multiply(rv).subtract(ru.multiply(d)).divide(det));
  const auto rx = finite_round(x), ry = finite_round(y);
  const auto infinity = std::numeric_limits<double>::infinity();
  return {rx ? *rx : (x.sign() < 0 ? -infinity : infinity),
          ry ? *ry : (y.sign() < 0 ? -infinity : infinity)};
}
std::optional<double> metric_shape_potential(std::span<const Rational, 6> uv,
                                             const std::array<Rational, 4> &factor) {
  auto &b = uv[0].budget();
  const auto m = gram(factor);
  const auto area2 = uv[2]
                         .subtract(uv[0])
                         .multiply(uv[5].subtract(uv[1]))
                         .subtract(uv[3].subtract(uv[1]).multiply(uv[4].subtract(uv[0])));
  if (area2.sign() == 0)
    return std::nullopt;
  Rational sum(b, 0);
  for (std::size_t i = 0; i < 3; ++i) {
    const auto j = (i + 1) % 3;
    const auto x = uv[2 * j].subtract(uv[2 * i]), y = uv[2 * j + 1].subtract(uv[2 * i + 1]);
    const auto a = factor[0].multiply(x).add(factor[1].multiply(y)),
               c = factor[2].multiply(x).add(factor[3].multiply(y));
    sum = sum.add(a.multiply(a)).add(c.multiply(c));
  }
  const auto value = sum.multiply(sum).divide(
      Rational(b, 12).multiply(determinant(m)).multiply(area2.multiply(area2)));
  const auto chosen = finite_round(value);
  return chosen && *chosen > 0 ? chosen : std::nullopt;
}
} // namespace bgmesh
