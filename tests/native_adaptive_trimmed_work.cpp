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
void print_intervals(std::span<const Interval, 9> coordinates) {
  for (const auto &coordinate : coordinates)
    std::cout << '\t' << coordinate.lower << '\t' << coordinate.upper;
}
} // namespace
int main(int argc, char **argv) {
  try {
    require(argc == 2, "original source required");
    auto resource = std::make_shared<RunResource>(uint64_t{1} << 31);
    JobContext context({uint64_t{1} << 28, uint64_t{1} << 42}, resource.get(), {});
    auto &budget = context.budget;
    const auto source = load_source(argv[1], resource);
    const auto &original = source.view();
    require(original.faces.size() == 1, "one original source face required");
    std::vector<CurveRequest> requests;
    for (uint32_t i = 0; i < original.curves.size(); ++i)
      requests.push_back({{i}, original.curves[i].parameter_range});
    const auto curves = prepare_curve_source_cache(source, requests, resource, {}, {}, budget);
    CurveLimits curve_limits;
    curve_limits.cpu_threads = 4;
    curve_limits.host_bytes = uint64_t{1} << 28;
    AdaptiveBoundarySession boundary(resource, source, {0}, curves, curve_limits);
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
    require(static_cast<bool>(chains), "shared source boundary did not complete");
    const auto carrier = original.carrier_index.find(original.faces[0].carrier_id);
    const CarrierRequest request{carrier, {0., 1., 0., 1.}};
    const auto cache =
        prepare_trim_source_cache(source, std::span(&request, 1), resource, {}, {}, budget);
    const FaceConstructionKey identity{cache.generation(),
                                       1,
                                       chains->uses.front().boundary_generation,
                                       1,
                                       cache.profile().revision,
                                       1,
                                       {0},
                                       {0}};
    const auto domain =
        prepare_face_domain({chains, identity, {{OwnerKind::face, 0, 900}, 1, 1}}, context);
    require(domain.domain && domain.requests.empty(), "source domain did not admit");
    FaceDomainClassifier classifier(resource, domain.domain, {});
    CacheLimits limits;
    limits.cpu_threads = 4;
    limits.host_bytes = uint64_t{1} << 28;
    SurfaceCacheSession surface(resource, cache, limits);
    std::vector<RootAllowance> root_allowances;
    for (uint32_t i = 0; i < cache.roots().size(); ++i)
      root_allowances.push_back({{i}, tau});
    surface.begin({1,
                   {{0},
                    {0},
                    cache.generation(),
                    identity.boundary_generation,
                    effective_face_generation(identity),
                    1},
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
    const auto packets = build_packet_mesh(budget, resource, view.selection(), {});
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
    const auto candidates =
        prepare_trim_facet_candidates(context, view, mesh, domain.domain, {values, tau}, facets,
                                      {{OwnerKind::face, 0, 900}, 1, 98});
    require(candidates.facets().size() == facets.size(), "candidate parent inventory differs");
    require(candidates.stats().clip_vertices == 0 && candidates.stats().candidate_visits == 0,
            "candidate preparation eagerly materialized leaf clips");
    std::vector<uint32_t> band_rows;
    uint32_t direct_passes = 0;
    for (uint32_t i = 0; i < candidates.facets().size(); ++i) {
      const auto &parent = candidates.facets()[i];
      const auto &facet = mesh->facets[parent.construction_facet];
      if (facet.key.generator == FacetGenerator::packet) {
        require(parent.pieces.count == 1, "packet did not retain one known-leaf piece");
        const auto &piece = candidates.pieces()[parent.pieces.first];
        require(piece.geometry == TrimPieceGeometry::whole_parent && piece.leaf == facet.leaf &&
                    piece.original_uv.count == 0 && piece.barycentric.count == 0,
                "packet direct piece was clipped or copied virtual coordinates");
        const std::array piece_ids{static_cast<uint32_t>(parent.pieces.first)};
        const auto packed = pack_base_pieces(context, candidates, piece_ids);
        for (uint32_t j = 0; j < 3; ++j)
          for (uint32_t axis = 0; axis < 3; ++axis) {
            const auto actual = mesh->sites[facet.sites[j]].actual_xyz[axis];
            const auto value = packed.inputs[0].affine_xyz[3 * j + axis];
            require(value.lower == actual && value.upper == actual,
                    "packet filter changed actual Q");
          }
      } else if (!parent.original_plane) {
        require(parent.pieces.count == 0, "candidate Band already has numerical leaf pieces");
        band_rows.push_back(i);
        if (parent.whole_original) {
          const std::array parent_ids{i};
          const auto packed = pack_original_facets(context, candidates, parent_ids);
          const auto answer = bound_source_triangle(cache, packed.inputs[0]);
          direct_passes += answer.status == NumericStatus::ready &&
                           Rational::from_double(budget, answer.position_upper)
                                   .compare(import_exact(budget, values.view(tau))) <= 0;
        }
      }
    }
    require(!band_rows.empty(), "real candidate work omitted Band parents");
    const auto fallback = materialize_trim_facet_fallback(context, candidates, band_rows,
                                                          {{OwnerKind::face, 0, 900}, 1, 97});
    require(fallback.facets().size() == candidates.facets().size(),
            "fallback discarded direct parent rows");
    for (uint32_t i = 0; i < fallback.facets().size(); ++i)
      require(fallback.facets()[i].construction_facet == candidates.facets()[i].construction_facet,
              "fallback reordered captured parent rows");
    for (auto i : band_rows)
      require(fallback.facets()[i].pieces.count != 0,
              "unaccepted Band omitted canonical fallback pieces");
    // The initial boundary-only CDT is deliberately coarse and may have no
    // passing whole-source Band. Obtain the accepted construction at the SAME
    // requested quality through the actual face owner, rather than widening tau.
    TrimFaceLimits accepted_limits;
    accepted_limits.surface = limits;
    FaceTrimSession face_owner(resource, cache, accepted_limits);
    face_owner.begin({1, {0}, {0}, chains, values, tau, delta});
    bool face_complete = false;
    for (uint32_t step = 0; step < 1000000; ++step) {
      const auto state = face_owner.step(context);
      if (state == TrimFaceProgress::complete) {
        face_complete = true;
        break;
      }
      if (state == TrimFaceProgress::needs_boundary) {
        boundary.refine(chains, face_owner.boundary_requests());
        bool settled = false;
        for (uint32_t child = 0; child < 1000000; ++child)
          if (boundary.step(context) == StepState::done) {
            settled = true;
            break;
          }
        require(settled, "accepted construction physical horizon did not settle");
        chains = boundary.result();
        face_owner.replace_boundary(chains);
      }
    }
    require(face_complete, "same-quality accepted construction did not complete");
    const auto accepted_face = face_owner.result();
    std::vector<uint32_t> accepted_ids(accepted_face->construction->facets.size());
    std::iota(accepted_ids.begin(), accepted_ids.end(), 0U);
    const auto accepted_candidates =
        prepare_trim_facet_candidates(context, accepted_face->surface, accepted_face->construction,
                                      accepted_face->construction->domain, {values, tau},
                                      accepted_ids, {{OwnerKind::face, 0, 900}, 1, 96});
    require(accepted_candidates.stats().clip_vertices == 0 &&
                accepted_candidates.stats().candidate_visits == 0,
            "accepted candidate pass eagerly clipped numerical leaves");
    direct_passes = 0;
    for (uint32_t i = 0; i < accepted_candidates.facets().size(); ++i) {
      const auto &parent = accepted_candidates.facets()[i];
      if (accepted_face->construction->facets[parent.construction_facet].key.generator !=
              FacetGenerator::band ||
          !parent.whole_original)
        continue;
      require(parent.pieces.count == 0, "accepted Band candidate materialized pieces");
      const std::array ids{i};
      const auto packed = pack_original_facets(context, accepted_candidates, ids);
      const auto answer = bound_source_triangle(cache, packed.inputs[0]);
      direct_passes += answer.status == NumericStatus::ready &&
                       Rational::from_double(budget, answer.position_upper)
                               .compare(import_exact(budget, values.view(tau))) <= 0;
    }
    require(direct_passes != 0, "actual accepted Band did not exercise whole-original filter");
    face_owner.close();
    const auto work = prepare_trim_facet_work(context, view, mesh, domain.domain, {values, tau},
                                              facets, {{OwnerKind::face, 0, 900}, 1, 99});
    require(work.facets().size() == facets.size() && !facets.empty(),
            "actual facet work is incomplete");
    // Selected staging contract: clipped rows retain exact parent barycentric
    // triples, rather than eagerly evaluated virtual XYZ. The previous payload
    // is valid geometry but does not satisfy this new representation contract.
    for (const auto &piece : work.pieces()) {
      require(piece.barycentric.count == 9, "clipped barycentric payload has wrong shape");
      for (uint32_t vertex = 0; vertex < 3; ++vertex) {
        auto sum = Rational(budget);
        for (uint32_t corner = 0; corner < 3; ++corner) {
          const auto lambda = import_exact(
              budget, work.values().view(
                          {work.values().key(), piece.barycentric.first + 3 * vertex + corner}));
          require(lambda.sign() >= 0, "selected clipped barycentric weight is negative");
          sum = sum.add(lambda);
        }
        require(sum.compare(Rational(budget, 1)) == 0,
                "selected clipped payload is not exact barycentric triples");
      }
    }
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
      std::cout << "trim-work-piece\t" << piece.parent << '\t' << piece.leaf << '\t'
                << result.position_upper;
      for (uint64_t at = piece.original_uv.first;
           at < piece.original_uv.first + piece.original_uv.count; ++at)
        print_exact(import_exact(budget, work.values().view({work.values().key(), at})));
      const auto &parent = work.facets()[piece.parent];
      const auto &actual_facet = mesh->facets[parent.construction_facet];
      for (uint32_t vertex = 0; vertex < 3; ++vertex)
        for (uint32_t axis = 0; axis < 3; ++axis) {
          auto value = Rational(budget);
          for (uint32_t corner = 0; corner < 3; ++corner) {
            const auto lambda = import_exact(
                budget, work.values().view(
                            {work.values().key(), piece.barycentric.first + 3 * vertex + corner}));
            value = value.add(lambda.multiply(Rational::from_double(
                budget, mesh->sites[actual_facet.sites[corner]].actual_xyz[axis])));
          }
          print_exact(value); // Test-only exact expression export; reader derives L independently.
        }
      const auto base = pack_base_pieces(context, work, ids);
      print_intervals(base.inputs[0].affine_xyz);
      print_intervals(packet.inputs[0].affine_xyz);
      std::cout << '\n';
    }
    require(!work.pieces().empty(), "precise virtual piece fixture is empty");
    const auto precise =
        compare_trim_piece_precise(context, work, 0, TrimBoundMethod::original_source);
    require(precise.bounds.status == NumericStatus::ready,
            "existing precise virtual piece route did not materialize exact SAME-L");
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
      require(state == SurfaceProgress::pending || state == SurfaceProgress::progressed,
              "bound executor entered an unrelated operation");
      std::this_thread::yield();
    }
    require(complete, "whole actual-facet horizon did not complete");
    const auto completed = surface.facet_result();
    require(completed->work.key().surface == view.key() &&
                completed->work.key().construction == mesh->key && completed->ordinal == 1 &&
                completed->facets.size() == reversed.size(),
            "bound executor lost operation identity or subset");
    for (uint32_t i = 0; i < reversed.size(); ++i) {
      const auto &answer = completed->facets[i];
      require(answer.facet == reversed[i], "bound executor reordered captured subset");
      if (answer.accepted) {
        require(answer.positional_upper &&
                    import_exact(budget, completed->values.view(*answer.positional_upper))
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
      (void)prepare_trim_facet_work(context, surface.selection(), mesh, domain.domain,
                                    {values, tau}, facets, {{OwnerKind::face, 0, 900}, 1, 100});
    } catch (const Error &error) {
      stale_rejected = error.code == ErrorCode::invalid_source;
    }
    require(stale_rejected, "new source cover relabeled stale construction facets");
    bool stale_runtime = false;
    try {
      surface.bound({surface.selection(), mesh, domain.domain, {values, tau}, facets, 2, {}});
    } catch (const Error &error) {
      stale_runtime = error.code == ErrorCode::invalid_source;
    }
    require(stale_runtime && surface.selection().key() != view.key(),
            "runtime accepted stale construction or discarded current cover");
    surface.cancel();
    bool cancelled = false;
    try {
      surface.bound({surface.selection(), mesh, domain.domain, {values, tau}, facets, 3, {}});
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
      face_session.begin(face_query);   // preflight refusal did not mutate query state
      (void)face_session.step(context); // install the real persistent classifier
      std::stop_source caller_stop;
      JobContext stopped_context({uint64_t{1} << 26, uint64_t{1} << 40}, resource.get(),
                                 caller_stop.get_token());
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
      FaceTrimSession face_session(resource, cache, face_limits, external_stop.get_token());
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
