#include "fixtures.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>

using namespace discovery_benchmark;
using Clock=std::chrono::steady_clock;
static bg_diagnostic diag{};
double elapsed(Clock::time_point t){return std::chrono::duration<double,std::milli>(Clock::now()-t).count();}
void check(bg_status s){if(s!=BG_OK)throw std::runtime_error("ABI failure "+std::to_string(s)+": "+diag.message);}
void close(bg_handle& h){if(h){check(bg1_close(h,&diag,sizeof diag));h=0;}}
struct Handles {
    bg_handle context=0,patches=0,hierarchy=0,queries=0,result=0;
    bool owns_context=true;
    ~Handles(){for(auto h:{result,queries,hierarchy,patches})if(h)bg1_close(h,nullptr,0);if(owns_context&&context)bg1_close(context,nullptr,0);}
    void children(){close(result);close(queries);close(hierarchy);close(patches);}
};
struct Outputs {
    std::vector<double> fields;
    std::vector<uint32_t> statuses,flags,visits,pruned,refinements,evaluations;
    explicit Outputs(uint64_t n):fields(8*n),statuses(n),flags(n),visits(n),pruned(n),refinements(n),evaluations(n){}
};
struct Stats {
    uint64_t gap=0,coverage=0,visit_limit=0,refinement_limit=0;
    uint64_t visits=0,pruned=0,refinements=0,evaluations=0;
    double sum_gap=0,max_gap=0;
    std::map<uint32_t,uint64_t> visit_histogram,refinement_histogram;
};
Stats consume(const Fixture& f,const Outputs& o,const bg_discovery_options& options) {
    const uint64_t n=f.slots.size();Stats s;
    for(uint64_t i=0;i<n;++i) {
        if(o.statuses[i]!=BG_DISC_GAP_SATISFIED&&o.statuses[i]!=BG_DISC_UNRESOLVED)
            throw std::runtime_error("invalid arithmetic/geometric row "+std::to_string(i));
        if(!(o.flags[i]&BG_DISC_CANDIDATE_VALID))throw std::runtime_error("absent witness");
        for(unsigned k=0;k<8;++k)if(!std::isfinite(o.fields[k*n+i]))throw std::runtime_error("nonfinite result");
        if(o.fields[i]<0||o.fields[i]>1||o.fields[n+i]<0||o.fields[n+i]>1)throw std::runtime_error("invalid UV witness");
        const double l=o.fields[6*n+i],u=o.fields[7*n+i],truth=f.truth[i];
        // All three operands are actual binary64 values and truth is exact.
        // Approximate XYZ/distance never supplies either certificate endpoint.
        if(l<0||l>truth||truth>u)throw std::runtime_error("bracket misses exact analytic minimum at row "+std::to_string(i));
        const bool gap=(o.flags[i]&BG_DISC_GAP)!=0;
        if(gap!=(o.statuses[i]==BG_DISC_GAP_SATISFIED))throw std::runtime_error("gap status/flag mismatch");
        if((o.flags[i]&BG_DISC_VISIT_LIMIT)&&(o.flags[i]&BG_DISC_COVERAGE_COMPLETE))throw std::runtime_error("false completed traversal");
        if(!o.visits[i]||o.visits[i]>options.max_visits||o.refinements[i]>options.max_refinements
            ||o.evaluations[i]>1+833*o.refinements[i]||o.pruned[i]>o.visits[i])throw std::runtime_error("work cap/counter mismatch");
        s.gap+=gap;s.coverage+=(o.flags[i]&BG_DISC_COVERAGE_COMPLETE)!=0;
        s.visit_limit+=(o.flags[i]&BG_DISC_VISIT_LIMIT)!=0;s.refinement_limit+=(o.flags[i]&BG_DISC_REFINEMENT_LIMIT)!=0;
        s.visits+=o.visits[i];s.pruned+=o.pruned[i];s.refinements+=o.refinements[i];s.evaluations+=o.evaluations[i];
        const double width=u-l;s.sum_gap+=width;s.max_gap=std::max(s.max_gap,width);
        ++s.visit_histogram[o.visits[i]];++s.refinement_histogram[o.refinements[i]];
    }
    return s;
}
void patch(const Fixture& f,Handles& h){check(bg1_create_patch_set(h.context,&f.patch,1,sizeof f.patch,f.controls.data(),f.controls.size(),&h.patches,&diag,sizeof diag));}
void hierarchy(Handles& h,const bg_hierarchy_options& o){check(bg3_create_hierarchy(h.patches,&o,sizeof o,&h.hierarchy,&diag,sizeof diag));}
void query(const Fixture& f,Handles& h){check(bg3_create_discovery_query_set(h.hierarchy,f.slots.data(),f.slots.size(),f.targets.data(),f.targets.size(),&h.queries,&diag,sizeof diag));}
void discover(Handles& h,const bg_discovery_options& o){check(bg3_discover(h.queries,&o,sizeof o,&h.result,&diag,sizeof diag));}
void download(Handles& h,Outputs& o){const auto n=o.statuses.size();check(bg3_discovery_download(h.result,o.fields.data(),o.fields.size(),o.statuses.data(),n,o.flags.data(),n,o.visits.data(),n,o.pruned.data(),n,o.refinements.data(),n,o.evaluations.data(),n,&diag,sizeof diag));}

int main(int argc,char**argv)try {
    if(argc!=10&&argc!=11)throw std::runtime_error("usage: discovery_benchmark cpu|cuda device threads count corpus mode depth repetitions max_refinements [work.tsv]");
    const std::string backend=argv[1],corpus=argv[5];const int device=std::stoi(argv[2]);
    const uint32_t threads=std::stoul(argv[3]),mode=std::stoul(argv[6]),depth=std::stoul(argv[7]),reps=std::stoul(argv[8]),ref=std::stoul(argv[9]);
    const uint64_t n=std::stoull(argv[4]);
    if((backend!="cpu"&&backend!="cuda")||!n||n>1048576||!reps||reps>100||mode>2||depth>12||ref>64)
        throw std::runtime_error("invalid benchmark arguments");
    diag.abi_version=BG_ABI_VERSION;diag.struct_size=sizeof diag;
    auto begin=Clock::now();Fixture f(n,corpus);Outputs out(n);const double generation=elapsed(begin);
    bg_context_desc c{};c.abi_version=BG_ABI_VERSION;c.struct_size=sizeof c;c.backend=backend=="cuda"?BG_BACKEND_CUDA:BG_BACKEND_CPU;c.device=device;c.threads=threads;
    bg_hierarchy_options ho{BG_DISCOVERY_ABI_VERSION,sizeof(bg_hierarchy_options),depth,0,BG_HIERARCHY_MAX_NODES,mode,0};
    bg_discovery_options options{BG_DISCOVERY_ABI_VERSION,sizeof(bg_discovery_options),BG_DISCOVERY_DEFAULT_VISITS,ref,BG_DISCOVERY_DEFAULT_TOLERANCE_SQUARED,0,0};
    if(corpus=="capped")options.max_visits=1;
    Handles h;begin=Clock::now();check(bg1_create_context(&c,sizeof c,&h.context,&diag,sizeof diag));const double first=elapsed(begin);
    begin=Clock::now();patch(f,h);const double initial_patch=elapsed(begin);
    begin=Clock::now();hierarchy(h,ho);const double initial_hierarchy=elapsed(begin);
    bg_hierarchy_info hi{};hi.abi_version=BG_DISCOVERY_ABI_VERSION;hi.struct_size=sizeof hi;check(bg3_hierarchy_info(h.hierarchy,&hi,sizeof hi,&diag,sizeof diag));
    const uint64_t stride=mode==0?0:mode==1?20:490,cache_bytes=hi.node_count*(sizeof(bg_hierarchy_node)+stride*sizeof(double))+hi.patch_count*sizeof(uint32_t);
    query(f,h);discover(h,options);download(h,out);const Stats initial=consume(f,out,options);close(h.result); // excluded warm-up
    std::ofstream work;
    if(argc==11){work.exceptions(std::ios::failbit|std::ios::badbit);work.open(argv[10]);work<<"backend\tdevice\tthreads\tcount\tcorpus\tmode\tdepth\tsample\tphase\tquantity\tvalue\trequests\n";}
    std::cout<<std::setprecision(17)<<"backend\tdevice\tthreads\tcount\tcorpus\tmode\tdepth\tmax_visits\tmax_refinements\ttolerance_squared\tsample\tphase\tms\tnodes\tcache_bytes\tgap_rows\tcoverage_rows\tvisit_limit_rows\trefinement_limit_rows\tmean_visits\tmean_pruned\tmean_refinements\tmean_evaluations\tmean_interval_witnesses\tmean_gap\tmax_gap\n";
    auto emit=[&](int sample,const char* phase,double ms,const Stats& s){std::cout<<backend<<'\t'<<device<<'\t'<<threads<<'\t'<<n<<'\t'<<corpus<<'\t'<<mode<<'\t'<<depth<<'\t'<<options.max_visits<<'\t'<<ref<<'\t'<<options.tolerance_squared<<'\t'<<sample<<'\t'<<phase<<'\t'<<ms<<'\t'<<hi.node_count<<'\t'<<cache_bytes<<'\t'<<s.gap<<'\t'<<s.coverage<<'\t'<<s.visit_limit<<'\t'<<s.refinement_limit<<'\t'<<double(s.visits)/n<<'\t'<<double(s.pruned)/n<<'\t'<<double(s.refinements)/n<<'\t'<<double(s.evaluations)/n<<'\t'<<1.+double(s.refinements)/n<<'\t'<<s.sum_gap/n<<'\t'<<s.max_gap<<'\n';};
    auto hist=[&](int sample,const char* phase,const Stats& s){if(!work)return;auto rows=[&](const char* quantity,const std::map<uint32_t,uint64_t>& values){for(const auto& e:values)work<<backend<<'\t'<<device<<'\t'<<threads<<'\t'<<n<<'\t'<<corpus<<'\t'<<mode<<'\t'<<depth<<'\t'<<sample<<'\t'<<phase<<'\t'<<quantity<<'\t'<<e.first<<'\t'<<e.second<<'\n';};rows("visits",s.visit_histogram);rows("refinements",s.refinement_histogram);};
    emit(-1,"input_generation_output_allocation",generation,initial);emit(-1,"context_first_use",first,initial);emit(-1,"initial_patch_snapshot_upload",initial_patch,initial);emit(-1,"initial_hierarchy_build_upload",initial_hierarchy,initial);
    for(uint32_t k=0;k<reps;++k){
        Handles setup;setup.context=h.context;setup.owns_context=false;
        begin=Clock::now();patch(f,setup);const double pm=elapsed(begin);
        begin=Clock::now();hierarchy(setup,ho);const double hm=elapsed(begin);
        emit(k,"patch_snapshot_upload",pm,initial);emit(k,"hierarchy_build_upload",hm,initial);setup.children();
        Handles request;request.context=h.context;request.owns_context=false;request.hierarchy=h.hierarchy;
        begin=Clock::now();query(f,request);const double qm=elapsed(begin);close(request.queries);request.hierarchy=0;emit(k,"query_snapshot_upload",qm,initial);
        begin=Clock::now();discover(h,options);const double resident=elapsed(begin);
        begin=Clock::now();download(h,out);const double dm=elapsed(begin);const Stats rs=consume(f,out,options);close(h.result);
        emit(k,"resident_discover",resident,rs);emit(k,"download_prepared",dm,rs);hist(k,"resident_discover",rs);
        Handles consumed;consumed.context=h.context;consumed.owns_context=false;consumed.hierarchy=h.hierarchy;
        begin=Clock::now();query(f,consumed);discover(consumed,options);download(consumed,out);const Stats hs=consume(f,out,options);const double host=elapsed(begin);
        begin=Clock::now();close(consumed.result);close(consumed.queries);consumed.hierarchy=0;const double cleanup=elapsed(begin);
        emit(k,"cached_hierarchy_host_to_host",host,hs);emit(k,"prepared_cleanup",cleanup,hs);hist(k,"cached_hierarchy_host_to_host",hs);
    }
    if(c.backend==BG_BACKEND_CUDA){
        Handles p;c.flags=BG_CONTEXT_PROFILE;check(bg1_create_context(&c,sizeof c,&p.context,&diag,sizeof diag));patch(f,p);hierarchy(p,ho);query(f,p);
        discover(p,options);download(p,out);consume(f,out,options);close(p.result);
        for(uint32_t k=0;k<reps;++k){discover(p,options);bg_discovery_info info{};info.abi_version=BG_DISCOVERY_ABI_VERSION;info.struct_size=sizeof info;
            check(bg3_discovery_info(p.result,&info,sizeof info,&diag,sizeof diag));download(p,out);const auto ps=consume(f,out,options);
            if(!(info.flags&BG_RESULT_PROFILED)||!std::isfinite(info.kernel_ms))throw std::runtime_error("missing actual kernel profile");
            emit(k,"profiled_kernel",info.kernel_ms,ps);hist(k,"profiled_kernel",ps);close(p.result);}
    }
    begin=Clock::now();h.children();close(h.context);emit(-1,"context_and_retained_cleanup",elapsed(begin),initial);
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
