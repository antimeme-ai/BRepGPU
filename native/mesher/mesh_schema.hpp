#ifndef BREPGPU_MESHER_MESH_SCHEMA_HPP
#define BREPGPU_MESHER_MESH_SCHEMA_HPP
#include "mesh.hpp"
#include <type_traits>

// One schema traversal drives the writer and decoder. These are semantic rows,
// not native struct images; every reserved field and variant is explicit.
// Keep the declarative X-macro schema stable across formatter passes.
// clang-format off
#define BGNMESH_BEFORE_QUALITY(X) \
  X("DEFS____", MeshDefinition, definitions, 0) \
  X("FACES___", MeshFace, faces, 0) \
  X("NODES___", MeshNode, nodes, 0) \
  X("SITES___", MeshSite, sites, 0) \
  X("LINEAGE_", MeshLineage, lineage, 0) \
  X("EDGES___", MeshEdge, edges, 0) \
  X("EDGSAM__", MeshEdgeSample, edge_samples, 0) \
  X("JOINTS__", MeshJoint, joints, 0) \
  X("USES____", MeshUse, uses, 0) \
  X("USESAM__", MeshUseSample, use_samples, 0) \
  X("LOOPS___", MeshLoop, loops, 0) \
  X("CHARTS__", MeshChart, charts, 0) \
  X("DOMMAPS_", MeshDomainMap, domain_maps, 0) \
  X("SUPPORTS", MeshSupport, supports, 0) \
  X("PROFILES", MeshProfile, profiles, 0) \
  X("CELLS___", MeshCell, cells, 0) \
  X("CELLMAPS", MeshCellMap, cell_maps, 0) \
  X("CELLFAC_", MeshCellFacet, cell_facets, 0) \
  X("FACETS__", MeshFacet, facets, 64) \
  X("CORNERS_", MeshCorner, corners, 0) \
  X("QUOTIENT", MeshQuotient, quotients, 0) \
  X("CLOSURES", MeshClosure, closures, 0) \
  X("COMPONTS", MeshComponent, components, 0)
#define BGNMESH_AFTER_QUALITY(X) \
  X("INSTS___", MeshInstance, instances, 0) \
  X("APPEAR__", MeshAppearance, appearances, 48)
#define BGNMESH_TABLES(X) BGNMESH_BEFORE_QUALITY(X) BGNMESH_AFTER_QUALITY(X)
namespace bgmesh::mesh_detail {
template <class T, class D> auto &rows(D &data) {
#define BGNMESH_LOOKUP(tag, type, member, width)                                                   \
  if constexpr (std::is_same_v<T, type>)                                                           \
    return data.member;                                                                            \
  else
  BGNMESH_TABLES(BGNMESH_LOOKUP)
#undef BGNMESH_LOOKUP
  static_assert(!std::is_same_v<T, T>, "unknown semantic mesh table");
}
template <class T, class D> auto &pool(D &d) {
#define BGNMESH_POOL(type, member)                                                                 \
  if constexpr (std::is_same_v<T, type>)                                                           \
    return d.member;                                                                               \
  else
  BGNMESH_POOL(MFace, face_refs)
  BGNMESH_POOL(MNode, node_refs)
  BGNMESH_POOL(MSite, site_refs)
  BGNMESH_POOL(MFacet, facet_refs)
  BGNMESH_POOL(MLoop, loop_refs)
  BGNMESH_POOL(MCell, cell_refs)
  BGNMESH_POOL(MLineage, lineage_refs)
  BGNMESH_POOL(MEdgeSample, edge_sample_refs)
  BGNMESH_POOL(MUse, use_refs)
  BGNMESH_POOL(MUseSample, use_sample_refs)
  BGNMESH_POOL(MProfile, profile_refs)
  BGNMESH_POOL(MComponent, component_refs)
  BGNMESH_POOL(MClosure, closure_refs)
  BGNMESH_POOL(MSupport, support_refs)
  BGNMESH_POOL(MCellFacet, cell_facet_refs)
  BGNMESH_POOL(MeshPoint2, points)
  BGNMESH_POOL(uint32_t, interval_paths)
  BGNMESH_POOL(VertexId, source_vertex_refs)
  BGNMESH_POOL(UsageId, usage_paths)
#undef BGNMESH_POOL
  static_assert(!std::is_same_v<T, T>, "unknown mesh list pool");
}
// clang-format on
template <class S> void transfer(S &s, uint64_t &v) { s.u64(v); }
template <class S> void transfer(S &s, uint32_t &v) { s.u32(v); }
template <class S> void transfer(S &s, int32_t &v) { s.i32(v); }
template <class S> void transfer(S &s, double &v) { s.f64(v); }
template <class S> void transfer(S &s, bool &v) {
  uint32_t word = v ? 1u : 0u;
  s.u32(word);
  s.require(word <= 1, "noncanonical mesh bool");
  v = word != 0;
}
template <class S, class T>
  requires std::is_enum_v<T>
void transfer(S &s, T &v) {
  auto word = static_cast<uint32_t>(v);
  s.u32(word);
  uint32_t maximum = 0;
  if constexpr (std::is_same_v<T, MeshLineageOperation>)
    maximum = 4;
  else if constexpr (std::is_same_v<T, MeshReferencePolicy> || std::is_same_v<T, MeshInterpolant> ||
                     std::is_same_v<T, MeshValueRole>)
    maximum = 2;
  else if constexpr (std::is_same_v<T, MeshQuotientKind> || std::is_same_v<T, MeshClosureKind>)
    maximum = 4;
  else if constexpr (std::is_same_v<T, MeshComponentKind>)
    maximum = 9;
  s.require(word >= 1 && word <= maximum, "unknown mesh enum tag");
  v = static_cast<T>(word);
}
template <class S, class T> void transfer(S &s, SourceId<T> &v) { s.source(v); }
template <class S, class T> void transfer(S &s, MeshRef<T> &v) { s.reference(v); }
template <class S> void transfer(S &s, ExactRef &v) { s.exact(v); }
template <class S, std::size_t N, class T> void transfer(S &s, std::array<T, N> &v) {
  for (auto &x : v)
    s.fields(x);
}
template <class S, class T> void transfer(S &s, Range<T> &v) { s.list(v); }
template <class S, class... T> void transfer(S &s, std::variant<T...> &v) { s.variant(v); }
template <class S> void transfer(S &, std::monostate &) {}
template <class S> void transfer(S &s, std::optional<ExactRef> &v) { s.optional_exact(v); }
template <class S> void transfer(S &s, MeshPole &v) {
  auto tag = static_cast<uint32_t>(v);
  s.u32(tag);
  s.zero32();
  s.require(tag <= 3, "unknown pole descriptor");
  v = static_cast<MeshPole>(tag);
}
#define BGNMESH_ROW(type, ...)                                                                     \
  template <class S> void transfer(S &s, type &v) { s.fields(__VA_ARGS__); }
BGNMESH_ROW(MeshDefinition, v.source, v.faces, v.nodes, v.sites, v.facets)
template <class S> void transfer(S &s, MeshFace &v) {
  s.fields(v.source, v.definition, v.chart);
  s.optional_reference(v.domain_map);
  s.fields(v.generation, v.loops, v.cells, v.facets);
}
BGNMESH_ROW(OriginalVertexGenerator, v.vertex)
BGNMESH_ROW(OriginalEdgeGenerator, v.edge, v.parameter)
BGNMESH_ROW(FaceSiteGenerator, v.face, v.site, v.lineage)
BGNMESH_ROW(QuotientGenerator, v.quotient)
template <class S> void transfer(S &s, MeshNode &v) {
  s.fields(v.definition, v.xyz, v.generator);
  s.optional_reference(v.quotient);
}
BGNMESH_ROW(MeshSite, v.face, v.chart, v.xy, v.node, v.branch, v.lineage)
BGNMESH_ROW(MeshLineage, v.face, v.generation, v.operation, v.parents, v.parent_sites)
BGNMESH_ROW(MeshEdge, v.source, v.definition, v.generation, v.samples, v.components)
BGNMESH_ROW(MeshEdgeSample, v.edge, v.parameter, v.node, v.interval_path)
template <class S> void transfer(S &s, MeshLocator &v) {
  auto tag = static_cast<uint32_t>(v.kind);
  s.u32(tag);
  s.zero32();
  s.require(tag >= 1 && tag <= 4, "unknown source locator");
  v.kind = static_cast<MeshLocatorKind>(tag);
  if (tag == 4) {
    s.fields(v.unavailable_reason);
    return;
  }
  s.fields(v.chosen, v.enclosure);
  s.require(v.enclosure[0] <= v.enclosure[1] && v.enclosure[2] <= v.enclosure[3],
            "reversed locator enclosure");
  if (tag == 2)
    s.fields(v.chart);
  else if (tag == 3) {
    s.fields(v.lift);
    s.optional_reference(v.quotient);
  }
}
template <class S> void transfer(S &s, MeshJoint &v) {
  s.fields(v.face, v.source_vertex, v.chart, v.branch, v.site, v.uses, v.recovery);
  s.optional_reference(v.quotient);
}
template <class S> void transfer(S &s, MeshUse &v) {
  s.fields(v.source, v.source_loop, v.face, v.edge, v.sense, v.reconstructed, v.parameter_map_kind);
  s.zero32();
  s.fields(v.lift, v.samples, v.profiles, v.components);
}
BGNMESH_ROW(MeshUseSample, v.use, v.edge_sample, v.site, v.pcurve_parameter, v.raw, v.recovery)
BGNMESH_ROW(MeshLoop, v.source, v.face, v.outer, v.traversal_adapter, v.uses, v.normalized_sites,
            v.closures)
BGNMESH_ROW(OriginalUVChart, v.rectangle)
BGNMESH_ROW(UnwrappedUVChart, v.rectangle, v.lift)
BGNMESH_ROW(PhysicalPlaneChart, v.rectangle, v.projection, v.preconditioner, v.origin, v.basis,
            v.quadrilateral)
BGNMESH_ROW(MeshChart, v.source, v.chart)
template <class S> void transfer(S &s, MeshDomainMap &v) {
  s.fields(v.face, v.policy);
  s.zero32();
  s.fields(v.fans, v.strips);
}
BGNMESH_ROW(OriginalPcurvePoint, v.use, v.parameter)
BGNMESH_ROW(ProjectedPcurvePoint, v.chart, v.use, v.parameter)
BGNMESH_ROW(NodalFanSupport, v.source_point, v.chosen, v.polygon)
BGNMESH_ROW(GraphStripSupport, v.use, v.edge_interval, v.pre_fans, v.endpoints, v.origin,
            v.x_covector, v.y_covector, v.extent, v.c, v.w0)
BGNMESH_ROW(MeshSupport, v.face, v.support)
template <class S> void transfer(S &, AffineParameterProfile &) {}
template <class S> void transfer(S &s, GeometricLineProfile &v) {
  s.fields(v.covector, v.scalar, v.endpoints, v.traversal);
  s.zero32();
  s.require(v.traversal == 1 || v.traversal == -1, "unknown line-profile traversal");
}
template <class S> void transfer(S &s, CylinderArcProfile &v) {
  s.fields(v.dual_frame, v.orientation, v.period_side, v.endpoint_rays, v.halfplane_anchor);
  s.require(v.orientation == 1 || v.orientation == -1, "unknown cylinder-profile orientation");
}
template <class S> void transfer(S &s, MeshProfile &v) {
  s.fields(v.face);
  s.optional_reference(v.use);
  s.fields(v.source_curve, v.source_carrier, v.interval, v.correspondence, v.components);
}
BGNMESH_ROW(MeshCell, v.face, v.chart, v.lineage, v.map, v.facets, v.components)
BGNMESH_ROW(UVTriangleMap, v.sites)
BGNMESH_ROW(PlaneTriangleMap, v.sites, v.chart)
BGNMESH_ROW(PoleFanMap, v.quotient, v.pole, v.angular_limits, v.edge_nodes, v.edge_uv,
            v.cap_cut_chain, v.xi, v.rho, v.generation)
BGNMESH_ROW(MeshCellMap, v.cell, v.map)
template <class S> void transfer(S &s, MeshCellFacet &v) {
  s.fields(v.cell, v.facet, v.interpolant);
  s.zero32();
}
BGNMESH_ROW(MeshFacet, v.face, v.nodes, v.corners, v.appearance)
template <class S> void transfer(S &s, MeshCorner &v) {
  s.fields(v.face, v.site, v.carrier, v.orientation);
  s.zero32();
  s.fields(v.uv);
}
BGNMESH_ROW(MeshQuotient, v.face, v.carrier, v.kind, v.source_vertices, v.uses, v.sites, v.node,
            v.cells, v.pole)
template <class S> void transfer(S &s, MeshClosure &v) {
  s.fields(v.face, v.loop, v.vertex, v.incoming, v.outgoing, v.endpoints, v.kind);
  s.zero32();
  s.optional_reference(v.quotient);
  s.fields(v.components);
}
template <class S> void transfer(S &s, MeshComponent &v) {
  s.fields(v.owner, v.kind, v.role, v.value);
  s.optional_reference(v.profile);
}
template <class S> void transfer(S &s, MeshQuality &v) {
  s.fields(v.positional, v.node_source, v.policy_revision);
  s.zero32();
}
BGNMESH_ROW(MeshInstance, v.path, v.product, v.definition, v.matrix, v.explosion, v.appearance)
BGNMESH_ROW(MeshAppearance, v.source, v.rgb, v.roughness, v.label_product)
#undef BGNMESH_ROW
} // namespace bgmesh::mesh_detail
#endif
