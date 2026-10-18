// Separate original-source boundary for the external CPU reference. No product
// source, owned mesh, foreign mesher or occurrence transform is involved.
#include "step_units.hpp"
#include "step_styles.hpp"
#include "step_vertex_loops.hpp"
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
#include <iostream>
#include <map>
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

int main(int argc,char** argv){
  if(argc!=3){std::cerr<<"usage: brepgpu-reference-source INPUT.stp NEW_OUTPUT_DIRECTORY\n";return 2;}
  try{
    const auto begin=Clock::now();
    std::ifstream original(argv[1],std::ios::binary);if(!original)throw std::runtime_error("cannot read original STEP");
    std::vector<unsigned char> input((std::istreambuf_iterator<char>(original)),{});
    if(input.empty()||input.size()>(U(1)<<30))throw std::runtime_error("invalid original STEP size");
    auto digest=sha256(input);
    STEPCAFControl_Reader reader;reader.SetNameMode(true);reader.SetColorMode(true);reader.SetMatMode(true);reader.SetPropsMode(true);
    if(reader.ReadFile(argv[1])!=IFSelect_RetDone)throw std::runtime_error("original STEP read failed");
    const auto read_end=Clock::now();
    reader.SetShapeProcessFlags(ShapeProcess::OperationsFlags());
    auto require_no_processing=[&](const char* stage){
      const auto& flags=reader.GetShapeProcessFlags();
      std::cout<<"shape_process_"<<stage<<"\t"<<flags.second<<"\t"<<flags.first.count()<<"\n";
      if(!flags.second||flags.first.any())throw std::runtime_error(std::string("reference STEP processing policy differs at ")+stage+" supplied="+std::to_string(flags.second)+" operations="+flags.first.to_string());
    };
    require_no_processing("before_transfer");
    Handle(TDocStd_Document) doc;XCAFApp_Application::GetApplication()->NewDocument("BinXCAF",doc);
    namespace units=brepgpu::step_units;
    units::prepare_millimetre_transfer(doc);
    if(!reader.Transfer(doc))throw std::runtime_error("original STEP/XDE transfer failed");
    require_no_processing("after_transfer");
    const auto transfer_end=Clock::now();
    auto model=Handle(StepData_StepModel)::DownCast(reader.Reader().WS()->Model());
    units::require_transferred_millimetre(doc,model);
    // Length units per context from the unit entities, never names; the same
    // millimetre-only rule and contexts as the STEP source adapter.
    const units::RepresentationGraph representations(reader.Reader().WS()->Graph());
    {std::map<U,Handle(StepRepr_Representation)> governed;
     for(const auto& rep:units::product_representations(model))governed.emplace(units::label(model,rep),rep);
     for(int i=1;i<=model->NbEntities();++i)if(auto s=Handle(StepShape_ManifoldSolidBrep)::DownCast(model->Value(i));!s.IsNull()){
       auto users=reader.Reader().WS()->Graph().Sharings(s);for(users.Start();users.More();users.Next())if(auto rep=Handle(StepShape_ShapeRepresentation)::DownCast(users.Value());!rep.IsNull())for(const auto& r:representations.transfer_path(rep))governed.emplace(units::label(model,r),r);}
     for(const auto& g:governed)units::require_millimetre(model,g.second,units::context_length(model,g.second));}
    auto process=reader.Reader().WS()->TransferReader()->TransientProcess();
    auto shape_tool=XCAFDoc_DocumentTool::ShapeTool(doc->Main());
    auto id=[&](const Handle(Standard_Transient)& e)->U{if(e.IsNull())return 0;int n=model->IdentLabel(e);if(n<=0)throw std::runtime_error("missing original STEP label");return U(n);};
    auto shape=[&](const Handle(Standard_Transient)& e){auto s=TransferBRep::ShapeResult(process,e);if(s.IsNull())throw std::runtime_error("unmapped original source #"+std::to_string(id(e)));return s;};
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
        if(held.governing.governed){Bnd_Box box;BRepBndLib::Add(solids[id(s)],box);const double diagonal=box.IsVoid()?0.:std::sqrt(box.SquareExtent());
          if(units::uncertainty_usable(held.governing.mm,diagonal)){usable=held.governing.mm;uncertainty=std::max(uncertainty,held.governing.mm);}}}
      if(auto p=Handle(StepBasic_ProductDefinition)::DownCast(e);!p.IsNull())products.push_back(p);
      if(auto u=Handle(StepRepr_NextAssemblyUsageOccurrence)::DownCast(e);!u.IsNull()){usages.push_back(u);parents.insert(id(u->RelatingProductDefinition()));children.insert(id(u->RelatedProductDefinition()));}
    }
    if(solids.empty())throw std::runtime_error("no original solid definitions");
    std::map<U,Style> styles;std::map<U,U> product_solids,solid_products;
    // The same styles as the STEP source adapter (native/step_styles.hpp), so
    // the reference and owned sources agree style for style. Per-occurrence
    // styles have no table here.
    brepgpu::step_styles::Resolver style_resolver(model,reader.Reader().WS());
    for(const auto& c:style_resolver.colours()){Style d;d.id=c.id;d.sources=c.sources;d.rgba={c.rgba[0],c.rgba[1],c.rgba[2],c.rgba[3]};d.color=c.color;styles[d.id]=d;}
    std::map<std::string,std::vector<Bytes>> tables;
    std::vector<U> roots;
    for(auto& p:products){U pi=id(p);if(!children.count(pi))roots.push_back(pi);auto ps=shape(p);
      if(!parents.count(pi)){std::set<U> matches;for(TopExp_Explorer ex(ps,TopAbs_SOLID);ex.More();ex.Next())for(auto& s:solids)if(ex.Current().IsSame(s.second))matches.insert(s.first);
        if(matches.size()!=1)throw std::runtime_error("leaf product #"+std::to_string(pi)+" does not bind exactly one original solid");
        U si=*matches.begin();if(solid_products.count(si))throw std::runtime_error("original solid has multiple leaf products #"+std::to_string(si));product_solids[pi]=si;solid_products[si]=pi;}
      std::set<U> part_reps;if(product_solids.count(pi))for(const auto& holder:representations.holders(raw_solids.at(product_solids.at(pi))))for(const auto& rep:representations.transfer_path(holder))part_reps.insert(units::label(model,rep));
      U style=style_resolver.product_style(pi,part_reps,product_solids.count(pi)?product_solids.at(pi):0);TDF_Label label;shape_tool->Search(ps,label,true,true,false);
      if(!label.IsNull()){Handle(TDataStd_TreeNode) node;if(label.FindAttribute(XCAFDoc::MaterialRefGUID(),node)&&node->HasFather()){
        Handle(TCollection_HAsciiString) name,description,unit,type;double density=0;
        if(XCAFDoc_MaterialTool::GetMaterial(node->Father()->Label(),name,description,density,unit,type)){Style d;if(style)d=styles.at(style);d.id=(U(1)<<62)|pi;d.sources.push_back(pi);d.material=text(name);d.unit=text(unit)+" "+text(type);d.density=density;d.density_set=true;styles[d.id]=d;style=d.id;}}}
      auto product=p->Formation()->OfProduct();std::string name=text(product->Description());if(name.empty())name=text(product->Name());
      Bytes w;w.u64(pi);w.u64(product_solids.count(pi)?product_solids.at(pi):0);w.u64(style);w.text(name);w.text(text(product->Id()));tables["PRODUCTS"].push_back(w);
    }
    if(roots.size()!=1||solid_products.size()!=solids.size())throw std::runtime_error("ambiguous root or unowned original solid");
    for(auto& u:usages){Bytes w;w.u64(id(u));w.u64(id(u->RelatingProductDefinition()));w.u64(id(u->RelatedProductDefinition()));w.u64(0);w.text(text(u->Name()));w.doubles(matrix(shape(u).Location().Transformation()));tables["USAGES__"].push_back(w);}
    for(auto& s:styles)tables["STYLES__"].push_back(s.second.bytes());
    const auto graph_end=Clock::now();
    std::filesystem::path directory(argv[2]);
    if(!std::filesystem::create_directory(directory))throw std::runtime_error("reference output directory must be new");
    U total_faces=0;
    for(auto& entry:raw_solids){U si=entry.first;const auto& solid=solids.at(si);
      if(solid.ShapeType()!=TopAbs_SOLID||!solid.Location().IsIdentity()||solid.Orientation()!=TopAbs_FORWARD)
        throw std::runtime_error("reference definition #"+std::to_string(si)+" requires one identity-located forward solid root");
      BinTools_ShapeSet set;set.SetFormatNb(BinTools_FormatVersion_VERSION_4);set.SetWithTriangles(false);set.SetWithNormals(false);set.Add(solid);
      std::string filename="definition-"+std::to_string(si)+".brep";Bytes row;row.u64(si);row.u64(solid_products.at(si));row.text(filename);
      auto raw=entry.second;row.u64(raw->Outer()->NbCfsFaces());std::set<int> actual_faces,mapped_faces;
      std::map<int,TopAbs_Orientation> actual_orientations;
      for(TopExp_Explorer e(solid,TopAbs_FACE);e.More();e.Next()){
        auto f=TopoDS::Face(e.Current());if(BRep_Tool::Surface(f).IsNull())throw std::runtime_error("triangulation-only face in definition #"+std::to_string(si));
        int index=set.Index(f.Located(TopLoc_Location()));if(index<=0||!actual_faces.insert(index).second)throw std::runtime_error("duplicate or unmapped translated face in definition #"+std::to_string(si));
        actual_orientations[index]=f.Orientation();
      }
      for(int i=1;i<=raw->Outer()->NbCfsFaces();++i){auto f=Handle(StepShape_AdvancedFace)::DownCast(raw->Outer()->CfsFacesValue(i));
        if(f.IsNull())throw std::runtime_error("non-advanced original face in definition #"+std::to_string(si));
        U fi=id(f);auto translated=shape(f);
        int index=set.Index(translated.Located(TopLoc_Location()));
        if(index<=0||!actual_faces.count(index)||!mapped_faces.insert(index).second)throw std::runtime_error("original face #"+std::to_string(fi)+" does not uniquely bind exported solid");
        auto expected_orientation=f->SameSense()?TopAbs_FORWARD:TopAbs_REVERSED;
        if(translated.Orientation()!=expected_orientation||actual_orientations.at(index)!=expected_orientation)
          throw std::runtime_error("translated result or exported solid face orientation disagrees with original #"+std::to_string(fi));
        row.u64(fi);row.u64(id(f->FaceGeometry()));row.u64(U(set.NbShapes()-index+1));row.i32(f->SameSense()?1:-1);row.u32(0);
        Bytes style;style.u64(fi);style.u64(style_resolver.item_style(fi));tables["FACE_STY"].push_back(style);++total_faces;
      }
      if(actual_faces!=mapped_faces)throw std::runtime_error("exported solid face inventory differs from original #"+std::to_string(si));
      auto temporary=directory/(filename+".tmp");std::ofstream out(temporary,std::ios::binary);if(!out)throw std::runtime_error("cannot write definition #"+std::to_string(si));
      set.Write(out);set.Write(solid,out);out.flush();if(!out||out.tellp()>std::streamoff(256)*1024*1024)throw std::runtime_error("definition write failed or exceeds official256MiB limit #"+std::to_string(si));out.close();if(!out)throw std::runtime_error("definition close failed #"+std::to_string(si));
      std::filesystem::rename(temporary,directory/filename);tables["DEFS____"].push_back(row);
    }
    // The reference format has no diagnostics or occurrence-style table
    // (occurrence styles there: bead brepgpu-12s.37.23). What the resolver
    // found, the occurrence styles not carried, and the vertex-loop faces OCCT
    // exported that the STEP source adapter refuses are records in
    // records.tsv beside the manifest (stage, severity, space-separated ids,
    // message), and are also said on stdout.
    std::vector<std::string> records{"stage\tseverity\tids\tmessage"};
    auto record=[&](const std::string& stage,std::uint32_t severity,const std::vector<U>& ids,const std::string& message){std::string line=stage+'\t'+std::to_string(severity)+'\t';for(std::size_t k=0;k<ids.size();++k)line+=(k?" ":"")+std::to_string(ids[k]);records.push_back(line+'\t'+message);};
    // The adapter's vertex-loop rule (native/step_vertex_loops.hpp) on the
    // faces this export writes: OCCT's face is exported as translated; a face
    // the adapter would refuse is recorded with its reason.
    U refused_exported=0;
    // Every shell of a solid, the outer one and each BREP_WITH_VOIDS void, as
    // the STEP source adapter walks them (the export itself still walks the
    // outer shell only; a void solid is refused at its face inventory above).
    auto shell_sets=[&](const Handle(StepShape_ManifoldSolidBrep)& raw){std::vector<Handle(StepShape_ConnectedFaceSet)> sets;
      if(auto outer=Handle(StepShape_OrientedClosedShell)::DownCast(raw->Outer());!outer.IsNull())sets.push_back(outer->ClosedShellElement());else sets.push_back(raw->Outer());
      if(auto with_voids=Handle(StepShape_BrepWithVoids)::DownCast(raw);!with_voids.IsNull())for(int k=1;k<=with_voids->NbVoids();++k)if(!with_voids->VoidsValue(k).IsNull())sets.push_back(with_voids->VoidsValue(k)->ClosedShellElement());
      sets.erase(std::remove_if(sets.begin(),sets.end(),[](const auto& set){return set.IsNull();}),sets.end());return sets;};
    for(auto& entry:raw_solids)for(const auto& set:shell_sets(entry.second))for(int i=1;i<=set->NbCfsFaces();++i){auto f=Handle(StepShape_AdvancedFace)::DownCast(set->CfsFacesValue(i));if(f.IsNull())continue;
      std::vector<Handle(StepShape_VertexLoop)> loops;std::vector<TopoDS_Shape> edges;
      for(int b=1;b<=f->NbBounds();++b){auto bound=f->BoundsValue(b)->Bound();if(auto loop=Handle(StepShape_VertexLoop)::DownCast(bound);!loop.IsNull()){loops.push_back(loop);continue;}
        if(auto loop=Handle(StepShape_EdgeLoop)::DownCast(bound);!loop.IsNull())for(int k=1;k<=loop->NbEdgeList();++k)if(auto curve=Handle(StepShape_EdgeCurve)::DownCast(loop->EdgeListValue(k)->EdgeElement());!curve.IsNull())edges.push_back(TransferBRep::ShapeResult(process,curve));}
      if(loops.empty())continue;
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
      for(const auto& r:refusals){record("source_face_refused",1,r.first,r.second+"; exported as OCCT translated it, which the STEP source adapter refuses");std::cout<<"reference_face_refused\t"<<r.first[0]<<'\t'<<r.first[1]<<'\t'<<r.first[2]<<"\n";}}
    std::cout<<"reference_faces_refused_by_adapter\t"<<refused_exported<<"\n";
    {auto face_on=[&](U product,U face_id){auto solid=product_solids.find(product);if(solid==product_solids.end())return false;
       for(const auto& set:shell_sets(raw_solids.at(solid->second)))for(int k=1;k<=set->NbCfsFaces();++k)if(id(set->CfsFacesValue(k))==face_id)return true;return false;};
     const auto rows=style_resolver.occurrences(roots[0],face_on);style_resolver.finish();
     for(const auto& note:style_resolver.notes){std::cout<<"reference_style_note\t"<<note.stage;for(U i:note.ids)std::cout<<'\t'<<i;std::cout<<'\t'<<note.message<<"\n";record(note.stage,note.severity,note.ids,note.message);}
     for(const auto& row:rows){std::cout<<"reference_occurrence_style_not_carried\t"<<row.style<<'\t'<<row.face;for(U u:row.path)std::cout<<'\t'<<u;std::cout<<"\n";
       std::vector<U> ids{row.style,row.face};ids.insert(ids.end(),row.path.begin(),row.path.end());record("reference_occurrence_style_not_carried",1,ids,"occurrence style (style, face, usage path) that the reference format does not carry");}
     std::cout<<"reference_occurrence_styles_not_carried\t"<<rows.size()<<"\n";std::cout<<"style_path_work\t"<<style_resolver.path_work()<<"\n";}
    {auto temporary=directory/"records.tsv.tmp";std::ofstream out(temporary);for(const auto& line:records)out<<line<<"\n";out.close();if(!out)throw std::runtime_error("reference records write failed");std::filesystem::rename(temporary,directory/"records.tsv");}
    const auto export_end=Clock::now();Bytes meta;meta.text("owned reference source v4; OCCT b8f597c677811d1f9f4d8a97f5ae2825c0353a42; context-units; shape-processing=translation-only; OCCT translation without ShapeProcess (empty bitset set after read, asserted before and after transfer); OCCT translation-layer edits apply: E1-E16, G1-G22, T1-T8, U1-U4, P1-P4, R1-R3 of papers/2026-10-17-occt-ingress-translation-edits.md section 2.1");meta.u32(4);meta.u32(0);
    meta.doubles({seconds(begin,read_end),seconds(read_end,transfer_end),seconds(transfer_end,graph_end),seconds(graph_end,export_end)});tables["META____"].push_back(meta);
    const std::vector<std::string> tags={"META____","PRODUCTS","DEFS____","USAGES__","STYLES__","FACE_STY"};Bytes body;
    for(auto& tag:tags){Bytes rows;for(auto& r:tables[tag])rows.row(r);body.raw(tag);body.u64(tables[tag].size());body.u64(rows.v.size());body.v.insert(body.v.end(),rows.v.begin(),rows.v.end());}
    Bytes out;out.raw("BGOCC001");out.u32(1);out.u32(0);out.v.insert(out.v.end(),digest.begin(),digest.end());out.f64(1);out.f64(uncertainty);out.u64(roots[0]);out.u32(tags.size());out.u32(0);out.u64(88+body.v.size());out.v.insert(out.v.end(),body.v.begin(),body.v.end());
    auto temporary=directory/"source.bgocct.tmp";std::ofstream manifest(temporary,std::ios::binary);manifest.write(reinterpret_cast<const char*>(out.v.data()),out.v.size());manifest.close();if(!manifest)throw std::runtime_error("reference manifest write failed");std::filesystem::rename(temporary,directory/"source.bgocct");
    std::cout<<"definitions\t"<<solids.size()<<"\nfaces\t"<<total_faces<<"\nproducts\t"<<products.size()<<"\nusages\t"<<usages.size()<<"\n";
    return 0;
  }catch(const Standard_Failure& e){std::cerr<<"reference source export failed: "<<e.what()<<"\n";return 1;}
  catch(const std::exception& e){std::cerr<<"reference source export failed: "<<e.what()<<"\n";return 1;}
}
