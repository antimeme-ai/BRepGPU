#include "backend_api.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <dlfcn.h>
#include <exception>
#include <limits>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#ifdef BG_ENABLE_TEST_HOOKS
#include "runtime_test_hooks.h"
#include <atomic>
namespace {
std::atomic<bg_test_hook_fn> test_hook{nullptr};
std::atomic<void*> test_hook_data{nullptr};
void run_test_hook(uint32_t event,bg_handle handle){auto hook=test_hook.load();if(hook)hook(event,handle,test_hook_data.load());}
}
extern "C" void bg_test_set_hook(bg_test_hook_fn hook,void* data){test_hook_data.store(data);test_hook.store(hook);}
#else
static void run_test_hook(uint32_t,bg_handle){}
#endif

namespace {
static_assert(sizeof(double)==8 && std::numeric_limits<double>::is_iec559);
static_assert(sizeof(bg_context_desc)==40 && sizeof(bg_patch_desc)==136);
static_assert(sizeof(bg_result_info)==48 && sizeof(bg_diagnostic)==288);
enum class Kind { Context, Patch, Query, Result };
struct Context;
struct Object {
    Kind kind;
    bg_handle id=0;
    std::shared_ptr<Context> context;
    explicit Object(Kind k):kind(k){}
    virtual ~Object()=default;
};
bg_diagnostic diagnostic(bg_status code=BG_OK, const char* message="", uint32_t stage=0) {
    bg_diagnostic d{};
    d.abi_version=BG_ABI_VERSION; d.struct_size=sizeof(d); d.code=code;
    d.stage=stage; d.query_index=-1;
    std::strncpy(d.message,message,sizeof(d.message)-1);
    return d;
}
struct Failure { bg_diagnostic value; };
[[noreturn]] void fail(bg_status code,const char* message,uint32_t stage=BG_STAGE_ARGUMENT) {
    throw Failure{diagnostic(code,message,stage)};
}
struct Registry {
    std::mutex mutex;
    std::unordered_map<bg_handle,std::shared_ptr<Object>> objects;
    uint64_t next=1;
    bool poisoned=false;
    bg_diagnostic poison{};
};
// Process lifetime storage has no CUDA-destructor work during global teardown.
// Explicit close drains all owned objects; the registry itself is process state.
Registry& registry(){ static Registry* value=new Registry; return *value; }
void observe(const bg_diagnostic& d) {
    if(d.flags & BG_DIAG_FATAL){ auto& r=registry(); std::lock_guard<std::mutex> l(r.mutex);
        r.poisoned=true; r.poison=d; }
}
void backend_status(bg_status code,bg_diagnostic& d) {
    d.message[sizeof(d.message)-1]=0;
    if(code!=BG_OK){ if(!d.code)d.code=code; observe(d); throw Failure{d}; }
    if(d.flags & BG_DIAG_FATAL){d.code=BG_DEVICE_FAILURE;observe(d);throw Failure{d};}
}
struct Plugin {
    void* library=nullptr;
    const bg_backend_v1* table=nullptr;
    ~Plugin(){if(library)dlclose(library);}
};
struct Context final:Object {
    std::recursive_mutex mutex;
    bool open=true;
    uint32_t backend=BG_BACKEND_CPU,threads=1,flags=0;
    std::unordered_set<bg_handle> children;
    std::shared_ptr<Plugin> plugin;
    void* token=nullptr;
    uint64_t device_children=0;
    bg_diagnostic cleanup_error=diagnostic();
    Context():Object(Kind::Context){}
    void record(bg_status code,bg_diagnostic d) noexcept {
        try{if(code!=BG_OK || (d.flags&BG_DIAG_FATAL)){
            if(code==BG_OK)code=BG_DEVICE_FAILURE;
            d.code=code;d.message[255]=0;
            if(cleanup_error.code==BG_OK)cleanup_error=d;
            else{
                cleanup_error.flags|=d.flags;
                const auto used=std::strlen(cleanup_error.message);
                if(used<sizeof(cleanup_error.message)-1){
                    std::strncat(cleanup_error.message,"; secondary: ",sizeof(cleanup_error.message)-1-used);
                    std::strncat(cleanup_error.message,d.message,sizeof(cleanup_error.message)-1-std::strlen(cleanup_error.message));
                }
            }
            observe(d);
        }}
        catch(...){}
    }
    void finish_backend() noexcept {
        if(open || device_children || !token)return;
        auto d=diagnostic();void* old=token;token=nullptr;
        try{const auto status=plugin->table->context_close(old,&d);record(status,d);}
        catch(...){record(BG_INTERNAL_ERROR,diagnostic(BG_INTERNAL_ERROR,"backend close threw",BG_STAGE_CLOSE));}
    }
    ~Context(){open=false;finish_backend();}
};
struct PatchSet final:Object {
    std::vector<bg_patch_desc> patches;
    std::vector<double> controls;
    void* token=nullptr;
    PatchSet():Object(Kind::Patch){}
    ~PatchSet(){cleanup();}
    void cleanup() noexcept;
};
struct QuerySet final:Object {
    std::shared_ptr<PatchSet> patches;
    std::vector<uint32_t> slots;
    std::vector<double> uv;
    void* token=nullptr;
    QuerySet():Object(Kind::Query){}
    ~QuerySet(){cleanup();}
    void cleanup() noexcept;
};
struct Result final:Object {
    std::shared_ptr<QuerySet> queries;
    uint32_t order=0;
    std::vector<double> jets;
    std::vector<uint32_t> statuses;
    bg_backend_profile profile{0,0,std::numeric_limits<double>::quiet_NaN()};
    void* token=nullptr;
    Result():Object(Kind::Result){}
    ~Result(){cleanup();}
    void cleanup() noexcept;
};
template<class Function> void cleanup_child(std::shared_ptr<Context>& c,void*& token,Function f) noexcept {
    if(!token)return;
    try{
        std::lock_guard<std::recursive_mutex> lock(c->mutex);
        auto d=diagnostic();void* old=token;token=nullptr;
        try{const auto status=f(c->token,old,&d);c->record(status,d);}catch(...){c->record(BG_INTERNAL_ERROR,diagnostic(BG_INTERNAL_ERROR,"backend child close threw",BG_STAGE_CLOSE));}
        if(c->device_children)--c->device_children;
        c->finish_backend();
    }catch(...){}
}
void PatchSet::cleanup()noexcept{if(token)cleanup_child(context,token,context->plugin->table->patch_close);}
void QuerySet::cleanup()noexcept{if(token)cleanup_child(context,token,context->plugin->table->query_close);}
void Result::cleanup()noexcept{if(token)cleanup_child(context,token,context->plugin->table->result_close);}

template<class T> void pointer(const T* p,uint64_t count) {
    if(count && (!p || reinterpret_cast<uintptr_t>(p)%alignof(T)))fail(BG_INVALID_ARGUMENT,"null or unaligned nonempty buffer");
    if(count>std::numeric_limits<size_t>::max()/sizeof(T) || count>std::vector<T>().max_size())fail(BG_INVALID_ARGUMENT,"buffer size overflow");
}
uint64_t multiply(uint64_t a,uint64_t b){if(b && a>UINT64_MAX/b)fail(BG_INVALID_ARGUMENT,"size multiplication overflow");return a*b;}
template<class T> void version(const T* p,uint64_t bytes){
    pointer(p,1);if(bytes<sizeof(T))fail(BG_INVALID_ARGUMENT,"short descriptor");
    if(p->abi_version!=BG_ABI_VERSION || p->struct_size!=sizeof(T))fail(BG_INVALID_ARGUMENT,"descriptor ABI or size mismatch");
}
template<class T> std::vector<T> snapshot(const T* p,uint64_t count){pointer(p,count);std::vector<T> v;if(count)v.assign(p,p+static_cast<size_t>(count));return v;}
uint32_t terms(uint32_t order){if(order>2)fail(BG_INVALID_ARGUMENT,"unsupported derivative order");return order==0?1u:order==1?3u:6u;}
void zero_handle(bg_handle* out){if(out && reinterpret_cast<uintptr_t>(out)%alignof(bg_handle)==0)*out=0;}
template<class F> bg_status call(bg_diagnostic* out,uint64_t bytes,F&& function) noexcept {
    bg_diagnostic d=diagnostic();
    if(out && (bytes<sizeof(*out) || reinterpret_cast<uintptr_t>(out)%alignof(bg_diagnostic)))return BG_INVALID_ARGUMENT;
    if(out && (out->abi_version!=BG_ABI_VERSION || out->struct_size!=sizeof(*out)))return BG_INVALID_ARGUMENT;
    try{function(d);}
    catch(const Failure& error){d=error.value;}
    catch(const std::bad_alloc&){d=diagnostic(BG_OUT_OF_MEMORY,"allocation failed");}
    catch(const std::length_error&){d=diagnostic(BG_INVALID_ARGUMENT,"container length overflow");}
    catch(const std::exception& e){d=diagnostic(BG_INTERNAL_ERROR,e.what());}
    catch(...){d=diagnostic(BG_INTERNAL_ERROR,"unknown native exception");}
    if(out)*out=d;
    return d.code;
}
std::shared_ptr<Object> lookup(bg_handle id) {
    auto& r=registry();std::lock_guard<std::mutex> lock(r.mutex);
    auto found=r.objects.find(id);if(!id || found==r.objects.end())fail(BG_INVALID_HANDLE,"invalid or closed handle");
    return found->second;
}
std::shared_ptr<Context> context_of(const std::shared_ptr<Object>& o){return o->kind==Kind::Context?std::static_pointer_cast<Context>(o):o->context;}
struct Admission {
    std::shared_ptr<Object> object;
    std::shared_ptr<Context> context;
    std::unique_lock<std::recursive_mutex> lock;
    Admission(bg_handle id,Kind kind,bool closing=false):object(lookup(id)),context(context_of(object)),lock(context->mutex,std::defer_lock){
        run_test_hook(1,id);
        lock.lock();
        if(object->kind!=kind)fail(BG_INVALID_HANDLE,"wrong handle kind");
        auto& r=registry();std::lock_guard<std::mutex> guard(r.mutex);
        auto found=r.objects.find(id);
        if(!context->open || found==r.objects.end() || found->second.get()!=object.get())fail(BG_INVALID_HANDLE,"handle or context closed before admission");
        if(!closing && context->backend==BG_BACKEND_CUDA && r.poisoned)fail(BG_CONTEXT_UNUSABLE,"CUDA backend requires process restart");
    }
};
bg_handle publish(const std::shared_ptr<Object>& o,const std::shared_ptr<Context>& c) {
    run_test_hook(2,o->id);
    auto& r=registry();std::lock_guard<std::mutex> lock(r.mutex);
    if(!c->open)fail(BG_INVALID_HANDLE,"context closed before publication");
    if(c->backend==BG_BACKEND_CUDA && r.poisoned)fail(BG_CONTEXT_UNUSABLE,"CUDA backend requires process restart");
    if(r.next==UINT64_MAX)fail(BG_OUT_OF_MEMORY,"handle namespace exhausted");
    const auto id=r.next++;
    r.objects.emplace(id,o);
    try{if(o->kind!=Kind::Context)c->children.insert(id);}catch(...){r.objects.erase(id);throw;}
    o->id=id;return id;
}
// Registry publication can fail after a completed native allocation. Releasing
// the unpublished object may itself report a device failure. Keep the original
// failure identity, but expose cleanup fatality as well as poisoning CUDA.
template<class T> bg_handle publish_owned(std::shared_ptr<T>& object,std::shared_ptr<Context> context){
    try{return publish(object,context);}
    catch(...){
        const auto original=std::current_exception();
        if(object->kind==Kind::Context){context->open=false;context->finish_backend();}
        object.reset();
        if(!context->cleanup_error.code)std::rethrow_exception(original);
        bg_diagnostic d=diagnostic();
        try{std::rethrow_exception(original);}
        catch(const Failure&e){d=e.value;}
        catch(const std::bad_alloc&){d=diagnostic(BG_OUT_OF_MEMORY,"allocation failed");}
        catch(const std::length_error&){d=diagnostic(BG_INVALID_ARGUMENT,"container length overflow");}
        catch(const std::exception&e){d=diagnostic(BG_INTERNAL_ERROR,e.what());}
        catch(...){d=diagnostic(BG_INTERNAL_ERROR,"unknown native exception");}
        d.flags|=context->cleanup_error.flags;
        std::strncat(d.message,"; cleanup: ",sizeof(d.message)-1-std::strlen(d.message));
        std::strncat(d.message,context->cleanup_error.message,sizeof(d.message)-1-std::strlen(d.message));
        throw Failure{d};
    }
}
void final_cuda_check(const std::shared_ptr<Context>& c){if(c->backend==BG_BACKEND_CUDA){auto&r=registry();std::lock_guard<std::mutex>l(r.mutex);if(r.poisoned)fail(BG_CONTEXT_UNUSABLE,"CUDA backend requires process restart");}}
void patch_metadata(const bg_patch_desc& p){
    if(p.abi_version!=BG_ABI_VERSION || p.struct_size!=sizeof(p) || (p.flags&~(BG_PATCH_LINEAGE|BG_PATCH_FRAME)))fail(BG_INVALID_ARGUMENT,"patch metadata ABI or flags mismatch");
    if(p.degree_u>3 || p.degree_v>3)fail(BG_UNSUPPORTED,"patch degree outside 0..3");
    if(p.control_count!=uint64_t(p.degree_u+1)*(p.degree_v+1))fail(BG_INVALID_ARGUMENT,"patch control count does not match degree");
    for(double x:p.uv)if(!std::isfinite(x))fail(BG_INVALID_ARGUMENT,"nonfinite source rectangle");
    if(p.flags&BG_PATCH_FRAME){for(double x:p.origin)if(!std::isfinite(x))fail(BG_INVALID_ARGUMENT,"nonfinite origin");if(!std::isfinite(p.scale))fail(BG_INVALID_ARGUMENT,"nonfinite scale");}
}
std::shared_ptr<Plugin> load_plugin(const bg_context_desc& desc){
    std::string path;
    if(desc.plugin_path_bytes){pointer(desc.plugin_path,desc.plugin_path_bytes);path.assign(desc.plugin_path,static_cast<size_t>(desc.plugin_path_bytes));if(path.find('\0')!=std::string::npos)fail(BG_INVALID_ARGUMENT,"embedded zero in plugin path");}
    else{Dl_info info{};if(!dladdr(reinterpret_cast<void*>(&bg1_abi_info),&info)||!info.dli_fname)fail(BG_BACKEND_UNAVAILABLE,"cannot locate core library",BG_STAGE_PLUGIN);path=info.dli_fname;auto slash=path.find_last_of('/');path=slash==std::string::npos?".":path.substr(0,slash);path+="/libbrepgpu_cuda.so";}
    auto p=std::make_shared<Plugin>();p->library=dlopen(path.c_str(),RTLD_NOW|RTLD_LOCAL);
    if(!p->library)fail(BG_BACKEND_UNAVAILABLE,dlerror(),BG_STAGE_PLUGIN);
    auto factory=reinterpret_cast<bg_backend_factory_fn>(dlsym(p->library,BG_BACKEND_FACTORY_SYMBOL));
    if(!factory)fail(BG_BACKEND_UNAVAILABLE,"CUDA backend factory missing",BG_STAGE_PLUGIN);
    auto d=diagnostic();backend_status(factory(BG_BACKEND_ABI_VERSION,&p->table,&d),d);
    const auto*t=p->table;
    if(!t || t->abi_version!=BG_BACKEND_ABI_VERSION || t->struct_size!=sizeof(bg_backend_v1))fail(BG_BACKEND_UNAVAILABLE,"CUDA backend ABI mismatch",BG_STAGE_PLUGIN);
    if(!(t->capabilities&BG_BACKEND_CAP_FP64) || !t->context_create || !t->context_close || !t->patch_create || !t->patch_close || !t->query_create || !t->query_close || !t->evaluate || !t->result_close || !t->download)fail(BG_BACKEND_UNAVAILABLE,"CUDA backend capability/function mismatch",BG_STAGE_PLUGIN);
    return p;
}
bool overlaps(const void*a,uint64_t a_bytes,const void*b,uint64_t b_bytes){if(!a_bytes||!b_bytes)return false;auto x=reinterpret_cast<uintptr_t>(a),y=reinterpret_cast<uintptr_t>(b);if(x>UINTPTR_MAX-a_bytes||y>UINTPTR_MAX-b_bytes)fail(BG_INVALID_ARGUMENT,"pointer extent overflow");return x<y+b_bytes && y<x+a_bytes;}
}

extern "C" bg_status bg1_abi_info(bg_abi_info*out,uint64_t bytes){return call(nullptr,0,[&](bg_diagnostic&){version(out,bytes);*out={BG_ABI_VERSION,sizeof(*out),sizeof(bg_context_desc),sizeof(bg_patch_desc),sizeof(bg_result_info),sizeof(bg_diagnostic),sizeof(double),sizeof(bg_handle),sizeof(uint32_t),sizeof(void*)};});}
extern "C" bg_status bg1_create_context(const bg_context_desc*desc,uint64_t bytes,bg_handle*out,bg_diagnostic*diag,uint64_t db){zero_handle(out);return call(diag,db,[&](bg_diagnostic&){
    pointer(out,1);version(desc,bytes);const auto input=*desc;
    if(input.backend>BG_BACKEND_CUDA || input.device<0 || !input.threads || input.threads>256 || (input.flags&~BG_CONTEXT_PROFILE))fail(BG_INVALID_ARGUMENT,"invalid context options");
    auto c=std::make_shared<Context>();c->backend=input.backend;c->threads=input.threads;c->flags=input.flags;
    if(c->backend==BG_BACKEND_CUDA){final_cuda_check(c);c->plugin=load_plugin(input);auto d=diagnostic();backend_status(c->plugin->table->context_create(&input,&c->token,&d),d);if(!c->token)fail(BG_INTERNAL_ERROR,"CUDA backend published no context",BG_STAGE_CONTEXT);}
    *out=publish_owned(c,c);
});}
extern "C" bg_status bg1_create_patch_set(bg_handle id,const bg_patch_desc*patches,uint64_t count,uint64_t bytes,const double*controls,uint64_t doubles,bg_handle*out,bg_diagnostic*diag,uint64_t db){zero_handle(out);return call(diag,db,[&](bg_diagnostic&){
    pointer(out,1);pointer(patches,count);if(count>=UINT32_MAX || multiply(count,sizeof(bg_patch_desc))!=bytes)fail(BG_INVALID_ARGUMENT,"patch descriptor extent mismatch");pointer(controls,doubles);if(doubles%4)fail(BG_INVALID_ARGUMENT,"controls must contain complete homogeneous records");
    Admission a(id,Kind::Context);auto p=std::make_shared<PatchSet>();p->context=a.context;p->patches=snapshot(patches,count);p->controls=snapshot(controls,doubles);
    for(const auto&v:p->patches){patch_metadata(v);if(v.control_offset>UINT64_MAX-v.control_count || multiply(v.control_offset+v.control_count,4)>doubles)fail(BG_INVALID_ARGUMENT,"control range exceeds storage");
        for(uint64_t i=0;i<v.control_count;++i){const double*x=p->controls.data()+4*(v.control_offset+i);if(!std::isfinite(x[3])||x[3]<1||x[3]>256)fail(BG_UNSUPPORTED,"weight outside 1..256");for(int k=0;k<3;++k)if(!std::isfinite(x[k])||std::fabs(x[k])>x[3])fail(BG_UNSUPPORTED,"control outside local envelope");}}
    if(a.context->backend==BG_BACKEND_CUDA){auto d=diagnostic();backend_status(a.context->plugin->table->patch_create(a.context->token,p->patches.data(),count,p->controls.data(),doubles,&p->token,&d),d);if(!p->token)fail(BG_INTERNAL_ERROR,"CUDA backend published no patch token");++a.context->device_children;}
    *out=publish_owned(p,a.context);
});}
extern "C" bg_status bg1_create_query_set(bg_handle id,const uint32_t*slots,uint64_t count,const double*uv,uint64_t doubles,bg_handle*out,bg_diagnostic*diag,uint64_t db){zero_handle(out);return call(diag,db,[&](bg_diagnostic&){
    pointer(out,1);pointer(slots,count);pointer(uv,doubles);if(multiply(count,2)!=doubles)fail(BG_INVALID_ARGUMENT,"UV/query count mismatch");
    Admission a(id,Kind::Patch);auto q=std::make_shared<QuerySet>();q->context=a.context;q->patches=std::static_pointer_cast<PatchSet>(a.object);q->slots=snapshot(slots,count);q->uv=snapshot(uv,doubles);
    if(a.context->backend==BG_BACKEND_CUDA){auto d=diagnostic();backend_status(a.context->plugin->table->query_create(a.context->token,q->patches->token,q->slots.data(),count,q->uv.data(),&q->token,&d),d);if(!q->token)fail(BG_INTERNAL_ERROR,"CUDA backend published no query token");++a.context->device_children;}
    *out=publish_owned(q,a.context);
});}
extern "C" bg_status bg1_evaluate(bg_handle id,uint32_t order,bg_handle*out,bg_diagnostic*diag,uint64_t db){zero_handle(out);return call(diag,db,[&](bg_diagnostic&){
    pointer(out,1);const auto k=terms(order);Admission a(id,Kind::Query);auto r=std::make_shared<Result>();r->context=a.context;r->queries=std::static_pointer_cast<QuerySet>(a.object);r->order=order;
    const auto n=uint64_t(r->queries->slots.size()),size=multiply(multiply(n,k),3);pointer<double>(nullptr,0);if(size>std::vector<double>().max_size())fail(BG_INVALID_ARGUMENT,"result size overflow");
    if(a.context->backend==BG_BACKEND_CUDA){auto d=diagnostic();backend_status(a.context->plugin->table->evaluate(a.context->token,r->queries->token,order,&r->token,&r->profile,&d),d);if(!r->token)fail(BG_INTERNAL_ERROR,"CUDA backend published no result token");++a.context->device_children;}
    else{r->jets.resize(static_cast<size_t>(size));r->statuses.resize(static_cast<size_t>(n));auto&q=*r->queries;auto&p=*q.patches;
        const uint64_t workers=std::min<uint64_t>(a.context->threads,n);std::vector<std::thread> threads;threads.reserve(static_cast<size_t>(workers));
        auto range=[&](uint64_t begin,uint64_t end){brepgpu::evaluate_cpu_range(p.patches.data(),p.patches.size(),p.controls.data(),q.slots.data(),q.uv.data(),n,order,begin,end,r->jets.data(),r->statuses.data());};
        auto boundary=[&](uint64_t i){return (n/workers)*i+std::min<uint64_t>(i,n%workers);};
        if(workers<=1)range(0,n);else{try{for(uint64_t i=0;i<workers;++i)threads.emplace_back(range,boundary(i),boundary(i+1));}catch(...){for(auto&t:threads)if(t.joinable())t.join();throw;}for(auto&t:threads)t.join();}}
    *out=publish_owned(r,a.context);
});}
extern "C" bg_status bg1_result_info(bg_handle id,bg_result_info*out,uint64_t bytes,bg_diagnostic*diag,uint64_t db){return call(diag,db,[&](bg_diagnostic&){
    version(out,bytes);Admission a(id,Kind::Result);auto r=std::static_pointer_cast<Result>(a.object);final_cuda_check(a.context);
    *out={BG_ABI_VERSION,sizeof(*out),r->queries->slots.size(),r->queries->patches->patches.size(),r->order,terms(r->order),a.context->backend,r->profile.flags,r->profile.kernel_ms};
});}
extern "C" bg_status bg1_download(bg_handle id,double*jets,uint64_t doubles,uint32_t*statuses,uint64_t count,bg_diagnostic*diag,uint64_t db){return call(diag,db,[&](bg_diagnostic&){
    pointer(jets,doubles);pointer(statuses,count);Admission a(id,Kind::Result);auto r=std::static_pointer_cast<Result>(a.object);const auto n=uint64_t(r->queries->slots.size());
    if(count!=n || doubles!=multiply(multiply(n,terms(r->order)),3))fail(BG_INVALID_ARGUMENT,"download capacity mismatch",BG_STAGE_DOWNLOAD);
    if(overlaps(jets,multiply(doubles,8),statuses,multiply(count,4)))fail(BG_INVALID_ARGUMENT,"overlapping download outputs");
    if(a.context->backend==BG_BACKEND_CUDA){auto d=diagnostic();backend_status(a.context->plugin->table->download(a.context->token,r->token,jets,doubles,statuses,count,&d),d);final_cuda_check(a.context);}
    else{if(doubles)std::copy(r->jets.begin(),r->jets.end(),jets);if(count)std::copy(r->statuses.begin(),r->statuses.end(),statuses);}
});}
extern "C" bg_status bg1_result_sources(bg_handle id,bg_patch_desc*patches,uint64_t pc,uint64_t bytes,uint32_t*slots,uint64_t count,double*uv,uint64_t doubles,bg_diagnostic*diag,uint64_t db){return call(diag,db,[&](bg_diagnostic&){
    pointer(patches,pc);pointer(slots,count);pointer(uv,doubles);Admission a(id,Kind::Result);auto r=std::static_pointer_cast<Result>(a.object);auto&q=*r->queries;
    if(pc!=q.patches->patches.size()||bytes!=multiply(pc,sizeof(bg_patch_desc))||count!=q.slots.size()||doubles!=multiply(count,2))fail(BG_INVALID_ARGUMENT,"provenance capacity mismatch");
    if(overlaps(patches,bytes,slots,multiply(count,4))||overlaps(patches,bytes,uv,multiply(doubles,8))||overlaps(slots,multiply(count,4),uv,multiply(doubles,8)))fail(BG_INVALID_ARGUMENT,"overlapping provenance outputs");
    final_cuda_check(a.context);if(pc)std::copy(q.patches->patches.begin(),q.patches->patches.end(),patches);if(count)std::copy(q.slots.begin(),q.slots.end(),slots);if(doubles)std::copy(q.uv.begin(),q.uv.end(),uv);
});}
extern "C" bg_status bg1_close(bg_handle id,bg_diagnostic*diag,uint64_t db){return call(diag,db,[&](bg_diagnostic&d){
    auto object=lookup(id);auto c=context_of(object);std::vector<std::shared_ptr<Object>> removed;
    {Admission a(id,object->kind,true);removed.reserve(object->kind==Kind::Context?c->children.size()+1:1);auto&r=registry();std::lock_guard<std::mutex>l(r.mutex);
        if(object->kind==Kind::Context){c->open=false;for(auto child:c->children){auto f=r.objects.find(child);if(f!=r.objects.end()){removed.push_back(std::move(f->second));r.objects.erase(f);}}c->children.clear();}
        else c->children.erase(id);
        auto f=r.objects.find(id);removed.push_back(std::move(f->second));r.objects.erase(f);
    }
    removed.clear();object.reset();
    {std::lock_guard<std::recursive_mutex>l(c->mutex);c->finish_backend();if(c->cleanup_error.code)d=c->cleanup_error;}
});}
extern "C" bg_status bg1_convert(const bg_patch_desc*patches,uint64_t pc,uint64_t bytes,const uint32_t*slots,uint64_t count,uint32_t order,const double*input,uint64_t input_count,const uint32_t*input_status,uint64_t sc,double*output,uint64_t output_count,uint32_t*output_status,uint64_t osc,bg_diagnostic*diag,uint64_t db){return call(diag,db,[&](bg_diagnostic&){
    const auto k=terms(order);const auto expected=multiply(multiply(count,k),3);pointer(patches,pc);pointer(slots,count);pointer(input,input_count);pointer(input_status,sc);pointer(output,output_count);pointer(output_status,osc);
    if(bytes!=multiply(pc,sizeof(bg_patch_desc))||input_count!=expected||output_count!=expected||sc!=count||osc!=count)fail(BG_INVALID_ARGUMENT,"conversion shape mismatch");
    for(uint64_t i=0;i<pc;++i)patch_metadata(patches[i]);
    const void*inputs[]={patches,slots,input,input_status};
    const uint64_t input_bytes[]={bytes,multiply(count,4),multiply(input_count,8),multiply(sc,4)};
    for(int j=0;j<4;++j)if(overlaps(output,multiply(output_count,8),inputs[j],input_bytes[j])||overlaps(output_status,multiply(osc,4),inputs[j],input_bytes[j]))fail(BG_INVALID_ARGUMENT,"overlapping conversion arrays");
    if(overlaps(output,multiply(output_count,8),output_status,multiply(osc,4)))fail(BG_INVALID_ARGUMENT,"overlapping conversion outputs");
    const double nan=std::numeric_limits<double>::quiet_NaN(),lo=std::ldexp(1.,-20),hi=std::ldexp(1.,20);
    const double bounds[6]={1+std::ldexp(1.,-40),2304+std::ldexp(1.,-30),2304+std::ldexp(1.,-30),3552768+std::ldexp(1.,-20),3552768+std::ldexp(1.,-20),3552768+std::ldexp(1.,-20)};
    for(uint64_t i=0;i<count;++i){uint32_t status=input_status[i];double factors[6]{};const bg_patch_desc*p=nullptr;
        if(status==BG_QUERY_OK){if(slots[i]>=pc)status=BG_QUERY_BAD_PATCH;else{p=&patches[slots[i]];const double du=p->uv[1]-p->uv[0],dv=p->uv[3]-p->uv[2],scale=p->flags&BG_PATCH_FRAME?p->scale:1.;
            bool supported=std::isfinite(du)&&std::isfinite(dv)&&du>=lo&&du<=hi&&dv>=lo&&dv<=hi&&scale>=lo&&scale<=hi;
            if(p->flags&BG_PATCH_FRAME)for(double x:p->origin)supported=supported&&std::fabs(x)<=std::ldexp(1.,40);
            if(!supported)status=BG_QUERY_UNSUPPORTED;
            else{factors[0]=scale;factors[1]=scale/du;factors[2]=scale/dv;factors[3]=factors[1]/du;factors[4]=factors[1]/dv;factors[5]=factors[2]/dv;
                for(uint32_t term=0;term<k;++term)for(uint32_t axis=0;axis<3;++axis){const double v=input[(term*3+axis)*count+i];if(!std::isfinite(v)||std::fabs(v)>bounds[term])status=BG_QUERY_UNSUPPORTED;}
            }}}
        for(uint32_t term=0;term<k;++term)for(uint32_t axis=0;axis<3;++axis){auto index=(term*3+axis)*count+i;output[index]=status==BG_QUERY_OK?input[index]*factors[term]+(term==0&&(p->flags&BG_PATCH_FRAME)?p->origin[axis]:0.):nan;if(!std::isfinite(output[index])&&status==BG_QUERY_OK)status=BG_QUERY_ARITHMETIC;}
        if(status!=BG_QUERY_OK)for(uint32_t term=0;term<k;++term)for(uint32_t axis=0;axis<3;++axis)output[(term*3+axis)*count+i]=nan;
        output_status[i]=status;
    }
});}
