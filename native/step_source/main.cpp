// Owned source extraction into BGSRC001. OCCT translates STEP and reconstructs
// UV curves; product sampling, triangulation and rendering are not performed.
#include "source_uv.hpp"
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
#include <StepShape_ConnectedFaceSet.hxx>
#include <StepShape_AdvancedFace.hxx>
#include <StepShape_FaceBound.hxx>
#include <StepShape_FaceOuterBound.hxx>
#include <StepShape_EdgeLoop.hxx>
#include <StepShape_OrientedEdge.hxx>
#include <StepShape_EdgeCurve.hxx>
#include <StepShape_VertexPoint.hxx>
#include <StepGeom_Surface.hxx>
#include <StepGeom_Curve.hxx>
#include <StepGeom_CartesianPoint.hxx>
#include <StepVisual_StyledItem.hxx>
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
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
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
    double source_uv_seconds=0,query_uv_seconds=0;U source_uv_projected=0,source_uv_seams=0,query_columns=0,query_endpoints=0,query_connectors=0;double source_uv_max_residual=0,query_max_residual=0,query_max_uv_metric=0;
    std::ifstream file(argv[1],std::ios::binary);if(!file)throw std::runtime_error("cannot read source");
    std::vector<unsigned char> input((std::istreambuf_iterator<char>(file)),{});if(input.empty()||input.size()>U(1)<<30)throw std::runtime_error("invalid source file size");auto digest=sha256(input);
    STEPCAFControl_Reader reader;reader.SetNameMode(true);reader.SetColorMode(true);reader.SetMatMode(true);reader.SetPropsMode(true);
    if(reader.ReadFile(argv[1])!=IFSelect_RetDone)throw std::runtime_error("STEP read failed");
    NCollection_Sequence<TCollection_AsciiString> length_units,angle_units,solid_angle_units;
    reader.ChangeReader().FileUnits(length_units,angle_units,solid_angle_units);
    if(length_units.IsEmpty())throw std::runtime_error("source has no representation length unit");
    double source_units_mm=0;
    for(int i=1;i<=length_units.Length();++i){
      std::string unit=length_units(i).ToCString();std::cout<<"source_length_unit\t"<<unit<<"\n";
      double factor=0;
      if(unit=="millimetre"||unit=="millimeter"||unit=="mm")factor=1;
      else if(unit=="centimetre"||unit=="centimeter"||unit=="cm")factor=10;
      else if(unit=="metre"||unit=="meter"||unit=="m")factor=1000;
      else throw std::runtime_error("unsupported source representation length unit "+unit);
      if(factor!=1)throw std::runtime_error("non-millimetre source parameters require an explicit source-UV conversion map");
      if(source_units_mm&&source_units_mm!=factor)throw std::runtime_error("mixed source context units require per-context conversion");
      source_units_mm=factor;
    }
    reader.ChangeReader().SetSystemLengthUnit(1.0);
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
    if(!reader.Transfer(doc))throw std::runtime_error("STEP/XDE transfer failed");
    require_no_processing("after_transfer");
    auto model=Handle(StepData_StepModel)::DownCast(reader.Reader().WS()->Model());
    auto tp=reader.Reader().WS()->TransferReader()->TransientProcess();
    auto shape_tool=XCAFDoc_DocumentTool::ShapeTool(doc->Main());
    auto id=[&](const Handle(Standard_Transient)& e)->U{if(e.IsNull())return 0;int n=model->IdentLabel(e);if(n<=0)throw std::runtime_error("missing original STEP entity label");return U(n);};
    auto result=[&](const Handle(Standard_Transient)& e){auto s=TransferBRep::ShapeResult(tp,e);if(s.IsNull())throw std::runtime_error("unmapped original source entity "+std::to_string(id(e)));return s;};
    std::map<std::string,std::vector<Bytes>> tables;std::map<U,Handle(StepShape_ManifoldSolidBrep)> raw_solids;
    std::map<U,TopoDS_Shape> solids,product_shapes;std::map<U,U> product_solids,solid_products,style_targets;
    std::map<U,StyleData> styles;std::set<U> parents,children;std::vector<Handle(StepBasic_ProductDefinition)> products;
    std::vector<Handle(StepRepr_NextAssemblyUsageOccurrence)> usages;
    double uncertainty=0,max_vertex_displacement=0;
    for(int i=1;i<=model->NbEntities();++i){auto e=model->Value(i);
      if(auto s=Handle(StepShape_ManifoldSolidBrep)::DownCast(e);!s.IsNull()){raw_solids[id(s)]=s;solids[id(s)]=result(s);}
      if(auto p=Handle(StepBasic_ProductDefinition)::DownCast(e);!p.IsNull())products.push_back(p);
      if(auto u=Handle(StepRepr_NextAssemblyUsageOccurrence)::DownCast(e);!u.IsNull()){usages.push_back(u);parents.insert(id(u->RelatingProductDefinition()));children.insert(id(u->RelatedProductDefinition()));}
      if(auto u=Handle(StepBasic_UncertaintyMeasureWithUnit)::DownCast(e);!u.IsNull())uncertainty=std::max(uncertainty,u->ValueComponent());
    }
    STEPConstruct_Styles style_reader(reader.Reader().WS());style_reader.LoadStyles();
    for(int i=1;i<=style_reader.NbStyles();++i){auto s=style_reader.Style(i);Handle(StepVisual_Colour) sc,bc,cc;STEPConstruct_RenderingProperties rendering;bool component=false;
      StyleData d;d.id=id(s);d.sources={d.id};if(style_reader.GetColors(s,sc,bc,cc,rendering,component)){
        Quantity_Color color;auto chosen=!sc.IsNull()?sc:(!bc.IsNull()?bc:cc);
        if(!chosen.IsNull()&&STEPConstruct_Styles::DecodeColor(chosen,color)){d.rgba={color.Red(),color.Green(),color.Blue(),1-rendering.Transparency()};d.color=true;d.sources.push_back(id(chosen));}
      }styles[d.id]=d;if(!s->Item().IsNull())style_targets[id(s->Item())]=d.id;
    }
    std::map<U,TDF_Label> product_labels;
    for(auto& p:products){U pi=id(p);auto ps=result(p);product_shapes[pi]=ps;
      std::set<U> matches;
      for(TopExp_Explorer ex(ps,TopAbs_SOLID);ex.More();ex.Next())for(auto& s:solids)if(ex.Current().IsSame(s.second))matches.insert(s.first);
      if(!parents.count(pi)){
        if(matches.size()!=1)throw std::runtime_error("leaf product does not bind one source solid "+std::to_string(pi));
        U si=*matches.begin();if(solid_products.count(si))throw std::runtime_error("solid has multiple leaf products");product_solids[pi]=si;solid_products[si]=pi;
      }
      TDF_Label label;shape_tool->Search(ps,label,true,true,false);product_labels[pi]=label;
      auto prod=p->Formation()->OfProduct();std::string name=str(prod->Description());if(name.empty())name=str(prod->Name());U style=product_solids.count(pi)?style_targets[product_solids[pi]]:0;
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
    for(auto& u:usages){U ui=id(u);auto s=result(u);Bytes w;w.u64(ui);w.u64(id(u->RelatingProductDefinition()));w.u64(id(u->RelatedProductDefinition()));w.u64(style_targets[ui]);w.text(str(u->Name()));w.doubles(transform(s.Location().Transformation()));tables["USAGES__"].push_back(w);}
    IDs roots;for(auto& p:products)if(!children.count(id(p)))roots.push_back(id(p));if(roots.size()!=1)throw std::runtime_error("ambiguous source root");
    std::set<U> seen_vertices,seen_edges,seen_curves,seen_surfaces;U nfaces=0,nloops=0,ncoedges=0,degenerate=0,not_same=0;unsigned max_pcurve_degree=0;
    for(auto& pair:raw_solids){U si=pair.first;auto raw=pair.second;IDs fs,es,vs;std::set<U> unique_edges,unique_vertices;
      std::vector<brepgpu::SourceUVFace> uv_faces;
      for(int fi=1;fi<=raw->Outer()->NbCfsFaces();++fi){auto rf=Handle(StepShape_AdvancedFace)::DownCast(raw->Outer()->CfsFacesValue(fi));if(rf.IsNull())throw std::runtime_error("non-advanced UV source face");
        brepgpu::SourceUVFace face;face.id=id(rf);face.carrier_id=id(rf->FaceGeometry());face.face=TopoDS::Face(result(rf));auto forward_face=face.face;
        std::vector<TopoDS_Wire> wires;for(TopExp_Explorer w(forward_face,TopAbs_WIRE);w.More();w.Next())wires.push_back(TopoDS::Wire(w.Current()));
        for(int bi=1;bi<=rf->NbBounds();++bi){auto bound=rf->BoundsValue(bi);auto loop=Handle(StepShape_EdgeLoop)::DownCast(bound->Bound());if(loop.IsNull())throw std::runtime_error("non-edge UV source loop");brepgpu::SourceUVLoop binding;binding.id=id(loop);
          for(int position=1;position<=loop->NbEdgeList();++position){int ei=bound->Orientation()?position:loop->NbEdgeList()+1-position;auto oe=loop->EdgeListValue(ei);auto re=Handle(StepShape_EdgeCurve)::DownCast(oe->EdgeElement());bool forward=oe->Orientation()==bound->Orientation();brepgpu::SourceUVUse use;use.id=id(oe);use.edge_id=id(re);use.edge=TopoDS::Edge(result(re));use.edge.Orientation((forward==re->SameSense())?TopAbs_FORWARD:TopAbs_REVERSED);auto va=forward?re->EdgeStart():re->EdgeEnd(),vb=forward?re->EdgeEnd():re->EdgeStart();use.start_id=id(va);use.end_id=id(vb);use.start=TopoDS::Vertex(result(va));use.end=TopoDS::Vertex(result(vb));binding.uses.push_back(use);}
          std::vector<std::size_t> matches;for(std::size_t wi=0;wi<wires.size();++wi){std::vector<TopoDS_Edge> uses;for(TopExp_Explorer e(wires[wi],TopAbs_EDGE);e.More();e.Next())uses.push_back(TopoDS::Edge(e.Current()));if(uses.size()!=binding.uses.size())continue;bool okay=true;for(auto& edge:uses)if(std::none_of(binding.uses.begin(),binding.uses.end(),[&](const brepgpu::SourceUVUse& use){return edge.IsSame(use.edge);}))okay=false;if(okay)matches.push_back(wi);}if(matches.size()!=1)throw std::runtime_error("ambiguous original wire UV binding at face "+std::to_string(face.id));binding.wire=wires[matches[0]];wires.erase(wires.begin()+matches[0]);face.loops.push_back(binding);
        }if(!wires.empty())throw std::runtime_error("unowned native wire before UV preparation");uv_faces.push_back(face);
      }
      auto prepared=brepgpu::prepare_source_uv(solids.at(si),uv_faces,uv_options);
      auto source_result=[&](const Handle(Standard_Transient)& e){return prepared.mapped(result(e));};solids.at(si)=prepared.raw_shape;
      source_uv_seconds+=prepared.stats.projection_seconds;query_uv_seconds+=prepared.stats.query_seconds;source_uv_projected+=prepared.stats.projected_edges;source_uv_max_residual=std::max(source_uv_max_residual,prepared.stats.max_residual_mm);source_uv_seams+=prepared.stats.seam_pairs;query_columns+=prepared.stats.query_column_edges;query_endpoints+=prepared.stats.query_joint_endpoints;query_connectors+=prepared.stats.query_connectors;query_max_residual=std::max(query_max_residual,prepared.stats.max_query_residual_mm);query_max_uv_metric=std::max(query_max_uv_metric,prepared.stats.max_query_uv_metric_mm);
      for(const auto& c:prepared.connectors)std::cout<<"source_query_connector\t"<<si<<'\t'<<c.face_id<<'\t'<<c.loop_id<<'\t'<<c.vertex_id<<'\t'<<c.incoming_id<<'\t'<<c.outgoing_id<<'\t'<<std::setprecision(17)<<c.u0<<'\t'<<c.v0<<'\t'<<c.u1<<'\t'<<c.v1<<"\n";
      if(uv_options.prepare_query){auto path=std::filesystem::path(query_directory)/("definition-"+std::to_string(si)+".brep");if(!BinTools::Write(prepared.query_shape,path.string().c_str(),false,false,BinTools_FormatVersion_VERSION_4))throw std::runtime_error("query BRep write failed");}

      if(!BRepCheck_Analyzer(solids.at(si)).IsValid()){
        Bytes w;w.u32(1);w.u32(0);w.text("reference_shape_check");w.ids({si});w.text("OCCT imported solid validity check is non-green; independent geometry qualification remains required");tables["DIAGS___"].push_back(w);
      }
      GProp_GProps volume;BRepGProp::VolumeProperties(solids.at(si),volume);std::cout<<"reference_volume\t"<<si<<"\t"<<std::setprecision(17)<<volume.Mass()<<"\n";
      for(int fi=1;fi<=raw->Outer()->NbCfsFaces();++fi){auto rf=Handle(StepShape_AdvancedFace)::DownCast(raw->Outer()->CfsFacesValue(fi));if(rf.IsNull())throw std::runtime_error("non-advanced source face");U fid=id(rf);fs.push_back(fid);++nfaces;auto f=TopoDS::Face(source_result(rf));TopLoc_Location fl;auto surf=BRep_Tool::Surface(f,fl);if(!fl.IsIdentity())surf=Handle(Geom_Surface)::DownCast(surf->Transformed(fl.Transformation()));
        std::vector<unsigned> raw_wire_counts;std::multiset<U> raw_edge_uses;
        std::map<U,TopoDS_Edge> face_edges,actual_face_edges;
        std::map<U,TopoDS_Vertex> face_vertices;
        for(int bi=1;bi<=rf->NbBounds();++bi){auto loop=Handle(StepShape_EdgeLoop)::DownCast(rf->BoundsValue(bi)->Bound());if(loop.IsNull())throw std::runtime_error("non-edge source loop");
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
            if(analytic){if(emit){Bytes diag;diag.u32(1);diag.u32(0);diag.text("translated_singular_chart_connector");diag.ids({fid,pole_id});diag.text("foreign UV chart connector collapses to original analytic pole vertex; original physical topology retained");tables["DIAGS___"].push_back(diag);}return 0;}
          }
          IDs candidates;
          for(auto& candidate:face_edges){TopLoc_Location ol;double of=0,oz=0;auto oc=BRep_Tool::Curve(candidate.second,ol,of,oz);if(ac==oc&&al==ol&&af==of&&az==oz)candidates.push_back(candidate.first);}
          if(candidates.size()!=1)throw std::runtime_error("translated face contains unmatched source edge "+std::to_string(fid));
          U match=candidates[0];auto original=face_edges.at(match);TopoDS_Vertex a1,a2,o1,o2;TopExp::Vertices(actual,a1,a2);TopExp::Vertices(original,o1,o2);
          if(!a1.IsSame(o1)||!a2.IsSame(o2)||BRep_Tool::SameParameter(actual)!=BRep_Tool::SameParameter(original)||BRep_Tool::Degenerated(actual)!=BRep_Tool::Degenerated(original))throw std::runtime_error("translated edge copy changed source endpoints/flags at face "+std::to_string(fid));
          actual_face_edges[match]=actual;
          if(emit){Bytes diag;diag.u32(1);diag.u32(0);diag.text("translated_edge_topology_copy");diag.ids({fid,match});std::ostringstream message;message<<std::setprecision(17)<<"distinct edge topology with identical carrier/location/range/endpoints/flags; source tolerance="<<BRep_Tool::Tolerance(original)<<" face tolerance="<<BRep_Tool::Tolerance(actual);diag.text(message.str());tables["DIAGS___"].push_back(diag);}return match;
        };
        for(TopExp_Explorer actual(f,TopAbs_EDGE);actual.More();actual.Next())match_actual_edge(TopoDS::Edge(actual.Current()),true);
        U sid=id(rf->FaceGeometry());double u0,u1,v0,v1;BRepTools::UVBounds(f,u0,u1,v0,v1);
        if(seen_surfaces.insert(sid).second)tables["CARRIERS"].push_back(surface(sid,surf,{u0,u1,v0,v1}));
        IDs ls;
        for(int bi=1;bi<=rf->NbBounds();++bi){auto bound=rf->BoundsValue(bi);auto loop=Handle(StepShape_EdgeLoop)::DownCast(bound->Bound());if(loop.IsNull())throw std::runtime_error("non-edge source loop");U lid=id(loop);ls.push_back(lid);++nloops;IDs cs;
          raw_wire_counts.push_back(loop->NbEdgeList());
          for(int position=1;position<=loop->NbEdgeList();++position){int ei=bound->Orientation()?position:loop->NbEdgeList()+1-position;auto oe=loop->EdgeListValue(ei);auto re=Handle(StepShape_EdgeCurve)::DownCast(oe->EdgeElement());if(re.IsNull())throw std::runtime_error("non-curve source edge");U eid=id(re),cid=id(oe);cs.push_back(cid);++ncoedges;auto e=TopoDS::Edge(source_result(re));unique_edges.insert(eid);raw_edge_uses.insert(eid);face_edges[eid]=e;
            auto start=re->EdgeStart(),end=re->EdgeEnd();U va=id(start),vb=id(end);unique_vertices.insert(va);unique_vertices.insert(vb);
            for(auto vp:{start,end}){U vi=id(vp);if(seen_vertices.insert(vi).second){
              auto rv=Handle(StepShape_VertexPoint)::DownCast(vp);if(rv.IsNull())throw std::runtime_error("non-point source vertex");auto rp=Handle(StepGeom_CartesianPoint)::DownCast(rv->VertexGeometry());if(rp.IsNull()||rp->NbCoordinates()!=3)throw std::runtime_error("non-Cartesian source vertex");
              auto v=TopoDS::Vertex(source_result(vp));auto p=BRep_Tool::Pnt(v);gp_Pnt original(rp->CoordinatesValue(1)*source_units_mm,rp->CoordinatesValue(2)*source_units_mm,rp->CoordinatesValue(3)*source_units_mm);double displacement=p.Distance(original);max_vertex_displacement=std::max(max_vertex_displacement,displacement);
              if(displacement>0){Bytes diag;diag.u32(1);diag.u32(0);diag.text("translated_vertex_displacement");diag.ids({vi,id(rp)});diag.text("source-to-translated displacement mm="+std::to_string(displacement));tables["DIAGS___"].push_back(diag);}
              Bytes w;w.u64(vi);w.doubles({original.X(),original.Y(),original.Z()});w.f64(BRep_Tool::Tolerance(v));tables["VERTICES"].push_back(w);
            }}
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
            Bytes w;w.u64(cid);w.u64(eid);w.u64(lid);w.u64(pcid);w.i32(forward?1:-1);w.u32(1);w.doubles({pf,pl});w.i32(0);w.i32(0);w.u32(same?1:3);w.u32(0);
            if(same)w.vec({});else{D map;for(int j=0;j<=8;++j){double t=ta+(tb-ta)*j/8;double s=pf+(pl-pf)*(t-first)/(last-first);map.insert(map.end(),{t,s});}w.vec(map);}tables["COEDGES_"].push_back(w);
          }
          Bytes w;w.u64(lid);w.u64(fid);w.u32(bound->IsKind(STANDARD_TYPE(StepShape_FaceOuterBound)));w.u32(0);w.ids(cs);tables["LOOPS___"].push_back(w);
        }
        std::vector<unsigned> translated_wire_counts;std::multiset<U> translated_edge_uses;
        for(TopExp_Explorer wires(f,TopAbs_WIRE);wires.More();wires.Next()){
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
        Bytes w;w.u64(fid);w.u64(si);w.u64(sid);w.u64(style_targets[fid]);w.i32(rf->SameSense()?1:-1);w.u32(0);w.f64(BRep_Tool::Tolerance(f));w.ids(ls);tables["FACES___"].push_back(w);
      }
      es.assign(unique_edges.begin(),unique_edges.end());vs.assign(unique_vertices.begin(),unique_vertices.end());Bytes w;w.u64(si);w.u64(solid_products.at(si));w.ids(fs);w.ids(es);w.ids(vs);tables["SOLIDS__"].push_back(w);
    }
    for(auto& s:styles)tables["STYLES__"].push_back(s.second.bytes());
    Bytes meta;meta.text("OCCT b8f597c677811d1f9f4d8a97f5ae2825c0353a42; owned STEP source adapter v3; explicit-uv-projector; original-t; shape-processing=none; set-after-read; asserted-before-and-after-transfer");tables["META____"].push_back(meta);
    std::cout<<"source_uv_seconds\t"<<std::setprecision(17)<<source_uv_seconds<<"\nquery_uv_seconds\t"<<query_uv_seconds<<"\nsource_uv_projected_edges\t"<<source_uv_projected<<"\nsource_uv_max_residual_mm\t"<<source_uv_max_residual<<"\nsource_uv_seam_pairs\t"<<source_uv_seams<<"\nsource_query_column_edges\t"<<query_columns<<"\nsource_query_endpoint_edits\t"<<query_endpoints<<"\nsource_query_connectors\t"<<query_connectors<<"\nsource_query_max_residual_mm\t"<<query_max_residual<<"\nsource_query_max_uv_metric_mm\t"<<query_max_uv_metric<<"\n";
    const std::vector<std::string> tags={"META____","PRODUCTS","SOLIDS__","USAGES__","VERTICES","EDGES___","COEDGES_","LOOPS___","FACES___","CARRIERS","CURVES__","STYLES__","DIAGS___"};Bytes body;
    for(auto& tag:tags){Bytes section;for(auto& row:tables[tag])section.row(row);body.raw(tag);body.u64(tables[tag].size());body.u64(section.v.size());body.v.insert(body.v.end(),section.v.begin(),section.v.end());}
    Bytes out;out.raw("BGSRC001");out.u32(1);out.u32(0);out.v.insert(out.v.end(),digest.begin(),digest.end());out.f64(source_units_mm);out.f64(uncertainty*source_units_mm);out.u64(roots[0]);out.u32(tags.size());out.u32(0);out.u64(88+body.v.size());out.v.insert(out.v.end(),body.v.begin(),body.v.end());
    std::string temporary=std::string(argv[2])+".tmp";std::ofstream output(temporary,std::ios::binary);output.write(reinterpret_cast<const char*>(out.v.data()),out.v.size());output.close();if(!output)throw std::runtime_error("source binary write failed");if(std::rename(temporary.c_str(),argv[2]))throw std::runtime_error("source binary rename failed");
    std::cout<<"solids\t"<<raw_solids.size()<<"\nfaces\t"<<nfaces<<"\nloops\t"<<nloops<<"\ncoedges\t"<<ncoedges<<"\nedges\t"<<seen_edges.size()<<"\nvertices\t"<<seen_vertices.size()<<"\nnon_same_parameter\t"<<not_same<<"\ndegenerate_edges\t"<<degenerate<<"\nmax_pcurve_degree\t"<<max_pcurve_degree<<"\nmax_vertex_displacement_mm\t"<<max_vertex_displacement<<"\n";
    return 0;
  }catch(const Standard_Failure& e){std::cerr<<"source extraction failed: "<<e.what()<<"\n";return 1;}
  catch(const std::exception& e){std::cerr<<"source extraction failed: "<<e.what()<<"\n";return 1;}
}
