#include "adaptive_trimmed.hpp"
#include "adaptive_regions.hpp"
#include "adaptive_trace.hpp"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <numeric>

namespace bgmesh::adaptive {
namespace {
using Clock = std::chrono::steady_clock;
std::atomic<uint64_t> next_face_session{1};
void require(bool yes, const char *message) {
  if (!yes)
    throw Error(ErrorCode::invalid_source, message);
}
double elapsed(Clock::time_point begin) {
  return std::chrono::duration<double>(Clock::now() - begin).count();
}
uint32_t count32(std::size_t count) {
  if (count > UINT32_MAX)
    throw Error(ErrorCode::resource_exhausted, "trimmed-face index capacity");
  return static_cast<uint32_t>(count);
}
} // namespace
struct FaceTrimSession::Impl final {
  enum class Phase {
    idle,
    domain,
    start,
    select,
    anchors,
    cuts,
    construct,
    bound,
    cap_bound,
    promotions,
    boundary,
    complete,
    closed
  };
  std::shared_ptr<RunResource> resource;
  SourceCache source;
  TrimFaceLimits limits;
  std::stop_source cancel_source;
  struct ForwardStop final {
    std::stop_source *source;
    void operator()() const noexcept { source->request_stop(); }
  };
  std::optional<std::stop_callback<ForwardStop>> external_cancel;
  std::stop_token stop;
  SurfaceCacheSession surface;
  std::optional<TrimFaceQuery> query;
  Phase phase = Phase::idle;
  std::atomic<bool> cancelled{false};
  uint64_t session, last_query = 0, operation = 0, horizon = 0;
  enum class AssignmentMode { unset, legacy, pooled };
  AssignmentMode assignment_mode = AssignmentMode::unset;
  Phase trace_phase = Phase::idle;
  uint64_t trace_span = 0, trace_failed = 0;
  uint64_t last_assignment = 0;
  uint64_t construction_work_used = 0;
  uint32_t table_ordinal = 0;
  FaceConstructionKey key;
  FaceDomainLease domain;
  std::shared_ptr<FaceDomainClassifier> classifier;
  SurfaceBoundView view;
  PacketFootprint footprint;
  FaceConstructionResult construction;
  SurfaceFacetResult bounds;
  SurfaceCapFacetResult cap_bounds;
  CapCutControlLease cap_control;
  std::optional<FaceConstructionKey> replacement_key;
  FaceContinuation resume_output;
  ConstructionRefinement refinement;
  FaceTrimResult output;
  std::optional<ExactTable> anchor_values, promotion_values;
  std::pmr::vector<SurfacePointInput> anchors, promotions;
  std::pmr::vector<uint64_t> births, anchor_components;
  std::pmr::vector<BoundaryKey> boundary_wanted;
  std::pmr::vector<uint32_t> pending_facets, ordinary_facets, cap_facets, cut_plan_indices;
  SurfacePointResult sampled_cuts;
  bool refresh_cuts_after_anchors = false;
  SurfacePointResult sampled_anchors;
  bool surface_started = false;
  bool recheck_failed = false;
  bool trace_open = false;
  Clock::time_point operation_started{};
  TrimFaceStats counts;

  Impl(std::shared_ptr<RunResource> owner, SourceCache cache, TrimFaceLimits selected,
       const std::stop_token &token)
      : resource(std::move(owner)), source(std::move(cache)), limits(selected),
        stop(cancel_source.get_token()), surface(resource, source, limits.surface, stop),
        session(next_face_session.fetch_add(1, std::memory_order_relaxed)), anchors(resource.get()),
        promotions(resource.get()), births(resource.get()), anchor_components(resource.get()),
        boundary_wanted(resource.get()), pending_facets(resource.get()),
        ordinary_facets(resource.get()), cap_facets(resource.get()),
        cut_plan_indices(resource.get()) {
    require(session && source.source().view().resource == resource && limits.regions &&
                limits.anchors && limits.exact_bits && limits.construction_work,
            "invalid trimmed-face source owner or limits");
    external_cancel.emplace(token, ForwardStop{&cancel_source});
  }
  static const char *phase_name(Phase value) noexcept {
    constexpr const char *names[]{"idle",     "domain",    "start", "select",    "anchors",
                                  "cuts",     "construct", "bound", "cap_bound", "promotions",
                                  "boundary", "complete",  "closed"};
    return names[static_cast<uint32_t>(value)];
  }
  diagnostic::Context trace_context() const noexcept {
    diagnostic::Context result;
    if (query) {
      result.face = source.source().view().faces[query->face.value].id.value;
      result.query = query->generation;
      result.assignment = query->assignment;
    }
    result.span = trace_span;
    result.horizon = horizon;
    result.revision = key.construction_revision;
    result.source_sites = domain ? domain->sites.size() : 0;
    result.sites = construction ? construction->sites.size() : 0;
    result.facets = construction ? construction->facets.size() : 0;
    result.pending = pending_facets.size();
    result.failed = trace_failed;
    result.anchors = anchors.size();
    result.leaves = view ? view.selection()->leaves.size() : 0;
    result.cumulative_work = construction_work_used;
    return result;
  }
  void trace_early_failure() noexcept {
    if (!limits.trace_progress || !trace_open)
      return;
    const auto captured = trace_context();
    diagnostic::Scope scope(&captured);
    diagnostic::emit({.event = "phase_failure", .phase = phase_name(trace_phase)});
    trace_open = false;
  }
  void release_face_state() {
    trace_open = false;
    trace_span = trace_failed = 0;
    surface_started = false;
    output.reset();
    bounds.reset();
    cap_bounds.reset();
    cap_control = {};
    replacement_key.reset();
    resume_output.reset();
    sampled_cuts.reset();
    refinement.reset();
    construction.reset();
    recheck_failed = false;
    domain.reset();
    classifier.reset();
    footprint.reset();
    view = {};
    query.reset();
    anchor_values.reset();
    promotion_values.reset();
    sampled_anchors.reset();
    anchors.clear();
    births.clear();
    anchor_components.clear();
    promotions.clear();
    boundary_wanted.clear();
    pending_facets.clear();
    ordinary_facets.clear();
    cap_facets.clear();
    cut_plan_indices.clear();
    phase = Phase::idle;
  }
  void stopped() const {
    if (cancelled.load(std::memory_order_relaxed) || stop.stop_requested())
      throw Error(ErrorCode::cancelled, "trimmed-face operation cancelled");
    require(phase != Phase::closed, "trimmed-face session closed");
  }
  ExactTableKey next_table() {
    if (!query)
      throw Error(ErrorCode::internal_error, "trimmed-face table has no active query");
    if (table_ordinal == UINT32_MAX)
      throw Error(ErrorCode::resource_exhausted, "trimmed-face exact table ordinal exhausted");
    return {{OwnerKind::face, query.value().definition.value, session},
            query.value().generation,
            ++table_ordinal};
  }
  uint64_t next_operation() {
    if (operation == UINT64_MAX)
      throw Error(ErrorCode::resource_exhausted, "trimmed-face request ordinal exhausted");
    return ++operation;
  }
  FaceConstructionKey admit_boundary(const TrimFaceQuery &candidate,
                                     const SharedCurveSnapshot &snapshot,
                                     bgfd::Budget &budget) const {
    require(snapshot && snapshot->boundary && snapshot->resource == resource &&
                &snapshot->source.view() == &source.source().view() &&
                snapshot->definition == candidate.definition &&
                snapshot->query_generation == candidate.generation,
            "foreign or stale trimmed-face shared boundary");
    const auto &s = source.source().view();
    require(candidate.face.value < s.faces.size() &&
                candidate.definition.value < s.definitions.size() &&
                s.faces[candidate.face.value].definition_id ==
                    s.definitions[candidate.definition.value].id,
            "trimmed face does not belong to original definition");
    const auto carrier = s.carrier_index.find(s.faces[candidate.face.value].carrier_id);
    std::array<double, 4> envelope{};
    bool first_root = true;
    for (const auto &root : source.roots()) {
      if (root.source_slot != carrier.value)
        continue;
      if (first_root) {
        for (uint32_t a = 0; a < 4; ++a)
          envelope[a] = root.prepared_box[a].lower;
        first_root = false;
      } else {
        for (uint32_t a = 0; a < 2; ++a) {
          envelope[2 * a] = std::min(envelope[2 * a], root.prepared_box[2 * a].lower);
          envelope[2 * a + 1] = std::max(envelope[2 * a + 1], root.prepared_box[2 * a + 1].upper);
        }
      }
    }
    require(!first_root, "trimmed source cache has no selected face carrier roots");
    const auto tau = import_exact(budget, candidate.values.view(candidate.positional));
    const auto delta = import_exact(budget, candidate.values.view(candidate.node_source));
    require(tau.sign() > 0 && delta.sign() > 0, "nonpositive trimmed-face allowance");
    bool found = false;
    uint64_t boundary_generation = 0;
    for (const auto &use : snapshot->uses) {
      if (use.face != candidate.face)
        continue;
      if (!found) {
        boundary_generation = use.boundary_generation;
        found = true;
      }
      require(use.boundary_generation == boundary_generation &&
                  use.sites.first <= snapshot->sites.size() &&
                  use.sites.count <= snapshot->sites.size() - use.sites.first &&
                  use.intervals.first <= snapshot->intervals.size() &&
                  use.intervals.count <= snapshot->intervals.size() - use.intervals.first,
              "mixed or incomplete trimmed-face boundary generation");
      for (uint64_t i = 0; i < use.sites.count; ++i) {
        budget.charge(1);
        const auto &site = snapshot->sites[use.sites.first + i];
        require(
            import_exact(budget, snapshot->values.view(site.node_source_upper)).compare(delta) <= 0,
            "shared boundary source-node quality does not fit face request");
        for (uint32_t axis = 0; axis < 2; ++axis) {
          const auto uv =
              import_exact(budget, snapshot->values.view(site.chosen_original_uv[axis]));
          if (uv.compare(Rational::from_double(budget, envelope[2 * axis])) < 0 ||
              uv.compare(Rational::from_double(budget, envelope[2 * axis + 1])) > 0)
            throw Error(ErrorCode::unsupported,
                        "chosen trim lies outside original finite UV profile envelope");
        }
      }
      for (uint64_t i = 0; i < use.intervals.count; ++i) {
        budget.charge(1);
        const auto &interval = snapshot->intervals[use.intervals.first + i];
        require(import_exact(budget,
                             snapshot->values.view(interval.carrier_trace_to_actual_chord_upper))
                        .compare(tau) <= 0,
                "shared boundary trace quality does not fit face positional "
                "request");
      }
    }
    require(found, "trimmed face has no complete original use inventory");
    FaceConstructionKey admitted{};
    admitted.source_generation = source.generation();
    admitted.query = candidate.generation;
    admitted.boundary_generation = boundary_generation;
    admitted.chart_revision = 1;
    admitted.profile_revision = source.profile().revision;
    admitted.definition = candidate.definition;
    admitted.face = candidate.face;
    admitted.assignment_epoch = candidate.assignment;
    admitted.face_generation = boundary_generation;
    return admitted;
  }
  uint64_t remaining_work(bgfd::Budget &budget) const {
    require(construction_work_used <= limits.construction_work &&
                budget.work() <= limits.construction_work - construction_work_used,
            "trimmed construction work accounting");
    return limits.construction_work - construction_work_used - budget.work();
  }
  template <class Function> auto classify_metered(bgfd::Budget &budget, Function &&function) {
    const auto before = classifier->stats().work;
    try {
      auto result = function(remaining_work(budget));
      budget.charge(classifier->stats().work - before);
      return result;
    } catch (...) {
      const auto failure = std::current_exception();
      try {
        budget.charge(classifier->stats().work - before);
      } catch (...) {
      } // preserve the computation's original failure
      std::rethrow_exception(failure);
    }
  }
  void classify(bgfd::Budget &budget) {
    const auto tick = Clock::now();
    auto labels = classify_metered(budget, [&](uint64_t allowance) {
      return classifier->classify(surface.candidates(), allowance);
    });
    counts.classifications += labels.labels.size();
    if (footprint)
      labels = apply_footprint(footprint, std::move(labels), budget);
    surface.classify(std::move(labels));
    counts.classification_seconds += elapsed(tick);
  }
  static CapDomainKey cap_key(const FaceConstructionKey &key) {
    return {key.source_generation,   key.query,           key.definition,     key.face,
            key.boundary_generation, key.face_generation, key.chart_revision, key.domain_revision,
            key.assignment_epoch,    key.profile_revision};
  }
  static SurfaceFaceIdentity surface_key(const FaceConstructionKey &key) {
    return {key.definition,      key.face,           key.source_generation, key.boundary_generation,
            key.face_generation, key.chart_revision, key.assignment_epoch,  key.domain_revision};
  }
  static uint64_t next_revision(uint64_t current) {
    if (current == UINT64_MAX)
      throw Error(ErrorCode::resource_exhausted, "face/domain revision exhausted");
    return current + 1;
  }
  std::pmr::vector<RootAllowance> root_allowances() const {
    if (!query)
      throw Error(ErrorCode::internal_error, "surface allowances lack face query");
    std::pmr::vector<RootAllowance> rows(resource.get());
    const auto &s = source.source().view();
    const auto carrier = s.carrier_index.find(s.faces[query->face.value].carrier_id);
    for (uint32_t i = 0; i < source.roots().size(); ++i)
      if (source.roots()[i].source_slot == carrier.value)
        rows.push_back({{i}, query->positional});
    return rows;
  }
  void retain_control(JobContext &context) {
    if (!domain || !domain->caps || domain->caps.caps().empty() ||
        (cap_control && cap_control.fixed_metric_factor()))
      return;
    const auto metric_values = construction ? std::optional(construction->values) : std::nullopt;
    const auto metric_factor =
        construction ? std::optional(construction->fixed_metric_factor) : std::nullopt;
    cap_control = continue_cap_domain(
        {domain->caps, CapCutDirection::retain, next_table(), metric_values, metric_factor},
        context);
  }
  void submit_cuts(bool after_anchors = false) {
    refresh_cuts_after_anchors = after_anchors;
    cut_plan_indices.clear();
    sampled_cuts.reset();
    if (!domain->caps || domain->caps.points().empty()) {
      submit_anchors();
      return;
    }
    std::pmr::vector<SurfacePointInput> points(resource.get());
    for (uint32_t i = 0; i < domain->caps.points().size(); ++i) {
      const auto &point = domain->caps.points()[i];
      if (std::holds_alternative<SurfaceCapPoint>(point.authority)) {
        cut_plan_indices.push_back(i);
        points.push_back({point.uv, ArithmeticStage::rn64});
      }
    }
    if (points.empty()) {
      submit_anchors();
      return;
    }
    operation_started = Clock::now();
    surface.sample({view, domain->caps.values(), next_operation(), points});
    phase = Phase::cuts;
  }
  void submit_bounds() {
    if (!query)
      throw Error(ErrorCode::internal_error, "surface bound lacks face query");
    ordinary_facets.clear();
    cap_facets.clear();
    for (auto index : pending_facets) {
      require(index < construction->facets.size(),
              "whole facet horizon index outside construction");
      if (construction->facets[index].mapped)
        cap_facets.push_back(index);
      else
        ordinary_facets.push_back(index);
    }
    operation_started = Clock::now();
    if (!ordinary_facets.empty()) {
      surface.bound({view,
                     construction,
                     domain,
                     {query->values, query->positional},
                     ordinary_facets,
                     next_operation(),
                     limits.facets});
      phase = Phase::bound;
    } else {
      bounds.reset();
      surface.bound_caps({view,
                          construction,
                          domain,
                          {query->values, query->positional},
                          cap_facets,
                          next_operation(),
                          limits.facets});
      phase = Phase::cap_bound;
    }
    ++counts.facet_horizons;
  }
  TrimFaceProgress finish_horizon(JobContext &context) {
    const auto tau = import_exact(context.budget, query->values.view(query->positional));
    bool all = true, cap_failed = false;
    trace_failed = 0;
    if (!ordinary_facets.empty()) {
      require(bounds && bounds->work.key().construction == construction->key &&
                  bounds->work.key().surface == view.key() &&
                  bounds->facets.size() == ordinary_facets.size(),
              "stale or partial ordinary-facet horizon");
      for (uint32_t i = 0; i < ordinary_facets.size(); ++i) {
        const auto &answer = bounds->facets[i];
        require(answer.facet == ordinary_facets[i], "ordinary facet horizon order differs");
        if (answer.accepted)
          require(answer.positional_upper &&
                      import_exact(context.budget, bounds->values.view(*answer.positional_upper))
                              .compare(tau) <= 0,
                  "accepted ordinary upper exceeds original exact allowance");
        else {
          all = false;
          if (limits.trace_progress)
            ++trace_failed;
        }
      }
    }
    auto angular = Rational(context.budget), radial = Rational(context.budget);
    if (!cap_facets.empty()) {
      require(cap_bounds && cap_bounds->work.key().construction == construction->key &&
                  cap_bounds->work.key().surface == view.key() &&
                  cap_bounds->facets.size() == cap_facets.size(),
              "stale or partial mapped-cap horizon");
      for (uint32_t i = 0; i < cap_facets.size(); ++i) {
        const auto &answer = cap_bounds->facets[i];
        require(answer.facet == cap_facets[i], "mapped facet horizon order differs");
        if (answer.accepted) {
          require(answer.bounds &&
                      import_exact(context.budget, cap_bounds->values.view(answer.bounds->total))
                              .compare(tau) <= 0,
                  "accepted cap total exceeds original exact allowance");
        } else {
          all = false;
          cap_failed = true;
          if (limits.trace_progress)
            ++trace_failed;
          if (!answer.bounds)
            throw Error(ErrorCode::unresolved, "mapped cap numerical bound unresolved");
          angular = angular.add(
              import_exact(context.budget, cap_bounds->values.view(answer.bounds->angular)));
          radial =
              radial
                  .add(import_exact(context.budget, cap_bounds->values.view(answer.bounds->radial)))
                  .add(import_exact(context.budget, cap_bounds->values.view(answer.bounds->band)));
        }
      }
    }
    if (all) {
      require(!bounds || ordinary_facets.empty() || bounds->point_floors.empty(),
              "accepted horizon has unresolved point floors");
      output = std::allocate_shared<FaceTrimResultData>(
          RunAllocator<FaceTrimResultData>(resource), resource, construction, view, query.value());
      phase = Phase::complete;
      return TrimFaceProgress::complete;
    }
    if (cap_failed) {
      const auto direction =
          angular.compare(radial) >= 0 ? CapCutDirection::angular : CapCutDirection::radial;
      cap_control = continue_cap_domain({domain->caps, direction, next_table(),
                                         construction->values, construction->fixed_metric_factor},
                                        context);
      auto next = key;
      next.face_generation = next_revision(key.face_generation);
      next.domain_revision = next_revision(key.domain_revision);
      next.construction_revision = 0;
      replacement_key = next;
      resume_output.reset();
      phase = Phase::domain;
      return TrimFaceProgress::progressed;
    }
    require(bounds && !ordinary_facets.empty(), "failed horizon has no actual facet authority");
    if (!bounds->point_floors.empty()) {
      submit_floors(context);
      return TrimFaceProgress::progressed;
    }
    if (horizon == UINT64_MAX)
      throw Error(ErrorCode::resource_exhausted, "trimmed construction horizon exhausted");
    refinement = plan_construction_refinement(
        {construction, bounds, next_table(), ++horizon, limits.exact_bits}, context);
    if (!refinement->boundary_requests.empty()) {
      boundary_wanted = refinement->boundary_requests;
      counts.boundary_requests += boundary_wanted.size();
      phase = Phase::boundary;
      return TrimFaceProgress::needs_boundary;
    }
    counts.band_anchors += refinement->band_anchors.size();
    counts.packet_targets += refinement->packet_targets.size();
    append_anchors(context);
    surface.refine({view, refinement->splits, refinement->packet_targets, {}, construction});
    phase = Phase::select;
    return TrimFaceProgress::progressed;
  }
  void submit_anchors() {
    if (anchors.empty()) {
      sampled_anchors.reset();
      phase = Phase::construct;
      return;
    }
    if (!anchor_values)
      throw Error(ErrorCode::internal_error, "anchor exact values missing");
    operation_started = Clock::now();
    surface.sample({view, anchor_values.value(), next_operation(), anchors});
    phase = Phase::anchors;
  }
  void compact_anchors(std::span<const uint64_t> components) {
    require(components.size() == anchors.size(), "construction anchor inventory differs");
    uint32_t kept = 0;
    for (uint32_t i = 0; i < anchors.size(); ++i) {
      if (components[i] == UINT64_MAX)
        continue;
      anchors[kept] = anchors[i];
      births[kept] = births[i];
      anchor_components[kept++] = components[i];
    }
    if (kept != anchors.size())
      sampled_anchors.reset(); // next construction obtains a REAL compact request result
    anchors.resize(kept);
    births.resize(kept);
    anchor_components.resize(kept);
  }
  void append_anchors(JobContext &context) {
    require(static_cast<bool>(refinement), "missing trimmed construction refinement");
    if (anchors.size() > limits.anchors ||
        refinement->band_anchors.size() > limits.anchors - anchors.size())
      throw Error(ErrorCode::resource_exhausted, "trimmed-face anchor capacity");
    ExactBuilder values(resource, next_table());
    for (auto &anchor : anchors) {
      if (!anchor_values)
        throw Error(ErrorCode::internal_error, "retained anchor exact values missing");
      for (auto &uv : anchor.original_uv)
        uv = values.append(import_exact(context.budget, anchor_values.value().view(uv)));
    }
    for (const auto &anchor : refinement->band_anchors) {
      SurfacePointInput point;
      for (uint32_t axis = 0; axis < 2; ++axis)
        point.original_uv[axis] =
            values.append(import_exact(context.budget, refinement->values.view(anchor.uv[axis])));
      anchors.push_back(point);
      births.push_back(anchor.birth_horizon);
      anchor_components.push_back(anchor.component);
    }
    anchor_values = values.seal();
    sampled_anchors.reset();
  }
  void submit_floors(JobContext &context) {
    for (const auto &floor : bounds->point_floors)
      if (floor.kind == TrimFloorKind::source_boundary)
        throw Error(ErrorCode::unresolved,
                    "unchanged physical boundary has a source/precision floor");
    ExactBuilder values(resource, next_table());
    promotions.clear();
    const auto selection = view.selection();
    for (const auto &floor : bounds->point_floors) {
      SurfacePointInput point;
      point.demanded = floor.demanded;
      if (floor.kind == TrimFloorKind::bilinear_corner) {
        require(floor.authority < selection->points.size(), "B-corner floor authority missing");
        for (uint32_t axis = 0; axis < 2; ++axis)
          point.original_uv[axis] = values.append(import_exact(
              context.budget,
              selection->values.view(selection->points[floor.authority].original_uv[axis])));
      } else {
        require(floor.authority < construction->sites.size(),
                "facet-point floor authority missing");
        for (uint32_t axis = 0; axis < 2; ++axis)
          point.original_uv[axis] = values.append(import_exact(
              context.budget,
              construction->values.view(construction->sites[floor.authority].uv[axis])));
      }
      promotions.push_back(point);
    }
    promotion_values = values.seal();
    const auto compare_uv = [&](const SurfacePointInput &a, const SurfacePointInput &b) {
      for (uint32_t axis = 0; axis < 2; ++axis) {
        const auto cmp =
            import_exact(context.budget, promotion_values->view(a.original_uv[axis]))
                .compare(import_exact(context.budget, promotion_values->view(b.original_uv[axis])));
        if (cmp)
          return cmp;
      }
      return 0;
    };
    std::sort(promotions.begin(), promotions.end(),
              [&](const auto &a, const auto &b) { return compare_uv(a, b) < 0; });
    std::size_t kept = 0;
    for (const auto &point : promotions) {
      if (kept && compare_uv(promotions[kept - 1], point) == 0)
        promotions[kept - 1].demanded = std::max(promotions[kept - 1].demanded, point.demanded);
      else
        promotions[kept++] = point;
    }
    promotions.resize(kept);
    operation_started = Clock::now();
    surface.sample({view, *promotion_values, next_operation(), promotions});
    recheck_failed = true;
    phase = Phase::promotions;
  }
};

FaceTrimSession::FaceTrimSession(std::shared_ptr<RunResource> resource, SourceCache source,
                                 TrimFaceLimits limits, const std::stop_token &stop)
    : resource_(std::move(resource)), impl_(nullptr) {
  require(resource_ != nullptr, "missing trimmed-face resource");
  RunAllocator<Impl> allocator(resource_);
  impl_ = allocator.allocate(1);
  try {
    std::construct_at(impl_, resource_, std::move(source), limits, stop);
  } catch (...) {
    allocator.deallocate(impl_, 1);
    impl_ = nullptr;
    throw;
  }
}
FaceTrimSession::~FaceTrimSession() {
  if (!impl_)
    return;
  try {
    close();
  } catch (...) {
    std::fputs("trimmed-face close failed; surface owner retains its drain protocol\n", stderr);
  }
  std::destroy_at(impl_);
  RunAllocator<Impl>(resource_).deallocate(impl_, 1);
}
void FaceTrimSession::open_device(const CacheDevice &device) {
  impl_->stopped();
  require(impl_->phase == Impl::Phase::idle, "trimmed-face device open during query");
  impl_->surface.open_device(device);
}
void FaceTrimSession::begin(const TrimFaceQuery &query) {
  auto &p = *impl_;
  p.stopped();
  const bool pooled = query.assignment != 0;
  require(p.phase == Impl::Phase::idle && query.generation && query.values.resource() == resource_,
          "active trimmed query or foreign values");
  require(pooled ? p.assignment_mode != Impl::AssignmentMode::legacy &&
                       query.assignment > p.last_assignment && query.generation >= p.last_query
                 : p.assignment_mode != Impl::AssignmentMode::pooled &&
                       query.generation > p.last_query,
          "stale trimmed quality/assignment or legacy/pooled mode mismatch");
  bgfd::Budget budget({p.limits.surface.child_bytes, p.limits.construction_work}, resource_.get(),
                      p.stop);
  auto key = p.admit_boundary(query, query.boundary, budget);
  if (query.continuation) {
    const auto &resume = *query.continuation;
    const auto &origin = resume.origin;
    require(resume.resource == resource_ && &resume.source.view() == &p.source.view() &&
                origin.source_generation == p.source.generation() &&
                origin.query == query.generation && origin.definition == query.definition &&
                origin.face == query.face &&
                origin.profile_revision == p.source.profile().revision &&
                origin.chart_revision == key.chart_revision &&
                origin.boundary_generation <= key.boundary_generation && resume.control &&
                import_exact(budget, resume.values.view(resume.positional))
                        .compare(import_exact(budget, query.values.view(query.positional))) == 0 &&
                import_exact(budget, resume.values.view(resume.node_source))
                        .compare(import_exact(budget, query.values.view(query.node_source))) == 0,
            "foreign or incompatible face continuation");
    key.face_generation =
        std::max(key.boundary_generation, Impl::next_revision(origin.face_generation));
    key.domain_revision = Impl::next_revision(origin.domain_revision);
  }
  p.query = query;
  p.horizon = 0; // semantic anchor births restart only on a fresh query/assignment
  p.trace_open = false;
  p.trace_span = p.trace_failed = 0;
  if (query.generation > p.last_query)
    p.table_ordinal = 0;
  p.assignment_mode = pooled ? Impl::AssignmentMode::pooled : Impl::AssignmentMode::legacy;
  if (pooled)
    p.last_assignment = query.assignment;
  p.key = key;
  p.cap_control = query.continuation ? query.continuation->control : CapCutControlLease{};
  p.construction_work_used = budget.work();
  p.last_query = query.generation;
  p.phase = Impl::Phase::domain;
}
TrimFaceProgress FaceTrimSession::step(JobContext &caller) {
  auto &p = *impl_;
  try {
    p.stopped();
  } catch (...) {
    p.trace_early_failure();
    throw;
  }
  require(p.query.has_value(), "no trimmed-face query");
  require(p.construction_work_used <= p.limits.construction_work,
          "trimmed construction work accounting");
  std::stop_callback forward(caller.stop, Impl::ForwardStop{&p.cancel_source});
  JobContext context(
      {p.limits.surface.child_bytes, p.limits.construction_work - p.construction_work_used},
      resource_.get(), p.stop);
  auto trace_context = p.trace_context();
  diagnostic::Scope trace_scope(p.limits.trace_progress ? &trace_context : nullptr);
  if (p.limits.trace_progress && !p.trace_open) {
    p.trace_phase = p.phase;
    p.trace_open = true;
    ++p.trace_span;
    trace_context.span = p.trace_span;
    diagnostic::emit({.event = "phase_begin",
                      .phase = Impl::phase_name(p.trace_phase),
                      .byte_limit = p.limits.surface.child_bytes,
                      .work_limit = p.limits.construction_work - p.construction_work_used});
  }
  const auto advance = [&]() -> TrimFaceProgress {
    context.budget.charge(0);
    using Phase = Impl::Phase;
    if (p.phase == Phase::boundary)
      return TrimFaceProgress::needs_boundary;
    if (p.phase == Phase::complete)
      return TrimFaceProgress::complete;
    if (p.phase == Phase::domain) {
      const auto candidate_key = p.replacement_key.value_or(p.key);
      ProducedCapDomainLease cap_plan;
      if (p.source.purpose() == SourceCachePurpose::quotient_trim) {
        auto prepared =
            prepare_cap_domain({p.source, p.query->boundary, Impl::cap_key(candidate_key),
                                p.next_table(), p.cap_control},
                               context);
        p.cap_control = prepared.continuation;
        if (!prepared.boundary_requests.empty()) {
          p.boundary_wanted = std::move(prepared.boundary_requests);
          p.counts.boundary_requests += p.boundary_wanted.size();
          p.replacement_key.reset();
          p.phase = Phase::boundary;
          return TrimFaceProgress::needs_boundary;
        }
        cap_plan = std::move(prepared.plan);
      }
      const auto prepared = prepare_face_domain(
          {p.query->boundary, candidate_key, p.next_table(), cap_plan}, context);
      if (!prepared.requests.empty()) {
        p.boundary_wanted = prepared.requests;
        p.counts.boundary_requests += p.boundary_wanted.size();
        p.replacement_key.reset();
        p.phase = Phase::boundary;
        return TrimFaceProgress::needs_boundary;
      }
      const bool local = p.replacement_key.has_value();
      if (local) {
        const auto allowances = p.root_allowances();
        p.surface.replace_domain({p.view, prepared.domain, Impl::surface_key(candidate_key),
                                  p.query->values, allowances});
        p.key = candidate_key;
        p.replacement_key.reset();
        p.footprint.reset();
        p.construction.reset();
        p.bounds.reset();
        p.cap_bounds.reset();
        p.refinement.reset();
        p.sampled_cuts.reset();
        p.sampled_anchors.reset();
        p.output.reset();
        p.view = {};
      }
      p.domain = prepared.domain;
      p.classifier = std::allocate_shared<FaceDomainClassifier>(
          RunAllocator<FaceDomainClassifier>(resource_), resource_, p.domain,
          DomainClassifierLimits{std::min(p.limits.surface.host_bytes, uint64_t{1} << 28),
                                 p.remaining_work(context.budget)},
          p.stop);
      context.budget.charge(p.classifier->stats().work);
      if (!p.anchors.empty()) {
        std::pmr::vector<std::array<ExactRef, 2>> uv(resource_.get());
        for (const auto &anchor : p.anchors)
          uv.push_back(anchor.original_uv);
        const auto labels = p.classify_metered(context.budget, [&](uint64_t allowance) {
          return p.classifier->classify_points(*p.anchor_values, uv, allowance);
        });
        std::pmr::vector<uint64_t> keep(p.anchors.size(), UINT64_MAX, resource_.get());
        for (uint32_t i = 0; i < labels.size(); ++i)
          if (labels[i] == SurfaceClass::interior)
            keep[i] = 0; // new global CDT assigns actual component IDs
        p.compact_anchors(keep);
      }
      p.phase = local ? Phase::select : Phase::start;
      return TrimFaceProgress::progressed;
    }
    if (p.phase == Phase::start) {
      const auto allowances = p.root_allowances();
      p.surface.begin({p.query->generation, Impl::surface_key(p.key), p.source, p.query->values,
                       allowances, p.domain});
      p.surface_started = true;
      p.phase = Phase::select;
      return TrimFaceProgress::progressed;
    }
    if (p.phase == Phase::select || p.phase == Phase::anchors || p.phase == Phase::promotions ||
        p.phase == Phase::cuts || p.phase == Phase::bound || p.phase == Phase::cap_bound) {
      const auto state = p.surface.step(context);
      if (state == SurfaceProgress::needs_classification) {
        p.classify(context.budget);
        return TrimFaceProgress::progressed;
      }
      if (state == SurfaceProgress::pending)
        return TrimFaceProgress::pending;
      if (state == SurfaceProgress::progressed)
        return TrimFaceProgress::progressed;
      if (p.phase == Phase::select && state == SurfaceProgress::selected) {
        p.view = p.surface.selection();
        p.submit_cuts();
        return TrimFaceProgress::progressed;
      }
      if (p.phase == Phase::cuts && state == SurfaceProgress::points_complete) {
        p.sampled_cuts = p.surface.point_result();
        p.view = p.surface.selection();
        require(p.sampled_cuts->points.size() == p.cut_plan_indices.size(),
                "incomplete cut point horizon");
        p.surface.release_operation();
        if (p.refresh_cuts_after_anchors && p.sampled_anchors &&
            p.sampled_anchors->view.key() == p.view.key() &&
            p.sampled_anchors->view.selection() == p.view.selection()) {
          p.phase = Phase::construct;
          p.refresh_cuts_after_anchors = false;
        } else {
          p.submit_anchors();
        }
        return TrimFaceProgress::progressed;
      }
      if ((p.phase == Phase::anchors || p.phase == Phase::promotions) &&
          state == SurfaceProgress::points_complete) {
        const auto points = p.surface.point_result();
        p.view = p.surface.selection();
        if (p.phase == Phase::anchors) {
          require(points->points.size() == p.anchors.size(), "incomplete anchor point horizon");
          p.sampled_anchors = points;
          for (uint32_t i = 0; i < p.anchors.size(); ++i)
            p.anchors[i].demanded = points->points[i].key.stage;
          if (p.sampled_cuts && (p.sampled_cuts->view.key() != p.view.key() ||
                                 p.sampled_cuts->view.selection() != p.view.selection())) {
            p.surface.release_operation();
            p.submit_cuts(true);
            return TrimFaceProgress::progressed;
          }
          p.phase = Phase::construct;
        } else {
          p.counts.anchor_seconds += elapsed(p.operation_started);
          p.promotions.clear();
          p.promotion_values.reset();
          p.surface.release_operation();
          p.submit_cuts();
          return TrimFaceProgress::progressed;
        }
        p.surface.release_operation();
        p.counts.anchor_seconds += elapsed(p.operation_started);
        return TrimFaceProgress::progressed;
      }
      if (p.phase == Phase::bound && state == SurfaceProgress::facets_complete) {
        p.bounds = p.surface.facet_result();
        p.counts.bound_pieces += p.bounds->work.pieces().size();
        p.surface.release_operation();
        if (!p.cap_facets.empty()) {
          p.surface.bound_caps({p.view,
                                p.construction,
                                p.domain,
                                {p.query->values, p.query->positional},
                                p.cap_facets,
                                p.next_operation(),
                                p.limits.facets});
          p.phase = Phase::cap_bound;
          return TrimFaceProgress::progressed;
        }
        p.counts.bound_seconds += elapsed(p.operation_started);
        return p.finish_horizon(context);
      }
      if (p.phase == Phase::cap_bound && state == SurfaceProgress::caps_complete) {
        p.cap_bounds = p.surface.cap_result();
        p.surface.release_operation();
        p.counts.bound_seconds += elapsed(p.operation_started);
        return p.finish_horizon(context);
      }
      throw Error(ErrorCode::internal_error,
                  "surface operation completed in wrong whole-face phase");
    }
    if (p.phase == Phase::construct) {
      auto tick = Clock::now();
      const auto enabled = packet_leaf_slots(p.footprint, p.view, context.budget);
      const auto packets =
          build_packet_mesh(context.budget, resource_, p.view.selection(), enabled);
      ++p.counts.packet_rounds;
      if (p.footprint && !packets->demoted_leaf_slots.empty())
        throw Error(ErrorCode::unresolved, "refined frozen packet footprint developed a pinch");
      if (!p.footprint)
        p.footprint = freeze_packet_footprint(packets, p.view, context.budget);
      p.counts.packet_seconds += elapsed(tick);
      tick = Clock::now();
      if (p.key.construction_revision == UINT64_MAX)
        throw Error(ErrorCode::resource_exhausted, "face connectivity revision exhausted");
      ++p.key.construction_revision;
      const auto previous = p.construction;
      const RegionInput input{p.domain,
                              p.view,
                              packets,
                              p.key,
                              p.next_table(),
                              p.sampled_anchors,
                              p.births,
                              p.anchor_components,
                              previous,
                              p.refinement
                                  ? std::span<const uint64_t>(p.refinement->changed_components)
                                  : std::span<const uint64_t>{},
                              p.sampled_cuts,
                              p.cut_plan_indices};
      p.construction =
          previous ? rebuild_face_regions(input, context) : build_face_regions(input, context);
      require(p.construction->components.size() <= p.limits.regions,
              "trimmed-face region capacity exceeded");
      p.compact_anchors(p.construction->anchor_components);
      ++p.counts.region_builds;
      p.counts.region_seconds += elapsed(tick);
      p.pending_facets = p.construction->changed_facets;
      if (!previous) {
        require(p.pending_facets.size() == p.construction->facets.size(),
                "initial face did not request all actual facets");
      } else if (p.bounds) {
        // Previously undecided facets cannot become silently accepted through
        // reuse. This queries only that failed horizon, not the whole mesh.
        for (const auto &answer : p.bounds->facets) {
          if (answer.accepted)
            continue;
          const auto old_key = previous->facets[answer.facet].key;
          const auto found = std::lower_bound(
              p.construction->facets.begin(), p.construction->facets.end(), old_key,
              [](const auto &facet, const auto &key) { return facet.key < key; });
          if (found != p.construction->facets.end() && found->key == old_key) {
            const auto index =
                count32(static_cast<std::size_t>(found - p.construction->facets.begin()));
            if (p.recheck_failed)
              p.pending_facets.push_back(index);
            else
              require(std::binary_search(p.pending_facets.begin(), p.pending_facets.end(), index),
                      "undecided facet survived without a new bound request");
          }
        }
      }
      p.refinement.reset();
      std::sort(p.pending_facets.begin(), p.pending_facets.end());
      p.pending_facets.erase(std::unique(p.pending_facets.begin(), p.pending_facets.end()),
                             p.pending_facets.end());
      p.recheck_failed = false;
      require(!p.pending_facets.empty(), "construction changed no undecided actual facets");
      p.submit_bounds();
      return TrimFaceProgress::progressed;
    }
    throw Error(ErrorCode::invalid_source, "trimmed-face session is not active");
  };
  std::optional<TrimFaceProgress> progress;
  std::exception_ptr failure;
  try {
    progress = advance();
  } catch (...) {
    failure = std::current_exception();
  }
  p.construction_work_used += context.budget.work();
  try {
    caller.budget.charge(context.budget.work());
  } catch (...) {
    if (!failure)
      failure = std::current_exception();
  }
  if (p.limits.trace_progress && (failure || p.phase != p.trace_phase ||
                                  (progress && (*progress == TrimFaceProgress::complete ||
                                                *progress == TrimFaceProgress::needs_boundary)))) {
    trace_context = p.trace_context();
    diagnostic::emit({.event = failure ? "phase_failure" : "phase_end",
                      .phase = Impl::phase_name(p.trace_phase),
                      .work = context.budget.work(),
                      .peak = context.budget.peak(),
                      .byte_limit = p.limits.surface.child_bytes,
                      .work_limit = p.limits.construction_work});
    p.trace_open = false;
  }
  if (failure) {
    cancel();
    std::rethrow_exception(failure);
  }
  return *progress;
}
std::span<const BoundaryKey> FaceTrimSession::boundary_requests() const {
  require(impl_->phase == Impl::Phase::boundary, "trimmed-face boundary requests not ready");
  return impl_->boundary_wanted;
}
void FaceTrimSession::replace_boundary(SharedCurveSnapshot snapshot) {
  auto &p = *impl_;
  p.stopped();
  require(p.phase == Impl::Phase::boundary, "unexpected face boundary replacement");
  if (!p.query)
    throw Error(ErrorCode::invalid_source, "boundary replacement has no active query");
  auto &current_query = p.query.value();
  const auto prior = p.key.boundary_generation;
  bgfd::Budget budget(
      {p.limits.surface.child_bytes, p.limits.construction_work - p.construction_work_used},
      resource_.get(), p.stop);
  auto candidate = p.admit_boundary(current_query, snapshot, budget);
  require(candidate.boundary_generation > prior,
          "face boundary replacement did not advance generation");
  candidate.face_generation =
      std::max(candidate.boundary_generation, Impl::next_revision(p.key.face_generation));
  if (p.source.purpose() == SourceCachePurpose::quotient_trim)
    candidate.domain_revision = Impl::next_revision(p.key.domain_revision);
  if (p.surface_started)
    p.surface.release_query();
  p.surface_started = false;
  p.key = candidate;
  p.construction_work_used += budget.work();
  p.recheck_failed = false;
  current_query.boundary = std::move(snapshot);
  p.domain.reset();
  p.classifier.reset();
  p.footprint.reset();
  p.construction.reset();
  p.bounds.reset();
  p.cap_bounds.reset();
  p.replacement_key.reset();
  p.resume_output.reset();
  p.refinement.reset();
  p.sampled_cuts.reset();
  p.sampled_anchors.reset();
  p.view = {};
  p.boundary_wanted.clear();
  p.pending_facets.clear();
  p.phase = Impl::Phase::domain;
}
FaceContinuation FaceTrimSession::continuation() const {
  auto &p = *impl_;
  p.stopped();
  if (!p.query)
    throw Error(ErrorCode::invalid_source, "face continuation has no query");
  require(p.phase == Impl::Phase::complete || p.phase == Impl::Phase::boundary,
          "face continuation requires complete or needs-boundary state");
  if (p.resume_output)
    return p.resume_output;
  if (!p.cap_control)
    return {};
  JobContext context(
      {p.limits.surface.child_bytes, p.limits.construction_work - p.construction_work_used},
      resource_.get(), p.stop);
  p.retain_control(context);
  auto output = std::allocate_shared<FaceContinuationData>(
      RunAllocator<FaceContinuationData>(resource_), resource_, p.source, Impl::cap_key(p.key),
      FacetAllowances{p.query->values, p.query->positional}, p.query->node_source, p.cap_control);
  p.construction_work_used += context.budget.work();
  p.resume_output = std::move(output);
  return p.resume_output;
}
void FaceTrimSession::replace_domain(const TrimDomainReplacement &request) {
  auto &p = *impl_;
  p.stopped();
  if (!p.query)
    throw Error(ErrorCode::invalid_source, "local domain replacement has no query");
  require(p.phase == Impl::Phase::complete && p.view && request.expected &&
              request.expected.selection() == p.view.selection() &&
              request.expected.key() == p.view.key() && request.domain &&
              p.source.purpose() == SourceCachePurpose::quotient_trim && request.domain->caps &&
              request.values.resource() == resource_,
          "stale or busy local face domain replacement");
  JobContext context(
      {p.limits.surface.child_bytes, p.limits.construction_work - p.construction_work_used},
      resource_.get(), p.stop);
  require(import_exact(context.budget, request.values.view(request.positional))
                      .compare(import_exact(context.budget,
                                            p.query->values.view(p.query->positional))) == 0 &&
              import_exact(context.budget, request.values.view(request.node_source))
                      .compare(import_exact(context.budget,
                                            p.query->values.view(p.query->node_source))) == 0,
          "local domain replacement changes exact face quality");
  require(static_cast<bool>(request.domain->caps.control()),
          "local domain replacement loses cap control");
  const auto &control = request.domain->caps.control();
  const auto &factor = control.fixed_metric_factor();
  if (!factor)
    throw Error(ErrorCode::invalid_source, "local domain replacement loses fixed face metric");
  for (uint32_t i = 0; i < 4; ++i)
    require(import_exact(context.budget, control.values().view((*factor)[i]))
                    .compare(import_exact(
                        context.budget,
                        p.construction->values.view(p.construction->fixed_metric_factor[i]))) == 0,
            "local domain replacement changes fixed face metric");
  const auto classifier = std::allocate_shared<FaceDomainClassifier>(
      RunAllocator<FaceDomainClassifier>(resource_), resource_, request.domain,
      DomainClassifierLimits{std::min(p.limits.surface.host_bytes, uint64_t{1} << 28),
                             p.remaining_work(context.budget)},
      p.stop);
  context.budget.charge(classifier->stats().work);
  std::pmr::vector<uint64_t> keep(p.anchors.size(), UINT64_MAX, resource_.get());
  if (!p.anchors.empty()) {
    if (!p.anchor_values)
      throw Error(ErrorCode::internal_error, "local domain anchors have no exact table");
    std::pmr::vector<std::array<ExactRef, 2>> uv(resource_.get());
    for (const auto &anchor : p.anchors)
      uv.push_back(anchor.original_uv);
    const auto before = classifier->stats().work;
    const auto labels =
        classifier->classify_points(*p.anchor_values, uv, p.remaining_work(context.budget));
    context.budget.charge(classifier->stats().work - before);
    for (uint32_t i = 0; i < labels.size(); ++i)
      if (labels[i] == SurfaceClass::interior)
        keep[i] = 0;
  }
  const auto allowances = p.root_allowances();
  p.surface.replace_domain({p.view, request.domain, Impl::surface_key(request.domain->key),
                            p.query->values, allowances});
  p.domain = request.domain;
  p.key = request.domain->key;
  p.classifier = classifier;
  p.cap_control = control;
  p.compact_anchors(keep);
  p.view = {};
  p.footprint.reset();
  p.output.reset();
  p.resume_output.reset();
  p.construction.reset();
  p.bounds.reset();
  p.cap_bounds.reset();
  p.refinement.reset();
  p.sampled_cuts.reset();
  p.sampled_anchors.reset();
  p.pending_facets.clear();
  p.construction_work_used += context.budget.work();
  p.phase = Impl::Phase::select;
}
FaceTrimResult FaceTrimSession::result() const {
  require(impl_->phase == Impl::Phase::complete && impl_->output, "trimmed face not complete");
  return impl_->output;
}
TrimFaceStats FaceTrimSession::stats() const noexcept {
  auto result = impl_->counts;
  result.surface = impl_->surface.stats();
  result.direct_source_tests = result.surface.source_triangle_tests;
  return result;
}
void FaceTrimSession::release_result() {
  auto &p = *impl_;
  p.stopped();
  require(p.phase == Impl::Phase::complete, "trimmed-face result not ready for release");
  if (p.assignment_mode == Impl::AssignmentMode::pooled) {
    retire_assignment();
    return;
  }
  p.surface.release_query();
  p.release_face_state();
}
void FaceTrimSession::retire_assignment() {
  auto &p = *impl_;
  p.stopped();
  require(p.assignment_mode == Impl::AssignmentMode::pooled && p.query &&
              (p.phase == Impl::Phase::complete || p.phase == Impl::Phase::boundary),
          "trimmed-face assignment is not ready for retirement");
  p.surface.retire_assignment();
  p.release_face_state();
}

void FaceTrimSession::cancel() noexcept {
  impl_->cancelled.store(true, std::memory_order_relaxed);
  impl_->cancel_source.request_stop();
  impl_->surface.cancel();
}
void FaceTrimSession::close() {
  if (impl_->phase == Impl::Phase::closed)
    return;
  impl_->surface.close();
  impl_->phase = Impl::Phase::closed;
}
} // namespace bgmesh::adaptive
