#include "face_engine.hpp"
#include "domain.hpp"
#include <algorithm>
#include <cmath>
#include <memory>

namespace bgmesh {
namespace {
[[noreturn]] void invalid(const char *message) { throw Error(ErrorCode::invalid_source, message); }
[[noreturn]] void internal(const char *message) { throw Error(ErrorCode::internal_error, message); }
template <class T> void grow(std::pmr::vector<T> &v, std::size_t added) {
  if (added > v.max_size() - v.size())
    throw Error(ErrorCode::resource_exhausted, "face storage exhausted");
  const auto required = v.size() + added;
  if (required > v.capacity())
    v.reserve(
        std::max(required, v.capacity() <= v.max_size() / 2 ? v.capacity() * 2 : v.max_size()));
}
} // namespace
struct FaceEngine::Impl final {
  bgfd::Budget &budget;
  GeometryContext &context;
  FaceBoundarySnapshot boundary;
  OwnerKey owner;
  Slot<CarrierTag> carrier;
  MutableFaceTopology mesh;
  FaceCellFrontier frontier;
  Rational h2, width_u, width_v, allowance;
  std::pmr::vector<PhysicalNode> nodes;
  std::pmr::vector<TopologyEdit> changes;
  std::pmr::vector<int32_t> adapters;
  std::pmr::vector<BoundaryKey> protected_keys;
  FaceCandidateWork candidate;
  std::optional<HeuristicTicket> ticket;
  std::optional<EdgeHandle> midpoint_edge;
  std::optional<TriangleHandle> closure;
  std::optional<BoundaryKey> request;
  uint64_t candidate_ordinal = 0, node_ordinal_base = 0;
  uint32_t table_ordinal = 0;
  FaceEngineStats counters;
  bool usable = true;
  Impl(bgfd::Budget &b, GeometryContext &geometry_context, FaceBoundarySnapshot snapshot,
       FaceMetricView metric, const Rational &tau, uint64_t seed)
      : budget(b), context(geometry_context), boundary(std::move(snapshot)), owner(face_owner()),
        carrier(source().carrier_index.find(face().carrier_id)), mesh(b, metric.fixed_factor),
        frontier(b, mesh, {owner, boundary->face_generation, boundary->boundary_generation}),
        h2(metric.scales.physical_diagonal_squared.clone_into(b)),
        width_u(metric.scales.chart_width_u.clone_into(b)),
        width_v(metric.scales.chart_width_v.clone_into(b)), allowance(tau.clone_into(b)), nodes(&b),
        changes(&b), adapters(&b), protected_keys(&b) {
    if (&context.budget() != &budget)
      invalid("face geometry context uses another owner");
    if (boundary->chart_kind != WorkingChartKind::original_uv &&
        boundary->chart_kind != WorkingChartKind::analytic_unwrapped_uv)
      throw Error(ErrorCode::unsupported, "face map specialization is not connected");
    if (boundary->plane || allowance.sign() <= 0 || metric.scales.allowance.compare(tau) != 0)
      invalid("face chart/allowance mismatch");
    if (boundary->values.key().owner != owner ||
        boundary->values.key().generation != boundary->face_generation ||
        boundary->values.key().ordinal != 0)
      invalid("face seed table must reserve owner/generation ordinal0");
    nodes.reserve(boundary->sites.size());
    for (const auto &site : boundary->sites) {
      budget.charge(1);
      const std::array uv{read(site.chart[0]), read(site.chart[1])};
      for (std::size_t axis = 0; axis < 2; ++axis)
        if (uv[axis].compare(read(site.original_uv[axis])) != 0)
          invalid("ordinary chart and original UV disagree");
      if (site.node.key.definition != boundary->definition.value)
        invalid("boundary physical node belongs to another definition");
      finite(site.node.xyz);
      const auto slot = mesh.append_site(uv);
      if (slot.value != nodes.size())
        internal("face site/node append mismatch");
      nodes.push_back(site.node);
      if (site.node.key.generator == NodeGenerator::face_site && site.node.key.source == owner.id &&
          site.node.key.generation == boundary->face_generation) {
        if (site.node.key.ordinal == UINT64_MAX)
          throw Error(ErrorCode::resource_exhausted, "face node ordinal exhausted");
        node_ordinal_base = std::max(node_ordinal_base, site.node.key.ordinal + 1);
      }
    }
    std::pmr::vector<SiteSlot> cycles(&budget);
    std::pmr::vector<BoundaryKey> constraints(&budget);
    std::pmr::vector<DomainLoopView> loops(&budget);
    std::size_t segment_total = 0;
    for (const auto &loop : boundary->loops) {
      budget.charge(1);
      if (loop.segments.count > cycles.max_size() - segment_total)
        throw Error(ErrorCode::resource_exhausted, "face segment capacity exhausted");
      segment_total += static_cast<std::size_t>(loop.segments.count);
    }
    cycles.reserve(segment_total);
    constraints.reserve(segment_total);
    loops.reserve(boundary->loops.size());
    if (boundary->loops.size() != face().loops.count)
      invalid("face seed drops an original source loop");
    std::pmr::vector<uint32_t> loop_slots(&budget);
    for (const auto &loop : boundary->loops) {
      budget.charge(1);
      if (loop.source_loop.value >= source().loops.size())
        invalid("unknown source seed loop");
      const auto &original = source().loops[loop.source_loop.value];
      if (original.face_id != face().id || original.outer != loop.outer)
        invalid("source loop/face ownership mismatch");
      loop_slots.push_back(loop.source_loop.value);
      if (loop.segments.first > boundary->segments.size() ||
          loop.segments.count > boundary->segments.size() - loop.segments.first)
        invalid("face seed loop range invalid");
      const auto begin = cycles.size();
      for (uint64_t i = 0; i < loop.segments.count; ++i) {
        budget.charge(1);
        const auto &segment = boundary->segments[static_cast<std::size_t>(loop.segments.first + i)];
        if (segment.site >= nodes.size() || segment.use.value >= source().coedges.size())
          invalid("face seed segment site/use invalid");
        const auto &use = source().coedges[segment.use.value];
        if (use.loop_id != original.id)
          invalid("face seed changes original use loop");
        if (segment.constraint.kind == BoundaryOwnerKind::source_edge &&
            segment.constraint.owner != use.edge_id.value)
          invalid("face source constraint owner mismatch");
        cycles.push_back({segment.site});
        constraints.push_back(segment.constraint);
      }
      loops.push_back(
          {loop.outer,
           std::span<const SiteSlot>(cycles.data() + begin,
                                     static_cast<std::size_t>(loop.segments.count)),
           std::span<const BoundaryKey>(constraints.data() + begin,
                                        static_cast<std::size_t>(loop.segments.count))});
    }
    std::sort(loop_slots.begin(), loop_slots.end(), [&](auto a, auto c) {
      budget.charge(1);
      return a < c;
    });
    if (std::adjacent_find(loop_slots.begin(), loop_slots.end()) != loop_slots.end())
      invalid("duplicated original face seed loop");
    protected_keys.assign(constraints.begin(), constraints.end());
    std::sort(protected_keys.begin(), protected_keys.end(), [&](auto a, auto c) {
      budget.charge(1);
      return a < c;
    });
    protected_keys.erase(std::unique(protected_keys.begin(), protected_keys.end()),
                         protected_keys.end());
    auto domain = initialize_face_domain(budget, mesh, loops, seed);
    adapters = std::move(domain.traversal_adapters);
    frontier.initialize();
  }
  const SourceData &source() const {
    if (!boundary)
      invalid("null face boundary seed");
    return boundary->geometry.view().source.view();
  }
  const Face &face() const {
    const auto &s = source();
    if (boundary->face.value >= s.faces.size())
      invalid("unknown face seed source face");
    return s.faces[boundary->face.value];
  }
  OwnerKey face_owner() const {
    const auto &s = source();
    if (boundary->definition.value >= s.definitions.size() ||
        face().definition_id != s.definitions[boundary->definition.value].id)
      invalid("face seed definition mismatch");
    return {OwnerKind::face, boundary->definition.value, face().id.value};
  }
  Rational read(ExactRef ref) { return import_exact(budget, boundary->values.view(ref)); }
  void check() const {
    if (!usable)
      internal("face generation has failed");
  }
  static void finite(Vec3 xyz) {
    for (const auto x : xyz)
      if (!std::isfinite(x))
        invalid("nonfinite face node XYZ");
  }
  ExactTableKey next_table() {
    if (table_ordinal == UINT32_MAX)
      throw Error(ErrorCode::resource_exhausted, "face packet table ordinal exhausted");
    return {owner, boundary->face_generation, ++table_ordinal};
  }
  BoundTaskHeader header(CellResultKey key) const {
    return {{owner.id, key.triangle.slot, key.triangle.generation, 0},
            boundary->source_generation,
            boundary->boundary_generation,
            1,
            key.request_revision,
            boundary->face_generation};
  }
  std::array<Rational, 6> uv(TriangleHandle handle) const {
    const auto sites = mesh.triangle(handle).sites;
    return {
        mesh.coordinate(sites[0], CoordinateAxis::x), mesh.coordinate(sites[0], CoordinateAxis::y),
        mesh.coordinate(sites[1], CoordinateAxis::x), mesh.coordinate(sites[1], CoordinateAxis::y),
        mesh.coordinate(sites[2], CoordinateAxis::x), mesh.coordinate(sites[2], CoordinateAxis::y)};
  }
  FaceCandidateWork make_candidate(CellResultKey key, FaceCandidateKind kind,
                                   const std::array<Rational, 2> &point) {
    if (candidate_ordinal == UINT64_MAX)
      throw Error(ErrorCode::resource_exhausted, "face candidate ordinal exhausted");
    ExactBuilder builder(boundary->resource, next_table());
    const std::array refs{builder.append(point[0]), builder.append(point[1])};
    auto work = std::allocate_shared<FaceCandidateData>(
        RunAllocator<FaceCandidateData>(boundary->resource),
        FaceCandidateData{boundary, key, ++candidate_ordinal, kind, builder.seal(), refs, refs});
    ++counters.candidates;
    return work;
  }
  FaceAdvance boundary_request(BoundaryKey key, CellResultKey target) {
    request = key;
    return {FaceAdvanceKind::boundary_request, target, {}, key};
  }
  bool current_target(CellResultKey key) const {
    return mesh.is_live(key.triangle) && frontier.key(key.triangle) == key;
  }
  void fold_changes() {
    auto edits = mesh.take_changes();
    grow(changes, edits.size());
    frontier.apply_changes(edits);
    changes.insert(changes.end(), edits.begin(), edits.end());
  }
};
FaceEngine::FaceEngine(bgfd::Budget &budget, GeometryContext &context,
                       FaceBoundarySnapshot boundary, FaceMetricView metric,
                       const Rational &allowance, uint64_t seed)
    : budget_(&budget), impl_(nullptr) {
  std::pmr::polymorphic_allocator<Impl> allocator(&budget);
  impl_ = allocator.allocate(1);
  try {
    std::construct_at(impl_, budget, context, std::move(boundary), metric, allowance, seed);
  } catch (...) {
    allocator.deallocate(impl_, 1);
    throw;
  }
}
FaceEngine::~FaceEngine() {
  std::destroy_at(impl_);
  std::pmr::polymorphic_allocator<Impl>(budget_).deallocate(impl_, 1);
}
std::optional<FaceCellWork> FaceEngine::take_cell_work(uint32_t maximum) {
  impl_->check();
  try {
    const auto keys = impl_->frontier.take_pending(maximum);
    if (keys.empty())
      return std::nullopt;
    ExactBuilder builder(impl_->boundary->resource, impl_->next_table());
    std::pmr::vector<FaceCellInput> rows(budget_);
    rows.reserve(keys.size());
    for (const auto key : keys) {
      budget_->charge(1);
      const auto triangle = impl_->mesh.triangle(key.triangle);
      FaceCellInput input{impl_->header(key), impl_->carrier, CellMapKind::uv_triangle, {}, {}, {}};
      for (std::size_t corner = 0; corner < 3; ++corner) {
        const auto site = triangle.sites[corner];
        input.original_uv[2 * corner] =
            builder.append(impl_->mesh.coordinate(site, CoordinateAxis::x));
        input.original_uv[2 * corner + 1] =
            builder.append(impl_->mesh.coordinate(site, CoordinateAxis::y));
        const auto &node = impl_->nodes[site.value];
        for (std::size_t axis = 0; axis < 3; ++axis)
          input.actual_xyz[3 * corner + axis] =
              builder.append(Rational::from_double(*budget_, node.xyz[axis]));
      }
      input.allowance = builder.append(impl_->allowance);
      rows.push_back(input);
    }
    auto work = std::allocate_shared<FaceCellWorkData>(
        RunAllocator<FaceCellWorkData>(impl_->boundary->resource), impl_->boundary->resource,
        impl_->boundary->geometry, builder.seal());
    work->cells.assign(rows.begin(), rows.end());
    ++impl_->counters.cell_packets;
    return FaceCellWork(work);
  } catch (...) {
    impl_->usable = false;
    throw;
  }
}
bool FaceEngine::complete(const CellBoundResult &result) {
  impl_->check();
  const auto &h = result.header;
  if (h.key.owner != impl_->owner.id || h.key.cell > UINT32_MAX || h.key.ordinal != 0 ||
      h.source_generation != impl_->boundary->source_generation || h.bound_revision != 1 ||
      h.face_generation != impl_->boundary->face_generation ||
      h.boundary_generation != impl_->boundary->boundary_generation) {
    ++impl_->counters.rejected_results;
    return false;
  }
  const CellResultKey key{{impl_->owner, h.face_generation, h.boundary_generation},
                          {static_cast<uint32_t>(h.key.cell), h.key.generation},
                          h.request_revision};
  CellNumericalState state;
  switch (result.decision) {
  case BoundDecision::accept:
    state = CellNumericalState::accepted;
    break;
  case BoundDecision::bound_fail:
    state = CellNumericalState::needs_refine;
    break;
  case BoundDecision::unresolved:
    state = CellNumericalState::unresolved;
    break;
  default:
    invalid("invalid cell decision");
  }
  const bool accepted = impl_->frontier.complete(key, state);
  if (!accepted)
    ++impl_->counters.rejected_results;
  return accepted;
}
FaceAdvance FaceEngine::advance() {
  impl_->check();
  try {
    if (impl_->request)
      return {FaceAdvanceKind::boundary_request, {}, {}, impl_->request};
    if (impl_->candidate)
      return {FaceAdvanceKind::candidate, impl_->candidate->target, impl_->candidate, {}};
    std::optional<CellResultKey> selected;
    if (impl_->closure && impl_->mesh.is_live(*impl_->closure))
      selected = impl_->frontier.key(*impl_->closure);
    else {
      impl_->closure.reset();
      const auto top = impl_->frontier.next();
      if (!top)
        return {FaceAdvanceKind::settled, {}, {}, {}};
      if (top->state == CellNumericalState::pending)
        return {FaceAdvanceKind::pending, top->key, {}, {}};
      if (top->state == CellNumericalState::unresolved)
        return {FaceAdvanceKind::unresolved, top->key, {}, {}};
      selected = top->key;
      const auto triangle = impl_->mesh.triangle(selected->triangle);
      if (!triangle.heuristic_spent) {
        const auto uv = impl_->uv(selected->triangle);
        const Rational three(*budget_, 3);
        const std::array center{uv[0].add(uv[2]).add(uv[4]).divide(three),
                                uv[1].add(uv[3]).add(uv[5]).divide(three)};
        const auto local = original_source_metric(
            impl_->context, impl_->boundary->geometry, impl_->carrier, center,
            {impl_->h2, impl_->width_u, impl_->width_v, impl_->allowance});
        const auto chosen = metric_circumcenter(uv, local);
        impl_->ticket = impl_->mesh.stage_heuristic(selected->triangle, chosen);
        if (impl_->ticket) {
          const std::array point{
              impl_->mesh.heuristic_coordinate(*impl_->ticket, CoordinateAxis::x),
              impl_->mesh.heuristic_coordinate(*impl_->ticket, CoordinateAxis::y)};
          impl_->candidate = impl_->make_candidate(*selected, FaceCandidateKind::heuristic, point);
          return {FaceAdvanceKind::candidate, selected, impl_->candidate, {}};
        }
      }
      impl_->closure = selected->triangle;
    }
    const auto terminal = impl_->mesh.next_terminal(selected->triangle);
    const auto edge = impl_->mesh.edge(terminal);
    if (edge.constraint)
      return impl_->boundary_request(*edge.constraint, *selected);
    if (edge.incident_count != 2)
      internal("unowned face boundary encountered");
    const Rational two(*budget_, 2);
    const std::array point{impl_->mesh.coordinate(edge.sites[0], CoordinateAxis::x)
                               .add(impl_->mesh.coordinate(edge.sites[1], CoordinateAxis::x))
                               .divide(two),
                           impl_->mesh.coordinate(edge.sites[0], CoordinateAxis::y)
                               .add(impl_->mesh.coordinate(edge.sites[1], CoordinateAxis::y))
                               .divide(two)};
    impl_->midpoint_edge = terminal;
    impl_->candidate =
        impl_->make_candidate(*selected, FaceCandidateKind::longest_edge_midpoint, point);
    return {FaceAdvanceKind::candidate, selected, impl_->candidate, {}};
  } catch (...) {
    impl_->usable = false;
    throw;
  }
}
bool FaceEngine::complete_candidate(const FaceCandidateResult &result) {
  impl_->check();
  if (!impl_->candidate || result.target != impl_->candidate->target ||
      result.ordinal != impl_->candidate->ordinal || !impl_->current_target(result.target)) {
    ++impl_->counters.rejected_results;
    return false;
  }
  try {
    if (result.decision == FaceCandidateDecision::protected_feature) {
      if (!result.feature)
        invalid("protected candidate lacks feature owner");
      if (!std::binary_search(impl_->protected_keys.begin(), impl_->protected_keys.end(),
                              *result.feature, [&](auto a, auto b) {
                                budget_->charge(1);
                                return a < b;
                              }))
        invalid("candidate requested a foreign boundary feature");
      if (impl_->ticket)
        impl_->mesh.cancel_heuristic(*impl_->ticket);
      impl_->ticket.reset();
      impl_->midpoint_edge.reset();
      impl_->candidate.reset();
      impl_->request = result.feature;
      return true;
    }
    if (result.decision != FaceCandidateDecision::admitted || result.feature)
      invalid("candidate admission/feature mismatch");
    Impl::finite(result.xyz);
    if (impl_->node_ordinal_base > UINT64_MAX - impl_->nodes.size())
      throw Error(ErrorCode::resource_exhausted, "face node creation ordinal exhausted");
    const auto ordinal = impl_->node_ordinal_base + impl_->nodes.size();
    const PhysicalNode node{{impl_->boundary->definition.value, NodeGenerator::face_site,
                             impl_->owner.id, impl_->boundary->face_generation, ordinal},
                            result.xyz,
                            {}};
    grow(impl_->nodes, 1);
    SiteSlot site;
    if (impl_->ticket)
      site = impl_->mesh.commit_heuristic(*impl_->ticket);
    else if (impl_->midpoint_edge)
      site = impl_->mesh.bisect_internal_edge(*impl_->midpoint_edge);
    else
      internal("candidate lacks owned topology transaction");
    if (site.value != impl_->nodes.size())
      internal("committed face site/node mismatch");
    impl_->nodes.push_back(node);
    impl_->ticket.reset();
    impl_->midpoint_edge.reset();
    impl_->candidate.reset();
    impl_->fold_changes();
    ++impl_->counters.accepted_candidates;
    return true;
  } catch (...) {
    impl_->usable = false;
    throw;
  }
}
CellResultKey FaceEngine::retry(CellResultKey key) {
  impl_->check();
  if (impl_->candidate)
    invalid("cannot retry while a candidate transaction is staged");
  if (!impl_->current_target(key))
    invalid("stale face retry request");
  return impl_->frontier.retry(key.triangle);
}
bool FaceEngine::mark_contact_failure(CellResultKey key) {
  impl_->check();
  if (impl_->candidate || impl_->closure || impl_->request)
    invalid("contact epoch requires settled face construction");
  return impl_->frontier.mark_contact_failure(key);
}
std::pmr::vector<TopologyEdit> FaceEngine::take_changes() {
  impl_->check();
  std::pmr::vector<TopologyEdit> out(budget_);
  out.swap(impl_->changes);
  return out;
}
std::pmr::vector<CellResultKey> FaceEngine::take_settled() {
  impl_->check();
  return impl_->frontier.take_settled();
}
const MutableFaceTopology &FaceEngine::topology() const {
  impl_->check();
  return impl_->mesh;
}
const PhysicalNode &FaceEngine::node(SiteSlot site) const {
  impl_->check();
  if (site.value >= impl_->nodes.size())
    invalid("unknown face physical site");
  return impl_->nodes[site.value];
}
std::span<const int32_t> FaceEngine::traversal_adapters() const {
  impl_->check();
  return impl_->adapters;
}
std::array<DeclaredContactEdge,3> FaceEngine::declared_edges(TriangleHandle handle) const {
  impl_->check();
  const auto triangle=impl_->mesh.triangle(handle);
  std::array<DeclaredContactEdge,3> result{};
  for(std::size_t i=0;i<3;++i) {
    const auto edge=impl_->mesh.edge(triangle.edges[i]);
    if(edge.constraint) {
      const auto &key=*edge.constraint;
      result[i]={key.kind==BoundaryOwnerKind::source_edge ? ContactEdgeKind::source_edge : ContactEdgeKind::face_cut,
                 key.owner,key.kind==BoundaryOwnerKind::source_edge ? 0 : impl_->boundary->face_generation,key.interval};
    } else {
      const auto a=std::min(edge.sites[0].value,edge.sites[1].value),
                 b=std::max(edge.sites[0].value,edge.sites[1].value);
      result[i]={ContactEdgeKind::internal_face,impl_->owner.id,impl_->boundary->face_generation,
                 (uint64_t{a}<<32)|b};
    }
  }
  return result;
}
FaceEngineStats FaceEngine::stats() const {
  impl_->check();
  auto result = impl_->counters;
  result.topology = impl_->mesh.stats();
  result.frontier = impl_->frontier.stats();
  return result;
}
} // namespace bgmesh
