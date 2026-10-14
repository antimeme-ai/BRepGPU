// Direct native ownership oracle. Each scenario must run in a fresh process.
#include "brepgpu_projection.h"
#include "brepgpu_discovery.h"
#include <cfenv>
#include <xmmintrin.h>
#include "runtime_test_hooks.h"
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <iostream>
#include <mutex>
#include <new>
#include <stdexcept>
#include <string>
#include <thread>
extern "C" void bg_fault_mode(int);
extern "C" int bg_fault_live(int);
extern "C" int bg_fault_factory_calls();
namespace {
void check(bool v,const char* message){if(!v)throw std::runtime_error(message);}
bg_diagnostic diagnostic(){bg_diagnostic d{};d.abi_version=BG_ABI_VERSION;d.struct_size=sizeof(d);return d;}
void close_ok(bg_handle h){auto d=diagnostic();check(bg1_close(h,&d,sizeof(d))==BG_OK,"close failed");}
struct Fixture {
    bg_handle context=0,patch=0,query=0,result=0;
    explicit Fixture(const char* plugin,bool gpu=true,uint32_t threads=1){
        bg_context_desc c{};c.abi_version=BG_ABI_VERSION;c.struct_size=sizeof(c);c.backend=gpu?BG_BACKEND_CUDA:BG_BACKEND_CPU;c.threads=threads;
        if(gpu){c.plugin_path=plugin;c.plugin_path_bytes=std::strlen(plugin);}
        auto d=diagnostic();check(bg1_create_context(&c,sizeof(c),&context,&d,sizeof(d))==BG_OK,"context failed");
        bg_patch_desc p{};p.abi_version=BG_ABI_VERSION;p.struct_size=sizeof(p);p.control_count=1;p.uv[1]=p.uv[3]=1;p.scale=1;
        const double controls[]={0,0,0,1};d=diagnostic();check(bg1_create_patch_set(context,&p,1,sizeof(p),controls,4,&patch,&d,sizeof(d))==BG_OK,"patch failed");
        const uint32_t slot=0;const double uv[]={0.5,0.5};d=diagnostic();check(bg1_create_query_set(patch,&slot,1,uv,2,&query,&d,sizeof(d))==BG_OK,"query failed");
    }
    void evaluate(){auto d=diagnostic();check(bg1_evaluate(query,0,&result,&d,sizeof(d))==BG_OK,"evaluate failed");}
};
bg_projection_options options(){return {BG_PROJECTION_ABI_VERSION,sizeof(bg_projection_options),64,24,BG_PROJECTION_DEFAULT_TOLERANCE,0,0};}
bg_handle projection_query(Fixture&f){const uint32_t slot=0;const double target[]={0,0,0},seed[]={0.5,0.5};bg_handle q=0;auto d=diagnostic();
    check(bg2_create_projection_query_set(f.patch,&slot,1,target,3,seed,2,&q,&d,sizeof(d))==BG_OK,"projection query failed");return q;}
bg_handle projection_result(bg_handle q){auto o=options();auto d=diagnostic();bg_handle r=0;check(bg2_project(q,&o,sizeof(o),&r,&d,sizeof(d))==BG_OK,"projection result failed");return r;}

bg_hierarchy_options hierarchy_options(uint32_t mode=0){return {3,sizeof(bg_hierarchy_options),8,0,BG_HIERARCHY_MAX_NODES,mode,0};}
bg_discovery_options discovery_options(){return {3,sizeof(bg_discovery_options),8191,8,BG_DISCOVERY_DEFAULT_TOLERANCE_SQUARED,0,0};}
bg_handle hierarchy(Fixture& f,uint32_t mode=0){auto o=hierarchy_options(mode);auto d=diagnostic();bg_handle h=0;
    check(bg3_create_hierarchy(f.patch,&o,sizeof(o),&h,&d,sizeof(d))==BG_OK,"hierarchy failed");return h;}
bg_handle discovery_query(bg_handle h){const uint32_t slot=0;const double target[3]={0,0,0};bg_handle q=0;auto d=diagnostic();
    check(bg3_create_discovery_query_set(h,&slot,1,target,3,&q,&d,sizeof(d))==BG_OK,"discovery query failed");return q;}
bg_handle discovery_result(bg_handle q){auto o=discovery_options();auto d=diagnostic();bg_handle r=0;
    check(bg3_discover(q,&o,sizeof(o),&r,&d,sizeof(d))==BG_OK,"discovery result failed");return r;}
bg_status discovery_download(bg_handle r,double*fields,bg_diagnostic&d){uint32_t slabs[6]{};
    return bg3_discovery_download(r,fields,8,slabs,1,slabs+1,1,slabs+2,1,slabs+3,1,slabs+4,1,slabs+5,1,&d,sizeof(d));}

struct Gate {std::mutex mutex;std::condition_variable cv;bg_handle target=0;bool arrived=false,resume=false;};
void pause_admission(uint32_t event,bg_handle handle,void* data){auto&g=*static_cast<Gate*>(data);if(event!=BG_TEST_ADMISSION||handle!=g.target)return;
    std::unique_lock<std::mutex> l(g.mutex);g.arrived=true;g.cv.notify_all();g.cv.wait(l,[&]{return g.resume;});}
void throw_publication(uint32_t event,bg_handle,void*){if(event==BG_TEST_PUBLICATION)throw std::bad_alloc();}
void assert_drained(){for(int i=0;i<9;++i)check(bg_fault_live(i)==0,"backend token leaked");}
void cpu_still_operates(const char* plugin){Fixture cpu(plugin,false);cpu.evaluate();double j[3];uint32_t status;auto d=diagnostic();check(bg1_download(cpu.result,j,3,&status,1,&d,sizeof(d))==BG_OK,"CPU blocked by CUDA poison");close_ok(cpu.context);}
}
int main(int argc,char**argv){try{
    check(argc==3,"usage: brepgpu_runtime_faults SCENARIO PLUGIN");const std::string scenario=argv[1];const char*plugin=argv[2];
    if(scenario=="admission-close"){
        Fixture f(plugin);Gate gate;gate.target=f.query;bg_test_set_hook(pause_admission,&gate);bg_status status=99;bg_handle output=99;
        std::thread worker([&]{auto d=diagnostic();status=bg1_evaluate(f.query,0,&output,&d,sizeof(d));});
        {std::unique_lock<std::mutex>l(gate.mutex);check(gate.cv.wait_for(l,std::chrono::seconds(10),[&]{return gate.arrived;}),"admission hook timed out");}
        close_ok(f.context);
        {std::lock_guard<std::mutex>l(gate.mutex);gate.resume=true;}gate.cv.notify_all();worker.join();bg_test_set_hook(nullptr,nullptr);
        check(status==BG_INVALID_HANDLE&&output==0,"closed context admitted promoted query");assert_drained();
    }else if(scenario=="publication-oom"){
        Fixture f(plugin);bg_test_set_hook(throw_publication,nullptr);auto d=diagnostic();bg_handle output=99;
        check(bg1_evaluate(f.query,0,&output,&d,sizeof(d))==BG_OUT_OF_MEMORY&&output==0,"publication OOM exposed result");
        bg_test_set_hook(nullptr,nullptr);check(bg_fault_live(3)==0,"unpublished result token leaked");f.evaluate();close_ok(f.context);assert_drained();
    }else if(scenario=="publication-oom-fatal-cleanup"){
        Fixture f(plugin);bg_fault_mode(4);bg_test_set_hook(throw_publication,nullptr);auto d=diagnostic();bg_handle output=99;
        check(bg1_evaluate(f.query,0,&output,&d,sizeof(d))==BG_OUT_OF_MEMORY&&output==0,"fatal cleanup masked original publication OOM");
        check((d.flags&BG_DIAG_FATAL)&&std::strstr(d.message,"cleanup"),"publication OOM lost fatal cleanup cause");
        bg_test_set_hook(nullptr,nullptr);bg_fault_mode(0);check(bg_fault_live(3)==0,"fatally cleaned unpublished token leaked");
        d=diagnostic();check(bg1_evaluate(f.query,0,&output,&d,sizeof(d))==BG_CONTEXT_UNUSABLE&&output==0,"fatal unpublished cleanup did not poison CUDA");
        d=diagnostic();check(bg1_close(f.context,&d,sizeof(d))==BG_DEVICE_FAILURE&&(d.flags&BG_DIAG_FATAL),"fatal cleanup history lost on close");assert_drained();cpu_still_operates(plugin);
    }else if(scenario=="cleanup-fatal"){
        Fixture f(plugin);f.evaluate();bg_fault_mode(1);auto d=diagnostic();
        check(bg1_close(f.context,&d,sizeof(d))==BG_DEVICE_FAILURE,"cleanup failure lost");
        check((d.flags&BG_DIAG_FATAL)&&d.backend_code==2&&std::strstr(d.message,"secondary"),"first cleanup diagnostic lost later fatal cause");
        bg_fault_mode(0);bg_context_desc c{};c.abi_version=BG_ABI_VERSION;c.struct_size=sizeof(c);c.backend=BG_BACKEND_CUDA;c.threads=1;c.plugin_path=plugin;c.plugin_path_bytes=std::strlen(plugin);bg_handle output=99;d=diagnostic();
        check(bg1_create_context(&c,sizeof(c),&output,&d,sizeof(d))==BG_CONTEXT_UNUSABLE&&output==0,"fatal cleanup did not poison CUDA");assert_drained();cpu_still_operates(plugin);
    }else if(scenario=="download-fatal"){
        Fixture f(plugin);f.evaluate();bg_fault_mode(2);double j[]={-1,-1,-1};uint32_t status=99;auto d=diagnostic();
        check(bg1_download(f.result,j,3,&status,1,&d,sizeof(d))==BG_DEVICE_FAILURE&&(d.flags&BG_DIAG_FATAL),"fatal download reported success");
        check(j[0]==123&&j[1]==-1&&status==99,"modeled partial destination witness missing");
        bg_handle output=99;d=diagnostic();check(bg1_evaluate(f.query,0,&output,&d,sizeof(d))==BG_CONTEXT_UNUSABLE&&output==0,"fatal download left resident context usable");
        bg_fault_mode(0);close_ok(f.context);assert_drained();cpu_still_operates(plugin);
    }else if(scenario=="restore-failure"){
        Fixture f(plugin);bg_fault_mode(3);bg_handle output=99;auto d=diagnostic();
        check(bg1_evaluate(f.query,0,&output,&d,sizeof(d))==BG_DEVICE_FAILURE&&output==0&&!(d.flags&BG_DIAG_FATAL),"restore failure published result or falsely poisoned CUDA");
        check(bg_fault_live(3)==0,"failed callback leaked result");bg_fault_mode(0);f.evaluate();close_ok(f.context);assert_drained();
    }else if(scenario=="projection-admission-close"){
        Fixture f(plugin);const auto q=projection_query(f);Gate gate;gate.target=q;bg_test_set_hook(pause_admission,&gate);bg_status status=99;bg_handle output=99;
        std::thread worker([&]{auto d=diagnostic();auto o=options();status=bg2_project(q,&o,sizeof(o),&output,&d,sizeof(d));});
        {std::unique_lock<std::mutex>l(gate.mutex);check(gate.cv.wait_for(l,std::chrono::seconds(10),[&]{return gate.arrived;}),"projection admission timed out");}
        close_ok(f.context);{std::lock_guard<std::mutex>l(gate.mutex);gate.resume=true;}gate.cv.notify_all();worker.join();bg_test_set_hook(nullptr,nullptr);
        check(status==BG_INVALID_HANDLE&&output==0,"closed context admitted promoted projection query");assert_drained();
    }else if(scenario=="projection-parent-close"){
        Fixture f(plugin);const auto q=projection_query(f),r=projection_result(q);close_ok(q);close_ok(f.patch);
        bg_projection_info info{BG_PROJECTION_ABI_VERSION,sizeof(bg_projection_info)};auto d=diagnostic();check(bg2_projection_info(r,&info,sizeof(info),&d,sizeof(d))==BG_OK,"parent close invalidated result");
        bg_patch_desc p{};uint32_t slot=99;double target[3],seed[2];auto o=options();d=diagnostic();
        check(bg2_projection_sources(r,&p,1,sizeof(p),&slot,1,target,3,seed,2,&o,sizeof(o),&d,sizeof(d))==BG_OK,"parent close lost source provenance");
        check(slot==0&&target[0]==0&&seed[0]==0.5&&o.max_iterations==64,"retained source mismatch");close_ok(f.context);assert_drained();
    }else if(scenario=="projection-publication-oom-fatal-cleanup"){
        Fixture f(plugin);const auto q=projection_query(f);bg_fault_mode(18);bg_test_set_hook(throw_publication,nullptr);auto d=diagnostic();auto o=options();bg_handle output=99;
        check(bg2_project(q,&o,sizeof(o),&output,&d,sizeof(d))==BG_OUT_OF_MEMORY&&output==0,"projection publication OOM lost identity");
        check((d.flags&BG_DIAG_FATAL)&&std::strstr(d.message,"cleanup"),"projection OOM lost fatal cleanup");bg_test_set_hook(nullptr,nullptr);bg_fault_mode(0);
        check(bg_fault_live(5)==0,"unpublished projection token leaked");d=diagnostic();check(bg1_evaluate(f.query,0,&output,&d,sizeof(d))==BG_CONTEXT_UNUSABLE&&output==0,"projection poison did not cross into evaluator");
        d=diagnostic();check(bg1_close(f.context,&d,sizeof(d))==BG_DEVICE_FAILURE,"projection cleanup history lost");assert_drained();cpu_still_operates(plugin);
    }else if(scenario=="projection-download-fatal"){
        Fixture f(plugin);const auto q=projection_query(f),r=projection_result(q);double fields[7];for(auto&v:fields)v=-1;uint32_t s=99,u=99,e=99,flags=99;auto d=diagnostic();
        bg_fault_mode(19);check(bg2_projection_download(r,fields,7,&s,1,&u,1,&e,1,&flags,1,&d,sizeof(d))==BG_DEVICE_FAILURE&&(d.flags&BG_DIAG_FATAL),"partial projection copy reported success");
        check(fields[0]==456&&fields[1]==-1&&s==99&&u==99&&e==99&&flags==99,"partial projection witness missing");
        bg_handle output=99;d=diagnostic();check(bg1_evaluate(f.query,0,&output,&d,sizeof(d))==BG_CONTEXT_UNUSABLE&&output==0,"download poison did not cross into evaluator");
        bg_fault_mode(0);close_ok(f.context);assert_drained();cpu_still_operates(plugin);
    }else if(scenario=="projection-diagnostic-alias"){
        Fixture f(plugin);const auto q=projection_query(f),r=projection_result(q);
        alignas(bg_diagnostic) unsigned char storage[320]{};unsigned char before[320];
        auto* diag=reinterpret_cast<bg_diagnostic*>(storage);*diag=diagnostic();std::memcpy(before,storage,sizeof(storage));
        uint32_t status=99,updates=99,evaluations=99,flags=99;
        check(bg2_projection_download(r,reinterpret_cast<double*>(storage),7,&status,1,&updates,1,&evaluations,1,&flags,1,diag,sizeof(*diag))==BG_INVALID_ARGUMENT,"diagnostic alias admitted successful download");
        check(std::memcmp(storage,before,sizeof(storage))==0&&status==99&&updates==99&&evaluations==99&&flags==99,"diagnostic-alias rejection wrote destinations");
        uint32_t slot=99;double targets[3]={99,99,99},seeds[2]={99,99};auto o=options();
        check(bg2_projection_sources(r,reinterpret_cast<bg_patch_desc*>(storage),1,sizeof(bg_patch_desc),&slot,1,targets,3,seeds,2,&o,sizeof(o),diag,sizeof(*diag))==BG_INVALID_ARGUMENT,"diagnostic alias admitted successful sources");
        check(std::memcmp(storage,before,sizeof(storage))==0&&slot==99&&targets[0]==99&&seeds[0]==99,"source diagnostic-alias rejection wrote destinations");
        close_ok(f.context);assert_drained();
    }else if(scenario=="evaluation-poison-projection"){
        Fixture f(plugin);const auto q=projection_query(f),r=projection_result(q);f.evaluate();
        double fields[7];uint32_t s=99,u=99,e=99,flags=99;auto d=diagnostic();
        check(bg2_projection_download(r,fields,7,&s,1,&u,1,&e,1,&flags,1,&d,sizeof(d))==BG_OK,"completed projection host copy failed");
        const double saved=fields[0];const auto saved_status=s;bg_fault_mode(2);double jets[3];uint32_t js=99;d=diagnostic();
        check(bg1_download(f.result,jets,3,&js,1,&d,sizeof(d))==BG_DEVICE_FAILURE&&(d.flags&BG_DIAG_FATAL),"evaluator fatal model failed");
        bg_handle output=99;auto o=options();d=diagnostic();check(bg2_project(q,&o,sizeof(o),&output,&d,sizeof(d))==BG_CONTEXT_UNUSABLE&&output==0,"evaluator poison did not cross into projection");
        bg_projection_info info{BG_PROJECTION_ABI_VERSION,sizeof(bg_projection_info)};d=diagnostic();check(bg2_projection_info(r,&info,sizeof(info),&d,sizeof(d))==BG_CONTEXT_UNUSABLE,"poisoned resident projection result remained usable");
        check(fields[0]==saved&&s==saved_status,"prior completed host arrays were changed");bg_fault_mode(0);close_ok(f.context);assert_drained();cpu_still_operates(plugin);
    }else if(scenario=="projection-restore-failure"){
        Fixture f(plugin);const auto q=projection_query(f);auto o=options();bg_handle output=99;auto d=diagnostic();bg_fault_mode(20);
        check(bg2_project(q,&o,sizeof(o),&output,&d,sizeof(d))==BG_DEVICE_FAILURE&&output==0&&!(d.flags&BG_DIAG_FATAL),"projection restore failure publication/poison mismatch");
        check(bg_fault_live(5)==0,"failed projection callback leaked");bg_fault_mode(0);const auto r=projection_result(q);close_ok(r);close_ok(f.context);assert_drained();
    }else if(scenario=="projection-extension-refusal"||scenario=="projection-extension-base"||scenario=="projection-extension-size"){
        Fixture f(plugin);bg_fault_mode(scenario=="projection-extension-refusal"?10:scenario=="projection-extension-base"?11:16);
        const uint32_t slot=0;const double target[3]={0,0,0},seed[2]={0.5,0.5};bg_handle out=99;auto d=diagnostic();
        check(bg2_create_projection_query_set(f.patch,&slot,1,target,3,seed,2,&out,&d,sizeof(d))==BG_UNSUPPORTED&&out==0,"invalid extension admitted projection");
        bg_fault_mode(0);d=diagnostic();check(bg2_create_projection_query_set(f.patch,&slot,1,target,3,seed,2,&out,&d,sizeof(d))==BG_UNSUPPORTED&&out==0,"stable refusal not retained");
        check(bg_fault_factory_calls()==1,"stable refusal renegotiated");f.evaluate();close_ok(f.context);assert_drained();
    }else if(scenario=="projection-extension-transient"){
        Fixture f(plugin);bg_fault_mode(12);const uint32_t slot=0;const double target[3]={0,0,0},seed[2]={0.5,0.5};bg_handle out=99;auto d=diagnostic();
        check(bg2_create_projection_query_set(f.patch,&slot,1,target,3,seed,2,&out,&d,sizeof(d))==BG_OUT_OF_MEMORY&&out==0,"factory OOM masked as unsupported");
        bg_fault_mode(0);projection_query(f);check(bg_fault_factory_calls()==2,"transient factory error cached");f.evaluate();close_ok(f.context);assert_drained();
    }else if(scenario=="projection-extension-fatal"||scenario=="projection-extension-fatal-ok"||scenario=="projection-extension-fatal-unsupported"){
        Fixture f(plugin);bg_fault_mode(scenario=="projection-extension-fatal"?13:scenario=="projection-extension-fatal-ok"?14:15);
        const uint32_t slot=0;const double target[3]={0,0,0},seed[2]={0.5,0.5};bg_handle out=99;auto d=diagnostic();
        const auto status=bg2_create_projection_query_set(f.patch,&slot,1,target,3,seed,2,&out,&d,sizeof(d));
        check(status==(scenario=="projection-extension-fatal-unsupported"?BG_UNSUPPORTED:BG_DEVICE_FAILURE)&&out==0&&(d.flags&BG_DIAG_FATAL),"factory fatal diagnostic swallowed");
        bg_fault_mode(0);d=diagnostic();check(bg1_evaluate(f.query,0,&out,&d,sizeof(d))==BG_CONTEXT_UNUSABLE&&out==0,"factory poison did not cross evaluator boundary");
        close_ok(f.context);assert_drained();cpu_still_operates(plugin);

    }else if(scenario=="discovery-parent-close"){
        Fixture f(plugin);const auto h=hierarchy(f,1),q=discovery_query(h),r=discovery_result(q);
        close_ok(f.query);close_ok(f.patch);close_ok(h);close_ok(q);
        check(bg_fault_live(6)==1&&bg_fault_live(7)==1&&bg_fault_live(8)==1,"retained discovery graph drained early");
        double fields[8];auto d=diagnostic();check(discovery_download(r,fields,d)==BG_OK,"retained result download failed");
        bg_patch_desc p{};uint32_t slot=99;double target[3];auto ho=hierarchy_options();auto o=discovery_options();d=diagnostic();
        check(bg3_discovery_sources(r,&p,1,sizeof(p),&slot,1,target,3,&ho,sizeof(ho),&o,sizeof(o),&d,sizeof(d))==BG_OK&&slot==0&&ho.flags==1,"retained sources lost");
        close_ok(r);check(bg_fault_live(6)==0&&bg_fault_live(7)==0&&bg_fault_live(8)==0,"retained graph leaked after result close");close_ok(f.context);assert_drained();
    }else if(scenario=="discovery-admission-close"){
        Fixture f(plugin);const auto h=hierarchy(f),q=discovery_query(h);Gate gate;gate.target=q;bg_test_set_hook(pause_admission,&gate);bg_status status=99;bg_handle output=99;
        std::thread worker([&]{auto o=discovery_options();auto d=diagnostic();status=bg3_discover(q,&o,sizeof(o),&output,&d,sizeof(d));});
        {std::unique_lock<std::mutex>l(gate.mutex);check(gate.cv.wait_for(l,std::chrono::seconds(10),[&]{return gate.arrived;}),"discovery admission hook timed out");}
        close_ok(f.context);{std::lock_guard<std::mutex>l(gate.mutex);gate.resume=true;}gate.cv.notify_all();worker.join();bg_test_set_hook(nullptr,nullptr);
        check(status==BG_INVALID_HANDLE&&output==0,"closed context admitted promoted discovery query");assert_drained();
    }else if(scenario=="discovery-publication-oom-fatal-cleanup"||scenario=="hierarchy-publication-oom-fatal-cleanup"||scenario=="discovery-query-publication-oom-fatal-cleanup"){
        Fixture f(plugin);bg_handle h=0,q=0;
        if(scenario!="hierarchy-publication-oom-fatal-cleanup")h=hierarchy(f);
        if(scenario=="discovery-publication-oom-fatal-cleanup")q=discovery_query(h);
        bg_fault_mode(scenario=="hierarchy-publication-oom-fatal-cleanup"?33:scenario=="discovery-query-publication-oom-fatal-cleanup"?34:35);
        bg_test_set_hook(throw_publication,nullptr);bg_handle output=99;auto d=diagnostic();bg_status code;
        if(scenario=="hierarchy-publication-oom-fatal-cleanup"){auto o=hierarchy_options();code=bg3_create_hierarchy(f.patch,&o,sizeof(o),&output,&d,sizeof(d));}
        else if(scenario=="discovery-query-publication-oom-fatal-cleanup"){const uint32_t slot=0;const double target[3]={0,0,0};code=bg3_create_discovery_query_set(h,&slot,1,target,3,&output,&d,sizeof(d));}
        else{auto o=discovery_options();code=bg3_discover(q,&o,sizeof(o),&output,&d,sizeof(d));}
        check(code==BG_OUT_OF_MEMORY&&output==0&&(d.flags&BG_DIAG_FATAL)&&std::strstr(d.message,"cleanup"),"discovery publication lost OOM/fatal cleanup");
        bg_test_set_hook(nullptr,nullptr);bg_fault_mode(0);d=diagnostic();check(bg1_evaluate(f.query,0,&output,&d,sizeof(d))==BG_CONTEXT_UNUSABLE&&output==0,"discovery publication poison did not cross evaluator");
        d=diagnostic();check(bg1_close(f.context,&d,sizeof(d))==BG_DEVICE_FAILURE&&(d.flags&BG_DIAG_FATAL),"discovery cleanup history lost");assert_drained();cpu_still_operates(plugin);
    }else if(scenario=="discovery-download-fatal"||scenario=="hierarchy-download-fatal"||scenario=="hierarchy-bound-download-fatal"){
        Fixture f(plugin);const auto h=hierarchy(f,1),q=discovery_query(h),r=discovery_result(q);const auto pq=projection_query(f);
        bg_fault_mode(scenario=="discovery-download-fatal"?38:scenario=="hierarchy-download-fatal"?39:40);auto d=diagnostic();bg_status code;
        if(scenario=="discovery-download-fatal"){double fields[8]{};code=discovery_download(r,fields,d);check(fields[0]==123,"modeled discovery partial-copy witness missing");}
        else if(scenario=="hierarchy-download-fatal"){bg_hierarchy_node nodes[1]{};uint32_t roots[1];code=bg3_hierarchy_download(h,nodes,1,sizeof(nodes),roots,1,&d,sizeof(d));check(nodes[0].lower[0]==123,"modeled node partial-copy witness missing");}
        else{double bounds[20]{};code=bg3_hierarchy_bound_download(h,bounds,20,&d,sizeof(d));check(bounds[0]==123,"modeled cache partial-copy witness missing");}
        check(code==BG_DEVICE_FAILURE&&(d.flags&BG_DIAG_FATAL),"discovery partial download fatal lost");bg_fault_mode(0);bg_handle out=99;auto po=options();d=diagnostic();
        check(bg2_project(pq,&po,sizeof(po),&out,&d,sizeof(d))==BG_CONTEXT_UNUSABLE&&out==0,"discovery poison did not cross projection");close_ok(f.context);assert_drained();cpu_still_operates(plugin);
    }else if(scenario=="discovery-diagnostic-alias"){
        Fixture f(plugin);const auto h=hierarchy(f),q=discovery_query(h),r=discovery_result(q);auto d=diagnostic();
        auto* info=new(d.message) bg_discovery_info{};info->abi_version=3;info->struct_size=sizeof(*info);bg_diagnostic saved=d;
        check(bg3_discovery_info(r,info,sizeof(*info),&d,sizeof(d))==BG_INVALID_ARGUMENT&&std::memcmp(&d,&saved,sizeof(d))==0,"discovery info diagnostic alias scribbled");
        auto* hi=new(d.message) bg_hierarchy_info{};hi->abi_version=3;hi->struct_size=sizeof(*hi);saved=d;
        check(bg3_hierarchy_info(h,hi,sizeof(*hi),&d,sizeof(d))==BG_INVALID_ARGUMENT&&std::memcmp(&d,&saved,sizeof(d))==0,"hierarchy info diagnostic alias scribbled");
        auto ho=hierarchy_options();auto*out=new(d.message) bg_handle(99);saved=d;
        check(bg3_create_hierarchy(f.patch,&ho,sizeof(ho),out,&d,sizeof(d))==BG_INVALID_ARGUMENT&&std::memcmp(&d,&saved,sizeof(d))==0,"creator handle diagnostic alias scribbled");
        double fields[8];d=diagnostic();check(discovery_download(r,fields,d)==BG_OK,"alias refusal damaged resident result");close_ok(f.context);assert_drained();
    }else if(scenario=="discovery-restore-failure"||scenario=="hierarchy-upload-failure"||scenario=="discovery-query-allocation-failure"){
        Fixture f(plugin);bg_handle h=0,q=0,out=99;auto d=diagnostic();bg_status code;
        if(scenario!="hierarchy-upload-failure")h=hierarchy(f);
        if(scenario=="discovery-restore-failure")q=discovery_query(h);
        bg_fault_mode(scenario=="hierarchy-upload-failure"?31:scenario=="discovery-query-allocation-failure"?36:32);
        if(scenario=="hierarchy-upload-failure"){auto o=hierarchy_options();code=bg3_create_hierarchy(f.patch,&o,sizeof(o),&out,&d,sizeof(d));}
        else if(scenario=="discovery-query-allocation-failure"){const uint32_t slot=0;const double target[3]={0,0,0};code=bg3_create_discovery_query_set(h,&slot,1,target,3,&out,&d,sizeof(d));}
        else{auto o=discovery_options();code=bg3_discover(q,&o,sizeof(o),&out,&d,sizeof(d));}
        check(code==(scenario=="discovery-restore-failure"?BG_DEVICE_FAILURE:BG_OUT_OF_MEMORY)&&out==0&&!(d.flags&BG_DIAG_FATAL),"discovery callback refusal publication mismatch");
        bg_fault_mode(0);if(!h)h=hierarchy(f);if(!q)q=discovery_query(h);const auto r=discovery_result(q);close_ok(r);close_ok(f.context);assert_drained();
    }else if(scenario=="discovery-extension-refusal"||scenario=="discovery-extension-base"||scenario=="discovery-extension-size"){
        Fixture f(plugin);bg_fault_mode(scenario=="discovery-extension-refusal"?50:scenario=="discovery-extension-base"?51:52);auto o=hierarchy_options();auto d=diagnostic();bg_handle out=99;
        check(bg3_create_hierarchy(f.patch,&o,sizeof(o),&out,&d,sizeof(d))==BG_UNSUPPORTED&&out==0,"invalid discovery extension admitted");bg_fault_mode(0);d=diagnostic();
        check(bg3_create_hierarchy(f.patch,&o,sizeof(o),&out,&d,sizeof(d))==BG_UNSUPPORTED&&out==0,"discovery stable refusal not cached");check(bg_fault_factory_calls()==1,"discovery stable refusal renegotiated");
        f.evaluate();const auto pq=projection_query(f),pr=projection_result(pq);close_ok(pr);close_ok(f.context);assert_drained();
    }else if(scenario=="discovery-extension-transient"){
        Fixture f(plugin);bg_fault_mode(53);auto o=hierarchy_options();auto d=diagnostic();bg_handle out=99;
        check(bg3_create_hierarchy(f.patch,&o,sizeof(o),&out,&d,sizeof(d))==BG_OUT_OF_MEMORY&&out==0,"discovery factory OOM cached as capability absence");bg_fault_mode(0);hierarchy(f);check(bg_fault_factory_calls()==2,"transient discovery factory failure cached");close_ok(f.context);assert_drained();
    }else if(scenario=="discovery-extension-fatal"||scenario=="discovery-extension-fatal-ok"||scenario=="discovery-extension-fatal-unsupported"){
        Fixture f(plugin);bg_fault_mode(scenario=="discovery-extension-fatal"?54:scenario=="discovery-extension-fatal-ok"?55:56);auto o=hierarchy_options();auto d=diagnostic();bg_handle out=99;
        const auto code=bg3_create_hierarchy(f.patch,&o,sizeof(o),&out,&d,sizeof(d));
        check(code==(scenario=="discovery-extension-fatal-unsupported"?BG_UNSUPPORTED:BG_DEVICE_FAILURE)&&out==0&&(d.flags&BG_DIAG_FATAL),"discovery factory fatal flags lost");bg_fault_mode(0);d=diagnostic();
        check(bg1_evaluate(f.query,0,&out,&d,sizeof(d))==BG_CONTEXT_UNUSABLE&&out==0,"discovery factory poison did not cross evaluator");close_ok(f.context);assert_drained();cpu_still_operates(plugin);
    }else if(scenario=="discovery-environment"){
        Fixture f(plugin,false,24);const auto h=hierarchy(f);const uint32_t slots[2]={0,0};const double targets[6]{};auto d=diagnostic();bg_handle q=0,empty=0;
        check(bg3_create_discovery_query_set(h,slots,2,targets,6,&q,&d,sizeof(d))==BG_OK,"environment query setup failed");d=diagnostic();
        check(bg3_create_discovery_query_set(h,nullptr,0,nullptr,0,&empty,&d,sizeof(d))==BG_OK,"empty query setup failed");
        auto o=discovery_options();auto ho=hierarchy_options();const unsigned saved=_mm_getcsr();
        const unsigned unsupported[]={saved|0x8000u,saved|0x40u,(saved&~0x6000u)|0x2000u,(saved&~0x3fu)&~0x80u};
        for(unsigned env:unsupported){_mm_setcsr(env);bg_handle out=99;d=diagnostic();const auto build=bg3_create_hierarchy(f.patch,&ho,sizeof(ho),&out,&d,sizeof(d));
            const bool refused=build==BG_UNSUPPORTED&&out==0;out=99;d=diagnostic();const auto work=bg3_discover(q,&o,sizeof(o),&out,&d,sizeof(d));const bool workers_refused=work==BG_UNSUPPORTED&&out==0;
            bg_handle er=0;d=diagnostic();const auto ecode=bg3_discover(empty,&o,sizeof(o),&er,&d,sizeof(d));
            auto invalid=o;const uint64_t snan=0x7ff0000000000001ull;std::memcpy(&invalid.tolerance_squared,&snan,8);out=99;d=diagnostic();const auto bad=bg3_discover(empty,&invalid,sizeof(invalid),&out,&d,sizeof(d));
            _mm_setcsr(saved);check(refused&&workers_refused,"unsupported actual CPU environment admitted");check(ecode==BG_OK&&er!=0,"empty discovery ran environment guard");check(bad==BG_INVALID_ARGUMENT&&out==0,"sNaN tolerance admitted or zeroing lost");close_ok(er);
        }
        discovery_result(q);close_ok(f.context);assert_drained();
    }else throw std::runtime_error("unknown scenario");
    std::cout<<scenario<<": PASS (owned callback model; not hardware fault injection)\n";return 0;
}catch(const std::exception&e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}
