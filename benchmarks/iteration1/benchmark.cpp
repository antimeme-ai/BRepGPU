#include "brepgpu.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

// Owned dyadic polynomial fixtures. Truth is evaluated from the expanded
// polynomial, independently of both Bernstein and de Casteljau evaluators.
using Clock=std::chrono::steady_clock;
double elapsed(Clock::time_point start) {
    return std::chrono::duration<double,std::milli>(Clock::now()-start).count();
}
bg_diagnostic diagnostic{};
void check(bg_status code) {
    if(code!=BG_OK) throw std::runtime_error("ABI code "+std::to_string(code)+": "+diagnostic.message);
}
void close(bg_handle& h) {
    if(h) { check(bg1_close(h,&diagnostic,sizeof diagnostic)); h=0; }
}
struct Handles {
    bg_handle context=0, patches=0, queries=0, result=0;
    ~Handles() {
        // Emergency cleanup after a failed run; no failed case produces rows.
        for(auto h:{result,queries,patches,context}) if(h) bg1_close(h,nullptr,0);
    }
    void children() {close(result);close(queries);close(patches);}
};

struct Fixture {
    uint64_t n; unsigned order, patch_count;
    std::vector<bg_patch_desc> patches;
    std::vector<double> controls, uv, jets;
    std::vector<uint32_t> slots, statuses;
    Fixture(uint64_t count,unsigned requested,unsigned np):n(count),order(requested),patch_count(np),
      patches(np),controls(np*64),uv(count*2),jets(count*3*(requested==0?1:requested==1?3:6)),
      slots(count),statuses(count) {
        const double c[4]={0,.25,.75,1};
        for(unsigned p=0;p<np;++p) {
            auto& d=patches[p];d.abi_version=BG_ABI_VERSION;d.struct_size=sizeof d;
            d.degree_u=d.degree_v=3;d.control_offset=p*16;d.control_count=16;
            d.scale=1;d.uv[1]=d.uv[3]=1;
            const double factor=np==1?1:.5, dx=np==1?0:(p%4)*.125, dy=np==1?0:(p/4)*.125;
            for(unsigned i=0;i<4;++i)for(unsigned j=0;j<4;++j) {
                auto k=p*64+(i*4+j)*4;
                controls[k]=factor*c[i]+dx;controls[k+1]=factor*c[j]+dy;
                controls[k+2]=factor*((i==1||i==2)&&(j==1||j==2)?.5:0);
                controls[k+3]=1;
            }
        }
        for(uint64_t i=0;i<n;++i) {
            slots[i]=np==1?0:static_cast<uint32_t>((i*2654435761ULL)%np);
            uv[2*i]=(static_cast<double>(i%1024)+.5)/1024;
            uv[2*i+1]=(static_cast<double>((i*37)%1024)+.5)/1024;
        }
    }
    void upload(Handles& h) {
        check(bg1_create_patch_set(h.context,patches.data(),patches.size(),patches.size()*sizeof(bg_patch_desc),
              controls.data(),controls.size(),&h.patches,&diagnostic,sizeof diagnostic));
        check(bg1_create_query_set(h.patches,slots.data(),n,uv.data(),uv.size(),&h.queries,&diagnostic,sizeof diagnostic));
    }
    void evaluate(Handles& h) {check(bg1_evaluate(h.queries,order,&h.result,&diagnostic,sizeof diagnostic));}
    void download(Handles& h) {check(bg1_download(h.result,jets.data(),jets.size(),statuses.data(),statuses.size(),&diagnostic,sizeof diagnostic));}
    void consume() const {
        for(auto status:statuses)if(status!=BG_QUERY_OK)throw std::runtime_error("failed query in benchmark");
    }
    double qualify() const {
        consume();double maximum=0;
        for(uint64_t i=0;i<n;++i) {
            long double s=uv[2*i],t=uv[2*i+1];
            long double f=patch_count==1?1:.5L,dx=patch_count==1?0:(slots[i]%4)*.125L;
            long double dy=patch_count==1?0:(slots[i]/4)*.125L;
            auto g=[](long double x){return .75L*x+.75L*x*x-.5L*x*x*x;};
            auto dg=[](long double x){return .75L+1.5L*x-1.5L*x*x;};
            long double truth[6][3]={
                {f*g(s)+dx,f*g(t)+dy,f*4.5L*s*(1-s)*t*(1-t)},
                {f*dg(s),0,f*4.5L*(1-2*s)*t*(1-t)},
                {0,f*dg(t),f*4.5L*s*(1-s)*(1-2*t)},
                {f*(1.5L-3*s),0,-f*9*t*(1-t)},
                {0,0,f*4.5L*(1-2*s)*(1-2*t)},
                {0,f*(1.5L-3*t),-f*9*s*(1-s)}};
            unsigned terms=order==0?1:order==1?3:6;
            for(unsigned term=0;term<terms;++term)for(unsigned axis=0;axis<3;++axis) {
                double value=jets[(term*3+axis)*n+i];
                double error=static_cast<double>(std::abs(static_cast<long double>(value)-truth[term][axis]));
                const double grade=std::ldexp(1.,term==0?-40:term<3?-30:-20);
                if(!std::isfinite(value)||error>grade)throw std::runtime_error("analytic benchmark qualification failed");
                maximum=std::max(maximum,error);
            }
        }
        return maximum;
    }
};

int main(int argc,char** argv) {
 try {
    if(argc!=8)throw std::runtime_error("usage: brepgpu_benchmark cpu|cuda device threads count order patches repetitions");
    std::string backend=argv[1];
    if(backend!="cpu"&&backend!="cuda")throw std::runtime_error("invalid backend");
    int device=std::stoi(argv[2]);unsigned threads=std::stoul(argv[3]);
    uint64_t count=std::stoull(argv[4]);unsigned order=std::stoul(argv[5]),np=std::stoul(argv[6]),reps=std::stoul(argv[7]);
    if(!count||count>1048576||order>2||(np!=1&&np!=16)||reps<1||reps>100||threads>48)
        throw std::runtime_error("case outside benchmark bounds");
    diagnostic.abi_version=BG_ABI_VERSION;diagnostic.struct_size=sizeof diagnostic;
    Fixture fixture(count,order,np);Handles h;
    bg_context_desc context{};context.abi_version=BG_ABI_VERSION;context.struct_size=sizeof context;
    context.backend=backend=="cpu"?BG_BACKEND_CPU:BG_BACKEND_CUDA;context.device=device;context.threads=threads;
    auto begin=Clock::now();
    check(bg1_create_context(&context,sizeof context,&h.context,&diagnostic,sizeof diagnostic));
    double initialization=elapsed(begin);
    fixture.upload(h);fixture.evaluate(h);fixture.download(h);double maxerror=fixture.qualify();close(h.result);
    struct Sample {double eval,download,host,cleanup,error;};std::vector<Sample> samples;
    for(unsigned sample=0;sample<reps;++sample) {
        Sample row{};begin=Clock::now();fixture.evaluate(h);row.eval=elapsed(begin);
        begin=Clock::now();fixture.download(h);row.download=elapsed(begin);
        row.error=fixture.qualify();close(h.result);
        // Resident inputs persist across public evaluate calls. A second input
        // pair is built for the prepared host-to-host path in the same context.
        Handles prepared;prepared.context=h.context;
        begin=Clock::now();fixture.upload(prepared);fixture.evaluate(prepared);fixture.download(prepared);fixture.consume();row.host=elapsed(begin);
        row.error=std::max(row.error,fixture.qualify());
        begin=Clock::now();prepared.children();row.cleanup=elapsed(begin);prepared.context=0;
        maxerror=std::max(maxerror,row.error);samples.push_back(row);
    }
    h.children();close(h.context);
    std::cout<<"backend\tdevice\tthreads\tcount\torder\tpatches\tsample\tphase\tms\tmax_abs_error\n"<<std::setprecision(17);
    auto emit=[&](int sample,const char* phase,double ms,double err){std::cout<<backend<<'\t'<<device<<'\t'<<threads<<'\t'<<count<<'\t'<<order<<'\t'<<np<<'\t'<<sample<<'\t'<<phase<<'\t'<<ms<<'\t'<<err<<'\n';};
    emit(-1,"context_first_use",initialization,maxerror);
    for(unsigned i=0;i<samples.size();++i) {
        auto& r=samples[i];emit(i,"resident_evaluate",r.eval,r.error);emit(i,"download",r.download,r.error);
        emit(i,"prepared_host_to_host",r.host,r.error);emit(i,"prepared_cleanup",r.cleanup,r.error);
    }
    if(backend=="cuda") {
        // Events are enabled in a separate context, outside public wall samples.
        Handles profiled;context.flags=BG_CONTEXT_PROFILE;
        check(bg1_create_context(&context,sizeof context,&profiled.context,&diagnostic,sizeof diagnostic));fixture.upload(profiled);
        fixture.evaluate(profiled);fixture.download(profiled);fixture.qualify();close(profiled.result);
        for(unsigned i=0;i<reps;++i) {
            fixture.evaluate(profiled);bg_result_info info{};info.abi_version=BG_ABI_VERSION;info.struct_size=sizeof info;
            check(bg1_result_info(profiled.result,&info,sizeof info,&diagnostic,sizeof diagnostic));
            fixture.download(profiled);double error=fixture.qualify();
            if(!(info.flags&BG_RESULT_PROFILED)||!std::isfinite(info.kernel_ms))throw std::runtime_error("profile unavailable");
            emit(i,"profiled_kernel",info.kernel_ms,error);close(profiled.result);
        }
        profiled.children();close(profiled.context);
    }
    return 0;
 } catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
