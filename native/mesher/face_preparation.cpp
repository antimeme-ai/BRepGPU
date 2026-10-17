#include "assembly.hpp"
#include "face_preparation.hpp"
#include "geometry_domain.hpp"
#include "geometry_enclosure.hpp"
#include "geometry_protection.hpp"
#include "metrics.hpp"
#include <algorithm>
#include <cmath>
#include <memory>

namespace bgmesh {
namespace {
using UV = std::array<Rational, 2>;
using namespace geometry_detail;
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
  FaceUseKind kind = FaceUseKind::ordinary_chain;
  std::optional<std::array<Rational, 2>> quotient_pc{};
};
struct DraftLoop final {
  Slot<LoopTag> loop;
  bool outer;
  Range<FaceBoundarySegment> segments;
  Range<FaceUseDisposition> uses;
};
struct DraftPolePiece final {
  uint32_t cap;
  std::array<uint32_t, 2> cut_sites;
  BoundaryKey cut;
  std::array<Rational, 4> cut_uv;
  std::array<uint32_t, 3> roles;
  Rational position, band, angular, radial, corner;
};
struct DraftPoleCap final {
  AnalyticPoleKind kind;
  Slot<CoedgeTag> quotient;
  std::array<Slot<CoedgeTag>, 2> meridians;
  std::array<Range<FaceBoundarySegment>, 2> consumed;
  std::array<Rational, 2> s, v;
  uint32_t pole_site;
  Range<FrozenPolePiece> pieces;
  std::array<BoundaryKey, 2> radial;
  int32_t sheet;
  Rational fixed_pole, fixed_band;
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
  std::pmr::vector<DraftPoleCap> caps;
  std::pmr::vector<DraftPolePiece> cap_pieces;
  std::pmr::vector<FaceConstructionSegment> construction_segments;
  std::pmr::vector<FaceConstructionLoop> construction_loops;
  bool construction_domain_present = false;

  std::optional<uint64_t> failed_segment;
  std::size_t cursor = 0, awaiting_count = 0;
  std::optional<BoundBatch> outstanding;
  FaceBoundarySnapshot published;
  DomainMap domain_map;
  PoleCutControl pole_control;
  bool usable = true;
  Impl(bgfd::Budget &b, GeometryContext &ctx, GeometryStore store, SourceBoundarySnapshot input,
       const FaceChartRequest &quality, PoleCutControl control)
      : budget(b), context(ctx), geometry(std::move(store)), original(std::move(input)),
        request(quality), chart(prepare_chart()),
        owner{OwnerKind::face, original->definition.value,
              source().faces[original->face.value].id.value},
        source_generation(geometry.view().table.key().generation), sites(&b), segments(&b),
        uses(&b), loops(&b), pending(&b), caps(&b), cap_pieces(&b), construction_segments(&b),
        construction_loops(&b), pole_control(std::move(control)) {
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
      caps.clear();
      cap_pieces.clear();
      construction_segments.clear();
      construction_loops.clear();
      construction_domain_present = false;
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
    const bool quotient = source().edges[edge.edge.value].degenerate;
    if (quotient ? (values.size() != 2 || edge.intervals.count != 0)
                 : (values.size() < 2 || edge.intervals.count != values.size() - 1))
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
  UseNodeRequest node_request(Slot<CoedgeTag> use, const PhysicalNode &node,
                              std::optional<uint32_t> endpoint = {}) {
    UseNodeRequest result{
        use, parameter(node), request.node_source_allowance, request.precision, {}};
    const auto &edge =
        source().edges[source().edge_index.find(source().coedges[use.value].edge_id).value];
    if (edge.degenerate || node.key.generator == NodeGenerator::declared_quotient) {
      if (!endpoint)
        invalid("declared quotient endpoint lacks its original occurrence side");
      result.endpoint.emplace(UseEndpointRequest{original, *endpoint});
    }
    return result;
  }
  void preflight() {
    if (request.face.value >= source().faces.size())
      invalid("face preparation source slot outside table");
    const auto &face = source().faces[request.face.value];
    uint32_t quotient_count = 0;
    for (const auto loop_id : slice(source().loop_ids, face.loops)) {
      const auto &loop = source().loops[source().loop_index.find(loop_id).value];
      for (const auto use_id : slice(source().coedge_ids, loop.coedges)) {
        budget.charge(1);
        const auto &use = source().coedges[source().coedge_index.find(use_id).value];
        const auto &edge = source().edges[source().edge_index.find(use.edge_id).value];
        if (edge.degenerate) {
          const auto kind = source().carriers[chart.envelope.carrier.value].kind;
          if (++quotient_count > 1 || (kind != 3 && kind != 4))
            throw Error(ErrorCode::unsupported,
                        "finite pole profile admits one sphere/cone quotient per face");
        } else if (edge.parameter_range[0] == edge.parameter_range[1])
          throw Error(ErrorCode::unsupported, "nondeclared zero spatial parameter extent");
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
      const std::array<UseNodeRequest, 2> requests{
          node_request(previous, endpoint(previous, true),
                       source().coedges[previous.value].sense == 1 ? 1U : 0U),
          node_request(current, endpoint(current, false),
                       source().coedges[current.value].sense == 1 ? 0U : 1U)};
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
  void prepare_quotient_use(Slot<CoedgeTag> slot, const Joint &begin, const Joint &finish) {
    const auto &use = source().coedges[slot.value];
    const auto edge_slot = source().edge_index.find(use.edge_id);
    const auto &row = chain(edge_slot);
    const auto physical = nodes(row);
    if (physical.size() != 2 || row.intervals.count != 0)
      invalid("finite pole source use has unexpected private master samples");
    const auto &a = use.sense == 1 ? physical.front() : physical.back();
    const auto &c = use.sense == 1 ? physical.back() : physical.front();
    if (a.key != c.key || a.xyz != c.xyz)
      invalid("declared collapsed edge lacks canonical source quotient relation");
    if (sites.size() >= UINT32_MAX)
      throw Error(ErrorCode::resource_exhausted, "source quotient inventory exceeds site capacity");
    const auto site = static_cast<uint32_t>(sites.size());
    const auto first = segments.size();
    const auto t0 = parameter(a), t1 = parameter(c);
    sites.push_back({begin.chart, begin.uv, t0, a, begin.enclosure});
    const std::array<Rational, 4> box{
        begin.uv[0].compare(finish.uv[0]) < 0 ? begin.uv[0] : finish.uv[0],
        begin.uv[0].compare(finish.uv[0]) > 0 ? begin.uv[0] : finish.uv[0],
        begin.uv[1].compare(finish.uv[1]) < 0 ? begin.uv[1] : finish.uv[1],
        begin.uv[1].compare(finish.uv[1]) > 0 ? begin.uv[1] : finish.uv[1]};
    const auto carrier = enclose_original_surface_jet(context, geometry, chart.envelope.carrier,
                                                      box, request.precision.transcendental_bits);
    const std::array<ExactEnclosure, 3> delta{
        difference(carrier.position[0], ExactEnclosure(Rational::from_double(budget, a.xyz[0]))),
        difference(carrier.position[1], ExactEnclosure(Rational::from_double(budget, a.xyz[1]))),
        difference(carrier.position[2], ExactEnclosure(Rational::from_double(budget, a.xyz[2])))};
    auto upper = enclosure_norm_upper(budget, delta, request.precision.norm_fractional_bits);
    const auto &edge = source().edges[edge_slot.value];
    if (edge.curve_id) {
      const auto curve =
          enclose_original_curve_range(context, geometry, source().curve_index.find(edge.curve_id),
                                       {t0.compare(t1) < 0 ? t0 : t1, t0.compare(t1) > 0 ? t0 : t1},
                                       request.precision.transcendental_bits);
      const std::array<ExactEnclosure, 3> original_delta{
          difference(curve.coordinates[0], ExactEnclosure(Rational::from_double(budget, a.xyz[0]))),
          difference(curve.coordinates[1], ExactEnclosure(Rational::from_double(budget, a.xyz[1]))),
          difference(curve.coordinates[2],
                     ExactEnclosure(Rational::from_double(budget, a.xyz[2])))};
      upper = upper.add(
          enclosure_norm_upper(budget, original_delta, request.precision.norm_fractional_bits));
    }
    if (upper.compare(request.positional_allowance) > 0)
      throw Error(ErrorCode::unresolved,
                  "collapsed original source trace exceeds finite pole positional budget");
    segments.push_back({site,
                        {edge.id.value, 0, BoundaryOwnerKind::source_edge},
                        slot,
                        {t0, t1},
                        {begin.outgoing_s, finish.incoming_s},
                        use.periodic_shift,
                        TraceCorrespondence::affine_parameter,
                        {},
                        std::move(upper)});
    uses.push_back({slot,
                    {first, 1},
                    {t0, t1},
                    FaceUseKind::declared_quotient,
                    std::array<Rational, 2>{begin.outgoing_s, finish.incoming_s}});
  }
  void prepare_use(Slot<CoedgeTag> slot, const Joint &begin, const Joint &finish) {
    const auto &use = source().coedges[slot.value];
    const auto edge = source().edge_index.find(use.edge_id);
    if (source().edges[edge.value].degenerate) {
      prepare_quotient_use(slot, begin, finish);
      return;
    }
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
    if (!failed_segment)
      prepare_cap();
  }
  static Rational magnitude(const Rational &v) { return v.sign() < 0 ? v.negative() : Rational(v); }
  static Rational lesser(const Rational &a, const Rational &b) {
    return a.compare(b) < 0 ? Rational(a) : Rational(b);
  }
  static Rational greater(const Rational &a, const Rational &b) {
    return a.compare(b) > 0 ? Rational(a) : Rational(b);
  }
  uint32_t following_site(uint64_t ordinal, const DraftLoop &loop) const {
    return segments[loop.segments.first + (ordinal - loop.segments.first + 1) % loop.segments.count]
        .site;
  }
  void prepare_cap() {
    const auto quotient = std::find_if(uses.begin(), uses.end(), [](const auto &use) {
      return use.kind == FaceUseKind::declared_quotient;
    });
    if (quotient == uses.end())
      return;
    if (chart.kind == WorkingChartKind::physical_plane_convex_q)
      throw Error(ErrorCode::unsupported, "analytic quotient requires original unwrapped chart");
    const auto ui = static_cast<uint64_t>(quotient - uses.begin());
    const auto loop_it = std::find_if(loops.begin(), loops.end(), [&](const auto &loop) {
      return ui >= loop.uses.first && ui - loop.uses.first < loop.uses.count;
    });
    if (loop_it == loops.end() || !loop_it->outer || loop_it->uses.count != 4)
      throw Error(ErrorCode::unsupported, "finite pole profile requires a four-use outer sector");
    const auto &loop = *loop_it;
    const auto prev =
        loop.uses.first + (ui - loop.uses.first + loop.uses.count - 1) % loop.uses.count;
    const auto next = loop.uses.first + (ui - loop.uses.first + 1) % loop.uses.count;
    const auto incoming = uses[prev].segments.first + uses[prev].segments.count - 1;
    const auto outgoing = uses[next].segments.first;
    const auto qseg = quotient->segments.first;
    const auto qbegin = segments[qseg].site, qend = following_site(qseg, loop);
    const auto pcut = segments[incoming].site, ncut = following_site(outgoing, loop);
    const UV a = sites[qbegin].uv, c = sites[qend].uv;
    if (a[0].compare(c[0]) == 0 || a[1].compare(c[1]) != 0)
      throw Error(ErrorCode::unsupported,
                  "finite pole source boundary must be a constant-v interval");
    std::array<uint64_t, 2> consumed{incoming, outgoing};
    std::array<uint32_t, 2> ends{pcut, ncut};
    if (sites[ends[0]].uv[0].compare(sites[ends[1]].uv[0]) > 0) {
      std::swap(ends[0], ends[1]);
      std::swap(consumed[0], consumed[1]);
    }
    const UV left = sites[ends[0]].uv, right = sites[ends[1]].uv;
    if (left[0].compare(lesser(a[0], c[0])) != 0 || right[0].compare(greater(a[0], c[0])) != 0)
      throw Error(ErrorCode::unsupported,
                  "pole meridian selected branches do not have constant-u ownership");
    for (const auto index : {prev, next}) {
      const auto &use = uses[index];
      const auto u = sites[segments[use.segments.first].site].uv[0];
      int direction = 0;
      for (uint64_t i = 0; i < use.segments.count; ++i) {
        budget.charge(1);
        const auto ordinal = use.segments.first + i;
        const auto &p = sites[segments[ordinal].site].uv,
                   &q = sites[following_site(ordinal, loop)].uv;
        const int sign = q[1].compare(p[1]);
        if (p[0].compare(u) != 0 || q[0].compare(u) != 0 || sign == 0 ||
            (direction && sign != direction))
          throw Error(ErrorCode::unsupported,
                      "finite pole meridians require monotone constant-u selected chains");
        direction = sign;
      }
    }
    // A private face cut may only terminate at settled INTERIOR master nodes.
    // Until both exist, request the actual original meridian interval.
    for (std::size_t side = 0; side < 2; ++side) {
      const auto &node = sites[ends[side]].node;
      if (node.key.generator != NodeGenerator::original_edge ||
          node.key.source != segments[consumed[side]].key.owner) {
        failure(consumed[side]);
        return;
      }
    }
    const auto &carrier = source().carriers[chart.envelope.carrier.value];
    const auto kind = carrier.kind == 3 ? AnalyticPoleKind::cone_apex
                      : a[1].sign() > 0 ? AnalyticPoleKind::sphere_north
                                        : AnalyticPoleKind::sphere_south;
    const auto finite_v = std::array<Rational, 2>{lesser(a[1], c[1]), greater(a[1], c[1])};
    const auto du = right[0].subtract(left[0]), dg = right[1].subtract(left[1]);
    const auto &pi = context.pi(request.precision.transcendental_bits + 64);
    const auto half = Rational(budget, 1).divide(Rational(budget, 2));
    uint32_t base_level = 0;
    auto width = du;
    while (width.compare(pi.lower.multiply(half)) > 0) {
      budget.charge(1);
      if (base_level == 30)
        throw Error(ErrorCode::resource_exhausted, "pole base angular partition exhausted");
      ++base_level;
      width = width.multiply(half);
    }
    if (pole_control.angular_level > 30 - base_level)
      throw Error(ErrorCode::resource_exhausted, "pole angular partition exhausted");
    const auto level = base_level + pole_control.angular_level;
    const uint32_t count = uint32_t{1} << level, base_count = uint32_t{1} << base_level;
    if (sites.size() >= UINT32_MAX || static_cast<uint64_t>(count) + 1 > UINT32_MAX - sites.size())
      throw Error(ErrorCode::resource_exhausted, "pole site capacity exhausted");
    const auto proxy_u = left[0].add(right[0]).multiply(half),
               proxy_v = a[1].add(c[1]).multiply(half);
    const auto pole_site = static_cast<uint32_t>(sites.size());
    auto pole_node = sites[qbegin].node;
    pole_node.original_parameter.reset();
    sites.push_back({{proxy_u, proxy_v}, {proxy_u, proxy_v}, Rational(budget), pole_node, {}});
    std::pmr::vector<uint32_t> cuts(&budget);
    cuts.reserve(static_cast<std::size_t>(count) + 1);
    cuts.push_back(ends[0]);
    for (uint32_t i = 1; i < count; ++i) {
      budget.charge(1);
      const auto theta = Rational(budget, i).divide(Rational(budget, count));
      const UV uv{left[0].add(du.multiply(theta)), left[1].add(dg.multiply(theta))};
      const auto point =
          canonical_original_surface(context, geometry, chart.envelope.carrier, uv[0], uv[1]);
      const auto site = static_cast<uint32_t>(sites.size());
      PhysicalNode node{{original->definition.value, NodeGenerator::face_site, owner.id,
                         request.generation, site},
                        point.xyz,
                        {}};
      sites.push_back({uv, uv, Rational(budget), node, {}});
      cuts.push_back(site);
    }
    cuts.push_back(ends[1]);
    const auto actual = [&](uint32_t l, uint32_t r) {
      return std::array<double, 9>{
          pole_node.xyz[0],     pole_node.xyz[1],     pole_node.xyz[2],
          sites[l].node.xyz[0], sites[l].node.xyz[1], sites[l].node.xyz[2],
          sites[r].node.xyz[0], sites[r].node.xyz[1], sites[r].node.xyz[2]};
    };
    const auto plan_for = [&](uint32_t l, uint32_t r) {
      return bound_analytic_pole_fan(
          context, carrier,
          {kind,
           {sites[l].uv[0], sites[l].uv[1], sites[r].uv[0], sites[r].uv[1]},
           actual(l, r),
           finite_v,
           1,
           request.precision});
    };
    const auto first_plan = plan_for(cuts[0], cuts[1]);
    // The original finite boundary and the regular cut stay on the same side
    // of the ideal singularity. Mixed/tangent sectors are explicit refusals.
    const int side = left[1].compare(first_plan.pole_parameter.upper) > 0 ? 1 : -1;
    if ((side > 0 && (left[1].compare(finite_v[1]) <= 0 || right[1].compare(finite_v[1]) <= 0)) ||
        (side < 0 && (left[1].compare(finite_v[0]) >= 0 || right[1].compare(finite_v[0]) >= 0)))
      throw Error(ErrorCode::unsupported,
                  "finite pole cut is not strictly nested inside source sector");
    if (pole_control.history) {
      const auto &history = *pole_control.history;
      if (&history.geometry.view() != &geometry.view() || history.face != original->face ||
          history.kind != kind || history.generations[0] != source_generation ||
          history.generations[1] > request.generation ||
          history.generations[2] > original->boundary_generation ||
          (history.radial_pending_mask != 0 && history.radial_pending_mask != 3) ||
          history.values.key().owner != owner ||
          history.values.key().generation != history.generations[1] ||
          history.values.key().ordinal == 0)
        invalid("pole cut history source/generation mismatch");
      for (std::size_t end = 0; end < 2; ++end)
        if (import_exact(budget, history.values.view(history.cut_uv[2 * end]))
                .compare(sites[ends[end]].uv[0]) != 0)
          invalid("pole cut history changed original meridian branch");
      if (history.radial_pending_mask)
        for (std::size_t end = 0; end < 2; ++end) {
          const auto old = import_exact(budget, history.values.view(history.cut_uv[2 * end + 1]));
          const auto &now = sites[ends[end]].uv[1];
          const auto &vp = first_plan.pole_parameter;
          const auto old_min = side > 0 ? old.subtract(vp.upper) : vp.lower.subtract(old);
          const auto new_max =
              greater(magnitude(now.subtract(vp.lower)), magnitude(now.subtract(vp.upper)));
          if (old_min.sign() <= 0 ||
              new_max.compare(old_min.multiply(Rational(budget, 3)).divide(Rational(budget, 4))) >
                  0) {
            failure(consumed[end]);
            return;
          }
        }
      pole_control.history.reset();
    }
    // Every other selected boundary is outside the convex source cap. A
    // strict separating half-plane is sufficient; no trim-contact guessing.
    const auto cross = [&](const UV &p, const UV &q, const UV &r) {
      return q[0]
          .subtract(p[0])
          .multiply(r[1].subtract(p[1]))
          .subtract(q[1].subtract(p[1]).multiply(r[0].subtract(p[0])));
    };
    const std::array<UV, 4> quad{{{left[0], finite_v[0]}, {right[0], finite_v[0]}, right, left}};
    // finite_v is constant under this profile. Check nondegenerate convexity.
    int winding = 0;
    for (std::size_t i = 0; i < 4; ++i) {
      const int turn = cross(quad[i], quad[(i + 1) % 4], quad[(i + 2) % 4]).sign();
      if (turn == 0 || (winding && turn != winding))
        throw Error(ErrorCode::unsupported, "finite pole source cap is not a convex quadrilateral");
      winding = turn;
    }
    for (const auto &other : loops)
      for (uint64_t i = 0; i < other.segments.count; ++i) {
        const auto ordinal = other.segments.first + i;
        if (ordinal == incoming || ordinal == outgoing || ordinal == qseg)
          continue;
        const auto &p = sites[segments[ordinal].site].uv,
                   &q = sites[following_site(ordinal, other)].uv;
        bool separated = false;
        for (std::size_t edge = 0; edge < 4; ++edge) {
          const int sp = cross(quad[edge], quad[(edge + 1) % 4], p).sign() * winding;
          const int sq = cross(quad[edge], quad[(edge + 1) % 4], q).sign() * winding;
          if (sp < 0 && sq < 0) {
            separated = true;
            break;
          }
          const bool meridian =
              segments[ordinal].use == uses[prev].use || segments[ordinal].use == uses[next].use;
          const bool endpoint = ((p[0].compare(left[0]) == 0 && p[1].compare(left[1]) == 0) ||
                                 (p[0].compare(right[0]) == 0 && p[1].compare(right[1]) == 0) ||
                                 (q[0].compare(left[0]) == 0 && q[1].compare(left[1]) == 0) ||
                                 (q[0].compare(right[0]) == 0 && q[1].compare(right[1]) == 0));
          if (meridian && endpoint && sp <= 0 && sq <= 0 && (sp < 0 || sq < 0)) {
            separated = true;
            break;
          }
        }
        if (!separated)
          throw Error(ErrorCode::unsupported, "another trim is not separated from finite pole cap");
      }
    Rational fixed_band(budget);
    int32_t sheet = first_plan.regular_sheet_sign;
    const auto &pole = first_plan.pole_parameter;
    const auto distance = greater(greater(magnitude(finite_v[0].subtract(pole.lower)),
                                          magnitude(finite_v[0].subtract(pole.upper))),
                                  greater(magnitude(finite_v[1].subtract(pole.lower)),
                                          magnitude(finite_v[1].subtract(pole.upper))));
    // The prepared ORIGINAL source envelope contains the complete meridian
    // ranges and every admitted future locator/cut. This canonical profile
    // upper is independent of the current radial cut and extra theta levels.
    const auto source_lo = lesser(chart.envelope.uv[2], pole.lower),
               source_hi = greater(chart.envelope.uv[3], pole.upper);
    for (uint32_t i = 0; i < base_count; ++i) {
      const auto theta0 = Rational(budget, i).divide(Rational(budget, base_count));
      const auto theta1 =
          Rational(budget, static_cast<int64_t>(i) + 1).divide(Rational(budget, base_count));
      const auto source_jet =
          enclose_analytic_surface_jet(context, carrier,
                                       {left[0].add(du.multiply(theta0)),
                                        left[0].add(du.multiply(theta1)), source_lo, source_hi},
                                       request.precision.transcendental_bits);
      fixed_band = greater(fixed_band, enclosure_norm_upper(budget, source_jet.dv,
                                                            request.precision.norm_fractional_bits)
                                           .multiply(distance));
    }
    const auto piece_first = cap_pieces.size();
    for (uint32_t i = 0; i < count; ++i) {
      auto plan = i == 0 ? first_plan : plan_for(cuts[i], cuts[i + 1]);
      if (plan.regular_sheet_sign != sheet)
        invalid("pole angular pieces disagree on original regular sheet");
      const BoundaryKey key{owner.id, i, BoundaryOwnerKind::face_cut};
      cap_pieces.push_back({0,
                            {cuts[i], cuts[i + 1]},
                            key,
                            {sites[cuts[i]].uv[0], sites[cuts[i]].uv[1], sites[cuts[i + 1]].uv[0],
                             sites[cuts[i + 1]].uv[1]},
                            plan.facet_roles,
                            plan.position_upper,
                            plan.band_upper,
                            plan.angular_upper,
                            plan.radial_upper,
                            plan.corner_upper});
    }
    caps.push_back(
        {kind,
         quotient->use,
         {segments[consumed[0]].use, segments[consumed[1]].use},
         {Range<FaceBoundarySegment>{consumed[0], 1}, Range<FaceBoundarySegment>{consumed[1], 1}},
         *quotient->quotient_pc,
         finite_v,
         pole_site,
         {piece_first, count},
         {segments[consumed[0]].key, segments[consumed[1]].key},
         sheet,
         first_plan.pole_departure_upper,
         fixed_band});
    for (const auto &original_loop : loops) {
      const auto first = construction_segments.size();
      for (uint64_t i = 0; i < original_loop.segments.count; ++i) {
        const auto ordinal = original_loop.segments.first + i;
        if (ordinal == incoming) {
          const bool forward = pcut == cuts.front();
          for (uint32_t j = 0; j < count; ++j) {
            const auto piece = forward ? j : count - 1 - j;
            const auto site = forward ? cuts[j] : cuts[count - j];
            construction_segments.push_back({site, cap_pieces[piece_first + piece].cut});
          }
        } else if (ordinal != qseg && ordinal != outgoing)
          construction_segments.push_back({segments[ordinal].site, segments[ordinal].key});
      }
      if (construction_segments.size() - first < 3)
        throw Error(ErrorCode::unsupported,
                    "strict-interior cap did not leave an ordinary source remainder");
      construction_loops.push_back(
          {original_loop.loop, original_loop.outer, {first, construction_segments.size() - first}});
    }
    construction_domain_present = true;
  }

  void admit_selected_inventory() {
    const auto select = [&](std::span<const uint64_t> candidates) {
      std::optional<uint64_t> chosen;
      for (const auto ordinal : candidates)
        if (!source()
                 .edges[source()
                            .edge_index.find(source().coedges[segments[ordinal].use.value].edge_id)
                            .value]
                 .degenerate &&
            segments[ordinal].trace_upper.sign() > 0 &&
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
          throw Error(ErrorCode::unresolved, "collapsed selected loop needs a faithful source "
                                             "chart or direct source-extent diagnosis");
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
        arcs.push_back(
            {a.use,
             a.key,
             a.site,
             c.site,
             a.t,
             a.s,
             {p[0], p[1], q[0], q[1]},
             source()
                 .edges[source().edge_index.find(source().coedges[a.use.value].edge_id).value]
                 .degenerate});
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
      frozen_uses.push_back({use.use, use.kind, use.segments, freeze(builder, use.t),
                             use.quotient_pc ? std::optional<std::array<ExactRef, 2>>{freeze(
                                                   builder, *use.quotient_pc)}
                                             : std::nullopt});
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
    std::pmr::vector<FrozenPolePiece> frozen_pieces(&budget);
    std::pmr::vector<FrozenPoleCap> frozen_caps(&budget);
    for (const auto &piece : cap_pieces)
      frozen_pieces.push_back({piece.cap, piece.cut_sites, piece.cut, freeze(builder, piece.cut_uv),
                               piece.roles, builder.append(piece.position),
                               builder.append(piece.band), builder.append(piece.angular),
                               builder.append(piece.radial), builder.append(piece.corner)});
    for (const auto &cap : caps)
      frozen_caps.push_back({cap.kind, cap.quotient, cap.meridians, cap.consumed,
                             freeze(builder, cap.s), freeze(builder, cap.v), cap.pole_site,
                             cap.pieces, cap.radial, pole_control.revision, cap.sheet,
                             builder.append(cap.fixed_pole), builder.append(cap.fixed_band),
                             request.precision});
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
    result->source_boundary = original;
    result->pole_control = pole_control;
    result->construction_domain_present = construction_domain_present;
    result->construction_segments.assign(construction_segments.begin(),
                                         construction_segments.end());
    result->construction_loops.assign(construction_loops.begin(), construction_loops.end());
    result->pole_pieces.assign(frozen_pieces.begin(), frozen_pieces.end());
    result->pole_caps.assign(frozen_caps.begin(), frozen_caps.end());
    result->protection = prepare_boundary_protection(budget, *result);
    published = std::move(result);
    return published;
  }
};
FaceBoundaryPreparation::FaceBoundaryPreparation(bgfd::Budget &budget, GeometryContext &context,
                                                 GeometryStore geometry,
                                                 SourceBoundarySnapshot original,
                                                 const FaceChartRequest &request,
                                                 PoleCutControl control)
    : budget_(&budget), impl_(nullptr) {
  if (!original)
    invalid("face preparation has no original boundary snapshot");
  std::pmr::polymorphic_allocator<Impl> allocator(&budget);
  impl_ = allocator.allocate(1);
  try {
    std::construct_at(impl_, budget, context, std::move(geometry), std::move(original), request,
                      std::move(control));
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
PoleCutControl FaceBoundaryPreparation::next_pole_control() const {
  impl_->check();
  return impl_->pole_control;
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
