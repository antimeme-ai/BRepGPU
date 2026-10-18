// Separate original-source boundary for the external CPU reference. No product
// source, owned mesh, foreign mesher or occurrence transform is involved.
#include "step_units.hpp"
#include "step_styles.hpp"
#include "step_vertex_loops.hpp"
#include "step_placements.hpp"
#include "shape_extent.hpp"
#include "source_uv/source_uv.hpp"
#include "orientation_verdict.hpp"
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <StepGeom_CartesianPoint.hxx>
#include <StepShape_EdgeCurve.hxx>
#include <StepShape_EdgeLoop.hxx>
#include <StepShape_FaceBound.hxx>
#include <StepShape_OrientedEdge.hxx>
#include <StepShape_VertexLoop.hxx>
#include <StepShape_VertexPoint.hxx>
#include <StepShape_BrepWithVoids.hxx>
#include <StepShape_OrientedClosedShell.hxx>
#include <StepShape_ShapeDefinitionRepresentation.hxx>
#include <StepRepr_MappedItem.hxx>
#include <StepRepr_RepresentationMap.hxx>
#include <StepRepr_ProductDefinitionShape.hxx>
#include <StepGeom_Axis2Placement3d.hxx>
#include <StepGeom_Direction.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepTools_ReShape.hxx>
#include <BRep_Builder.hxx>
#include <TopoDS_Iterator.hxx>
#include <TopAbs.hxx>
#include <NCollection_DataMap.hxx>
#include <TopTools_ShapeMapHasher.hxx>
#include <memory>
#include <STEPControl_Reader.hxx>
#include <BRepTools.hxx>
#include <BRepTopAdaptor_FClass2d.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <GeomAPI_ProjectPointOnSurf.hxx>
#include <GeomLProp_SLProps.hxx>
#include <Geom_Surface.hxx>
#include <Precision.hxx>
#include <IntCurvesFace_ShapeIntersector.hxx>
#include <gp_Lin.hxx>
#include <gp_Pnt2d.hxx>
#include <TopoDS_Wire.hxx>
#include <STEPCAFControl_Reader.hxx>
#include <STEPConstruct_Styles.hxx>
#include <STEPConstruct_RenderingProperties.hxx>
#include <XSControl_WorkSession.hxx>
#include <XSControl_TransferReader.hxx>
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
#include <StepGeom_Surface.hxx>
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
#include <BinTools_ShapeSet.hxx>
#include <BinTools_FormatVersion.hxx>
#include <TopoDS.hxx>
#include <TopExp_Explorer.hxx>
#include <TopLoc_Location.hxx>
#include <Quantity_Color.hxx>
#include <Standard_Failure.hxx>
#include <array>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <BinTools.hxx>
#include <iostream>
#include <map>
#include <iomanip>
#include <functional>
#include <sstream>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

using U=std::uint64_t;
using D=std::vector<double>;
using Clock=std::chrono::steady_clock;
struct Bytes {
  std::vector<unsigned char> v;
  void u32(std::uint32_t x){for(int i=0;i<4;++i)v.push_back((x>>(8*i))&255);}
  void i32(std::int32_t x){u32(static_cast<std::uint32_t>(x));}
  void u64(U x){for(int i=0;i<8;++i)v.push_back((x>>(8*i))&255);}
  void f64(double x){if(!std::isfinite(x))throw std::runtime_error("nonfinite export scalar");U b;std::memcpy(&b,&x,8);u64(b);}
  void raw(const std::string& s){v.insert(v.end(),s.begin(),s.end());}
  void text(const std::string& s){if(s.size()>UINT32_MAX)throw std::runtime_error("oversized export string");u32(s.size());raw(s);}
  void doubles(const D& a){for(double x:a)f64(x);}
  void row(const Bytes& b){u64(b.v.size());v.insert(v.end(),b.v.begin(),b.v.end());}
};

// Owned SHA-256 scalar implementation, also used by the product source
// adapter. The separate target deliberately does not include its private main.
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
std::string text(const Handle(TCollection_HAsciiString)& s){return s.IsNull()?"":s->ToCString();}
D matrix(const gp_Trsf& t){D v;for(int i=1;i<=3;++i)for(int j=1;j<=4;++j)v.push_back(t.Value(i,j));return v;}
double seconds(Clock::time_point a,Clock::time_point b){return std::chrono::duration<double>(b-a).count();}
struct Style {
  U id=0;std::vector<U> sources;D rgba={0,0,0,1};bool color=false,density_set=false;std::string material,unit;double density=0;
  Bytes bytes()const{Bytes w;w.u64(id);w.u64(sources.size());for(U s:sources)w.u64(s);w.doubles(rgba);w.u32(color);w.u32(density_set);w.text(material);w.text(unit);w.f64(density);return w;}
};

// Raw placements (bead .37.33) are read with the STEP source adapter's
// transcription of the oracle, native/step_placements.hpp: usages through
// placements::Reader, mapped items as target o origin^-1 (placements::frames
// of the origin onto the target, the PDM guide's C = B A^-1 for the same pair
// of frames). OCCT's ComputeTransformation forms origin^-1 o target
// (STEPControl_ActorRead.cxx:2488), equal only when the origin is the identity.
namespace placements=brepgpu::step_placements;

// --face-sides INPUT.stp FACE...: for each original face, which side its raw
// use's outward normal (same_sense composed with its shell) faces: by ray parity
// on OCCT's default read, against the raw surface of OCCT's translation without
// ShapeProcess (orientation_verdict.hpp raw_use_side). An independent check for
// the oracle (python/brepgpu/imported_oracles.py) of a side the STEP source
// adapter corrected; it shares no code with the adapter's classifier.
int face_sides(const char* path,const std::set<U>& wanted){
  STEPControl_Reader processed,raw;
  for(auto* r:{&processed,&raw})if(r->ReadFile(path)!=IFSelect_RetDone)throw std::runtime_error("original STEP read failed");
  processed.SetSystemLengthUnit(1.0);raw.SetSystemLengthUnit(1.0);raw.SetShapeProcessFlags(ShapeProcess::OperationsFlags());
  for(auto* r:{&processed,&raw})brepgpu::step_placements::refuse_unbounded_transfer(r->StepModel(),r->WS()->Graph());
  processed.TransferRoots();raw.TransferRoots();
  const auto pm=processed.StepModel(),rm=raw.StepModel();const auto ptp=processed.WS()->TransferReader()->TransientProcess(),rtp=raw.WS()->TransferReader()->TransientProcess();
  std::map<U,Handle(Standard_Transient)> processed_entities;for(int i=1;i<=pm->NbEntities();++i)if(const int l=pm->IdentLabel(pm->Value(i));l>0)processed_entities[U(l)]=pm->Value(i);
  std::map<U,std::string> sides;
  for(int i=1;i<=rm->NbEntities();++i){auto solid=Handle(StepShape_ManifoldSolidBrep)::DownCast(rm->Value(i));if(solid.IsNull()||solid->Outer().IsNull())continue;
    const U si=U(rm->IdentLabel(solid));const auto pe=processed_entities.find(si);const TopoDS_Shape P=pe==processed_entities.end()?TopoDS_Shape():TransferBRep::ShapeResult(ptp,pe->second);
    std::vector<std::pair<Handle(StepShape_ConnectedFaceSet),bool>> shells;Handle(StepShape_ConnectedFaceSet) outer=solid->Outer();
    if(auto o=Handle(StepShape_OrientedClosedShell)::DownCast(outer);!o.IsNull())outer=o->ClosedShellElement();
    shells.push_back({outer,true});
    if(auto v=Handle(StepShape_BrepWithVoids)::DownCast(solid);!v.IsNull())for(int k=1;k<=v->NbVoids();++k)if(!v->VoidsValue(k).IsNull())shells.push_back({v->VoidsValue(k)->ClosedShellElement(),v->VoidsValue(k)->Orientation()});
    for(const auto& [set,forward]:shells){if(set.IsNull())continue;for(int k=1;k<=set->NbCfsFaces();++k){auto f=Handle(StepShape_AdvancedFace)::DownCast(set->CfsFacesValue(k));if(f.IsNull())continue;const U fi=U(rm->IdentLabel(f));if(!wanted.count(fi))continue;
      if(P.IsNull()||P.ShapeType()!=TopAbs_SOLID){sides[fi]="undecidable: OCCT's default read does not make its definition one solid";continue;}
      const auto t=TransferBRep::ShapeResult(rtp,f);const auto fe=processed_entities.find(fi);const TopoDS_Shape r=fe==processed_entities.end()?TopoDS_Shape():TransferBRep::ShapeResult(ptp,fe->second);
      std::vector<TopoDS_Face> pieces;if(!r.IsNull())for(TopExp_Explorer e(r,TopAbs_FACE);e.More();e.Next())pieces.push_back(TopoDS::Face(e.Current()));
      if(t.IsNull()||t.ShapeType()!=TopAbs_FACE||pieces.empty()){sides[fi]="undecidable: no translated face";continue;}
      // Every piece must give the same side (the face's raw use is one side of one surface).
      std::string side;for(const auto& piece:pieces){const auto one=brepgpu::reference_orientation::raw_use_side(piece,BRep_Tool::Surface(TopoDS::Face(t)),f->SameSense()==forward?1.:-1.,P);
        if(one.rfind("undecidable",0)==0){side=one;break;}if(side.empty())side=one;else if(side!=one){side="undecidable: the raw use points into the material on some of OCCT's pieces of the face and out of it on others";break;}}
      sides[fi]=side;}}}
  for(U fi:wanted)std::cout<<"face_side\t"<<fi<<'\t'<<(sides.count(fi)?sides.at(fi):std::string("undecidable: not a face of any solid"))<<"\n";
  return 0;
}

int main(int argc,char** argv){
  // The shape-processing policy is the NVIDIA ingress arm it serves
  // (papers/2026-10-17-occt-ingress-translation-edits.md section 5):
  // translation-only, OCCT's translation without ShapeProcess (the default);
  // default, OCCT's own default STEP read (FixShape), whose face map is built
  // from the processed shape.
  if(argc>=3&&std::string(argv[1])=="--face-sides"){try{std::set<U> wanted;for(int i=3;i<argc;++i)wanted.insert(U(std::stoull(argv[i])));return face_sides(argv[2],wanted);}
    catch(const Standard_Failure& e){std::cerr<<"face-side check failed: "<<e.what()<<"\n";return 1;}catch(const std::exception& e){std::cerr<<"face-side check failed: "<<e.what()<<"\n";return 1;}}
  std::string mode="translation-only";int first=1;
  if(argc==5&&std::string(argv[1])=="--shape-processing"){mode=argv[2];first=3;}
  if(argc-first!=2||(mode!="translation-only"&&mode!="default")){std::cerr<<"usage: brepgpu-reference-source [--shape-processing translation-only|default] INPUT.stp NEW_OUTPUT_DIRECTORY\n";return 2;}
  const bool processing=mode=="default";const char* input_path=argv[first];const char* output_path=argv[first+1];
  try{
    const auto begin=Clock::now();
    std::ifstream original(input_path,std::ios::binary);if(!original)throw std::runtime_error("cannot read original STEP");
    std::vector<unsigned char> input((std::istreambuf_iterator<char>(original)),{});
    if(input.empty()||input.size()>(U(1)<<30))throw std::runtime_error("invalid original STEP size");
    auto digest=sha256(input);
    STEPCAFControl_Reader reader;reader.SetNameMode(true);reader.SetColorMode(true);reader.SetMatMode(true);reader.SetPropsMode(true);
    if(reader.ReadFile(input_path)!=IFSelect_RetDone)throw std::runtime_error("original STEP read failed");
    const auto read_end=Clock::now();
    if(!processing)reader.SetShapeProcessFlags(ShapeProcess::OperationsFlags());
    // translation-only: the empty bitset is supplied before the transfer and
    // holds after it. default: nothing of ours is supplied (neither flags nor
    // fix parameters), so the transfer runs OCCT's own default processing,
    // which must then be in force.
    std::string operation_bits;
    auto require_policy=[&](const char* stage,bool after){
      const auto& flags=reader.GetShapeProcessFlags();
      std::cout<<"shape_process_"<<stage<<"\t"<<flags.second<<"\t"<<flags.first.count()<<"\n";
      const bool ok=!processing?(flags.second&&!flags.first.any()):after?(flags.second&&flags.first.any()):(!flags.second&&reader.GetShapeFixParameters().IsEmpty());
      if(!ok)throw std::runtime_error(std::string("reference STEP processing policy differs from ")+mode+" at "+stage+" supplied="+std::to_string(flags.second)+" operations="+flags.first.to_string());
      if(after)operation_bits=flags.first.to_string();
    };
    require_policy("before_transfer",false);
    Handle(TDocStd_Document) doc;XCAFApp_Application::GetApplication()->NewDocument("BinXCAF",doc);
    namespace units=brepgpu::step_units;
    units::prepare_millimetre_transfer(doc);
    placements::refuse_unbounded_transfer(Handle(StepData_StepModel)::DownCast(reader.Reader().WS()->Model()),reader.Reader().WS()->Graph());
    if(!reader.Transfer(doc))throw std::runtime_error("original STEP/XDE transfer failed");
    require_policy("after_transfer",true);
    if(processing)std::cout<<"shape_fix_parameters\t"<<reader.GetShapeFixParameters().Size()<<"\n";
    const auto transfer_end=Clock::now();
    auto model=Handle(StepData_StepModel)::DownCast(reader.Reader().WS()->Model());
    units::require_transferred_millimetre(doc,model);
    // Length units per context from the unit entities, never names; the same
    // millimetre-only rule and contexts as the STEP source adapter.
    const auto& entity_graph=reader.Reader().WS()->Graph();
    const units::RepresentationGraph representations(entity_graph);
    {std::map<U,Handle(StepRepr_Representation)> governed;
     for(const auto& rep:units::product_representations(model))governed.emplace(units::label(model,rep),rep);
     for(int i=1;i<=model->NbEntities();++i)if(auto s=Handle(StepShape_ManifoldSolidBrep)::DownCast(model->Value(i));!s.IsNull()){
       auto users=entity_graph.Sharings(s);for(users.Start();users.More();users.Next())if(auto rep=Handle(StepShape_ShapeRepresentation)::DownCast(users.Value());!rep.IsNull())for(const auto& r:representations.transfer_path(rep))governed.emplace(units::label(model,r),r);}
     for(const auto& g:governed)units::require_millimetre(model,g.second,units::context_length(model,g.second));}
    auto process=reader.Reader().WS()->TransferReader()->TransientProcess();
    auto shape_tool=XCAFDoc_DocumentTool::ShapeTool(doc->Main());
    auto id=[&](const Handle(Standard_Transient)& e)->U{if(e.IsNull())return 0;int n=model->IdentLabel(e);if(n<=0)throw std::runtime_error("missing original STEP label");return U(n);};
    // Under the default read, TransferBRep::ShapeResult is OCCT's processed
    // result: XSAlgo_ShapeProcessor::MergeTransferInfo rebinds every entity
    // whose shape the processing replaced (a split face to its pieces).
    auto shape=[&](const Handle(Standard_Transient)& e){auto s=TransferBRep::ShapeResult(process,e);if(s.IsNull())throw std::runtime_error("unmapped original source #"+std::to_string(id(e)));return s;};
    // Records: what the resolver found, the vertex-loop faces OCCT exported
    // that the STEP source adapter refuses, and what the default read changed
    // (stage, severity, space-separated ids, message), in records.tsv beside
    // the manifest and also said on stdout.
    std::vector<std::string> records{"stage\tseverity\tids\tmessage"};
    auto record=[&](const std::string& stage,std::uint32_t severity,const std::vector<U>& ids,const std::string& message){std::string line=stage+'\t'+std::to_string(severity)+'\t';for(std::size_t k=0;k<ids.size();++k)line+=(k?" ":"")+std::to_string(ids[k]);records.push_back(line+'\t'+message);};
    std::map<U,Handle(StepShape_ManifoldSolidBrep)> raw_solids;
    std::map<U,TopoDS_Shape> solids;
    std::vector<Handle(StepBasic_ProductDefinition)> products;
    std::vector<Handle(StepRepr_NextAssemblyUsageOccurrence)> usages;
    std::set<U> parents,children;double uncertainty=0;std::map<U,std::optional<double>> usable_uncertainty;
    for(int i=1;i<=model->NbEntities();++i){auto e=model->Value(i);
      if(auto s=Handle(StepShape_ManifoldSolidBrep)::DownCast(e);!s.IsNull()){raw_solids[id(s)]=s;solids[id(s)]=shape(s);
        // The solid's B-rep context governs its uncertainty, in mm (G6 UR-7),
        // chosen as the STEP source adapter chooses it; the header maximum
        // takes only usable values (UR-7: one below the floor makes no claim).
        const auto held=units::held_uncertainty(model,representations.holders(s));auto& usable=usable_uncertainty[id(s)];
        // B2 .37.39, the adapter's rule: the diagonal of a box that holds the
        // solid (native/shape_extent.hpp), not BRepBndLib's, which spans an
        // unbounded carrier where pcurves are missing and misses a cone's
        // apex. An unbounded extent has no diagonal to hold the value against.
        if(held.governing.governed){const double diagonal=brepgpu::shape_extent::shape_diagonal(solids[id(s)]);
          if(!std::isfinite(diagonal))record("reference_uncertainty_unbounded",1,{id(s)},"the definition's extent is not bounded (a carrier native/shape_extent.hpp cannot bound), so its declared uncertainty makes no usable claim (G6 UR-7)");
          else if(units::uncertainty_usable(held.governing.mm,diagonal)){usable=held.governing.mm;uncertainty=std::max(uncertainty,held.governing.mm);}}}
      if(auto p=Handle(StepBasic_ProductDefinition)::DownCast(e);!p.IsNull())products.push_back(p);
      if(auto u=Handle(StepRepr_NextAssemblyUsageOccurrence)::DownCast(e);!u.IsNull()){usages.push_back(u);parents.insert(id(u->RelatingProductDefinition()));children.insert(id(u->RelatedProductDefinition()));}
    }
    if(solids.empty())throw std::runtime_error("no original solid definitions");
    // A definition is one solid. OCCT's default read can make one original
    // solid several (ShapeFix splits a shell); that read cannot serve the arm.
    if(processing)for(const auto& s:solids)if(s.second.ShapeType()!=TopAbs_SOLID){U n=0;for(TopExp_Explorer e(s.second,TopAbs_SOLID);e.More();e.Next())++n;
      throw std::runtime_error("OCCT's default read made original solid #"+std::to_string(s.first)+" into "+std::to_string(n)+" solids; a reference definition is one solid");}
    // Every shell of a solid, as the STEP source adapter walks them: the outer
    // shell, then each BREP_WITH_VOIDS void in file order. OCCT
    // (StepToTopoDS_Builder::Init(BrepWithVoids)) reverses a void marked .F.,
    // so a face's use in the solid is same_sense composed with its shell's
    // orientation. ABSR WR4 requires every void to be .F. and WR3 forbids an
    // oriented outer shell; OCCT reads a .T. one as its closed shell, which is
    // accepted with a record, and a .F. one refuses, as in the adapter.
    struct Shell{U ordinal=0,id=0,element=0;bool forward=true;Handle(StepShape_ConnectedFaceSet) faces;};
    auto raw_shells=[&](const Handle(StepShape_ManifoldSolidBrep)& raw,bool write_records){std::vector<Shell> shells;
      if(raw->Outer().IsNull())throw std::runtime_error("solid without outer shell "+std::to_string(id(raw)));
      Handle(StepShape_ConnectedFaceSet) outer=raw->Outer();
      if(auto oriented=Handle(StepShape_OrientedClosedShell)::DownCast(outer);!oriented.IsNull()){
        if(!oriented->Orientation()||oriented->ClosedShellElement().IsNull())throw std::runtime_error("outer shell "+std::to_string(id(oriented))+" of solid "+std::to_string(id(raw))+" is a reversed oriented_closed_shell (ABSR WR3 forbids an oriented outer shell)");
        outer=oriented->ClosedShellElement();if(write_records)record("source_oriented_outer_shell",1,{id(raw),id(oriented),id(outer)},"outer shell is an oriented_closed_shell .T. (ABSR WR3 forbids it); read as its closed shell, as OCCT reads it");}
      shells.push_back({0,id(raw->Outer()),id(outer),true,outer});
      if(auto with_voids=Handle(StepShape_BrepWithVoids)::DownCast(raw);!with_voids.IsNull())for(int k=1;k<=with_voids->NbVoids();++k){auto shell=with_voids->VoidsValue(k);
        if(shell.IsNull()||shell->ClosedShellElement().IsNull())throw std::runtime_error("void without closed shell at solid "+std::to_string(id(raw)));
        if(shell->Orientation())throw std::runtime_error("void shell "+std::to_string(id(shell))+" of solid "+std::to_string(id(raw))+" has orientation .T.; ABSR WR4 requires every void to have orientation .F.");
        shells.push_back({U(k),id(shell),id(shell->ClosedShellElement()),shell->Orientation(),shell->ClosedShellElement()});}
      std::set<U> faces;for(const auto& s:shells)for(int fi=1;fi<=s.faces->NbCfsFaces();++fi)if(!faces.insert(id(s.faces->CfsFacesValue(fi))).second)throw std::runtime_error("source face in more than one shell of solid "+std::to_string(id(raw)));
      return shells;};
    std::map<U,Style> styles;std::map<U,U> product_solids,solid_products;
    // The same styles as the STEP source adapter (native/step_styles.hpp), so
    // the reference and owned sources agree style for style.
    brepgpu::step_styles::Resolver style_resolver(model,reader.Reader().WS());
    for(const auto& c:style_resolver.colours()){Style d;d.id=c.id;d.sources=c.sources;d.rgba={c.rgba[0],c.rgba[1],c.rgba[2],c.rgba[3]};d.color=c.color;styles[d.id]=d;}
    std::map<std::string,std::vector<Bytes>> tables;
    std::vector<U> roots;
    // A leaf product whose shape representation places its solid through a
    // MAPPED_ITEM (bead .37.25). The mapped item is published as an occurrence:
    // the product becomes an assembly node, the mapped representation a part
    // product (keyed by that representation's label, with the holder's names)
    // owning the solid, and the mapped item a usage (keyed by its own label)
    // whose placement is read raw (target o origin^-1). Only one level is
    // read: a mapped representation that holds exactly one solid and no further
    // mapped item, and that no product owns. Every other solid-bearing mapped
    // item (in a parent's representation, a product's representation mapped
    // into another, one reaching a solid through related relationships) is
    // refused by name below, after the products are read.
    struct Mapped{U item=0,rep=0,solid=0;std::string name;D transform;Handle(StepRepr_Representation) mapped,holder;};
    std::map<U,std::vector<Handle(StepRepr_Representation)>> product_reps;
    for(int i=1;i<=model->NbEntities();++i)if(auto sdr=Handle(StepShape_ShapeDefinitionRepresentation)::DownCast(model->Value(i));!sdr.IsNull()){
      auto pds=Handle(StepRepr_ProductDefinitionShape)::DownCast(sdr->Definition().PropertyDefinition());
      if(!pds.IsNull()&&!pds->Definition().ProductDefinition().IsNull()&&!Handle(StepShape_ShapeRepresentation)::DownCast(sdr->UsedRepresentation()).IsNull())product_reps[id(pds->Definition().ProductDefinition())].push_back(sdr->UsedRepresentation());}
    auto mapped_items=[&](const Handle(StepBasic_ProductDefinition)& p){std::vector<Mapped> out;const U pi=id(p);
      for(const auto& rep:product_reps[pi])
        for(int k=1;k<=rep->NbItems();++k){auto item=Handle(StepRepr_MappedItem)::DownCast(rep->ItemsValue(k));if(item.IsNull())continue;
          const std::string where="mapped item #"+std::to_string(id(item))+" of product #"+std::to_string(pi);
          auto map=item->MappingSource();if(map.IsNull()||map->MappedRepresentation().IsNull())throw std::runtime_error(where+" has no mapped representation");
          auto mapped=map->MappedRepresentation();if(representations.sdr_used(mapped))throw std::runtime_error(where+" maps the representation of a product, which this exporter does not read (bead .37.26)");
          std::vector<U> held;bool nested=false;for(int j=1;j<=mapped->NbItems();++j){auto e=mapped->ItemsValue(j);
            if(!Handle(StepRepr_MappedItem)::DownCast(e).IsNull())nested=true;
            if(auto s=Handle(StepShape_ManifoldSolidBrep)::DownCast(e);!s.IsNull())held.push_back(id(s));}
          if(nested)throw std::runtime_error(where+" maps a representation that holds a further mapped item; nested mapped items are not read");
          // A mapped item that places no solid (construction or presentation
          // geometry) is no solid occurrence; it is recorded and not read.
          if(held.empty()){record("reference_mapped_item_without_solid",0,{pi,id(item),id(mapped)},"mapped item places no solid; not an occurrence");continue;}
          if(held.size()!=1)throw std::runtime_error(where+" maps a representation holding "+std::to_string(held.size())+" solids; exactly one is read");
          auto target=Handle(StepGeom_Axis2Placement3d)::DownCast(item->MappingTarget());auto origin=Handle(StepGeom_Axis2Placement3d)::DownCast(map->MappingOrigin());
          if(target.IsNull()||origin.IsNull())throw std::runtime_error(where+" is placed by a cartesian_transformation_operator or a non-3D placement; only axis2_placement_3d pairs are read raw");
          const auto m=placements::frames(placements::frame(origin,where+" origin"),placements::frame(target,where+" target"));
          out.push_back({id(item),id(mapped),held[0],text(item->Name()),D(m.begin(),m.end()),mapped,rep});}
      return out;};
    // Every usage's raw placement; the reader refuses by name a usage with no
    // placing relation or several, an untransformed placing relation, and a
    // relation that does not orient its usage, as the adapter does.
    const placements::Reader raw_reader(model);std::set<std::string> corrections;U raw_placements=0;
    std::vector<Bytes> mapped_usages;U mapped_products=0;
    // The one placement-agreement predicate (native/step_placements.hpp agree,
    // review round 1 item 8), which the STEP source adapter calls too: the
    // 2^-32 relative rule, translation within the placing contexts' resolution,
    // and a rotation difference moving no point of the child's extent by more
    // than it. The extent must bound the child's points. OCCT's box does not:
    // a face whose edges carry no pcurves is boxed over its whole unbounded
    // carrier, and one with pcurves over their parameter box only, which
    // misses a cone's apex beyond a base circle. So each original solid is
    // bounded by native/shape_extent.hpp shape_bound (B2 .37.39), in both
    // modes: each face by its carrier and 3D edges (a plane by its edges, a
    // cylinder or cone over its edges' axial range, a cone with its apex, a
    // sphere, torus or B-spline surface whole or by its poles). A carrier it
    // cannot bound makes the solid unbounded, so a rotation difference over it
    // never agrees and its placement is written raw. A product's
    // extent in its own frame is its solid's box, and each child's extent
    // carried in by the raw placement of its usage or mapped item, as the
    // adapter accumulates it.
    std::map<U,Bnd_Box> solid_boxes,product_boxes;std::set<U> box_open;std::map<U,std::vector<std::pair<U,D>>> mapped_children;
    auto solid_box=[&](U si)->const Bnd_Box&{if(auto found=solid_boxes.find(si);found!=solid_boxes.end())return found->second;Bnd_Box box;
      try{box=brepgpu::shape_extent::shape_bound(solids.at(si));}catch(const Standard_Failure&){box.SetWhole();}
      return solid_boxes.emplace(si,box).first->second;};
    auto carried=[](const Bnd_Box& box,const auto& m,Bnd_Box& into){if(box.IsVoid())return;if(box.IsWhole()||box.IsOpen()){into.SetWhole();return;}double x[2],y[2],z[2];box.Get(x[0],y[0],z[0],x[1],y[1],z[1]);
      for(int a=0;a<2;++a)for(int b=0;b<2;++b)for(int c=0;c<2;++c){const double q[3]={x[a],y[b],z[c]};double r[3];for(int i=0;i<3;++i)r[i]=m[4*i]*q[0]+m[4*i+1]*q[1]+m[4*i+2]*q[2]+m[4*i+3];into.Add(gp_Pnt(r[0],r[1],r[2]));}};
    auto extent_of=[](const Bnd_Box& box){placements::Extent extent;if(box.IsVoid())return extent;extent.empty=false;
      if(box.IsWhole()||box.IsOpen()){extent.lo.fill(-INFINITY);extent.hi.fill(INFINITY);return extent;}
      box.Get(extent.lo[0],extent.lo[1],extent.lo[2],extent.hi[0],extent.hi[1],extent.hi[2]);return extent;};
    auto agreement_values=[](const placements::Agreement& a){std::ostringstream o;o<<std::setprecision(17)<<"max relative entry difference "<<a.relative<<", max translation difference "<<a.translation_mm<<" mm, max point difference over the child's extent "<<a.moved_mm<<" mm, resolution "<<a.resolution_mm<<" mm";return o.str();};
    for(auto& p:products){U pi=id(p);if(!children.count(pi))roots.push_back(pi);auto ps=shape(p);
      auto product=p->Formation()->OfProduct();std::string name=text(product->Description());if(name.empty())name=text(product->Name());
      U style=0,definition=0;
      if(!parents.count(pi)){const auto mapped=mapped_items(p);
        if(mapped.empty()){std::set<U> matches;for(TopExp_Explorer ex(ps,TopAbs_SOLID);ex.More();ex.Next())for(auto& s:solids)if(ex.Current().IsSame(s.second))matches.insert(s.first);
          if(matches.size()!=1)throw std::runtime_error("leaf product #"+std::to_string(pi)+" does not bind exactly one original solid");
          U si=*matches.begin();if(solid_products.count(si))throw std::runtime_error("original solid has multiple leaf products #"+std::to_string(si));product_solids[pi]=si;solid_products[si]=pi;definition=si;}
        else{
          // OCCT as evaluator: each mapped item has one located copy of its
          // solid in the product's shape, and the product holds no solid
          // directly. Each item is paired with the unused copy of its solid
          // nearest its raw placement; where that copy agrees to the adapter's
          // grade OCCT's value is written, otherwise the raw one with a record
          // and the declared correction P2 (G1 authority rule). The written
          // value is within the grade of the raw one either way.
          std::vector<std::pair<U,gp_Trsf>> located;
          for(TopExp_Explorer ex(ps,TopAbs_SOLID);ex.More();ex.Next()){bool found=false;for(auto& s:solids)if(ex.Current().IsPartner(s.second)){located.push_back({s.first,ex.Current().Location().Transformation()});found=true;}
            if(!found)throw std::runtime_error("product #"+std::to_string(pi)+" holds a solid that is no original solid");}
          if(located.size()!=mapped.size())throw std::runtime_error("product #"+std::to_string(pi)+" holds "+std::to_string(located.size())+" solids where its mapped items place "+std::to_string(mapped.size())+" (a solid held beside mapped items is not read)");
          std::vector<bool> used(located.size(),false);
          for(const auto& m:mapped){std::size_t best=located.size();double nearest=0;placements::Matrix raw{};std::copy(m.transform.begin(),m.transform.end(),raw.begin());
            for(std::size_t k=0;k<located.size();++k){if(used[k]||located[k].first!=m.solid)continue;const auto occt=matrix(located[k].second);std::array<double,12> o{};std::copy(occt.begin(),occt.end(),o.begin());
              const double d=placements::placement_difference(raw,o);if(best==located.size()||d<nearest){best=k;nearest=d;}}
            if(best==located.size())throw std::runtime_error("mapped item #"+std::to_string(m.item)+" of product #"+std::to_string(pi)+" has no located copy of its solid in OCCT's shape");
            used[best]=true;D written=matrix(located[best].second);
            // The nearest copy pairs the item; the agreement predicate decides
            // what is written. The resolution is that of the two frames'
            // contexts: the mapped representation (origin) and the holder (target).
            {placements::Placement frames;frames.rep1=m.mapped;frames.rep2=m.holder;std::array<double,12> occt{};std::copy(written.begin(),written.end(),occt.begin());
             const auto agreement=placements::agree(raw,occt,placements::placement_resolution(model,frames),extent_of(solid_box(m.solid)));
             if(!agreement.agrees){written=m.transform;corrections.insert("P2");++raw_placements;
               record("reference_placement_raw",1,{m.item,pi,m.rep},"OCCT's located placement of this mapped item differs from the raw one (target o origin^-1; OCCT forms origin^-1 o target, STEPControl_ActorRead.cxx:2488): "+agreement_values(agreement)+"; the raw placement is written (G1 P2)");
               std::cout<<"reference_placement_raw\t"<<m.item<<'\t'<<std::setprecision(17)<<agreement.relative<<'\t'<<agreement.translation_mm<<'\t'<<agreement.moved_mm<<'\t'<<agreement.resolution_mm<<"\n";}}
            mapped_children[pi].push_back({m.rep,m.transform});
            if(solid_products.count(m.solid)&&solid_products.at(m.solid)!=m.rep)throw std::runtime_error("original solid has multiple leaf products #"+std::to_string(m.solid));
            if(!solid_products.count(m.solid)){solid_products[m.solid]=m.rep;product_solids[m.rep]=m.solid;
              std::set<U> part_reps;for(const auto& holder:representations.holders(raw_solids.at(m.solid)))for(const auto& rep:representations.transfer_path(holder))part_reps.insert(units::label(model,rep));
              Bytes w;w.u64(m.rep);w.u64(m.solid);w.u64(style_resolver.product_style(m.rep,part_reps,m.solid));w.text(name);w.text(text(product->Id()));tables["PRODUCTS"].push_back(w);++mapped_products;}
            Bytes w;w.u64(m.item);w.u64(pi);w.u64(m.rep);w.u64(0);w.text(m.name);w.doubles(written);mapped_usages.push_back(w);
            record("reference_mapped_item_occurrence",0,{pi,m.item,m.rep,m.solid},"a solid placed through a mapped item is published as a usage of its mapped representation");}}}
      std::set<U> part_reps;if(definition)for(const auto& holder:representations.holders(raw_solids.at(definition)))for(const auto& rep:representations.transfer_path(holder))part_reps.insert(units::label(model,rep));
      style=style_resolver.product_style(pi,part_reps,definition);TDF_Label label;shape_tool->Search(ps,label,true,true,false);
      if(!label.IsNull()){Handle(TDataStd_TreeNode) node;if(label.FindAttribute(XCAFDoc::MaterialRefGUID(),node)&&node->HasFather()){
        Handle(TCollection_HAsciiString) material_name,description,unit,type;double density=0;
        if(XCAFDoc_MaterialTool::GetMaterial(node->Father()->Label(),material_name,description,density,unit,type)){Style d;if(style)d=styles.at(style);d.id=(U(1)<<62)|pi;d.sources.push_back(pi);d.material=text(material_name);d.unit=text(unit)+" "+text(type);d.density=density;d.density_set=true;styles[d.id]=d;style=d.id;}}}
      Bytes w;w.u64(pi);w.u64(definition);w.u64(style);w.text(name);w.text(text(product->Id()));tables["PRODUCTS"].push_back(w);
    }
    if(roots.size()!=1||solid_products.size()!=solids.size())throw std::runtime_error("ambiguous root or unowned original solid");
    // Every other solid-bearing mapped item in a representation OCCT transfers
    // places a solid this export does not publish (another product's
    // occurrence outside every usage, or one reached through related shape
    // representation relationships): refused by name, as the STEP source
    // adapter refuses them (native/step_placements.hpp mapped_links and
    // reaches_solid; its refuse_unpublished_mapped_items would also refuse the
    // leaf occurrences published above, bead .37.25).
    {std::set<U> published;for(const auto& w:mapped_usages){U item=0;for(int k=0;k<8;++k)item|=U(w.v[k])<<(8*k);published.insert(item);}
     for(int i=1;i<=model->NbEntities();++i){auto item=Handle(StepRepr_MappedItem)::DownCast(model->Value(i));if(item.IsNull()||published.count(id(item)))continue;
       auto users=entity_graph.Sharings(item);for(users.Start();users.More();users.Next()){auto holder=Handle(StepShape_ShapeRepresentation)::DownCast(users.Value());if(holder.IsNull()||!representations.transferred(holder))continue;
         bool held=false;for(int k=1;k<=holder->NbItems()&&!held;++k)held=holder->ItemsValue(k)==item;if(!held)continue;
         std::vector<placements::MappedLink> own;for(const auto& link:placements::mapped_links(entity_graph,holder))if(link.item==item)own.push_back(link);
         if(placements::reaches_solid(entity_graph,own))throw std::runtime_error("mapped item #"+std::to_string(id(item))+" in representation #"+std::to_string(id(holder))+" places a solid that this export does not publish as an occurrence (OCCT translates it as geometry of the holder's product); refused rather than dropped");}}}
    // Usage placements read raw (bead .37.33), as the STEP source adapter reads
    // them: OCCT's located value is written where it agrees with the raw one
    // to the adapter's grade, so the arm is uncorrected there; elsewhere the
    // raw value, a record, and the declared correction (P1 a cartesian
    // operator, P2 an item-defined transformation).
    std::function<const Bnd_Box&(U)> product_box=[&](U pi)->const Bnd_Box&{if(auto found=product_boxes.find(pi);found!=product_boxes.end())return found->second;
      if(!box_open.insert(pi).second)throw std::runtime_error("cyclic product structure at "+std::to_string(pi));
      Bnd_Box box;static const std::array<double,12> identity{1,0,0,0,0,1,0,0,0,0,1,0};
      if(auto solid=product_solids.find(pi);solid!=product_solids.end())carried(solid_box(solid->second),identity,box);
      for(auto& v:usages)if(id(v->RelatingProductDefinition())==pi)carried(product_box(id(v->RelatedProductDefinition())),raw_reader.at(id(v)).matrix,box);
      if(auto mapped=mapped_children.find(pi);mapped!=mapped_children.end())for(const auto& child:mapped->second)carried(product_box(child.first),child.second,box);
      box_open.erase(pi);return product_boxes.emplace(pi,box).first->second;};
    for(auto& u:usages){const auto& raw=raw_reader.at(id(u));const D located=matrix(shape(u).Location().Transformation());std::array<double,12> occt{};std::copy(located.begin(),located.end(),occt.begin());
      const auto agreement=placements::agree(raw.matrix,occt,placements::placement_resolution(model,raw),extent_of(product_box(id(u->RelatedProductDefinition()))));D written=located;
      if(!agreement.agrees){written.assign(raw.matrix.begin(),raw.matrix.end());corrections.insert(raw.cartesian?"P1":"P2");++raw_placements;
        record("reference_placement_raw",1,{id(u),raw.cdsr,raw.relation,raw.operator_id},std::string("OCCT's located placement differs from the raw one read from the ")+(raw.cartesian?"cartesian_transformation_operator_3d (ISO 10303-42 base_axis; G1 P1)":"item_defined_transformation as transform_item_2 composed with the inverse of transform_item_1 (PDM Schema Usage Guide Example 14; G1 P2)")+": "+agreement_values(agreement)+"; the raw placement is written");
        std::cout<<"reference_placement_raw\t"<<id(u)<<'\t'<<std::setprecision(17)<<agreement.relative<<'\t'<<agreement.translation_mm<<'\t'<<agreement.moved_mm<<'\t'<<agreement.resolution_mm<<"\n";}
      Bytes w;w.u64(id(u));w.u64(id(u->RelatingProductDefinition()));w.u64(id(u->RelatedProductDefinition()));w.u64(0);w.text(text(u->Name()));w.doubles(written);tables["USAGES__"].push_back(w);}
    std::cout<<"reference_placements_raw\t"<<raw_placements<<"\n";
    for(auto& w:mapped_usages)tables["USAGES__"].push_back(w);
    for(auto& s:styles)tables["STYLES__"].push_back(s.second.bytes());
    const auto graph_end=Clock::now();
    std::filesystem::path directory(output_path);
    if(!std::filesystem::create_directory(directory))throw std::runtime_error("reference output directory must be new");
    // Face sides (beads .37.35, .37.36). OCCT's default read is the candidate
    // finder and the geometry: a face whose use in its processed solid differs
    // from the raw use (same_sense composed with its shell) is checked against
    // the raw surface (OCCT's translation without ShapeProcess) by
    // native/reference_source/orientation_verdict.hpp. This mode's own
    // transfer serves where it is that read; the other read is made here.
    // A face whose raw use points into the material (by ray parity) is written
    // with its outward side in both modes: S1 when its own bound runs against
    // it, S2 when its whole shell is inside out. A face whose side is not
    // decidable refuses the export in both modes, as ingress (review round 2
    // ruling; the harness reads an exporter refusal as exit 4, ours). A face
    // the default read turned against its shell refuses default mode (NVIDIA
    // would read it inward); translation-only writes its raw use, which is
    // decided outward, with a record. Translation-only also corrects a face
    // whose raw use is inward where the default read left a piece inward.
    namespace ro=brepgpu::reference_orientation;
    std::set<U> inward,reversed_flag;std::map<U,std::string> letters;U candidates_seen=0,repaired_faces=0,reversed_together=0,turned_faces=0;std::string first_refusal;std::map<U,bool> bound_agrees;std::map<std::pair<U,U>,U> shell_agreeing;
    {
      std::unique_ptr<STEPControl_Reader> other;Handle(Transfer_TransientProcess) processed=process;std::map<U,Handle(Standard_Transient)> by_label;
      auto read_other=[&](bool with_processing){other=std::make_unique<STEPControl_Reader>();if(other->ReadFile(input_path)!=IFSelect_RetDone)throw std::runtime_error("original STEP re-read for the face-side check failed");
        other->SetSystemLengthUnit(1.0);if(!with_processing)other->SetShapeProcessFlags(ShapeProcess::OperationsFlags());placements::refuse_unbounded_transfer(other->StepModel(),other->WS()->Graph());other->TransferRoots();by_label.clear();
        const auto m=other->StepModel();for(int i=1;i<=m->NbEntities();++i){const int label=m->IdentLabel(m->Value(i));if(label>0&&(!Handle(StepShape_ManifoldSolidBrep)::DownCast(m->Value(i)).IsNull()||!Handle(StepShape_AdvancedFace)::DownCast(m->Value(i)).IsNull()))by_label[U(label)]=m->Value(i);}
        return other->WS()->TransferReader()->TransientProcess();};
      if(!processing)processed=read_other(true);
      auto in_processed=[&](U label,const Handle(Standard_Transient)& own)->Handle(Standard_Transient){if(processing)return own;auto f=by_label.find(label);return f==by_label.end()?Handle(Standard_Transient)():f->second;};
      struct Candidate{U si=0,fi=0,ordinal=0;TopAbs_Orientation used=TopAbs_FORWARD;Handle(StepShape_AdvancedFace) f;std::vector<std::pair<TopoDS_Face,TopAbs_Orientation>> pieces;TopoDS_Shape solid;};
      std::vector<Candidate> candidates;std::map<std::pair<U,U>,std::pair<U,U>> shell_counts;
      for(auto& entry:raw_solids){const U si=entry.first;const auto ent=in_processed(si,entry.second);const auto solid=ent.IsNull()?TopoDS_Shape():TransferBRep::ShapeResult(processed,ent);
        // Coordinator ruling (review round 2): a definition whose face sides
        // cannot be checked refuses as ingress, as an undecided face does;
        // NVIDIA never receives an unverified side. (Default mode refuses it
        // already: its default read is not one solid.)
        if(solid.IsNull()||solid.ShapeType()!=TopAbs_SOLID){
          raw_shells(entry.second,false); // a raw rule is named first
          // Named from the geometry where it is a shell left open: a shell of
          // the translation that is not closed (an edge used once; a trimmed
          // band's sole bound closes nothing) bounds no material at all.
          if(!processing){bool open=false;for(TopExp_Explorer sh(solids.at(si),TopAbs_SHELL);sh.More();sh.Next())open|=!BRep_Tool::IsClosed(sh.Current());
            throw std::runtime_error("definition #"+std::to_string(si)+(open?": a shell of OCCT's translation is not closed, so it bounds no material and":": OCCT's default read does not make it one solid, so")+" its face sides cannot be checked against geometry and NVIDIA would receive an unverified side; the export refuses and no reference source is written");}
          continue;}
        NCollection_DataMap<TopoDS_Shape,TopAbs_Orientation,TopTools_ShapeMapHasher> in_solid;for(TopExp_Explorer e(solid,TopAbs_FACE);e.More();e.Next())if(!in_solid.IsBound(e.Current()))in_solid.Bind(e.Current(),e.Current().Orientation());
        for(const auto& s:raw_shells(entry.second,false))for(int i=1;i<=s.faces->NbCfsFaces();++i){auto f=Handle(StepShape_AdvancedFace)::DownCast(s.faces->CfsFacesValue(i));if(f.IsNull())continue;const U fi=id(f);
          ++shell_counts[{si,s.ordinal}].first;const auto used=f->SameSense()==s.forward?TopAbs_FORWARD:TopAbs_REVERSED;
          const auto fe=in_processed(fi,f);const auto r=fe.IsNull()?TopoDS_Shape():TransferBRep::ShapeResult(processed,fe);if(r.IsNull())continue;
          Candidate c;c.si=si;c.fi=fi;c.ordinal=s.ordinal;c.used=used;c.f=f;c.solid=solid;bool differs=false;
          for(TopExp_Explorer e(r,TopAbs_FACE);e.More();e.Next()){if(!in_solid.IsBound(e.Current()))continue;const auto o=in_solid.Find(e.Current());c.pieces.push_back({TopoDS::Face(e.Current()),o});if(o!=used)differs=true;}
          if(differs)candidates.push_back(c);}}
      candidates_seen=candidates.size();
      // The raw surface: this transfer's face in translation-only mode, a
      // translation-only re-read in default mode (only when a face differs).
      Handle(Transfer_TransientProcess) raw_process=process;
      if(processing&&!candidates.empty())raw_process=read_other(false);
      for(const auto& c:candidates){Handle(Standard_Transient) raw_entity=Handle(Standard_Transient)(c.f);if(processing){auto found=by_label.find(c.fi);raw_entity=found==by_label.end()?Handle(Standard_Transient)():found->second;}
        const auto t=raw_entity.IsNull()?TopoDS_Shape():TransferBRep::ShapeResult(raw_process,raw_entity);
        std::string verdict="reversed_together";
        if(t.IsNull()||t.ShapeType()!=TopAbs_FACE)verdict="undecidable: OCCT's translation without ShapeProcess gives no face to read its raw surface from";
        else{const Handle(Geom_Surface) raw_surface=BRep_Tool::Surface(TopoDS::Face(t));
          verdict=ro::face_verdict(c.pieces,raw_surface,c.used==TopAbs_REVERSED?-1.:1.,c.solid);}
        std::cout<<"default_read_face_verdict\t"<<c.si<<'\t'<<c.fi<<'\t'<<verdict<<'\t'<<c.pieces.size()<<" pieces\n";
        if(verdict==ro::repaired||(!processing&&verdict==ro::left_inward)){
          // S1 or S2 from the face's own bound (dispatcher ruling: never from
          // counting): its outer bound against the face is S1; a face that
          // agrees with its bound can only be part of a whole shell inside out.
          const int contradicts=ro::bound_contradicts(TopoDS::Face(t));
          if(contradicts<0)verdict="undecidable: the face's outer bound cannot be classified against the face";
          else{inward.insert(c.fi);bound_agrees[c.fi]=contradicts==0;++shell_counts[{c.si,c.ordinal}].second;if(contradicts==0)++shell_agreeing[{c.si,c.ordinal}];++repaired_faces;continue;}}
        if(verdict=="reversed_together"){reversed_flag.insert(c.fi);++reversed_together;record("default_read_face_flag_and_surface_reversed",0,{c.si,c.fi},"OCCT's default read reversed this face's orientation flag and its surface together; its material side is the raw use's");continue;}
        const bool decided=verdict.rfind("turned",0)==0;if(decided)++turned_faces;
        if(processing||!decided){if(first_refusal.empty())first_refusal="original face #"+std::to_string(c.fi)+" of definition #"+std::to_string(c.si)+": "+verdict;}
        else record("reference_face_side_kept",0,{c.si,c.fi},"OCCT's default read would turn this face ("+verdict+"); its raw use is decided outward, and the translation-only export writes it");}
      for(const auto& c:candidates){if(!inward.count(c.fi))continue;
        if(!bound_agrees.at(c.fi)){letters[c.fi]="S1";continue;}
        // S2: every face of the shell inward, and every one agreeing with its bound.
        const auto& n=shell_counts.at({c.si,c.ordinal});
        if(n.second==n.first&&shell_agreeing[{c.si,c.ordinal}]==n.first){letters[c.fi]="S2";continue;}
        inward.erase(c.fi);--repaired_faces;const std::string verdict="undecidable: the face agrees with its own bound, but its shell is not inside out as a whole";
        std::cout<<"default_read_face_verdict\t"<<c.si<<'\t'<<c.fi<<'\t'<<verdict<<"\n";
        if(first_refusal.empty())first_refusal="original face #"+std::to_string(c.fi)+" of definition #"+std::to_string(c.si)+": "+verdict;}
    }
    std::cout<<"default_read_reoriented_faces\t"<<candidates_seen<<"\ndefault_read_flag_and_surface_reversed\t"<<reversed_together<<"\ndefault_read_repaired_faces\t"<<repaired_faces<<"\ndefault_read_turned_faces\t"<<turned_faces<<"\n";
    if(!first_refusal.empty())throw std::runtime_error(first_refusal+(processing?"; OCCT's default read would hand NVIDIA this definition with a face whose material side is not decided or is inward":"; its material side is not decided, so its raw use cannot be written as an outward side")+", so the export refuses and no reference source is written ("+std::to_string(turned_faces)+" faces turned against the shell)");
    // verification.tsv: each verified definition (a written correction or a
    // repaired side), whether it verified, and the largest edge tolerance the
    // verification copy raised to cover a pcurve it added (mm; 0 when none).
    std::vector<std::string> verification_rows;
    U total_faces=0,split_faces=0,corrected_faces=0;
    std::map<U,std::vector<TopoDS_Shape>> face_pieces;
    for(auto& entry:raw_solids){U si=entry.first;
      if(solids.at(si).ShapeType()!=TopAbs_SOLID||!solids.at(si).Location().IsIdentity()||solids.at(si).Orientation()!=TopAbs_FORWARD)
        throw std::runtime_error("reference definition #"+std::to_string(si)+" requires one identity-located forward solid root");
      // The raw shells first, so a raw rule is named before OCCT's result is read.
      const auto shells=raw_shells(entry.second,true);U nfaces=0;
      // Translation-only: a face whose raw use points into the material (the
      // pre-pass above) is written with its outward side (G1 authority rule
      // with the raw entity's own evidence, bead .37.36). S1, one face against
      // its own bound: the face is turned and its wires reversed inside it, so
      // every edge keeps its use in the shell. S2, a whole shell inside out:
      // each face is turned as it is.
      bool sided=false;for(const auto& s:shells)for(int i=1;i<=s.faces->NbCfsFaces()&&!sided;++i)sided=inward.count(id(s.faces->CfsFacesValue(i)))>0;
      auto serialize=[](const TopoDS_Shape& shape,const std::filesystem::path& path){BinTools_ShapeSet set;set.SetFormatNb(BinTools_FormatVersion_VERSION_4);set.SetWithTriangles(false);set.SetWithNormals(false);set.Add(shape);
        std::ofstream out(path,std::ios::binary);if(!out)throw std::runtime_error("cannot write "+path.filename().string());set.Write(out);set.Write(shape,out);out.close();if(!out)throw std::runtime_error("write failed: "+path.filename().string());};
      // The uncorrected translation of a definition this export corrects, as
      // the minimal NVIDIA ingress writes it (same OCCT translation, same
      // BinTools settings): the harness requires it byte-identical to that
      // ingress's file before it hands NVIDIA the corrected one, so the only
      // bytes that differ from NVIDIA's own translation are the declared
      // correction (review round 2 ruling).
      if(!processing&&sided){std::filesystem::create_directories(directory/"uncorrected");serialize(solids.at(si),directory/"uncorrected"/("definition-"+std::to_string(si)+".brep"));}
      BRepTools_ReShape reshape;bool reshaped=false;std::vector<TopoDS_Shape> corrected_translated;
      if(!processing)for(const auto& s:shells)for(int i=1;i<=s.faces->NbCfsFaces();++i){auto f=Handle(StepShape_AdvancedFace)::DownCast(s.faces->CfsFacesValue(i));if(f.IsNull()||!inward.count(id(f)))continue;
        const auto t=TransferBRep::ShapeResult(process,f);if(t.IsNull()||t.ShapeType()!=TopAbs_FACE)throw std::runtime_error("original face #"+std::to_string(id(f))+" has no translated face to correct");
        const TopoDS_Shape base=t.Oriented(TopAbs_FORWARD);
        // A new face of the same surface: S1 holds each wire reversed, S2 each as it is.
        const bool s1=letters.at(id(f))!="S2";TopoDS_Shape copy=base.EmptyCopied();BRep_Builder builder;
        for(TopoDS_Iterator it(base);it.More();it.Next())builder.Add(copy,s1?it.Value().Reversed():it.Value());
        reshape.Replace(base,copy.Oriented(TopAbs_REVERSED));reshaped=true;corrected_translated.push_back(t);}
      if(reshaped)solids.at(si)=reshape.Apply(solids.at(si));
      // Each seam use of a corrected face on its own pcurve (review round 2):
      // reversing an S1 face's wires inside it moves each seam use to the
      // seam's other pcurve; the pairing under which every wire closes in the
      // parameter plane is the one OCCT's default read gives the repaired face.
      for(const auto& t:corrected_translated){const TopoDS_Shape now=reshape.Value(t);bool paired=false,found=false;
        for(TopExp_Explorer e(solids.at(si),TopAbs_FACE);e.More()&&!found;e.Next())if(e.Current().IsSame(now)){found=true;paired=ro::pair_seams(TopoDS::Face(e.Current()));}
        if(!found||!paired)throw std::runtime_error("definition #"+std::to_string(si)+": a corrected face has no pairing of its seam pcurves under which its wires close, so the export refuses and no reference source is written");}
      // A written correction (translation-only) or a repaired side (default) is
      // verified on the published bytes, read back (review round 2 ruling),
      // after the file is written and before it is published.
      auto verify_written=[&](const std::filesystem::path& path){if(!sided)return;
       TopoDS_Shape written;if(!BinTools::Read(written,path.string().c_str()))written.Nullify();
       ro::Completion completion;
       // The cap on a raised tolerance: the definition's usable declared
       // uncertainty, else the adapter's source residual grade.
       const double cap=usable_uncertainty[si]?*usable_uncertainty[si]:brepgpu::SourceUVOptions{}.source_residual_mm;
       {const auto failure=written.IsNull()?std::string("the written file does not read back"):ro::verify_solid(written,&completion,cap);
        std::ostringstream row;row<<std::setprecision(17)<<si<<'\t'<<(processing?"default":"translation-only")<<'\t'<<(failure.empty()?1:0)<<'\t'<<completion.largest_raised_tolerance_mm<<'\t'<<completion.largest_added_deviation_mm<<'\t'<<cap;verification_rows.push_back(row.str());
         if(!failure.empty())throw std::runtime_error("definition #"+std::to_string(si)+(processing?": OCCT's default read with its repaired faces":": the corrected face sides")+" fail verification ("+failure+"), so the export refuses and no reference source is written");}};
      const auto& solid=solids.at(si);
      BinTools_ShapeSet set;set.SetFormatNb(BinTools_FormatVersion_VERSION_4);set.SetWithTriangles(false);set.SetWithNormals(false);set.Add(solid);
      std::string filename="definition-"+std::to_string(si)+".brep";
      // Translated faces of the exported solid by storage index, with their
      // orientation as used in the solid.
      std::map<int,TopAbs_Orientation> actual;
      for(TopExp_Explorer e(solid,TopAbs_FACE);e.More();e.Next()){
        auto f=TopoDS::Face(e.Current());if(BRep_Tool::Surface(f).IsNull())throw std::runtime_error("triangulation-only face in definition #"+std::to_string(si));
        int index=set.Index(f.Located(TopLoc_Location()));if(index<=0||actual.count(index))throw std::runtime_error("duplicate or unmapped translated face in definition #"+std::to_string(si));
        actual[index]=f.Orientation();}
      {U translated_shells=0;for(TopExp_Explorer e(solid,TopAbs_SHELL);e.More();e.Next())++translated_shells;
       if(translated_shells!=shells.size())throw std::runtime_error("definition #"+std::to_string(si)+" translated shells "+std::to_string(translated_shells)+" differ from original "+std::to_string(shells.size()));
       for(const auto& s:shells)nfaces+=U(s.faces->NbCfsFaces());}
      Bytes row;row.u64(si);row.u64(solid_products.at(si));row.text(filename);row.u64(actual.size());row.u64(nfaces);
      std::set<int> claimed;
      for(const auto& s:shells)for(int i=1;i<=s.faces->NbCfsFaces();++i){auto f=Handle(StepShape_AdvancedFace)::DownCast(s.faces->CfsFacesValue(i));
        if(f.IsNull())throw std::runtime_error("non-advanced original face in definition #"+std::to_string(si));
        U fi=id(f);auto translated=TransferBRep::ShapeResult(process,f);const bool turned_in=inward.count(fi)>0;
        if(reshaped&&turned_in)translated=reshape.Value(translated);
        const auto raw_built=f->SameSense()?TopAbs_FORWARD:TopAbs_REVERSED,used=f->SameSense()==s.forward?TopAbs_FORWARD:TopAbs_REVERSED;
        // The side written: the raw use, or its opposite where the raw use points into the material.
        const auto built=(turned_in&&!processing)?TopAbs::Reverse(raw_built):raw_built,written=turned_in?TopAbs::Reverse(used):used;
        // translation-only: one translated face as built, used in the solid
        // as same_sense composed with its shell. default: OCCT's processed
        // result, a face or the pieces FixShape split it into, every piece a
        // face of the exported solid that no other original face claims.
        std::vector<TopoDS_Shape> pieces;
        if(translated.IsNull()){}else if(translated.ShapeType()==TopAbs_FACE)pieces.push_back(translated);else if(processing)for(TopExp_Explorer e(translated,TopAbs_FACE);e.More();e.Next())pieces.push_back(e.Current());
        if(pieces.empty())throw std::runtime_error("original face #"+std::to_string(fi)+" has no translated face in definition #"+std::to_string(si)+(processing?" after OCCT's default read":""));
        if(!processing&&translated.Orientation()!=built)throw std::runtime_error("translated result face orientation disagrees with original #"+std::to_string(fi));
        std::vector<U> storage;
        for(const auto& piece:pieces){int index=set.Index(piece.Located(TopLoc_Location()));
          if(index<=0||!actual.count(index)||!claimed.insert(index).second)throw std::runtime_error("original face #"+std::to_string(fi)+" does not uniquely bind exported solid");
          // Only a face the pre-pass found reversed together with its surface
          // may carry another flag than the side written.
          if(actual.at(index)!=written&&!(processing&&reversed_flag.count(fi)))throw std::runtime_error("exported solid face orientation disagrees with original #"+std::to_string(fi));
          storage.push_back(U(set.NbShapes()-index+1));}
        if(turned_in){const auto& letter=letters.at(fi);
          if(processing){record("default_read_face_repaired",1,{si,fi},"the raw use's outward normal (same_sense composed with its shell) points into the material, by ray parity ("+letter+"); OCCT's default read turned the face to agree with its closed shell; the exported face is OCCT's and the face row writes that side");std::cout<<"default_read_face_repaired\t"<<si<<'\t'<<fi<<'\t'<<letter<<"\n";}
          else{corrections.insert(letter);++corrected_faces;record("reference_face_orientation_raw",1,{si,fi},std::string("the raw use's outward normal points into the material, by ray parity; ")+(letter=="S2"?"its whole shell is inside out (S2), so the face is turned":"the face is turned against its raw same_sense with its wires reversed inside it (S1), so its edges keep their uses")+"; the corrected side is written");std::cout<<"reference_face_orientation_corrected\t"<<si<<'\t'<<fi<<'\t'<<letter<<"\n";}}
        if(pieces.size()>1){++split_faces;record("default_read_face_split",0,{si,fi},"OCCT's default read split this original face; its translated faces are the face's storage indices in DEFS____");}
        face_pieces[fi]=pieces;
        row.u64(fi);row.u64(id(f->FaceGeometry()));row.i32(written==TopAbs_FORWARD?1:-1);row.u32(std::uint32_t(s.ordinal));row.u64(storage.size());for(U k:storage)row.u64(k);
        Bytes face_style;face_style.u64(fi);face_style.u64(style_resolver.item_style(fi));tables["FACE_STY"].push_back(face_style);++total_faces;
      }
      if(claimed.size()!=actual.size())throw std::runtime_error("exported solid face inventory differs from original #"+std::to_string(si)+": "+std::to_string(actual.size()-claimed.size())+" translated faces come from no original face");
      auto temporary=directory/(filename+".tmp");std::ofstream out(temporary,std::ios::binary);if(!out)throw std::runtime_error("cannot write definition #"+std::to_string(si));
      set.Write(out);set.Write(solid,out);out.flush();if(!out||out.tellp()>std::streamoff(256)*1024*1024)throw std::runtime_error("definition write failed or exceeds official256MiB limit #"+std::to_string(si));out.close();if(!out)throw std::runtime_error("definition close failed #"+std::to_string(si));
      try{verify_written(temporary);}catch(...){std::filesystem::remove(temporary);throw;}
      std::filesystem::rename(temporary,directory/filename);tables["DEFS____"].push_back(row);
    }
    std::cout<<"default_read_split_faces\t"<<split_faces<<"\nreference_faces_orientation_corrected\t"<<corrected_faces<<"\n";
    // Vertex-loop faces (bead .37.23). translation-only: the adapter's rule
    // (native/step_vertex_loops.hpp) on OCCT's face as translated; a face the
    // adapter would refuse is recorded with its reason. default: OCCT's
    // processing rebuilt the face, so the rule (which reads OCCT's translation)
    // is not re-derived here; the face's admission is the adapter's, which the
    // harness reads before NVIDIA's ingress. In both, BRepCheck on each
    // exported piece: OCCT's translation without ShapeProcess builds no seam
    // and no pcurves for such a face (UnorientableShape, NotClosed), its
    // default read does; an invalid one is recorded.
    U refused_exported=0,invalid_exported=0;
    for(auto& entry:raw_solids)for(const auto& s:raw_shells(entry.second,false)){const auto& set=s.faces;for(int i=1;i<=set->NbCfsFaces();++i){auto f=Handle(StepShape_AdvancedFace)::DownCast(set->CfsFacesValue(i));if(f.IsNull())continue;
      std::vector<Handle(StepShape_VertexLoop)> loops;std::vector<TopoDS_Shape> edges;
      for(int b=1;b<=f->NbBounds();++b){auto bound=f->BoundsValue(b)->Bound();if(auto loop=Handle(StepShape_VertexLoop)::DownCast(bound);!loop.IsNull()){loops.push_back(loop);continue;}
        if(auto loop=Handle(StepShape_EdgeLoop)::DownCast(bound);!loop.IsNull())for(int k=1;k<=loop->NbEdgeList();++k)if(auto curve=Handle(StepShape_EdgeCurve)::DownCast(loop->EdgeListValue(k)->EdgeElement());!curve.IsNull())edges.push_back(TransferBRep::ShapeResult(process,curve));}
      if(loops.empty())continue;
      bool invalid=false;for(const auto& piece:face_pieces.at(id(f))){BRepCheck_Analyzer check(piece);if(!check.IsValid())invalid=true;}
      if(invalid){++invalid_exported;for(const auto& loop:loops)record("reference_vertex_loop_face_invalid",1,{id(f),id(loop),id(loop->LoopVertex())},processing?"BRepCheck fails on this exported face after OCCT's default read":"BRepCheck fails on this exported face as OCCT's translation without ShapeProcess built it (no seam, no pcurves); the occt-default-read arm runs OCCT's own FixShape");}
      if(processing){for(const auto& loop:loops)record("reference_vertex_loop_default_read",0,{id(f),id(loop),id(loop->LoopVertex())},"exported as OCCT's default read built it; its admission is the STEP source adapter's");continue;}
      const auto face=TopoDS::Face(shape(f));std::vector<TopoDS_Wire> remaining;
      for(TopExp_Explorer w(face,TopAbs_WIRE);w.More();w.Next()){bool edge_loop=false;for(TopExp_Explorer e(w.Current(),TopAbs_EDGE);e.More()&&!edge_loop;e.Next())for(const auto& edge:edges)if(!edge.IsNull()&&e.Current().IsSame(edge)){edge_loop=true;break;}if(!edge_loop)remaining.push_back(TopoDS::Wire(w.Current()));}
      std::vector<std::pair<std::vector<U>,std::string>> refusals;std::set<U> refused;
      for(const auto& loop:loops){auto pole=Handle(StepShape_VertexPoint)::DownCast(loop->LoopVertex());auto point=pole.IsNull()?Handle(StepGeom_CartesianPoint)():Handle(StepGeom_CartesianPoint)::DownCast(pole->VertexGeometry());
        if(point.IsNull()||point->NbCoordinates()!=3){refusals.push_back({{id(f),id(loop),id(pole)},"its vertex is not a vertex_point on a cartesian_point (AP242 advanced_face WR6)"});refused.insert(id(loop));continue;}
        const auto translated=TransferBRep::ShapeResult(process,pole);const TopoDS_Vertex vertex=translated.IsNull()?TopoDS_Vertex():TopoDS::Vertex(translated);
        // The raw coordinates are millimetres as they stand: the unit gate above
        // admits only contexts whose length unit resolves to 1 mm (mm-only until
        // M8, bead brepgpu-12s.25.6), so the length factor here is 1.
        auto assessed=brepgpu::step_vertex_loops::assess(face,remaining,f->NbBounds(),vertex,gp_Pnt(point->CoordinatesValue(1),point->CoordinatesValue(2),point->CoordinatesValue(3)),usable_uncertainty[entry.first]);
        if(assessed.reason!=brepgpu::step_vertex_loops::Reason::admitted){refusals.push_back({{id(f),id(loop),id(pole)},std::string(brepgpu::step_vertex_loops::name(assessed.reason))+": "+assessed.refusal});refused.insert(id(loop));}}
      if(!remaining.empty())for(const auto& loop:loops)if(!refused.count(id(loop))){refusals.push_back({{id(f),id(loop),id(loop->LoopVertex())},"translation_differs: OCCT made a wire on this face that its vertex-loop rule does not explain"});refused.insert(id(loop));}
      if(!refusals.empty())++refused_exported;
      for(const auto& r:refusals){record("source_face_refused",1,r.first,r.second+"; exported as OCCT translated it, which the STEP source adapter refuses");std::cout<<"reference_face_refused\t"<<r.first[0]<<'\t'<<r.first[1]<<'\t'<<r.first[2]<<"\n";}}}
    std::cout<<"reference_faces_refused_by_adapter\t"<<refused_exported<<"\nreference_vertex_loop_faces_invalid\t"<<invalid_exported<<"\n";
    // Per-occurrence styles (CDORSI, Model Styling 5.1), the adapter's rows
    // (native/step_styles.hpp), keyed by usage path from the root.
    {auto face_on=[&](U product,U face_id){auto solid=product_solids.find(product);if(solid==product_solids.end())return false;
       for(const auto& s:raw_shells(raw_solids.at(solid->second),false)){for(int k=1;k<=s.faces->NbCfsFaces();++k)if(id(s.faces->CfsFacesValue(k))==face_id)return true;}
       return false;};
     const auto rows=style_resolver.occurrences(roots[0],face_on);style_resolver.finish();
     for(const auto& note:style_resolver.notes){std::cout<<"reference_style_note\t"<<note.stage;for(U i:note.ids)std::cout<<'\t'<<i;std::cout<<'\t'<<note.message<<"\n";record(note.stage,note.severity,note.ids,note.message);}
     for(const auto& row:rows){Bytes w;w.u64(row.style);w.u64(row.face);w.u64(row.path.size());for(U u:row.path)w.u64(u);tables["OCCSTYLE"].push_back(w);}
     std::cout<<"reference_occurrence_styles\t"<<rows.size()<<"\n";std::cout<<"style_path_work\t"<<style_resolver.path_work()<<"\n";}
    {auto temporary=directory/"verification.tsv.tmp";std::ofstream out(temporary);out<<"definition_id\tmode\tverified\tlargest_added_pcurve_tolerance_mm\tlargest_added_pcurve_deviation_mm\ttolerance_cap_mm\n";for(const auto& line:verification_rows)out<<line<<"\n";out.close();if(!out)throw std::runtime_error("reference verification write failed");std::filesystem::rename(temporary,directory/"verification.tsv");}
    {auto temporary=directory/"records.tsv.tmp";std::ofstream out(temporary);for(const auto& line:records)out<<line<<"\n";out.close();if(!out)throw std::runtime_error("reference records write failed");std::filesystem::rename(temporary,directory/"records.tsv");}
    std::string declared_corrections;for(const auto& c:corrections)declared_corrections+=(declared_corrections.empty()?"":",")+c;if(declared_corrections.empty())declared_corrections="none";
    const auto export_end=Clock::now();Bytes meta;
    // shape-processing names the read; raw-entity-corrections names every
    // kind of translated value this export replaced by a raw one (G1 letters;
    // none when OCCT's values all agree). The .brep files are OCCT's own shapes.
    const std::string policy=processing?"shape-processing=default; shape-process-operations="+operation_bits+"; OCCT's default STEP read (no flags or fix parameters supplied; OCCT's defaults asserted in force after transfer); the face map binds each original face to the faces OCCT's processing made of it":
      "shape-processing=translation-only; OCCT translation without ShapeProcess (empty bitset set after read, asserted before and after transfer); OCCT translation-layer edits apply: E1-E16, G1-G22, T1-T8, U1-U4, P1-P4, R1-R3 of papers/2026-10-17-occt-ingress-translation-edits.md section 2.1";
    meta.text("owned reference source v5; OCCT b8f597c677811d1f9f4d8a97f5ae2825c0353a42; context-units; void-shells; occurrence-styles; mapped-item-occurrences; "+policy+"; raw-entity-corrections="+declared_corrections);meta.u32(4);meta.u32(0);
    meta.doubles({seconds(begin,read_end),seconds(read_end,transfer_end),seconds(transfer_end,graph_end),seconds(graph_end,export_end)});tables["META____"].push_back(meta);
    const std::vector<std::string> tags={"META____","PRODUCTS","DEFS____","USAGES__","STYLES__","FACE_STY","OCCSTYLE"};Bytes body;
    for(auto& tag:tags){Bytes rows;for(auto& r:tables[tag])rows.row(r);body.raw(tag);body.u64(tables[tag].size());body.u64(rows.v.size());body.v.insert(body.v.end(),rows.v.begin(),rows.v.end());}
    Bytes out;out.raw("BGOCC001");out.u32(2);out.u32(0);out.v.insert(out.v.end(),digest.begin(),digest.end());out.f64(1);out.f64(uncertainty);out.u64(roots[0]);out.u32(tags.size());out.u32(0);out.u64(88+body.v.size());out.v.insert(out.v.end(),body.v.begin(),body.v.end());
    auto temporary=directory/"source.bgocct.tmp";std::ofstream manifest(temporary,std::ios::binary);manifest.write(reinterpret_cast<const char*>(out.v.data()),out.v.size());manifest.close();if(!manifest)throw std::runtime_error("reference manifest write failed");std::filesystem::rename(temporary,directory/"source.bgocct");
    std::cout<<"definitions\t"<<solids.size()<<"\nfaces\t"<<total_faces<<"\nproducts\t"<<products.size()+mapped_products<<"\nusages\t"<<usages.size()+mapped_usages.size()<<"\n";
    return 0;
  }catch(const Standard_Failure& e){std::cerr<<"reference source export failed: "<<e.what()<<"\n";return 1;}
  catch(const std::exception& e){std::cerr<<"reference source export failed: "<<e.what()<<"\n";return 1;}
}
