#include "../native/mesher/adaptive_curve_geometry.hpp"
#include "../native/mesher/adaptive_curve_numeric.hpp"
#include "native_adaptive_curve_numeric_device.hpp"
#include <bit>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
namespace {
using namespace bgmesh;
using namespace bgmesh::adaptive;
void require(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}
void emit_sample(uint64_t id, double t, const CurveSampleResult &r) {
  require(r.status == NumericStatus::ready, "original sample unresolved");
  std::cout << "sample\t" << id << '\t' << t;
  for (auto x : r.original_xyz)
    std::cout << '\t' << x.lower << '\t' << x.upper;
  for (auto x : r.actual_xyz)
    std::cout << '\t' << x;
  std::cout << '\t' << r.departure_upper << '\n';
}
void emit_bound(uint64_t id, double a, double z, const CurveNodeInput &in,
                const CurveNodeBounds &r) {
  require(r.status == NumericStatus::ready, "original chord unresolved");
  std::cout << "chord\t" << id << '\t' << a << '\t' << z;
  for (auto x : in.actual_endpoints)
    std::cout << '\t' << x;
  std::cout << '\t' << r.parameter_upper << '\n';
}
} // namespace
int main(int argc, char **argv) {
  try {
    require(argc == 2 || argc == 4, "source [module device] arguments required");
    std::cout << std::setprecision(17);
    auto resource = std::make_shared<RunResource>(uint64_t{1} << 29);
    auto source = load_source(argv[1], resource);
    bgfd::Budget budget({uint64_t{1} << 28, uint64_t{1} << 35});
    std::vector<CurveRequest> requests;
    for (auto id : {7, 11, 6000, 6001, 6002, 6003, 6004}) {
      auto slot = source.view().curve_index.find(CurveId{uint64_t(id)});
      const auto &c = source.view().curves[slot.value];
      requests.push_back({slot, id == 6002 ? Vec2{.125, .875} : c.parameter_range});
    }
    auto cache = prepare_curve_source_cache(source, requests, resource, {}, {}, budget);
    std::unique_ptr<adaptive_test::CurveDeviceFixture> device;
    if (argc == 4)
      device =
          std::make_unique<adaptive_test::CurveDeviceFixture>(cache, argv[2], std::stoi(argv[3]));
    struct SampleLocation {
      double t;
      uint32_t destination;
    };
    auto sample = [&](const CurveRootDescriptor &r, SampleLocation location, bool fixed = false) {
      const auto t = location.t;
      const auto destination = location.destination;
      CurveSampleInput input;
      input.original = find_original_curve_root(cache, Slot<CurveTag>{r.source_slot},
                                                Rational::from_double(budget, t));
      input.t = {t, t};
      input.fixed_actual = fixed;
      if (fixed) {
        input.actual_xyz[0] = -.125;
        input.actual_xyz[1] = .25;
        input.actual_xyz[2] = r.dimension == 2 ? 0. : .5;
      }
      return device ? device->sample(input, destination) : evaluate_curve_sample(cache, input);
    };
    uint32_t destination = static_cast<uint32_t>(cache.controls().size());
    for (uint32_t n = 0; n < cache.roots().size(); ++n) {
      const auto &r = cache.roots()[n];
      double a = r.prepared_t[0].lower, z = r.prepared_t[1].upper, m = (a + z) * .5;
      for (auto t : {a, a + (z - a) * .25, m, a + (z - a) * .75, z})
        emit_sample(r.source_id, t, sample(r, {t, 2}));
      emit_sample(r.source_id, m, sample(r, {m, 2}, true));
      auto run = [&](double x, double y, ScalarRange slab, std::span<const Interval> net) {
        CurveNodeInput in;
        in.root = {n};
        in.controls = slab;
        in.t[0] = {x, x};
        in.t[1] = {y, y};
        auto left = sample(r, {x, 0}, true), right = sample(r, {y, 1}, true);
        for (uint32_t j = 0; j < 3; ++j) {
          in.actual_endpoints[j] = left.actual_xyz[j];
          in.actual_endpoints[3 + j] = right.actual_xyz[j];
        }
        in.endpoint_departures[0] = left.departure_upper;
        in.endpoint_departures[1] = right.departure_upper;
        emit_bound(r.source_id, x, y, in,
                   device ? device->node(in) : bound_curve_node(cache, in, net));
      };
      run(a, z, r.controls, {});
      if (r.family == CurveFamily::rational) {
        std::vector<Interval> left(r.controls.count), right(r.controls.count),
            scratch(r.controls.count);
        auto parent = cache.controls().subspan(r.controls.first, r.controls.count);
        restrict_curve_midpoint_pair(r, parent, left, right, scratch);
        if (device)
          device->children({{n}, r.controls, destination, destination + r.controls.count});
        run(a, m, {destination, r.controls.count}, left);
        destination += r.controls.count;
        run(m, z, {destination, r.controls.count}, right);
        destination += r.controls.count;
      }
    }
    {
      auto slot = source.view().curve_index.find(CurveId{11});
      bool rejected = false;
      try {
        std::array bad{CurveRequest{slot, {std::numeric_limits<double>::quiet_NaN(), 1.}}};
        (void)prepare_curve_source_cache(source, bad, resource, {}, {}, budget);
      } catch (const Error &) {
        rejected = true;
      }
      require(rejected, "nonfinite request reached source sorting");
      std::array interval{Rational(budget, -2), Rational(budget, 5)};
      CurveNodeInput deep;
      deep.root = find_prepared_curve_root(cache, slot, interval);
      deep.t[0] = deep.t[1] = {.5, .5000000000000001};
      require(bound_curve_node(cache, deep).status == NumericStatus::needs_wider,
              "overlapping exact-t enclosures rejected as source");
      CurveSampleInput fixed;
      fixed.original = find_original_curve_root(cache, slot, Rational(budget));
      fixed.fixed_actual = 1;
      for (auto &x : fixed.actual_xyz)
        x = -0.;
      const auto result = device ? device->sample(fixed, 3) : evaluate_curve_sample(cache, fixed);
      for (auto x : result.actual_xyz)
        require(std::bit_cast<uint64_t>(x) == std::bit_cast<uint64_t>(-0.),
                "RN fixed source negative zero changed");
    }
    // Precise fallback resolves source values and an equality threshold without
    // changing fixed original vertex bit patterns.
    {
      auto slot = source.view().curve_index.find(CurveId{11});
      auto t = Rational(budget);
      auto original = find_original_curve_root(cache, slot, t);
      auto fixed = evaluate_curve_sample_precise(cache, original, t, ArithmeticStage::p128, budget,
                                                 Vec3{-0., -0., -0.});
      require(std::bit_cast<uint64_t>(fixed.actual_xyz[0]) == std::bit_cast<uint64_t>(-0.),
              "fixed source negative zero changed");
      std::array interval{Rational(budget, -2), Rational(budget, 5)};
      auto root = find_prepared_curve_root(cache, slot, interval);
      std::array actual{-2., .125, 0., 5., .125, 0.};
      auto proof = bound_curve_node_precise(cache, root, interval, actual,
                                            Rational::from_double(budget, .125), budget);
      require(proof.parameter_certified, "precise equality threshold unresolved");
      require(proof.bounds.numerical_upper >= .125, "precise response lost actual endpoint floor");
      const auto tiny = std::ldexp(1., -600);
      actual[1] = actual[4] = tiny;
      const auto small = bound_curve_node_precise(cache, root, interval, actual,
                                                  Rational::from_double(budget, tiny), budget);
      require(small.parameter_certified && small.bounds.numerical_upper == tiny,
              "precise norm introduced absolute-grid floor");
    }
    {
      auto slot = source.view().curve_index.find(CurveId{6000});
      auto t = Rational::from_double(budget, .5);
      emit_sample(6000, .5,
                  evaluate_curve_sample_precise(cache, find_original_curve_root(cache, slot, t), t,
                                                ArithmeticStage::p128, budget));
    }
    {
      auto slot = source.view().curve_index.find(CurveId{7});
      auto q = [&](double x) { return Rational::from_double(budget, x); };
      CurveChordRequest request{
          slot, {q(1.), q(0.)}, {q(-.25), q(.125), q(0.), q(2.25), q(.75), q(0.)}};
      auto plan = prepare_curve_chord(cache, request, budget);
      for (const auto &piece : plan.pieces) {
        auto bound =
            device ? device->trace(piece.bound, plan.controls)
                   : numeric::bound_affine_trace_rn64(
                         piece.bound, plan.controls.data() + piece.bound.input.controls.first);
        require(bound.status == NumericStatus::ready, "reversed raw pcurve chord unresolved");
        std::cout << "raw\t7\t" << piece.normalized_interval[0].to_double() << '\t'
                  << piece.normalized_interval[1].to_double() << '\t' << bound.parameter_upper
                  << '\n';
      }
    }
    {
      auto slot = source.view().curve_index.find(CurveId{3001});
      std::array request{CurveRequest{slot, {0., 1.}}};
      auto boundary_cache = prepare_curve_source_cache(source, request, resource, {}, {}, budget);
      std::array interval{Rational(budget), Rational(budget, 1)};
      const auto hull = enclose_curve_hull_precise(boundary_cache, slot, interval, budget);
      require(hull[0].lower == 1. && hull[0].upper == 1.,
              "exact source hull inflated beyond natural boundary");
    }
    // Direct affine UV restrictions cross the original C0 U/V spans. Virtual L
    // corners remain the exact restriction of this SAME physical affine chord.
    for (auto id : {9, 12}) {
      auto slot = source.view().carrier_index.find(CarrierId{uint64_t(id)});
      std::array request{CarrierRequest{slot, {0., 1., 0., 1.}}};
      auto surfaces = prepare_trace_source_cache(source, request, resource, {}, {}, budget);
      auto q = [&](double x) { return Rational::from_double(budget, x); };
      AffineTraceRequest req{slot,
                             {q(.125), q(.125), q(.875), q(.875)},
                             {q(-.25), q(.125), q(0.), q(1.25), q(.75), q(1.)}};
      auto plan = prepare_affine_trace(surfaces, req, budget);
      for (size_t i = 0; i < plan.pieces.size(); ++i) {
        const auto &piece = plan.pieces[i];
        auto b =
            device ? device->trace(piece.bound, plan.controls) : bound_affine_trace_piece(plan, i);
        require(b.status == NumericStatus::ready, "direct affine trace unresolved");
        std::cout << "trace\t" << id << '\t' << piece.normalized_interval[0].to_double() << '\t'
                  << piece.normalized_interval[1].to_double() << '\t' << b.parameter_upper << '\n';
      }
    }
    if (device)
      device->close();
    std::cout << "result\tPASS\n";
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
