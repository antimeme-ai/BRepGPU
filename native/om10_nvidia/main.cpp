// Owned reference adapter. Geometry operations belong to the named NVIDIA SDK.
#include <SmApiGeneral.h>
#include <SmApiBrep.h>
#include <SmApiUsd.h>
#include <SmBrep.h>
#include <SmPoly.h>
#include <array>
#include <chrono>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

// Official entry has no USD types; its public header otherwise requires the
// full OpenUSD C++ headers, which the native release does not package.
namespace occt {
bool ConvertOcctFileToUsd(const std::string &, const std::string &, std::string &);
}
namespace fs = std::filesystem;
using Clock = std::chrono::steady_clock;
using Vec = std::array<double,3>;
void require(bool value,const std::string &message) {
  if(!value)throw std::runtime_error(message);
}
double elapsed(Clock::time_point start) {
  return std::chrono::duration<double>(Clock::now()-start).count();
}
std::string clean(std::string value) {
  for(char &c:value)if(c=='\t'||c=='\n'||c=='\r')c=' ';
  return value;
}
uint64_t integer(const std::string &value) {
  require(!value.empty() && value.find_first_not_of("0123456789")==std::string::npos,"argument: unsigned ID expected");
  size_t count=0;auto id=std::stoull(value,&count);
  require(count==value.size() && id!=UINT64_MAX,"argument: invalid/reserved ID");
  return id;
}
struct Writer {
  std::ofstream stream;
  explicit Writer(const fs::path &path):stream(path,std::ios::binary) {
    require(bool(stream),"mesh_export: cannot open output");
  }
  void u32(uint32_t v){for(int i=0;i<4;++i)stream.put(char(v>>(8*i)));}
  void u64(uint64_t v){for(int i=0;i<8;++i)stream.put(char(v>>(8*i)));}
  void f64(double d){uint64_t v;std::memcpy(&v,&d,8);u64(v);}
  void close(){stream.close();require(!stream.fail(),"mesh_export: write/close failed");}
};
struct Corner { Vec point,normal;uint64_t physical; };
struct Triangle { std::array<uint32_t,3> corners;uint64_t face; };
struct Mesh {std::vector<Corner> corners;std::vector<Triangle> triangles;uint64_t physical_count=0;};
Vec point(const SmPoint3d &p){return {p[0],p[1],p[2]};}
Vec normal(const SmVector3d &p){return {p[0],p[1],p[2]};}
bool finite(const Vec &p){return std::isfinite(p[0])&&std::isfinite(p[1])&&std::isfinite(p[2]);}
Mesh extract(SmBrep &brep,SmPolyBrep &poly) {
  SmTArray<SmFace*> faces;brep.GetFaces(faces);
  std::map<const SmFace*,uint64_t> face_ids;
  for(ULONG i=0;i<faces.GetSize();++i)
    require(faces[i] && face_ids.emplace(faces[i],i).second,"mesh_export: invalid imported face list");
  SmTArray<SmPolyVertex*> vertices;poly.GetPolyVertices(vertices);
  std::map<const SmPolyVertex*,uint64_t> vertex_ids;
  for(ULONG i=0;i<vertices.GetSize();++i)
    require(vertices[i] && vertex_ids.emplace(vertices[i],i).second,"mesh_export: invalid physical vertex list");
  SmTArray<SmPolyFace*> polygons;poly.GetPolyFaces(polygons);
  require(polygons.GetSize()>0,"mesh_export: empty actual tessellation");
  require(uint64_t(polygons.GetSize())<=UINT32_MAX/3,"mesh_export: corner count exceeds native slots");
  Mesh output;output.physical_count=vertices.GetSize();
  output.corners.reserve(size_t(polygons.GetSize())*3);
  output.triangles.reserve(polygons.GetSize());
  std::vector<uint64_t> counts(faces.GetSize(),0);
  std::set<uint64_t> used_vertices;
  for(ULONG i=0;i<polygons.GetSize();++i){
    auto *polygon=polygons[i];require(polygon,"mesh_export: null polygon");
    const auto source=face_ids.find(polygon->GetOriginalFace());
    require(source!=face_ids.end(),"mesh_export: polygon has no original imported face identity");
    SmTArray<SmPolyLoop*> loops;polygon->GetPolyLoops(loops);
    require(loops.GetSize()==1 && loops[0],"mesh_export: polygon must have one outer loop and no holes");
    SmTArray<SmPolyEdge*> edges;loops[0]->GetPolyEdges(edges);
    require(edges.GetSize()==3,"mesh_export: actual polygon is not triangular");
    for(ULONG j=0;j<3;++j)
      require(edges[j] && edges[j]->GetAuxData(),"mesh_export: missing actual polygon edge/corner normal");
    Triangle triangle{{},source->second};
    std::set<uint64_t> distinct;
    for(ULONG j=0;j<3;++j){
      auto key=vertex_ids.find(edges[j]->GetStartPolyVertex());
      require(key!=vertex_ids.end(),"mesh_export: foreign physical vertex pointer");
      Corner corner{point(key->first->GetPoint()),normal(edges[j]->GetAuxData()->m_vNormal),key->second};
      require(finite(corner.point)&&finite(corner.normal),"mesh_export: nonfinite actual point/normal");
      double length=std::hypot(corner.normal[0],corner.normal[1],corner.normal[2]);
      require(std::isfinite(length)&&length>0,"mesh_export: zero actual normal");
      distinct.insert(key->second);used_vertices.insert(key->second);
      triangle.corners[j]=static_cast<uint32_t>(output.corners.size());
      output.corners.push_back(corner);
      require(edges[j]->GetEndPolyVertex()==edges[(j+1)%3]->GetStartPolyVertex(),"mesh_export: disconnected polygon loop");
    }
    require(distinct.size()==3,"mesh_export: collapsed physical triangle");
    const auto &a=output.corners[triangle.corners[0]].point;
    const auto &b=output.corners[triangle.corners[1]].point;
    const auto &c=output.corners[triangle.corners[2]].point;
    Vec cross={(b[1]-a[1])*(c[2]-a[2])-(b[2]-a[2])*(c[1]-a[1]),
               (b[2]-a[2])*(c[0]-a[0])-(b[0]-a[0])*(c[2]-a[2]),
               (b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0])};
    require(finite(cross)&&std::hypot(cross[0],cross[1],cross[2])>0,"mesh_export: zero/nonfinite triangle area");
    output.triangles.push_back(triangle);++counts[source->second];
  }
  for(uint64_t count:counts)require(count>0,"mesh_export: imported NVIDIA face omitted from actual triangles");
  require(used_vertices.size()==vertices.GetSize(),"mesh_export: unused actual physical vertices");
  return output;
}
void write_mesh(const fs::path &path,uint64_t id,const Mesh &mesh) {
  Writer w(path);w.stream.write("BGREFM01",8);w.u32(1);w.u32(0);
  w.u64(1);w.u64(0);w.u64(mesh.corners.size());w.u64(mesh.triangles.size());w.u64(1);w.u64(0);
  w.u64(id);w.u64(0);w.u64(mesh.corners.size());w.u64(0);w.u64(mesh.triangles.size());
  for(const auto &c:mesh.corners){for(double v:c.point)w.f64(v);for(double v:c.normal)w.f64(v);w.u64(c.physical);}
  for(const auto &t:mesh.triangles){for(auto c:t.corners)w.u32(c);w.u32(0);w.u64(t.face);}
  w.f64(.5);w.f64(.5);w.f64(.5);w.f64(.5);w.close();
}
struct Imported {
  std::vector<SmBrep*> values;
  ~Imported(){for(auto *p:values)delete p;}
};
int main(int argc,char **argv) {
  try {
    if(argc==2 && std::string(argv[1])=="--help"){
      std::cout<<"NVIDIA USD/BRep CPU import+tessellation; genuine f64 triangles/normals\n"
        "--heal 0|1 --chord-height POSITIVE --output-directory NEWDIR --definition ORIGINAL_ID INPUT.brep [--definition ...]\n";
      return 0;
    }
    fs::path directory;bool heal=false,have_heal=false,have_chord=false;double chord=0;
    std::vector<std::pair<uint64_t,fs::path>> jobs;std::set<uint64_t> ids;
    for(int i=1;i<argc;++i){
      std::string arg=argv[i];require(i+1<argc,"argument: missing value");
      if(arg=="--definition"){
        require(i+2<argc,"argument: definition requires ID and path");auto id=integer(argv[++i]);
        require(ids.insert(id).second,"argument: duplicate definition ID");jobs.emplace_back(id,argv[++i]);
      }else if(arg=="--heal"){
        require(!have_heal,"argument: duplicate heal");std::string v=argv[++i];
        require(v=="0"||v=="1","argument: heal must be0 or1");heal=v=="1";have_heal=true;
      }else if(arg=="--chord-height"){
        require(!have_chord,"argument: duplicate chord height");std::string v=argv[++i];size_t used=0;
        chord=std::stod(v,&used);require(used==v.size()&&std::isfinite(chord)&&chord>0,"argument: finite positive chord required");have_chord=true;
      }else if(arg=="--output-directory"){
        require(directory.empty(),"argument: duplicate output directory");directory=argv[++i];
      }else throw std::runtime_error("argument: unknown option "+arg);
    }
    require(have_heal&&have_chord&&!jobs.empty()&&!directory.empty(),"argument: missing required options");
    require(!fs::exists(directory),"output: destination must be new");
    fs::create_directories(directory);
    std::ofstream timings(directory/"stages.tsv"),reports(directory/"definitions.tsv"),failures(directory/"failures.tsv"),imports(directory/"imports.tsv");
    require(timings&&reports&&failures&&imports,"output: cannot open reports");
    timings<<"definition_id\tstage\tseconds\n"<<std::setprecision(17);
    reports<<"definition_id\tstatus\theal\tchord_mm\tinput_faces\tinput_edges\tinput_vertices\tphysical_vertices\ttriangles\tlamina_edges\tspine_edges\tinput_manifold\toutput_manifold\tmessage\n"<<std::setprecision(17);
    failures<<"definition_id\tnvidia_face_ordinal\tstage\tstatus\n";
    imports<<"definition_id\tprim_path\tpacked_index\tstatus\tmessage\n";
    auto start=Clock::now();SmApiCreateContext();
    require(SmApiUsdEnsurePluginRegistered()==SM_SUCCESS,"plugin registration failed; check isolated package runtime");
    timings<<"0\tworker_context_plugin_setup\t"<<elapsed(start)<<'\n';
    bool good=true;
    for(const auto &job:jobs){
      auto begin=Clock::now();std::string stage="convert_occt_to_usd";uint64_t nf=0,ne=0,nv=0;SmTessellationReport report;
      try {
        auto t=Clock::now();std::string message;fs::path usd=directory/("definition-"+std::to_string(job.first)+".usdc");
        const bool converted=occt::ConvertOcctFileToUsd(job.second.string(),usd.string(),message);
        require(converted,"official OCCT-to-USD bridge: "+message);
        timings<<job.first<<'\t'<<stage<<'\t'<<elapsed(t)<<'\n';stage="import_usd_brep_heal";t=Clock::now();
        Imported breps;std::vector<SmApiUsdBrepImportResult> results;
        auto status=SmApiUsdImportBreps(usd.c_str(),breps.values,heal?TRUE:FALSE,FALSE,&results);
        for(const auto &r:results)imports<<job.first<<'\t'<<clean(r.m_sPrimPath)<<'\t'<<r.m_iPackedBrepIndex<<'\t'<<int(r.m_sStatus)<<'\t'<<clean(r.m_sMessage)<<'\n';
        require(status==SM_SUCCESS&&breps.values.size()==1&&breps.values[0]&&results.size()==1,"strict USD import failed or did not yield exactly one Brep/member result");
        for(const auto &r:results)require(r.m_sStatus==SM_SUCCESS&&r.m_pBrep==breps.values[0],"strict member result failed/foreign");
        timings<<job.first<<'\t'<<stage<<'\t'<<elapsed(t)<<'\n';
        SmTArray<SmFace*> fs;SmTArray<SmEdge*> es;SmTArray<SmVertex*> vs;
        breps.values[0]->GetFaces(fs);breps.values[0]->GetEdges(es);breps.values[0]->GetVertices(vs);nf=fs.GetSize();ne=es.GetSize();nv=vs.GetSize();
        stage="tessellate";t=Clock::now();SmTessellationParams params;
        params.dChordHeightTol=chord;params.dCurveAngleTolDeg=0;params.dSurfaceAngleTolDeg=0;
        SmPolyBrep *pointer=nullptr;SmTArray<SmTessellationFailure> face_failures;
        auto tess=SmApiTessellate(breps.values[0],pointer,params,FALSE,&face_failures,&report);
        std::unique_ptr<SmPolyBrep> poly(pointer);
        for(const auto &f:face_failures)failures<<job.first<<'\t'<<f.lFaceIndex<<'\t'<<int(f.eStage)<<'\t'<<int(f.eStatus)<<'\n';
        timings<<job.first<<'\t'<<stage<<'\t'<<elapsed(t)<<'\n';
        require(tess==SM_SUCCESS&&poly&&face_failures.GetSize()==0,"strict NVIDIA tessellation failed");
        stage="extract_f64_corner_mesh";t=Clock::now();Mesh mesh=extract(*breps.values[0],*poly);
        timings<<job.first<<'\t'<<stage<<'\t'<<elapsed(t)<<'\n';stage="write_genuine_mesh";t=Clock::now();
        fs::path path=directory/("definition-"+std::to_string(job.first)+".bgrefmesh");fs::path temporary=path.string()+".tmp";
        write_mesh(temporary,job.first,mesh);fs::rename(temporary,path);
        timings<<job.first<<'\t'<<stage<<'\t'<<elapsed(t)<<'\n';
        reports<<job.first<<"\tPASS\t"<<heal<<'\t'<<chord<<'\t'<<nf<<'\t'<<ne<<'\t'<<nv<<'\t'<<mesh.physical_count<<'\t'<<mesh.triangles.size()<<'\t'<<report.laminaEdges<<'\t'<<report.spineEdges<<'\t'<<report.inputManifold<<'\t'<<report.outputManifold<<"\tcomplete genuine NVIDIA triangles\n";
      }catch(const std::exception &error){
        good=false;reports<<job.first<<"\tFAIL\t"<<heal<<'\t'<<chord<<'\t'<<nf<<'\t'<<ne<<'\t'<<nv<<"\t0\t0\t"<<report.laminaEdges<<'\t'<<report.spineEdges<<'\t'<<report.inputManifold<<'\t'<<report.outputManifold<<'\t'<<stage<<": "<<clean(error.what())<<'\n';
        std::cerr<<"definition "<<job.first<<' '<<stage<<": "<<error.what()<<'\n';
      }
      timings<<job.first<<"\tdefinition_complete_through_object_teardown\t"<<elapsed(begin)<<'\n';
    }
    for(auto *stream:{&timings,&reports,&failures,&imports}){stream->close();require(!stream->fail(),"output: report write/close failed");}
    return good?0:1;
  }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
