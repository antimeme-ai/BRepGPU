#include "../native/mesher/geometry_poles.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
namespace {
using namespace bgmesh;
void require(bool ok, const char *message) {
  if (!ok)
    throw std::runtime_error(message);
}
Rational f(bgfd::Budget &b, int64_t n, int64_t d = 1) {
  return Rational(b, n).divide(Rational(b, d));
}
void check(AnalyticPoleKind kind, bool skew) {
  const auto resource = std::make_shared<RunResource>(uint64_t{1} << 28);
  bgfd::Budget b({uint64_t{1} << 27, uint64_t{1} << 36}, resource.get());
  GeometryContext context(b);
  Carrier carrier{};
  carrier.kind = kind == AnalyticPoleKind::cone_apex ? 3U : 4U;
  carrier.frame = {0., 0., 0., 1., 0., 0., skew ? .25 : 0., 1., 0., skew ? .125 : 0., 0., 1.};
  carrier.radii = carrier.kind == 3 ? std::array<double, 2>{0., .5} : std::array<double, 2>{1., 0.};
  const bool north = kind == AnalyticPoleKind::sphere_north;
  const auto v = carrier.kind == 3 ? f(b, 1, 8) : north ? f(b, 3, 2) : f(b, -3, 2);
  const auto vp = carrier.kind == 3
                      ? f(b, 0)
                      : Rational::from_double(b, north ? 1.5707963267948966 : -1.5707963267948966);
  const auto right_v = skew ? v.add(f(b, 1, 16)) : Rational(v);
  const auto left = canonical_analytic_surface(context, carrier, f(b, 0), v),
             right = canonical_analytic_surface(context, carrier, f(b, 1, 2), right_v);
  const Vec3 pole{carrier.kind == 3 ? 0. : (north ? carrier.frame[9] : -carrier.frame[9]), 0.,
                  carrier.kind == 3 ? 0. : (north ? 1. : -1.)};
  PoleFanRequest request{kind,
                         {f(b, 0), v, f(b, 1, 2), right_v},
                         {pole[0], pole[1], pole[2], left.xyz[0], left.xyz[1], left.xyz[2],
                          right.xyz[0], right.xyz[1], right.xyz[2]},
                         {vp, vp},
                         1,
                         {}};
  const auto plan = bound_analytic_pole_fan(context, carrier, request);
  require(plan.facet_roles ==
              (north ? std::array<uint32_t, 3>{0, 1, 2} : std::array<uint32_t, 3>{0, 2, 1}),
          "source xi/rho orientation determines emitted facet role permutation");
  const long double pi = acosl(-1.L), polev = carrier.kind == 3 ? 0.L : north ? pi / 2 : -pi / 2;
  const auto upper = static_cast<long double>(plan.position_upper.to_double());
  for (unsigned i = 0; i <= 8; ++i)
    for (unsigned j = 0; j <= 8; ++j) {
      const long double xi = static_cast<long double>(i) / 8, rho = static_cast<long double>(j) / 8;
      const long double u = xi / 2,
                        vv = polev +
                             rho * ((1 - xi) * static_cast<long double>(v.to_double()) +
                                    xi * static_cast<long double>(right_v.to_double()) - polev);
      const long double radius = carrier.kind == 3 ? vv * sinl(.5L) : cosl(vv);
      const long double height = carrier.kind == 3 ? vv * cosl(.5L) : sinl(vv);
      const long double xx = radius * cosl(u), yy = radius * sinl(u);
      const long double x = xx + (skew ? .25L : 0) * yy + (skew ? .125L : 0) * height;
      const std::array<long double, 3> source{x, yy, height};
      long double norm2 = 0;
      for (std::size_t a = 0; a < 3; ++a) {
        const long double phi =
            (1 - rho) * request.actual_xyz[a] +
            rho * ((1 - xi) * request.actual_xyz[3 + a] + xi * request.actual_xyz[6 + a]);
        const long double d = source[a] - phi;
        norm2 += d * d;
      }
      require(sqrtl(norm2) <= upper + 1e-16L,
              "independent analytic source-Phi samples fit entire-cell bound");
    }
  auto displaced = request;
  displaced.actual_xyz[0] += .0625;
  const auto offset = bound_analytic_pole_fan(context, carrier, displaced);
  require(offset.position_upper.compare(
              plan.position_upper.add(f(b, 1, 16)).subtract(f(b, 1, 1LL << 40))) >= 0,
          "actual pole-node departure is charged once at four-corner comparison");
  require(offset.pole_departure_upper.compare(f(b, 1, 16)) == 0,
          "pole-only fixed departure excludes changing cut corners");
  require(plan.band_upper.sign() >= 0,
          "finite original pole boundary has separate source-domain band term");
  auto zero = request;
  for (std::size_t a = 0; a < 3; ++a) {
    zero.actual_xyz[3 + a] = zero.actual_xyz[a];
    zero.actual_xyz[6 + a] = zero.actual_xyz[a];
  }
  bool refused = false;
  try {
    static_cast<void>(bound_analytic_pole_fan(context, carrier, zero));
  } catch (const Error &error) {
    refused = error.code == ErrorCode::invalid_source;
  }
  require(refused, "zero physical facets never stand in for source quotient coverage");
}
void finite_band() {
  const auto resource = std::make_shared<RunResource>(uint64_t{1} << 28);
  bgfd::Budget b({uint64_t{1} << 27, uint64_t{1} << 36}, resource.get());
  GeometryContext context(b);
  Carrier carrier{};
  carrier.kind = 4;
  carrier.frame = {0., 0., 0., 1., 0., 0., 0., 1., 0., .25, 0., 100.};
  carrier.radii = {1., 0.};
  const auto cut = f(b, 1, 4), finite = f(b, 3, 2);
  const auto left = canonical_analytic_surface(context, carrier, f(b, 0), cut),
             right = canonical_analytic_surface(context, carrier, f(b, 1, 2), cut);
  const auto plan = bound_analytic_pole_fan(context, carrier,
                                            {AnalyticPoleKind::sphere_north,
                                             {f(b, 0), cut, f(b, 1, 2), cut},
                                             {.25, 0., 100., left.xyz[0], left.xyz[1], left.xyz[2],
                                              right.xyz[0], right.xyz[1], right.xyz[2]},
                                             {finite, finite},
                                             1,
                                             {}});
  // Independent original analytic formula at xi=rho=1/2: the finite-v
  // correspondence differs throughout the cap, not just at its pole strip.
  const long double ideal = (acosl(-1.L) / 2 + .25L) / 2, source = (1.5L + .25L) / 2;
  const long double dx =
      (cosl(source) - cosl(ideal)) * cosl(.25L) + .25L * (sinl(source) - sinl(ideal));
  const long double dy = (cosl(source) - cosl(ideal)) * sinl(.25L),
                    dz = 100 * (sinl(source) - sinl(ideal));
  require(static_cast<long double>(plan.band_upper.to_double()) >=
              sqrtl(dx * dx + dy * dy + dz * dz),
          "whole finite-to-ideal affine-rho band must bound interior source correspondence on "
          "nonorthogonal sphere");
}
} // namespace
int main() {
  try {
    finite_band();
    check(AnalyticPoleKind::sphere_north, false);
    check(AnalyticPoleKind::sphere_south, true);
    check(AnalyticPoleKind::cone_apex, true);
    std::cout << "PASS\toriginal analytic pole fan bounds\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "FAIL\t" << e.what() << '\n';
    return 1;
  } catch (const bgfd::Failure &e) {
    std::cerr << "FAIL\t" << e.what() << '\n';
    return 1;
  }
}
