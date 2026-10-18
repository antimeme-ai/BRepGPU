#include "adaptive_assembly.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>

namespace bgmesh::adaptive {
namespace {
using Box = std::array<Rational, 6>;
using UVBox = std::array<Rational, 4>;
using Axis = std::array<Rational, 3>;
struct AxisShape final {
  uint32_t degree;
  uint64_t controls;
};
[[noreturn]] void invalid(const char *message) { throw Error(ErrorCode::invalid_source, message); }
[[noreturn]] void unsupported(const char *message) { throw Error(ErrorCode::unsupported, message); }
template <std::size_t N> void unite(std::array<Rational, N> &a, const std::array<Rational, N> &b) {
  for (std::size_t i = 0; i < N; i += 2) {
    if (b[i].compare(a[i]) < 0)
      a[i] = b[i];
    if (b[i + 1].compare(a[i + 1]) > 0)
      a[i + 1] = b[i + 1];
  }
}
void unite(std::optional<Box> &a, const Box &b) {
  if (a)
    unite(*a, b);
  else
    a.emplace(b);
}
Rational abs(const Rational &value) { return value.sign() < 0 ? value.negative() : value; }
std::array<Rational, 2> ordered(const Rational &a, const Rational &b) {
  return a.compare(b) <= 0 ? std::array{a, b} : std::array{b, a};
}
Box point_box(bgfd::Budget &budget, const Vec3 &point) {
  return {Rational::from_double(budget, point[0]), Rational::from_double(budget, point[0]),
          Rational::from_double(budget, point[1]), Rational::from_double(budget, point[1]),
          Rational::from_double(budget, point[2]), Rational::from_double(budget, point[2])};
}
template <class T> std::span<const T> slice(const std::pmr::vector<T> &pool, Range<T> range) {
  if (range.first > pool.size() || range.count > pool.size() - range.first)
    invalid("source envelope range outside retained pool");
  return std::span<const T>(pool).subspan(static_cast<std::size_t>(range.first),
                                          static_cast<std::size_t>(range.count));
}
class Producer final {
  bgfd::Budget &budget;
  const SourceData &source;
  uint64_t remaining;
  std::pmr::vector<std::optional<Box>> curve_hulls, edge_hulls;
  std::pmr::vector<std::optional<std::array<Rational, 2>>> curve_domains;

  void charge() {
    budget.charge(1);
    if (remaining == 0)
      throw Error(ErrorCode::resource_exhausted, "adaptive envelope source-item limit");
    --remaining;
  }
  Rational scalar(double value) const { return Rational::from_double(budget, value); }
  std::array<Rational, 2> natural_axis(Range<double> knots, Range<uint32_t> multiplicities,
                                       AxisShape shape) {
    const auto values = source.view(knots);
    const auto mult = source.view(multiplicities);
    if (values.size() != mult.size())
      invalid("source natural-axis multiplicity shape");
    std::optional<double> first, last;
    uint64_t expanded = 0;
    for (std::size_t i = 0; i < values.size(); ++i) {
      charge();
      if (mult[i] > UINT64_MAX - expanded)
        invalid("source expanded natural-axis size overflow");
      const auto end = expanded + mult[i];
      if (expanded <= shape.degree && shape.degree < end)
        first = values[i];
      if (expanded <= shape.controls && shape.controls < end)
        last = values[i];
      expanded = end;
    }
    if (!first || !last || !(*first < *last))
      invalid("source has no positive natural axis");
    return {scalar(*first), scalar(*last)};
  }
  std::array<Rational, 2> natural_curve(const Curve &curve) {
    if (curve.kind == 4)
      return {Rational(budget), Rational(budget, 1)};
    if (curve.kind == 5) {
      const auto stride = curve.dimension + 1;
      return natural_axis(curve.knots, curve.multiplicities,
                          {curve.degree, source.view(curve.controls).size() / stride});
    }
    if (curve.kind < 1 || curve.kind > 3)
      unsupported("unimplemented original curve envelope family");
    return {scalar(curve.parameter_range[0]), scalar(curve.parameter_range[1])};
  }
  Box control_box(std::span<const double> controls, uint32_t dimension) {
    if ((dimension != 2 && dimension != 3) || controls.empty() ||
        controls.size() % (dimension + 1) != 0)
      invalid("original homogeneous envelope control shape");
    std::optional<Box> box;
    for (std::size_t i = 0; i < controls.size(); i += dimension + 1) {
      charge();
      const auto weight = scalar(controls[i + dimension]);
      if (weight.sign() <= 0)
        unsupported("original envelope requires positive homogeneous weights");
      Box point{Rational(budget), Rational(budget), Rational(budget),
                Rational(budget), Rational(budget), Rational(budget)};
      for (std::size_t axis = 0; axis < dimension; ++axis) {
        point[2 * axis] = scalar(controls[i + axis]).divide(weight);
        point[2 * axis + 1] = point[2 * axis];
      }
      unite(box, point);
    }
    if (!box)
      invalid("empty original homogeneous envelope net");
    return *box;
  }
  Box curve_box(Slot<CurveTag> slot, std::span<const Rational, 2> range) {
    charge();
    const auto &curve = source.curves.at(slot.value);
    if (curve.dimension != 2 && curve.dimension != 3)
      invalid("original envelope curve dimension");
    auto &natural = curve_domains[slot.value];
    if (!natural)
      natural.emplace(natural_curve(curve));
    if (range[0].compare(range[1]) > 0 || range[0].compare((*natural)[0]) < 0 ||
        range[1].compare((*natural)[1]) > 0)
      unsupported("original curve envelope request outside natural domain");
    if (curve.kind == 1) {
      Box box{Rational(budget), Rational(budget), Rational(budget),
              Rational(budget), Rational(budget), Rational(budget)};
      for (std::size_t axis = 0; axis < curve.dimension; ++axis) {
        const auto a =
            scalar(curve.frame[axis]).add(range[0].multiply(scalar(curve.frame[3 + axis])));
        const auto b =
            scalar(curve.frame[axis]).add(range[1].multiply(scalar(curve.frame[3 + axis])));
        const auto values = ordered(a, b);
        box[2 * axis] = values[0];
        box[2 * axis + 1] = values[1];
      }
      return box;
    }
    auto &cached = curve_hulls[slot.value];
    if (!cached) {
      if (curve.kind == 4 || curve.kind == 5) {
        cached.emplace(control_box(source.view(curve.controls), curve.dimension));
      } else {
        Box box{Rational(budget), Rational(budget), Rational(budget),
                Rational(budget), Rational(budget), Rational(budget)};
        for (std::size_t axis = 0; axis < curve.dimension; ++axis) {
          const auto extent =
              abs(scalar(curve.radii[0]).multiply(scalar(curve.frame[3 + axis])))
                  .add(abs(scalar(curve.radii[1]).multiply(scalar(curve.frame[6 + axis]))));
          box[2 * axis] = scalar(curve.frame[axis]).subtract(extent);
          box[2 * axis + 1] = scalar(curve.frame[axis]).add(extent);
        }
        cached.emplace(std::move(box));
      }
    }
    return *cached;
  }
  std::array<Rational, 2> mapped_range(const Coedge &use, const Edge &edge) {
    if (edge.degenerate)
      return ordered(scalar(use.parameter_range[0]), scalar(use.parameter_range[1]));
    const auto domain = ordered(scalar(edge.parameter_range[0]), scalar(edge.parameter_range[1]));
    if (use.parameter_map_kind == 1)
      return domain;
    if (use.parameter_map_kind == 4)
      unsupported(measured_parameter_map_refusal);
    if (use.parameter_map_kind != 2)
      unsupported("unimplemented original envelope parameter continuation map");
    const auto map = slice(source.parameter_maps, use.parameter_map);
    if (map.size() < 2)
      invalid("missing original envelope parameter map");
    const auto total = ordered(scalar(map.front()[0]), scalar(map.back()[0]));
    if (total[0].compare(domain[0]) > 0 || total[1].compare(domain[1]) < 0)
      invalid("original envelope map omits spatial interval");
    std::optional<std::array<Rational, 2>> image;
    for (std::size_t i = 1; i < map.size(); ++i) {
      charge();
      const auto x0 = scalar(map[i - 1][0]), x1 = scalar(map[i][0]);
      const auto interval = ordered(x0, x1);
      const auto lo = interval[0].compare(domain[0]) > 0 ? interval[0] : domain[0];
      const auto hi = interval[1].compare(domain[1]) < 0 ? interval[1] : domain[1];
      if (lo.compare(hi) > 0)
        continue;
      const auto y0 = scalar(map[i - 1][1]), y1 = scalar(map[i][1]);
      const auto evaluate = [&](const Rational &t) {
        return y0.add(t.subtract(x0).multiply(y1.subtract(y0)).divide(x1.subtract(x0)));
      };
      const auto values = ordered(evaluate(lo), evaluate(hi));
      if (image)
        unite(*image, values);
      else
        image.emplace(values);
    }
    if (!image)
      invalid("original envelope map has empty coverage");
    return *image;
  }
  Axis axis(const Frame &frame, std::size_t offset) const {
    return {scalar(frame[offset]), scalar(frame[offset + 1]), scalar(frame[offset + 2])};
  }
  Rational dot(const Axis &a, const Axis &b) const {
    return a[0].multiply(b[0]).add(a[1].multiply(b[1])).add(a[2].multiply(b[2]));
  }
  std::array<Rational, 2> project(const Box &physical, const Carrier &carrier, const Axis &dual) {
    Rational lo(budget), hi(budget);
    for (std::size_t a = 0; a < 3; ++a) {
      const auto first = physical[2 * a].subtract(scalar(carrier.frame[a])).multiply(dual[a]);
      const auto last = physical[2 * a + 1].subtract(scalar(carrier.frame[a])).multiply(dual[a]);
      const auto values = ordered(first, last);
      lo = lo.add(values[0]);
      hi = hi.add(values[1]);
    }
    return {lo, hi};
  }
  void expand_inverse(UVBox &uv, const Carrier &carrier, const Box &physical) {
    if (carrier.kind != 1 && carrier.kind != 2)
      return;
    const auto x = axis(carrier.frame, 3), y = axis(carrier.frame, 6);
    const auto update = [&](std::size_t coordinate, const Axis &dual) {
      const auto values = project(physical, carrier, dual);
      if (values[0].compare(uv[coordinate * 2]) < 0)
        uv[coordinate * 2] = values[0];
      if (values[1].compare(uv[coordinate * 2 + 1]) > 0)
        uv[coordinate * 2 + 1] = values[1];
    };
    if (carrier.kind == 1) {
      const auto xx = dot(x, x), yy = dot(y, y), xy = dot(x, y);
      const auto det = xx.multiply(yy).subtract(xy.multiply(xy));
      if (det.sign() > 0) {
        Axis u{Rational(budget), Rational(budget), Rational(budget)}, v = u;
        for (std::size_t a = 0; a < 3; ++a) {
          u[a] = yy.multiply(x[a]).subtract(xy.multiply(y[a])).divide(det);
          v[a] = xx.multiply(y[a]).subtract(xy.multiply(x[a])).divide(det);
        }
        update(0, u);
        update(1, v);
      }
    } else {
      Axis cross{x[1].multiply(y[2]).subtract(x[2].multiply(y[1])),
                 x[2].multiply(y[0]).subtract(x[0].multiply(y[2])),
                 x[0].multiply(y[1]).subtract(x[1].multiply(y[0]))};
      const auto det = dot(cross, axis(carrier.frame, 9));
      if (det.sign() != 0) {
        for (auto &value : cross)
          value = value.divide(det);
        update(1, cross);
      }
    }
  }

public:
  std::pmr::vector<std::optional<UVBox>> profiles;
  Producer(bgfd::Budget &owner, const SourceData &original, uint64_t items)
      : budget(owner), source(original), remaining(items),
        curve_hulls(original.curves.size(), &owner), edge_hulls(original.edges.size(), &owner),
        curve_domains(original.curves.size(), &owner), profiles(original.carriers.size(), &owner) {}
  const Box &edge_box(Slot<EdgeTag> slot) {
    auto &box = edge_hulls.at(slot.value);
    if (!box) {
      charge();
      const auto &edge = source.edges.at(slot.value);
      box.emplace(point_box(
          budget, source.vertices[source.vertex_index.find(edge.vertex_ids[0]).value].xyz));
      unite(*box,
            point_box(budget,
                      source.vertices[source.vertex_index.find(edge.vertex_ids[1]).value].xyz));
      if (edge.curve_id) {
        const auto range =
            ordered(scalar(edge.parameter_range[0]), scalar(edge.parameter_range[1]));
        unite(*box, curve_box(source.curve_index.find(edge.curve_id), range));
      } else if (!edge.degenerate) {
        invalid("nondegenerate source edge has no spatial curve");
      }
    }
    return *box;
  }
  void prepare_profiles() {
    for (const auto &face : source.faces) {
      charge();
      const auto slot = source.carrier_index.find(face.carrier_id);
      const auto &carrier = source.carriers[slot.value];
      auto &profile = profiles[slot.value];
      if (!profile) {
        if (carrier.kind == 6) {
          const auto u = natural_axis(carrier.knot_u, carrier.mult_u,
                                      {carrier.degrees[0], carrier.control_shape[0]});
          const auto v = natural_axis(carrier.knot_v, carrier.mult_v,
                                      {carrier.degrees[1], carrier.control_shape[1]});
          profile.emplace(UVBox{u[0], u[1], v[0], v[1]});
        } else {
          if (carrier.kind < 1 || carrier.kind > 5)
            unsupported("unimplemented original carrier envelope family");
          profile.emplace(UVBox{scalar(carrier.uv_bounds[0]), scalar(carrier.uv_bounds[1]),
                                scalar(carrier.uv_bounds[2]), scalar(carrier.uv_bounds[3])});
        }
      }
      for (auto loop_id : slice(source.loop_ids, face.loops)) {
        const auto &loop = source.loops[source.loop_index.find(loop_id).value];
        for (auto use_id : slice(source.coedge_ids, loop.coedges)) {
          charge();
          const auto &use = source.coedges[source.coedge_index.find(use_id).value];
          if (use.periodic_shift[0] != 0 || use.periodic_shift[1] != 0)
            unsupported("nonzero original lift outside adaptive envelope profile");
          if (use.parameter_map_kind == 4)
            unsupported(measured_parameter_map_refusal);
          if (use.parameter_map_kind != 1 && use.parameter_map_kind != 2)
            unsupported("unimplemented original envelope parameter map");
          const auto edge = source.edge_index.find(use.edge_id);
          const auto range = mapped_range(use, source.edges[edge.value]);
          const auto guide = source.curve_index.find(use.curve_id);
          if (source.curves[guide.value].dimension != 2)
            invalid("source envelope guide is not two-dimensional");
          const auto image = curve_box(guide, range);
          if (carrier.kind != 6) {
            unite(*profile, UVBox{image[0], image[1], image[2], image[3]});
            expand_inverse(*profile, carrier, edge_box(edge));
          }
        }
      }
    }
    const auto maximum_finite = scalar(std::numeric_limits<double>::max());
    for (auto &profile : profiles)
      if (profile)
        for (std::size_t i = 0; i < 4; ++i) {
          if (abs((*profile)[i]).compare(maximum_finite) > 0)
            throw Error(ErrorCode::unresolved,
                        "adaptive carrier profile has no finite binary64 enclosure");
          const auto converted = (*profile)[i].bounds();
          const auto endpoint = (i % 2 == 0) ? converted.lower : converted.upper;
          if (!std::isfinite(endpoint))
            throw Error(ErrorCode::unresolved,
                        "adaptive carrier profile has no finite binary64 enclosure");
          (*profile)[i] = scalar(endpoint);
        }
  }
  Box carrier_box(Slot<CarrierTag> slot) {
    charge();
    const auto &carrier = source.carriers.at(slot.value);
    if (carrier.kind == 6)
      return control_box(source.view(carrier.controls), 3);
    const auto &profile = profiles[slot.value];
    if (!profile)
      invalid("carrier envelope has no retained finite profile");
    const auto &uv = *profile;
    const auto maximum_v = abs(uv[2]).compare(abs(uv[3])) > 0 ? abs(uv[2]) : abs(uv[3]);
    const auto radius = abs(scalar(carrier.radii[0]));
    Box box{Rational(budget), Rational(budget), Rational(budget),
            Rational(budget), Rational(budget), Rational(budget)};
    for (std::size_t a = 0; a < 3; ++a) {
      const auto o = scalar(carrier.frame[a]), x = scalar(carrier.frame[3 + a]),
                 y = scalar(carrier.frame[6 + a]), z = scalar(carrier.frame[9 + a]);
      if (carrier.kind == 1) {
        const auto u = ordered(uv[0].multiply(x), uv[1].multiply(x));
        const auto v = ordered(uv[2].multiply(y), uv[3].multiply(y));
        box[2 * a] = o.add(u[0]).add(v[0]);
        box[2 * a + 1] = o.add(u[1]).add(v[1]);
      } else {
        const auto angular = abs(x).add(abs(y));
        Rational extent(budget);
        if (carrier.kind == 2)
          extent = radius.multiply(angular).add(maximum_v.multiply(abs(z)));
        else if (carrier.kind == 3)
          extent = radius.add(maximum_v).multiply(angular).add(maximum_v.multiply(abs(z)));
        else if (carrier.kind == 4)
          extent = radius.multiply(angular.add(abs(z)));
        else
          extent = radius.add(abs(scalar(carrier.radii[1])))
                       .multiply(angular)
                       .add(abs(scalar(carrier.radii[1])).multiply(abs(z)));
        box[2 * a] = o.subtract(extent);
        box[2 * a + 1] = o.add(extent);
      }
    }
    return box;
  }
  Box definition_box(Slot<DefinitionTag> slot, std::span<const std::optional<Box>> carriers) {
    charge();
    std::optional<Box> box;
    for (auto vertex : source.definition_vertices.at(slot.value)) {
      charge();
      unite(box, point_box(budget, source.vertices[vertex].xyz));
    }
    for (auto edge : source.definition_edges.at(slot.value))
      unite(box, edge_box({edge}));
    for (auto face : source.definition_faces.at(slot.value)) {
      charge();
      const auto carrier = source.carrier_index.find(source.faces[face].carrier_id);
      unite(box, *carriers[carrier.value]);
    }
    if (!box)
      unsupported("source definition has no physical geometry envelope");
    return *box;
  }
};
} // namespace
AdaptiveAssemblyData::AdaptiveAssemblyData(std::shared_ptr<RunResource> owner, SourceStore original,
                                           ExactTable values, AssemblyStore placements)
    : resource(std::move(owner)), source(std::move(original)), envelope_values(std::move(values)),
      assembly(std::move(placements)), definitions(resource.get()), carriers(resource.get()),
      faces(resource.get()) {}

AdaptiveAssembly prepare_adaptive_assembly(bgfd::Budget &budget, const SourceStore &source,
                                           const std::shared_ptr<RunResource> &resource,
                                           const RequestedQuality &quality,
                                           AdaptiveAssemblyKeys keys,
                                           AdaptiveAssemblyPrecision precision,
                                           AdaptiveAssemblyLimits limits) {
  if (!resource || resource != source.view().resource || &quality.positional.budget() != &budget ||
      &quality.node_source.budget() != &budget)
    invalid("adaptive assembly source/resource/arithmetic owner mismatch");
  if (keys.envelopes == keys.allowances || keys.envelopes.generation != keys.allowances.generation)
    invalid("adaptive envelope/allowance exact-table identity mismatch");
  if (quality.positional.sign() <= 0 || quality.node_source.sign() <= 0)
    invalid("adaptive assembly needs positive quality");
  auto work_scope = budget.scoped_work(limits.work);
  const auto &original = source.view();
  if (original.definitions.size() > UINT32_MAX || original.carriers.size() > UINT32_MAX ||
      original.faces.size() > UINT32_MAX)
    throw Error(ErrorCode::resource_exhausted, "adaptive envelope source slot capacity");
  Producer producer(budget, original, limits.source_items);
  producer.prepare_profiles();
  std::pmr::vector<std::optional<Box>> carrier_boxes(original.carriers.size(), &budget);
  for (std::size_t i = 0; i < producer.profiles.size(); ++i)
    if (producer.profiles[i])
      carrier_boxes[i].emplace(producer.carrier_box({static_cast<uint32_t>(i)}));
  std::pmr::vector<Box> boxes(&budget);
  boxes.reserve(original.definitions.size());
  for (std::size_t i = 0; i < original.definitions.size(); ++i)
    boxes.push_back(producer.definition_box({static_cast<uint32_t>(i)}, carrier_boxes));
  std::pmr::vector<DefinitionEnvelopeView> borrowed(&budget);
  borrowed.reserve(boxes.size());
  for (std::size_t i = 0; i < boxes.size(); ++i)
    borrowed.push_back({source, {static_cast<uint32_t>(i)}, boxes[i]});
  const auto assembly = prepare_assembly(budget, source, resource, borrowed, quality,
                                         keys.allowances, precision.transport, limits.transport);
  ExactBuilder values(resource, keys.envelopes);
  std::pmr::vector<AdaptiveDefinitionEnvelope> definitions(resource.get());
  std::pmr::vector<AdaptiveCarrierEnvelope> carriers(resource.get());
  std::pmr::vector<AdaptiveFaceEnvelope> faces(resource.get());
  for (std::size_t i = 0; i < boxes.size(); ++i) {
    budget.charge(1);
    AdaptiveDefinitionEnvelope row{{static_cast<uint32_t>(i)}, {}};
    for (std::size_t j = 0; j < 6; ++j)
      row.xyz[j] = values.append(boxes[i][j]);
    definitions.push_back(row);
  }
  std::pmr::vector<uint32_t> directory(original.carriers.size(), UINT32_MAX, &budget);
  for (std::size_t i = 0; i < producer.profiles.size(); ++i) {
    if (!producer.profiles[i])
      continue;
    budget.charge(1);
    directory[i] = static_cast<uint32_t>(carriers.size());
    AdaptiveCarrierEnvelope row{{static_cast<uint32_t>(i)}, {}, {}};
    for (std::size_t j = 0; j < 4; ++j) {
      row.uv[j] = values.append((*producer.profiles[i])[j]);
      row.cache_domain[j] = (*producer.profiles[i])[j].to_double();
    }
    carriers.push_back(row);
  }
  for (std::size_t i = 0; i < original.faces.size(); ++i) {
    budget.charge(1);
    const auto carrier = original.carrier_index.find(original.faces[i].carrier_id);
    faces.push_back({{static_cast<uint32_t>(i)}, directory[carrier.value]});
  }
  auto result = std::allocate_shared<AdaptiveAssemblyData>(
      RunAllocator<AdaptiveAssemblyData>(resource), resource, source, values.seal(), assembly);
  result->definitions = std::move(definitions);
  result->carriers = std::move(carriers);
  result->faces = std::move(faces);
  return result;
}
} // namespace bgmesh::adaptive
