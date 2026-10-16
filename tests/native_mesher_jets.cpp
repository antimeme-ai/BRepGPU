#include "../native/mesher/geometry_jets.hpp"
#include <array>
#include <iostream>
#include <stdexcept>

namespace {
using bgfd::Budget;
using bgmesh::Rational;
Rational f(Budget &b, int64_t n, int64_t d = 1) { return Rational(b, n).divide(Rational(b, d)); }
void require(bool ok, const char *message) {
  if (!ok)
    throw std::runtime_error(message);
}
void equals(const bgmesh::ExactEnclosure &value, int64_t lo, int64_t hi, int64_t denominator = 1) {
  auto &b = value.lower.budget();
  require(value.lower.compare(f(b, lo, denominator)) == 0 &&
              value.upper.compare(f(b, hi, denominator)) == 0,
          "exact original jet coefficient hull matches derivative polynomial");
}
void contains(const bgmesh::ExactEnclosure &value, const Rational &lo, const Rational &hi) {
  require(value.lower.compare(lo) <= 0 && value.upper.compare(hi) >= 0,
          "whole-source jet encloses analytic derivative extrema");
}
void curve_jets(Budget &b) {
  const std::array knots{0., 0., 0., 1., 1., 1.};
  const std::array controls{0., 0., 0., 1., .5, 0., 0., 1., 1., 1., 0., 1.};
  const auto roots = bgmesh::extract_curve(b, {3, 2, knots, controls});
  const auto jet = bgmesh::enclose_rational_curve_jet(roots[0], f(b, 1, 4), f(b, 3, 4));
  equals(jet.first[0], 1, 1);
  equals(jet.first[1], 1, 3, 2);
  equals(jet.second[0], 0, 0);
  equals(jet.second[1], 2, 2);
  // Positive rational denominator and common large origin: d/dt[t/(1+t)].
  const std::array linear_knots{0., 0., 1., 1.};
  const std::array rational{0., 0., 0., 1., 1., 0., 0., 2.};
  const std::array translated{0x1p40, 0., 0., 1., 0x1p41 + 1., 0., 0., 2.};
  const auto rr = bgmesh::extract_curve(b, {3, 1, linear_knots, rational});
  const auto rt = bgmesh::extract_curve(b, {3, 1, linear_knots, translated});
  const auto a = bgmesh::enclose_rational_curve_jet(rr[0], f(b, 0), f(b, 1));
  const auto c = bgmesh::enclose_rational_curve_jet(rt[0], f(b, 0), f(b, 1));
  equals(a.first[0], 1, 4, 4);
  equals(a.second[0], -8, -1, 4);
  require(a.first[0].lower.compare(c.first[0].lower) == 0 &&
              a.first[0].upper.compare(c.first[0].upper) == 0 &&
              a.second[0].lower.compare(c.second[0].lower) == 0 &&
              a.second[0].upper.compare(c.second[0].upper) == 0,
          "original rational jet bounds cancel common exact translation");
}
void surface_jets(Budget &b) {
  const std::array knots{0., 0., 0., 1., 1., 1.};
  std::array<double, 36> controls{};
  for (std::size_t i = 0; i < 3; ++i)
    for (std::size_t j = 0; j < 3; ++j) {
      const auto offset = (i * 3 + j) * 4;
      controls[offset] = i == 2 ? 1. : 0.;
      controls[offset + 1] = j == 2 ? 1. : 0.;
      controls[offset + 2] = static_cast<double>(i * j) / 4.;
      controls[offset + 3] = 1.;
    }
  const auto roots = bgmesh::extract_surface(b, {2, 2, 3, 3, knots, knots, controls});
  const auto jet = bgmesh::enclose_rational_surface_jet(
      roots[0], {f(b, 1, 4), f(b, 3, 4), f(b, 1, 4), f(b, 3, 4)});
  equals(jet.du[0], 1, 3, 2);
  equals(jet.du[1], 0, 0);
  equals(jet.du[2], 1, 3, 4);
  equals(jet.dv[0], 0, 0);
  equals(jet.dv[1], 1, 3, 2);
  equals(jet.dv[2], 1, 3, 4);
  equals(jet.duu[0], 2, 2);
  equals(jet.duu[2], 0, 0);
  equals(jet.duv[0], 0, 0);
  equals(jet.duv[2], 1, 1);
  equals(jet.dvv[1], 2, 2);
  // Independent quotient derivatives of (u,v,u*v)/(1+v), including mixed
  // partials and nonconstant denominator. Check exact rational grid values.
  const std::array linear{0., 0., 1., 1.};
  const std::array weighted{0., 0., 0., 1., 0., 1., 0., 2., 1., 0., 0., 1., 1., 1., 1., 2.};
  const auto rational = bgmesh::extract_surface(b, {1, 1, 2, 2, linear, linear, weighted});
  const auto r =
      bgmesh::enclose_rational_surface_jet(rational[0], {f(b, 0), f(b, 1), f(b, 0), f(b, 1)});
  for (int64_t i = 0; i <= 4; ++i)
    for (int64_t j = 0; j <= 4; ++j) {
      const auto u = f(b, i, 4), v = f(b, j, 4), w = Rational(b, 1).add(v);
      const auto w2 = w.multiply(w), w3 = w2.multiply(w);
      const std::array du{Rational(b, 1).divide(w), f(b, 0), v.divide(w)};
      const std::array dv{u.negative().divide(w2), Rational(b, 1).divide(w2), u.divide(w2)};
      const std::array duv{Rational(b, -1).divide(w2), f(b, 0), Rational(b, 1).divide(w2)};
      const std::array dvv{u.multiply(f(b, 2)).divide(w3), f(b, -2).divide(w3),
                           u.multiply(f(b, -2)).divide(w3)};
      for (std::size_t axis = 0; axis < 3; ++axis) {
        contains(r.du[axis], du[axis], du[axis]);
        contains(r.dv[axis], dv[axis], dv[axis]);
        contains(r.duv[axis], duv[axis], duv[axis]);
        contains(r.dvv[axis], dvv[axis], dvv[axis]);
      }
    }
}
void analytic_jets(Budget &b) {
  bgmesh::GeometryContext context(b);
  const bgmesh::Frame frame{0., 0., 0., 1., 0., 0., 0., 1., 0., 0., 0., 1.};
  bgmesh::Curve curve{};
  curve.kind = 2;
  curve.dimension = 3;
  curve.radii = {2., 2.};
  curve.frame = frame;
  const auto cj = bgmesh::enclose_analytic_curve_jet(context, curve, f(b, 0), f(b, 1, 2));
  contains(cj.position[0], f(b, 2), f(b, 2));
  contains(cj.first[1], f(b, 2), f(b, 2));
  contains(cj.second[0], f(b, -2), f(b, -2));
  equals(cj.second[2], 0, 0);
  static_cast<void>(context.pi(256));
  context.refine();
  const auto again = bgmesh::enclose_analytic_curve_jet(context, curve, f(b, 0), f(b, 1, 2));
  for (std::size_t axis = 0; axis < 3; ++axis) {
    require(cj.second[axis].lower.compare(again.second[axis].lower) == 0 &&
                cj.second[axis].upper.compare(again.second[axis].upper) == 0,
            "bound precision/profile independent of canonicalpoint and pi-cache history");
  }
  bgmesh::Carrier surface{};
  surface.kind = 4;
  surface.frame = frame;
  surface.radii = {2., 0.};
  const auto sj =
      bgmesh::enclose_analytic_surface_jet(context, surface, {f(b, 0), f(b, 0), f(b, 0), f(b, 0)});
  equals(sj.position[0], 2, 2);
  equals(sj.du[1], 2, 2);
  equals(sj.dv[2], 2, 2);
  equals(sj.duu[0], -2, -2);
  equals(sj.duv[0], 0, 0);
  equals(sj.dvv[0], -2, -2);
}
void actual_chord(Budget &b) {
  bgmesh::GeometryContext context(b);
  bgmesh::Curve circle{};
  circle.kind = 2;
  circle.dimension = 3;
  circle.radii = {2., 2.};
  circle.frame = {0., 0., 0., 1., 0., 0., 0., 1., 0., 0., 0., 1.};
  const auto a = f(b, 0), z = f(b, 1, 2);
  const auto qa = bgmesh::canonical_analytic_curve(context, circle, a),
             qz = bgmesh::canonical_analytic_curve(context, circle, z);
  std::array<Rational, 6> actual{
      Rational::from_double(b, qa.xyz[0]), Rational::from_double(b, qa.xyz[1]),
      Rational::from_double(b, qa.xyz[2]), Rational::from_double(b, qz.xyz[0]),
      Rational::from_double(b, qz.xyz[1]), Rational::from_double(b, qz.xyz[2])};
  const auto bound = bgmesh::analytic_curve_chord_upper(context, circle, a, z, actual);
  require(bound.compare(f(b, 1, 16)) >= 0 && bound.compare(f(b, 1, 16).add(f(b, 1, 1LL << 40))) < 0,
          "analytic chord uses exact frame curvature and actual endpoint departure once");
  actual[0] = actual[0].add(f(b, 1, 8));
  require(bgmesh::analytic_curve_chord_upper(context, circle, a, z, actual).compare(f(b, 3, 16)) ==
              0,
          "known actual endpoint offset appears once in chord upper");
  // Nonorthogonal ellipse axes; largest Gram eigenvalue, not sum of axis norms.
  circle.radii = {1., 1.};
  circle.frame = {0., 0., 0., 1., 0., 0., 1., 1., 0., 0., 0., 1.};
  const auto lo = f(b, -2), hi = f(b, 2);
  const auto qlo = bgmesh::canonical_analytic_curve(context, circle, lo),
             qhi = bgmesh::canonical_analytic_curve(context, circle, hi);
  for (std::size_t axis = 0; axis < 3; ++axis) {
    actual[axis] = Rational::from_double(b, qlo.xyz[axis]);
    actual[3 + axis] = Rational::from_double(b, qhi.xyz[axis]);
  }
  const auto skew = bgmesh::analytic_curve_chord_upper(context, circle, lo, hi, actual);
  require(skew.compare(f(b, 323, 100)) > 0 && skew.compare(f(b, 324, 100)) < 0,
          "nonorthogonal analytic frame uses its exact two-column Gram bound");
}
} // namespace
int main() {
  try {
    Budget b({uint64_t{1} << 28, uint64_t{1} << 34});
    curve_jets(b);
    surface_jets(b);
    analytic_jets(b);
    actual_chord(b);
    std::cout << "PASS\toriginal source jets\twork=" << b.work() << "\tpeak_bytes=" << b.peak()
              << '\n';
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "FAIL\t" << e.what() << '\n';
    return 1;
  } catch (const bgfd::Failure &e) {
    std::cerr << "FAIL\t" << e.what() << '\n';
    return 1;
  }
}
