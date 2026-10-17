#include "../native/mesher/adaptive_curves.hpp"
#include "../native/mesher/adaptive_regions.hpp"
#include "../native/mesher/adaptive_trimmed.hpp"
#include "../native/mesher/adaptive_trimmed_geometry.hpp"
#include <iomanip>
#include <iostream>
#include <numeric>
#include <thread>
#include <vector>

using namespace bgmesh;
using namespace bgmesh::adaptive;
namespace {
void require(bool condition, const char *message) {
  if (!condition)
    throw std::runtime_error(message);
}
void print_exact(const Rational &value) {
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
} // namespace
int main(int argc, char **argv) {
  try {
    require(argc == 2, "original source required");
    auto resource = std::make_shared<RunResource>(uint64_t{1} << 31);
    JobContext context({uint64_t{1} << 28, uint64_t{1} << 42}, resource.get(),
                       {});
    auto &budget = context.budget;
    const auto source = load_source(argv[1], resource);
    const auto &original = source.view();
    require(original.faces.size() == 1, "one original source face required");
    std::vector<CurveRequest> requests;
    for (uint32_t i = 0; i < original.curves.size(); ++i)
      requests.push_back({{i}, original.curves[i].parameter_range});
    const auto curves =
        prepare_curve_source_cache(source, requests, resource, {}, {}, budget);
    CurveLimits curve_limits;
    curve_limits.cpu_threads = 4;
    curve_limits.host_bytes = uint64_t{1} << 28;
    AdaptiveBoundarySession boundary(resource, source, {0}, curves,
                                     curve_limits);
    ExactBuilder quality(resource, {{OwnerKind::run, 811, 1}, 1, 1});
    const auto tau = quality.append(Rational::from_double(budget, .05));
    const auto delta = quality.append(Rational::from_double(budget, 1.e-8));
    const auto values = quality.seal();
    boundary.begin({1, values, tau, delta, {}});
    SharedCurveSnapshot chains;
    for (uint32_t step = 0; step < 1000000; ++step) {
      if (boundary.step(context) == StepState::done) {
        chains = boundary.result();
        break;
      }
      std::this_thread::yield();
    }
    require(static_cast<bool>(chains),
            "shared source boundary did not complete");
    const auto carrier =
        original.carrier_index.find(original.faces[0].carrier_id);
    const CarrierRequest request{carrier, {0., 1., 0., 1.}};
    const auto cache = prepare_trim_source_cache(source, std::span(&request, 1),
                                                 resource, {}, {}, budget);
    const FaceConstructionKey identity{cache.generation(),
                                       1,
                                       chains->uses.front().boundary_generation,
                                       1,
                                       cache.profile().revision,
                                       1,
                                       {0},
                                       {0}};
    const auto domain = prepare_face_domain(
        {chains, identity, {{OwnerKind::face, 0, 900}, 1, 1}}, context);
    require(domain.domain && domain.requests.empty(),
            "source domain did not admit");
    FaceDomainClassifier classifier(resource, domain.domain, {});
    CacheLimits limits;
    limits.cpu_threads = 4;
    limits.host_bytes = uint64_t{1} << 28;
    SurfaceCacheSession surface(resource, cache, limits);
    std::vector<RootAllowance> root_allowances;
    for (uint32_t i = 0; i < cache.roots().size(); ++i)
      root_allowances.push_back({{i}, tau});
    surface.begin(
        {1,
         {{0}, {0}, cache.generation(), identity.boundary_generation, 1, 1},
         cache,
         values,
         root_allowances});
    bool selected = false;
    for (uint32_t step = 0; step < 1000000; ++step) {
      const auto progress = surface.step(context);
      if (progress == SurfaceProgress::needs_classification)
        surface.classify(classifier.classify(surface.candidates()));
      else if (progress == SurfaceProgress::selected) {
        selected = true;
        break;
      }
    }
    require(selected, "real surface source cover did not complete");
    const auto view = surface.selection();
    const auto packets =
        build_packet_mesh(budget, resource, view.selection(), {});
    const auto mesh = build_face_regions({domain.domain,
                                          view,
                                          packets,
                                          identity,
                                          {{OwnerKind::face, 0, 900}, 1, 2},
                                          {},
                                          {},
                                          {},
                                          {},
                                          {}},
                                         context);
    std::vector<uint32_t> facets(mesh->facets.size());
    std::iota(facets.begin(), facets.end(), 0U);
    const auto work = prepare_trim_facet_work(
        context, view, mesh, domain.domain, {values, tau}, facets,
        {{OwnerKind::face, 0, 900}, 1, 99});
    require(work.facets().size() == facets.size() && !facets.empty(),
            "actual facet work is incomplete");
    bool crosses = false;
    std::cout << std::hexfloat;
    for (uint32_t i = 0; i < work.facets().size(); ++i) {
      const auto &record = work.facets()[i];
      const auto &facet = mesh->facets[record.construction_facet];
      crosses = crosses || record.pieces.count > 1;
      std::cout << "trim-work-parent\t" << i;
      for (const auto site : facet.sites)
        for (const auto ref : mesh->sites[site].uv)
          print_exact(import_exact(budget, mesh->values.view(ref)));
      for (const auto site : facet.sites)
        for (const auto value : mesh->sites[site].actual_xyz)
          std::cout << '\t' << value;
      std::cout << '\n';
    }
    require(crosses, "actual facet did not cross cached leaf boundaries");
    for (uint32_t i = 0; i < work.pieces().size(); ++i) {
      const auto &piece = work.pieces()[i];
      const std::array ids{i};
      const auto packet = pack_source_pieces(context, work, ids);
      const auto result = bound_source_triangle(cache, packet.inputs[0]);
      require(result.status == NumericStatus::ready,
              "actual virtual source-piece bound unavailable");
      std::cout << "trim-work-piece\t" << piece.parent << '\t' << piece.leaf
                << '\t' << result.position_upper;
      for (uint64_t at = piece.original_uv.first;
           at < piece.original_uv.first + piece.original_uv.count; ++at)
        print_exact(import_exact(
            budget, work.values().view({work.values().key(), at})));
      for (uint64_t at = piece.affine_xyz.first;
           at < piece.affine_xyz.first + piece.affine_xyz.count; ++at)
        print_exact(import_exact(
            budget, work.values().view({work.values().key(), at})));
      std::cout << '\n';
    }
    // Exercise the actual persistent runtime executor on this SAME source,
    // minted cover and construction. No authored numerical decisions.
    auto reversed = facets;
    std::reverse(reversed.begin(), reversed.end());
    surface.bound({view, mesh, domain.domain, {values, tau}, reversed, 1, {}});
    bool complete = false;
    for (uint32_t step = 0; step < 1000000; ++step) {
      const auto state = surface.step(context);
      if (state == SurfaceProgress::facets_complete) {
        complete = true;
        break;
      }
      require(state == SurfaceProgress::pending ||
                  state == SurfaceProgress::progressed,
              "bound executor entered an unrelated operation");
      std::this_thread::yield();
    }
    require(complete, "whole actual-facet horizon did not complete");
    const auto completed = surface.facet_result();
    require(completed->work.key().surface == view.key() &&
                completed->work.key().construction == mesh->key &&
                completed->ordinal == 1 &&
                completed->facets.size() == reversed.size(),
            "bound executor lost operation identity or subset");
    for (uint32_t i = 0; i < reversed.size(); ++i) {
      const auto &answer = completed->facets[i];
      require(answer.facet == reversed[i],
              "bound executor reordered captured subset");
      if (answer.accepted) {
        require(
            answer.positional_upper &&
                import_exact(budget,
                             completed->values.view(*answer.positional_upper))
                        .compare(import_exact(budget, values.view(tau))) <= 0,
            "bound executor accepted beyond the exact requested allowance");
      }
    }
    surface.release_operation();
    const std::array splits{view.selection()->leaves.front().key};
    surface.refine({view, splits, {}, {}, mesh});
    selected = false;
    for (uint32_t step = 0; step < 1000000; ++step) {
      const auto progress = surface.step(context);
      if (progress == SurfaceProgress::needs_classification)
        surface.classify(classifier.classify(surface.candidates()));
      else if (progress == SurfaceProgress::selected) {
        selected = true;
        break;
      }
    }
    require(selected, "refined source view did not complete");
    bool stale_rejected = false;
    try {
      (void)prepare_trim_facet_work(context, surface.selection(), mesh,
                                    domain.domain, {values, tau}, facets,
                                    {{OwnerKind::face, 0, 900}, 1, 100});
    } catch (const Error &error) {
      stale_rejected = error.code == ErrorCode::invalid_source;
    }
    require(stale_rejected,
            "new source cover relabeled stale construction facets");
    bool stale_runtime = false;
    try {
      surface.bound({surface.selection(),
                     mesh,
                     domain.domain,
                     {values, tau},
                     facets,
                     2,
                     {}});
    } catch (const Error &error) {
      stale_runtime = error.code == ErrorCode::invalid_source;
    }
    require(stale_runtime && surface.selection().key() != view.key(),
            "runtime accepted stale construction or discarded current cover");
    surface.cancel();
    bool cancelled = false;
    try {
      surface.bound({surface.selection(),
                     mesh,
                     domain.domain,
                     {values, tau},
                     facets,
                     3,
                     {}});
    } catch (const Error &error) {
      cancelled = error.code == ErrorCode::cancelled;
    }
    require(cancelled, "bound request ignored owner cancellation");
    surface.close();
    // Reuse the SAME original source/boundary for coordinator protocol checks;
    // the independent geometric reader above remains the sole shape oracle.
    TrimFaceLimits face_limits;
    face_limits.surface = limits;
    const TrimFaceQuery face_query{1, {0}, {0}, chains, values, tau, delta};
    {
      FaceTrimSession face_session(resource, cache, face_limits);
      bool inactive = false;
      try {
        (void)face_session.step(context);
      } catch (const Error &error) {
        inactive = error.code == ErrorCode::invalid_source;
      }
      require(inactive, "inactive face step did not refuse");
      face_session.begin(
          face_query); // preflight refusal did not mutate query state
      (void)face_session.step(
          context); // install the real persistent classifier
      std::stop_source caller_stop;
      JobContext stopped_context({uint64_t{1} << 26, uint64_t{1} << 40},
                                 resource.get(), caller_stop.get_token());
      caller_stop.request_stop();
      bool interrupted = false;
      try {
        (void)face_session.step(stopped_context);
      } catch (const bgfd::Cancelled &) {
        interrupted = true;
      } catch (const Error &error) {
        interrupted = error.code == ErrorCode::cancelled;
      }
      require(interrupted, "face step ignored caller cancellation");
      bool terminal = false;
      try {
        (void)face_session.step(context);
      } catch (const Error &error) {
        terminal = error.code == ErrorCode::cancelled;
      }
      require(terminal, "cancelled face coordinator resumed computation");
      bool no_result = false;
      try {
        (void)face_session.result();
      } catch (const Error &error) {
        no_result = error.code == ErrorCode::invalid_source;
      }
      require(no_result, "cancelled face coordinator published partial output");
      face_session.close();
    }
    {
      std::stop_source external_stop;
      FaceTrimSession face_session(resource, cache, face_limits,
                                   external_stop.get_token());
      external_stop.request_stop();
      bool interrupted = false;
      try {
        face_session.begin(face_query);
      } catch (const Error &error) {
        interrupted = error.code == ErrorCode::cancelled;
      }
      require(interrupted, "face coordinator ignored external cancellation");
      face_session.close();
    }
    {
      auto limited = face_limits;
      limited.construction_work = 1;
      FaceTrimSession face_session(resource, cache, limited);
      bool exhausted = false;
      try {
        face_session.begin(face_query);
      } catch (const bgfd::Failure &error) {
        exhausted = error.status == BGFD_RESOURCE_EXHAUSTED;
      }
      require(exhausted, "face admission exceeded its construction work cap");
      face_session.close();
    }
    boundary.close();
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
