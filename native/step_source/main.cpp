// Owned source extraction into BGSRC001. OCCT translates STEP and reconstructs
// UV curves; product sampling, triangulation and rendering are not performed.
#include "source_uv.hpp"
#include "step_units.hpp"
#include "step_styles.hpp"
#include "step_vertex_loops.hpp"
#include "step_placements.hpp"
#include "face_regions.hpp"
#include "shape_extent.hpp"
#include "face_contact.hpp"
#include <BinTools.hxx>
#include <GeomAPI_ProjectPointOnSurf.hxx>
#include <GCPnts_AbscissaPoint.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <BRepAlgoAPI_Section.hxx>
#include <filesystem>
#include <STEPCAFControl_Reader.hxx>
#include <STEPControl_Reader.hxx>
#include <STEPConstruct_Tool.hxx>
#include <STEPConstruct_Styles.hxx>
#include <STEPConstruct_RenderingProperties.hxx>
#include <XSControl_WorkSession.hxx>
#include <XSControl_TransferReader.hxx>
#include <Transfer_TransientProcess.hxx>
#include <TransferBRep.hxx>
#include <StepData_StepModel.hxx>
#include <StepBasic_ProductDefinition.hxx>
#include <StepBasic_ProductDefinitionFormation.hxx>
#include <StepBasic_Product.hxx>
#include <StepBasic_UncertaintyMeasureWithUnit.hxx>
#include <StepRepr_NextAssemblyUsageOccurrence.hxx>
#include <StepShape_ManifoldSolidBrep.hxx>
#include <StepShape_BrepWithVoids.hxx>
#include <StepShape_ClosedShell.hxx>
#include <StepShape_OrientedClosedShell.hxx>
#include <StepGeom_ConicalSurface.hxx>
#include <StepGeom_GeometricRepresentationContextAndGlobalUnitAssignedContext.hxx>
#include <StepGeom_GeomRepContextAndGlobUnitAssCtxAndGlobUncertaintyAssCtx.hxx>
#include <StepRepr_GlobalUnitAssignedContext.hxx>
#include <StepRepr_Representation.hxx>
#include <StepRepr_RepresentationContext.hxx>
#include <StepRepr_ReprItemAndMeasureWithUnit.hxx>
#include <StepBasic_ConversionBasedUnit.hxx>
#include <StepBasic_ConversionBasedUnitAndPlaneAngleUnit.hxx>
#include <StepBasic_MeasureWithUnit.hxx>
#include <StepBasic_PlaneAngleMeasureWithUnit.hxx>
#include <StepBasic_PlaneAngleUnit.hxx>
#include <StepBasic_SiUnit.hxx>
#include <StepBasic_SiUnitAndPlaneAngleUnit.hxx>
#include <STEPConstruct_UnitContext.hxx>
#include <DESTEP_Parameters.hxx>
#include <Interface_Graph.hxx>
#include <Interface_EntityIterator.hxx>
#include <StepGeom_RectangularTrimmedSurface.hxx>
#include <StepGeom_Pcurve.hxx>
#include <StepGeom_Conic.hxx>
#include <StepGeom_TrimmedCurve.hxx>
#include <StepGeom_Plane.hxx>
#include <StepGeom_BSplineSurface.hxx>
#include <StepGeom_BSplineCurveWithKnots.hxx>
#include <StepGeom_BSplineCurveWithKnotsAndRationalBSplineCurve.hxx>
#include <StepGeom_SurfaceCurve.hxx>
#include <StepGeom_Pcurve.hxx>
#include <StepGeom_PcurveOrSurface.hxx>
#include <StepRepr_DefinitionalRepresentation.hxx>
#include <StepGeom_BSplineSurfaceWithKnots.hxx>
#include <StepGeom_BSplineSurfaceWithKnotsAndRationalBSplineSurface.hxx>
#include <Geom2dAdaptor_Curve.hxx>
#include <StepToGeom.hxx>
#include <StepData_Factors.hxx>
#include <StepGeom_ElementarySurface.hxx>
#include <StepGeom_SurfaceOfRevolution.hxx>
#include <StepRepr_MappedItem.hxx>
#include <StepRepr_RepresentationMap.hxx>
#include <StepRepr_RepresentationRelationship.hxx>
#include <StepShape_ShapeDefinitionRepresentation.hxx>
#include <StepShape_ShapeRepresentation.hxx>
#include <StepShape_ConnectedFaceSet.hxx>
#include <StepShape_AdvancedFace.hxx>
#include <StepShape_FaceBound.hxx>
#include <StepShape_FaceOuterBound.hxx>
#include <StepShape_EdgeLoop.hxx>
#include <StepShape_VertexLoop.hxx>
#include <StepShape_OrientedEdge.hxx>
#include <StepShape_EdgeCurve.hxx>
#include <StepShape_VertexPoint.hxx>
#include <StepGeom_Surface.hxx>
#include <StepGeom_Curve.hxx>
#include <StepGeom_CartesianPoint.hxx>
#include <StepVisual_StyledItem.hxx>
#include <StepRepr_ProductDefinitionShape.hxx>
#include <StepShape_ContextDependentShapeRepresentation.hxx>
#include <StepRepr_ShapeRepresentationRelationship.hxx>
#include <StepVisual_OverRidingStyledItem.hxx>
#include <StepVisual_ContextDependentOverRidingStyledItem.hxx>
#include <StepVisual_Colour.hxx>
#include <TCollection_HAsciiString.hxx>
#include <TDocStd_Document.hxx>
#include <XCAFApp_Application.hxx>
#include <XCAFDoc_DocumentTool.hxx>
#include <XCAFDoc_ShapeTool.hxx>
#include <XCAFDoc_MaterialTool.hxx>
#include <XCAFDoc.hxx>
#include <TDataStd_TreeNode.hxx>
#include <TDF_Label.hxx>
#include <BRep_Tool.hxx>
#include <BRepTools.hxx>
#include <BRepTools_WireExplorer.hxx>
#include <BRepTopAdaptor_FClass2d.hxx>
#include <BRepGProp_Face.hxx>
#include <BRepGProp_Vinert.hxx>
#include <BRep_Builder.hxx>
#include <TopoDS_Compound.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <BndLib_AddSurface.hxx>
#include <BRepCheck.hxx>
#include <BRepCheck_Result.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopTools_ShapeMapHasher.hxx>
#include <NCollection_DataMap.hxx>
#include <ShapeAnalysis_Surface.hxx>
#include <Precision.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopoDS_Wire.hxx>
#include <TopExp_Explorer.hxx>
#include <TopExp.hxx>
#include <TopLoc_Location.hxx>
#include <GeomAdaptor_Surface.hxx>
#include <GeomAdaptor_Curve.hxx>
#include <Geom2dAdaptor_Curve.hxx>
#include <Geom_Surface.hxx>
#include <Geom_Curve.hxx>
#include <Geom_BSplineSurface.hxx>
#include <Geom_ToroidalSurface.hxx>
#include <Geom_Plane.hxx>
#include <Geom_SurfaceOfRevolution.hxx>
#include <Geom_SphericalSurface.hxx>
#include <Geom_RectangularTrimmedSurface.hxx>
#include <Geom_BezierSurface.hxx>
#include <Geom_BSplineCurve.hxx>
#include <Geom_BezierCurve.hxx>
#include <Geom2d_Curve.hxx>
#include <Geom2d_BSplineCurve.hxx>
#include <Geom2d_BezierCurve.hxx>
#include <GeomConvert_BSplineSurfaceToBezierSurface.hxx>
#include <GeomConvert_BSplineCurveToBezierCurve.hxx>
#include <Geom2dConvert_BSplineCurveToBezierCurve.hxx>
#include <gp_Ax3.hxx>
#include <gp_Ax2.hxx>
#include <gp_Ax22d.hxx>
#include <gp_Pln.hxx>
#include <gp_Cylinder.hxx>
#include <gp_Cone.hxx>
#include <gp_Sphere.hxx>
#include <gp_Torus.hxx>
#include <gp_Lin.hxx>
#include <gp_Circ.hxx>
#include <gp_Elips.hxx>
#include <gp_Lin2d.hxx>
#include <gp_Circ2d.hxx>
#include <gp_Elips2d.hxx>
#include <gp_Vec.hxx>
#include <Quantity_Color.hxx>
#include <Quantity_ColorRGBA.hxx>
#include <Standard_Failure.hxx>
#include <array>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using U=std::uint64_t;
using namespace brepgpu::face_regions;
// Review round 8, finding 3: a face's smaller principal radius of curvature,
// bounded below over its UV box (the blind-band target is a quarter of it):
// plane infinite; cylinder and sphere their radius; torus the smaller of its
// minor radius and |major - minor|; cone its smallest radius / cos(semi-angle)
// over the box's v-range (0 if the apex lies in it); any other carrier 0 (the
// target falls back to ten resolutions).
double smaller_principal_radius(const Handle(Geom_Surface)& s,double,double,double v0,double v1){GeomAdaptor_Surface a(s);
  switch(a.GetType()){case GeomAbs_Plane:return INFINITY;case GeomAbs_Cylinder:return a.Cylinder().Radius();case GeomAbs_Sphere:return a.Sphere().Radius();
    case GeomAbs_Torus:{const double R=a.Torus().MajorRadius(),r=a.Torus().MinorRadius();return std::abs(R-r)>0?std::min(r,std::abs(R-r)):0.;}
    case GeomAbs_Cone:{const auto c=a.Cone();const double r0=c.RefRadius()+v0*std::sin(c.SemiAngle()),r1=c.RefRadius()+v1*std::sin(c.SemiAngle());if(r0*r1<=0)return 0.;return std::min(std::abs(r0),std::abs(r1))/std::cos(c.SemiAngle());}
    default:return 0.;}}
using brepgpu::face_contact::region_meets;using brepgpu::face_contact::Contact;using brepgpu::face_contact::Fold;using brepgpu::face_contact::tangent_fold;
using D=std::vector<double>;
using IDs=std::vector<U>;
struct Bytes {
  std::vector<unsigned char> v;
  void u32(std::uint32_t x){for(int i=0;i<4;++i)v.push_back((x>>(8*i))&255);}
  void i32(std::int32_t x){u32(static_cast<std::uint32_t>(x));}
  void u64(U x){for(int i=0;i<8;++i)v.push_back((x>>(8*i))&255);}
  void f64(double x){if(!std::isfinite(x))throw std::runtime_error("nonfinite source scalar");U b;std::memcpy(&b,&x,8);u64(b);}
  void raw(const std::string& s){v.insert(v.end(),s.begin(),s.end());}
  void text(const std::string& s){if(s.size()>UINT32_MAX)throw std::runtime_error("oversized source string");u32(s.size());raw(s);}
  void doubles(const D& a){for(double x:a)f64(x);}
  void vec(const D& a){u64(a.size());doubles(a);}
  void ids(const IDs& a){u64(a.size());for(U x:a)u64(x);}
  void ints(const std::vector<unsigned>& a){u64(a.size());for(auto x:a)u32(x);}
  void row(const Bytes& b){u64(b.v.size());v.insert(v.end(),b.v.begin(),b.v.end());}
};

// SHA-256 on original input bytes. This is the published fixed SHA-256 round
// definition, implemented here without a dependency or shell subprocess.
std::array<unsigned char,32> sha256(const std::vector<unsigned char>& input){
  static const std::uint32_t k[64]={
    0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
    0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
    0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
    0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
    0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
    0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
    0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
    0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2};
  std::uint32_t h[8]={0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};
  auto data=input;U bits=U(data.size())*8;data.push_back(128);
  while(data.size()%64!=56)data.push_back(0);
  for(int i=7;i>=0;--i)data.push_back((bits>>(8*i))&255);
  auto rr=[](std::uint32_t x,int n){return(x>>n)|(x<<(32-n));};
  for(std::size_t off=0;off<data.size();off+=64){
    std::uint32_t w[64];for(int i=0;i<16;++i)w[i]=(std::uint32_t(data[off+4*i])<<24)|(std::uint32_t(data[off+4*i+1])<<16)|(std::uint32_t(data[off+4*i+2])<<8)|data[off+4*i+3];
    for(int i=16;i<64;++i){auto a=w[i-15],b=w[i-2];w[i]=w[i-16]+(rr(a,7)^rr(a,18)^(a>>3))+w[i-7]+(rr(b,17)^rr(b,19)^(b>>10));}
    auto a=h[0],b=h[1],c=h[2],d=h[3],e=h[4],f=h[5],g=h[6],z=h[7];
    for(int i=0;i<64;++i){auto t=z+(rr(e,6)^rr(e,11)^rr(e,25))+((e&f)^((~e)&g))+k[i]+w[i];auto t2=(rr(a,2)^rr(a,13)^rr(a,22))+((a&b)^(a&c)^(b&c));z=g;g=f;f=e;e=d+t;d=c;c=b;b=a;a=t+t2;}
    h[0]+=a;h[1]+=b;h[2]+=c;h[3]+=d;h[4]+=e;h[5]+=f;h[6]+=g;h[7]+=z;
  }
  std::array<unsigned char,32> out{};for(int i=0;i<8;++i)for(int j=0;j<4;++j)out[4*i+j]=(h[i]>>(24-8*j))&255;return out;
}

std::string str(const Handle(TCollection_HAsciiString)& s){return s.IsNull()?"":s->ToCString();}
D frame(const gp_Ax3& a){auto o=a.Location();auto x=a.XDirection();auto y=a.YDirection();auto z=a.Direction();return{o.X(),o.Y(),o.Z(),x.X(),x.Y(),x.Z(),y.X(),y.Y(),y.Z(),z.X(),z.Y(),z.Z()};}
D frame(const gp_Ax2& a){return frame(gp_Ax3(a));}
D frame(const gp_Ax22d& a){auto o=a.Location();auto x=a.XDirection();auto y=a.YDirection();return{o.X(),o.Y(),0,x.X(),x.Y(),0,y.X(),y.Y(),0,0,0,0};}
D transform(const gp_Trsf& t){D v;for(int i=1;i<=3;++i)for(int j=1;j<=4;++j)v.push_back(t.Value(i,j));return v;}

struct CurveData {
  U id=0;unsigned dimension=0,kind=0,degree=0;
  D fr=D(12,0),radii={0,0},range={0,0},knots,controls;
  std::vector<unsigned> mults;std::vector<Bytes> spans;
  Bytes bytes()const{Bytes w;w.u64(id);w.u32(dimension);w.u32(kind);w.doubles(fr);w.doubles(radii);w.doubles(range);w.u32(degree);w.u32(0);w.vec(knots);w.ints(mults);w.vec(controls);w.u64(spans.size());for(auto& s:spans)w.row(s);return w;}
};

template<class CurveHandle> D controls3(const CurveHandle& c){D v;for(int i=1;i<=c->NbPoles();++i){auto p=c->Pole(i);double w=c->Weight(i);v.insert(v.end(),{p.X()*w,p.Y()*w,p.Z()*w,w});}return v;}
template<class CurveHandle> D controls2(const CurveHandle& c){D v;for(int i=1;i<=c->NbPoles();++i){auto p=c->Pole(i);double w=c->Weight(i);v.insert(v.end(),{p.X()*w,p.Y()*w,w});}return v;}
template<class CurveHandle> void knots(const CurveHandle& c,CurveData& d){for(int i=1;i<=c->NbKnots();++i){d.knots.push_back(c->Knot(i));d.mults.push_back(c->Multiplicity(i));}}

CurveData curve3(U id,const Handle(Geom_Curve)& c,double a,double b){
  CurveData d;d.id=id;d.dimension=3;d.range={a,b};GeomAdaptor_Curve ac(c);
  switch(ac.GetType()){
    case GeomAbs_Line:{d.kind=1;auto l=ac.Line();auto p=l.Location();auto v=l.Direction();d.fr={p.X(),p.Y(),p.Z(),v.X(),v.Y(),v.Z(),0,0,0,0,0,0};break;}
    case GeomAbs_Circle:{d.kind=2;auto q=ac.Circle();d.fr=frame(q.Position());d.radii={q.Radius(),q.Radius()};break;}
    case GeomAbs_Ellipse:{d.kind=3;auto q=ac.Ellipse();d.fr=frame(q.Position());d.radii={q.MajorRadius(),q.MinorRadius()};break;}
    case GeomAbs_BezierCurve:{d.kind=4;d.range={0,1};auto q=ac.Bezier();d.degree=q->Degree();d.controls=controls3(q);break;}
    case GeomAbs_BSplineCurve:{d.kind=5;auto q=ac.BSpline();d.degree=q->Degree();knots(q,d);d.controls=controls3(q);GeomConvert_BSplineCurveToBezierCurve cv(q);NCollection_Array1<double> ks(1,cv.NbArcs()+1);cv.Knots(ks);for(int i=1;i<=cv.NbArcs();++i){Bytes s;s.f64(ks(i));s.f64(ks(i+1));s.u32(d.degree);s.u32(0);s.vec(controls3(cv.Arc(i)));d.spans.push_back(s);}break;}
    default:throw std::runtime_error("unsupported spatial curve at source "+std::to_string(id));
  }return d;
}

U unperiodized_uv_curves=0;
CurveData curve2(U id,const Handle(Geom2d_Curve)& c,double a,double b){
  CurveData d;d.id=id;d.dimension=2;d.range={a,b};Geom2dAdaptor_Curve ac(c);
  switch(ac.GetType()){
    case GeomAbs_Line:{d.kind=1;auto l=ac.Line();auto p=l.Location();auto v=l.Direction();d.fr={p.X(),p.Y(),0,v.X(),v.Y(),0,0,0,0,0,0,0};break;}
    case GeomAbs_Circle:{d.kind=2;auto q=ac.Circle();d.fr=frame(q.Axis());d.radii={q.Radius(),q.Radius()};break;}
    case GeomAbs_Ellipse:{d.kind=3;auto q=ac.Ellipse();d.fr=frame(q.Axis());d.radii={q.MajorRadius(),q.MinorRadius()};break;}
    case GeomAbs_BezierCurve:{d.kind=4;d.range={0,1};auto q=ac.Bezier();d.degree=q->Degree();d.controls=controls2(q);break;}
    // Reached only by a UV curve that is OCCT's or our own construction: a
    // periodic file pcurve is replaced by its raw net before this (coedge
    // writer, review 6). On a plane OCCT projects a periodic edge curve
    // (.37.30's closed B-spline) into a periodic one; the table has no periodic
    // form, so it is written as the same curve unperiodized, an exact change of
    // representation.
    case GeomAbs_BSplineCurve:{d.kind=5;auto q=ac.BSpline();if(q->IsPeriodic()){q=Handle(Geom2d_BSplineCurve)::DownCast(q->Copy());q->SetNotPeriodic();++unperiodized_uv_curves;}d.degree=q->Degree();knots(q,d);d.controls=controls2(q);Geom2dConvert_BSplineCurveToBezierCurve cv(q);NCollection_Array1<double> ks(1,cv.NbArcs()+1);cv.Knots(ks);for(int i=1;i<=cv.NbArcs();++i){Bytes s;s.f64(ks(i));s.f64(ks(i+1));s.u32(d.degree);s.u32(0);s.vec(controls2(cv.Arc(i)));d.spans.push_back(s);}break;}
    default:throw std::runtime_error("unsupported recovered UV curve at coedge "+std::to_string(id));
  }return d;
}

template<class S> D surface_controls(const S& s){D v;for(int i=1;i<=s->NbUPoles();++i)for(int j=1;j<=s->NbVPoles();++j){auto p=s->Pole(i,j);double w=s->Weight(i,j);v.insert(v.end(),{p.X()*w,p.Y()*w,p.Z()*w,w});}return v;}

Bytes surface(U id,const Handle(Geom_Surface)& s,const D& bounds){
  GeomAdaptor_Surface a(s);unsigned kind=0,p=0,q=0,nu=0,nv=0;D fr(12,0),rs={0,0},ku,kv,ctrl;std::vector<unsigned> mu,mv;std::vector<Bytes> spans;
  switch(a.GetType()){
    case GeomAbs_Plane:kind=1;fr=frame(a.Plane().Position());break;
    case GeomAbs_Cylinder:kind=2;fr=frame(a.Cylinder().Position());rs[0]=a.Cylinder().Radius();break;
    case GeomAbs_Cone:kind=3;fr=frame(a.Cone().Position());rs={a.Cone().RefRadius(),a.Cone().SemiAngle()};break;
    case GeomAbs_Sphere:kind=4;fr=frame(a.Sphere().Position());rs[0]=a.Sphere().Radius();break;
    case GeomAbs_Torus:kind=5;fr=frame(a.Torus().Position());rs={a.Torus().MajorRadius(),a.Torus().MinorRadius()};break;
    case GeomAbs_BSplineSurface:{
      kind=6;auto b=a.BSpline();p=b->UDegree();q=b->VDegree();nu=b->NbUPoles();nv=b->NbVPoles();ctrl=surface_controls(b);
      for(int i=1;i<=b->NbUKnots();++i){ku.push_back(b->UKnot(i));mu.push_back(b->UMultiplicity(i));}
      for(int i=1;i<=b->NbVKnots();++i){kv.push_back(b->VKnot(i));mv.push_back(b->VMultiplicity(i));}
      GeomConvert_BSplineSurfaceToBezierSurface cv(b);NCollection_Array1<double> uk(1,cv.NbUPatches()+1),vk(1,cv.NbVPatches()+1);cv.UKnots(uk);cv.VKnots(vk);
      for(int i=1;i<=cv.NbUPatches();++i)for(int j=1;j<=cv.NbVPatches();++j){Bytes z;z.doubles({uk(i),uk(i+1),vk(j),vk(j+1)});z.u32(p);z.u32(q);z.vec(surface_controls(cv.Patch(i,j)));spans.push_back(z);}break;
    }
    default:throw std::runtime_error("unsupported surface at source "+std::to_string(id));
  }
  Bytes w;w.u64(id);w.u32(kind);w.u32(0);w.doubles(fr);w.doubles(rs);w.doubles(bounds);w.u32(p);w.u32(q);w.u32(nu);w.u32(nv);w.vec(ku);w.ints(mu);w.vec(kv);w.ints(mv);w.vec(ctrl);w.u64(spans.size());for(auto& z:spans)w.row(z);return w;
}

// B2 .37.28, edge curves: StepToGeom::MakeBSplineCurve makes a closed raw
// curve periodic (StepToGeom.cxx:921-926, G1 G5), and the curve table has no
// periodic form. The raw net, from the entity with knots under its surface or
// trimmed curve: degree, poles in the context unit, weights, knots and
// multiplicities as written. A raw knot vector OCCT would itself edit (G1
// G2-G4: repeated, over-full or periodic) refuses by name.
Handle(Geom_BSplineCurve) raw_curve_net(Handle(StepGeom_Curve) raw,double length_factor,const std::string& where){
  if(auto s=Handle(StepGeom_SurfaceCurve)::DownCast(raw);!s.IsNull())raw=s->Curve3d();
  if(auto s=Handle(StepGeom_TrimmedCurve)::DownCast(raw);!s.IsNull())raw=s->BasisCurve();
  auto rational=Handle(StepGeom_BSplineCurveWithKnotsAndRationalBSplineCurve)::DownCast(raw);auto plain=Handle(StepGeom_BSplineCurveWithKnots)::DownCast(raw);
  if(rational.IsNull()&&plain.IsNull())throw std::runtime_error(where+": OCCT made it periodic and its raw entity is not a B-spline curve with knots");
  Handle(StepGeom_BSplineCurve) base=rational.IsNull()?Handle(StepGeom_BSplineCurve)(plain):Handle(StepGeom_BSplineCurve)(rational);
  const int degree=base->Degree(),n=base->NbControlPointsList(),nk=rational.IsNull()?plain->NbKnots():rational->NbKnots();
  if(degree<1||n<degree+1||nk<2||nk!=(rational.IsNull()?plain->NbKnotMultiplicities():rational->NbKnotMultiplicities()))throw std::runtime_error(where+": raw B-spline curve degree, poles or knots are inconsistent");
  NCollection_Array1<gp_Pnt> poles(1,n);NCollection_Array1<double> knots(1,nk),weights(1,n);NCollection_Array1<int> mults(1,nk);int total=0;
  for(int i=1;i<=n;++i){auto p=base->ControlPointsListValue(i);if(p.IsNull()||p->NbCoordinates()!=3)throw std::runtime_error(where+": raw B-spline curve pole is not 3D");poles(i)=gp_Pnt(p->CoordinatesValue(1)*length_factor,p->CoordinatesValue(2)*length_factor,p->CoordinatesValue(3)*length_factor);weights(i)=rational.IsNull()?1.:rational->WeightsDataValue(i);if(!(weights(i)>0))throw std::runtime_error(where+": raw B-spline curve weight is not positive");}
  for(int i=1;i<=nk;++i){knots(i)=rational.IsNull()?plain->KnotsValue(i):rational->KnotsValue(i);mults(i)=rational.IsNull()?plain->KnotMultiplicitiesValue(i):rational->KnotMultiplicitiesValue(i);total+=mults(i);
    if(mults(i)<1||mults(i)>degree+1||(i>1&&!(knots(i)>knots(i-1))))throw std::runtime_error(where+": raw knot vector is one OCCT edits (G1 G2-G4); refused");}
  if(total!=n+degree+1)throw std::runtime_error(where+": raw knot vector is one OCCT edits (G1 G2-G4); refused");
  if(rational.IsNull())return new Geom_BSplineCurve(poles,knots,mults,degree,false);
  return new Geom_BSplineCurve(poles,weights,knots,mults,degree,false);
}

// B2 .37.36 review 6: a file pcurve is a raw entity too. StepToGeom makes a
// closed 2D B-spline periodic as it does a 3D one (StepToGeom.cxx:921-926).
// The raw 2D net, as raw_curve_net builds the 3D one; parameter coordinates
// are written as the file states them (mm-only admission makes lengths 1:1).
Handle(Geom2d_BSplineCurve) raw_pcurve_net(Handle(StepGeom_Curve) raw,const std::string& where){
  if(auto s=Handle(StepGeom_TrimmedCurve)::DownCast(raw);!s.IsNull())raw=s->BasisCurve();
  auto rational=Handle(StepGeom_BSplineCurveWithKnotsAndRationalBSplineCurve)::DownCast(raw);auto plain=Handle(StepGeom_BSplineCurveWithKnots)::DownCast(raw);
  if(rational.IsNull()&&plain.IsNull())return {};
  Handle(StepGeom_BSplineCurve) base=rational.IsNull()?Handle(StepGeom_BSplineCurve)(plain):Handle(StepGeom_BSplineCurve)(rational);
  const int degree=base->Degree(),n=base->NbControlPointsList(),nk=rational.IsNull()?plain->NbKnots():rational->NbKnots();
  if(degree<1||n<degree+1||nk<2||nk!=(rational.IsNull()?plain->NbKnotMultiplicities():rational->NbKnotMultiplicities()))throw std::runtime_error(where+": raw B-spline pcurve degree, poles or knots are inconsistent");
  NCollection_Array1<gp_Pnt2d> poles(1,n);NCollection_Array1<double> knots(1,nk),weights(1,n);NCollection_Array1<int> mults(1,nk);int total=0;
  for(int i=1;i<=n;++i){auto p=base->ControlPointsListValue(i);if(p.IsNull()||p->NbCoordinates()!=2)throw std::runtime_error(where+": raw B-spline pcurve pole is not 2D");poles(i)=gp_Pnt2d(p->CoordinatesValue(1),p->CoordinatesValue(2));weights(i)=rational.IsNull()?1.:rational->WeightsDataValue(i);if(!(weights(i)>0))throw std::runtime_error(where+": raw B-spline pcurve weight is not positive");}
  for(int i=1;i<=nk;++i){knots(i)=rational.IsNull()?plain->KnotsValue(i):rational->KnotsValue(i);mults(i)=rational.IsNull()?plain->KnotMultiplicitiesValue(i):rational->KnotMultiplicitiesValue(i);total+=mults(i);
    if(mults(i)<1||mults(i)>degree+1||(i>1&&!(knots(i)>knots(i-1))))throw std::runtime_error(where+": raw pcurve knot vector is one OCCT edits (G1 G2-G4); refused");}
  if(total!=n+degree+1)throw std::runtime_error(where+": raw pcurve knot vector is one OCCT edits (G1 G2-G4); refused");
  if(rational.IsNull())return new Geom2d_BSplineCurve(poles,knots,mults,degree,false);
  return new Geom2d_BSplineCurve(poles,weights,knots,mults,degree,false);
}
// B2 .37.36 review 8: StepToGeom::MakeBSplineSurface merges knots within one
// Epsilon (StepToGeom.cxx:1015-1083) and clamps over-full end multiplicities
// (G1 G2, G3). The written net must keep the raw entity's knots and
// multiplicities exactly; otherwise the carrier refuses by name. Surfaces
// without explicit knots (uniform, quasi-uniform, Bezier; G1 G7) are not
// checked here.
void require_raw_surface_knots(const Handle(StepGeom_Surface)& raw,const Handle(Geom_BSplineSurface)& written,const std::string& where){
  Handle(StepGeom_BSplineSurfaceWithKnots) knots=Handle(StepGeom_BSplineSurfaceWithKnots)::DownCast(raw);
  if(auto r=Handle(StepGeom_BSplineSurfaceWithKnotsAndRationalBSplineSurface)::DownCast(raw);knots.IsNull()&&!r.IsNull())knots=r->BSplineSurfaceWithKnots();
  if(knots.IsNull()||written.IsNull())return;
  auto same=[&](int n,int m,auto raw_knot,auto raw_mult,auto knot,auto mult){if(n!=m)return false;for(int i=1;i<=n;++i)if(raw_knot(i)!=knot(i)||raw_mult(i)!=mult(i))return false;return true;};
  const bool u=same(knots->NbUKnots(),written->NbUKnots(),[&](int i){return knots->UKnotsValue(i);},[&](int i){return knots->UMultiplicitiesValue(i);},[&](int i){return written->UKnot(i);},[&](int i){return written->UMultiplicity(i);});
  const bool v=same(knots->NbVKnots(),written->NbVKnots(),[&](int i){return knots->VKnotsValue(i);},[&](int i){return knots->VMultiplicitiesValue(i);},[&](int i){return written->VKnot(i);},[&](int i){return written->VMultiplicity(i);});
  if(!u||!v||knots->NbControlPointsListI()!=written->NbUPoles()||knots->NbControlPointsListJ()!=written->NbVPoles())
    throw std::runtime_error(where+": its raw knot vector is one OCCT edits (repeated, within one Epsilon, or over-full; G1 G2, G3); refused rather than written as OCCT merged it");
}


// Numeric fields of a DIAGS___ row: the validation-records paper 4.1 ValueKind
// in its order (1 length_mm .. 6 boolean; 0 unused), then 7 code and 8 parameter,
// which this table adds. Every row ends with its field list; messages
// carry no magnitudes.
enum FieldKind:std::uint32_t{length_mm=1,angle_rad=2,area_mm2=3,ratio=4,count=5,boolean=6,code=7,parameter=8};
struct Field{std::string name;FieldKind kind;double value;};
void fields(Bytes& w,const std::vector<Field>& values={}){w.u64(values.size());for(const auto& f:values){w.text(f.name);w.u32(f.kind);w.u32(0);w.f64(f.value);}}

struct StyleData{
  U id=0;IDs sources;D rgba={0,0,0,1};bool color=false,density_set=false;std::string material,unit;double density=0;
  Bytes bytes()const{Bytes w;w.u64(id);w.ids(sources);w.doubles(rgba);w.u32(color);w.u32(density_set);w.text(material);w.text(unit);w.f64(density);return w;}
};

int main(int argc,char** argv){
  if(argc<3){std::cerr<<"usage: brepgpu-step-source INPUT.stp OUTPUT.bgsource [--query-brep-dir NEWDIR] [--source-residual-mm VALUE] [--tolerance-mm VALUE]\n";return 2;}
  try{
    brepgpu::SourceUVOptions uv_options;std::string query_directory;
    for(int i=3;i<argc;i+=2){if(i+1==argc)throw std::runtime_error("source option missing value");std::string key=argv[i];
      if(key=="--query-brep-dir")query_directory=argv[i+1];
      else if(key=="--source-residual-mm")uv_options.source_residual_mm=std::stod(argv[i+1]);
      else if(key=="--tolerance-mm")uv_options.tolerance_mm=std::stod(argv[i+1]);
      else throw std::runtime_error("unknown source option "+key);}
    uv_options.prepare_query=!query_directory.empty();if(uv_options.prepare_query&&!std::filesystem::create_directory(query_directory))throw std::runtime_error("query output directory already exists");
    double source_uv_seconds=0,query_uv_seconds=0;U source_domain_corrections=0,source_uv_projected=0,source_uv_seams=0,source_chart_count=0,query_columns=0,query_endpoints=0,query_connectors=0;double source_uv_max_residual=0,source_chart_max_uv_metric=0,query_max_residual=0,query_max_uv_metric=0;
    std::ifstream file(argv[1],std::ios::binary);if(!file)throw std::runtime_error("cannot read source");
    std::vector<unsigned char> input((std::istreambuf_iterator<char>(file)),{});if(input.empty()||input.size()>U(1)<<30)throw std::runtime_error("invalid source file size");auto digest=sha256(input);
    STEPCAFControl_Reader reader;reader.SetNameMode(true);reader.SetColorMode(true);reader.SetMatMode(true);reader.SetPropsMode(true);
    if(reader.ReadFile(argv[1])!=IFSelect_RetDone)throw std::runtime_error("STEP read failed");
    // ReadFile replaces the transfer actor. A pre-read policy assignment is
    // lost; set and verify the actual actor after read, never rely on defaults.
    reader.SetShapeProcessFlags(ShapeProcess::OperationsFlags());
    auto require_no_processing=[&](const char* stage){
      const auto& flags=reader.GetShapeProcessFlags();
      std::cout<<"shape_process_"<<stage<<"\t"<<flags.second<<"\t"<<flags.first.count()<<"\n";
      if(!flags.second||flags.first.any())throw std::runtime_error(std::string("STEP processing policy differs at ")+stage+" supplied="+std::to_string(flags.second)+" operations="+flags.first.to_string());
    };
    require_no_processing("before_transfer");
    Handle(TDocStd_Document) doc;XCAFApp_Application::GetApplication()->NewDocument("BinXCAF",doc);
    namespace units=brepgpu::step_units;namespace vl=brepgpu::step_vertex_loops;namespace placements=brepgpu::step_placements;
    // B2 .37.27: OCCT 8.0.1 recurses without bound on a cycle of mapped-item
    // links; refuse it on the raw graph before the transfer.
    {auto parsed=Handle(StepData_StepModel)::DownCast(reader.Reader().WS()->Model());if(parsed.IsNull())throw std::runtime_error("STEP model missing after read");placements::refuse_unbounded_transfer(parsed,reader.Reader().WS()->Graph());}
    units::prepare_millimetre_transfer(doc);
    if(!reader.Transfer(doc))throw std::runtime_error("STEP/XDE transfer failed");
    require_no_processing("after_transfer");
    auto model=Handle(StepData_StepModel)::DownCast(reader.Reader().WS()->Model());
    units::require_transferred_millimetre(doc,model);std::cout<<"transfer_length_unit_mm\t1\n";
    // OCCT applies read.step.angleunit.mode over the file's units unless it is File.
    if(model.IsNull()||model->InternalParameters.AngleUnit!=DESTEP_Parameters::AngleUnitMode_File)throw std::runtime_error("STEP angle-unit mode overrides the declared plane-angle unit");
    auto tp=reader.Reader().WS()->TransferReader()->TransientProcess();
    auto shape_tool=XCAFDoc_DocumentTool::ShapeTool(doc->Main());
    auto id=[&](const Handle(Standard_Transient)& e)->U{if(e.IsNull())return 0;int n=model->IdentLabel(e);if(n<=0)throw std::runtime_error("missing original STEP entity label");return U(n);};
    // Plane-angle unit of each solid, from the context of the shape
    // representation OCCT transfers it through (one used by a
    // SHAPE_DEFINITION_REPRESENTATION, or reached from one by a representation
    // relationship or mapped item), read the way
    // STEPControl_ActorRead::PrepareUnits and
    // STEPConstruct_UnitContext::ComputeFactors read it. OCCT assumes degrees
    // for a missing plane-angle unit and for a conversion it cannot follow.
    // OCCT does carry a caller's factors into some nested transfers
    // (STEPControl_ActorRead.cxx:1012-1014, kept by PrepareUnits when a context
    // has no assignment, :2340-2357), but not on every path: in the one case
    // measured here (a degree SHAPE_REPRESENTATION related to a unit-less
    // ADVANCED_BREP_SHAPE_REPRESENTATION) it read the cone in radians. This
    // adapter does not model which path applies; it conservatively treats a
    // context without a unit assignment as declaring no unit. An
    // uninterpretable unit always refuses; a missing one refuses only when the
    // solid holds an angle-bearing value (G6 UR-4.4) and is otherwise recorded.
    const Interface_Graph& graph=reader.Reader().WS()->Graph();
    const units::RepresentationGraph representations(graph);
    // Length units (G6 UR-1, UR-2, UR-4): every context that governs a value
    // written here must declare one length unit resolving, from its unit
    // entities, to exactly 1 mm. Those are the contexts of each product's
    // shape representation (it holds the placements OCCT reads in it) and of
    // every representation from a solid's holder up to the transferred one.
    // Admission stays millimetre-only (M8, bead brepgpu-12s.25.6).
    std::map<U,Handle(StepRepr_Representation)> governed;
    for(const auto& rep:units::product_representations(model))governed.emplace(id(rep),rep);
    // The frames and operators of each usage placement are read in its placing
    // relation's two representations.
    for(int i=1;i<=model->NbEntities();++i)if(auto c=Handle(StepShape_ContextDependentShapeRepresentation)::DownCast(model->Value(i));!c.IsNull()&&!c->RepresentationRelation().IsNull())
      for(const auto& rep:{c->RepresentationRelation()->Rep1(),c->RepresentationRelation()->Rep2()})if(!rep.IsNull())governed.emplace(id(rep),rep);
    for(int i=1;i<=model->NbEntities();++i)if(auto s=Handle(StepShape_ManifoldSolidBrep)::DownCast(model->Value(i));!s.IsNull()){
      auto users=graph.Sharings(s);for(users.Start();users.More();users.Next())if(auto rep=Handle(StepShape_ShapeRepresentation)::DownCast(users.Value());!rep.IsNull())for(const auto& r:representations.transfer_path(rep))governed.emplace(id(r),r);}
    std::map<U,double> reported_length_factors;std::vector<Bytes> unit_records;
    for(const auto& g:governed){auto length=units::context_length(model,g.second);units::require_millimetre(model,g.second,length);
      if(reported_length_factors.emplace(length.context,length.mm).second){std::cout<<"source_length_factor\t"<<length.context<<'\t'<<units::shortest(length.mm)<<"\n";
        // UR-2: a unit name stating another factor than the entities resolve.
        if(length.named_mm&&!(std::abs(*length.named_mm-length.mm)<=1e-9*std::max(*length.named_mm,length.mm))){Bytes diag;diag.u32(1);diag.u32(0);diag.text("source_unit_name");diag.ids({length.context,length.unit});diag.text("length unit named '"+length.name+"' resolves from its entities to another factor than its name states; the entities govern");fields(diag,{{"resolved_mm_per_unit",length_mm,length.mm},{"named_mm_per_unit",length_mm,*length.named_mm}});unit_records.push_back(diag);}}}
    // B2 .37.26: a solid-bearing mapped item in a transferred representation
    // places a solid the tables cannot publish (an occurrence outside every
    // usage, or a solid moved within its own definition); refuse it by name
    // rather than drop it or fail later at the leaf binding.
    placements::refuse_unpublished_mapped_items(model,graph,representations);
    struct AngleUnit{bool declared=false;double factor=0;U representation=0;};
    auto context_units=[&](const Handle(StepRepr_Representation)& rep){return units::unit_context(rep->ContextOfItems());};
    auto declared_angle=[&](const Handle(StepRepr_GlobalUnitAssignedContext)& units,const std::string& where)->AngleUnit{
      auto radian=[&](const Handle(StepBasic_NamedUnit)& unit,double& factor){auto si=Handle(StepBasic_SiUnitAndPlaneAngleUnit)::DownCast(unit);if(si.IsNull()||si->Name()!=StepBasic_sunRadian)return false;factor=si->HasPrefix()?STEPConstruct_UnitContext::ConvertSiPrefix(si->Prefix()):1.;return true;};
      std::vector<double> factors;
      for(int i=1;i<=units->NbUnits();++i){auto unit=units->UnitsValue(i);if(unit.IsNull())continue;double factor=0;
        if(unit->IsKind(STANDARD_TYPE(StepBasic_SiUnitAndPlaneAngleUnit))){if(!radian(unit,factor))throw std::runtime_error(where+" has an SI plane-angle unit that is not the radian");factors.push_back(factor);continue;}
        auto converted=Handle(StepBasic_ConversionBasedUnit)::DownCast(unit);Handle(StepBasic_MeasureWithUnit) measure;
        if(!converted.IsNull()){auto value=converted->ConversionFactor();measure=Handle(StepBasic_MeasureWithUnit)::DownCast(value);if(auto item=Handle(StepRepr_ReprItemAndMeasureWithUnit)::DownCast(value);measure.IsNull()&&!item.IsNull())measure=item->GetMeasureWithUnit();}
        bool angular=unit->IsKind(STANDARD_TYPE(StepBasic_ConversionBasedUnitAndPlaneAngleUnit))||unit->IsKind(STANDARD_TYPE(StepBasic_PlaneAngleUnit))||(!measure.IsNull()&&measure->IsKind(STANDARD_TYPE(StepBasic_PlaneAngleMeasureWithUnit)));
        if(!angular)continue;
        if(!unit->IsKind(STANDARD_TYPE(StepBasic_ConversionBasedUnitAndPlaneAngleUnit))||measure.IsNull())throw std::runtime_error(where+" has an uninterpretable plane-angle unit");
        double target=0,value=measure->ValueComponent();if(!radian(measure->UnitComponent().NamedUnit(),target)||!std::isfinite(value)||value<=0)throw std::runtime_error(where+" has an uninterpretable plane-angle unit: its conversion is not a positive measure in radians");
        factors.push_back(target*value);}
      if(factors.size()>1)throw std::runtime_error(where+" declares "+std::to_string(factors.size())+" plane-angle units; at most one is allowed");
      return factors.empty()?AngleUnit{}:AngleUnit{true,factors[0],0};};
    auto resolve=[&](const Handle(StepRepr_Representation)& rep)->AngleUnit{
      AngleUnit unit;if(auto units=context_units(rep);!units.IsNull())unit=declared_angle(units,"representation "+std::to_string(id(rep)));
      unit.representation=id(rep);return unit;};
    // First angle-bearing value reachable from a solid (cone semi-angle, conic
    // trim, pcurve or trimmed bounds on an angular surface, revolution).
    auto angle_bearing=[&](const Handle(Standard_Transient)& root)->std::string{
      auto angular=[](const Handle(StepGeom_Surface)& s){return !Handle(StepGeom_SurfaceOfRevolution)::DownCast(s).IsNull()||(!Handle(StepGeom_ElementarySurface)::DownCast(s).IsNull()&&Handle(StepGeom_Plane)::DownCast(s).IsNull());};
      std::set<const Standard_Transient*> seen{root.get()};std::vector<Handle(Standard_Transient)> stack{root};
      while(!stack.empty()){auto e=stack.back();stack.pop_back();
        if(!Handle(StepGeom_ConicalSurface)::DownCast(e).IsNull())return "conical_surface #"+std::to_string(id(e));
        if(!Handle(StepGeom_SurfaceOfRevolution)::DownCast(e).IsNull())return "surface_of_revolution #"+std::to_string(id(e));
        if(auto c=Handle(StepGeom_TrimmedCurve)::DownCast(e);!c.IsNull()&&!Handle(StepGeom_Conic)::DownCast(c->BasisCurve()).IsNull())return "conic trimmed_curve #"+std::to_string(id(e));
        if(auto c=Handle(StepGeom_Pcurve)::DownCast(e);!c.IsNull()&&angular(c->BasisSurface()))return "pcurve on an angular surface #"+std::to_string(id(e));
        if(auto s=Handle(StepGeom_RectangularTrimmedSurface)::DownCast(e);!s.IsNull()&&angular(s->BasisSurface()))return "rectangular_trimmed_surface #"+std::to_string(id(e));
        auto next=graph.Shareds(e);for(next.Start();next.More();next.Next())if(seen.insert(next.Value().get()).second)stack.push_back(next.Value());}
      return "";};
    std::set<double> reported_angle_factors;
    auto solid_angle_factor=[&](const Handle(StepShape_ManifoldSolidBrep)& solid,std::vector<Bytes>& diagnostics)->double{
      const std::string where="solid "+std::to_string(id(solid));std::set<std::pair<bool,double>> found;AngleUnit chosen;
      for(const auto& rep:representations.holders(solid)){auto unit=resolve(rep);found.insert({unit.declared,unit.factor});chosen=unit;}
      if(found.empty())throw std::runtime_error(where+" is not held by a shape representation that OCCT transfers");
      if(found.size()!=1)throw std::runtime_error(where+" is held by representations with different plane-angle units");
      Bytes diag;diag.u32(0);diag.u32(0);diag.text("source_plane_angle_unit");diag.ids({id(solid),chosen.representation});std::ostringstream message;message<<std::setprecision(17);
      if(chosen.declared){message<<"declared";if(reported_angle_factors.insert(chosen.factor).second)std::cout<<"source_plane_angle_factor\t"<<std::setprecision(17)<<chosen.factor<<"\n";}
      else{auto value=angle_bearing(solid);if(!value.empty())throw std::runtime_error(where+" has no declared plane-angle unit but holds angle-bearing "+value);message<<"absent; solid holds no angle-bearing value";std::cout<<"source_plane_angle_unit_absent\t"<<id(solid)<<"\n";}
      diag.text(message.str());if(chosen.declared)fields(diag,{{"radians_per_unit",angle_rad,chosen.factor}});else fields(diag);diagnostics.push_back(diag);return chosen.factor;};
    // Length unit and uncertainty of each solid, from the contexts of the
    // representations that hold it (UR-1, UR-6, UR-7). Raw coordinates are
    // read with this factor and nothing else; the gate above has made it 1.
    // The governing uncertainty is the B-rep context's own value in mm; with
    // several holders the largest governs (each sender claims its own gaps).
    // solid_uncertainty is the solid's usable value: the only threshold its
    // vertex-loop tests use; without one they refuse as not evaluable.
    double source_uncertainty_mm=0;bool any_usable=false,any_declared=false;std::optional<double> solid_uncertainty;
    auto solid_length=[&](const Handle(StepShape_ManifoldSolidBrep)& solid,std::vector<Bytes>& diagnostics)->double{
      const std::string where="solid "+std::to_string(id(solid));std::set<double> factors;
      const auto holders=representations.holders(solid);
      for(const auto& rep:holders){auto length=units::context_length(model,rep);units::require_millimetre(model,rep,length);factors.insert(length.mm);}
      const auto held=units::held_uncertainty(model,holders);const auto& governing=held.governing;const U holder=held.holder;
      if(factors.empty())throw std::runtime_error(where+" is not held by a shape representation that OCCT transfers");
      if(factors.size()!=1)throw std::runtime_error(where+" is held by representations with different length units");
      Bytes diag;diag.u32(0);diag.u32(0);diag.text("source_uncertainty");IDs ids{id(solid),holder,governing.context};for(const auto& m:governing.measures)ids.push_back(m.id);diag.ids(ids);
      std::ostringstream message;message<<std::setprecision(17);
      // UR-7: a declared value below 1e-15 of the definition's diagonal
      // (including a declared 0) makes no usable claim; absence is separate.
      solid_uncertainty.reset();
      // B2 .37.39: the diagonal of a box that holds the translated solid
      // (native/shape_extent.hpp), not BRepBndLib's, which spans an unbounded
      // carrier where pcurves are missing and misses a cone's apex.
      if(governing.governed){const double diagonal=brepgpu::shape_extent::shape_diagonal(TransferBRep::ShapeResult(tp,solid));
        // An unbounded extent (a carrier OCCT can only sample) has no diagonal
        // to hold the value against: no usable claim, recorded as unbounded.
        const bool bounded=std::isfinite(diagonal),usable=bounded&&units::uncertainty_usable(governing.mm,diagonal);
        message<<"governed by the "<<governing.rule<<(usable?"":bounded?"; below the G6 UR-7 floor relative to the definition diagonal, so it makes no usable claim":"; the definition's extent is not bounded, so the G6 UR-7 floor cannot be met and it makes no usable claim");
        any_declared=true;if(usable){any_usable=true;source_uncertainty_mm=std::max(source_uncertainty_mm,governing.mm);solid_uncertainty=governing.mm;}
        diag.text(message.str());if(bounded)fields(diag,{{"governing_mm",length_mm,governing.mm},{"diagonal_mm",length_mm,diagonal},{"usable",boolean,usable?1.:0.}});else fields(diag,{{"governing_mm",length_mm,governing.mm},{"diagonal_bounded",boolean,0.},{"usable",boolean,0.}});}
      else{message<<"absent; context declares no length uncertainty";diag.text(message.str());fields(diag);}
      diagnostics.push_back(diag);return *factors.begin();};
    // A BREP_WITH_VOIDS adds oriented void shells after the outer shell.
    // StepToTopoDS_Builder::Init(BrepWithVoids) translates each one and
    // reverses it when its orientation is .F.; walk the same shells in order.
    struct RawShell{U ordinal=0,id=0,element=0;bool forward=true;Handle(StepShape_ConnectedFaceSet) faces;};
    auto raw_shells=[&](const Handle(StepShape_ManifoldSolidBrep)& raw,std::vector<Bytes>& diagnostics){std::vector<RawShell> shells;
      if(raw->Outer().IsNull())throw std::runtime_error("solid without outer shell "+std::to_string(id(raw)));
      // ABSR WR3 forbids an oriented outer shell. OCCT translates one through
      // its closed shell without reversing it, so a .T. one is read
      // unambiguously as that closed shell and accepted with a record of the
      // WR3 violation; a .F. one (a reversed outer shell) refuses.
      Handle(StepShape_ConnectedFaceSet) outer=raw->Outer();U outer_element=id(outer);
      if(auto oriented=Handle(StepShape_OrientedClosedShell)::DownCast(outer);!oriented.IsNull()){if(!oriented->Orientation()||oriented->ClosedShellElement().IsNull())throw std::runtime_error("outer shell "+std::to_string(id(oriented))+" of solid "+std::to_string(id(raw))+" is a reversed oriented_closed_shell (ABSR WR3 forbids an oriented outer shell)");outer=oriented->ClosedShellElement();outer_element=id(outer);
        Bytes diag;diag.u32(1);diag.u32(0);diag.text("source_oriented_outer_shell");diag.ids({id(raw),id(oriented),outer_element});diag.text("outer shell is an oriented_closed_shell .T. (ABSR WR3 forbids it); read as its closed shell, as OCCT reads it");fields(diag);diagnostics.push_back(diag);}
      shells.push_back({0,id(raw->Outer()),outer_element,true,outer});
      if(auto with_voids=Handle(StepShape_BrepWithVoids)::DownCast(raw);!with_voids.IsNull())for(int k=1;k<=with_voids->NbVoids();++k){auto shell=with_voids->VoidsValue(k);if(shell.IsNull()||shell->ClosedShellElement().IsNull())throw std::runtime_error("void without closed shell at solid "+std::to_string(id(raw)));if(shell->Orientation())throw std::runtime_error("void shell "+std::to_string(id(shell))+" of solid "+std::to_string(id(raw))+" has orientation .T.; ABSR WR4 requires every void to have orientation .F.");shells.push_back({U(k),id(shell),id(shell->ClosedShellElement()),shell->Orientation(),shell->ClosedShellElement()});}
      std::set<U> faces;for(const auto& shell:shells)for(int fi=1;fi<=shell.faces->NbCfsFaces();++fi)if(!faces.insert(id(shell.faces->CfsFacesValue(fi))).second)throw std::runtime_error("source face in more than one shell of solid "+std::to_string(id(raw)));
      return shells;};
    auto result=[&](const Handle(Standard_Transient)& e){auto s=TransferBRep::ShapeResult(tp,e);if(s.IsNull())throw std::runtime_error("unmapped original source entity "+std::to_string(id(e)));return s;};
    std::map<std::string,std::vector<Bytes>> tables;tables["DIAGS___"]=unit_records;std::map<U,Handle(StepShape_ManifoldSolidBrep)> raw_solids;
    std::map<U,TopoDS_Shape> solids,product_shapes;std::map<U,U> product_solids,solid_products,part_styles;std::map<U,gp_Trsf> solid_in_product;
    std::map<U,StyleData> styles;std::set<U> parents,children;std::vector<Handle(StepBasic_ProductDefinition)> products;
    std::vector<Handle(StepRepr_NextAssemblyUsageOccurrence)> usages;
    double max_vertex_displacement=0;
    for(int i=1;i<=model->NbEntities();++i){auto e=model->Value(i);
      if(auto s=Handle(StepShape_ManifoldSolidBrep)::DownCast(e);!s.IsNull()){raw_solids[id(s)]=s;solids[id(s)]=result(s);}
      if(auto p=Handle(StepBasic_ProductDefinition)::DownCast(e);!p.IsNull())products.push_back(p);
      if(auto u=Handle(StepRepr_NextAssemblyUsageOccurrence)::DownCast(e);!u.IsNull()){usages.push_back(u);parents.insert(id(u->RelatingProductDefinition()));children.insert(id(u->RelatedProductDefinition()));}
    }
    // Styles: one resolver for both exporters (native/step_styles.hpp).
    brepgpu::step_styles::Resolver style_resolver(model,reader.Reader().WS());
    for(const auto& c:style_resolver.colours()){StyleData d;d.id=c.id;d.sources=c.sources;d.rgba={c.rgba[0],c.rgba[1],c.rgba[2],c.rgba[3]};d.color=c.color;styles[d.id]=d;}
    std::map<U,TDF_Label> product_labels;
    for(auto& p:products){U pi=id(p);auto ps=result(p);product_shapes[pi]=ps;
      std::set<U> matches;
      std::map<U,gp_Trsf> located_in;for(TopExp_Explorer ex(ps,TopAbs_SOLID);ex.More();ex.Next())for(auto& s:solids)if(ex.Current().IsSame(s.second)){matches.insert(s.first);located_in.emplace(s.first,ex.Current().Location().Transformation()*s.second.Location().Transformation().Inverted());}
      if(!parents.count(pi)){
        if(matches.size()!=1)throw std::runtime_error("leaf product does not bind one source solid "+std::to_string(pi));
        U si=*matches.begin();if(solid_products.count(si))throw std::runtime_error("solid has multiple leaf products");product_solids[pi]=si;solid_products[si]=pi;solid_in_product[si]=located_in.at(si);
      }
      TDF_Label label;shape_tool->Search(ps,label,true,true,false);product_labels[pi]=label;
      auto prod=p->Formation()->OfProduct();std::string name=str(prod->Description());if(name.empty())name=str(prod->Name());
      // Product style: its shape representation's (assembly node or component
      // part), else for a part its solid's. Outer levels beat inner ones.
      std::set<U> part_reps;if(product_solids.count(pi))for(const auto& holder:representations.holders(raw_solids.at(product_solids[pi])))for(const auto& rep:representations.transfer_path(holder))part_reps.insert(id(rep));
      U style=style_resolver.product_style(pi,part_reps,product_solids.count(pi)?product_solids[pi]:0);part_styles[pi]=style;
      if(!label.IsNull()){
        Handle(TDataStd_TreeNode) node;
        if(label.FindAttribute(XCAFDoc::MaterialRefGUID(),node)&&node->HasFather()){
          Handle(TCollection_HAsciiString) mn,md,un,vt;double density=0;
          if(XCAFDoc_MaterialTool::GetMaterial(node->Father()->Label(),mn,md,density,un,vt)){
            StyleData sd;if(style)sd=styles.at(style);sd.id=(U(1)<<62)|pi;sd.sources.push_back(pi);sd.material=str(mn);sd.unit=str(un)+" "+str(vt);sd.density=density;sd.density_set=true;styles[sd.id]=sd;style=sd.id;
          }
        }
      }
      Bytes w;w.u64(pi);w.u64(product_solids.count(pi)?product_solids[pi]:0);w.u64(style);w.text(name);w.text(str(prod->Id()));tables["PRODUCTS"].push_back(w);
    }
    if(solid_products.size()!=solids.size())throw std::runtime_error("unowned source solid");
    IDs roots;for(auto& p:products)if(!children.count(id(p)))roots.push_back(id(p));if(roots.size()!=1)throw std::runtime_error("ambiguous source root");
    // Per-occurrence styles (CDORSI, Model Styling 5.1): native/step_styles.hpp.
    auto face_on=[&](U product,U face_id){const auto solid=product_solids.find(product);if(solid==product_solids.end())return false;auto raw=raw_solids.at(solid->second);std::vector<Handle(StepShape_ConnectedFaceSet)> sets;
      if(auto outer=Handle(StepShape_OrientedClosedShell)::DownCast(raw->Outer());!outer.IsNull())sets.push_back(outer->ClosedShellElement());else sets.push_back(raw->Outer());
      if(auto with_voids=Handle(StepShape_BrepWithVoids)::DownCast(raw);!with_voids.IsNull())for(int k=1;k<=with_voids->NbVoids();++k)if(!with_voids->VoidsValue(k).IsNull())sets.push_back(with_voids->VoidsValue(k)->ClosedShellElement());
      for(const auto& set:sets)if(!set.IsNull())for(int k=1;k<=set->NbCfsFaces();++k)if(id(set->CfsFacesValue(k))==face_id)return true;return false;};
    for(const auto& row:style_resolver.occurrences(roots[0],face_on)){Bytes w;w.u64(row.style);w.u64(row.face);w.ids(row.path);tables["OCCSTYLE"].push_back(w);}
    // CARRIERS rows: one per surface entity in first-face order, written after
    // every face, with the UV hint the union of all its faces' hints (.37.29).
    std::map<U,Bnd_Box> solid_extents;
    struct CarrierRow{Handle(Geom_Surface) surface;double u0=0,u1=0,v0=0,v1=0;};std::map<U,CarrierRow> carriers;std::vector<U> carrier_order;U raw_nets=0,raw_curve_nets=0,raw_pcurve_nets=0;
    std::set<U> seen_vertices,seen_edges,seen_curves;U nfaces=0,nloops=0,ncoedges=0,degenerate=0,not_same=0,nvertex_loops=0;unsigned max_pcurve_degree=0;
    std::map<U,std::string> vertex_loop_translations;struct LoopRefusal{U loop=0,vertex=0;vl::Reason reason=vl::Reason::admitted;std::string message;};
    std::map<U,std::vector<LoopRefusal>> refused_faces;std::set<U> refused_loops;U incomplete_definitions=0;
    // Faces refused, per reason (a face refused for two reasons counts in both).
    std::map<vl::Reason,U> face_refusals;std::set<std::pair<U,std::uint32_t>> counted_refusals;U refused_face_count=0,faces_orientation_raw=0,open_shells=0,region_tests=0,region_choices=0;std::map<int,std::array<U,2>> orientation_counts;
    for(auto& pair:raw_solids){U si=pair.first;auto raw=pair.second;IDs fs,es,vs;std::set<U> unique_edges,unique_vertices;bool solid_refused=false;
      std::vector<brepgpu::SourceUVFace> uv_faces;const auto shells=raw_shells(raw,tables["DIAGS___"]);const double angle_factor=solid_angle_factor(raw,tables["DIAGS___"]);const double length_factor=solid_length(raw,tables["DIAGS___"]);
      for(const auto& shell:shells)for(int fi=1;fi<=shell.faces->NbCfsFaces();++fi){auto rf=Handle(StepShape_AdvancedFace)::DownCast(shell.faces->CfsFacesValue(fi));if(rf.IsNull())throw std::runtime_error("non-advanced UV source face");
        brepgpu::SourceUVFace face;face.id=id(rf);face.carrier_id=id(rf->FaceGeometry());face.face=TopoDS::Face(result(rf));auto forward_face=face.face;
        std::vector<TopoDS_Wire> wires;for(TopExp_Explorer w(forward_face,TopAbs_WIRE);w.More();w.Next())wires.push_back(TopoDS::Wire(w.Current()));
        std::vector<Handle(StepShape_VertexLoop)> vertex_loops;
        for(int bi=1;bi<=rf->NbBounds();++bi){auto bound=rf->BoundsValue(bi);
          if(auto vertex_loop=Handle(StepShape_VertexLoop)::DownCast(bound->Bound());!vertex_loop.IsNull()){vertex_loops.push_back(vertex_loop);continue;}
          auto loop=Handle(StepShape_EdgeLoop)::DownCast(bound->Bound());if(loop.IsNull())throw std::runtime_error("face "+std::to_string(face.id)+" bound "+std::to_string(id(bound->Bound()))+" is neither an edge_loop nor a vertex_loop");brepgpu::SourceUVLoop binding;binding.id=id(loop);
          for(int position=1;position<=loop->NbEdgeList();++position){int ei=bound->Orientation()?position:loop->NbEdgeList()+1-position;auto oe=loop->EdgeListValue(ei);auto re=Handle(StepShape_EdgeCurve)::DownCast(oe->EdgeElement());bool forward=oe->Orientation()==bound->Orientation();brepgpu::SourceUVUse use;use.id=id(oe);use.edge_id=id(re);use.edge=TopoDS::Edge(result(re));use.edge.Orientation((forward==re->SameSense())?TopAbs_FORWARD:TopAbs_REVERSED);auto va=forward?re->EdgeStart():re->EdgeEnd(),vb=forward?re->EdgeEnd():re->EdgeStart();use.start_id=id(va);use.end_id=id(vb);use.start=TopoDS::Vertex(result(va));use.end=TopoDS::Vertex(result(vb));binding.uses.push_back(use);}
          std::vector<std::size_t> matches;for(std::size_t wi=0;wi<wires.size();++wi){std::vector<TopoDS_Edge> uses;for(TopExp_Explorer e(wires[wi],TopAbs_EDGE);e.More();e.Next())uses.push_back(TopoDS::Edge(e.Current()));if(uses.size()!=binding.uses.size())continue;bool okay=true;for(auto& edge:uses)if(std::none_of(binding.uses.begin(),binding.uses.end(),[&](const brepgpu::SourceUVUse& use){return edge.IsSame(use.edge);}))okay=false;if(okay)matches.push_back(wi);}if(matches.size()!=1)throw std::runtime_error("ambiguous original wire UV binding at face "+std::to_string(face.id));binding.wire=wires[matches[0]];wires.erase(wires.begin()+matches[0]);face.loops.push_back(binding);
        }
        // A vertex_loop bound is one point vertex (AP242 advanced_face WR6,
        // WR7), always carried from the raw entity. native/step_vertex_loops.hpp
        // applies OCCT's rule to the surface OCCT built for the face and tests the
        // pole against the solid's usable uncertainty; a loop that fails is
        // refused with its reason, which refuses its face (status and record),
        // never the file.
        for(const auto& vertex_loop:vertex_loops){const std::string where="face "+std::to_string(face.id)+" vertex_loop "+std::to_string(id(vertex_loop));
          auto pole=Handle(StepShape_VertexPoint)::DownCast(vertex_loop->LoopVertex());auto point=pole.IsNull()?Handle(StepGeom_CartesianPoint)():Handle(StepGeom_CartesianPoint)::DownCast(pole->VertexGeometry());
          if(point.IsNull()||point->NbCoordinates()!=3)throw std::runtime_error(where+": its vertex is not a vertex_point on a cartesian_point (AP242 advanced_face WR6)");
          brepgpu::SourceUVLoop binding;binding.id=id(vertex_loop);binding.pole_id=id(pole);
          const auto translated=TransferBRep::ShapeResult(tp,pole);if(!translated.IsNull())binding.pole=TopoDS::Vertex(translated);
          const gp_Pnt p(point->CoordinatesValue(1)*length_factor,point->CoordinatesValue(2)*length_factor,point->CoordinatesValue(3)*length_factor);
          auto assessed=vl::assess(forward_face,wires,rf->NbBounds(),binding.pole,p,solid_uncertainty);binding.wire=assessed.wire;
          if(assessed.reason!=vl::Reason::admitted){refused_faces[face.id].push_back({binding.id,binding.pole_id,assessed.reason,assessed.refusal});refused_loops.insert(binding.id);}
          vertex_loop_translations[binding.id]=assessed.translation;face.loops.push_back(binding);}
        // A wire OCCT made that no loop explains: on a face with vertex loops
        // the translation differs from OCCT's rule, so the face owns the wire and
        // every vertex loop of it not already refused is refused for it. On a face
        // without vertex loops it is unexplained translation, and the file refuses.
        if(!wires.empty()){
          if(vertex_loops.empty())throw std::runtime_error("unowned native wire before UV preparation at face "+std::to_string(face.id));
          for(const auto& loop:face.loops)if(loop.pole_id&&!refused_loops.count(loop.id)){refused_faces[face.id].push_back({loop.id,loop.pole_id,vl::Reason::translation_differs,"OCCT made a wire on this face that its vertex-loop rule does not explain"});refused_loops.insert(loop.id);}
          face.unexplained=wires;wires.clear();}
        uv_faces.push_back(face);
      }
      // Every translated shell and face must be one this walk owns; a shell
      // OCCT dropped (or one we did not visit) refuses instead of passing.
      {U translated_shells=0,translated_faces=0;for(TopExp_Explorer e(solids.at(si),TopAbs_SHELL);e.More();e.Next())++translated_shells;for(TopExp_Explorer e(solids.at(si),TopAbs_FACE);e.More();e.Next())++translated_faces;
       if(translated_shells!=shells.size()||translated_faces!=uv_faces.size())throw std::runtime_error("solid "+std::to_string(si)+" translated shells/faces "+std::to_string(translated_shells)+"/"+std::to_string(translated_faces)+" differ from original "+std::to_string(shells.size())+"/"+std::to_string(uv_faces.size()));}
      auto prepared=[&](){try{return brepgpu::prepare_source_uv(solids.at(si),uv_faces,uv_options);}
        // B2 .37.31: an escaping UV curve the domain correction cannot take
        // refuses the file; its form and escape are written as typed fields
        // (name, kind, value), as DIAGS___ fields are, before the refusal.
        catch(const brepgpu::SourceUVDomainEscape& e){
          for(const auto& f:std::vector<Field>{{"solid",code,double(si)},{"face",code,double(e.face_id)},{"coedge",code,double(e.coedge_id)},{"curve_kind",code,double(e.kind)},{"rational",boolean,e.rational?1.:0.},{"periodic",boolean,e.periodic?1.:0.},{"trimmed",boolean,e.trimmed?1.:0.},{"largest_escape",parameter,e.escape}})
            std::cout<<"refusal_field\tsource_uv_domain_escape\t"<<f.name<<'\t'<<f.kind<<'\t'<<std::setprecision(17)<<f.value<<"\n";
          throw std::runtime_error(e.what());}
        catch(const Standard_Failure& e){throw std::runtime_error("solid "+std::to_string(si)+" source UV "+e.ExceptionType()+" "+e.what());}}();
      auto source_result=[&](const Handle(Standard_Transient)& e){return prepared.mapped(result(e));};solids.at(si)=prepared.raw_shape;
      // OCCT's wires for vertex loops carry no original edge use; the
      // edge-use comparisons below skip them.
      std::map<U,std::vector<TopoDS_Wire>> pole_wires;
      for(const auto& face:uv_faces){for(const auto& loop:face.loops)if(loop.pole_id&&!loop.wire.IsNull())pole_wires[face.id].push_back(TopoDS::Wire(prepared.mapped(loop.wire)));
        for(const auto& wire:face.unexplained)pole_wires[face.id].push_back(TopoDS::Wire(prepared.mapped(wire)));}
      source_uv_seconds+=prepared.stats.projection_seconds;query_uv_seconds+=prepared.stats.query_seconds;source_uv_projected+=prepared.stats.projected_edges;source_uv_max_residual=std::max(source_uv_max_residual,prepared.stats.max_residual_mm);source_uv_seams+=prepared.stats.seam_pairs;query_columns+=prepared.stats.query_column_edges;query_endpoints+=prepared.stats.query_joint_endpoints;query_connectors+=prepared.stats.query_connectors;query_max_residual=std::max(query_max_residual,prepared.stats.max_query_residual_mm);query_max_uv_metric=std::max(query_max_uv_metric,prepared.stats.max_query_uv_metric_mm);
      for(const auto& c:prepared.chart_approximations){++source_chart_count;source_chart_max_uv_metric=std::max(source_chart_max_uv_metric,c.uv_metric_mm);std::ostringstream message;message<<std::setprecision(17)<<"bounded-conic-natural-side; original-t; axis="<<c.axis<<" side="<<c.side<<" max_guide_coefficient_delta="<<c.max_guide_coefficient_delta<<" uv_metric_mm="<<c.uv_metric_mm<<" guide_extension_residual_mm="<<c.max_guide_extension_residual_mm<<" selected_finite_residual_mm="<<c.max_selected_residual_mm;Bytes w;w.u32(0);w.u32(0);w.text("source_chart_approximation");w.ids({si,c.face_id,c.coedge_id});w.text("bounded-conic-natural-side; original-t; the guide extension residual may evaluate outside the original domain");
        fields(w,{{"axis",code,double(c.axis)},{"side",parameter,c.side},{"max_guide_coefficient_delta",parameter,c.max_guide_coefficient_delta},{"uv_metric_mm",length_mm,c.uv_metric_mm},{"guide_extension_residual_mm",length_mm,c.max_guide_extension_residual_mm},{"selected_finite_residual_mm",length_mm,c.max_selected_residual_mm}});
        tables["DIAGS___"].push_back(w);std::cout<<"source_chart_approximation\t"<<si<<'\t'<<c.face_id<<'\t'<<c.coedge_id<<'\t'<<message.str()<<"\n";}
      // B2 .37.31: projected UV curves moved into their B-spline carrier's
      // natural knot rectangle, one record per coedge (native/source_uv).
      for(const auto& c:prepared.domain_clamps){++source_domain_corrections;Bytes w;w.u32(1);w.u32(0);w.text("source_chart_domain_correction");w.ids({si,c.face_id,c.coedge_id});
        w.text("projected UV curve left the B-spline carrier's natural knot rectangle; every coefficient was moved into it (convex hull); the sampled original-t correspondence is the guarantee, the derivative-metric term a charge to the chord allocation");
        fields(w,{{"max_u_move",parameter,c.max_u_move},{"max_v_move",parameter,c.max_v_move},{"uv_metric_mm",length_mm,c.uv_metric_mm},{"max_sampled_residual_mm",length_mm,c.max_residual_mm}});tables["DIAGS___"].push_back(w);}
      for(const auto& c:prepared.connectors)std::cout<<"source_query_connector\t"<<si<<'\t'<<c.face_id<<'\t'<<c.loop_id<<'\t'<<c.vertex_id<<'\t'<<c.incoming_id<<'\t'<<c.outgoing_id<<'\t'<<std::setprecision(17)<<c.u0<<'\t'<<c.v0<<'\t'<<c.u1<<'\t'<<c.v1<<"\n";
      if(uv_options.prepare_query){auto path=std::filesystem::path(query_directory)/("definition-"+std::to_string(si)+".brep");if(!BinTools::Write(prepared.query_shape,path.string().c_str(),false,false,BinTools_FormatVersion_VERSION_4))throw std::runtime_error("query BRep write failed");}

      // OCCT's BRepCheck on the translated solid (with the recovered pcurves),
      // in its default sampled mode. One finding per (subject, context,
      // status), naming the original entity: outcome 1 fail, 2 not evaluated
      // (BRepCheck_CheckFail: the check itself broke), never folded together
      // (G11 3.1). The summary counts subjects and both outcomes.
      {NCollection_DataMap<TopoDS_Shape,U,TopTools_ShapeMapHasher> originals;
       auto own=[&](const TopoDS_Shape& s,U original){if(!s.IsNull()&&!originals.IsBound(s))originals.Bind(s,original);};
       own(solids.at(si),si);
       for(const auto& face:uv_faces){own(prepared.mapped(face.face),face.id);for(const auto& wire:face.unexplained)own(prepared.mapped(wire),face.id);
         for(const auto& loop:face.loops){if(!loop.wire.IsNull())own(prepared.mapped(loop.wire),loop.id);if(loop.pole_id&&!loop.pole.IsNull())own(prepared.mapped(loop.pole),loop.pole_id);
           for(const auto& use:loop.uses){own(prepared.mapped(use.edge),use.edge_id);own(prepared.mapped(use.start),use.start_id);own(prepared.mapped(use.end),use.end_id);}}}
       // A translated shell is the original shell that holds its faces.
       for(TopExp_Explorer e(solids.at(si),TopAbs_SHELL);e.More();e.Next())for(TopExp_Explorer f(e.Current(),TopAbs_FACE);f.More();f.Next()){
         if(!originals.IsBound(f.Current()))continue;const U face_id=originals.Find(f.Current());
         for(const auto& shell:shells)for(int k=1;k<=shell.faces->NbCfsFaces();++k)if(id(shell.faces->CfsFacesValue(k))==face_id)own(e.Current(),shell.element);break;}
       auto original=[&](const TopoDS_Shape& s)->U{return originals.IsBound(s)?originals.Find(s):U(0);};
       U checked=0,failed=0,unevaluated=0,subject_total=0;
       auto finding=[&](const TopoDS_Shape& subject,const TopoDS_Shape& context,BRepCheck_Status status){
         const bool broke=status==BRepCheck_CheckFail;(broke?unevaluated:failed)++;std::ostringstream name;BRepCheck::Print(status,name);std::string text=name.str();while(!text.empty()&&std::isspace(static_cast<unsigned char>(text.back())))text.pop_back();
         Bytes diag;diag.u32(1);diag.u32(0);diag.text("brepcheck_finding");diag.ids({si,original(subject),context.IsNull()?U(0):original(context)});
         diag.text(std::string(broke?"not evaluated (the check itself failed): ":"fail: ")+text+(context.IsNull()?"":" in the context of its owner"));
         fields(diag,{{"outcome",code,broke?2.:1.},{"status",code,double(status)},{"subject_kind",code,double(subject.ShapeType())},{"context_kind",code,context.IsNull()?-1.:double(context.ShapeType())}});tables["DIAGS___"].push_back(diag);};
       try{BRepCheck_Analyzer analyzer(solids.at(si));TopTools_IndexedMapOfShape subjects;subjects.Add(solids.at(si));
         for(auto type:{TopAbs_SHELL,TopAbs_FACE,TopAbs_WIRE,TopAbs_EDGE,TopAbs_VERTEX})TopExp::MapShapes(solids.at(si),type,subjects);subject_total=U(subjects.Extent());
         for(int k=1;k<=subjects.Extent();++k){const auto& subject=subjects(k);Handle(BRepCheck_Result) check;bool raised=false;try{check=analyzer.Result(subject);}catch(const Standard_Failure&){raised=true;}
           // A subject the analyzer has no result for, or whose result raises,
           // was not evaluated: one outcome-2 finding, never silent.
           if(raised||check.IsNull()){finding(subject,TopoDS_Shape(),BRepCheck_CheckFail);continue;}++checked;
           for(const auto status:check->Status())if(status!=BRepCheck_NoError)finding(subject,TopoDS_Shape(),status);
           for(check->InitContextIterator();check->MoreShapeInContext();check->NextShapeInContext())for(const auto status:check->StatusOnShape())if(status!=BRepCheck_NoError)finding(subject,check->ContextualShape(),status);}}
       catch(const Standard_Failure&){finding(solids.at(si),TopoDS_Shape(),BRepCheck_CheckFail);}
       Bytes diag;diag.u32(failed||unevaluated?1:0);diag.u32(0);diag.text("brepcheck");diag.ids({si});
       diag.text("OCCT BRepCheck_Analyzer, default sampled mode, on the translated solid with recovered pcurves; findings are brepcheck_finding records");
       fields(diag,{{"subjects",count,double(subject_total)},{"checked_subjects",count,double(checked)},{"failed",count,double(failed)},{"not_evaluated",count,double(unevaluated)}});tables["DIAGS___"].push_back(diag);}
      GProp_GProps volume;BRepGProp::VolumeProperties(solids.at(si),volume);std::cout<<"reference_volume\t"<<si<<"\t"<<std::setprecision(17)<<volume.Mass()<<"\n";
      // B2 .37.36. Each face's raw use (same_sense composed with its shell) is
      // tested against the standard's two propositions: its outer bound runs
      // counter-clockwise about the face normal (OCCT's classifier on the face
      // as translated with its pcurves: the infinite point is outside), and
      // in a closed shell every edge is used once in each direction (raw
      // edge-use senses, which do not involve same_sense). The rows are
      // written once every face of the solid is decided.
      // Kinds: an edge face (edge loops only: OCCT's classifier and its volume
      // integral with pcurves), a pole face (edge loops and one vertex loop at
      // a singular pole: owned Green sums), a natural face (vertex loops only,
      // natural bounds: owned quadrature over the domain), or undecided.
      enum class FaceKind{edge,pole,natural,undecided,closed};
      struct PendingFace{U fid=0,carrier=0,style=0,ordinal=0,element=0;int raw=1,bound=0;double tolerance=0,volume=0,area=0;IDs loops;bool refused=false,prior_refused=false,contradicts=false,same_sense=true,shell_forward=true,two_regions=false;double alt_volume=0;TwoRegions region,alt_region;bool raw_inside=true,alt_inside=false,region_test=false,test_ran=false,corrected_region=false;int raw_meets=-1,alt_meets=-1,crossings_only=0;double blind_strip_mm=0,blind_cell_mm=0,blind_target_mm=INFINITY;int decided_by=0;Fold fold;std::array<double,4> hint{};Handle(Geom_Surface) surface;Handle(Geom_Surface) region_surface;double region_resolution=1e-7;
        FaceKind kind=FaceKind::undecided;std::string why;U refusal_loop=0,refusal_vertex=0;std::size_t refusal_row=0;bool refusal_row_set=false;TopoDS_Shape oriented;};
      std::vector<PendingFace> pending_faces;std::map<std::pair<U,U>,std::vector<std::pair<U,int>>> edge_uses;
      // Review round 3, finding 1: each shell's edge-connected components (faces
      // joined by a shared raw edge). Closure, S1/S2 and the volume sign are
      // decided per component, so one sign is never taken over two parts.
      // A component is keyed by its first face in shell order.
      std::map<U,U> face_component,component_shell;std::map<U,std::vector<U>> shell_components;
      {std::map<U,U> parent;std::function<U(U)> root=[&](U x)->U{U r=x;while(parent.at(r)!=r)r=parent.at(r);while(parent.at(x)!=r){U n=parent.at(x);parent[x]=r;x=n;}return r;};
        for(const auto& shell:shells){std::map<U,U> edge_face;std::vector<U> order;
          for(int fi=1;fi<=shell.faces->NbCfsFaces();++fi){auto rf=Handle(StepShape_AdvancedFace)::DownCast(shell.faces->CfsFacesValue(fi));if(rf.IsNull())throw std::runtime_error("non-advanced source face");const U fid=id(rf);parent[fid]=fid;order.push_back(fid);
            for(int bi=1;bi<=rf->NbBounds();++bi)if(auto loop=Handle(StepShape_EdgeLoop)::DownCast(rf->BoundsValue(bi)->Bound());!loop.IsNull())for(int k=1;k<=loop->NbEdgeList();++k){const U eid=id(loop->EdgeListValue(k)->EdgeElement());
              if(auto other=edge_face.find(eid);other==edge_face.end())edge_face.emplace(eid,fid);else{const U ra=root(fid),rb=root(other->second);if(ra!=rb)parent[std::max(ra,rb)]=std::min(ra,rb);}}}
          // The key is the component's first face in shell order, not the smallest id.
          std::map<U,U> key;for(U fid:order){const U r=root(fid);if(!key.count(r)){key[r]=fid;shell_components[shell.ordinal].push_back(fid);component_shell[fid]=shell.ordinal;}face_component[fid]=key.at(r);}}}
      // Each component's box (OCCT's translated faces): its centre is the point
      // every volume share of that component is taken about, its volume the
      // scale. The box is of the faces' edges (bounded by their vertices); a
      // face without edges (natural bounds) adds its own finite natural domain.
      // A face whose edges lack pcurves would otherwise box its whole surface.
      // Review round 4, item 1: component_box (of edges) only places the volume
      // reference point. Nesting is tested on component_extent, the union of
      // its faces' extents (each face's carrier boxed over its UV hint, which
      // reaches any pole the face's region holds), with component_point, a
      // point on the component (a vertex of its first face, else a point of
      // that face's natural domain).
      std::map<U,Bnd_Box> component_extent;std::map<U,gp_Pnt> component_point;
      std::map<U,Bnd_Box> component_box;std::map<U,gp_Pnt> component_centre;std::map<U,double> component_box_volume;
      for(const auto& shell:shells)for(int fi=1;fi<=shell.faces->NbCfsFaces();++fi){const U fid=id(shell.faces->CfsFacesValue(fi));Bnd_Box& box=component_box[face_component.at(fid)];const auto face=result(shell.faces->CfsFacesValue(fi));bool edged=false;
          for(TopExp_Explorer e(face,TopAbs_EDGE);e.More();e.Next())if(!BRep_Tool::Degenerated(TopoDS::Edge(e.Current()))){BRepBndLib::Add(e.Current(),box);edged=true;}
          if(!edged)BRepBndLib::Add(face,box);
          if(const U key=face_component.at(fid);!component_point.count(key)){TopExp_Explorer vertex(face,TopAbs_VERTEX);if(vertex.More())component_point[key]=BRep_Tool::Pnt(TopoDS::Vertex(vertex.Current()));
            else{TopLoc_Location l;auto surface=BRep_Tool::Surface(TopoDS::Face(face),l);double a0,a1,b0,b1;surface->Bounds(a0,a1,b0,b1);if(std::isfinite(a0)&&std::isfinite(a1)&&std::isfinite(b0)&&std::isfinite(b1))component_point[key]=surface->Value(.5*(a0+a1),.5*(b0+b1)).Transformed(l.Transformation());}}}
      for(const auto& [component,box]:component_box){double x0=0,y0=0,z0=0,x1=0,y1=0,z1=0;if(!box.IsVoid())box.Get(x0,y0,z0,x1,y1,z1);
        component_centre[component]=gp_Pnt(.5*(x0+x1),.5*(y0+y1),.5*(z0+z1));component_box_volume[component]=(x1-x0)*(y1-y0)*(z1-z0);}
      for(const auto& shell:shells)for(int fi=1;fi<=shell.faces->NbCfsFaces();++fi){auto rf=Handle(StepShape_AdvancedFace)::DownCast(shell.faces->CfsFacesValue(fi));if(rf.IsNull())throw std::runtime_error("non-advanced source face");U fid=id(rf);fs.push_back(fid);++nfaces;auto f=TopoDS::Face(source_result(rf));TopLoc_Location fl;auto surf=BRep_Tool::Surface(f,fl);if(!fl.IsIdentity())surf=Handle(Geom_Surface)::DownCast(surf->Transformed(fl.Transformation()));
        std::vector<unsigned> raw_wire_counts;std::multiset<U> raw_edge_uses;
        std::map<U,TopoDS_Edge> face_edges,actual_face_edges;
        std::map<U,TopoDS_Vertex> face_vertices;
        const auto& face_pole_wires=pole_wires[fid];
        auto pole_wire_edge=[&](const TopoDS_Shape& edge){for(const auto& wire:face_pole_wires)for(TopExp_Explorer e(wire,TopAbs_EDGE);e.More();e.Next())if(e.Current().IsSame(edge))return true;return false;};
        for(int bi=1;bi<=rf->NbBounds();++bi){if(rf->BoundsValue(bi)->Bound()->IsKind(STANDARD_TYPE(StepShape_VertexLoop)))continue;auto loop=Handle(StepShape_EdgeLoop)::DownCast(rf->BoundsValue(bi)->Bound());if(loop.IsNull())throw std::runtime_error("non-edge source loop");
          for(int j=1;j<=loop->NbEdgeList();++j){auto re=Handle(StepShape_EdgeCurve)::DownCast(loop->EdgeListValue(j)->EdgeElement());if(re.IsNull())throw std::runtime_error("non-curve source edge");face_edges[id(re)]=TopoDS::Edge(source_result(re));face_vertices[id(re->EdgeStart())]=TopoDS::Vertex(source_result(re->EdgeStart()));face_vertices[id(re->EdgeEnd())]=TopoDS::Vertex(source_result(re->EdgeEnd()));}}
        auto match_actual_edge=[&](const TopoDS_Edge& actual,bool emit)->U{
          for(auto& candidate:face_edges)if(actual.IsSame(candidate.second)){actual_face_edges[candidate.first]=actual;return candidate.first;}
          TopLoc_Location al;double af=0,az=0;auto ac=BRep_Tool::Curve(actual,al,af,az);
          if(ac.IsNull()&&BRep_Tool::Degenerated(actual)){
            // A source loop passing an analytic pole has two UV endpoint
            // occurrences but no physical source edge between them. OCCT may
            // add a chart connector. Check its actual original vertex and UV
            // singular locus; never invent an original EDGE_CURVE identifier.
            U pole_id=0;bool vertices_ok=true;
            for(TopExp_Explorer ve(actual,TopAbs_VERTEX);ve.More();ve.Next()){
              U found=0;for(auto& v:face_vertices)if(ve.Current().IsSame(v.second)){found=v.first;break;}
              if(!found||(pole_id&&pole_id!=found)){vertices_ok=false;break;}pole_id=found;
            }
            double pf=0,pl=0;auto pc=BRep_Tool::CurveOnSurface(actual,f,pf,pl);bool analytic=false;
            if(vertices_ok&&pole_id&&!pc.IsNull()){
              auto a=pc->Value(pf),b=pc->Value(pl);GeomAdaptor_Surface sa(surf);auto p=BRep_Tool::Pnt(face_vertices.at(pole_id));
              if(sa.GetType()==GeomAbs_Sphere){auto sphere=sa.Sphere();double pole=std::copysign(std::acos(-1.)/2,a.Y());auto expected=sphere.Location().Translated(gp_Vec(sphere.Position().Direction())*(std::copysign(sphere.Radius(),pole)));
                analytic=std::abs(a.Y()-pole)<1e-12&&std::abs(b.Y()-pole)<1e-12&&std::abs(a.X()-b.X())<=2*std::acos(-1.)+1e-12&&p.Distance(expected)<=std::ldexp(1.,-32);
              }else if(sa.GetType()==GeomAbs_Cone){auto cone=sa.Cone();double v=-cone.RefRadius()/std::sin(cone.SemiAngle());auto expected=cone.Location().Translated(gp_Vec(cone.Axis().Direction())*(v*std::cos(cone.SemiAngle())));
                analytic=std::abs(a.Y()-v)<1e-12&&std::abs(b.Y()-v)<1e-12&&std::abs(a.X()-b.X())<=2*std::acos(-1.)+1e-12&&p.Distance(expected)<=std::ldexp(1.,-32);
              }
            }
            if(analytic){if(emit){Bytes diag;diag.u32(1);diag.u32(0);diag.text("translated_singular_chart_connector");diag.ids({fid,pole_id});diag.text("foreign UV chart connector collapses to original analytic pole vertex; original physical topology retained");fields(diag);tables["DIAGS___"].push_back(diag);}return 0;}
          }
          IDs candidates;
          for(auto& candidate:face_edges){TopLoc_Location ol;double of=0,oz=0;auto oc=BRep_Tool::Curve(candidate.second,ol,of,oz);if(ac==oc&&al==ol&&af==of&&az==oz)candidates.push_back(candidate.first);}
          if(candidates.size()!=1)throw std::runtime_error("translated face contains unmatched source edge "+std::to_string(fid));
          U match=candidates[0];auto original=face_edges.at(match);TopoDS_Vertex a1,a2,o1,o2;TopExp::Vertices(actual,a1,a2);TopExp::Vertices(original,o1,o2);
          if(!a1.IsSame(o1)||!a2.IsSame(o2)||BRep_Tool::SameParameter(actual)!=BRep_Tool::SameParameter(original)||BRep_Tool::Degenerated(actual)!=BRep_Tool::Degenerated(original))throw std::runtime_error("translated edge copy changed source endpoints/flags at face "+std::to_string(fid));
          actual_face_edges[match]=actual;
          if(emit){Bytes diag;diag.u32(1);diag.u32(0);diag.text("translated_edge_topology_copy");diag.ids({fid,match});diag.text("distinct edge topology with identical carrier/location/range/endpoints/flags");fields(diag,{{"source_tolerance_mm",length_mm,BRep_Tool::Tolerance(original)},{"face_tolerance_mm",length_mm,BRep_Tool::Tolerance(actual)}});tables["DIAGS___"].push_back(diag);}return match;
        };
        for(TopExp_Explorer actual(f,TopAbs_EDGE);actual.More();actual.Next())if(!pole_wire_edge(actual.Current()))match_actual_edge(TopoDS::Edge(actual.Current()),true);
        U sid=id(rf->FaceGeometry());double u0,u1,v0,v1;BRepTools::UVBounds(f,u0,u1,v0,v1);
        // OCCT's vertex-loop edge has no pcurve, so the UV hint must also hold
        // each pole: at a singular point only the coordinate that is fixed
        // along the degenerate iso-line widens (the other is not determined);
        // elsewhere the projected point widens both. Without a usable
        // uncertainty the face is refused as not evaluable and its poles are
        // not projected (no defaulted threshold).
        if(solid_uncertainty)for(int bi=1;bi<=rf->NbBounds();++bi)if(auto vertex_loop=Handle(StepShape_VertexLoop)::DownCast(rf->BoundsValue(bi)->Bound());!vertex_loop.IsNull()){
          auto point=Handle(StepGeom_CartesianPoint)::DownCast(Handle(StepShape_VertexPoint)::DownCast(vertex_loop->LoopVertex())->VertexGeometry());
          const gp_Pnt p(point->CoordinatesValue(1)*length_factor,point->CoordinatesValue(2)*length_factor,point->CoordinatesValue(3)*length_factor);
          const double tolerance=*solid_uncertainty;ShapeAnalysis_Surface analysis(surf);gp_Pnt2d first,last;double a=0,b=0;
          if(analysis.DegeneratedValues(p,tolerance,first,last,a,b)){if(first.X()==last.X()){u0=std::min(u0,first.X());u1=std::max(u1,first.X());}else{v0=std::min(v0,first.Y());v1=std::max(v1,first.Y());}}
          else{auto uv=analysis.ValueOfUV(p,tolerance);u0=std::min(u0,uv.X());u1=std::max(u1,uv.X());v0=std::min(v0,uv.Y());v1=std::max(v1,uv.Y());}}
        // A refused face's hint can collapse (a pole on its only edge loop's
        // iso-line); grow a collapsed axis by the parameter change the context
        // uncertainty allows or, with no usable uncertainty, by one representable
        // step each side (the smallest box that holds it), so the carrier row
        // stays a box.
        if(!(u0<u1)||!(v0<v1)){const double inf=std::numeric_limits<double>::infinity();
          if(solid_uncertainty){GeomAdaptor_Surface adaptor(surf);const double tolerance=*solid_uncertainty;
            if(!(u0<u1)){const double r=adaptor.UResolution(tolerance);u0-=r;u1+=r;}if(!(v0<v1)){const double r=adaptor.VResolution(tolerance);v0-=r;v1+=r;}}
          else{if(!(u0<u1)){u0=std::nextafter(u0,-inf);u1=std::nextafter(u1,inf);}if(!(v0<v1)){v0=std::nextafter(v0,-inf);v1=std::nextafter(v1,inf);}}}
        // The face's extent: its carrier over its UV hint (product_extent).
        {const auto extent=face_extent(surf,u0,u1,v0,v1);solid_extents[si].Add(extent);component_extent[face_component.at(fid)].Add(extent);}
        // The cone semi-angle is the one raw angle that becomes a stored
        // carrier value: it must be the raw value in the declared unit.
        if(auto cone=Handle(StepGeom_ConicalSurface)::DownCast(rf->FaceGeometry());!cone.IsNull()){GeomAdaptor_Surface translated(surf);const double declared=cone->SemiAngle()*angle_factor;
          if(translated.GetType()!=GeomAbs_Cone||!(std::abs(translated.Cone().SemiAngle()-declared)<=1e-12)){std::ostringstream message;message<<std::setprecision(17)<<"conical carrier "<<sid<<" semi-angle differs from the declared unit: declared "<<declared<<" rad";throw std::runtime_error(message.str());}}
        if(auto found=carriers.find(sid);found!=carriers.end()){auto& c=found->second;c.u0=std::min(c.u0,u0);c.u1=std::max(c.u1,u1);c.v0=std::min(c.v0,v0);c.v1=std::max(c.v1,v1);}
        else{
          // B2 .37.28: OCCT makes a closed B-spline surface periodic
          // (StepToTopoDS_TranslateFace.cxx:556-566, ShapeCustom_Surface::
          // ConvertToPeriodic). The carrier table has no periodic form, and the
          // raw entity is authoritative: write the file's own net, as
          // StepToGeom builds it before that conversion, in the same parameters.
          // The largest distance between the two on a grid of every knot span's
          // ends and midpoints is recorded; a periodic raw knot vector
          // (schema-invalid, G1 G4) refuses.
          Handle(Geom_Surface) written=surf;
          if(auto periodic=Handle(Geom_BSplineSurface)::DownCast(surf);!periodic.IsNull()&&(periodic->IsUPeriodic()||periodic->IsVPeriodic())){
            auto raw_surface=Handle(StepGeom_BSplineSurface)::DownCast(rf->FaceGeometry());if(raw_surface.IsNull())throw std::runtime_error("periodic carrier "+std::to_string(sid)+" is not a raw B-spline surface");
            StepData_Factors factors;factors.InitializeFactors(length_factor,angle_factor>0?angle_factor:1.,1.);
            auto net=StepToGeom::MakeBSplineSurface(raw_surface,factors);if(net.IsNull())throw std::runtime_error("B-spline carrier "+std::to_string(sid)+" does not build from its raw entity");
            if(net->IsUPeriodic()||net->IsVPeriodic())throw std::runtime_error("B-spline carrier "+std::to_string(sid)+" has a periodic raw knot vector (schema-invalid; G1 G4)");
            Handle(Geom_Surface) raw_carrier=net;if(!fl.IsIdentity())raw_carrier=Handle(Geom_Surface)::DownCast(net->Transformed(fl.Transformation()));
            std::vector<double> us,vs;auto samples=[](std::vector<double>& out,const NCollection_Array1<double>& k){for(int i=k.Lower();i<k.Upper();++i){out.push_back(k(i));out.push_back(.5*(k(i)+k(i+1)));}out.push_back(k(k.Upper()));};
            samples(us,net->UKnots());samples(vs,net->VKnots());double deviation=0;for(double a:us)for(double b:vs)deviation=std::max(deviation,raw_carrier->Value(a,b).Distance(surf->Value(a,b)));
            Bytes diag;diag.u32(deviation>0?1:0);diag.u32(0);diag.text("source_carrier_raw_net");diag.ids({sid,fid});diag.text("OCCT made this closed B-spline carrier periodic; the file's own net is written (G1 G6 overridden by the raw entity); the field is the largest sampled distance between the two");
            fields(diag,{{"max_sampled_distance_mm",length_mm,deviation},{"u_periodic",boolean,periodic->IsUPeriodic()?1.:0.},{"v_periodic",boolean,periodic->IsVPeriodic()?1.:0.}});tables["DIAGS___"].push_back(diag);written=raw_carrier;++raw_nets;}
          // Review round 2, finding 4: the knots are tested on the net StepToGeom
          // builds from the raw entity itself, before any periodic conversion and
          // with no face location, so neither can make the test refuse falsely.
          if(auto raw_surface=Handle(StepGeom_BSplineSurface)::DownCast(rf->FaceGeometry());!raw_surface.IsNull()){StepData_Factors factors;factors.InitializeFactors(length_factor,angle_factor>0?angle_factor:1.,1.);
            require_raw_surface_knots(raw_surface,StepToGeom::MakeBSplineSurface(raw_surface,factors),"B-spline carrier "+std::to_string(sid));}
          carriers.emplace(sid,CarrierRow{written,u0,u1,v0,v1});carrier_order.push_back(sid);}
        IDs ls;bool face_outer_set=false,face_outer_flag=false;U face_outer_loop=0;std::size_t face_outer_row=0;IDs face_outer_coedges;
        // .37.36 review 2: a face with one vertex loop and edge loops is decided
        // from its edge loops' UV curves and the pole's iso-line, never from
        // OCCT's pole wire.
        int edge_bound_count=0,vertex_bound_count=0;for(int bi=1;bi<=rf->NbBounds();++bi)(rf->BoundsValue(bi)->Bound()->IsKind(STANDARD_TYPE(StepShape_VertexLoop))?vertex_bound_count:edge_bound_count)++;
        flux_centre=component_centre.at(face_component.at(fid));std::vector<PoleCandidate> poles;std::vector<UvUse> uv_uses;U first_vertex_loop=0,first_vertex=0;std::size_t first_vertex_row=0;bool first_vertex_set=false;
        // Pole candidates: a vertex loop's pole (its singular point), or, for a
        // face of edge loops only, every singular iso-line of its surface
        // (review round 3, finding 2: a sphere's poles, a cone's apex, a
        // degenerate boundary row), each as (axis, iso-coordinate).
        if(edge_bound_count&&vertex_bound_count==1&&solid_uncertainty)for(int bi=1;bi<=rf->NbBounds();++bi)if(auto vertex_loop=Handle(StepShape_VertexLoop)::DownCast(rf->BoundsValue(bi)->Bound());!vertex_loop.IsNull()){
          auto point=Handle(StepGeom_CartesianPoint)::DownCast(Handle(StepShape_VertexPoint)::DownCast(vertex_loop->LoopVertex())->VertexGeometry());const gp_Pnt p(point->CoordinatesValue(1)*length_factor,point->CoordinatesValue(2)*length_factor,point->CoordinatesValue(3)*length_factor);
          ShapeAnalysis_Surface analysis(surf);gp_Pnt2d first,last;double fa=0,fb=0;if(analysis.DegeneratedValues(p,*solid_uncertainty,first,last,fa,fb)){if(first.X()==last.X())poles.push_back({0,first.X()});else if(first.Y()==last.Y())poles.push_back({1,first.Y()});}}
        if(edge_bound_count&&!vertex_bound_count&&solid_uncertainty){ShapeAnalysis_Surface analysis(surf);const int n=analysis.NbSingularities(*solid_uncertainty);
          for(int k=1;k<=n;++k){double preci=0,fp=0,lp=0;gp_Pnt p3;gp_Pnt2d first,last;bool uiso=false;if(!analysis.Singularity(k,preci,p3,first,last,fp,lp,uiso))continue;
            const PoleCandidate c{uiso?0:1,uiso?first.X():first.Y()};if(std::none_of(poles.begin(),poles.end(),[&](const PoleCandidate& o){return o.axis==c.axis&&o.ref==c.ref;}))poles.push_back(c);}}
        // One VERTICES row per original vertex point: the raw coordinate in
        // its own context's unit, compared with OCCT's translated vertex.
        auto write_vertex=[&](const Handle(StepShape_Vertex)& vp){U vi=id(vp);unique_vertices.insert(vi);if(!seen_vertices.insert(vi).second)return;
          auto rv=Handle(StepShape_VertexPoint)::DownCast(vp);if(rv.IsNull())throw std::runtime_error("non-point source vertex");auto rp=Handle(StepGeom_CartesianPoint)::DownCast(rv->VertexGeometry());if(rp.IsNull()||rp->NbCoordinates()!=3)throw std::runtime_error("non-Cartesian source vertex");
          gp_Pnt original(rp->CoordinatesValue(1)*length_factor,rp->CoordinatesValue(2)*length_factor,rp->CoordinatesValue(3)*length_factor);
          // A pole OCCT dropped with its bound has no translated vertex: the raw
          // point is written with no translated tolerance (the column is OCCT's
          // vertex tolerance, so zero here); the context uncertainty, when
          // usable, is a field of its record.
          if(TransferBRep::ShapeResult(tp,vp).IsNull()){Bytes diag;diag.u32(1);diag.u32(0);diag.text("source_vertex_untranslated");diag.ids({vi,id(rp)});diag.text("OCCT did not translate this vertex (its bound was dropped); written from the raw point with no translated tolerance");if(solid_uncertainty)fields(diag,{{"context_uncertainty_mm",length_mm,*solid_uncertainty}});else fields(diag);tables["DIAGS___"].push_back(diag);
            Bytes w;w.u64(vi);w.doubles({original.X(),original.Y(),original.Z()});w.f64(0.);tables["VERTICES"].push_back(w);return;}
          auto v=TopoDS::Vertex(source_result(vp));auto p=BRep_Tool::Pnt(v);double displacement=p.Distance(original);max_vertex_displacement=std::max(max_vertex_displacement,displacement);
          if(displacement>0){Bytes diag;diag.u32(1);diag.u32(0);diag.text("translated_vertex_displacement");diag.ids({vi,id(rp)});diag.text("original vertex point differs from OCCT's translated vertex");fields(diag,{{"displacement_mm",length_mm,displacement}});tables["DIAGS___"].push_back(diag);}
          Bytes w;w.u64(vi);w.doubles({original.X(),original.Y(),original.Z()});w.f64(BRep_Tool::Tolerance(v));tables["VERTICES"].push_back(w);};
        for(int bi=1;bi<=rf->NbBounds();++bi){auto bound=rf->BoundsValue(bi);
          // A vertex loop: its pole vertex, no coedges (LOOPS vertex field).
          if(auto vertex_loop=Handle(StepShape_VertexLoop)::DownCast(bound->Bound());!vertex_loop.IsNull()){U lid=id(vertex_loop),vi=id(vertex_loop->LoopVertex());ls.push_back(lid);++nloops;++nvertex_loops;write_vertex(vertex_loop->LoopVertex());
            Bytes w;w.u64(lid);w.u64(fid);w.u32(bound->IsKind(STANDARD_TYPE(StepShape_FaceOuterBound)));w.u32(0);w.ids({});w.u64(vi);w.u32(refused_loops.count(lid)?1:0);
            if(!first_vertex_set){first_vertex_set=true;first_vertex_loop=lid;first_vertex=vi;first_vertex_row=tables["LOOPS___"].size();}
            tables["LOOPS___"].push_back(w);
            Bytes diag;diag.u32(0);diag.u32(0);diag.text("source_vertex_loop");diag.ids({fid,lid,vi});diag.text("vertex_loop bound (AP242 advanced_face WR6, WR7) carried as its pole vertex; OCCT translation: "+vertex_loop_translations.at(lid));fields(diag);tables["DIAGS___"].push_back(diag);
            // Every refused loop's reason, one record each; the LOOPS___ and
            // FACES___ status fields carry the refusal structurally.
            if(auto refusal=refused_faces.find(fid);refusal!=refused_faces.end())for(const auto& r:refusal->second)if(r.loop==lid){Bytes refused;refused.u32(1);refused.u32(0);refused.text("source_face_refused");refused.ids({fid,lid,vi});refused.text(std::string(vl::name(r.reason))+": "+r.message+"; the face is carried for its topology, marked refused, and no consumer may mesh it");fields(refused,{{"reason",code,double(static_cast<std::uint32_t>(r.reason))}});tables["DIAGS___"].push_back(refused);if(counted_refusals.insert({fid,static_cast<std::uint32_t>(r.reason)}).second)++face_refusals[r.reason];}
            continue;}
          auto loop=Handle(StepShape_EdgeLoop)::DownCast(bound->Bound());if(loop.IsNull())throw std::runtime_error("non-edge source loop");U lid=id(loop);ls.push_back(lid);++nloops;IDs cs;
          raw_wire_counts.push_back(loop->NbEdgeList());
          for(int position=1;position<=loop->NbEdgeList();++position){int ei=bound->Orientation()?position:loop->NbEdgeList()+1-position;auto oe=loop->EdgeListValue(ei);auto re=Handle(StepShape_EdgeCurve)::DownCast(oe->EdgeElement());if(re.IsNull())throw std::runtime_error("non-curve source edge");U eid=id(re),cid=id(oe);cs.push_back(cid);++ncoedges;auto e=TopoDS::Edge(source_result(re));unique_edges.insert(eid);raw_edge_uses.insert(eid);face_edges[eid]=e;
            auto start=re->EdgeStart(),end=re->EdgeEnd();U va=id(start),vb=id(end);
            for(auto vp:{start,end})write_vertex(vp);
            TopLoc_Location el;double first=0,last=0;auto ec=BRep_Tool::Curve(e,el,first,last);if(ec.IsNull())BRep_Tool::Range(e,first,last);bool deg=BRep_Tool::Degenerated(e);bool same=BRep_Tool::SameParameter(e);U curve_id=ec.IsNull()?0:id(re->EdgeGeometry());
            if(!ec.IsNull()&&!el.IsIdentity())ec=Handle(Geom_Curve)::DownCast(ec->Transformed(el.Transformation()));
            double ta=re->SameSense()?first:last,tb=re->SameSense()?last:first;
            if(va!=vb&&!ec.IsNull()){
              auto p=BRep_Tool::Pnt(TopoDS::Vertex(source_result(start)));
              if(ec->Value(ta).Distance(p)>BRep_Tool::Tolerance(e)&&ec->Value(tb).SquareDistance(p)<ec->Value(ta).SquareDistance(p))
                throw std::runtime_error("translated edge parameter mapping contradicts original SameSense at edge "+std::to_string(eid));
            }
            if(seen_edges.insert(eid).second){if(deg)++degenerate;if(!same)++not_same;
              if(curve_id&&seen_curves.insert(curve_id).second){Handle(Geom_Curve) written=ec;
                if(GeomAdaptor_Curve adaptor(ec);adaptor.GetType()==GeomAbs_BSplineCurve&&adaptor.BSpline()->IsPeriodic()){
                  auto net=raw_curve_net(re->EdgeGeometry(),length_factor,"edge "+std::to_string(eid)+" curve "+std::to_string(curve_id));
                  Handle(Geom_Curve) raw_curve=net;if(!el.IsIdentity())raw_curve=Handle(Geom_Curve)::DownCast(net->Transformed(el.Transformation()));
                  double deviation=0;for(int k=1;k<net->NbKnots();++k)for(double s:{0.,.5})deviation=std::max(deviation,raw_curve->Value(net->Knot(k)+s*(net->Knot(k+1)-net->Knot(k))).Distance(ec->Value(net->Knot(k)+s*(net->Knot(k+1)-net->Knot(k)))));
                  deviation=std::max(deviation,raw_curve->Value(net->Knot(net->NbKnots())).Distance(ec->Value(net->Knot(net->NbKnots()))));
                  Bytes diag;diag.u32(deviation>0?1:0);diag.u32(0);diag.text("source_curve_raw_net");diag.ids({curve_id,eid});diag.text("OCCT made this closed B-spline edge curve periodic; the file's own net is written (G1 G5 overridden by the raw entity); the field is the largest sampled distance between the two");
                  fields(diag,{{"max_sampled_distance_mm",length_mm,deviation}});tables["DIAGS___"].push_back(diag);written=raw_curve;++raw_curve_nets;}
                tables["CURVES__"].push_back(curve3(curve_id,written,std::min(first,last),std::max(first,last)).bytes());}
              Bytes w;w.u64(eid);w.u64(va);w.u64(vb);w.u64(curve_id);w.doubles({ta,tb});w.u32(re->SameSense());w.u32(same);w.u32(deg);w.u32(0);w.f64(BRep_Tool::Tolerance(e));tables["EDGES___"].push_back(w);
            }
            // Native edge binding follows its geometric parameter, whereas
            // original EDGE_CURVE endpoints may have SameSense=false. Compose
            // original edge-use, bound and geometric senses before selecting
            // the seam pcurve; BRep_Tool then composes the face orientation.
            bool forward=oe->Orientation()==bound->Orientation();auto pcurve_edge=actual_face_edges.at(eid);pcurve_edge.Orientation((forward==re->SameSense())?TopAbs_FORWARD:TopAbs_REVERSED);double pf,pl;auto pc=BRep_Tool::CurveOnSurface(pcurve_edge,f,pf,pl);if(pc.IsNull())throw std::runtime_error("missing recovered UV at coedge "+std::to_string(cid));U pcid=(U(1)<<63)|cid;
            // A periodic UV curve whose edge carries a file pcurve on this surface
            // is the file's own net, OCCT's StepToGeom periodization undone by
            // reading the raw entity; otherwise it is OCCT's or our construction
            // and curve2 writes it unperiodized.
            if(Geom2dAdaptor_Curve adaptor(pc);adaptor.GetType()==GeomAbs_BSplineCurve&&adaptor.BSpline()->IsPeriodic()){
              const std::string where="coedge "+std::to_string(cid)+" file pcurve";std::vector<Handle(Geom2d_BSplineCurve)> candidates;
              if(auto sc=Handle(StepGeom_SurfaceCurve)::DownCast(re->EdgeGeometry());!sc.IsNull())for(int k=1;k<=sc->NbAssociatedGeometry();++k){auto pg=sc->AssociatedGeometryValue(k).Pcurve();
                if(pg.IsNull()||pg->BasisSurface()!=rf->FaceGeometry()||pg->ReferenceToCurve().IsNull()||pg->ReferenceToCurve()->NbItems()!=1)continue;
                if(auto net=raw_pcurve_net(Handle(StepGeom_Curve)::DownCast(pg->ReferenceToCurve()->ItemsValue(1)),where);!net.IsNull())candidates.push_back(net);}
              if(!candidates.empty()){std::vector<std::pair<Handle(Geom2d_BSplineCurve),double>> matching;
                for(const auto& net:candidates){double worst=0;bool inside=net->FirstParameter()<=std::min(pf,pl)&&net->LastParameter()>=std::max(pf,pl);
                  for(int k=0;k<=16&&inside;++k){const double s=k==16?pl:pf+(pl-pf)*k/16;const auto a=net->Value(s),b=pc->Value(s);worst=std::max(worst,a.Distance(b)/std::max({1.,std::abs(b.X()),std::abs(b.Y())}));}
                  if(inside&&worst<=1e-9)matching.push_back({net,worst});}
                if(matching.size()!=1)throw std::runtime_error(where+": OCCT made it periodic and "+(matching.empty()?std::string("no raw pcurve on this surface matches it in the edge's parameter (reparameterised, converted or in another unit)"):std::string("several raw pcurves on this surface match it"))+"; refused");
                Bytes diag;diag.u32(0);diag.u32(0);diag.text("source_pcurve_raw_net");diag.ids({cid,eid,fid});diag.text("OCCT made this closed file pcurve periodic; the file's own 2D net is written (G1 G5 overridden by the raw entity); the field is the largest sampled relative difference between the two");fields(diag,{{"max_sampled_relative_difference",ratio,matching[0].second}});tables["DIAGS___"].push_back(diag);
                pc=matching[0].first;++raw_pcurve_nets;}}
            auto pcd=curve2(pcid,pc,pf,pl);max_pcurve_degree=std::max(max_pcurve_degree,pcd.degree);tables["CURVES__"].push_back(pcd.bytes());
            // SameParameter=false: publish a measured t->s map or refuse; never an assumed one.
            brepgpu::SourceParameterMap measured;measured.kind=1;
            if(!same){try{measured=brepgpu::measure_source_parameter_map(pcurve_edge,f,ta,tb,uv_options);}catch(const std::exception& e){throw std::runtime_error("coedge "+std::to_string(cid)+" edge "+std::to_string(eid)+": "+e.what());}
              Bytes diag;diag.u32(1);diag.u32(0);diag.text("measured_parameter_map");diag.ids({cid,eid});diag.text("measured map (kind four): edge t to pcurve s by projecting curve-on-surface samples onto the 3D curve (Approx_SameParameter method); residual sampled at nodes and quarter points, not a bound between samples");fields(diag,{{"kind",code,4.},{"rows",count,double(measured.rows.size())},{"max_sampled_residual_mm",length_mm,measured.max_residual_mm},{"bound_mm",length_mm,uv_options.source_residual_mm}});tables["DIAGS___"].push_back(diag);}
            Bytes w;w.u64(cid);w.u64(eid);w.u64(lid);w.u64(pcid);w.i32(forward?1:-1);w.u32(1);w.doubles({pf,pl});w.i32(0);w.i32(0);w.u32(measured.kind);w.u32(0);
            // Kind 4 rows are (t, s, sampled residual mm); kinds 1-3 rows are (t, s).
            D map;for(const auto& row:measured.rows){map.insert(map.end(),{row[0],row[1]});if(measured.kind==4)map.push_back(row[2]);}w.vec(map);tables["COEDGES_"].push_back(w);
            edge_uses[{U(shell.ordinal),eid}].push_back({fid,forward?1:-1});
            uv_uses.push_back({lid,pc,pf,pl,forward==re->SameSense()});
          }
          Bytes w;w.u64(lid);w.u64(fid);w.u32(bound->IsKind(STANDARD_TYPE(StepShape_FaceOuterBound)));w.u32(0);w.ids(cs);w.u64(0);w.u32(0);
          // The face's outer bound: the designated one, else its first edge loop.
          if(!face_outer_set||(bound->IsKind(STANDARD_TYPE(StepShape_FaceOuterBound))&&!face_outer_flag)){face_outer_set=true;face_outer_flag=bound->IsKind(STANDARD_TYPE(StepShape_FaceOuterBound));face_outer_loop=lid;face_outer_row=tables["LOOPS___"].size();face_outer_coedges=cs;}
          tables["LOOPS___"].push_back(w);
        }
        std::vector<unsigned> translated_wire_counts;std::multiset<U> translated_edge_uses;
        for(TopExp_Explorer wires(f,TopAbs_WIRE);wires.More();wires.Next()){
          if(std::any_of(face_pole_wires.begin(),face_pole_wires.end(),[&](const TopoDS_Wire& pole){return pole.IsSame(wires.Current());}))continue;
          auto wire=TopoDS::Wire(wires.Current());unsigned raw_count=0,actual_count=0,ordered_count=0;
          for(TopExp_Explorer edges(wire,TopAbs_EDGE);edges.More();edges.Next()){
            ++actual_count;U match=match_actual_edge(TopoDS::Edge(edges.Current()),false);
            if(match){++raw_count;translated_edge_uses.insert(match);}
          }
          for(BRepTools_WireExplorer ordered(wire,f);ordered.More();ordered.Next())++ordered_count;
          if(actual_count!=ordered_count){std::cerr<<"wire_uv_failure face "<<fid<<" face_orientation "<<f.Orientation()<<" actual "<<actual_count<<" ordered "<<ordered_count<<"\n";for(auto& item:actual_face_edges){auto edge=item.second;for(auto orientation:{TopAbs_FORWARD,TopAbs_REVERSED}){edge.Orientation(orientation);double a,b;auto pc=BRep_Tool::CurveOnSurface(edge,f,a,b);if(!pc.IsNull()){auto p=pc->Value(a),q=pc->Value(b);std::cerr<<std::setprecision(17)<<"wire_uv_edge "<<item.first<<" orientation "<<orientation<<" t "<<a<<","<<b<<" UV "<<p.X()<<","<<p.Y()<<" "<<q.X()<<","<<q.Y()<<"\n";}}}throw std::runtime_error("translated wire traversal omitted occurrences at face "+std::to_string(fid));}translated_wire_counts.push_back(raw_count);
        }
        std::sort(raw_wire_counts.begin(),raw_wire_counts.end());std::sort(translated_wire_counts.begin(),translated_wire_counts.end());
        if(raw_wire_counts!=translated_wire_counts||raw_edge_uses!=translated_edge_uses)throw std::runtime_error("source-to-translated topology changed at face "+std::to_string(fid));
        // Face use in the solid: original same_sense composed with its shell.
        // Within one part a plain face style is kept over the part's own style (G9 5.2
        // call, a recorded deviation from Model Styling 4.2.1); occurrence and
        // assembly-node styles beat both in the consumers.
        const U face_style=style_resolver.item_style(fid),part_style=part_styles[solid_products.at(si)];
        if(face_style&&part_style&&face_style!=part_style){Bytes diag;diag.u32(0);diag.u32(0);diag.text("source_style_precedence");diag.ids({fid,face_style,part_style});diag.text("plain face style kept over the part style within the part (G9 call, a deviation: CAx-IF Model Styling lists the solid before its faces)");fields(diag);tables["DIAGS___"].push_back(diag);}
        // Status: 0 admitted, 1 refused (a refused loop; D-partial-output).
        const bool refused_face=refused_faces.count(fid)!=0;if(refused_face)++refused_face_count,solid_refused=true;
        PendingFace pending;pending.fid=fid;pending.carrier=sid;pending.style=face_style;pending.ordinal=shell.ordinal;pending.element=shell.element;pending.raw=rf->SameSense()==shell.forward?1:-1;pending.tolerance=BRep_Tool::Tolerance(f);pending.loops=ls;
        pending.refused=pending.prior_refused=refused_face;pending.same_sense=rf->SameSense();pending.shell_forward=shell.forward;
        if(face_outer_set){pending.refusal_loop=face_outer_loop;pending.refusal_row=face_outer_row;pending.refusal_row_set=true;}else if(first_vertex_set){pending.refusal_loop=first_vertex_loop;pending.refusal_vertex=first_vertex;pending.refusal_row=first_vertex_row;pending.refusal_row_set=true;}
        const int shell_sign=shell.forward?1:-1,same_sign=rf->SameSense()?1:-1;
        // The face as its raw use orients it in the solid (bounding box below).
        pending.oriented=shell.forward?TopoDS_Shape(f):f.Reversed();
        if(!refused_face){
          // Review round 3, findings 2 and 5. The pole whose region this face is:
          // with a vertex loop, its pole, whose loops must wind it exactly once
          // (holes, which do not wind, allowed); with edge loops only, a
          // singular iso-line around which exactly one loop winds. The region
          // then lies between that loop and the pole less the holes, and its
          // loops' UV sense about it (Green's area from the pole's iso-line)
          // gives the bound's side. Where the surface has two such poles (a
          // sphere's), the two regions are complements with opposite senses:
          // the face is the one its same_sense and bound read together (its
          // region lies to the left of its bound about the face normal), so it
          // never contradicts; with one (a cone's apex) the other side is
          // unbounded, so the region is the pole's and same_sense may
          // contradict it (S1). OCCT's classifier and volume do not see a pole
          // without a vertex loop (its volume is 0 for the hemisphere).
          std::optional<std::size_t> pole_index;GreenSums pole_green;std::string pole_why;bool closed_region=false;
          if(edge_bound_count&&vertex_bound_count==1){
            if(poles.size()==1){switch(pole_winding(uv_uses,surf,poles[0].axis)){
              case Winding::one:pole_green=green_of(uv_uses,surf,poles[0]);if(std::abs(pole_green.area)>0)pole_index=0;else pole_why="its loops enclose no area about its pole";break;
              case Winding::none:pole_why="its edge loop does not wind once around the pole along the pole's iso-line, so the region between them is not determined";break;
              case Winding::several:pole_why="several of its edge loops wind around the pole, so the region between them and the pole is not determined";break;
              case Winding::irregular:pole_why="an edge loop of it changes the pole's free coordinate by neither one period nor zero: it does not wind once around the pole and is no hole, so the region between them and the pole is not determined";break;}}
            else pole_why="its pole is no singular point of the surface";}
          else if(edge_bound_count&&!vertex_bound_count){std::vector<std::size_t> wound;for(std::size_t k=0;k<poles.size();++k)if(pole_winding(uv_uses,surf,poles[k].axis)==Winding::one)wound.push_back(k);
            if(!wound.empty()){std::vector<std::pair<std::size_t,GreenSums>> agreeing;std::vector<GreenSums> sums;for(auto k:wound){sums.push_back(green_of(uv_uses,surf,poles[k]));const auto& g=sums.back();if(std::abs(g.area)>0&&(g.area>0?1:-1)==same_sign)agreeing.push_back({k,g});}
              if(agreeing.size()==1){pole_index=agreeing[0].first;pole_green=agreeing[0].second;pending.two_regions=wound.size()>1;
                // Review round 5: the other pole's region, with the other side.
                if(wound.size()==2){const std::size_t other=wound[0]==*pole_index?1:0;const auto& g=sums[other];
                  pending.region=pole_region(uv_uses,surf,poles[*pole_index]);pending.alt_region=pole_region(uv_uses,surf,poles[wound[other]]);pending.raw_inside=pending.alt_inside=true;pending.region_test=true;pending.region_surface=surf;
                  pending.alt_volume=-same_sign*shell_sign*(g.area>0?1.:-1.)*g.flux/3.;
                  pending.region_resolution=solid_uncertainty?*solid_uncertainty:1./4294967296.;pending.blind_target_mm=std::max(10*pending.region_resolution,.25*smaller_principal_radius(surf,u0,u1,v0,v1));}}
              else if(agreeing.empty()&&wound.size()==1&&std::abs(sums[0].area)>0){pole_index=wound[0];pole_green=sums[0];}
              else pole_why="its winding edge loop does not determine which of its surface's poles its region contains";}
            else if(closed_surface(surf,poles)){
              // Review round 4, item 2: a face on a closed surface whose loops
              // wind nothing; its same_sense chooses the region (face_regions.hpp).
              const auto region=two_regions(uv_uses,surf,same_sign,poles);
              if(!region.decided)pole_why="region-ambiguous: "+region.why;
              else if(region.unique){
                // A band whose complement is not one region: like a pole face,
                // the band is the region, and same_sense may contradict its loops.
                closed_region=true;pending.kind=FaceKind::closed;pending.area=region.signed_area;const int loop_sense=region.signed_area>0?1:-1;pending.contradicts=loop_sense!=same_sign;pending.bound=loop_sense*shell_sign;
                pending.volume=(pending.bound==pending.raw?1.:-1.)*shell_sign*region.signed_flux/3.;}
              else{closed_region=true;pending.kind=FaceKind::closed;pending.area=region.area;pending.contradicts=false;pending.bound=pending.raw;pending.two_regions=true;pending.region=region;pending.alt_region=region;pending.raw_inside=region.inside;pending.alt_inside=!region.inside;pending.region_test=!region.inside||region.band;pending.region_surface=surf;
                // Review round 5: the other reading, its other region with the other side.
                pending.alt_volume=-same_sign*shell_sign*(region.inside?region.complement_flux:region.inside_flux)/3.;
                pending.region_resolution=solid_uncertainty?*solid_uncertainty:1./4294967296.;pending.blind_target_mm=std::max(10*pending.region_resolution,.25*smaller_principal_radius(surf,u0,u1,v0,v1));
                pending.volume=same_sign*shell_sign*region.flux/3.;
                if(!region.inside){surf->Bounds(u0,u1,v0,v1);auto& c=carriers.at(sid);c.u0=std::min(c.u0,u0);c.u1=std::max(c.u1,u1);c.v0=std::min(c.v0,v0);c.v1=std::max(c.v1,v1);const auto extent=face_extent(surf,u0,u1,v0,v1);solid_extents[si].Add(extent);component_extent[face_component.at(fid)].Add(extent);}}}}
          if(pole_index){
            // Pole face: the region lies between its edge loops and the pole;
            // its loops run counter-clockwise in UV about it when the area is
            // positive, so their side is the surface normal's then. The flux
            // share with the face oriented by its loops is flux / 3 whatever
            // same_sense says (the loops fix both the region and the side).
            pending.kind=FaceKind::pole;pending.area=pole_green.area;const int loop_sense=pole_green.area>0?1:-1;pending.contradicts=loop_sense!=same_sign;pending.bound=loop_sense*shell_sign;
            pending.volume=(pending.bound==pending.raw?1.:-1.)*shell_sign*pole_green.flux/3.;
            // Review round 3, finding 3: the UV hint (and the face's extent)
            // reaches the pole, which the region contains.
            const auto& pole=poles[*pole_index];if(pole.axis==1){v0=std::min(v0,pole.ref);v1=std::max(v1,pole.ref);}else{u0=std::min(u0,pole.ref);u1=std::max(u1,pole.ref);}
            {auto& c=carriers.at(sid);c.u0=std::min(c.u0,u0);c.u1=std::max(c.u1,u1);c.v0=std::min(c.v0,v0);c.v1=std::max(c.v1,v1);}const auto extent=face_extent(surf,u0,u1,v0,v1);solid_extents[si].Add(extent);component_extent[face_component.at(fid)].Add(extent);}
          else if(!pole_why.empty())pending.why=pole_why;
          else if(closed_region){}
          else if(edge_bound_count&&!vertex_bound_count){
            // Edge face: the CCW rule by OCCT's classifier on the face as
            // translated (oriented from same_sense alone), its volume share by
            // OCCT's Gauss integration with its pcurves, about its component's
            // box centre.
            if(f.Orientation()!=(rf->SameSense()?TopAbs_FORWARD:TopAbs_REVERSED))throw std::runtime_error("face "+std::to_string(fid)+": translated orientation is not its same_sense");
            BRepTopAdaptor_FClass2d classifier(f,Precision::PConfusion());pending.kind=FaceKind::edge;pending.contradicts=classifier.PerformInfinitePoint()==TopAbs_IN;pending.bound=pending.contradicts?-pending.raw:pending.raw;
            BRepGProp_Face integrand(TopoDS::Face(pending.oriented));BRepGProp_Vinert share(integrand,component_centre.at(face_component.at(fid)));pending.volume=share.Mass();}
          else if(!edge_bound_count&&vertex_bound_count){
            // Natural face: no bound to test; its side is the shell's (below),
            // its volume share by owned quadrature over the natural domain.
            const double flux=natural_flux(surf);if(std::isfinite(flux)){pending.kind=FaceKind::natural;pending.bound=pending.raw;pending.volume=pending.raw*flux/3.;}else pending.why="its surface has no finite natural domain";}
          else pending.why=vertex_bound_count>1?"it has edge loops and several vertex loops":"its pole is no singular point of the surface";}
        pending.hint={u0,u1,v0,v1};pending.surface=surf;pending_faces.push_back(pending);
      }
      {// Per shell (B2 .37.36 and its review). Every face's side is decided
       // or the face refused by name; a correction is written only when the
       // shell's signed volume, computed on the uses as written, has the sign
       // its role requires (outer shell positive, void negative, ABSR WR4).
       auto refuse_face=[&](PendingFace& face,const std::string& why,std::vector<Field> evidence){
         if(face.refused)return;face.refused=true;++refused_face_count;solid_refused=true;if(counted_refusals.insert({face.fid,static_cast<std::uint32_t>(vl::Reason::orientation_undecided)}).second)++face_refusals[vl::Reason::orientation_undecided];
         if(!face.refusal_row_set)throw std::runtime_error("face "+std::to_string(face.fid)+": refused for its orientation but has no loop to carry the refusal");
         auto& row=tables["LOOPS___"].at(face.refusal_row).v;row[row.size()-4]=1;row[row.size()-3]=0;row[row.size()-2]=0;row[row.size()-1]=0;
         Bytes refused;refused.u32(1);refused.u32(0);refused.text("source_face_refused");IDs ids{face.fid,face.refusal_loop};if(face.refusal_vertex)ids.push_back(face.refusal_vertex);refused.ids(ids);
         refused.text(std::string(vl::name(vl::Reason::orientation_undecided))+": "+why+"; the face is carried for its topology, marked refused, and no consumer may mesh it");
         evidence.push_back({"reason",code,double(static_cast<std::uint32_t>(vl::Reason::orientation_undecided))});fields(refused,evidence);tables["DIAGS___"].push_back(refused);};
       // Review round 3, finding 1: decided per edge-connected component.
       std::map<U,std::vector<PendingFace*>> by_component;for(auto& face:pending_faces)by_component[face_component.at(face.fid)].push_back(&face);
       std::map<U,int> written;
       for(auto& [component,faces]:by_component){const U ordinal=component_shell.at(component);
         // The closure proposition of a CLOSED_SHELL (outer or void): every edge
         // is used exactly twice, once in each direction. The failures by kind.
         U used_once=0,used_more=0,used_same_way=0;for(const auto& entry:edge_uses){if(entry.first.first!=ordinal||face_component.at(entry.second.front().first)!=component)continue;const auto& u=entry.second;
           if(u.size()==1)++used_once;else if(u.size()>2)++used_more;else if(u.size()==2&&u[0].second!=-u[1].second)++used_same_way;}
         const bool consistent=used_once==0&&used_more==0&&used_same_way==0;
         U contradicting=0,undecided=0,prior=0;for(auto* face:faces){if(face->prior_refused)++prior;else if(face->kind==FaceKind::undecided)++undecided;else if(face->contradicts)++contradicting;}
         const double scale=component_box_volume.at(component);
         auto volume=[&](auto sense){double v=0;for(auto* face:faces)v+=face->volume*(sense(*face)==face->raw?1.:-1.);return v;};
         // A closed shell (every edge used exactly twice) whose edges are each
         // used once in each direction is the only one the volume speaks for.
         auto right=[&](double v){return std::abs(v)>1e-9*scale&&(ordinal==0?v>0:v<0);};
         const double bound_volume=volume([](const PendingFace& f){return f.bound;}),raw_volume=volume([](const PendingFace& f){return f.raw;});
         std::vector<Field> shell_evidence{{"faces",count,double(faces.size())},{"faces_contradicting_bound",count,double(contradicting)},{"faces_undecided",count,double(undecided)},{"faces_refused_before",count,double(prior)},
           {"shell_edges_used_once_each_way",boolean,consistent?1.:0.},{"edges_used_once",count,double(used_once)},{"edges_used_more_than_twice",count,double(used_more)},{"edges_used_twice_same_direction",count,double(used_same_way)},{"bound_signed_volume_mm3",parameter,bound_volume},{"raw_signed_volume_mm3",parameter,raw_volume},{"shell_box_volume_mm3",parameter,scale},{"component_first_face",code,double(component)},{"shell_components",count,double(shell_components.at(ordinal).size())}};
         auto shell_record=[&](const std::string& message){Bytes diag;diag.u32(1);diag.u32(0);diag.text("source_shell_orientation_refused");diag.ids({si,faces.front()->element});diag.text(message);fields(diag,shell_evidence);tables["DIAGS___"].push_back(diag);};
         auto face_evidence=[&](const PendingFace& face,int w){std::vector<Field> e{{"raw_same_sense",boolean,face.same_sense?1.:0.},{"shell_forward",boolean,face.shell_forward?1.:0.},{"raw_sense",code,double(face.raw)},{"written_sense",code,double(w)},{"bound_sense",code,double(face.bound)},{"face_kind",code,double(static_cast<int>(face.kind))}};e.insert(e.end(),shell_evidence.begin(),shell_evidence.end());return e;};
         auto refuse_shell=[&](const std::string& message,const std::string& why){shell_record(message);for(auto* face:faces){written[face->fid]=face->raw;refuse_face(*face,why,face_evidence(*face,face->raw));}};
         if(!consistent){
           // Coordinator ruling on review round 2, finding 2: a CLOSED_SHELL
           // that fails the closure proposition violates the schema and has no
           // material side to decide, so no face of it is admitted, not even one
           // whose bound agrees with same_sense (an open shell entirely inside
           // out would satisfy every face-local test).
           ++open_shells;
           refuse_shell("this closed shell fails the closure proposition (every edge used exactly twice, once in each direction), so it has no material side to decide; every face of the shell is refused","its closed shell is not closed with each edge used once in each direction, so it has no material side (source_shell_orientation_refused)");}
         else if(const auto nest=std::find_if(shell_components.at(ordinal).begin(),shell_components.at(ordinal).end(),[&](U other){if(other==component)return false;const auto box=component_extent.find(other);
                   // No point or no extent: nesting cannot be excluded.
                   if(box==component_extent.end()||box->second.IsVoid()||!component_point.count(component))return true;const auto& p=component_point.at(component);return !box->second.IsOut(p);});nest!=shell_components.at(ordinal).end())
           // Review round 3, finding 1, and round 4, item 1: a component nested
           // in another may be a cavity written into the same shell, whose sign
           // is then the opposite of its shell's role, or an island; its role
           // does not fix the sign, so every face of it is refused. A nested
           // component lies inside the other's region, so every point of it
           // lies within any box that holds the other's faces: a point of it
           // within the other's extent is the test, and it misses no nesting.
           refuse_shell("a point of this component lies within another component's box (the box holding that component's faces), so it may be nested in it and the sign its enclosed region requires is not determined by the shell's role; every face of the component is refused","its shell component lies within another component of the same shell, so its side is not verified (source_shell_orientation_refused)");
         else if(prior||undecided)
           // Review round 2, finding 1: without every face's share the volume
           // verifies nothing, and a bound agreeing with same_sense does not
           // show the shell is outward; every face of the shell is refused.
           {shell_record("a face of this closed shell is refused or its side undecided, so the shell's signed volume is no evidence for any face; every face of the shell is refused");
            for(auto* face:faces){written[face->fid]=face->raw;refuse_face(*face,face->kind==FaceKind::undecided&&!face->prior_refused?"its side cannot be decided ("+face->why+"), and its closed shell's side is then not verified (source_shell_orientation_refused)":"its closed shell has a refused or undecided face, so its side is not verified (source_shell_orientation_refused)",face_evidence(*face,face->raw));}}
         else if(std::any_of(faces.begin(),faces.end(),[](const PendingFace* f){return f->two_regions;})&&!(contradicting==0&&right(raw_volume)))
           // Review round 3, finding 2: on a surface with two poles (a sphere's)
           // a face's same_sense chooses its region as well as its side, so
           // turning that face alone reads the complementary region. When the
           // raw uses are not verified, that reading and turning every face (S2)
           // can both enclose the region with the required sign; nothing
           // decides between them, and every face of the component is refused.
           refuse_shell("a face of this component lies on a surface with two poles, where its same_sense chooses its region as well as its side, and the raw uses are not verified; correcting that face alone and turning every face are both readings, so the orientation is not determined; every face of the component is refused","its shell component's raw uses are not verified and a face of it on a two-pole surface makes the correction ambiguous (source_shell_orientation_refused)");
         else if(contradicting==0&&right(raw_volume)&&std::any_of(faces.begin(),faces.end(),[](const PendingFace* f){return f->two_regions&&f->region_test;})){
           // Review round 5: a face whose raw reading is the complement of its
           // loops on a closed surface (or either region of a band, where which
           // is inside is a matter of the chart), when the other reading also
           // verifies, is tested for meeting another face of its part away from
           // its own boundary (region_meets). A raw reading that meets none
           // stands (G1). One that meets one is not a solid's boundary: the
           // other reading is written (S1) if it meets none and the part's
           // volume on it verifies; otherwise the part is refused by name.
           // Review round 8, ruling 1: the tangent-edge fold test decides first
           // (tangent_fold): at a tangent common edge exactly one reading is
           // fold-free, and it is the face. The contact test runs only where no
           // common edge is tangent; it corrects only on a certain meet of the
           // raw reading against a clean other reading (findings 3 and 4).
           double volume=raw_volume;std::vector<PendingFace*> corrected;std::string failure;
           for(auto* c:faces){if(!(c->two_regions&&c->region_test))continue;const double alt=volume-c->volume+c->alt_volume;
             std::vector<std::pair<TopoDS_Face,int>> neighbours;for(auto* o:faces)if(o!=c)neighbours.push_back({TopoDS::Face(o->oriented),o->raw});
             c->fold=tangent_fold(TopoDS::Face(c->oriented),c->region_surface,c->raw,neighbours,c->region_resolution,1e-2);
             if(c->fold.tangent_edges){c->decided_by=1;
               if(c->fold.mixed||(c->fold.raw_smooth&&c->fold.raw_folds)){failure="a face on a closed surface folds back over a tangent neighbour in both readings of its region (at different common edges, or along one), so neither bounds a solid";break;}
               if(c->fold.raw_smooth)continue;
               if(!right(alt)){failure="a face on a closed surface folds back over a tangent neighbour in its raw reading, and its other reading does not enclose the region with the sign its role requires";break;}
               corrected.push_back(c);c->corrected_region=true;volume=alt;continue;}
             if(!right(alt))continue;++region_tests;c->decided_by=2;
             std::vector<brepgpu::face_contact::Neighbour> others;for(auto* o:faces)if(o!=c)others.push_back({TopoDS::Face(o->oriented),o->kind==FaceKind::pole&&!o->surface.IsNull()?brepgpu::face_contact::pole_proxy(o->surface,o->hint[0],o->hint[1],o->hint[2],o->hint[3]):TopoDS_Face()});
             const auto raw_contact=region_meets(c->region_surface,c->region,c->raw_inside,TopoDS::Face(c->oriented),others,c->region_resolution,c->blind_target_mm);const int raw_meets=raw_contact.result;c->test_ran=true;c->raw_meets=raw_meets;c->crossings_only=std::max(c->crossings_only,raw_contact.crossings_only);c->blind_strip_mm=std::max(c->blind_strip_mm,raw_contact.strip_mm);c->blind_cell_mm=std::max(c->blind_cell_mm,raw_contact.cell_mm);
             if(raw_meets<0){failure="the test whether a face's reading meets another face of its part did not complete";break;}
             if(raw_meets==0)continue;
             if(raw_meets==2){failure="a face's raw reading meets another face of its part only just outside a common boundary's contact strip, where the contact model is not sure, so no correction is grounded on it";break;}
             if(raw_meets==3){failure="the test whether a face's raw reading meets another face of its part left cells beside a common boundary larger than its blind-band target untested, so its outcome is undecided";break;}
             const auto alt_contact=region_meets(c->region_surface,c->alt_region,c->alt_inside,TopoDS::Face(c->oriented),others,c->region_resolution,c->blind_target_mm);const int alt_meets=alt_contact.result;c->alt_meets=alt_meets;c->crossings_only=std::max(c->crossings_only,alt_contact.crossings_only);c->blind_strip_mm=std::max(c->blind_strip_mm,alt_contact.strip_mm);c->blind_cell_mm=std::max(c->blind_cell_mm,alt_contact.cell_mm);
             if(alt_meets!=0){failure="a face on a closed surface meets another face of its part in its raw reading, and its other reading is not shown to meet none";break;}
             corrected.push_back(c);c->corrected_region=true;volume=alt;}
           if(failure.empty()&&!corrected.empty()&&!right(volume))failure="the readings of its faces on closed surfaces that meet no other face do not enclose the region with the sign its role requires";
           if(!failure.empty())refuse_shell(failure+"; every face of the component is refused","its shell component's faces on closed surfaces are not verified to bound a solid (source_shell_orientation_refused)");
           else for(auto* face:faces){const bool turn=std::find(corrected.begin(),corrected.end(),face)!=corrected.end();written[face->fid]=turn?-face->raw:face->raw;
             if(turn){++faces_orientation_raw;
               {// The corrected reading's region: its hint is the natural domain.
                 double a0,a1,b0,b1;face->region_surface->Bounds(a0,a1,b0,b1);auto& cr=carriers.at(face->carrier);cr.u0=std::min(cr.u0,a0);cr.u1=std::max(cr.u1,a1);cr.v0=std::min(cr.v0,b0);cr.v1=std::max(cr.v1,b1);solid_extents[si].Add(face_extent(face->region_surface,a0,a1,b0,b1));}
               Bytes diag;diag.u32(1);diag.u32(0);diag.text("source_face_orientation_raw");diag.ids({si,face->fid});diag.text(face->decided_by==1?"the face lies on a closed surface, where its same_sense chooses its region as well as its side; its raw reading folds back over a neighbour it is tangent to at a common edge (opposite outward normals: a zero dihedral angle), so it bounds no solid, and the other reading is fold-free there and the part's signed volume on it has the sign its role requires (raw-entity correction S1, review round 8 tangent fold)":"the face lies on a closed surface, where its same_sense chooses its region as well as its side; the region its raw use reads meets another face of its part away from its boundary, so it bounds no solid, and the other region with the other side meets none and the part's signed volume on it has the sign its role requires (raw-entity correction S1, review round 5)");
               auto evidence=face_evidence(*face,-face->raw);evidence.push_back({"raw_region_inside_loops",boolean,face->raw_inside?1.:0.});evidence.push_back({"raw_signed_volume_mm3_component",parameter,raw_volume});evidence.push_back({"written_signed_volume_mm3_component",parameter,volume});fields(diag,evidence);tables["DIAGS___"].push_back(diag);}}}
         else if(right(bound_volume)){
           // Every face takes the side its own bound gives (S1 where that is not
           // its raw use), verified by the volume of exactly those uses.
           for(auto* face:faces){written[face->fid]=face->bound;if(face->bound!=face->raw){++faces_orientation_raw;Bytes diag;diag.u32(1);diag.u32(0);diag.text("source_face_orientation_raw");diag.ids({si,face->fid});diag.text("same_sense contradicts the face's own bound (counter-clockwise about the face normal) in a closed shell whose edges are each used once in each direction; the side its bound gives is written, and the shell's signed volume on the written uses has the sign its role requires (raw-entity correction S1)");fields(diag,face_evidence(*face,face->bound));tables["DIAGS___"].push_back(diag);}}}
         else if(!contradicting&&right(-raw_volume)){
           // No face contradicts its bound but the whole shell encloses its
           // region inside out: every side is turned (S2), verified the same way.
           for(auto* face:faces){written[face->fid]=-face->raw;++faces_orientation_raw;Bytes diag;diag.u32(1);diag.u32(0);diag.text("source_face_orientation_raw");diag.ids({si,face->fid});diag.text("the face's closed shell is inside out as a whole: no face contradicts its own bound and every edge is used once in each direction, but the raw uses enclose the shell's region the wrong way (outer shell negative, void positive); every face's opposite side is written, and the shell's signed volume on the written uses has the sign its role requires (raw-entity correction S2)");fields(diag,face_evidence(*face,-face->raw));tables["DIAGS___"].push_back(diag);}}
         else
           refuse_shell("no reading of this closed shell is verified: the sides its faces' bounds give, and their raw uses, do not enclose its region with the sign its role requires; every face of the shell is refused","its closed shell's orientation is not verified (source_shell_orientation_refused)");
         }
       // connected_face_set's informal proposition (the faces are arcwise
       // connected) does not hold for a shell of several components: recorded,
       // not refused, since each component was verified on its own.
       for(const auto& shell:shells)if(const auto found=shell_components.find(shell.ordinal);found!=shell_components.end()&&found->second.size()>1){const auto& parts=found->second;Bytes diag;diag.u32(1);diag.u32(0);diag.text("source_shell_components");IDs ids{si,shell.element};ids.insert(ids.end(),parts.begin(),parts.end());diag.ids(ids);
         diag.text("the closed shell's faces form several edge-connected components (connected_face_set's informal proposition: its faces are arcwise connected); each component is decided and its volume verified on its own; the ids after the shell are each component's first face");fields(diag,{{"components",count,double(parts.size())}});tables["DIAGS___"].push_back(diag);}
       // Review round 6, ruling 2: every written region that its loops alone do
       // not fix (the complement of its loops, a side of a band, one of two
       // poles' sides) has a typed record, with whether the test ran and what it
       // found; never silent.
       for(const auto& face:pending_faces)if(face.two_regions&&face.region_test&&!face.refused){const bool pole_side=face.kind==FaceKind::pole;const bool inside_now=face.corrected_region?face.alt_inside:face.raw_inside;
         Bytes diag;diag.u32(0);diag.u32(0);diag.text("source_face_region_choice");diag.ids({si,face.fid});
         diag.text(std::string("the face lies on a surface where its same_sense chooses its region as well as its side; the written region is ")+(pole_side?"the side of its winding loop toward one of the surface's two poles":face.region.band?"one side of a band, which of the two lies inside the loops being a matter of the chart":inside_now?"the region inside its loops":"the complement of its loops")+(face.corrected_region?", the other reading, its raw one meeting another face of its part (S1)":", its raw reading")+(face.test_ran?"; the test whether a reading meets another face of its part ran":"; the test did not run (the other reading does not verify, or its raw region is inside its loops)")+(face.crossings_only?"; a neighbour sharing its boundary was held to crossings only, its contact strip along the common boundary having no bound (neighbours_crossings_only)":"")+(face.decided_by==1?"; decided by the tangent-fold test (decided_by 1: at its tangent common edges exactly one reading is fold-free), with no contact test":"")+(face.test_ran?"; contact with a neighbour is not seen within blind_strip_half_width_mm of their common boundary plus blind_cell_mm beside it":""));
         fields(diag,{{"region_kind",code,pole_side?3.:face.region.band?2.:1.},{"written_region_inside_loops",boolean,inside_now?1.:0.},{"raw_region_inside_loops",boolean,face.raw_inside?1.:0.},{"test_ran",boolean,face.test_ran?1.:0.},{"raw_meets",code,double(face.raw_meets)},{"other_meets",code,double(face.alt_meets)},{"written_other_reading",boolean,face.corrected_region?1.:0.},{"resolution_mm",length_mm,face.region_resolution},{"neighbours_crossings_only",count,double(face.crossings_only)},{"blind_strip_half_width_mm",length_mm,face.blind_strip_mm},{"blind_cell_mm",length_mm,face.blind_cell_mm},
           {"decided_by",code,double(face.decided_by)},{"tangent_edges",count,double(face.fold.tangent_edges)},{"raw_smooth_edges",count,double(face.fold.raw_smooth)},{"raw_fold_edges",count,double(face.fold.raw_folds)},{"tangent_min_abs_dot",ratio,face.fold.tangent_edges?face.fold.min_abs_dot:0.},{"blind_target_mm",length_mm,std::isfinite(face.blind_target_mm)?face.blind_target_mm:0.}});tables["DIAGS___"].push_back(diag);++region_choices;}
       for(const auto& face:pending_faces)if(!face.prior_refused){auto& c=orientation_counts[static_cast<int>(face.kind)];++c[face.refused?1:0];}
       for(auto& face:pending_faces){Bytes w;w.u64(face.fid);w.u64(si);w.u64(face.carrier);w.u64(face.style);w.i32(written.at(face.fid));w.u32(static_cast<std::uint32_t>(face.ordinal));w.f64(face.tolerance);w.ids(face.loops);w.u32(face.refused?1:0);tables["FACES___"].push_back(w);}}
      for(const auto& shell:shells)if(shell.ordinal){Bytes diag;diag.u32(0);diag.u32(0);diag.text("source_void_shell");diag.ids({si,shell.id,shell.element});diag.text(std::string("void shell orientation=")+(shell.forward?".T.":".F.")+"; face orientation composed with shell orientation");fields(diag,{{"ordinal",count,double(shell.ordinal)},{"faces",count,double(shell.faces->NbCfsFaces())}});tables["DIAGS___"].push_back(diag);}
      // Status: 0 complete, 1 incomplete (a refused face; D-partial-output).
      es.assign(unique_edges.begin(),unique_edges.end());vs.assign(unique_vertices.begin(),unique_vertices.end());Bytes w;w.u64(si);w.u64(solid_products.at(si));w.ids(fs);w.ids(es);w.ids(vs);w.u32(solid_refused?1:0);tables["SOLIDS__"].push_back(w);if(solid_refused)++incomplete_definitions;
    }
    // B2 .37.20: every usage placement is read raw (native/step_placements.hpp).
    // OCCT's located placement is written only where it agrees with the raw
    // one to the oracle's grade, so the reference exporter's translation-only
    // usages stay identical there; elsewhere the raw placement is written with
    // a record (G1 authority rule).
    const placements::Reader raw_placements(model);U raw_placement_count=0;
    // A product's extent in its own frame: each of its solid's faces is boxed
    // as its carrier over the face's UV hint (solid_extents, from the face as
    // translated with recovered pcurves), carried into the product's frame by
    // the solid's location there (solid_in_product, taken when the product is
    // bound: the solid is later replaced by its copy with recovered pcurves), and each child's extent is carried in by
    // that usage's raw placement. OCCT's box of the product shape is not used:
    // a face whose edges have no pcurves boxes its whole unbounded carrier.
    std::map<U,Bnd_Box> extents;std::set<U> extent_open;
    auto carried=[](const Bnd_Box& box,const auto& m,Bnd_Box& into){if(box.IsVoid())return;double x[2],y[2],z[2];box.Get(x[0],y[0],z[0],x[1],y[1],z[1]);
      for(int a=0;a<2;++a)for(int b=0;b<2;++b)for(int c=0;c<2;++c){const double q[3]={x[a],y[b],z[c]};double r[3];for(int i=0;i<3;++i)r[i]=m[4*i]*q[0]+m[4*i+1]*q[1]+m[4*i+2]*q[2]+m[4*i+3];into.Add(gp_Pnt(r[0],r[1],r[2]));}};
    std::function<const Bnd_Box&(U)> product_extent=[&](U pi)->const Bnd_Box&{if(auto found=extents.find(pi);found!=extents.end())return found->second;
      if(!extent_open.insert(pi).second)throw std::runtime_error("cyclic product structure at "+std::to_string(pi));Bnd_Box box;
      if(auto solid=product_solids.find(pi);solid!=product_solids.end())if(auto e=solid_extents.find(solid->second);e!=solid_extents.end())
        carried(e->second,transform(solid_in_product.at(solid->second)),box);
      for(auto& v:usages)if(id(v->RelatingProductDefinition())==pi)carried(product_extent(id(v->RelatedProductDefinition())),raw_placements.at(id(v)).matrix,box);
      extent_open.erase(pi);return extents.emplace(pi,box).first->second;};
    for(auto& u:usages){U ui=id(u);auto s=result(u);const auto& raw=raw_placements.at(ui);const D located=transform(s.Location().Transformation());std::array<double,12> occt{};std::copy(located.begin(),located.end(),occt.begin());
      // The one agreement predicate (native/step_placements.hpp agree: the
      // 2^-32 relative rule, the translation resolution of review 9a and the
      // rotation bound over the child's extent of review round 2 finding 3),
      // which the reference exporter calls too.
      D written=located;placements::Extent extent;{const Bnd_Box& box=product_extent(id(u->RelatedProductDefinition()));
        if(!box.IsVoid()){extent.empty=false;box.Get(extent.lo[0],extent.lo[1],extent.lo[2],extent.hi[0],extent.hi[1],extent.hi[2]);}}
      const auto agreement=placements::agree(raw.matrix,occt,placements::placement_resolution(model,raw),extent);
      if(!agreement.agrees){written.assign(raw.matrix.begin(),raw.matrix.end());++raw_placement_count;
        Bytes diag;diag.u32(1);diag.u32(0);diag.text("source_placement_raw");diag.ids({ui,raw.cdsr,raw.relation,raw.operator_id});
        diag.text(std::string("OCCT's located placement differs from the raw placement read from the ")+(raw.cartesian?"cartesian_transformation_operator_3d (ISO 10303-42 base_axis; G1 P1)":"item_defined_transformation as transform_item_2 composed with the inverse of transform_item_1 (PDM Schema Usage Guide Example 14)")+"; the raw placement is written and admission is the readers' rigid rule");
        std::vector<Field> values{{"max_relative_entry_difference",ratio,agreement.relative},{"max_translation_difference_mm",length_mm,agreement.translation_mm},{"max_point_difference_mm",length_mm,agreement.moved_mm},{"translation_resolution_mm",length_mm,agreement.resolution_mm},{"operator",code,raw.cartesian?2.:1.},{"reversed",boolean,raw.reversed?1.:0.},{"swapped",boolean,raw.swapped?1.:0.}};
        for(int k=0;k<12;++k)values.push_back({"occt_m"+std::to_string(k/4)+std::to_string(k%4),k%4==3?length_mm:parameter,occt[k]});
        fields(diag,values);tables["DIAGS___"].push_back(diag);}
      Bytes w;w.u64(ui);w.u64(id(u->RelatingProductDefinition()));w.u64(id(u->RelatedProductDefinition()));w.u64(0);w.text(str(u->Name()));w.doubles(written);tables["USAGES__"].push_back(w);}
    std::cout<<"source_placements_raw\t"<<raw_placement_count<<"\n";
    for(U sid:carrier_order){const auto& c=carriers.at(sid);tables["CARRIERS"].push_back(surface(sid,c.surface,{c.u0,c.u1,c.v0,c.v1}));}
    std::cout<<"source_carrier_raw_nets\t"<<raw_nets<<"\nsource_curve_raw_nets\t"<<raw_curve_nets<<"\nsource_pcurve_raw_nets\t"<<raw_pcurve_nets<<"\nsource_uv_curves_unperiodized\t"<<unperiodized_uv_curves<<"\nsource_domain_corrections\t"<<source_domain_corrections<<"\nsource_faces_orientation_raw\t"<<faces_orientation_raw<<"\nsource_shells_not_closed_consistent\t"<<open_shells<<"\nsource_region_intersection_tests\t"<<region_tests<<"\nsource_face_region_choices\t"<<region_choices<<"\n";
    // Faces by how their side was decided (edge, pole, natural, undecided): decided, refused here.
    for(int k=0;k<5;++k)std::cout<<"source_face_sides\t"<<(k==0?"edge":k==1?"pole":k==2?"natural":k==3?"undecided":"closed")<<'\t'<<orientation_counts[k][0]<<'\t'<<orientation_counts[k][1]<<"\n";
    // Style records (conflicts, unplaced styles) from the shared resolver.
    style_resolver.finish();
    for(const auto& note:style_resolver.notes){Bytes diag;diag.u32(note.severity);diag.u32(0);diag.text(note.stage);diag.ids(note.ids);diag.text(note.message);fields(diag);tables["DIAGS___"].push_back(diag);}
    std::cout<<"style_path_work\t"<<style_resolver.path_work()<<"\n";
    for(auto& s:styles)tables["STYLES__"].push_back(s.second.bytes());
    Bytes meta;meta.text("OCCT b8f597c677811d1f9f4d8a97f5ae2825c0353a42; owned STEP source adapter v8; explicit-uv-projector; bounded-conic-source-chart; original-t; void-shells; declared-plane-angle; measured-parameter-map; vertex-loops; face-status; context-units; occurrence-styles; typed-diagnostics; raw-placements; carrier-hint-union; natural-domain-uv; mapped-item-refusals; shape-processing=translation-only; raw-entity-corrections=P1,P2,G5,G6,S1,S2; OCCT translation without ShapeProcess (empty bitset set after read, asserted before and after transfer); OCCT translation-layer edits apply: E1-E16, G1-G22, T1-T8, U1-U4, P1-P4, R1-R3 of papers/2026-10-17-occt-ingress-translation-edits.md section 2.1; raw-entity corrections: P1 cartesian operators and P2 item-defined composition read raw, G5 and G6 closed B-spline nets written raw, S1 a face's side corrected where its same_sense contradicts its own outer bound (an edge face) or its loops about its pole (a pole face), or where a face on a closed surface, whose same_sense chooses its region as well as its side, reads a region that folds back over a tangent neighbour (tangent-fold test) or meets another face of its part (contact test), S2 a shell part whose faces agree with their bounds but enclose its region inside out written turned, in a closed shell part whose edges are each used once in each direction, each written only when the part's signed volume on the written uses has the sign its role requires");tables["META____"].push_back(meta);
    // Header: lengths are mm by construction. The uncertainty is a file-wide
    // maximum of the solids' usable governing B-rep values (0 when none makes a
    // usable claim; UR-7-unusable values are excluded), a summary only: no
    // consumer may use it as a part's value. Each solid's own value, its absence
    // or its unusable declaration is its source_uncertainty record.
    if(any_usable)std::cout<<"source_uncertainty_mm\t"<<units::shortest(source_uncertainty_mm)<<"\n";else std::cout<<(any_declared?"source_uncertainty_unusable\n":"source_uncertainty_absent\n");
    std::cout<<"source_uv_seconds\t"<<std::setprecision(17)<<source_uv_seconds<<"\nquery_uv_seconds\t"<<query_uv_seconds<<"\nsource_uv_projected_edges\t"<<source_uv_projected<<"\nsource_uv_max_residual_mm\t"<<source_uv_max_residual<<"\nsource_chart_count\t"<<source_chart_count<<"\nsource_chart_max_uv_metric_mm\t"<<source_chart_max_uv_metric<<"\nsource_uv_seam_pairs\t"<<source_uv_seams<<"\nsource_query_column_edges\t"<<query_columns<<"\nsource_query_endpoint_edits\t"<<query_endpoints<<"\nsource_query_connectors\t"<<query_connectors<<"\nsource_query_max_residual_mm\t"<<query_max_residual<<"\nsource_query_max_uv_metric_mm\t"<<query_max_uv_metric<<"\n";
    const std::vector<std::string> tags={"META____","PRODUCTS","SOLIDS__","USAGES__","VERTICES","EDGES___","COEDGES_","LOOPS___","FACES___","CARRIERS","CURVES__","STYLES__","DIAGS___","OCCSTYLE"};Bytes body;
    for(auto& tag:tags){Bytes section;for(auto& row:tables[tag])section.row(row);body.raw(tag);body.u64(tables[tag].size());body.u64(section.v.size());body.v.insert(body.v.end(),section.v.begin(),section.v.end());}
    Bytes out;out.raw("BGSRC001");out.u32(2);out.u32(0);out.v.insert(out.v.end(),digest.begin(),digest.end());out.f64(1.0);out.f64(source_uncertainty_mm);out.u64(roots[0]);out.u32(tags.size());out.u32(0);out.u64(88+body.v.size());out.v.insert(out.v.end(),body.v.begin(),body.v.end());
    std::string temporary=std::string(argv[2])+".tmp";std::ofstream output(temporary,std::ios::binary);output.write(reinterpret_cast<const char*>(out.v.data()),out.v.size());output.close();if(!output)throw std::runtime_error("source binary write failed");if(std::rename(temporary.c_str(),argv[2]))throw std::runtime_error("source binary rename failed");
    std::cout<<"refused_faces\t"<<refused_face_count<<"\nincomplete_definitions\t"<<incomplete_definitions<<"\n";
    for(const auto reason:vl::reasons)std::cout<<"face_refusal\t"<<vl::name(reason)<<'\t'<<(face_refusals.count(reason)?face_refusals.at(reason):0)<<"\n";
    std::cout<<"solids\t"<<raw_solids.size()<<"\nfaces\t"<<nfaces<<"\nloops\t"<<nloops<<"\nvertex_loops\t"<<nvertex_loops<<"\ncoedges\t"<<ncoedges<<"\nedges\t"<<seen_edges.size()<<"\nvertices\t"<<seen_vertices.size()<<"\nnon_same_parameter\t"<<not_same<<"\ndegenerate_edges\t"<<degenerate<<"\nmax_pcurve_degree\t"<<max_pcurve_degree<<"\nmax_vertex_displacement_mm\t"<<max_vertex_displacement<<"\n";
    return 0;
  }catch(const Standard_Failure& e){std::cerr<<"source extraction failed: "<<e.what()<<"\n";return 1;}
  catch(const std::exception& e){std::cerr<<"source extraction failed: "<<e.what()<<"\n";return 1;}
}
