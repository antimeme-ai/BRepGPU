#include "../native/mesher/geometry_boundary.hpp"
#include <filesystem>
#include <iostream>
#include <stdexcept>
namespace {
using bgmesh::Rational;
void require(bool condition,const char *message){if(!condition)throw std::runtime_error(message);}
void sample(const std::filesystem::path &path,bool plane,bool displaced_guide) {
 const auto resource=std::make_shared<bgmesh::RunResource>(uint64_t{1}<<27);
 bgfd::Budget b({uint64_t{1}<<27,uint64_t{1}<<35},resource.get());
 const auto source=bgmesh::load_source(path,resource);
 const auto store=bgmesh::compile_geometry(source,resource,{{bgmesh::OwnerKind::geometry,0,131},0,0},b);
 bgmesh::GeometryContext context(b);
 const Rational allowance=Rational(b,1).divide(Rational(b,1000000));
 const auto chart=bgmesh::choose_chart(context,store,{.face={0},.generation=11,
   .positional_allowance=Rational(b,1).divide(Rational(b,200)),.node_source_allowance=allowance,
   .allow_plane_profile=plane});
 const auto choice=bgmesh::locate_boundary_site(context,chart,{.use={0},
   .source_parameter=Rational(b,1).divide(Rational(b,4)),.node_source_allowance=allowance});
 require(choice.node_carrier_upper.compare(allowance)<=0,"actual original target to source at stored dyadic locator meets node cap");
 require(choice.source_parameter.compare(Rational(b,1).divide(Rational(b,4)))==0,
         "physical original t retained independently of mapped guide s");
 // Independent original POWER expression, not the producer's returned bound:
 // S=(3u/2-u*u/2,v,0), Ce(1/4)=(11/32,0,0).
 const auto u=choice.original_uv[0],v=choice.original_uv[1];
 const auto dx=u.multiply(Rational(b,3)).divide(Rational(b,2))
       .subtract(u.multiply(u).divide(Rational(b,2))).subtract(Rational(b,11).divide(Rational(b,32)));
 require(dx.multiply(dx).add(v.multiply(v)).compare(allowance.multiply(allowance))<=0,
         "actual stored recovery locator satisfies independently authored original polynomial node cap");
 if(plane) require(choice.chart[0].compare(Rational(b,11).divide(Rational(b,32)))==0,
                    "plane chart chooses original spatial image before UV parameterization");
 else require(choice.chart[0].compare(choice.original_uv[0])==0,"UV chart site is stored chosen source locator");
 const std::array<bgmesh::UseNodeRequest,2> joint_requests{{
   {.use={3},.source_parameter=Rational(b,1),.node_source_allowance=allowance},
   {.use={0},.source_parameter=Rational(b,0),.node_source_allowance=allowance}}};
 bgfd::Budget foreign({uint64_t{1}<<20,uint64_t{1}<<20},resource.get());
 const std::array<bgmesh::UseNodeRequest,2> foreign_requests{{
   {.use={3},.source_parameter=Rational(b,1),.node_source_allowance=allowance},
   {.use={0},.source_parameter=Rational(b,0),.node_source_allowance=Rational(foreign,1)}}};
 bool rejected_owner=false;
 try{static_cast<void>(bgmesh::choose_boundary_joint(context,chart,foreign_requests));}
 catch(const bgmesh::Error &e){rejected_owner=e.code==bgmesh::ErrorCode::invalid_source;}
 require(rejected_owner,"every incident request is admitted before any cross-owner arithmetic");
 const auto joint=bgmesh::choose_boundary_joint(context,chart,joint_requests);
 require(joint.occurrences.size()==2,"complete incoming/outgoing joint occurrences");
 for(const auto &occurrence:joint.occurrences)
   for(std::size_t axis=0;axis<2;++axis)
     require(occurrence.original_uv[axis].compare(joint.original_uv[axis])==0&&
             occurrence.chart[axis].compare(joint.chart[axis])==0,
             "incident uses consume literally one chosen joint, not independently rounded endpoints");
 require(joint.original_uv[0].sign()==0&&joint.original_uv[1].sign()==0,
         "joint is the original shared corner in this hand-derived source model");
 if(displaced_guide) {
   const auto error=choice.original_uv[0].subtract(Rational(b,1).divide(Rational(b,4)));
   require((error.sign()<0?error.negative():error).compare(Rational(b,1).divide(Rational(b,100000)))<0,
           "incorrect reconstructed guide is only a locator seed");
 }
}
void open_authoritative(const std::filesystem::path &path) {
 const auto resource=std::make_shared<bgmesh::RunResource>(uint64_t{1}<<27);
 bgfd::Budget b({uint64_t{1}<<27,uint64_t{1}<<35},resource.get());
 const auto source=bgmesh::load_source(path,resource);
 const auto store=bgmesh::compile_geometry(source,resource,{{bgmesh::OwnerKind::geometry,0,132},0,0},b);
 bgmesh::GeometryContext context(b);
 const auto allowance=Rational(b,1).divide(Rational(b,1000000));
 const auto chart=bgmesh::choose_chart(context,store,{.face={0},.generation=11,
   .positional_allowance=Rational(b,1).divide(Rational(b,200)),.node_source_allowance=allowance,
   .allow_plane_profile=false});
 const std::array<bgmesh::UseNodeRequest,2> requests{{
   {.use={3},.source_parameter=Rational(b,1),.node_source_allowance=allowance},
   {.use={0},.source_parameter=Rational(b,0),.node_source_allowance=allowance}}};
 bool rejected=false;
 try{static_cast<void>(bgmesh::choose_boundary_joint(context,chart,requests));}
 catch(const bgmesh::Error &e){rejected=e.code==bgmesh::ErrorCode::invalid_source;}
 require(rejected,"open original authoritative UV endpoints cannot be silently joined by a chosen polygon");
}
}
int main(int argc,char **argv) {
 try {
  require(argc==2,"expected source fixture directory");const std::filesystem::path root(argv[1]);
  sample(root/"warped-plane.bgsource",false,false);sample(root/"warped-plane.bgsource",true,false);
  sample(root/"mapped-guide.bgsource",false,true);
  sample(root/"authoritative-joint.bgsource",false,false);
  open_authoritative(root/"open-authoritative-joint.bgsource");
  std::cout<<"PASS\toriginal source boundary recovery\n";return 0;
 }catch(const std::exception &e){std::cerr<<"FAIL\t"<<e.what()<<'\n';return 1;}
 catch(const bgfd::Failure &e){std::cerr<<"FAIL\t"<<e.what()<<'\n';return 1;}
}
