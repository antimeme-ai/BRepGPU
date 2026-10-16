#include "mesh_publish.hpp"
#include "geometry_domain.hpp"
#include <algorithm>
#include <bit>
#include <map>

namespace bgmesh {
namespace {
[[noreturn]] void invalid(const char *message) { throw Error(ErrorCode::invalid_source, message); }
template <class T> std::span<const T> slice(const std::pmr::vector<T> &pool, Range<T> r) {
  if (r.first > pool.size() || r.count > pool.size() - r.first)
    invalid("publisher range outside retained pool");
  return std::span<const T>(pool).subspan(static_cast<std::size_t>(r.first),
                                          static_cast<std::size_t>(r.count));
}
template <class T> Range<T> append(std::pmr::vector<T> &pool, std::span<const T> values) {
  const Range<T> result{pool.size(), values.size()};
  pool.insert(pool.end(), values.begin(), values.end());
  return result;
}
template <class T> Range<T> one(std::pmr::vector<T> &pool, T value) {
  const Range<T> result{pool.size(), 1};
  pool.push_back(value);
  return result;
}
bool same_xyz(const Vec3 &a, const Vec3 &b) {
  for (std::size_t i = 0; i < 3; ++i)
    if (std::bit_cast<uint64_t>(a[i]) != std::bit_cast<uint64_t>(b[i]))
      return false;
  return true;
}
struct MasterEdge final {
  const DefinitionBoundarySnapshotData *owner;
  SourceBoundaryEdge row;
};
class Publisher final {
  bgfd::Budget &budget;
  const PreparedRun &run;
  const SourceData &source;
  NativeMeshBuilder builder;
  NativeMeshData &d;
  std::pmr::map<DefinitionId, MDef> definitions;
  std::pmr::map<FaceId, MFace> faces;
  std::pmr::map<CoedgeId, MUse> uses;
  std::pmr::map<LoopId, MLoop> loops;
  std::pmr::map<std::pair<DefinitionId, EdgeId>, MEdge> edges;
  std::pmr::vector<PhysicalNode> originals;
  std::pmr::map<uint64_t, MasterEdge> masters;
  std::pmr::map<ExactTableKey, uint64_t> tables;
  std::pmr::map<std::pair<StyleId, ProductId>, MAppearance> appearances;
  std::pmr::map<std::pair<uint32_t, uint64_t>, Range<VertexId>> alias_components;
  std::pmr::vector<MSite> source_sites;
  std::pmr::map<MNode, MQuotient> face_quotients;
  std::array<ExactRef, 2> unit_interval{};
  void retain(const ExactTable &table) {
    if (table.resource() != run->resource)
      invalid("publisher exact table belongs to another run");
    const auto [it, inserted] = tables.emplace(table.key(), d.exact_tables.size());
    if (inserted) {
      d.exact_tables.push_back(table);
      return;
    }
    const auto &old = d.exact_tables[static_cast<std::size_t>(it->second)];
    if (old.size() != table.size())
      invalid("publisher same exact key has different values");
    for (uint64_t i = 0; i < table.size(); ++i) {
      budget.charge(1);
      const auto a = old.view({old.key(), i}), b = table.view({table.key(), i});
      if (a.sign != b.sign || !std::ranges::equal(a.numerator, b.numerator) ||
          !std::ranges::equal(a.denominator, b.denominator))
        invalid("publisher exact key aliases different values");
    }
  }
  const ExactTable &table(ExactRef ref) const {
    const auto it = tables.find(ref.table);
    if (it == tables.end())
      invalid("publisher exact reference has no retained owner");
    return d.exact_tables[static_cast<std::size_t>(it->second)];
  }
  Rational read(ExactRef ref) { return import_exact(budget, table(ref).view(ref)); }
  Rational parameter(const PhysicalNode &node, const ExactTable &values) {
    const auto &ref = node.original_parameter;
    if (!ref.has_value())
      invalid("publisher master node lost its original parameter");
    return import_exact(budget, values.view(ref.value()));
  }
  MAppearance appearance(StyleId style, ProductId product) {
    const auto [it, inserted] =
        appearances.emplace(std::pair{style, product}, MAppearance{d.appearances.size() + 1});
    if (inserted) {
      Vec3 rgb{.7, .7, .7};
      if (style) {
        const auto &s = source.styles[source.style_index.find(style).value];
        if (s.has_color)
          rgb = {s.rgba[0], s.rgba[1], s.rgba[2]};
      }
      d.appearances.push_back({style, rgb, .5, product});
    }
    return it->second;
  }
  MEdge edge(Slot<DefinitionTag> def, EdgeId id) const {
    const auto it = edges.find({source.definitions[def.value].id, id});
    if (it == edges.end())
      invalid("publisher face edge absent from complete definition boundary");
    return it->second;
  }
  void collect_node(const PhysicalNode &node) {
    if (node.key.definition >= source.definitions.size())
      invalid("publisher node definition outside source");
    originals.push_back(node);
  }
  bool node_before(const PhysicalNodeKey &a, const PhysicalNodeKey &b) {
    budget.charge(1);
    const auto da = source.definitions[a.definition].id, db = source.definitions[b.definition].id;
    return da != db ? da < db : a < b;
  }
  MNode node(const PhysicalNodeKey &key) {
    const auto found = std::lower_bound(
        originals.begin(), originals.end(), key,
        [&](const auto &value, const auto &search) { return node_before(value.key, search); });
    if (found == originals.end() || found->key != key)
      invalid("publisher physical node absent from canonical index");
    return {static_cast<uint64_t>(found - originals.begin()) + 1};
  }
  MEdgeSample master(MEdge id, ExactRef t, const PhysicalNodeKey &key, std::size_t ordinal) {
    const auto &m = masters.at(id.value);
    const auto values = slice(m.owner->nodes, m.row.nodes);
    if (ordinal >= values.size() || values[ordinal].key != key ||
        parameter(values[ordinal], m.owner->parameters).compare(read(t)) != 0)
      invalid("publisher occurrence is not its ordered settled master sample");
    const auto &row = d.edges[static_cast<std::size_t>(id.value - 1)];
    return slice(d.edge_sample_refs, row.samples)[ordinal];
  }
  template <class Site> MeshLocator locator(const Site &site, WorkingChartKind kind, MChart chart) {
    MeshLocator value;
    value.kind = MeshLocatorKind::direct_original_uv;
    value.chart = chart;
    for (std::size_t axis = 0; axis < 2; ++axis) {
      const auto coordinate = read(site.original_uv[axis]);
      value.chosen[axis] = coordinate.to_double();
      const auto endpoints = coordinate.bounds();
      value.enclosure[2 * axis] = endpoints.lower;
      value.enclosure[2 * axis + 1] = endpoints.upper;
    }
    if (site.original_uv_enclosure) {
      value.kind = MeshLocatorKind::projection_inverse;
      for (std::size_t axis = 0; axis < 2; ++axis) {
        value.enclosure[2 * axis] = read((*site.original_uv_enclosure)[2 * axis]).bounds().lower;
        value.enclosure[2 * axis + 1] =
            read((*site.original_uv_enclosure)[2 * axis + 1]).bounds().upper;
      }
    } else if (kind == WorkingChartKind::physical_plane_convex_q)
      invalid("publisher plane site lost its actual inverse enclosure");
    const auto q = face_quotients.find(node(site.node.key));
    if (q != face_quotients.end() &&
        d.quotients[static_cast<std::size_t>(q->second.value - 1)].pole != MeshPole::none) {
      // The finite locator wire stores quotient/period in its analytic branch.
      // Regular incident-face joint quotients live on Joint/Closure/Node; an
      // inverse-projection locator keeps its distinct recovery interpretation.
      value.kind = MeshLocatorKind::analytic_branch;
      value.quotient = q->second;
    }
    return value;
  }
  MeshLocator locator(const FaceResult &result, uint32_t slot, MChart chart) {
    if (slot >= result->sites.size() || result->sites[slot].site.value != slot)
      invalid("publisher site slot not append ordered");
    return locator(result->sites[slot], result->boundary->chart_kind, chart);
  }
  MSite source_site(uint32_t slot) const {
    if (slot >= source_sites.size())
      invalid("publisher source occurrence outside explicit site map");
    return source_sites[slot];
  }
  static MeshPole pole_kind(AnalyticPoleKind kind) {
    switch (kind) {
    case AnalyticPoleKind::sphere_north:
      return MeshPole::sphere_north;
    case AnalyticPoleKind::sphere_south:
      return MeshPole::sphere_south;
    case AnalyticPoleKind::cone_apex:
      return MeshPole::cone_apex;
    }
    invalid("publisher unknown original analytic pole");
  }
  template <class Ref, class Setter>
  void quotient_members(std::pmr::vector<Ref> &pool, std::pmr::vector<std::pair<MNode, Ref>> &rows,
                        Setter set) {
    std::sort(rows.begin(), rows.end(), [&](const auto &a, const auto &b) {
      budget.charge(1);
      return a < b;
    });
    budget.charge(rows.size());
    rows.erase(std::unique(rows.begin(), rows.end()), rows.end());
    budget.charge(rows.size());
    std::size_t first = 0;
    while (first < rows.size()) {
      const auto key = rows[first].first;
      const auto it = face_quotients.find(key);
      if (it == face_quotients.end())
        invalid("publisher quotient membership has no owner");
      const Range<Ref> range{pool.size(), 0};
      auto out = range;
      do {
        pool.push_back(rows[first++].second);
        ++out.count;
      } while (first < rows.size() && rows[first].first == key);
      set(d.quotients[static_cast<std::size_t>(it->second.value - 1)], out);
    }
  }
  void publish_quotients(const FaceResult &result, MFace face, uint64_t sitebase) {
    const auto &seed = *result->boundary;
    face_quotients.clear();
    std::pmr::map<MNode, MeshPole> kinds(&budget);
    for (const auto &site : seed.sites) {
      budget.charge(1);
      if (site.node.key.generator == NodeGenerator::declared_quotient)
        kinds.emplace(node(site.node.key), MeshPole::none);
    }
    for (const auto &cap : seed.pole_caps) {
      if (cap.pole_site >= seed.sites.size())
        invalid("publisher cap pole source site outside table");
      const auto physical = node(seed.sites[cap.pole_site].node.key);
      const auto pole = pole_kind(cap.kind);
      const auto [it, inserted] = kinds.emplace(physical, pole);
      if (!inserted) {
        if (it->second != MeshPole::none && it->second != pole)
          invalid("publisher conflicting poles at one face physical key");
        it->second = pole;
      }
    }
    for (const auto &[physical, pole] : kinds) {
      const MQuotient self{d.quotients.size() + 1};
      face_quotients.emplace(physical, self);
      const auto &key = originals[static_cast<std::size_t>(physical.value - 1)].key;
      const auto found = alias_components.find({key.definition, key.source});
      Range<VertexId> vertices{d.source_vertex_refs.size(), 0};
      if (found != alias_components.end()) {
        // All incident face quotients borrow the one immutable component list.
        vertices = found->second;
      } else {
        if (key.generator != NodeGenerator::original_vertex)
          invalid("publisher declared component absent from source alias owner");
        vertices = one(d.source_vertex_refs, VertexId{key.source});
      }
      const auto kind = pole == MeshPole::cone_apex ? MeshQuotientKind::apex
                        : pole == MeshPole::none    ? MeshQuotientKind::joint
                                                    : MeshQuotientKind::pole;
      d.quotients.push_back({face,
                             source.faces[seed.face.value].carrier_id,
                             kind,
                             vertices,
                             {},
                             {},
                             physical,
                             {},
                             pole});
      auto &output = d.nodes[static_cast<std::size_t>(physical.value - 1)];
      if (!output.quotient.value) {
        output.quotient = self;
        if (key.generator == NodeGenerator::declared_quotient)
          output.generator = QuotientGenerator{self};
      }
    }
    std::pmr::vector<std::pair<MNode, MSite>> sites(&budget);
    for (uint64_t i = sitebase; i < d.sites.size(); ++i) {
      const auto physical = d.sites[static_cast<std::size_t>(i)].node;
      if (face_quotients.contains(physical))
        sites.emplace_back(physical, MSite{i + 1});
    }
    quotient_members(d.site_refs, sites, [](auto &q, auto range) { q.sites = range; });
    std::pmr::vector<std::pair<MNode, MUse>> uses_by_node(&budget);
    for (const auto &loop : seed.loops) {
      const auto segments = slice(seed.segments, loop.segments);
      for (std::size_t i = 0; i < segments.size(); ++i) {
        const auto use = uses.at(source.coedges[segments[i].use.value].id);
        for (const auto slot : {segments[i].site, segments[(i + 1) % segments.size()].site}) {
          const auto physical = d.sites[static_cast<std::size_t>(source_site(slot).value - 1)].node;
          if (face_quotients.contains(physical))
            uses_by_node.emplace_back(physical, use);
        }
      }
    }
    quotient_members(d.use_refs, uses_by_node, [](auto &q, auto range) { q.uses = range; });
  }
  void inventory() {
    for (const auto &[id, slot] : source.definition_index.entries) {
      definitions.emplace(id, MDef{d.definitions.size() + 1});
      d.definitions.push_back({source.definitions[slot.value].id, {}, {}, {}, {}});
    }
    for (const auto &[id, slot] : source.face_index.entries) {
      const auto &f = source.faces[slot.value];
      faces.emplace(id, MFace{d.faces.size() + 1});
      d.faces.push_back({id, definitions.at(f.definition_id), {}, {}, 0, {}, {}, {}});
    }
    for (const auto &[id, slot] : source.coedge_index.entries) {
      const auto &u = source.coedges[slot.value];
      const auto &l = source.loops[source.loop_index.find(u.loop_id).value];
      uses.emplace(id, MUse{d.uses.size() + 1});
      d.uses.push_back({id,
                        u.loop_id,
                        faces.at(l.face_id),
                        {},
                        u.sense,
                        u.reconstructed,
                        u.parameter_map_kind,
                        u.periodic_shift,
                        {},
                        {},
                        {}});
    }
    for (const auto &[id, slot] : source.loop_index.entries) {
      const auto &l = source.loops[slot.value];
      loops.emplace(id, MLoop{d.loops.size() + 1});
      d.loops.push_back({id, faces.at(l.face_id), l.outer, 1, {}, {}, {}});
    }
  }
  void boundary(const DefinitionBoundarySnapshot &snapshot) {
    if (!snapshot || snapshot->resource != run->resource || &snapshot->source.view() != &source ||
        snapshot->definition.value >= source.definitions.size())
      invalid("publisher foreign definition boundary");
    retain(snapshot->parameters);
    if (!snapshot->vertex_aliases.empty() &&
        (!snapshot->vertex_alias_owner || snapshot->vertex_alias_owner->resource != run->resource ||
         snapshot->vertex_alias_owner->rows.data() != snapshot->vertex_aliases.data() ||
         snapshot->vertex_alias_owner->rows.size() != snapshot->vertex_aliases.size()))
      invalid("publisher source aliases lost their immutable owner");
    std::pmr::vector<std::pair<VertexId, VertexId>> components(&budget);
    for (const auto &alias : snapshot->vertex_aliases) {
      budget.charge(1);
      if (alias.original.value >= source.vertices.size() ||
          alias.representative.value >= source.vertices.size())
        invalid("publisher original alias vertex outside source");
      components.emplace_back(source.vertices[alias.representative.value].id,
                              source.vertices[alias.original.value].id);
    }
    std::sort(components.begin(), components.end(), [&](const auto &a, const auto &b) {
      budget.charge(1);
      return a < b;
    });
    std::size_t component_first = 0;
    while (component_first < components.size()) {
      const auto representative = components[component_first].first;
      Range<VertexId> range{d.source_vertex_refs.size(), 0};
      do {
        d.source_vertex_refs.push_back(components[component_first++].second);
        ++range.count;
      } while (component_first < components.size() &&
               components[component_first].first == representative);
      if (!alias_components
               .emplace(std::pair{snapshot->definition.value, representative.value}, range)
               .second)
        invalid("publisher duplicate definition alias component");
    }
    const auto def = source.definitions[snapshot->definition.value].id;
    EdgeId previous_edge{};
    for (const auto &row : snapshot->edges) {
      budget.charge(1);
      if (row.edge.value >= source.edges.size())
        invalid("publisher edge slot outside source");
      const auto id = source.edges[row.edge.value].id;
      if (previous_edge && !(previous_edge < id))
        invalid("publisher master edges are not in canonical source order");
      previous_edge = id;
      const MEdge self{d.edges.size() + 1};
      if (!edges.emplace(std::pair{def, id}, self).second)
        invalid("publisher duplicate definition edge");
      masters.emplace(self.value, MasterEdge{snapshot.get(), row});
      const auto values = slice(snapshot->nodes, row.nodes);
      const auto &original_edge = source.edges[row.edge.value];
      if (values.size() < 2 ||
          (original_edge.degenerate ? (values.size() != 2 || row.intervals.count != 0)
                                    : row.intervals.count + 1 != values.size()))
        invalid("publisher incomplete source master chain");
      MeshEdge output{id,
                      definitions.at(def),
                      snapshot->epoch.value,
                      {d.edge_sample_refs.size(), values.size()},
                      {}};
      for (std::size_t i = 0; i < values.size(); ++i) {
        const auto &node = values[i];
        if (node.key.definition != snapshot->definition.value || !node.original_parameter)
          invalid("publisher master sample lacks definition/parameter");
        const auto original_parameter = node.original_parameter.value();
        collect_node(node);
        const auto pathIndex = static_cast<std::size_t>(row.nodes.first) + i;
        if (pathIndex >= snapshot->sample_paths.size())
          invalid("publisher master sample lacks ancestry");
        const auto bits = slice(snapshot->path_bits, snapshot->sample_paths[pathIndex]);
        const Range<uint32_t> path{d.interval_paths.size(), bits.size()};
        for (const auto bit : bits) {
          if (bit > 1)
            invalid("publisher invalid master ancestry bit");
          d.interval_paths.push_back(bit);
        }
        d.edge_sample_refs.push_back({d.edge_samples.size() + 1});
        d.edge_samples.push_back({self, original_parameter, {}, path});
      }
      d.edges.push_back(output);
    }
  }
  void initialize_nodes() {
    std::sort(originals.begin(), originals.end(),
              [&](const auto &a, const auto &b) { return node_before(a.key, b.key); });
    std::size_t unique = 0;
    for (std::size_t i = 0; i < originals.size(); ++i) {
      budget.charge(1);
      const auto &value = originals[i];
      if (unique && originals[unique - 1].key == value.key) {
        auto &previous = originals[unique - 1];
        if (!same_xyz(previous.xyz, value.xyz))
          invalid("publisher shared physical identity has different XYZ words");
        if (value.key.generator == NodeGenerator::original_edge) {
          if (previous.original_parameter && value.original_parameter &&
              read(*previous.original_parameter).compare(read(*value.original_parameter)) != 0)
            invalid("publisher original-edge identity has contradictory source parameters");
          if (!previous.original_parameter)
            previous.original_parameter = value.original_parameter;
        }
      } else {
        if (unique != i)
          originals[unique] = value;
        ++unique;
      }
    }
    originals.resize(unique);
    d.nodes.reserve(unique);
    for (const auto &value : originals) {
      const auto &key = value.key;
      budget.charge(1);
      MeshNodeGenerator generator = OriginalVertexGenerator{VertexId{key.source}};
      if (key.generator == NodeGenerator::original_edge) {
        if (!value.original_parameter)
          invalid("publisher edge node lacks original parameter");
        generator = OriginalEdgeGenerator{edge({key.definition}, EdgeId{key.source}),
                                          *value.original_parameter};
      } else if (key.generator == NodeGenerator::face_site)
        generator = FaceSiteGenerator{};
      else if (key.generator == NodeGenerator::declared_quotient)
        generator = QuotientGenerator{};
      else if (key.generator != NodeGenerator::original_vertex)
        invalid("publisher unknown physical node generator");
      d.nodes.push_back(
          {definitions.at(source.definitions[key.definition].id), value.xyz, generator, {}});
    }
    for (const auto &[id, m] : masters) {
      const auto values = slice(m.owner->nodes, m.row.nodes);
      const auto refs =
          slice(d.edge_sample_refs, d.edges[static_cast<std::size_t>(id - 1)].samples);
      for (std::size_t i = 0; i < values.size(); ++i)
        d.edge_samples[static_cast<std::size_t>(refs[i].value - 1)].node = node(values[i].key);
    }
  }
  void publish_face(const FaceResult &result) {
    const auto &seed = *result->boundary;
    const auto &original = source.faces[seed.face.value];
    const auto self = faces.at(original.id);
    const MChart chart{d.charts.size() + 1};
    auto &face = d.faces[static_cast<std::size_t>(self.value - 1)];
    face.chart = chart;
    face.generation = seed.face_generation;
    if (seed.chart_kind == WorkingChartKind::physical_plane_convex_q) {
      if (!seed.plane)
        invalid("publisher plane source descriptor absent");
      const auto &p = seed.plane.value();
      d.charts.push_back({original.carrier_id,
                          PhysicalPlaneChart{p.natural_rectangle, p.projection, p.preconditioner,
                                             p.origin, p.basis, p.quadrilateral}});
    } else if (seed.chart_kind == WorkingChartKind::original_uv)
      d.charts.push_back({original.carrier_id, OriginalUVChart{seed.prepared_original_rectangle}});
    else if (seed.chart_kind == WorkingChartKind::analytic_unwrapped_uv)
      d.charts.push_back(
          {original.carrier_id, UnwrappedUVChart{seed.prepared_original_rectangle, {0, 0}}});
    else
      throw Error(ErrorCode::unsupported, "publisher source chart specialization unavailable");
    const uint64_t sitebase = d.sites.size(), linebase = d.lineage.size();
    std::pmr::vector<uint64_t> firstLineage(result->sites.size(), UINT64_MAX, &budget);
    for (std::size_t i = 0; i < result->lineages.size(); ++i) {
      const auto &l = result->lineages[i];
      const Range<MLineage> parents{d.lineage_refs.size(), 0};
      auto count = parents;
      for (const auto p : {l.parent, l.secondary_parent})
        if (p != UINT64_MAX) {
          if (p >= i)
            invalid("publisher cyclic face lineage");
          d.lineage_refs.push_back({linebase + p + 1});
          ++count.count;
        }
      for (const auto slot : l.sites) {
        if (slot.value >= firstLineage.size())
          invalid("publisher lineage site outside final table");
        firstLineage[slot.value] = std::min(firstLineage[slot.value], static_cast<uint64_t>(i));
      }
      Range<MSite> ancestors{d.site_refs.size(), 0};
      const auto add_ancestors = [&](const std::array<SiteSlot, 3> &sites) {
        for (const auto slot : sites) {
          if (slot.value >= firstLineage.size())
            invalid("publisher input ancestor site outside final table");
          const MSite reference{sitebase + slot.value + 1};
          const auto existing = slice(d.site_refs, ancestors);
          if (std::find(existing.begin(), existing.end(), reference) == existing.end()) {
            d.site_refs.push_back(reference);
            ++ancestors.count;
          }
        }
      };
      if (l.parent == UINT64_MAX) {
        if (l.operation != LineageOperation::initial || l.secondary_parent != UINT64_MAX)
          invalid("publisher generated ancestry lacks its actual input parent");
        add_ancestors(l.sites);
      } else {
        add_ancestors(result->lineages[static_cast<std::size_t>(l.parent)].sites);
        if (l.secondary_parent != UINT64_MAX)
          add_ancestors(result->lineages[static_cast<std::size_t>(l.secondary_parent)].sites);
      }
      MeshLineageOperation op;
      switch (l.operation) {
      case LineageOperation::initial:
        op = MeshLineageOperation::initial;
        break;
      case LineageOperation::heuristic:
        op = MeshLineageOperation::circumcenter;
        break;
      case LineageOperation::bisection:
        op = MeshLineageOperation::midpoint;
        break;
      case LineageOperation::flip:
        op = MeshLineageOperation::optimization;
        break;
      default:
        invalid("publisher unknown lineage operation");
      }
      d.lineage.push_back({self, seed.face_generation, op, count, ancestors});
    }
    for (std::size_t i = 0; i < result->sites.size(); ++i) {
      const auto &s = result->sites[i];
      if (s.site.value != i || firstLineage[i] == UINT64_MAX)
        invalid("publisher site lacks actual creation lineage");
      const auto physical = node(s.node.key);
      const MLineage lineage{linebase + firstLineage[i] + 1};
      const MSite site{sitebase + i + 1};
      d.sites.push_back({self, chart, s.chart, physical, {}, lineage});
      if (s.node.key.generator == NodeGenerator::face_site) {
        if (s.node.key.source != original.id.value || s.node.key.generation != seed.face_generation)
          invalid("publisher generated node has foreign source face/generation");
        auto &generator = d.nodes[static_cast<std::size_t>(physical.value - 1)].generator;
        if (const auto *existing = std::get_if<FaceSiteGenerator>(&generator);
            existing && existing->site.value)
          invalid("publisher generated physical node repeats a face site");
        generator = FaceSiteGenerator{self, site, lineage};
      }
    }
    source_sites.clear();
    source_sites.reserve(seed.sites.size());
    if (result->source_sites.size() != seed.sites.size())
      invalid("publisher complete source-to-active map missing");
    for (std::size_t i = 0; i < seed.sites.size(); ++i) {
      budget.charge(1);
      const auto &source_site = seed.sites[i];
      if (result->source_sites[i]) {
        const auto slot = result->source_sites[i]->value;
        if (slot >= result->sites.size())
          invalid("publisher source-to-active slot outside result");
        const auto &active = result->sites[slot];
        if (active.node.key != source_site.node.key ||
            !same_xyz(active.node.xyz, source_site.node.xyz))
          invalid("publisher source-to-active physical identity changed");
        for (std::size_t axis = 0; axis < 2; ++axis)
          if (read(active.chart[axis]).compare(read(source_site.chart[axis])) ||
              read(active.original_uv[axis]).compare(read(source_site.original_uv[axis])))
            invalid("publisher source-to-active coordinate changed");
        if (active.original_uv_enclosure.has_value() !=
            source_site.original_uv_enclosure.has_value())
          invalid("publisher source-to-active inverse enclosure presence changed");
        if (active.original_uv_enclosure)
          for (std::size_t axis = 0; axis < 4; ++axis)
            if (read((*active.original_uv_enclosure)[axis])
                    .compare(read((*source_site.original_uv_enclosure)[axis])))
              invalid("publisher source-to-active inverse enclosure changed");
        source_sites.push_back({sitebase + slot + 1});
      } else {
        const MSite site{d.sites.size() + 1};
        const MLineage lineage{d.lineage.size() + 1};
        d.lineage.push_back({self,
                             seed.face_generation,
                             MeshLineageOperation::initial,
                             {},
                             one(d.site_refs, site)});
        d.sites.push_back(
            {self, chart, source_site.chart, node(source_site.node.key), {}, lineage});
        source_sites.push_back(site);
      }
    }
    publish_quotients(result, self, sitebase);
    publish_domain(result, self, chart);
    publish_loops(result, self, chart);
    face.cells = {d.cell_refs.size(), result->triangles.size()};
    face.facets = {d.facet_refs.size(), result->triangles.size()};
    const auto definition = source.definitions[seed.definition.value];
    const auto &product = source.products[source.product_index.find(definition.product_id).value];
    const auto look =
        appearance(original.style_id ? original.style_id : product.style_id, product.id);
    const auto allowance = run->assembly.view().definitions[seed.definition.value].positional;
    std::pmr::vector<std::pair<MNode, MCell>> quotient_cells(&budget);
    for (const auto &triangle : result->triangles) {
      budget.charge(1);
      const MCell cell{d.cells.size() + 1};
      const MFacet facet{d.facets.size() + 1};
      const FaceFrontierIdentity expected{
          {OwnerKind::face, seed.definition.value, original.id.value},
          seed.face_generation,
          seed.boundary_generation};
      if (triangle.cell.face != expected)
        invalid("publisher triangle escaped source face generation");
      std::array<MSite, 3> roles{};
      std::array<MNode, 3> xyz{};
      for (std::size_t i = 0; i < 3; ++i) {
        if (triangle.sites[i].value >= result->sites.size())
          invalid("publisher live triangle has foreign site");
        roles[i] = {sitebase + triangle.sites[i].value + 1};
        xyz[i] = node(result->sites[triangle.sites[i].value].node.key);
      }
      const MCellMap map{d.cell_maps.size() + 1};
      if (triangle.map_kind == CellMapKind::uv_triangle &&
          seed.chart_kind != WorkingChartKind::physical_plane_convex_q)
        d.cell_maps.push_back({cell, UVTriangleMap{roles}});
      else if (triangle.map_kind == CellMapKind::plane_xy_triangle &&
               seed.chart_kind == WorkingChartKind::physical_plane_convex_q)
        d.cell_maps.push_back({cell, PlaneTriangleMap{roles, chart}});
      else if (triangle.map_kind == CellMapKind::analytic_pole_fan_quad && triangle.pole_piece) {
        if (*triangle.pole_piece >= seed.pole_pieces.size())
          invalid("publisher pole piece outside retained source transport");
        const auto &piece = seed.pole_pieces[*triangle.pole_piece];
        if (piece.cap >= seed.pole_caps.size())
          invalid("publisher pole piece has foreign cap");
        const auto &cap = seed.pole_caps[piece.cap];
        if (*triangle.pole_piece < cap.pieces.first ||
            *triangle.pole_piece - cap.pieces.first >= cap.pieces.count ||
            cap.pole_site >= seed.sites.size() || piece.cut_sites[0] >= seed.sites.size() ||
            piece.cut_sites[1] >= seed.sites.size())
          invalid("publisher pole piece escaped cap source roles");
        const std::array original_roles{source_site(cap.pole_site), source_site(piece.cut_sites[0]),
                                        source_site(piece.cut_sites[1])};
        for (std::size_t i = 0; i < 3; ++i)
          if (piece.positive_chart_roles[i] >= 3 ||
              roles[i] != original_roles[piece.positive_chart_roles[i]])
            invalid("publisher pole map role permutation changed actual connectivity");
        const auto pole_node = d.sites[static_cast<std::size_t>(original_roles[0].value - 1)].node;
        const auto quotient = face_quotients.at(pole_node);
        const std::array cut_sites{original_roles[1], original_roles[2]};
        const auto cut_chain = append(d.site_refs, std::span<const MSite>(cut_sites));
        d.cell_maps.push_back(
            {cell, PoleFanMap{quotient,
                              pole_kind(cap.kind),
                              {piece.cut_uv[0], piece.cut_uv[2]},
                              {d.sites[static_cast<std::size_t>(original_roles[1].value - 1)].node,
                               d.sites[static_cast<std::size_t>(original_roles[2].value - 1)].node},
                              piece.cut_uv,
                              cut_chain,
                              unit_interval,
                              unit_interval,
                              seed.face_generation}});
      } else
        throw Error(ErrorCode::unsupported, "publisher cell map/source chart mismatch");
      if (triangle.lineage >= result->lineages.size())
        invalid("publisher triangle lineage outside retained history");
      const auto association = one(d.cell_facet_refs, MCellFacet{d.cell_facets.size() + 1});
      Range<MComponent> component{d.component_refs.size(), 0};
      const bool pole = triangle.map_kind == CellMapKind::analytic_pole_fan_quad;
      if (pole) {
        const auto &piece = seed.pole_pieces[*triangle.pole_piece];
        for (const auto &[kind, value] :
             {std::pair{MeshComponentKind::actual_cell_position, piece.position_upper},
              std::pair{MeshComponentKind::pole_band_position, piece.band_upper}}) {
          d.component_refs.push_back({d.components.size() + 1});
          ++component.count;
          d.components.push_back({cell, kind, MeshValueRole::canonical_upper, value, {}});
        }
      } else {
        component = one(d.component_refs, MComponent{d.components.size() + 1});
        d.components.push_back({cell,
                                MeshComponentKind::actual_cell_position,
                                MeshValueRole::allowance,
                                allowance,
                                {}});
      }
      for (const auto physical : xyz)
        if (face_quotients.contains(physical))
          quotient_cells.emplace_back(physical, cell);
      d.cells.push_back(
          {self, chart, {linebase + triangle.lineage + 1}, map, association, component});
      d.cell_facets.push_back(
          {cell, facet,
           pole ? MeshInterpolant::pole_bilinear_fan : MeshInterpolant::barycentric_triangle});
      if (original.orientation < 0) {
        std::swap(roles[1], roles[2]);
        std::swap(xyz[1], xyz[2]);
      }
      std::array<MCorner, 3> corners{};
      for (std::size_t i = 0; i < 3; ++i) {
        corners[i] = {d.corners.size() + 1};
        const auto slot = static_cast<uint32_t>(roles[i].value - sitebase - 1);
        d.corners.push_back({self, roles[i], original.carrier_id, original.orientation,
                             locator(result, slot, chart)});
      }
      d.facets.push_back({self, xyz, corners, look});
      d.cell_refs.push_back(cell);
      d.facet_refs.push_back(facet);
    }
    quotient_members(d.cell_refs, quotient_cells, [](auto &q, auto range) { q.cells = range; });
  }
  void publish_domain(const FaceResult &result, MFace face, MChart chart) {
    const auto &seed = *result->boundary;
    const auto &map = seed.domain_map;
    if (!map)
      return;
    if (map->resource != run->resource || &map->geometry.view() != &run->geometry.view() ||
        map->definition != seed.definition || map->face != seed.face ||
        map->generations !=
            std::array{seed.source_generation, seed.face_generation, seed.boundary_generation})
      invalid("publisher retained domain map escaped source face/generation");
    retain(map->values);
    const MDomainMap self{d.domain_maps.size() + 1};
    d.faces[static_cast<std::size_t>(face.value - 1)].domain_map = self;
    MeshReferencePolicy policy;
    switch (map->policy) {
    case DomainReferencePolicy::authoritative_original:
      policy = MeshReferencePolicy::authoritative_original;
      break;
    case DomainReferencePolicy::hybrid_pullback:
      policy = MeshReferencePolicy::hybrid_pullback;
      break;
    default:
      invalid("publisher unknown domain reference policy");
    }
    const uint64_t supportbase = d.supports.size();
    const Range<MSupport> fans{d.support_refs.size(), map->fans.size()};
    for (std::size_t i = 0; i < map->fans.size(); ++i)
      d.support_refs.push_back({supportbase + i + 1});
    const Range<MSupport> strips{d.support_refs.size(), map->strips.size()};
    for (std::size_t i = 0; i < map->strips.size(); ++i)
      d.support_refs.push_back({supportbase + map->fans.size() + i + 1});
    d.domain_maps.push_back({face, policy, fans, strips});
    const auto use_ref = [&](Slot<CoedgeTag> slot) {
      if (slot.value >= source.coedges.size())
        invalid("publisher domain support use outside source");
      const auto use = uses.at(source.coedges[slot.value].id);
      if (d.uses[static_cast<std::size_t>(use.value - 1)].face != face)
        invalid("publisher domain support use escaped source face");
      return use;
    };
    const auto site_ref = [&](uint32_t slot) {
      if (slot >= seed.sites.size())
        invalid("publisher domain support site outside selected boundary");
      return source_site(slot);
    };
    for (const auto &fan : map->fans) {
      budget.charge(1);
      if (fan.projected != (seed.chart_kind == WorkingChartKind::physical_plane_convex_q))
        invalid("publisher source point descriptor has a different chart interpretation");
      const auto use = use_ref(fan.use);
      std::variant<OriginalPcurvePoint, ProjectedPcurvePoint> source_point =
          OriginalPcurvePoint{use, fan.parameter};
      if (fan.projected)
        source_point = ProjectedPcurvePoint{chart, use, fan.parameter};
      const Range<MeshPoint2> polygon{d.points.size(), 4};
      for (std::size_t i = 0; i < 4; ++i)
        d.points.push_back({fan.polygon[2 * i], fan.polygon[2 * i + 1]});
      d.supports.push_back(
          {face, NodalFanSupport{source_point, site_ref(fan.chosen_site), polygon}});
    }
    for (const auto &strip : map->strips) {
      budget.charge(1);
      static_cast<void>(slice(map->fans, strip.pre_fans));
      const Range<MSupport> before{d.support_refs.size(), strip.pre_fans.count};
      for (uint64_t i = 0; i < strip.pre_fans.count; ++i)
        d.support_refs.push_back({supportbase + strip.pre_fans.first + i + 1});
      d.supports.push_back(
          {face, GraphStripSupport{use_ref(strip.use),
                                   strip.source_parameter,
                                   before,
                                   {site_ref(strip.endpoints[0]), site_ref(strip.endpoints[1])},
                                   strip.origin,
                                   strip.x_covector,
                                   strip.y_covector,
                                   strip.extent,
                                   strip.c,
                                   strip.w0}});
    }
  }
  void publish_loops(const FaceResult &result, MFace face, MChart chart) {
    const auto &seed = *result->boundary;
    if (seed.trace_maps.size() != seed.segments.size() ||
        result->traversal_adapters.size() != seed.loops.size())
      invalid("publisher source trace/loop metadata incomplete");
    auto &f = d.faces[static_cast<std::size_t>(face.value - 1)];
    f.loops = {d.loop_refs.size(), seed.loops.size()};
    for (std::size_t li = 0; li < seed.loops.size(); ++li) {
      const auto &loop = seed.loops[li];
      const auto segments = slice(seed.segments, loop.segments);
      const auto dispositions = slice(seed.uses, loop.uses);
      if (segments.empty() || dispositions.empty())
        invalid("publisher ordinary source loop is empty");
      const auto id = loops.at(source.loops[loop.source_loop.value].id);
      auto &output = d.loops[static_cast<std::size_t>(id.value - 1)];
      output.traversal_adapter = result->traversal_adapters[li];
      output.uses = {d.use_refs.size(), dispositions.size()};
      for (const auto &use : dispositions)
        d.use_refs.push_back(uses.at(source.coedges[use.source_use.value].id));
      output.normalized_sites = {d.site_refs.size(), segments.size()};
      for (std::size_t j = 0; j < segments.size(); ++j) {
        const auto index = output.traversal_adapter == 1 || j == 0 ? j : segments.size() - j;
        d.site_refs.push_back(source_site(segments[index].site));
      }
      output.closures = {d.closure_refs.size(), dispositions.size()};
      for (std::size_t ui = 0; ui < dispositions.size(); ++ui) {
        const auto &disposition = dispositions[ui];
        const bool collapsed = disposition.kind == FaceUseKind::declared_quotient;
        if (!collapsed && disposition.kind != FaceUseKind::ordinary_chain)
          invalid("publisher unknown source use kind");
        const auto &original = source.coedges[disposition.source_use.value];
        if (!original.reconstructed && !seed.domain_map)
          invalid("publisher authoritative face lacks its retained B/T realization");
        const auto self = uses.at(original.id);
        auto &use = d.uses[static_cast<std::size_t>(self.value - 1)];
        use.edge = edge(seed.definition, original.edge_id);
        const auto master_samples = slice(
            d.edge_sample_refs, d.edges[static_cast<std::size_t>(use.edge.value - 1)].samples);
        if (disposition.segments.count + 1 != master_samples.size())
          invalid("publisher use omits a settled shared master sample");
        use.samples = {d.use_sample_refs.size(), disposition.segments.count + 1};
        const auto &edgeSource = source.edges[source.edge_index.find(original.edge_id).value];
        if (collapsed && (!edgeSource.degenerate || !disposition.quotient_pcurve_parameter ||
                          disposition.segments.count != 1))
          invalid("publisher quotient lost its literal source pcurve interval");
        use.profiles = {d.profile_refs.size(), collapsed ? 0 : disposition.segments.count};
        const auto chain = slice(seed.segments, disposition.segments);
        if (chain.empty())
          invalid("publisher ordinary use has no selected chain");
        for (std::size_t j = 0; j <= chain.size(); ++j) {
          const bool finish = j == chain.size();
          const auto index =
              static_cast<std::size_t>(disposition.segments.first) + (finish ? j - 1 : j);
          const auto &segment = seed.segments[index];
          const auto &trace = seed.trace_maps[index];
          const auto next =
              (index - static_cast<std::size_t>(loop.segments.first) + 1) % segments.size();
          const auto slot = finish ? segments[next].site : segment.site;
          const auto t = segment.source_parameter[finish ? 1 : 0];
          const auto parameter = collapsed
                                     ? (*disposition.quotient_pcurve_parameter)[finish ? 1 : 0]
                                     : trace.mapped_pcurve_parameter[finish ? 1 : 0];
          const auto &site = seed.sites[slot];
          MeshLocator raw;
          raw.kind = MeshLocatorKind::unavailable_guide;
          // Source definition + exact mapped s + branch lift retain the raw
          // point symbolically. No chosen recovery UV is passed off as raw A.
          raw.unavailable_reason = original.reconstructed ? 1u : 2u;
          raw.chart = chart;
          raw.lift = original.periodic_shift;
          d.use_sample_refs.push_back({d.use_samples.size() + 1});
          auto recovered = locator(site, seed.chart_kind, chart);
          recovered.lift = original.periodic_shift;
          const auto selected_master = master(
              use.edge, t, site.node.key, original.sense == 1 ? j : master_samples.size() - 1 - j);
          d.use_samples.push_back(
              {self, selected_master, source_site(slot), parameter, raw, recovered});
          if (finish)
            break;
          if (collapsed) {
            if (trace.trace_position_upper) {
              use.components = one(d.component_refs, MComponent{d.components.size() + 1});
              d.components.push_back({self,
                                      MeshComponentKind::trace_position_residual,
                                      MeshValueRole::canonical_upper,
                                      *trace.trace_position_upper,
                                      {}});
            }
            continue;
          }
          const auto endslot = segments[next].site;
          const auto first = read(segment.source_parameter[0]),
                     last = read(segment.source_parameter[1]);
          const bool forward = first.compare(last) < 0;
          const std::array interval =
              forward ? segment.source_parameter
                      : std::array{segment.source_parameter[1], segment.source_parameter[0]};
          const std::array<MNode, 2> endpoints =
              forward ? std::array{node(site.node.key), node(seed.sites[endslot].node.key)}
                      : std::array{node(seed.sites[endslot].node.key), node(site.node.key)};
          std::variant<AffineParameterProfile, GeometricLineProfile, CylinderArcProfile>
              correspondence = AffineParameterProfile{};
          if (trace.kind == TraceCorrespondence::geometric_line) {
            if (!trace.line)
              invalid("publisher line profile lost selected scalar map");
            const auto &line = trace.line.value();
            correspondence =
                GeometricLineProfile{line.covector, line.curve_scalar, endpoints, forward ? 1 : -1};
          } else if (trace.kind != TraceCorrespondence::affine_parameter)
            throw Error(ErrorCode::unsupported,
                        "publisher curve correspondence descriptor unavailable");
          const MProfile profile{d.profiles.size() + 1};
          d.profile_refs.push_back(profile);
          Range<MComponent> trace_components{};
          if (trace.trace_position_upper) {
            trace_components = one(d.component_refs, MComponent{d.components.size() + 1});
            d.components.push_back({self, MeshComponentKind::trace_position_residual,
                                    MeshValueRole::canonical_upper, *trace.trace_position_upper,
                                    profile});
          }
          d.profiles.push_back({face, self, edgeSource.curve_id,
                                source.faces[seed.face.value].carrier_id, interval, correspondence,
                                trace_components});
          if (j != 0)
            d.sites[static_cast<std::size_t>(source_site(slot).value - 1)].branch = self;
        }
        const auto slot = chain.front().site;
        const auto &physical_node = seed.sites[slot].node;
        if (physical_node.key.generator != NodeGenerator::original_vertex &&
            physical_node.key.generator != NodeGenerator::declared_quotient)
          invalid("publisher source joint lacks original vertex identity");
        const auto original_vertex = edgeSource.vertex_ids[original.sense == 1 ? 0u : 1u];
        const auto q = face_quotients.find(node(physical_node.key));
        const MQuotient quotient = q == face_quotients.end() ? MQuotient{} : q->second;
        const MJoint joint{d.joints.size() + 1};
        const MSite site = source_site(slot);
        const auto previous =
            uses.at(source
                        .coedges[dispositions[(ui + dispositions.size() - 1) % dispositions.size()]
                                     .source_use.value]
                        .id);
        const std::array incident{previous, self};
        const auto incidentRange = append(d.use_refs, std::span<const MUse>(incident));
        d.joints.push_back({face, original_vertex, chart, original.id.value, site, incidentRange,
                            locator(seed.sites[slot], seed.chart_kind, chart), quotient});
        d.sites[static_cast<std::size_t>(site.value - 1)].branch = joint;
        d.closure_refs.push_back({d.closures.size() + 1});
        d.closures.push_back(
            {face,
             output.source,
             original_vertex,
             previous,
             self,
             {site, site},
             quotient.value ? static_cast<MeshClosureKind>(
                                  d.quotients[static_cast<std::size_t>(quotient.value - 1)].kind)
                            : MeshClosureKind::joint,
             quotient,
             {}});
      }
      d.loop_refs.push_back(id);
    }
  }
  template <class T, class Owner, class RangeSetter>
  void ownership(std::pmr::vector<T> &pool, uint64_t count, Owner owner, RangeSetter set) {
    std::pmr::vector<uint64_t> counts(d.definitions.size(), 0, &budget), positions(&budget);
    for (uint64_t i = 0; i < count; ++i)
      ++counts.at(static_cast<std::size_t>(owner(i).value - 1));
    positions.resize(counts.size());
    uint64_t total = pool.size();
    for (std::size_t i = 0; i < counts.size(); ++i) {
      positions[i] = total;
      set(d.definitions[i], Range<T>{total, counts[i]});
      total += counts[i];
    }
    pool.resize(static_cast<std::size_t>(total));
    for (uint64_t i = 0; i < count; ++i)
      pool[static_cast<std::size_t>(positions[static_cast<std::size_t>(owner(i).value - 1)]++)] = {
          i + 1};
  }

public:
  Publisher(bgfd::Budget &scratch, const PreparedRun &prepared, MeshQuality quality)
      : budget(scratch), run(prepared), source(prepared->geometry.view().source.view()),
        builder(prepared->resource, prepared->geometry.view().source), d(builder.data()),
        definitions(&scratch), faces(&scratch), uses(&scratch), loops(&scratch), edges(&scratch),
        originals(&scratch), masters(&scratch), tables(&scratch), appearances(&scratch),
        alias_components(&scratch), source_sites(&scratch), face_quotients(&scratch) {
    d.quality = quality;
    retain(run->assembly.view().values);
    // Reserved publication namespace; numerical service run IDs are bounded u32.
    ExactBuilder constants(run->resource, {{OwnerKind::run, UINT32_MAX, UINT64_MAX}, 0, 1});
    unit_interval = {constants.append(Rational(budget)), constants.append(Rational(budget, 1))};
    retain(constants.seal());
    inventory();
  }
  NativeMesh finish(std::span<const DefinitionBoundarySnapshot> boundaries,
                    std::span<const FaceResult> results) {
    if (boundaries.size() != source.definitions.size() || results.size() != source.faces.size())
      invalid("publisher requires complete original definition and face inventories");
    std::pmr::vector<DefinitionBoundarySnapshot> bs(boundaries.begin(), boundaries.end(), &budget);
    std::size_t node_capacity = 0;
    const auto reserve_nodes = [&](std::size_t count) {
      if (count > originals.max_size() - node_capacity)
        throw Error(ErrorCode::resource_exhausted, "publisher physical node count overflow");
      node_capacity += count;
    };
    for (const auto &b : bs)
      if (!b || b->definition.value >= source.definitions.size())
        invalid("publisher invalid boundary identity");
      else
        reserve_nodes(b->nodes.size());
    for (const auto &r : results) {
      if (!r)
        invalid("publisher null face result");
      reserve_nodes(r->sites.size());
      if (r->boundary)
        reserve_nodes(r->boundary->sites.size());
    }
    originals.reserve(node_capacity);
    std::sort(bs.begin(), bs.end(), [&](const auto &a, const auto &b) {
      return source.definitions[a->definition.value].id <
             source.definitions[b->definition.value].id;
    });
    for (std::size_t i = 0; i < bs.size(); ++i) {
      if (i && bs[i - 1]->definition == bs[i]->definition)
        invalid("publisher duplicate definition boundary");
      boundary(bs[i]);
    }
    std::pmr::vector<FaceResult> fs(results.begin(), results.end(), &budget);
    for (const auto &r : fs) {
      if (!r || !r->boundary || r->boundary->resource != run->resource ||
          &r->boundary->geometry.view() != &run->geometry.view() ||
          r->boundary->face.value >= source.faces.size() ||
          r->boundary->definition.value >= source.definitions.size() ||
          source.faces[r->boundary->face.value].definition_id !=
              source.definitions[r->boundary->definition.value].id)
        invalid("publisher face result escaped original geometry/source owner");
      retain(r->boundary->values);
      retain(r->values);
      if (r->boundary->source_generation != run->geometry.view().table.key().generation)
        invalid("publisher face source generation mismatch");
      for (const auto &site : r->boundary->sites) {
        if (site.node.key.definition != r->boundary->definition.value)
          invalid("publisher source occurrence physical node has foreign definition");
        collect_node(site.node);
      }
      for (const auto &segment : r->boundary->segments)
        if (segment.site >= r->boundary->sites.size() || segment.use.value >= source.coedges.size())
          invalid("publisher source segment site/use outside table");
      for (const auto &use : r->boundary->uses)
        if (use.source_use.value >= source.coedges.size())
          invalid("publisher source use slot outside table");
      for (const auto &loop : r->boundary->loops)
        if (loop.source_loop.value >= source.loops.size())
          invalid("publisher source loop slot outside table");
      for (const auto &s : r->sites) {
        if (s.node.key.definition != r->boundary->definition.value)
          invalid("publisher face physical node has foreign definition");
        collect_node(s.node);
      }
    }
    std::sort(fs.begin(), fs.end(), [&](const auto &a, const auto &b) {
      return source.faces[a->boundary->face.value].id < source.faces[b->boundary->face.value].id;
    });
    for (std::size_t i = 1; i < fs.size(); ++i)
      if (fs[i - 1]->boundary->face == fs[i]->boundary->face)
        invalid("publisher duplicate original face result");
    initialize_nodes();
    for (const auto &f : fs)
      publish_face(f);
    ownership(
        d.face_refs, d.faces.size(),
        [&](auto i) { return d.faces[static_cast<std::size_t>(i)].definition; },
        [](auto &v, auto r) { v.faces = r; });
    ownership(
        d.node_refs, d.nodes.size(),
        [&](auto i) { return d.nodes[static_cast<std::size_t>(i)].definition; },
        [](auto &v, auto r) { v.nodes = r; });
    ownership(
        d.site_refs, d.sites.size(),
        [&](auto i) {
          return d
              .faces[static_cast<std::size_t>(d.sites[static_cast<std::size_t>(i)].face.value - 1)]
              .definition;
        },
        [](auto &v, auto r) { v.sites = r; });
    ownership(
        d.facet_refs, d.facets.size(),
        [&](auto i) {
          return d
              .faces[static_cast<std::size_t>(d.facets[static_cast<std::size_t>(i)].face.value - 1)]
              .definition;
        },
        [](auto &v, auto r) { v.facets = r; });
    for (const auto &definition : d.definitions) {
      const auto slot = source.definition_index.find(definition.source);
      const auto &a = run->assembly.view().definitions.at(slot.value);
      if (a.definition != slot)
        invalid("publisher assembly definition quality index mismatch");
      const auto def = definitions.at(definition.source);
      d.components.push_back({def,
                              MeshComponentKind::definition_positional,
                              MeshValueRole::allowance,
                              a.positional,
                              {}});
      d.components.push_back({def,
                              MeshComponentKind::definition_node_source,
                              MeshValueRole::allowance,
                              a.node_source,
                              {}});
    }
    for (const auto &i : run->assembly.view().instances) {
      const auto path = slice(run->assembly.view().paths, i.path);
      d.instances.push_back({append(d.usage_paths, path),
                             i.product,
                             definitions.at(source.definitions[i.definition.value].id),
                             i.matrix,
                             {0, 0, 0},
                             appearance(i.style, i.product)});
    }
    return builder.seal(budget);
  }
};
} // namespace
NativeMesh publish_native_mesh(bgfd::Budget &budget, const PreparedRun &run,
                               std::span<const DefinitionBoundarySnapshot> boundaries,
                               std::span<const FaceResult> faces, MeshQuality quality) {
  try {
    budget.charge(0);
    if (!run || run->resource != run->geometry.view().source.view().resource ||
        &run->assembly.view().source.view() != &run->geometry.view().source.view())
      invalid("publisher requires one retained native run/source");
    if (quality.positional != run->requested.positional ||
        quality.node_source != run->requested.node_source ||
        quality.policy_revision != run->requested.policy_revision)
      invalid("publisher requested quality differs from the prepared run");
    return Publisher(budget, run, quality).finish(boundaries, faces);
  } catch (const bgfd::Cancelled &) {
    throw Error(ErrorCode::cancelled, "native publication cancelled");
  } catch (const bgfd::Failure &e) {
    throw Error(e.status == BGFD_RESOURCE_EXHAUSTED ? ErrorCode::resource_exhausted
                                                    : ErrorCode::internal_error,
                e.what());
  }
}
} // namespace bgmesh
