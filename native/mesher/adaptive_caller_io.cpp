#include "adaptive_caller_io.hpp"
#include <algorithm>
#include <bit>
#include <cmath>

namespace bgmesh::adaptive::caller {
void write_curves(const std::filesystem::path &path, const SharedCurveSnapshotData &mesh,
                  bgfd::Budget &budget) {
  const auto &s = mesh.source.view();
  const auto &b = *mesh.boundary;
  if (mesh.physical_intervals.size() != b.intervals.size() ||
      b.nodes.size() != b.constructed_revisions.size() || b.nodes.size() != b.sample_paths.size())
    throw Error(ErrorCode::internal_error, "incomplete shared-boundary row inventory");
  Wire out(path, budget);
  constexpr std::array magic{std::byte{'B'}, std::byte{'G'}, std::byte{'A'}, std::byte{'C'},
                             std::byte{'R'}, std::byte{'V'}, std::byte{'0'}, std::byte{'1'}};
  out.bytes(magic);
  out.word<4>(mesh.seam_wedges.empty() ? 2 : 3); // Explicit whole-use line-image record; interval
                                                 // semantics stay unchanged.
  out.word<4>(1);
  for (uint64_t value : {s.definitions[mesh.definition.value].id.value, uint64_t(b.nodes.size()),
                         uint64_t(b.edges.size()), uint64_t(b.vertex_aliases.size()),
                         uint64_t(mesh.physical_intervals.size()), uint64_t(mesh.uses.size()),
                         uint64_t(mesh.sites.size()), uint64_t(mesh.intervals.size())})
    out.word<8>(value);
  out.bytes(s.source_digest);
  const auto rational = [&](ExactRef ref) { out.rational(mesh.values.view(ref)); };
  const auto optional_exact = [&](const std::optional<ExactRef> &ref) {
    out.word<4>(ref.has_value());
    if (ref)
      rational(*ref);
  };
  const auto key = [&](BoundaryKey value) {
    out.word<8>(value.owner);
    out.word<8>(value.interval);
    out.word<4>(static_cast<uint32_t>(value.kind));
  };
  const auto line = [&](const std::optional<GeometricLineRecord> &value) {
    out.word<4>(value.has_value());
    if (!value)
      return;
    for (auto ref : value->covector)
      rational(ref);
    for (auto ref : value->curve_scalar)
      rational(ref);
    for (auto ref : value->trace_scalar)
      rational(ref);
    for (auto ref : value->original_t)
      rational(ref);
    out.word<4>(static_cast<uint32_t>(value->traversal));
  };
  const auto range = [](uint64_t first, uint64_t count, uint64_t size) {
    if (first > size || count > size - first)
      throw Error(ErrorCode::internal_error, "shared-boundary output range outside retained rows");
  };
  for (std::size_t i = 0; i < b.nodes.size(); ++i) {
    const auto &node = b.nodes[i];
    if (!node.original_parameter)
      throw Error(ErrorCode::internal_error, "boundary node lacks original parameter");
    out.word<4>(static_cast<uint32_t>(node.key.generator));
    for (uint64_t value :
         {node.key.source, node.key.generation, node.key.ordinal, b.constructed_revisions[i]})
      out.word<8>(value);
    for (double value : node.xyz) {
      if (!std::isfinite(value))
        throw Error(ErrorCode::internal_error, "nonfinite boundary XYZ");
      out.word<8>(std::bit_cast<uint64_t>(value));
    }
    out.rational(b.parameters.view(*node.original_parameter));
    const auto path_bits = b.sample_paths[i];
    range(path_bits.first, path_bits.count, b.path_bits.size());
    if (path_bits.count > UINT32_MAX)
      throw Error(ErrorCode::resource_exhausted, "boundary path exceeds wire index");
    out.word<4>(path_bits.count);
    for (uint64_t j = 0; j < path_bits.count; ++j) {
      const auto direction = b.path_bits[path_bits.first + j];
      if (direction > 1)
        throw Error(ErrorCode::internal_error, "invalid source sample path");
      const std::array byte{static_cast<std::byte>(direction)};
      out.bytes(byte);
    }
  }
  for (const auto &row : b.edges) {
    const auto &edge = s.edges.at(row.edge.value);
    range(row.nodes.first, row.nodes.count, b.nodes.size());
    range(row.intervals.first, row.intervals.count, b.intervals.size());
    for (uint64_t value :
         {edge.id.value, edge.curve_id.value, edge.vertex_ids[0].value, edge.vertex_ids[1].value,
          row.nodes.first, row.nodes.count, row.intervals.first, row.intervals.count})
      out.word<8>(value);
    for (double value : edge.parameter_range)
      out.word<8>(std::bit_cast<uint64_t>(value));
    for (uint32_t value : {uint32_t(edge.curve_same_sense), uint32_t(edge.same_parameter),
                           uint32_t(edge.degenerate), uint32_t{0}})
      out.word<4>(value);
  }
  for (const auto &alias : b.vertex_aliases) {
    out.word<8>(s.vertices.at(alias.original.value).id.value);
    out.word<8>(s.vertices.at(alias.representative.value).id.value);
  }
  for (std::size_t i = 0; i < mesh.physical_intervals.size(); ++i) {
    const auto &row = mesh.physical_intervals[i];
    if (row.boundary_key != b.intervals[i])
      throw Error(ErrorCode::internal_error, "physical interval ordering differs from boundary");
    key(row.boundary_key);
    for (auto node : row.boundary_nodes) {
      if (node >= b.nodes.size())
        throw Error(ErrorCode::internal_error, "physical interval references absent node");
      out.word<4>(node);
    }
    for (auto ref : row.original_t)
      rational(ref);
    rational(row.parameter_chord_upper);
    optional_exact(row.image_chord_upper);
    out.word<4>(static_cast<uint32_t>(row.selected_correspondence));
    line(row.image_map);
  }
  for (const auto &row : mesh.uses) {
    const auto &use = s.coedges.at(row.use.value);
    const auto &face = s.faces.at(row.face.value);
    range(row.sites.first, row.sites.count, mesh.sites.size());
    range(row.intervals.first, row.intervals.count, mesh.intervals.size());
    for (uint64_t value :
         {use.id.value, face.id.value, use.edge_id.value, face.carrier_id.value, use.loop_id.value})
      out.word<8>(value);
    out.word<4>(static_cast<uint32_t>(use.sense));
    out.word<4>(use.reconstructed);
    out.word<4>(use.parameter_map_kind);
    for (int32_t value : use.periodic_shift)
      out.word<4>(static_cast<uint32_t>(value));
    out.word<4>(static_cast<uint32_t>(row.authority));
    for (uint64_t value : {row.sites.first, row.sites.count, row.intervals.first,
                           row.intervals.count, row.face_generation, row.boundary_generation})
      out.word<8>(value);
    out.word<4>(row.whole_line_image.has_value());
    if (row.whole_line_image) {
      const auto &image = *row.whole_line_image;
      out.word<8>(s.curves.at(image.raw_curve.value).id.value);
      out.word<4>(static_cast<uint32_t>(image.raw_traversal));
      for (auto ref : image.endpoints)
        rational(ref);
      for (auto ref : image.covector)
        rational(ref);
      for (auto ref : image.scalar_endpoints)
        rational(ref);
    }
  }
  for (const auto &row : mesh.sites) {
    if (row.boundary_node >= b.nodes.size())
      throw Error(ErrorCode::internal_error, "use site references absent boundary node");
    out.word<4>(row.boundary_node);
    out.word<8>(s.loops.at(row.loop.value).id.value);
    out.word<4>(row.junction_occurrence);
    for (int32_t value : row.original_periodic_shift)
      out.word<4>(static_cast<uint32_t>(value));
    for (int32_t value : row.selected_branch)
      out.word<4>(static_cast<uint32_t>(value));
    rational(row.original_t);
    rational(row.mapped_s);
    for (auto ref : row.chosen_original_uv)
      rational(ref);
    for (auto ref : row.chart_xy)
      rational(ref);
    rational(row.node_source_upper);
  }
  for (const auto &row : mesh.intervals) {
    key(row.boundary_key);
    for (auto site : row.sites) {
      if (site >= mesh.sites.size())
        throw Error(ErrorCode::internal_error, "use interval references absent site");
      out.word<4>(site);
    }
    out.word<4>(static_cast<uint32_t>(row.authority));
    out.word<4>(static_cast<uint32_t>(row.correspondence));
    rational(row.carrier_trace_to_actual_chord_upper);
    rational(row.source_to_selected_trace_upper);
    for (auto ref : row.uv_chord_component_upper)
      rational(ref);
    optional_exact(row.authoritative_domain_upper);
    line(row.image_map);
    out.word<4>(row.cylinder_identity.has_value());
    if (row.cylinder_identity) {
      const auto &identity = *row.cylinder_identity;
      out.word<8>(s.curves.at(identity.spatial.value).id.value);
      out.word<8>(s.carriers.at(identity.carrier.value).id.value);
      out.word<4>(static_cast<uint32_t>(identity.orientation));
      for (auto ref : identity.original_t)
        rational(ref);
      for (auto ref : identity.chosen_uv)
        rational(ref);
      rational(identity.constant_v);
    }
  }
  if (!mesh.seam_wedges.empty()) {
    out.word<8>(mesh.seam_wedges.size());
    for (const auto &wedge : mesh.seam_wedges) {
      if (wedge.vertex.value >= s.vertices.size() || wedge.loop.value >= s.loops.size())
        throw Error(ErrorCode::internal_error,
                    "wedge artifact references absent source vertex/loop");
      out.word<8>(s.vertices[wedge.vertex.value].id.value);
      out.word<8>(s.loops[wedge.loop.value].id.value);
      for (auto use : wedge.meridian_uses) {
        if (use.value >= s.coedges.size() ||
            s.coedges[use.value].loop_id != s.loops[wedge.loop.value].id)
          throw Error(ErrorCode::internal_error, "wedge artifact source occurrence mismatch");
        out.word<8>(s.coedges[use.value].id.value);
      }
      for (auto site : wedge.shared_endpoint_sites) {
        if (site >= mesh.sites.size())
          throw Error(ErrorCode::internal_error, "wedge artifact references absent source site");
        out.word<4>(site);
      }
      out.word<4>(static_cast<uint32_t>(wedge.kind));
      for (auto ref : wedge.finite_connector_uv)
        rational(ref);
    }
  }
  out.finish();
}
void digest(std::ostream &out, const SourceDigest &source) {
  constexpr std::string_view digits = "0123456789abcdef";
  for (auto byte : source) {
    const auto value = std::to_integer<unsigned>(byte);
    out << digits[value >> 4] << digits[value & 15];
  }
}
CurveRequest endpoint_curve_request(const SourceData &source, Slot<CurveTag> slot, double t) {
  const auto &curve = source.curves.at(slot.value);
  auto domain = curve.parameter_range;
  if (curve.kind == 4)
    domain = {0., 1.};
  if (curve.kind == 5) {
    const auto knots = source.view(curve.knots);
    const auto mult = source.view(curve.multiplicities);
    const auto controls = source.view(curve.controls).size() / (curve.dimension + 1);
    uint64_t expanded = 0;
    bool first = false, last = false;
    for (std::size_t i = 0; i < knots.size(); ++i) {
      const auto end = expanded + mult[i];
      if (expanded <= curve.degree && curve.degree < end) {
        domain[0] = knots[i];
        first = true;
      }
      if (expanded <= controls && controls < end) {
        domain[1] = knots[i];
        last = true;
      }
      expanded = end;
    }
    if (first && last)
      for (std::size_t i = 1; i < knots.size(); ++i)
        if (knots[i - 1] >= domain[0] && knots[i] <= domain[1] && knots[i - 1] < knots[i] &&
            knots[i - 1] <= t && t <= knots[i])
          return {slot, {knots[i - 1], knots[i]}};
    throw Error(ErrorCode::unsupported, "zero-range edge has no original positive spline span");
  }
  if (!(domain[0] < domain[1] && domain[0] <= t && t <= domain[1]))
    throw Error(ErrorCode::unsupported, "zero-range edge has no original positive curve span");
  return {slot, domain};
}
std::pmr::vector<CurveRequest>
definition_curve_requests(const SourceStore &source, Slot<DefinitionTag> definition,
                          const std::shared_ptr<RunResource> &resource) {
  const auto &s = source.view();
  std::pmr::vector<CurveRequest> requests(resource.get());
  for (auto edge_slot : s.definition_edges.at(definition.value)) {
    const auto &edge = s.edges.at(edge_slot);
    // A declared quotient has no positive physical interval, but its original
    // endpoint Ce facts still need a real original evaluator span.
    auto physical_domain = edge.parameter_range;
    if (physical_domain[1] < physical_domain[0])
      std::swap(physical_domain[0], physical_domain[1]);
    if (edge.curve_id) {
      const auto curve = s.curve_index.find(edge.curve_id);
      requests.push_back(physical_domain[0] < physical_domain[1]
                             ? CurveRequest{curve, physical_domain}
                             : endpoint_curve_request(s, curve, physical_domain[0]));
    }
    for (auto use_slot : s.edge_coedges.at(edge_slot)) {
      const auto &use = s.coedges.at(use_slot);
      const auto &loop = s.loops.at(s.loop_index.find(use.loop_id).value);
      const auto &face = s.faces.at(s.face_index.find(loop.face_id).value);
      if (face.definition_id != s.definitions[definition.value].id)
        continue;
      if (use.parameter_map_kind == 4)
        throw Error(ErrorCode::unsupported, measured_parameter_map_refusal);
      if (use.parameter_map_kind != 1 && use.parameter_map_kind != 2)
        throw Error(ErrorCode::unsupported, "unimplemented original use parameter map");
      // Keep every occurrence of THIS definition, not one guide per edge. A
      // kind2 map may demand values beyond the saved guide range.
      auto domain = physical_domain;
      if (use.parameter_map_kind == 2) {
        const auto map =
            std::span(s.parameter_maps).subspan(use.parameter_map.first, use.parameter_map.count);
        if (map.empty())
          throw Error(ErrorCode::invalid_source, "empty original piecewise parameter map");
        domain = {map.front()[1], map.front()[1]};
        for (const auto &pair : map) {
          domain[0] = std::min(domain[0], pair[1]);
          domain[1] = std::max(domain[1], pair[1]);
        }
      }
      const auto guide = s.curve_index.find(use.curve_id);
      requests.push_back(domain[0] < domain[1] ? CurveRequest{guide, domain}
                                               : endpoint_curve_request(s, guide, domain[0]));
    }
  }
  return requests;
}
} // namespace bgmesh::adaptive::caller
