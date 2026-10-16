#include "adaptive_geometry.hpp"
#include "adaptive_numeric.hpp"
#include "source_cache.hpp"
#include <algorithm>
#include <array>
#include <limits>
#include <map>
#include <tuple>

namespace bgmesh::adaptive {
SourceCacheData::SourceCacheData(std::shared_ptr<RunResource> owner, SourceStore input,
                                 NumericProfile selected)
    : resource(std::move(owner)), source(std::move(input)), profile(selected),
      roots(resource.get()), original_roots(resource.get()), controls(resource.get()) {}
std::span<const Interval> SourceCache::controls(ArithmeticStage stage) const {
  if (stage != ArithmeticStage::rn64)
    throw Error(ErrorCode::unsupported, "requested root stage is not resident");
  return data_->controls;
}
namespace {
using numeric::add;
using numeric::div;
using numeric::mul;
using numeric::point;
using numeric::sub;
using Polynomial = std::array<Interval, 4>;
struct AxisSpan {
  uint32_t interval, first_control;
  double begin, end;
  std::array<Polynomial, 4> basis;
};
void check(bool ok, const char *text) {
  if (!ok)
    throw Error(ErrorCode::invalid_source, text);
}
void charge(bgfd::Budget &budget, uint64_t &remaining, uint64_t amount = 1) {
  if (amount > remaining)
    throw Error(ErrorCode::resource_exhausted, "source cache work limit");
  remaining -= amount;
  budget.charge(amount);
}
std::pmr::vector<double> expanded_axis(const SourceData &source, Range<double> knots,
                                       Range<uint32_t> mult, std::pmr::memory_resource *resource,
                                       bgfd::Budget &budget, uint64_t &work) {
  const auto axis = source.view(knots);
  const auto multiplicities = source.view(mult);
  check(axis.size() == multiplicities.size(), "original knot/multiplicity shape");
  std::pmr::vector<double> result(resource);
  uint64_t count = 0;
  for (const auto m : multiplicities) {
    charge(budget, work);
    count += m;
    if (count > UINT32_MAX)
      throw Error(ErrorCode::resource_exhausted, "expanded knot capacity");
  }
  result.reserve(static_cast<size_t>(count));
  for (size_t i = 0; i < axis.size(); ++i)
    for (uint32_t j = 0; j < multiplicities[i]; ++j) {
      charge(budget, work);
      result.push_back(axis[i]);
    }
  return result;
}
Polynomial basis_power(std::span<const double> knots, uint32_t span, uint32_t i, uint32_t d,
                       bgfd::Budget &budget, uint64_t &work) {
  charge(budget, work);
  Polynomial result{};
  if (d == 0) {
    if (i == span)
      result[0] = point(1.);
    return result;
  }
  const auto a = knots[span];
  const auto h = sub(point(knots[span + 1]), point(a));
  const auto leftden = sub(point(knots[i + d]), point(knots[i]));
  if (knots[i + d] != knots[i]) {
    const auto prior = basis_power(knots, span, i, d - 1, budget, work);
    const auto c = div(sub(point(a), point(knots[i])), leftden), x = div(h, leftden);
    for (uint32_t k = 0; k < d; ++k) {
      result[k] = add(result[k], mul(c, prior[k]));
      result[k + 1] = add(result[k + 1], mul(x, prior[k]));
    }
  }
  const auto rightden = sub(point(knots[i + d + 1]), point(knots[i + 1]));
  if (knots[i + d + 1] != knots[i + 1]) {
    const auto prior = basis_power(knots, span, i + 1, d - 1, budget, work);
    const auto c = div(sub(point(knots[i + d + 1]), point(a)), rightden),
               x = numeric::neg(div(h, rightden));
    for (uint32_t k = 0; k < d; ++k) {
      result[k] = add(result[k], mul(c, prior[k]));
      result[k + 1] = add(result[k + 1], mul(x, prior[k]));
    }
  }
  return result;
}
std::pmr::vector<AxisSpan> axis_spans(std::span<const double> knots, uint32_t degree,
                                      uint32_t count, double lo, double hi,
                                      std::pmr::memory_resource *resource, bgfd::Budget &budget,
                                      uint64_t &work) {
  if (degree > 3)
    throw Error(ErrorCode::unsupported, "initial cached rational profile degree exceeds three");
  check(count > degree && knots.size() == static_cast<size_t>(count) + degree + 1,
        "original NURBS axis shape");
  if (lo < knots[degree] || hi > knots[count])
    throw Error(ErrorCode::unsupported, "source query outside original natural knot domain");
  std::pmr::vector<AxisSpan> result(resource);
  for (uint32_t k = degree; k < count; ++k) {
    charge(budget, work);
    if (knots[k] >= knots[k + 1] || knots[k + 1] < lo || knots[k] > hi)
      continue;
    AxisSpan row{k, k - degree, knots[k], knots[k + 1], {}};
    for (uint32_t i = 0; i <= degree; ++i) {
      const auto power = basis_power(knots, k, k - degree + i, degree, budget, work);
      for (uint32_t b = 0; b <= degree; ++b)
        for (uint32_t j = 0; j <= b; ++j) {
          const auto factor = div(point(static_cast<double>(numeric::binomial(b, j))),
                                  point(static_cast<double>(numeric::binomial(degree, j))));
          row.basis[i][b] = add(row.basis[i][b], mul(power[j], factor));
        }
    }
    result.push_back(row);
  }
  // Multiplicity<=degree establishes C0 from the original B-spline definition.
  // A fully disconnected knot would require a separate exact junction check.
  for (uint32_t k = degree + 1; k < count; ++k)
    if (knots[k] > lo && knots[k] < hi) {
      uint32_t m = 1;
      for (uint32_t j = k + 1; j < knots.size() && knots[j] == knots[k]; ++j)
        ++m;
      if (m > degree)
        throw Error(ErrorCode::unsupported, "disconnected original NURBS knot junction");
    }
  return result;
}
Interval restrict_coefficient(const Interval *input, uint32_t degree, uint32_t output, Interval lo,
                              Interval hi) {
  // Blossom of a degree<=3 curve with degree-output copies of lo, output hi.
  Interval row[4]{};
  for (uint32_t i = 0; i <= degree; ++i)
    row[i] = input[i];
  for (uint32_t level = 0; level < degree; ++level) {
    const auto t = level < degree - output ? lo : hi;
    for (uint32_t i = 0; i < degree - level; ++i)
      row[i] = numeric::lerp(row[i], row[i + 1], t);
  }
  return row[0];
}
std::array<Interval, 64> restrict_net(const RootDescriptor &root, const Interval *input,
                                      const std::array<double, 4> &box) {
  std::array<Interval, 64> temporary{}, result{};
  const auto ulo = div(sub(point(box[0]), root.original_box[0]),
                       sub(root.original_box[1], root.original_box[0]));
  const auto uhi = div(sub(point(box[1]), root.original_box[0]),
                       sub(root.original_box[1], root.original_box[0]));
  const auto vlo = div(sub(point(box[2]), root.original_box[2]),
                       sub(root.original_box[3], root.original_box[2]));
  const auto vhi = div(sub(point(box[3]), root.original_box[2]),
                       sub(root.original_box[3], root.original_box[2]));
  for (uint32_t j = 0; j <= root.degree_v; ++j)
    for (uint32_t a = 0; a < 4; ++a) {
      Interval row[4]{};
      for (uint32_t i = 0; i <= root.degree_u; ++i)
        row[i] = input[(i * (root.degree_v + 1) + j) * 4 + a];
      for (uint32_t i = 0; i <= root.degree_u; ++i)
        temporary[(i * (root.degree_v + 1) + j) * 4 + a] =
            restrict_coefficient(row, root.degree_u, i, ulo, uhi);
    }
  for (uint32_t i = 0; i <= root.degree_u; ++i)
    for (uint32_t a = 0; a < 4; ++a) {
      Interval row[4]{};
      for (uint32_t j = 0; j <= root.degree_v; ++j)
        row[j] = temporary[(i * (root.degree_v + 1) + j) * 4 + a];
      for (uint32_t j = 0; j <= root.degree_v; ++j)
        result[(i * (root.degree_v + 1) + j) * 4 + a] =
            restrict_coefficient(row, root.degree_v, j, vlo, vhi);
    }
  return result;
}
double gram_upper(const Frame &frame, uint32_t columns) {
  double scale = 0.;
  for (uint32_t i = 0; i < columns * 3; ++i)
    scale = numeric::maximum(scale, numeric::absolute(frame[3 + i]));
  if (scale == 0.)
    return 0.;
  double largest = 0.;
  for (uint32_t i = 0; i < columns; ++i) {
    Interval row{};
    for (uint32_t j = 0; j < columns; ++j) {
      Interval dot{};
      for (uint32_t a = 0; a < 3; ++a)
        dot = add(dot, mul(div(point(frame[3 + 3 * i + a]), point(scale)),
                           div(point(frame[3 + 3 * j + a]), point(scale))));
      row = add(row, point(numeric::magnitude(dot)));
    }
    largest = numeric::maximum(largest, row.upper);
  }
  const auto value = numeric::up(numeric::rn_mul(scale, numeric::up(numeric::rn_sqrt(largest))));
  if (!numeric::finite(value))
    throw Error(ErrorCode::unresolved, "original frame Gram enclosure");
  return value;
}
void admit_analytic(const Carrier &carrier, const std::array<double, 4> &box,
                    bgfd::Budget &budget,SourceCachePurpose purpose) {
  std::array<Rational, 9> axes{Rational(budget), Rational(budget), Rational(budget),
                               Rational(budget), Rational(budget), Rational(budget),
                               Rational(budget), Rational(budget), Rational(budget)};
  for (size_t i = 0; i < 9; ++i)
    axes[i] = Rational::from_double(budget, carrier.frame[3 + i]);
  std::array<Rational, 3> cross{Rational(budget), Rational(budget), Rational(budget)};
  bool nonzero = false;
  for (size_t a = 0; a < 3; ++a) {
    const auto b = (a + 1) % 3, c = (a + 2) % 3;
    cross[a] = axes[b].multiply(axes[3 + c]).subtract(axes[c].multiply(axes[3 + b]));
    nonzero = nonzero || cross[a].sign() != 0;
  }
  if (!nonzero)
    throw Error(ErrorCode::unsupported, "rank-deficient original analytic XY frame");
  if (carrier.kind == 1)
    return;
  auto det = Rational(budget);
  for (size_t a = 0; a < 3; ++a)
    det = det.add(cross[a].multiply(axes[6 + a]));
  if (det.sign() == 0)
    throw Error(ErrorCode::unsupported, "rank-deficient original analytic frame");
  if (carrier.radii[0] == 0.)
    throw Error(ErrorCode::unsupported, "zero original analytic radius");
  const auto pi_lower = numeric::rn_mul(numeric::half_pi().lower, 2.);
  if (purpose==SourceCachePurpose::regular_patch && numeric::sub(point(box[1]), point(box[0])).upper >= numeric::rn_mul(pi_lower, 2.))
    throw Error(ErrorCode::unsupported, "wrapped angular patch outside initial profile");
  if (carrier.kind == 3) {
    const auto angle = numeric::sincos(point(carrier.radii[1]));
    if (!angle.ready || (angle.cosine.lower <= 0. && angle.cosine.upper >= 0.))
      throw Error(ErrorCode::unresolved, "cone height direction not resolved");
    const auto radius =
        numeric::add(point(carrier.radii[0]), numeric::mul({box[2], box[3]}, angle.sine));
    if (radius.lower <= 0. && radius.upper >= 0.)
      throw Error(ErrorCode::unsupported, "cone apex lies in requested rectangle");
  }
  if (carrier.kind == 4) {
    const auto left = numeric::sincos(point(box[2])), right = numeric::sincos(point(box[3]));
    const bool sign = left.ready && right.ready &&
                      ((left.cosine.lower > 0. && right.cosine.lower > 0.) ||
                       (left.cosine.upper < 0. && right.cosine.upper < 0.));
    if (!sign || numeric::sub(point(box[3]), point(box[2])).upper >= pi_lower)
      throw Error(ErrorCode::unsupported, "sphere pole or unresolved regular chart");
  }
  if (carrier.kind == 5) {
    if (carrier.radii[1] == 0. ||
        numeric::absolute(carrier.radii[0]) <= numeric::absolute(carrier.radii[1]))
      throw Error(ErrorCode::unsupported,
                  "initial torus profile requires nonzero minor and separated radial factor");
    if (purpose==SourceCachePurpose::regular_patch && numeric::sub(point(box[3]), point(box[2])).upper >= numeric::rn_mul(pi_lower, 2.))
      throw Error(ErrorCode::unsupported, "wrapped torus chart outside initial profile");
  }
}

struct SourceSpanPair {
  const AxisSpan &u;
  const AxisSpan &v;
};
uint32_t affine_separable(const Carrier &carrier, std::span<const double> controls,
                          SourceSpanPair spans, bgfd::Budget &budget) {
  const auto &u = spans.u;
  const auto &v = spans.v;
  uint32_t mask = 0;
  for (uint32_t direction = 0; direction < 2; ++direction) {
    const auto degree = direction == 0 ? carrier.degrees[0] : carrier.degrees[1];
    if (degree != 1)
      continue;
    const auto count = direction == 0 ? carrier.degrees[1] + 1 : carrier.degrees[0] + 1;
    const auto baseline = (u.first_control * carrier.control_shape[1] + v.first_control) * 4;
    const auto step = direction == 0 ? carrier.control_shape[1] * 4 : 4;
    const auto w0 = Rational::from_double(budget, controls[baseline + 3]);
    std::array<Rational, 3> delta{Rational(budget), Rational(budget), Rational(budget)};
    for (uint32_t a = 0; a < 3; ++a)
      delta[a] = Rational::from_double(budget, controls[baseline + step + a])
                     .subtract(Rational::from_double(budget, controls[baseline + a]));
    bool equal = true;
    for (uint32_t i = 0; i < count && equal; ++i) {
      budget.charge(1);
      const auto index = baseline + (direction == 0 ? i * 4 : i * carrier.control_shape[1] * 4);
      if (controls[index + 3] != controls[index + step + 3]) {
        equal = false;
        break;
      }
      const auto w = Rational::from_double(budget, controls[index + 3]);
      for (uint32_t a = 0; a < 3; ++a) {
        const auto d = Rational::from_double(budget, controls[index + step + a])
                           .subtract(Rational::from_double(budget, controls[index + a]));
        if (d.multiply(w0).compare(delta[a].multiply(w)) != 0) {
          equal = false;
          break;
        }
      }
    }
    if (equal)
      mask |= uint32_t{1} << direction;
  }
  return mask;
}
void append_controls(SourceCacheData &data, RootDescriptor &root,
                     std::span<const Interval> controls, const SourceCacheLimits &limits,
                     bgfd::Budget &budget, uint64_t &work) {
  if (controls.size() > limits.control_scalars - data.controls.size())
    throw Error(ErrorCode::resource_exhausted, "source coefficient capacity");
  charge(budget, work, controls.size());
  root.controls = {static_cast<uint32_t>(data.controls.size()),
                   static_cast<uint32_t>(controls.size())};
  data.controls.insert(data.controls.end(), controls.begin(), controls.end());
}

void prepare_point_owners(SourceCacheData &data, bgfd::Budget &budget, uint64_t &remaining) {
  using Key = std::tuple<uint32_t, uint64_t, uint64_t>;
  struct Compare {
    bgfd::Budget *budget;
    uint64_t *remaining;
    bool operator()(const Key &a, const Key &b) const {
      charge(*budget, *remaining);
      return a < b;
    }
  };
  using Index = std::pmr::map<Key, OriginalRootSlot, Compare>;
  Index corners(Compare{&budget, &remaining}, data.resource.get());
  Index u_edges(Compare{&budget, &remaining}, data.resource.get());
  Index v_edges(Compare{&budget, &remaining}, data.resource.get());
  auto word = [](double x) { return numeric::bits(x == 0. ? 0. : x); };
  for (const auto &root : data.original_roots) {
    for (uint32_t u = 0; u < 2; ++u)
      for (uint32_t v = 0; v < 2; ++v)
        corners.try_emplace(Key{root.source_slot, word(root.original_box[u].lower),
                                word(root.original_box[2 + v].lower)},
                            root.original);
    for (uint32_t i = 0; i < 2; ++i) {
      u_edges.try_emplace(Key{root.source_slot, word(root.original_box[i].lower), root.span_v},
                          root.original);
      v_edges.try_emplace(Key{root.source_slot, root.span_u, word(root.original_box[2 + i].lower)},
                          root.original);
    }
  }
  for (auto &root : data.roots)
    for (uint32_t u = 0; u < 3; ++u)
      for (uint32_t v = 0; v < 3; ++v) {
        auto owner = root.original;
        if (u != 1 && v != 1) {
          const auto found =
              corners.find(Key{root.source_slot, word(root.prepared_box[u == 0 ? 0 : 1].lower),
                               word(root.prepared_box[v == 0 ? 2 : 3].lower)});
          if (found != corners.end())
            owner = found->second;
        } else if (u != 1) {
          const auto found = u_edges.find(
              Key{root.source_slot, word(root.prepared_box[u == 0 ? 0 : 1].lower), root.span_v});
          if (found != u_edges.end())
            owner = found->second;
        } else if (v != 1) {
          const auto found = v_edges.find(
              Key{root.source_slot, root.span_u, word(root.prepared_box[v == 0 ? 2 : 3].lower)});
          if (found != v_edges.end())
            owner = found->second;
        }
        root.sample_owners[u + 3 * v] = owner;
      }
}
} // namespace
std::shared_ptr<const SourceCacheData> prepare_source_cache_data(const SourceStore &source,
                                 std::span<const CarrierRequest> requests,
                                 const std::shared_ptr<RunResource> &resource,
                                 NumericProfile profile, SourceCacheLimits limits,
                                  bgfd::Budget &budget,SourceCachePurpose purpose) {
  admit_numeric_environment();
  const auto entry_work = budget.work();
  const auto check_work = [&] {
    if (budget.work() - entry_work > limits.work)
      throw Error(ErrorCode::resource_exhausted, "source preparation actual work limit");
  };
  check(resource != nullptr && source.view().resource == resource,
        "foreign adaptive source resource");
  if (profile.revision != 1)
    throw Error(ErrorCode::unsupported, "unsupported adaptive numeric profile");
  auto data = std::allocate_shared<SourceCacheData>(RunAllocator<SourceCacheData>(resource),
                                                    resource, source, profile);
  data->purpose=purpose;
  auto remaining = limits.work;
  std::pmr::vector<CarrierRequest> ordered(requests.begin(), requests.end(), resource.get());
  std::sort(ordered.begin(), ordered.end(), [&](const auto &a, const auto &b) {
    charge(budget, remaining);
    return a.source.value < b.source.value;
  });
  for (size_t r = 0; r < ordered.size(); ++r) {
    charge(budget, remaining);
    const auto &request = ordered[r];
    check(request.source.value < source.view().carriers.size(), "foreign carrier slot");
    check(r == 0 || ordered[r - 1].source.value != request.source.value,
          "duplicate carrier request");
    const auto &carrier = source.view().carriers[request.source.value];
    const auto &box = request.domain;
    check(std::all_of(box.begin(), box.end(), [](double x) { return numeric::finite(x); }) &&
              box[0] < box[1] && box[2] < box[3],
          "regular finite source rectangle required");
    RootDescriptor base;
    base.source_slot = request.source.value;
    base.source_id = carrier.id.value;
    std::copy(carrier.frame.begin(), carrier.frame.end(), base.frame);
    std::copy(carrier.radii.begin(), carrier.radii.end(), base.radii);
    check(carrier.kind >= 1 && carrier.kind <= 6, "unknown original carrier family");
    base.family = static_cast<GeometryFamily>(carrier.kind - 1);
    if (carrier.kind < 6) {
      admit_analytic(carrier, box, budget,purpose);
      check_work();
      base.frame_a = point(gram_upper(carrier.frame, 2));
      base.frame_f = (carrier.kind == 4 || carrier.kind == 5) ? point(gram_upper(carrier.frame, 3))
                                                              : base.frame_a;
      if (carrier.kind == 3) {
        const auto angle = numeric::sincos(point(carrier.radii[1]));
        if (!angle.ready)
          throw Error(ErrorCode::unresolved, "cone source angle outside RN profile");
        base.cone_sin = angle.sine;
        base.cone_cos = angle.cosine;
      }
      if (carrier.kind == 1)
        base.affine_separable_axes = 3;
      if (carrier.kind == 2 || (carrier.kind == 3 && carrier.radii[1] == 0.))
        base.affine_separable_axes = 2;
      if (data->roots.size() >= limits.roots)
        throw Error(ErrorCode::resource_exhausted, "source root capacity");
      if (data->original_roots.size() >= limits.roots)
        throw Error(ErrorCode::resource_exhausted, "source evaluator root capacity");
      base.original = {static_cast<uint32_t>(data->original_roots.size())};
      for (uint32_t a = 0; a < 4; ++a)
        base.original_box[a] = base.prepared_box[a] = point(box[a]);
      data->original_roots.push_back(base);
      data->roots.push_back(base);
      continue;
    }
    const auto ku = expanded_axis(source.view(), carrier.knot_u, carrier.mult_u, resource.get(),
                                  budget, remaining);
    const auto kv = expanded_axis(source.view(), carrier.knot_v, carrier.mult_v, resource.get(),
                                  budget, remaining);
    const auto us = axis_spans(ku, carrier.degrees[0], carrier.control_shape[0], box[0], box[1],
                               resource.get(), budget, remaining);
    const auto vs = axis_spans(kv, carrier.degrees[1], carrier.control_shape[1], box[2], box[3],
                               resource.get(), budget, remaining);
    const auto original = source.view().view(carrier.controls);
    check(original.size() ==
              static_cast<size_t>(carrier.control_shape[0]) * carrier.control_shape[1] * 4,
          "original tensor control shape");
    for (const auto &u : us)
      for (const auto &v : vs) {
        if (data->roots.size() >= limits.roots)
          throw Error(ErrorCode::resource_exhausted, "source root capacity");
        auto root = base;
        root.degree_u = carrier.degrees[0];
        root.degree_v = carrier.degrees[1];
        root.span_u = u.interval;
        root.span_v = v.interval;
        root.affine_separable_axes = affine_separable(carrier, original, {u, v}, budget);
        check_work();
        if (data->original_roots.size() >= limits.roots)
          throw Error(ErrorCode::resource_exhausted, "source evaluator root capacity");
        root.original = {static_cast<uint32_t>(data->original_roots.size())};
        const std::array natural{u.begin, u.end, v.begin, v.end};
        for (uint32_t a = 0; a < 4; ++a)
          root.original_box[a] = root.prepared_box[a] = point(natural[a]);
        std::array<Interval, 64> net{};
        const auto count = (root.degree_u + 1) * (root.degree_v + 1) * 4;
        for (uint32_t i = 0; i <= root.degree_u; ++i)
          for (uint32_t j = 0; j <= root.degree_v; ++j)
            for (uint32_t a = 0; a < 4; ++a) {
              auto &value = net[(i * (root.degree_v + 1) + j) * 4 + a];
              for (uint32_t ci = 0; ci <= root.degree_u; ++ci)
                for (uint32_t cj = 0; cj <= root.degree_v; ++cj) {
                  charge(budget, remaining);
                  const auto source_index =
                      ((u.first_control + ci) * carrier.control_shape[1] + v.first_control + cj) *
                          4 +
                      a;
                  value = add(value, mul(mul(u.basis[ci][i], v.basis[cj][j]),
                                         point(original[source_index])));
                }
            }
        bool positive = true;
        for (uint32_t k = 0; k < count; ++k)
          if (!numeric::valid(net[k]) || (k % 4 == 3 && net[k].lower <= 0.))
            positive = false;
        if (!positive) {
          const auto precise = enclose_source_root_precise(source, root, budget);
          check_work();
          std::copy(precise.begin(), precise.end(), net.begin());
          for (uint32_t k = 3; k < count; k += 4)
            if (net[k].lower <= 0.)
              throw Error(ErrorCode::unresolved,
                          "positive precise original weight not representable");
        }
        append_controls(*data, root, std::span(net).first(count), limits, budget, remaining);
        data->original_roots.push_back(root);
        if (u.end <= box[0] || u.begin >= box[1] || v.end <= box[2] || v.begin >= box[3])
          continue;
        const std::array clipped{std::max(u.begin, box[0]), std::min(u.end, box[1]),
                                 std::max(v.begin, box[2]), std::min(v.end, box[3])};
        if (clipped != natural) {
          const auto prepared = restrict_net(root, net.data(), clipped);
          append_controls(*data, root, std::span(prepared).first(count), limits, budget, remaining);
        }
        for (uint32_t a = 0; a < 4; ++a)
          root.prepared_box[a] = point(clipped[a]);
        data->roots.push_back(root);
      }
  }
  prepare_point_owners(*data, budget, remaining);
  check_work();
  return data;
}
SourceCache prepare_source_cache(const SourceStore& source,std::span<const CarrierRequest> requests,
    const std::shared_ptr<RunResource>& resource,NumericProfile profile,SourceCacheLimits limits,bgfd::Budget& b){
  return SourceCache(prepare_source_cache_data(source,requests,resource,profile,limits,b,SourceCachePurpose::regular_patch));
}
SourceCache prepare_trace_source_cache(const SourceStore& source,std::span<const CarrierRequest> requests,
    const std::shared_ptr<RunResource>& resource,NumericProfile profile,SourceCacheLimits limits,bgfd::Budget& b){
  return SourceCache(prepare_source_cache_data(source,requests,resource,profile,limits,b,SourceCachePurpose::trace_geometry));
}
} // namespace bgmesh::adaptive
