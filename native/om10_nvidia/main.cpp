// Owned reference adapter. Geometry operations belong to the named NVIDIA SDK.
#include <SmApiGeneral.h>
#include <SmApiBrep.h>
#include <SmApiUsd.h>
#include <SmBrep.h>
#include <SmPoly.h>
#include "raw_mesh.hpp"
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
        "--heal 0|1 --chord-height POSITIVE --output-directory NEWDIR --definition ORIGINAL_ID INPUT.brep [--definition ...]\n"
        "[--curve-angle NONNEGATIVE_DEGREES] [--surface-angle NONNEGATIVE_DEGREES]\n"
        "Omitted angles preserve legacy0/0; pass25/25 explicitly for SDK angular defaults.\n";
      return 0;
    }
    fs::path directory;bool heal=false,have_heal=false,have_chord=false;double chord=0;
    bool have_curve_angle=false,have_surface_angle=false;double curve_angle=0,surface_angle=0;
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
      }else if(arg=="--curve-angle"||arg=="--surface-angle"){
        bool &seen=arg=="--curve-angle"?have_curve_angle:have_surface_angle;
        double &angle=arg=="--curve-angle"?curve_angle:surface_angle;
        require(!seen,"argument: duplicate angular control");std::string v=argv[++i];size_t used=0;
        angle=std::stod(v,&used);
        require(used==v.size()&&std::isfinite(angle)&&angle>=0,"argument: finite nonnegative angle required");seen=true;
      }else if(arg=="--output-directory"){
        require(directory.empty(),"argument: duplicate output directory");directory=argv[++i];
      }else throw std::runtime_error("argument: unknown option "+arg);
    }
    require(have_heal&&have_chord&&!jobs.empty()&&!directory.empty(),"argument: missing required options");
    require(!fs::exists(directory),"output: destination must be new");
    fs::create_directories(directory);
    std::ofstream parameters(directory/"parameters.tsv");
    require(bool(parameters),"output: cannot open parameters");
    parameters<<"parameter\tvalue\n"<<std::setprecision(17)
      <<"heal\t"<<heal<<"\nchord_height_mm\t"<<chord
      <<"\ncurve_angle_degrees\t"<<curve_angle<<"\nsurface_angle_degrees\t"<<surface_angle
      <<"\nmax_edge_length\t0\nmax_aspect_ratio\t0\nallow_partial\t0\n";
    parameters.close();require(!parameters.fail(),"output: parameters write/close failed");
    std::ofstream timings(directory/"stages.tsv"),reports(directory/"definitions.tsv"),failures(directory/"failures.tsv"),imports(directory/"imports.tsv"),polygons(directory/"polygon-export.tsv");
    require(timings&&reports&&failures&&imports&&polygons,"output: cannot open reports");
    timings<<"definition_id\tstage\tseconds\n"<<std::setprecision(17);
    reports<<"definition_id\tstatus\theal\tchord_mm\tinput_faces\tinput_edges\tinput_vertices\tphysical_vertices\ttriangles\tlamina_edges\tspine_edges\tinput_manifold\toutput_manifold\tmessage\n"<<std::setprecision(17);
    failures<<"definition_id\tnvidia_face_ordinal\tstage\tstatus\n";
    imports<<"definition_id\tprim_path\tpacked_index\tstatus\tmessage\n";
    polygons<<"definition_id\toriginal_sdk_polygons\toriginal_sdk_triangles\tadmitted_rectangles\tofficial_rectangle_split_calls\tproduced_triangles\n";
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
        params.dChordHeightTol=chord;params.dCurveAngleTolDeg=curve_angle;params.dSurfaceAngleTolDeg=surface_angle;
        SmPolyBrep *pointer=nullptr;SmTArray<SmTessellationFailure> face_failures;
        auto tess=SmApiTessellate(breps.values[0],pointer,params,FALSE,&face_failures,&report);
        std::unique_ptr<SmPolyBrep> poly(pointer);
        for(const auto &f:face_failures)failures<<job.first<<'\t'<<f.lFaceIndex<<'\t'<<int(f.eStage)<<'\t'<<int(f.eStatus)<<'\n';
        timings<<job.first<<'\t'<<stage<<'\t'<<elapsed(t)<<'\n';
        require(tess==SM_SUCCESS&&poly&&face_failures.GetSize()==0,"strict NVIDIA tessellation failed");
        stage="extract_f64_corner_mesh";t=Clock::now();Mesh mesh=extract(*breps.values[0],*poly);
        polygons<<job.first<<'\t'<<mesh.original_polygons<<'\t'<<mesh.original_triangles<<'\t'<<mesh.rectangle_count<<'\t'<<mesh.official_split_calls<<'\t'<<mesh.triangles.size()<<'\n';
        timings<<job.first<<"\tofficial_rectangle_triangulation_nested_in_extract\t"<<mesh.official_split_seconds<<'\n';
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
    for(auto *stream:{&timings,&reports,&failures,&imports,&polygons}){stream->close();require(!stream->fail(),"output: report write/close failed");}
    return good?0:1;
  }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
