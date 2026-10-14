// Direct native ownership oracle. Each scenario must run in a fresh process.
#include "brepgpu_projection.h"
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
    explicit Fixture(const char* plugin,bool gpu=true){
        bg_context_desc c{};c.abi_version=BG_ABI_VERSION;c.struct_size=sizeof(c);c.backend=gpu?BG_BACKEND_CUDA:BG_BACKEND_CPU;c.threads=1;
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

struct Gate {std::mutex mutex;std::condition_variable cv;bg_handle target=0;bool arrived=false,resume=false;};
void pause_admission(uint32_t event,bg_handle handle,void* data){auto&g=*static_cast<Gate*>(data);if(event!=BG_TEST_ADMISSION||handle!=g.target)return;
    std::unique_lock<std::mutex> l(g.mutex);g.arrived=true;g.cv.notify_all();g.cv.wait(l,[&]{return g.resume;});}
void throw_publication(uint32_t event,bg_handle,void*){if(event==BG_TEST_PUBLICATION)throw std::bad_alloc();}
void assert_drained(){for(int i=0;i<6;++i)check(bg_fault_live(i)==0,"backend token leaked");}
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
    }else throw std::runtime_error("unknown scenario");
    std::cout<<scenario<<": PASS (owned callback model; not hardware fault injection)\n";return 0;
}catch(const std::exception&e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}
