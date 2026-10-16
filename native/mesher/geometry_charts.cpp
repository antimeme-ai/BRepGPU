#include "geometry_charts.hpp"
#include "geometry_enclosure.hpp"
#include "geometry_profile.hpp"
#include <algorithm>

namespace bgmesh {
namespace {
using namespace geometry_detail;
using V3 = std::array<Rational, 3>;
using P2 = std::array<Rational, 2>;
using UVBox = std::array<Rational, 4>;
using Matrix = std::array<Rational, 4>;
struct PlaneFrame final {
  V3 origin, x, y, dual_x, dual_y;
};
struct Projection final {
  const PlaneFrame &frame;
  const V3 &point;
};
Rational dot(const V3 &a, const V3 &b) {
  return a[0].multiply(b[0]).add(a[1].multiply(b[1])).add(a[2].multiply(b[2]));
}
V3 difference3(const V3 &a, const V3 &b) {
  return {a[0].subtract(b[0]), a[1].subtract(b[1]), a[2].subtract(b[2])};
}
P2 project(Projection request) {
  const auto delta = difference3(request.point, request.frame.origin);
  return {dot(request.frame.dual_x, delta), dot(request.frame.dual_y, delta)};
}
std::optional<PlaneFrame> frame(const std::array<V3, 4> &corners) {
  const auto x = difference3(corners[1], corners[0]);
  const auto y = difference3(corners[3], corners[0]);
  const auto xx = dot(x, x), xy = dot(x, y), yy = dot(y, y);
  const auto det = xx.multiply(yy).subtract(xy.multiply(xy));
  if (det.sign() <= 0)
    return std::nullopt;
  const auto dual = [&](const Rational &a, const V3 &p, const Rational &c, const V3 &q) {
    return V3{a.multiply(p[0]).subtract(c.multiply(q[0])).divide(det),
              a.multiply(p[1]).subtract(c.multiply(q[1])).divide(det),
              a.multiply(p[2]).subtract(c.multiply(q[2])).divide(det)};
  };
  return PlaneFrame{corners[0], x, y, dual(yy, x, xy, y), dual(xx, y, xy, x)};
}
Rational turn(const P2 &a, const P2 &b, const P2 &c) {
  return b[0]
      .subtract(a[0])
      .multiply(c[1].subtract(a[1]))
      .subtract(b[1].subtract(a[1]).multiply(c[0].subtract(a[0])));
}
std::array<P2, 4> projected_corners(const PlaneFrame &f, const std::array<V3, 4> &corners) {
  return {project({f, corners[0]}), project({f, corners[1]}), project({f, corners[2]}),
          project({f, corners[3]})};
}
ExactEnclosure projected_vector(const V3 &covector, const std::array<ExactEnclosure, 3> &vector) {
  return sum(sum(times(vector[0], covector[0]), times(vector[1], covector[1])),
             times(vector[2], covector[2]));
}
std::array<ExactEnclosure, 4> projected_jacobian(const PlaneFrame &f,
                                                 const SurfaceJetEnclosure &jet) {
  return {projected_vector(f.dual_x, jet.du), projected_vector(f.dual_x, jet.dv),
          projected_vector(f.dual_y, jet.du), projected_vector(f.dual_y, jet.dv)};
}
std::optional<Matrix> center_preconditioner(const PlaneFrame &f, const SurfaceJetEnclosure &jet) {
  const auto j = projected_jacobian(f, jet);
  for (const auto &entry : j)
    if (entry.lower.compare(entry.upper) != 0)
      return std::nullopt;
  const auto det = j[0].lower.multiply(j[3].lower).subtract(j[1].lower.multiply(j[2].lower));
  if (det.sign() <= 0)
    return std::nullopt;
  return Matrix{j[3].lower.divide(det), j[1].lower.negative().divide(det),
                j[2].lower.negative().divide(det), j[0].lower.divide(det)};
}
std::optional<Rational> positive_symmetric(const PlaneFrame &f, const Matrix &m,
                                           const SurfaceJetEnclosure &jet) {
  const auto j = projected_jacobian(f, jet);
  const auto a = sum(times(j[0], m[0]), times(j[2], m[1]));
  const auto d = sum(times(j[1], m[2]), times(j[3], m[3]));
  if (a.lower.sign() <= 0 || d.lower.sign() <= 0)
    return std::nullopt;
  const auto b =
      sum(sum(times(j[1], m[0]), times(j[3], m[1])), sum(times(j[0], m[2]), times(j[2], m[3])));
  const Rational four(a.lower.budget(), 4);
  const auto det =
      difference(product(a, d), times(squared(b), Rational(a.lower.budget(), 1).divide(four)));
  if (det.lower.sign() <= 0)
    return std::nullopt;
  return det.lower.divide(a.upper.add(d.upper));
}
struct Admission final {
  const PlaneFrame &frame;
  const Matrix &preconditioner;
  const ChartProfileLimits &limits;
  Rational &monotonicity;
  uint64_t visited = 0;
};
bool regular_cell(Admission &admission, const BezierSurface &root, const UVBox &box,
                  uint32_t depth) {
  if (admission.visited >= admission.limits.cells)
    return false;
  ++admission.visited;
  if (const auto lower = positive_symmetric(admission.frame, admission.preconditioner,
                                            enclose_rational_surface_jet(root, box))) {
    if (admission.monotonicity.sign() == 0 || lower->compare(admission.monotonicity) < 0)
      admission.monotonicity = *lower;
    return true;
  }
  if (depth >= admission.limits.subdivision_depth)
    return false;
  const std::size_t axis = static_cast<std::size_t>(depth % 2) * 2;
  const auto mid = box[axis].add(box[axis + 1]).divide(Rational(box[0].budget(), 2));
  auto left = box, right = box;
  left[axis + 1] = mid;
  right[axis] = mid;
  return regular_cell(admission, root, left, depth + 1) &&
         regular_cell(admission, root, right, depth + 1);
}
struct BoundaryProfile final {
  const PlaneFrame &frame;
  const std::array<P2, 4> &corners;
  const UVBox &natural;
};
bool straight_boundary(const BoundaryProfile &profile, const BezierSurface &root,
                       std::size_t side) {
  const bool varying_u = side == 0 || side == 2;
  const auto fixed_axis = varying_u ? 2U : 0U;
  const bool upper = side == 1 || side == 2;
  if (root.bounds[fixed_axis + static_cast<std::size_t>(upper)].compare(
          profile.natural[fixed_axis + static_cast<std::size_t>(upper)]) != 0)
    return true;
  const std::array<std::size_t, 4> start{0, 1, 3, 0}, end{1, 2, 2, 3};
  const auto &a = profile.corners[start[side]], &b = profile.corners[end[side]];
  const P2 direction{b[0].subtract(a[0]), b[1].subtract(a[1])};
  auto &budget = root.bounds[0].budget();
  std::pmr::vector<Rational> scalar(&budget), weights(&budget);
  const auto degree = varying_u ? root.p : root.q;
  for (uint32_t i = 0; i <= degree; ++i) {
    const auto u = varying_u ? i : (upper ? root.p : 0U);
    const auto v = varying_u ? (upper ? root.q : 0U) : i;
    const auto index = (static_cast<std::size_t>(u) * (root.q + 1) + v) * 4;
    const auto &w = root.controls[index + 3];
    V3 centered{root.controls[index].subtract(w.multiply(profile.frame.origin[0])),
                root.controls[index + 1].subtract(w.multiply(profile.frame.origin[1])),
                root.controls[index + 2].subtract(w.multiply(profile.frame.origin[2]))};
    const auto x = dot(profile.frame.dual_x, centered).subtract(w.multiply(a[0]));
    const auto y = dot(profile.frame.dual_y, centered).subtract(w.multiply(a[1]));
    if (x.multiply(direction[1]).subtract(y.multiply(direction[0])).sign() != 0)
      return false;
    scalar.push_back(x.multiply(direction[0]).add(y.multiply(direction[1])));
    weights.push_back(w);
  }
  return ratio_increasing(scalar, weights);
}
Rational root_height(const PlaneFrame &f, const BezierSurface &root, uint32_t bits) {
  auto &budget = root.bounds[0].budget();
  Rational upper(budget, 0);
  for (std::size_t i = 0; i < root.controls.size(); i += 4) {
    V3 point{root.controls[i].divide(root.controls[i + 3]),
             root.controls[i + 1].divide(root.controls[i + 3]),
             root.controls[i + 2].divide(root.controls[i + 3])};
    const auto delta = difference3(point, f.origin);
    const auto xy = project({f, point});
    std::array<ExactEnclosure, 3> residual{
        ExactEnclosure(delta[0].subtract(f.x[0].multiply(xy[0])).subtract(f.y[0].multiply(xy[1]))),
        ExactEnclosure(delta[1].subtract(f.x[1].multiply(xy[0])).subtract(f.y[1].multiply(xy[1]))),
        ExactEnclosure(delta[2].subtract(f.x[2].multiply(xy[0])).subtract(f.y[2].multiply(xy[1])))};
    const auto next = enclosure_norm_upper(budget, residual, bits);
    if (next.compare(upper) > 0)
      upper = next;
  }
  return upper;
}
std::optional<PlaneChart> plane_profile(GeometryContext &ctx, const GeometryStore &geometry,
                                        const FaceEnvelope &envelope,
                                        const FaceChartRequest &request) {
  auto &budget = ctx.budget();
  const auto &source = geometry.view().source.view();
  const auto &carrier = source.carriers[envelope.carrier.value];
  if (carrier.kind != 6 && carrier.kind != 1)
    return std::nullopt;
  const auto &r = envelope.uv;
  std::array<V3, 4> corners{V3{Rational(budget), Rational(budget), Rational(budget)},
                            V3{Rational(budget), Rational(budget), Rational(budget)},
                            V3{Rational(budget), Rational(budget), Rational(budget)},
                            V3{Rational(budget), Rational(budget), Rational(budget)}};
  const std::array<std::array<std::size_t, 2>, 4> indices{{{0, 2}, {1, 2}, {1, 3}, {0, 3}}};
  for (std::size_t i = 0; i < 4; ++i) {
    const auto point = enclose_original_surface(ctx, geometry, envelope.carrier, r[indices[i][0]],
                                                r[indices[i][1]]);
    for (std::size_t axis = 0; axis < 3; ++axis) {
      if (point.coordinates[axis].lower.compare(point.coordinates[axis].upper) != 0)
        return std::nullopt;
      corners[i][axis] = point.coordinates[axis].lower;
    }
  }
  const auto candidate = frame(corners);
  if (!candidate)
    return std::nullopt;
  const auto &f = *candidate;
  const auto q = projected_corners(f, corners);
  for (std::size_t i = 0; i < 4; ++i)
    if (turn(q[i], q[(i + 1) % 4], q[(i + 2) % 4]).sign() <= 0)
      return std::nullopt;
  const auto u = r[0].add(r[1]).divide(Rational(budget, 2));
  const auto v = r[2].add(r[3]).divide(Rational(budget, 2));
  const auto m = center_preconditioner(
      f, enclose_original_surface_point_jet(ctx, geometry, envelope.carrier, u, v,
                                            request.precision.transcendental_bits));
  if (!m)
    return std::nullopt;
  Rational height(budget, 0), monotonicity(budget, 0);
  if (carrier.kind == 6) {
    Admission admission{f, *m, request.profile_limits, monotonicity};
    BoundaryProfile boundary{f, q, r};
    const auto roots = intersecting_surface_roots(budget, geometry, envelope.carrier, r);
    for (const auto slot : roots) {
      const auto root = import_surface_root(budget, geometry, slot);
      if (!regular_cell(admission, root, root.bounds, 0))
        return std::nullopt;
      for (std::size_t side = 0; side < 4; ++side)
        if (!straight_boundary(boundary, root, side))
          return std::nullopt;
      const auto bound = root_height(f, root, request.precision.norm_fractional_bits);
      if (bound.compare(height) > 0)
        height = bound;
    }
  } else {
    const auto lower = positive_symmetric(
        f, *m,
        enclose_analytic_surface_jet(ctx, carrier, r, request.precision.transcendental_bits));
    if (!lower)
      return std::nullopt;
    monotonicity = *lower;
  }
  if (height.compare(request.positional_allowance) >= 0)
    return std::nullopt;
  return PlaneChart{r,
                    f.origin,
                    {f.x[0], f.y[0], f.x[1], f.y[1], f.x[2], f.y[2]},
                    {f.dual_x[0], f.dual_x[1], f.dual_x[2], f.dual_y[0], f.dual_y[1], f.dual_y[2]},
                    {q[0][0], q[0][1], q[1][0], q[1][1], q[2][0], q[2][1], q[3][0], q[3][1]},
                    *m,
                    std::move(height),
                    std::move(monotonicity)};
}
} // namespace
WorkingChart choose_chart(GeometryContext &ctx, const GeometryStore &geometry,
                          const FaceChartRequest &request) {
  if (&request.positional_allowance.budget() != &ctx.budget() ||
      &request.node_source_allowance.budget() != &ctx.budget() ||
      request.positional_allowance.sign() <= 0 || request.node_source_allowance.sign() <= 0)
    throw Error(ErrorCode::invalid_source, "invalid face chart allowance owner or value");
  auto envelope = prepare_face_envelope(
      ctx, geometry,
      {.face = request.face, .generation = request.generation, .precision = request.precision});
  const auto kind = geometry.view().source.view().carriers[envelope.carrier.value].kind;
  if (request.allow_plane_profile) {
    auto plane = plane_profile(ctx, geometry, envelope, request);
    if (plane)
      return {WorkingChartKind::physical_plane_convex_q, std::move(envelope), std::move(plane)};
  }
  return {kind == 6 || kind == 1 ? WorkingChartKind::original_uv
                                 : WorkingChartKind::analytic_unwrapped_uv,
          std::move(envelope), std::nullopt};
}
} // namespace bgmesh
