#include "face_engine.hpp"
#include "domain.hpp"
#include "geometry_domain.hpp"
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
  std::pmr::vector<std::optional<SiteSlot>> source_sites;
  std::pmr::vector<std::array<Rational, 2>> original_locations;
  std::pmr::vector<std::optional<std::array<Rational, 4>>> original_enclosures;
  std::optional<std::array<Rational, 4>> plane_factor;
  std::pmr::vector<TopologyEdit> changes;
  std::pmr::vector<int32_t> adapters;
  std::pmr::vector<BoundaryKey> protected_keys;
  FaceCandidateWork candidate;
  std::optional<HeuristicTicket> ticket;
  std::optional<EdgeHandle> midpoint_edge;
  std::optional<TriangleHandle> closure;
  std::optional<BoundaryKey> request;
  struct Potential final {
    uint64_t generation = 0;
    std::optional<double> value;
  };
  std::pmr::vector<Potential> potentials;
  struct DomainFailure final {
    CellResultKey cell;
    BoundaryKey feature;
  };
  std::pmr::vector<std::optional<DomainFailure>> domain_failures;
  struct PoleCell final {
    TriangleHandle triangle;
    uint32_t piece;
  };
  std::pmr::vector<std::optional<PoleCell>> pole_cells;
  FaceOptimizationWork optimization;
  std::optional<FlipTicket> flip;
  uint64_t optimizer_ordinal = 0;
  bool optimizing = false;
  FaceResult result;
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
        source_sites(&b), original_locations(&b), original_enclosures(&b), changes(&b),
        adapters(&b), protected_keys(&b), potentials(&b), domain_failures(&b), pole_cells(&b) {
    if (&context.budget() != &budget)
      invalid("face geometry context uses another owner");
    if (boundary->chart_kind != WorkingChartKind::original_uv &&
        boundary->chart_kind != WorkingChartKind::analytic_unwrapped_uv && !is_plane())
      throw Error(ErrorCode::unsupported, "face map specialization is not connected");
    if (boundary->plane.has_value() != is_plane() || allowance.sign() <= 0 ||
        metric.scales.allowance.compare(tau) != 0)
      invalid("face chart/allowance mismatch");
    if (is_plane())
      plane_factor = make_plane_factor();
    if (boundary->values.key().owner != owner ||
        boundary->values.key().generation != boundary->face_generation)
      invalid("face seed table owner/generation mismatch");
    table_ordinal = boundary->values.key().ordinal;
    if (boundary->construction_domain_present != !boundary->pole_caps.empty() ||
        boundary->pole_caps.empty() != boundary->pole_pieces.empty() || (has_poles() && is_plane()))
      invalid("finite cap/construction domain inventory mismatch");
    nodes.reserve(boundary->sites.size());
    source_sites.resize(boundary->sites.size());
    original_locations.reserve(boundary->sites.size());
    original_enclosures.reserve(boundary->sites.size());
    for (std::size_t index = 0; index < boundary->sites.size(); ++index) {
      budget.charge(1);
      const auto &site = boundary->sites[index];
      if (!has_poles())
        (void)activate_source(static_cast<uint32_t>(index));
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
      validate_uses(loop, original);
      if (loop.segments.first > boundary->segments.size() ||
          loop.segments.count > boundary->segments.size() - loop.segments.first)
        invalid("face seed loop range invalid");
      const auto begin = cycles.size();
      for (uint64_t i = 0; i < loop.segments.count; ++i) {
        budget.charge(1);
        const auto &segment = boundary->segments[static_cast<std::size_t>(loop.segments.first + i)];
        if (segment.site >= boundary->sites.size() || segment.use.value >= source().coedges.size())
          invalid("face seed segment site/use invalid");
        const auto &use = source().coedges[segment.use.value];
        if (use.loop_id != original.id)
          invalid("face seed changes original use loop");
        if (segment.constraint.kind == BoundaryOwnerKind::source_edge &&
            segment.constraint.owner != use.edge_id.value)
          invalid("face source constraint owner mismatch");
        if (!has_poles())
          cycles.push_back(active_source(segment.site));
        constraints.push_back(segment.constraint);
      }
      if (!has_poles())
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
    validate_domain_map();
    if (has_poles())
      initialize_poles(seed);
    else {
      auto domain =
          initialize_face_domain(budget, mesh, loops, seed, DomainFailureMode::report_geometry);
      if (domain.issue)
        request = classify_coarse_domain_issue(budget, *boundary, *domain.issue);
      adapters = std::move(domain.traversal_adapters);
    }
    frontier.initialize();
  }
  bool has_poles() const { return !boundary->pole_caps.empty(); }
  SiteSlot active_source(uint32_t index) const {
    if (index >= source_sites.size())
      invalid("source site outside active mapping");
    const auto &site = source_sites[index];
    if (!site)
      internal("source occurrence has no active topology site");
    return *site;
  }
  SiteSlot activate_source(uint32_t index) {
    if (index >= boundary->sites.size())
      invalid("construction site outside complete source pool");
    if (source_sites[index])
      return active_source(index);
    const auto &site = boundary->sites[index];
    const std::array uv{read(site.chart[0]), read(site.chart[1])};
    const std::array locator{read(site.original_uv[0]), read(site.original_uv[1])};
    for (std::size_t axis = 0; axis < 2; ++axis)
      if (!is_plane() && uv[axis].compare(locator[axis]) != 0)
        invalid("ordinary chart and original UV disagree");
    if (site.node.key.definition != boundary->definition.value)
      invalid("boundary physical node belongs to another definition");
    finite(site.node.xyz);
    const auto slot = mesh.append_site(uv);
    if (slot.value != nodes.size())
      internal("face site/node append mismatch");
    nodes.push_back(site.node);
    source_sites[index] = slot;
    original_locations.push_back(locator);
    if (site.original_uv_enclosure) {
      const auto &refs = *site.original_uv_enclosure;
      std::array bounds{read(refs[0]), read(refs[1]), read(refs[2]), read(refs[3])};
      if (bounds[0].compare(bounds[1]) > 0 || bounds[2].compare(bounds[3]) > 0)
        invalid("boundary source locator enclosure is reversed");
      original_enclosures.emplace_back(std::move(bounds));
    } else
      original_enclosures.push_back(std::nullopt);
    return slot;
  }
  void initialize_poles(uint64_t seed) {
    if (boundary->pole_caps.size() != 1 || !boundary->source_boundary)
      throw Error(ErrorCode::unsupported, "finite cap requires one retained source quotient");
    std::pmr::vector<SiteSlot> cycles(&budget);
    std::pmr::vector<BoundaryKey> constraints(&budget);
    std::pmr::vector<DomainLoopView> loops(&budget);
    cycles.reserve(boundary->construction_segments.size());
    constraints.reserve(boundary->construction_segments.size());
    loops.reserve(boundary->construction_loops.size());
    uint64_t cursor = 0;
    for (const auto &loop : boundary->construction_loops) {
      budget.charge(1);
      if (loop.source_loop.value >= source().loops.size() ||
          source().loops[loop.source_loop.value].face_id != face().id ||
          source().loops[loop.source_loop.value].outer != loop.outer ||
          loop.segments.first != cursor ||
          loop.segments.count > boundary->construction_segments.size() - cursor)
        invalid("finite cap ordinary construction loop ownership/range mismatch");
      const auto begin = cycles.size();
      for (uint64_t i = 0; i < loop.segments.count; ++i) {
        budget.charge(1);
        const auto &segment = boundary->construction_segments[static_cast<std::size_t>(cursor + i)];
        cycles.push_back(activate_source(segment.site));
        constraints.push_back(segment.constraint);
      }
      loops.push_back(
          {loop.outer,
           std::span<const SiteSlot>(cycles.data() + begin,
                                     static_cast<std::size_t>(loop.segments.count)),
           std::span<const BoundaryKey>(constraints.data() + begin,
                                        static_cast<std::size_t>(loop.segments.count))});
      cursor += loop.segments.count;
    }
    if (cursor != boundary->construction_segments.size())
      invalid("unowned finite construction segments");
    if (!loops.empty()) {
      const auto domain =
          initialize_face_domain(budget, mesh, loops, seed, DomainFailureMode::report_geometry,
                                 DomainInitialSeal::deferred);
      if (domain.issue)
        throw Error(ErrorCode::unresolved, "finite cap ordinary remainder is not embedded");
    }
    // Source adapters describe ORIGINAL occurrence traversal, even when the
    // ordinary remainder is empty or its boundary has a different cut chain.
    for (const auto &loop : boundary->loops) {
      Rational area(budget);
      for (uint64_t i = 0; i < loop.segments.count; ++i) {
        budget.charge(1);
        const auto a = boundary->segments[static_cast<std::size_t>(loop.segments.first + i)].site;
        const auto b = boundary
                           ->segments[static_cast<std::size_t>(loop.segments.first +
                                                               (i + 1) % loop.segments.count)]
                           .site;
        const auto &p = boundary->sites[a].chart, &q = boundary->sites[b].chart;
        area = area.add(read(p[0]).multiply(read(q[1])).subtract(read(p[1]).multiply(read(q[0]))));
      }
      if (area.sign() == 0)
        throw Error(ErrorCode::unresolved, "finite cap original chart loop is collapsed");
      adapters.push_back((loop.outer ? 1 : -1) * area.sign());
    }
    const auto &cap = boundary->pole_caps.front();
    if (cap.pieces.first != 0 || cap.pieces.count != boundary->pole_pieces.size() ||
        cap.pieces.count == 0)
      invalid("finite cap piece partition is incomplete");
    for (std::size_t side = 0; side < 2; ++side) {
      const auto range = cap.consumed_meridians[side];
      if (range.count != 1 || range.first >= boundary->segments.size() ||
          boundary->segments[static_cast<std::size_t>(range.first)].use !=
              cap.meridian_uses[side] ||
          boundary->segments[static_cast<std::size_t>(range.first)].constraint !=
              cap.radial_features[side] ||
          cap.radial_features[side].kind != BoundaryOwnerKind::source_edge)
        invalid("finite cap cut must consume the nearest current original meridian interval");
    }
    const auto pole = activate_source(cap.pole_site);
    for (uint32_t i = 0; i < boundary->pole_pieces.size(); ++i) {
      budget.charge(1);
      const auto &piece = boundary->pole_pieces[i];
      if (piece.cap != 0 ||
          (piece.cut.kind == BoundaryOwnerKind::face_cut && piece.cut.owner != face().id.value))
        invalid("finite fan piece owner mismatch");
      const std::array roles{pole, activate_source(piece.cut_sites[0]),
                             activate_source(piece.cut_sites[1])};
      std::array<SiteSlot, 3> sites{};
      std::array<bool, 3> used{};
      for (uint32_t j = 0; j < 3; ++j) {
        const auto role = piece.positive_chart_roles[j];
        if (role >= 3 || used[role])
          invalid("finite fan role permutation invalid");
        used[role] = true;
        sites[j] = roles[role];
      }
      for (std::size_t endpoint = 0; endpoint < 2; ++endpoint)
        for (std::size_t axis = 0; axis < 2; ++axis)
          if (read(piece.cut_uv[2 * endpoint + axis])
                  .compare(mesh.coordinate(roles[endpoint + 1],
                                           axis == 0 ? CoordinateAxis::x : CoordinateAxis::y)) != 0)
            invalid("finite fan cut sites disagree with source coverage map");
      if (i != 0 && boundary->pole_pieces[i - 1].cut_sites[1] != piece.cut_sites[0])
        invalid("finite fan cut chain has gaps or changed shared nodes");
      const auto triangle = mesh.add_initial_triangle(sites);
      if (triangle.slot >= pole_cells.size())
        pole_cells.resize(static_cast<std::size_t>(triangle.slot) + 1);
      pole_cells[triangle.slot] = PoleCell{triangle, i};
      const auto protect = [&](SiteSlot a, SiteSlot b, BoundaryKey key) {
        const auto edge = mesh.find_edge(a, b);
        if (!edge)
          internal("finite fan source edge absent");
        mesh.protect_edge(*edge, key);
        protected_keys.push_back(key);
      };
      protect(roles[1], roles[2], piece.cut);
      if (i == 0)
        protect(roles[0], roles[1], cap.radial_features[0]);
      if (i + 1 == boundary->pole_pieces.size())
        protect(roles[0], roles[2], cap.radial_features[1]);
      const auto cut_edge = mesh.find_edge(roles[1], roles[2]);
      if (!cut_edge || mesh.edge(*cut_edge).incident_count !=
                           (piece.cut.kind == BoundaryOwnerKind::face_cut ? 2u : 1u))
        invalid(
            "finite cut source/interior provenance disagrees with ordinary remainder incidence");
    }
    std::sort(protected_keys.begin(), protected_keys.end(), [&](auto a, auto b) {
      budget.charge(1);
      return a < b;
    });
    protected_keys.erase(std::unique(protected_keys.begin(), protected_keys.end()),
                         protected_keys.end());
    validate_pole_source_coverage();
    mesh.seal_initial();
  }
  void validate_pole_source_coverage() {
    struct CoverageEdge final {
      BoundaryKey key;
      std::array<PhysicalNodeKey, 2> nodes;
      auto operator<=>(const CoverageEdge &) const = default;
    };
    std::pmr::vector<CoverageEdge> expected(&budget), actual(&budget);
    const auto add = [&](auto &rows, BoundaryKey key, std::array<uint32_t, 2> endpoints) {
      budget.charge(1);
      const auto first = endpoints[0], second = endpoints[1];
      if (first >= boundary->sites.size() || second >= boundary->sites.size())
        invalid("finite source coverage site outside complete inventory");
      auto a = boundary->sites[first].node.key, b = boundary->sites[second].node.key;
      if (b < a)
        std::swap(a, b);
      rows.push_back({key, {a, b}});
    };
    for (const auto &loop : boundary->loops)
      for (uint64_t i = 0; i < loop.segments.count; ++i) {
        const auto &segment = boundary->segments[static_cast<std::size_t>(loop.segments.first + i)];
        const auto &use = source().coedges[segment.use.value];
        if (source().edges[source().edge_index.find(use.edge_id).value].degenerate)
          continue;
        const auto &next = boundary->segments[static_cast<std::size_t>(
            loop.segments.first + (i + 1) % loop.segments.count)];
        add(expected, segment.constraint, {segment.site, next.site});
      }
    for (const auto &loop : boundary->construction_loops)
      for (uint64_t i = 0; i < loop.segments.count; ++i) {
        const auto &segment =
            boundary->construction_segments[static_cast<std::size_t>(loop.segments.first + i)];
        const auto &next = boundary->construction_segments[static_cast<std::size_t>(
            loop.segments.first + (i + 1) % loop.segments.count)];
        if (segment.constraint.kind == BoundaryOwnerKind::source_edge)
          add(actual, segment.constraint, {segment.site, next.site});
      }
    for (const auto &cap : boundary->pole_caps) {
      const auto &first = boundary->pole_pieces[static_cast<std::size_t>(cap.pieces.first)];
      const auto &last =
          boundary->pole_pieces[static_cast<std::size_t>(cap.pieces.first + cap.pieces.count - 1)];
      add(actual, cap.radial_features[0], {cap.pole_site, first.cut_sites[0]});
      add(actual, cap.radial_features[1], {cap.pole_site, last.cut_sites[1]});
    }
    for (const auto &piece : boundary->pole_pieces)
      if (piece.cut.kind == BoundaryOwnerKind::source_edge)
        add(actual, piece.cut, {piece.cut_sites[0], piece.cut_sites[1]});
    const auto sort = [&](auto &rows) {
      std::sort(rows.begin(), rows.end(), [&](const auto &a, const auto &b) {
        budget.charge(1);
        return a < b;
      });
    };
    sort(expected);
    sort(actual);
    budget.charge(std::max(expected.size(), actual.size()));
    if (expected != actual)
      invalid("finite construction omitted or invented original physical source segments");
  }
  std::optional<uint32_t> pole_piece(TriangleHandle triangle) const {
    if (triangle.slot < pole_cells.size()) {
      const auto &cell = pole_cells[triangle.slot];
      if (cell && cell->triangle == triangle)
        return cell->piece;
    }
    return std::nullopt;
  }
  PoleFanPlan evaluate_pole(uint32_t index) {
    const auto &piece = boundary->pole_pieces[index];
    const auto &cap = boundary->pole_caps[piece.cap];
    const std::array roles{active_source(cap.pole_site), active_source(piece.cut_sites[0]),
                           active_source(piece.cut_sites[1])};
    std::array<double, 9> xyz{};
    for (std::size_t corner = 0; corner < 3; ++corner)
      for (std::size_t axis = 0; axis < 3; ++axis)
        xyz[3 * corner + axis] = nodes[roles[corner].value].xyz[axis];
    const auto plan =
        bound_analytic_pole_fan(context, source().carriers[carrier.value],
                                {cap.kind,
                                 {read(piece.cut_uv[0]), read(piece.cut_uv[1]),
                                  read(piece.cut_uv[2]), read(piece.cut_uv[3])},
                                 xyz,
                                 {read(cap.original_pole_v[0]), read(cap.original_pole_v[1])},
                                 1,
                                 cap.precision});
    if (plan.facet_roles != piece.positive_chart_roles)
      invalid("finite cap map orientation differs from installed positive chart roles");
    if (plan.regular_sheet_sign != cap.regular_sheet_sign)
      invalid("finite cap sheet differs from its original regular source branch");
    return plan;
  }
  BoundaryKey pole_refinement(uint32_t index, const PoleFanPlan &plan) const {
    const auto &piece = boundary->pole_pieces[index];
    const auto &cap = boundary->pole_caps[piece.cap];
    // These are the actual same-map Hessian contributions. Corner/band terms
    // are never assumed to be removable by angular subdivision.
    return plan.angular_upper.compare(plan.radial_upper) >= 0
               ? piece.cut
               : std::min(cap.radial_features[0], cap.radial_features[1]);
  }
  void validate_domain_map() const {
    bool authoritative = false, reconstructed = false;
    for (const auto &item : boundary->uses) {
      budget.charge(1);
      const auto &use = source().coedges[item.source_use.value];
      authoritative = authoritative || !use.reconstructed;
      reconstructed = reconstructed || use.reconstructed;
    }
    const auto &map = boundary->domain_map;
    if (!authoritative) {
      if (map)
        invalid("reconstructed face has an unexpected source-domain map");
      return;
    }
    if (!map)
      throw Error(ErrorCode::unsupported, "authoritative face lacks its actual domain map");
    if (map->resource != boundary->resource || map->values.resource() != boundary->resource ||
        &map->geometry.view() != &boundary->geometry.view() ||
        boundary->source_generation != boundary->geometry.view().table.key().generation ||
        map->definition != boundary->definition || map->face != boundary->face ||
        map->generations != std::array<uint64_t, 3>{boundary->source_generation,
                                                    boundary->face_generation,
                                                    boundary->boundary_generation} ||
        map->values.key().owner != owner ||
        map->values.key().generation != boundary->face_generation ||
        map->values.key().ordinal >= boundary->values.key().ordinal ||
        map->policy != (reconstructed ? DomainReferencePolicy::hybrid_pullback
                                      : DomainReferencePolicy::authoritative_original))
      invalid("source-domain map escaped its actual face generation");
    if (has_poles() && (!map->fans.empty() || !map->strips.empty()))
      throw Error(ErrorCode::unsupported,
                  "finite source cap requires named identity authoritative domain map");
  }
  DomainDisplacement domain_displacement(const std::array<Rational, 6> &chart) {
    if (!boundary->domain_map)
      return {Rational(budget), std::nullopt};
    auto displacement = bound_domain_displacement(context, boundary->domain_map, chart);
    if (&displacement.upper.budget() != &budget || displacement.upper.sign() < 0)
      internal("source-domain displacement has an invalid owner or sign");
    if (displacement.feature && !std::binary_search(protected_keys.begin(), protected_keys.end(),
                                                    *displacement.feature, [&](auto a, auto b) {
                                                      budget.charge(1);
                                                      return a < b;
                                                    }))
      internal("source-domain displacement names an unknown boundary feature");
    return displacement;
  }
  void forget_domain_failure(TriangleHandle triangle) {
    if (triangle.slot < domain_failures.size()) {
      auto &entry = domain_failures[triangle.slot];
      if (entry && entry->cell.triangle == triangle)
        entry.reset();
    }
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
  void original_vertex(uint32_t site, VertexId expected) {
    if (site >= boundary->sites.size())
      invalid("source use endpoint site out of range");
    const auto &node = boundary->sites[site].node;
    auto representative = source().vertex_index.find(expected);
    auto generator = NodeGenerator::original_vertex;
    if (const auto &master = boundary->source_boundary) {
      if (&master->source.view() != &source() || master->definition != boundary->definition ||
          master->face != boundary->face ||
          master->boundary_generation != boundary->boundary_generation)
        invalid("face source quotient snapshot belongs to another boundary generation");
      if (master->vertex_alias_owner
              ? (master->vertex_aliases.data() != master->vertex_alias_owner->rows.data() ||
                 master->vertex_aliases.size() != master->vertex_alias_owner->rows.size())
              : !master->vertex_aliases.empty())
        invalid("source quotient relation lost its immutable storage lease");
      const auto &aliases = master->vertex_aliases;
      const auto it = std::lower_bound(aliases.begin(), aliases.end(), representative,
                                       [&](const auto &row, auto value) {
                                         budget.charge(1);
                                         return row.original < value;
                                       });
      if (it != aliases.end() && it->original == representative) {
        representative = it->representative;
        generator = NodeGenerator::declared_quotient;
      }
    }
    if (representative.value >= source().vertices.size())
      invalid("source quotient representative outside source");
    const auto &vertex = source().vertices[representative.value];
    const PhysicalNodeKey key{boundary->definition.value, generator, vertex.id.value, 0, 0};
    if (node.key != key || node.xyz != vertex.xyz)
      invalid("source use endpoint lost original physical vertex authority");
  }
  void validate_uses(const FaceBoundaryLoop &loop, const Loop &original) {
    if (loop.uses.first > boundary->uses.size() ||
        loop.uses.count > boundary->uses.size() - loop.uses.first ||
        loop.uses.count != original.coedges.count ||
        loop.segments.first > boundary->segments.size() ||
        loop.segments.count > boundary->segments.size() - loop.segments.first)
      invalid("source use inventory/range mismatch");
    auto cursor = loop.segments.first;
    const auto end = loop.segments.first + loop.segments.count;
    for (uint64_t ui = 0; ui < loop.uses.count; ++ui) {
      budget.charge(1);
      const auto &disposition = boundary->uses[static_cast<std::size_t>(loop.uses.first + ui)];
      const auto source_id =
          source().coedge_ids[static_cast<std::size_t>(original.coedges.first + ui)];
      const auto expected = source().coedge_index.find(source_id);
      if (disposition.source_use != expected)
        invalid("source coedge order/inventory changed");
      const bool quotient = disposition.kind == FaceUseKind::declared_quotient;
      if (disposition.kind != FaceUseKind::ordinary_chain && !quotient)
        invalid("unknown source use disposition");
      if (disposition.segments.first != cursor || disposition.segments.count == 0 ||
          disposition.segments.count > end - cursor)
        invalid("source coedge segment partition has gaps/overlap");
      const auto &use = source().coedges[expected.value];
      const auto &edge = source().edges[source().edge_index.find(use.edge_id).value];
      if (use.sense != 1 && use.sense != -1)
        invalid("source use sense invalid");
      if (!quotient && (edge.degenerate || edge.parameter_range[0] == edge.parameter_range[1]))
        throw Error(ErrorCode::unsupported,
                    "degenerate source use requires explicit quotient disposition");
      const auto from = use.sense == 1 ? 0u : 1u, to = 1u - from;
      const auto begin = Rational::from_double(budget, edge.parameter_range[from]);
      const auto finish = Rational::from_double(budget, edge.parameter_range[to]);
      if (read(disposition.original_parameter[0]).compare(begin) != 0 ||
          read(disposition.original_parameter[1]).compare(finish) != 0)
        invalid("source use t endpoints changed or curve sense applied twice");
      auto current = begin;
      const auto direction = finish.compare(begin);
      original_vertex(boundary->segments[static_cast<std::size_t>(cursor)].site,
                      edge.vertex_ids[from]);
      if (quotient) {
        if (!edge.degenerate || !has_poles() || boundary->pole_caps.size() != 1 ||
            boundary->pole_caps.front().quotient_use != expected ||
            disposition.segments.count != 1 || !disposition.quotient_pcurve_parameter)
          throw Error(ErrorCode::unsupported, "quotient use lacks its finite source cap coverage");
        const auto pcurve_from = ((use.sense == 1) == edge.curve_same_sense) ? 0u : 1u;
        const auto &pc = *disposition.quotient_pcurve_parameter;
        if (read(pc[0]).compare(Rational::from_double(budget, use.parameter_range[pcurve_from])) !=
                0 ||
            read(pc[1]).compare(
                Rational::from_double(budget, use.parameter_range[1 - pcurve_from])) != 0)
          invalid("source quotient pcurve traversal is incomplete or applies ordinary sense");
        const auto &segment = boundary->segments[static_cast<std::size_t>(cursor)];
        if (segment.use != expected || read(segment.source_parameter[0]).compare(begin) != 0 ||
            read(segment.source_parameter[1]).compare(finish) != 0)
          invalid("source quotient spatial endpoint inventory changed");
        ++cursor;
        const auto next_segment = cursor == end ? loop.segments.first : cursor;
        original_vertex(boundary->segments[static_cast<std::size_t>(next_segment)].site,
                        edge.vertex_ids[to]);
        continue;
      }
      if (disposition.quotient_pcurve_parameter)
        invalid("ordinary use has quotient pcurve coverage");
      for (uint64_t si = 0; si < disposition.segments.count; ++si) {
        budget.charge(1);
        const auto &segment = boundary->segments[static_cast<std::size_t>(cursor + si)];
        if (segment.use != expected || read(segment.source_parameter[0]).compare(current) != 0)
          invalid("source spatial edge schedule has gaps/changed uses");
        const auto next = read(segment.source_parameter[1]);
        if (next.compare(current) != direction ||
            (direction > 0 ? next.compare(finish) > 0 : next.compare(finish) < 0))
          invalid("source spatial schedule reverses or exceeds original interval");
        if (si != 0) {
          if (segment.site >= boundary->sites.size())
            invalid("source edge sample site invalid");
          const auto &node = boundary->sites[segment.site].node;
          if (node.key.generator != NodeGenerator::original_edge ||
              node.key.source != edge.id.value || node.key.generation != 0 ||
              !node.original_parameter || read(*node.original_parameter).compare(current) != 0)
            invalid("interior source edge sample lost physical key/original t");
        }
        current = next;
      }
      if (current.compare(finish) != 0)
        invalid("source spatial edge coverage incomplete");
      cursor += disposition.segments.count;
      const auto next_segment = cursor == end ? loop.segments.first : cursor;
      original_vertex(boundary->segments[static_cast<std::size_t>(next_segment)].site,
                      edge.vertex_ids[to]);
    }
    if (cursor != end)
      invalid("selected loop has segments outside source use inventory");
  }
  Rational read(ExactRef ref) { return import_exact(budget, boundary->values.view(ref)); }
  bool is_plane() const {
    return boundary->chart_kind == WorkingChartKind::physical_plane_convex_q;
  }
  CellMapKind map_kind() const {
    return is_plane() ? CellMapKind::plane_xy_triangle : CellMapKind::uv_triangle;
  }
  std::array<Rational, 4> make_plane_factor() {
    const auto &profile = boundary->plane;
    if (!profile)
      invalid("plane face lacks admitted source profile");
    const auto &p = *profile;
    const std::array<ExactEnclosure, 3> zero{ExactEnclosure(Rational(budget)),
                                             ExactEnclosure(Rational(budget)),
                                             ExactEnclosure(Rational(budget))};
    const std::array<ExactEnclosure, 3> du{ExactEnclosure(read(p.basis[0])),
                                           ExactEnclosure(read(p.basis[2])),
                                           ExactEnclosure(read(p.basis[4]))};
    const std::array<ExactEnclosure, 3> dv{ExactEnclosure(read(p.basis[1])),
                                           ExactEnclosure(read(p.basis[3])),
                                           ExactEnclosure(read(p.basis[5]))};
    return metric_from_point_jet({zero, du, dv, zero, zero, zero},
                                 {h2, width_u, width_v, allowance});
  }
  void check() const {
    if (!usable)
      internal("face generation has failed");
  }
  void mutable_check() const {
    check();
    if (result)
      invalid("face generation has been frozen");
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
  std::array<Rational, 6> chart_coordinates(TriangleHandle handle) const {
    return chart_coordinates(mesh.triangle(handle).sites);
  }
  std::array<Rational, 6> chart_coordinates(const std::array<SiteSlot, 3> &sites) const {
    return {
        mesh.coordinate(sites[0], CoordinateAxis::x), mesh.coordinate(sites[0], CoordinateAxis::y),
        mesh.coordinate(sites[1], CoordinateAxis::x), mesh.coordinate(sites[1], CoordinateAxis::y),
        mesh.coordinate(sites[2], CoordinateAxis::x), mesh.coordinate(sites[2], CoordinateAxis::y)};
  }
  std::array<Rational, 4> local_factor(std::span<const Rational, 6> coordinates) {
    if (plane_factor)
      return *plane_factor;
    const Rational three(budget, 3);
    const std::array center{coordinates[0].add(coordinates[2]).add(coordinates[4]).divide(three),
                            coordinates[1].add(coordinates[3]).add(coordinates[5]).divide(three)};
    return original_source_metric(context, boundary->geometry, carrier, center,
                                  {h2, width_u, width_v, allowance});
  }
  std::optional<double> potential(const std::array<SiteSlot, 3> &sites) {
    const auto coordinates = chart_coordinates(sites);
    try {
      const auto local = local_factor(coordinates);
      return metric_shape_potential(coordinates, local);
    } catch (const Error &error) {
      if (error.code == ErrorCode::unsupported)
        return std::nullopt;
      throw;
    }
  }
  std::optional<double> potential(TriangleHandle handle) {
    if (handle.slot >= potentials.size()) {
      grow(potentials, static_cast<std::size_t>(handle.slot) + 1 - potentials.size());
      potentials.resize(static_cast<std::size_t>(handle.slot) + 1);
    }
    auto &cached = potentials[handle.slot];
    if (cached.generation != handle.generation) {
      cached.value = potential(mesh.triangle(handle).sites);
      cached.generation = handle.generation;
    }
    return cached.value;
  }
  DeclaredContactEdge declaration(SiteSlot a, SiteSlot b) const {
    const auto handle = mesh.find_edge(a, b);
    if (handle) {
      const auto edge = mesh.edge(*handle);
      if (edge.constraint) {
        const auto &key = *edge.constraint;
        return {key.kind == BoundaryOwnerKind::source_edge ? ContactEdgeKind::source_edge
                                                           : ContactEdgeKind::face_cut,
                key.owner,
                key.kind == BoundaryOwnerKind::source_edge ? 0 : boundary->face_generation,
                key.interval};
      }
    }
    const auto low = std::min(a.value, b.value), high = std::max(a.value, b.value);
    return {ContactEdgeKind::internal_face, owner.id, boundary->face_generation,
            (uint64_t{low} << 32) | high};
  }
  std::array<DeclaredContactEdge, 3> declarations(const std::array<SiteSlot, 3> &sites) const {
    return {declaration(sites[0], sites[1]), declaration(sites[1], sites[2]),
            declaration(sites[2], sites[0])};
  }
  FaceCandidateWork make_candidate(CellResultKey key, FaceCandidateKind kind,
                                   const std::array<Rational, 2> &point) {
    if (candidate_ordinal == UINT64_MAX)
      throw Error(ErrorCode::resource_exhausted, "face candidate ordinal exhausted");
    ExactBuilder builder(boundary->resource, next_table());
    const std::array refs{builder.append(point[0]), builder.append(point[1])};
    auto locator = refs;
    if (is_plane()) {
      const auto sites = mesh.triangle(key.triangle).sites;
      const Rational three(budget, 3);
      for (std::size_t axis = 0; axis < 2; ++axis)
        locator[axis] = builder.append(original_locations[sites[0].value][axis]
                                           .add(original_locations[sites[1].value][axis])
                                           .add(original_locations[sites[2].value][axis])
                                           .divide(three));
    }
    auto values = builder.seal();
    const auto result_key = is_plane() ? std::optional(next_table()) : std::nullopt;
    auto work = std::allocate_shared<FaceCandidateData>(
        RunAllocator<FaceCandidateData>(boundary->resource),
        FaceCandidateData{boundary, key, ++candidate_ordinal, kind, std::move(values), refs,
                          locator, result_key});
    ++counters.candidates;
    return work;
  }
  FaceAdvance boundary_request(BoundaryKey key, CellResultKey target) {
    request = key;
    return {FaceAdvanceKind::boundary_request, target, {}, key};
  }
  struct ImportedLocator final {
    std::array<Rational, 2> chosen;
    std::optional<std::array<Rational, 4>> enclosure;
  };
  ImportedLocator candidate_location(const FaceCandidateResult &completion) {
    if (!candidate)
      internal("candidate location without staged request");
    if (!is_plane()) {
      if (completion.locator || candidate->result_table_key)
        invalid("ordinary candidate returned a foreign locator");
      return {{import_exact(budget, candidate->values.view(candidate->original_uv[0])),
               import_exact(budget, candidate->values.view(candidate->original_uv[1]))},
              std::nullopt};
    }
    const auto &reserved_key = candidate->result_table_key;
    if (!completion.locator || !reserved_key)
      invalid("plane candidate lacks its selected original source inverse locator");
    const auto &locator = *completion.locator;
    if (locator.values.key() != *reserved_key)
      invalid("plane candidate returned a foreign reserved locator table");
    std::array point{import_exact(budget, locator.values.view(locator.uv[0])),
                     import_exact(budget, locator.values.view(locator.uv[1]))};
    std::array enclosure{import_exact(budget, locator.values.view(locator.enclosure[0])),
                         import_exact(budget, locator.values.view(locator.enclosure[1])),
                         import_exact(budget, locator.values.view(locator.enclosure[2])),
                         import_exact(budget, locator.values.view(locator.enclosure[3]))};
    for (std::size_t axis = 0; axis < 2; ++axis) {
      const auto &lo = enclosure[2 * axis], &hi = enclosure[2 * axis + 1];
      if (lo.compare(hi) > 0 ||
          point[axis].compare(read(boundary->prepared_original_rectangle[2 * axis])) < 0 ||
          point[axis].compare(read(boundary->prepared_original_rectangle[2 * axis + 1])) > 0)
        invalid("plane source locator/enclosure leaves prepared profile");
    }
    return {std::move(point), std::move(enclosure)};
  }
  bool current_target(CellResultKey key) const {
    return mesh.is_live(key.triangle) && frontier.key(key.triangle) == key;
  }
  void fold_changes() {
    auto edits = mesh.take_changes();
    grow(changes, edits.size());
    for (const auto &edit : edits)
      for (uint32_t i = 0; i < edit.removed_count; ++i)
        forget_domain_failure(edit.removed[i]);
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
  impl_->mutable_check();
  if (impl_->optimizing)
    return std::nullopt;
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
      if (const auto piece = impl_->pole_piece(key.triangle)) {
        const auto plan = impl_->evaluate_pole(*piece);
        ++impl_->counters.host_pole_cells;
        const bool accepted =
            plan.position_upper.add(plan.band_upper).compare(impl_->allowance) <= 0;
        const auto &cap = impl_->boundary->pole_caps[impl_->boundary->pole_pieces[*piece].cap];
        if (cap.fixed_pole_upper.has_value() != cap.fixed_band_upper.has_value())
          invalid("finite cap fixed source profile is incomplete");
        if (cap.fixed_pole_upper && cap.fixed_band_upper) {
          const auto pole = impl_->read(*cap.fixed_pole_upper),
                     band = impl_->read(*cap.fixed_band_upper);
          if (pole.sign() < 0 || band.sign() < 0)
            invalid("finite cap source floor has negative bound");
          const auto fixed =
              (pole.compare(plan.pole_departure_upper) > 0 ? pole : plan.pole_departure_upper)
                  .add(band);
          if (!accepted && fixed.compare(impl_->allowance) >= 0) {
            if (!impl_->frontier.complete(key, CellNumericalState::unresolved))
              internal("finite cap fixed profile lost its canonical pending cell");
            continue;
          }
        } else if (!accepted && plan.pole_departure_upper.compare(impl_->allowance) >= 0) {
          if (!impl_->frontier.complete(key, CellNumericalState::unresolved))
            internal("finite cap fixed pole departure lost its pending cell");
          continue;
        }
        if (!accepted) {
          if (key.triangle.slot >= impl_->domain_failures.size())
            impl_->domain_failures.resize(static_cast<std::size_t>(key.triangle.slot) + 1);
          impl_->domain_failures[key.triangle.slot] =
              Impl::DomainFailure{key, impl_->pole_refinement(*piece, plan)};
        }
        if (!impl_->frontier.complete(key, accepted ? CellNumericalState::accepted
                                                    : CellNumericalState::needs_refine))
          internal("finite pole result lost its canonical pending cell");
        continue;
      }
      const auto displacement = impl_->domain_displacement(impl_->chart_coordinates(key.triangle));
      const auto remaining = impl_->allowance.subtract(displacement.upper);
      if (remaining.sign() <= 0) {
        if (!displacement.feature)
          internal("exhausted source-domain allowance lacks a boundary feature");
        if (key.triangle.slot >= impl_->domain_failures.size())
          impl_->domain_failures.resize(static_cast<std::size_t>(key.triangle.slot) + 1);
        impl_->domain_failures[key.triangle.slot] = Impl::DomainFailure{key, *displacement.feature};
        if (!impl_->frontier.complete(key, CellNumericalState::needs_refine))
          internal("source-domain result lost its pending cell");
        ++impl_->counters.domain_failures;
        continue;
      }
      FaceCellInput input{impl_->header(key), impl_->carrier, impl_->map_kind(), {}, {}, {}};
      for (std::size_t corner = 0; corner < 3; ++corner) {
        const auto site = triangle.sites[corner];
        input.chart_xy[2 * corner] =
            builder.append(impl_->mesh.coordinate(site, CoordinateAxis::x));
        input.chart_xy[2 * corner + 1] =
            builder.append(impl_->mesh.coordinate(site, CoordinateAxis::y));
        for (std::size_t axis = 0; axis < 2; ++axis)
          input.original_uv[2 * corner + axis] =
              impl_->is_plane() ? builder.append(impl_->original_locations[site.value][axis])
                                : input.chart_xy[2 * corner + axis];
        const auto &node = impl_->nodes[site.value];
        for (std::size_t axis = 0; axis < 3; ++axis)
          input.actual_xyz[3 * corner + axis] =
              builder.append(Rational::from_double(*budget_, node.xyz[axis]));
      }
      input.allowance = builder.append(remaining);
      rows.push_back(input);
    }
    if (rows.empty())
      return std::nullopt;
    auto work = std::allocate_shared<FaceCellWorkData>(
        RunAllocator<FaceCellWorkData>(impl_->boundary->resource), impl_->boundary->resource,
        impl_->boundary->geometry, builder.seal(), impl_->boundary);
    work->cells.assign(rows.begin(), rows.end());
    ++impl_->counters.cell_packets;
    return FaceCellWork(work);
  } catch (...) {
    impl_->usable = false;
    throw;
  }
}
bool FaceEngine::complete(const CellBoundResult &result) {
  impl_->mutable_check();
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
  impl_->mutable_check();
  if (impl_->optimizing)
    return {FaceAdvanceKind::settled, {}, {}, {}};
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
      if (selected->triangle.slot < impl_->domain_failures.size()) {
        const auto &failure = impl_->domain_failures[selected->triangle.slot];
        if (failure && failure->cell == *selected)
          return impl_->boundary_request(failure->feature, *selected);
      }
      if (const auto piece = impl_->pole_piece(selected->triangle))
        return impl_->boundary_request(impl_->boundary->pole_pieces[*piece].cut, *selected);
      const auto triangle = impl_->mesh.triangle(selected->triangle);
      if (!triangle.heuristic_spent) {
        const auto uv = impl_->chart_coordinates(selected->triangle);
        const auto local = impl_->local_factor(uv);
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
  impl_->mutable_check();
  if (!impl_->candidate || result.target != impl_->candidate->target ||
      result.ordinal != impl_->candidate->ordinal || !impl_->current_target(result.target)) {
    ++impl_->counters.rejected_results;
    return false;
  }
  // Validate immutable response identity/locator before entering a topology
  // commit transaction. A foreign reply must not destroy its pending ticket.
  std::optional<Impl::ImportedLocator> location;
  if (result.decision == FaceCandidateDecision::admitted) {
    if (result.feature)
      invalid("admitted candidate also requests a feature");
    Impl::finite(result.xyz);
    location.emplace(impl_->candidate_location(result));
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
    if (!location)
      internal("admitted candidate location missing");
    if (impl_->node_ordinal_base > UINT64_MAX - impl_->nodes.size())
      throw Error(ErrorCode::resource_exhausted, "face node creation ordinal exhausted");
    const auto ordinal = impl_->node_ordinal_base + impl_->nodes.size();
    const PhysicalNode node{{impl_->boundary->definition.value, NodeGenerator::face_site,
                             impl_->owner.id, impl_->boundary->face_generation, ordinal},
                            result.xyz,
                            {}};
    grow(impl_->nodes, 1);
    grow(impl_->original_locations, 1);
    grow(impl_->original_enclosures, 1);
    SiteSlot site;
    if (impl_->ticket)
      site = impl_->mesh.commit_heuristic(*impl_->ticket);
    else if (impl_->midpoint_edge)
      site = impl_->mesh.bisect_internal_edge(*impl_->midpoint_edge);
    else
      internal("candidate lacks owned topology transaction");
    if (site.value != impl_->nodes.size() || site.value != impl_->original_locations.size() ||
        site.value != impl_->original_enclosures.size())
      internal("committed face site/node mismatch");
    impl_->nodes.push_back(node);
    impl_->original_locations.push_back(std::move(location->chosen));
    impl_->original_enclosures.push_back(std::move(location->enclosure));
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
  impl_->mutable_check();
  if (impl_->optimizing)
    invalid("refinement cannot resume during fixed-site optimization");
  if (impl_->candidate)
    invalid("cannot retry while a candidate transaction is staged");
  if (!impl_->current_target(key))
    invalid("stale face retry request");
  impl_->forget_domain_failure(key.triangle);
  return impl_->frontier.retry(key.triangle);
}
bool FaceEngine::mark_contact_failure(CellResultKey key) {
  impl_->mutable_check();
  if (impl_->optimizing)
    invalid("fixed-site optimizer requires settled current contacts");
  if (impl_->candidate || impl_->closure || impl_->request)
    invalid("contact epoch requires settled face construction");
  return impl_->frontier.mark_contact_failure(key);
}
std::pmr::vector<TopologyEdit> FaceEngine::take_changes() {
  impl_->mutable_check();
  std::pmr::vector<TopologyEdit> out(budget_);
  out.swap(impl_->changes);
  return out;
}
std::pmr::vector<CellResultKey> FaceEngine::take_settled() {
  impl_->mutable_check();
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
std::array<DeclaredContactEdge, 3> FaceEngine::declared_edges(TriangleHandle handle) const {
  impl_->check();
  return impl_->declarations(impl_->mesh.triangle(handle).sites);
}
std::pmr::vector<EdgeHandle> FaceEngine::begin_optimizer() {
  impl_->mutable_check();
  if (impl_->optimizing || impl_->candidate || impl_->request || impl_->closure ||
      impl_->frontier.next())
    invalid("optimizer requires settled construction exactly once");
  auto edges = impl_->mesh.internal_edges();
  impl_->optimizing = true;
  return edges;
}
uint64_t FaceEngine::optimizer_edge_key(EdgeHandle handle) const {
  impl_->check();
  const auto sites = impl_->mesh.edge(handle).sites;
  return (uint64_t{std::min(sites[0].value, sites[1].value)} << 32) |
         std::max(sites[0].value, sites[1].value);
}
std::optional<FaceOptimizationWork> FaceEngine::prepare_flip(EdgeHandle edge) {
  impl_->mutable_check();
  if (!impl_->optimizing || impl_->optimization)
    invalid("optimizer proposal state mismatch");
  if (!impl_->mesh.is_live(edge))
    return std::nullopt;
  const auto incident = impl_->mesh.edge(edge);
  for (uint32_t i = 0; i < incident.incident_count; ++i)
    if (impl_->pole_piece(incident.incidents[i]))
      return std::nullopt;
  const auto ticket = impl_->mesh.stage_flip(edge);
  if (!ticket)
    return std::nullopt;
  impl_->flip = ticket;
  try {
    const auto sites = impl_->mesh.flip_sites(*ticket);
    const auto old = impl_->mesh.flip_cells(*ticket);
    const auto before0 = impl_->potential(old[0]), before1 = impl_->potential(old[1]);
    const auto after0 = impl_->potential(sites[0]), after1 = impl_->potential(sites[1]);
    auto cancel = [&]() {
      impl_->mesh.cancel_flip(*impl_->flip);
      impl_->flip.reset();
    };
    if (!before0 || !before1 || !after0 || !after1) {
      cancel();
      return std::nullopt;
    }
    const auto old_sum =
        Rational::from_double(*budget_, *before0).add(Rational::from_double(*budget_, *before1));
    const auto new_sum =
        Rational::from_double(*budget_, *after0).add(Rational::from_double(*budget_, *after1));
    if (new_sum.compare(old_sum) >= 0) {
      cancel();
      return std::nullopt;
    }
    if (impl_->optimizer_ordinal >= ((uint64_t{1} << 62) - 1))
      throw Error(ErrorCode::resource_exhausted, "optimizer temporary cell serial exhausted");
    const auto ordinal = ++impl_->optimizer_ordinal;
    ExactBuilder builder(impl_->boundary->resource, impl_->next_table());
    std::array<FaceCellInput, 2> inputs{};
    std::array<std::array<PhysicalNode, 3>, 2> physical{};
    for (std::size_t i = 0; i < 2; ++i) {
      const auto displacement = impl_->domain_displacement(impl_->chart_coordinates(sites[i]));
      const auto remaining = impl_->allowance.subtract(displacement.upper);
      if (remaining.sign() <= 0) {
        cancel();
        return std::nullopt;
      }
      auto &input = inputs[i];
      input.header = impl_->header(impl_->frontier.key(old[i]));
      input.header.key.cell = (uint64_t{1} << 63) | (ordinal * 2 + i);
      input.header.key.generation = edge.generation;
      input.carrier = impl_->carrier;
      input.map_kind = impl_->map_kind();
      for (std::size_t c = 0; c < 3; ++c) {
        const auto site = sites[i][c];
        physical[i][c] = impl_->nodes[site.value];
        input.chart_xy[2 * c] = builder.append(impl_->mesh.coordinate(site, CoordinateAxis::x));
        input.chart_xy[2 * c + 1] = builder.append(impl_->mesh.coordinate(site, CoordinateAxis::y));
        for (std::size_t axis = 0; axis < 2; ++axis)
          input.original_uv[2 * c + axis] =
              impl_->is_plane() ? builder.append(impl_->original_locations[site.value][axis])
                                : input.chart_xy[2 * c + axis];
        for (std::size_t a = 0; a < 3; ++a)
          input.actual_xyz[3 * c + a] =
              builder.append(Rational::from_double(*budget_, physical[i][c].xyz[a]));
      }
      input.allowance = builder.append(remaining);
    }
    auto cells = std::allocate_shared<FaceCellWorkData>(
        RunAllocator<FaceCellWorkData>(impl_->boundary->resource), impl_->boundary->resource,
        impl_->boundary->geometry, builder.seal(), impl_->boundary);
    cells->cells.assign(inputs.begin(), inputs.end());
    auto work = std::allocate_shared<FaceOptimizationData>(
        RunAllocator<FaceOptimizationData>(impl_->boundary->resource),
        FaceOptimizationData{impl_->boundary,
                             cells,
                             ordinal,
                             edge,
                             {impl_->frontier.key(old[0]), impl_->frontier.key(old[1])},
                             sites,
                             physical,
                             {impl_->declarations(sites[0]), impl_->declarations(sites[1])},
                             {*before0, *before1},
                             {*after0, *after1}});
    impl_->optimization = work;
    ++impl_->counters.optimizer_proposals;
    return FaceOptimizationWork(work);
  } catch (...) {
    impl_->usable = false;
    throw;
  }
}
void FaceEngine::reject_flip(uint64_t ordinal) {
  impl_->mutable_check();
  if (!impl_->optimization || impl_->optimization->ordinal != ordinal || !impl_->flip)
    invalid("stale optimizer proposal rejection");
  impl_->mesh.cancel_flip(*impl_->flip);
  impl_->flip.reset();
  impl_->optimization.reset();
}
FaceOptimizationCommit FaceEngine::complete_flip(uint64_t ordinal,
                                                 const std::array<CellBoundResult, 2> &results,
                                                 const ContactApproval &approval) {
  impl_->mutable_check();
  if (!impl_->optimization || impl_->optimization->ordinal != ordinal || !impl_->flip)
    invalid("stale optimizer proposal completion");
  const auto &work = *impl_->optimization;
  for (std::size_t i = 0; i < 2; ++i) {
    const auto &a = results[i].header, &b = work.cells->cells[i].header;
    if (a.key != b.key || a.source_generation != b.source_generation ||
        a.boundary_generation != b.boundary_generation || a.face_generation != b.face_generation ||
        a.bound_revision != b.bound_revision || a.request_revision != b.request_revision ||
        results[i].decision != BoundDecision::accept || !impl_->current_target(work.old_cells[i]))
      invalid("optimizer needs exact same two current accepted cell bounds");
  }
  if (!contact_approval_matches(approval, {ordinal, work.old_cells, work.nodes, work.edges}))
    invalid("optimizer contact approval does not match proposed identities/geometry");
  FaceOptimizationCommit output(budget_);
  output.affected_edges.reserve(5);
  const auto sites = work.sites;
  const auto potential = work.new_potential;
  try {
    impl_->mesh.commit_flip(*impl_->flip);
    impl_->flip.reset();
    auto edits = impl_->mesh.take_changes();
    if (edits.size() != 1 || edits[0].inserted_count != 2 || edits[0].removed_count != 2)
      internal("optimizer lost local two-cell transaction");
    grow(impl_->changes, 1);
    impl_->frontier.apply_changes(edits);
    impl_->changes.push_back(edits[0]);
    for (std::size_t i = 0; i < 2; ++i) {
      const auto handle = edits[0].inserted[i];
      if (impl_->mesh.triangle(handle).sites != sites[i])
        internal("optimizer commit changed proved cell roles");
      output.cells[i] = impl_->frontier.key(handle);
      if (!impl_->frontier.complete(output.cells[i], CellNumericalState::accepted))
        internal("optimizer cannot rekey accepted cell proof");
      if (handle.slot >= impl_->potentials.size()) {
        grow(impl_->potentials,
             static_cast<std::size_t>(handle.slot) + 1 - impl_->potentials.size());
        impl_->potentials.resize(static_cast<std::size_t>(handle.slot) + 1);
      }
      impl_->potentials[handle.slot] = {handle.generation, potential[i]};
      for (const auto edge : impl_->mesh.triangle(handle).edges) {
        const auto view = impl_->mesh.edge(edge);
        if (view.incident_count == 2 && !view.constraint &&
            std::find(output.affected_edges.begin(), output.affected_edges.end(), edge) ==
                output.affected_edges.end())
          output.affected_edges.push_back(edge);
      }
    }
    // The two local cells already carry the same accepted proofs. Retire their
    // pending-work records now rather than accumulating one stale log per flip.
    if (!impl_->frontier.take_pending(1).empty() || impl_->frontier.next())
      internal("accepted optimizer cells unexpectedly require refinement");
    std::sort(output.affected_edges.begin(), output.affected_edges.end(), [&](auto a, auto b) {
      budget_->charge(1);
      return optimizer_edge_key(a) < optimizer_edge_key(b);
    });
    impl_->optimization.reset();
    ++impl_->counters.optimizer_flips;
    return output;
  } catch (...) {
    impl_->usable = false;
    throw;
  }
}
FaceResult FaceEngine::freeze_result() {
  impl_->check();
  if (impl_->result)
    return impl_->result;
  if (impl_->candidate || impl_->closure || impl_->request || impl_->optimization || impl_->flip ||
      impl_->frontier.next())
    invalid("final face export requires settled complete owner");
  try {
    ExactBuilder builder(impl_->boundary->resource, impl_->next_table());
    std::pmr::vector<FaceResultSite> sites(budget_);
    sites.reserve(impl_->nodes.size());
    for (std::size_t i = 0; i < impl_->nodes.size(); ++i) {
      budget_->charge(1);
      if (i > UINT32_MAX)
        internal("final face site slot overflow");
      const SiteSlot slot{static_cast<uint32_t>(i)};
      const std::array refs{builder.append(impl_->mesh.coordinate(slot, CoordinateAxis::x)),
                            builder.append(impl_->mesh.coordinate(slot, CoordinateAxis::y))};
      const auto locator = impl_->is_plane()
                               ? std::array{builder.append(impl_->original_locations[i][0]),
                                            builder.append(impl_->original_locations[i][1])}
                               : refs;
      std::optional<std::array<ExactRef, 4>> enclosure;
      const auto &retained_enclosure = impl_->original_enclosures[i];
      if (retained_enclosure) {
        const auto &bounds = *retained_enclosure;
        enclosure.emplace(std::array{builder.append(bounds[0]), builder.append(bounds[1]),
                                     builder.append(bounds[2]), builder.append(bounds[3])});
      }
      sites.push_back({slot, refs, locator, impl_->nodes[i], enclosure});
    }
    auto frozen = std::allocate_shared<FaceResultData>(
        RunAllocator<FaceResultData>(impl_->boundary->resource), impl_->boundary, builder.seal());
    frozen->sites.assign(sites.begin(), sites.end());
    frozen->source_sites.assign(impl_->source_sites.begin(), impl_->source_sites.end());
    const auto count = impl_->mesh.lineage_count();
    if (count > frozen->lineages.max_size())
      throw Error(ErrorCode::resource_exhausted, "final lineage capacity exhausted");
    frozen->lineages.reserve(static_cast<std::size_t>(count));
    for (uint64_t i = 0; i < count; ++i) {
      budget_->charge(1);
      frozen->lineages.push_back(impl_->mesh.lineage(i));
    }
    const auto live = impl_->mesh.triangles();
    frozen->triangles.reserve(live.size());
    for (const auto handle : live) {
      budget_->charge(1);
      const auto triangle = impl_->mesh.triangle(handle);
      const auto piece = impl_->pole_piece(handle);
      frozen->triangles.push_back({impl_->frontier.key(handle),
                                   piece ? CellMapKind::analytic_pole_fan_quad : impl_->map_kind(),
                                   triangle.sites, triangle.lineage,
                                   impl_->declarations(triangle.sites), piece});
    }
    frozen->traversal_adapters.assign(impl_->adapters.begin(), impl_->adapters.end());
    impl_->result = frozen;
    return impl_->result;
  } catch (...) {
    impl_->usable = false;
    throw;
  }
}
FaceEngineStats FaceEngine::stats() const {
  impl_->check();
  auto result = impl_->counters;
  result.topology = impl_->mesh.stats();
  result.frontier = impl_->frontier.stats();
  return result;
}
} // namespace bgmesh
