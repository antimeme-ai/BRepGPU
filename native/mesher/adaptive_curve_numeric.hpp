#ifndef BREPGPU_MESHER_ADAPTIVE_CURVE_NUMERIC_HPP
#define BREPGPU_MESHER_ADAPTIVE_CURVE_NUMERIC_HPP
#include "adaptive_curve_types.h"
#include "adaptive_numeric.hpp"
namespace bgmesh::adaptive::numeric {
BGAD_HD inline Interval curve_normalized_parameter_rn64(const CurveRootDescriptor &root,
                                                        Interval t) {
  if (!valid(t) || t.lower < root.original_t[0].lower || t.upper > root.original_t[1].upper)
    return wide();
  if (t.lower == t.upper && t.lower == root.original_t[0].lower)
    return point(0.);
  if (t.lower == t.upper && t.lower == root.original_t[1].upper)
    return point(1.);
  auto u = div(sub(t, root.original_t[0]), sub(root.original_t[1], root.original_t[0]));
  if (!valid(u))
    return u;
  // Admission established the original point is in the closed source span.
  u.lower = maximum(0., u.lower);
  u.upper = minimum(1., u.upper);
  return u;
}
BGAD_HD inline Interval curve_lerp(Interval a, Interval b, Interval t) {
  if (t.lower == 0. && t.upper == 0.)
    return a;
  if (t.lower == 1. && t.upper == 1.)
    return b;
  return add(mul(sub(point(1.), t), a), mul(t, b));
}
BGAD_HD inline Interval curve_midpoint(Interval a, Interval b) {
  return add(scale(a, .5), scale(b, .5));
}
struct CurveMidpointOutputs {
  Interval *left;
  Interval *right;
  Interval *workspace;
};
BGAD_HD inline void curve_midpoint_pair(const CurveRootDescriptor &root, const Interval *parent,
                                        CurveMidpointOutputs outputs) {
  auto *left = outputs.left;
  auto *right = outputs.right;
  auto *workspace = outputs.workspace;
  const auto stride = root.dimension + 1, count = (root.degree + 1) * stride;
  for (uint32_t j = 0; j < count; ++j)
    workspace[j] = parent[j];
  for (uint32_t a = 0; a < stride; ++a) {
    left[a] = workspace[a];
    right[root.degree * stride + a] = workspace[root.degree * stride + a];
  }
  for (uint32_t level = 1; level <= root.degree; ++level) {
    for (uint32_t i = 0; i <= root.degree - level; ++i)
      for (uint32_t a = 0; a < stride; ++a)
        workspace[i * stride + a] =
            curve_midpoint(workspace[i * stride + a], workspace[(i + 1) * stride + a]);
    for (uint32_t a = 0; a < stride; ++a) {
      left[level * stride + a] = workspace[a];
      right[(root.degree - level) * stride + a] = workspace[(root.degree - level) * stride + a];
    }
  }
}
BGAD_HD inline CurveSampleResult curve_sample_from_homogeneous(const CurveRootDescriptor &root,
                                                               const Interval *homogeneous,
                                                               const CurveSampleInput &input) {
  CurveSampleResult result;
  if (root.dimension == 2 && input.fixed_actual && input.actual_xyz[2] != 0.) {
    result.status = NumericStatus::invalid;
    return result;
  }
  if (root.dimension < 2 || root.dimension > 3 || !valid(homogeneous[root.dimension]) ||
      homogeneous[root.dimension].lower <= 0.)
    return result;
  Interval residual[3]{};
  for (uint32_t a = 0; a < 3; ++a) {
    const auto value =
        a < root.dimension ? div(homogeneous[a], homogeneous[root.dimension]) : point(0.);
    if (!valid(value))
      return result;
    result.original_xyz[a] = value;
    auto q = input.fixed_actual ? input.actual_xyz[a]
                                : rn_add(rn_mul(value.lower, .5), rn_mul(value.upper, .5));
    if (!finite(q))
      return result;
    if (a >= root.dimension && !input.fixed_actual)
      q = 0.;
    result.actual_xyz[a] = input.fixed_actual ? q : (q == 0. ? 0. : q);
    residual[a] = sub(value, point(result.actual_xyz[a]));
  }
  result.departure_upper = norm_upper(residual);
  if (finite(result.departure_upper))
    result.status = NumericStatus::ready;
  return result;
}
// Workspace is caller-owned: CPU private scratch or a cooperative device slab.
BGAD_HD inline CurveSampleResult evaluate_curve_sample_rn64(const CurveRootDescriptor &root,
                                                            const Interval *net,
                                                            const CurveSampleInput &input,
                                                            Interval *workspace) {
  if (root.family == CurveFamily::rational) {
    if (!workspace || root.degree > 32 || root.dimension < 2 || root.dimension > 3)
      return CurveSampleResult{};
    const auto stride = root.dimension + 1;
    const auto u = curve_normalized_parameter_rn64(root, input.t);
    if (!valid(u) || u.lower < 0. || u.upper > 1.)
      return CurveSampleResult{};
    for (uint32_t i = 0; i < (root.degree + 1) * stride; ++i)
      workspace[i] = net[i];
    for (uint32_t level = 0; level < root.degree; ++level)
      for (uint32_t i = 0; i < root.degree - level; ++i)
        for (uint32_t a = 0; a < stride; ++a)
          workspace[i * stride + a] =
              curve_lerp(workspace[i * stride + a], workspace[(i + 1) * stride + a], u);
    return curve_sample_from_homogeneous(root, workspace, input);
  }
  Interval value[4]{};
  value[root.dimension] = point(1.);
  const auto trig =
      root.family == CurveFamily::line ? SinCos{point(0.), point(1.), true} : sincos(input.t);
  if (!trig.ready)
    return CurveSampleResult{};
  for (uint32_t a = 0; a < root.dimension; ++a) {
    value[a] = point(root.frame[a]);
    if (root.family == CurveFamily::line)
      value[a] = add(value[a], scale(input.t, root.frame[3 + a]));
    else
      value[a] = add(value[a], add(mul(scale(trig.cosine, root.radii[0]), point(root.frame[3 + a])),
                                   mul(scale(trig.sine, root.radii[1]), point(root.frame[6 + a]))));
  }
  return curve_sample_from_homogeneous(root, value, input);
}
struct CurveResidualData {
  const Interval *net;
  const Interval *actual;
};
BGAD_HD inline Interval curve_residual_coefficient_interval_rn64(const CurveRootDescriptor &root,
                                                                 CurveResidualData data,
                                                                 const double *anchor,
                                                                 CurveResidualCoefficient c) {
  const auto *net = data.net;
  const auto *actual = data.actual;
  const auto p = root.degree, stride = root.dimension + 1, k = c.index, a = c.axis;
  Interval value{};
  if (a >= root.dimension)
    return value;
  if (k <= p) {
    const auto w = net[k * stride + root.dimension];
    const auto n = sub(net[k * stride + a], scale(w, anchor[a]));
    const auto residual = sub(n, mul(w, sub(actual[a], point(anchor[a]))));
    value = add(value,
                mul(div(point(static_cast<double>(p + 1 - k)), point(static_cast<double>(p + 1))),
                    residual));
  }
  if (k > 0) {
    const auto w = net[(k - 1) * stride + root.dimension];
    const auto n = sub(net[(k - 1) * stride + a], scale(w, anchor[a]));
    const auto residual = sub(n, mul(w, sub(actual[3 + a], point(anchor[a]))));
    value = add(value, mul(div(point(static_cast<double>(k)), point(static_cast<double>(p + 1))),
                           residual));
  }
  return value;
}
BGAD_HD inline Interval curve_residual_coefficient_rn64(const CurveRootDescriptor &root,
                                                        const Interval *net,
                                                        const CurveNodeInput &input,
                                                        const double *anchor,
                                                        CurveResidualCoefficient c) {
  Interval actual[6]{};
  for (uint32_t i = 0; i < 6; ++i)
    actual[i] = point(input.actual_endpoints[i]);
  return curve_residual_coefficient_interval_rn64(root, {net, actual}, anchor, c);
}
BGAD_HD inline CurveNodeBounds bound_curve_node_rn64(const CurveRootDescriptor &root,
                                                     const Interval *net,
                                                     const CurveNodeInput &input,
                                                     double supplied_residual = -1.) {
  CurveNodeBounds result;
  if (root.dimension == 2 && (input.actual_endpoints[2] != 0. || input.actual_endpoints[5] != 0.)) {
    result.status = NumericStatus::invalid;
    return result;
  }
  const auto departure = maximum(input.endpoint_departures[0], input.endpoint_departures[1]);
  if (!finite(departure) || departure < 0.)
    return result;
  result.numerical_upper = departure;
  if (root.family == CurveFamily::rational) {
    const auto stride = root.dimension + 1;
    double wmin = infinity();
    for (uint32_t i = 0; i <= root.degree; ++i) {
      const auto w = net[i * stride + root.dimension];
      if (!valid(w) || w.lower <= 0.)
        return result;
      wmin = minimum(wmin, w.lower);
    }
    double anchor[3]{};
    for (uint32_t a = 0; a < root.dimension; ++a) {
      const auto x = div(net[a], net[root.dimension]);
      anchor[a] = rn_add(rn_mul(x.lower, .5), rn_mul(x.upper, .5));
    }
    double residual = supplied_residual >= 0. ? supplied_residual : 0.;
    if (supplied_residual < 0.)
      for (uint32_t k = 0; k <= root.degree + 1; ++k) {
        Interval xyz[3]{};
        for (uint32_t a = 0; a < root.dimension; ++a)
          xyz[a] = curve_residual_coefficient_rn64(root, net, input, anchor, {k, a});
        residual = maximum(residual, norm_upper(xyz));
      }
    result.parameter_upper = residual == 0. ? 0. : up(rn_div(residual, wmin));
  } else {
    const auto width = sub(input.t[1], input.t[0]);
    result.parameter_upper =
        add(scale(mul(root.second_derivative_upper, square(width)), .125), point(departure)).upper;
  }
  if (root.line_profile) {
    result.image_available = 1;
    result.image_upper = departure;
    result.correspondence = CurveCorrespondence::monotone_line_image;
  }
  if (finite(result.parameter_upper))
    result.status = NumericStatus::ready;
  return result;
}
BGAD_HD inline CurveNodeBounds bound_affine_trace_rn64(const AffineTraceBoundInput &task,
                                                       const Interval *controls,
                                                       double supplied_residual = -1.) {
  if (task.kind == AffineTraceBoundKind::rational) {
    if (!controls || task.rational_curve.degree > 32 || task.rational_curve.dimension < 2 ||
        task.rational_curve.dimension > 3)
      return CurveNodeBounds{};
    const auto &root = task.rational_curve;
    if (root.dimension == 2 &&
        (!zero(task.affine_endpoints[2]) || !zero(task.affine_endpoints[5]))) {
      CurveNodeBounds invalid;
      invalid.status = NumericStatus::invalid;
      return invalid;
    }
    double anchor[3]{};
    for (uint32_t a = 0; a < 3; ++a) {
      const auto x = div(controls[a], controls[root.dimension]);
      anchor[a] = rn_add(rn_mul(x.lower, .5), rn_mul(x.upper, .5));
    }
    double residual = supplied_residual >= 0. ? supplied_residual : 0.;
    if (supplied_residual < 0.)
      for (uint32_t k = 0; k <= root.degree + 1; ++k) {
        Interval xyz[3]{};
        for (uint32_t a = 0; a < 3; ++a)
          xyz[a] = curve_residual_coefficient_interval_rn64(root, {controls, task.affine_endpoints},
                                                            anchor, {k, a});
        residual = maximum(residual, norm_upper(xyz));
      }
    auto result = bound_curve_node_rn64(root, controls, task.input, residual);
    result.image_available = 0;
    result.correspondence = CurveCorrespondence::affine_parameter;
    return result;
  }
  CurveNodeBounds result;
  const auto departure =
      maximum(task.input.endpoint_departures[0], task.input.endpoint_departures[1]);
  const auto width = sub(task.input.t[1], task.input.t[0]);
  result.numerical_upper = departure;
  result.parameter_upper =
      add(scale(mul(point(task.second_derivative_upper), square(width)), .125), point(departure))
          .upper;
  if (finite(result.parameter_upper) && departure >= 0. && task.second_derivative_upper >= 0.)
    result.status = NumericStatus::ready;
  return result;
}
} // namespace bgmesh::adaptive::numeric
#endif
