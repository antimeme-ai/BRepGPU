#include "../native/mesher/geometry_correspondence.hpp"
#include <filesystem>
#include <iostream>
#include <stdexcept>
namespace {
using bgmesh::Rational;
void require(bool value,const char *message){if(!value)throw std::runtime_error(message);}
void check(const std::filesystem::path &path,bool weighted) {
 const auto resource=std::make_shared<bgmesh::RunResource>(uint64_t{1}<<27);
 bgfd::Budget b({uint64_t{1}<<27,uint64_t{1}<<36},resource.get());
 const auto source=bgmesh::load_source(path,resource);
 const auto store=bgmesh::compile_geometry(source,resource,{{bgmesh::OwnerKind::geometry,0,134},0,0},b);
 bgmesh::GeometryContext ctx(b);
 const auto chart=bgmesh::choose_chart(ctx,store,{.face={0},.generation=4,
   .positional_allowance=Rational(b,1).divide(Rational(b,200)),
   .node_source_allowance=Rational(b,1).divide(Rational(b,1000)),.allow_plane_profile=false});
 bgmesh::TraceIntervalRequest request{.use={0},.source_parameter={Rational(b,0),Rational(b,1)},
   .chart_endpoints={Rational(b,0),Rational(b,0),Rational(b,1),Rational(b,0)},
   .original_uv={Rational(b,0),Rational(b,0),Rational(b,1),Rational(b,0)},
   .actual_endpoint_xyz={Rational(b,0),Rational(b,0),Rational(b,0),Rational(b,1),Rational(b,0),Rational(b,0)},
   .positional_allowance=Rational(b,1).divide(Rational(b,200))};
 const auto fast=bgmesh::bound_trace_interval(ctx,chart,request);
 require(fast.correspondence==bgmesh::TraceCorrespondence::geometric_line&&fast.trace_upper.sign()==0&&
         fast.chord_upper&&fast.chord_upper->sign()==0&&fast.chord_pieces.empty(),
         "nonlinear straight geometry uses checked image correspondence with zero error, not parameter-speed subdivision");
 request.allow_geometric_line=false;
 const auto affine=bgmesh::bound_trace_interval(ctx,chart,request);
 // Weighted curve C=(3t-2t²)/(1+2t-2t²), S=3t/2-t²/2.
 // Residual numerator POWER=(3/2)t-(9/2)t²+4t³-t⁴.
 // Its degree6 Bernstein extrema are +1/4,-1/12; product Wmin=1.
 require(affine.correspondence==bgmesh::TraceCorrespondence::affine_parameter&&
         (weighted?affine.trace_upper.compare(Rational(b,1).divide(Rational(b,4)))==0:affine.trace_upper.sign()==0),
         "independently authored POWER residual matches original homogeneous Bernstein bound");
 require(affine.chord_pieces.size()==1&&!affine.chord_upper,
         "general rational physical chord goes to existing original-root residual runtime");
 if(weighted)return;
 request.original_uv[1]=Rational(b,1).divide(Rational(b,64));request.original_uv[3]=request.original_uv[1];
 request.chart_endpoints=request.original_uv;
 const auto displaced=bgmesh::bound_trace_interval(ctx,chart,request);
 require(displaced.trace_upper.compare(Rational(b,1).divide(Rational(b,64)))==0,
         "known original source-to-selected trace departure is charged, not silently renamed as source truth");
}
}
int main(int argc,char **argv) {
 try{require(argc==2,"expected authored chart fixture directory");const std::filesystem::path root(argv[1]);
  check(root/"warped-plane.bgsource",false);check(root/"weighted-trace.bgsource",true);
  check(root/"weighted-scaled-trace.bgsource",true);
  std::cout<<"PASS\toriginal coupled boundary traces\n";return 0;}
 catch(const std::exception &e){std::cerr<<"FAIL\t"<<e.what()<<'\n';return 1;}
 catch(const bgfd::Failure &e){std::cerr<<"FAIL\t"<<e.what()<<'\n';return 1;}
}
