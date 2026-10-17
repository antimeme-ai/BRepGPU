#include "../native/mesher/metrics.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool condition, const char *message) {
  if (!condition)
    throw std::runtime_error(message);
}
std::array<bgmesh::ExactEnclosure, 3> axes(bgfd::Budget &b, std::array<int64_t, 3> v) {
  return {bgmesh::ExactEnclosure(bgmesh::Rational(b, v[0])),
          bgmesh::ExactEnclosure(bgmesh::Rational(b, v[1])),
          bgmesh::ExactEnclosure(bgmesh::Rational(b, v[2]))};
}
bgmesh::SurfaceJetEnclosure ridge(bgfd::Budget &b, int64_t k) {
  return {axes(b, {0, 0, 0}),     axes(b, {1, 0, 0}),      axes(b, {0, 1, 0}),
          axes(b, {0, 0, 2 * k}), axes(b, {0, 0, -2 * k}), axes(b, {0, 0, 2 * k})};
}
bgmesh::Rational length(const std::array<bgmesh::Rational, 4> &factor, std::array<int64_t, 2> v) {
  auto &b = factor[0].budget();
  const auto x = bgmesh::Rational(b, v[0]), y = bgmesh::Rational(b, v[1]);
  const auto a = factor[0].multiply(x).add(factor[1].multiply(y));
  const auto c = factor[2].multiply(x).add(factor[3].multiply(y));
  return a.multiply(a).add(c.multiply(c));
}
void metric_families() {
  bgfd::Budget b({1u << 28, uint64_t{1} << 34});
  const bgmesh::Rational one(b, 1), tau = one.divide(bgmesh::Rational(b, 1000));
  auto previous = bgmesh::Rational(b, 0);
  for (const int64_t k : {0, 1, 16, 256}) {
    const bgmesh::Rational h2(b, 2 + k * k);
    const auto factor = bgmesh::metric_from_point_jet(ridge(b, k), {h2, one, one, tau});
    require(factor[0].multiply(factor[3]).subtract(factor[1].multiply(factor[2])).sign() > 0,
            "metric lost SPD factor orientation");
    const auto ratio = length(factor, {1, -1}).divide(length(factor, {1, 1}));
    if (k == 0)
      require(ratio.compare(one) == 0, "flat isotropic control was distorted");
    else
      require(ratio.compare(previous) > 0,
              "rotated curvature family did not increase directional resolution");
    previous = ratio;
    std::cout << "metric_ridge_k\t" << k << "\tdirectional_ratio\t" << ratio.to_double() << '\n';
  }
  const bgmesh::Rational h2(b, 3);
  const auto up = bgmesh::metric_from_point_jet(ridge(b, 1), {h2, one, one, tau});
  const auto down = bgmesh::metric_from_point_jet(ridge(b, -1), {h2, one, one, tau});
  for (unsigned i = 0; i < 4; ++i)
    require(up[i].compare(down[i]) == 0, "negative curvature sign changed spectral absolute");
  const auto saddle =
      bgmesh::SurfaceJetEnclosure{axes(b, {0, 0, 0}), axes(b, {1, 0, 0}), axes(b, {0, 1, 0}),
                                  axes(b, {0, 0, 0}), axes(b, {0, 0, 3}), axes(b, {0, 0, 0})};
  const auto saddle_factor = bgmesh::metric_from_point_jet(saddle, {h2, one, one, tau});
  require(length(saddle_factor, {1, 0}).compare(length(saddle_factor, {0, 1})) == 0 &&
              length(saddle_factor, {1, 1}).compare(length(saddle_factor, {1, -1})) == 0,
          "indefinite mixed curvature absolute is not isotropic");
  const auto zero =
      bgmesh::SurfaceJetEnclosure{axes(b, {0, 0, 0}), axes(b, {1, 0, 0}), axes(b, {2, 0, 0}),
                                  axes(b, {0, 0, 0}), axes(b, {0, 0, 0}), axes(b, {0, 0, 0})};
  bool refused = false;
  try {
    (void)bgmesh::metric_from_point_jet(zero, {one, one, one, tau});
  } catch (const bgmesh::Error &) {
    refused = true;
  }
  require(refused, "zero source normal silently became isotropic metric");
}
void exact_proposals() {
  bgfd::Budget b({1u << 27, uint64_t{1} << 33});
  const std::array<bgmesh::Rational, 4> identity{bgmesh::Rational(b, 1), bgmesh::Rational(b, 0),
                                                 bgmesh::Rational(b, 0), bgmesh::Rational(b, 1)};
  const std::array<bgmesh::Rational, 6> triangle{bgmesh::Rational(b, 0), bgmesh::Rational(b, 0),
                                                 bgmesh::Rational(b, 1), bgmesh::Rational(b, 0),
                                                 bgmesh::Rational(b, 0), bgmesh::Rational(b, 1)};
  const auto center = bgmesh::metric_circumcenter(triangle, identity);
  require(center == std::array<double, 2>{.5, .5},
          "exact metric2x2 solve returned wrong chosen point");
  const auto phi = bgmesh::metric_shape_potential(triangle, identity);
  if (!phi)
    throw std::runtime_error("finite shape potential rejected");
  require(*phi == bgmesh::Rational(b, 4).divide(bgmesh::Rational(b, 3)).to_double(),
          "Phi was not once-rounded exact rational expression");
  const std::array<bgmesh::Rational, 4> scaled{bgmesh::Rational(b, 2), bgmesh::Rational(b, 0),
                                               bgmesh::Rational(b, 0), bgmesh::Rational(b, 2)};
  require(bgmesh::metric_shape_potential(triangle, scaled) == phi,
          "shape potential depends on scalar metric normalization");
  const std::array<bgmesh::Rational,4>rotated{bgmesh::Rational(b,1),bgmesh::Rational(b,-1),
    bgmesh::Rational::from_double(b,.125),bgmesh::Rational::from_double(b,.125)};
  require(bgmesh::metric_circumcenter(triangle,rotated)==std::array<double,2>{16.25,16.25},
          "full metric circumcenter discarded mixed entries");
  require(bgmesh::metric_shape_potential(triangle,rotated)==
          bgmesh::Rational(b,37249).divide(bgmesh::Rational(b,768)).to_double(),
          "full metric Phi discarded mixed entries or determinant");
  const auto two = bgmesh::Rational(b, 2);
  const auto root = bgmesh::rounded_sqrt53(two);
  require(root.compare(bgmesh::Rational::from_double(b, 0x1.6a09e667f3bcdp+0)) == 0,
          "scaled53 sqrt2 rounding wrong");
  const std::array<uint32_t, 1> word{1};
  const auto p = bgmesh::Rational::from_ratio(bgfd::Integer::from_words(b, 1, word),
                                              bgfd::Integer::from_words(b, 1, word).shift_left(53));
  const auto midpoint = bgmesh::Rational(b, 1).add(p);
  require(bgmesh::rounded_sqrt53(midpoint.multiply(midpoint)).compare(bgmesh::Rational(b, 1)) == 0,
          "sqrt53 exact midpoint did not ties-to-even");
  const auto tiny =
      bgmesh::Rational::from_ratio(bgfd::Integer::from_words(b, 1, word),
                                   bgfd::Integer::from_words(b, 1, word).shift_left(4000));
  const auto expected =
      bgmesh::Rational::from_ratio(bgfd::Integer::from_words(b, 1, word),
                                   bgfd::Integer::from_words(b, 1, word).shift_left(2000));
  require(bgmesh::rounded_sqrt53(tiny).compare(expected) == 0,
          "scaled sqrt underflowed separate exponent");
  const auto perturbation = bgmesh::Rational::from_ratio(
      bgfd::Integer::from_words(b, 1, word), bgfd::Integer::from_words(b, 1, word).shift_left(200));
  const auto boundary = midpoint.multiply(midpoint);
  require(bgmesh::rounded_sqrt53(boundary.subtract(perturbation)).compare(bgmesh::Rational(b, 1)) ==
                  0 &&
              bgmesh::rounded_sqrt53(boundary.add(perturbation))
                      .compare(bgmesh::Rational(b, 1).add(p.multiply(bgmesh::Rational(b, 2)))) == 0,
          "sqrt53 ambiguous bracket ignored exact squared-midpoint side");
  const auto subnormal = bgmesh::Rational::from_double(b, 0x1p-1074);
  const std::array<bgmesh::Rational, 6> flat{bgmesh::Rational(b, 0), bgmesh::Rational(b, 0),
                                             bgmesh::Rational(b, 1), bgmesh::Rational(b, 0),
                                             bgmesh::Rational(b, 2), subnormal};
  const auto huge_center = bgmesh::metric_circumcenter(flat, identity);
  require(huge_center[0] == .5 && std::isinf(huge_center[1]) &&
              !bgmesh::metric_shape_potential(flat, identity),
          "overflowed proposal/Phi was not routine rejection");
}
void original_consumer(const char *path) {
  auto resource = std::make_shared<bgmesh::RunResource>(uint64_t{1} << 28);
  const auto source = bgmesh::load_source(path, resource);
  bgfd::Budget b({1u << 28, uint64_t{1} << 35});
  const auto geometry =
      bgmesh::compile_geometry(source, resource, {{bgmesh::OwnerKind::geometry, 0, 4}, 0, 0}, b);
  bgmesh::GeometryContext context(b);
  const bgmesh::Rational one(b, 1), h2(b, 3), tau = one.divide(bgmesh::Rational(b, 1000));
  const std::array<bgmesh::Rational, 2> point{one.divide(bgmesh::Rational(b, 2)),
                                              one.divide(bgmesh::Rational(b, 2))};
  for (const uint64_t source_id : std::array<uint64_t, 2>{9, 12}) {
    const auto carrier = source.view().carrier_index.find(bgmesh::CarrierId{source_id});
    const auto factor =
        bgmesh::original_source_metric(context, geometry, carrier, point, {h2, one, one, tau});
    require(factor[0].multiply(factor[3]).subtract(factor[1].multiply(factor[2])).sign() > 0,
            "actual original weighted/knot metric consumer lost SPD");
  }
}
} // namespace
int main(int argc, char **argv) {
  try {
    require(argc == 2, "expected original geometry fixture path");
    metric_families();
    exact_proposals();
    original_consumer(argv[1]);
    std::cout << "native source metrics/exact proposals PASS\n";
  } catch (const bgfd::Failure &e) {
    std::cerr << "native metric failure: " << e.what() << '\n';
    return 1;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
