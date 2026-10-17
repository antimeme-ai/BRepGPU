#include "adaptive_cap_domain.hpp"
#include "adaptive_cap_geometry.hpp"
#include "adaptive_numeric.hpp"
#include "scheduler.hpp"
#include <algorithm>
#include <bit>
#include <limits>

namespace bgmesh::adaptive {
namespace {
void require(bool yes, const char *message) {
  if (!yes)
    throw Error(ErrorCode::invalid_source, message);
}
[[noreturn]] void unsupported(const char *message) { throw Error(ErrorCode::unsupported, message); }
uint32_t narrow(uint64_t value) {
  if (value > UINT32_MAX)
    throw Error(ErrorCode::resource_exhausted, "cap construction index exhausted");
  return static_cast<uint32_t>(value);
}
uint64_t next(uint64_t value) {
  if (value == UINT64_MAX)
    throw Error(ErrorCode::resource_exhausted, "cap cut revision exhausted");
  return value + 1;
}
using UV = std::array<Rational, 2>;
Rational read(bgfd::Budget &budget, const ExactTable &values, ExactRef ref) {
  return import_exact(budget, values.view(ref));
}
template <size_t N>
std::array<ExactRef, N> copy(const ExactTable &values, const std::array<ExactRef, N> &refs,
                           ExactBuilder &builder, bgfd::Budget &budget) {
  std::array<ExactRef, N> result;
  for (size_t i = 0; i < N; ++i)
    result[i] = builder.append(read(budget, values, refs[i]));
  return result;
}
template <class T, class... Args>
std::shared_ptr<T> retained(const std::shared_ptr<RunResource> &resource, Args &&...args) {
  return std::allocate_shared<T>(RunAllocator<T>(resource), std::forward<Args>(args)...);
}
template <class T> const T &present(const std::shared_ptr<const T> &data) {
  require(static_cast<bool>(data), "empty produced cap handle");
  return *data;
}
} // namespace
struct CapCutControlData final {
  std::shared_ptr<RunResource> resource;
  SourceCache source;
  CapDomainKey origin;
  ExactTable values;
  std::array<ExactRef, 4> old_cut;
  std::array<PhysicalCapEndpoint, 2> endpoints;
  uint32_t angular_level = 0, pending = 0;
  uint64_t revision = 0, next_interval = 0;
  std::optional<std::array<ExactRef, 4>> metric;
  CapCutControlData(std::shared_ptr<RunResource> owner, SourceCache bank, CapDomainKey key,
                    ExactTable exact)
      : resource(std::move(owner)), source(std::move(bank)), origin(key), values(std::move(exact)) {}
};
struct ProducedCapDomainData final {
  std::shared_ptr<RunResource> resource;
  SourceCache source;
  SharedCurveSnapshot curves;
  CapDomainKey key;
  ExactTable values;
  std::pmr::vector<SeamWedge> wedges;
  std::pmr::vector<CapDomainRow> caps;
  std::pmr::vector<CapPointPlan> points;
  std::pmr::vector<CapDomainPiece> pieces;
  CapCutControlLease control;
  ProducedCapDomainData(std::shared_ptr<RunResource> owner, SourceCache bank,
                       SharedCurveSnapshot boundary, CapDomainKey identity, ExactTable exact)
      : resource(std::move(owner)), source(std::move(bank)), curves(std::move(boundary)),
        key(identity), values(std::move(exact)), wedges(resource.get()), caps(resource.get()),
        points(resource.get()), pieces(resource.get()) {}
};
const CapDomainKey &CapCutControlLease::origin() const { return present(data_).origin; }
const SourceCache &CapCutControlLease::source() const { return present(data_).source; }
const ExactTable &CapCutControlLease::values() const { return present(data_).values; }
const std::array<ExactRef, 4> &CapCutControlLease::old_cut() const { return present(data_).old_cut; }
std::span<const PhysicalCapEndpoint, 2> CapCutControlLease::endpoints() const {
  return present(data_).endpoints;
}
uint32_t CapCutControlLease::angular_level() const { return present(data_).angular_level; }
uint64_t CapCutControlLease::revision() const { return present(data_).revision; }
uint32_t CapCutControlLease::pending_radial_sides() const { return present(data_).pending; }
const std::optional<std::array<ExactRef, 4>> &CapCutControlLease::fixed_metric_factor() const {
  return present(data_).metric;
}
const CapDomainKey &ProducedCapDomainLease::key() const { return present(data_).key; }
const SourceCache &ProducedCapDomainLease::source() const { return present(data_).source; }
const SharedCurveSnapshot &ProducedCapDomainLease::curves() const { return present(data_).curves; }
const ExactTable &ProducedCapDomainLease::values() const { return present(data_).values; }
std::shared_ptr<RunResource> ProducedCapDomainLease::resource() const { return present(data_).resource; }
std::span<const SeamWedge> ProducedCapDomainLease::wedges() const { return present(data_).wedges; }
std::span<const CapDomainRow> ProducedCapDomainLease::caps() const { return present(data_).caps; }
std::span<const CapPointPlan> ProducedCapDomainLease::points() const { return present(data_).points; }
std::span<const CapDomainPiece> ProducedCapDomainLease::pieces() const { return present(data_).pieces; }
const CapCutControlLease &ProducedCapDomainLease::control() const { return present(data_).control; }

class CapDomainProducer final {
  const CapDomainRequest &request;
  bgfd::Budget &budget;
  std::shared_ptr<RunResource> resource;
  ExactBuilder exact;
  CapDomainPreparation result;
  std::pmr::vector<const UseCurveTrace *> uses;
  const SourceData &source;
  const Face &face;
  const UseCurveTrace &trace(Slot<CoedgeTag> use) {
    for (const auto *row : uses) {
      budget.charge(1);
      if (row->use == use)
        return *row;
    }
    throw Error(ErrorCode::invalid_source, "cap wedge names foreign original use");
  }
  void validate() {
    require(resource && request.curves && request.curves->resource == resource &&
                request.source.view().resource == resource &&
                &request.curves->source.view() == &request.source.source().view() &&
                request.key.source_generation == request.source.generation() &&
                request.key.profile_revision == request.source.profile().revision &&
                request.key.query == request.curves->query_generation &&
                request.key.definition == request.curves->definition &&
                request.key.definition.value < source.definitions.size() &&
                face.definition_id == source.definitions[request.key.definition.value].id,
            "cap plan source/query/definition/profile mismatch");
    require(request.table_key != request.curves->values.key() &&
                request.table_key != request.curves->boundary->parameters.key() &&
                (!request.continuation || request.table_key != request.continuation.values().key()),
            "cap plan exact table aliases a retained input");
    require(request.transcendental_bits >= 32 && request.transcendental_bits <= 4096 &&
                request.norm_fractional_bits <= 4096 && request.key.query != 0,
            "cap plan precision/query invalid");
    for (const auto &use : request.curves->uses) {
      budget.charge(1);
      if (use.face == request.key.face) {
        require(use.boundary_generation == request.key.boundary_generation &&
                    use.use.value < source.coedges.size() && use.sites.count >= 2 &&
                    use.sites.first <= request.curves->sites.size() &&
                    use.sites.count <= request.curves->sites.size() - use.sites.first,
                "cap plan has stale or malformed face occurrence");
        uses.push_back(&use);
      }
    }
    uint64_t original_count = 0;
    for (uint64_t i = 0; i < face.loops.count; ++i) {
      const auto &loop = source.loops[source.loop_index.find(source.loop_ids[face.loops.first + i]).value];
      original_count += loop.coedges.count;
      for (uint64_t j = 0; j < loop.coedges.count; ++j) {
        const auto slot = source.coedge_index.find(source.coedge_ids[loop.coedges.first + j]);
        (void)trace(slot);
      }
    }
    require(original_count == uses.size(), "cap plan occurrence inventory is incomplete");
    if (request.continuation) {
      const auto &old = request.continuation.origin();
      require(&request.continuation.source().view() == &request.source.view() &&
                  old.source_generation == request.key.source_generation &&
                  old.query == request.key.query && old.definition == request.key.definition &&
                  old.face == request.key.face && old.profile_revision == request.key.profile_revision &&
                  old.chart_revision == request.key.chart_revision &&
                  old.boundary_generation <= request.key.boundary_generation,
              "cap continuation is foreign/stale or changes branches");
    }
  }
  uint32_t boundary_row(Slot<EdgeTag> edge) {
    const auto &boundary = *request.curves->boundary;
    for (uint32_t i = 0; i < boundary.edges.size(); ++i) {
      budget.charge(1);
      if (boundary.edges[i].edge == edge)
        return i;
    }
    throw Error(ErrorCode::invalid_source, "cap meridian absent from physical snapshot");
  }
  const UseCurveSite &site(const UseCurveTrace &use, uint32_t node) {
    for (uint64_t i = 0; i < use.sites.count; ++i) {
      const auto &row = request.curves->sites[use.sites.first + i];
      budget.charge(1);
      if (row.boundary_node == node)
        return row;
    }
    throw Error(ErrorCode::invalid_source, "cap cut node absent from source occurrence");
  }
  void enqueue(BoundaryKey key) {
    require(key.kind == BoundaryOwnerKind::source_edge, "cap radial request is not source-owned");
    if (std::find(result.boundary_requests.begin(), result.boundary_requests.end(), key) ==
        result.boundary_requests.end())
      result.boundary_requests.push_back(key);
  }

public:
  CapDomainProducer(const CapDomainRequest &r, JobContext &context)
      : request(r), budget(context.budget), resource(r.curves ? r.curves->resource : nullptr),
        exact(resource, r.table_key), result(resource.get()), uses(&budget),
        source(r.source.source().view()),
        face([&]() -> const Face & {
          require(r.key.face.value < source.faces.size(), "cap plan face outside original source");
          return source.faces[r.key.face.value];
        }()) {}
  CapDomainPreparation run() {
    validate();
    const SeamWedge *wedge = nullptr;
    for (const auto &row : request.curves->seam_wedges) {
      budget.charge(1);
      require(row.meridian_uses[0].value < source.coedges.size() &&
                  row.meridian_uses[1].value < source.coedges.size() &&
                  row.loop.value < source.loops.size(), "cap wedge use/loop outside source");
      if (source.coedges[row.meridian_uses[0].value].loop_id !=
          source.loops[row.loop.value].id)
        throw Error(ErrorCode::invalid_source, "cap wedge has foreign loop");
      if (source.loops[row.loop.value].face_id != face.id)
        continue;
      if (wedge)
        unsupported("multiple seam wedges per face are not in the finite cap profile");
      wedge = &row;
    }
    if (!wedge) {
      // A missing trace capability cannot silently become an ordinary zero-cap face.
      if ((source.carriers[source.carrier_index.find(face.carrier_id).value].kind == 3 ||
           source.carriers[source.carrier_index.find(face.carrier_id).value].kind == 4) &&
          face.loops.count == 1 && uses.size() == 3)
        for (uint32_t i = 0; i < uses.size(); ++i)
          for (uint32_t j = i + 1; j < uses.size(); ++j)
            if (source.coedges[uses[i]->use.value].edge_id ==
                source.coedges[uses[j]->use.value].edge_id)
              unsupported("singular repeated-meridian face requires produced wedge traces");
      auto data = retained<ProducedCapDomainData>(resource, resource, request.source,
                                                 request.curves, request.key, exact.seal());
      result.plan = ProducedCapDomainLease(std::move(data));
      return std::move(result);
    }
    require(request.source.purpose() == SourceCachePurpose::quotient_trim && face.loops.count == 1 &&
                uses.size() == 3 && wedge->loop.value < source.loops.size() &&
                source.loops[wedge->loop.value].outer &&
                source.loops[wedge->loop.value].coedges.count == 3 &&
                wedge->vertex.value < source.vertices.size(),
            "seam cap lacks finite original three-use outer topology");
    const auto &a = source.coedges[wedge->meridian_uses[0].value];
    const auto &c = source.coedges[wedge->meridian_uses[1].value];
    require(a.edge_id == c.edge_id && a.sense == -c.sense && a.reconstructed && c.reconstructed,
            "cap meridians are not opposite reconstructed uses of one edge");
    const auto edge_slot = source.edge_index.find(a.edge_id);
    const auto &edge = source.edges[edge_slot.value];
    const auto pole_id = source.vertices[wedge->vertex.value].id;
    require(!edge.degenerate && edge.vertex_ids[0] != edge.vertex_ids[1] &&
                (edge.vertex_ids[0] == pole_id || edge.vertex_ids[1] == pole_id),
            "cap pole is not a literal nondegenerate meridian endpoint");
    const auto &chain = request.curves->boundary->edges[boundary_row(edge_slot)];
    require(chain.nodes.count >= 2 && chain.nodes.count == chain.intervals.count + 1 &&
                chain.nodes.first + chain.nodes.count <= request.curves->boundary->nodes.size() &&
                chain.intervals.first + chain.intervals.count <= request.curves->boundary->intervals.size(),
            "cap meridian physical chain malformed");
    const uint32_t pole_side = edge.vertex_ids[0] == pole_id ? 0 : 1;
    const auto pole_node = narrow(chain.nodes.first + (pole_side ? chain.nodes.count - 1 : 0));
    const auto &incoming = trace(wedge->meridian_uses[0]);
    const auto &outgoing = trace(wedge->meridian_uses[1]);
    require(wedge->shared_endpoint_sites[0] == incoming.sites.first + incoming.sites.count - 1 &&
                wedge->shared_endpoint_sites[1] == outgoing.sites.first,
            "cap wedge loses original incoming/outgoing occurrence roles");
    const auto &loop = source.loops[wedge->loop.value];
    std::array<uint64_t, 2> use_positions{UINT64_MAX, UINT64_MAX};
    for (uint64_t j = 0; j < loop.coedges.count; ++j)
      for (uint32_t i = 0; i < 2; ++i)
        if (source.coedge_ids[loop.coedges.first + j] == source.coedges[wedge->meridian_uses[i].value].id)
          use_positions[i] = j;
    require(use_positions[0] < 3 && use_positions[1] == (use_positions[0] + 1) % 3,
            "cap wedge meridians are not consecutive in original loop order");
    const auto near_interval = request.curves->boundary->intervals[
        chain.intervals.first + (pole_side ? chain.intervals.count - 1 : 0)];
    if (chain.nodes.count == 2) {
      enqueue(near_interval);
      result.continuation = request.continuation;
      return std::move(result);
    }
    const auto cut_node = narrow(chain.nodes.first + (pole_side ? chain.nodes.count - 2 : 1));
    std::array<uint32_t, 2> angular{0, 1};
    const auto &values = request.curves->values;
    auto u0 = read(budget, values, wedge->finite_connector_uv[0]);
    auto u1 = read(budget, values, wedge->finite_connector_uv[2]);
    if (u0.compare(u1) > 0)
      std::swap(angular[0], angular[1]);
    std::array<UV, 2> finite{{{Rational(budget), Rational(budget)},
                              {Rational(budget), Rational(budget)}}};
    std::array<UV, 2> cut{{{Rational(budget), Rational(budget)},
                           {Rational(budget), Rational(budget)}}};
    std::array<PhysicalCapEndpoint, 2> endpoints;
    const auto &physical = request.curves->boundary->nodes[cut_node];
    require(physical.original_parameter.has_value(), "cap cut node lacks original parameter");
    const auto t = read(budget, request.curves->boundary->parameters, *physical.original_parameter);
    const auto t_ref = exact.append(t);
    for (uint32_t i = 0; i < 2; ++i) {
      const auto sector = angular[i];
      require(wedge->shared_endpoint_sites[sector] < request.curves->sites.size(),
              "wedge endpoint row out of range");
      const auto &pole_site = request.curves->sites[wedge->shared_endpoint_sites[sector]];
      require(pole_site.boundary_node == pole_node, "cap pole sector physical key differs");
      finite[i] = {read(budget, values, wedge->finite_connector_uv[2 * sector]),
                     read(budget, values, wedge->finite_connector_uv[2 * sector + 1])};
      for (uint32_t axis = 0; axis < 2; ++axis)
        require(finite[i][axis].compare(read(budget, values, pole_site.chosen_original_uv[axis])) == 0 &&
                    finite[i][axis].compare(read(budget, values, pole_site.chart_xy[axis])) == 0,
                "cap finite connector differs from captured original pole sector");
      const auto &use_trace = trace(wedge->meridian_uses[sector]);
      const auto &cut_site = site(use_trace, cut_node);
      cut[i] = {read(budget, values, cut_site.chosen_original_uv[0]),
                  read(budget, values, cut_site.chosen_original_uv[1])};
      require(cut[i][0].compare(finite[i][0]) == 0, "cap selected meridian changes U branch");
      std::optional<Rational> prior;
      int v_direction = 0;
      for (uint64_t j = 0; j < use_trace.sites.count; ++j) {
        const auto &p = request.curves->sites[use_trace.sites.first + j];
        require(read(budget, values, p.chosen_original_uv[0]).compare(finite[i][0]) == 0,
                "cap selected meridian is not constant-U");
        const auto v = read(budget, values, p.chosen_original_uv[1]);
        if (prior) {
          const auto sign = v.compare(*prior);
          require(sign != 0 && (v_direction == 0 || sign == v_direction),
                  "cap selected meridian is not strictly ordered");
          v_direction = sign;
        }
        prior = v;
      }
      endpoints[i] = {edge_slot, wedge->meridian_uses[sector], t_ref, physical.key};
    }
    const auto width = cut[1][0].subtract(cut[0][0]);
    require(width.sign() > 0, "cap angular branches coincide");
    const auto side = cut[0][1].subtract(finite[0][1]).sign();
    require(side != 0 && side == cut[1][1].subtract(finite[1][1]).sign(),
            "cap finite reference height changes sign");
    for (const auto *use : uses) {
      if (use->use == wedge->meridian_uses[0] || use->use == wedge->meridian_uses[1])
        continue;
      const auto &original = source.coedges[use->use.value];
      const auto &opposite = source.edges[source.edge_index.find(original.edge_id).value];
      const auto far_id = edge.vertex_ids[1 - pole_side];
      require(opposite.vertex_ids[0] == far_id && opposite.vertex_ids[1] == far_id,
              "cap opposite edge is not closed at the regular meridian vertex");
      for (uint64_t j = 0; j < use->sites.count; ++j) {
        const auto &p = request.curves->sites[use->sites.first + j];
        const auto u = read(budget, values, p.chosen_original_uv[0]);
        const auto v = read(budget, values, p.chosen_original_uv[1]);
        const auto g = cut[0][1].add(u.subtract(cut[0][0])
                                        .multiply(cut[1][1].subtract(cut[0][1])).divide(width));
        require(v.subtract(g).sign() == side, "whole opposite trace is not beyond cap cut");
      }
    }
    const auto carrier = source.carrier_index.find(face.carrier_id);
    const auto found = std::find_if(request.source.original_roots().begin(),
                                     request.source.original_roots().end(), [&](const auto &root) {
                                       budget.charge(1);
                                       return root.source_slot == carrier.value;
                                     });
    require(found != request.source.original_roots().end(), "cap source root missing");
    const auto prepared = prepare_cap_source(request.source, found->original, wedge->kind, budget);
    const std::array<Rational, 4> full_cut{cut[0][0], cut[0][1], cut[1][0], cut[1][1]};
    const GeometryBoundPrecision precision{request.transcendental_bits, request.norm_fractional_bits};
    if (request.continuation && request.continuation.pending_radial_sides()) {
      for (const auto &endpoint : request.continuation.endpoints()) {
        require(endpoint.edge == edge_slot, "cap continuation changes original meridian");
        const auto old_t = read(budget, request.continuation.values(), endpoint.original_t);
        bool retained_sample = false;
        for (uint64_t j = 0; j < chain.nodes.count; ++j) {
          budget.charge(1);
          const auto &node = request.curves->boundary->nodes[chain.nodes.first + j];
          if (node.key == endpoint.expected_key && node.original_parameter &&
              read(budget, request.curves->boundary->parameters, *node.original_parameter).compare(old_t) == 0)
            retained_sample = true;
        }
        require(retained_sample, "cap continuation lost original master cut sample");
      }
      std::array<Rational, 4> old{Rational(budget), Rational(budget), Rational(budget), Rational(budget)};
      for (uint32_t i = 0; i < 4; ++i)
        old[i] = read(budget, request.continuation.values(), request.continuation.old_cut()[i]);
      const auto progress = cap_cut_progress(prepared, old, full_cut, precision, budget);
      if (progress[0] != CapProgressState::proved || progress[1] != CapProgressState::proved) {
        enqueue(near_interval);
        result.continuation = request.continuation;
        return std::move(result);
      }
    }
    std::pmr::vector<Rational> borders(&budget);
    borders.push_back(cut[0][0]);
    borders.push_back(cut[1][0]);
    for (const auto &root : request.source.roots())
      if (root.source_slot == carrier.value)
        for (uint32_t i = 0; i < 2; ++i) {
          const auto u = Rational::from_double(budget, root.prepared_box[i].lower);
          if (u.compare(cut[0][0]) > 0 && u.compare(cut[1][0]) < 0)
            borders.push_back(u);
        }
    std::sort(borders.begin(), borders.end(), [](const auto &a0, const auto &b0) { return a0.compare(b0) < 0; });
    borders.erase(std::unique(borders.begin(), borders.end(), [](const auto &a0, const auto &b0) {
                    return a0.compare(b0) == 0;
                  }), borders.end());
    const auto half_pi = Rational::from_double(budget, numeric::half_pi().lower);
    for (size_t i = 0; i + 1 < borders.size();) {
      budget.charge(1);
      if (borders[i + 1].subtract(borders[i]).compare(half_pi) > 0) {
        const auto middle = borders[i].add(borders[i + 1]).divide(Rational(budget, 2));
        borders.insert(borders.begin() + static_cast<ptrdiff_t>(i + 1), middle);
      } else
        ++i;
    }
    const auto level = request.continuation ? request.continuation.angular_level() : 0;
    if (level >= 32 || borders.size() - 1 > (UINT32_MAX - 1) / (uint64_t{1} << level))
      throw Error(ErrorCode::resource_exhausted, "cap angular construction horizon exceeds index cap");
    const auto steps = uint64_t{1} << level;
    std::pmr::vector<CapPointPlan> points(resource.get());
    std::pmr::vector<CapDomainPiece> pieces(resource.get());
    const auto count = (borders.size() - 1) * steps;
    points.reserve(count + 1);
    pieces.reserve(count);
    const auto start_interval = request.continuation ? request.continuation.data_->next_interval : 0;
    if (count > UINT64_MAX - start_interval)
      throw Error(ErrorCode::resource_exhausted, "cap cut interval creation ordinal exhausted");
    std::optional<UV> previous;
    for (size_t cell = 0; cell + 1 < borders.size(); ++cell)
      for (uint64_t j = cell == 0 ? 0 : 1; j <= steps; ++j) {
        budget.charge(1);
        const auto u = borders[cell].add(borders[cell + 1].subtract(borders[cell])
                                           .multiply(Rational(budget, static_cast<int64_t>(j)))
                                           .divide(Rational(budget, static_cast<int64_t>(steps))));
        const auto xi = u.subtract(cut[0][0]).divide(width);
        const auto v = cut[0][1].add(xi.multiply(cut[1][1].subtract(cut[0][1])));
        CapPointPlan point{{exact.append(u), exact.append(v)}, SurfaceCapPoint{found->original}};
        if (points.empty())
          point.authority = endpoints[0];
        else if (cell + 2 == borders.size() && j == steps)
          point.authority = endpoints[1];
        points.push_back(point);
        if (previous) {
          const auto x0 = (*previous)[0].subtract(cut[0][0]).divide(width);
          const std::array<Rational, 2> vf{finite[0][1].add(x0.multiply(finite[1][1].subtract(finite[0][1]))),
                                           finite[0][1].add(xi.multiply(finite[1][1].subtract(finite[0][1])))};
          const std::array<Rational, 4> local{(*previous)[0], (*previous)[1], u, v};
          const auto map = admit_cap_map(prepared, local, vf, precision, budget);
          require(pieces.empty() || pieces.front().regular_sheet_sign == map.regular_sheet_sign,
                  "cap pieces disagree on original regular sheet");
          const auto ordinal = narrow(pieces.size());
          pieces.push_back({0, found->original, {ordinal, ordinal + 1},
                             {points[ordinal].uv[0], points[ordinal].uv[1], point.uv[0], point.uv[1]},
                             {exact.append(vf[0]), exact.append(vf[1])}, map.positive_chart_roles,
                             map.regular_sheet_sign,
                             {face.id.value, start_interval + ordinal, BoundaryOwnerKind::face_cut}});
        }
        previous = UV{u, v};
      }
    SeamWedge saved = *wedge;
    saved.finite_connector_uv = copy(values, wedge->finite_connector_uv, exact, budget);
    std::array<ExactRef, 4> old_cut;
    for (uint32_t i = 0; i < 4; ++i)
      old_cut[i] = exact.append(full_cut[i]);
    std::optional<std::array<ExactRef, 4>> metric;
    if (request.continuation && request.continuation.fixed_metric_factor())
      metric = copy(request.continuation.values(), *request.continuation.fixed_metric_factor(), exact, budget);
    auto sealed = exact.seal();
    auto control = retained<CapCutControlData>(resource, resource, request.source, request.key, sealed);
    control->old_cut = old_cut;
    control->endpoints = endpoints;
    control->angular_level = level;
    control->revision = request.continuation ? request.continuation.revision() : 0;
    control->next_interval = start_interval + count;
    control->metric = metric;
    auto data = retained<ProducedCapDomainData>(resource, resource, request.source, request.curves,
                                               request.key, sealed);
    data->wedges.push_back(saved);
    data->caps.push_back({0, angular, pole_node, {0, points.size()}, {0, pieces.size()},
                           request.transcendental_bits, request.norm_fractional_bits,
                           {near_interval, near_interval}});
    data->points = std::move(points);
    data->pieces = std::move(pieces);
    data->control = CapCutControlLease(std::move(control));
    result.plan = ProducedCapDomainLease(std::move(data));
    result.continuation = result.plan.control();
    return std::move(result);
  }
  static CapCutControlLease advance(const CapContinuationRequest &r, JobContext &context) {
    require(r.expected && r.expected.caps().size() == 1 && r.expected.control(),
            "cap continuation requires one produced mapped domain");
    require(r.metric_values.has_value() == r.metric_factor.has_value(),
            "cap continuation metric lease/ref mismatch");
    require(r.table_key.generation == r.expected.key().query,
            "cap continuation table generation differs from query");
    require(r.table_key != r.expected.values().key() &&
                r.table_key != r.expected.control().values().key() &&
                (!r.metric_values || r.table_key != r.metric_values->key()),
            "cap continuation exact table aliases a retained input");
    const auto &old = r.expected.control();
    auto resource = r.expected.resource();
    ExactBuilder builder(resource, r.table_key);
    const auto old_cut = copy(old.values(), old.old_cut(), builder, context.budget);
    std::array<PhysicalCapEndpoint, 2> endpoints;
    for (uint32_t i = 0; i < 2; ++i) {
      endpoints[i] = old.endpoints()[i];
      endpoints[i].original_t = builder.append(read(context.budget, old.values(), endpoints[i].original_t));
    }
    auto metric = old.fixed_metric_factor()
                      ? std::optional(copy(old.values(), *old.fixed_metric_factor(), builder, context.budget))
                      : std::nullopt;
    if (r.metric_values) {
      require(r.metric_values->resource() == resource, "cap continuation metric owner mismatch");
      const auto fresh = copy(*r.metric_values, *r.metric_factor, builder, context.budget);
      if (metric)
        for (uint32_t i = 0; i < 4; ++i)
          require(read(context.budget, *r.metric_values, (*r.metric_factor)[i])
                      .compare(read(context.budget, old.values(), (*old.fixed_metric_factor())[i])) == 0,
                  "cap continuation changes fixed face metric");
      metric = fresh;
    }
    auto data = retained<CapCutControlData>(resource, resource, old.source(), r.expected.key(), builder.seal());
    data->old_cut = old_cut;
    data->endpoints = endpoints;
    data->angular_level = old.angular_level();
    data->revision = r.direction == CapCutDirection::retain ? old.revision() : next(old.revision());
    data->next_interval = old.data_->next_interval;
    data->metric = metric;
    if (r.direction == CapCutDirection::angular) {
      if (data->angular_level == UINT32_MAX)
        throw Error(ErrorCode::resource_exhausted, "cap angular level exhausted");
      ++data->angular_level;
    } else if (r.direction == CapCutDirection::radial)
      data->pending = 3;
    else if (r.direction == CapCutDirection::retain)
      data->pending = old.pending_radial_sides();
    else
      throw Error(ErrorCode::invalid_source, "unknown cap cut direction");
    return CapCutControlLease(std::move(data));
  }
};
CapDomainPreparation prepare_cap_domain(const CapDomainRequest &request, JobContext &context) {
  admit_numeric_environment();
  require(request.curves && request.curves->boundary && request.curves->resource,
          "cap plan requires a retained source boundary snapshot");
  require(request.table_key.generation == request.key.query,
          "cap plan table generation differs from query");
  return CapDomainProducer(request, context).run();
}
CapCutControlLease continue_cap_domain(const CapContinuationRequest &request, JobContext &context) {
  admit_numeric_environment();
  return CapDomainProducer::advance(request, context);
}
} // namespace bgmesh::adaptive
