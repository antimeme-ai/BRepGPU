#include "adaptive_trimmed_io.hpp"
#include "adaptive_caller_io.hpp"
#include <algorithm>
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
  const auto &caps = mesh.domain->caps;
  const bool extended =
      key.domain_revision != 0 || key.face_generation != key.boundary_generation ||
      (caps && !caps.caps().empty()) ||
      std::ranges::any_of(mesh.source_bindings,
                          [](const auto &binding) { return binding.wedge_sector.has_value(); }) ||
      std::ranges::any_of(mesh.facets, [](const auto &facet) { return facet.mapped.has_value(); });
  require(!extended || static_cast<bool>(caps), "extended face artifact lacks produced cap domain");
  if (caps) {
    const auto &identity = caps.key();
    require(caps.curves() == mesh.curves && &caps.source().view() == &mesh.surface_source.view() &&
                identity.source_generation == key.source_generation &&
                identity.query == key.query && identity.definition == key.definition &&
                identity.face == key.face &&
                identity.boundary_generation == key.boundary_generation &&
                identity.face_generation == key.face_generation &&
                identity.chart_revision == key.chart_revision &&
                identity.domain_revision == key.domain_revision &&
                identity.assignment_epoch == key.assignment_epoch &&
                identity.profile_revision == key.profile_revision,
            "face artifact cap/source key mismatch");
  }
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
  out.word<4>(extended ? 2 : 1);
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
    require((facet.key.generator == FacetGenerator::mapped_cap) == facet.mapped.has_value(),
            "face artifact mapped generator/payload mismatch");
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
  if (extended) {
    const auto cap_exact = [&](ExactRef ref) { out.rational(caps.values().view(ref)); };
    const auto boundary_key = [&](BoundaryKey value) {
      out.word<8>(value.owner);
      out.word<8>(value.interval);
      out.word<4>(static_cast<uint32_t>(value.kind));
    };
    std::pmr::vector<uint32_t> wedge_remap(result->resource.get());
    for (const auto &local : caps.wedges()) {
      uint32_t found = UINT32_MAX;
      for (uint32_t i = 0; i < mesh.curves->seam_wedges.size(); ++i) {
        budget.charge(1);
        const auto &original = mesh.curves->seam_wedges[i];
        if (local.vertex != original.vertex || local.loop != original.loop ||
            local.meridian_uses != original.meridian_uses ||
            local.shared_endpoint_sites != original.shared_endpoint_sites ||
            local.kind != original.kind)
          continue;
        require(found == UINT32_MAX, "face artifact wedge identity is duplicated");
        for (uint32_t j = 0; j < 4; ++j)
          require(import_exact(budget, caps.values().view(local.finite_connector_uv[j]))
                          .compare(import_exact(budget, mesh.curves->values.view(
                                                            original.finite_connector_uv[j]))) == 0,
                  "face artifact finite connector changed");
        found = i;
      }
      require(found != UINT32_MAX, "face artifact wedge absent from paired snapshot");
      wedge_remap.push_back(found);
    }
    for (uint64_t value : {key.face_generation, key.domain_revision, key.assignment_epoch})
      out.word<8>(value);
    const auto mapped_count = std::ranges::count_if(
        mesh.facets, [](const auto &facet) { return facet.mapped.has_value(); });
    for (uint64_t value : {uint64_t(caps.caps().size()), uint64_t(caps.points().size()),
                           uint64_t(caps.pieces().size()), uint64_t(mapped_count)})
      out.word<8>(value);
    for (const auto &cap : caps.caps()) {
      require(cap.wedge < wedge_remap.size() &&
                  cap.pole_boundary_node < mesh.curves->boundary->nodes.size(),
              "face artifact cap row exceeds retained source pool");
      check_range(cap.points, caps.points().size());
      check_range(cap.pieces, caps.pieces().size());
      out.word<4>(wedge_remap[cap.wedge]);
      out.word<4>(cap.pole_boundary_node);
      for (auto sector : cap.angular_to_sector)
        out.word<4>(sector);
      out.word<8>(source.loops.at(caps.wedges()[cap.wedge].loop.value).id.value);
      for (uint64_t value : {uint64_t(cap.points.first), uint64_t(cap.points.count),
                             uint64_t(cap.pieces.first), uint64_t(cap.pieces.count)})
        out.word<8>(value);
      out.word<4>(cap.transcendental_bits);
      out.word<4>(cap.norm_fractional_bits);
      for (auto interval : cap.meridian_intervals)
        boundary_key(interval);
    }
    require(mesh.cap_point_bindings.size() == caps.points().size(),
            "face artifact missing cut bindings");
    for (uint32_t i = 0; i < caps.points().size(); ++i) {
      const auto &point = caps.points()[i];
      for (auto ref : point.uv)
        cap_exact(ref);
      if (const auto *physical = std::get_if<PhysicalCapEndpoint>(&point.authority)) {
        out.word<4>(1);
        out.word<8>(source.edges.at(physical->edge.value).id.value);
        out.word<8>(source.coedges.at(physical->sector.value).id.value);
        cap_exact(physical->original_t);
        out.word<4>(static_cast<uint32_t>(physical->expected_key.generator));
        for (auto value : {physical->expected_key.source, physical->expected_key.generation,
                           physical->expected_key.ordinal})
          out.word<8>(value);
      } else {
        out.word<4>(2);
        out.word<4>(local_root(std::get<SurfaceCapPoint>(point.authority).original_root));
      }
      const auto &binding = mesh.cap_point_bindings[i];
      require(binding.construction_site < mesh.sites.size(),
              "face artifact cut binding lacks actual site");
      out.word<4>(binding.construction_site);
      out.word<4>(binding.sample_row);
    }
    for (const auto &piece : caps.pieces()) {
      require(piece.cap < caps.caps().size(), "face artifact piece references absent cap");
      out.word<4>(piece.cap);
      out.word<4>(local_root(piece.original_root));
      for (auto point : piece.cut_points) {
        require(point < caps.points().size(), "face artifact piece references absent cut point");
        out.word<4>(point);
      }
      for (auto ref : piece.cut_uv)
        cap_exact(ref);
      for (auto ref : piece.finite_v)
        cap_exact(ref);
      for (auto role : piece.positive_chart_roles)
        out.word<4>(role);
      out.word<4>(static_cast<uint32_t>(piece.regular_sheet_sign));
      boundary_key(piece.cut_key);
    }
    for (uint32_t i = 0; i < mesh.facets.size(); ++i) {
      if (!mesh.facets[i].mapped)
        continue;
      const auto &mapped = *mesh.facets[i].mapped;
      require(mapped.cap < caps.caps().size() && mapped.piece < caps.pieces().size(),
              "face artifact mapped facet references absent source map");
      out.word<4>(i);
      out.word<4>(mapped.cap);
      out.word<4>(mapped.piece);
      for (auto role : mapped.source_role_to_facet)
        out.word<4>(role);
      out.word<4>(static_cast<uint32_t>(mapped.normal.kind));
      out.word<4>(static_cast<uint32_t>(mapped.normal.regular_sheet_sign));
      for (auto ref : mapped.normal.angular_interval)
        exact(ref);
    }
    for (const auto &binding : mesh.source_bindings) {
      out.word<4>(binding.wedge_sector.has_value());
      if (binding.wedge_sector) {
        require(binding.wedge_sector->wedge < wedge_remap.size() &&
                    binding.wedge_sector->sector < 2,
                "face artifact wedge binding outside retained source pool");
        out.word<4>(wedge_remap[binding.wedge_sector->wedge]);
        out.word<4>(binding.wedge_sector->sector);
      }
    }
  }
  out.finish();
}
} // namespace bgmesh::adaptive::caller
