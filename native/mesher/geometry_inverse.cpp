#include "geometry_inverse.hpp"
#include "face_seed.hpp"
#include "geometry_enclosure.hpp"
#include <algorithm>
namespace bgmesh {
namespace {
using namespace geometry_detail;
using UV = std::array<Rational, 2>;
using Pair = std::array<ExactEnclosure, 2>;
[[noreturn]] void unresolved(const char *message) { throw Error(ErrorCode::unresolved, message); }
ExactEnclosure project(std::span<const Rational, 3> covector,
                       const std::array<ExactEnclosure, 3> &vector) {
  return sum(sum(times(vector[0], covector[0]), times(vector[1], covector[1])),
             times(vector[2], covector[2]));
}
struct State final {
  Pair residual;
  std::array<ExactEnclosure, 4> jacobian;
};
struct Query final {const UV &xy,&uv;};
State state(GeometryContext &ctx, const GeometryStore &geometry, Slot<CarrierTag> carrier,
            const PlaneChart &plane, Query query, uint32_t precision) {
  const auto &xy=query.xy,&uv=query.uv;
  const auto jet =
      enclose_original_surface_point_jet(ctx, geometry, carrier, uv[0], uv[1], precision);
  const std::array<ExactEnclosure, 3> delta{
      difference(jet.position[0], ExactEnclosure(plane.origin[0])),
      difference(jet.position[1], ExactEnclosure(plane.origin[1])),
      difference(jet.position[2], ExactEnclosure(plane.origin[2]))};
  const auto x = std::span<const Rational, 3>(plane.projection.data(), 3),
             y = std::span<const Rational, 3>(plane.projection.data() + 3, 3);
  return {{difference(project(x, delta), ExactEnclosure(xy[0])),
           difference(project(y, delta), ExactEnclosure(xy[1]))},
          {project(x, jet.du), project(x, jet.dv), project(y, jet.du), project(y, jet.dv)}};
}
Pair conditioned(const PlaneChart &p, const Pair &r) {
  return {sum(times(r[0], p.preconditioner[0]), times(r[1], p.preconditioner[1])),
          sum(times(r[0], p.preconditioner[2]), times(r[1], p.preconditioner[3]))};
}
Rational midpoint(const ExactEnclosure &x) {
  return x.lower.add(x.upper).divide(Rational(x.lower.budget(), 2));
}
bool inside(const std::array<Rational, 4> &r, const UV &uv) {
  return uv[0].compare(r[0]) >= 0 && uv[0].compare(r[1]) <= 0 && uv[1].compare(r[2]) >= 0 &&
         uv[1].compare(r[3]) <= 0;
}
} // namespace
PlaneLocator recover_plane_chart_locator(GeometryContext &ctx, const WorkingChart &chart,
                                         const UV &xy, GeometryBoundPrecision precision,
                                         PlaneLocatorLimits limits) {
  if (chart.kind != WorkingChartKind::physical_plane_convex_q || !chart.plane)
    throw Error(ErrorCode::invalid_source, "plane inverse requires admitted working chart");
  const auto &geometry = chart.envelope.geometry;
  const auto carrier = chart.envelope.carrier;
  const auto &plane = *chart.plane;
  auto &budget = ctx.budget();
  const auto &source = geometry.view().source.view();
  if (carrier.value >= source.carriers.size() ||
      (source.carriers[carrier.value].kind != 1 && source.carriers[carrier.value].kind != 6))
    throw Error(ErrorCode::invalid_source,
                "plane inverse requires original plane-profile source carrier");
  for (const auto &value : xy)
    if (&value.budget() != &budget)
      throw Error(ErrorCode::invalid_source, "plane inverse owner mismatch");
  for (const auto &value : plane.natural_rectangle)
    if (&value.budget() != &budget)
      throw Error(ErrorCode::invalid_source, "plane inverse chart owner mismatch");
  if (&plane.inverse_monotonicity_lower.budget() != &budget ||
      plane.inverse_monotonicity_lower.sign() <= 0 || precision.transcendental_bits < 32 ||
      precision.transcendental_bits > ctx.maximum_bits() ||
      precision.norm_fractional_bits > ctx.maximum_bits() ||
      limits.normalized_fractional_bits == 0 ||
      limits.normalized_fractional_bits > ctx.maximum_bits())
    throw Error(ErrorCode::invalid_source, "plane inverse invalid regularity/precision profile");
  for (std::size_t i = 0; i < 4; ++i) {
    const auto j = (i + 1) % 4;
    const auto &q = plane.quadrilateral;
    if (q[2 * j]
            .subtract(q[2 * i])
            .multiply(xy[1].subtract(q[2 * i + 1]))
            .subtract(q[2 * j + 1].subtract(q[2 * i + 1]).multiply(xy[0].subtract(q[2 * i])))
            .sign() < 0)
      throw Error(ErrorCode::unsupported, "plane inverse request outside admitted convex Q");
  }
  const auto &r = plane.natural_rectangle;
  const UV widths{r[1].subtract(r[0]), r[3].subtract(r[2])};
  uint64_t grid_bits = static_cast<uint64_t>(limits.normalized_fractional_bits) + 16;
  for (const auto &width : widths) {
    if (width.sign() <= 0)
      throw Error(ErrorCode::invalid_source, "plane inverse empty natural rectangle");
    const auto n = width.numerator().bit_length(), d = width.denominator().bit_length();
    const auto needed =
        static_cast<uint64_t>(limits.normalized_fractional_bits) + 16 + (d > n ? d - n + 1 : 0);
    grid_bits = std::max(grid_bits, needed);
  }
  const auto norm_bits = std::max<uint64_t>(precision.norm_fractional_bits, grid_bits + 16);
  if (norm_bits > ctx.maximum_bits())
    unresolved("plane inverse normalized locator precision exceeds cap");
  const auto grid = static_cast<uint32_t>(grid_bits);
  const auto one = Rational(budget, 1);
  const auto scale = Rational::from_ratio(
      one.numerator(), one.denominator().shift_left(limits.normalized_fractional_bits));
  const UV requested{widths[0].multiply(scale), widths[1].multiply(scale)};
  UV uv{r[0].add(r[1]).divide(Rational(budget, 2)), r[2].add(r[3]).divide(Rational(budget, 2))};
  for (uint32_t iteration = 0;; ++iteration) {
    const auto current =
        state(ctx, geometry, carrier, plane, {xy,uv}, precision.transcendental_bits);
    const auto residual = conditioned(plane, current.residual);
    const auto upper = enclosure_norm_upper(budget, residual, static_cast<uint32_t>(norm_bits));
    const auto radius = upper.divide(plane.inverse_monotonicity_lower);
    std::array<Rational, 4> enclosure{uv[0].subtract(radius), uv[0].add(radius),
                                      uv[1].subtract(radius), uv[1].add(radius)};
    bool settled = true;
    for (std::size_t axis = 0; axis < 2; ++axis) {
      if (enclosure[2 * axis].compare(r[2 * axis]) < 0)
        enclosure[2 * axis] = r[2 * axis];
      if (enclosure[2 * axis + 1].compare(r[2 * axis + 1]) > 0)
        enclosure[2 * axis + 1] = r[2 * axis + 1];
      if (enclosure[2 * axis + 1].subtract(enclosure[2 * axis]).compare(requested[axis]) > 0)
        settled = false;
    }
    if (settled)
      return {std::move(uv), std::move(enclosure)};
    if (iteration >= limits.iterations)
      break;
    const std::array<Rational, 4> j{midpoint(current.jacobian[0]), midpoint(current.jacobian[1]),
                                    midpoint(current.jacobian[2]), midpoint(current.jacobian[3])};
    const auto det = j[0].multiply(j[3]).subtract(j[1].multiply(j[2]));
    if (det.sign() == 0)
      break;
    const auto rx = midpoint(current.residual[0]), ry = midpoint(current.residual[1]);
    const UV step{j[3].multiply(rx).subtract(j[1].multiply(ry)).divide(det),
                  j[0].multiply(ry).subtract(j[2].multiply(rx)).divide(det)};
    Rational fraction(budget, 1);
    bool moved = false;
    for (uint32_t attempt = 0; attempt < limits.backtracks; ++attempt) {
      UV trial{dyadic_outward(uv[0].subtract(step[0].multiply(fraction)), grid, false),
               dyadic_outward(uv[1].subtract(step[1].multiply(fraction)), grid, false)};
      fraction = fraction.divide(Rational(budget, 2));
      for (std::size_t axis = 0; axis < 2; ++axis) {
        if (trial[axis].compare(r[2 * axis]) < 0)
          trial[axis] = r[2 * axis];
        if (trial[axis].compare(r[2 * axis + 1]) > 0)
          trial[axis] = r[2 * axis + 1];
      }
      if (!inside(r, trial))
        continue;
      const auto next =
          state(ctx, geometry, carrier, plane, {xy,trial}, precision.transcendental_bits);
      const auto next_upper = enclosure_norm_upper(budget, conditioned(plane, next.residual),
                                                   static_cast<uint32_t>(norm_bits));
      if (next_upper.compare(upper) >= 0)
        continue;
      uv = std::move(trial);
      moved = true;
      break;
    }
    if (!moved)
      break;
  }
  unresolved("plane inverse locator did not settle within normalized precision/iteration limits");
}
PlaneLocator recover_plane_chart_locator(GeometryContext &context,
                                         const FaceBoundarySnapshotData &seed, const UV &xy,
                                         GeometryBoundPrecision precision,
                                         PlaneLocatorLimits limits) {
  if (seed.chart_kind != WorkingChartKind::physical_plane_convex_q || !seed.plane)
    throw Error(ErrorCode::invalid_source, "frozen plane locator lacks admitted source chart");
  auto &budget = context.budget();
  const auto &source = seed.geometry.view().source.view();
  if (seed.face.value >= source.faces.size() ||
      seed.source_generation != seed.geometry.view().table.key().generation)
    throw Error(ErrorCode::invalid_source, "frozen plane locator source generation mismatch");
  const auto carrier = source.carrier_index.find(source.faces[seed.face.value].carrier_id);
  const auto &p = *seed.plane;
  const auto load = [&]<std::size_t N>(const std::array<ExactRef, N> &refs) {
    std::array<Rational, N> out = [&]<std::size_t... I>(std::index_sequence<I...>) {
      return std::array<Rational, N>{(static_cast<void>(I), Rational(budget))...};
    }(std::make_index_sequence<N>{});
    for (std::size_t i = 0; i < N; ++i)
      out[i] = import_exact(budget, seed.values.view(refs[i]));
    return out;
  };
  PlaneChart plane{load(p.natural_rectangle),
                   load(p.origin),
                   load(p.basis),
                   load(p.projection),
                   load(p.quadrilateral),
                   load(p.preconditioner),
                   import_exact(budget, seed.values.view(p.height_upper)),
                   import_exact(budget, seed.values.view(p.inverse_monotonicity_lower))};
  FaceEnvelope envelope(seed.geometry, seed.face, carrier, seed.face_generation,
                        load(seed.prepared_original_rectangle));
  WorkingChart chart{seed.chart_kind, std::move(envelope), std::move(plane)};
  return recover_plane_chart_locator(context, chart, xy, precision, limits);
}
} // namespace bgmesh
