#include "geometry_store.hpp"
#include <cmath>
#include <limits>
#include <utility>

namespace bgmesh {
namespace {
[[noreturn]] void invalid(const char *message) { throw Error(ErrorCode::invalid_source, message); }
uint32_t narrow(std::size_t value) {
  if (value > std::numeric_limits<uint32_t>::max())
    throw Error(ErrorCode::resource_exhausted, "geometry root slot count overflow");
  return static_cast<uint32_t>(value);
}
std::pmr::vector<double> expand(bgfd::Budget &budget, std::span<const double> knots,
                                std::span<const uint32_t> multiplicities, uint32_t degree) {
  if (knots.size() != multiplicities.size() || degree > 32 || knots.empty())
    invalid("original compressed knot shape mismatch");
  std::size_t count = 0;
  for (std::size_t i = 0; i < knots.size(); ++i) {
    if (!std::isfinite(knots[i]) || (i != 0 && knots[i] <= knots[i - 1]) ||
        multiplicities[i] == 0 || multiplicities[i] > degree + 1 ||
        multiplicities[i] > std::numeric_limits<std::size_t>::max() - count)
      invalid("invalid original compressed knot values");
    count += multiplicities[i];
  }
  std::pmr::vector<double> result(&budget);
  result.reserve(count);
  for (std::size_t i = 0; i < knots.size(); ++i) {
    budget.charge(multiplicities[i]);
    result.insert(result.end(), multiplicities[i], knots[i]);
  }
  return result;
}
Range<ExactEntry> append_net(ExactBuilder &builder, std::span<const Rational> controls) {
  if (controls.empty())
    invalid("empty original homogeneous root");
  const auto first = builder.append(controls[0]).index;
  for (std::size_t i = 1; i < controls.size(); ++i) {
    const auto next = builder.append(controls[i]);
    if (next.index != first + i)
      throw Error(ErrorCode::internal_error, "noncontiguous geometry coefficient append");
  }
  return {first, controls.size()};
}
Range<ExactEntry> append_axis(ExactBuilder &builder, bgfd::Budget &budget,
                              std::span<const double> expanded, uint32_t degree,
                              std::size_t count) {
  if (count >= expanded.size() || degree >= expanded.size())
    invalid("original axis domain index out of range");
  const auto first = builder.append(Rational::from_double(budget, expanded[degree])).index;
  uint64_t size = 1;
  for (std::size_t k = degree; k < count; ++k)
    if (expanded[k] < expanded[k + 1]) {
      static_cast<void>(builder.append(Rational::from_double(budget, expanded[k + 1])));
      ++size;
    }
  if (size < 2)
    invalid("empty original axis domain");
  return {first, size};
}
Rational scalar(bgfd::Budget &budget, const GeometryStore &store, uint64_t index) {
  return import_exact(budget, store.exact(index));
}
template <class T> const T &record(const std::pmr::vector<T> &records, uint32_t index) {
  if (index >= records.size())
    throw Error(ErrorCode::internal_error, "geometry source/root slot out of bounds");
  return records[index];
}
std::size_t axis_interval(const GeometryStore &store, Range<ExactEntry> axis,
                          const Rational &parameter) {
  if (axis.count < 2)
    throw Error(ErrorCode::unsupported, "geometry formula has no rational span roots");
  auto &budget = parameter.budget();
  if (parameter.compare(scalar(budget, store, axis.first)) < 0 ||
      parameter.compare(scalar(budget, store, axis.first + axis.count - 1)) > 0)
    throw Error(ErrorCode::unsupported, "geometry parameter outside original natural domain");
  uint64_t lo = 0, hi = axis.count - 1;
  while (lo < hi) {
    const auto mid = lo + (hi - lo) / 2;
    if (scalar(budget, store, axis.first + mid + 1).compare(parameter) >= 0)
      hi = mid;
    else
      lo = mid + 1;
  }
  return static_cast<std::size_t>(lo);
}
std::pair<uint64_t, uint64_t> intersecting_axis(bgfd::Budget &budget, const GeometryStore &store,
                                                Range<ExactEntry> axis, const Rational &begin,
                                                const Rational &end) {
  if (&begin.budget() != &budget || &end.budget() != &budget)
    throw Error(ErrorCode::internal_error, "geometry query owner mismatch");
  if (begin.compare(end) > 0)
    invalid("reversed source-root query interval");
  if (axis.count < 2)
    throw Error(ErrorCode::unsupported, "geometry formula has no rational span roots");
  if (begin.compare(scalar(budget, store, axis.first)) < 0 ||
      end.compare(scalar(budget, store, axis.first + axis.count - 1)) > 0)
    throw Error(ErrorCode::unsupported, "source-cell query outside original natural domain");
  const auto count = axis.count - 1;
  uint64_t lo = 0, hi = count;
  while (lo < hi) {
    const auto mid = lo + (hi - lo) / 2;
    if (scalar(budget, store, axis.first + mid + 1).compare(begin) < 0)
      lo = mid + 1;
    else
      hi = mid;
  }
  const auto first = lo;
  hi = count;
  while (lo < hi) {
    const auto mid = lo + (hi - lo) / 2;
    if (scalar(budget, store, axis.first + mid).compare(end) <= 0)
      lo = mid + 1;
    else
      hi = mid;
  }
  return {first, lo};
}
} // namespace

GeometryData::GeometryData(std::shared_ptr<RunResource> owner, SourceStore original,
                           ExactTable exact)
    : resource(std::move(owner)), source(std::move(original)), table(std::move(exact)),
      curves(resource.get()), carriers(resource.get()), curve_roots(resource.get()),
      surface_roots(resource.get()) {}

ExactView GeometryStore::exact(uint64_t local_index) const {
  return data_->table.view({data_->table.key(), local_index});
}

GeometryStore compile_geometry(const SourceStore &source,
                               const std::shared_ptr<RunResource> &resource, ExactTableKey identity,
                               bgfd::Budget &budget) {
  if (!resource)
    throw Error(ErrorCode::internal_error, "geometry store requires an owned run resource");
  const auto &original = source.view();
  static_cast<void>(narrow(original.curves.size()));
  static_cast<void>(narrow(original.carriers.size()));
  ExactBuilder builder(resource, identity);
  std::pmr::vector<CurveGeometry> curves(resource.get());
  std::pmr::vector<CarrierGeometry> carriers(resource.get());
  std::pmr::vector<CurveGeometryRoot> curve_roots(resource.get());
  std::pmr::vector<SurfaceGeometryRoot> surface_roots(resource.get());
  curves.reserve(original.curves.size());
  carriers.reserve(original.carriers.size());
  for (std::size_t slot = 0; slot < original.curves.size(); ++slot) {
    budget.charge(1);
    const auto &curve = original.curves[slot];
    CurveGeometry output{curve.id,     curve.kind, curve.dimension,
                         curve.degree, {},         {curve_roots.size(), 0}};
    if (curve.kind >= 1 && curve.kind <= 3) {
      const auto first =
          builder.append(Rational::from_double(budget, curve.parameter_range[0])).index;
      static_cast<void>(builder.append(Rational::from_double(budget, curve.parameter_range[1])));
      output.axis = {first, 2};
      curves.push_back(output);
      continue;
    }
    std::pmr::vector<double> knots(&budget);
    if (curve.kind == 4) {
      if (curve.parameter_range != Vec2{0., 1.} || curve.degree > 32)
        invalid("original Bezier curve requires its natural [0,1] domain");
      knots.insert(knots.end(), static_cast<std::size_t>(curve.degree) + 1, 0.);
      knots.insert(knots.end(), static_cast<std::size_t>(curve.degree) + 1, 1.);
    } else if (curve.kind == 5) {
      knots = expand(budget, original.view(curve.knots), original.view(curve.multiplicities),
                     curve.degree);
    } else {
      throw Error(ErrorCode::unsupported, "unsupported original curve formula");
    }
    const auto controls = original.view(curve.controls);
    const auto spans = extract_curve(budget, {curve.dimension, curve.degree, knots, controls});
    output.axis = append_axis(builder, budget, knots, curve.degree,
                              controls.size() / (static_cast<std::size_t>(curve.dimension) + 1));
    for (std::size_t i = 0; i < spans.size(); ++i) {
      static_cast<void>(narrow(curve_roots.size()));
      curve_roots.push_back({{narrow(slot)},
                             curve.dimension,
                             curve.degree,
                             {output.axis.first + i, output.axis.first + i + 1},
                             append_net(builder, spans[i].controls)});
    }
    output.roots.count = spans.size();
    curves.push_back(output);
  }
  for (std::size_t slot = 0; slot < original.carriers.size(); ++slot) {
    budget.charge(1);
    const auto &carrier = original.carriers[slot];
    CarrierGeometry output{carrier.id, carrier.kind, carrier.degrees[0],       carrier.degrees[1],
                           {},         {},           {surface_roots.size(), 0}};
    if (carrier.kind >= 1 && carrier.kind <= 5) {
      carriers.push_back(output); // analytic source formula/envelopes are
                                  // separate from NURBS roots
      continue;
    }
    if (carrier.kind != 6)
      throw Error(ErrorCode::unsupported, "unsupported original carrier formula");
    const auto ku = expand(budget, original.view(carrier.knot_u), original.view(carrier.mult_u),
                           carrier.degrees[0]);
    const auto kv = expand(budget, original.view(carrier.knot_v), original.view(carrier.mult_v),
                           carrier.degrees[1]);
    const auto spans = extract_surface(budget, {carrier.degrees[0], carrier.degrees[1],
                                                carrier.control_shape[0], carrier.control_shape[1],
                                                ku, kv, original.view(carrier.controls)});
    output.u_axis = append_axis(builder, budget, ku, carrier.degrees[0], carrier.control_shape[0]);
    output.v_axis = append_axis(builder, budget, kv, carrier.degrees[1], carrier.control_shape[1]);
    const auto u_count = output.u_axis.count - 1, v_count = output.v_axis.count - 1;
    if (u_count != 0 && v_count > std::numeric_limits<uint64_t>::max() / u_count)
      throw Error(ErrorCode::resource_exhausted, "geometry tensor root grid overflow");
    if (spans.size() != u_count * v_count)
      throw Error(ErrorCode::internal_error, "geometry source grid/root inventory mismatch");
    for (uint64_t i = 0; i < u_count; ++i)
      for (uint64_t j = 0; j < v_count; ++j) {
        static_cast<void>(narrow(surface_roots.size()));
        surface_roots.push_back(
            {{narrow(slot)},
             carrier.degrees[0],
             carrier.degrees[1],
             {output.u_axis.first + i, output.u_axis.first + i + 1, output.v_axis.first + j,
              output.v_axis.first + j + 1},
             append_net(builder, spans[static_cast<std::size_t>(i * v_count + j)].controls)});
      }
    output.roots.count = spans.size();
    carriers.push_back(output);
  }
  auto data = std::allocate_shared<GeometryData>(RunAllocator<GeometryData>(resource), resource,
                                                 source, builder.seal());
  data->curves = std::move(curves);
  data->carriers = std::move(carriers);
  data->curve_roots = std::move(curve_roots);
  data->surface_roots = std::move(surface_roots);
  return GeometryStore(std::move(data));
}

CurveRootSlot find_curve_root(const GeometryStore &store, Slot<CurveTag> source,
                              const Rational &parameter) {
  const auto &curve = record(store.view().curves, source.value);
  if (curve.roots.count == 0)
    throw Error(ErrorCode::unsupported, "analytic source curve has no rational root");
  const auto interval = axis_interval(store, curve.axis, parameter);
  return {narrow(static_cast<std::size_t>(curve.roots.first) + interval)};
}
SurfaceRootSlot find_surface_root(const GeometryStore &store, Slot<CarrierTag> source,
                                  const Rational &u, const Rational &v) {
  if (&u.budget() != &v.budget())
    throw Error(ErrorCode::internal_error, "geometry query owner mismatch");
  const auto &carrier = record(store.view().carriers, source.value);
  const auto i = axis_interval(store, carrier.u_axis, u),
             j = axis_interval(store, carrier.v_axis, v);
  return {narrow(static_cast<std::size_t>(carrier.roots.first) +
                 i * static_cast<std::size_t>(carrier.v_axis.count - 1) + j)};
}
std::pmr::vector<SurfaceRootSlot>
intersecting_surface_roots(bgfd::Budget &budget, const GeometryStore &store,
                           Slot<CarrierTag> source, const std::array<Rational, 4> &bounds) {
  const auto &carrier = record(store.view().carriers, source.value);
  const auto u = intersecting_axis(budget, store, carrier.u_axis, bounds[0], bounds[1]);
  const auto v = intersecting_axis(budget, store, carrier.v_axis, bounds[2], bounds[3]);
  std::pmr::vector<SurfaceRootSlot> result(&budget);
  for (uint64_t i = u.first; i < u.second; ++i)
    for (uint64_t j = v.first; j < v.second; ++j) {
      budget.charge(1);
      result.push_back({narrow(
          static_cast<std::size_t>(carrier.roots.first + i * (carrier.v_axis.count - 1) + j))});
    }
  return result;
}
std::pmr::vector<CurveRootSlot> intersecting_curve_roots(
    bgfd::Budget &budget,const GeometryStore &store,Slot<CurveTag> source,
    const std::array<Rational,2> &bounds) {
  const auto &curve=record(store.view().curves,source.value);
  if(curve.roots.count==0)
    throw Error(ErrorCode::unsupported,"analytic source curve has no rational roots");
  const auto range=intersecting_axis(budget,store,curve.axis,bounds[0],bounds[1]);
  std::pmr::vector<CurveRootSlot> result(&budget);
  for(uint64_t i=range.first;i<range.second;++i) {
    budget.charge(1);
    result.push_back({narrow(static_cast<std::size_t>(curve.roots.first+i))});
  }
  return result;
}
BezierCurve import_curve_root(bgfd::Budget &budget, const GeometryStore &store,
                              CurveRootSlot slot) {
  const auto &root = record(store.view().curve_roots, slot.value);
  BezierCurve result(
      budget, {.dimension = root.dimension, .degree = root.degree},
      {scalar(budget, store, root.bounds[0]), scalar(budget, store, root.bounds[1])});
  result.controls.reserve(static_cast<std::size_t>(root.controls.count));
  for (uint64_t i = 0; i < root.controls.count; ++i)
    result.controls.push_back(scalar(budget, store, root.controls.first + i));
  return result;
}
BezierSurface import_surface_root(bgfd::Budget &budget, const GeometryStore &store,
                                  SurfaceRootSlot slot) {
  const auto &root = record(store.view().surface_roots, slot.value);
  BezierSurface result(
      budget, root.p, root.q,
      {scalar(budget, store, root.bounds[0]), scalar(budget, store, root.bounds[1]),
       scalar(budget, store, root.bounds[2]), scalar(budget, store, root.bounds[3])});
  result.controls.reserve(static_cast<std::size_t>(root.controls.count));
  for (uint64_t i = 0; i < root.controls.count; ++i)
    result.controls.push_back(scalar(budget, store, root.controls.first + i));
  return result;
}
} // namespace bgmesh
