#include "../native/mesher/adaptive_caller_io.hpp"
#include "../native/mesher/adaptive_regions.hpp"
#include "../native/mesher/adaptive_trimmed_io.hpp"
#include <iostream>
#include <set>
#include <thread>

using namespace bgmesh;
using namespace bgmesh::adaptive;
namespace {
void require(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}
void run_boundary(AdaptiveBoundarySession &owner, JobContext &context) {
  for (uint32_t i = 0; i < 1000000; ++i) {
    if (owner.step(context) == StepState::done)
      return;
    std::this_thread::yield();
  }
  throw std::runtime_error("boundary did not settle");
}
TrimFaceProgress run_face(FaceTrimSession &owner, JobContext &context) {
  for (uint32_t i = 0; i < 1000000; ++i) {
    const auto progress = owner.step(context);
    if (progress == TrimFaceProgress::complete || progress == TrimFaceProgress::needs_boundary)
      return progress;
    std::this_thread::yield();
  }
  throw std::runtime_error("face did not settle");
}
} // namespace
int main(int argc, char **argv) {
  try {
    require(argc == 4 || argc == 6, "usage: assignment source output threads [module device]");
    const std::filesystem::path output(argv[2]);
    auto resource = std::make_shared<RunResource>(uint64_t{1} << 30);
    JobContext context({uint64_t{1} << 27, uint64_t{1} << 42}, resource.get(), {});
    const auto source = load_source(argv[1], resource);
    const auto &original = source.view();
    require(original.faces.size() == 5, "four regular faces and one closed curved face required");
    const auto requests = caller::definition_curve_requests(source, {0}, resource);
    const auto curves =
        prepare_curve_source_cache(source, requests, resource, {}, {}, context.budget);
    std::vector<CarrierRequest> carriers;
    for (uint32_t i = 0; i < original.carriers.size(); ++i)
      carriers.push_back({{i}, original.carriers[i].uv_bounds});
    const auto cache =
        prepare_trim_source_cache(source, carriers, resource, {}, {}, context.budget);
    CurveLimits curve_limits;
    curve_limits.cpu_threads = static_cast<uint32_t>(std::stoul(argv[3]));
    AdaptiveBoundarySession boundary(resource, source, {0}, curves, curve_limits);
    TrimFaceLimits limits;
    limits.surface.cpu_threads = curve_limits.cpu_threads;
    limits.surface.nodes = 1;
    limits.surface.points = 4;
    limits.surface.frontier = 64;
    limits.facets.batch_records = 64;
    FaceTrimSession owner(resource, cache, limits);
    if (argc == 6)
      owner.open_device({static_cast<int32_t>(std::stoi(argv[5])), argv[4]});
    uint64_t assignment = 0;
    std::set<ExactTableKey> construction_keys, surface_keys, domain_keys;
    FaceTrimResult first, second;
    for (uint64_t query = 1; query <= 2; ++query) {
      ExactBuilder quality(resource, {{OwnerKind::run, 801, 1}, query, 1});
      const auto positional = quality.append(Rational::from_double(context.budget, .01));
      const auto loose = quality.append(Rational(context.budget, 16));
      const auto delta = quality.append(Rational::from_double(context.budget, 1.e-8));
      const auto values = quality.seal();
      boundary.begin({query, values, loose, delta, {}});
      run_boundary(boundary, context);
      for (uint32_t face = 0; face < 4; ++face) {
        TrimFaceQuery input{query,  {0},        {face}, boundary.result(),
                            values, positional, delta,  ++assignment};
        auto bad = input;
        bad.face = {UINT32_MAX};
        bool invalid = false;
        try {
          owner.begin(bad);
        } catch (const Error &error) {
          invalid = error.code == ErrorCode::invalid_source;
        }
        require(invalid, "invalid assignment preflight accepted");
        if (query == 1 && face == 0) {
          auto other = std::make_shared<RunResource>(uint64_t{1} << 20);
          ExactBuilder foreign(other, {{OwnerKind::run, 801, 2}, 1, 1});
          const auto foreign_tau = foreign.append(Rational::from_double(context.budget, .01));
          const auto foreign_delta = foreign.append(Rational::from_double(context.budget, 1.e-8));
          const auto foreign_values = foreign.seal();
          auto wrong_values = input;
          wrong_values.values = foreign_values;
          wrong_values.positional = foreign_tau;
          wrong_values.node_source = foreign_delta;
          bool refused = false;
          try {
            owner.begin(wrong_values);
          } catch (const Error &error) {
            refused = error.code == ErrorCode::invalid_source;
          }
          require(refused, "foreign assignment allowance owner accepted");
        }
        owner.begin(input);
        require(run_face(owner, context) == TrimFaceProgress::complete,
                "regular assignment did not complete");
        const auto result = owner.result();
        require(result->construction->key.assignment_epoch == assignment &&
                    result->surface.key().face.assignment_epoch == assignment,
                "assignment identity lost");
        require(construction_keys.insert(result->construction->values.key()).second &&
                    surface_keys.insert(result->surface.selection()->values.key()).second &&
                    domain_keys.insert(result->construction->domain->values.key()).second,
                "same-quality table key reused for different face values");
        if (!first)
          first = result;
        if (query == 1 && face == 1)
          second = result;
        const auto stem = std::to_string(query) + "-" + std::to_string(face);
        caller::write_curves(output / (stem + ".bgacrv"), *result->boundary, context.budget);
        caller::write_trimmed_face(output / (stem + ".bgatrm"), result, context.budget);
        owner.retire_assignment();
        bool stale = false;
        try {
          owner.begin(input);
        } catch (const Error &error) {
          stale = error.code == ErrorCode::invalid_source;
        }
        require(stale, "nonmonotone assignment accepted after retirement");
      }
      if (query == 1) {
        owner.begin({query, {0}, {4}, boundary.result(), values, loose, delta, ++assignment});
        require(run_face(owner, context) == TrimFaceProgress::needs_boundary,
                "closed curved source did not yield real shared-boundary requests");
        const auto captured = boundary.result();
        const std::vector<BoundaryKey> wanted(owner.boundary_requests().begin(),
                                              owner.boundary_requests().end());
        require(!wanted.empty(), "empty real boundary continuation");
        owner.retire_assignment();
        boundary.refine(captured, wanted);
        run_boundary(boundary, context);
      }
      boundary.release_result();
    }
    require(first && first->construction->sites.size() >= 4, "old host face lost");
    {
      // The standalone cache's public operations must reject an old resident
      // capture; complete real sample/bound work before retiring its pages.
      SurfaceCacheSession direct(resource, cache, limits.surface);
      const auto select = [&](const FaceTrimResult &face) {
        std::vector<RootAllowance> allowed;
        const auto carrier = original.carrier_index.find(
            original.faces[face->construction->key.face.value].carrier_id);
        for (uint32_t i = 0; i < cache.roots().size(); ++i)
          if (cache.roots()[i].source_slot == carrier.value)
            allowed.push_back({{i}, face->positional_upper});
        direct.begin({1, face->surface.key().face, cache, face->bound_values, allowed});
        FaceDomainClassifier classifier(resource, face->construction->domain, {});
        for (uint32_t i = 0; i < 1000000; ++i) {
          const auto state = direct.step(context);
          if (state == SurfaceProgress::needs_classification)
            direct.classify(classifier.classify(direct.candidates()));
          else if (state == SurfaceProgress::selected)
            return direct.selection();
        }
        throw std::runtime_error("standalone assignment did not select");
      };
      const auto old = select(first);
      const std::array<SurfacePointInput, 1> point{{{old.selection()->points[0].original_uv}}};
      const SurfacePointRequest sample{old, old.selection()->values, 1, point};
      direct.sample(sample);
      bool pending = false;
      try {
        direct.retire_assignment();
      } catch (const Error &error) {
        pending = error.code == ErrorCode::invalid_source;
      }
      require(pending, "retirement accepted pending point work");
      while (direct.step(context) != SurfaceProgress::points_complete)
        std::this_thread::yield();
      const auto old_points = direct.point_result();
      direct.release_operation();
      const auto packets = build_packet_mesh(context.budget, resource, old.selection(), {});
      const auto construction = build_face_regions({first->construction->domain,
                                                    old,
                                                    packets,
                                                    first->construction->key,
                                                    {{OwnerKind::face, 0, 990}, 1, 1},
                                                    {},
                                                    {},
                                                    {},
                                                    {},
                                                    {}},
                                                   context);
      std::vector<uint32_t> facets;
      for (uint32_t i = 0; i < construction->facets.size(); ++i)
        facets.push_back(i);
      const SurfaceFacetRequest bound{old,
                                      construction,
                                      construction->domain,
                                      {first->bound_values, first->positional_upper},
                                      facets,
                                      1,
                                      limits.facets};
      direct.bound(bound);
      while (direct.step(context) != SurfaceProgress::facets_complete)
        std::this_thread::yield();
      const auto old_bounds = direct.facet_result();
      for (const auto &answer : old_bounds->facets)
        require(answer.accepted, "real original plane bound refused");
      direct.retire_assignment();
      const auto current = select(second);
      const std::array<LeafKey, 1> splits{old.selection()->leaves[0].key};
      uint32_t refused = 0;
      try {
        direct.sample(sample);
      } catch (const Error &error) {
        refused += error.code == ErrorCode::invalid_source;
      }
      try {
        direct.refine({old, splits, {}, {}, {}});
      } catch (const Error &error) {
        refused += error.code == ErrorCode::invalid_source;
      }
      try {
        direct.bound(bound);
      } catch (const Error &error) {
        refused += error.code == ErrorCode::invalid_source;
      }
      require(refused == 3 && direct.selection().selection() == current.selection(),
              "old resident request entered another assignment");
      require(old_points->points.size() == 1 && old_bounds->facets.size() == facets.size(),
              "old standalone host operation lost copied values");
      direct.retire_assignment();
      direct.close();
    }
    const auto stats = owner.stats();
    if (argc == 6)
      require(stats.surface.device_points > 0 && stats.surface.device_bounds > 0,
              "device assignment omitted geometry");
    owner.close();
    boundary.close();
    caller::write_trimmed_face(output / "first-after-close.bgatrm", first, context.budget);
    caller::write_curves(output / "first-after-close.bgacrv", *first->boundary, context.budget);
    std::cout << "real pooled assignments PASS\t" << assignment << '\n';
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
