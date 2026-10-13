// Direct native ownership oracle. Each scenario must run in a fresh process.
#include "brepgpu.h"
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
struct Gate {std::mutex mutex;std::condition_variable cv;bg_handle target=0;bool arrived=false,resume=false;};
void pause_admission(uint32_t event,bg_handle handle,void* data){auto&g=*static_cast<Gate*>(data);if(event!=BG_TEST_ADMISSION||handle!=g.target)return;
    std::unique_lock<std::mutex> l(g.mutex);g.arrived=true;g.cv.notify_all();g.cv.wait(l,[&]{return g.resume;});}
void throw_publication(uint32_t event,bg_handle,void*){if(event==BG_TEST_PUBLICATION)throw std::bad_alloc();}
void assert_drained(){for(int i=0;i<4;++i)check(bg_fault_live(i)==0,"backend token leaked");}
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
    }else throw std::runtime_error("unknown scenario");
    std::cout<<scenario<<": PASS (owned callback model; not hardware fault injection)\n";return 0;
}catch(const std::exception&e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}
