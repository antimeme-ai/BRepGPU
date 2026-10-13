// Owned test-only callback model: no CUDA execution or numerical oracle.
#include "backend_api.h"
#include <atomic>
#include <cstring>
#include <new>
namespace {
std::atomic<int> mode{0}, contexts{0}, patches{0}, queries{0}, results{0};
struct Token { uint64_t count=0; uint32_t order=0; };
bg_status error(bg_diagnostic* d,const char* text,bool fatal,uint32_t stage){
    *d={};d->abi_version=BG_ABI_VERSION;d->struct_size=sizeof(*d);
    d->code=BG_DEVICE_FAILURE;d->stage=stage;d->query_index=-1;
    d->backend_code=fatal?700:2;d->flags=fatal?BG_DIAG_FATAL:0;
    std::strncpy(d->message,text,sizeof(d->message)-1);return d->code;
}
bg_status create(std::atomic<int>& live,uint64_t count,uint32_t order,void** out){
    *out=nullptr;try{*out=new Token{count,order};++live;return BG_OK;}
    catch(const std::bad_alloc&){return BG_OUT_OF_MEMORY;}
}
bg_status context_create(const bg_context_desc*,void**o,bg_diagnostic*){return create(contexts,0,0,o);}
bg_status context_close(void*p,bg_diagnostic*d){delete static_cast<Token*>(p);--contexts;
    if(mode==1)return error(d,"modeled fatal context cleanup",true,BG_STAGE_CLOSE);return BG_OK;}
bg_status patch_create(void*,const bg_patch_desc*,uint64_t,const double*,uint64_t,void**o,bg_diagnostic*){return create(patches,0,0,o);}
bg_status patch_close(void*,void*p,bg_diagnostic*){delete static_cast<Token*>(p);--patches;return BG_OK;}
bg_status query_create(void*,void*,const uint32_t*,uint64_t n,const double*,void**o,bg_diagnostic*){return create(queries,n,0,o);}
bg_status query_close(void*,void*p,bg_diagnostic*){delete static_cast<Token*>(p);--queries;return BG_OK;}
bg_status evaluate(void*,void*q,uint32_t order,void**o,bg_backend_profile*profile,bg_diagnostic*d){
    if(mode==3){*o=nullptr;return error(d,"modeled device restoration failure",false,BG_STAGE_EVALUATE);}
    *profile={0,0,0};return create(results,static_cast<Token*>(q)->count,order,o);
}
bg_status result_close(void*,void*p,bg_diagnostic*d){delete static_cast<Token*>(p);--results;
    if(mode==1)return error(d,"modeled first nonfatal result cleanup",false,BG_STAGE_CLOSE);
    if(mode==4)return error(d,"modeled fatal unpublished result cleanup",true,BG_STAGE_CLOSE);return BG_OK;}
bg_status download(void*,void*,double*j,uint64_t n,uint32_t*s,uint64_t count,bg_diagnostic*d){
    if(mode==2){if(n)j[0]=123;return error(d,"modeled fatal partial download",true,BG_STAGE_DOWNLOAD);}
    for(uint64_t i=0;i<n;++i)j[i]=0;for(uint64_t i=0;i<count;++i)s[i]=0;return BG_OK;
}
const bg_backend_v1 table={BG_BACKEND_ABI_VERSION,sizeof(bg_backend_v1),BG_BACKEND_CAP_FP64,
    context_create,context_close,patch_create,patch_close,query_create,query_close,evaluate,result_close,download};
}
extern "C" bg_status brepgpu_cuda_backend_v1(uint32_t version,const bg_backend_v1**out,bg_diagnostic*){
    *out=nullptr;if(version!=BG_BACKEND_ABI_VERSION)return BG_BACKEND_UNAVAILABLE;*out=&table;return BG_OK;
}
extern "C" void bg_fault_mode(int value){mode=value;}
extern "C" int bg_fault_live(int kind){return kind==0?contexts.load():kind==1?patches.load():kind==2?queries.load():results.load();}
