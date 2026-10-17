// Finite numerical reference queries. No render, meshing or NVIDIA dependency.
#include "../om10_reference/raw.hpp"
#include "closest.hpp"
#include <BRepExtrema_DistShapeShape.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRep_Builder.hxx>
#include <BRep_Tool.hxx>
#include <BinTools.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Face.hxx>
#include <TopExp_Explorer.hxx>
#include <Standard_Failure.hxx>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <string>
namespace fs=std::filesystem;
using Clock=std::chrono::steady_clock;
constexpr double NUMERICAL_GRADE=0x1p-32,DEFLECTION=0x1p-40;
double elapsed(Clock::time_point start){return std::chrono::duration<double>(Clock::now()-start).count();}
using bgom10::require;
using Point=refgeom::Point;
Point point(const double *p){return {p[0],p[1],p[2]};}
struct Witness {uint64_t face,triangle;Point xyz;};
std::vector<Witness> witnesses(const std::string &path){
  bgom10::Bytes bytes(path);bytes.magic("BGRPROB1");uint64_t n=bytes.u64();
  require(n>0&&n<=UINT32_MAX,"witness_decode: invalid count");
  require(bgom10::checked_add(16,bgom10::checked_mul(n,40))==bytes.bytes.size(),"witness_decode: section length");
  std::vector<Witness> output(n);
  for(auto &row:output){row.face=bytes.u64();row.triangle=bytes.u64();for(auto &v:row.xyz)v=bytes.number();}
  bytes.done();return output;
}
TopoDS_Compound boundary(const std::string &path,uint64_t &face_count){
  TopoDS_Shape solid;
  require(BinTools::Read(solid,path.c_str())&&!solid.IsNull(),"SOURCE_QUERY_PREPARATION: cannot read original binary shape");
  require(solid.ShapeType()==TopAbs_SOLID&&solid.Location().IsIdentity()&&solid.Orientation()==TopAbs_FORWARD,
          "SOURCE_QUERY_PREPARATION: expected original identity-located forward solid");
  TopoDS_Compound result;BRep_Builder builder;builder.MakeCompound(result);face_count=0;
  for(TopExp_Explorer faces(solid,TopAbs_FACE);faces.More();faces.Next()){
    auto face=TopoDS::Face(faces.Current());TopLoc_Location location;
    require(BRep_Tool::Triangulation(face,location).IsNull(),"SOURCE_QUERY_PREPARATION: input contains tessellation");
    for(TopExp_Explorer edges(face,TopAbs_EDGE);edges.More();edges.Next()){
      auto edge=TopoDS::Edge(edges.Current());double first=0,last=0;
      auto curve=BRep_Tool::CurveOnSurface(edge,face,first,last);
      require(!curve.IsNull()&&std::isfinite(first)&&std::isfinite(last)&&first<=last,
              "SOURCE_QUERY_PREPARATION: face-edge has no usable trimmed pcurve");
      require(BRep_Tool::Polygon3D(edge,location).IsNull(),"SOURCE_QUERY_PREPARATION: input contains edge polygon");
    }
    builder.Add(result,face);++face_count;
  }
  require(face_count>0,"SOURCE_QUERY_PREPARATION: no original faces");return result;
}
void finish(std::ofstream &stream){stream.close();require(!stream.fail(),"output: write/close failed");}
int run(int argc,char **argv){
  auto full_start=Clock::now();
  if(argc==2&&std::string(argv[1])=="--help"){
    std::cout<<"--source ORIGINAL_V4.brep --mesh ACTUAL_BGREFM01 --witnesses BGRPROB1 --tolerance POSITIVE_MM --output-directory NEWDIR\n"
               "Finite numerical boundary/triangle queries; no meshing or rendering.\n";return 0;
  }
  std::map<std::string,std::string> args;
  for(int i=1;i<argc;++i){
    std::string key=argv[i];require(key=="--source"||key=="--mesh"||key=="--witnesses"||key=="--tolerance"||key=="--output-directory","argument: unknown option");
    require(i+1<argc,"argument: missing value");require(args.emplace(key,argv[++i]).second,"argument: repeated option");
  }
  for(const auto *key:{"--source","--mesh","--witnesses","--tolerance","--output-directory"})require(args.count(key),"argument: missing required option");
  size_t used=0;double tolerance=std::stod(args["--tolerance"],&used),forward_limit=tolerance+NUMERICAL_GRADE,reverse_limit=2*tolerance+NUMERICAL_GRADE;
  require(used==args["--tolerance"].size()&&std::isfinite(tolerance)&&tolerance>0&&std::isfinite(reverse_limit),"argument: invalid finite positive tolerance");
  fs::path output=args["--output-directory"];require(!fs::exists(output),"output: destination must be new");
  auto start=Clock::now();const bgref::RawScene mesh(args["--mesh"]);
  require(mesh.definitions.size()==1&&mesh.instances.empty()&&!mesh.triangles.empty(),"mesh_decode: one definition-local nonempty mesh with zero occurrences required");
  auto source_witnesses=witnesses(args["--witnesses"]);uint64_t original_faces=0;
  auto source=boundary(args["--source"],original_faces);double source_read_seconds=elapsed(start);
  start=Clock::now();
  std::vector<refgeom::Triangle> triangles;triangles.reserve(mesh.triangles.size());
  std::map<uint64_t,std::vector<uint64_t>> groups;
  for(size_t i=0;i<mesh.triangles.size();++i){
    const auto &t=mesh.triangles[i];std::array<Point,3> p;
    for(int k=0;k<3;++k)p[k]=point(mesh.raw[t.vertex[k]].used);
    triangles.emplace_back(p);groups[t.face].push_back(i);
  }
  std::set<uint64_t> selected;
  for(const auto &group:groups)selected.insert(group.second[group.second.size()/2]);
  uint64_t count=std::min<uint64_t>(64,triangles.size());
  for(uint64_t i=0;i<count;++i)selected.insert(((2*i+1)*triangles.size())/(2*count));
  fs::create_directories(output);std::ofstream forward(output/"forward.tsv"),reverse(output/"reverse.tsv");
  require(bool(forward)&&bool(reverse),"output: cannot open query tables");
  forward<<std::setprecision(std::numeric_limits<long double>::max_digits10)
    <<"nv_face_ordinal\tnv_triangle_index\tprobe\tx_mm\ty_mm\tz_mm\tdistance_mm\tstatus\n";
  reverse<<std::setprecision(std::numeric_limits<long double>::max_digits10)
    <<"source_face_id\towned_triangle_index\tx_mm\ty_mm\tz_mm\tclosest_nv_triangle_index\tb0\tb1\tb2\tclosest_x_mm\tclosest_y_mm\tclosest_z_mm\tdistance_mm\tstatus\n";
  BRepExtrema_DistShapeShape distance;distance.SetDeflection(DEFLECTION);distance.LoadS2(source);
  uint64_t forward_queries=0,forward_failures=0,forward_solver_failures=0,reverse_failures=0;double forward_max=0;long double reverse_max=0;
  double setup_seconds=elapsed(start);
  start=Clock::now();
  for(uint64_t index:selected){
    const auto &p=triangles[index].p;std::array<Point,4> probes;
    for(int k=0;k<3;++k){
      probes[0][k]=(p[0][k]/3+p[1][k]/3)+p[2][k]/3;
      for(int j=0;j<3;++j)probes[j+1][k]=p[j][k]/2+p[(j+1)%3][k]/2;
    }
    const char *names[]={"centroid","edge01","edge12","edge20"};
    for(int probe=0;probe<4;++probe){
      const auto &q=probes[probe];require(refgeom::finite(q),"forward: nonfinite selected probe");++forward_queries;
      forward<<mesh.triangles[index].face<<'\t'<<index<<'\t'<<names[probe];
      // The actual numerical source query uses binary64 gp_Pnt; retain those
      // exact submitted coordinates in the table, not unsubmitted LD values.
      double xyz[3]={double(q[0]),double(q[1]),double(q[2])};
      require(std::isfinite(xyz[0])&&std::isfinite(xyz[1])&&std::isfinite(xyz[2]),"forward: nonfinite binary64 probe");
      for(double v:xyz)forward<<'\t'<<v;
      distance.LoadS1(BRepBuilderAPI_MakeVertex(gp_Pnt(xyz[0],xyz[1],xyz[2])).Vertex());
      bool solved=false;double value=0;
      try{solved=distance.Perform()&&distance.IsDone()&&distance.NbSolution()>0&&!distance.InnerSolution();if(solved)value=distance.Value();}
      catch(const Standard_Failure &){solved=false;}
      if(!solved||!std::isfinite(value)||value<0){++forward_failures;++forward_solver_failures;forward<<"\t\tSOLVER_FAIL\n";continue;}
      forward_max=std::max(forward_max,value);bool ok=value<=forward_limit;
      if(!ok)++forward_failures;
      forward<<'\t'<<value<<'\t'<<(ok?"PASS":"FAIL")<<'\n';
    }
  }
  double forward_seconds=elapsed(start);start=Clock::now();
  for(const auto &w:source_witnesses){
    refgeom::Closest best;uint64_t index=UINT64_MAX;
    for(size_t i=0;i<triangles.size();++i){
      auto candidate=triangles[i].nearest(w.xyz);
      if(candidate.squared<best.squared){best=candidate;index=i;}
    }
    require(index!=UINT64_MAX,"reverse: no finite nearest triangle");long double value=std::sqrt(best.squared);
    require(std::isfinite(value)&&value>=0,"reverse: nonfinite nearest distance");
    reverse_max=std::max(reverse_max,value);bool ok=value<=reverse_limit;if(!ok)++reverse_failures;
    reverse<<w.face<<'\t'<<w.triangle;for(auto v:w.xyz)reverse<<'\t'<<v;
    reverse<<'\t'<<index;for(auto v:best.bary)reverse<<'\t'<<v;for(auto v:best.point)reverse<<'\t'<<v;
    reverse<<'\t'<<value<<'\t'<<(ok?"PASS":"FAIL")<<'\n';
  }
  double reverse_seconds=elapsed(start);finish(forward);finish(reverse);
  double complete_seconds=elapsed(full_start);
  bool pass=forward_failures==0&&reverse_failures==0;
  std::ofstream summary(output/"summary.tsv.tmp"),metadata(output/"metadata.tsv");require(bool(summary)&&bool(metadata),"output: cannot open summary/metadata");
  summary<<std::setprecision(std::numeric_limits<long double>::max_digits10)<<"field\tvalue\n"
    <<"status\t"<<(pass?"PASS":"FAIL")<<"\noriginal_faces\t"<<original_faces<<"\nselected_nv_triangles\t"<<selected.size()
    <<"\nforward_queries\t"<<forward_queries<<"\nforward_max_mm\t"<<forward_max<<"\nforward_failures\t"<<forward_failures
    <<"\nforward_solver_failures\t"<<forward_solver_failures
    <<"\nreverse_queries\t"<<source_witnesses.size()<<"\nreverse_max_mm\t"<<reverse_max<<"\nreverse_failures\t"<<reverse_failures
    <<"\ntolerance_mm\t"<<tolerance<<"\nnumerical_grade_mm\t"<<NUMERICAL_GRADE<<"\nsolver_deflection_mm\t"<<DEFLECTION
    <<"\nforward_limit_mm\t"<<forward_limit<<"\nreverse_limit_mm\t"<<reverse_limit<<"\nsource_read_seconds\t"<<source_read_seconds
    <<"\nforward_seconds\t"<<forward_seconds<<"\nreverse_seconds\t"<<reverse_seconds<<'\n';
  metadata<<std::setprecision(17)<<"field\tvalue\nsource_boundary\toriginal translated unmeshed trimmed FACE compound; never filled solid"
    <<"\nsolver_deflection_mm\t"<<DEFLECTION<<"\nreverse_arithmetic\tlong double\nreverse_precision_bits\t"<<std::numeric_limits<long double>::digits
    <<"\nquery_scope\tfinite selected probes; numerical admission only; not universal Hausdorff certificate"
    <<"\nsource_read_scope\traw mesh and witness decode plus original solid read/pcurve preflight/FACE compound preparation"
    <<"\nsource_query_preparation_failure\tnonzero exit and no summary; a reference-query capability failure, not NVIDIA geometry failure"
    <<"\ntiming_scope\tqualification query walls; excludes final reports/shutdown; not complete benchmark costs\n";
  finish(metadata);
  std::ofstream times(output/"timings.tsv");require(bool(times),"output: cannot open timings");
  times<<std::setprecision(17)<<"stage\tseconds\tscope\n"
    <<"input_decode_and_boundary_prepare\t"<<source_read_seconds<<"\traw mesh/witness/original solid/pcurve guard/trimmed boundary\n"
    <<"triangle_selection_and_query_setup\t"<<setup_seconds<<"\ttriangle cache/fixed schedule/output tables/resident OCCT solver setup\n"
    <<"forward_queries_and_rows\t"<<forward_seconds<<"\tactual OCCT boundary queries and result rows\n"
    <<"reverse_queries_and_rows\t"<<reverse_seconds<<"\tactual exhaustive point-triangle searches and result rows\n"
    <<"complete_through_query_table_close\t"<<complete_seconds<<"\tenclosing qualification wall; excludes final reports/host teardown/exit; nested not additive\n";
  finish(times);finish(summary);fs::rename(output/"summary.tsv.tmp",output/"summary.tsv");return pass?0:1;
}
int main(int argc,char **argv){
  try{return run(argc,argv);}
  catch(const Standard_Failure &error){std::cerr<<"OCCT reference query: "<<error.what()<<'\n';return 1;}
  catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
