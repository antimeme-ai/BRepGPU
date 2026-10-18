#include "adaptive_curve_numeric.hpp"
#include "adaptive_curves.hpp"
#include "adaptive_traces.hpp"
#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <vector>
using namespace bgmesh;
using namespace bgmesh::adaptive;
namespace {
void require(bool ok, const char *message) {
  if (!ok)
    throw std::runtime_error(message);
}
template <class F> void refuses(F &&run, ErrorCode code, const char *message) {
  try {
    run();
  } catch (const Error &error) {
    require(error.code == code, message);
    return;
  }
  throw std::runtime_error(message);
}
CurveSourceCache curves_for(const SourceStore &source, const std::shared_ptr<RunResource> &resource,
                            bgfd::Budget &budget) {
  std::vector<CurveRequest> requests;
  for (uint32_t i = 0; i < source.view().curves.size(); ++i)
    requests.push_back({{i}, source.view().curves[i].parameter_range});
  return prepare_curve_source_cache(source, requests, resource, {}, {}, budget);
}
struct Fixture {
  std::shared_ptr<RunResource> resource = std::make_shared<RunResource>(uint64_t{1} << 30);
  bgfd::Budget budget{{uint64_t{1} << 28, uint64_t{1} << 42}, resource.get()};
  SourceStore source;
  CurveSourceCache curves;
  SourceCache carriers;
  Slot<DefinitionTag> definition;
  Slot<FaceTag> face;
  DefinitionBoundarySnapshot boundary;
  PhysicalCurveBounds physical;
  explicit Fixture(const std::filesystem::path &path, bool image = false,
                   bool force_q_plane = false, bool split_first = false)
      : source(load_source(path, resource)), curves(curves_for(source, resource, budget)),
        carriers([&] {
          const auto &s = source.view();
          const auto slot =
              s.carrier_index.find(s.faces[s.face_index.find(FaceId{900}).value].carrier_id);
          CarrierRequest r{slot, s.carriers[slot.value].uv_bounds};
          return prepare_trace_source_cache(source, std::span(&r, 1), resource, {}, {}, budget);
        }()),
        definition(source.view().definition_index.find(DefinitionId{200})),
        face(source.view().face_index.find(FaceId{900})) {
    DefinitionBoundaryForest forest(budget, source, definition, resource);
    auto complete = [&](const BoundaryEvaluationBatch &batch) {
      std::vector<BoundaryEvaluationResult> results;
      for (const auto &task : batch.tasks()) {
        auto t = import_exact(budget, batch.parameters().view(task.parameter));
        auto root = find_original_curve_root(curves, task.curve, t);
        auto bound = t.bounds();
        CurveSampleInput input;
        input.original = root;
        input.t = {bound.lower, bound.upper};
        auto value = evaluate_curve_sample(curves, input);
        require(value.status == NumericStatus::ready, "real original curve sample unavailable");
        Vec3 q{value.actual_xyz[0], value.actual_xyz[1], value.actual_xyz[2]};
        if (force_q_plane)
          q[2] = 0.;
        results.push_back({task, 3, q});
      }
      static_cast<void>(forest.complete_evaluations(batch.epoch(), results));
    };
    if (auto batch = forest.pending_evaluations())
      complete(*batch);
    if (split_first) {
      const auto epoch = forest.begin_epoch();
      const auto intervals = forest.intervals({0});
      require(intervals.size() == 1, "analytic locator split fixture has wrong initial interval");
      forest.request_owner_splits(epoch, intervals);
      forest.arrive(epoch, face, forest.face_generation(face), {});
      complete(forest.prepare_splits(epoch));
    }
    boundary = forest.freeze_definition_boundary();
    ExactBuilder builder(resource, {{OwnerKind::boundary, definition.value, 200}, 1, 900});
    std::pmr::vector<PhysicalCurveInterval> rows(resource.get());
    for (const auto &edge : boundary->edges) {
      const auto curve =
          source.view().curve_index.find(source.view().edges[edge.edge.value].curve_id);
      for (uint64_t i = 0; i < edge.intervals.count; ++i) {
        PhysicalCurveInterval row;
        row.boundary_key = boundary->intervals[edge.intervals.first + i];
        row.boundary_nodes = {static_cast<uint32_t>(edge.nodes.first + i),
                              static_cast<uint32_t>(edge.nodes.first + i + 1)};
        std::array<Rational, 2> t{Rational(budget), Rational(budget)};
        std::array<Rational, 6> xyz{Rational(budget), Rational(budget), Rational(budget),
                                    Rational(budget), Rational(budget), Rational(budget)};
        for (uint32_t side = 0; side < 2; ++side) {
          const auto &node = boundary->nodes[row.boundary_nodes[side]];
          t[side] = import_exact(budget, boundary->parameters.view(*node.original_parameter));
          row.original_t[side] = builder.append(t[side]);
          for (uint32_t axis = 0; axis < 3; ++axis)
            xyz[3 * side + axis] = Rational::from_double(budget, node.xyz[axis]);
        }
        auto plan = prepare_curve_chord(curves, {curve, t, xyz}, budget);
        double parameter = 0., image_bound = 0.;
        bool all_image = image;
        for (const auto &piece : plan.pieces) {
          auto value = numeric::bound_affine_trace_rn64(
              piece.bound, piece.bound.input.controls.count
                               ? plan.controls.data() + piece.bound.input.controls.first
                               : nullptr);
          require(value.status == NumericStatus::ready, "real spatial chord bound unavailable");
          parameter = std::max(parameter, value.parameter_upper);
          image_bound = std::max(image_bound, value.image_upper);
          all_image = all_image && value.image_available;
        }
        if (image) {
          all_image = false;
          const bool forward = t[0].compare(t[1]) < 0;
          const std::array interval{forward ? t[0] : t[1], forward ? t[1] : t[0]};
          for (uint32_t ri = 0; ri < curves.roots().size(); ++ri) {
            const auto &root = curves.roots()[ri];
            if (root.source_slot != curve.value ||
                interval[0].compare(Rational::from_double(budget, root.prepared_t[0].lower)) < 0 ||
                interval[1].compare(Rational::from_double(budget, root.prepared_t[1].upper)) > 0)
              continue;
            auto net = prepare_curve_interval_controls(curves, {ri}, interval, budget);
            CurveNodeInput input;
            input.root = {ri};
            for (uint32_t side = 0; side < 2; ++side) {
              auto range = interval[side].bounds();
              input.t[side] = {range.lower, range.upper};
              for (uint32_t axis = 0; axis < 3; ++axis)
                input.actual_endpoints[3 * side + axis] =
                    xyz[3 * (forward ? side : 1 - side) + axis].to_double();
            }
            const auto value = bound_curve_node(curves, input, net);
            if (value.status == NumericStatus::ready && value.image_available) {
              all_image = true;
              image_bound = value.image_upper;
            }
            break;
          }
        }
        row.parameter_chord_upper = builder.append(Rational::from_double(budget, parameter));
        row.selected_correspondence = CurveCorrespondence::affine_parameter;
        if (all_image) {
          auto aa = exact_curve_value(curves, curve, t[0], budget),
               cc = exact_curve_value(curves, curve, t[1], budget);
          require(aa && cc, "checked image original endpoint value absent");
          uint32_t axis = 0;
          while (axis < 3 && (*aa)[axis].compare((*cc)[axis]) == 0)
            ++axis;
          require(axis < 3, "image source has no endpoint progress");
          GeometricLineRecord map;
          for (uint32_t a = 0; a < 3; ++a)
            map.covector[a] = builder.append(Rational(budget, a == axis ? 1 : 0));
          for (uint32_t side = 0; side < 2; ++side) {
            const auto original_side = t[0].compare(t[1]) < 0 ? side : 1 - side;
            map.original_t[side] = builder.append(t[original_side]);
            map.curve_scalar[side] = builder.append(original_side == 0 ? (*aa)[axis] : (*cc)[axis]);
            map.trace_scalar[side] = map.curve_scalar[side];
          }
          map.traversal = t[0].compare(t[1]) < 0 ? 1 : -1;
          row.image_map = map;
          row.image_chord_upper = builder.append(Rational::from_double(budget, image_bound));
          row.selected_correspondence = CurveCorrespondence::monotone_line_image;
        }
        rows.push_back(row);
      }
    }
    auto data = std::allocate_shared<PhysicalCurveBoundsData>(
        RunAllocator<PhysicalCurveBoundsData>(resource), resource, boundary, builder.seal());
    data->intervals = std::move(rows);
    physical = data;
  }
  TraceFaceRequest request(double positional = 1e-4, double cap = 1e-7) {
    ExactBuilder values(resource, {{OwnerKind::run, definition.value, 919}, 1, 1});
    auto eps = values.append(Rational::from_double(budget, positional));
    auto node = values.append(Rational::from_double(budget, cap));
    auto retained_curves =
        std::allocate_shared<CurveSourceCache>(RunAllocator<CurveSourceCache>(resource), curves);
    return {resource, source,    retained_curves,
            carriers, boundary,  physical,
            face,     {1, 1, 1}, values.seal(),
            eps,      node,      {{OwnerKind::face, definition.value, 900}, 1, 100}};
  }
};
std::vector<CurveSampleResult> source_samples(const Fixture &fixture, const TraceSourceWork &work,
                                              bgfd::Budget &budget) {
  std::vector<CurveSampleResult> result;
  for (const auto &task : trace_curve_samples(work)) {
    auto t = import_exact(budget, trace_values(work).view(task.parameter));
    auto range = t.bounds();
    CurveSampleInput input;
    input.original = find_original_curve_root(fixture.curves, task.curve, t);
    input.t = {range.lower, range.upper};
    result.push_back(evaluate_curve_sample(fixture.curves, input));
  }
  return result;
}
std::vector<SampleResult> carrier_samples(const Fixture &fixture, const TraceSiteWork &work,
                                          bgfd::Budget &budget) {
  std::vector<SampleResult> result;
  for (const auto &task : trace_carrier_samples(work)) {
    std::array<Rational, 2> uv{import_exact(budget, trace_values(work).view(task.uv[0])),
                               import_exact(budget, trace_values(work).view(task.uv[1]))};
    SampleInput input;
    try {
      input.original = find_original_carrier_root(fixture.carriers, task.carrier, uv);
    } catch (...) {
      std::cerr << "carrier-locator source=" << fixture.source.view().adapter_revision.count
                << " uv=" << std::hexfloat << uv[0].to_double() << "," << uv[1].to_double() << "\n";
      throw;
    }
    for (uint32_t a = 0; a < 2; ++a) {
      auto range = uv[a].bounds();
      input.uv[a] = {range.lower, range.upper};
    }
    result.push_back(evaluate_sample(fixture.carriers, input));
  }
  return result;
}
UseTraceResult run(Fixture &fixture, double tolerance = 1e-4) {
  JobContext ctx({uint64_t{1} << 28, uint64_t{1} << 42}, fixture.resource.get(), {});
  auto source = prepare_use_trace(fixture.request(tolerance), ctx);
  auto samples = source_samples(fixture, source, ctx.budget);
  auto stale = trace_completion_key(source);
  ++stale.identity.query;
  refuses([&] { static_cast<void>(prepare_trace_sites(source, stale, samples, ctx)); },
          ErrorCode::invalid_source, "stale source completion accepted");
  auto work = prepare_trace_sites(source, trace_completion_key(source), samples, ctx);
  auto carriers = carrier_samples(fixture, work, ctx.budget);
  auto sites = complete_trace_sites(work, trace_completion_key(work), carriers, ctx);
  auto intervals = prepare_trace_intervals(sites, ctx);
  std::vector<CurveNodeBounds> bounds;
  for (const auto &task : trace_bound_tasks(intervals)) {
    auto controls = trace_bound_controls(intervals);
    bounds.push_back(numeric::bound_affine_trace_rn64(
        task.bound, task.bound.input.controls.count
                        ? controls.data() + task.bound.input.controls.first
                        : nullptr));
  }
  auto result = complete_trace_intervals(intervals, trace_completion_key(intervals), bounds, ctx);
  require(trace_uses(result).size() == 4, "original face use inventory lost");
  require(trace_intervals(result).size() > 0, "trace interval operation returned no intervals");
  return result;
}
void narrow_bank_refuses(Fixture &fixture, std::array<double, 4> box = {.25, .75, 0., 1.}) {
  const auto carrier = fixture.source.view().carrier_index.find(CarrierId{9});
  const CarrierRequest requested{carrier, box};
  auto bank = prepare_trace_source_cache(fixture.source, std::span(&requested, 1), fixture.resource,
                                         {}, {}, fixture.budget);
  AdaptiveBoundarySession session(fixture.resource, fixture.source, fixture.definition,
                                  fixture.curves, bank, {});
  const auto request = fixture.request(1.e-4);
  session.begin({1, request.allowances, request.positional, request.node_source, {}});
  JobContext context({uint64_t{1} << 28, uint64_t{1} << 40}, fixture.resource.get(), {});
  refuses(
      [&] {
        for (uint32_t i = 0; i < 1000000; ++i)
          if (session.step(context) == StepState::done)
            throw std::runtime_error("narrow prepared bank silently widened");
        throw std::runtime_error("narrow prepared bank did not settle");
      },
      ErrorCode::unsupported, "natural-domain recovery clipped to narrow cache bank");
  session.close();
}
} // namespace
int main(int argc, char **argv) {
  try {
    require(argc == 3, "usage: traces source variant-directory");
    const std::filesystem::path variants = argv[2];
    Fixture plane_foot(variants / "plane-foot-outside.bgsource", true);
    const auto plane_recovered = run(plane_foot);
    require(trace_requests(plane_recovered).empty(),
            "valid plane guide did not survive outside inverse candidate");
    for (const auto &site : trace_sites(plane_recovered))
      for (const auto ref : site.chosen_original_uv) {
        const auto coordinate =
            import_exact(plane_foot.budget, trace_values(plane_recovered).view(ref));
        require(coordinate.sign() >= 0 && coordinate.compare(Rational(plane_foot.budget, 1)) <= 0,
                "outside plane foot was emitted instead of valid original guide");
      }
    Fixture plane_mismatch(variants / "plane-foot-mismatch.bgsource", true);
    refuses([&] { static_cast<void>(run(plane_mismatch)); }, ErrorCode::unresolved,
            "plane guide fallback bypassed original source-node cap");
    narrow_bank_refuses(plane_foot);
    for (const auto kind : {2, 3, 4, 5}) {
      Fixture analytic(variants / ("locator-" + std::to_string(kind) + ".bgsource"), false, false,
                       true);
      const auto result = run(analytic, 10.);
      require(trace_requests(result).empty(),
              "analytic locator trace did not pass unchanged bound");
      bool interior = false;
      for (const auto &site : trace_sites(result)) {
        const auto t = import_exact(analytic.budget, trace_values(result).view(site.original_t));
        if (t.compare(Rational::from_double(analytic.budget, .5)) != 0)
          continue;
        const auto u =
            import_exact(analytic.budget, trace_values(result).view(site.chosen_original_uv[0]));
        require(std::abs(u.to_double() - .5) < 1.e-10,
                "analytic inverse retained inaccurate reconstructed interior clock");
        const auto upper =
            import_exact(analytic.budget, trace_values(result).view(site.node_source_upper));
        require(upper.compare(Rational::from_double(analytic.budget, 1.e-7)) <= 0,
                "analytic inverse bypassed unchanged source-node cap");
        const auto s = import_exact(analytic.budget, trace_values(result).view(site.mapped_s));
        require(s.compare(t) == 0, "analytic inverse changed original mapped s");
        interior = true;
      }
      require(interior, "analytic locator fixture omitted real forest interior sample");
    }
    Fixture analytic_auth(variants / "locator-authoritative.bgsource", false, false, true);
    refuses([&] { static_cast<void>(run(analytic_auth, 10.)); }, ErrorCode::unresolved,
            "analytic inverse repaired an authoritative guide");
    Fixture analytic_bad(variants / "locator-mismatch.bgsource", false, false, true);
    refuses([&] { static_cast<void>(run(analytic_bad, 10.)); }, ErrorCode::unresolved,
            "analytic inverse bypassed original vertex/spatial mismatch");
    Fixture analytic_narrow(variants / "locator-3.bgsource", false, false, true);
    narrow_bank_refuses(analytic_narrow, {.4, .6, .25, .75});
    Fixture base(argv[1]);
    auto ordinary = run(base);
    require(trace_requests(ordinary).empty(),
            "literal C0 exact boundary requested unnecessary split");
    Fixture natural(variants / "natural-proposal.bgsource");
    const auto recovered = run(natural);
    require(trace_requests(recovered).empty(),
            "reconstructed guide outside natural R did not recover");
    for (const auto &site : trace_sites(recovered))
      for (uint32_t axis = 0; axis < 2; ++axis) {
        const auto uv = import_exact(natural.budget,
                                     trace_values(recovered).view(site.chosen_original_uv[axis]));
        require(uv.sign() >= 0 && uv.compare(Rational(natural.budget, 1)) <= 0,
                "reconstructed natural-domain proposal was not inside original R");
      }
    narrow_bank_refuses(natural);
    Fixture natural_bad(variants / "natural-sourceguard.bgsource");
    refuses([&] { static_cast<void>(run(natural_bad)); }, ErrorCode::unresolved,
            "natural-domain projection bypassed original source guard");
    Fixture mapped(variants / "maps.bgsource");
    const auto use = mapped.source.view().coedge_index.find(CoedgeId{500});
    auto t = Rational::from_double(mapped.budget, .75);
    require(map_trace_parameter(mapped.source, use, t)
                    .compare(Rational::from_double(mapped.budget, .625)) == 0,
            "descending original PL parameter mapping reversed twice");
    require(trace_requests(run(mapped)).empty(), "mapped original source trace did not settle");
    Fixture unknown(variants / "map3.bgsource");
    refuses(
        [&] {
          static_cast<void>(
              map_trace_parameter(unknown.source, {0}, Rational::from_double(unknown.budget, .5)));
        },
        ErrorCode::unsupported, "unproved map3 was treated as identity");
    Fixture measured(variants / "map4.bgsource");
    {
      const auto &measured_use = measured.source.view().coedges[0];
      const auto residuals = measured.source.view().view(measured_use.parameter_residuals);
      require(measured_use.parameter_map_kind == 4 && measured_use.parameter_map.count == 3 &&
                  residuals.size() == 3 &&
                  residuals[0] == 0 && residuals[1] == 2.5e-4 && residuals[2] == 1e-4,
              "measured map rows/residuals lost by the reader");
      bool named = false;
      try {
        static_cast<void>(
            map_trace_parameter(measured.source, {0}, Rational::from_double(measured.budget, .5)));
      } catch (const Error &error) {
        named = error.code == ErrorCode::unsupported &&
                std::string_view(error.what()) == measured_parameter_map_refusal;
      }
      require(named, "measured map4 was consumed or refused without its name");
    }
    Fixture auth_good(variants / "auth.bgsource");
    require(trace_requests(run(auth_good)).empty(),
            "original authoritative C0 boundary did not settle");
    Fixture auth_outside(variants / "authoutside.bgsource");
    refuses([&] { static_cast<void>(run(auth_outside, 10.)); }, ErrorCode::unsupported,
            "one-dimensional authoritative support outside natural domain "
            "accepted by zero area");
    Fixture auth(variants / "authjoint.bgsource");
    refuses([&] { static_cast<void>(run(auth)); }, ErrorCode::invalid_source,
            "unequal original authoritative joint accepted");
    Fixture mismatch(variants / "sourceguard.bgsource", false, true);
    refuses([&] { static_cast<void>(run(mismatch)); }, ErrorCode::unresolved,
            "generated Q on carrier replaced ORIGINAL source node guard");
    Fixture speed(variants / "speed.bgsource", true);
    auto straight = run(speed);
    require(trace_requests(straight).empty() && trace_intervals(straight).size() == 4,
            "stationary nonlinear straight image triggered speed-only source "
            "splits");
    bool image = false;
    for (const auto &interval : trace_intervals(straight))
      image |= interval.image_map.has_value();
    require(image, "nonlinear straight trace omitted actual image map");
    Fixture authspeed(variants / "authspeed.bgsource", true);
    const auto exact_image = run(authspeed);
    require(trace_requests(exact_image).empty(),
            "whole authoritative line image refined only for parameter speed");
    for (const auto &use_row : trace_uses(exact_image))
      require(use_row.whole_line_image.has_value(),
              "exact authoritative whole-use image record absent");
    for (const auto &interval : trace_intervals(exact_image))
      require(!interval.authoritative_domain_upper,
              "whole-use identity was misreported as per-interval flatten zero");
    for (const auto name : {"seam", "lift"}) {
      Fixture seam(variants / (std::string(name) + ".bgsource"));
      auto result = run(seam, 10.);
      require(trace_requests(result).empty(),
              "literal seam trace refused bounded coarse operation");
      const auto uses = trace_uses(result);
      const auto sites = trace_sites(result);
      const auto &right = sites[uses[1].sites.first],
                 &left = sites[uses[3].sites.first + uses[3].sites.count - 1];
      auto ur = import_exact(seam.budget, trace_values(result).view(right.chosen_original_uv[0]));
      auto ul = import_exact(seam.budget, trace_values(result).view(left.chosen_original_uv[0]));
      require(ur.compare(ul) > 0, "shared physical seam welded distinct original UV occurrences");
      require(seam.boundary->nodes[right.boundary_node].key ==
                  seam.boundary->nodes[left.boundary_node].key,
              "same physical seam lost shared edge/vertex identity");
    }
    std::cout << "trace_original_map_joint_guard_image_seam=PASS\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
