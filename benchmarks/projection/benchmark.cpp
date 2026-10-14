#include "brepgpu_projection.h"
#include "fixtures.hpp"
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <numeric>

using namespace projection_benchmark;
using Clock=std::chrono::steady_clock;
double elapsed(Clock::time_point begin) {return std::chrono::duration<double,std::milli>(Clock::now()-begin).count();}
bg_diagnostic diagnostic{};
void check(bg_status code) {if(code!=BG_OK)throw std::runtime_error("ABI code "+std::to_string(code)+": "+diagnostic.message);}
void close(bg_handle& handle) {if(handle){check(bg1_close(handle,&diagnostic,sizeof diagnostic));handle=0;}}
struct Handles {
    bg_handle context=0,patches=0,queries=0,result=0;
    bool owns_context=true;
    ~Handles(){for(auto h:{result,queries,patches})if(h)bg1_close(h,nullptr,0);if(owns_context&&context)bg1_close(context,nullptr,0);}
    void children(){close(result);close(queries);close(patches);}
};
struct Outputs {
    std::vector<double> fields;
    std::vector<uint32_t> statuses,updates,evaluations,flags;
    explicit Outputs(uint64_t n):fields(7*n),statuses(n),updates(n),evaluations(n),flags(n){}
};
struct Stats {
    std::array<uint64_t,9> statuses{};
    uint64_t positive=0,negative=0,ambiguous=0,update_sum=0,evaluation_sum=0;
    uint32_t update_min=UINT32_MAX,update_max=0,evaluation_min=UINT32_MAX,evaluation_max=0;
    double point_error=0,distance_error=0,rho_error=0;
    std::map<uint32_t,uint64_t> update_histogram,evaluation_histogram;
};
uint32_t expected_status(const Expected& e) {
    switch(e.outcome) {
    case Expected::Solved:return BG_PROJ_STATIONARY;
    case Expected::BadSlot:return BG_PROJ_BAD_PATCH;
    case Expected::BadParameter:return BG_PROJ_BAD_PARAMETER;
    case Expected::Rank:return BG_PROJ_RANK_UNRESOLVED;
    case Expected::Budget:return BG_PROJ_ITERATION_LIMIT;
    case Expected::Search:return BG_PROJ_LINESEARCH_LIMIT;
    }
    throw std::runtime_error("unknown expected outcome");
}
void consume(const Fixture& fixture,const Outputs& out) {
    for(uint64_t i=0;i<fixture.n;++i)if(out.statuses[i]!=expected_status(fixture.expected[i]))
        throw std::runtime_error("unexpected projection status at row "+std::to_string(i)+": "+std::to_string(out.statuses[i]));
}
Stats qualify(const Fixture& fixture,const Outputs& out,const bg_projection_options& options) {
    consume(fixture,out);Stats stats;
    constexpr long double E0=0x1p-40L,E1=0x1p-30L;
    const long double arithmetic=64*std::numeric_limits<double>::epsilon();
    for(uint64_t i=0;i<fixture.n;++i) {
        const auto& e=fixture.expected[i];auto status=out.statuses[i];++stats.statuses[status];
        stats.update_sum+=out.updates[i];stats.evaluation_sum+=out.evaluations[i];
        ++stats.update_histogram[out.updates[i]];++stats.evaluation_histogram[out.evaluations[i]];
        stats.update_min=std::min(stats.update_min,out.updates[i]);stats.update_max=std::max(stats.update_max,out.updates[i]);
        stats.evaluation_min=std::min(stats.evaluation_min,out.evaluations[i]);stats.evaluation_max=std::max(stats.evaluation_max,out.evaluations[i]);
        if(status==BG_PROJ_BAD_PATCH||status==BG_PROJ_BAD_PARAMETER) {
            for(unsigned k=0;k<7;++k)if(!std::isnan(out.fields[k*fixture.n+i]))throw std::runtime_error("invalid row retained numeric field");
            if(out.updates[i]||out.evaluations[i]||out.flags[i])throw std::runtime_error("invalid row retained counters/flags");
            continue;
        }
        for(unsigned k=0;k<7;++k)if(!std::isfinite(out.fields[k*fixture.n+i]))throw std::runtime_error("nonfinite candidate");
        long double s=out.fields[i],t=out.fields[fixture.n+i];
        if(s<0||s>1||t<0||t>1)throw std::runtime_error("candidate outside box");
        Geometry g=geometry(e.shape,s,t),seed=geometry(e.shape,fixture.seeds[2*i],fixture.seeds[2*i+1]);
        long double d2=0,seed_d2=0,distance_allowance=0,seed_allowance=0,grad[2]{},grad_allowance[2]{};
        for(unsigned axis=0;axis<3;++axis) {
            long double r=g.point[axis]-fixture.targets[3*i+axis],rs=seed.point[axis]-fixture.targets[3*i+axis];
            long double error=std::abs(static_cast<long double>(out.fields[(axis+2)*fixture.n+i])-g.point[axis]);
            if(error>E0)throw std::runtime_error("candidate position disagrees with expanded analytic map");
            stats.point_error=std::max(stats.point_error,static_cast<double>(error));d2+=r*r;seed_d2+=rs*rs;
            distance_allowance+=2*std::abs(r)*E0+E0*E0;seed_allowance+=2*std::abs(rs)*E0+E0*E0;
            const long double derivatives[]={g.ds[axis],g.dt[axis]};
            for(unsigned k=0;k<2;++k) {
                grad[k]+=r*derivatives[k];
                grad_allowance[k]+=std::abs(r)*E1+std::abs(derivatives[k])*E0+E0*E1+arithmetic*std::abs(r*derivatives[k]);
            }
        }
        distance_allowance+=arithmetic*std::max(1.L,d2);seed_allowance+=arithmetic*std::max(1.L,seed_d2);
        long double distance_error=std::abs(static_cast<long double>(out.fields[5*fixture.n+i])-d2);
        if(distance_error>distance_allowance)throw std::runtime_error("candidate squared distance disagrees with target");
        stats.distance_error=std::max(stats.distance_error,static_cast<double>(distance_error));
        const long double rho=std::max(std::abs(s-std::clamp(s-grad[0],0.L,1.L)),std::abs(t-std::clamp(t-grad[1],0.L,1.L)));
        const long double rho_allowance=std::max(grad_allowance[0],grad_allowance[1])+2*arithmetic;
        long double rho_error=std::abs(static_cast<long double>(out.fields[6*fixture.n+i])-rho);
        if(rho_error>rho_allowance)throw std::runtime_error("candidate projected gradient disagrees with analytic map");
        stats.rho_error=std::max(stats.rho_error,static_cast<double>(rho_error));
        if(d2>seed_d2+distance_allowance+seed_allowance)throw std::runtime_error("final exact distance exceeds seed beyond corpus budget");
        uint32_t flags=BG_PROJ_CANDIDATE_VALID;
        if(s==0||s==1||t==0||t==1)flags|=BG_PROJ_BOUNDARY;
        if(status==BG_PROJ_STATIONARY) {
            if(out.fields[6*fixture.n+i]>options.gradient_tolerance||rho>options.gradient_tolerance+rho_allowance)
                throw std::runtime_error("stationary row violates declared residual predicate");
            if(e.curvature==Expected::Positive){flags|=BG_PROJ_CURVATURE_POSITIVE;++stats.positive;}
            else if(e.curvature==Expected::Negative){flags|=BG_PROJ_CURVATURE_NEGATIVE;++stats.negative;}
            else if(e.curvature==Expected::Ambiguous){flags|=BG_PROJ_CURVATURE_AMBIGUOUS;++stats.ambiguous;}
            else throw std::runtime_error("stationary fixture has no independent curvature expectation");
            if(e.known_uv) {
                // These known solutions have reduced distance-Hessian
                // eigenvalues >=1. This local corpus allowance is not an API
                // promise or a cylinder Hessian bound throughout the whole box.
                const long double uv_allowance=8*options.gradient_tolerance+arithmetic;
                if(std::abs(s-e.s)>uv_allowance||std::abs(t-e.t)>uv_allowance)
                    throw std::runtime_error("stable basin missed independently known solution");
                const auto answer=geometry(e.shape,e.s,e.t);
                long double answer_d2=0,map_allowance=0;
                for(unsigned axis=0;axis<3;++axis) {
                    const long double r=answer.point[axis]-fixture.targets[3*i+axis];answer_d2+=r*r;
                    // Named maps have |Ds_axis|+|Dt_axis| <=6 throughout
                    // these basins (cylinder <=4, parabolic <=6).
                    const long double position_allowance=6*uv_allowance+E0;
                    if(std::abs(static_cast<long double>(out.fields[(axis+2)*fixture.n+i])-answer.point[axis])>position_allowance)
                        throw std::runtime_error("known solution position missed corpus map allowance");
                    map_allowance+=2*std::abs(r)*position_allowance+position_allowance*position_allowance;
                }
                if(std::abs(static_cast<long double>(out.fields[5*fixture.n+i])-answer_d2)>map_allowance+distance_allowance)
                    throw std::runtime_error("known solution distance missed corpus map allowance");
            }
        }
        if(status==BG_PROJ_RANK_UNRESOLVED)flags|=BG_PROJ_RANK_DEFICIENT;
        if(out.flags[i]!=flags)throw std::runtime_error("unexpected candidate/rank/curvature flags");
        if(out.updates[i]>options.max_iterations||out.evaluations[i]<out.updates[i]+1||out.evaluations[i]>BG_PROJECTION_MAX_EVALUATIONS)
            throw std::runtime_error("counter violates bounded solver contract");
        if(e.outcome==Expected::Search&&(out.updates[i]!=0||out.evaluations[i]!=2||s!=fixture.seeds[2*i]||t!=fixture.seeds[2*i+1]))
            throw std::runtime_error("exhausted full-trial witness changed seed/counters");
    }
    return stats;
}
void upload(const Fixture& f,Handles& h) {
    check(bg1_create_patch_set(h.context,f.patches.data(),f.patches.size(),f.patches.size()*sizeof(bg_patch_desc),f.controls.data(),f.controls.size(),&h.patches,&diagnostic,sizeof diagnostic));
    check(bg2_create_projection_query_set(h.patches,f.slots.data(),f.n,f.targets.data(),f.targets.size(),f.seeds.data(),f.seeds.size(),&h.queries,&diagnostic,sizeof diagnostic));
}
void project(Handles& h,const bg_projection_options& options){check(bg2_project(h.queries,&options,sizeof options,&h.result,&diagnostic,sizeof diagnostic));}
void download(Handles& h,Outputs& o){check(bg2_projection_download(h.result,o.fields.data(),o.fields.size(),o.statuses.data(),o.statuses.size(),o.updates.data(),o.updates.size(),o.evaluations.data(),o.evaluations.size(),o.flags.data(),o.flags.size(),&diagnostic,sizeof diagnostic));}

int main(int argc,char**argv) {
 try {
    if(argc!=7&&argc!=8)throw std::runtime_error("usage: brepgpu_projection_benchmark cpu|cuda device threads count corpus repetitions [work.tsv]");
    const std::string backend=argv[1],corpus=argv[5];int device=std::stoi(argv[2]);unsigned threads=std::stoul(argv[3]),reps=std::stoul(argv[6]);uint64_t count=std::stoull(argv[4]);
    if((backend!="cpu"&&backend!="cuda")||!count||count>1048576||threads<1||threads>48||reps<1||reps>100)throw std::runtime_error("case outside benchmark bounds");
    std::ofstream work;
    if(argc==8){work.exceptions(std::ios::failbit|std::ios::badbit);work.open(argv[7]);work<<"backend\tdevice\tthreads\tcount\tcorpus\tsample\tphase\tquantity\tvalue\trequests\n";}
    diagnostic.abi_version=BG_ABI_VERSION;diagnostic.struct_size=sizeof diagnostic;
    auto begin=Clock::now();Fixture fixture(count,corpus);Outputs out(count);double generation=elapsed(begin);
    bg_projection_options options{BG_PROJECTION_ABI_VERSION,sizeof(bg_projection_options),BG_PROJECTION_DEFAULT_ITERATIONS,BG_PROJECTION_DEFAULT_BACKTRACKS,BG_PROJECTION_DEFAULT_TOLERANCE,0,0};
    if(corpus=="adverse"){options.max_iterations=1;options.max_backtracks=0;}
    bg_context_desc context{};context.abi_version=BG_ABI_VERSION;context.struct_size=sizeof context;context.backend=backend=="cpu"?BG_BACKEND_CPU:BG_BACKEND_CUDA;context.device=device;context.threads=threads;
    Handles h;begin=Clock::now();check(bg1_create_context(&context,sizeof context,&h.context,&diagnostic,sizeof diagnostic));double first=elapsed(begin);
    upload(fixture,h);project(h,options);download(h,out);Stats initial=qualify(fixture,out,options);close(h.result);
    struct Sample {double snapshot,resident,download,host,cleanup;Stats stats,host_stats;};std::vector<Sample> samples;
    for(unsigned sample=0;sample<reps;++sample) {
        Sample row{};Handles snapshot;snapshot.context=h.context;snapshot.owns_context=false;
        begin=Clock::now();upload(fixture,snapshot);row.snapshot=elapsed(begin);snapshot.children();
        begin=Clock::now();project(h,options);row.resident=elapsed(begin);
        begin=Clock::now();download(h,out);row.download=elapsed(begin);row.stats=qualify(fixture,out,options);close(h.result);
        Handles prepared;prepared.context=h.context;prepared.owns_context=false;
        begin=Clock::now();upload(fixture,prepared);project(prepared,options);download(prepared,out);consume(fixture,out);row.host=elapsed(begin);
        row.host_stats=qualify(fixture,out,options);begin=Clock::now();prepared.children();row.cleanup=elapsed(begin);samples.push_back(row);
    }
    h.children();begin=Clock::now();close(h.context);double context_cleanup=elapsed(begin);
    std::cout<<"backend\tdevice\tthreads\tcount\tcorpus\tmax_iterations\tmax_backtracks\ttolerance\tsample\tphase\tms\tmax_point_error\tmax_distance_error\tmax_rho_error\tstationary\tbad_patch\tbad_parameter\tarithmetic\tunsupported\titeration_limit\tlinesearch_limit\tstagnation\trank_unresolved\tcurvature_positive\tcurvature_negative\tcurvature_ambiguous\tupdates_min\tupdates_max\tupdates_mean\tevaluations_min\tevaluations_max\tevaluations_mean\n"<<std::setprecision(17);
    auto emit=[&](int sample,const char*phase,double ms,const Stats& s) {
        std::cout<<backend<<'\t'<<device<<'\t'<<threads<<'\t'<<count<<'\t'<<corpus<<'\t'<<options.max_iterations<<'\t'<<options.max_backtracks<<'\t'<<options.gradient_tolerance<<'\t'<<sample<<'\t'<<phase<<'\t'<<ms<<'\t'<<s.point_error<<'\t'<<s.distance_error<<'\t'<<s.rho_error;
        for(auto value:s.statuses)std::cout<<'\t'<<value;
        std::cout<<'\t'<<s.positive<<'\t'<<s.negative<<'\t'<<s.ambiguous<<'\t'<<s.update_min<<'\t'<<s.update_max<<'\t'<<static_cast<double>(s.update_sum)/count<<'\t'<<s.evaluation_min<<'\t'<<s.evaluation_max<<'\t'<<static_cast<double>(s.evaluation_sum)/count<<'\n';
    };
    auto histogram=[&](int sample,const char*phase,const Stats&s) {
        if(argc!=8)return;
        for(const auto& item:std::array<std::pair<const char*,const std::map<uint32_t,uint64_t>*>,2>{{{"accepted_updates",&s.update_histogram},{"evaluations",&s.evaluation_histogram}}})
            for(const auto& bin:*item.second)work<<backend<<'\t'<<device<<'\t'<<threads<<'\t'<<count<<'\t'<<corpus<<'\t'<<sample<<'\t'<<phase<<'\t'<<item.first<<'\t'<<bin.first<<'\t'<<bin.second<<'\n';
    };
    emit(-1,"input_generation_and_host_output_allocation",generation,initial);emit(-1,"context_first_use",first,initial);emit(-1,"context_cleanup",context_cleanup,initial);
    histogram(-1,"excluded_warmup",initial);
    for(unsigned i=0;i<samples.size();++i){const auto&r=samples[i];emit(i,"snapshot_upload",r.snapshot,initial);emit(i,"resident_project",r.resident,r.stats);emit(i,"download_prepared",r.download,r.stats);emit(i,"prepared_host_to_host",r.host,r.host_stats);emit(i,"prepared_cleanup",r.cleanup,r.host_stats);histogram(i,"resident_project",r.stats);histogram(i,"prepared_host_to_host",r.host_stats);}
    if(backend=="cuda") {
        Handles profiled;context.flags=BG_CONTEXT_PROFILE;check(bg1_create_context(&context,sizeof context,&profiled.context,&diagnostic,sizeof diagnostic));upload(fixture,profiled);
        project(profiled,options);download(profiled,out);qualify(fixture,out,options);close(profiled.result);
        for(unsigned i=0;i<reps;++i) {
            project(profiled,options);bg_projection_info info{};info.abi_version=BG_PROJECTION_ABI_VERSION;info.struct_size=sizeof info;
            check(bg2_projection_info(profiled.result,&info,sizeof info,&diagnostic,sizeof diagnostic));download(profiled,out);auto s=qualify(fixture,out,options);
            if(!(info.flags&BG_RESULT_PROFILED)||info.field_count!=7||!std::isfinite(info.kernel_ms))throw std::runtime_error("profile unavailable");
            emit(i,"profiled_kernel",info.kernel_ms,s);histogram(i,"profiled_kernel",s);close(profiled.result);
        }
        profiled.children();close(profiled.context);
    }
    return 0;
 } catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
}
