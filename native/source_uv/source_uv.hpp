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
#include <cstdint>
#include <vector>

namespace brepgpu {
using SourceID=std::uint64_t;
struct SourceUVUse {
  SourceID id=0,edge_id=0,start_id=0,end_id=0;
  TopoDS_Edge edge; // original geometric orientation composed with source use
  TopoDS_Vertex start,end;
};
struct SourceUVLoop { SourceID id=0; TopoDS_Wire wire; std::vector<SourceUVUse> uses; };
struct SourceUVFace { SourceID id=0,carrier_id=0; TopoDS_Face face; std::vector<SourceUVLoop> loops; };
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
// These direct guards are also exercised by authored altered-native fixtures.
void require_source_uv_uses(const SourceUVFace&,const std::vector<SourceUVConnector>& = {});
void require_source_uv_period_translation(const Handle(Geom2d_Curve)& before,
  const Handle(Geom2d_Curve)& after,const Handle(Geom_Surface)& surface,double first,double last);
}
