#include "adaptive_trimmed_io.hpp"
#include "adaptive_caller_io.hpp"
#include <bit>
#include <cmath>

namespace bgmesh::adaptive::caller {
namespace {
void require(bool okay, const char *message) {
  if (!okay)
    throw Error(ErrorCode::internal_error, message);
}
template <class Range> void check_range(const Range &range, std::size_t count) {
  require(range.first <= count && range.count <= count - range.first,
          "trimmed artifact range exceeds retained table");
}
} // namespace
void write_trimmed_face(const std::filesystem::path &path, const FaceTrimResult &result,
                        bgfd::Budget &budget) {
  require(result && result->construction && result->boundary && result->surface,
          "trimmed artifact requires complete accepted face");
  const auto &mesh = *result->construction;
  require(mesh.curves == result->boundary && mesh.surface.key() == result->surface.key() &&
              mesh.domain && mesh.domain->curves == result->boundary,
          "trimmed artifact has mixed generations");
  const auto &source = mesh.source.view();
  const auto &key = mesh.key;
  const auto &face = source.faces.at(key.face.value);
  const auto &carrier = source.carriers.at(source.carrier_index.find(face.carrier_id).value);
  require(face.definition_id == source.definitions.at(key.definition.value).id &&
              key.profile_revision <= UINT32_MAX,
          "trimmed artifact face/source mismatch");
  const auto roots = mesh.surface_source.original_roots();
  require(roots.size() <= UINT32_MAX, "trimmed source bank root count exhausted");
  // The fixed service bank can contain many carriers. Keep the complete
  // original closure of THIS carrier, including branches outside the trim,
  // while giving the public face artifact its own canonical root ordinals.
  std::pmr::vector<uint32_t> local_roots(result->resource.get());
  std::pmr::vector<uint32_t> root_remap(roots.size(), UINT32_MAX, result->resource.get());
  for (uint32_t i = 0; i < roots.size(); ++i) {
    budget.charge(1);
    if (roots[i].source_id != carrier.id.value)
      continue;
    require(local_roots.size() < UINT32_MAX, "trimmed original root count exhausted");
    root_remap[i] = static_cast<uint32_t>(local_roots.size());
    local_roots.push_back(i);
  }
  require(!local_roots.empty(), "trimmed artifact has no original carrier closure");
  const auto local_root = [&](OriginalRootSlot original) {
    require(original.value < root_remap.size() && root_remap[original.value] != UINT32_MAX,
            "trimmed artifact references another carrier's original root");
    return root_remap[original.value];
  };
  const auto selection = mesh.surface.selection();
  // Facets are ordered by semantic key, so one row per persistent packet
  // owner suffices. Cache leaf ordinals remain private implementation details.
  std::pmr::vector<const RegionFacet *> packets(result->resource.get());
  for (const auto &facet : mesh.facets) {
    if (facet.key.generator != FacetGenerator::packet)
      continue;
    require(facet.leaf < selection->leaves.size(), "packet references missing selected leaf");
    if (packets.empty() || packets.back()->key.owner != facet.key.owner)
      packets.push_back(&facet);
    else
      require(packets.back()->key.revision == facet.key.revision &&
                  packets.back()->leaf == facet.leaf,
              "one packet owner has mixed revisions or leaves");
  }
  Wire out(path, budget);
  constexpr char magic[] = "BGATRM01";
  out.bytes(std::as_bytes(std::span(magic).first(8)));
  out.word<4>(1);
  out.word<4>(key.profile_revision);
  out.bytes(source.source_digest);
  for (uint64_t value :
       {key.source_generation, key.query, key.boundary_generation, key.chart_revision,
        key.construction_revision, mesh.surface.key().selection.cover_revision,
        source.definitions.at(key.definition.value).id.value, face.id.value, carrier.id.value})
    out.word<8>(value);
  out.word<4>(static_cast<uint32_t>(face.orientation));
  uint32_t domain_policy = static_cast<uint32_t>(DomainPolicy::reconstructed_selected);
  for (const auto &use : mesh.domain->uses)
    if (use.policy == DomainPolicy::authoritative_exact_image)
      domain_policy = static_cast<uint32_t>(DomainPolicy::authoritative_exact_image);
  out.word<4>(domain_policy);
  for (uint64_t value :
       {uint64_t(mesh.sites.size()), uint64_t(mesh.source_bindings.size()),
        uint64_t(mesh.loop_sites.size()), uint64_t(mesh.edges.size()), uint64_t(mesh.loops.size()),
        uint64_t(mesh.components.size()), uint64_t(mesh.facets.size()),
        uint64_t(local_roots.size()), uint64_t(mesh.domain->uses.size()), uint64_t(packets.size())})
    out.word<8>(value);
  const auto exact = [&](ExactRef ref) { out.rational(mesh.values.view(ref)); };
  const auto scalar = [&](double value) {
    require(std::isfinite(value), "trimmed artifact contains nonfinite scalar");
    out.word<8>(std::bit_cast<uint64_t>(value));
  };
  for (auto ref : mesh.fixed_metric_factor)
    exact(ref);
  // This is the accepted threshold upper for ALL facets, not an observed max.
  out.rational(result->bound_values.view(result->positional_upper));
  out.rational(result->bound_values.view(result->requested_node_source));
  for (const auto original : local_roots) {
    const auto &root = roots[original];
    out.word<8>(root.source_id);
    out.word<4>(root.span_u);
    out.word<4>(root.span_v);
    out.word<4>(static_cast<uint32_t>(root.family));
    out.word<4>(root.degree_u);
    out.word<4>(root.degree_v);
    for (auto endpoint : root.original_box) {
      require(endpoint.lower == endpoint.upper, "original root endpoint is not exact binary64");
      scalar(endpoint.lower);
    }
  }
  for (const auto *packet : packets) {
    const auto &leaf = selection->leaves[packet->leaf];
    require(leaf.key.root.value < mesh.surface_source.roots().size(),
            "packet prepared root missing");
    const auto &root = mesh.surface_source.roots()[leaf.key.root.value];
    out.word<8>(packet->key.owner);
    out.word<8>(packet->key.revision);
    out.word<4>(local_root(root.original));
    out.word<4>(root.span_u);
    out.word<4>(root.span_v);
    for (auto endpoint : root.prepared_box) {
      require(endpoint.lower == endpoint.upper, "prepared endpoint is not exact binary64");
      scalar(endpoint.lower);
    }
    check_range(leaf.key.path, selection->path_words.size());
    require(leaf.key.depth <= 128 && leaf.key.path.count == (uint64_t(leaf.key.depth) + 31) / 32,
            "packet path shape outside finite grid profile");
    out.word<4>(leaf.key.depth);
    out.word<4>(leaf.key.path.count);
    for (uint64_t i = 0; i < leaf.key.path.count; ++i)
      out.word<8>(selection->path_words[leaf.key.path.first + i]);
    for (auto ref : leaf.box)
      out.rational(selection->values.view(ref));
  }
  for (const auto &site : mesh.sites) {
    const auto *physical = std::get_if<BoundarySiteSource>(&site.source);
    const auto *surface = std::get_if<SurfaceSiteSource>(&site.source);
    out.word<4>(physical ? 1 : surface->anchor ? 3 : 2);
    out.word<8>(site.constructed_revision);
    for (auto value : site.actual_xyz)
      scalar(value);
    for (auto ref : site.uv)
      exact(ref);
    if (physical) {
      require(physical->boundary_node < mesh.curves->boundary->nodes.size(),
              "trimmed site references missing physical row");
      out.word<4>(physical->boundary_node);
    } else {
      out.word<4>(local_root(surface->original));
      out.word<4>(static_cast<uint32_t>(surface->stage));
      out.word<8>(surface->birth_horizon);
    }
  }
  for (const auto &binding : mesh.source_bindings) {
    require(binding.shared_site < mesh.curves->sites.size() &&
                binding.construction_site < mesh.sites.size(),
            "trimmed source occurrence binding exceeds table");
    out.word<4>(binding.shared_site);
    out.word<4>(binding.construction_site);
  }
  for (auto site : mesh.loop_sites) {
    require(site < mesh.sites.size(), "trimmed loop references missing site");
    out.word<4>(site);
  }
  for (const auto &edge : mesh.edges) {
    for (auto site : edge.sites) {
      require(site < mesh.sites.size(), "trimmed edge references missing site");
      out.word<4>(site);
    }
    if (const auto *physical = std::get_if<BoundaryKey>(&edge.provenance)) {
      out.word<4>(1);
      out.word<8>(physical->owner);
      out.word<8>(physical->interval);
      out.word<4>(static_cast<uint32_t>(physical->kind));
    } else {
      const auto &rim = std::get<RimIntervalKey>(edge.provenance);
      out.word<4>(2);
      out.word<8>(rim.footprint_side);
      out.word<8>(rim.revision);
      out.word<4>(rim.ordinal);
    }
  }
  for (const auto &loop : mesh.loops) {
    check_range(loop.sites, mesh.loop_sites.size());
    check_range(loop.edges, mesh.edges.size());
    out.word<4>(loop.positive);
    for (uint64_t value : {uint64_t(loop.sites.first), uint64_t(loop.sites.count),
                           uint64_t(loop.edges.first), uint64_t(loop.edges.count)})
      out.word<8>(value);
    out.word<8>(loop.source_loop ? source.loops.at(loop.source_loop->value).id.value : 0);
    require(loop.traversal_adapter == 1 || loop.traversal_adapter == -1,
            "invalid original loop traversal adapter");
    out.word<4>(static_cast<uint32_t>(loop.traversal_adapter));
  }
  for (const auto &component : mesh.components) {
    check_range(component.loops, mesh.loops.size());
    check_range(component.facets, mesh.facets.size());
    for (uint64_t value : {component.id, component.revision, uint64_t(component.loops.first),
                           uint64_t(component.loops.count), uint64_t(component.facets.first),
                           uint64_t(component.facets.count)})
      out.word<8>(value);
  }
  for (const auto &facet : mesh.facets) {
    out.word<4>(static_cast<uint32_t>(facet.key.generator));
    out.word<8>(facet.key.owner);
    out.word<8>(facet.key.revision);
    out.word<4>(facet.key.ordinal);
    for (auto site : facet.sites) {
      require(site < mesh.sites.size(), "trimmed facet references missing site");
      out.word<4>(site);
    }
    require(facet.component == UINT32_MAX || facet.component < mesh.components.size(),
            "trimmed facet references missing component");
    out.word<4>(facet.component);
  }
  for (const auto &use : mesh.domain->uses) {
    out.word<8>(source.coedges.at(use.use.value).id.value);
    out.word<4>(static_cast<uint32_t>(use.policy));
    out.word<4>(use.image.has_value());
    if (!use.image)
      continue;
    const auto &image = *use.image;
    check_range(image.chosen_sites, mesh.curves->sites.size());
    out.word<8>(source.curves.at(image.raw_curve.value).id.value);
    out.word<4>(static_cast<uint32_t>(image.raw_traversal));
    out.word<8>(image.chosen_sites.first);
    out.word<8>(image.chosen_sites.count);
    const auto domain_exact = [&](ExactRef ref) { out.rational(mesh.domain->values.view(ref)); };
    for (auto ref : image.endpoints)
      domain_exact(ref);
    for (auto ref : image.covector)
      domain_exact(ref);
    for (auto ref : image.scalar_endpoints)
      domain_exact(ref);
  }
  out.finish();
}
} // namespace bgmesh::adaptive::caller
