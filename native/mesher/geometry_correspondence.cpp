#include "geometry_correspondence.hpp"
#include "geometry_enclosure.hpp"
#include "geometry_profile.hpp"
#include <algorithm>

namespace bgmesh {
namespace {
using namespace geometry_detail;
using V3 = std::array<Rational, 3>;
using UV = std::array<Rational, 2>;
using Net = BezierCurve;
[[noreturn]] void invalid(const char *message) { throw Error(ErrorCode::invalid_source, message); }
Rational lerp(const Rational &a, const Rational &b, const Rational &t) {
  return a.add(t.multiply(b.subtract(a)));
}
Rational normalized(const TraceIntervalRequest &request, const Rational &t) {
  return t.subtract(request.source_parameter[0])
      .divide(request.source_parameter[1].subtract(request.source_parameter[0]));
}
UV uv_at(const TraceIntervalRequest &request, const Rational &t) {
  const auto s = normalized(request, t);
  return {lerp(request.original_uv[0], request.original_uv[2], s),
          lerp(request.original_uv[1], request.original_uv[3], s)};
}
UV chart_at(const TraceIntervalRequest &request, const Rational &t) {
  const auto s = normalized(request, t);
  return {lerp(request.chart_endpoints[0], request.chart_endpoints[2], s),
          lerp(request.chart_endpoints[1], request.chart_endpoints[3], s)};
}
V3 xyz_at(const TraceIntervalRequest &request, const Rational &t) {
  const auto s = normalized(request, t);
  return {lerp(request.actual_endpoint_xyz[0], request.actual_endpoint_xyz[3], s),
          lerp(request.actual_endpoint_xyz[1], request.actual_endpoint_xyz[4], s),
          lerp(request.actual_endpoint_xyz[2], request.actual_endpoint_xyz[5], s)};
}
V3 point(const Net &net, bool end) {
  const auto i = end ? static_cast<std::size_t>(net.degree) * 4 : 0;
  return {net.controls[i].divide(net.controls[i + 3]),
          net.controls[i + 1].divide(net.controls[i + 3]),
          net.controls[i + 2].divide(net.controls[i + 3])};
}
Rational dot(const V3 &a, const V3 &b) {
  return a[0].multiply(b[0]).add(a[1].multiply(b[1])).add(a[2].multiply(b[2]));
}
Net affine_net(bgfd::Budget &b, const std::array<V3,2> &endpoints) {
  Net result(b, {3, 1}, {Rational(b, 0), Rational(b, 1)});
  for (const auto &coordinate : endpoints[0])
    result.controls.push_back(coordinate);
  result.controls.emplace_back(b, 1);
  for (const auto &coordinate : endpoints[1])
    result.controls.push_back(coordinate);
  result.controls.emplace_back(b, 1);
  return result;
}
V3 collapsed_point(const EnclosedPoint &point) {
  for (const auto &coordinate : point.coordinates)
    if (coordinate.lower.compare(coordinate.upper) != 0)
      throw Error(ErrorCode::internal_error,
                  "rational trace expected an exact original source point");
  return {point.coordinates[0].lower, point.coordinates[1].lower, point.coordinates[2].lower};
}
std::optional<Net> curve_net(GeometryContext &ctx, const GeometryStore &geometry,
                             Slot<CurveTag> slot, const std::array<Rational, 2> &interval) {
  const auto &curve = geometry.view().source.view().curves[slot.value];
  if (curve.kind == 1) {
    return affine_net(ctx.budget(),
                      {collapsed_point(enclose_original_curve(ctx, geometry, slot, interval[0])),
                      collapsed_point(enclose_original_curve(ctx, geometry, slot, interval[1]))});
  }
  if (curve.kind <= 3)
    return std::nullopt;
  const auto middle = interval[0].add(interval[1]).divide(Rational(ctx.budget(), 2));
  const auto root =
      import_curve_root(ctx.budget(), geometry, find_curve_root(geometry, slot, middle));
  return restrict_curve(root, interval[0], interval[1]);
}
Net tensor_segment(const BezierSurface &surface, const UV &a, const UV &b) {
  auto &budget = a[0].budget();
  const std::array<Rational, 6> uv{a[0], a[1], b[0], b[1], a[0], a[1]};
  const auto triangle = restrict_to_triangle(surface, uv);
  Net result(budget, {3, triangle.degree}, {Rational(budget, 0), Rational(budget, 1)});
  std::size_t offset = 0;
  for (uint32_t i = 0; i <= triangle.degree; ++i) {
    for (std::size_t axis = 0; axis < 4; ++axis)
      result.controls.push_back(triangle.controls[4 * offset + axis]);
    offset += triangle.degree - i + 1;
  }
  return result;
}
V3 plane_lift(const PlaneChart &p, const UV &xy) {
  return {p.origin[0].add(p.basis[0].multiply(xy[0])).add(p.basis[1].multiply(xy[1])),
          p.origin[1].add(p.basis[2].multiply(xy[0])).add(p.basis[3].multiply(xy[1])),
          p.origin[2].add(p.basis[4].multiply(xy[0])).add(p.basis[5].multiply(xy[1]))};
}
std::optional<Net> trace_net(GeometryContext &ctx, const WorkingChart &chart,
                             const TraceIntervalRequest &request,
                             const std::array<Rational, 2> &interval) {
  if (chart.kind == WorkingChartKind::physical_plane_convex_q) {
    if (!chart.plane)
      throw Error(ErrorCode::internal_error, "plane trace lacks admitted chart");
    return affine_net(ctx.budget(), {plane_lift(*chart.plane, chart_at(request, interval[0])),
                      plane_lift(*chart.plane, chart_at(request, interval[1]))});
  }
  const auto a = uv_at(request, interval[0]), b = uv_at(request, interval[1]);
  const auto &geometry = chart.envelope.geometry;
  const auto &carrier = geometry.view().source.view().carriers[chart.envelope.carrier.value];
  if (carrier.kind == 1)
    return affine_net(ctx.budget(),
                      {collapsed_point(enclose_original_surface(ctx, geometry,
                                                               chart.envelope.carrier, a[0], a[1])),
                      collapsed_point(enclose_original_surface(
                          ctx, geometry, chart.envelope.carrier, b[0], b[1]))});
  if (carrier.kind != 6)
    return std::nullopt;
  const auto u = a[0].add(b[0]).divide(Rational(ctx.budget(), 2)),
             v = a[1].add(b[1]).divide(Rational(ctx.budget(), 2));
  const auto root = import_surface_root(ctx.budget(), geometry,
                                        find_surface_root(geometry, chart.envelope.carrier, u, v));
  return tensor_segment(root, a, b);
}
struct PolynomialProduct final {
  uint32_t left, right, index;
};
Rational product_factor(bgfd::Budget &b, PolynomialProduct degree, uint32_t i) {
  const auto j = degree.index - i;
  const auto numerator =
      coefficient_choose({degree.left, i}) * coefficient_choose({degree.right, j});
  return Rational(b, static_cast<int64_t>(numerator))
      .divide(Rational(
          b, static_cast<int64_t>(coefficient_choose({degree.left + degree.right, degree.index}))));
}
Rational coupled(const Net &left, const Net &right, uint32_t bits) {
  auto &b = left.controls[0].budget();
  if (left.degree + right.degree > 38)
    throw Error(ErrorCode::unsupported, "host coupled trace degree exceeded");
  const auto anchor = point(left, false);
  const auto degree = left.degree + right.degree;
  std::pmr::vector<Rational> coefficients(&b), weights(&b);
  coefficients.reserve(static_cast<std::size_t>(degree + 1) * 3);
  weights.reserve(degree + 1);
  for (uint32_t k = 0; k <= degree; ++k) {
    V3 value{Rational(b), Rational(b), Rational(b)};
    Rational weight(b);
    for (uint32_t i = 0; i <= left.degree; ++i) {
      if (k < i || k - i > right.degree)
        continue;
      const auto j = k - i;
      const auto factor = product_factor(b, {left.degree, right.degree, k}, i);
      const auto &w = left.controls[4 * i + 3], &v = right.controls[4 * j + 3];
      weight = weight.add(w.multiply(v).multiply(factor));
      for (std::size_t axis = 0; axis < 3; ++axis) {
        const auto a = left.controls[4 * i + axis].subtract(w.multiply(anchor[axis]));
        const auto c = right.controls[4 * j + axis].subtract(v.multiply(anchor[axis]));
        value[axis] = value[axis].add(a.multiply(v).subtract(c.multiply(w)).multiply(factor));
      }
    }
    for (const auto &coordinate : value)
      coefficients.push_back(coordinate);
    weights.push_back(weight);
  }
  const auto minimum =
      std::min_element(weights.begin(), weights.end(),
                       [](const auto &a, const auto &c) { return a.compare(c) < 0; });
  if (minimum == weights.end() || minimum->sign() <= 0)
    invalid("coupled trace has nonpositive source weight");
  Rational upper(b);
  for (std::size_t i = 0; i < coefficients.size(); i += 3) {
    const std::array<ExactEnclosure, 3> coefficient{
        ExactEnclosure(coefficients[i].divide(*minimum)),
        ExactEnclosure(coefficients[i + 1].divide(*minimum)),
        ExactEnclosure(coefficients[i + 2].divide(*minimum))};
    const auto norm = enclosure_norm_upper(b, coefficient, bits);
    if (norm.compare(upper) > 0)
      upper = norm;
  }
  return upper;
}
struct LineBound final {
  Rational upper, begin, end;
};
std::optional<LineBound> line_bound(std::span<const Net> pieces, const V3 &covector,
                                    const std::array<Rational, 6> &actual, uint32_t bits) {
  if (pieces.empty())
    return std::nullopt;
  auto &b = pieces.front().controls[0].budget();
  const auto pa = dot(covector, point(pieces.front(), false)),
             pb = dot(covector, point(pieces.back(), true));
  const auto delta = pb.subtract(pa);
  if (delta.sign() == 0)
    return std::nullopt;
  const Rational sign(b, delta.sign());
  V3 direction{actual[3].subtract(actual[0]), actual[4].subtract(actual[1]),
               actual[5].subtract(actual[2])};
  Rational upper(b);
  for (const auto &net : pieces) {
    std::pmr::vector<Rational> scalar(&b), weights(&b);
    for (uint32_t i = 0; i <= net.degree; ++i) {
      const auto offset = 4 * static_cast<std::size_t>(i);
      const auto &w = net.controls[offset + 3];
      const V3 numerator{net.controls[offset], net.controls[offset + 1], net.controls[offset + 2]};
      scalar.push_back(dot(covector, numerator).subtract(pa.multiply(w)));
      weights.push_back(w);
    }
    std::pmr::vector<Rational> increasing(&b);
    for (const auto &value : scalar)
      increasing.push_back(value.multiply(sign));
    if (!ratio_increasing(increasing, weights))
      return std::nullopt;
    const auto minw =
        std::min_element(weights.begin(), weights.end(),
                         [](const auto &a, const auto &c) { return a.compare(c) < 0; });
    for (uint32_t i = 0; i <= net.degree; ++i) {
      const auto offset = 4 * static_cast<std::size_t>(i);
      const auto &w = weights[i];
      std::array<ExactEnclosure, 3> residual{
          ExactEnclosure(net.controls[offset]
                             .subtract(w.multiply(actual[0]))
                             .subtract(scalar[i].multiply(direction[0]).divide(delta))
                             .divide(*minw)),
          ExactEnclosure(net.controls[offset + 1]
                             .subtract(w.multiply(actual[1]))
                             .subtract(scalar[i].multiply(direction[1]).divide(delta))
                             .divide(*minw)),
          ExactEnclosure(net.controls[offset + 2]
                             .subtract(w.multiply(actual[2]))
                             .subtract(scalar[i].multiply(direction[2]).divide(delta))
                             .divide(*minw))};
      const auto norm = enclosure_norm_upper(b, residual, bits);
      if (norm.compare(upper) > 0)
        upper = norm;
    }
  }
  return LineBound{upper, pa, pb};
}
void include_cut(std::pmr::vector<Rational> &cuts, const Rational &cut,
                 const TraceIntervalRequest &request) {
  if (cut.compare(request.source_parameter[0]) > 0 && cut.compare(request.source_parameter[1]) < 0)
    cuts.push_back(cut);
}
std::pmr::vector<Rational> partition(GeometryContext &ctx, const WorkingChart &chart,
                                     Slot<CurveTag> curve, const TraceIntervalRequest &request) {
  auto &b = ctx.budget();
  const auto &geometry = chart.envelope.geometry;
  std::pmr::vector<Rational> cuts(&b);
  cuts.push_back(request.source_parameter[0]);
  cuts.push_back(request.source_parameter[1]);
  const auto &record = geometry.view().curves[curve.value];
  for (uint64_t i = 0; i < record.axis.count; ++i)
    include_cut(cuts, import_exact(b, geometry.exact(record.axis.first + i)), request);
  const auto &surface = geometry.view().carriers[chart.envelope.carrier.value];
  if (surface.kind == 6 && chart.kind != WorkingChartKind::physical_plane_convex_q) {
    for (std::size_t axis = 0; axis < 2; ++axis) {
      const auto delta = request.original_uv[axis + 2].subtract(request.original_uv[axis]);
      if (delta.sign() == 0)
        continue;
      const auto range = axis == 0 ? surface.u_axis : surface.v_axis;
      for (uint64_t i = 0; i < range.count; ++i) {
        const auto knot = import_exact(b, geometry.exact(range.first + i));
        const auto s = knot.subtract(request.original_uv[axis]).divide(delta);
        if (s.sign() > 0 && s.compare(Rational(b, 1)) < 0)
          include_cut(cuts, lerp(request.source_parameter[0], request.source_parameter[1], s),
                      request);
      }
    }
  }
  std::sort(cuts.begin(), cuts.end(),
            [](const auto &a, const auto &c) { return a.compare(c) < 0; });
  cuts.erase(std::unique(cuts.begin(), cuts.end(),
                         [](const auto &a, const auto &c) { return a.compare(c) == 0; }),
             cuts.end());
  return cuts;
}
ExactEnclosure trace_component_second(const SurfaceJetEnclosure &jet, const UV &velocity,
                                      std::size_t axis) {
  return sum(
      sum(times(jet.duu[axis], velocity[0].multiply(velocity[0])),
          times(jet.duv[axis],
                velocity[0].multiply(velocity[1]).multiply(Rational(velocity[0].budget(), 2)))),
      times(jet.dvv[axis], velocity[1].multiply(velocity[1])));
}
Rational analytic_coupled(GeometryContext &ctx, const WorkingChart &chart, Slot<CurveTag> curve,
                          const TraceIntervalRequest &request,
                          const std::array<Rational, 2> &interval, const Net *selected) {
  auto &b = ctx.budget();
  const auto &geometry = chart.envelope.geometry;
  const auto a = uv_at(request, interval[0]), c = uv_at(request, interval[1]);
  const std::array<Rational, 4> box{
      a[0].compare(c[0]) < 0 ? a[0] : c[0], a[0].compare(c[0]) < 0 ? c[0] : a[0],
      a[1].compare(c[1]) < 0 ? a[1] : c[1], a[1].compare(c[1]) < 0 ? c[1] : a[1]};
  const auto edge = enclose_original_curve_jet(ctx, geometry, curve, interval[0], interval[1],
                                               request.precision.transcendental_bits);
  const auto width = request.source_parameter[1].subtract(request.source_parameter[0]);
  const auto h = interval[1].subtract(interval[0]);
  const UV velocity{request.original_uv[2].subtract(request.original_uv[0]).divide(width),
                    request.original_uv[3].subtract(request.original_uv[1]).divide(width)};
  const auto trace_second = [&]() -> std::array<ExactEnclosure, 3> {
    if (selected) {
      const auto jet = enclose_rational_curve_jet(*selected, Rational(b, 0), Rational(b, 1));
      const auto scale = Rational(b, 1).divide(h.multiply(h));
      return {times(jet.second[0], scale), times(jet.second[1], scale),
              times(jet.second[2], scale)};
    }
    const auto carrier = enclose_original_surface_jet(ctx, geometry, chart.envelope.carrier, box,
                                                      request.precision.transcendental_bits);
    return {trace_component_second(carrier, velocity, 0),
            trace_component_second(carrier, velocity, 1),
            trace_component_second(carrier, velocity, 2)};
  }();
  std::array<ExactEnclosure, 3> second{difference(edge.second[0], trace_second[0]),
                                       difference(edge.second[1], trace_second[1]),
                                       difference(edge.second[2], trace_second[2])};
  const auto m2 = enclosure_norm_upper(b, second, request.precision.norm_fractional_bits);
  Rational endpoints(b);
  for (std::size_t side = 0; side < 2; ++side) {
    const auto uv = uv_at(request, interval[side]);
    const auto source =
        enclose_original_curve_range(ctx, geometry, curve, {interval[side], interval[side]},
                                     request.precision.transcendental_bits);
    const auto face = [&]() -> std::array<ExactEnclosure, 3> {
      if (selected) {
        const auto xyz = point(*selected, side != 0);
        return {ExactEnclosure(xyz[0]), ExactEnclosure(xyz[1]), ExactEnclosure(xyz[2])};
      }
      return enclose_original_surface_point_jet(ctx, geometry, chart.envelope.carrier, uv[0], uv[1],
                                                request.precision.transcendental_bits)
          .position;
    }();
    const std::array<ExactEnclosure, 3> residual{difference(source.coordinates[0], face[0]),
                                                 difference(source.coordinates[1], face[1]),
                                                 difference(source.coordinates[2], face[2])};
    const auto norm = enclosure_norm_upper(b, residual, request.precision.norm_fractional_bits);
    if (norm.compare(endpoints) > 0)
      endpoints = norm;
  }
  return endpoints.add(m2.multiply(h.multiply(h)).divide(Rational(b, 8)));
}
void validate(GeometryContext &ctx, const WorkingChart &chart,
              const TraceIntervalRequest &request) {
  const auto check = [&](const auto &values) {
    for (const auto &value : values)
      if (&value.budget() != &ctx.budget())
        invalid("trace uses a foreign arithmetic owner");
  };
  check(request.source_parameter);
  check(request.chart_endpoints);
  check(request.original_uv);
  check(request.actual_endpoint_xyz);
  if (&request.positional_allowance.budget() != &ctx.budget() ||
      request.positional_allowance.sign() <= 0 ||
      request.source_parameter[0].compare(request.source_parameter[1]) >= 0)
    invalid("invalid trace interval or positional allowance");
  if (request.precision.transcendental_bits < 32 ||
      request.precision.transcendental_bits > ctx.maximum_bits() ||
      request.precision.norm_fractional_bits > ctx.maximum_bits())
    invalid("invalid trace precision profile");
  const auto &source = chart.envelope.geometry.view().source.view();
  if (request.use.value >= source.coedges.size())
    invalid("trace use outside original source");
  const auto &use = source.coedges[request.use.value];
  if (source.loops[source.loop_index.find(use.loop_id).value].face_id !=
      source.faces[chart.envelope.face.value].id)
    invalid("trace use belongs to a foreign face");
  const auto &edge = source.edges[source.edge_index.find(use.edge_id).value];
  const auto a = Rational::from_double(ctx.budget(), edge.parameter_range[0]),
             b = Rational::from_double(ctx.budget(), edge.parameter_range[1]);
  if (request.source_parameter[0].compare(a.compare(b) < 0 ? a : b) < 0 ||
      request.source_parameter[1].compare(a.compare(b) < 0 ? b : a) > 0)
    invalid("trace interval outside original spatial edge");
  for (std::size_t side = 0; side < 2; ++side) {
    for (std::size_t axis = 0; axis < 2; ++axis) {
      const auto &uv = request.original_uv[2 * side + axis];
      if (uv.compare(chart.envelope.uv[2 * axis]) < 0 ||
          uv.compare(chart.envelope.uv[2 * axis + 1]) > 0)
        throw Error(ErrorCode::unsupported,
                    "selected trace leaves prepared original source envelope");
      if (chart.kind != WorkingChartKind::physical_plane_convex_q &&
          uv.compare(request.chart_endpoints[2 * side + axis]) != 0)
        invalid("ordinary UV trace chart and source sites disagree");
    }
    if(chart.kind==WorkingChartKind::physical_plane_convex_q) {
      if(!chart.plane)throw Error(ErrorCode::internal_error,"plane trace lacks admitted source chart");
      const auto &q=chart.plane->quadrilateral;
      for(std::size_t i=0;i<4;++i) {
        const auto j=(i+1)%4;
        const auto x=request.chart_endpoints[2*side].subtract(q[2*i]);
        const auto y=request.chart_endpoints[2*side+1].subtract(q[2*i+1]);
        if(q[2*j].subtract(q[2*i]).multiply(y).subtract(q[2*j+1].subtract(q[2*i+1]).multiply(x)).sign()<0)
          throw Error(ErrorCode::unsupported,"selected trace leaves physical-plane inverse image Q");
      }
    }
  }
}
} // namespace
TraceIntervalPlan bound_trace_interval(GeometryContext &ctx, const WorkingChart &chart,
                                       const TraceIntervalRequest &request) {
  validate(ctx, chart, request);
  auto &b = ctx.budget();
  const auto &geometry = chart.envelope.geometry;
  const auto &source = geometry.view().source.view();
  const auto &edge =
      source.edges[source.edge_index.find(source.coedges[request.use.value].edge_id).value];
  if (!edge.curve_id)
    throw Error(ErrorCode::unsupported, "curve-free edge requires declared quotient coverage");
  const auto curve = source.curve_index.find(edge.curve_id);
  if (source.curves[curve.value].dimension != 3)
    invalid("trace edge does not name an original spatial curve");
  const auto cuts = partition(ctx, chart, curve, request);
  std::pmr::vector<Net> curves(&b), traces(&b);
  std::pmr::vector<CurveChordPiece> chord_pieces(&b);
  Rational trace_upper(b), analytic_chord(b);
  bool all_rational = true, has_analytic_chord = false;
  for (std::size_t i = 1; i < cuts.size(); ++i) {
    const std::array<Rational, 2> interval{cuts[i - 1], cuts[i]};
    const auto a = xyz_at(request, interval[0]), c = xyz_at(request, interval[1]);
    const std::array<Rational, 6> actual{a[0], a[1], a[2], c[0], c[1], c[2]};
    auto original = curve_net(ctx, geometry, curve, interval),
         selected = trace_net(ctx, chart, request, interval);
    Rational bound(b);
    if (original && selected) {
      bound = coupled(*original, *selected, request.precision.norm_fractional_bits);
      curves.push_back(std::move(*original));
      traces.push_back(std::move(*selected));
    } else {
      all_rational = false;
      bound =
          analytic_coupled(ctx, chart, curve, request, interval, selected ? &*selected : nullptr);
    }
    if (bound.compare(trace_upper) > 0)
      trace_upper = bound;
    if (source.curves[curve.value].kind > 3) {
      const auto midpoint = interval[0].add(interval[1]).divide(Rational(b, 2));
      chord_pieces.push_back({find_curve_root(geometry, curve, midpoint), interval, actual});
    } else {
      const auto upper = analytic_curve_chord_upper(ctx, source.curves[curve.value], interval[0],
                                                    interval[1], actual, request.precision);
      if (upper.compare(analytic_chord) > 0)
        analytic_chord = upper;
      has_analytic_chord = true;
    }
  }
  if (chart.kind == WorkingChartKind::physical_plane_convex_q) {
    if (!chart.plane)
      throw Error(ErrorCode::internal_error, "trace plane descriptor missing");
    trace_upper = trace_upper.add(chart.plane->height_upper);
  }
  std::optional<Rational> chord_upper;
  if (has_analytic_chord)
    chord_upper.emplace(analytic_chord);
  std::optional<GeometricLineMap> line;
  auto correspondence = TraceCorrespondence::affine_parameter;
  if (all_rational && request.allow_geometric_line) {
    const V3 covector{request.actual_endpoint_xyz[3].subtract(request.actual_endpoint_xyz[0]),
                      request.actual_endpoint_xyz[4].subtract(request.actual_endpoint_xyz[1]),
                      request.actual_endpoint_xyz[5].subtract(request.actual_endpoint_xyz[2])};
    auto physical = line_bound(curves, covector, request.actual_endpoint_xyz,
                               request.precision.norm_fractional_bits);
    const auto face = line_bound(traces, covector, request.actual_endpoint_xyz,
                                 request.precision.norm_fractional_bits);
    if (physical && face) {
      auto image_bound = physical->upper.add(face->upper);
      if (chart.kind == WorkingChartKind::physical_plane_convex_q)
        image_bound = image_bound.add(chart.plane->height_upper);
      if (image_bound.compare(trace_upper) <= 0) {
        trace_upper = image_bound;
        correspondence = TraceCorrespondence::geometric_line;
        line.emplace(
            GeometricLineMap{covector, {physical->begin, physical->end}, {face->begin, face->end}});
        chord_upper.emplace(physical->upper);
        chord_pieces.clear();
      }
    }
  }
  return {correspondence, std::move(trace_upper), std::move(chord_upper), std::move(line),
          std::move(chord_pieces)};
}
} // namespace bgmesh
