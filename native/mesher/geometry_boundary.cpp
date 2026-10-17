#include "geometry_boundary.hpp"
#include "geometry_enclosure.hpp"
#include "geometry_inverse.hpp"
#include <algorithm>
#include <cmath>

namespace bgmesh {
namespace {
using namespace geometry_detail;
using UV = std::array<Rational, 2>;
using Point = std::array<ExactEnclosure, 3>;
[[noreturn]] void unresolved(const char *message) { throw Error(ErrorCode::unresolved, message); }
Rational middle(const ExactEnclosure &x) {
  return x.lower.add(x.upper).divide(Rational(x.lower.budget(), 2));
}
Rational chosen(const ExactEnclosure &x) {
  return Rational::from_double(x.lower.budget(), middle(x).to_double());
}
Point vertex_point(bgfd::Budget &budget, const Vertex &vertex) {
  return {ExactEnclosure(Rational::from_double(budget, vertex.xyz[0])),
          ExactEnclosure(Rational::from_double(budget, vertex.xyz[1])),
          ExactEnclosure(Rational::from_double(budget, vertex.xyz[2]))};
}
struct UseSource final {
  const SourceData &source;
  const Coedge &use;
  const Edge &edge;
  Slot<CurveTag> pcurve;
  std::optional<Slot<CurveTag>> spatial;
};
UseSource use_source(const WorkingChart &chart, Slot<CoedgeTag> slot) {
  const auto &source = chart.envelope.geometry.view().source.view();
  if (slot.value >= source.coedges.size())
    throw Error(ErrorCode::internal_error, "boundary use slot outside source");
  const auto &use = source.coedges[slot.value];
  const auto &loop = source.loops[source.loop_index.find(use.loop_id).value];
  if (loop.face_id != source.faces[chart.envelope.face.value].id)
    throw Error(ErrorCode::invalid_source, "boundary use belongs to a foreign face");
  const auto &edge = source.edges[source.edge_index.find(use.edge_id).value];
  std::optional<Slot<CurveTag>> spatial;
  if (edge.curve_id)
    spatial.emplace(source.curve_index.find(edge.curve_id));
  return {source, use, edge, source.curve_index.find(use.curve_id), spatial};
}
Rational mapped_parameter(bgfd::Budget &budget, const UseSource &source, const Rational &t) {
  const auto a = Rational::from_double(budget, source.edge.parameter_range[0]);
  const auto b = Rational::from_double(budget, source.edge.parameter_range[1]);
  if (t.compare(a.compare(b) < 0 ? a : b) < 0 || t.compare(a.compare(b) < 0 ? b : a) > 0)
    throw Error(ErrorCode::invalid_source, "boundary sample outside original spatial edge");
  if (source.use.parameter_map_kind == 1)
    return t;
  if (source.use.parameter_map_kind != 2)
    throw Error(ErrorCode::unsupported, "unproved source parameter continuation map");
  const auto range = source.use.parameter_map;
  const auto map =
      std::span<const Vec2>(source.source.parameter_maps)
          .subspan(static_cast<std::size_t>(range.first), static_cast<std::size_t>(range.count));
  // Source t rows are strictly monotone in the literal original edge direction.
  // Locate the first closed piece endpoint at/after t without scanning all rows.
  const auto direction = a.compare(b) < 0 ? 1 : -1;
  std::size_t first = 1, last = map.size();
  while (first < last) {
    budget.charge(1);
    const auto mid = first + (last - first) / 2;
    const auto x = Rational::from_double(budget, map[mid][0]);
    if (x.compare(t) * direction < 0)
      first = mid + 1;
    else
      last = mid;
  }
  if (first < map.size()) {
    const auto x = Rational::from_double(budget, map[first - 1][0]);
    const auto y = Rational::from_double(budget, map[first][0]);
    if (t.compare(x) * direction >= 0 && t.compare(y) * direction <= 0) {
      const auto p = Rational::from_double(budget, map[first - 1][1]);
      const auto q = Rational::from_double(budget, map[first][1]);
      return p.add(t.subtract(x).multiply(q.subtract(p)).divide(y.subtract(x)));
    }
  }
  throw Error(ErrorCode::invalid_source, "source parameter map does not cover sample");
}
Slot<VertexTag> endpoint_representative(GeometryContext &ctx, const WorkingChart &chart,
                                        const UseSource &source, const UseNodeRequest &request) {
  if (!request.endpoint)
    throw Error(ErrorCode::invalid_source, "missing explicit source endpoint binding");
  const auto &binding = *request.endpoint;
  const auto &snapshot = binding.boundary;
  if (!snapshot || &snapshot->source.view() != &source.source ||
      snapshot->face != chart.envelope.face ||
      snapshot->definition.value >= source.source.definitions.size() ||
      source.source.definitions[snapshot->definition.value].id !=
          source.source.faces[snapshot->face.value].definition_id ||
      binding.original_endpoint > 1 ||
      request.source_parameter.compare(Rational::from_double(
          ctx.budget(), source.edge.parameter_range[binding.original_endpoint])) != 0)
    throw Error(ErrorCode::invalid_source, "explicit source endpoint/forest binding mismatch");
  if (!snapshot->vertex_aliases.empty() &&
      (!snapshot->vertex_alias_owner ||
       snapshot->vertex_aliases.data() != snapshot->vertex_alias_owner->rows.data() ||
       snapshot->vertex_aliases.size() != snapshot->vertex_alias_owner->rows.size()))
    throw Error(ErrorCode::invalid_source, "source alias span lacks its retained immutable owner");
  const auto original =
      source.source.vertex_index.find(source.edge.vertex_ids[binding.original_endpoint]);
  const auto row =
      std::lower_bound(snapshot->vertex_aliases.begin(), snapshot->vertex_aliases.end(), original,
                       [&](const auto &a, auto value) {
                         ctx.budget().charge(1);
                         return a.original < value;
                       });
  if (row == snapshot->vertex_aliases.end() || row->original != original)
    return original;
  if (row->representative.value >= source.source.vertices.size())
    throw Error(ErrorCode::invalid_source, "declared source representative outside vertex pool");
  return row->representative;
}
Rational request_parameter(GeometryContext &ctx, const WorkingChart &chart, const UseSource &source,
                           const UseNodeRequest &request) {
  if (!source.edge.degenerate)
    return mapped_parameter(ctx.budget(), source, request.source_parameter);
  if (!request.endpoint)
    throw Error(ErrorCode::invalid_source, "missing explicit quotient endpoint binding");
  static_cast<void>(endpoint_representative(ctx, chart, source, request));
  // Quotient-only sense composition; ordinary literal t above already contains
  // the original geometric sense and must never apply it a second time.
  return original_quotient_parameter(ctx.budget(), chart.envelope.geometry.view().source,
                                     request.use, request.endpoint->original_endpoint);
}
void validate_request(GeometryContext &ctx, const WorkingChart &chart,
                      const UseNodeRequest &request) {
  if (&request.source_parameter.budget() != &ctx.budget() ||
      &request.node_source_allowance.budget() != &ctx.budget() ||
      request.node_source_allowance.sign() <= 0 || &chart.envelope.uv[0].budget() != &ctx.budget())
    throw Error(ErrorCode::invalid_source, "invalid recovery owner or source allowance");
  if (request.precision.transcendental_bits < 32 ||
      request.precision.transcendental_bits > ctx.maximum_bits() ||
      request.precision.norm_fractional_bits > ctx.maximum_bits())
    throw Error(ErrorCode::invalid_source, "invalid source recovery precision profile");
  if (request.endpoint)
    static_cast<void>(endpoint_representative(ctx, chart, use_source(chart, request.use), request));
}
Point source_curve_point(GeometryContext &ctx, const WorkingChart &chart, Slot<CurveTag> curve,
                         const Rational &parameter, GeometryBoundPrecision precision) {
  return enclose_original_curve_range(ctx, chart.envelope.geometry, curve, {parameter, parameter},
                                      precision.transcendental_bits)
      .coordinates;
}
struct Target final {
  Point point;
  Rational endpoint_curve_upper;
};
Target target(GeometryContext &ctx, const WorkingChart &chart, const UseSource &source,
              const UseNodeRequest &request) {
  auto &budget = ctx.budget();
  const auto a = Rational::from_double(budget, source.edge.parameter_range[0]);
  const auto b = Rational::from_double(budget, source.edge.parameter_range[1]);
  const bool first = request.endpoint ? request.endpoint->original_endpoint == 0
                                      : request.source_parameter.compare(a) == 0,
             last = request.endpoint ? request.endpoint->original_endpoint == 1
                                     : request.source_parameter.compare(b) == 0;
  if (first || last) {
    auto point = vertex_point(
        budget,
        source.source.vertices
            [source.source.vertex_index.find(source.edge.vertex_ids[first ? 0U : 1U]).value]);
    Rational upper(budget, 0);
    if (source.spatial) {
      const auto curve = source_curve_point(ctx, chart, *source.spatial, request.source_parameter,
                                            request.precision);
      std::array<ExactEnclosure, 3> residual{difference(point[0], curve[0]),
                                             difference(point[1], curve[1]),
                                             difference(point[2], curve[2])};
      upper = enclosure_norm_upper(budget, residual, request.precision.norm_fractional_bits);
      if (upper.compare(request.node_source_allowance) > 0)
        unresolved("original vertex to original edge endpoint exceeds node source allowance");
    }
    return {std::move(point), std::move(upper)};
  }
  if (!source.spatial)
    throw Error(ErrorCode::unsupported, "interior sample on curve-free degenerate edge");
  return {
      source_curve_point(ctx, chart, *source.spatial, request.source_parameter, request.precision),
      Rational(budget, 0)};
}
UV guide(GeometryContext &ctx, const WorkingChart &chart, const UseSource &source,
         const Rational &parameter, GeometryBoundPrecision precision) {
  const auto point = source_curve_point(ctx, chart, source.pcurve, parameter, precision);
  auto &budget = ctx.budget();
  UV result{chosen(point[0]), chosen(point[1])};
  const auto &carrier = source.source.carriers[chart.envelope.carrier.value];
  for (std::size_t axis = 0; axis < 2; ++axis) {
    if (source.use.periodic_shift[axis] == 0)
      continue;
    const bool periodic = axis == 0 ? (carrier.kind >= 2 && carrier.kind <= 5) : carrier.kind == 5;
    if (!periodic)
      throw Error(ErrorCode::unsupported, "boundary use lacks original periodic metadata");
    const auto shift = Rational(budget, static_cast<int64_t>(source.use.periodic_shift[axis]) * 2);
    const auto period = times(ctx.pi(precision.transcendental_bits + 64), shift);
    result[axis] = chosen(sum(ExactEnclosure(result[axis]), period));
  }
  return result;
}
bool inside(const WorkingChart &chart, const UV &uv) {
  const auto &bounds = chart.envelope.uv;
  return uv[0].compare(bounds[0]) >= 0 && uv[0].compare(bounds[1]) <= 0 &&
         uv[1].compare(bounds[2]) >= 0 && uv[1].compare(bounds[3]) <= 0;
}
Rational source_residual(GeometryContext &ctx, const WorkingChart &chart, const UV &uv,
                         const Point &point, GeometryBoundPrecision precision) {
  const auto actual =
      enclose_original_surface_point_jet(ctx, chart.envelope.geometry, chart.envelope.carrier,
                                         uv[0], uv[1], precision.transcendental_bits);
  std::array<ExactEnclosure, 3> residual{difference(point[0], actual.position[0]),
                                         difference(point[1], actual.position[1]),
                                         difference(point[2], actual.position[2])};
  return enclosure_norm_upper(ctx.budget(), residual, precision.norm_fractional_bits);
}
struct Locator final {
  GeometryContext &context;
  const WorkingChart &chart;
  const UseSource &source;
  const UseNodeRequest &request;
  const Point &target;
  const UV &seed;
};
bool same_branch(const Locator &locator, const UV &candidate) {
  const auto kind = locator.source.source.carriers[locator.chart.envelope.carrier.value].kind;
  for (std::size_t axis = 0; axis < 2; ++axis) {
    if (!(axis == 0 ? (kind >= 2 && kind <= 5) : kind == 5))
      continue;
    const auto delta = candidate[axis].subtract(locator.seed[axis]);
    const auto absolute = delta.sign() < 0 ? delta.negative() : delta;
    if (absolute.compare(
            locator.context.pi(locator.request.precision.transcendental_bits + 64).lower) >= 0)
      return false;
  }
  return true;
}
// Newton/line-search arithmetic proposes finite dyadic locators only. Every
// accepted value is read again through original source enclosures above.
UV recover(Locator locator) {
  auto &ctx = locator.context;
  auto &budget = ctx.budget();
  UV uv = locator.seed;
  for (std::size_t axis = 0; axis < 2; ++axis) {
    const auto &lo = locator.chart.envelope.uv[2 * axis],
               &hi = locator.chart.envelope.uv[2 * axis + 1];
    if (uv[axis].compare(lo) < 0)
      uv[axis] = Rational::from_double(budget, lo.bounds().upper);
    if (uv[axis].compare(hi) > 0)
      uv[axis] = Rational::from_double(budget, hi.bounds().lower);
  }
  if (!inside(locator.chart, uv))
    unresolved("source locator cannot enter prepared face envelope");
  Rational residual =
      source_residual(ctx, locator.chart, uv, locator.target, locator.request.precision);
  for (uint32_t iteration = 0;; ++iteration) {
    if (residual.compare(locator.request.node_source_allowance) <= 0 && same_branch(locator, uv))
      return uv;
    if (iteration >= locator.request.locator_limits.iterations)
      break;
    const auto jet = enclose_original_surface_point_jet(
        ctx, locator.chart.envelope.geometry, locator.chart.envelope.carrier, uv[0], uv[1],
        locator.request.precision.transcendental_bits);
    std::array<double, 3> r{}, a{}, b{};
    for (std::size_t axis = 0; axis < 3; ++axis) {
      r[axis] = middle(difference(locator.target[axis], jet.position[axis])).to_double();
      a[axis] = middle(jet.du[axis]).to_double();
      b[axis] = middle(jet.dv[axis]).to_double();
    }
    double aa = 0, ab = 0, bb = 0, ar = 0, br = 0;
    for (std::size_t axis = 0; axis < 3; ++axis) {
      aa += a[axis] * a[axis];
      ab += a[axis] * b[axis];
      bb += b[axis] * b[axis];
      ar += a[axis] * r[axis];
      br += b[axis] * r[axis];
    }
    const auto determinant = aa * bb - ab * ab;
    if (!(determinant > 0) || !std::isfinite(determinant))
      break;
    const auto du = (bb * ar - ab * br) / determinant, dv = (aa * br - ab * ar) / determinant;
    if (!std::isfinite(du) || !std::isfinite(dv))
      break;
    const auto u0 = uv[0].to_double(), v0 = uv[1].to_double();
    bool accepted = false;
    double scale = 1;
    for (uint32_t backtrack = 0; backtrack < locator.request.locator_limits.backtracks;
         ++backtrack) {
      const auto u = u0 + scale * du, v = v0 + scale * dv;
      scale *= .5;
      if (!std::isfinite(u) || !std::isfinite(v))
        continue;
      UV candidate{Rational::from_double(budget, u), Rational::from_double(budget, v)};
      if (!inside(locator.chart, candidate) || !same_branch(locator, candidate))
        continue;
      const auto next =
          source_residual(ctx, locator.chart, candidate, locator.target, locator.request.precision);
      if (next.compare(residual) >= 0)
        continue;
      uv = std::move(candidate);
      residual = next;
      accepted = true;
      break;
    }
    if (!accepted)
      break;
  }
  unresolved(
      "original source locator cannot satisfy node allowance within admitted branch and limits");
}
UV chart_coordinate(GeometryContext &ctx, const WorkingChart &chart, const Point &point,
                    const UV &uv) {
  if (chart.kind != WorkingChartKind::physical_plane_convex_q)
    return uv;
  if (!chart.plane)
    throw Error(ErrorCode::internal_error, "physical plane chart has no descriptor");
  const auto &p = *chart.plane;
  std::array<ExactEnclosure, 3> delta{difference(point[0], ExactEnclosure(p.origin[0])),
                                      difference(point[1], ExactEnclosure(p.origin[1])),
                                      difference(point[2], ExactEnclosure(p.origin[2]))};
  UV result{Rational(ctx.budget()), Rational(ctx.budget())};
  for (std::size_t axis = 0; axis < 2; ++axis) {
    const auto offset = axis * 3;
    result[axis] = chosen(
        sum(sum(times(delta[0], p.projection[offset]), times(delta[1], p.projection[offset + 1])),
            times(delta[2], p.projection[offset + 2])));
  }
  const auto &q = p.quadrilateral;
  for (std::size_t i = 0; i < 4; ++i) {
    const auto j = (i + 1) % 4;
    if (q[2 * j]
            .subtract(q[2 * i])
            .multiply(result[1].subtract(q[2 * i + 1]))
            .subtract(q[2 * j + 1].subtract(q[2 * i + 1]).multiply(result[0].subtract(q[2 * i])))
            .sign() < 0)
      unresolved("selected physical plane node is outside admitted inverse image Q");
  }
  return result;
}
Slot<VertexTag> endpoint_vertex(GeometryContext &ctx, const WorkingChart &chart,
                                const UseSource &source, const UseNodeRequest &request) {
  if (request.endpoint)
    return endpoint_representative(ctx, chart, source, request);
  auto &budget = ctx.budget();
  const auto &t = request.source_parameter;
  if (t.compare(Rational::from_double(budget, source.edge.parameter_range[0])) == 0)
    return source.source.vertex_index.find(source.edge.vertex_ids[0]);
  if (t.compare(Rational::from_double(budget, source.edge.parameter_range[1])) == 0)
    return source.source.vertex_index.find(source.edge.vertex_ids[1]);
  throw Error(ErrorCode::invalid_source, "face joint request is not an original edge endpoint");
}
void authoritative_agreement(GeometryContext &ctx, const WorkingChart &chart,
                             const UseNodeRequest &first, const UseNodeRequest &second) {
  const auto a = use_source(chart, first.use), b = use_source(chart, second.use);
  const auto s = request_parameter(ctx, chart, a, first);
  const auto t = request_parameter(ctx, chart, b, second);
  if (a.pcurve == b.pcurve && s.compare(t) == 0 && a.use.periodic_shift == b.use.periodic_shift)
    return;
  if (a.use.periodic_shift != b.use.periodic_shift)
    unresolved("different authoritative endpoint lifts have no established common joint");
  const auto p = source_curve_point(ctx, chart, a.pcurve, s, first.precision);
  const auto q = source_curve_point(ctx, chart, b.pcurve, t, second.precision);
  for (std::size_t axis = 0; axis < 2; ++axis) {
    if (p[axis].lower.compare(p[axis].upper) != 0 || q[axis].lower.compare(q[axis].upper) != 0)
      unresolved("distinct analytic authoritative endpoint descriptors need an equality identity");
    if (p[axis].lower.compare(q[axis].lower) != 0)
      throw Error(ErrorCode::invalid_source,
                  "authoritative pcurve endpoints do not form a common face joint");
  }
}
std::optional<Point> canonical_target(GeometryContext &ctx, const WorkingChart &chart,
                                      const UseSource &source, const UseNodeRequest &request) {
  if (!request.endpoint)
    return std::nullopt;
  const auto representative = endpoint_representative(ctx, chart, source, request);
  const auto original =
      source.source.vertex_index.find(source.edge.vertex_ids[request.endpoint->original_endpoint]);
  if (original == representative)
    return std::nullopt;
  return vertex_point(ctx.budget(), source.source.vertices[representative.value]);
}
struct CanonicalChecks final {
  std::optional<Rational> source, carrier;
};
CanonicalChecks canonical_checks(GeometryContext &ctx, const WorkingChart &chart,
                                 const UseSource &source, const UseNodeRequest &request,
                                 const Point &original, const UV &uv) {
  const auto q = canonical_target(ctx, chart, source, request);
  if (!q)
    return {};
  const auto norm = [&](const Point &other) {
    const Point delta{difference((*q)[0], other[0]), difference((*q)[1], other[1]),
                      difference((*q)[2], other[2])};
    return enclosure_norm_upper(ctx.budget(), delta, request.precision.norm_fractional_bits);
  };
  auto source_upper = norm(original);
  if (source.spatial) {
    const auto curve = source_curve_point(ctx, chart, *source.spatial, request.source_parameter,
                                          request.precision);
    const auto upper = norm(curve);
    if (upper.compare(source_upper) > 0)
      source_upper = upper;
  }
  const auto carrier_upper = source_residual(ctx, chart, uv, *q, request.precision);
  if (source_upper.compare(request.node_source_allowance) > 0 ||
      carrier_upper.compare(request.node_source_allowance) > 0)
    unresolved(
        "declared canonical source representative exceeds independent node source allowance");
  return {std::move(source_upper), carrier_upper};
}
} // namespace
Rational original_quotient_parameter(bgfd::Budget &budget, const SourceStore &store,
                                     Slot<CoedgeTag> slot, uint32_t original_endpoint) {
  const auto &source = store.view();
  if (slot.value >= source.coedges.size() || original_endpoint > 1)
    throw Error(ErrorCode::invalid_source, "quotient occurrence endpoint outside source");
  const auto &use = source.coedges[slot.value];
  const auto &edge = source.edges[source.edge_index.find(use.edge_id).value];
  if (!edge.degenerate)
    throw Error(ErrorCode::invalid_source, "ordinary use passed to collapsed pcurve mapping");
  const auto from = (original_endpoint == 0) == edge.curve_same_sense ? 0U : 1U;
  return Rational::from_double(budget, use.parameter_range[from]);
}
Rational map_original_use_parameter(GeometryContext &ctx, const WorkingChart &chart,
                                    OriginalUsePoint point) {
  if (&point.source_parameter.budget() != &ctx.budget())
    throw Error(ErrorCode::invalid_source, "source parameter map owner mismatch");
  return mapped_parameter(ctx.budget(), use_source(chart, point.use), point.source_parameter);
}
void require_authoritative_point_agreement(GeometryContext &ctx, const WorkingChart &chart,
                                           std::array<OriginalUsePoint, 2> points,
                                           GeometryBoundPrecision precision) {
  const UseNodeRequest a{
      points[0].use, points[0].source_parameter, Rational(ctx.budget(), 1), precision, {}};
  const UseNodeRequest b{
      points[1].use, points[1].source_parameter, Rational(ctx.budget(), 1), precision, {}};
  validate_request(ctx, chart, a);
  validate_request(ctx, chart, b);
  if (use_source(chart, a.use).use.reconstructed || use_source(chart, b.use).use.reconstructed)
    throw Error(ErrorCode::invalid_source,
                "authoritative point comparison received reconstructed guide");
  authoritative_agreement(ctx, chart, a, b);
}
BoundarySiteChoice locate_boundary_site(GeometryContext &ctx, const WorkingChart &chart,
                                        const UseNodeRequest &request) {
  validate_request(ctx, chart, request);
  const auto source = use_source(chart, request.use);
  const auto parameter = request_parameter(ctx, chart, source, request);
  const auto original = target(ctx, chart, source, request);
  const auto seed = guide(ctx, chart, source, parameter, request.precision);
  const auto canonical = canonical_target(ctx, chart, source, request);
  const auto &proposal = canonical ? *canonical : original.point;
  auto uv = recover({ctx, chart, source, request, proposal, seed});
  const auto xy = chart_coordinate(ctx, chart, proposal, uv);
  std::optional<std::array<Rational, 4>> enclosure;
  if (chart.kind == WorkingChartKind::physical_plane_convex_q) {
    const auto inverse = recover_plane_chart_locator(ctx, chart, xy, request.precision);
    uv = inverse.chosen;
    enclosure.emplace(inverse.enclosure);
  }
  const auto residual = source_residual(ctx, chart, uv, original.point, request.precision);
  if (residual.compare(request.node_source_allowance) > 0)
    unresolved("chosen inverse locator exceeds original node source allowance");
  const auto canonical_bounds = canonical_checks(ctx, chart, source, request, original.point, uv);
  return {request.use,
          request.source_parameter,
          parameter,
          xy,
          uv,
          residual,
          original.endpoint_curve_upper,
          std::move(enclosure),
          canonical_bounds.source,
          canonical_bounds.carrier};
}
BoundaryJointChoice choose_boundary_joint(GeometryContext &ctx, const WorkingChart &chart,
                                          std::span<const UseNodeRequest> requests) {
  if (requests.empty())
    throw Error(ErrorCode::invalid_source, "empty face joint request");
  // Admit every occurrence before selecting one or comparing any owner-affine
  // values. An unselected request is still part of this common joint.
  for (const auto &item : requests) {
    validate_request(ctx, chart, item);
    static_cast<void>(request_parameter(ctx, chart, use_source(chart, item.use), item));
  }
  std::size_t selected = 0;
  std::optional<std::size_t> authoritative;
  const auto vertex = endpoint_vertex(ctx, chart, use_source(chart, requests[0].use), requests[0]);
  for (std::size_t i = 0; i < requests.size(); ++i) {
    const auto source = use_source(chart, requests[i].use);
    if (endpoint_vertex(ctx, chart, source, requests[i]) != vertex)
      throw Error(ErrorCode::invalid_source,
                  "incident face joint occurrences name different original vertices");
    if (!source.use.reconstructed) {
      if (authoritative)
        authoritative_agreement(ctx, chart, requests[*authoritative], requests[i]);
      else {
        authoritative.emplace(i);
        selected = i;
      }
    }
  }
  auto request = requests[selected];
  for (const auto &item : requests)
    if (item.node_source_allowance.compare(request.node_source_allowance) < 0)
      request.node_source_allowance = item.node_source_allowance;
  const auto chosen_site = locate_boundary_site(ctx, chart, request);
  std::pmr::vector<BoundarySiteChoice> occurrences(&ctx.budget());
  occurrences.reserve(requests.size());
  for (const auto &item : requests) {
    const auto source = use_source(chart, item.use);
    const auto parameter = request_parameter(ctx, chart, source, item);
    const auto original = target(ctx, chart, source, item);
    const auto seed = guide(ctx, chart, source, parameter, item.precision);
    if (!same_branch({ctx, chart, source, item, original.point, seed}, chosen_site.original_uv))
      unresolved("incident periodic uses do not admit one common face-vertex branch");
    const auto residual =
        source_residual(ctx, chart, chosen_site.original_uv, original.point, item.precision);
    if (residual.compare(item.node_source_allowance) > 0)
      unresolved("common face-vertex joint exceeds an incident node source allowance");
    const auto canonical_bounds =
        canonical_checks(ctx, chart, source, item, original.point, chosen_site.original_uv);
    occurrences.push_back({item.use, item.source_parameter, parameter, chosen_site.chart,
                           chosen_site.original_uv, residual, original.endpoint_curve_upper,
                           chosen_site.original_uv_enclosure, canonical_bounds.source,
                           canonical_bounds.carrier});
  }
  return {vertex, chosen_site.chart, chosen_site.original_uv, std::move(occurrences),
          chosen_site.original_uv_enclosure};
}
} // namespace bgmesh
