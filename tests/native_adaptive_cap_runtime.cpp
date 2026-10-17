#include "../native/mesher/adaptive_caller_io.hpp"
#include "../native/mesher/adaptive_regions.hpp"
#include "../native/mesher/adaptive_trimmed.hpp"
#include "../native/mesher/adaptive_trimmed_io.hpp"
#include <iostream>
#include <thread>

using namespace bgmesh;
using namespace bgmesh::adaptive;
namespace {
void require(bool value, const char *why) {
  if (!value)
    throw std::runtime_error(why);
}
void settle_boundary(AdaptiveBoundarySession &owner, JobContext &context) {
  for (uint32_t i = 0; i < 1000000; ++i) {
    if (owner.step(context) == StepState::done)
      return;
    std::this_thread::yield();
  }
  throw std::runtime_error("cap boundary did not settle");
}
TrimFaceProgress settle_face(FaceTrimSession &owner, JobContext &context) {
  for (uint32_t i = 0; i < 1000000; ++i) {
    const auto state = owner.step(context);
    if (state == TrimFaceProgress::complete || state == TrimFaceProgress::needs_boundary)
      return state;
    std::this_thread::yield();
  }
  throw std::runtime_error("cap face did not settle");
}
template <class Function> void invalid(Function &&function, const char *why) {
  bool refused = false;
  try {
    function();
  } catch (const Error &error) {
    refused = error.code == ErrorCode::invalid_source;
  }
  require(refused, why);
}
void run(const char *path, uint32_t threads, const std::optional<CacheDevice> &device, bool mapped,
         const std::optional<std::filesystem::path> &output) {
  auto resource = std::make_shared<RunResource>(uint64_t{1} << 31);
  JobContext context({uint64_t{1} << 28, uint64_t{1} << 44}, resource.get(), {});
  const auto source = load_source(path, resource);
  require(source.view().faces.size() == 1, "cap runtime fixture needs one original face");
  const auto requests = caller::definition_curve_requests(source, {0}, resource);
  const auto curves =
      prepare_curve_source_cache(source, requests, resource, {}, {}, context.budget);
  const auto carrier = source.view().carrier_index.find(source.view().faces[0].carrier_id);
  const CarrierRequest request{carrier, source.view().carriers[carrier.value].uv_bounds};
  const auto cache = prepare_quotient_trim_source_cache(source, std::span(&request, 1), resource,
                                                        {}, {}, context.budget);
  CurveLimits curve_limits;
  curve_limits.cpu_threads = threads;
  AdaptiveBoundarySession boundary(resource, source, {0}, curves, cache, curve_limits);
  TrimFaceLimits limits;
  limits.surface.cpu_threads = threads;
  limits.surface.host_bytes = limits.surface.device_bytes = uint64_t{1} << 29;
  limits.surface.nodes = limits.surface.frontier = 1u << 16;
  limits.surface.points = 1u << 18;
  limits.facets.batch_records = 64;
  FaceTrimSession owner(resource, cache, limits);
  if (device) {
    boundary.open_device(*device);
    owner.open_device(*device);
  }
  ExactBuilder builder(resource, {{OwnerKind::run, 919, 1}, 1, 1});
  const auto tau = builder.append(Rational::from_double(context.budget, .02));
  const auto delta = builder.append(Rational::from_double(context.budget, 1.e-8));
  const auto values = builder.seal();
  boundary.begin({1, values, tau, delta, {}});
  settle_boundary(boundary, context);
  uint64_t assignment = 0;
  uint32_t boundary_rounds = 0;
  FaceContinuation resume;
  const auto complete = [&] {
    for (uint32_t round = 0; round < 16; ++round) {
      owner.begin({1, {0}, {0}, boundary.result(), values, tau, delta, ++assignment, resume});
      if (settle_face(owner, context) == TrimFaceProgress::complete)
        return;
      const auto expected = boundary.result();
      const std::vector<BoundaryKey> wanted(owner.boundary_requests().begin(),
                                            owner.boundary_requests().end());
      require(!wanted.empty(), "cap continuation omitted real physical request");
      resume = owner.continuation();
      owner.retire_assignment();
      boundary.refine(expected, wanted);
      settle_boundary(boundary, context);
      ++boundary_rounds;
    }
    throw std::runtime_error("bounded cap boundary continuation exhausted");
  };
  complete();
  const auto first = owner.result();
  std::cout << "cap_runtime\tinitial_complete\t" << mapped << std::endl;
  require(first->construction->domain->caps &&
              first->surface.classified_domain() == first->construction->domain,
          "quotient face lost produced domain authority");
  require(first->bound_values.view(first->positional_upper).sign > 0,
          "cap result has no accepted exact threshold");
  if (!mapped) {
    require(first->construction->domain->caps.caps().empty() && !owner.continuation(),
            "regular quotient-bank face fabricated cap history");
    SurfaceCacheSession direct(resource, cache, limits.surface);
    if (device)
      direct.open_device(*device);
    std::vector<RootAllowance> allowances;
    for (uint32_t i = 0; i < cache.roots().size(); ++i)
      allowances.push_back({{i}, tau});
    auto face = first->surface.key().face;
    invalid([&] { direct.begin({1, face, cache, values, allowances}); },
            "quotient source admitted without produced empty inventory");
    auto policy = first->construction->domain;
    direct.begin({1, face, cache, values, allowances, policy});
    auto classifier =
        std::make_shared<FaceDomainClassifier>(resource, policy, DomainClassifierLimits{});
    const auto select = [&] {
      for (uint32_t i = 0; i < 1000000; ++i) {
        const auto state = direct.step(context);
        if (state == SurfaceProgress::needs_classification)
          direct.classify(classifier->classify(direct.candidates()));
        else if (state == SurfaceProgress::selected)
          return;
        std::this_thread::yield();
      }
      throw std::runtime_error("direct cap view did not select");
    };
    select();
    const auto old_view = direct.selection();
    auto next_key = policy->key;
    ++next_key.face_generation;
    ++next_key.domain_revision;
    next_key.construction_revision = 0;
    CapDomainKey cap_key{next_key.source_generation,   next_key.query,
                         next_key.definition,          next_key.face,
                         next_key.boundary_generation, next_key.face_generation,
                         next_key.chart_revision,      next_key.domain_revision,
                         next_key.assignment_epoch,    next_key.profile_revision};
    const auto cap = prepare_cap_domain(
        {cache, boundary.result(), cap_key, {{OwnerKind::run, 919, 3}, 1, 1}, {}}, context);
    const auto domain = prepare_face_domain(
        {boundary.result(), next_key, {{OwnerKind::run, 919, 3}, 1, 2}, cap.plan}, context);
    require(domain.domain && domain.requests.empty(),
            "empty cap replacement did not produce domain");
    ++face.face_generation;
    ++face.domain_revision;
    auto wrong = allowances;
    wrong.front().positional = delta;
    invalid([&] { direct.replace_domain({old_view, domain.domain, face, values, wrong}); },
            "local domain replacement silently changed exact quality");
    require(direct.selection().key() == old_view.key(), "failed replacement mutated current view");
    direct.replace_domain({old_view, domain.domain, face, values, allowances});
    invalid([&] { direct.replace_domain({old_view, domain.domain, face, values, allowances}); },
            "stale local domain replacement accepted");
    invalid(
        [&] {
          direct.bound_caps(
              {old_view, first->construction, policy, {values, tau}, {}, 1, limits.facets});
        },
        "old resident bound accepted after domain replacement");
    classifier =
        std::make_shared<FaceDomainClassifier>(resource, domain.domain, DomainClassifierLimits{});
    select();
    const auto current = direct.selection();
    require(current.key().face.domain_revision == face.domain_revision &&
                current.key().selection.cover_revision > old_view.key().selection.cover_revision &&
                old_view.selection()->leaves.size() &&
                current.selection()->values.key() != old_view.selection()->values.key(),
            "local cut lost old host data or reused exact table identity");
    direct.release_query();
    direct.close();
    owner.retire_assignment();
    owner.close();
    boundary.close();
    std::cout << "cap_runtime\tproduced_empty\tPASS\n";
    return;
  }
  require(!first->construction->domain->caps.caps().empty() &&
              owner.stats().surface.cap_facet_tests,
          "mapped source did not execute real cap bound tasks");
  if (device)
    require(owner.stats().surface.device_cap_facets,
            "cap numerical work was not executed on device");
  const auto captured = owner.continuation();
  require(captured && captured->control.fixed_metric_factor(),
          "completed cap lost fixed metric continuation");
  const auto physical = boundary.result();
  const auto radial = continue_cap_domain({first->construction->domain->caps,
                                           CapCutDirection::radial,
                                           {{OwnerKind::run, 919, 2}, 1, 1},
                                           first->construction->values,
                                           first->construction->fixed_metric_factor},
                                          context);
  require(radial.pending_radial_sides() == 3, "radial control lost BOTH-side baseline");
  resume = std::allocate_shared<FaceContinuationData>(RunAllocator<FaceContinuationData>(resource),
                                                      resource, cache, captured->origin,
                                                      FacetAllowances{values, tau}, delta, radial);
  owner.retire_assignment();
  require(first->construction->facets.size() && captured->control.fixed_metric_factor(),
          "retirement invalidated old immutable host result/history");
  owner.begin({1, {0}, {0}, physical, values, tau, delta, ++assignment, resume});
  require(settle_face(owner, context) == TrimFaceProgress::needs_boundary,
          "uncontracted radial control silently emitted old-cut fan");
  std::cout << "cap_runtime\tforced_radial_boundary\tPASS" << std::endl;
  const auto pending = owner.continuation();
  require(pending && pending->control.pending_radial_sides() == 3,
          "retirement continuation reset one-sided contraction history");
  const std::vector<BoundaryKey> wanted(owner.boundary_requests().begin(),
                                        owner.boundary_requests().end());
  owner.retire_assignment();
  boundary.refine(physical, wanted);
  settle_boundary(boundary, context);
  ++boundary_rounds;
  resume = pending;
  complete();
  const auto final = owner.result();
  std::cout << "cap_runtime\tresumed_complete\tPASS" << std::endl;
  require(final->construction->domain->caps.control().pending_radial_sides() == 0,
          "new current cut did not discharge BOTH-side contraction");
  const auto final_history = owner.continuation();
  const auto &final_metric = final_history->control.fixed_metric_factor();
  const auto &captured_metric = captured->control.fixed_metric_factor();
  if (!final_metric || !captured_metric)
    throw std::runtime_error("boundary retirement lost fixed face metric");
  for (uint32_t i = 0; i < 4; ++i)
    require(import_exact(context.budget, final_history->control.values().view((*final_metric)[i]))
                    .compare(import_exact(context.budget, captured->control.values().view(
                                                              (*captured_metric)[i]))) == 0,
            "boundary retirement changed fixed face metric");
  require(final->construction->key.face_generation > first->construction->key.face_generation,
          "physical continuation did not advance independent face generation");
  owner.retire_assignment();
  invalid(
      [&] {
        owner.begin({1, {0}, {0}, boundary.result(), values, tau, delta, assignment, resume});
      },
      "old assignment admitted after cap retirement");
  // Cancellation observes a genuinely submitted cap task horizon on a fresh owner.
  FaceTrimSession cancelled(resource, cache, limits);
  if (device)
    cancelled.open_device(*device);
  cancelled.begin(
      {1, {0}, {0}, boundary.result(), values, tau, delta, ++assignment, final_history});
  bool submitted = false;
  for (uint32_t i = 0; i < 1000000; ++i) {
    const auto state = cancelled.step(context);
    if (cancelled.stats().surface.cap_facet_tests) {
      submitted = true;
      break;
    }
    require(state != TrimFaceProgress::needs_boundary && state != TrimFaceProgress::complete,
            "cap cancellation fixture completed before submission");
    std::this_thread::yield();
  }
  require(submitted, "cap cancellation had no actual numerical task");
  cancelled.cancel();
  bool stopped = false;
  try {
    (void)cancelled.step(context);
  } catch (const Error &error) {
    stopped = error.code == ErrorCode::cancelled;
  }
  require(stopped, "cancelled cap owner continued");
  cancelled.close();
  owner.close();
  boundary.close();
  if (output) {
    caller::write_curves(*output / "final.bgacrv", *final->boundary, context.budget);
    caller::write_trimmed_face(*output / "final.bgatrm", final, context.budget);
  }
  std::cout << "cap_runtime\tmapped\tPASS\tboundary_rounds\t" << boundary_rounds << "\tfacets\t"
            << final->construction->facets.size() << '\n';
}
} // namespace
int main(int argc, char **argv) {
  try {
    require(argc >= 4 && argc <= 7,
            "usage: cap_runtime ordinary mapped threads [module device] [output]");
    const auto threads = static_cast<uint32_t>(std::stoul(argv[3]));
    std::optional<CacheDevice> device;
    if (argc >= 6)
      device = CacheDevice{static_cast<int32_t>(std::stoi(argv[5])), argv[4]};
    std::optional<std::filesystem::path> output;
    if (argc == 5 || argc == 7)
      output = argv[argc - 1];
    run(argv[1], threads, device, false, output);
    run(argv[2], threads, device, true, output);
    return 0;
  } catch (const bgfd::Failure &error) {
    std::cerr << "cap_runtime\texact\t" << static_cast<int>(error.status) << '\t' << error.what()
              << '\n';
    return 1;
  } catch (const std::exception &error) {
    std::cerr << "cap_runtime\t" << error.what() << '\n';
    return 1;
  }
}
