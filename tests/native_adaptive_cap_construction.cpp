#include "../native/mesher/adaptive_cap_domain.hpp"
#include "../native/mesher/adaptive_curves.hpp"
#include "../native/mesher/curve_source_cache.hpp"
#include "../native/mesher/adaptive_regions.hpp"
#include "native_adaptive_cap_work_test.hpp"
#include <iostream>
#include <stdexcept>
#include <thread>

using namespace bgmesh;
using namespace bgmesh::adaptive;
namespace {
void require(bool condition, const char *message) {
  if (!condition)
    throw std::runtime_error(message);
}
void construct(const ProducedCapDomainLease &plan, const ExactTable &quality, ExactRef tau,
               JobContext &context, uint32_t &ordinal) {
  auto resource = plan.resource();
  const auto &cache = plan.source();
  const auto &identity = plan.key();
  const auto key_for_table = [&]() { return ExactTableKey{{OwnerKind::run, 918, 2}, 1, ordinal++}; };
  FaceConstructionKey key;
  key.source_generation = identity.source_generation;
  key.query = identity.query;
  key.boundary_generation = identity.boundary_generation;
  key.chart_revision = identity.chart_revision;
  key.profile_revision = identity.profile_revision;
  key.construction_revision = 1;
  key.definition = identity.definition;
  key.face = identity.face;
  key.assignment_epoch = identity.assignment_epoch;
  key.face_generation = identity.face_generation;
  key.domain_revision = identity.domain_revision;
  const auto prepared = prepare_face_domain({plan.curves(), key, key_for_table(), plan}, context);
  require(prepared.domain && prepared.requests.empty(), "real cap selected source graph did not admit");
  FaceDomainClassifier classifier(resource, prepared.domain, {}, {});
  CacheLimits limits;
  limits.cpu_threads = 1;
  limits.host_bytes = uint64_t{1} << 28;
  SurfaceCacheSession surface(resource, cache, limits);
  std::vector<RootAllowance> allowances;
  for (uint32_t i = 0; i < cache.roots().size(); ++i)
    allowances.push_back({{i}, tau});
  SurfaceFaceIdentity face{identity.definition, identity.face, identity.source_generation,
                            identity.boundary_generation, identity.face_generation,
                            identity.chart_revision, identity.assignment_epoch, identity.domain_revision};
  surface.begin({identity.query, face, cache, quality, allowances, prepared.domain});
  bool done = false;
  for (uint32_t step = 0; step < 1000000; ++step) {
    const auto progress = surface.step(context);
    if (progress == SurfaceProgress::needs_classification)
      surface.classify(classifier.classify(surface.candidates()));
    else if (progress == SurfaceProgress::selected) {
      done = true;
      break;
    }
  }
  require(done, "real quotient surface cover did not settle");
  auto view = surface.selection();
  require(!view.mapped_cells().empty(), "mapped source material was lost as outside");
  std::vector<SurfacePointInput> samples;
  std::vector<uint32_t> sample_plans;
  for (uint32_t i = 0; i < plan.points().size(); ++i)
    if (std::holds_alternative<SurfaceCapPoint>(plan.points()[i].authority)) {
      sample_plans.push_back(i);
      samples.push_back({plan.points()[i].uv});
    }
  SurfacePointResult cut_points;
  if (!samples.empty()) {
    surface.sample({view, plan.values(), 1, samples});
    done = false;
    for (uint32_t step = 0; step < 1000000; ++step) {
      const auto progress = surface.step(context);
      if (progress == SurfaceProgress::needs_classification)
        surface.classify(classifier.classify(surface.candidates()));
      else if (progress == SurfaceProgress::points_complete) {
        done = true;
        break;
      }
    }
    require(done, "SAME PointPool cut sample horizon did not settle");
    cut_points = surface.point_result();
    view = surface.selection();
    surface.release_operation();
  }
  const auto enabled = packet_leaf_slots({}, view, context.budget);
  const auto packets = build_packet_mesh(context.budget, resource, view.selection(), enabled);
  RegionInput input{prepared.domain, view, packets, key, key_for_table(), {}, {}, {}, {}, {},
                    cut_points, sample_plans};
  const auto construction = build_face_regions(input, context);
  require(construction->source_bindings.size() == plan.curves()->sites.size() &&
              construction->cap_point_bindings.size() == plan.points().size(),
          "cap construction dropped original use occurrences or point provenance");
  uint32_t mapped = 0;
  for (const auto &facet : construction->facets)
    if (facet.mapped) {
      ++mapped;
      require(facet.key.generator == FacetGenerator::mapped_cap,
              "cap facet masquerades as original UV triangle");
      const auto &cap = plan.caps()[facet.mapped->cap];
      const auto pole = facet.sites[facet.mapped->source_role_to_facet[0]];
      require(std::get<BoundarySiteSource>(construction->sites[pole].source).boundary_node ==
                  cap.pole_boundary_node,
              "mapped cap moved original vertex authority");
    }
  require(mapped == plan.pieces().size(), "mapped fan triangle count is not cut intervals");
  for (uint32_t cap_id = 0; cap_id < plan.caps().size(); ++cap_id) {
    const auto &cap = plan.caps()[cap_id];
    const auto &first = plan.pieces()[cap.pieces.first];
    const auto &last = plan.pieces()[cap.pieces.first + cap.pieces.count - 1];
    for (uint32_t side = 0; side < 2; ++side) {
      const auto cut = construction->cap_point_bindings[
          side ? last.cut_points[1] : first.cut_points[0]].construction_site;
      uint32_t found = 0;
      for (const auto &edge : construction->edges)
        if (const auto *source_interval = std::get_if<BoundaryKey>(&edge.provenance);
            source_interval && *source_interval == cap.meridian_intervals[cap.angular_to_sector[side]] &&
            (edge.sites[0] == cut || edge.sites[1] == cut)) {
          const auto other = edge.sites[0] == cut ? edge.sites[1] : edge.sites[0];
          const auto *source = std::get_if<BoundarySiteSource>(&construction->sites[other].source);
          if (source && source->boundary_node == cap.pole_boundary_node)
            ++found;
        }
      require(found == 1, "mapped source meridian spoke lost exact current interval identity");
    }
  }

  const auto work = check_captured_cap_work(context, view, construction, prepared.domain,
                                           {quality, tau}, key_for_table());
  require(work.inputs.size() == mapped, "source-backed cap work omitted an actual fan triangle");
  std::cout << "constructed\t" << construction->sites.size() << '\t'
            << construction->facets.size() << '\t' << mapped << '\t'
            << view.mapped_cells().size() << '\n';
  surface.close();
}
void ordinary(const char *path) {
  auto resource = std::make_shared<RunResource>(uint64_t{1} << 31);
  JobContext context({uint64_t{1} << 28, uint64_t{1} << 40}, resource.get(), {});
  const auto source = load_source(path, resource);
  require(source.view().faces.size() == 1, "direct cap fixture needs one source face");
  std::vector<CurveRequest> requested;
  for (uint32_t i = 0; i < source.view().curves.size(); ++i)
    requested.push_back({{i}, source.view().curves[i].parameter_range});
  const auto curves = prepare_curve_source_cache(source, requested, resource, {}, {}, context.budget);
  const auto carrier = source.view().carrier_index.find(source.view().faces[0].carrier_id);
  const auto is_cap = source.view().coedges.size() == 3 && source.view().edges.size() == 2;
  const CarrierRequest request{carrier, is_cap ? source.view().carriers[carrier.value].uv_bounds
                                               : std::array<double, 4>{0, 1, 0, 1}};
  const auto cache = prepare_quotient_trim_source_cache(source, std::span(&request, 1), resource,
                                                        {}, {}, context.budget);
  CurveLimits limits;
  limits.cpu_threads = 1;
  limits.host_bytes = uint64_t{1} << 28;
  AdaptiveBoundarySession boundary(resource, source, {0}, curves, cache, limits);
  ExactBuilder quality(resource, {{OwnerKind::run, 918, 1}, 1, 1});
  const auto tau = quality.append(Rational::from_double(context.budget, .02));
  const auto delta = quality.append(Rational::from_double(context.budget, 1.e-8));
  const auto quality_values = quality.seal();
  boundary.begin({1, quality_values, tau, delta, {}});
  SharedCurveSnapshot snapshot;
  for (uint32_t step = 0; step < 1000000; ++step) {
    if (boundary.step(context) == StepState::done) {
      snapshot = boundary.result();
      break;
    }
    std::this_thread::yield();
  }
  require(static_cast<bool>(snapshot), "original boundary did not complete");
  CapDomainKey key;
  key.source_generation = cache.generation();
  key.query = 1;
  key.definition = {0};
  key.face = {0};
  key.boundary_generation = snapshot->uses.front().boundary_generation;
  key.face_generation = key.boundary_generation;
  key.chart_revision = 1;
  key.profile_revision = cache.profile().revision;
  uint32_t ordinal = 1;
  auto prepared = prepare_cap_domain(
      {cache, snapshot, key, {{OwnerKind::run, 918, 2}, 1, ordinal++}, {}}, context);
  while (!prepared.boundary_requests.empty()) {
    boundary.refine(snapshot, prepared.boundary_requests);
    snapshot.reset();
    for (uint32_t step = 0; step < 1000000; ++step) {
      if (boundary.step(context) == StepState::done) {
        snapshot = boundary.result();
        break;
      }
      std::this_thread::yield();
    }
    require(static_cast<bool>(snapshot), "requested physical cut did not settle");
    key.boundary_generation = snapshot->uses.front().boundary_generation;
    key.face_generation = key.boundary_generation;
    prepared = prepare_cap_domain(
        {cache, snapshot, key, {{OwnerKind::run, 918, 2}, 1, ordinal++}, prepared.continuation}, context);
  }
  require(prepared.plan && prepared.boundary_requests.empty(), "ordinary face has no produced plan");
  const auto initial_key = key;
  if (!is_cap)
    require(prepared.plan.caps().empty() && prepared.plan.pieces().empty(),
            "ordinary face acquired fabricated cap coverage");
  else {
    require(snapshot->uses.size() == 3 && snapshot->seam_wedges.size() == 1 &&
                prepared.plan.caps().size() == 1 && !prepared.plan.pieces().empty(),
            "original three-use wedge inventory was not preserved");
    const auto &wedge = snapshot->seam_wedges.front();
    const auto &left = snapshot->sites[wedge.shared_endpoint_sites[0]];
    const auto &right = snapshot->sites[wedge.shared_endpoint_sites[1]];
    require(left.boundary_node == right.boundary_node &&
                import_exact(context.budget, snapshot->values.view(left.chosen_original_uv[0]))
                        .compare(import_exact(context.budget, snapshot->values.view(right.chosen_original_uv[0]))) != 0,
            "pole chart sectors were merged or physical authority duplicated");
    require(prepared.plan.points().size() == prepared.plan.pieces().size() + 1 &&
                std::holds_alternative<PhysicalCapEndpoint>(prepared.plan.points().front().authority) &&
                std::holds_alternative<PhysicalCapEndpoint>(prepared.plan.points().back().authority),
            "cut endpoint authority or angular chain differs");
    for (size_t i = 1; i + 1 < prepared.plan.points().size(); ++i)
      require(std::holds_alternative<SurfaceCapPoint>(prepared.plan.points()[i].authority),
              "interior cut point acquired private physical-edge authority");
    const auto control = continue_cap_domain(
        {prepared.plan, CapCutDirection::retain, {{OwnerKind::run, 918, 2}, 1, ordinal++}, {}, {}}, context);
    require(control.angular_level() == prepared.plan.control().angular_level() &&
                control.revision() == prepared.plan.control().revision() &&
                control.pending_radial_sides() == 0,
            "completed control capture changed a cut");
    const auto angular = continue_cap_domain(
        {prepared.plan, CapCutDirection::angular, {{OwnerKind::run, 918, 2}, 1, ordinal++}, {}, {}}, context);
    ++key.face_generation;
    ++key.domain_revision;
    const auto refined = prepare_cap_domain(
        {cache, snapshot, key, {{OwnerKind::run, 918, 2}, 1, ordinal++}, angular}, context);
    require(refined.plan && refined.boundary_requests.empty() &&
                refined.plan.pieces().size() == 2 * prepared.plan.pieces().size() &&
                refined.plan.key().boundary_generation == prepared.plan.key().boundary_generation,
            "local angular cut changed physical boundary or failed dyadic refinement");
  }
  require(&prepared.plan.source().view() == &cache.view() &&
              prepared.plan.curves() == snapshot && prepared.plan.key() == initial_key,
          "produced empty cap inventory lost source authority");
  if (is_cap)
    construct(prepared.plan, quality_values, tau, context, ordinal);
  boundary.close();
}
} // namespace
int main(int argc, char **argv) {
  try {
    require(argc == 2, "original ordinary source path required");
    ordinary(argv[1]);
    std::cout << "cap_construction\tPASS\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << "cap_construction\t" << error.what() << '\n';
    return 1;
  }
}
