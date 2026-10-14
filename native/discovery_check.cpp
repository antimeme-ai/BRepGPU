#include "discovery_cpu.h"
#include "discovery_math.hpp"
#include <cmath>
#include <cfenv>
#include <iostream>
#include <stdexcept>
#if defined(__linux__) && defined(__x86_64__)
#include <xmmintrin.h>
#endif
namespace {
void require(bool value,const char* reason){if(!value)throw std::runtime_error(reason);}
bg_patch_desc patch(unsigned p,unsigned q,uint64_t offset=0){bg_patch_desc a{};
    a.abi_version=BG_ABI_VERSION;a.struct_size=sizeof(a);a.degree_u=p;a.degree_v=q;
    a.control_offset=offset;a.control_count=(p+1)*(q+1);return a;}
struct Row {double fields[8];uint32_t status,flags,visits,pruned,refinements,evaluations;};
Row run(const bg_patch_desc& p,const double* controls,const std::vector<bg_hierarchy_node>& nodes,
    const std::vector<uint32_t>& roots,const std::vector<double>& cache,uint32_t mode,
    const double* target,uint32_t visits=8191,uint32_t refinements=0){
    const uint32_t slot=0;const bg_discovery_options options{BG_DISCOVERY_ABI_VERSION,
        sizeof(bg_discovery_options),visits,refinements,0x1p-24,0,0};Row row{};
    brepgpu::discover_cpu_range(&p,1,controls,nodes.data(),nodes.size(),roots.data(),cache.data(),
        mode,&slot,target,1,&options,0,1,row.fields,&row.status,&row.flags,&row.visits,
        &row.pruned,&row.refinements,&row.evaluations);return row;
}
void environment(){
    require(brepgpu::discovery_environment_supported(),"qualified baseline environment unsupported");
#if defined(__linux__) && defined(__x86_64__)
    struct Restore {std::fenv_t saved{};unsigned mxcsr;
        Restore():mxcsr(_mm_getcsr()){require(std::fegetenv(&saved)==0,"fegetenv failed");}
        ~Restore(){std::fesetenv(&saved);_mm_setcsr(mxcsr);}} restore;
    require(std::fesetround(FE_DOWNWARD)==0,"rounding change failed");
    require(!brepgpu::discovery_environment_supported(),"directed caller environment accepted");
    require(std::fesetround(FE_TONEAREST)==0,"rounding restore failed");
    _mm_setcsr(restore.mxcsr|0x8000u);
    require(!brepgpu::discovery_environment_supported(),"FTZ environment accepted");
    _mm_setcsr(restore.mxcsr|0x0040u);
    require(!brepgpu::discovery_environment_supported(),"DAZ environment accepted");
    _mm_setcsr(restore.mxcsr&~0x0200u);
    require(!brepgpu::discovery_environment_supported(),"unmasked SSE exception accepted");
#endif
}
void analytic(){
    const double controls[]={0,0,-1,1, 0,1,0,1, 1,0,0,1, 1,1,1,1};
    const auto p=patch(1,1);const double target[]={.75,.75,-.25};
    for(uint32_t mode=0;mode<3;++mode){
        std::vector<bg_hierarchy_node> nodes;std::vector<uint32_t> roots;std::vector<double> cache;
        bg_hierarchy_options options{BG_DISCOVERY_ABI_VERSION,sizeof(bg_hierarchy_options),0,0,1048576,mode,0};
        require(brepgpu::build_discovery_hierarchy(&p,1,controls,options,nodes,roots,cache),"plane hierarchy failure");
        const Row row=run(p,controls,nodes,roots,cache,mode,target);
        require(row.fields[6]<=.1875&&row.fields[7]>=.1875,"dyadic plane minimum outside bracket");
        require(row.visits==1&&row.refinements==0&&row.evaluations==1,"root-only work counts wrong");
        require((row.flags&(BG_DISC_CANDIDATE_VALID|BG_DISC_COVERAGE_COMPLETE|BG_DISC_REFINEMENT_LIMIT))==
            (BG_DISC_CANDIDATE_VALID|BG_DISC_COVERAGE_COMPLETE|BG_DISC_REFINEMENT_LIMIT),"root-only flags wrong");
        if(mode==1)require(row.fields[6]>.18&&row.status==BG_DISC_GAP_SATISFIED,"DOP diagonal separation lost");
        options.depth=2;
        require(brepgpu::build_discovery_hierarchy(&p,1,controls,options,nodes,roots,cache),"split plane hierarchy failure");
        require(nodes.size()==7,"complete depth2 tree wrong");
        for(uint32_t budget: {1u,2u}){const Row limited=run(p,controls,nodes,roots,cache,mode,target,budget);
            require(limited.visits==1&&(limited.flags&BG_DISC_VISIT_LIMIT)&&
                !(limited.flags&BG_DISC_COVERAGE_COMPLETE),"two-child visit reservation/fallback wrong");
            require(limited.fields[6]==row.fields[6],"cap did not return identical root lower");}
    }
}
void rational_and_moments(){
    /* Original record offset exercises scalar and interval adapters together. */
    const double controls[]={-1,-1,-1,1, 1,0,0,3};const auto point=patch(0,0,1);
    const double target[]={0,0,0};
    std::vector<bg_hierarchy_node> nodes;std::vector<uint32_t> roots;std::vector<double> cache;
    bg_hierarchy_options options{BG_DISCOVERY_ABI_VERSION,sizeof(bg_hierarchy_options),8,0,1048576,2,0};
    require(brepgpu::build_discovery_hierarchy(&point,1,controls,options,nodes,roots,cache),"rational point build failed");
    const Row row=run(point,controls,nodes,roots,cache,2,target,8191,8);
    require(nodes.size()==1&&row.refinements==1&&row.evaluations==2,"constant/root duplicate refinement");
    /* 1/3 is rounded down in binary64: its ordinary square is not an upper. */
    const double ordinary=(1.0/3)*(1.0/3);
    require(row.fields[7]>ordinary,"ordinary rational point distance reused as certified upper");
    const double folded[]={-1,0,0,1, 1,0,0,1};const auto line=patch(1,0);options.depth=0;
    require(brepgpu::build_discovery_hierarchy(&line,1,folded,options,nodes,roots,cache),"moment build failed");
    require(cache[2]<=-1&&cache[3]>=-1&&cache[3]<0,"negative C0 middle coefficient clipped");
    for(unsigned k=0;k<5;++k)for(unsigned i=3;i<49;++i)
        require(cache[k*98+2*i]==0&&cache[k*98+2*i+1]==0,"cache padding not zero");
    const Row valid=run(line,folded,nodes,roots,cache,2,target);
    require(valid.status!=BG_DISC_ARITHMETIC,"inactive zero denominators entered query");
}
}
int main(){try{environment();analytic();rational_and_moments();
    std::cout<<"discovery native numerical check PASS\n";return 0;
}catch(const std::exception& e){std::cerr<<"discovery native numerical check FAILED: "<<e.what()<<'\n';return 1;}}
