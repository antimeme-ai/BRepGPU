// Owned source extraction into BGSRC001. OCCT translates STEP and reconstructs
// UV curves; product sampling, triangulation and rendering are not performed.
#include "source_uv.hpp"
#include "step_units.hpp"
#include "step_styles.hpp"
#include "step_vertex_loops.hpp"
#include <BinTools.hxx>
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
#include <BRepCheck_Analyzer.hxx>
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
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

CurveData curve2(U id,const Handle(Geom2d_Curve)& c,double a,double b){
  CurveData d;d.id=id;d.dimension=2;d.range={a,b};Geom2dAdaptor_Curve ac(c);
  switch(ac.GetType()){
    case GeomAbs_Line:{d.kind=1;auto l=ac.Line();auto p=l.Location();auto v=l.Direction();d.fr={p.X(),p.Y(),0,v.X(),v.Y(),0,0,0,0,0,0,0};break;}
    case GeomAbs_Circle:{d.kind=2;auto q=ac.Circle();d.fr=frame(q.Axis());d.radii={q.Radius(),q.Radius()};break;}
    case GeomAbs_Ellipse:{d.kind=3;auto q=ac.Ellipse();d.fr=frame(q.Axis());d.radii={q.MajorRadius(),q.MinorRadius()};break;}
    case GeomAbs_BezierCurve:{d.kind=4;d.range={0,1};auto q=ac.Bezier();d.degree=q->Degree();d.controls=controls2(q);break;}
    case GeomAbs_BSplineCurve:{d.kind=5;auto q=ac.BSpline();d.degree=q->Degree();knots(q,d);d.controls=controls2(q);Geom2dConvert_BSplineCurveToBezierCurve cv(q);NCollection_Array1<double> ks(1,cv.NbArcs()+1);cv.Knots(ks);for(int i=1;i<=cv.NbArcs();++i){Bytes s;s.f64(ks(i));s.f64(ks(i+1));s.u32(d.degree);s.u32(0);s.vec(controls2(cv.Arc(i)));d.spans.push_back(s);}break;}
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
    double source_uv_seconds=0,query_uv_seconds=0;U source_uv_projected=0,source_uv_seams=0,source_chart_count=0,query_columns=0,query_endpoints=0,query_connectors=0;double source_uv_max_residual=0,source_chart_max_uv_metric=0,query_max_residual=0,query_max_uv_metric=0;
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
    namespace units=brepgpu::step_units;namespace vl=brepgpu::step_vertex_loops;
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
    for(int i=1;i<=model->NbEntities();++i)if(auto s=Handle(StepShape_ManifoldSolidBrep)::DownCast(model->Value(i));!s.IsNull()){
      auto users=graph.Sharings(s);for(users.Start();users.More();users.Next())if(auto rep=Handle(StepShape_ShapeRepresentation)::DownCast(users.Value());!rep.IsNull())for(const auto& r:representations.transfer_path(rep))governed.emplace(id(r),r);}
    std::map<U,double> reported_length_factors;std::vector<Bytes> unit_records;
    for(const auto& g:governed){auto length=units::context_length(model,g.second);units::require_millimetre(model,g.second,length);
      if(reported_length_factors.emplace(length.context,length.mm).second){std::cout<<"source_length_factor\t"<<length.context<<'\t'<<units::shortest(length.mm)<<"\n";
        // UR-2: a unit name stating another factor than the entities resolve.
        if(length.named_mm&&!(std::abs(*length.named_mm-length.mm)<=1e-9*std::max(*length.named_mm,length.mm))){Bytes diag;diag.u32(1);diag.u32(0);diag.text("source_unit_name");diag.ids({length.context,length.unit});diag.text("length unit named '"+length.name+"' resolves from its entities to another factor than its name states; the entities govern");fields(diag,{{"resolved_mm_per_unit",length_mm,length.mm},{"named_mm_per_unit",length_mm,*length.named_mm}});unit_records.push_back(diag);}}}
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
      if(governing.governed){Bnd_Box box;{const auto shape=TransferBRep::ShapeResult(tp,solid);if(!shape.IsNull())BRepBndLib::Add(shape,box);}const double diagonal=box.IsVoid()?0.:std::sqrt(box.SquareExtent());
        const bool usable=units::uncertainty_usable(governing.mm,diagonal);message<<"governed by the "<<governing.rule<<(usable?"":"; below the G6 UR-7 floor relative to the definition diagonal, so it makes no usable claim");
        any_declared=true;if(usable){any_usable=true;source_uncertainty_mm=std::max(source_uncertainty_mm,governing.mm);solid_uncertainty=governing.mm;}
        diag.text(message.str());fields(diag,{{"governing_mm",length_mm,governing.mm},{"diagonal_mm",length_mm,diagonal},{"usable",boolean,usable?1.:0.}});}
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
    std::map<U,TopoDS_Shape> solids,product_shapes;std::map<U,U> product_solids,solid_products,part_styles;
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
      for(TopExp_Explorer ex(ps,TopAbs_SOLID);ex.More();ex.Next())for(auto& s:solids)if(ex.Current().IsSame(s.second))matches.insert(s.first);
      if(!parents.count(pi)){
        if(matches.size()!=1)throw std::runtime_error("leaf product does not bind one source solid "+std::to_string(pi));
        U si=*matches.begin();if(solid_products.count(si))throw std::runtime_error("solid has multiple leaf products");product_solids[pi]=si;solid_products[si]=pi;
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
    for(auto& u:usages){U ui=id(u);auto s=result(u);Bytes w;w.u64(ui);w.u64(id(u->RelatingProductDefinition()));w.u64(id(u->RelatedProductDefinition()));w.u64(0);w.text(str(u->Name()));w.doubles(transform(s.Location().Transformation()));tables["USAGES__"].push_back(w);}
    IDs roots;for(auto& p:products)if(!children.count(id(p)))roots.push_back(id(p));if(roots.size()!=1)throw std::runtime_error("ambiguous source root");
    // Per-occurrence styles (CDORSI, Model Styling 5.1): native/step_styles.hpp.
    auto face_on=[&](U product,U face_id){const auto solid=product_solids.find(product);if(solid==product_solids.end())return false;auto raw=raw_solids.at(solid->second);std::vector<Handle(StepShape_ConnectedFaceSet)> sets;
      if(auto outer=Handle(StepShape_OrientedClosedShell)::DownCast(raw->Outer());!outer.IsNull())sets.push_back(outer->ClosedShellElement());else sets.push_back(raw->Outer());
      if(auto with_voids=Handle(StepShape_BrepWithVoids)::DownCast(raw);!with_voids.IsNull())for(int k=1;k<=with_voids->NbVoids();++k)if(!with_voids->VoidsValue(k).IsNull())sets.push_back(with_voids->VoidsValue(k)->ClosedShellElement());
      for(const auto& set:sets)if(!set.IsNull())for(int k=1;k<=set->NbCfsFaces();++k)if(id(set->CfsFacesValue(k))==face_id)return true;return false;};
    for(const auto& row:style_resolver.occurrences(roots[0],face_on)){Bytes w;w.u64(row.style);w.u64(row.face);w.ids(row.path);tables["OCCSTYLE"].push_back(w);}
    std::set<U> seen_vertices,seen_edges,seen_curves,seen_surfaces;U nfaces=0,nloops=0,ncoedges=0,degenerate=0,not_same=0,nvertex_loops=0;unsigned max_pcurve_degree=0;
    std::map<U,std::string> vertex_loop_translations;struct LoopRefusal{U loop=0,vertex=0;vl::Reason reason=vl::Reason::admitted;std::string message;};
    std::map<U,std::vector<LoopRefusal>> refused_faces;std::set<U> refused_loops;U incomplete_definitions=0;
    // Faces refused, per reason (a face refused for two reasons counts in both).
    std::map<vl::Reason,U> face_refusals;std::set<std::pair<U,std::uint32_t>> counted_refusals;U refused_face_count=0;
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
      auto prepared=[&](){try{return brepgpu::prepare_source_uv(solids.at(si),uv_faces,uv_options);}catch(const Standard_Failure& e){throw std::runtime_error("solid "+std::to_string(si)+" source UV "+e.ExceptionType()+" "+e.what());}}();
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
        // The cone semi-angle is the one raw angle that becomes a stored
        // carrier value: it must be the raw value in the declared unit.
        if(auto cone=Handle(StepGeom_ConicalSurface)::DownCast(rf->FaceGeometry());!cone.IsNull()){GeomAdaptor_Surface translated(surf);const double declared=cone->SemiAngle()*angle_factor;
          if(translated.GetType()!=GeomAbs_Cone||!(std::abs(translated.Cone().SemiAngle()-declared)<=1e-12)){std::ostringstream message;message<<std::setprecision(17)<<"conical carrier "<<sid<<" semi-angle differs from the declared unit: declared "<<declared<<" rad";throw std::runtime_error(message.str());}}
        if(seen_surfaces.insert(sid).second)tables["CARRIERS"].push_back(surface(sid,surf,{u0,u1,v0,v1}));
        IDs ls;
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
            Bytes w;w.u64(lid);w.u64(fid);w.u32(bound->IsKind(STANDARD_TYPE(StepShape_FaceOuterBound)));w.u32(0);w.ids({});w.u64(vi);w.u32(refused_loops.count(lid)?1:0);tables["LOOPS___"].push_back(w);
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
              if(curve_id&&seen_curves.insert(curve_id).second)tables["CURVES__"].push_back(curve3(curve_id,ec,std::min(first,last),std::max(first,last)).bytes());
              Bytes w;w.u64(eid);w.u64(va);w.u64(vb);w.u64(curve_id);w.doubles({ta,tb});w.u32(re->SameSense());w.u32(same);w.u32(deg);w.u32(0);w.f64(BRep_Tool::Tolerance(e));tables["EDGES___"].push_back(w);
            }
            // Native edge binding follows its geometric parameter, whereas
            // original EDGE_CURVE endpoints may have SameSense=false. Compose
            // original edge-use, bound and geometric senses before selecting
            // the seam pcurve; BRep_Tool then composes the face orientation.
            bool forward=oe->Orientation()==bound->Orientation();auto pcurve_edge=actual_face_edges.at(eid);pcurve_edge.Orientation((forward==re->SameSense())?TopAbs_FORWARD:TopAbs_REVERSED);double pf,pl;auto pc=BRep_Tool::CurveOnSurface(pcurve_edge,f,pf,pl);if(pc.IsNull())throw std::runtime_error("missing recovered UV at coedge "+std::to_string(cid));U pcid=(U(1)<<63)|cid;auto pcd=curve2(pcid,pc,pf,pl);max_pcurve_degree=std::max(max_pcurve_degree,pcd.degree);tables["CURVES__"].push_back(pcd.bytes());
            // SameParameter=false: publish a measured t->s map or refuse; never an assumed one.
            brepgpu::SourceParameterMap measured;measured.kind=1;
            if(!same){try{measured=brepgpu::measure_source_parameter_map(pcurve_edge,f,ta,tb,uv_options);}catch(const std::exception& e){throw std::runtime_error("coedge "+std::to_string(cid)+" edge "+std::to_string(eid)+": "+e.what());}
              Bytes diag;diag.u32(1);diag.u32(0);diag.text("measured_parameter_map");diag.ids({cid,eid});diag.text("measured map (kind four): edge t to pcurve s by projecting curve-on-surface samples onto the 3D curve (Approx_SameParameter method); residual sampled at nodes and quarter points, not a bound between samples");fields(diag,{{"kind",code,4.},{"rows",count,double(measured.rows.size())},{"max_sampled_residual_mm",length_mm,measured.max_residual_mm},{"bound_mm",length_mm,uv_options.source_residual_mm}});tables["DIAGS___"].push_back(diag);}
            Bytes w;w.u64(cid);w.u64(eid);w.u64(lid);w.u64(pcid);w.i32(forward?1:-1);w.u32(1);w.doubles({pf,pl});w.i32(0);w.i32(0);w.u32(measured.kind);w.u32(0);
            // Kind 4 rows are (t, s, sampled residual mm); kinds 1-3 rows are (t, s).
            D map;for(const auto& row:measured.rows){map.insert(map.end(),{row[0],row[1]});if(measured.kind==4)map.push_back(row[2]);}w.vec(map);tables["COEDGES_"].push_back(w);
          }
          Bytes w;w.u64(lid);w.u64(fid);w.u32(bound->IsKind(STANDARD_TYPE(StepShape_FaceOuterBound)));w.u32(0);w.ids(cs);w.u64(0);w.u32(0);tables["LOOPS___"].push_back(w);
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
        Bytes w;w.u64(fid);w.u64(si);w.u64(sid);w.u64(face_style);w.i32(rf->SameSense()==shell.forward?1:-1);w.u32(static_cast<std::uint32_t>(shell.ordinal));w.f64(BRep_Tool::Tolerance(f));w.ids(ls);w.u32(refused_face?1:0);tables["FACES___"].push_back(w);
      }
      for(const auto& shell:shells)if(shell.ordinal){Bytes diag;diag.u32(0);diag.u32(0);diag.text("source_void_shell");diag.ids({si,shell.id,shell.element});diag.text(std::string("void shell orientation=")+(shell.forward?".T.":".F.")+"; face orientation composed with shell orientation");fields(diag,{{"ordinal",count,double(shell.ordinal)},{"faces",count,double(shell.faces->NbCfsFaces())}});tables["DIAGS___"].push_back(diag);}
      // Status: 0 complete, 1 incomplete (a refused face; D-partial-output).
      es.assign(unique_edges.begin(),unique_edges.end());vs.assign(unique_vertices.begin(),unique_vertices.end());Bytes w;w.u64(si);w.u64(solid_products.at(si));w.ids(fs);w.ids(es);w.ids(vs);w.u32(solid_refused?1:0);tables["SOLIDS__"].push_back(w);if(solid_refused)++incomplete_definitions;
    }
    // Style records (conflicts, unplaced styles) from the shared resolver.
    style_resolver.finish();
    for(const auto& note:style_resolver.notes){Bytes diag;diag.u32(note.severity);diag.u32(0);diag.text(note.stage);diag.ids(note.ids);diag.text(note.message);fields(diag);tables["DIAGS___"].push_back(diag);}
    std::cout<<"style_path_work\t"<<style_resolver.path_work()<<"\n";
    for(auto& s:styles)tables["STYLES__"].push_back(s.second.bytes());
    Bytes meta;meta.text("OCCT b8f597c677811d1f9f4d8a97f5ae2825c0353a42; owned STEP source adapter v7; explicit-uv-projector; bounded-conic-source-chart; original-t; void-shells; declared-plane-angle; measured-parameter-map; vertex-loops; face-status; context-units; occurrence-styles; typed-diagnostics; shape-processing=translation-only; OCCT translation without ShapeProcess (empty bitset set after read, asserted before and after transfer); OCCT translation-layer edits apply: E1-E16, G1-G22, T1-T8, U1-U4, P1-P4, R1-R3 of papers/2026-10-17-occt-ingress-translation-edits.md section 2.1");tables["META____"].push_back(meta);
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
