#include "adaptive_geometry.hpp"
#include "adaptive_numeric.hpp"
#include "geometry.hpp"
#include "geometry_enclosure.hpp"
#include "geometry_jets.hpp"
#include <cfenv>
#include <cfloat>
#if defined(__SSE__)
#include <xmmintrin.h>
#endif
namespace bgmesh::adaptive {
void admit_numeric_environment() {
  if (FLT_EVAL_METHOD != 0 || std::fegetround() != FE_TONEAREST)
    throw Error(ErrorCode::unsupported, "adaptive RN requires binary64 nearest arithmetic");
#if defined(__SSE__)
  if ((_mm_getcsr() & ((1U << 15) | (1U << 6))) != 0)
    throw Error(ErrorCode::unsupported, "adaptive RN requires gradual underflow");
#endif
}
namespace {
void check(bool ok, const char *message) {
  if (!ok)
    throw Error(ErrorCode::invalid_source, message);
}
} // namespace
uint32_t find_carrier_root_slot(std::span<const RootDescriptor> roots, Slot<CarrierTag> source,
                               std::span<const Rational, 2> uv, RootBoxKind kind) {
  const auto box = [kind](const RootDescriptor &root) {
    return kind == RootBoxKind::original ? root.original_box : root.prepared_box;
  };
  auto &b = uv[0].budget();
  check(&uv[1].budget() == &b, "foreign carrier coordinate owner");
  size_t first = 0, last = roots.size();
  while (first < last) {
    b.charge(1);
    const auto m = first + (last - first) / 2;
    if (roots[m].source_slot < source.value ||
        (roots[m].source_slot == source.value &&
         Rational::from_double(b, box(roots[m])[1].lower).compare(uv[0]) < 0))
      first = m + 1;
    else
      last = m;
  }
  check(first < roots.size() && roots[first].source_slot == source.value &&
            Rational::from_double(b, box(roots[first])[0].lower).compare(uv[0]) <= 0,
        "carrier point outside compiled U roots");
  const auto row = roots[first].span_u;
  size_t end = first;
  // Binary search the contiguous U row's end; no all-span point scan.
  last = roots.size();
  while (end < last) {
    b.charge(1);
    const auto m = end + (last - end) / 2;
    if (roots[m].source_slot == source.value && roots[m].span_u == row)
      end = m + 1;
    else
      last = m;
  }
  last = end;
  while (first < last) {
    b.charge(1);
    const auto m = first + (last - first) / 2;
    if (Rational::from_double(b, box(roots[m])[3].lower).compare(uv[1]) < 0)
      first = m + 1;
    else
      last = m;
  }
  check(first < end &&
            Rational::from_double(b, box(roots[first])[2].lower).compare(uv[1]) <= 0,
        "carrier point outside compiled V roots");
  return static_cast<uint32_t>(first);
}
SampleResult evaluate_sample(const SourceCache &cache, const SampleInput &input) {
  admit_numeric_environment();
  check(input.original.value < cache.original_roots().size(), "foreign original root");
  const auto &root = cache.original_roots()[input.original.value];
  const auto controls = cache.controls();
  check(root.controls.first <= controls.size() &&
            root.controls.count <= controls.size() - root.controls.first,
        "original coefficient range");
  for (uint32_t a = 0; a < 2; ++a)
    check(numeric::valid(input.uv[a]) && input.uv[a].lower >= root.original_box[2 * a].lower &&
              input.uv[a].upper <= root.original_box[2 * a + 1].upper,
          "source point outside original root");
  return numeric::evaluate_sample_rn64(
      root, root.controls.count ? controls.data() + root.controls.first : nullptr, input);
}
NodeBounds bound_node(const SourceCache &cache, const NodePredicate &input,
                      std::span<const Interval> controls) {
  admit_numeric_environment();
  check(input.root.value < cache.roots().size(), "foreign prepared root");
  const auto &root = cache.roots()[input.root.value];
  for (size_t a = 0; a < 2; ++a) {
    check(numeric::valid(input.box[2 * a]) && numeric::valid(input.box[2 * a + 1]) &&
              input.box[2 * a].lower >= root.prepared_box[2 * a].lower &&
              input.box[2 * a + 1].upper <= root.prepared_box[2 * a + 1].upper,
          "node outside prepared source rectangle");
  }
  if (root.family == GeometryFamily::rational_tensor) {
    if (controls.empty()) {
      const auto pool = cache.controls();
      check(input.controls.first <= pool.size() &&
                input.controls.count <= pool.size() - input.controls.first,
            "node coefficient range");
      controls = pool.subspan(input.controls.first, input.controls.count);
    }
    check(controls.size() == static_cast<size_t>(root.degree_u + 1) * (root.degree_v + 1) * 4,
          "node coefficient shape");
  }
  for (const auto q : input.actual_corners)
    check(numeric::finite(q), "nonfinite actual corner");
  return numeric::bound_node_rn64(root, controls.empty() ? nullptr : controls.data(), input);
}
ActualFacetBounds bound_actual_facet(const ActualFacetInput &input) {
  admit_numeric_environment();
  for (const auto x : input.actual_corners)
    check(numeric::finite(x), "nonfinite actual B corner");
  for (const auto x : input.uv)
    check(numeric::valid(x) && x.lower >= 0. && x.upper <= 1., "virtual facet outside same leaf");
  for (const auto x : input.affine_xyz)
    check(numeric::valid(x), "invalid actual affine facet interval");
  return numeric::bound_actual_facet_rn64(input);
}
bool actual_facet_nonzero(std::span<const double, 9> xyz, bgfd::Budget &budget) {
  admit_numeric_environment();
  for (const auto value : xyz)
    check(numeric::finite(value), "nonfinite actual facet coordinate");
  for (uint32_t i = 0; i < 3; ++i) {
    budget.charge(1);
    const auto j = (i + 1) % 3, k = (i + 2) % 3;
    const auto cross = numeric::sub(
      numeric::mul(numeric::sub(numeric::point(xyz[3 + j]), numeric::point(xyz[j])),
                    numeric::sub(numeric::point(xyz[6 + k]), numeric::point(xyz[k]))),
      numeric::mul(numeric::sub(numeric::point(xyz[3 + k]), numeric::point(xyz[k])),
                    numeric::sub(numeric::point(xyz[6 + j]), numeric::point(xyz[j]))));
    if (numeric::valid(cross) && (cross.lower > 0. || cross.upper < 0.))
      return true;
  }
  for (uint32_t i = 0; i < 3; ++i) {
    const auto j = (i + 1) % 3, k = (i + 2) % 3;
    const auto cross = Rational::from_double(budget, xyz[3 + j])
      .subtract(Rational::from_double(budget, xyz[j]))
      .multiply(Rational::from_double(budget, xyz[6 + k]).subtract(Rational::from_double(budget, xyz[k])))
      .subtract(Rational::from_double(budget, xyz[3 + k])
        .subtract(Rational::from_double(budget, xyz[k]))
        .multiply(Rational::from_double(budget, xyz[6 + j]).subtract(Rational::from_double(budget, xyz[j]))));
    if (cross.sign() != 0)
      return true;
  }
  return false;
}

namespace {
Rational ratio(bgfd::Budget &b, int64_t n, int64_t d = 1) {
  return Rational(b, n).divide(Rational(b, d));
}
std::pmr::vector<Rational> zeros(bgfd::Budget &b, size_t count) {
  std::pmr::vector<Rational> values(&b);
  values.reserve(count);
  for (size_t i = 0; i < count; ++i)
    values.emplace_back(b);
  return values;
}
std::pmr::vector<double> expanded(const SourceData &data, Range<double> axis, Range<uint32_t> mult,
                                  bgfd::Budget &b) {
  std::pmr::vector<double> out(&b);
  const auto knots = data.view(axis);
  const auto multiplicities = data.view(mult);
  for (size_t i = 0; i < knots.size(); ++i)
    for (uint32_t j = 0; j < multiplicities[i]; ++j) {
      b.charge(1);
      out.push_back(knots[i]);
    }
  return out;
}
std::pmr::vector<Rational> exact_basis(std::span<const double> knots, uint32_t span, uint32_t i,
                                       uint32_t degree, bgfd::Budget &b) {
  b.charge(1);
  auto result = zeros(b, 4);
  if (degree == 0) {
    if (i == span)
      result[0] = Rational(b, 1);
    return result;
  }
  const auto origin = Rational::from_double(b, knots[span]);
  const auto width = Rational::from_double(b, knots[span + 1]).subtract(origin);
  if (knots[i + degree] != knots[i]) {
    const auto denominator =
        Rational::from_double(b, knots[i + degree]).subtract(Rational::from_double(b, knots[i]));
    const auto c = origin.subtract(Rational::from_double(b, knots[i])).divide(denominator),
               x = width.divide(denominator);
    const auto previous = exact_basis(knots, span, i, degree - 1, b);
    for (uint32_t k = 0; k < degree; ++k) {
      result[k] = result[k].add(c.multiply(previous[k]));
      result[k + 1] = result[k + 1].add(x.multiply(previous[k]));
    }
  }
  if (knots[i + degree + 1] != knots[i + 1]) {
    const auto denominator = Rational::from_double(b, knots[i + degree + 1])
                                 .subtract(Rational::from_double(b, knots[i + 1]));
    const auto
        c = Rational::from_double(b, knots[i + degree + 1]).subtract(origin).divide(denominator),
        x = width.divide(denominator).negative();
    const auto previous = exact_basis(knots, span, i + 1, degree - 1, b);
    for (uint32_t k = 0; k < degree; ++k) {
      result[k] = result[k].add(c.multiply(previous[k]));
      result[k + 1] = result[k + 1].add(x.multiply(previous[k]));
    }
  }
  return result;
}
BezierSurface exact_original_root(const SourceStore &source_store, const RootDescriptor &root,
                                  bgfd::Budget &b) {
  const auto &source = source_store.view();
  check(root.source_slot < source.carriers.size(), "foreign exact source carrier");
  const auto &carrier = source.carriers[root.source_slot];
  check(carrier.kind == 6 && carrier.id.value == root.source_id &&
            root.degree_u == carrier.degrees[0] && root.degree_v == carrier.degrees[1] &&
            root.degree_u <= 3 && root.degree_v <= 3 && root.span_u >= root.degree_u &&
            root.span_u < carrier.control_shape[0] && root.span_v >= root.degree_v &&
            root.span_v < carrier.control_shape[1],
        "exact source root metadata");
  const auto ku = expanded(source, carrier.knot_u, carrier.mult_u, b),
             kv = expanded(source, carrier.knot_v, carrier.mult_v, b);
  const double expected[]{ku[root.span_u], ku[root.span_u + 1], kv[root.span_v],
                          kv[root.span_v + 1]};
  for (size_t a = 0; a < 4; ++a)
    check(root.original_box[a].lower == expected[a] && root.original_box[a].upper == expected[a],
          "exact original span bounds");
  check(expected[0] < expected[1] && expected[2] < expected[3], "positive original span rectangle");
  auto bu = zeros(b, 16), bv = zeros(b, 16);
  for (uint32_t axis = 0; axis < 2; ++axis) {
    const auto degree = axis == 0 ? root.degree_u : root.degree_v,
               span = axis == 0 ? root.span_u : root.span_v;
    auto &target = axis == 0 ? bu : bv;
    for (uint32_t i = 0; i <= degree; ++i) {
      const auto power =
          exact_basis(axis == 0 ? std::span<const double>(ku) : std::span<const double>(kv), span,
                      span - degree + i, degree, b);
      for (uint32_t j = 0; j <= degree; ++j)
        for (uint32_t k = 0; k <= j; ++k)
          target[i * 4 + j] = target[i * 4 + j].add(
              power[k].multiply(ratio(b, numeric::binomial(j, k), numeric::binomial(degree, k))));
    }
  }
  const std::array bounds{Rational::from_double(b, root.original_box[0].lower),
                          Rational::from_double(b, root.original_box[1].lower),
                          Rational::from_double(b, root.original_box[2].lower),
                          Rational::from_double(b, root.original_box[3].lower)};
  BezierSurface net(b, root.degree_u, root.degree_v, bounds);
  const auto original = source.view(carrier.controls);
  for (uint32_t i = 0; i <= root.degree_u; ++i)
    for (uint32_t j = 0; j <= root.degree_v; ++j)
      for (uint32_t a = 0; a < 4; ++a) {
        auto value = Rational(b);
        for (uint32_t ci = 0; ci <= root.degree_u; ++ci)
          for (uint32_t cj = 0; cj <= root.degree_v; ++cj) {
            const auto index = ((root.span_u - root.degree_u + ci) * carrier.control_shape[1] +
                                root.span_v - root.degree_v + cj) *
                                   4 +
                               a;
            value = value.add(bu[ci * 4 + i]
                                  .multiply(bv[cj * 4 + j])
                                  .multiply(Rational::from_double(b, original[index])));
          }
        net.controls.push_back(std::move(value));
      }
  return net;
}
Rational exact_norm(std::span<const Rational> vector, bgfd::Budget &b) {
  auto square = Rational(b);
  for (const auto &value : vector)
    square = square.add(value.multiply(value));
  if (square.sign() == 0)
    return Rational(b);
  const auto numerator_bits = square.numerator().bit_length();
  const auto denominator_bits = square.denominator().bit_length();
  const auto gap = denominator_bits > numerator_bits ? denominator_bits - numerator_bits : 0;
  if (gap > 8192)
    throw Error(ErrorCode::unresolved, "precise norm exponent outside finite profile");
  return square.square_root_bounds(160 + static_cast<uint32_t>((gap + 1) / 2)).upper;
}
uint32_t stage_bits(ArithmeticStage stage) {
  if (stage == ArithmeticStage::p128)
    return 128;
  if (stage == ArithmeticStage::p192)
    return 192;
  if (stage == ArithmeticStage::p256)
    return 256;
  return 384;
}
void owner(bgfd::Budget &b, std::span<const Rational> values) {
  for (const auto &value : values)
    check(&value.budget() == &b, "precise geometry owner mismatch");
}
} // namespace
std::pmr::vector<Interval> enclose_source_root_precise(const SourceStore &source,
                                                       const RootDescriptor &root,
                                                       bgfd::Budget &budget) {
  admit_numeric_environment();
  check(root.source_slot < source.view().carriers.size() &&
            root.family == GeometryFamily::rational_tensor,
        "precise root source association");
  const auto net = exact_original_root(source, root, budget);
  std::pmr::vector<Interval> intervals(&budget);
  intervals.reserve(net.controls.size());
  for (const auto &value : net.controls) {
    const auto x = value.bounds();
    intervals.push_back({x.lower, x.upper});
  }
  return intervals;
}
SampleResult evaluate_sample_precise(const SourceCache &cache, OriginalRootSlot slot,
                                     std::span<const Rational, 2> uv, ArithmeticStage stage,
                                     bgfd::Budget &b) {
  admit_numeric_environment();
  owner(b, uv);
  check(slot.value < cache.original_roots().size(), "foreign precise source root");
  check(stage == ArithmeticStage::p128 || stage == ArithmeticStage::p192 ||
            stage == ArithmeticStage::p256 || stage == ArithmeticStage::precise,
        "unknown precise point profile");
  const auto &root = cache.original_roots()[slot.value];
  for (size_t a = 0; a < 2; ++a)
    check(uv[a].compare(Rational::from_double(b, root.original_box[2 * a].lower)) >= 0 &&
              uv[a].compare(Rational::from_double(b, root.original_box[2 * a + 1].lower)) <= 0,
          "precise point outside original root");
  // ExactEnclosure has owner-safe construction but intentionally no assignment.
  auto emit = [&](std::span<const ExactEnclosure, 3> source) {
    SampleResult result;
    auto departure = zeros(b, 3);
    for (size_t a = 0; a < 3; ++a) {
      const auto lo = source[a].lower.bounds().lower, hi = source[a].upper.bounds().upper;
      result.original_xyz[a] = {lo, hi};
      const auto q = numeric::rn_add(numeric::rn_mul(lo, .5), numeric::rn_mul(hi, .5));
      result.actual_xyz[a] = q == 0. ? 0. : q;
      const auto actual = Rational::from_double(b, result.actual_xyz[a]);
      auto left = source[a].lower.subtract(actual), right = source[a].upper.subtract(actual);
      if (left.sign() < 0)
        left = left.negative();
      if (right.sign() < 0)
        right = right.negative();
      departure[a] = left.compare(right) >= 0 ? left : right;
    }
    result.departure_upper = exact_norm(departure, b).bounds().upper;
    result.status = NumericStatus::ready;
    return result;
  };
  if (root.family == GeometryFamily::rational_tensor) {
    const auto net = exact_original_root(cache.source(), root, b);
    const auto point = evaluate_surface(net, uv[0], uv[1]);
    const std::array exact{ExactEnclosure(point[0]), ExactEnclosure(point[1]),
                           ExactEnclosure(point[2])};
    return emit(exact);
  }
  GeometryContext context(b, {stage_bits(stage), stage_bits(stage) + 256});
  const auto point = evaluate_analytic_surface(
      context, cache.source().view().carriers[root.source_slot], uv[0], uv[1]);
  return emit(point.coordinates);
}
PreciseBoundResult bound_node_precise(const SourceCache &cache, RootSlot slot,
                                      std::span<const Rational, 4> box,
                                      std::span<const double, 12> actual, const Rational &allowance,
                                      bgfd::Budget &b) {
  admit_numeric_environment();
  owner(b, box);
  check(&allowance.budget() == &b && allowance.sign() > 0, "precise bound allowance owner/value");
  check(slot.value < cache.roots().size(), "foreign precise prepared root");
  const auto &root = cache.roots()[slot.value];
  for (size_t a = 0; a < 2; ++a)
    check(box[2 * a].compare(Rational::from_double(b, root.prepared_box[2 * a].lower)) >= 0 &&
              box[2 * a + 1].compare(
                  Rational::from_double(b, root.prepared_box[2 * a + 1].upper)) <= 0 &&
              box[2 * a].compare(box[2 * a + 1]) < 0,
          "precise node outside prepared rectangle");
  auto e = Rational(b);
  NodeBounds bounds;
  bounds.status = NumericStatus::ready;
  auto twist = zeros(b, 3);
  for (size_t a = 0; a < 3; ++a)
    twist[a] = Rational::from_double(b, actual[a])
                   .subtract(Rational::from_double(b, actual[3 + a]))
                   .subtract(Rational::from_double(b, actual[6 + a]))
                   .add(Rational::from_double(b, actual[9 + a]));
  const auto twist_norm = exact_norm(twist, b);
  if (root.family == GeometryFamily::rational_tensor) {
    const auto original = exact_original_root(cache.source(), root, b);
    const std::array domain{box[0], box[1], box[2], box[3]};
    const auto net = restrict_surface(original, domain);
    auto wmin = net.controls[3];
    for (size_t i = 7; i < net.controls.size(); i += 4)
      if (net.controls[i].compare(wmin) < 0)
        wmin = net.controls[i];
    check(wmin.sign() > 0, "precise original positive weight");
    for (uint32_t i = 0; i <= net.p + 1; ++i)
      for (uint32_t j = 0; j <= net.q + 1; ++j) {
        auto residual = zeros(b, 3);
        for (uint32_t c = 0; c < 2; ++c)
          for (uint32_t d = 0; d < 2; ++d) {
            if (i < c || j < d)
              continue;
            const auto a = i - c, z = j - d;
            if (a > net.p || z > net.q)
              continue;
            const auto factor =
                ratio(b, numeric::binomial(net.p, a), numeric::binomial(net.p + 1, i))
                    .multiply(
                        ratio(b, numeric::binomial(net.q, z), numeric::binomial(net.q + 1, j)));
            const auto index = (a * (net.q + 1) + z) * 4;
            for (uint32_t channel = 0; channel < 3; ++channel)
              residual[channel] = residual[channel].add(factor.multiply(
                  net.controls[index + channel].subtract(net.controls[index + 3].multiply(
                      Rational::from_double(b, actual[(c + 2 * d) * 3 + channel])))));
          }
        const auto upper = exact_norm(residual, b).divide(wmin);
        if (upper.compare(e) > 0)
          e = upper;
      }
    // Local immersion is still admitted by the SAME interval derivative net.
    std::array<Interval, 64> intervals{};
    for (size_t i = 0; i < net.controls.size(); ++i) {
      const auto x = net.controls[i].bounds();
      intervals[i] = {x.lower, x.upper};
    }
    double anchor[3]{};
    for (size_t a = 0; a < 3; ++a)
      anchor[a] = net.controls[a].divide(net.controls[3]).to_double();
    bounds.regularity_proved =
        numeric::regular_rational_net(root, intervals.data(), anchor) ? 1U : 0U;
  } else {
    GeometryContext context(b, {192, 512});
    const std::array domain{box[0], box[1], box[2], box[3]};
    const auto &carrier = cache.source().view().carriers[root.source_slot];
    const auto jet = enclose_analytic_surface_jet(context, carrier, domain, 192);
    const auto mu = geometry_detail::enclosure_norm_upper(b, jet.duu, 160),
               mv = geometry_detail::enclosure_norm_upper(b, jet.dvv, 160);
    auto corner = Rational(b);
    for (size_t i = 0; i < 4; ++i) {
      const auto point = evaluate_analytic_surface(context, carrier, box[i % 2], box[2 + i / 2]);
      const std::array residual{
          geometry_detail::difference(point.coordinates[0],
                                      ExactEnclosure(Rational::from_double(b, actual[3 * i]))),
          geometry_detail::difference(point.coordinates[1],
                                      ExactEnclosure(Rational::from_double(b, actual[3 * i + 1]))),
          geometry_detail::difference(point.coordinates[2],
                                      ExactEnclosure(Rational::from_double(b, actual[3 * i + 2])))};
      const auto upper = geometry_detail::enclosure_norm_upper(b, residual, 160);
      if (upper.compare(corner) > 0)
        corner = upper;
    }
    const auto hu = box[1].subtract(box[0]), hv = box[3].subtract(box[2]);
    e = mu.multiply(hu.multiply(hu))
            .add(mv.multiply(hv.multiply(hv)))
            .divide(Rational(b, 8))
            .add(corner);
    bounds.regularity_proved = 1;
  }
  const auto packet = e.add(twist_norm.divide(Rational(b, 4)));
  bounds.source_to_bilinear_upper = e.bounds().upper;
  bounds.twist_upper = twist_norm.bounds().upper;
  bounds.packet_upper = packet.bounds().upper;
  bounds.score_u = bounds.packet_upper;
  bounds.score_v = bounds.packet_upper;
  return {bounds, bounds.regularity_proved != 0 && packet.compare(allowance) <= 0};
}
PreciseFacetResult bound_actual_facet_precise(const ActualFacetInput &input,
                                              std::span<const Rational, 6> uv,
                                              std::span<const Rational, 9> affine,
                                              const Rational &allowance, bgfd::Budget &b) {
  admit_numeric_environment();
  owner(b, uv);
  owner(b, affine);
  check(&allowance.budget() == &b && allowance.sign() > 0, "precise facet allowance owner/value");
  check(numeric::finite(input.source_to_bilinear_upper) && input.source_to_bilinear_upper >= 0.,
        "precise facet original source upper");
  for (const auto &coordinate : uv)
    check(coordinate.sign() >= 0 && coordinate.compare(Rational(b, 1)) <= 0,
          "precise virtual facet outside leaf");
  auto maximum_square = Rational(b), diagonal_square = Rational(b);
  for (size_t i = 0; i < 3; ++i)
    for (size_t j = i; j < 3; ++j) {
      auto residual = zeros(b, 3);
      for (size_t a = 0; a < 3; ++a) {
        const auto c = Rational::from_double(b, input.actual_corners[a]);
        const auto x = Rational::from_double(b, input.actual_corners[3 + a]).subtract(c),
                   y = Rational::from_double(b, input.actual_corners[6 + a]).subtract(c);
        const auto d = Rational::from_double(b, input.actual_corners[9 + a])
                           .subtract(Rational::from_double(b, input.actual_corners[6 + a]))
                           .subtract(x);
        residual[a] =
            c.add(uv[2 * i].add(uv[2 * j]).multiply(x).divide(Rational(b, 2)))
                .add(uv[2 * i + 1].add(uv[2 * j + 1]).multiply(y).divide(Rational(b, 2)))
                .add(uv[2 * i]
                         .multiply(uv[2 * j + 1])
                         .add(uv[2 * j].multiply(uv[2 * i + 1]))
                         .multiply(d)
                         .divide(Rational(b, 2)))
                .subtract(affine[3 * i + a].add(affine[3 * j + a]).divide(Rational(b, 2)));
      }
      auto square = Rational(b);
      for (const auto &x : residual)
        square = square.add(x.multiply(x));
      if (square.compare(maximum_square) > 0)
        maximum_square = square;
      if (i == j && square.compare(diagonal_square) > 0)
        diagonal_square = square;
    }
  auto twist_square = Rational(b);
  for (size_t a = 0; a < 3; ++a) {
    const auto d = Rational::from_double(b, input.actual_corners[9 + a])
                       .subtract(Rational::from_double(b, input.actual_corners[6 + a]))
                       .subtract(Rational::from_double(b, input.actual_corners[3 + a]))
                       .add(Rational::from_double(b, input.actual_corners[a]));
    twist_square = twist_square.add(d.multiply(d));
  }
  auto range_product = Rational(b, 1);
  for (size_t axis = 0; axis < 2; ++axis) {
    auto lo = uv[axis], hi = uv[axis];
    for (size_t vertex = 1; vertex < 3; ++vertex) {
      const auto &value = uv[vertex * 2 + axis];
      if (value.compare(lo) < 0)
        lo = value;
      if (value.compare(hi) > 0)
        hi = value;
    }
    range_product = range_product.multiply(hi.subtract(lo));
  }
  const auto quarter_square = twist_square.multiply(range_product.multiply(range_product))
                                  .divide(Rational(b, 16));
  auto maximum = maximum_square.square_root_bounds(160).upper;
  const auto covariance = quarter_square.square_root_bounds(160).upper.add(
      diagonal_square.square_root_bounds(160).upper);
  if (covariance.compare(maximum) < 0)
    maximum = covariance;
  const auto e = Rational::from_double(b, input.source_to_bilinear_upper),
             remaining = allowance.subtract(e);
  bool certified = false;
  if (remaining.sign() >= 0) {
    const auto square = remaining.multiply(remaining);
    const auto slack = square.subtract(quarter_square).subtract(diagonal_square);
    // Nonnegative sum-of-roots comparison, including exact quarter-twist
    // equality without a rounded square-root upper.
    certified = maximum_square.compare(square) <= 0 ||
                (slack.sign() >= 0 &&
                 slack.multiply(slack).compare(
                     quarter_square.multiply(diagonal_square).multiply(Rational(b, 4))) >= 0);
  }
  ActualFacetBounds result;
  result.status = NumericStatus::ready;
  result.bilinear_to_facet_upper = maximum.bounds().upper;
  result.total_upper = maximum.add(e).bounds().upper;
  return {result, certified};
}
std::array<Rational,3> exact_original_surface_point(const SourceCache&cache,OriginalRootSlot slot,
    std::span<const Rational,2>uv,bgfd::Budget&b){
  admit_numeric_environment();owner(b,uv);
  check(slot.value<cache.original_roots().size(),"foreign exact surface point root");
  const auto&root=cache.original_roots()[slot.value];
  check(root.family==GeometryFamily::rational_tensor,"exact homogeneous surface point requires rational source");
  const auto net=exact_original_root(cache.source(),root,b);
  return evaluate_surface(net,uv[0],uv[1]);
}
BezierSurface surface_detail::original_net_exact(const SourceCache &cache, OriginalRootSlot slot,
                                                 bgfd::Budget &budget) {
  admit_numeric_environment();
  check(slot.value < cache.original_roots().size(), "foreign exact original surface root");
  const auto &root = cache.original_roots()[slot.value];
  check(root.family == GeometryFamily::rational_tensor, "exact surface root requires rational source");
  return exact_original_root(cache.source(), root, budget);
}
} // namespace bgmesh::adaptive
