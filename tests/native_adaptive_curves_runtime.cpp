#include "adaptive_cache.hpp"
#include "adaptive_curves.hpp"
#include "curve_source_cache.hpp"
#include <bit>
#include <iostream>
#include <optional>
#include <thread>
using namespace bgmesh;
using namespace bgmesh::adaptive;
namespace {
void check(bool ok, const char *message) {
  if (!ok)
    throw std::runtime_error(message);
}
struct QueryProfile {
  uint64_t generation;
  double tolerance;
  bool exact_two_thirds = false;
};
SharedCurveSnapshot query(AdaptiveBoundarySession &session,
                          const std::shared_ptr<RunResource> &resource, QueryProfile profile) {
  const auto generation = profile.generation;
  const auto tolerance = profile.tolerance;
  const auto exact_two_thirds = profile.exact_two_thirds;
  bgfd::Budget budget({uint64_t{1} << 26, uint64_t{1} << 36}, resource.get());
  ExactBuilder builder(resource, {{OwnerKind::run, 999, 1}, generation, 1});
  const auto tau = builder.append(exact_two_thirds ? Rational(budget, 2).divide(Rational(budget, 3))
                                                   : Rational::from_double(budget, tolerance));
  const auto delta = builder.append(Rational::from_double(budget, 1.e-8));
  session.begin({generation, builder.seal(), tau, delta, {}});
  JobContext context({uint64_t{1} << 27, uint64_t{1} << 38}, resource.get(), {});
  for (uint32_t steps = 0; steps < 10000000; ++steps) {
    if (session.step(context) == StepState::done)
      return session.result();
    std::this_thread::yield();
  }
  throw std::runtime_error("bounded shared-curve continuation did not finish");
}
} // namespace
int main(int argc, char **argv) {
  try {
    check(argc == 2 || argc == 3 || argc == 5,
          "usage: curves_runtime source [threads [cuda-module device]]");
    auto run = std::make_shared<RunResource>(uint64_t{1} << 30);
    auto source = load_source(argv[1], run);
    bgfd::Budget budget({uint64_t{1} << 28, uint64_t{1} << 38}, run.get());
    std::pmr::vector<CurveRequest> requests(&budget);
    for (const auto &curve : source.view().curves) {
      const auto slot = source.view().curve_index.find(curve.id);
      requests.push_back({slot, curve.parameter_range});
    }
    auto roots = prepare_curve_source_cache(source, requests, run, {}, {}, budget);
    CurveLimits limits;
    limits.host_bytes = uint64_t{1} << 28;
    limits.device_bytes = uint64_t{1} << 28;
    limits.nodes = 1 << 16;
    limits.points = 1 << 17;
    limits.control_scalars = 1 << 20;
    limits.frontier = 1 << 16;
    const bool auth_profile = argc == 3 && std::string_view(argv[2]) == "--profile-auth";
    const bool regular_profile = argc == 3 && std::string_view(argv[2]) == "--profile-regular";
    limits.cpu_threads = argc >= 3 && !auth_profile && !regular_profile
                             ? static_cast<uint32_t>(std::stoul(argv[2]))
                             : 1;
    const auto &carrier = source.view().carriers.front();
    const auto domain = carrier.kind == 6 ? std::array{0., 1., 0., 1.} : carrier.uv_bounds;
    const std::array carrier_requests{CarrierRequest{{0}, domain}};
    const auto supplied = prepare_trim_source_cache(source, carrier_requests, run, {}, {}, budget);
    if (regular_profile) {
      AdaptiveBoundarySession fixed(run, source, {0}, roots, supplied, limits);
      const auto retained = query(fixed, run, {1, 10.});
      fixed.close();
      check(retained->carrier_source && &retained->carrier_source->view() == &supplied.view(),
            "rational source bank identity did not survive close");
      const std::array narrow_request{CarrierRequest{{0}, {.25, .75, .25, .75}}};
      const auto narrow = prepare_trim_source_cache(source, narrow_request, run, {}, {}, budget);
      AdaptiveBoundarySession constrained(run, source, {0}, roots, narrow, limits);
      bool refused_profile = false;
      try {
        (void)query(constrained, run, {1, 10.});
      } catch (const Error &error) {
        refused_profile =
            error.code == ErrorCode::unsupported &&
            std::string(error.what()).find("supplied carrier profile") != std::string::npos;
      }
      check(refused_profile, "original rational span bypassed prepared rectangle");
      constrained.close();
      std::cout << "rational prepared-profile handoff GREEN\n";
      return 0;
    }
    if (auth_profile) {
      AdaptiveBoundarySession guarded(run, source, {0}, roots, supplied, limits);
      bool refused_profile = false;
      try {
        (void)query(guarded, run, {1, 10.});
      } catch (const Error &error) {
        refused_profile =
            error.code == ErrorCode::unresolved &&
            std::string(error.what())
                    .find("authoritative original pcurve outside supplied carrier profile") !=
                std::string::npos;
      }
      check(refused_profile, "whole authoritative image escaped the fixed prepared profile");
      bool incomplete = false;
      try {
        (void)guarded.result();
      } catch (const Error &) {
        incomplete = true;
      }
      check(incomplete, "outside authoritative image exposed partial output");
      guarded.close();
      std::cout << "authoritative prepared-profile refusal GREEN\n";
      return 0;
    }
    std::optional<AdaptiveBoundarySession> owner;
    const auto &initial_curve =
        source.view().curves[source.view().curve_index.find(source.view().edges[0].curve_id).value];
    const bool use_supplied = carrier.kind == 2 || initial_curve.kind != 4 ||
                              source.view().view(initial_curve.controls)[5] == 0.;
    if (use_supplied)
      owner.emplace(run, source, Slot<DefinitionTag>{0}, roots, supplied, limits);
    else
      owner.emplace(run, source, Slot<DefinitionTag>{0}, roots, limits);
    auto &session = *owner;
    bool refused = false;
    try {
      (void)session.result();
    } catch (const Error &) {
      refused = true;
    }
    check(refused, "session exposed incomplete curve result");
    if (argc == 5)
      session.open_device({std::stoi(argv[4]), argv[3]});
    if (source.view().edges.size() == 5 && source.view().edges.back().curve_id.value == 4000) {
      bool refused_source = false;
      try {
        (void)query(session, run, {1, 1.});
      } catch (const Error &error) {
        refused_source = error.code == ErrorCode::unresolved &&
                         std::string(error.what()).find("source-node") != std::string::npos;
      }
      check(refused_source && session.stats().installed_splits == 0,
            "wire source-node mismatch was accepted or refined geometrically");
      session.close();
      std::cout << "wire source-node refusal GREEN\n";
      return 0;
    }
    const auto first = query(session, run, {1, .05});
    if (use_supplied)
      check(first->carrier_source && &first->carrier_source->view() == &supplied.view(),
            "shared boundary replaced its supplied original carrier profile");
    const bool seam = source.view().carriers[0].kind == 2;
    const auto &first_curve =
        source.view().curves[source.view().curve_index.find(source.view().edges[0].curve_id).value];
    const bool bent =
        !seam && first_curve.kind == 4 && source.view().view(first_curve.controls)[5] != 0.;
    check(first->uses.size() == 4, "lost original use occurrence");
    if (!seam && !bent)
      check(first->physical_intervals.size() == 4,
            "monotone speed image caused parameter-only splits");
    else
      check(first->physical_intervals.size() > (seam ? 3u : 4u) &&
                session.stats().installed_splits > 0,
            "circle geometry bypassed shared owner refinement");
    check(first->boundary->edges.size() == (seam ? 3u : 4u) && first->sites.size() >= 8 &&
              first->intervals.size() >= 4,
          "whole boundary/use inventory mismatch");
    const auto physical = first->physical_intervals[0];
    if (!seam && !bent)
      check(physical.selected_correspondence == CurveCorrespondence::monotone_line_image &&
                physical.image_map.has_value(),
            "checked original line image not selected");
    if (!seam && !bent)
      check(import_exact(budget, first->values.view(physical.parameter_chord_upper))
                    .compare(Rational::from_double(budget, .05)) > 0,
            "speed fixture did not exercise distinct parameter/image meanings");
    for (const auto &row : first->sites) {
      check(row.boundary_node < first->boundary->nodes.size(),
            "use site lost shared physical authority");
      check(import_exact(budget, first->values.view(row.node_source_upper))
                    .compare(Rational::from_double(budget, 1.e-8)) <= 0,
            "original node guard not retained");
    }
    refused = false;
    try {
      session.begin(
          {1, first->values, physical.parameter_chord_upper, physical.parameter_chord_upper, {}});
    } catch (const Error &) {
      refused = true;
    }
    check(refused, "active curve query accepted another begin");
    session.release_result();
    const auto second = query(session, run, {2, .005});
    check(second->uses.size() == 4 && (!seam && !bent ? second->physical_intervals.size() == 4
                                                      : second->physical_intervals.size() >=
                                                            first->physical_intervals.size()),
          "warm query lost correct quality reselection");
    check(first->boundary->nodes[0].xyz == second->boundary->nodes[0].xyz,
          "retained boundary mutated across query");
    const auto counts = session.stats();
    check(counts.queries == 2 && counts.nodes_reused > 0 && counts.point_hits > 0 &&
              counts.chord_tests >= 8,
          "persistent curve cache was bypassed");
    if (seam) {
      const auto a = std::find_if(second->uses.begin(), second->uses.end(), [&](const auto &row) {
        return source.view().coedges[row.use.value].id.value == 501;
      });
      const auto b = std::find_if(second->uses.begin(), second->uses.end(), [&](const auto &row) {
        return source.view().coedges[row.use.value].id.value == 503;
      });
      check(a != second->uses.end() && b != second->uses.end(), "literal seam use missing");
      const auto &as = second->sites[a->sites.first];
      const auto &bs = second->sites[b->sites.first + b->sites.count - 1];
      check(second->boundary->nodes[as.boundary_node].key ==
                second->boundary->nodes[bs.boundary_node].key,
            "seam occurrences lost single physical authority");
      check(import_exact(budget, second->values.view(as.chosen_original_uv[0]))
                    .compare(import_exact(budget, second->values.view(bs.chosen_original_uv[0]))) !=
                0,
            "literal seam chart occurrences were welded");
    }
    if (argc == 5)
      check(counts.device_points && counts.device_bounds && counts.uploaded_bytes &&
                counts.downloaded_bytes && (!bent || counts.device_restrictions),
            "device geometry/costs missing");
    {
      check(!second->boundary->intervals.empty(), "runtime fixture has no interval for feedback");
      const auto requested = second->boundary->intervals.front();
      session.refine(second, std::span(&requested, 1));
      JobContext continuation({uint64_t{1} << 27, uint64_t{1} << 38}, run.get(), {});
      bool complete = false;
      for (uint32_t step = 0; step < 10000000; ++step) {
        if (session.step(continuation) == StepState::done) {
          complete = true;
          break;
        }
        std::this_thread::yield();
      }
      check(complete, "consumer boundary continuation did not finish");
      const auto refined = session.result();
      check(refined->query_generation == second->query_generation &&
                refined->boundary->epoch.value > second->boundary->epoch.value &&
                refined->boundary->nodes.size() > second->boundary->nodes.size(),
            "consumer request did not continue the same shared authority");
      refused = false;
      try {
        session.refine(second, std::span(&requested, 1));
      } catch (const Error &error) {
        refused = error.code == ErrorCode::invalid_source;
      }
      check(refused && session.result() == refined, "stale feedback mutated current boundary");
    }
    session.release_result();
    if (bent && first_curve.degree == 2) {
      const auto before = session.stats().installed_splits;
      const auto exact = query(session, run, {3, 0., true});
      check(exact->physical_intervals.size() == 4 && session.stats().installed_splits == before,
            "exact residual equality caused geometric split");
      const auto upper = import_exact(
          budget, exact->values.view(exact->physical_intervals[0].parameter_chord_upper));
      check(upper.compare(Rational(budget, 2).divide(Rational(budget, 3))) <= 0,
            "exact accepted upper was rounded beyond allowance");
      session.release_result();
    }
    session.close();
    if (use_supplied) {
      check(first->carrier_source && &first->carrier_source->view() == &supplied.view(),
            "retained trace profile did not survive session close");
      const std::array narrow_request{CarrierRequest{{0}, {.25, .75, .25, .75}}};
      const auto narrow = prepare_trim_source_cache(source, narrow_request, run, {}, {}, budget);
      AdaptiveBoundarySession constrained(run, source, {0}, roots, narrow, limits);
      bool outside = false;
      try {
        (void)query(constrained, run, {1, .05});
      } catch (const Error &error) {
        outside = error.code == ErrorCode::unsupported &&
                  std::string(error.what()).find("supplied carrier profile") != std::string::npos;
      }
      check(outside, "narrow supplied profile silently expanded to chosen outside UV");
      bool partial = false;
      try {
        (void)constrained.result();
      } catch (const Error &) {
        partial = true;
      }
      check(partial, "outside supplied profile exposed a partial snapshot");
      constrained.close();
      const auto missing = prepare_trim_source_cache(source, {}, run, {}, {}, budget);
      bool closure = false;
      try {
        AdaptiveBoundarySession absent(run, source, {0}, roots, missing, limits);
      } catch (const Error &error) {
        closure = error.code == ErrorCode::invalid_source;
      }
      check(closure, "supplied bank omitted required original carrier closure");
    }
    auto tiny_limits = limits;
    tiny_limits.nodes = 1;
    AdaptiveBoundarySession bounded(run, source, {0}, roots, tiny_limits);
    refused = false;
    try {
      (void)query(bounded, run, {1, .05});
    } catch (const Error &e) {
      refused = e.code == ErrorCode::resource_exhausted;
    }
    check(refused, "whole physical frontier ignored the node resource cap");
    refused = false;
    try {
      (void)bounded.result();
    } catch (const Error &) {
      refused = true;
    }
    check(refused, "resource failure exposed partial shared curves");
    bounded.close();
    const auto other = load_source(argv[1], run);
    const auto other_bank = prepare_trim_source_cache(other, carrier_requests, run, {}, {}, budget);
    refused = false;
    try {
      AdaptiveBoundarySession foreign_bank(run, source, {0}, roots, other_bank, limits);
    } catch (const Error &error) {
      refused = error.code == ErrorCode::invalid_source;
    }
    check(refused, "session accepted foreign carrier source profile");
    refused = false;
    try {
      AdaptiveBoundarySession foreign(run, other, {0}, roots, limits);
    } catch (const Error &e) {
      refused = e.code == ErrorCode::invalid_source;
    }
    check(refused, "session accepted foreign immutable source closure");
    AdaptiveBoundarySession cancelled(run, source, {0}, roots, limits);
    cancelled.cancel();
    refused = false;
    try {
      (void)query(cancelled, run, {1, .05});
    } catch (const Error &e) {
      refused = e.code == ErrorCode::cancelled;
    }
    check(refused, "cancelled session admitted source work");
    cancelled.close();
    std::cout << "shared curve runtime GREEN\t" << counts.point_evaluations << '\t'
              << counts.point_hits << '\t' << counts.chord_tests << '\n';
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
