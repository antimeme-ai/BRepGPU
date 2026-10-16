#include "mesh.hpp"
#include "mesh_walk.hpp"
#include <cmath>
#include <map>
#include <set>

namespace bgmesh {
NativeMeshData::NativeMeshData(std::shared_ptr<RunResource> owner, SourceStore original)
    : resource(std::move(owner)), source(std::move(original)), exact_tables(resource.get()),
      definitions(resource.get()), faces(resource.get()), nodes(resource.get()),
      sites(resource.get()), lineage(resource.get()), edges(resource.get()),
      edge_samples(resource.get()), joints(resource.get()), uses(resource.get()),
      use_samples(resource.get()), loops(resource.get()), charts(resource.get()),
      domain_maps(resource.get()), supports(resource.get()), profiles(resource.get()),
      cells(resource.get()), cell_maps(resource.get()), cell_facets(resource.get()),
      facets(resource.get()), corners(resource.get()), quotients(resource.get()),
      closures(resource.get()), components(resource.get()), instances(resource.get()),
      appearances(resource.get()), face_refs(resource.get()), node_refs(resource.get()),
      site_refs(resource.get()), facet_refs(resource.get()), loop_refs(resource.get()),
      cell_refs(resource.get()), lineage_refs(resource.get()), edge_sample_refs(resource.get()),
      use_refs(resource.get()), use_sample_refs(resource.get()), profile_refs(resource.get()),
      component_refs(resource.get()), closure_refs(resource.get()), support_refs(resource.get()),
      cell_facet_refs(resource.get()), points(resource.get()), interval_paths(resource.get()),
      source_vertex_refs(resource.get()), usage_paths(resource.get()) {}
namespace {
[[noreturn]] void invalid(const char *message) { throw Error(ErrorCode::invalid_source, message); }
template <class T> const T &row(const std::pmr::vector<T> &table, MeshRef<T> reference) {
  if (!reference.value || reference.value > table.size())
    invalid("mesh semantic row reference outside table");
  return table[static_cast<std::size_t>(reference.value - 1)];
}
template <class T> std::span<const T> list(const std::pmr::vector<T> &pool, Range<T> range) {
  if (range.first > pool.size() || range.count > pool.size() - range.first)
    invalid("mesh semantic list outside pool");
  return std::span<const T>(pool).subspan(static_cast<std::size_t>(range.first),
                                          static_cast<std::size_t>(range.count));
}
template <class T, class Key> void canonical(const std::pmr::vector<T> &rows, Key key) {
  for (std::size_t i = 1; i < rows.size(); ++i)
    if (!(key(rows[i - 1]) < key(rows[i])))
      invalid("mesh table is not in canonical source order");
}
struct Check final : mesh_detail::MeshObserver<Check> {
  std::pmr::vector<const ExactTable *> tables;
  Check(const NativeMeshData &d, bgfd::Budget &b) : MeshObserver{d, b}, tables(d.resource.get()) {
    for (const auto &table : d.exact_tables)
      tables.push_back(&table);
    std::sort(tables.begin(), tables.end(), [](auto a, auto c) { return a->key() < c->key(); });
    for (std::size_t i = 1; i < tables.size(); ++i)
      if (tables[i - 1]->key() == tables[i]->key())
        invalid("duplicate mesh exact key");
  }
  void u64(uint64_t &) { budget.charge(1); }
  void u32(uint32_t &) { budget.charge(1); }
  void i32(int32_t &) { budget.charge(1); }
  void f64(double &v) {
    require(std::isfinite(v), "nonfinite mesh scalar");
    budget.charge(1);
  }
  void exact(ExactRef &v) {
    const auto it = std::lower_bound(tables.begin(), tables.end(), v.table,
                                     [](auto t, auto key) { return t->key() < key; });
    require(it != tables.end() && (*it)->key() == v.table, "mesh exact value has no table lease");
    static_cast<void>((*it)->view(v));
    budget.charge(1);
  }
};
void fields(const NativeMeshData &d, bgfd::Budget &budget) {
  Check check(d, budget);
#define BGNMESH_CHECK(tag, type, member, width)                                                    \
  for (const auto &original : d.member) {                                                          \
    auto copy = original;                                                                          \
    check.fields(copy);                                                                            \
  }
  BGNMESH_BEFORE_QUALITY(BGNMESH_CHECK)
  auto quality = d.quality;
  check.fields(quality);
  BGNMESH_AFTER_QUALITY(BGNMESH_CHECK)
#undef BGNMESH_CHECK
}
void validate(const NativeMeshData &d, bgfd::Budget &budget) {
  const auto &s = d.source.view();
  if (d.resource != s.resource)
    invalid("mesh source belongs to a different run");
  fields(d, budget);
  if (d.definitions.size() != s.definitions.size() || d.faces.size() != s.faces.size() ||
      d.uses.size() != s.coedges.size() || d.loops.size() != s.loops.size())
    invalid("mesh drops original definition/face/use/loop inventory");
  if (!std::isfinite(d.quality.positional) || !std::isfinite(d.quality.node_source) ||
      d.quality.positional <= 0 || d.quality.node_source <= 0 || d.quality.policy_revision != 1)
    invalid("mesh has unsupported or invalid requested quality");
  canonical(d.definitions, [](const auto &x) { return x.source; });
  canonical(d.faces, [](const auto &x) { return x.source; });
  canonical(d.uses, [](const auto &x) { return x.source; });
  canonical(d.loops, [](const auto &x) { return x.source; });
  std::pmr::map<ExactTableKey, const ExactTable *> keys(d.resource.get());
  for (const auto &table : d.exact_tables)
    if (!keys.emplace(table.key(), &table).second)
      invalid("mesh repeats an exact storage key");
  const auto exact_sign = [&](ExactRef ref) { return keys.at(ref.table)->view(ref).sign; };
  std::pmr::vector<uint8_t> owned_faces(d.faces.size(), 0, d.resource.get()),
      owned_nodes(d.nodes.size(), 0, d.resource.get()),
      owned_sites(d.sites.size(), 0, d.resource.get()),
      owned_facets(d.facets.size(), 0, d.resource.get());
  uint64_t expected_edges = 0;
  std::pmr::set<std::pair<uint64_t, uint64_t>> source_edges(d.resource.get());
  std::pmr::set<std::pair<uint64_t, uint64_t>> source_vertices(d.resource.get());
  for (std::size_t i = 0; i < d.definitions.size(); ++i) {
    const auto &definition = d.definitions[i];
    const MDef self{i + 1};
    const auto &original = s.definitions[s.definition_index.find(definition.source).value];
    if (definition.faces.count != original.faces.count)
      invalid("mesh definition drops original faces");
    if (original.edges.count > UINT64_MAX - expected_edges)
      invalid("mesh source edge count overflow");
    expected_edges += original.edges.count;
    for (uint64_t j = 0; j < original.edges.count; ++j)
      source_edges.emplace(self.value,
                           s.edge_ids[static_cast<std::size_t>(original.edges.first + j)].value);
    for (uint64_t j = 0; j < original.vertices.count; ++j)
      source_vertices.emplace(
          self.value, s.vertex_ids[static_cast<std::size_t>(original.vertices.first + j)].value);
    for (const auto id : list(d.face_refs, definition.faces)) {
      if (row(d.faces, id).definition != self ||
          owned_faces[static_cast<std::size_t>(id.value - 1)]++)
        invalid("mesh face has duplicate or wrong definition owner");
    }
    for (const auto id : list(d.node_refs, definition.nodes)) {
      if (row(d.nodes, id).definition != self ||
          owned_nodes[static_cast<std::size_t>(id.value - 1)]++)
        invalid("mesh node has duplicate or wrong definition owner");
    }
    for (const auto id : list(d.site_refs, definition.sites)) {
      if (row(d.faces, row(d.sites, id).face).definition != self ||
          owned_sites[static_cast<std::size_t>(id.value - 1)]++)
        invalid("mesh site has duplicate or wrong definition owner");
    }
    for (const auto id : list(d.facet_refs, definition.facets)) {
      if (row(d.faces, row(d.facets, id).face).definition != self ||
          owned_facets[static_cast<std::size_t>(id.value - 1)]++)
        invalid("mesh facet has duplicate or wrong definition owner");
    }
  }
  if (d.edges.size() != expected_edges)
    invalid("mesh drops definition-local source edge inventory");
  const auto complete = [](const auto &owners) {
    return std::all_of(owners.begin(), owners.end(), [](auto n) { return n == 1; });
  };
  if (!complete(owned_faces) || !complete(owned_nodes) || !complete(owned_sites) ||
      !complete(owned_facets))
    invalid("mesh has unowned semantic records");
  for (std::size_t i = 0; i < d.faces.size(); ++i) {
    const auto &face = d.faces[i];
    const MFace self{i + 1};
    const auto &original = s.faces[s.face_index.find(face.source).value];
    if (row(d.definitions, face.definition).source != original.definition_id ||
        row(d.charts, face.chart).source != original.carrier_id ||
        face.loops.count != original.loops.count)
      invalid("mesh face source/definition/chart/loop ownership mismatch");
    for (const auto id : list(d.loop_refs, face.loops))
      if (row(d.loops, id).face != self)
        invalid("mesh loop has wrong face owner");
    for (const auto id : list(d.cell_refs, face.cells))
      if (row(d.cells, id).face != self)
        invalid("mesh cell has wrong face owner");
    for (const auto id : list(d.facet_refs, face.facets))
      if (row(d.facets, id).face != self)
        invalid("mesh facet has wrong face owner");
    bool authoritative = false, reconstructed = false;
    for (const auto loop_id : list(d.loop_refs, face.loops)) {
      const auto &loop = row(d.loops, loop_id);
      for (const auto use_id : list(d.use_refs, loop.uses)) {
        const auto &use = row(d.uses, use_id);
        authoritative = authoritative || !use.reconstructed;
        reconstructed = reconstructed || use.reconstructed;
      }
    }
    if (authoritative) {
      const auto &map = row(d.domain_maps, face.domain_map);
      if (map.face != self ||
          map.policy != (reconstructed ? MeshReferencePolicy::hybrid_pullback
                                       : MeshReferencePolicy::authoritative_original))
        invalid("mesh authoritative source has no matching domain realization");
    } else if (face.domain_map.value)
      invalid("all-reconstructed face has a foreign reference policy");
  }
  for (const auto &use : d.uses) {
    const auto &original = s.coedges[s.coedge_index.find(use.source).value];
    if (use.source_loop != original.loop_id || row(d.edges, use.edge).source != original.edge_id ||
        use.sense != original.sense || use.reconstructed != original.reconstructed ||
        use.parameter_map_kind != original.parameter_map_kind ||
        use.lift != original.periodic_shift ||
        row(d.faces, use.face).source != s.loops[s.loop_index.find(original.loop_id).value].face_id)
      invalid("mesh use changes original source occurrence semantics");
  }
  for (const auto &loop : d.loops) {
    const auto &original = s.loops[s.loop_index.find(loop.source).value];
    if (row(d.faces, loop.face).source != original.face_id || loop.outer != original.outer ||
        (loop.traversal_adapter != 1 && loop.traversal_adapter != -1) ||
        loop.uses.count != original.coedges.count)
      invalid("mesh loop changes original ownership or use inventory");
    const auto uses = list(d.use_refs, loop.uses);
    for (std::size_t i = 0; i < uses.size(); ++i)
      if (row(d.uses, uses[i]).source !=
          s.coedge_ids[static_cast<std::size_t>(original.coedges.first) + i])
        invalid("mesh rewrites original use traversal");
  }
  for (const auto &edge : d.edges) {
    static_cast<void>(row(d.definitions, edge.definition));
    if (source_edges.erase({edge.definition.value, edge.source.value}) != 1 ||
        edge.samples.count < 2)
      invalid("mesh edge source/definition/endpoint inventory mismatch");
  }
  if (!source_edges.empty())
    invalid("mesh omits original definition-local edges");
  for (const auto &facet : d.facets) {
    const auto definition = row(d.faces, facet.face).definition;
    if (facet.nodes[0] == facet.nodes[1] || facet.nodes[1] == facet.nodes[2] ||
        facet.nodes[2] == facet.nodes[0])
      invalid("mesh facet repeats physical node identity");
    for (std::size_t i = 0; i < 3; ++i) {
      const auto &corner = row(d.corners, facet.corners[i]);
      if (row(d.nodes, facet.nodes[i]).definition != definition || corner.face != facet.face ||
          row(d.sites, corner.site).node != facet.nodes[i])
        invalid("mesh facet corner/physical-node ownership mismatch");
    }
    static_cast<void>(row(d.appearances, facet.appearance));
  }
  for (std::size_t i = 0; i < d.sites.size(); ++i) {
    const auto &site = d.sites[i];
    const auto &face = row(d.faces, site.face);
    if (site.chart != face.chart || row(d.nodes, site.node).definition != face.definition ||
        row(d.lineage, site.lineage).face != site.face)
      invalid("mesh chart site ownership mismatch");
    std::visit(
        [&](const auto &branch) {
          using T = std::decay_t<decltype(branch)>;
          if constexpr (std::is_same_v<T, MUse>) {
            if (row(d.uses, branch).face != site.face)
              invalid("site use branch belongs to another face");
          } else if constexpr (std::is_same_v<T, MJoint>) {
            if (row(d.joints, branch).face != site.face)
              invalid("site joint branch belongs to another face");
          }
        },
        site.branch);
  }
  for (std::size_t i = 0; i < d.lineage.size(); ++i) {
    const auto &item = d.lineage[i];
    for (auto parent : list(d.lineage_refs, item.parents))
      if (parent.value >= i + 1 || row(d.lineage, parent).face != item.face)
        invalid("mesh lineage is cyclic/foreign");
    for (auto site : list(d.site_refs, item.parent_sites))
      if (row(d.sites, site).face != item.face)
        invalid("lineage parent site belongs to another face");
  }
  for (std::size_t i = 0; i < d.nodes.size(); ++i) {
    const auto &node = d.nodes[i];
    const MNode self{i + 1};
    std::visit(
        [&](const auto &g) {
          using T = std::decay_t<decltype(g)>;
          if constexpr (std::is_same_v<T, OriginalVertexGenerator>) {
            if (!source_vertices.contains({node.definition.value, g.vertex.value}))
              invalid("node vertex outside original definition");
          } else if constexpr (std::is_same_v<T, OriginalEdgeGenerator>) {
            if (row(d.edges, g.edge).definition != node.definition)
              invalid("node edge belongs to another definition");
          } else if constexpr (std::is_same_v<T, FaceSiteGenerator>) {
            if (row(d.faces, g.face).definition != node.definition ||
                row(d.sites, g.site).face != g.face || row(d.sites, g.site).node != self ||
                row(d.lineage, g.lineage).face != g.face)
              invalid("face node generator mismatch");
          } else {
            if (row(d.quotients, g.quotient).node != self ||
                row(d.faces, row(d.quotients, g.quotient).face).definition != node.definition)
              invalid("quotient node generator mismatch");
          }
        },
        node.generator);
    if (node.quotient.value && row(d.quotients, node.quotient).node != self)
      invalid("node quotient backlink mismatch");
  }
  for (std::size_t i = 0; i < d.edge_samples.size(); ++i) {
    const auto &sample = d.edge_samples[i];
    if (row(d.nodes, sample.node).definition != row(d.edges, sample.edge).definition)
      invalid("edge sample node belongs to another definition");
    for (auto bit : list(d.interval_paths, sample.interval_path))
      if (bit > 1)
        invalid("unknown edge interval-tree direction");
  }
  for (std::size_t i = 0; i < d.edges.size(); ++i)
    for (auto sample : list(d.edge_sample_refs, d.edges[i].samples))
      if (row(d.edge_samples, sample).edge != MEdge{i + 1})
        invalid("edge sample backlink mismatch");
  for (std::size_t i = 0; i < d.uses.size(); ++i) {
    const auto &use = d.uses[i];
    if (row(d.edges, use.edge).definition != row(d.faces, use.face).definition)
      invalid("use edge belongs to another definition");
    for (auto reference : list(d.use_sample_refs, use.samples)) {
      const auto &sample = row(d.use_samples, reference);
      const auto &edge = row(d.edge_samples, sample.edge_sample);
      if (sample.use != MUse{i + 1} || edge.edge != use.edge ||
          row(d.sites, sample.site).face != use.face || row(d.sites, sample.site).node != edge.node)
        invalid("use sample source/physical identity mismatch");
    }
    for (auto profile : list(d.profile_refs, use.profiles))
      if (row(d.profiles, profile).use != MUse{i + 1} || row(d.profiles, profile).face != use.face)
        invalid("use profile ownership mismatch");
  }
  for (const auto &joint : d.joints) {
    if (row(d.sites, joint.site).face != joint.face ||
        row(d.sites, joint.site).chart != joint.chart)
      invalid("joint site/chart ownership mismatch");
    for (auto use : list(d.use_refs, joint.uses))
      if (row(d.uses, use).face != joint.face)
        invalid("joint use belongs to another face");
  }
  std::pmr::vector<uint8_t> support_owners(d.supports.size(), 0, d.resource.get());
  for (std::size_t i = 0; i < d.domain_maps.size(); ++i) {
    const auto &map = d.domain_maps[i];
    if (row(d.faces, map.face).domain_map != MDomainMap{i + 1})
      invalid("domain map face backlink mismatch");
    const auto own = [&](MSupport id, bool fan) {
      const auto &support = row(d.supports, id);
      if (support.face != map.face ||
          std::holds_alternative<NodalFanSupport>(support.support) != fan ||
          support_owners[size_t(id.value - 1)]++)
        invalid("domain support duplicate/owner/type mismatch");
    };
    for (auto id : list(d.support_refs, map.fans))
      own(id, true);
    for (auto id : list(d.support_refs, map.strips))
      own(id, false);
  }
  if (!std::all_of(support_owners.begin(), support_owners.end(), [](auto n) { return n == 1; }))
    invalid("unowned domain support");
  for (const auto &support : d.supports)
    std::visit(
        [&](const auto &payload) {
          using T = std::decay_t<decltype(payload)>;
          if constexpr (std::is_same_v<T, NodalFanSupport>) {
            if (row(d.sites, payload.chosen).face != support.face || payload.polygon.count < 3)
              invalid("nodal fan chosen site/support mismatch");
            std::visit(
                [&](const auto &point) {
                  if (row(d.uses, point.use).face != support.face)
                    invalid("fan source use belongs to another face");
                  if constexpr (std::is_same_v<std::decay_t<decltype(point)>, ProjectedPcurvePoint>)
                    if (point.chart != row(d.faces, support.face).chart)
                      invalid("fan projected chart mismatch");
                },
                payload.source_point);
          } else {
            if (row(d.uses, payload.use).face != support.face ||
                row(d.uses, payload.use).reconstructed)
              invalid("strip source is not authoritative on this face");
            if (exact_sign(payload.extent) <= 0 || exact_sign(payload.c) <= 0 ||
                exact_sign(payload.w0) <= 0)
              invalid("strip exact extent/taper/width must be positive");
            for (auto site : payload.endpoints)
              if (row(d.sites, site).face != support.face)
                invalid("strip endpoint belongs to another face");
            const auto &map = row(d.domain_maps, row(d.faces, support.face).domain_map);
            const auto fans = list(d.support_refs, map.fans);
            std::size_t after = 0;
            for (auto fan : list(d.support_refs, payload.pre_fans)) {
              const auto found =
                  std::find(fans.begin() + static_cast<std::ptrdiff_t>(after), fans.end(), fan);
              if (found == fans.end())
                invalid("strip fan order/source mismatch");
              after = static_cast<std::size_t>(found - fans.begin()) + 1;
            }
          }
        },
        support.support);
  for (const auto &profile : d.profiles) {
    if (profile.use.value && row(d.uses, profile.use).face != profile.face)
      invalid("profile use face mismatch");
    if (row(d.charts, row(d.faces, profile.face).chart).source != profile.source_carrier)
      invalid("profile original carrier mismatch");
  }
  std::pmr::vector<uint8_t> owned_cells(d.cells.size(), 0, d.resource.get()),
      owned_loops(d.loops.size(), 0, d.resource.get()),
      face_facets(d.facets.size(), 0, d.resource.get()),
      owned_maps(d.cell_maps.size(), 0, d.resource.get()),
      owned_associations(d.cell_facets.size(), 0, d.resource.get());
  for (const auto &face : d.faces) {
    for (auto id : list(d.cell_refs, face.cells))
      if (owned_cells[size_t(id.value - 1)]++)
        invalid("repeated face cell");
    for (auto id : list(d.loop_refs, face.loops))
      if (owned_loops[size_t(id.value - 1)]++)
        invalid("repeated face loop");
    for (auto id : list(d.facet_refs, face.facets))
      if (face_facets[size_t(id.value - 1)]++)
        invalid("repeated face facet");
  }
  if (!complete(owned_cells) || !complete(owned_loops) || !complete(face_facets))
    invalid("face cell/loop/facet inventory is incomplete or duplicated");
  for (std::size_t i = 0; i < d.cells.size(); ++i) {
    const auto &cell = d.cells[i];
    const MCell self{i + 1};
    const auto &map = row(d.cell_maps, cell.map);
    if (cell.chart != row(d.faces, cell.face).chart ||
        row(d.lineage, cell.lineage).face != cell.face || map.cell != self ||
        owned_maps[size_t(cell.map.value - 1)]++)
      invalid("cell source/map ownership mismatch");
    for (auto id : list(d.cell_facet_refs, cell.facets)) {
      const auto &association = row(d.cell_facets, id);
      if (association.cell != self || row(d.facets, association.facet).face != cell.face ||
          owned_associations[size_t(id.value - 1)]++)
        invalid("cell/facet association ownership mismatch");
    }
    std::visit(
        [&](const auto &payload) {
          using T = std::decay_t<decltype(payload)>;
          if constexpr (std::is_same_v<T, PoleFanMap>) {
            if (payload.pole == MeshPole::none ||
                row(d.quotients, payload.quotient).face != cell.face ||
                row(d.quotients, payload.quotient).pole != payload.pole)
              invalid("pole cell has no matching source quotient");
            for (auto id : list(d.site_refs, payload.cap_cut_chain))
              if (row(d.sites, id).face != cell.face)
                invalid("pole cut site face mismatch");
          } else {
            for (auto site : payload.sites)
              if (row(d.sites, site).face != cell.face)
                invalid("cell site belongs to another face");
            if constexpr (std::is_same_v<T, PlaneTriangleMap>)
              if (payload.chart != cell.chart)
                invalid("plane cell chart mismatch");
          }
        },
        map.map);
  }
  if (!complete(owned_maps) || !complete(owned_associations))
    invalid("orphan cell map/facet association");
  for (const auto &a : d.cell_facets) {
    const auto &cell = row(d.cells, a.cell);
    const auto &map = row(d.cell_maps, cell.map);
    std::array<MNode, 3> roles{};
    std::visit(
        [&](const auto &p) {
          using T = std::decay_t<decltype(p)>;
          if constexpr (std::is_same_v<T, PoleFanMap>) {
            if (a.interpolant != MeshInterpolant::pole_bilinear_fan)
              invalid("pole map interpolant mismatch");
            roles = {row(d.quotients, p.quotient).node, p.edge_nodes[0], p.edge_nodes[1]};
          } else {
            if (a.interpolant != MeshInterpolant::barycentric_triangle)
              invalid("triangle map interpolant mismatch");
            for (std::size_t j = 0; j < 3; ++j)
              roles[j] = row(d.sites, p.sites[j]).node;
          }
        },
        map.map);
    auto nodes = row(d.facets, a.facet).nodes;
    std::sort(roles.begin(), roles.end());
    std::sort(nodes.begin(), nodes.end());
    if (roles != nodes)
      invalid("cell actual interpolant/facet physical keys mismatch");
  }
  for (const auto &q : d.quotients) {
    const auto definition = row(d.faces, q.face).definition;
    if (row(d.nodes, q.node).definition != definition ||
        row(d.charts, row(d.faces, q.face).chart).source != q.carrier)
      invalid("quotient source/physical owner mismatch");
    const auto source_kind = s.carriers[s.carrier_index.find(q.carrier).value].kind;
    if ((q.kind == MeshQuotientKind::pole &&
         (source_kind != 4 ||
          (q.pole != MeshPole::sphere_north && q.pole != MeshPole::sphere_south))) ||
        (q.kind == MeshQuotientKind::apex && (source_kind != 3 || q.pole != MeshPole::cone_apex)) ||
        ((q.kind == MeshQuotientKind::joint || q.kind == MeshQuotientKind::seam) &&
         q.pole != MeshPole::none))
      invalid("quotient pole descriptor disagrees with original carrier kind");
    for (auto vertex : list(d.source_vertex_refs, q.source_vertices))
      if (!source_vertices.contains({definition.value, vertex.value}))
        invalid("quotient source vertex outside definition");
    for (auto use : list(d.use_refs, q.uses))
      if (row(d.uses, use).face != q.face)
        invalid("quotient use mismatch");
    for (auto site : list(d.site_refs, q.sites))
      if (row(d.sites, site).face != q.face || row(d.sites, site).node != q.node)
        invalid("quotient site physical key mismatch");
    for (auto cell : list(d.cell_refs, q.cells))
      if (row(d.cells, cell).face != q.face)
        invalid("quotient cell mismatch");
  }
  for (const auto &closure : d.closures) {
    if (row(d.uses, closure.incoming).face != closure.face ||
        row(d.uses, closure.outgoing).face != closure.face ||
        row(d.uses, closure.incoming).source_loop != closure.loop ||
        row(d.uses, closure.outgoing).source_loop != closure.loop)
      invalid("closure original use mismatch");
    for (auto site : closure.endpoints)
      if (row(d.sites, site).face != closure.face)
        invalid("closure site mismatch");
  }
  for (const auto &corner : d.corners)
    if (row(d.faces, corner.face).chart != row(d.sites, corner.site).chart ||
        row(d.charts, row(d.faces, corner.face).chart).source != corner.carrier ||
        corner.orientation !=
            s.faces[s.face_index.find(row(d.faces, corner.face).source).value].orientation)
      invalid("facet corner original source mismatch");
  for (const auto &instance : d.instances) {
    const auto &definition =
        s.definitions[s.definition_index.find(row(d.definitions, instance.definition).source)
                          .value];
    ProductId current = s.root_product_id;
    for (auto id : list(d.usage_paths, instance.path)) {
      const auto &usage = s.usages[s.usage_index.find(id).value];
      if (usage.parent_product_id != current)
        invalid("mesh usage path is disconnected");
      current = usage.child_product_id;
    }
    if (current != instance.product || definition.product_id != instance.product)
      invalid("mesh instance product/definition mismatch");
  }
  for (const auto &appearance : d.appearances) {
    if (appearance.roughness < 0 || appearance.roughness > 1)
      invalid("mesh roughness outside range");
    for (double x : appearance.rgb)
      if (x < 0 || x > 1)
        invalid("mesh color outside range");
  }
  for (const auto &component : d.components)
    if (exact_sign(component.value) < 0)
      invalid("negative canonical mesh allowance/bound");
}
} // namespace
NativeMeshBuilder::NativeMeshBuilder(const std::shared_ptr<RunResource> &resource,
                                     SourceStore source) {
  if (!resource)
    invalid("null mesh run resource");
  data_ = std::allocate_shared<NativeMeshData>(RunAllocator<NativeMeshData>(resource), resource,
                                               std::move(source));
}
NativeMeshData &NativeMeshBuilder::data() {
  if (!data_)
    invalid("mesh builder already sealed");
  return *data_;
}
NativeMesh NativeMeshBuilder::seal() {
  bgfd::Budget budget({UINT64_MAX, uint64_t{1} << 40}, data().resource.get());
  return seal(budget);
}
NativeMesh NativeMeshBuilder::seal(bgfd::Budget &budget) {
  if (!data_)
    invalid("mesh builder already sealed");
  try {
    budget.charge(0);
    validate(*data_, budget);
  } catch (const bgfd::Failure &failure) {
    throw Error(failure.status == BGFD_RESOURCE_EXHAUSTED ? ErrorCode::resource_exhausted
                                                          : ErrorCode::invalid_source,
                failure.what());
  } catch (const bgfd::Cancelled &) {
    throw Error(ErrorCode::cancelled, "mesh publication cancelled");
  }
  NativeMesh result(data_);
  data_.reset();
  return result;
}
} // namespace bgmesh
