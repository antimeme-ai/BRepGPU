#pragma once
// Isolated foreign source conversion. No source 3D geometry is repaired here.
#include <TopoDS_Shape.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Wire.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Vertex.hxx>
#include <NCollection_DataMap.hxx>
#include <TopTools_ShapeMapHasher.hxx>
#include <Geom2d_Curve.hxx>
#include <Geom_Surface.hxx>
#include <array>
#include <cstdint>
#include <vector>

namespace brepgpu {
using SourceID=std::uint64_t;
struct SourceUVUse {
  SourceID id=0,edge_id=0,start_id=0,end_id=0;
  TopoDS_Edge edge; // original geometric orientation composed with source use
  TopoDS_Vertex start,end;
};
// A vertex_loop bound has a pole vertex and no uses. Its wire is OCCT's
// translation of it: one degenerated edge at the pole, natural bounds for a
// sole bound on a closed surface, or null where OCCT ignores the bound.
struct SourceUVLoop { SourceID id=0; TopoDS_Wire wire; std::vector<SourceUVUse> uses; SourceID pole_id=0; TopoDS_Vertex pole; };
// unexplained: wires OCCT made for a refused face that no source loop explains
// (a vertex-loop translation that differs from OCCT's own rule). They are
// owned by the face so the translated inventory stays whole; nothing meshes them.
struct SourceUVFace { SourceID id=0,carrier_id=0; TopoDS_Face face; std::vector<SourceUVLoop> loops; std::vector<TopoDS_Wire> unexplained; };
struct SourceUVOptions {
  double projection_precision_mm=1.e-7,source_residual_mm=.001,tolerance_mm=.005;
  bool prepare_query=false;
};
struct SourceUVStats {
  std::uint64_t projected_edges=0,seam_pairs=0,original_uses=0,query_column_edges=0,
    query_joint_endpoints=0,query_connectors=0;
  double max_residual_mm=0,max_query_residual_mm=0;
  // Cumulative natural-domain-metric UV admission. Not an extrapolation theorem.
  double max_query_uv_metric_mm=0,projection_seconds=0,query_seconds=0;
};
struct SourceUVConnector {
  SourceID face_id=0,loop_id=0,vertex_id=0,incoming_id=0,outgoing_id=0;
  double u0=0,v0=0,u1=0,v1=0;
  TopoDS_Edge edge;
};
struct SourceUVChartApproximation {
  SourceID face_id=0,coedge_id=0;
  int axis=0;
  double side=0,max_guide_coefficient_delta=0,uv_metric_mm=0;
  // Guide residual may evaluate foreign extension outside the original domain.
  double max_guide_extension_residual_mm=0,max_selected_residual_mm=0;
};
struct PreparedSourceUV {
  TopoDS_Shape raw_shape,query_shape;
  NCollection_DataMap<TopoDS_Shape,TopoDS_Shape,TopTools_ShapeMapHasher> copies,query_copies;
  SourceUVStats stats;
  std::vector<SourceUVConnector> connectors;
  std::vector<SourceUVChartApproximation> chart_approximations;
  TopoDS_Shape mapped(const TopoDS_Shape& original) const;
  TopoDS_Shape query_mapped(const TopoDS_Shape& original) const;
};
PreparedSourceUV prepare_source_uv(const TopoDS_Shape&,const std::vector<SourceUVFace>&,
                                  const SourceUVOptions& = {});
// Edge-parameter t to pcurve-parameter s correspondence for one use whose edge
// is not SameParameter: kind 4, measured piecewise-linear rows (t, s, sampled
// residual mm). Rows run between the EDGE_CURVE's parameter ends, from ta to tb
// as the adapter orders the edge (by its original same_sense), not by the use.
struct SourceParameterMap {
  std::uint32_t kind=0;
  std::vector<std::array<double,3>> rows;
  double max_residual_mm=0;
  std::uint64_t probes=0;
};
SourceParameterMap measure_source_parameter_map(const TopoDS_Edge& use_edge,const TopoDS_Face& face,
  double ta,double tb,const SourceUVOptions& = {});
// These direct guards are also exercised by authored altered-native fixtures.
void require_source_uv_uses(const SourceUVFace&,const std::vector<SourceUVConnector>& = {});
void require_source_uv_period_translation(const Handle(Geom2d_Curve)& before,
  const Handle(Geom2d_Curve)& after,const Handle(Geom_Surface)& surface,double first,double last);
}
