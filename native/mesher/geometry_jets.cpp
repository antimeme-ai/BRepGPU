#include "geometry_jets.hpp"
#include "geometry_enclosure.hpp"
#include "geometry_polynomial.hpp"
#include <algorithm>
#include <cmath>

namespace bgmesh {
namespace {
using namespace geometry_detail;
using Net = std::pmr::vector<Rational>;
using Axes = std::array<ExactEnclosure, 3>;
[[noreturn]] void invalid(const char *message) { throw Error(ErrorCode::invalid_source, message); }
void owner(GeometryContext &context, const Rational &value) {
  if (&context.budget() != &value.budget())
    throw Error(ErrorCode::internal_error, "source jet owner mismatch");
}
Net zero_net(bgfd::Budget &b, std::size_t count) {
  Net out(&b);
  out.reserve(count);
  for (std::size_t i = 0; i < count; ++i)
    out.emplace_back(b, 0);
  return out;
}
ExactEnclosure hull(std::span<const Rational> net, std::size_t stride, std::size_t axis) {
  auto low = net[axis], high = net[axis];
  for (std::size_t i = stride + axis; i < net.size(); i += stride) {
    if (net[i].compare(low) < 0)
      low = net[i];
    if (net[i].compare(high) > 0)
      high = net[i];
  }
  return {std::move(low), std::move(high)};
}
ExactEnclosure cartesian_hull(std::span<const Rational> net, std::size_t stride, std::size_t axis) {
  auto low = net[axis].divide(net[stride - 1]), high = low;
  for (std::size_t i = stride; i < net.size(); i += stride) {
    const auto point = net[i + axis].divide(net[i + stride - 1]);
    if (point.compare(low) < 0)
      low = point;
    if (point.compare(high) > 0)
      high = point;
  }
  return {std::move(low), std::move(high)};
}
Net centered(std::span<const Rational> original, std::size_t stride) {
  auto &b = original[0].budget();
  Net out(original.begin(), original.end(), &b);
  for (std::size_t axis = 0; axis + 1 < stride; ++axis) {
    const auto anchor = original[axis].divide(original[stride - 1]);
    for (std::size_t i = axis; i < out.size(); i += stride)
      out[i] = out[i].subtract(original[i - axis + stride - 1].multiply(anchor));
  }
  return out;
}
Net curve_derivative(std::span<const Rational> net, uint32_t degree, std::size_t stride,
                     const Rational &width) {
  auto &b = width.budget();
  if (degree == 0)
    return zero_net(b, stride);
  const auto factor = Rational(b, degree).divide(width);
  Net result(&b);
  result.reserve(static_cast<std::size_t>(degree) * stride);
  for (uint32_t i = 0; i < degree; ++i)
    for (std::size_t axis = 0; axis < stride; ++axis) {
      const auto index = static_cast<std::size_t>(i) * stride + axis;
      result.push_back(net[index + stride].subtract(net[index]).multiply(factor));
    }
  return result;
}
struct Tensor final {
  uint32_t p, q;
  Net values;
  Tensor(std::array<uint32_t, 2> degrees, Net net)
      : p(degrees[0]), q(degrees[1]), values(std::move(net)) {}
};
Tensor tensor_derivative(const Tensor &net, bool u, const Rational &width) {
  auto &b = width.budget();
  const auto degree = u ? net.p : net.q;
  if (degree == 0)
    return {{0, 0}, zero_net(b, 4)};
  const uint32_t p = net.p - static_cast<uint32_t>(u), q = net.q - static_cast<uint32_t>(!u);
  const auto factor = Rational(b, degree).divide(width);
  Net out(&b);
  out.reserve((static_cast<std::size_t>(p) + 1) * (q + 1) * 4);
  const auto stride = static_cast<std::size_t>(net.q) + 1;
  for (uint32_t i = 0; i <= p; ++i)
    for (uint32_t j = 0; j <= q; ++j)
      for (std::size_t axis = 0; axis < 4; ++axis) {
        const auto a = (static_cast<std::size_t>(i) * stride + j) * 4 + axis;
        const auto step = u ? stride * 4 : 4;
        out.push_back(net.values[a + step].subtract(net.values[a]).multiply(factor));
      }
  return {{p, q}, std::move(out)};
}
Net tensor_value(const Tensor &net, const Rational &u, const Rational &v) {
  auto &budget = u.budget();
  Net rows(&budget);
  const auto row_size = (static_cast<std::size_t>(net.q) + 1) * 4;
  rows.reserve((static_cast<std::size_t>(net.p) + 1) * 4);
  for (uint32_t i = 0; i <= net.p; ++i) {
    const auto row = evaluate_homogeneous_polynomial(
        std::span<const Rational>(net.values)
            .subspan(static_cast<std::size_t>(i) * row_size, row_size),
        {.degree = net.q, .stride = 4}, v);
    rows.insert(rows.end(), row.begin(), row.end());
  }
  return evaluate_homogeneous_polynomial(rows, {.degree = net.p, .stride = 4}, u);
}
ExactEnclosure zero(bgfd::Budget &b) { return ExactEnclosure(Rational(b, 0)); }
Axes zero_axes(bgfd::Budget &b) { return {zero(b), zero(b), zero(b)}; }
void validate_analytic(GeometryContext &context, const Frame &frame,
                       std::span<const double> radii) {
  static_cast<void>(context);
  for (const auto value : frame)
    if (!std::isfinite(value))
      invalid("nonfinite original analytic jet frame");
  for (const auto value : radii)
    if (!std::isfinite(value))
      invalid("nonfinite original analytic jet radii");
}
Axes frame_axis(bgfd::Budget &b, const Frame &frame, std::size_t offset) {
  return {ExactEnclosure(Rational::from_double(b, frame[offset])),
          ExactEnclosure(Rational::from_double(b, frame[offset + 1])),
          ExactEnclosure(Rational::from_double(b, frame[offset + 2]))};
}
Axes vector_sum(const Axes &a, const Axes &b) {
  return {sum(a[0], b[0]), sum(a[1], b[1]), sum(a[2], b[2])};
}
Axes vector_times(const Axes &a, const ExactEnclosure &scale) {
  return {product(a[0], scale), product(a[1], scale), product(a[2], scale)};
}
Axes vector_negative(const Axes &a) { return {negate(a[0]), negate(a[1]), negate(a[2])}; }
Axes radial_vector(const Axes &x, const Axes &y, const SinCosEnclosure &trig) {
  return vector_sum(vector_times(x, trig.cosine), vector_times(y, trig.sine));
}
Axes angular_vector(const Axes &x, const Axes &y, const SinCosEnclosure &trig) {
  return vector_sum(vector_times(x, negate(trig.sine)), vector_times(y, trig.cosine));
}
} // namespace

CurveJetEnclosure enclose_rational_curve_jet(const BezierCurve &source, const Rational &begin,
                                             const Rational &end) {
  if (begin.compare(end) >= 0)
    invalid("rational curve jet requires a positive source interval");
  const auto root = restrict_curve(source, begin, end);
  auto &b = begin.budget();
  const auto stride = static_cast<std::size_t>(root.dimension) + 1;
  const auto net = centered(root.controls, stride);
  const auto width = end.subtract(begin);
  const auto first = curve_derivative(net, root.degree, stride, width);
  const auto second =
      curve_derivative(first, root.degree == 0 ? 0 : root.degree - 1, stride, width);
  const auto w = hull(net, stride, root.dimension), w1 = hull(first, stride, root.dimension),
             w2 = hull(second, stride, root.dimension);
  const auto pos = [&](std::size_t axis) {
    return axis < root.dimension ? cartesian_hull(root.controls, stride, axis) : zero(b);
  };
  const auto c = [&](std::size_t axis) {
    return axis < root.dimension ? cartesian_hull(net, stride, axis) : zero(b);
  };
  const auto d1 = [&](std::size_t axis) {
    return axis < root.dimension
               ? quotient(difference(hull(first, stride, axis), product(c(axis), w1)), w)
               : zero(b);
  };
  const auto d2 = [&](std::size_t axis) {
    return axis < root.dimension
               ? quotient(difference(difference(hull(second, stride, axis),
                                                times(product(d1(axis), w1), Rational(b, 2))),
                                     product(c(axis), w2)),
                          w)
               : zero(b);
  };
  return {root.dimension, {pos(0), pos(1), pos(2)}, {d1(0), d1(1), d1(2)}, {d2(0), d2(1), d2(2)}};
}
SurfaceJetEnclosure enclose_rational_surface_jet(const BezierSurface &source,
                                                 const std::array<Rational, 4> &bounds) {
  const auto root = restrict_surface(source, bounds);
  auto &b = bounds[0].budget();
  const auto uw = bounds[1].subtract(bounds[0]), vw = bounds[3].subtract(bounds[2]);
  const Tensor net({root.p, root.q}, centered(root.controls, 4));
  const auto nu = tensor_derivative(net, true, uw), nv = tensor_derivative(net, false, vw);
  const auto nuu = tensor_derivative(nu, true, uw), nuv = tensor_derivative(nu, false, vw),
             nvv = tensor_derivative(nv, false, vw);
  const auto w = hull(net.values, 4, 3), wu = hull(nu.values, 4, 3), wv = hull(nv.values, 4, 3);
  const auto wuu = hull(nuu.values, 4, 3), wuv = hull(nuv.values, 4, 3),
             wvv = hull(nvv.values, 4, 3);
  const auto c = [&](std::size_t axis) { return cartesian_hull(net.values, 4, axis); };
  const auto du = [&](std::size_t axis) {
    return quotient(difference(hull(nu.values, 4, axis), product(c(axis), wu)), w);
  };
  const auto dv = [&](std::size_t axis) {
    return quotient(difference(hull(nv.values, 4, axis), product(c(axis), wv)), w);
  };
  const auto duu = [&](std::size_t axis) {
    return quotient(difference(difference(hull(nuu.values, 4, axis),
                                          times(product(du(axis), wu), Rational(b, 2))),
                               product(c(axis), wuu)),
                    w);
  };
  const auto duv = [&](std::size_t axis) {
    return quotient(
        difference(difference(difference(hull(nuv.values, 4, axis), product(du(axis), wv)),
                              product(dv(axis), wu)),
                   product(c(axis), wuv)),
        w);
  };
  const auto dvv = [&](std::size_t axis) {
    return quotient(difference(difference(hull(nvv.values, 4, axis),
                                          times(product(dv(axis), wv), Rational(b, 2))),
                               product(c(axis), wvv)),
                    w);
  };
  return {{cartesian_hull(root.controls, 4, 0), cartesian_hull(root.controls, 4, 1),
           cartesian_hull(root.controls, 4, 2)},
          {du(0), du(1), du(2)},
          {dv(0), dv(1), dv(2)},
          {duu(0), duu(1), duu(2)},
          {duv(0), duv(1), duv(2)},
          {dvv(0), dvv(1), dvv(2)}};
}
CurveJetEnclosure enclose_analytic_curve_jet(GeometryContext &context, const Curve &curve,
                                             const Rational &begin, const Rational &end,
                                             uint32_t precision_bits) {
  owner(context, begin);
  owner(context, end);
  validate_analytic(context, curve.frame, curve.radii);
  if ((curve.dimension != 2 && curve.dimension != 3) || curve.kind < 1 || curve.kind > 3 ||
      begin.compare(end) > 0)
    invalid("invalid original analytic curve jet query");
  auto &b = context.budget();
  const auto o = frame_axis(b, curve.frame, 0), x = frame_axis(b, curve.frame, 3),
             y = frame_axis(b, curve.frame, 6);
  const auto range = ExactEnclosure(begin, end);
  if (curve.kind == 1) {
    const auto pos = vector_sum(o, vector_times(x, range));
    return {curve.dimension,
            {pos[0], pos[1], curve.dimension == 3 ? pos[2] : zero(b)},
            {x[0], x[1], curve.dimension == 3 ? x[2] : zero(b)},
            zero_axes(b)};
  }
  const auto trig = trig_range(context, begin, end, precision_bits);
  const auto rx = vector_times(x, ExactEnclosure(Rational::from_double(b, curve.radii[0]))),
             ry = vector_times(y, ExactEnclosure(Rational::from_double(b, curve.radii[1])));
  const auto radial = radial_vector(rx, ry, trig), angular = angular_vector(rx, ry, trig);
  const auto pos = vector_sum(o, radial), second = vector_negative(radial);
  return {curve.dimension,
          {pos[0], pos[1], curve.dimension == 3 ? pos[2] : zero(b)},
          {angular[0], angular[1], curve.dimension == 3 ? angular[2] : zero(b)},
          {second[0], second[1], curve.dimension == 3 ? second[2] : zero(b)}};
}
SurfaceJetEnclosure enclose_analytic_surface_jet(GeometryContext &context, const Carrier &carrier,
                                                 const std::array<Rational, 4> &bounds,
                                                 uint32_t precision_bits) {
  for (const auto &value : bounds)
    owner(context, value);
  validate_analytic(context, carrier.frame, carrier.radii);
  if (carrier.kind < 1 || carrier.kind > 5 || bounds[0].compare(bounds[1]) > 0 ||
      bounds[2].compare(bounds[3]) > 0)
    invalid("invalid original analytic surface jet query");
  auto &b = context.budget();
  const auto o = frame_axis(b, carrier.frame, 0), x = frame_axis(b, carrier.frame, 3),
             y = frame_axis(b, carrier.frame, 6), z = frame_axis(b, carrier.frame, 9);
  const ExactEnclosure u(bounds[0], bounds[1]), v(bounds[2], bounds[3]);
  if (carrier.kind == 1)
    return {vector_sum(o, vector_sum(vector_times(x, u), vector_times(y, v))),
            x,
            y,
            zero_axes(b),
            zero_axes(b),
            zero_axes(b)};
  const auto trig_u = trig_range(context, bounds[0], bounds[1], precision_bits);
  const auto e = radial_vector(x, y, trig_u), ep = angular_vector(x, y, trig_u);
  const ExactEnclosure r(Rational::from_double(b, carrier.radii[0]));
  if (carrier.kind == 2) {
    const auto radial = vector_times(e, r);
    return {vector_sum(o, vector_sum(radial, vector_times(z, v))),
            vector_times(ep, r),
            z,
            vector_negative(radial),
            zero_axes(b),
            zero_axes(b)};
  }
  if (carrier.kind == 3) {
    const auto semiangle = Rational::from_double(b, carrier.radii[1]);
    const auto angle = trig_range(context, semiangle, semiangle, precision_bits);
    const auto radial = sum(r, product(v, angle.sine)), axial = product(v, angle.cosine);
    return {vector_sum(o, vector_sum(vector_times(e, radial), vector_times(z, axial))),
            vector_times(ep, radial),
            vector_sum(vector_times(e, angle.sine), vector_times(z, angle.cosine)),
            vector_negative(vector_times(e, radial)),
            vector_times(ep, angle.sine),
            zero_axes(b)};
  }
  const auto trig_v = trig_range(context, bounds[2], bounds[3], precision_bits);
  const auto minor =
      carrier.kind == 4 ? r : ExactEnclosure(Rational::from_double(b, carrier.radii[1]));
  const auto radial = product(minor, trig_v.cosine), axial = product(minor, trig_v.sine);
  const auto major_radial = carrier.kind == 4 ? radial : sum(r, radial);
  return {vector_sum(o, vector_sum(vector_times(e, major_radial), vector_times(z, axial))),
          vector_times(ep, major_radial),
          vector_sum(vector_negative(vector_times(e, axial)), vector_times(z, radial)),
          vector_negative(vector_times(e, major_radial)),
          vector_negative(vector_times(ep, axial)),
          vector_negative(vector_sum(vector_times(e, radial), vector_times(z, axial)))};
}
EnclosedPoint enclose_original_curve_range(GeometryContext &context,const GeometryStore &store,
                                            Slot<CurveTag> slot,const std::array<Rational,2> &bounds,
                                            uint32_t precision_bits) {
  owner(context,bounds[0]);owner(context,bounds[1]);
  if(bounds[0].compare(bounds[1])>0) invalid("reversed original curve image interval");
  auto &budget=context.budget();
  const auto &curves=store.view().source.view().curves;
  if(slot.value>=curves.size()) throw Error(ErrorCode::internal_error,"original curve image slot out of bounds");
  const auto &curve=curves[slot.value];
  if(curve.kind<=3) {
    const auto jet=enclose_analytic_curve_jet(context,curve,bounds[0],bounds[1],precision_bits);
    return {jet.dimension,jet.position};
  }
  if(bounds[0].compare(bounds[1])==0) return enclose_original_curve(context,store,slot,bounds[0]);
  const auto roots=intersecting_curve_roots(budget,store,slot,bounds);
  std::optional<std::array<Rational,6>> box;
  for(const auto root_slot:roots) {
    const auto root=import_curve_root(budget,store,root_slot);
    const auto begin=root.begin.compare(bounds[0])>0?root.begin:bounds[0];
    const auto end=root.end.compare(bounds[1])<0?root.end:bounds[1];
    if(begin.compare(end)>=0) continue;
    const auto leaf=restrict_curve(root,begin,end);
    const auto stride=static_cast<std::size_t>(leaf.dimension)+1;
    const auto coordinate=[&](std::size_t axis){return axis<leaf.dimension?cartesian_hull(leaf.controls,stride,axis):zero(budget);};
    const auto x=coordinate(0),y=coordinate(1),z=coordinate(2);
    const std::array<Rational,6> next{x.lower,x.upper,y.lower,y.upper,z.lower,z.upper};
    if(!box) box.emplace(next);
    else for(std::size_t axis=0;axis<3;++axis) {
      if(next[2*axis].compare((*box)[2*axis])<0) (*box)[2*axis]=next[2*axis];
      if(next[2*axis+1].compare((*box)[2*axis+1])>0) (*box)[2*axis+1]=next[2*axis+1];
    }
  }
  if(!box) throw Error(ErrorCode::internal_error,"original curve image omitted its source interval");
  return {curve.dimension,{ExactEnclosure((*box)[0],(*box)[1]),ExactEnclosure((*box)[2],(*box)[3]),ExactEnclosure((*box)[4],(*box)[5])}};
}
CurveJetEnclosure enclose_original_curve_jet(GeometryContext &context, const GeometryStore &store,
                                             Slot<CurveTag> slot, const Rational &begin,
                                             const Rational &end, uint32_t precision_bits) {
  owner(context, begin);
  owner(context, end);
  const auto &curves = store.view().source.view().curves;
  if (slot.value >= curves.size())
    throw Error(ErrorCode::internal_error, "original curve jet slot out of bounds");
  if (curves[slot.value].kind <= 3)
    return enclose_analytic_curve_jet(context, curves[slot.value], begin, end, precision_bits);
  const auto midpoint = begin.add(end).divide(Rational(context.budget(), 2));
  const auto root =
      import_curve_root(context.budget(), store, find_curve_root(store, slot, midpoint));
  return enclose_rational_curve_jet(root, begin, end);
}
SurfaceJetEnclosure enclose_original_surface_jet(GeometryContext &context,
                                                 const GeometryStore &store, Slot<CarrierTag> slot,
                                                 const std::array<Rational, 4> &bounds,
                                                 uint32_t precision_bits) {
  for (const auto &value : bounds)
    owner(context, value);
  const auto &carriers = store.view().source.view().carriers;
  if (slot.value >= carriers.size())
    throw Error(ErrorCode::internal_error, "original surface jet slot out of bounds");
  if (carriers[slot.value].kind <= 5)
    return enclose_analytic_surface_jet(context, carriers[slot.value], bounds, precision_bits);
  const auto u = bounds[0].add(bounds[1]).divide(Rational(context.budget(), 2)),
             v = bounds[2].add(bounds[3]).divide(Rational(context.budget(), 2));
  const auto root =
      import_surface_root(context.budget(), store, find_surface_root(store, slot, u, v));
  return enclose_rational_surface_jet(root, bounds);
}
SurfaceJetEnclosure enclose_original_surface_point_jet(GeometryContext &context,
                                                       const GeometryStore &store,
                                                       Slot<CarrierTag> slot, const Rational &u,
                                                       const Rational &v, uint32_t precision_bits) {
  owner(context, u);
  owner(context, v);
  const auto &carriers = store.view().source.view().carriers;
  if (slot.value >= carriers.size())
    throw Error(ErrorCode::internal_error, "original point-jet slot out of bounds");
  if (carriers[slot.value].kind <= 5)
    return enclose_analytic_surface_jet(context, carriers[slot.value], {u, u, v, v},
                                        precision_bits);
  const auto root =
      import_surface_root(context.budget(), store, find_surface_root(store, slot, u, v));
  // Source evaluation admits the owner/shape/domain and provides its exact
  // position. Derivative polynomials reuse the same de Casteljau engine, with
  // original span widths, never a vanishing-width box or finite differences.
  const auto position = evaluate_surface(root, u, v);
  auto &b = context.budget();
  const auto uw = root.bounds[1].subtract(root.bounds[0]),
             vw = root.bounds[3].subtract(root.bounds[2]);
  const auto s = u.subtract(root.bounds[0]).divide(uw), t = v.subtract(root.bounds[2]).divide(vw);
  const Tensor net({root.p, root.q}, centered(root.controls, 4));
  const auto nu = tensor_derivative(net, true, uw), nv = tensor_derivative(net, false, vw);
  const auto nuu = tensor_derivative(nu, true, uw), nuv = tensor_derivative(nu, false, vw),
             nvv = tensor_derivative(nv, false, vw);
  const auto h = tensor_value(net, s, t), hu = tensor_value(nu, s, t), hv = tensor_value(nv, s, t);
  const auto huu = tensor_value(nuu, s, t), huv = tensor_value(nuv, s, t),
             hvv = tensor_value(nvv, s, t);
  const auto c = [&](std::size_t axis) { return h[axis].divide(h[3]); };
  const auto du = [&](std::size_t axis) {
    return hu[axis].subtract(c(axis).multiply(hu[3])).divide(h[3]);
  };
  const auto dv = [&](std::size_t axis) {
    return hv[axis].subtract(c(axis).multiply(hv[3])).divide(h[3]);
  };
  const auto duu = [&](std::size_t axis) {
    return huu[axis]
        .subtract(du(axis).multiply(hu[3]).multiply(Rational(b, 2)))
        .subtract(c(axis).multiply(huu[3]))
        .divide(h[3]);
  };
  const auto duv = [&](std::size_t axis) {
    return huv[axis]
        .subtract(du(axis).multiply(hv[3]))
        .subtract(dv(axis).multiply(hu[3]))
        .subtract(c(axis).multiply(huv[3]))
        .divide(h[3]);
  };
  const auto dvv = [&](std::size_t axis) {
    return hvv[axis]
        .subtract(dv(axis).multiply(hv[3]).multiply(Rational(b, 2)))
        .subtract(c(axis).multiply(hvv[3]))
        .divide(h[3]);
  };
  return {{ExactEnclosure(position[0]), ExactEnclosure(position[1]), ExactEnclosure(position[2])},
          {ExactEnclosure(du(0)), ExactEnclosure(du(1)), ExactEnclosure(du(2))},
          {ExactEnclosure(dv(0)), ExactEnclosure(dv(1)), ExactEnclosure(dv(2))},
          {ExactEnclosure(duu(0)), ExactEnclosure(duu(1)), ExactEnclosure(duu(2))},
          {ExactEnclosure(duv(0)), ExactEnclosure(duv(1)), ExactEnclosure(duv(2))},
          {ExactEnclosure(dvv(0)), ExactEnclosure(dvv(1)), ExactEnclosure(dvv(2))}};
}
namespace geometry_detail {
Rational enclosure_norm_upper(bgfd::Budget &b,std::span<const ExactEnclosure> value,
                                uint32_t fractional_bits) {
  if (value.size()>3)
    throw Error(ErrorCode::internal_error, "geometry norm dimension exceeds its coordinate span");
  Rational squared_norm(b, 0);
  for (const auto &component:value) {
    if(&component.lower.budget()!=&b||&component.upper.budget()!=&b)
      throw Error(ErrorCode::internal_error,"geometry norm owner mismatch");
    const auto lo = component.lower.sign() < 0 ? component.lower.negative() : component.lower;
    const auto hi = component.upper.sign() < 0 ? component.upper.negative() : component.upper;
    const auto magnitude = lo.compare(hi) > 0 ? lo : hi;
    squared_norm = squared_norm.add(magnitude.multiply(magnitude));
  }
  return squared_norm.square_root_bounds(fractional_bits).upper;
}
} // namespace geometry_detail
namespace {
Rational gram_curvature_upper(bgfd::Budget &b, const Curve &curve, uint32_t bits) {
  Rational aa(b, 0), bb(b, 0), ab(b, 0);
  for (uint32_t axis = 0; axis < curve.dimension; ++axis) {
    const auto x = Rational::from_double(b, curve.frame[3 + axis])
                       .multiply(Rational::from_double(b, curve.radii[0]));
    const auto y = Rational::from_double(b, curve.frame[6 + axis])
                       .multiply(Rational::from_double(b, curve.radii[1]));
    aa = aa.add(x.multiply(x));
    bb = bb.add(y.multiply(y));
    ab = ab.add(x.multiply(y));
  }
  const auto delta = aa.subtract(bb);
  const auto discriminant = delta.multiply(delta).add(ab.multiply(ab).multiply(Rational(b, 4)));
  const auto lambda =
      aa.add(bb).add(discriminant.square_root_bounds(bits).upper).divide(Rational(b, 2));
  return lambda.square_root_bounds(bits).upper;
}
} // namespace
Rational analytic_curve_chord_upper(GeometryContext &context, const Curve &curve,
                                    const Rational &begin, const Rational &end,
                                    std::span<const Rational, 6> actual,
                                    GeometryBoundPrecision precision) {
  owner(context, begin);
  owner(context, end);
  for (const auto &value : actual)
    owner(context, value);
  if (begin.compare(end) >= 0)
    invalid("analytic chord bound requires positive source interval");
  if (precision.transcendental_bits < 32 ||
      precision.transcendental_bits > context.maximum_bits() ||
      precision.norm_fractional_bits > context.maximum_bits())
    invalid("analytic chord precision exceeds the declared profile");
  auto &b = context.budget();
  const auto jet =
      enclose_analytic_curve_jet(context, curve, begin, end, precision.transcendental_bits);
  const auto departure = [&](const Rational &parameter, std::size_t endpoint) {
    const auto point = enclose_analytic_curve_jet(context, curve, parameter, parameter,
                                                  precision.transcendental_bits);
    const Axes residual{difference(point.position[0], ExactEnclosure(actual[endpoint])),
                          difference(point.position[1], ExactEnclosure(actual[endpoint + 1])),
                          curve.dimension==3?difference(point.position[2], ExactEnclosure(actual[endpoint + 2])):zero(b)};
    return enclosure_norm_upper(b,residual,precision.norm_fractional_bits);
  };
  const auto a = departure(begin, 0), z = departure(end, 3);
  const auto endpoint_error = a.compare(z) > 0 ? a : z;
  auto curvature = enclosure_norm_upper(b,jet.second,precision.norm_fractional_bits);
  if (curve.kind != 1) {
    const auto gram = gram_curvature_upper(b, curve, precision.norm_fractional_bits);
    if (gram.compare(curvature) < 0)
      curvature = gram;
  }
  const auto width = end.subtract(begin);
  return endpoint_error.add(curvature.multiply(width.multiply(width)).divide(Rational(b, 8)));
}
} // namespace bgmesh
