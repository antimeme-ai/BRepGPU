#include "assembly.hpp"
#include "face_preparation.hpp"
#include "geometry_domain.hpp"
#include "geometry_protection.hpp"
#include "metrics.hpp"
#include <algorithm>
#include <cmath>
#include <memory>

namespace bgmesh {
namespace {
using UV = std::array<Rational, 2>;
[[noreturn]] void invalid(const char *message) { throw Error(ErrorCode::invalid_source, message); }
template <class T> std::span<const T> slice(const std::pmr::vector<T> &pool, Range<T> range) {
  if (range.first > pool.size() || range.count > pool.size() - range.first)
    invalid("face preparation source range outside retained pool");
  return std::span<const T>(pool).subspan(static_cast<std::size_t>(range.first),
                                          static_cast<std::size_t>(range.count));
}
struct Joint final {
  UV chart, uv;
  Rational incoming_s, outgoing_s;
  std::optional<std::array<Rational, 4>> enclosure{};
};
struct UseSite final {
  UV chart, uv;
  Rational t, s;
  PhysicalNode node;
  std::optional<std::array<Rational, 4>> enclosure{};
};
struct DraftSite final {
  UV chart, uv;
  Rational t;
  PhysicalNode node;
  std::optional<std::array<Rational, 4>> enclosure{};
};
struct DraftSegment final {
  uint32_t site;
  BoundaryKey key;
  Slot<CoedgeTag> use;
  std::array<Rational, 2> t, s;
  std::array<int32_t, 2> shift;
  TraceCorrespondence correspondence;
  std::optional<GeometricLineMap> line;
  Rational trace_upper;
};
struct DraftUse final {
  Slot<CoedgeTag> use;
  Range<FaceBoundarySegment> segments;
  std::array<Rational, 2> t;
};
struct DraftLoop final {
  Slot<LoopTag> loop;
  bool outer;
  Range<FaceBoundarySegment> segments;
  Range<FaceUseDisposition> uses;
};
struct PendingChord final {
  uint64_t segment;
  CurveChordPiece piece;
  BoundTaskHeader header;
};
bool same_header(const BoundTaskHeader &a, const BoundTaskHeader &b) {
  return a.key == b.key && a.source_generation == b.source_generation &&
         a.boundary_generation == b.boundary_generation && a.bound_revision == b.bound_revision &&
         a.request_revision == b.request_revision && a.face_generation == b.face_generation;
}
template <std::size_t N>
std::array<ExactRef, N> freeze(ExactBuilder &builder, const std::array<Rational, N> &values) {
  std::array<ExactRef, N> refs{};
  for (std::size_t i = 0; i < N; ++i)
    refs[i] = builder.append(values[i]);
  return refs;
}
} // namespace
struct FaceBoundaryPreparation::Impl final {
  bgfd::Budget &budget;
  GeometryContext &context;
  GeometryStore geometry;
  SourceBoundarySnapshot original;
  FaceChartRequest request;
  WorkingChart chart;
  OwnerKey owner;
  uint64_t source_generation;
  uint32_t table_ordinal = 0;
  std::pmr::vector<DraftSite> sites;
  std::pmr::vector<DraftSegment> segments;
  std::pmr::vector<DraftUse> uses;
  std::pmr::vector<DraftLoop> loops;
  std::pmr::vector<PendingChord> pending;
  std::optional<uint64_t> failed_segment;
  std::size_t cursor = 0, awaiting_count = 0;
  std::optional<BoundBatch> outstanding;
  FaceBoundarySnapshot published;
  DomainMap domain_map;
  bool usable = true;
  Impl(bgfd::Budget &b, GeometryContext &ctx, GeometryStore store, SourceBoundarySnapshot input,
       const FaceChartRequest &quality)
      : budget(b), context(ctx), geometry(std::move(store)), original(std::move(input)),
        request(quality), chart(prepare_chart()),
        owner{OwnerKind::face, original->definition.value,
              source().faces[original->face.value].id.value},
        source_generation(geometry.view().table.key().generation), sites(&b), segments(&b),
        uses(&b), loops(&b), pending(&b) {
    try {
      prepare();
    } catch (const Error &error) {
      if (chart.kind != WorkingChartKind::physical_plane_convex_q ||
          (error.code != ErrorCode::unsupported && error.code != ErrorCode::unresolved))
        throw;
      // Failed optional inverse profile retries the original UV chart once.
      // Work remains cumulative; no child work or seed has been published.
      chart.kind = WorkingChartKind::original_uv;
      chart.plane.reset();
      sites.clear();
      segments.clear();
      uses.clear();
      loops.clear();
      pending.clear();
      failed_segment.reset();
      domain_map.reset();
      prepare();
    }
  }
  const SourceData &source() const { return geometry.view().source.view(); }
  WorkingChart prepare_chart() {
    if (&context.budget() != &budget || !original || &original->source.view() != &source() ||
        request.face != original->face || request.face.value >= source().faces.size() ||
        original->definition.value >= source().definitions.size() ||
        source().faces[request.face.value].definition_id !=
            source().definitions[original->definition.value].id)
      invalid("face preparation source/context ownership mismatch");
    // Admit flat ranges/typed slots before a lookup comparator or numerical
    // proposal can index the immutable native transport.
    std::pmr::vector<Slot<EdgeTag>> expected(&budget);
    const auto &face = source().faces[request.face.value];
    for (const auto id : slice(source().loop_ids, face.loops)) {
      const auto &loop = source().loops[source().loop_index.find(id).value];
      for (const auto use : slice(source().coedge_ids, loop.coedges))
        expected.push_back(source().edge_index.find(
            source().coedges[source().coedge_index.find(use).value].edge_id));
    }
    std::sort(expected.begin(), expected.end(), [&](auto a, auto b) {
      return source().edges[a.value].id < source().edges[b.value].id;
    });
    expected.erase(std::unique(expected.begin(), expected.end()), expected.end());
    if (expected.size() != original->edges.size())
      invalid("settled source boundary edge inventory differs from face");
    for (std::size_t i = 0; i < expected.size(); ++i) {
      const auto &row = original->edges[i];
      if (row.edge != expected[i])
        invalid("settled source boundary edge slots/order differ from face");
      const auto values = nodes(row);
      const auto &edge = source().edges[row.edge.value];
      for (const auto &key : slice(original->intervals, row.intervals))
        if (key.kind != BoundaryOwnerKind::source_edge || key.owner != edge.id.value)
          invalid("settled source interval belongs to a foreign feature");
      for (const auto &node : values) {
        if (node.key.definition != original->definition.value)
          invalid("settled source node belongs to foreign definition");
        for (const auto coordinate : node.xyz)
          if (!std::isfinite(coordinate))
            invalid("nonfinite settled source node XYZ");
        static_cast<void>(parameter(node));
      }
    }
    return choose_chart(context, geometry, request);
  }
  void check() const {
    if (!usable)
      throw Error(ErrorCode::internal_error, "failed face preparation owner");
  }
  ExactTableKey next_table() {
    if (table_ordinal == UINT32_MAX)
      throw Error(ErrorCode::resource_exhausted, "face preparation table ordinal exhausted");
    return {owner, request.generation, ++table_ordinal};
  }
  const SourceBoundaryEdge &chain(Slot<EdgeTag> edge) const {
    const auto id = source().edges[edge.value].id;
    const auto found = std::lower_bound(
        original->edges.begin(), original->edges.end(), id,
        [&](const auto &row, auto value) { return source().edges[row.edge.value].id < value; });
    if (found == original->edges.end() || found->edge != edge)
      invalid("settled source boundary omits a face edge");
    return *found;
  }
  Rational parameter(const PhysicalNode &node) {
    if (!node.original_parameter)
      invalid("settled source node omits original spatial parameter");
    return import_exact(budget, original->parameters.view(*node.original_parameter));
  }
  std::span<const PhysicalNode> nodes(const SourceBoundaryEdge &edge) const {
    const auto values = slice(original->nodes, edge.nodes);
    if (values.size() < 2 || edge.intervals.count != values.size() - 1)
      invalid("settled source chain has invalid interval inventory");
    return values;
  }
  const PhysicalNode &endpoint(Slot<CoedgeTag> slot, bool finish) const {
    const auto &use = source().coedges[slot.value];
    const auto &row = chain(source().edge_index.find(use.edge_id));
    const auto values = nodes(row);
    const bool last = (use.sense == 1) == finish;
    return last ? values.back() : values.front();
  }
  UseNodeRequest node_request(Slot<CoedgeTag> use, const PhysicalNode &node) {
    return {use, parameter(node), request.node_source_allowance, request.precision, {}};
  }
  void preflight() {
    if (request.face.value >= source().faces.size())
      invalid("face preparation source slot outside table");
    const auto &face = source().faces[request.face.value];
    for (const auto loop_id : slice(source().loop_ids, face.loops)) {
      const auto &loop = source().loops[source().loop_index.find(loop_id).value];
      for (const auto use_id : slice(source().coedge_ids, loop.coedges)) {
        budget.charge(1);
        const auto &use = source().coedges[source().coedge_index.find(use_id).value];
        const auto &edge = source().edges[source().edge_index.find(use.edge_id).value];
        if (edge.degenerate || edge.parameter_range[0] == edge.parameter_range[1])
          throw Error(ErrorCode::unsupported,
                      "declared quotient use requires explicit pole/cut coverage preparation");
        if (use.sense != 1 && use.sense != -1)
          invalid("original coedge traversal sense invalid");
        static_cast<void>(nodes(chain(source().edge_index.find(use.edge_id))));
      }
    }
  }
  std::pmr::vector<Joint> joints(std::span<const CoedgeId> ids) {
    std::pmr::vector<Joint> values(&budget);
    values.reserve(ids.size());
    for (std::size_t i = 0; i < ids.size(); ++i) {
      const auto previous = source().coedge_index.find(ids[(i + ids.size() - 1) % ids.size()]);
      const auto current = source().coedge_index.find(ids[i]);
      const std::array<UseNodeRequest, 2> requests{node_request(previous, endpoint(previous, true)),
                                                   node_request(current, endpoint(current, false))};
      const auto chosen = choose_boundary_joint(context, chart, requests);
      if (chosen.occurrences.size() != 2)
        throw Error(ErrorCode::internal_error, "joint producer lost an occurrence");
      values.push_back({chosen.chart, chosen.original_uv, chosen.occurrences[0].pcurve_parameter,
                        chosen.occurrences[1].pcurve_parameter, chosen.original_uv_enclosure});
    }
    return values;
  }
  void failure(uint64_t segment) {
    if (!failed_segment || segment < *failed_segment)
      failed_segment.emplace(segment);
  }
  BoundTaskHeader header(uint64_t ordinal) const {
    return {{owner.id, ordinal, request.generation, 1},
            source_generation,
            original->boundary_generation,
            1,
            0,
            request.generation};
  }
  void prepare_use(Slot<CoedgeTag> slot, const Joint &begin, const Joint &finish) {
    const auto &use = source().coedges[slot.value];
    const auto edge = source().edge_index.find(use.edge_id);
    const auto &row = chain(edge);
    const auto physical = nodes(row);
    const auto constraints = slice(original->intervals, row.intervals);
    std::pmr::vector<UseSite> choices(&budget);
    choices.reserve(physical.size());
    for (std::size_t i = 0; i < physical.size(); ++i) {
      const auto index = use.sense == 1 ? i : physical.size() - 1 - i;
      const auto &node = physical[index];
      const auto t = parameter(node);
      if (i == 0)
        choices.push_back({begin.chart, begin.uv, t, begin.outgoing_s, node, begin.enclosure});
      else if (i + 1 == physical.size())
        choices.push_back({finish.chart, finish.uv, t, finish.incoming_s, node, finish.enclosure});
      else {
        const auto chosen = locate_boundary_site(context, chart, node_request(slot, node));
        choices.push_back({chosen.chart, chosen.original_uv, t, chosen.pcurve_parameter, node,
                           chosen.original_uv_enclosure});
      }
    }
    const auto first = segments.size();
    for (std::size_t i = 1; i < choices.size(); ++i) {
      budget.charge(1);
      const auto &a = choices[i - 1], &c = choices[i];
      if (sites.size() >= UINT32_MAX)
        throw Error(ErrorCode::resource_exhausted, "face boundary site count exhausted");
      const auto site = static_cast<uint32_t>(sites.size());
      sites.push_back({a.chart, a.uv, a.t, a.node, a.enclosure});
      const auto key = constraints[use.sense == 1 ? i - 1 : constraints.size() - i];
      const auto forward = a.t.compare(c.t) < 0;
      const auto &left = forward ? a : c, &right = forward ? c : a;
      TraceIntervalRequest trace{slot,
                                 {left.t, right.t},
                                 {left.chart[0], left.chart[1], right.chart[0], right.chart[1]},
                                 {left.uv[0], left.uv[1], right.uv[0], right.uv[1]},
                                 {Rational::from_double(budget, left.node.xyz[0]),
                                  Rational::from_double(budget, left.node.xyz[1]),
                                  Rational::from_double(budget, left.node.xyz[2]),
                                  Rational::from_double(budget, right.node.xyz[0]),
                                  Rational::from_double(budget, right.node.xyz[1]),
                                  Rational::from_double(budget, right.node.xyz[2])},
                                 request.positional_allowance,
                                 request.precision,
                                 true};
      auto plan = bound_trace_interval(context, chart, trace);
      const auto segment = segments.size();
      segments.push_back({site,
                          key,
                          slot,
                          {a.t, c.t},
                          {a.s, c.s},
                          use.periodic_shift,
                          plan.correspondence,
                          std::move(plan.line),
                          plan.trace_upper});
      if (plan.trace_upper.compare(request.positional_allowance) > 0 ||
          (plan.chord_upper && plan.chord_upper->compare(request.positional_allowance) > 0))
        failure(segment);
      for (auto &piece : plan.chord_pieces) {
        const auto ordinal = pending.size();
        pending.push_back({segment, std::move(piece), header(ordinal)});
      }
    }
    uses.push_back({slot, {first, segments.size() - first}, {choices.front().t, choices.back().t}});
  }
  void prepare() {
    preflight();
    const auto &face = source().faces[request.face.value];
    for (const auto id : slice(source().loop_ids, face.loops)) {
      const auto slot = source().loop_index.find(id);
      const auto &loop = source().loops[slot.value];
      const auto ids = slice(source().coedge_ids, loop.coedges);
      if (ids.empty())
        invalid("original face loop has no uses");
      const auto common = joints(ids);
      const auto first = segments.size(), use_first = uses.size();
      for (std::size_t i = 0; i < ids.size(); ++i)
        prepare_use(source().coedge_index.find(ids[i]), common[i], common[(i + 1) % common.size()]);
      loops.push_back({slot,
                       loop.outer,
                       {first, segments.size() - first},
                       {use_first, uses.size() - use_first}});
    }
    admit_selected_inventory();
    if (!failed_segment)
      prepare_domain();
  }
  void admit_selected_inventory() {
    const auto select = [&](std::span<const uint64_t> candidates) {
      std::optional<uint64_t> chosen;
      for (const auto ordinal : candidates)
        if (segments[ordinal].trace_upper.sign() > 0 &&
            (!chosen || segments[ordinal].key < segments[*chosen].key))
          chosen = ordinal;
      return chosen;
    };
    for (const auto &loop : loops)
      if (loop.segments.count < 3) {
        std::pmr::vector<uint64_t> candidates(&budget);
        for (uint64_t i = 0; i < loop.segments.count; ++i)
          candidates.push_back(loop.segments.first + i);
        const auto chosen = select(candidates);
        if (!chosen)
          throw Error(ErrorCode::unresolved,"collapsed selected loop needs a faithful source chart or direct source-extent diagnosis");
        failure(*chosen);
      }
    // Exact working-coordinate aliases are classified before mutable topology
    // appends sites. Source occurrences/physical IDs are never welded together.
    std::pmr::vector<uint32_t> order(&budget), previous(sites.size(), UINT32_MAX, &budget);
    order.reserve(sites.size());
    for (uint32_t i = 0; i < sites.size(); ++i)
      order.push_back(i);
    for (const auto &loop : loops)
      for (uint64_t i = 0; i < loop.segments.count; ++i)
        previous[segments[loop.segments.first + (i + 1) % loop.segments.count].site] =
            static_cast<uint32_t>(loop.segments.first + i);
    std::sort(order.begin(), order.end(), [&](auto a, auto c) {
      budget.charge(1);
      const auto x = sites[a].chart[0].compare(sites[c].chart[0]);
      if (x != 0)
        return x < 0;
      const auto y = sites[a].chart[1].compare(sites[c].chart[1]);
      return y != 0 ? y < 0 : a < c;
    });
    for (std::size_t i = 1; i < order.size(); ++i) {
      const auto a = order[i - 1], c = order[i];
      if (sites[a].chart[0].compare(sites[c].chart[0]) != 0 ||
          sites[a].chart[1].compare(sites[c].chart[1]) != 0)
        continue;
      const std::array<uint64_t, 4> candidates{a, c, previous[a], previous[c]};
      if (previous[a] == UINT32_MAX || previous[c] == UINT32_MAX)
        invalid("source selected alias lacks complete incident inventory");
      const auto chosen = select(candidates);
      if (!chosen)
        throw Error(ErrorCode::unresolved, "exact selected alias requires another declared source "
                                           "chart/occurrence representation");
      failure(*chosen);
    }
  }
  void prepare_domain() {
    const auto any = std::any_of(uses.begin(), uses.end(), [&](const auto &u) {
      return !source().coedges[u.use.value].reconstructed;
    });
    if (!any)
      return;
    std::pmr::vector<DomainArcInput> arcs(&budget);
    arcs.reserve(segments.size());
    for (const auto &loop : loops)
      for (uint64_t i = 0; i < loop.segments.count; ++i) {
        const auto ordinal = loop.segments.first + i;
        const auto next = loop.segments.first + (i + 1) % loop.segments.count;
        const auto &a = segments[ordinal], &c = segments[next];
        const auto &p = sites[a.site].chart, &q = sites[c.site].chart;
        arcs.push_back({a.use, a.key, a.site, c.site, a.t, a.s, {p[0], p[1], q[0], q[1]}});
      }
    auto result = construct_domain_map(
        context, {chart,
                  original->definition,
                  {source_generation, request.generation, original->boundary_generation},
                  arcs,
                  original->resource,
                  next_table(),
                  {},
                  request.precision});
    if (result.boundary_request) {
      const auto found = std::find_if(segments.begin(), segments.end(), [&](const auto &s) {
        return s.key == *result.boundary_request;
      });
      if (found == segments.end())
        invalid("domain construction requested foreign source feature");
      failure(static_cast<uint64_t>(found - segments.begin()));
      return;
    }
    if (!result.map)
      invalid("authoritative preparation omitted named original domain map");
    domain_map = std::move(result.map);
    // Source-only reference trace approximation includes the named inverse-map
    // movement.  Actual-cell tau minus E_domain is a separate, later claim.
    for (std::size_t i = 0; i < arcs.size(); ++i) {
      const auto &a = arcs[i];
      const std::array<Rational, 6> edge{a.selected[0], a.selected[1], a.selected[2],
                                         a.selected[3], a.selected[0], a.selected[1]};
      segments[i].trace_upper =
          segments[i].trace_upper.add(bound_domain_displacement(context, domain_map, edge).upper);
      if (segments[i].trace_upper.compare(request.positional_allowance) > 0)
        failure(i);
    }
  }
  FacePreparationState state() const {
    check();
    if (outstanding || cursor < pending.size())
      return FacePreparationState::curve_work;
    return failed_segment ? FacePreparationState::boundary_request : FacePreparationState::ready;
  }
  FaceBoundarySnapshot freeze_seed() {
    if (state() != FacePreparationState::ready)
      invalid("face seed requested before complete source geometry/chord fold");
    if (published)
      return published;
    ExactBuilder builder(original->resource, next_table());
    std::pmr::vector<FaceBoundarySite> frozen_sites(&budget);
    std::pmr::vector<FaceBoundarySegment> frozen_segments(&budget);
    std::pmr::vector<FaceUseDisposition> frozen_uses(&budget);
    std::pmr::vector<FaceBoundaryLoop> frozen_loops(&budget);
    std::pmr::vector<FaceTraceMap> frozen_maps(&budget);
    for (const auto &site : sites) {
      auto node = site.node;
      node.original_parameter.reset();
      if (node.key.generator == NodeGenerator::original_edge)
        node.original_parameter.emplace(builder.append(site.t));
      std::optional<std::array<ExactRef, 4>> enclosure;
      if (site.enclosure)
        enclosure.emplace(freeze(builder, *site.enclosure));
      frozen_sites.push_back(
          {freeze(builder, site.chart), freeze(builder, site.uv), node, std::move(enclosure)});
    }
    for (const auto &segment : segments) {
      frozen_segments.push_back(
          {segment.site, segment.key, segment.use, freeze(builder, segment.t), segment.shift});
      std::optional<FrozenGeometricLineMap> line;
      if (segment.line)
        line.emplace(FrozenGeometricLineMap{freeze(builder, segment.line->covector),
                                            freeze(builder, segment.line->curve_scalar),
                                            freeze(builder, segment.line->trace_scalar)});
      frozen_maps.push_back({segment.correspondence, freeze(builder, segment.s), std::move(line),
                             builder.append(segment.trace_upper)});
    }
    for (const auto &use : uses)
      frozen_uses.push_back(
          {use.use, FaceUseKind::ordinary_chain, use.segments, freeze(builder, use.t)});
    for (const auto &loop : loops)
      frozen_loops.push_back({loop.loop, loop.outer, loop.segments, loop.uses});
    std::optional<FrozenPlaneChart> plane;
    if (chart.plane)
      plane.emplace(FrozenPlaneChart{
          freeze(builder, chart.plane->natural_rectangle), freeze(builder, chart.plane->origin),
          freeze(builder, chart.plane->basis), freeze(builder, chart.plane->projection),
          freeze(builder, chart.plane->quadrilateral), freeze(builder, chart.plane->preconditioner),
          builder.append(chart.plane->height_upper),
          builder.append(chart.plane->inverse_monotonicity_lower)});
    const auto prepared_rectangle = freeze(builder, chart.envelope.uv);
    auto result = std::allocate_shared<FaceBoundarySnapshotData>(
        RunAllocator<FaceBoundarySnapshotData>(original->resource), original->resource, geometry,
        original->definition, original->face,
        std::array<uint64_t, 3>{source_generation, request.generation,
                                original->boundary_generation},
        chart.kind, builder.seal());
    result->sites.assign(frozen_sites.begin(), frozen_sites.end());
    result->segments.assign(frozen_segments.begin(), frozen_segments.end());
    result->uses.assign(frozen_uses.begin(), frozen_uses.end());
    result->loops.assign(frozen_loops.begin(), frozen_loops.end());
    result->trace_maps.assign(frozen_maps.begin(), frozen_maps.end());
    result->prepared_original_rectangle = prepared_rectangle;
    result->plane = std::move(plane);
    result->domain_map = domain_map;
    result->protection = prepare_boundary_protection(budget, *result);
    published = std::move(result);
    return published;
  }
};
FaceBoundaryPreparation::FaceBoundaryPreparation(bgfd::Budget &budget, GeometryContext &context,
                                                 GeometryStore geometry,
                                                 SourceBoundarySnapshot original,
                                                 const FaceChartRequest &request)
    : budget_(&budget), impl_(nullptr) {
  if (!original)
    invalid("face preparation has no original boundary snapshot");
  std::pmr::polymorphic_allocator<Impl> allocator(&budget);
  impl_ = allocator.allocate(1);
  try {
    std::construct_at(impl_, budget, context, std::move(geometry), std::move(original), request);
  } catch (...) {
    allocator.deallocate(impl_, 1);
    throw;
  }
}
FaceBoundaryPreparation::~FaceBoundaryPreparation() {
  std::destroy_at(impl_);
  std::pmr::polymorphic_allocator<Impl>(budget_).deallocate(impl_, 1);
}
FacePreparationState FaceBoundaryPreparation::state() const { return impl_->state(); }
std::optional<BoundaryKey> FaceBoundaryPreparation::boundary_request() const {
  impl_->check();
  if (impl_->state() != FacePreparationState::boundary_request || !impl_->failed_segment)
    return std::nullopt;
  return impl_->segments[static_cast<std::size_t>(*impl_->failed_segment)].key;
}
std::pmr::vector<BoundRootSelection> FaceBoundaryPreparation::curve_roots() const {
  impl_->check();
  std::pmr::vector<BoundRootSelection> roots(budget_);
  for (const auto &row : impl_->pending)
    roots.emplace_back(row.piece.root);
  std::sort(roots.begin(), roots.end());
  roots.erase(std::unique(roots.begin(), roots.end()), roots.end());
  return roots;
}
std::optional<BoundBatch> FaceBoundaryPreparation::take_curve_work(const BoundRootBank &bank,
                                                                   BoundBatchLimits limits) {
  impl_->check();
  if (impl_->outstanding)
    invalid("face preparation already awaits a numerical chord batch");
  if (impl_->cursor == impl_->pending.size())
    return std::nullopt;
  if (limits.task_capacity == 0 || limits.table_capacity == 0)
    invalid("zero preparation chord batch capacity");
  if (&bank.geometry().view() != &impl_->geometry.view())
    invalid("preparation chord bank belongs to foreign geometry");
  const auto count = std::min(static_cast<std::size_t>(limits.task_capacity),
                              impl_->pending.size() - impl_->cursor);
  ExactBuilder builder(impl_->original->resource, impl_->next_table());
  std::pmr::vector<BoundInput> inputs(budget_);
  inputs.reserve(count);
  for (std::size_t i = 0; i < count; ++i) {
    const auto &row = impl_->pending[impl_->cursor + i];
    inputs.emplace_back(CurveBoundInput{row.header, bank.find(row.piece.root),
                                        freeze(builder, row.piece.original_parameter),
                                        freeze(builder, row.piece.actual_endpoint_xyz),
                                        builder.append(impl_->request.positional_allowance)});
  }
  const std::array<ExactTable, 1> tables{builder.seal()};
  auto batch = seal_bound_batch(impl_->original->resource, bank, tables, inputs, limits);
  impl_->outstanding.emplace(batch);
  impl_->awaiting_count = count;
  return batch;
}
void FaceBoundaryPreparation::complete(const BoundCompleted &completed) {
  impl_->check();
  if (!impl_->outstanding)
    invalid("face preparation has no numerical chord input outstanding");
  const auto headers = completed.headers();
  const auto answers = completed.answers();
  if (headers.size() != impl_->awaiting_count || answers.size() != headers.size())
    invalid("partial preparation chord result");
  std::pmr::vector<uint8_t> seen(impl_->awaiting_count, 0, budget_);
  for (std::size_t i = 0; i < headers.size(); ++i) {
    const auto ordinal = headers[i].key.cell;
    if (ordinal < impl_->cursor || ordinal - impl_->cursor >= impl_->awaiting_count ||
        answers[i].key != headers[i].key)
      invalid("foreign preparation chord result key");
    const auto local = static_cast<std::size_t>(ordinal - impl_->cursor);
    if (seen[local]++ ||
        !same_header(headers[i], impl_->pending[static_cast<std::size_t>(ordinal)].header))
      invalid("duplicate or stale preparation chord result");
    if (answers[i].decision != BoundDecision::accept &&
        answers[i].decision != BoundDecision::bound_fail)
      throw Error(ErrorCode::unresolved, "preparation chord bound remains unresolved");
  }
  for (std::size_t i = 0; i < answers.size(); ++i)
    if (answers[i].decision == BoundDecision::bound_fail)
      impl_->failure(impl_->pending[static_cast<std::size_t>(headers[i].key.cell)].segment);
  impl_->cursor += impl_->awaiting_count;
  impl_->awaiting_count = 0;
  impl_->outstanding.reset();
}
FaceBoundarySnapshot FaceBoundaryPreparation::snapshot() {
  impl_->check();
  try {
    return impl_->freeze_seed();
  } catch (...) {
    if (impl_->state() == FacePreparationState::ready)
      impl_->usable = false;
    throw;
  }
}
FrozenFaceMetric prepare_face_metric(GeometryContext &context, FaceBoundarySnapshot seed,
                                     const DefinitionEnvelopeView &envelope,
                                     const Rational &allowance, GeometryBoundPrecision precision) {
  auto &budget = context.budget();
  if (!seed || &envelope.source.view() != &seed->geometry.view().source.view() ||
      envelope.definition != seed->definition || &allowance.budget() != &budget ||
      allowance.sign() <= 0)
    invalid("face metric source/envelope ownership mismatch");
  if (precision.transcendental_bits < 32 ||
      precision.transcendental_bits > context.maximum_bits() ||
      precision.norm_fractional_bits > context.maximum_bits())
    invalid("invalid face metric precision profile");
  const auto &source = seed->geometry.view().source.view();
  if (seed->face.value >= source.faces.size() ||
      seed->definition.value >= source.definitions.size() || seed->sites.empty())
    invalid("face metric has no complete source boundary");
  const OwnerKey owner{OwnerKind::face, seed->definition.value,
                       source.faces[seed->face.value].id.value};
  if (seed->values.key().owner != owner || seed->values.key().generation != seed->face_generation ||
      seed->values.key().ordinal == 0)
    invalid("face metric requires a final prepared seed above reserved metric ordinal0");
  Rational diagonal_squared(budget, 0);
  for (std::size_t axis = 0; axis < 3; ++axis) {
    const auto &lo = envelope.xyz[2 * axis], &hi = envelope.xyz[2 * axis + 1];
    if (&lo.budget() != &budget || &hi.budget() != &budget || lo.compare(hi) > 0)
      invalid("invalid original definition metric envelope");
    const auto width = hi.subtract(lo);
    diagonal_squared = diagonal_squared.add(width.multiply(width));
  }
  std::array<Rational, 4> uv{
      import_exact(budget, seed->values.view(seed->sites[0].original_uv[0])),
      import_exact(budget, seed->values.view(seed->sites[0].original_uv[0])),
      import_exact(budget, seed->values.view(seed->sites[0].original_uv[1])),
      import_exact(budget, seed->values.view(seed->sites[0].original_uv[1]))};
  std::array<Rational, 4> chart_box{
      import_exact(budget, seed->values.view(seed->sites[0].chart[0])),
      import_exact(budget, seed->values.view(seed->sites[0].chart[0])),
      import_exact(budget, seed->values.view(seed->sites[0].chart[1])),
      import_exact(budget, seed->values.view(seed->sites[0].chart[1]))};
  const std::array<Rational, 4> rectangle{
      import_exact(budget, seed->values.view(seed->prepared_original_rectangle[0])),
      import_exact(budget, seed->values.view(seed->prepared_original_rectangle[1])),
      import_exact(budget, seed->values.view(seed->prepared_original_rectangle[2])),
      import_exact(budget, seed->values.view(seed->prepared_original_rectangle[3]))};
  for (const auto &site : seed->sites)
    for (std::size_t axis = 0; axis < 2; ++axis) {
      const auto original = import_exact(budget, seed->values.view(site.original_uv[axis]));
      const auto chosen = import_exact(budget, seed->values.view(site.chart[axis]));
      if (original.compare(rectangle[2 * axis]) < 0 ||
          original.compare(rectangle[2 * axis + 1]) > 0)
        invalid("face metric source locator leaves prepared original rectangle");
      if (original.compare(uv[2 * axis]) < 0)
        uv[2 * axis] = original;
      if (original.compare(uv[2 * axis + 1]) > 0)
        uv[2 * axis + 1] = original;
      if (chosen.compare(chart_box[2 * axis]) < 0)
        chart_box[2 * axis] = chosen;
      if (chosen.compare(chart_box[2 * axis + 1]) > 0)
        chart_box[2 * axis + 1] = chosen;
    }
  const auto width_u = chart_box[1].subtract(chart_box[0]),
             width_v = chart_box[3].subtract(chart_box[2]);
  const MetricScales scales{diagonal_squared, width_u, width_v, allowance};
  const auto factor = [&]() -> std::array<Rational, 4> {
    if (seed->chart_kind == WorkingChartKind::physical_plane_convex_q) {
      if (!seed->plane)
        invalid("plane metric lacks the admitted lift descriptor");
      const auto &p = *seed->plane;
      const std::array<ExactEnclosure, 3> zero{ExactEnclosure(Rational(budget, 0)),
                                               ExactEnclosure(Rational(budget, 0)),
                                               ExactEnclosure(Rational(budget, 0))};
      const std::array<ExactEnclosure, 3> du{
          ExactEnclosure(import_exact(budget, seed->values.view(p.basis[0]))),
          ExactEnclosure(import_exact(budget, seed->values.view(p.basis[2]))),
          ExactEnclosure(import_exact(budget, seed->values.view(p.basis[4])))};
      const std::array<ExactEnclosure, 3> dv{
          ExactEnclosure(import_exact(budget, seed->values.view(p.basis[1]))),
          ExactEnclosure(import_exact(budget, seed->values.view(p.basis[3]))),
          ExactEnclosure(import_exact(budget, seed->values.view(p.basis[5])))};
      return metric_from_point_jet({zero, du, dv, zero, zero, zero}, scales);
    }
    const std::array<Rational, 2> center{uv[0].add(uv[1]).divide(Rational(budget, 2)),
                                         uv[2].add(uv[3]).divide(Rational(budget, 2))};
    const auto carrier = source.carrier_index.find(source.faces[seed->face.value].carrier_id);
    return original_source_metric(context, seed->geometry, carrier, center, scales,
                                  precision.transcendental_bits);
  }();
  ExactBuilder builder(seed->resource, {owner, seed->face_generation, 0});
  const auto fixed = freeze(builder, factor);
  const auto frozen_scales =
      freeze(builder, std::array<Rational, 4>{diagonal_squared, width_u, width_v, allowance});
  return {builder.seal(), fixed, frozen_scales};
}
} // namespace bgmesh
