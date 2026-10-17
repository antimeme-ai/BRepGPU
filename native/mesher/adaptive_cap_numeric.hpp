#ifndef BREPGPU_MESHER_ADAPTIVE_CAP_NUMERIC_HPP
#define BREPGPU_MESHER_ADAPTIVE_CAP_NUMERIC_HPP
#include "adaptive_cap_types.h"
#include "adaptive_numeric.hpp"

namespace bgmesh::adaptive::numeric {
BGAD_HD inline int32_t cap_interval_sign(Interval value) {
  return !valid(value) ? 0 : value.lower > 0. ? 1 : value.upper < 0. ? -1 : 0;
}
BGAD_HD inline double cap_interval_radius(const Interval *xyz) {
  Interval radius[3]{};
  for (uint32_t axis = 0; axis < 3; ++axis) {
    if (!valid(xyz[axis]))
      return infinity();
    const auto center =
        rn_add(rn_mul(xyz[axis].lower, .5), rn_mul(xyz[axis].upper, .5));
    radius[axis] = point(magnitude(sub(xyz[axis], point(center))));
  }
  return norm_upper(radius);
}
// The source descriptor is produced once from the original root. This operation
// only bounds the supplied finite source map against its SAME actual fan L.
BGAD_HD inline CapFacetBounds
bound_cap_facet_rn64(const RootDescriptor &root,
                     const CapSourceDescriptor &source,
                     const CapFacetInput &input) {
  CapFacetBounds result;
  const bool cone = source.kind == AnalyticCapKind::cone_apex;
  if (source.original.value != root.original.value ||
      input.original.value != root.original.value ||
      (cone ? root.family != GeometryFamily::cone
            : root.family != GeometryFamily::sphere) ||
      (!cone && source.kind != AnalyticCapKind::sphere_north &&
       source.kind != AnalyticCapKind::sphere_south) ||
      !valid(source.pole_v) || !valid(root.frame_a) || !valid(root.frame_f) ||
      !finite(input.allowance_down) || input.allowance_down < 0.) {
    result.status = NumericStatus::invalid;
    return result;
  }
  for (uint32_t i = 0; i < 4; ++i)
    if (!valid(input.cut_uv[i])) {
      result.status = NumericStatus::invalid;
      return result;
    }
  for (uint32_t i = 0; i < 2; ++i)
    if (!valid(input.finite_v[i])) {
      result.status = NumericStatus::invalid;
      return result;
    }
  for (uint32_t i = 0; i < 9; ++i)
    if (!finite(input.actual_xyz[i])) {
      result.status = NumericStatus::invalid;
      return result;
    }
  const auto du = sub(input.cut_uv[2], input.cut_uv[0]);
  if (du.lower <= 0. || du.upper > half_pi().lower)
    return result;
  const auto finite_left = sub(input.cut_uv[1], input.finite_v[0]),
             finite_right = sub(input.cut_uv[3], input.finite_v[1]);
  const auto finite_side = cap_interval_sign(finite_left);
  if (finite_side == 0 || finite_side != cap_interval_sign(finite_right))
    return result;
  const auto left = sub(input.cut_uv[1], source.pole_v),
             right = sub(input.cut_uv[3], source.pole_v);
  const auto side = cap_interval_sign(left);
  if (side == 0 || side != cap_interval_sign(right))
    return result;
  const auto height = maximum(magnitude(left), magnitude(right));
  if (!finite(height) || (!cone && height >= half_pi().lower))
    return result;
  int32_t sheet = source.kind == AnalyticCapKind::sphere_north ? -side : side;
  if (cone) {
    const auto sine_sign = cap_interval_sign(root.cone_sin);
    if (sine_sign == 0 || cap_interval_sign(root.cone_cos) == 0)
      return result;
    sheet = side * sine_sign;
  }
  Interval residual[3]{};
  for (uint32_t axis = 0; axis < 3; ++axis)
    residual[axis] = sub(source.ideal_xyz[axis], point(input.actual_xyz[axis]));
  const auto pole_departure = norm_upper(residual);
  auto corner = pole_departure;
  auto uncertainty = cap_interval_radius(source.ideal_xyz);
  for (uint32_t i = 0; i < 2; ++i) {
    SampleInput point_input;
    point_input.original = root.original;
    point_input.uv[0] = input.cut_uv[2 * i];
    point_input.uv[1] = input.cut_uv[2 * i + 1];
    const auto point_result = evaluate_sample_rn64(root, nullptr, point_input);
    if (point_result.status != NumericStatus::ready)
      return result;
    for (uint32_t axis = 0; axis < 3; ++axis)
      residual[axis] = sub(point_result.original_xyz[axis],
                           point(input.actual_xyz[3 + 3 * i + axis]));
    corner = maximum(corner, norm_upper(residual));
    uncertainty =
        maximum(uncertainty, cap_interval_radius(point_result.original_xyz));
  }
  const auto factor = cone ? magnitude(root.cone_sin) : absolute(root.radii[0]);
  const auto base = scale(root.frame_a, factor);
  const auto muu = scale(base, cone ? height : minimum(1., height));
  const auto muv = base;
  const auto mvv = cone ? point(0.) : scale(root.frame_f, factor);
  // The exact cone Sv coefficient vector has unit Euclidean norm. Sphere Sv
  // has coefficient norm |r|. F bounds the FULL original frame in both cases.
  const auto mv = cone ? root.frame_f : scale(root.frame_f, factor);
  const auto dg = sub(input.cut_uv[3], input.cut_uv[1]);
  const auto angular =
      scale(add(add(mul(square(du), muu),
                    scale(mul(point(magnitude(mul(du, dg))), muv), 2.)),
                mul(square(dg), mvv)),
            .125);
  const auto radial = scale(mul(square(point(height)), mvv), .125);
  const auto gap = maximum(magnitude(sub(input.finite_v[0], source.pole_v)),
                           magnitude(sub(input.finite_v[1], source.pole_v)));
  const auto band = mul(point(gap), mv);
  const auto position = add(add(angular, radial), point(corner));
  const auto total = add(position, band);
  if (!valid(total) || !finite(pole_departure) || !finite(uncertainty))
    return result;
  result.status = NumericStatus::ready;
  result.position_upper = position.upper;
  result.band_upper = band.upper;
  result.total_upper = total.upper;
  result.angular_upper = angular.upper;
  result.radial_upper = radial.upper;
  result.corner_upper = corner;
  result.pole_departure_upper = pole_departure;
  // Point-enclosure routing diagnostic only, never a source or geometric floor.
  result.uncertainty_upper = uncertainty;
  result.regular_sheet_sign = sheet;
  result.positive_chart_roles[0] = 0;
  result.positive_chart_roles[1] = finite_side > 0 ? 2U : 1U;
  result.positive_chart_roles[2] = finite_side > 0 ? 1U : 2U;
  return result;
}
} // namespace bgmesh::adaptive::numeric
#endif
