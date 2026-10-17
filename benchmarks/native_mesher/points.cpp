#include "../../native/mesher/geometry_analytic.hpp"
#include <bit>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <string_view>

namespace {
using Clock=std::chrono::steady_clock;
double milliseconds(Clock::time_point begin,Clock::time_point end) {
  return std::chrono::duration<double,std::milli>(end-begin).count();
}
template<class Evaluate>
void family(std::string_view name,Evaluate evaluate) {
  bgfd::Budget budget({uint64_t{1}<<29,uint64_t{1}<<36});
  const auto start=Clock::now();
  bgmesh::GeometryContext context(budget);
  uint64_t checksum=0;
  constexpr int64_t count=1024;
  for(int64_t i=0;i<count;++i) {
    const auto t=bgmesh::Rational(budget,i).divide(bgmesh::Rational(budget,count));
    const auto point=evaluate(context,t);
    for(uint32_t axis=0;axis<point.dimension;++axis)
      checksum=std::rotl(checksum,1)^std::bit_cast<uint64_t>(point.xyz[axis]);
  }
  const auto finish=Clock::now();
  std::cout<<name<<'\t'<<count<<'\t'<<milliseconds(start,finish)<<'\t'<<budget.work()<<'\t'
           <<budget.peak()<<'\t'<<context.bits()<<'\t'<<checksum<<std::endl;
}
}
int main(int argc,char **argv) {
 try {
  if(argc!=2)throw std::runtime_error("expected authored original geometry source fixture");
  const auto start=Clock::now();
  const auto resource=std::make_shared<bgmesh::RunResource>(uint64_t{1}<<27);
  const auto source=bgmesh::load_source(argv[1],resource);
  bgfd::Budget compiler({uint64_t{1}<<27,uint64_t{1}<<35},resource.get());
  const auto store=bgmesh::compile_geometry(source,resource,{{bgmesh::OwnerKind::geometry,0,129},0,0},compiler);
  std::cout<<std::setprecision(17);
  std::cout<<"family\tpoints\twall_ms\texact_work\tbudget_peak_bytes\tfinal_point_stage\toutput_checksum\n";
  std::cout<<"source_prepare\t0\t"<<milliseconds(start,Clock::now())<<'\t'<<compiler.work()<<'\t'<<compiler.peak()<<"\t0\t0"<<std::endl;
  family("line_source_dispatch",[&](auto &context,const auto &t){return bgmesh::canonical_original_curve(context,store,bgmesh::Slot<bgmesh::CurveTag>{1},t);});
  bgmesh::Curve circle{};circle.kind=2;circle.dimension=3;circle.radii={1.,1.};
  circle.frame={0.,0.,0.,1.,0.,0.,0.,1.,0.,0.,0.,1.};circle.parameter_range={0.,1.};
  family("circle_original_formula",[&](auto &context,const auto &t){return bgmesh::canonical_analytic_curve(context,circle,t);});
  family("positive_rational_source_dispatch",[&](auto &context,const auto &t){return bgmesh::canonical_original_curve(context,store,bgmesh::Slot<bgmesh::CurveTag>{4},t);});
  return 0;
 }catch(const std::exception &e){std::cerr<<"FAIL\t"<<e.what()<<'\n';return 1;}
 catch(const bgfd::Failure &e){std::cerr<<"FAIL\t"<<e.what()<<'\n';return 1;}
}
