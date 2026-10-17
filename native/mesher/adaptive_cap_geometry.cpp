#include "adaptive_cap_geometry.hpp"
#include "adaptive_cap_numeric.hpp"
#include "geometry_enclosure.hpp"
#include <algorithm>
#include <cmath>

namespace bgmesh::adaptive {
namespace {
void require(bool okay, const char *message) {
  if (!okay)
    throw Error(ErrorCode::invalid_source, message);
}
AnalyticPoleKind exact_kind(AnalyticCapKind kind) {
  switch (kind) {
  case AnalyticCapKind::sphere_north:
    return AnalyticPoleKind::sphere_north;
  case AnalyticCapKind::sphere_south:
    return AnalyticPoleKind::sphere_south;
  case AnalyticCapKind::cone_apex:
    return AnalyticPoleKind::cone_apex;
  }
  throw Error(ErrorCode::invalid_source, "unknown analytic cap kind");
}
const RootDescriptor &original_root(const PreparedCapSource &prepared) {
  const auto slot = prepared.numeric().original.value;
  require(slot < prepared.source().original_roots().size(),
          "foreign original cap root");
  return prepared.source().original_roots()[slot];
}
bool descriptor_ready(const CapSourceDescriptor &value) {
  if (!numeric::valid(value.pole_v))
    return false;
  return std::all_of(std::begin(value.ideal_xyz), std::end(value.ideal_xyz),
                     numeric::valid);
}
Interval outward(const ExactEnclosure &value) {
  return {value.lower.bounds().lower, value.upper.bounds().upper};
}
ExactEnclosure exact_pole(GeometryContext &context, const Carrier &carrier,
                          AnalyticCapKind kind, uint32_t bits) {
  using namespace geometry_detail;
  auto &budget = context.budget();
  if (kind != AnalyticCapKind::cone_apex)
    return times(
        context.pi(bits + 64),
        Rational(budget, kind == AnalyticCapKind::sphere_north ? 1 : -1)
            .divide(Rational(budget, 2)));
  const auto alpha = Rational::from_double(budget, carrier.radii[1]);
  const auto angle = trig_range(context, alpha, alpha, bits);
  if (angle.sine.lower.sign() <= 0 && angle.sine.upper.sign() >= 0)
    throw Error(ErrorCode::unresolved,
                "cap apex source sine is not separated from zero");
  const auto radius = Rational::from_double(budget, carrier.radii[0]);
  return angle.sine.lower.sign() > 0
             ? quotient(ExactEnclosure(radius.negative()), angle.sine)
             : quotient(ExactEnclosure(radius), negate(angle.sine));
}
CapSourceDescriptor prepare_exact_descriptor(const SourceCache &cache,
                                             OriginalRootSlot original,
                                             AnalyticCapKind kind,
                                             bgfd::Budget &budget) {
  using namespace geometry_detail;
  const auto &root = cache.original_roots()[original.value];
  const auto &carrier = cache.source().view().carriers[root.source_slot];
  for (const auto bits : {128U, 192U, 256U}) {
    try {
      budget.charge(1);
      GeometryContext context(budget, {bits, bits + 256});
      const auto radius = Rational::from_double(budget, carrier.radii[0]);
      const auto pole = exact_pole(context, carrier, kind, bits);
      const auto axial = [&]() -> ExactEnclosure {
        if (kind != AnalyticCapKind::cone_apex)
          return ExactEnclosure(kind == AnalyticCapKind::sphere_north
                                    ? radius
                                    : radius.negative());
        const auto alpha = Rational::from_double(budget, carrier.radii[1]);
        return product(pole, trig_range(context, alpha, alpha, bits).cosine);
      }();
      CapSourceDescriptor result;
      result.original = original;
      result.kind = kind;
      result.stage = bits == 128   ? ArithmeticStage::p128
                     : bits == 192 ? ArithmeticStage::p192
                                   : ArithmeticStage::p256;
      result.pole_v = outward(pole);
      for (uint32_t axis = 0; axis < 3; ++axis) {
        const auto point = sum(
            ExactEnclosure(Rational::from_double(budget, carrier.frame[axis])),
            times(axial,
                  Rational::from_double(budget, carrier.frame[9 + axis])));
        result.ideal_xyz[axis] = outward(point);
      }
      if (descriptor_ready(result))
        return result;
    } catch (const Error &error) {
      if (error.code != ErrorCode::unresolved)
        throw;
    }
  }
  throw Error(ErrorCode::unresolved,
              "original cap auxiliary values are not representable");
}
void interval_in_profile(Interval value, const RootDescriptor &root,
                         uint32_t axis) {
  require(numeric::valid(value) &&
              value.lower >= root.original_box[2 * axis].lower &&
              value.upper <= root.original_box[2 * axis + 1].upper,
          "cap source coordinate outside retained original profile");
}
void exact_in_profile(const Rational &value, const RootDescriptor &root,
                      uint32_t axis, bgfd::Budget &budget) {
  require(&value.budget() == &budget &&
              value.compare(Rational::from_double(
                  budget, root.original_box[2 * axis].lower)) >= 0 &&
              value.compare(Rational::from_double(
                  budget, root.original_box[2 * axis + 1].upper)) <= 0,
          "exact cap source coordinate outside retained original profile");
}
void precision_valid(GeometryBoundPrecision precision) {
  require(precision.transcendental_bits >= 32 &&
              precision.transcendental_bits <= 4096 &&
              precision.norm_fractional_bits <= 4096,
          "cap numerical profile outside finite range");
}
int32_t interval_sign(const ExactEnclosure &value) {
  return value.lower.sign() > 0 ? 1 : value.upper.sign() < 0 ? -1 : 0;
}
ExactEnclosure stored_pole(const PreparedCapSource &source,
                           bgfd::Budget &budget) {
  const auto value = source.numeric().pole_v;
  return {Rational::from_double(budget, value.lower),
          Rational::from_double(budget, value.upper)};
}
int32_t cap_sheet(const PreparedCapSource &source, int32_t side,
                  GeometryContext *context, uint32_t bits) {
  const auto kind = source.numeric().kind;
  if (kind != AnalyticCapKind::cone_apex)
    return kind == AnalyticCapKind::sphere_north ? -side : side;
  const auto &root = original_root(source);
  auto sine_sign = numeric::cap_interval_sign(root.cone_sin);
  if (sine_sign == 0 && context) {
    const auto alpha = Rational::from_double(context->budget(), root.radii[1]);
    sine_sign = interval_sign(
        geometry_detail::trig_range(*context, alpha, alpha, bits).sine);
  }
  return side * sine_sign;
}
} // namespace
PreparedCapSource prepare_cap_source(const SourceCache &cache,
                                     OriginalRootSlot original,
                                     AnalyticCapKind kind,
                                     bgfd::Budget &budget) {
  admit_numeric_environment();
  require(cache.purpose() == SourceCachePurpose::quotient_trim &&
              original.value < cache.original_roots().size(),
          "cap source requires the retained quotient source bank");
  (void)exact_kind(kind);
  const auto &root = cache.original_roots()[original.value];
  const bool cone = kind == AnalyticCapKind::cone_apex;
  require(cone ? root.family == GeometryFamily::cone
               : root.family == GeometryFamily::sphere,
          "cap kind differs from original source family");
  if (cone && root.radii[1] == 0.)
    throw Error(ErrorCode::unsupported,
                "zero-angle cone has no isolated cap apex");
  budget.charge(1);
  CapSourceDescriptor result;
  result.original = original;
  result.kind = kind;
  if (cone) {
    result.pole_v = numeric::div(numeric::point(-root.radii[0]), root.cone_sin);
    const auto height = numeric::mul(result.pole_v, root.cone_cos);
    for (uint32_t axis = 0; axis < 3; ++axis)
      result.ideal_xyz[axis] =
          numeric::add(numeric::point(root.frame[axis]),
                       numeric::scale(height, root.frame[9 + axis]));
  } else {
    const auto sign = kind == AnalyticCapKind::sphere_north ? 1. : -1.;
    result.pole_v = numeric::scale(numeric::half_pi(), sign);
    for (uint32_t axis = 0; axis < 3; ++axis)
      result.ideal_xyz[axis] = numeric::add(
          numeric::point(root.frame[axis]),
          numeric::scale(numeric::mul(numeric::point(root.radii[0]),
                                      numeric::point(root.frame[9 + axis])),
                         sign));
  }
  if (!descriptor_ready(result))
    result = prepare_exact_descriptor(cache, original, kind, budget);
  return PreparedCapSource(cache, result);
}
CapFacetBounds bound_cap_facet(const PreparedCapSource &prepared,
                               const CapFacetInput &input,
                               bgfd::Budget &budget) {
  admit_numeric_environment();
  const auto &root = original_root(prepared);
  require(input.original.value == root.original.value,
          "cap input belongs to another source root");
  for (uint32_t corner = 0; corner < 2; ++corner) {
    budget.charge(1);
    interval_in_profile(input.cut_uv[2 * corner], root, 0);
    interval_in_profile(input.cut_uv[2 * corner + 1], root, 1);
    interval_in_profile(input.finite_v[corner], root, 1);
  }
  require(actual_facet_nonzero(input.actual_xyz, budget),
          "cap actual physical facet is collapsed");
  return numeric::bound_cap_facet_rn64(root, prepared.numeric(), input);
}
CapMapPlan admit_cap_map(const PreparedCapSource &prepared,
                         std::span<const Rational, 4> cut,
                         std::span<const Rational, 2> finite,
                         GeometryBoundPrecision precision,
                         bgfd::Budget &budget) {
  admit_numeric_environment();
  precision_valid(precision);
  const auto &root = original_root(prepared);
  for (uint32_t i = 0; i < 2; ++i) {
    exact_in_profile(cut[2 * i], root, 0, budget);
    exact_in_profile(cut[2 * i + 1], root, 1, budget);
    exact_in_profile(finite[i], root, 1, budget);
  }
  const auto du = cut[2].subtract(cut[0]);
  const auto gap = cut[1].subtract(finite[0]),
             other_gap = cut[3].subtract(finite[1]);
  require(
      du.sign() > 0 && gap.sign() != 0 && gap.sign() == other_gap.sign(),
      "cap source map has nonpositive width or changing finite height sign");
  const auto roles = gap.sign() > 0 ? std::array<uint32_t, 3>{0, 2, 1}
                                    : std::array<uint32_t, 3>{0, 1, 2};
  const auto admitted = [&](const ExactEnclosure &pole, const Rational &half_pi,
                            GeometryContext *context) {
    if (du.compare(half_pi) > 0)
      return int32_t{0};
    const auto a = geometry_detail::difference(ExactEnclosure(cut[1]), pole),
               b = geometry_detail::difference(ExactEnclosure(cut[3]), pole);
    const auto side = interval_sign(a);
    if (side == 0 || side != interval_sign(b))
      return int32_t{0};
    if (prepared.numeric().kind != AnalyticCapKind::cone_apex)
      for (const auto *value : {&a.lower, &a.upper, &b.lower, &b.upper}) {
        const auto magnitude =
            value->sign() < 0 ? value->negative() : Rational(*value);
        if (magnitude.compare(half_pi) >= 0)
          return int32_t{0};
      }
    return cap_sheet(prepared, side, context, precision.transcendental_bits);
  };
  const auto coarse = stored_pole(prepared, budget);
  const auto half = Rational::from_double(budget, numeric::half_pi().lower);
  if (const auto sheet = admitted(coarse, half, nullptr); sheet != 0)
    return {coarse, sheet, roles};
  GeometryContext context(
      budget, {precision.transcendental_bits,
               std::max(512U, precision.transcendental_bits + 256)});
  const auto pole = exact_pole(
      context, prepared.source().source().view().carriers[root.source_slot],
      prepared.numeric().kind, precision.transcendental_bits);
  const auto exact_half = context.pi(precision.transcendental_bits + 64)
                              .lower.divide(Rational(budget, 2));
  const auto sheet = admitted(pole, exact_half, &context);
  if (sheet == 0)
    throw Error(ErrorCode::unresolved,
                "cap source map has no proved single regular sheet");
  return {pole, sheet, roles};
}
std::array<CapProgressState, 2>
cap_cut_progress(const PreparedCapSource &prepared,
                 std::span<const Rational, 4> old_cut,
                 std::span<const Rational, 4> new_cut,
                 GeometryBoundPrecision precision, bgfd::Budget &budget) {
  admit_numeric_environment();
  precision_valid(precision);
  const auto &root = original_root(prepared);
  require(old_cut[2].subtract(old_cut[0]).sign() > 0 &&
              new_cut[2].subtract(new_cut[0]).sign() > 0,
          "cap radial history has a nonpositive angular width");
  for (uint32_t i = 0; i < 2; ++i) {
    exact_in_profile(old_cut[2 * i], root, 0, budget);
    exact_in_profile(new_cut[2 * i], root, 0, budget);
    exact_in_profile(old_cut[2 * i + 1], root, 1, budget);
    exact_in_profile(new_cut[2 * i + 1], root, 1, budget);
    require(old_cut[2 * i].compare(new_cut[2 * i]) == 0,
            "cap radial history changes angular endpoint sectors");
  }
  const auto compare = [&](const ExactEnclosure &pole) {
    std::array<CapProgressState, 2> result{CapProgressState::unresolved,
                                           CapProgressState::unresolved};
    for (uint32_t i = 0; i < 2; ++i) {
      const auto old_gap = geometry_detail::difference(
                     ExactEnclosure(old_cut[2 * i + 1]), pole),
                 new_gap = geometry_detail::difference(
                     ExactEnclosure(new_cut[2 * i + 1]), pole);
      const auto side = interval_sign(old_gap),
                 new_side = interval_sign(new_gap);
      if (side == 0 || new_side == 0)
        continue;
      require(side == new_side, "cap radial history crosses the symbolic pole");
      const auto offset =
          new_cut[2 * i + 1]
              .multiply(Rational(budget, 4))
              .subtract(old_cut[2 * i + 1].multiply(Rational(budget, 3)));
      const auto test = geometry_detail::times(
          geometry_detail::difference(ExactEnclosure(offset), pole),
          Rational(budget, side));
      if (test.upper.sign() <= 0)
        result[i] = CapProgressState::proved;
      else if (test.lower.sign() > 0)
        result[i] = CapProgressState::not_contracted;
    }
    return result;
  };
  const auto coarse = compare(stored_pole(prepared, budget));
  if (std::none_of(coarse.begin(), coarse.end(), [](auto state) {
        return state == CapProgressState::unresolved;
      }))
    return coarse;
  GeometryContext context(
      budget, {precision.transcendental_bits,
               std::max(512U, precision.transcendental_bits + 256)});
  return compare(exact_pole(
      context, prepared.source().source().view().carriers[root.source_slot],
      prepared.numeric().kind, precision.transcendental_bits));
}
PreciseCapResult compare_cap_facet_precise(const PreparedCapSource &prepared,
                                           std::span<const Rational, 4> cut,
                                           std::span<const Rational, 2> finite,
                                           std::span<const double, 9> actual,
                                           const Rational &allowance,
                                           GeometryBoundPrecision precision,
                                           bgfd::Budget &budget) {
  admit_numeric_environment();
  const auto &root = original_root(prepared);
  require(&allowance.budget() == &budget && allowance.sign() > 0,
          "precise cap allowance owner/value mismatch");
  precision_valid(precision);
  for (uint32_t i = 0; i < 2; ++i) {
    exact_in_profile(cut[2 * i], root, 0, budget);
    exact_in_profile(cut[2 * i + 1], root, 1, budget);
    exact_in_profile(finite[i], root, 1, budget);
  }
  const auto left = cut[1].subtract(finite[0]),
             right = cut[3].subtract(finite[1]);
  require(left.sign() != 0 && left.sign() == right.sign(),
          "precise cap finite source height changes sign");
  require(actual_facet_nonzero(actual, budget),
          "precise cap actual physical facet is collapsed");
  GeometryContext context(
      budget, {precision.transcendental_bits,
               std::max(512U, precision.transcendental_bits + 256)});
  const bool ordered = finite[0].compare(finite[1]) <= 0;
  const PoleFanRequest request{
      exact_kind(prepared.numeric().kind),
      {cut[0], cut[1], cut[2], cut[3]},
      {actual[0], actual[1], actual[2], actual[3], actual[4], actual[5],
       actual[6], actual[7], actual[8]},
      {finite[ordered ? 0 : 1], finite[ordered ? 1 : 0]},
      1,
      precision};
  auto bounds = bound_analytic_pole_fan(
      context, prepared.source().source().view().carriers[root.source_slot],
      request);
  // The finite-source reference orientation, not proxy UV or ideal-cut side,
  // decides positive chart roles. Barycentric (rho(1-xi),rho*xi) has Jacobian
  // -rho.
  bounds.facet_roles = left.sign() > 0 ? std::array<uint32_t, 3>{0, 2, 1}
                                       : std::array<uint32_t, 3>{0, 1, 2};
  const bool certified =
      bounds.position_upper.add(bounds.band_upper).compare(allowance) <= 0;
  return {std::move(bounds), certified};
}
} // namespace bgmesh::adaptive
