#include "../native/mesher/adaptive_cap_geometry.hpp"
#include "../native/mesher/adaptive_numeric.hpp"
#include "../native/mesher/geometry_enclosure.hpp"
#include "native_adaptive_device_test.hpp"
#include <algorithm>
#include <array>
#include <iomanip>
#include <iostream>
#include <stdexcept>
using namespace bgmesh;
using namespace bgmesh::adaptive;
namespace {
void require(bool yes, const char *why) {
  if (!yes)
    throw std::runtime_error(why);
}
void exact(const Rational &value) {
  const auto words = [](std::span<const uint32_t> limbs) {
    if (limbs.empty()) {
      std::cout << '0';
      return;
    }
    std::cout << std::hex << limbs.back();
    for (size_t i = limbs.size() - 1; i > 0; --i)
      std::cout << std::setw(8) << std::setfill('0') << limbs[i - 1];
  };
  std::cout << "\tr:";
  if (value.sign() < 0)
    std::cout << '-';
  words(value.numerator().magnitude());
  std::cout << '/';
  words(value.denominator().magnitude());
  std::cout << std::dec << std::setfill(' ');
}
void emit(const char *name, uint32_t scenario, const CapFacetInput &input,
          const CapFacetBounds &b) {
  std::cout << name << '\t' << scenario << '\t' << b.total_upper;
  for (const auto x : input.cut_uv)
    std::cout << '\t' << x.lower;
  for (const auto x : input.finite_v)
    std::cout << '\t' << x.lower;
  for (const auto x : input.actual_xyz)
    std::cout << '\t' << x;
  std::cout << '\t' << b.position_upper << '\t' << b.band_upper << '\t'
            << b.angular_upper << '\t' << b.radial_upper << '\t'
            << b.corner_upper << '\t' << b.pole_departure_upper << '\t'
            << b.regular_sheet_sign;
  for (const auto x : b.positive_chart_roles)
    std::cout << '\t' << x;
  std::cout << '\n';
}
} // namespace
int main(int argc, char **argv) {
  try {
    require(argc == 2 || argc == 5,
            "original seam-wedge source [--cuda module device] required");
    const auto resource = std::make_shared<RunResource>(uint64_t{1} << 28);
    const auto source = load_source(argv[1], resource);
    bgfd::Budget budget({uint64_t{1} << 27, uint64_t{1} << 35}, resource.get());
    const auto slot = source.view().carrier_index.find(CarrierId{9});
    const auto &carrier = source.view().carriers[slot.value];
    const std::array requests{CarrierRequest{slot, carrier.uv_bounds}};
    const auto cache = prepare_quotient_trim_source_cache(
        source, requests, resource, {}, {}, budget);
    require(cache.purpose() == SourceCachePurpose::quotient_trim &&
                cache.original_roots().size() == 1,
            "quotient capability or original evaluator changed");
    require(
        source.view().coedges.size() == 3 && source.view().edges.size() == 2 &&
            source.view().vertices.size() == 2 &&
            std::none_of(source.view().edges.begin(), source.view().edges.end(),
                         [](const auto &edge) { return edge.degenerate; }),
        "bound fixture invented a collapsed original coedge");
    std::unique_ptr<adaptive_test::DeviceFixture> device;
    if (argc == 5) {
      require(std::string_view(argv[2]) == "--cuda", "expected --cuda");
      device = std::make_unique<adaptive_test::DeviceFixture>(
          cache, argv[3], std::stoi(argv[4]));
    }
    const auto kind = carrier.kind == 3 ? AnalyticCapKind::cone_apex
                      : carrier.uv_bounds[2] < -1.5
                          ? AnalyticCapKind::sphere_south
                          : AnalyticCapKind::sphere_north;
    const auto prepared =
        prepare_cap_source(cache, cache.roots()[0].original, kind, budget);
    const auto &root = cache.roots()[0];
    for (uint32_t a = 0; a < 4; ++a)
      require(cache.original_roots()[0].original_box[a].lower ==
                  carrier.uv_bounds[a],
              "quotient factory changed original envelope");
    for (const bool trace : {false, true}) {
      bool refused = false;
      try {
        if (trace)
          (void)prepare_trace_source_cache(source, requests, resource, {}, {},
                                           budget);
        else
          (void)prepare_trim_source_cache(source, requests, resource, {}, {},
                                          budget);
      } catch (const Error &e) {
        refused = e.code == ErrorCode::unsupported;
      }
      require(refused, "old regular/trace source capability widened");
    }
    const auto finite = source.view().edges[0].parameter_range[0];
    const auto far = source.view().edges[0].parameter_range[1];
    const auto cut = finite + (far - finite) * .5,
               side = far > finite ? 1. : -1.;
    std::cout << std::hexfloat << "cap-source\t" << static_cast<uint32_t>(kind)
              << '\t' << cache.roots().size() << '\t'
              << static_cast<uint32_t>(prepared.numeric().stage);
    std::cout << '\t' << prepared.numeric().pole_v.lower << '\t'
              << prepared.numeric().pole_v.upper;
    for (const auto v : prepared.numeric().ideal_xyz)
      std::cout << '\t' << v.lower << '\t' << v.upper;
    std::cout << '\t' << root.frame_a.upper << '\t' << root.frame_f.upper
              << '\n';
    for (const bool regular : {false, true}) {
      NodePredicate node;
      node.root = {0};
      node.controls = root.controls;
      std::copy_n(root.prepared_box, 4, node.box);
      if (regular) {
        node.box[2] = {std::min(cut, far), std::min(cut, far)};
        node.box[3] = {std::max(cut, far), std::max(cut, far)};
      }
      for (uint32_t i = 0; i < 4; ++i) {
        SampleInput p;
        p.original = root.original;
        p.uv[0] = node.box[i % 2];
        p.uv[1] = node.box[2 + i / 2];
        const auto q = device ? device->sample(p) : evaluate_sample(cache, p);
        require(q.status == NumericStatus::ready,
                "original singular-envelope sample unresolved");
        std::copy_n(q.actual_xyz, 3, node.actual_corners + 3 * i);
        node.corner_departures[i] = q.departure_upper;
      }
      const auto b =
          device ? device->node(cache, node) : bound_node(cache, node);
      require(b.status == NumericStatus::ready &&
                  b.regularity_proved == static_cast<uint32_t>(regular),
              "ordinary node inherits singular source regularity");
      const std::array box{Rational::from_double(budget, node.box[0].lower),
                           Rational::from_double(budget, node.box[1].upper),
                           Rational::from_double(budget, node.box[2].lower),
                           Rational::from_double(budget, node.box[3].upper)};
      const auto precise =
          bound_node_precise(cache, node.root, box, node.actual_corners,
                             Rational(budget, 1000000), budget);
      require(precise.bounds.regularity_proved ==
                      static_cast<uint32_t>(regular) &&
                  precise.certified == regular,
              "precise node bypasses local regularity");
      std::cout << "cap-regularity\t" << b.regularity_proved << '\t'
                << precise.bounds.regularity_proved;
      for (const auto v : node.box)
        std::cout << '\t' << v.lower;
      std::cout << '\n';
    }
    for (uint32_t scenario = 0; scenario < 4; ++scenario) {
      CapFacetInput in;
      in.original = root.original;
      in.cut_uv[0] = {0., 0.};
      in.cut_uv[2] = {.5, .5};
      in.cut_uv[1] = {cut, cut};
      const auto right = cut + (scenario == 1 ? side * .03125 : 0.);
      in.cut_uv[3] = {right, right};
      const auto vf = scenario == 3 ? far : finite,
                 vf1 = vf + (scenario == 1 ? side * .015625 : 0.);
      in.finite_v[0] = {vf, vf};
      in.finite_v[1] = {vf1, vf1};
      const std::array uv{Rational::from_double(budget, in.cut_uv[0].lower),
                          Rational::from_double(budget, in.cut_uv[1].lower),
                          Rational::from_double(budget, in.cut_uv[2].lower),
                          Rational::from_double(budget, in.cut_uv[3].lower)};
      const std::array vfexact{
          Rational::from_double(budget, in.finite_v[0].lower),
          Rational::from_double(budget, in.finite_v[1].lower)};
      const auto map = admit_cap_map(prepared, uv, vfexact, {96, 128}, budget);
      const auto &fixed = source.view().vertices[0].xyz;
      std::copy(fixed.begin(), fixed.end(), in.actual_xyz);
      for (uint32_t corner = 0; corner < 2; ++corner) {
        SampleInput p;
        p.original = root.original;
        p.uv[0] = in.cut_uv[2 * corner];
        p.uv[1] = in.cut_uv[2 * corner + 1];
        const auto q = device ? device->sample(p) : evaluate_sample(cache, p);
        require(q.status == NumericStatus::ready,
                "cap source cut sample unresolved");
        std::copy_n(q.actual_xyz, 3, in.actual_xyz + 3 + 3 * corner);
      }
      // Bound-only cut-Q departure attack. The pole Q remains ORIGINAL V.
      if (scenario == 2)
        in.actual_xyz[5] += .125;
      in.allowance_down = 1000.;
      const auto b = device ? device->cap_facet(prepared.numeric(), in)
                            : bound_cap_facet(prepared, in, budget);
      require(b.status == NumericStatus::ready &&
                  b.pole_departure_upper <= b.corner_upper,
              "original actual-cap bound unresolved");
      require(map.regular_sheet_sign == b.regular_sheet_sign,
              "source-only cap sheet differs");
      for (uint32_t i = 0; i < 3; ++i)
        require(map.positive_chart_roles[i] == b.positive_chart_roles[i],
                "source-only cap roles differ");
      emit("cap-bound", scenario, in, b);
      if (scenario == 1 || scenario == 3) {
        const auto p = compare_cap_facet_precise(
            prepared, uv, vfexact, in.actual_xyz, Rational(budget, 1000),
            {96, 128}, budget);
        require(p.certified, "direct cap precise comparison unresolved");
        auto answer = b;
        answer.total_upper =
            p.bounds.position_upper.add(p.bounds.band_upper).bounds().upper;
        for (uint32_t i = 0; i < 3; ++i)
          require(p.bounds.facet_roles[i] == b.positive_chart_roles[i],
                  "precise source roles ignore finite height");
        emit("cap-precise", scenario, in, answer);
      }
      if (scenario == 0) {
        std::array next = uv;
        next[1] = uv[1]
                      .add(Rational::from_double(budget, finite))
                      .divide(Rational(budget, 2));
        // A one-sided forest horizon must not masquerade as a full contraction.
        auto states = cap_cut_progress(prepared, uv, next, {96, 128}, budget);
        require(states[0] == CapProgressState::proved &&
                    states[1] == CapProgressState::not_contracted,
                "source cut history lost the uncontracted endpoint");
        next[3] = uv[3]
                      .add(Rational::from_double(budget, finite))
                      .divide(Rational(budget, 2));
        states = cap_cut_progress(prepared, uv, next, {96, 128}, budget);
        require(states[0] == CapProgressState::proved &&
                    states[1] == CapProgressState::proved,
                "source cut 3/4 progress uses finite/Q distance");
        std::cout << "cap-progress\t" << static_cast<uint32_t>(states[0])
                  << '\t' << static_cast<uint32_t>(states[1]);
        for (const auto &x : uv)
          exact(x);
        for (const auto &x : next)
          exact(x);
        std::cout << '\n';
        if (kind == AnalyticCapKind::cone_apex) {
          // Select an input at the middle of the SAME finite 32-bit source
          // enclosure. A midpoint of the much wider RN descriptor alone need
          // not remain ambiguous after the requested precise comparison.
          GeometryContext context(budget, {32, 512});
          const auto angle = geometry_detail::trig_range(
              context, Rational::from_double(budget, carrier.radii[1]),
              Rational::from_double(budget, carrier.radii[1]), 32);
          const auto pole = geometry_detail::quotient(
              ExactEnclosure(Rational::from_double(budget, -carrier.radii[0])),
              angle.sine);
          const auto mid =
              pole.lower.add(pole.upper).divide(Rational(budget, 2));
          next[1] = uv[1]
                        .multiply(Rational(budget, 3))
                        .add(mid)
                        .divide(Rational(budget, 4));
          next[3] = next[1];
          states = cap_cut_progress(prepared, uv, next, {32, 64}, budget);
          require(
              states[0] == CapProgressState::unresolved &&
                  states[1] == CapProgressState::unresolved,
              "ambiguous symbolic source-gap test became a contraction claim");
          std::cout << "cap-progress-uncertain\tPASS\n";
        }
        bool source_refused = false;
        try {
          const std::array invalid_v{Rational::from_double(budget, finite),
                                     Rational::from_double(budget, far)};
          (void)admit_cap_map(prepared, uv, invalid_v, {96, 128}, budget);
        } catch (const Error &e) {
          source_refused = e.code == ErrorCode::invalid_source;
        }
        require(source_refused, "source-only changing-sign cap map accepted");
        bool work_refused = false;
        try {
          auto scope = budget.scoped_work(0);
          (void)admit_cap_map(prepared, uv, vfexact, {96, 128}, budget);
        } catch (const bgfd::Failure &e) {
          work_refused = e.status == BGFD_RESOURCE_EXHAUSTED;
        }
        require(work_refused, "source-only map ignored its caller work cap");
        auto collapsed = in;
        std::copy_n(collapsed.actual_xyz, 3, collapsed.actual_xyz + 3);
        std::copy_n(collapsed.actual_xyz, 3, collapsed.actual_xyz + 6);
        bool collapsed_refused = false;
        try {
          (void)bound_cap_facet(prepared, collapsed, budget);
        } catch (const Error &e) {
          collapsed_refused = e.code == ErrorCode::invalid_source;
        }
        require(collapsed_refused,
                "collapsed actual physical cap facet accepted");
        for (uint32_t attack = 0; attack < 2; ++attack) {
          auto bad = in;
          if (attack == 0)
            ++bad.original.value;
          else
            bad.cut_uv[0] = {carrier.uv_bounds[0] - 1.,
                             carrier.uv_bounds[0] - 1.};
          bool refused = false;
          try {
            const auto invalid =
                device ? device->cap_facet(prepared.numeric(), bad)
                       : bound_cap_facet(prepared, bad, budget);
            refused = invalid.status == NumericStatus::invalid;
          } catch (const Error &e) {
            refused = e.code == ErrorCode::invalid_source;
          } catch (const std::runtime_error &e) {
            refused = std::string_view(e.what()).find("cap") !=
                      std::string_view::npos;
          }
          require(refused, "foreign/out-of-profile cap request accepted");
        }
      }
    }
    if (device)
      device->close();
    std::cout << "cap-native\tPASS\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
