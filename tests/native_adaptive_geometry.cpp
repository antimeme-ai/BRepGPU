#include "../native/mesher/adaptive_geometry.hpp"
#include "../native/mesher/adaptive_numeric.hpp"
#include "native_adaptive_device_test.hpp"
#include <array>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string_view>

namespace {
const char *device_module = nullptr;
int32_t selected_device = 0;
std::unique_ptr<adaptive_test::DeviceFixture>
device_fixture(const bgmesh::adaptive::SourceCache &cache) {
  if (!device_module)
    return {};
  return std::make_unique<adaptive_test::DeviceFixture>(cache, device_module, selected_device);
}
bgmesh::adaptive::SampleResult sample_value(adaptive_test::DeviceFixture *gpu,
                                            const bgmesh::adaptive::SourceCache &cache,
                                            const bgmesh::adaptive::SampleInput &input) {
  return gpu ? gpu->sample(input) : bgmesh::adaptive::evaluate_sample(cache, input);
}
bgmesh::adaptive::NodeBounds node_value(adaptive_test::DeviceFixture *gpu,
                                        const bgmesh::adaptive::SourceCache &cache,
                                        const bgmesh::adaptive::NodePredicate &input,
                                        std::span<const bgmesh::adaptive::Interval> controls = {}) {
  return gpu ? gpu->node(cache, input) : bgmesh::adaptive::bound_node(cache, input, controls);
}
bgmesh::adaptive::ActualFacetBounds facet_value(adaptive_test::DeviceFixture *gpu,
                                                const bgmesh::adaptive::ActualFacetInput &input) {
  return gpu ? gpu->facet(input) : bgmesh::adaptive::bound_actual_facet(input);
}
void require(bool ok, const char *why) {
  if (!ok)
    throw std::runtime_error(why);
}

int fallback_case(const char *path) {
  auto resource = std::make_shared<bgmesh::RunResource>(uint64_t{1} << 28);
  const auto source = bgmesh::load_source(path, resource);
  bgfd::Budget budget({uint64_t{1} << 27, uint64_t{1} << 33});
  const auto slot = source.view().carrier_index.find(bgmesh::CarrierId{9});
  const std::array requests{bgmesh::adaptive::CarrierRequest{slot, {0., 1., 0., 1.}}};
  const auto cache =
      bgmesh::adaptive::prepare_source_cache(source, requests, resource, {}, {}, budget);
  require(cache.roots().size() == 1,
          "positive extreme-weight source prepares selected original root");
  const auto &root = cache.original_roots()[0];
  for (uint32_t i = 3; i < root.controls.count; i += 4)
    require(cache.controls()[root.controls.first + i].lower > 0.,
            "precise selected-root positive coefficients");
  const std::array uv{bgmesh::Rational(budget, 1), bgmesh::Rational(budget, 1)};
  const auto value = bgmesh::adaptive::evaluate_sample_precise(
      cache, root.original, uv, bgmesh::adaptive::ArithmeticStage::p128, budget);
  require(value.original_xyz[0].lower <= 1. && value.original_xyz[0].upper >= 1. &&
              value.original_xyz[1].lower <= 1. && value.original_xyz[1].upper >= 1. &&
              value.actual_xyz[2] == 0.,
          "original endpoint survives extreme positive weight range");
  std::cout << "selected-root-fallback\tPASS\n";
  return 0;
}

int extrusion_case(const char *path, int kind) {
  auto resource = std::make_shared<bgmesh::RunResource>(uint64_t{1} << 28);
  const auto source = bgmesh::load_source(path, resource);
  bgfd::Budget budget({uint64_t{1} << 27, uint64_t{1} << 33});
  const auto slot = source.view().carrier_index.find(bgmesh::CarrierId{9});
  const std::array requests{bgmesh::adaptive::CarrierRequest{slot, {0., 1., 0., 1.}}};
  const auto cache =
      bgmesh::adaptive::prepare_source_cache(source, requests, resource, {}, {}, budget);
  auto gpu = device_fixture(cache);
  const auto &root = cache.roots()[0];
  const auto expected = kind == 101 ? 2U : kind == 102 ? 1U : 0U;
  require(root.affine_separable_axes == expected,
          "original coefficient extrusion identity, not box aspect");
  bgmesh::adaptive::NodePredicate node;
  node.root = {0};
  node.controls = root.controls;
  for (size_t a = 0; a < 4; ++a)
    node.box[a] = root.prepared_box[a];
  for (size_t i = 0; i < 4; ++i) {
    bgmesh::adaptive::SampleInput input;
    input.original = root.original;
    input.uv[0] = {static_cast<double>(i % 2), static_cast<double>(i % 2)};
    input.uv[1] = {static_cast<double>(i / 2), static_cast<double>(i / 2)};
    const auto point = sample_value(gpu.get(), cache, input);
    require(point.status == bgmesh::adaptive::NumericStatus::ready, "source extrusion sample");
    for (size_t a = 0; a < 3; ++a)
      node.actual_corners[3 * i + a] = point.actual_xyz[a];
    node.corner_departures[i] = point.departure_upper;
  }
  const auto bound = node_value(gpu.get(), cache, node);
  require(bound.status == bgmesh::adaptive::NumericStatus::ready, "rational directional bound");
  require(kind == 102 ? bound.score_v > bound.score_u : bound.score_u > bound.score_v,
          "source directional curvature dominates flat extrusion, independent square box");
  std::cout << std::hexfloat << "extrusion-leaf\t" << kind << '\t'
            << bound.source_to_bilinear_upper;
  for (const auto q : node.actual_corners)
    std::cout << '\t' << q;
  std::cout << '\n';
  if (gpu)
    gpu->close();
  return 0;
}
int analytic_case(const char *path, int kind) {
  auto resource = std::make_shared<bgmesh::RunResource>(uint64_t{1} << 28);
  const auto source = bgmesh::load_source(path, resource);
  bgfd::Budget budget({uint64_t{1} << 27, uint64_t{1} << 33});
  const auto slot = source.view().carrier_index.find(bgmesh::CarrierId{9});
  const std::array requests{bgmesh::adaptive::CarrierRequest{slot, {0., 1., 0., 1.}}};
  const auto cache =
      bgmesh::adaptive::prepare_source_cache(source, requests, resource, {}, {}, budget);
  auto gpu = device_fixture(cache);
  const auto &root = cache.roots()[0];
  std::cout << std::hexfloat;
  for (int i = 0; i <= 4; ++i)
    for (int j = 0; j <= 4; ++j) {
      bgmesh::adaptive::SampleInput input;
      input.original = root.original;
      input.uv[0] = {i / 4., i / 4.};
      input.uv[1] = {j / 4., j / 4.};
      const auto value = sample_value(gpu.get(), cache, input);
      require(value.status == bgmesh::adaptive::NumericStatus::ready,
              "analytic primary source point");
      std::cout << "analytic\t" << kind << '\t' << i / 4. << '\t' << j / 4.;
      for (const auto x : value.original_xyz)
        std::cout << '\t' << x.lower << '\t' << x.upper;
      for (const auto x : value.actual_xyz)
        std::cout << '\t' << x;
      std::cout << '\t' << value.departure_upper << '\n';
    }
  bgmesh::adaptive::NodePredicate node;
  node.root = {0};
  for (size_t a = 0; a < 4; ++a)
    node.box[a] = root.prepared_box[a];
  for (size_t i = 0; i < 4; ++i) {
    bgmesh::adaptive::SampleInput input;
    input.original = root.original;
    input.uv[0] = {static_cast<double>(i % 2), static_cast<double>(i % 2)};
    input.uv[1] = {static_cast<double>(i / 2), static_cast<double>(i / 2)};
    const auto value = sample_value(gpu.get(), cache, input);
    for (size_t a = 0; a < 3; ++a)
      node.actual_corners[3 * i + a] = value.actual_xyz[a];
    node.corner_departures[i] = value.departure_upper;
  }
  const auto bound = node_value(gpu.get(), cache, node);
  require(bound.status == bgmesh::adaptive::NumericStatus::ready && bound.regularity_proved,
          "admitted analytic regular source bound");
  std::cout << "analytic-leaf\t" << kind << '\t' << bound.source_to_bilinear_upper;
  for (const auto q : node.actual_corners)
    std::cout << '\t' << q;
  std::cout << '\n';
  if (gpu)
    gpu->close();
  return 0;
}
} // namespace
int main(int argc, char **argv) {
  try {
    int effective = argc;
    if (argc >= 5 && std::string_view(argv[argc - 3]) == "--cuda") {
      device_module = argv[argc - 2];
      selected_device = std::stoi(argv[argc - 1]);
      effective -= 3;
    }
    require(effective == 2 || effective == 3, "source fixture path required");
    if (effective == 3) {
      const auto kind = std::stoi(argv[2]);
      if (kind == 100)
        return fallback_case(argv[1]);
      if (kind >= 101 && kind <= 103)
        return extrusion_case(argv[1], kind);
      return analytic_case(argv[1], kind);
    }
    auto resource = std::make_shared<bgmesh::RunResource>(uint64_t{1} << 28);
    auto source = bgmesh::load_source(argv[1], resource);
    bgfd::Budget budget({uint64_t{1} << 27, uint64_t{1} << 33});
    const auto slot = source.view().carrier_index.find(bgmesh::CarrierId{9});
    const std::array requests{bgmesh::adaptive::CarrierRequest{slot, {0., 1., 0., 1.}}};
    const auto cache =
        bgmesh::adaptive::prepare_source_cache(source, requests, resource, {}, {}, budget);
    require(cache.roots().size() == 1 && cache.original_roots().size() == 1,
            "requested-only original/prepared closure");
    auto gpu = device_fixture(cache);
    const auto root = cache.roots()[0];
    bgmesh::adaptive::SampleInput point;
    point.original = root.original;
    point.uv[0] = {.5, .5};
    point.uv[1] = {.5, .5};
    const auto sampled = sample_value(gpu.get(), cache, point);
    require(sampled.status == bgmesh::adaptive::NumericStatus::ready,
            "original rational source point evaluated");
    // Independent original H net gives (u*(1+v),2v,2uv)/(1+v), not cached span hints.
    const std::array truth{.5, 2. / 3., 1. / 3.};
    for (size_t a = 0; a < 3; ++a)
      require(sampled.original_xyz[a].lower <= truth[a] &&
                  sampled.original_xyz[a].upper >= truth[a],
              "original rational point enclosure");
    bgmesh::adaptive::ActualFacetInput facet;
    // Flat actual bilinear map, deliberately displaced affine triangle.
    const double corners[]{0., 0., 0., 1., 0., 0., 0., 1., 0., 1., 1., 0.};
    for (size_t a = 0; a < 12; ++a)
      facet.actual_corners[a] = corners[a];
    const double uv[]{0., 0., 1., 0., 0., 1.};
    for (size_t a = 0; a < 6; ++a)
      facet.uv[a] = {uv[a], uv[a]};
    for (size_t i = 0; i < 3; ++i) {
      facet.affine_xyz[3 * i] = {uv[2 * i], uv[2 * i]};
      facet.affine_xyz[3 * i + 1] = {uv[2 * i + 1], uv[2 * i + 1]};
      facet.affine_xyz[3 * i + 2] = {.125, .125};
    }
    const auto bound = facet_value(gpu.get(), facet);
    require(bound.status == bgmesh::adaptive::NumericStatus::ready && bound.total_upper >= .125 &&
                bound.total_upper < .126,
            "actual displaced L cannot use source-only flatness");

    const auto f = [&](int64_t n, int64_t d = 1) {
      return bgmesh::Rational(budget, n).divide(bgmesh::Rational(budget, d));
    };
    const std::array exact_uv{f(0), f(0), f(1), f(0), f(0), f(1)};
    const std::array exact_l{f(0), f(0), f(1, 8), f(1), f(0), f(1, 8), f(0), f(1), f(1, 8)};
    require(bgmesh::adaptive::bound_actual_facet_precise(facet, exact_uv, exact_l, f(1, 8), budget)
                .certified,
            "exact facet allowance equality accepts through squared residual");
    require(!bgmesh::adaptive::bound_actual_facet_precise(facet, exact_uv, exact_l, f(1, 9), budget)
                 .certified,
            "precise positive residual cannot accept too-small allowance");
    std::cout << std::hexfloat;
    // The source and SAME actual L are known exactly: B=(u,v,uv),
    // L=(u,v,v+shift) on 0<=v<=u<=1. Their maximum error is 1/4+shift.
    auto twist_facet = facet;
    twist_facet.actual_corners[11] = 1.;
    const std::array twist_uv{f(0), f(0), f(1), f(0), f(1), f(1)};
    for (const auto shift : {0., .125}) {
      const double coordinates[]{0., 0., 1., 0., 1., 1.};
      for (size_t a = 0; a < 6; ++a)
        twist_facet.uv[a] = {coordinates[a], coordinates[a]};
      for (size_t i = 0; i < 3; ++i) {
        twist_facet.affine_xyz[3 * i] = {coordinates[2 * i], coordinates[2 * i]};
        twist_facet.affine_xyz[3 * i + 1] = {coordinates[2 * i + 1], coordinates[2 * i + 1]};
        const auto z = coordinates[2 * i] * coordinates[2 * i + 1] + shift;
        twist_facet.affine_xyz[3 * i + 2] = {z, z};
      }
      const auto transfer = facet_value(gpu.get(), twist_facet);
      require(transfer.status == bgmesh::adaptive::NumericStatus::ready &&
                  transfer.bilinear_to_facet_upper >= .25 + shift &&
                  transfer.bilinear_to_facet_upper < .251 + shift,
              "actual quarter-twist transfer retains displaced-L error");
      const auto dz = bgmesh::Rational::from_double(budget, shift);
      const std::array twist_l{f(0), f(0), dz, f(1), f(0), dz, f(1), f(1), f(1).add(dz)};
      require(bgmesh::adaptive::bound_actual_facet_precise(twist_facet, twist_uv, twist_l,
                                                           f(1, 4).add(dz), budget)
                  .certified,
              "precise quarter-twist equality accepts SAME actual L");
      require(!bgmesh::adaptive::bound_actual_facet_precise(twist_facet, twist_uv, twist_l,
                                                            f(1, 5).add(dz), budget)
                   .certified,
              "quarter-twist transfer refuses insufficient allowance");
      std::cout << "twist-facet\t" << shift << '\t' << transfer.bilinear_to_facet_upper << '\n';
    }
    std::cout << std::hexfloat;
    for (int i = 0; i <= 8; ++i)
      for (int j = 0; j <= 8; ++j) {
        point.uv[0] = {i / 8., i / 8.};
        point.uv[1] = {j / 8., j / 8.};
        const auto value = sample_value(gpu.get(), cache, point);
        require(value.status == bgmesh::adaptive::NumericStatus::ready,
                "original grid sample ready");
        std::cout << "point\t" << i / 8. << '\t' << j / 8.;
        for (const auto x : value.original_xyz)
          std::cout << '\t' << x.lower << '\t' << x.upper;
        for (const auto x : value.actual_xyz)
          std::cout << '\t' << x;
        std::cout << '\t' << value.departure_upper << '\n';
      }
    const std::array exact_point{f(1, 3), f(2, 3)};
    const auto precise = bgmesh::adaptive::evaluate_sample_precise(
        cache, root.original, exact_point, bgmesh::adaptive::ArithmeticStage::p128, budget);
    require(precise.status == bgmesh::adaptive::NumericStatus::ready,
            "precise original parameter sample");
    std::cout << "precise\t";
    for (const auto x : precise.original_xyz)
      std::cout << x.lower << '\t' << x.upper << '\t';
    std::cout << "end\n";
    bgmesh::adaptive::NodePredicate node;
    node.root = {0};
    node.controls = root.controls;
    for (size_t a = 0; a < 4; ++a)
      node.box[a] = root.prepared_box[a];
    for (size_t i = 0; i < 4; ++i) {
      point.uv[0] = {static_cast<double>(i % 2), static_cast<double>(i % 2)};
      point.uv[1] = {static_cast<double>(i / 2), static_cast<double>(i / 2)};
      const auto value = sample_value(gpu.get(), cache, point);
      for (size_t a = 0; a < 3; ++a)
        node.actual_corners[3 * i + a] = value.actual_xyz[a];
      node.corner_departures[i] = value.departure_upper;
    }
    const auto leaf = node_value(gpu.get(), cache, node);
    require(leaf.status == bgmesh::adaptive::NumericStatus::ready,
            "original rational residual bound ready");
    std::cout << "leaf\t" << leaf.source_to_bilinear_upper;
    for (const auto q : node.actual_corners)
      std::cout << '\t' << q;
    std::cout << '\n';

    const auto count = root.controls.count;
    std::array<bgmesh::adaptive::Interval, 64> child_controls{};
    const auto parent = cache.controls().subspan(root.controls.first, count);
    for (uint32_t k = 0; k < count; ++k)
      child_controls[k] = bgmesh::adaptive::numeric::restricted_coefficient_rn64(
          root, parent.data(), bgmesh::adaptive::SplitAxis::v, {0, k});
    auto child = node;
    child.box[3] = {.5, .5};
    child.controls = {static_cast<uint32_t>(cache.controls().size()), count};
    if (gpu) {
      bgmesh::adaptive::DeviceRestriction split{};
      split.root = {0};
      split.parent = root.controls;
      split.destination = child.controls.first;
      split.axis = bgmesh::adaptive::SplitAxis::v;
      split.child = 0;
      gpu->restrict_node(split);
    }
    for (size_t i = 0; i < 4; ++i) {
      point.uv[0] = {static_cast<double>(i % 2), static_cast<double>(i % 2)};
      point.uv[1] = {static_cast<double>(i / 2) * .5, static_cast<double>(i / 2) * .5};
      const auto value = sample_value(gpu.get(), cache, point);
      for (size_t a = 0; a < 3; ++a)
        child.actual_corners[3 * i + a] = value.actual_xyz[a];
      child.corner_departures[i] = value.departure_upper;
    }
    const auto restricted =
        node_value(gpu.get(), cache, child, std::span(child_controls).first(count));
    require(restricted.status == bgmesh::adaptive::NumericStatus::ready &&
                restricted.regularity_proved,
            "same child control restriction and regularity bound");
    std::cout << "leaf-child\t" << restricted.source_to_bilinear_upper;
    for (const auto q : child.actual_corners)
      std::cout << '\t' << q;
    std::cout << '\n';
    const std::array exact_box{f(0), f(1), f(0), f(1, 2)};
    require(bgmesh::adaptive::bound_node_precise(cache, {0}, exact_box, child.actual_corners, f(2),
                                                 budget)
                .certified,
            "selected exact original rational residual and local regularity");
    const auto c0slot = source.view().carrier_index.find(bgmesh::CarrierId{12});
    const std::array c0request{bgmesh::adaptive::CarrierRequest{c0slot, {.5, 1., .5, 1.}}};
    const auto c0 =
        bgmesh::adaptive::prepare_source_cache(source, c0request, resource, {}, {}, budget);
    require(c0.roots().size() == 1 && c0.original_roots().size() == 4,
            "touching evaluator-only original span closure");
    const auto c0owner = c0.original_roots()[c0.roots()[0].sample_owners[0].value];
    require(c0owner.original_box[0].lower == 0. && c0owner.original_box[2].lower == 0.,
            "source-knot node uses lower original evaluator independent requested rectangle");
    bool refused = false;
    try {
      const std::array duplicate{requests[0], requests[0]};
      static_cast<void>(
          bgmesh::adaptive::prepare_source_cache(source, duplicate, resource, {}, {}, budget));
    } catch (const bgmesh::Error &error) {
      refused = error.code == bgmesh::ErrorCode::invalid_source;
    }
    require(refused, "duplicate requested closure refuses");
    refused = false;
    try {
      auto foreign = std::make_shared<bgmesh::RunResource>(uint64_t{1} << 24);
      static_cast<void>(
          bgmesh::adaptive::prepare_source_cache(source, requests, foreign, {}, {}, budget));
    } catch (const bgmesh::Error &error) {
      refused = error.code == bgmesh::ErrorCode::invalid_source;
    }
    require(refused, "foreign source resource refuses before publication");
    for (int i = -64; i <= 64; ++i) {
      const double angle = i / 32.;
      const auto trig = bgmesh::adaptive::numeric::sincos({angle, angle});
      require(trig.ready, "RN primary trig source interval ready");
      std::cout << "trig\t" << angle << '\t' << trig.sine.lower << '\t' << trig.sine.upper << '\t'
                << trig.cosine.lower << '\t' << trig.cosine.upper << '\n';
    }
    namespace n = bgmesh::adaptive::numeric;
    const auto overflow = n::mul(n::point(1e308), n::point(1e308));
    require(!n::valid(overflow) && !n::valid(n::mul(n::point(0.), overflow)),
            "nonfinite intermediate cannot be erased by zero shortcut");
    const auto tiny = n::mul(n::point(std::numeric_limits<double>::denorm_min()), n::point(.5));
    std::cout << "tiny\t" << tiny.lower << '\t' << tiny.upper << '\n';
    if (gpu)
      gpu->close();
    std::cout << "adaptive-original-geometry\tPASS\n";
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
