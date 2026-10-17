#ifndef BREPGPU_MESHER_ADAPTIVE_TRIMMED_NUMERIC_HPP
#define BREPGPU_MESHER_ADAPTIVE_TRIMMED_NUMERIC_HPP
#include "adaptive_numeric.hpp"
#include "adaptive_trimmed_types.h"

namespace bgmesh::adaptive::numeric {
BGAD_HD inline uint32_t triangle_coefficient_index(TriangleCoefficient c) {
  return c.i * (c.degree + 1) - c.i * (c.i == 0 ? 0 : c.i - 1) / 2 + c.j;
}
BGAD_HD inline uint32_t triangle_multinomial(TriangleCoefficient c) {
  return binomial(c.degree, c.i) * binomial(c.degree - c.i, c.j);
}
BGAD_HD inline Interval trim_ratio(uint32_t numerator, uint32_t denominator) {
  return numerator == denominator
             ? point(1.)
             : div(point(static_cast<double>(numerator)), point(static_cast<double>(denominator)));
}
BGAD_HD inline Interval trim_lerp(Interval a, Interval b, Interval t) {
  if (t.lower == 0. && t.upper == 0.)
    return a;
  if (t.lower == 1. && t.upper == 1.)
    return b;
  return add(mul(sub(point(1.), t), a), mul(t, b));
}
BGAD_HD inline double source_triangle_anchor(const SourceTriangleInput &input, uint32_t axis) {
  return rn_add(rn_mul(input.affine_xyz[axis].lower, .5), rn_mul(input.affine_xyz[axis].upper, .5));
}
BGAD_HD inline double source_triangle_interval_radius_rn64(const Interval *value) {
  Interval radii[3]{};
  for (uint32_t axis = 0; axis < 3; ++axis) {
    if (!valid(value[axis]))
      return infinity();
    const auto center = rn_add(rn_mul(value[axis].lower, .5), rn_mul(value[axis].upper, .5));
    radii[axis] = point(magnitude(sub(value[axis], point(center))));
  }
  return norm_upper(radii);
}
// The Nx/Ny/Nz channels are centered by the SAME input-derived constant anchor
// BEFORE restriction. W is unchanged. Residual helper below uses L-anchor.
BGAD_HD inline Interval
source_triangle_homogeneous_coefficient_rn64(const RootDescriptor &root, const Interval *source,
                                             const SourceTriangleInput &input,
                                             TriangleCoefficient target, uint32_t channel) {
  const auto p = root.degree_u, q = root.degree_v, d = p + q;
  if (p > 3 || q > 3 || target.degree != d || target.i + target.j > d || channel > 3 || !source)
    return wide();
  const uint32_t alpha[]{d - target.i - target.j, target.i, target.j};
  const auto anchor = channel < 3 ? source_triangle_anchor(input, channel) : 0.;
  Interval result{};
  for (uint32_t i = 0; i <= p; ++i)
    for (uint32_t j = 0; j <= p - i; ++j) {
      const uint32_t beta[]{p - i - j, i, j};
      if (beta[0] > alpha[0] || beta[1] > alpha[1] || beta[2] > alpha[2])
        continue;
      const uint32_t gamma[]{alpha[0] - beta[0], alpha[1] - beta[1], alpha[2] - beta[2]};
      Interval work[16]{};
      for (uint32_t u = 0; u <= p; ++u)
        for (uint32_t v = 0; v <= q; ++v) {
          const auto at = (u * (q + 1) + v) * 4;
          work[u * (q + 1) + v] = channel == 3
                                      ? source[at + 3]
                                      : sub(source[at + channel], scale(source[at + 3], anchor));
        }
      for (uint32_t level = 0; level < p; ++level) {
        const auto vertex = level < beta[0] ? 0U : level < beta[0] + beta[1] ? 1U : 2U;
        const auto t = input.normalized_uv[2 * vertex];
        for (uint32_t u = 0; u < p - level; ++u)
          for (uint32_t v = 0; v <= q; ++v)
            work[u * (q + 1) + v] =
                trim_lerp(work[u * (q + 1) + v], work[(u + 1) * (q + 1) + v], t);
      }
      for (uint32_t level = 0; level < q; ++level) {
        const auto vertex = level < gamma[0] ? 0U : level < gamma[0] + gamma[1] ? 1U : 2U;
        const auto t = input.normalized_uv[2 * vertex + 1];
        for (uint32_t v = 0; v < q - level; ++v)
          work[v] = trim_lerp(work[v], work[v + 1], t);
      }
      const auto numerator =
          triangle_multinomial({p, i, j}) * triangle_multinomial({q, gamma[1], gamma[2]});
      const auto denominator = triangle_multinomial(target);
      result = add(result, mul(work[0], trim_ratio(numerator, denominator)));
    }
  return result;
}
BGAD_HD inline Interval source_triangle_residual_coefficient_rn64(const RootDescriptor &root,
                                                                  const Interval *triangle,
                                                                  const SourceTriangleInput &input,
                                                                  TriangleCoefficient target,
                                                                  uint32_t axis) {
  const auto degree = root.degree_u + root.degree_v;
  if (!triangle || target.degree != degree + 1 || target.i + target.j > target.degree || axis > 2)
    return wide();
  const uint32_t alpha[]{target.degree - target.i - target.j, target.i, target.j};
  const auto anchor = point(source_triangle_anchor(input, axis));
  Interval residual{};
  for (uint32_t vertex = 0; vertex < 3; ++vertex) {
    if (alpha[vertex] == 0)
      continue;
    const TriangleCoefficient parent{degree, target.i - static_cast<uint32_t>(vertex == 1),
                                     target.j - static_cast<uint32_t>(vertex == 2)};
    const auto at = triangle_coefficient_index(parent) * 4;
    const auto term = sub(triangle[at + axis],
                          mul(triangle[at + 3], sub(input.affine_xyz[vertex * 3 + axis], anchor)));
    residual = add(residual, mul(term, trim_ratio(alpha[vertex], target.degree)));
  }
  return residual;
}
BGAD_HD inline SourceTriangleBounds finish_source_triangle_rn64(const RootDescriptor &,
                                                                const SourceTriangleInput &,
                                                                double max_residual_norm,
                                                                double weight_lower,
                                                                double max_residual_radius = 0.) {
  SourceTriangleBounds result;
  if (!finite(max_residual_norm) || max_residual_norm < 0. || !finite(weight_lower) ||
      weight_lower <= 0. || !finite(max_residual_radius) || max_residual_radius < 0.)
    return result;
  result.position_upper = div(point(max_residual_norm), point(weight_lower)).upper;
  result.uncertainty_upper = div(point(max_residual_radius), point(weight_lower)).upper;
  if (finite(result.position_upper) && finite(result.uncertainty_upper))
    result.status = NumericStatus::ready;
  return result;
}
BGAD_HD inline SourceTriangleBounds bound_original_plane_rn64(const RootDescriptor &root,
                                                              const SourceTriangleInput &input) {
  SourceTriangleBounds result;
  if (root.family != GeometryFamily::plane) {
    result.status = NumericStatus::unsupported;
    return result;
  }
  double largest = 0., uncertainty = 0.;
  for (uint32_t vertex = 0; vertex < 3; ++vertex) {
    Interval residual[3]{};
    for (uint32_t axis = 0; axis < 3; ++axis) {
      const auto u =
          add(root.original_box[0], mul(input.normalized_uv[vertex * 2],
                                        sub(root.original_box[1], root.original_box[0])));
      const auto v =
          add(root.original_box[2], mul(input.normalized_uv[vertex * 2 + 1],
                                        sub(root.original_box[3], root.original_box[2])));
      const auto anchor = point(source_triangle_anchor(input, axis));
      residual[axis] = sub(add(sub(point(root.frame[axis]), anchor),
                               add(scale(u, root.frame[3 + axis]), scale(v, root.frame[6 + axis]))),
                           sub(input.affine_xyz[vertex * 3 + axis], anchor));
    }
    const auto upper = norm_upper(residual);
    if (!finite(upper))
      return result;
    largest = maximum(largest, upper);
    uncertainty = maximum(uncertainty, source_triangle_interval_radius_rn64(residual));
  }
  result.position_upper = largest;
  result.uncertainty_upper = uncertainty;
  if (finite(result.uncertainty_upper))
    result.status = NumericStatus::ready;
  return result;
}
BGAD_HD inline SourceTriangleBounds
bound_analytic_source_triangle_rn64(const RootDescriptor &root, const SourceTriangleInput &input) {
  if (root.family == GeometryFamily::plane)
    return bound_original_plane_rn64(root, input);
  Interval uv[6]{};
  double low[2]{infinity(), infinity()}, high[2]{-infinity(), -infinity()};
  double corner = 0., uncertainty = 0.;
  for (uint32_t vertex = 0; vertex < 3; ++vertex) {
    SampleInput point_input;
    point_input.original = input.original;
    for (uint32_t axis = 0; axis < 2; ++axis) {
      uv[vertex * 2 + axis] =
          add(root.original_box[2 * axis],
              mul(input.normalized_uv[vertex * 2 + axis],
                  sub(root.original_box[2 * axis + 1], root.original_box[2 * axis])));
      point_input.uv[axis] = uv[vertex * 2 + axis];
      low[axis] = minimum(low[axis], point_input.uv[axis].lower);
      high[axis] = maximum(high[axis], point_input.uv[axis].upper);
    }
    const auto point_result = evaluate_sample_rn64(root, nullptr, point_input);
    if (point_result.status != NumericStatus::ready)
      return SourceTriangleBounds{};
    Interval residual[3]{};
    for (uint32_t axis = 0; axis < 3; ++axis)
      residual[axis] = sub(point_result.original_xyz[axis], input.affine_xyz[vertex * 3 + axis]);
    const auto upper = norm_upper(residual);
    if (!finite(upper))
      return SourceTriangleBounds{};
    corner = maximum(corner, upper);
    uncertainty = maximum(uncertainty, source_triangle_interval_radius_rn64(residual));
  }
  Interval muu{}, muv{}, mvv{};
  const auto radius = point(absolute(root.radii[0]));
  if (root.family == GeometryFamily::cylinder)
    muu = mul(radius, root.frame_a);
  else if (root.family == GeometryFamily::cone) {
    const auto r = add(point(root.radii[0]), mul({low[1], high[1]}, root.cone_sin));
    muu = scale(root.frame_a, magnitude(r));
    muv = scale(root.frame_a, magnitude(root.cone_sin));
  } else if (root.family == GeometryFamily::sphere) {
    muu = mul(radius, root.frame_a);
    muv = muu;
    mvv = mul(radius, root.frame_f);
  } else if (root.family == GeometryFamily::torus) {
    muu = mul(root.frame_a, add(radius, point(absolute(root.radii[1]))));
    muv = scale(root.frame_a, absolute(root.radii[1]));
    mvv = scale(root.frame_f, absolute(root.radii[1]));
  } else {
    SourceTriangleBounds result;
    result.status = NumericStatus::unsupported;
    return result;
  }
  const auto hu = sub(point(high[0]), point(low[0])), hv = sub(point(high[1]), point(low[1]));
  const auto remainder = scale(
      add(add(mul(muu, square(hu)), scale(mul(muv, mul(hu, hv)), 2.)), mul(mvv, square(hv))), .125);
  SourceTriangleBounds result;
  result.position_upper = add(remainder, point(corner)).upper;
  result.uncertainty_upper = uncertainty;
  if (finite(result.position_upper) && finite(result.uncertainty_upper))
    result.status = NumericStatus::ready;
  return result;
}
BGAD_HD inline SourceTriangleBounds bound_source_triangle_rn64(const RootDescriptor &root,
                                                               const Interval *source,
                                                               const SourceTriangleInput &input) {
  if (root.family == GeometryFamily::plane)
    return bound_original_plane_rn64(root, input);
  if (root.family == GeometryFamily::rational_tensor) {
    SourceTriangleBounds result;
    const auto d = root.degree_u + root.degree_v;
    if (root.degree_u > 3 || root.degree_v > 3 || !source)
      return result;
    Interval triangle[112]{};
    double weight_lower = infinity();
    for (uint32_t i = 0; i <= d; ++i)
      for (uint32_t j = 0; j <= d - i; ++j) {
        const TriangleCoefficient coefficient{d, i, j};
        const auto at = triangle_coefficient_index(coefficient) * 4;
        for (uint32_t channel = 0; channel < 4; ++channel) {
          triangle[at + channel] = source_triangle_homogeneous_coefficient_rn64(
              root, source, input, coefficient, channel);
          if (!valid(triangle[at + channel]))
            return result;
        }
        weight_lower = minimum(weight_lower, triangle[at + 3].lower);
      }
    double largest = 0., uncertainty = 0.;
    for (uint32_t i = 0; i <= d + 1; ++i)
      for (uint32_t j = 0; j <= d + 1 - i; ++j) {
        Interval residual[3]{};
        for (uint32_t axis = 0; axis < 3; ++axis)
          residual[axis] =
              source_triangle_residual_coefficient_rn64(root, triangle, input, {d + 1, i, j}, axis);
        const auto upper = norm_upper(residual);
        if (!finite(upper))
          return result;
        largest = maximum(largest, upper);
        uncertainty = maximum(uncertainty, source_triangle_interval_radius_rn64(residual));
      }
    return finish_source_triangle_rn64(root, input, largest, weight_lower, uncertainty);
  }
  return bound_analytic_source_triangle_rn64(root, input);
}
} // namespace bgmesh::adaptive::numeric
#endif
