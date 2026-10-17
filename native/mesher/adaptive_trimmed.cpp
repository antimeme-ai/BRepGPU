#include "adaptive_trimmed.hpp"
#include "adaptive_regions.hpp"
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
    construct,
    bound,
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
  uint64_t construction_work_used = 0;
  uint32_t table_ordinal = 0;
  FaceConstructionKey key;
  FaceDomainLease domain;
  std::shared_ptr<FaceDomainClassifier> classifier;
  SurfaceBoundView view;
  PacketFootprint footprint;
  FaceConstructionResult construction;
  SurfaceFacetResult bounds;
  ConstructionRefinement refinement;
  FaceTrimResult output;
  std::optional<ExactTable> anchor_values, promotion_values;
  std::pmr::vector<SurfacePointInput> anchors, promotions;
  std::pmr::vector<uint64_t> births, anchor_components;
  std::pmr::vector<BoundaryKey> boundary_wanted;
  std::pmr::vector<uint32_t> pending_facets;
  SurfacePointResult sampled_anchors;
  bool surface_started = false;
  bool recheck_failed = false;
  Clock::time_point operation_started{};
  TrimFaceStats counts;

  Impl(std::shared_ptr<RunResource> owner, SourceCache cache,
       TrimFaceLimits selected, const std::stop_token &token)
      : resource(std::move(owner)), source(std::move(cache)), limits(selected),
        stop(cancel_source.get_token()),
        surface(resource, source, limits.surface, stop),
        session(next_face_session.fetch_add(1, std::memory_order_relaxed)),
        anchors(resource.get()), promotions(resource.get()),
        births(resource.get()), anchor_components(resource.get()),
        boundary_wanted(resource.get()), pending_facets(resource.get()) {
    require(session && source.source().view().resource == resource &&
                limits.regions && limits.anchors && limits.exact_bits &&
                limits.construction_work,
            "invalid trimmed-face source owner or limits");
    external_cancel.emplace(token, ForwardStop{&cancel_source});
  }
  void stopped() const {
    if (cancelled.load(std::memory_order_relaxed) || stop.stop_requested())
      throw Error(ErrorCode::cancelled, "trimmed-face operation cancelled");
    require(phase != Phase::closed, "trimmed-face session closed");
  }
  ExactTableKey next_table() {
    if (!query)
      throw Error(ErrorCode::internal_error,
                  "trimmed-face table has no active query");
    if (table_ordinal == UINT32_MAX)
      throw Error(ErrorCode::resource_exhausted,
                  "trimmed-face exact table ordinal exhausted");
    return {{OwnerKind::face, query.value().definition.value, session},
            query.value().generation,
            ++table_ordinal};
  }
  uint64_t next_operation() {
    if (operation == UINT64_MAX)
      throw Error(ErrorCode::resource_exhausted,
                  "trimmed-face request ordinal exhausted");
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
    const auto carrier =
        s.carrier_index.find(s.faces[candidate.face.value].carrier_id);
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
          envelope[2 * a] =
              std::min(envelope[2 * a], root.prepared_box[2 * a].lower);
          envelope[2 * a + 1] =
              std::max(envelope[2 * a + 1], root.prepared_box[2 * a + 1].upper);
        }
      }
    }
    require(!first_root,
            "trimmed source cache has no selected face carrier roots");
    const auto tau =
        import_exact(budget, candidate.values.view(candidate.positional));
    const auto delta =
        import_exact(budget, candidate.values.view(candidate.node_source));
    require(tau.sign() > 0 && delta.sign() > 0,
            "nonpositive trimmed-face allowance");
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
                  use.intervals.count <=
                      snapshot->intervals.size() - use.intervals.first,
              "mixed or incomplete trimmed-face boundary generation");
      for (uint64_t i = 0; i < use.sites.count; ++i) {
        budget.charge(1);
        const auto &site = snapshot->sites[use.sites.first + i];
        require(
            import_exact(budget, snapshot->values.view(site.node_source_upper))
                    .compare(delta) <= 0,
            "shared boundary source-node quality does not fit face request");
        for (uint32_t axis = 0; axis < 2; ++axis) {
          const auto uv = import_exact(
              budget, snapshot->values.view(site.chosen_original_uv[axis]));
          if (uv.compare(Rational::from_double(budget, envelope[2 * axis])) <
                  0 ||
              uv.compare(
                  Rational::from_double(budget, envelope[2 * axis + 1])) > 0)
            throw Error(
                ErrorCode::unsupported,
                "chosen trim lies outside original finite UV profile envelope");
        }
      }
      for (uint64_t i = 0; i < use.intervals.count; ++i) {
        budget.charge(1);
        const auto &interval = snapshot->intervals[use.intervals.first + i];
        require(import_exact(budget,
                             snapshot->values.view(
                                 interval.carrier_trace_to_actual_chord_upper))
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
    return admitted;
  }
  uint64_t remaining_work(bgfd::Budget &budget) const {
    require(construction_work_used <= limits.construction_work &&
                budget.work() <=
                    limits.construction_work - construction_work_used,
            "trimmed construction work accounting");
    return limits.construction_work - construction_work_used - budget.work();
  }
  template <class Function>
  auto classify_metered(bgfd::Budget &budget, Function &&function) {
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
    require(components.size() == anchors.size(),
            "construction anchor inventory differs");
    uint32_t kept = 0;
    for (uint32_t i = 0; i < anchors.size(); ++i) {
      if (components[i] == UINT64_MAX)
        continue;
      anchors[kept] = anchors[i];
      births[kept] = births[i];
      anchor_components[kept++] = components[i];
    }
    if (kept != anchors.size())
      sampled_anchors
          .reset(); // next construction obtains a REAL compact request result
    anchors.resize(kept);
    births.resize(kept);
    anchor_components.resize(kept);
  }
  void append_anchors(JobContext &context) {
    require(static_cast<bool>(refinement),
            "missing trimmed construction refinement");
    if (anchors.size() > limits.anchors ||
        refinement->band_anchors.size() > limits.anchors - anchors.size())
      throw Error(ErrorCode::resource_exhausted,
                  "trimmed-face anchor capacity");
    ExactBuilder values(resource, next_table());
    for (auto &anchor : anchors) {
      if (!anchor_values)
        throw Error(ErrorCode::internal_error,
                    "retained anchor exact values missing");
      for (auto &uv : anchor.original_uv)
        uv = values.append(
            import_exact(context.budget, anchor_values.value().view(uv)));
    }
    for (const auto &anchor : refinement->band_anchors) {
      SurfacePointInput point;
      for (uint32_t axis = 0; axis < 2; ++axis)
        point.original_uv[axis] = values.append(import_exact(
            context.budget, refinement->values.view(anchor.uv[axis])));
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
        require(floor.authority < selection->points.size(),
                "B-corner floor authority missing");
        for (uint32_t axis = 0; axis < 2; ++axis)
          point.original_uv[axis] = values.append(import_exact(
              context.budget,
              selection->values.view(
                  selection->points[floor.authority].original_uv[axis])));
      } else {
        require(floor.authority < construction->sites.size(),
                "facet-point floor authority missing");
        for (uint32_t axis = 0; axis < 2; ++axis)
          point.original_uv[axis] = values.append(
              import_exact(context.budget,
                           construction->values.view(
                               construction->sites[floor.authority].uv[axis])));
      }
      promotions.push_back(point);
    }
    promotion_values = values.seal();
    const auto compare_uv = [&](const SurfacePointInput &a,
                                const SurfacePointInput &b) {
      for (uint32_t axis = 0; axis < 2; ++axis) {
        const auto cmp =
            import_exact(context.budget,
                         promotion_values->view(a.original_uv[axis]))
                .compare(
                    import_exact(context.budget,
                                 promotion_values->view(b.original_uv[axis])));
        if (cmp)
          return cmp;
      }
      return 0;
    };
    std::sort(
        promotions.begin(), promotions.end(),
        [&](const auto &a, const auto &b) { return compare_uv(a, b) < 0; });
    std::size_t kept = 0;
    for (const auto &point : promotions) {
      if (kept && compare_uv(promotions[kept - 1], point) == 0)
        promotions[kept - 1].demanded =
            std::max(promotions[kept - 1].demanded, point.demanded);
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

FaceTrimSession::FaceTrimSession(std::shared_ptr<RunResource> resource,
                                 SourceCache source, TrimFaceLimits limits,
                                 const std::stop_token &stop)
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
    std::fputs(
        "trimmed-face close failed; surface owner retains its drain protocol\n",
        stderr);
  }
  std::destroy_at(impl_);
  RunAllocator<Impl>(resource_).deallocate(impl_, 1);
}
void FaceTrimSession::open_device(const CacheDevice &device) {
  impl_->stopped();
  require(impl_->phase == Impl::Phase::idle,
          "trimmed-face device open during query");
  impl_->surface.open_device(device);
}
void FaceTrimSession::begin(const TrimFaceQuery &query) {
  auto &p = *impl_;
  p.stopped();
  require(p.phase == Impl::Phase::idle && query.generation > p.last_query &&
              query.values.resource() == resource_,
          "active/stale trimmed query or foreign values");
  bgfd::Budget budget(
      {p.limits.surface.child_bytes, p.limits.construction_work},
      resource_.get(), p.stop);
  const auto key = p.admit_boundary(query, query.boundary, budget);
  p.query = query;
  p.table_ordinal = 0;
  p.key = key;
  p.construction_work_used = budget.work();
  p.last_query = query.generation;
  p.phase = Impl::Phase::domain;
}
TrimFaceProgress FaceTrimSession::step(JobContext &caller) {
  auto &p = *impl_;
  p.stopped();
  require(p.query.has_value(), "no trimmed-face query");
  require(p.construction_work_used <= p.limits.construction_work,
          "trimmed construction work accounting");
  std::stop_callback forward(caller.stop, Impl::ForwardStop{&p.cancel_source});
  JobContext context({p.limits.surface.child_bytes,
                      p.limits.construction_work - p.construction_work_used},
                     resource_.get(), p.stop);
  const auto advance = [&]() -> TrimFaceProgress {
    context.budget.charge(0);
    using Phase = Impl::Phase;
    if (p.phase == Phase::boundary)
      return TrimFaceProgress::needs_boundary;
    if (p.phase == Phase::complete)
      return TrimFaceProgress::complete;
    if (p.phase == Phase::domain) {
      const auto prepared = prepare_face_domain(
          {p.query->boundary, p.key, p.next_table()}, context);
      if (!prepared.requests.empty()) {
        p.boundary_wanted = prepared.requests;
        p.counts.boundary_requests += p.boundary_wanted.size();
        p.phase = Phase::boundary;
        return TrimFaceProgress::needs_boundary;
      }
      p.domain = prepared.domain;
      p.classifier = std::allocate_shared<FaceDomainClassifier>(
          RunAllocator<FaceDomainClassifier>(resource_), resource_, p.domain,
          DomainClassifierLimits{
              std::min(p.limits.surface.host_bytes, uint64_t{1} << 28),
              p.remaining_work(context.budget)},
          p.stop);
      context.budget.charge(p.classifier->stats().work);
      if (!p.anchors.empty()) {
        std::pmr::vector<std::array<ExactRef, 2>> uv(resource_.get());
        for (const auto &anchor : p.anchors)
          uv.push_back(anchor.original_uv);
        const auto labels =
            p.classify_metered(context.budget, [&](uint64_t allowance) {
              return p.classifier->classify_points(*p.anchor_values, uv,
                                                   allowance);
            });
        std::pmr::vector<uint64_t> keep(p.anchors.size(), UINT64_MAX,
                                        resource_.get());
        for (uint32_t i = 0; i < labels.size(); ++i)
          if (labels[i] == SurfaceClass::interior)
            keep[i] = 0; // new global CDT assigns actual component IDs
        p.compact_anchors(keep);
      }
      p.phase = Phase::start;
      return TrimFaceProgress::progressed;
    }
    if (p.phase == Phase::start) {
      std::pmr::vector<RootAllowance> allowances(resource_.get());
      const auto &s = p.source.source().view();
      const auto carrier =
          s.carrier_index.find(s.faces[p.query->face.value].carrier_id);
      for (uint32_t i = 0; i < p.source.roots().size(); ++i)
        if (p.source.roots()[i].source_slot == carrier.value)
          allowances.push_back({{i}, p.query->positional});
      p.surface.begin({p.query->generation,
                       {p.key.definition, p.key.face, p.key.source_generation,
                        p.key.boundary_generation, p.key.boundary_generation,
                        p.key.chart_revision},
                       p.source,
                       p.query->values,
                       allowances});
      p.surface_started = true;
      p.phase = Phase::select;
      return TrimFaceProgress::progressed;
    }
    if (p.phase == Phase::select || p.phase == Phase::anchors ||
        p.phase == Phase::promotions || p.phase == Phase::bound) {
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
        p.submit_anchors();
        return TrimFaceProgress::progressed;
      }
      if ((p.phase == Phase::anchors || p.phase == Phase::promotions) &&
          state == SurfaceProgress::points_complete) {
        const auto points = p.surface.point_result();
        p.view = p.surface.selection();
        if (p.phase == Phase::anchors) {
          require(points->points.size() == p.anchors.size(),
                  "incomplete anchor point horizon");
          p.sampled_anchors = points;
          for (uint32_t i = 0; i < p.anchors.size(); ++i)
            p.anchors[i].demanded = points->points[i].key.stage;
          p.phase = Phase::construct;
        } else {
          p.counts.anchor_seconds += elapsed(p.operation_started);
          p.promotions.clear();
          p.promotion_values.reset();
          p.surface.release_operation();
          p.submit_anchors();
          return TrimFaceProgress::progressed;
        }
        p.surface.release_operation();
        p.counts.anchor_seconds += elapsed(p.operation_started);
        return TrimFaceProgress::progressed;
      }
      if (p.phase == Phase::bound &&
          state == SurfaceProgress::facets_complete) {
        p.bounds = p.surface.facet_result();
        require(p.bounds->work.key().construction == p.construction->key &&
                    p.bounds->work.key().surface == p.view.key() &&
                    p.bounds->facets.size() == p.pending_facets.size(),
                "stale or partial whole-facet horizon");
        const auto tau = import_exact(
            context.budget, p.query->values.view(p.query->positional));
        bool all = true;
        for (uint32_t i = 0; i < p.pending_facets.size(); ++i) {
          const auto &answer = p.bounds->facets[i];
          require(answer.facet == p.pending_facets[i],
                  "facet horizon order/inventory differs");
          if (answer.accepted) {
            require(answer.positional_upper.has_value() &&
                        import_exact(
                            context.budget,
                            p.bounds->values.view(*answer.positional_upper))
                                .compare(tau) <= 0,
                    "accepted facet upper exceeds original exact allowance");
          } else {
            all = false;
          }
        }
        p.counts.bound_pieces += p.bounds->work.pieces().size();
        p.surface.release_operation();
        p.counts.bound_seconds += elapsed(p.operation_started);
        if (all) {
          require(p.bounds->point_floors.empty(),
                  "accepted facet horizon has unresolved point floors");
          p.output = std::allocate_shared<FaceTrimResultData>(
              RunAllocator<FaceTrimResultData>(resource_), resource_,
              p.construction, p.view, p.query.value());
          p.phase = Phase::complete;
          return TrimFaceProgress::complete;
        }
        if (!p.bounds->point_floors.empty()) {
          p.submit_floors(context);
          return TrimFaceProgress::progressed;
        }
        if (p.horizon == UINT64_MAX)
          throw Error(ErrorCode::resource_exhausted,
                      "trimmed construction horizon exhausted");
        p.refinement = plan_construction_refinement(
            {p.construction, p.bounds, p.next_table(), ++p.horizon,
             p.limits.exact_bits},
            context);
        if (!p.refinement->boundary_requests.empty()) {
          p.boundary_wanted = p.refinement->boundary_requests;
          p.counts.boundary_requests += p.boundary_wanted.size();
          p.phase = Phase::boundary;
          return TrimFaceProgress::needs_boundary;
        }
        p.counts.band_anchors += p.refinement->band_anchors.size();
        p.counts.packet_targets += p.refinement->packet_targets.size();
        p.append_anchors(context);
        p.surface.refine({p.view,
                          p.refinement->splits,
                          p.refinement->packet_targets,
                          {},
                          p.construction});
        p.phase = Phase::select;
        return TrimFaceProgress::progressed;
      }
      throw Error(ErrorCode::internal_error,
                  "surface operation completed in wrong whole-face phase");
    }
    if (p.phase == Phase::construct) {
      auto tick = Clock::now();
      const auto enabled =
          packet_leaf_slots(p.footprint, p.view, context.budget);
      const auto packets = build_packet_mesh(context.budget, resource_,
                                             p.view.selection(), enabled);
      ++p.counts.packet_rounds;
      if (p.footprint && !packets->demoted_leaf_slots.empty())
        throw Error(ErrorCode::unresolved,
                    "refined frozen packet footprint developed a pinch");
      if (!p.footprint)
        p.footprint = freeze_packet_footprint(packets, p.view, context.budget);
      p.counts.packet_seconds += elapsed(tick);
      tick = Clock::now();
      if (p.key.construction_revision == UINT64_MAX)
        throw Error(ErrorCode::resource_exhausted,
                    "face connectivity revision exhausted");
      ++p.key.construction_revision;
      const auto previous = p.construction;
      const RegionInput input{
          p.domain,
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
              : std::span<const uint64_t>{}};
      p.construction = previous ? rebuild_face_regions(input, context)
                                : build_face_regions(input, context);
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
              p.construction->facets.begin(), p.construction->facets.end(),
              old_key, [](const auto &facet, const auto &key) {
                return facet.key < key;
              });
          if (found != p.construction->facets.end() && found->key == old_key) {
            const auto index = count32(static_cast<std::size_t>(
                found - p.construction->facets.begin()));
            if (p.recheck_failed)
              p.pending_facets.push_back(index);
            else
              require(std::binary_search(p.pending_facets.begin(),
                                         p.pending_facets.end(), index),
                      "undecided facet survived without a new bound request");
          }
        }
      }
      p.refinement.reset();
      std::sort(p.pending_facets.begin(), p.pending_facets.end());
      p.pending_facets.erase(
          std::unique(p.pending_facets.begin(), p.pending_facets.end()),
          p.pending_facets.end());
      p.recheck_failed = false;
      require(!p.pending_facets.empty(),
              "construction changed no undecided actual facets");
      p.operation_started = Clock::now();
      p.surface.bound({p.view,
                       p.construction,
                       p.domain,
                       {p.query->values, p.query->positional},
                       p.pending_facets,
                       p.next_operation(),
                       p.limits.facets});
      ++p.counts.facet_horizons;
      p.phase = Phase::bound;
      return TrimFaceProgress::progressed;
    }
    throw Error(ErrorCode::invalid_source,
                "trimmed-face session is not active");
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
  if (failure) {
    cancel();
    std::rethrow_exception(failure);
  }
  return *progress;
}
std::span<const BoundaryKey> FaceTrimSession::boundary_requests() const {
  require(impl_->phase == Impl::Phase::boundary,
          "trimmed-face boundary requests not ready");
  return impl_->boundary_wanted;
}
void FaceTrimSession::replace_boundary(SharedCurveSnapshot snapshot) {
  auto &p = *impl_;
  p.stopped();
  require(p.phase == Impl::Phase::boundary,
          "unexpected face boundary replacement");
  if (!p.query)
    throw Error(ErrorCode::invalid_source,
                "boundary replacement has no active query");
  auto &current_query = p.query.value();
  const auto prior = p.key.boundary_generation;
  bgfd::Budget budget({p.limits.surface.child_bytes,
                       p.limits.construction_work - p.construction_work_used},
                      resource_.get(), p.stop);
  const auto candidate = p.admit_boundary(current_query, snapshot, budget);
  require(candidate.boundary_generation > prior,
          "face boundary replacement did not advance generation");
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
  p.refinement.reset();
  p.sampled_anchors.reset();
  p.view = {};
  p.boundary_wanted.clear();
  p.pending_facets.clear();
  p.phase = Impl::Phase::domain;
}
FaceTrimResult FaceTrimSession::result() const {
  require(impl_->phase == Impl::Phase::complete && impl_->output,
          "trimmed face not complete");
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
  require(p.phase == Impl::Phase::complete,
          "trimmed-face result not ready for release");
  p.surface.release_query();
  p.surface_started = false;
  p.output.reset();
  p.bounds.reset();
  p.refinement.reset();
  p.construction.reset();
  p.recheck_failed = false;
  p.domain.reset();
  p.classifier.reset();
  p.footprint.reset();
  p.view = {};
  p.query.reset();
  p.anchor_values.reset();
  p.promotion_values.reset();
  p.sampled_anchors.reset();
  p.anchors.clear();
  p.births.clear();
  p.anchor_components.clear();
  p.promotions.clear();
  p.boundary_wanted.clear();
  p.pending_facets.clear();
  p.phase = Impl::Phase::idle;
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
