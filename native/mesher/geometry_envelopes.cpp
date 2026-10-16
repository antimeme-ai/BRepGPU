#include "geometry_charts.hpp"
#include "geometry_enclosure.hpp"
#include <algorithm>
#include <bit>

namespace bgmesh {
namespace {
using namespace geometry_detail;
using Box = std::array<Rational, 6>;
using UVBox = std::array<Rational, 4>;
using Covector = std::array<Rational, 3>;
[[noreturn]] void invalid(const char *message) { throw Error(ErrorCode::invalid_source, message); }
[[noreturn]] void unsupported(const char *message) { throw Error(ErrorCode::unsupported, message); }
template <class T> std::span<const T> slice(const std::pmr::vector<T> &pool, Range<T> range) {
  if (range.first > pool.size() || range.count > pool.size() - range.first)
    throw Error(ErrorCode::internal_error, "source envelope range outside retained pool");
  return std::span<const T>(pool).subspan(static_cast<std::size_t>(range.first),
                                          static_cast<std::size_t>(range.count));
}
template <std::size_t N> void unite(std::array<Rational, N> &a, const std::array<Rational, N> &b) {
  static_assert(N % 2 == 0);
  for (std::size_t i = 0; i < N; i += 2) {
    if (b[i].compare(a[i]) < 0)
      a[i] = b[i];
    if (b[i + 1].compare(a[i + 1]) > 0)
      a[i + 1] = b[i + 1];
  }
}
Box point_box(bgfd::Budget &b, const Vec3 &point) {
  return {Rational::from_double(b, point[0]), Rational::from_double(b, point[0]),
          Rational::from_double(b, point[1]), Rational::from_double(b, point[1]),
          Rational::from_double(b, point[2]), Rational::from_double(b, point[2])};
}
Box point_box(const EnclosedPoint &point) {
  return {point.coordinates[0].lower, point.coordinates[0].upper, point.coordinates[1].lower,
          point.coordinates[1].upper, point.coordinates[2].lower, point.coordinates[2].upper};
}
void unite(std::optional<Box> &box, const Box &next) {
  if (box)
    unite(*box, next);
  else
    box.emplace(next);
}
std::array<Rational, 2> ordered(const Rational &a, const Rational &b) {
  return a.compare(b) <= 0 ? std::array<Rational, 2>{a, b} : std::array<Rational, 2>{b, a};
}
void precision(GeometryContext &ctx, GeometryBoundPrecision profile) {
  if (profile.transcendental_bits < 32 || profile.transcendental_bits > ctx.maximum_bits() ||
      profile.norm_fractional_bits > ctx.maximum_bits())
    invalid("invalid source envelope precision profile");
}
std::array<Rational, 2> mapped_range(bgfd::Budget &b, const SourceData &source, const Coedge &use,
                                     const Edge &edge) {
  if (edge.degenerate)
    return ordered(Rational::from_double(b, use.parameter_range[0]),
                   Rational::from_double(b, use.parameter_range[1]));
  const auto begin = Rational::from_double(b, edge.parameter_range[0]),
             end = Rational::from_double(b, edge.parameter_range[1]);
  const auto domain = ordered(begin, end);
  if (use.parameter_map_kind == 1)
    return domain;
  if (use.parameter_map_kind != 2)
    unsupported("unproved source parameter continuation map");
  const auto map = slice(source.parameter_maps, use.parameter_map);
  if (map.size() < 2)
    invalid("missing source parameter map coverage");
  const auto total =
      ordered(Rational::from_double(b, map.front()[0]), Rational::from_double(b, map.back()[0]));
  if (total[0].compare(domain[0]) > 0 || total[1].compare(domain[1]) < 0)
    invalid("source parameter map does not cover its physical edge");
  std::optional<std::array<Rational, 2>> image;
  for (std::size_t i = 1; i < map.size(); ++i) {
    const auto x0 = Rational::from_double(b, map[i - 1][0]),
               x1 = Rational::from_double(b, map[i][0]);
    const auto y0 = Rational::from_double(b, map[i - 1][1]),
               y1 = Rational::from_double(b, map[i][1]);
    const auto interval = ordered(x0, x1);
    const auto lo = interval[0].compare(domain[0]) > 0 ? interval[0] : domain[0];
    const auto hi = interval[1].compare(domain[1]) < 0 ? interval[1] : domain[1];
    if (lo.compare(hi) > 0)
      continue;
    const auto evaluate = [&](const Rational &t) {
      return y0.add(t.subtract(x0).multiply(y1.subtract(y0)).divide(x1.subtract(x0)));
    };
    const auto range = ordered(evaluate(lo), evaluate(hi));
    if (image)
      unite(*image, range);
    else
      image.emplace(range);
  }
  if (!image)
    invalid("source parameter map omitted its physical interval");
  return *image;
}
UVBox shifted_uv(GeometryContext &ctx, const Carrier &carrier, const Coedge &use,
                 const EnclosedPoint &point, GeometryBoundPrecision profile) {
  auto &b = ctx.budget();
  UVBox box{point.coordinates[0].lower, point.coordinates[0].upper, point.coordinates[1].lower,
            point.coordinates[1].upper};
  for (std::size_t axis = 0; axis < 2; ++axis) {
    const auto shift = use.periodic_shift[axis];
    if (shift == 0)
      continue;
    const bool periodic = axis == 0 ? (carrier.kind >= 2 && carrier.kind <= 5) : carrier.kind == 5;
    if (!periodic)
      unsupported("source carrier lacks the requested periodic metadata");
    const auto magnitude = shift < 0 ? static_cast<uint64_t>(-static_cast<int64_t>(shift))
                                     : static_cast<uint64_t>(shift);
    const auto required =
        profile.transcendental_bits + 32 + static_cast<uint32_t>(std::bit_width(magnitude));
    const auto offset = times(ctx.pi(required), Rational(b, static_cast<int64_t>(shift) * 2));
    box[2 * axis] = box[2 * axis].add(offset.lower);
    box[2 * axis + 1] = box[2 * axis + 1].add(offset.upper);
  }
  return box;
}
Covector frame_axis(bgfd::Budget &b, const Frame &frame, std::size_t offset) {
  return {Rational::from_double(b, frame[offset]), Rational::from_double(b, frame[offset + 1]),
          Rational::from_double(b, frame[offset + 2])};
}
Rational dot(const Covector &a, const Covector &b) {
  return a[0].multiply(b[0]).add(a[1].multiply(b[1])).add(a[2].multiply(b[2]));
}
Covector cross(const Covector &a, const Covector &b) {
  return {a[1].multiply(b[2]).subtract(a[2].multiply(b[1])),
          a[2].multiply(b[0]).subtract(a[0].multiply(b[2])),
          a[0].multiply(b[1]).subtract(a[1].multiply(b[0]))};
}
Covector divided(const Covector &a, const Rational &d) {
  return {a[0].divide(d), a[1].divide(d), a[2].divide(d)};
}
std::optional<std::array<Covector, 2>> plane_duals(bgfd::Budget &b, const Carrier &carrier) {
  const auto x = frame_axis(b, carrier.frame, 3), y = frame_axis(b, carrier.frame, 6);
  const auto xx = dot(x, x), xy = dot(x, y), yy = dot(y, y),
             det = xx.multiply(yy).subtract(xy.multiply(xy));
  if (det.sign() <= 0)
    return std::nullopt;
  const auto dual = [&](const Rational &a, const Covector &p, const Rational &c,
                        const Covector &q) {
    return Covector{a.multiply(p[0]).subtract(c.multiply(q[0])).divide(det),
                    a.multiply(p[1]).subtract(c.multiply(q[1])).divide(det),
                    a.multiply(p[2]).subtract(c.multiply(q[2])).divide(det)};
  };
  return std::array<Covector, 2>{dual(yy, x, xy, y), dual(xx, y, xy, x)};
}
std::optional<Covector> height_dual(bgfd::Budget &b, const Carrier &carrier) {
  const auto x = frame_axis(b, carrier.frame, 3), y = frame_axis(b, carrier.frame, 6),
             z = frame_axis(b, carrier.frame, 9);
  const auto numerator = cross(x, y);
  const auto det = dot(numerator, z);
  if (det.sign() == 0)
    return std::nullopt;
  return divided(numerator, det);
}
ExactEnclosure coordinate(bgfd::Budget &b, const Box &box, const Carrier &carrier,
                          const Covector &dual) {
  // Scalars are copied through successful temporaries; no retained numerical
  // state can escape on an arithmetic failure.
  Rational lo(b, 0), hi(b, 0);
  for (std::size_t axis = 0; axis < 3; ++axis) {
    const auto origin = Rational::from_double(b, carrier.frame[axis]);
    const auto term =
        times(ExactEnclosure(box[2 * axis].subtract(origin), box[2 * axis + 1].subtract(origin)),
              dual[axis]);
    lo = lo.add(term.lower);
    hi = hi.add(term.upper);
  }
  return {std::move(lo), std::move(hi)};
}
Box edge_box(GeometryContext &ctx, const GeometryStore &geometry, const Edge &edge,
             GeometryBoundPrecision profile) {
  auto &b = ctx.budget();
  const auto &source = geometry.view().source.view();
  auto box = point_box(b, source.vertices[source.vertex_index.find(edge.vertex_ids[0]).value].xyz);
  unite(box, point_box(b, source.vertices[source.vertex_index.find(edge.vertex_ids[1]).value].xyz));
  if (edge.curve_id) {
    const auto range = ordered(Rational::from_double(b, edge.parameter_range[0]),
                               Rational::from_double(b, edge.parameter_range[1]));
    unite(box, point_box(enclose_original_curve_range(ctx, geometry,
                                                      source.curve_index.find(edge.curve_id), range,
                                                      profile.transcendental_bits)));
  }
  return box;
}
Box surface_box(GeometryContext &ctx, const GeometryStore &geometry, Slot<CarrierTag> carrier_slot,
                const UVBox &uv, GeometryBoundPrecision profile) {
  auto &b = ctx.budget();
  const auto &carrier = geometry.view().source.view().carriers[carrier_slot.value];
  if (carrier.kind != 6) {
    const auto jet = enclose_analytic_surface_jet(ctx, carrier, uv, profile.transcendental_bits);
    return point_box({3, jet.position});
  }
  const auto roots = intersecting_surface_roots(b, geometry, carrier_slot, uv);
  std::optional<Box> total;
  for (const auto slot : roots) {
    const auto root = import_surface_root(b, geometry, slot);
    std::optional<Box> image;
    for (std::size_t i = 0; i < root.controls.size(); i += 4) {
      const auto x = root.controls[i].divide(root.controls[i + 3]),
                 y = root.controls[i + 1].divide(root.controls[i + 3]),
                 z = root.controls[i + 2].divide(root.controls[i + 3]);
      unite(image, {x, x, y, y, z, z});
    }
    if (!image)
      throw Error(ErrorCode::internal_error, "empty original carrier root image");
    unite(total, *image);
  }
  if (!total)
    throw Error(ErrorCode::internal_error, "prepared original carrier rectangle has no roots");
  return *total;
}
} // namespace

FaceEnvelope::FaceEnvelope(const GeometryStore &source, Slot<FaceTag> face_slot,
                           Slot<CarrierTag> carrier_slot, uint64_t revision,
                           std::array<Rational, 4> bounds)
    : geometry(source), face(face_slot), carrier(carrier_slot), generation(revision),
      uv(std::move(bounds)), uses(&uv[0].budget()) {}
FaceEnvelope::FaceEnvelope(const FaceEnvelope &other)
    : geometry(other.geometry), face(other.face), carrier(other.carrier),
      generation(other.generation), uv(other.uv),
      uses(other.uses, other.uses.get_allocator().resource()) {}
DefinitionEnvelope::DefinitionEnvelope(const GeometryStore &source,
                                       Slot<DefinitionTag> definition_slot, uint64_t revision,
                                       std::array<Rational, 6> bounds)
    : geometry(source), definition(definition_slot), generation(revision), xyz(std::move(bounds)),
      faces(&xyz[0].budget()) {}
DefinitionEnvelope::DefinitionEnvelope(const DefinitionEnvelope &other)
    : geometry(other.geometry), definition(other.definition), generation(other.generation),
      xyz(other.xyz), faces(other.faces, other.faces.get_allocator().resource()) {}

FaceEnvelope prepare_face_envelope(GeometryContext &ctx, const GeometryStore &geometry,
                                   const FaceEnvelopeRequest &request) {
  precision(ctx, request.precision);
  auto &b = ctx.budget();
  const auto &source = geometry.view().source.view();
  if (request.face.value >= source.faces.size())
    throw Error(ErrorCode::internal_error, "face envelope slot outside original source");
  const auto &face = source.faces[request.face.value];
  const auto carrier_slot = source.carrier_index.find(face.carrier_id);
  const auto &carrier = source.carriers[carrier_slot.value];
  UVBox uv{Rational::from_double(b, carrier.uv_bounds[0]),
           Rational::from_double(b, carrier.uv_bounds[1]),
           Rational::from_double(b, carrier.uv_bounds[2]),
           Rational::from_double(b, carrier.uv_bounds[3])};
  if (carrier.kind == 6) {
    const auto &record = geometry.view().carriers[carrier_slot.value];
    uv = {import_exact(b, geometry.exact(record.u_axis.first)),
          import_exact(b, geometry.exact(record.u_axis.first + record.u_axis.count - 1)),
          import_exact(b, geometry.exact(record.v_axis.first)),
          import_exact(b, geometry.exact(record.v_axis.first + record.v_axis.count - 1))};
  }
  FaceEnvelope out(geometry, request.face, carrier_slot, request.generation, std::move(uv));
  const auto plane = carrier.kind == 1 ? plane_duals(b, carrier) : std::nullopt;
  const auto height =
      (carrier.kind == 2 || carrier.kind == 3) ? height_dual(b, carrier) : std::nullopt;
  for (const auto loop_id : slice(source.loop_ids, face.loops)) {
    const auto &loop = source.loops[source.loop_index.find(loop_id).value];
    for (const auto use_id : slice(source.coedge_ids, loop.coedges)) {
      b.charge(1);
      const auto use_slot = source.coedge_index.find(use_id);
      const auto &use = source.coedges[use_slot.value];
      const auto &edge = source.edges[source.edge_index.find(use.edge_id).value];
      const auto mapped = mapped_range(b, source, use, edge);
      const auto image =
          enclose_original_curve_range(ctx, geometry, source.curve_index.find(use.curve_id), mapped,
                                       request.precision.transcendental_bits);
      const auto bounds = shifted_uv(ctx, carrier, use, image, request.precision);
      out.uses.push_back({use_slot, mapped, bounds});
      if (carrier.kind != 6)
        unite(out.uv, bounds);
      if (plane || height) {
        const auto physical = edge_box(ctx, geometry, edge, request.precision);
        if (plane)
          for (std::size_t axis = 0; axis < 2; ++axis) {
            const auto inverse = coordinate(b, physical, carrier, (*plane)[axis]);
            if (inverse.lower.compare(out.uv[2 * axis]) < 0)
              out.uv[2 * axis] = inverse.lower;
            if (inverse.upper.compare(out.uv[2 * axis + 1]) > 0)
              out.uv[2 * axis + 1] = inverse.upper;
          }
        if (height) {
          const auto value = coordinate(b, physical, carrier, *height);
          std::optional<ExactEnclosure> inverse;
          if (carrier.kind == 2)
            inverse.emplace(value);
          else {
            const auto angle = Rational::from_double(b, carrier.radii[1]);
            const auto trig = trig_range(ctx, angle, angle, request.precision.transcendental_bits);
            if (trig.cosine.lower.sign() > 0)
              inverse.emplace(quotient(value, trig.cosine));
            else if (trig.cosine.upper.sign() < 0)
              inverse.emplace(quotient(negate(value), negate(trig.cosine)));
          }
          // This inverse range only expands preparation when its denominator
          // is proved nonzero. Recovery may use the retained guide envelope;
          // any later site outside it requires expansion or refusal.
          if (inverse) {
            if (inverse->lower.compare(out.uv[2]) < 0)
              out.uv[2] = inverse->lower;
            if (inverse->upper.compare(out.uv[3]) > 0)
              out.uv[3] = inverse->upper;
          }
        }
      }
      if (edge.degenerate && carrier.kind == 4) {
        const auto half = times(ctx.pi(request.precision.transcendental_bits + 32),
                                Rational(b, 1).divide(Rational(b, 2)));
        const UVBox caps{out.uv[0], out.uv[1], half.upper.negative(), half.upper};
        unite(out.uv, caps);
      } else if (edge.degenerate && carrier.kind == 3) {
        const auto angle = Rational::from_double(b, carrier.radii[1]);
        if (angle.sign() == 0)
          unsupported("zero cone angle has no isolated declared apex");
        const auto trig = trig_range(ctx, angle, angle, request.precision.transcendental_bits);
        const auto numerator = ExactEnclosure(Rational::from_double(b, -carrier.radii[0]));
        std::optional<ExactEnclosure> apex;
        if (trig.sine.lower.sign() > 0)
          apex.emplace(quotient(numerator, trig.sine));
        else if (trig.sine.upper.sign() < 0)
          apex.emplace(quotient(negate(numerator), negate(trig.sine)));
        if (!apex)
          throw Error(ErrorCode::unresolved,
                      "declared cone apex is unresolved at source envelope precision");
        const UVBox cap{out.uv[0], out.uv[1], apex->lower, apex->upper};
        unite(out.uv, cap);
      }
    }
  }
  return out;
}
DefinitionEnvelope prepare_definition_envelope(GeometryContext &ctx, const GeometryStore &geometry,
                                               const DefinitionEnvelopeRequest &request) {
  precision(ctx, request.precision);
  auto &b = ctx.budget();
  const auto &source = geometry.view().source.view();
  if (request.definition.value >= source.definitions.size())
    throw Error(ErrorCode::internal_error, "definition envelope slot outside original source");
  const auto &definition = source.definitions[request.definition.value];
  std::optional<Box> total;
  for (const auto id : slice(source.vertex_ids, definition.vertices))
    unite(total, point_box(b, source.vertices[source.vertex_index.find(id).value].xyz));
  for (const auto id : slice(source.edge_ids, definition.edges))
    unite(total, edge_box(ctx, geometry, source.edges[source.edge_index.find(id).value],
                          request.precision));
  std::pmr::vector<FaceEnvelope> faces(&b);
  for (const auto id : slice(source.face_ids, definition.faces)) {
    auto face = prepare_face_envelope(ctx, geometry,
                                      {.face = source.face_index.find(id),
                                       .generation = request.generation,
                                       .precision = request.precision});
    unite(total, surface_box(ctx, geometry, face.carrier, face.uv, request.precision));
    faces.push_back(std::move(face));
  }
  if (!total)
    unsupported("source definition has no physical geometry envelope");
  DefinitionEnvelope out(geometry, request.definition, request.generation, *total);
  out.faces.swap(faces);
  return out;
}
} // namespace bgmesh
