#include "../native/mesher/geometry_domain.hpp"
#include <filesystem>
#include <iostream>
#include <stdexcept>
namespace {
using namespace bgmesh;
void require(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
void identity(const std::filesystem::path &path,bool missing_partition=false) {
 const auto resource=std::make_shared<RunResource>(uint64_t{1}<<28);
 const auto source=load_source(path,resource);bgfd::Budget b({uint64_t{1}<<27,uint64_t{1}<<36},resource.get());
 const auto geometry=compile_geometry(source,resource,{{OwnerKind::geometry,0,143},0,0},b);GeometryContext ctx(b);
 const auto chart=choose_chart(ctx,geometry,{.face={0},.generation=7,.positional_allowance=Rational(b,1),
  .node_source_allowance=Rational(b,1).divide(Rational(b,1000000)),.allow_plane_profile=false});
 const std::array<std::array<int64_t,2>,4> sites{{{0,0},{1,0},{1,1},{0,1}}};
 std::pmr::vector<DomainArcInput> arcs(&b);
 for(uint32_t i=0;i<4;++i) {
  const auto j=(i+1)%4;
  arcs.push_back({{i},{400+i,0},i,j,{Rational(b,0),Rational(b,1)},{Rational(b,0),Rational(b,1)},
   {Rational(b,sites[i][0]),Rational(b,sites[i][1]),Rational(b,sites[j][0]),Rational(b,sites[j][1])}});
 }
 if(missing_partition) {
  bool refused=false;
  try {static_cast<void>(construct_domain_map(ctx,{chart,{0},{0,7,0},arcs,resource,{{OwnerKind::face,0,900},7,1}}));}
  catch(const Error &e){refused=e.code==ErrorCode::invalid_source;}
  require(refused,"whole original map must not omit mandatory source-t continuation partition");
  return;
 }
 {
  bool refused=false;
  try {static_cast<void>(construct_domain_map(ctx,{chart,{0},{0,7,0},{},resource,{{OwnerKind::face,0,900},7,1}}));}
  catch(const Error &e){refused=e.code==ErrorCode::invalid_source;}
  require(refused,"whole original face map must reject omitted complete source-loop inventory");
 }
 const auto result=construct_domain_map(ctx,{chart,{0},{0,7,0},arcs,resource,{{OwnerKind::face,0,900},7,1}});
 require(!result.boundary_request&&result.map,"actual authoritative identity domain retains named original policy");
 require(result.map->fans.empty()&&result.map->strips.empty(),"exact original straight arcs need no artificial supports");
 const std::array<Rational,6> triangle{Rational(b,0),Rational(b,0),Rational(b,1),Rational(b,0),Rational(b,0),Rational(b,1)};
 require(bound_domain_displacement(ctx,result.map,triangle).upper.sign()==0,"actual identity map consumes no positional allowance");
}
void nonlinear(const std::filesystem::path &path,bool fan,bool mixed) {
 const auto resource=std::make_shared<RunResource>(uint64_t{1}<<28);
 const auto source=load_source(path,resource);bgfd::Budget b({uint64_t{1}<<27,uint64_t{1}<<36},resource.get());
 const auto geometry=compile_geometry(source,resource,{{OwnerKind::geometry,0,144},0,0},b);GeometryContext ctx(b);
 const auto chart=choose_chart(ctx,geometry,{.face={0},.generation=7,.positional_allowance=Rational(b,1),
  .node_source_allowance=Rational(b,1).divide(Rational(b,1000000)),.allow_plane_profile=false});
 const auto zero=Rational(b,0),one=Rational(b,1),half=one.divide(Rational(b,2));
 const auto q=Rational::from_double(b,1./3.);
 std::pmr::vector<DomainArcInput> arcs(&b);
 if(fan) {
  arcs.push_back({{0},{400,1},0,1,{zero,half},{zero,half},{zero,zero,q,zero}});
  arcs.push_back({{0},{400,2},1,2,{half,one},{half,one},{q,zero,one,zero}});
 } else arcs.push_back({{0},{400,0},0,1,{zero,one},{zero,one},{zero,zero,one,zero}});
 const uint32_t first=fan?2U:1U,last=fan?4U:3U;
 arcs.push_back({{1},{401,0},first,first+1,{zero,one},{zero,one},{one,zero,one,one}});
 arcs.push_back({{2},{402,0},first+1,last,{zero,one},{zero,one},{one,one,zero,one}});
 arcs.push_back({{3},{403,0},last,0,{zero,one},{zero,one},{zero,one,zero,zero}});
 const auto result=construct_domain_map(ctx,{chart,{0},{0,7,0},arcs,resource,{{OwnerKind::face,0,900},7,1}});
 require(result.map&&!result.boundary_request,"actual nonlinear original source builds finite B/T map");
 require(result.map->fans.size()==(fan?1U:0U)&&result.map->strips.size()==(fan?0U:1U),
  "nondyadic rational node requires fan; genuinely curved trim requires strip");
 require(result.map->policy==(mixed?DomainReferencePolicy::hybrid_pullback:DomainReferencePolicy::authoritative_original),
  "per-use authority determines explicit actual reference policy");
 if(fan) {
  const auto &f=result.map->fans.front();const auto a=Rational(b,1).divide(Rational(b,3));
  const auto p0=import_exact(b,result.map->values.view(f.polygon[0]));
  const auto p1=import_exact(b,result.map->values.view(f.polygon[2]));
  // B(A)=Q follows a specific actual source point/selected site, not a rounded A.
  require(import_exact(b,result.map->values.view(f.source_enclosure[0])).compare(a)==0&&f.chosen_site==1,
   "fan retains exact original rational A=1/3 and chosen original site identity");
  const auto radius=p1.subtract(p0).divide(Rational(b,8));
  const std::array<Rational,6> cell{q.subtract(radius),zero,q.add(radius),zero,q,radius};
  const auto bound=bound_domain_displacement(ctx,result.map,cell);
  const auto power=[&](const Rational &u){return u.multiply(Rational(b,3)).subtract(u.multiply(u)).divide(Rational(b,2));};
  const auto exact=power(a).subtract(power(q));
  require(bound.upper.compare(exact)>=0&&bound.upper.compare(one.divide(Rational(b,1LL<<40)))<0,
   "local domain allowance encloses independent original POWER displacement without a global diameter");
  require(protected_domain_point(b,result.map,{q,zero}).has_value(),"actual fan protects its source feature before insertion");
 } else {
  const auto &s=result.map->strips.front();
  require(import_exact(b,result.map->values.view(s.slope_upper)).compare(one.divide(Rational(b,16)))==0,
   "independent y'= (1-2t)/16 and x'=1 yield exact graph slope1/16");
  const std::array<Rational,6> cell{zero,zero,one,zero,zero,one};
  const auto bound=bound_domain_displacement(ctx,result.map,cell);
  require(bound.upper.compare(one.divide(Rational(b,32)))==0&&bound.feature&&bound.feature->owner==400,
   "same source domain bound charges m/2 times exact unit Sv, not separate full tau");
  const std::array<Rational,6> interior{zero,half,one,half,zero,one};
  require(bound_domain_displacement(ctx,result.map,interior).upper.sign()==0,"indexed cells away from strips consume zero domain budget");
 }
}

}
int main(int argc,char **argv){try{require(argc==2,"expected fixture directory");const std::filesystem::path root(argv[1]);identity(root/"authoritative-joint.bgsource");
 identity(root/"authoritative-map-partition.bgsource",true);
 nonlinear(root/"authoritative-rational-node.bgsource",true,false);
 nonlinear(root/"authoritative-curved-trim.bgsource",false,false);
 nonlinear(root/"mixed-curved-trim.bgsource",false,true);
 std::cout<<"PASS\toriginal authoritative domain maps\n";return 0;}catch(const std::exception &e){std::cerr<<"FAIL\t"<<e.what()<<'\n';return 1;}catch(const bgfd::Failure &e){std::cerr<<"FAIL\t"<<e.what()<<'\n';return 1;}}
