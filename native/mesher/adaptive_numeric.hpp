#ifndef BREPGPU_MESHER_ADAPTIVE_NUMERIC_HPP
#define BREPGPU_MESHER_ADAPTIVE_NUMERIC_HPP
#include "adaptive_geometry_types.h"
#include <cmath>
#include <cstring>
#include <limits>
#if defined(__CUDACC__)
#define BGAD_HD __host__ __device__
#else
#define BGAD_HD
#endif
namespace bgmesh::adaptive::numeric {
BGAD_HD inline double rn_add(double a, double b) {
#if defined(__CUDA_ARCH__)
  return __dadd_rn(a, b);
#else
  volatile double r = a + b;
  return r;
#endif
}
BGAD_HD inline double rn_sub(double a, double b) {
#if defined(__CUDA_ARCH__)
  return __dsub_rn(a, b);
#else
  volatile double r = a - b;
  return r;
#endif
}
BGAD_HD inline double rn_mul(double a, double b) {
#if defined(__CUDA_ARCH__)
  return __dmul_rn(a, b);
#else
  volatile double r = a * b;
  return r;
#endif
}
BGAD_HD inline double rn_div(double a, double b) {
#if defined(__CUDA_ARCH__)
  return __ddiv_rn(a, b);
#else
  volatile double r = a / b;
  return r;
#endif
}
BGAD_HD inline double rn_sqrt(double a) {
#if defined(__CUDA_ARCH__)
  return __dsqrt_rn(a);
#else
  return std::sqrt(a);
#endif
}
BGAD_HD inline uint64_t bits(double x) {
#if defined(__CUDA_ARCH__)
  return static_cast<uint64_t>(__double_as_longlong(x));
#else
  uint64_t u;
  std::memcpy(&u, &x, sizeof(u));
  return u;
#endif
}
BGAD_HD inline double from_bits(uint64_t u) {
#if defined(__CUDA_ARCH__)
  return __longlong_as_double(static_cast<long long>(u));
#else
  double x;
  std::memcpy(&x, &u, sizeof(x));
  return x;
#endif
}
BGAD_HD inline double infinity() { return from_bits(0x7ff0000000000000ULL); }
BGAD_HD inline bool finite(double x) {
  return (bits(x) & 0x7ff0000000000000ULL) != 0x7ff0000000000000ULL;
}
BGAD_HD inline double up(double x) {
  if (!finite(x))
    return x;
  if (x == 0.)
    return from_bits(1);
  return from_bits(bits(x) + (x > 0. ? uint64_t{1} : ~uint64_t{0}));
}
BGAD_HD inline double down(double x) {
  if (!finite(x))
    return x;
  if (x == 0.)
    return from_bits(0x8000000000000001ULL);
  return from_bits(bits(x) + (x > 0. ? ~uint64_t{0} : uint64_t{1}));
}
BGAD_HD inline double maximum(double a, double b) { return a > b ? a : b; }
BGAD_HD inline double minimum(double a, double b) { return a < b ? a : b; }
BGAD_HD inline double absolute(double x) { return x < 0. ? -x : x; }
BGAD_HD inline bool valid(Interval x) {
  return finite(x.lower) && finite(x.upper) && x.lower <= x.upper;
}
BGAD_HD inline Interval point(double x) { return {x, x}; }
BGAD_HD inline bool zero(Interval x) { return x.lower == 0. && x.upper == 0.; }
BGAD_HD inline Interval wide() { return {-infinity(), infinity()}; }
BGAD_HD inline Interval neg(Interval x) { return {-x.upper, -x.lower}; }
BGAD_HD inline Interval add(Interval a, Interval b) {
  if (!valid(a) || !valid(b))
    return wide();
  if (zero(a))
    return b;
  if (zero(b))
    return a;
  return {down(rn_add(a.lower, b.lower)), up(rn_add(a.upper, b.upper))};
}
BGAD_HD inline Interval sub(Interval a, Interval b) {
  if (!valid(a) || !valid(b))
    return wide();
  if (zero(b))
    return a;
  if (zero(a))
    return neg(b);
  if (a.lower == a.upper && b.lower == b.upper && a.lower == b.lower)
    return {0., 0.};
  return {down(rn_sub(a.lower, b.upper)), up(rn_sub(a.upper, b.lower))};
}
BGAD_HD inline Interval mul(Interval a, Interval b) {
  if (!valid(a) || !valid(b))
    return wide();
  if (zero(a) || zero(b))
    return {0., 0.};
  const double p[]{rn_mul(a.lower, b.lower), rn_mul(a.lower, b.upper), rn_mul(a.upper, b.lower),
                   rn_mul(a.upper, b.upper)};
  double lo = p[0], hi = p[0];
  for (uint32_t i = 1; i < 4; ++i) {
    lo = minimum(lo, p[i]);
    hi = maximum(hi, p[i]);
  }
  return {down(lo), up(hi)};
}
BGAD_HD inline Interval div(Interval a, Interval b) {
  if (!valid(a) || !valid(b))
    return wide();
  if (b.lower <= 0. && b.upper >= 0.)
    return wide();
  if (zero(a))
    return {0., 0.};
  return mul(a, {down(rn_div(1., b.upper)), up(rn_div(1., b.lower))});
}
BGAD_HD inline Interval scale(Interval a, double b) { return mul(a, point(b)); }
BGAD_HD inline Interval square(Interval a) {
  if (!valid(a))
    return wide();
  const double hi = maximum(absolute(a.lower), absolute(a.upper));
  const double lo =
      a.lower <= 0. && a.upper >= 0. ? 0. : minimum(absolute(a.lower), absolute(a.upper));
  return {lo == 0. ? 0. : down(rn_mul(lo, lo)), hi == 0. ? 0. : up(rn_mul(hi, hi))};
}
BGAD_HD inline double magnitude(Interval x) {
  if (!valid(x))
    return infinity();
  return maximum(absolute(x.lower), absolute(x.upper));
}
BGAD_HD inline double norm_upper(const Interval *xyz) {
  double m = 0.;
  for (uint32_t a = 0; a < 3; ++a) {
    if (!valid(xyz[a]))
      return infinity();
    m = maximum(m, magnitude(xyz[a]));
  }
  if (m == 0.)
    return 0.;
  Interval sum{};
  for (uint32_t a = 0; a < 3; ++a)
    sum = add(sum, square(div(xyz[a], point(m))));
  return up(rn_mul(m, up(rn_sqrt(sum.upper))));
}
BGAD_HD inline int64_t ties_even(double x) {
  // Caller bounds finite x to 2^30+32 before integer conversion.
  const auto i = static_cast<int64_t>(x);
  const auto d = rn_sub(x, static_cast<double>(i));
  if (d > .5 || (d == .5 && (i & 1)))
    return i + 1;
  if (d < -.5 || (d == -.5 && (i & 1)))
    return i - 1;
  return i;
}
} // namespace bgmesh::adaptive::numeric
#include "adaptive_rn_tables.hpp"
namespace bgmesh::adaptive::numeric {
struct SinCos {
  Interval sine, cosine;
  bool ready;
};
BGAD_HD inline SinCos sincos(Interval x) {
  if (zero(x))
    return {{0., 0.}, {1., 1.}, true};
  if (!valid(x))
    return {{-1., 1.}, {-1., 1.}, false};
  const auto middle = rn_add(rn_mul(x.lower, .5), rn_mul(x.upper, .5));
  const auto locator = rn_mul(middle, 0x1.45f306dc9c883p-1);
  if (!finite(locator) || absolute(locator) > 0x1p30)
    return {{-1., 1.}, {-1., 1.}, false};
  const auto k = ties_even(locator);
  auto r = sub(x, scale(half_pi(), static_cast<double>(k)));
  if (!valid(r) || r.lower < -1. || r.upper > 1.)
    return {{-1., 1.}, {-1., 1.}, false};
  const auto j = ties_even(rn_mul(rn_add(rn_mul(r.lower, .5), rn_mul(r.upper, .5)), 32.));
  if (j < -32 || j > 32)
    return {{-1., 1.}, {-1., 1.}, false};
  const auto delta = sub(r, point(static_cast<double>(j) / 32.));
  if (delta.lower < -0x1p-5 || delta.upper > 0x1p-5)
    return {{-1., 1.}, {-1., 1.}, false};
  const auto z = square(delta);
  auto s = sine_coefficient(sine_count() - 1), c = cosine_coefficient(cosine_count() - 1);
  for (uint32_t i = sine_count() - 1; i > 0; --i)
    s = add(mul(s, z), sine_coefficient(i - 1));
  for (uint32_t i = cosine_count() - 1; i > 0; --i)
    c = add(mul(c, z), cosine_coefficient(i - 1));
  s = add(mul(s, delta), {-sine_error(), sine_error()});
  c = add(c, {-cosine_error(), cosine_error()});
  const auto index = static_cast<uint32_t>(j < 0 ? -j : j);
  auto sa = sine_anchor(index);
  if (j < 0)
    sa = neg(sa);
  const auto ca = cosine_anchor(index);
  auto sr = add(mul(sa, c), mul(ca, s)), cr = sub(mul(ca, c), mul(sa, s));
  auto q = k % 4;
  if (q < 0)
    q += 4;
  Interval so{}, co{};
  if (q == 0) {
    so = sr;
    co = cr;
  } else if (q == 1) {
    so = cr;
    co = neg(sr);
  } else if (q == 2) {
    so = neg(sr);
    co = neg(cr);
  } else {
    so = neg(cr);
    co = sr;
  }
  so = {maximum(-1., so.lower), minimum(1., so.upper)};
  co = {maximum(-1., co.lower), minimum(1., co.upper)};
  return {so, co, valid(so) && valid(co)};
}
BGAD_HD inline uint32_t binomial(uint32_t n, uint32_t k) {
  if (k > n)
    return 0;
  uint32_t r = 1;
  for (uint32_t i = 1; i <= k; ++i)
    r = r * (n + 1 - i) / i;
  return r;
}
BGAD_HD inline Interval lerp(Interval a, Interval b, Interval t) {
  return add(a, mul(t, sub(b, a)));
}
struct UVPair {
  Interval u, v;
};
struct RestrictionCoefficient {
  uint32_t child, output_index;
};
struct TensorCoefficient {
  uint32_t u, v, axis;
};
BGAD_HD inline Interval homogeneous_value(const RootDescriptor &root, const Interval *net,
                                          uint32_t channel, UVPair uv) {
  const auto u = uv.u, v = uv.v;
  Interval rows[4]{};
  for (uint32_t i = 0; i <= root.degree_u; ++i) {
    Interval row[4]{};
    for (uint32_t j = 0; j <= root.degree_v; ++j)
      row[j] = net[(i * (root.degree_v + 1) + j) * 4 + channel];
    for (uint32_t d = root.degree_v; d > 0; --d)
      for (uint32_t j = 0; j < d; ++j)
        row[j] = lerp(row[j], row[j + 1], v);
    rows[i] = row[0];
  }
  for (uint32_t d = root.degree_u; d > 0; --d)
    for (uint32_t i = 0; i < d; ++i)
      rows[i] = lerp(rows[i], rows[i + 1], u);
  return rows[0];
}
BGAD_HD inline SampleResult evaluate_sample_rn64(const RootDescriptor &root, const Interval *net,
                                                 const SampleInput &input) {
  SampleResult result;
  if (input.stage != ArithmeticStage::rn64)
    return result;
  for (uint32_t a = 0; a < 2; ++a)
    if (!valid(input.uv[a])) {
      result.status = NumericStatus::invalid;
      return result;
    }
  if (root.family == GeometryFamily::rational_tensor) {
    if (root.degree_u > 3 || root.degree_v > 3 || net == nullptr) {
      result.status = NumericStatus::invalid;
      return result;
    }
    const auto u = div(sub(input.uv[0], root.original_box[0]),
                       sub(root.original_box[1], root.original_box[0]));
    const auto v = div(sub(input.uv[1], root.original_box[2]),
                       sub(root.original_box[3], root.original_box[2]));
    const auto w = homogeneous_value(root, net, 3, {u, v});
    if (w.lower <= 0.)
      return result;
    for (uint32_t a = 0; a < 3; ++a)
      result.original_xyz[a] = div(homogeneous_value(root, net, a, {u, v}), w);
  } else {
    SinCos u{{0., 0.}, {1., 1.}, true}, v{{0., 0.}, {1., 1.}, true};
    if (root.family != GeometryFamily::plane) {
      u = sincos(input.uv[0]);
      if (!u.ready)
        return result;
    }
    if (root.family == GeometryFamily::sphere || root.family == GeometryFamily::torus) {
      v = sincos(input.uv[1]);
      if (!v.ready)
        return result;
    }
    Interval radial{}, axial{};
    if (root.family == GeometryFamily::cylinder) {
      radial = point(root.radii[0]);
      axial = input.uv[1];
    } else if (root.family == GeometryFamily::cone) {
      radial = add(point(root.radii[0]), mul(input.uv[1], root.cone_sin));
      axial = mul(input.uv[1], root.cone_cos);
    } else if (root.family == GeometryFamily::sphere) {
      radial = scale(v.cosine, root.radii[0]);
      axial = scale(v.sine, root.radii[0]);
    } else if (root.family == GeometryFamily::torus) {
      radial = add(point(root.radii[0]), scale(v.cosine, root.radii[1]));
      axial = scale(v.sine, root.radii[1]);
    }
    for (uint32_t a = 0; a < 3; ++a) {
      if (root.family == GeometryFamily::plane)
        result.original_xyz[a] =
            add(point(root.frame[a]),
                add(scale(input.uv[0], root.frame[3 + a]), scale(input.uv[1], root.frame[6 + a])));
      else
        result.original_xyz[a] =
            add(point(root.frame[a]), add(mul(radial, add(scale(u.cosine, root.frame[3 + a]),
                                                          scale(u.sine, root.frame[6 + a]))),
                                          scale(axial, root.frame[9 + a])));
    }
  }
  Interval departure[3]{};
  for (uint32_t a = 0; a < 3; ++a) {
    const auto x = result.original_xyz[a];
    if (!valid(x))
      return result;
    auto q = rn_add(rn_mul(x.lower, .5), rn_mul(x.upper, .5));
    if (q == 0.)
      q = 0.;
    result.actual_xyz[a] = q;
    departure[a] = sub(x, point(q));
  }
  result.departure_upper = norm_upper(departure);
  if (!finite(result.departure_upper))
    return result;
  result.status = NumericStatus::ready;
  return result;
}
// Each call produces ONE coefficient: callers stage parent nets in external
// shared workspace, and lanes write disjoint child/coefficient destinations.
BGAD_HD inline Interval restricted_coefficient_rn64(const RootDescriptor &root,
                                                    const Interval *parent, SplitAxis axis,
                                                    RestrictionCoefficient request) {
  const auto child = request.child, output_index = request.output_index;
  const uint32_t channel = output_index % 4, vertex = output_index / 4;
  const uint32_t out_u = vertex / (root.degree_v + 1), out_v = vertex % (root.degree_v + 1);
  const auto degree = axis == SplitAxis::u ? root.degree_u : root.degree_v;
  const auto i = axis == SplitAxis::u ? out_u : out_v;
  const uint32_t n = child == 0 ? i : degree - i;
  const double divisor = static_cast<double>(uint32_t{1} << n);
  Interval value{};
  for (uint32_t k = 0; k <= n; ++k) {
    const auto source = child == 0 ? k : i + k;
    const auto u = axis == SplitAxis::u ? source : out_u;
    const auto v = axis == SplitAxis::v ? source : out_v;
    const auto coefficient = static_cast<double>(binomial(n, k)) / divisor;
    value = add(value, scale(parent[(u * (root.degree_v + 1) + v) * 4 + channel], coefficient));
  }
  return value;
}
BGAD_HD inline Interval residual_coefficient_rn64(const RootDescriptor &root, const Interval *net,
                                                  const double *corners, const double *anchor,
                                                  TensorCoefficient coefficient) {
  const auto i = coefficient.u, j = coefficient.v, axis = coefficient.axis;
  Interval result{};
  for (uint32_t c = 0; c < 2; ++c)
    for (uint32_t d = 0; d < 2; ++d) {
      if (i < c || j < d)
        continue;
      const auto a = i - c, b = j - d;
      if (a > root.degree_u || b > root.degree_v)
        continue;
      const auto factor = mul(div(point(static_cast<double>(binomial(root.degree_u, a))),
                                  point(static_cast<double>(binomial(root.degree_u + 1, i)))),
                              div(point(static_cast<double>(binomial(root.degree_v, b))),
                                  point(static_cast<double>(binomial(root.degree_v + 1, j)))));
      const auto index = (a * (root.degree_v + 1) + b) * 4;
      const auto w = net[index + 3];
      const auto anchored_n = sub(net[index + axis], scale(w, anchor[axis]));
      const auto anchored_b = sub(point(corners[(c + 2 * d) * 3 + axis]), point(anchor[axis]));
      result = add(result, mul(factor, sub(anchored_n, mul(w, anchored_b))));
    }
  return result;
}
BGAD_HD inline bool regular_rational_net(const RootDescriptor &root, const Interval *net,
                                         const double *anchor) {
  if (root.degree_u == 0 || root.degree_v == 0)
    return false;
  Interval h[4]{}, du[4]{}, dv[4]{};
  bool first = true;
  for (uint32_t i = 0; i <= root.degree_u; ++i)
    for (uint32_t j = 0; j <= root.degree_v; ++j) {
      const auto index = (i * (root.degree_v + 1) + j) * 4;
      for (uint32_t a = 0; a < 4; ++a) {
        const auto value =
            a == 3 ? net[index + 3] : sub(net[index + a], scale(net[index + 3], anchor[a]));
        if (first)
          h[a] = value;
        else
          h[a] = {minimum(h[a].lower, value.lower), maximum(h[a].upper, value.upper)};
      }
      first = false;
    }
  for (uint32_t direction = 0; direction < 2; ++direction) {
    bool initial = true;
    auto *derivative = direction == 0 ? du : dv;
    for (uint32_t i = 0; i <= root.degree_u - (direction == 0 ? 1 : 0); ++i)
      for (uint32_t j = 0; j <= root.degree_v - (direction == 1 ? 1 : 0); ++j) {
        const auto before = (i * (root.degree_v + 1) + j) * 4;
        const auto after = before + (direction == 0 ? (root.degree_v + 1) * 4 : 4);
        const auto degree = direction == 0 ? root.degree_u : root.degree_v;
        for (uint32_t a = 0; a < 4; ++a) {
          auto difference = sub(net[after + a], net[before + a]);
          if (a < 3)
            difference = sub(difference, scale(sub(net[after + 3], net[before + 3]), anchor[a]));
          const auto value = scale(difference, static_cast<double>(degree));
          if (initial)
            derivative[a] = value;
          else
            derivative[a] = {minimum(derivative[a].lower, value.lower),
                             maximum(derivative[a].upper, value.upper)};
        }
        initial = false;
      }
  }
  Interval u[3]{}, v[3]{};
  for (uint32_t a = 0; a < 3; ++a) {
    u[a] = sub(mul(du[a], h[3]), mul(h[a], du[3]));
    v[a] = sub(mul(dv[a], h[3]), mul(h[a], dv[3]));
  }
  for (uint32_t a = 0; a < 3; ++a) {
    const auto b = (a + 1) % 3, c = (a + 2) % 3;
    const auto cross = sub(mul(u[b], v[c]), mul(u[c], v[b]));
    if (valid(cross) && (cross.lower > 0. || cross.upper < 0.))
      return true;
  }
  return false;
}

BGAD_HD inline double rational_axis_score(const RootDescriptor &root, const Interval *net,
                                          const double *anchor, double wmin, SplitAxis axis) {
  const auto degree = axis == SplitAxis::u ? root.degree_u : root.degree_v;
  const auto lines = axis == SplitAxis::u ? root.degree_v + 1 : root.degree_u + 1;
  if (degree == 0)
    return 0.;
  double first = 0., second = 0., weight = 0.;
  for (uint32_t line = 0; line < lines; ++line)
    for (uint32_t i = 0; i < degree; ++i) {
      const auto offset =
          (axis == SplitAxis::u ? i * (root.degree_v + 1) + line : line * (root.degree_v + 1) + i) *
          4;
      const auto step = axis == SplitAxis::u ? (root.degree_v + 1) * 4 : 4;
      const auto wd = sub(net[offset + step + 3], net[offset + 3]);
      weight = maximum(weight, magnitude(wd));
      double d1 = 0., d2 = 0.;
      for (uint32_t a = 0; a < 3; ++a) {
        const auto delta = sub(sub(net[offset + step + a], net[offset + a]), scale(wd, anchor[a]));
        d1 = rn_add(d1, magnitude(delta));
        if (i + 1 < degree) {
          const auto next =
              sub(sub(net[offset + 2 * step + a], net[offset + step + a]),
                  scale(sub(net[offset + 2 * step + 3], net[offset + step + 3]), anchor[a]));
          d2 = rn_add(d2, magnitude(sub(next, delta)));
        }
      }
      first = maximum(first, d1);
      second = maximum(second, d2);
    }
  const auto curvature =
      div(scale(point(second), static_cast<double>(degree * (degree - 1))), point(wmin));
  const auto denominator =
      scale(mul(div(point(first), point(wmin)), div(point(weight), point(wmin))),
            static_cast<double>(2 * degree * degree));
  const auto score = add(curvature, denominator).upper;
  return finite(score) ? score : infinity();
}
BGAD_HD inline NodeBounds bound_node_rn64(const RootDescriptor &root, const Interval *net,
                                          const NodePredicate &input,
                                          double supplied_residual = -1.) {
  NodeBounds result;
  if (input.stage != ArithmeticStage::rn64)
    return result;
  for (uint32_t i = 0; i < 4; ++i)
    if (!valid(input.box[i])) {
      result.status = NumericStatus::invalid;
      return result;
    }
  const auto hu = sub(input.box[1], input.box[0]), hv = sub(input.box[3], input.box[2]);
  if (hu.lower <= 0. || hv.lower <= 0.)
    return result;
  double corner = 0.;
  for (uint32_t i = 0; i < 4; ++i) {
    if (!finite(input.corner_departures[i]) || input.corner_departures[i] < 0.) {
      result.status = NumericStatus::invalid;
      return result;
    }
    corner = maximum(corner, input.corner_departures[i]);
  }
  Interval twist[3]{};
  for (uint32_t a = 0; a < 3; ++a)
    twist[a] = sub(sub(point(input.actual_corners[a]), point(input.actual_corners[3 + a])),
                   sub(point(input.actual_corners[6 + a]), point(input.actual_corners[9 + a])));
  result.twist_upper = norm_upper(twist);
  double e = 0., eu = 0., ev = 0.;
  if (root.family == GeometryFamily::rational_tensor) {
    if (root.degree_u > 3 || root.degree_v > 3 || net == nullptr) {
      result.status = NumericStatus::invalid;
      return result;
    }
    const auto count = (root.degree_u + 1) * (root.degree_v + 1) * 4;
    double wmin = infinity();
    for (uint32_t i = 3; i < count; i += 4) {
      if (!valid(net[i]))
        return result;
      wmin = minimum(wmin, net[i].lower);
    }
    if (wmin <= 0.)
      return result;
    double anchor[3]{};
    for (uint32_t a = 0; a < 3; ++a) {
      const auto first = div(net[a], net[3]);
      anchor[a] = rn_add(rn_mul(first.lower, .5), rn_mul(first.upper, .5));
      if (!finite(anchor[a]))
        return result;
    }
    result.regularity_proved = regular_rational_net(root, net, anchor) ? 1U : 0U;
    double residual = supplied_residual;
    if (supplied_residual < 0.) {
      residual = 0.;
      for (uint32_t i = 0; i <= root.degree_u + 1; ++i)
        for (uint32_t j = 0; j <= root.degree_v + 1; ++j) {
          Interval xyz[3]{};
          for (uint32_t a = 0; a < 3; ++a)
            xyz[a] = residual_coefficient_rn64(root, net, input.actual_corners, anchor, {i, j, a});
          residual = maximum(residual, norm_upper(xyz));
        }
    }
    if (!finite(residual) || residual < 0.)
      return result;
    e = up(rn_div(residual, wmin));
    // Directional homogeneous variation, not scalar-error/box aspect alone.
    // Scores remain heuristic: numeric ambiguity never triggers exact ranking.
    eu = rational_axis_score(root, net, anchor, wmin, SplitAxis::u);
    ev = rational_axis_score(root, net, anchor, wmin, SplitAxis::v);
  } else {
    result.regularity_proved = 1;
    Interval mu{}, mv{};
    const auto a = root.frame_a, f = root.frame_f, r = point(absolute(root.radii[0]));
    if (root.family == GeometryFamily::cylinder)
      mu = mul(r, a);
    else if (root.family == GeometryFamily::cone) {
      const auto radius =
          add(point(root.radii[0]), mul({input.box[2].lower, input.box[3].upper}, root.cone_sin));
      mu = scale(a, magnitude(radius));
    } else if (root.family == GeometryFamily::sphere) {
      mu = mul(r, a);
      mv = mul(r, f);
    } else if (root.family == GeometryFamily::torus) {
      mu = mul(a, add(point(absolute(root.radii[0])), point(absolute(root.radii[1]))));
      mv = scale(f, absolute(root.radii[1]));
    }
    const auto uterm = scale(mul(mu, square(hu)), .125), vterm = scale(mul(mv, square(hv)), .125);
    eu = uterm.upper;
    ev = vterm.upper;
    e = add(add(uterm, vterm), point(corner)).upper;
  }
  result.source_to_bilinear_upper = e;
  result.numerical_upper = corner;
  result.packet_upper = add(point(e), scale(point(result.twist_upper), .25)).upper;
  const auto mixed = scale(point(result.twist_upper), .25).upper;
  result.score_u = (root.affine_separable_axes & 1U) ? 0. : rn_add(eu, mixed);
  result.score_v = (root.affine_separable_axes & 2U) ? 0. : rn_add(ev, mixed);
  if (!finite(e) || !finite(result.packet_upper))
    return result;
  result.status = NumericStatus::ready;
  return result;
}
BGAD_HD inline ActualFacetBounds bound_actual_facet_rn64(const ActualFacetInput &input) {
  ActualFacetBounds result;
  if (!finite(input.source_to_bilinear_upper) || input.source_to_bilinear_upper < 0.) {
    result.status = NumericStatus::invalid;
    return result;
  }
  double largest = 0.;
  for (uint32_t i = 0; i < 3; ++i)
    for (uint32_t j = i; j < 3; ++j) {
      Interval residual[3]{};
      for (uint32_t a = 0; a < 3; ++a) {
        const auto anchor = point(input.actual_corners[a]);
        const auto c = point(0.);
        const auto x = sub(point(input.actual_corners[3 + a]), anchor);
        const auto y = sub(point(input.actual_corners[6 + a]), anchor);
        const auto d =
            sub(sub(point(input.actual_corners[9 + a]), point(input.actual_corners[6 + a])), x);
        const auto ui = input.uv[2 * i], uj = input.uv[2 * j], vi = input.uv[2 * i + 1],
                   vj = input.uv[2 * j + 1];
        auto coefficient =
            add(c, add(scale(mul(add(ui, uj), x), .5), scale(mul(add(vi, vj), y), .5)));
        coefficient = add(coefficient, scale(mul(add(mul(ui, vj), mul(uj, vi)), d), .5));
        residual[a] = sub(coefficient, scale(add(sub(input.affine_xyz[3 * i + a], anchor),
                                                 sub(input.affine_xyz[3 * j + a], anchor)),
                                             .5));
      }
      largest = maximum(largest, norm_upper(residual));
    }
  if (!finite(largest))
    return result;
  result.bilinear_to_facet_upper = largest;
  result.total_upper = input.source_to_bilinear_upper == 0.
                           ? largest
                           : up(rn_add(input.source_to_bilinear_upper, largest));
  result.status = NumericStatus::ready;
  return result;
}
} // namespace bgmesh::adaptive::numeric
#endif
