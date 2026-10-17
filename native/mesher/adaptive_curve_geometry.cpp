#include "adaptive_curve_geometry.hpp"
#include "adaptive_curve_numeric.hpp"
#include "geometry_enclosure.hpp"
#include <algorithm>
#include <array>
namespace bgmesh::adaptive {
namespace {
void check(bool ok, const char *text) {
  if (!ok)
    throw Error(ErrorCode::invalid_source, text);
}
void owner(bgfd::Budget &b, std::span<const Rational> values) {
  for (const auto &x : values)
    check(&x.budget() == &b, "curve arithmetic owner mismatch");
}
Interval enclosure(const Rational &x) {
  const auto q = x.bounds();
  return {q.lower, q.upper};
}
Interval unit_enclosure(const Rational &x) {
  check(x.sign() >= 0 && x.compare(Rational(x.budget(), 1)) <= 0,
        "curve restriction outside source interval");
  auto q = enclosure(x);
  q.lower = std::max(0., q.lower);
  q.upper = std::min(1., q.upper);
  return q;
}
std::pmr::vector<Interval> empty_intervals(size_t count, bgfd::Budget &b) {
  std::pmr::vector<Interval> result(&b);
  result.resize(count);
  return result;
}
// Both children share one pyramid. Arbitrary source restriction is a one-time
// preparation; adaptive midpoint descendants use curve_midpoint_pair instead.
struct SplitOutputs {
  std::span<Interval> left, right, scratch;
};
void split_at(const CurveRootDescriptor &root, std::span<const Interval> parent, Interval t,
              SplitOutputs outputs, bgfd::Budget &b) {
  const auto left = outputs.left, right = outputs.right, scratch = outputs.scratch;
  const auto stride = root.dimension + 1;
  std::copy(parent.begin(), parent.end(), scratch.begin());
  for (uint32_t a = 0; a < stride; ++a) {
    left[a] = scratch[a];
    right[root.degree * stride + a] = scratch[root.degree * stride + a];
  }
  for (uint32_t level = 1; level <= root.degree; ++level) {
    for (uint32_t i = 0; i <= root.degree - level; ++i)
      for (uint32_t a = 0; a < stride; ++a) {
        b.charge(1);
        scratch[i * stride + a] =
            numeric::curve_lerp(scratch[i * stride + a], scratch[(i + 1) * stride + a], t);
      }
    for (uint32_t a = 0; a < stride; ++a) {
      left[level * stride + a] = scratch[a];
      right[(root.degree - level) * stride + a] = scratch[(root.degree - level) * stride + a];
    }
  }
}
std::pmr::vector<Interval> restrict_net(const CurveRootDescriptor &root,
                                        std::span<const Interval> parent, const Rational &lo,
                                        const Rational &hi, bgfd::Budget &b) {
  auto result = empty_intervals(parent.size(), b), other = empty_intervals(parent.size(), b),
       scratch = empty_intervals(parent.size(), b);
  std::copy(parent.begin(), parent.end(), result.begin());
  if (lo.sign() != 0) {
    split_at(root, result, unit_enclosure(lo), {other, result, scratch}, b);
  }
  if (hi.compare(Rational(b, 1)) != 0) {
    const auto parameter = hi.subtract(lo).divide(Rational(b, 1).subtract(lo));
    split_at(root, result, unit_enclosure(parameter), {result, other, scratch}, b);
  }
  return result;
}
void validate_root(const CurveRootDescriptor &root) {
  check(root.dimension >= 2 && root.dimension <= 3 && root.degree <= 32,
        "unsupported curve root shape");
  check(static_cast<uint32_t>(root.family) <= static_cast<uint32_t>(CurveFamily::rational),
        "unknown curve family");
}
void validate_interval(const CurveRootDescriptor &root, std::span<const Rational, 2> t,
                       bgfd::Budget &b) {
  owner(b, t);
  check(t[0].compare(t[1]) < 0 &&
            t[0].compare(Rational::from_double(b, root.prepared_t[0].lower)) >= 0 &&
            t[1].compare(Rational::from_double(b, root.prepared_t[1].lower)) <= 0,
        "curve interval outside prepared original domain");
}
uint32_t precision_bits(ArithmeticStage stage) {
  return stage == ArithmeticStage::p128   ? 128
         : stage == ArithmeticStage::p192 ? 192
         : stage == ArithmeticStage::p256 ? 256
                                          : 384;
}
Rational norm_from_square(const Rational &square) {
  if (square.sign() == 0)
    return square;
  const auto n = square.numerator().bit_length(), d = square.denominator().bit_length();
  const auto gap = d > n ? d - n : 0;
  if (gap > 8192)
    throw Error(ErrorCode::unresolved, "curve precise norm exponent profile");
  return square.square_root_bounds(160 + static_cast<uint32_t>((gap + 1) / 2)).upper;
}
Rational norm_upper(std::span<const Rational> xyz, bgfd::Budget &b) {
  auto square = Rational(b);
  for (const auto &x : xyz)
    square = square.add(x.multiply(x));
  return norm_from_square(square);
}
} // namespace
CurveSampleResult evaluate_curve_sample(const CurveSourceCache &cache,
                                        const CurveSampleInput &input) {
  admit_numeric_environment();
  check(input.original.value < cache.original_roots().size(), "foreign original curve sample root");
  const auto &root = cache.original_roots()[input.original.value];
  validate_root(root);
  check(input.stage == ArithmeticStage::rn64 && input.fixed_actual <= 1,
        "curve ordinary sample profile");
  check(numeric::valid(input.t) && input.t.lower >= root.original_t[0].lower &&
            input.t.upper <= root.original_t[1].upper,
        "curve sample outside original source span");
  for (const auto x : input.actual_xyz)
    if (input.fixed_actual)
      check(numeric::finite(x), "nonfinite fixed curve point");
  if (input.fixed_actual && root.dimension == 2)
    check(input.actual_xyz[2] == 0., "2D fixed curve point has nonzero z");
  const auto controls = cache.controls();
  check(root.controls.first <= controls.size() &&
            root.controls.count <= controls.size() - root.controls.first,
        "curve sample control range");
  std::array<Interval, 132> workspace{};
  return numeric::evaluate_curve_sample_rn64(
      root, root.controls.count ? controls.data() + root.controls.first : nullptr, input,
      workspace.data());
}
CurveNodeBounds bound_curve_node(const CurveSourceCache &cache, const CurveNodeInput &input,
                                 std::span<const Interval> net) {
  admit_numeric_environment();
  check(input.root.value < cache.roots().size(), "foreign prepared curve root");
  const auto &root = cache.roots()[input.root.value];
  validate_root(root);
  check(input.stage == ArithmeticStage::rn64, "curve ordinary node profile");
  check(numeric::valid(input.t[0]) && numeric::valid(input.t[1]) &&
            input.t[0].lower >= root.prepared_t[0].lower &&
            input.t[1].upper <= root.prepared_t[1].upper && input.t[0].lower <= input.t[1].upper,
        "curve node outside prepared span");
  if (input.t[0].upper >= input.t[1].lower)
    return CurveNodeBounds{};
  for (const auto x : input.actual_endpoints)
    check(numeric::finite(x), "nonfinite actual curve chord");
  if (root.dimension == 2)
    check(input.actual_endpoints[2] == 0. && input.actual_endpoints[5] == 0.,
          "2D actual curve chord has nonzero z");
  for (const auto x : input.endpoint_departures)
    check(numeric::finite(x) && x >= 0., "invalid original endpoint departure");
  if (root.family == CurveFamily::rational) {
    if (net.empty()) {
      const auto controls = cache.controls();
      check(input.controls.first <= controls.size() &&
                input.controls.count <= controls.size() - input.controls.first,
            "curve node control range");
      net = controls.subspan(input.controls.first, input.controls.count);
    }
    check(net.size() == static_cast<size_t>(root.degree + 1) * (root.dimension + 1),
          "curve node net shape");
  }
  return numeric::bound_curve_node_rn64(root, net.empty() ? nullptr : net.data(), input);
}
std::pmr::vector<Interval> prepare_curve_interval_controls(const CurveSourceCache &cache,
                                                           CurveRootSlot slot,
                                                           std::span<const Rational, 2> t,
                                                           bgfd::Budget &b) {
  admit_numeric_environment();
  check(slot.value < cache.roots().size(), "foreign curve interval root");
  const auto &root = cache.roots()[slot.value];
  validate_interval(root, t, b);
  if (root.family != CurveFamily::rational)
    return std::pmr::vector<Interval>(&b);
  const auto controls = cache.controls();
  check(root.controls.first <= controls.size() &&
            root.controls.count <= controls.size() - root.controls.first,
        "curve restriction net range");
  const auto a = Rational::from_double(b, root.prepared_t[0].lower),
             width = Rational::from_double(b, root.prepared_t[1].lower).subtract(a);
  return restrict_net(root, controls.subspan(root.controls.first, root.controls.count),
                      t[0].subtract(a).divide(width), t[1].subtract(a).divide(width), b);
}
void restrict_curve_midpoint_pair(const CurveRootDescriptor &root, std::span<const Interval> parent,
                                  std::span<Interval> left, std::span<Interval> right,
                                  std::span<Interval> workspace) {
  admit_numeric_environment();
  validate_root(root);
  const auto count = static_cast<size_t>(root.degree + 1) * (root.dimension + 1);
  check(root.family == CurveFamily::rational && parent.size() == count && left.size() == count &&
            right.size() == count && workspace.size() >= count,
        "paired curve restriction shape");
  // Public owner-local buffers must be disjoint, as the device batch validator
  // also requires; aliasing a parent/child would change the shared pyramid.
  const auto overlaps = [](auto a, auto c) {
    const auto ap = reinterpret_cast<uintptr_t>(a.data()),
               cp = reinterpret_cast<uintptr_t>(c.data());
    return ap < cp + c.size_bytes() && cp < ap + a.size_bytes();
  };
  check(!overlaps(parent, left) && !overlaps(parent, right) && !overlaps(parent, workspace) &&
            !overlaps(left, right) && !overlaps(left, workspace) && !overlaps(right, workspace),
        "overlapping paired restriction buffers");
  numeric::curve_midpoint_pair(root, parent.data(), {left.data(), right.data(), workspace.data()});
}
std::optional<std::array<Rational, 3>> exact_curve_value(const CurveSourceCache &cache,
                                                         Slot<CurveTag> source, const Rational &t,
                                                         bgfd::Budget &b) {
  owner(b, std::span(&t, 1));
  const auto slot = find_original_curve_root(cache, source, t);
  const auto &root = cache.original_roots()[slot.value];
  std::array<Rational, 3> result{Rational(b), Rational(b), Rational(b)};
  if (root.family == CurveFamily::line) {
    for (uint32_t a = 0; a < root.dimension; ++a)
      result[a] = Rational::from_double(b, root.frame[a])
                      .add(t.multiply(Rational::from_double(b, root.frame[3 + a])));
    return result;
  }
  if (root.family != CurveFamily::rational)
    return std::nullopt;
  const auto net = curve_detail::original_net_exact(cache, slot, b);
  const auto point = evaluate_curve(net, t);
  for (uint32_t a = 0; a < root.dimension; ++a)
    result[a] = point[a];
  return result;
}
std::array<Interval, 3> enclose_curve_hull_precise(const CurveSourceCache &cache,
                                                   Slot<CurveTag> source,
                                                   std::span<const Rational, 2> input,
                                                   bgfd::Budget &b) {
  owner(b, input);
  const auto &lo = input[0].compare(input[1]) < 0 ? input[0] : input[1];
  const auto &hi = input[0].compare(input[1]) < 0 ? input[1] : input[0];
  const auto first = find_original_curve_root(cache, source, lo),
             last = find_original_curve_root(cache, source, hi);
  std::array<Rational, 3> lower{Rational(b), Rational(b), Rational(b)},
      upper{Rational(b), Rational(b), Rational(b)};
  bool initialized = false;
  auto include = [&](std::span<const Rational, 3> point) {
    for (uint32_t a = 0; a < 3; ++a) {
      b.charge(1);
      if (!initialized || point[a].compare(lower[a]) < 0)
        lower[a] = point[a];
      if (!initialized || point[a].compare(upper[a]) > 0)
        upper[a] = point[a];
    }
    initialized = true;
  };
  for (uint32_t i = first.value; i <= last.value; ++i) {
    b.charge(1);
    const auto &root = cache.original_roots()[i];
    auto a = Rational::from_double(b, root.original_t[0].lower),
         z = Rational::from_double(b, root.original_t[1].lower);
    if (a.compare(lo) < 0)
      a = lo;
    if (z.compare(hi) > 0)
      z = hi;
    if (a.compare(z) > 0)
      continue;
    if (root.family == CurveFamily::line) {
      for (const auto *t : {&a, &z}) {
        auto point = exact_curve_value(cache, source, *t, b);
        if (point)
          include(*point);
        else
          throw Error(ErrorCode::invalid_source, "missing original line point");
      }
      continue;
    }
    if (root.family != CurveFamily::rational)
      throw Error(ErrorCode::unsupported, "exact analytic curve hull profile unavailable");
    const auto original = curve_detail::original_net_exact(cache, {i}, b);
    if (a.compare(z) == 0) {
      const auto point = evaluate_curve(original, a);
      std::array<Rational, 3> xyz{point[0], point[1], root.dimension == 3 ? point[2] : Rational(b)};
      include(xyz);
      continue;
    }
    const auto restricted = restrict_curve(original, a, z);
    const auto stride = root.dimension + 1;
    for (uint32_t j = 0; j <= root.degree; ++j) {
      std::array<Rational, 3> xyz{Rational(b), Rational(b), Rational(b)};
      const auto &w = restricted.controls[j * stride + root.dimension];
      check(w.sign() > 0, "nonpositive original curve weight");
      for (uint32_t axis = 0; axis < root.dimension; ++axis)
        xyz[axis] = restricted.controls[j * stride + axis].divide(w);
      include(xyz);
    }
  }
  check(initialized, "empty original curve hull");
  std::array<Interval, 3> result{};
  for (uint32_t a = 0; a < 3; ++a)
    result[a] = {lower[a].bounds().lower, upper[a].bounds().upper};
  return result;
}
CurveSampleResult evaluate_curve_sample_precise(const CurveSourceCache &cache,
                                                OriginalCurveRootSlot slot, const Rational &t,
                                                ArithmeticStage stage, bgfd::Budget &b,
                                                std::optional<Vec3> fixed) {
  admit_numeric_environment();
  owner(b, std::span(&t, 1));
  check(slot.value < cache.original_roots().size() &&
            (stage == ArithmeticStage::p128 || stage == ArithmeticStage::p192 ||
             stage == ArithmeticStage::p256 || stage == ArithmeticStage::precise),
        "precise curve point profile/root");
  const auto &root = cache.original_roots()[slot.value];
  check(t.compare(Rational::from_double(b, root.original_t[0].lower)) >= 0 &&
            t.compare(Rational::from_double(b, root.original_t[1].lower)) <= 0,
        "precise curve point outside original root");
  GeometryContext context(b, {precision_bits(stage), precision_bits(stage) + 128});
  auto source = [&]() -> EnclosedPoint {
    if (root.family != CurveFamily::rational)
      return evaluate_analytic_curve(context, cache.source().view().curves[root.source_slot], t);
    const auto net = curve_detail::original_net_exact(cache, slot, b);
    const auto xyz = evaluate_curve(net, t);
    return {root.dimension,
            {ExactEnclosure(xyz[0]), ExactEnclosure(xyz[1]),
             ExactEnclosure(root.dimension == 3 ? xyz[2] : Rational(b))}};
  }();
  CurveSampleResult result;
  // ExactEnclosure deliberately has no assignment; compute departure separately.
  std::array<Rational, 3> departure{Rational(b), Rational(b), Rational(b)};
  for (uint32_t a = 0; a < 3; ++a) {
    const auto lo = source.coordinates[a].lower.bounds().lower,
               hi = source.coordinates[a].upper.bounds().upper;
    result.original_xyz[a] = {lo, hi};
    auto q =
        fixed ? (*fixed)[a] : numeric::rn_add(numeric::rn_mul(lo, .5), numeric::rn_mul(hi, .5));
    check(numeric::finite(q) && (root.dimension == 3 || a != 2 || q == 0.),
          "invalid precise fixed original point");
    result.actual_xyz[a] = fixed ? q : (q == 0. ? 0. : q);
    auto left = source.coordinates[a].lower.subtract(Rational::from_double(b, q)),
         right = source.coordinates[a].upper.subtract(Rational::from_double(b, q));
    if (left.sign() < 0)
      left = left.negative();
    if (right.sign() < 0)
      right = right.negative();
    departure[a] = left.compare(right) > 0 ? left : right;
  }
  result.departure_upper = norm_upper(departure, b).bounds().upper;
  result.status = NumericStatus::ready;
  return result;
}
PreciseCurveBoundResult bound_curve_node_precise(const CurveSourceCache &cache, CurveRootSlot slot,
                                                 std::span<const Rational, 2> t,
                                                 std::span<const double, 6> actual,
                                                 const Rational &allowance, bgfd::Budget &b) {
  admit_numeric_environment();
  check(slot.value < cache.roots().size() && &allowance.budget() == &b && allowance.sign() > 0,
        "precise curve bound owner/root");
  const auto &root = cache.roots()[slot.value];
  validate_interval(root, t, b);
  std::array<Rational, 2> endpoint_square{Rational(b), Rational(b)};
  for (uint32_t i = 0; i < 2; ++i) {
    const auto original = find_original_curve_root(cache, Slot<CurveTag>{root.source_slot}, t[i]);
    const auto point =
        evaluate_curve_sample_precise(cache, original, t[i], ArithmeticStage::p192, b,
                                      Vec3{actual[3 * i], actual[3 * i + 1], actual[3 * i + 2]});
    // The returned departure is an upper; rational endpoint equality below is
    // checked exactly when the source permits it.
    const auto exact = exact_curve_value(cache, Slot<CurveTag>{root.source_slot}, t[i], b);
    if (exact)
      for (uint32_t a = 0; a < 3; ++a) {
        const auto d = (*exact)[a].subtract(Rational::from_double(b, actual[3 * i + a]));
        endpoint_square[i] = endpoint_square[i].add(d.multiply(d));
      }
    else
      endpoint_square[i] = Rational::from_double(b, point.departure_upper)
                               .multiply(Rational::from_double(b, point.departure_upper));
  }
  auto maximum_square =
      endpoint_square[0].compare(endpoint_square[1]) > 0 ? endpoint_square[0] : endpoint_square[1];
  auto upper = norm_from_square(maximum_square);
  const auto endpoint_upper = upper.bounds().upper;
  bool parameter = false;
  const auto allowance_square = allowance.multiply(allowance);
  if (root.family == CurveFamily::rational) {
    const auto original = curve_detail::original_net_exact(cache, root.original, b);
    const auto net = restrict_curve(original, t[0], t[1]);
    const auto stride = root.dimension + 1, p = root.degree;
    auto wmin = net.controls[root.dimension];
    maximum_square = Rational(b);
    for (uint32_t i = 0; i <= p; ++i)
      if (net.controls[i * stride + root.dimension].compare(wmin) < 0)
        wmin = net.controls[i * stride + root.dimension];
    for (uint32_t k = 0; k <= p + 1; ++k) {
      std::array<Rational, 3> residual{Rational(b), Rational(b), Rational(b)};
      for (uint32_t a = 0; a < root.dimension; ++a) {
        if (k <= p)
          residual[a] =
              residual[a].add(net.controls[k * stride + a]
                                  .subtract(net.controls[k * stride + root.dimension].multiply(
                                      Rational::from_double(b, actual[a])))
                                  .multiply(Rational(b, static_cast<int64_t>(p + 1 - k)))
                                  .divide(Rational(b, static_cast<int64_t>(p + 1))));
        if (k > 0)
          residual[a] = residual[a].add(
              net.controls[(k - 1) * stride + a]
                  .subtract(net.controls[(k - 1) * stride + root.dimension].multiply(
                      Rational::from_double(b, actual[3 + a])))
                  .multiply(Rational(b, static_cast<int64_t>(k)))
                  .divide(Rational(b, static_cast<int64_t>(p + 1))));
      }
      auto square = Rational(b);
      for (const auto &x : residual)
        square = square.add(x.multiply(x));
      if (square.compare(maximum_square) > 0)
        maximum_square = square;
    }
    parameter = maximum_square.compare(allowance_square.multiply(wmin.multiply(wmin))) <= 0;
    upper = norm_from_square(maximum_square).divide(wmin);
  } else {
    const auto h = t[1].subtract(t[0]);
    upper = upper.add(Rational::from_double(b, root.second_derivative_upper.upper)
                          .multiply(h.multiply(h))
                          .divide(Rational(b, 8)));
    parameter = upper.compare(allowance) <= 0;
  }
  CurveNodeBounds bounds;
  bounds.status = NumericStatus::ready;
  bounds.parameter_upper = upper.bounds().upper;
  bounds.numerical_upper = endpoint_upper;
  if (root.line_profile) {
    const auto image_square = endpoint_square[0].compare(endpoint_square[1]) > 0
                                  ? endpoint_square[0]
                                  : endpoint_square[1];
    bounds.image_available = 1;
    bounds.image_upper = norm_from_square(image_square).bounds().upper;
    bounds.correspondence = CurveCorrespondence::monotone_line_image;
  }
  return {bounds, parameter,
          root.line_profile && endpoint_square[0].compare(allowance_square) <= 0 &&
              endpoint_square[1].compare(allowance_square) <= 0};
}
OriginalRootSlot find_original_carrier_root(const SourceCache &cache, Slot<CarrierTag> source,
                                            std::span<const Rational, 2> uv) {
  return {find_carrier_root_slot(cache.original_roots(), source, uv, RootBoxKind::original)};
}
std::optional<std::array<Rational, 3>> exact_carrier_value(const SourceCache &cache,
                                                           Slot<CarrierTag> source,
                                                           std::span<const Rational, 2> uv,
                                                           bgfd::Budget &b) {
  owner(b, uv);
  const auto slot = find_original_carrier_root(cache, source, uv);
  const auto &root = cache.original_roots()[slot.value];
  if (root.family == GeometryFamily::rational_tensor)
    return exact_original_surface_point(cache, slot, uv, b);
  if (root.family != GeometryFamily::plane)
    return std::nullopt;
  GeometryContext context(b);
  const auto value = evaluate_analytic_surface(
      context, cache.source().view().carriers[source.value], uv[0], uv[1]);
  std::array<Rational, 3> result{Rational(b), Rational(b), Rational(b)};
  for (uint32_t a = 0; a < 3; ++a) {
    check(value.coordinates[a].lower.compare(value.coordinates[a].upper) == 0,
          "original plane source is not exact rational");
    result[a] = value.coordinates[a].lower;
  }
  return result;
}
namespace {
std::array<Rational, 3> actual_at(std::span<const Rational, 6> actual, const Rational &u) {
  return {actual[0].add(u.multiply(actual[3].subtract(actual[0]))),
          actual[1].add(u.multiply(actual[4].subtract(actual[1]))),
          actual[2].add(u.multiply(actual[5].subtract(actual[2])))};
}
std::array<Rational, 2> uv_at(std::span<const Rational, 4> uv, const Rational &u) {
  return {uv[0].add(u.multiply(uv[2].subtract(uv[0]))),
          uv[1].add(u.multiply(uv[3].subtract(uv[1])))};
}
void add_cuts(std::pmr::vector<Rational> &cuts, std::span<const double> knots, const Rational &a,
              const Rational &c, bgfd::Budget &b) {
  if (a.compare(c) == 0)
    return;
  const auto delta = c.subtract(a);
  const auto &lo = a.compare(c) < 0 ? a : c;
  const auto &hi = a.compare(c) < 0 ? c : a;
  const auto first =
      std::upper_bound(knots.begin(), knots.end(), lo, [&](const Rational &x, double k) {
        b.charge(1);
        return x.compare(Rational::from_double(b, k)) < 0;
      });
  for (auto i = first; i != knots.end(); ++i) {
    b.charge(1);
    const auto k = Rational::from_double(b, *i);
    if (k.compare(hi) >= 0)
      break;
    cuts.push_back(k.subtract(a).divide(delta));
  }
}
void order_cuts(std::pmr::vector<Rational> &cuts, bgfd::Budget &b) {
  std::sort(cuts.begin(), cuts.end(), [&](const auto &a, const auto &c) {
    b.charge(1);
    return a.compare(c) < 0;
  });
  cuts.erase(std::unique(cuts.begin(), cuts.end(),
                         [&](const auto &a, const auto &c) {
                           b.charge(1);
                           return a.compare(c) == 0;
                         }),
             cuts.end());
}
Interval original_departure(const SampleResult &source, std::span<const Rational, 3> actual,
                            bgfd::Budget &b) {
  check(source.status == NumericStatus::ready, "selected original carrier point unresolved");
  Interval difference[3]{};
  for (uint32_t a = 0; a < 3; ++a) {
    b.charge(1);
    difference[a] = numeric::sub(source.original_xyz[a], enclosure(actual[a]));
  }
  return numeric::point(numeric::norm_upper(difference));
}
struct TraceBlossom {
  uint32_t channel, high_u, high_v;
  Interval u0, u1, v0, v1;
};
Interval trace_blossom(const RootDescriptor &root, const Interval *net, TraceBlossom request,
                       bgfd::Budget &b) {
  const auto channel = request.channel, high_u = request.high_u, high_v = request.high_v;
  const auto u0 = request.u0, u1 = request.u1, v0 = request.v0, v1 = request.v1;
  Interval temporary[16]{};
  const auto p = root.degree_u, q = root.degree_v;
  for (uint32_t i = 0; i <= p; ++i)
    for (uint32_t j = 0; j <= q; ++j)
      temporary[i * (q + 1) + j] = net[(i * (q + 1) + j) * 4 + channel];
  for (uint32_t level = 0; level < p; ++level) {
    const auto u = level < p - high_u ? u0 : u1;
    for (uint32_t i = 0; i < p - level; ++i)
      for (uint32_t j = 0; j <= q; ++j) {
        b.charge(1);
        temporary[i * (q + 1) + j] =
            numeric::lerp(temporary[i * (q + 1) + j], temporary[(i + 1) * (q + 1) + j], u);
      }
  }
  for (uint32_t level = 0; level < q; ++level) {
    const auto v = level < q - high_v ? v0 : v1;
    for (uint32_t j = 0; j < q - level; ++j) {
      b.charge(1);
      temporary[j] = numeric::lerp(temporary[j], temporary[j + 1], v);
    }
  }
  return temporary[0];
}
std::pmr::vector<Interval> restrict_surface_line(const SourceCache &cache,
                                                 const RootDescriptor &root,
                                                 std::span<const Rational, 4> endpoints,
                                                 bgfd::Budget &b) {
  const auto controls = cache.controls();
  const auto *net = controls.data() + root.controls.first;
  const auto ua = Rational::from_double(b, root.original_box[0].lower),
             uh = Rational::from_double(b, root.original_box[1].lower).subtract(ua);
  const auto va = Rational::from_double(b, root.original_box[2].lower),
             vh = Rational::from_double(b, root.original_box[3].lower).subtract(va);
  const auto u0 = unit_enclosure(endpoints[0].subtract(ua).divide(uh)),
             u1 = unit_enclosure(endpoints[2].subtract(ua).divide(uh));
  const auto v0 = unit_enclosure(endpoints[1].subtract(va).divide(vh)),
             v1 = unit_enclosure(endpoints[3].subtract(va).divide(vh));
  const auto p = root.degree_u, q = root.degree_v, d = p + q;
  std::pmr::vector<Interval> result(&b);
  result.resize(static_cast<size_t>(d + 1) * 4);
  for (uint32_t k = 0; k <= d; ++k)
    for (uint32_t channel = 0; channel < 4; ++channel) {
      Interval value{};
      for (uint32_t i = 0; i <= p; ++i) {
        if (k < i || k - i > q)
          continue;
        const auto j = k - i;
        const auto factor = numeric::div(
            numeric::point(static_cast<double>(numeric::binomial(p, i) * numeric::binomial(q, j))),
            numeric::point(static_cast<double>(numeric::binomial(d, k))));
        value = numeric::add(
            value,
            numeric::mul(factor, trace_blossom(root, net, {channel, i, j, u0, u1, v0, v1}, b)));
      }
      result[k * 4 + channel] = value;
    }
  return result;
}
struct HomogeneousDerivative {
  uint32_t channel, du, dv;
  double anchor;
};
Interval homogeneous_derivative_hull(const RootDescriptor &root, const Interval *net,
                                     HomogeneousDerivative request, bgfd::Budget &b) {
  const auto channel = request.channel, du = request.du, dv = request.dv;
  const auto anchor = request.anchor;
  if (du > root.degree_u || dv > root.degree_v)
    return {};
  Interval hull{};
  bool first = true;
  for (uint32_t i = 0; i <= root.degree_u - du; ++i)
    for (uint32_t j = 0; j <= root.degree_v - dv; ++j) {
      Interval value{};
      for (uint32_t a = 0; a <= du; ++a)
        for (uint32_t c = 0; c <= dv; ++c) {
          b.charge(1);
          const auto index = ((i + a) * (root.degree_v + 1) + j + c) * 4;
          auto h = net[index + channel];
          if (channel < 3)
            h = numeric::sub(h, numeric::scale(net[index + 3], anchor));
          const auto coefficient =
              static_cast<double>(numeric::binomial(du, a) * numeric::binomial(dv, c));
          auto term = numeric::scale(h, coefficient);
          if ((du + dv - a - c) % 2)
            term = numeric::neg(term);
          value = numeric::add(value, term);
        }
      if (first) {
        hull = value;
        first = false;
      } else {
        hull.lower = std::min(hull.lower, value.lower);
        hull.upper = std::max(hull.upper, value.upper);
      }
    }
  for (uint32_t a = 0; a < du; ++a)
    hull =
        numeric::mul(hull, numeric::div(numeric::point(static_cast<double>(root.degree_u - a)),
                                        numeric::sub(root.original_box[1], root.original_box[0])));
  for (uint32_t a = 0; a < dv; ++a)
    hull =
        numeric::mul(hull, numeric::div(numeric::point(static_cast<double>(root.degree_v - a)),
                                        numeric::sub(root.original_box[3], root.original_box[2])));
  return hull;
}
} // namespace
CarrierDerivativeBounds bound_original_carrier_derivatives(const SourceCache &cache,
                                                           OriginalRootSlot slot,
                                                           std::span<const Interval, 4> box,
                                                           bgfd::Budget &b) {
  admit_numeric_environment();
  check(slot.value < cache.original_roots().size(), "foreign original derivative root");
  const auto &root = cache.original_roots()[slot.value];
  for (uint32_t a = 0; a < 2; ++a)
    check(numeric::valid(box[2 * a]) && numeric::valid(box[2 * a + 1]) &&
              box[2 * a].lower >= root.original_box[2 * a].lower &&
              box[2 * a + 1].upper <= root.original_box[2 * a + 1].upper &&
              box[2 * a].lower <= box[2 * a + 1].upper,
          "derivative box outside original span");
  CarrierDerivativeBounds result;
  if (root.family != GeometryFamily::rational_tensor) {
    Interval x[3]{}, y[3]{}, z[3]{};
    for (uint32_t a = 0; a < 3; ++a) {
      x[a] = numeric::point(root.frame[3 + a]);
      y[a] = numeric::point(root.frame[6 + a]);
      z[a] = numeric::point(root.frame[9 + a]);
    }
    const auto xn = numeric::norm_upper(x), yn = numeric::norm_upper(y),
               zn = numeric::norm_upper(z);
    if (root.family == GeometryFamily::plane) {
      result.du = xn;
      result.dv = yn;
      return result;
    }
    const auto A = root.frame_a, F = root.frame_f,
               r = numeric::point(numeric::absolute(root.radii[0]));
    if (root.family == GeometryFamily::cylinder) {
      result.du = result.duu = numeric::mul(r, A).upper;
      result.dv = zn;
    } else if (root.family == GeometryFamily::cone) {
      const auto radius = numeric::add(numeric::point(root.radii[0]),
                                       numeric::mul({box[2].lower, box[3].upper}, root.cone_sin));
      result.du = result.duu = numeric::scale(A, numeric::magnitude(radius)).upper;
      result.dv = numeric::add(numeric::scale(A, numeric::magnitude(root.cone_sin)),
                               numeric::mul(numeric::point(numeric::magnitude(root.cone_cos)),
                                            numeric::point(zn)))
                      .upper;
      result.duv = numeric::scale(A, numeric::magnitude(root.cone_sin)).upper;
    } else {
      const auto minor = numeric::point(
          numeric::absolute(root.family == GeometryFamily::sphere ? root.radii[0] : root.radii[1]));
      result.du = result.duu =
          numeric::mul(root.family == GeometryFamily::sphere ? r : numeric::add(r, minor), A).upper;
      result.dv = result.dvv = numeric::mul(minor, F).upper;
      result.duv = numeric::mul(minor, A).upper;
    }
  } else {
    const auto all = cache.controls();
    const auto *net = all.data() + root.controls.first;
    const auto w = homogeneous_derivative_hull(root, net, {3, 0, 0, 0.}, b),
               wu = homogeneous_derivative_hull(root, net, {3, 1, 0, 0.}, b),
               wv = homogeneous_derivative_hull(root, net, {3, 0, 1, 0.}, b);
    const auto wuu = homogeneous_derivative_hull(root, net, {3, 2, 0, 0.}, b),
               wuv = homogeneous_derivative_hull(root, net, {3, 1, 1, 0.}, b),
               wvv = homogeneous_derivative_hull(root, net, {3, 0, 2, 0.}, b);
    if (!numeric::valid(w) || w.lower <= 0.)
      throw Error(ErrorCode::unresolved, "carrier derivative positive weight enclosure");
    Interval du[3]{}, dv[3]{}, duu[3]{}, duv[3]{}, dvv[3]{};
    for (uint32_t a = 0; a < 3; ++a) {
      const auto first = numeric::div(net[a], net[3]);
      const auto anchor =
          numeric::rn_add(numeric::rn_mul(first.lower, .5), numeric::rn_mul(first.upper, .5));
      const auto value =
          numeric::div(homogeneous_derivative_hull(root, net, {a, 0, 0, anchor}, b), w);
      du[a] =
          numeric::div(numeric::sub(homogeneous_derivative_hull(root, net, {a, 1, 0, anchor}, b),
                                    numeric::mul(value, wu)),
                       w);
      dv[a] =
          numeric::div(numeric::sub(homogeneous_derivative_hull(root, net, {a, 0, 1, anchor}, b),
                                    numeric::mul(value, wv)),
                       w);
      duu[a] = numeric::div(
          numeric::sub(numeric::sub(homogeneous_derivative_hull(root, net, {a, 2, 0, anchor}, b),
                                    numeric::mul(value, wuu)),
                       numeric::scale(numeric::mul(du[a], wu), 2.)),
          w);
      duv[a] =
          numeric::div(numeric::sub(numeric::sub(numeric::sub(homogeneous_derivative_hull(
                                                                  root, net, {a, 1, 1, anchor}, b),
                                                              numeric::mul(value, wuv)),
                                                 numeric::mul(du[a], wv)),
                                    numeric::mul(dv[a], wu)),
                       w);
      dvv[a] = numeric::div(
          numeric::sub(numeric::sub(homogeneous_derivative_hull(root, net, {a, 0, 2, anchor}, b),
                                    numeric::mul(value, wvv)),
                       numeric::scale(numeric::mul(dv[a], wv), 2.)),
          w);
    }
    result = {numeric::norm_upper(du), numeric::norm_upper(dv), numeric::norm_upper(duu),
              numeric::norm_upper(duv), numeric::norm_upper(dvv)};
  }
  for (const auto x : {result.du, result.dv, result.duu, result.duv, result.dvv})
    if (!numeric::finite(x) || x < 0.)
      throw Error(ErrorCode::unresolved, "original carrier derivative enclosure");
  return result;
}
AffineTracePlan prepare_affine_trace(const SourceCache &cache, const AffineTraceRequest &request,
                                     bgfd::Budget &b) {
  admit_numeric_environment();
  owner(b, request.uv);
  owner(b, request.actual_endpoints);
  check(request.carrier.value < cache.source().view().carriers.size(),
        "foreign affine trace carrier");
  const auto &carrier = cache.source().view().carriers[request.carrier.value];
  std::pmr::vector<Rational> cuts(&b);
  cuts.emplace_back(b, 0);
  cuts.emplace_back(b, 1);
  if (carrier.kind == 6) {
    add_cuts(cuts, cache.source().view().view(carrier.knot_u), request.uv[0], request.uv[2], b);
    add_cuts(cuts, cache.source().view().view(carrier.knot_v), request.uv[1], request.uv[3], b);
  }
  order_cuts(cuts, b);
  AffineTracePlan plan(cache, b);
  for (size_t i = 1; i < cuts.size(); ++i) {
    const auto mid = cuts[i - 1].add(cuts[i]).divide(Rational(b, 2));
    const auto uvmid = uv_at(request.uv, mid);
    const auto original = find_original_carrier_root(cache, request.carrier, uvmid);
    const auto &root = cache.original_roots()[original.value];
    const auto left = uv_at(request.uv, cuts[i - 1]), right = uv_at(request.uv, cuts[i]);
    const std::array endpoints{left[0], left[1], right[0], right[1]};
    const auto qa = actual_at(request.actual_endpoints, cuts[i - 1]),
               qb = actual_at(request.actual_endpoints, cuts[i]);
    AffineTraceBoundInput bound;
    bound.input.t[0] = enclosure(cuts[i - 1]);
    bound.input.t[1] = enclosure(cuts[i]);
    for (uint32_t a = 0; a < 3; ++a) {
      bound.affine_endpoints[a] = enclosure(qa[a]);
      bound.affine_endpoints[3 + a] = enclosure(qb[a]);
    }
    if (root.family == GeometryFamily::rational_tensor) {
      auto net = restrict_surface_line(cache, root, endpoints, b);
      bound.kind = AffineTraceBoundKind::rational;
      bound.rational_curve.family = CurveFamily::rational;
      bound.rational_curve.dimension = 3;
      bound.rational_curve.degree = root.degree_u + root.degree_v;
      check(plan.controls.size() <= UINT32_MAX && net.size() <= UINT32_MAX - plan.controls.size(),
            "affine trace control capacity");
      bound.input.controls = {static_cast<uint32_t>(plan.controls.size()),
                              static_cast<uint32_t>(net.size())};
      bound.rational_curve.controls = bound.input.controls;
      plan.controls.insert(plan.controls.end(), net.begin(), net.end());
    } else {
      bound.kind = AffineTraceBoundKind::direct_derivative;
      const std::array box{enclosure(left[0].compare(right[0]) < 0 ? left[0] : right[0]),
                           enclosure(left[0].compare(right[0]) < 0 ? right[0] : left[0]),
                           enclosure(left[1].compare(right[1]) < 0 ? left[1] : right[1]),
                           enclosure(left[1].compare(right[1]) < 0 ? right[1] : left[1])};
      const auto derivatives = bound_original_carrier_derivatives(cache, original, box, b);
      const auto u = enclosure(request.uv[2].subtract(request.uv[0])),
                 v = enclosure(request.uv[3].subtract(request.uv[1]));
      bound.second_derivative_upper =
          numeric::add(
              numeric::add(numeric::scale(numeric::square(u), derivatives.duu),
                           numeric::scale(numeric::point(numeric::magnitude(numeric::mul(u, v))),
                                          2. * derivatives.duv)),
              numeric::scale(numeric::square(v), derivatives.dvv))
              .upper;
      for (uint32_t side = 0; side < 2; ++side) {
        const auto &uv = side ? right : left;
        SampleInput input;
        input.original = find_original_carrier_root(cache, request.carrier, uv);
        input.uv[0] = enclosure(uv[0]);
        input.uv[1] = enclosure(uv[1]);
        const auto source = evaluate_sample(cache, input);
        bound.input.endpoint_departures[side] =
            original_departure(
                source, side ? std::span<const Rational, 3>(qb) : std::span<const Rational, 3>(qa),
                b)
                .upper;
      }
    }
    plan.pieces.push_back({original, {cuts[i - 1], cuts[i]}, bound});
  }
  return plan;
}
CurveNodeBounds bound_affine_trace_piece(const AffineTracePlan &plan, size_t index) {
  admit_numeric_environment();
  check(index < plan.pieces.size(), "affine trace piece outside plan");
  const auto &bound = plan.pieces[index].bound;
  check(bound.input.controls.first <= plan.controls.size() &&
            bound.input.controls.count <= plan.controls.size() - bound.input.controls.first,
        "affine trace net range");
  return numeric::bound_affine_trace_rn64(
      bound,
      bound.input.controls.count ? plan.controls.data() + bound.input.controls.first : nullptr);
}
CurveChordPlan prepare_curve_chord(const CurveSourceCache &cache, const CurveChordRequest &request,
                                   bgfd::Budget &b) {
  owner(b, request.original_t);
  owner(b, request.actual_endpoints);
  check(request.original_t[0].compare(request.original_t[1]) != 0,
        "zero original curve chord interval");
  check(request.curve.value < cache.source().view().curves.size(), "foreign curve chord source");
  const auto &curve = cache.source().view().curves[request.curve.value];
  std::pmr::vector<Rational> cuts(&b);
  cuts.emplace_back(b, 0);
  cuts.emplace_back(b, 1);
  if (curve.kind == 5)
    add_cuts(cuts, cache.source().view().view(curve.knots), request.original_t[0],
             request.original_t[1], b);
  order_cuts(cuts, b);
  CurveChordPlan plan(cache, b);
  const auto width = request.original_t[1].subtract(request.original_t[0]);
  for (size_t i = 1; i < cuts.size(); ++i) {
    const auto a = request.original_t[0].add(cuts[i - 1].multiply(width)),
               c = request.original_t[0].add(cuts[i].multiply(width));
    const bool forward = a.compare(c) < 0;
    const std::array interval{forward ? a : c, forward ? c : a};
    const auto prepared = find_prepared_curve_root(cache, request.curve, interval);
    const auto &root = cache.roots()[prepared.value];
    const auto qa = actual_at(request.actual_endpoints, cuts[i - 1]),
               qb = actual_at(request.actual_endpoints, cuts[i]);
    AffineTraceBoundInput bound;
    bound.rational_curve = root;
    bound.rational_curve.line_profile = 0;
    for (uint32_t j = 0; j < 3; ++j) {
      check(numeric::valid(request.source_translation[j]), "invalid original curve lift enclosure");
      bound.affine_endpoints[j] = numeric::sub(enclosure(qa[j]), request.source_translation[j]);
      bound.affine_endpoints[3 + j] = numeric::sub(enclosure(qb[j]), request.source_translation[j]);
    }
    if (root.family == CurveFamily::rational) {
      auto net = prepare_curve_interval_controls(cache, prepared, interval, b);
      const auto stride = root.dimension + 1;
      if (!forward)
        for (uint32_t k = 0; k < (root.degree + 1) / 2; ++k)
          for (uint32_t j = 0; j < stride; ++j)
            std::swap(net[k * stride + j], net[(root.degree - k) * stride + j]);
      // Trace-bound packets require spatial stride4. Preserve UV as exact z0.
      if (root.dimension == 2) {
        auto spatial = empty_intervals(static_cast<size_t>(root.degree + 1) * 4, b);
        for (uint32_t k = 0; k <= root.degree; ++k) {
          spatial[4 * k] = net[3 * k];
          spatial[4 * k + 1] = net[3 * k + 1];
          spatial[4 * k + 3] = net[3 * k + 2];
        }
        net = std::move(spatial);
        bound.rational_curve.dimension = 3;
      }
      bound.kind = AffineTraceBoundKind::rational;
      bound.input.controls = {static_cast<uint32_t>(plan.controls.size()),
                              static_cast<uint32_t>(net.size())};
      plan.controls.insert(plan.controls.end(), net.begin(), net.end());
      bound.input.t[0] = enclosure(cuts[i - 1]);
      bound.input.t[1] = enclosure(cuts[i]);
    } else {
      bound.kind = AffineTraceBoundKind::direct_derivative;
      bound.second_derivative_upper =
          numeric::mul(root.second_derivative_upper, numeric::square(enclosure(width))).upper;
      bound.input.t[0] = enclosure(cuts[i - 1]);
      bound.input.t[1] = enclosure(cuts[i]);
      for (uint32_t side = 0; side < 2; ++side) {
        const auto &t = side ? c : a;
        CurveSampleInput input;
        input.original = find_original_curve_root(cache, request.curve, t);
        input.t = enclosure(t);
        const auto value = evaluate_curve_sample(cache, input);
        check(value.status == NumericStatus::ready, "raw curve chord endpoint unresolved");
        Interval residual[3]{};
        for (uint32_t j = 0; j < 3; ++j)
          residual[j] = numeric::sub(value.original_xyz[j], bound.affine_endpoints[3 * side + j]);
        bound.input.endpoint_departures[side] = numeric::norm_upper(residual);
      }
    }
    plan.pieces.push_back({{cuts[i - 1], cuts[i]}, bound});
  }
  return plan;
}
namespace {
std::pmr::vector<Rational> image_binomials(uint32_t degree, bgfd::Budget &budget) {
  std::pmr::vector<Rational> result(&budget);
  result.reserve(degree + 1);
  result.emplace_back(budget, 1);
  for (uint32_t i = 1; i <= degree; ++i)
    result.push_back(result.back().multiply(Rational(budget, degree + 1 - i))
                                      .divide(Rational(budget, i)));
  return result;
}
} // namespace
std::optional<OriginalLineImage> original_curve_line_image(
    const CurveSourceCache &cache, Slot<CurveTag> source, std::span<const Rational, 2> input,
    bgfd::Budget &budget) {
  admit_numeric_environment();
  owner(budget, input);
  if (input[0].compare(input[1]) == 0)
    return std::nullopt;
  const auto a = exact_curve_value(cache, source, input[0], budget),
             z = exact_curve_value(cache, source, input[1], budget);
  if (!a || !z)
    return std::nullopt;
  const auto dx = (*z)[0].subtract((*a)[0]), dy = (*z)[1].subtract((*a)[1]);
  if (dx.sign() == 0 && dy.sign() == 0)
    return std::nullopt;
  const auto ascending = input[0].compare(input[1]) < 0;
  const auto &lo = ascending ? input[0] : input[1];
  const auto &hi = ascending ? input[1] : input[0];
  const auto first = find_original_curve_root(cache, source, lo),
             last = find_original_curve_root(cache, source, hi);
  auto covered = lo;
  for (uint32_t index = first.value; index <= last.value; ++index) {
    budget.charge(1);
    const auto &root = cache.original_roots()[index];
    check(root.source_slot == source.value && root.dimension == 2,
          "line image requires complete original 2D curve interval");
    auto begin = Rational::from_double(budget, root.original_t[0].lower),
         end = Rational::from_double(budget, root.original_t[1].lower);
    if (begin.compare(lo) < 0)
      begin = lo;
    if (end.compare(hi) > 0)
      end = hi;
    if (begin.compare(end) >= 0)
      continue;
    check(begin.compare(covered) == 0, "line image missing original span coverage");
    covered = end;
    if (root.family == CurveFamily::line)
      continue;
    if (root.family != CurveFamily::rational)
      return std::nullopt;
    const auto original = curve_detail::original_net_exact(cache, {index}, budget);
    const auto net = restrict_curve(original, begin, end);
    const auto p = net.degree;
    if (p == 0)
      return std::nullopt;
    std::pmr::vector<Rational> projection(&budget), weights(&budget);
    projection.reserve(p + 1);
    weights.reserve(p + 1);
    for (uint32_t i = 0; i <= p; ++i) {
      budget.charge(1);
      const auto &x = net.controls[i * 3], &y = net.controls[i * 3 + 1],
                 &w = net.controls[i * 3 + 2];
      check(w.sign() > 0, "line image original positive weight");
      const auto cross = dx.multiply(y.subtract(w.multiply((*a)[1])))
                             .subtract(dy.multiply(x.subtract(w.multiply((*a)[0]))));
      if (cross.sign() != 0)
        return std::nullopt;
      projection.push_back(dx.multiply(x).add(dy.multiply(y)));
      weights.push_back(w);
    }
    const auto lower = image_binomials(p - 1, budget),
               upper = image_binomials(p, budget),
               product = image_binomials(2 * p - 1, budget);
    for (uint32_t k = 0; k < 2 * p; ++k) {
      auto coefficient = Rational(budget);
      for (uint32_t i = 0; i < p; ++i) {
        if (k < i || k - i > p)
          continue;
        budget.charge(1);
        const auto j = k - i;
        const auto numerator = projection[i + 1].subtract(projection[i]).multiply(weights[j])
                                   .subtract(weights[i + 1].subtract(weights[i]).multiply(projection[j]));
        coefficient = coefficient.add(numerator.multiply(lower[i]).multiply(upper[j])
                                                     .divide(product[k]));
      }
      if ((ascending && coefficient.sign() < 0) || (!ascending && coefficient.sign() > 0))
        return std::nullopt;
    }
  }
  check(covered.compare(hi) == 0, "line image incomplete original interval");
  const auto sa = dx.multiply((*a)[0]).add(dy.multiply((*a)[1])),
             sz = dx.multiply((*z)[0]).add(dy.multiply((*z)[1]));
  return OriginalLineImage{{(*a)[0], (*a)[1], (*z)[0], (*z)[1]}, {dx, dy}, {sa, sz}};
}
} // namespace bgmesh::adaptive
