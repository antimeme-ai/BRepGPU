#include "../native/mesher/adaptive_curves.hpp"
#include "../native/mesher/adaptive_region_cdt.hpp"
#include "../native/mesher/adaptive_regions.hpp"
#include "../native/mesher/curve_source_cache.hpp"
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <thread>
#include <vector>
using namespace bgmesh;
using namespace bgmesh::adaptive;
namespace {
void require(bool yes, const char *message) {
  if (!yes)
    throw std::runtime_error(message);
}
void run(const char *name, const std::vector<std::array<int64_t, 2>> &input,
         const std::vector<std::vector<uint32_t>> &loops,
         int64_t denominator = 1) {
  bgfd::Budget budget({uint64_t{1} << 28, uint64_t{1} << 32});
  std::vector<std::array<Rational, 2>> points;
  for (auto p : input)
    points.push_back(
        {Rational(budget, p[0]).divide(Rational(budget, denominator)),
         Rational(budget, p[1]).divide(Rational(budget, denominator))});
  std::vector<RegionCycleView> cycles;
  for (const auto &loop : loops)
    cycles.push_back({loop});
  const std::array<Rational, 4> factor{Rational(budget, 2), Rational(budget, 1),
                                       Rational(budget, 0),
                                       Rational(budget, 1)};
  auto result = construct_region_cdt(budget, points, cycles, factor, 0);
  std::printf("case\t%s\n", name);
  for (uint32_t i = 0; i < input.size(); ++i)
    std::printf("point\t%u\t%lld/%lld\t%lld/%lld\n", i,
                static_cast<long long>(input[i][0]),
                static_cast<long long>(denominator),
                static_cast<long long>(input[i][1]),
                static_cast<long long>(denominator));
  for (const auto &loop : loops) {
    std::printf("loop");
    for (auto i : loop)
      std::printf("\t%u", i);
    std::puts("");
  }
  require(result.triangles.size() == result.components.size(),
          "component rows missing");
  for (size_t i = 0; i < result.triangles.size(); ++i) {
    const auto t = result.triangles[i];
    std::printf("triangle\t%u\t%u\t%u\t%u\n", t[0], t[1], t[2],
                result.components[i]);
  }
}
void print_exact(const Rational &value) {
  const auto numerator = value.numerator().magnitude(),
             denominator = value.denominator().magnitude();
  const auto integer = [](std::span<const uint32_t> words) {
    std::vector<uint32_t> digits(1, 0);
    for (auto word = words.rbegin(); word != words.rend(); ++word) {
      uint64_t carry = *word;
      for (auto &digit : digits) {
        const auto accumulator = (static_cast<uint64_t>(digit) << 32) + carry;
        digit = static_cast<uint32_t>(accumulator % 1000000000);
        carry = accumulator / 1000000000;
      }
      while (carry) {
        digits.push_back(static_cast<uint32_t>(carry % 1000000000));
        carry /= 1000000000;
      }
    }
    std::printf("%u", digits.back());
    for (size_t j = digits.size() - 1; j > 0; --j)
      std::printf("%09u", digits[j - 1]);
  };
  if (value.sign() < 0)
    std::printf("-");
  integer(numerator);
  std::printf("/");
  integer(denominator);
}
void storage_grid() {
  constexpr uint32_t side = 128;
  bgfd::Budget budget({uint64_t{16} << 20, uint64_t{1} << 40});
  std::vector<std::array<Rational, 2>> points;
  points.reserve(side * side + 4);
  const auto append = [&](int64_t x, int64_t y) {
    points.push_back({Rational(budget, x), Rational(budget, y)});
  };
  append(0, 0);
  append(side + 1, 0);
  append(side + 1, side + 1);
  append(0, side + 1);
  for (uint32_t y = 1; y <= side; ++y)
    for (uint32_t x = 1; x <= side; ++x)
      append(x, y);
  const std::array<uint32_t, 4> loop{0, 1, 2, 3};
  const std::array<RegionCycleView, 1> cycles{{{loop}}};
  const std::array<Rational, 4> factor{Rational(budget, 2), Rational(budget, 1),
                                       Rational(budget), Rational(budget, 1)};
  const auto baseline = budget.live();
  {
    const auto result = construct_region_cdt(budget, points, cycles, factor, 0);
    require(result.triangles.size() == 2 * points.size() - 6,
            "storage grid original-site inventory differs");
    std::puts("case\tstorage");
    for (uint32_t i = 0; i < points.size(); ++i) {
      std::printf("point\t%u\t", i);
      print_exact(points[i][0]);
      std::printf("\t");
      print_exact(points[i][1]);
      std::puts("");
    }
    std::puts("loop\t0\t1\t2\t3");
    for (size_t i = 0; i < result.triangles.size(); ++i) {
      const auto triangle = result.triangles[i];
      std::printf("triangle\t%u\t%u\t%u\t%u\n", triangle[0], triangle[1],
                  triangle[2], result.components[i]);
    }
    std::fprintf(stderr,
                 "region_storage sites=%zu boundary=4 triangles=%zu peak=%llu "
                 "work=%llu\n",
                 points.size(), result.triangles.size(),
                 static_cast<unsigned long long>(budget.peak()),
                 static_cast<unsigned long long>(budget.work()));
  }
  require(budget.live() == baseline,
          "storage grid retained completed bridge scratch");
  bgfd::Budget tiny({2048, uint64_t{1} << 24});
  std::vector<std::array<Rational, 2>> tiny_points;
  for (const auto &point :
       std::array<std::array<int64_t, 2>, 4>{{{0, 0}, {1, 0}, {1, 1}, {0, 1}}})
    tiny_points.push_back({Rational(tiny, point[0]), Rational(tiny, point[1])});
  const std::array<Rational, 4> tiny_factor{Rational(tiny, 1), Rational(tiny),
                                            Rational(tiny), Rational(tiny, 1)};
  const auto tiny_baseline = tiny.live();
  bool refused = false;
  try {
    static_cast<void>(
        construct_region_cdt(tiny, tiny_points, cycles, tiny_factor, 0));
  } catch (const bgfd::Failure &error) {
    refused = error.status == BGFD_RESOURCE_EXHAUSTED;
  }
  require(refused && tiny.live() == tiny_baseline,
          "tiny region cap was ignored or exception retained bridge scratch");
}
void domain_recovery(const char *path, uint32_t initial_refinements) {
  auto resource = std::make_shared<RunResource>(uint64_t{1} << 31);
  JobContext context({uint64_t{1} << 28, uint64_t{1} << 40}, resource.get(),
                     {});
  auto &budget = context.budget;
  const auto source = load_source(path, resource);
  const auto &original = source.view();
  require(original.faces.size() == 1,
          "recovery expects one original source face");
  std::vector<CurveRequest> requests;
  for (uint32_t i = 0; i < original.curves.size(); ++i)
    requests.push_back({{i}, original.curves[i].parameter_range});
  const auto curves =
      prepare_curve_source_cache(source, requests, resource, {}, {}, budget);
  CurveLimits limits;
  limits.cpu_threads = 1;
  AdaptiveBoundarySession boundary(resource, source, {0}, curves, limits);
  ExactBuilder allowances(resource, {{OwnerKind::run, 811, 1}, 1, 1});
  const auto tau = allowances.append(Rational::from_double(budget, .005));
  const auto delta = allowances.append(Rational::from_double(budget, 1.e-8));
  boundary.begin({1, allowances.seal(), tau, delta, {}});
  uint32_t rounds = 0;
  for (; rounds < 128; ++rounds) {
    while (boundary.step(context) != StepState::done)
      std::this_thread::yield();
    const auto snapshot = boundary.result();
    if (rounds < initial_refinements) {
      std::vector<BoundaryKey> all;
      for (const auto &interval : snapshot->intervals)
        all.push_back(interval.boundary_key);
      std::sort(all.begin(), all.end());
      all.erase(std::unique(all.begin(), all.end()), all.end());
      boundary.refine(snapshot, all);
      continue;
    }
    const FaceConstructionKey key{
        1, 1, snapshot->uses.front().boundary_generation, 1, 1, 1, {0}, {0}};
    const auto prepared = prepare_face_domain(
        {snapshot, key, {{OwnerKind::face, 0, 901}, 1, 100 + rounds}}, context);
    if (!prepared.domain) {
      require(prepared.requests.size() == 1,
              "domain recovery requested unrelated whole-face keys");
      require(prepared.requests.front().owner != 402,
              "domain recovery refined the unrelated closing line");
      std::fprintf(
          stderr, "domain_request round=%u edge=%llu\n", rounds,
          static_cast<unsigned long long>(prepared.requests.front().owner));
      boundary.refine(snapshot, prepared.requests);
      continue;
    }
    require(prepared.domain->uses.size() == original.coedges.size() &&
                prepared.domain->source_bindings.size() ==
                    snapshot->sites.size(),
            "recovery lost original use/site inventory");
    const auto &domain = *prepared.domain;
    std::vector<std::array<Rational, 2>> points;
    std::vector<std::vector<uint32_t>> loops;
    for (const auto &site : domain.sites)
      points.push_back({import_exact(budget, domain.values.view(site.uv[0])),
                        import_exact(budget, domain.values.view(site.uv[1]))});
    std::vector<RegionCycleView> cycles;
    for (const auto &loop : domain.loops) {
      loops.emplace_back();
      for (uint64_t j = 0; j < loop.edges.count; ++j)
        loops.back().push_back(domain.edges[loop.edges.first + j].sites[0]);
    }
    for (const auto &loop : loops)
      cycles.push_back({loop});
    const std::array<Rational, 4> factor{Rational(budget, 1), Rational(budget),
                                         Rational(budget), Rational(budget, 1)};
    const auto mesh = construct_region_cdt(budget, points, cycles, factor, 0);
    std::puts("case\trecovery");
    for (uint32_t i = 0; i < points.size(); ++i) {
      std::printf("point\t%u\t", i);
      print_exact(points[i][0]);
      std::printf("\t");
      print_exact(points[i][1]);
      std::puts("");
    }
    for (const auto &loop : loops) {
      std::printf("loop");
      for (auto site : loop)
        std::printf("\t%u", site);
      std::puts("");
    }
    for (size_t i = 0; i < mesh.triangles.size(); ++i) {
      const auto triangle = mesh.triangles[i];
      std::printf("triangle\t%u\t%u\t%u\t%u\n", triangle[0], triangle[1],
                  triangle[2], mesh.components[i]);
    }
    std::fprintf(stderr, "domain_recovery rounds=%u sites=%zu PASS\n", rounds,
                 points.size());
    return;
  }
  throw std::runtime_error("finite domain recovery horizon exhausted");
}
void whole_face(const char *path) {
  auto resource = std::make_shared<RunResource>(uint64_t{1} << 31);
  JobContext context({uint64_t{1} << 28, uint64_t{1} << 40}, resource.get(),
                     {});
  auto &budget = context.budget;
  const auto source = load_source(path, resource);
  const auto &original = source.view();
  require(original.faces.size() == 1,
          "direct face fixture expects one source face");
  std::vector<CurveRequest> curve_requests;
  for (uint32_t i = 0; i < original.curves.size(); ++i)
    curve_requests.push_back({{i}, original.curves[i].parameter_range});
  const auto curves = prepare_curve_source_cache(source, curve_requests,
                                                 resource, {}, {}, budget);
  CurveLimits curve_limits;
  curve_limits.host_bytes = uint64_t{1} << 28;
  curve_limits.cpu_threads = 1;
  AdaptiveBoundarySession boundary(resource, source, {0}, curves, curve_limits);
  ExactBuilder allowance(resource, {{OwnerKind::run, 812, 1}, 1, 1});
  const auto tau = allowance.append(Rational::from_double(budget, .02)),
             delta = allowance.append(Rational::from_double(budget, 1.e-8));
  const auto values = allowance.seal();
  boundary.begin({1, values, tau, delta, {}});
  SharedCurveSnapshot snapshot;
  for (uint32_t step = 0; step < 1000000; ++step) {
    if (boundary.step(context) == StepState::done) {
      snapshot = boundary.result();
      break;
    }
    std::this_thread::yield();
  }
  require(static_cast<bool>(snapshot),
          "direct source boundary did not complete");
  const auto carrier =
      original.carrier_index.find(original.faces[0].carrier_id);
  const CarrierRequest requested{carrier, {0, 1, 0, 1}};
  const auto cache = prepare_trim_source_cache(source, std::span(&requested, 1),
                                               resource, {}, {}, budget);
  FaceConstructionKey key{cache.generation(),
                          1,
                          snapshot->uses.front().boundary_generation,
                          1,
                          cache.profile().revision,
                          1,
                          {0},
                          {0}};
  const auto prepared = prepare_face_domain(
      {snapshot, key, {{OwnerKind::face, 0, 900}, 1, 1}}, context);
  require(prepared.domain && prepared.requests.empty(),
          "literal source graph did not admit a chosen domain");
  FaceDomainClassifier classifier(resource, prepared.domain, {}, {});
  {
    ExactBuilder point_values(resource, {{OwnerKind::face, 0, 900}, 1, 5});
    const auto p = [&](double x, double y) {
      return std::array<ExactRef, 2>{
          point_values.append(Rational::from_double(budget, x)),
          point_values.append(Rational::from_double(budget, y))};
    };
    const std::array<std::array<ExactRef, 2>, 3> probes{p(.25, .25), p(-1., .5),
                                                        p(0., .5)};
    const auto point_classes =
        classifier.classify_points(point_values.seal(), probes);
    require(point_classes == std::pmr::vector<SurfaceClass>(
                                 {SurfaceClass::interior, SurfaceClass::outside,
                                  SurfaceClass::band},
                                 resource.get()),
            "retained exact point classifier lost strict/contact distinction");
  }
  CacheLimits limits;
  limits.cpu_threads = 1;
  limits.host_bytes = uint64_t{1} << 28;
  SurfaceCacheSession surface(resource, cache, limits);
  std::vector<RootAllowance> root_allowances;
  for (uint32_t i = 0; i < cache.roots().size(); ++i)
    root_allowances.push_back({{i}, tau});
  surface.begin({1,
                 {{0},
                  {0},
                  cache.generation(),
                  key.boundary_generation,
                  effective_face_generation(key),
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
  require(selected, "real classified surface view did not complete");
  const auto view = surface.selection();
  std::vector<uint32_t> enabled;
  const auto selection = view.selection();
  // Reuse the exact source classifier on the admitted selected boxes; no corner
  // mask.
  auto boxes = std::allocate_shared<CandidateBoxesData>(
      RunAllocator<CandidateBoxesData>(resource), resource, cache,
      selection->values, view.key(), 999);
  boxes->path_words = selection->path_words;
  for (const auto &leaf : selection->leaves)
    boxes->boxes.push_back({leaf.key, leaf.box});
  const auto classified = classifier.classify(boxes);
  for (uint32_t i = 0; i < classified.labels.size(); ++i)
    if (classified.labels[i] == SurfaceClass::interior)
      enabled.push_back(i);
  const auto packets = build_packet_mesh(budget, resource, selection, enabled);
  require(!packets->triangles.empty() && !packets->rims.empty(),
          "direct face did not exercise packets plus connected band");
  const auto initial = build_face_regions({prepared.domain,
                                           view,
                                           packets,
                                           key,
                                           {{OwnerKind::face, 0, 900}, 1, 2},
                                           {},
                                           {},
                                           {},
                                           {},
                                           {}},
                                          context);
  require(initial->changed_facets.size() == initial->facets.size(),
          "first construction omitted a required facet bound");
  auto next_key = key;
  ++next_key.construction_revision;
  const auto result = rebuild_face_regions({prepared.domain,
                                            view,
                                            packets,
                                            next_key,
                                            {{OwnerKind::face, 0, 900}, 1, 3},
                                            {},
                                            {},
                                            {},
                                            initial,
                                            {}},
                                           context);
  require(result->changed_facets.empty(),
          "unchanged packet/band circuit was needlessly rebound");
  require(result->facets.size() == initial->facets.size(),
          "unchanged local rebuild changed triangle inventory");
  for (uint32_t i = 0; i < result->facets.size(); ++i)
    require(result->facets[i].key == initial->facets[i].key,
            "unchanged generator lost persistent facet identity");
  const auto footprint = freeze_packet_footprint(packets, view, budget);
  const auto stable =
      apply_footprint(footprint, classifier.classify(boxes), budget);
  require(stable.labels == classified.labels,
          "initial footprint changed exact source labels");
  const auto old_facet = std::find_if(
      result->facets.begin(), result->facets.end(), [](const auto &facet) {
        return facet.key.generator == FacetGenerator::packet;
      });
  require(old_facet != result->facets.end(),
          "real packet target has no packet triangle");
  PacketGridTarget target;
  target.old_facet = old_facet->key;
  target.surface = view.key();
  target.leaf = old_facet->leaf;
  target.facet = static_cast<uint32_t>(old_facet - result->facets.begin());
  target.root = selection->leaves[target.leaf].key.root;
  std::array<Rational, 2> target_uv{Rational(budget), Rational(budget)};
  for (uint32_t axis = 0; axis < 2; ++axis) {
    for (uint32_t j = 0; j < 3; ++j)
      target_uv[axis] = target_uv[axis].add(
          import_exact(budget, result->values.view(
                                   result->sites[old_facet->sites[j]].uv[axis]))
              .multiply(Rational(budget, j == 2 ? 2 : 1)));
    target_uv[axis] = target_uv[axis].divide(Rational(budget, 4));
    const auto &root = cache.roots()[target.root.value];
    const auto low =
        Rational::from_double(budget, root.prepared_box[2 * axis].lower);
    const auto width =
        Rational::from_double(budget, root.prepared_box[2 * axis + 1].upper)
            .subtract(low);
    const auto normalized = target_uv[axis].subtract(low).divide(width);
    const auto numerator = normalized.numerator().magnitude();
    require(numerator.size() <= 2, "test target numerator exceeds uint64");
    target.numerator[axis] =
        (numerator.empty() ? 0 : numerator[0]) |
        (numerator.size() > 1 ? static_cast<uint64_t>(numerator[1]) << 32 : 0);
    const auto exponent = normalized.denominator().bit_length() - 1;
    require(exponent <= 64, "test target exceeds depth profile");
    require(normalized.denominator().compare(
                Rational(budget, 1).numerator().shift_left(exponent)) == 0,
            "test target is not a prepared-root dyadic");
    target.depth[axis] = static_cast<uint32_t>(exponent);
  }
  surface.refine({view, {}, std::span(&target, 1), {}, result});
  selected = false;
  for (uint32_t step = 0; step < 1000000; ++step) {
    const auto progress = surface.step(context);
    if (progress == SurfaceProgress::needs_classification)
      surface.classify(apply_footprint(
          footprint, classifier.classify(surface.candidates()), budget));
    else if (progress == SurfaceProgress::selected) {
      selected = true;
      break;
    }
  }
  require(selected,
          "forced packet target did not settle a real selected cover");
  const auto refined_view = surface.selection();
  const auto refined_selection = refined_view.selection();
  bool active_target = false;
  for (const auto &leaf : refined_selection->leaves)
    for (auto corner : leaf.corners) {
      const auto &point = refined_selection->points[corner.value];
      if (import_exact(budget,
                       refined_selection->values.view(point.chart_xy[0]))
                  .compare(target_uv[0]) == 0 &&
          import_exact(budget,
                       refined_selection->values.view(point.chart_xy[1]))
                  .compare(target_uv[1]) == 0)
        active_target = true;
    }
  require(
      active_target,
      "numerical existence was mistaken for active packet target incidence");
  const auto enabled_refined =
      packet_leaf_slots(footprint, refined_view, budget);
  const auto refined_packets =
      build_packet_mesh(budget, resource, refined_selection, enabled_refined);
  require(refined_packets->demoted_leaf_slots.empty(),
          "postfreeze target refinement changed packet footprint");
  ++next_key.construction_revision;
  const auto refined = rebuild_face_regions({prepared.domain,
                                             refined_view,
                                             refined_packets,
                                             next_key,
                                             {{OwnerKind::face, 0, 900}, 1, 4},
                                             {},
                                             {},
                                             {},
                                             result,
                                             {}},
                                            context);
  require(!refined->changed_facets.empty() &&
              refined->changed_facets.size() < refined->facets.size(),
          "target refinement rebound unchanged whole-face facets");
  bool old_triangle = false;
  for (const auto &facet : refined->facets) {
    uint32_t common = 0;
    for (auto site : facet.sites) {
      const auto x = import_exact(
          budget, refined->values.view(refined->sites[site].uv[0]));
      const auto y = import_exact(
          budget, refined->values.view(refined->sites[site].uv[1]));
      for (auto old_site : old_facet->sites)
        if (x.compare(import_exact(
                budget, result->values.view(result->sites[old_site].uv[0]))) ==
                0 &&
            y.compare(import_exact(
                budget, result->values.view(result->sites[old_site].uv[1]))) ==
                0) {
          ++common;
          break;
        }
    }
    if (common == 3)
      old_triangle = true;
  }
  require(!old_triangle,
          "old packet triangle survived an active strict-interior target");
  require(result->source_bindings.size() == snapshot->sites.size(),
          "complete original occurrence inventory was lost");
  require(!result->components.empty(), "connected band inventory is empty");
  ExactBuilder bound_values(resource, {{OwnerKind::face, 0, 900}, 1, 6});
  const auto tight = bound_values.append(Rational::from_double(budget, 1.e-4));
  const auto quality = bound_values.seal();
  std::vector<uint32_t> band_facets;
  for (uint32_t i = 0; i < refined->facets.size(); ++i)
    if (refined->facets[i].key.generator == FacetGenerator::band)
      band_facets.push_back(i);
  surface.bound({refined_view,
                 refined,
                 prepared.domain,
                 {quality, tight},
                 band_facets,
                 17,
                 {}});
  bool bounded = false;
  for (uint32_t step = 0; step < 1000000; ++step) {
    if (surface.step(context) == SurfaceProgress::facets_complete) {
      bounded = true;
      break;
    }
    std::this_thread::yield();
  }
  require(bounded, "actual band facet numerical horizon did not complete");
  const auto actual_bounds = surface.facet_result();
  require(actual_bounds->point_floors.empty(),
          "source saddle unexpectedly hit a point-construction floor");
  require(std::any_of(actual_bounds->facets.begin(),
                      actual_bounds->facets.end(),
                      [](const auto &answer) { return !answer.accepted; }),
          "authored curved band did not exercise actual failed-bound progress");
  const auto requests = plan_construction_refinement(
      {refined, actual_bounds, {{OwnerKind::face, 0, 900}, 1, 7}, 1, 1024},
      context);
  require(!requests->band_anchors.empty() && !requests->splits.empty() &&
              requests->packet_targets.empty(),
          "failed band omitted its strict anchor/cache construction change");
  for (const auto &anchor : requests->band_anchors) {
    const auto old = std::find_if(
        refined->facets.begin(), refined->facets.end(),
        [&](const auto &facet) { return facet.key == anchor.old_facet; });
    require(old != refined->facets.end(),
            "central anchor lost actual parent facet");
    std::array<std::array<Rational, 2>, 3> uv{
        {{import_exact(budget, refined->values.view(
                                   refined->sites[old->sites[0]].uv[0])),
          import_exact(budget, refined->values.view(
                                   refined->sites[old->sites[0]].uv[1]))},
         {import_exact(budget, refined->values.view(
                                   refined->sites[old->sites[1]].uv[0])),
          import_exact(budget, refined->values.view(
                                   refined->sites[old->sites[1]].uv[1]))},
         {import_exact(budget, refined->values.view(
                                   refined->sites[old->sites[2]].uv[0])),
          import_exact(budget, refined->values.view(
                                   refined->sites[old->sites[2]].uv[1]))}}};
    const std::array<Rational, 2> point{
        import_exact(budget, requests->values.view(anchor.uv[0])),
        import_exact(budget, requests->values.view(anchor.uv[1]))};
    const auto oriented = [](const auto &a, const auto &b, const auto &c) {
      return b[0]
          .subtract(a[0])
          .multiply(c[1].subtract(a[1]))
          .subtract(b[1].subtract(a[1]).multiply(c[0].subtract(a[0])));
    };
    const auto area = oriented(uv[0], uv[1], uv[2]);
    for (uint32_t j = 0; j < 3; ++j)
      require(oriented(uv[j], uv[(j + 1) % 3], point)
                      .multiply(Rational(budget, 6))
                      .compare(area) >= 0,
              "band anchor hugs a clipped-piece corner instead of its actual "
              "parent center");
  }
  surface.release_operation();
  surface.refine({refined_view, requests->splits, {}, {}, refined});
  selected = false;
  for (uint32_t step = 0; step < 1000000; ++step) {
    const auto progress = surface.step(context);
    if (progress == SurfaceProgress::needs_classification)
      surface.classify(apply_footprint(
          footprint, classifier.classify(surface.candidates()), budget));
    else if (progress == SurfaceProgress::selected) {
      selected = true;
      break;
    }
  }
  require(selected, "band source splits failed to settle");
  auto band_view = surface.selection();
  std::vector<SurfacePointInput> anchor_inputs;
  std::vector<uint64_t> births, components;
  for (const auto &anchor : requests->band_anchors) {
    anchor_inputs.push_back({anchor.uv, ArithmeticStage::rn64});
    births.push_back(anchor.birth_horizon);
    components.push_back(anchor.component);
  }
  surface.sample({band_view, requests->values, 18, anchor_inputs});
  bool sampled = false;
  for (uint32_t step = 0; step < 1000000; ++step) {
    const auto progress = surface.step(context);
    if (progress == SurfaceProgress::needs_classification)
      surface.classify(apply_footprint(
          footprint, classifier.classify(surface.candidates()), budget));
    else if (progress == SurfaceProgress::points_complete) {
      sampled = true;
      break;
    }
  }
  require(sampled, "actual original carrier anchor sampling failed");
  const auto sampled_points = surface.point_result();
  band_view = surface.selection();
  surface.release_operation();
  const auto band_enabled = packet_leaf_slots(footprint, band_view, budget);
  const auto band_packets =
      build_packet_mesh(budget, resource, band_view.selection(), band_enabled);
  require(band_packets->demoted_leaf_slots.empty(),
          "band refinement changed frozen packet union");
  ++next_key.construction_revision;
  const auto anchored = rebuild_face_regions({prepared.domain,
                                              band_view,
                                              band_packets,
                                              next_key,
                                              {{OwnerKind::face, 0, 900}, 1, 8},
                                              sampled_points,
                                              births,
                                              components,
                                              refined,
                                              requests->changed_components},
                                             context);
  require(anchored->anchor_components ==
              std::pmr::vector<uint64_t>(components.begin(), components.end(),
                                         resource.get()),
          "local rebuild lost failed-facet anchor component identity");
  const auto &emitted = anchored;
  std::puts("case\tface");
  for (uint32_t i = 0; i < emitted->sites.size(); ++i) {
    std::printf("point\t%u\t", i);
    print_exact(
        import_exact(budget, emitted->values.view(emitted->sites[i].uv[0])));
    std::printf("\t");
    print_exact(
        import_exact(budget, emitted->values.view(emitted->sites[i].uv[1])));
    std::puts("");
  }
  for (const auto &loop : prepared.domain->loops) {
    std::printf("loop");
    for (uint64_t j = 0; j < loop.edges.count; ++j)
      std::printf("\t%u",
                  prepared.domain->edges[loop.edges.first + j].sites[0]);
    std::puts("");
  }
  for (const auto &triangle : emitted->facets)
    std::printf("triangle\t%u\t%u\t%u\t0\n", triangle.sites[0],
                triangle.sites[1], triangle.sites[2]);
  surface.close();
  boundary.close();
}
} // namespace
int main(int argc, char **argv) {
  try {
    if ((argc == 3 || argc == 4) &&
        std::strcmp(argv[1], "--domain-recovery") == 0) {
      const auto initial =
          argc == 4 ? static_cast<uint32_t>(std::stoul(argv[3])) : 0;
      require(initial <= 3, "recovery initial schedule cap");
      domain_recovery(argv[2], initial);
      return 0;
    }

    if (argc == 2 && std::strcmp(argv[1], "--storage-only") == 0) {
      storage_grid();
      return 0;
    }
    run("rational", {{0, 0}, {8, 0}, {8, 8}, {0, 8}, {3, 4}}, {{0, 1, 2, 3}},
        3);
    run("anchor", {{0, 0}, {8, 0}, {8, 8}, {0, 8}, {3, 4}}, {{0, 1, 2, 3}});
    run("disconnected",
        {{0, 0},
         {4, 0},
         {4, 4},
         {0, 4},
         {8, 0},
         {12, 0},
         {12, 4},
         {8, 4},
         {2, 2},
         {10, 2}},
        {{0, 1, 2, 3}, {4, 5, 6, 7}});
    run("nested",
        {{0, 0},
         {20, 0},
         {20, 20},
         {0, 20},
         {2, 2},
         {2, 18},
         {18, 18},
         {18, 2},
         {5, 5},
         {15, 5},
         {15, 15},
         {5, 15},
         {7, 7},
         {7, 13},
         {13, 13},
         {13, 7},
         {1, 10},
         {6, 10}},
        {{0, 1, 2, 3}, {4, 5, 6, 7}, {8, 9, 10, 11}, {12, 13, 14, 15}});
    for (const auto bad :
         std::array<std::array<int64_t, 2>, 3>{{{-1, 2}, {4, 0}, {0, 0}}}) {
      bgfd::Budget budget({uint64_t{1} << 24, uint64_t{1} << 28});
      std::vector<std::array<Rational, 2>> points;
      for (auto p : std::vector<std::array<int64_t, 2>>{
               {0, 0}, {8, 0}, {8, 8}, {0, 8}, bad})
        points.push_back({Rational(budget, p[0]), Rational(budget, p[1])});
      const std::array<uint32_t, 4> loop{0, 1, 2, 3};
      const std::array<RegionCycleView, 1> cycles{{{loop}}};
      const std::array<Rational, 4> factor{
          Rational(budget, 1), Rational(budget, 0), Rational(budget, 0),
          Rational(budget, 1)};
      bool rejected = false;
      try {
        static_cast<void>(
            construct_region_cdt(budget, points, cycles, factor, 0));
      } catch (const bgfd::Failure &error) {
        rejected = error.status == BGFD_INVALID_DOMAIN;
      }
      require(rejected, "outside/on-edge/duplicate interior anchor accepted");
    }
    {
      bgfd::Budget budget({uint64_t{1} << 24, uint64_t{1} << 28});
      std::vector<std::array<Rational, 2>> points;
      for (const auto p : std::array<std::array<int64_t, 2>, 7>{
               {{0, 0}, {8, 0}, {8, 8}, {0, 8}, {-1, 2}, {4, 0}, {4, 4}}})
        points.push_back({Rational(budget, p[0]), Rational(budget, p[1])});
      const std::array<uint32_t, 4> loop{0, 1, 2, 3};
      const std::array<RegionCycleView, 1> cycles{{{loop}}};
      const std::array<Rational, 4> factor{Rational(budget, 1),
                                           Rational(budget), Rational(budget),
                                           Rational(budget, 1)};
      const auto filtered =
          construct_region_cdt(budget, points, cycles, factor, 0, true);
      std::array<bool, 7> used{};
      for (const auto &triangle : filtered.triangles)
        for (auto site : triangle)
          used[site] = true;
      require(used == std::array<bool, 7>{true, true, true, true, false, false,
                                          true},
              "restart anchor filtering changed material or retained "
              "nonmaterial samples");
    }
    require(argc == 1 || argc == 2, "usage: regions [source]");
    if (argc == 2)
      whole_face(argv[1]);
    return 0;
  } catch (const bgfd::Failure &e) {
    std::fprintf(stderr, "%s\n", e.what());
    return 1;
  } catch (const std::exception &e) {
    std::fprintf(stderr, "%s\n", e.what());
    return 1;
  }
}
