#include "adaptive_curve_geometry.hpp"
#include "adaptive_curve_numeric.hpp"
#include <algorithm>
#include <array>
#include <tuple>
namespace bgmesh::adaptive {
CurveSourceCacheData::CurveSourceCacheData(std::shared_ptr<RunResource> owner, SourceStore original,
                                           NumericProfile selected)
    : resource(std::move(owner)), source(std::move(original)), profile(selected),
      roots(resource.get()), original_roots(resource.get()), controls(resource.get()) {}
std::span<const Interval> CurveSourceCache::controls(ArithmeticStage stage) const {
  if (stage != ArithmeticStage::rn64)
    throw Error(ErrorCode::unsupported, "curve root tier not resident");
  return data_->controls;
}
namespace {
void check(bool ok, const char *message) {
  if (!ok)
    throw Error(ErrorCode::invalid_source, message);
}
struct IntervalArithmetic {
  bgfd::Budget &budget;
  Interval zero() const { return {}; }
  Interval from(double x) const { return numeric::point(x); }
  Interval add(Interval a, Interval b) const { return numeric::add(a, b); }
  Interval sub(Interval a, Interval b) const { return numeric::sub(a, b); }
  Interval mul(Interval a, Interval b) const { return numeric::mul(a, b); }
  Interval div(Interval a, Interval b) const { return numeric::div(a, b); }
};
struct ExactArithmetic {
  bgfd::Budget &budget;
  Rational zero() const { return Rational(budget); }
  Rational from(double x) const { return Rational::from_double(budget, x); }
  Rational add(const Rational &a, const Rational &b) const { return a.add(b); }
  Rational sub(const Rational &a, const Rational &b) const { return a.subtract(b); }
  Rational mul(const Rational &a, const Rational &b) const { return a.multiply(b); }
  Rational div(const Rational &a, const Rational &b) const { return a.divide(b); }
};
// Local original Cox recurrence IN Bernstein basis; each level is computed
// once. O(p^3), not a recursive re-expansion for every basis/coefficient.
template <class Ops>
auto extract_span(const SourceData &source, const Curve &curve, std::span<const double> knots,
                  uint32_t span, const Ops &op) {
  using V = decltype(op.zero());
  // An original Bezier row already IS the natural-span Bernstein net.
  // Passing it through Cox recurrence adds artificial dependency growth at p32.
  if (curve.kind == 4) {
    std::pmr::vector<V> direct(&op.budget);
    for (const auto x : source.view(curve.controls)) {
      op.budget.charge(1);
      direct.push_back(op.from(x));
    }
    return direct;
  }
  const auto p = curve.degree, width = p + 1, rows = p + 2;
  std::pmr::vector<V> previous(&op.budget), next(&op.budget), output(&op.budget);
  previous.reserve(rows * width);
  next.reserve(rows * width);
  for (uint32_t i = 0; i < rows * width; ++i) {
    previous.push_back(op.zero());
    next.push_back(op.zero());
  }
  previous[p * width] = op.from(1.);
  const auto a = op.from(knots[span]), b = op.from(knots[span + 1]);
  for (uint32_t degree = 1; degree <= p; ++degree) {
    for (auto &x : next)
      x = op.zero();
    for (uint32_t row = p - degree; row <= p; ++row) {
      const auto index = span - p + row;
      for (uint32_t j = 0; j <= degree; ++j) {
        op.budget.charge(1);
        auto value = op.zero();
        const auto lo_factor =
            op.div(op.from(static_cast<double>(degree - j)), op.from(static_cast<double>(degree)));
        const auto hi_factor =
            op.div(op.from(static_cast<double>(j)), op.from(static_cast<double>(degree)));
        if (knots[index + degree] != knots[index]) {
          const auto den = op.sub(op.from(knots[index + degree]), op.from(knots[index]));
          if (j < degree)
            value = op.add(value,
                           op.mul(op.mul(lo_factor, op.div(op.sub(a, op.from(knots[index])), den)),
                                  previous[row * width + j]));
          if (j > 0)
            value = op.add(value,
                           op.mul(op.mul(hi_factor, op.div(op.sub(b, op.from(knots[index])), den)),
                                  previous[row * width + j - 1]));
        }
        if (knots[index + degree + 1] != knots[index + 1]) {
          const auto den = op.sub(op.from(knots[index + degree + 1]), op.from(knots[index + 1]));
          if (j < degree)
            value = op.add(
                value, op.mul(op.mul(lo_factor,
                                     op.div(op.sub(op.from(knots[index + degree + 1]), a), den)),
                              previous[(row + 1) * width + j]));
          if (j > 0)
            value = op.add(
                value, op.mul(op.mul(hi_factor,
                                     op.div(op.sub(op.from(knots[index + degree + 1]), b), den)),
                              previous[(row + 1) * width + j - 1]));
        }
        next[row * width + j] = std::move(value);
      }
    }
    previous.swap(next);
  }
  const auto controls = source.view(curve.controls);
  const auto stride = curve.dimension + 1;
  output.reserve(width * stride);
  for (uint32_t j = 0; j <= p; ++j)
    for (uint32_t axis = 0; axis < stride; ++axis) {
      auto value = op.zero();
      for (uint32_t i = 0; i <= p; ++i) {
        op.budget.charge(1);
        value = op.add(value, op.mul(previous[i * width + j],
                                     op.from(controls[(span - p + i) * stride + axis])));
      }
      output.push_back(std::move(value));
    }
  return output;
}
std::pmr::vector<double> expanded(const SourceData &source, const Curve &curve, bgfd::Budget &b) {
  std::pmr::vector<double> result(&b);
  if (curve.kind == 4) {
    result.reserve(static_cast<size_t>(curve.degree + 1) * 2);
    for (uint32_t i = 0; i <= curve.degree; ++i) {
      b.charge(1);
      result.push_back(0.);
    }
    for (uint32_t i = 0; i <= curve.degree; ++i) {
      b.charge(1);
      result.push_back(1.);
    }
    return result;
  }
  const auto knots = source.view(curve.knots);
  const auto mult = source.view(curve.multiplicities);
  uint64_t count = 0;
  for (const auto m : mult) {
    b.charge(1);
    count += m;
    if (count > UINT32_MAX)
      throw Error(ErrorCode::resource_exhausted, "curve expanded knot capacity");
  }
  result.reserve(static_cast<size_t>(count));
  for (size_t i = 0; i < knots.size(); ++i)
    for (uint32_t j = 0; j < mult[i]; ++j) {
      b.charge(1);
      result.push_back(knots[i]);
    }
  return result;
}
bool finite_positive(std::span<const Interval> net, uint32_t dimension) {
  for (const auto x : net)
    if (!numeric::valid(x))
      return false;
  for (size_t i = dimension; i < net.size(); i += dimension + 1)
    if (net[i].lower <= 0.)
      return false;
  return true;
}
void freeze_net(CurveSourceCacheData &data, CurveRootDescriptor &root,
                std::span<const Interval> net, const CurveCacheLimits &limits, bgfd::Budget &b) {
  if (data.controls.size() > limits.control_scalars ||
      net.size() > limits.control_scalars - data.controls.size())
    throw Error(ErrorCode::resource_exhausted, "curve control capacity");
  root.controls = {static_cast<uint32_t>(data.controls.size()), static_cast<uint32_t>(net.size())};
  b.charge(net.size());
  data.controls.insert(data.controls.end(), net.begin(), net.end());
}
double analytic_second(const Curve &curve) {
  if (curve.kind == 1)
    return 0.;
  Interval axes[6]{};
  double scale = 0.;
  for (uint32_t i = 0; i < 2; ++i)
    for (uint32_t a = 0; a < curve.dimension; ++a) {
      axes[i * 3 + a] =
          numeric::mul(numeric::point(curve.radii[i]), numeric::point(curve.frame[3 + 3 * i + a]));
      scale = numeric::maximum(scale, numeric::magnitude(axes[i * 3 + a]));
    }
  if (scale == 0.)
    return 0.;
  if (!numeric::finite(scale))
    throw Error(ErrorCode::unresolved, "curve frame scale enclosure");
  double largest = 0.;
  for (uint32_t i = 0; i < 2; ++i) {
    Interval row{};
    for (uint32_t j = 0; j < 2; ++j) {
      Interval dot{};
      for (uint32_t a = 0; a < curve.dimension; ++a)
        dot = numeric::add(dot, numeric::mul(numeric::div(axes[i * 3 + a], numeric::point(scale)),
                                             numeric::div(axes[j * 3 + a], numeric::point(scale))));
      row = numeric::add(row, numeric::point(numeric::magnitude(dot)));
    }
    largest = numeric::maximum(largest, row.upper);
  }
  const auto result = numeric::up(numeric::rn_mul(scale, numeric::up(numeric::rn_sqrt(largest))));
  if (!numeric::finite(result))
    throw Error(ErrorCode::unresolved, "curve Gram enclosure");
  return result;
}
// Sufficient original homogeneous collinearity + monotone scalar-control order.
// All arithmetic is exact and executed once per requested original source.
std::optional<uint32_t> original_line(const SourceData &source, const Curve &curve,
                                      bgfd::Budget &b) {
  if (curve.kind == 1) {
    for (uint32_t a = 0; a < curve.dimension; ++a)
      if (curve.frame[3 + a] != 0.)
        return a;
    return std::nullopt;
  }
  if (curve.kind <= 3)
    return std::nullopt;
  const auto net = source.view(curve.controls);
  const auto stride = curve.dimension + 1;
  const auto count = net.size() / stride;
  auto cart = [&](size_t i, uint32_t axis) {
    return Rational::from_double(b, net[i * stride + axis])
        .divide(Rational::from_double(b, net[i * stride + curve.dimension]));
  };
  std::array<Rational, 3> origin{Rational(b), Rational(b), Rational(b)},
      direction{Rational(b), Rational(b), Rational(b)};
  for (uint32_t a = 0; a < curve.dimension; ++a)
    origin[a] = cart(0, a);
  size_t last = 1;
  uint32_t axis = 0;
  for (; last < count; ++last) {
    bool changed = false;
    for (uint32_t a = 0; a < curve.dimension; ++a) {
      direction[a] = cart(last, a).subtract(origin[a]);
      if (direction[a].sign() != 0) {
        axis = a;
        changed = true;
      }
    }
    if (changed)
      break;
  }
  if (last == count)
    return std::nullopt;
  auto prior = origin[axis];
  const auto sign = direction[axis].sign();
  for (size_t i = 1; i < count; ++i) {
    b.charge(1);
    const auto scalar = cart(i, axis);
    if (scalar.compare(prior) * sign < 0)
      return std::nullopt;
    for (uint32_t a = 0; a < curve.dimension; ++a)
      if (cart(i, a)
              .subtract(origin[a])
              .multiply(direction[axis])
              .compare(scalar.subtract(origin[axis]).multiply(direction[a])) != 0)
        return std::nullopt;
    prior = scalar;
  }
  return axis;
}
} // namespace
CurveSourceCache prepare_curve_source_cache(const SourceStore &source,
                                            std::span<const CurveRequest> requests,
                                            const std::shared_ptr<RunResource> &resource,
                                            NumericProfile profile, CurveCacheLimits limits,
                                            bgfd::Budget &b) {
  admit_numeric_environment();
  check(resource && resource == source.view().resource, "curve cache source resource identity");
  check(profile.revision == 1 && limits.roots > 0 && limits.control_scalars > 0 && limits.work > 0,
        "curve cache profile/limits");
  const auto initial_work = b.work();
  auto data = std::allocate_shared<CurveSourceCacheData>(
      RunAllocator<CurveSourceCacheData>(resource), resource, source, profile);
  for (const auto &request : requests) {
    b.charge(1);
    check(request.source.value < source.view().curves.size() &&
              numeric::finite(request.domain[0]) && numeric::finite(request.domain[1]) &&
              request.domain[0] < request.domain[1],
          "invalid curve source request");
  }
  std::pmr::vector<CurveRequest> ordered(requests.begin(), requests.end(), &b);
  std::sort(ordered.begin(), ordered.end(), [&](const auto &a, const auto &c) {
    b.charge(1);
    return std::tie(a.source.value, a.domain) < std::tie(c.source.value, c.domain);
  });
  std::pmr::vector<CurveRequest> merged(&b);
  for (const auto &request : ordered) {
    check(request.source.value < source.view().curves.size() &&
              numeric::finite(request.domain[0]) && numeric::finite(request.domain[1]) &&
              request.domain[0] < request.domain[1],
          "invalid curve source request");
    if (!merged.empty() && merged.back().source == request.source &&
        request.domain[0] <= merged.back().domain[1])
      merged.back().domain[1] = std::max(merged.back().domain[1], request.domain[1]);
    else
      merged.push_back(request);
  }
  ordered = std::move(merged);
  for (size_t first = 0; first < ordered.size();) {
    const auto slot = ordered[first].source;
    check(slot.value < source.view().curves.size(), "foreign original curve request");
    size_t last = first + 1;
    while (last < ordered.size() && ordered[last].source == slot)
      ++last;
    const auto &curve = source.view().curves[slot.value];
    for (size_t j = first; j < last; ++j) {
      check(numeric::finite(ordered[j].domain[0]) && numeric::finite(ordered[j].domain[1]) &&
                ordered[j].domain[0] < ordered[j].domain[1],
            "invalid curve request interval");
      if (j > first)
        check(ordered[j].domain != ordered[j - 1].domain, "duplicate original curve request");
    }
    const auto line = original_line(source.view(), curve, b);
    auto knots = expanded(source.view(), curve, b);
    const auto count =
        curve.kind > 3 ? source.view().view(curve.controls).size() / (curve.dimension + 1) : 0;
    const auto begin = curve.kind > 3 ? knots[curve.degree] : curve.parameter_range[0];
    const auto end = curve.kind > 3 ? knots[count] : curve.parameter_range[1];
    for (size_t j = first; j < last; ++j)
      check(ordered[j].domain[0] >= begin && ordered[j].domain[1] <= end,
            "curve request outside original domain");
    const auto span_begin = curve.kind > 3 ? curve.degree : 0U;
    const auto span_end = curve.kind > 3 ? static_cast<uint32_t>(count) : 1U;
    for (uint32_t k = span_begin; k < span_end; ++k) {
      b.charge(1);
      const double lo = curve.kind > 3 ? knots[k] : begin, hi = curve.kind > 3 ? knots[k + 1] : end;
      if (lo >= hi)
        continue;
      bool touches = false;
      for (size_t j = first; j < last; ++j)
        touches = touches || (hi >= ordered[j].domain[0] && lo <= ordered[j].domain[1]);
      if (!touches)
        continue;
      if (data->original_roots.size() >= limits.roots)
        throw Error(ErrorCode::resource_exhausted, "original curve root capacity");
      CurveRootDescriptor root;
      root.family = curve.kind == 1   ? CurveFamily::line
                    : curve.kind == 2 ? CurveFamily::circle
                    : curve.kind == 3 ? CurveFamily::ellipse
                                      : CurveFamily::rational;
      root.dimension = curve.dimension;
      root.degree = curve.kind > 3 ? curve.degree : 0;
      root.original = {static_cast<uint32_t>(data->original_roots.size())};
      root.source_slot = slot.value;
      root.source_id = curve.id.value;
      root.original_span = k;
      root.original_t[0] = root.prepared_t[0] = numeric::point(lo);
      root.original_t[1] = root.prepared_t[1] = numeric::point(hi);
      std::copy(curve.frame.begin(), curve.frame.end(), root.frame);
      std::copy(curve.radii.begin(), curve.radii.end(), root.radii);
      root.line_profile = line ? 1U : 0U;
      root.line_axis = line.value_or(0);
      if (curve.kind > 3) {
        auto net = extract_span(source.view(), curve, knots, k, IntervalArithmetic{b});
        if (!finite_positive(net, curve.dimension)) {
          const auto precise = extract_span(source.view(), curve, knots, k, ExactArithmetic{b});
          for (size_t i = 0; i < net.size(); ++i) {
            const auto value = precise[i].bounds();
            net[i] = {value.lower, value.upper};
          }
          if (!finite_positive(net, curve.dimension))
            throw Error(ErrorCode::unresolved, "original curve extraction enclosure");
        }
        freeze_net(*data, root, net, limits, b);
      } else
        root.second_derivative_upper = numeric::point(analytic_second(curve));
      // Full multiplicity can encode a genuine jump; preserve only exact C0.
      if (!data->original_roots.empty()) {
        const auto &previous = data->original_roots.back();
        if (curve.kind > 3 && previous.source_slot == slot.value &&
            previous.original_t[1].lower == lo) {
          uint32_t multiplicity = 1;
          for (uint32_t j = k; j > 0 && knots[j - 1] == lo; --j)
            ++multiplicity;
          for (size_t j = static_cast<size_t>(k) + 1; j < knots.size() && knots[j] == lo; ++j)
            ++multiplicity;
          if (multiplicity > curve.degree) {
            const auto raw = source.view().view(curve.controls);
            const auto stride = curve.dimension + 1;
            const auto left = previous.original_span, right = k - curve.degree;
            for (uint32_t a = 0; a < curve.dimension; ++a)
              if (Rational::from_double(b, raw[left * stride + a])
                      .multiply(Rational::from_double(b, raw[right * stride + curve.dimension]))
                      .compare(Rational::from_double(b, raw[right * stride + a])
                                   .multiply(Rational::from_double(
                                       b, raw[left * stride + curve.dimension]))) != 0)
                throw Error(ErrorCode::unsupported, "discontinuous original curve junction");
          }
        }
      }
      data->original_roots.push_back(root);
      for (size_t j = first; j < last; ++j) {
        const auto a = std::max(lo, ordered[j].domain[0]), c = std::min(hi, ordered[j].domain[1]);
        if (a >= c)
          continue;
        if (data->roots.size() >= limits.roots)
          throw Error(ErrorCode::resource_exhausted, "prepared curve root capacity");
        auto prepared = root;
        prepared.prepared_t[0] = numeric::point(a);
        prepared.prepared_t[1] = numeric::point(c);
        if (curve.kind > 3 && (a != lo || c != hi)) {
          // Restriction is performed through the same checked service after an
          // immutable temporary lease; no second source extraction.
          const CurveSourceCache temporary(data);
          data->roots.push_back(root);
          const auto temporary_slot = CurveRootSlot{static_cast<uint32_t>(data->roots.size() - 1)};
          const std::array interval{Rational::from_double(b, a), Rational::from_double(b, c)};
          auto restricted = prepare_curve_interval_controls(temporary, temporary_slot, interval, b);
          data->roots.pop_back();
          freeze_net(*data, prepared, restricted, limits, b);
        }
        data->roots.push_back(prepared);
      }
      if (b.work() - initial_work > limits.work)
        throw Error(ErrorCode::resource_exhausted, "curve source preparation work");
    }
    first = last;
  }
  return CurveSourceCache(std::move(data));
}
OriginalCurveRootSlot find_original_curve_root(const CurveSourceCache &cache, Slot<CurveTag> source,
                                               const Rational &t) {
  const auto roots = cache.original_roots();
  size_t first = 0, last = roots.size();
  while (first < last) {
    t.budget().charge(1);
    const auto mid = first + (last - first) / 2;
    if (roots[mid].source_slot < source.value ||
        (roots[mid].source_slot == source.value &&
         Rational::from_double(t.budget(), roots[mid].original_t[1].lower).compare(t) < 0))
      first = mid + 1;
    else
      last = mid;
  }
  check(first < roots.size() && roots[first].source_slot == source.value &&
            Rational::from_double(t.budget(), roots[first].original_t[0].lower).compare(t) <= 0,
        "curve point outside compiled original roots");
  return {static_cast<uint32_t>(first)};
}
CurveRootSlot find_prepared_curve_root(const CurveSourceCache &cache, Slot<CurveTag> source,
                                       std::span<const Rational, 2> interval) {
  check(&interval[0].budget() == &interval[1].budget() && interval[0].compare(interval[1]) < 0,
        "invalid prepared curve interval");
  const auto roots = cache.roots();
  size_t first = 0, last = roots.size();
  while (first < last) {
    interval[0].budget().charge(1);
    const auto mid = first + (last - first) / 2;
    if (roots[mid].source_slot < source.value ||
        (roots[mid].source_slot == source.value &&
         Rational::from_double(interval[0].budget(), roots[mid].prepared_t[1].lower)
                 .compare(interval[1]) < 0))
      first = mid + 1;
    else
      last = mid;
  }
  check(first < roots.size() && roots[first].source_slot == source.value &&
            Rational::from_double(interval[0].budget(), roots[first].prepared_t[0].lower)
                    .compare(interval[0]) <= 0,
        "no single prepared source span covers curve interval");
  return {static_cast<uint32_t>(first)};
}
namespace curve_detail {
BezierCurve original_net_exact(const CurveSourceCache &cache, OriginalCurveRootSlot slot,
                               bgfd::Budget &b) {
  check(slot.value < cache.original_roots().size(), "foreign exact curve root");
  const auto &root = cache.original_roots()[slot.value];
  const auto &curve = cache.source().view().curves[root.source_slot];
  check(root.family == CurveFamily::rational, "exact homogeneous net requires rational original");
  const auto knots = expanded(cache.source().view(), curve, b);
  auto controls =
      extract_span(cache.source().view(), curve, knots, root.original_span, ExactArithmetic{b});
  BezierCurve result(b, {root.dimension, root.degree},
                     {Rational::from_double(b, root.original_t[0].lower),
                      Rational::from_double(b, root.original_t[1].lower)});
  result.controls = std::move(controls);
  return result;
}
} // namespace curve_detail
} // namespace bgmesh::adaptive
