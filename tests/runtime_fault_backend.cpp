// Owned test-only callback model: no CUDA execution or numerical oracle.
#include "projection_backend_api.h"
#include <atomic>
#include <cstring>
#include <new>
namespace {
std::atomic<int> mode{0}, contexts{0}, patches{0}, queries{0}, results{0}, projection_queries{0}, projection_results{0}, factory_calls{0};
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
bg_status projection_query_create(void*,void*,const uint32_t*,uint64_t n,const double*,const double*,void**o,bg_diagnostic*){return create(projection_queries,n,0,o);}
bg_status projection_query_close(void*,void*p,bg_diagnostic*){delete static_cast<Token*>(p);--projection_queries;return BG_OK;}
bg_status project(void*,void*q,const bg_projection_options*,void**o,bg_backend_profile*profile,bg_diagnostic*d){
    if(mode==20){*o=nullptr;return error(d,"modeled projection restoration failure",false,BG_STAGE_EVALUATE);}
    *profile={0,0,0};return create(projection_results,static_cast<Token*>(q)->count,0,o);
}
bg_status projection_result_close(void*,void*p,bg_diagnostic*d){delete static_cast<Token*>(p);--projection_results;
    if(mode==18)return error(d,"modeled fatal unpublished projection cleanup",true,BG_STAGE_CLOSE);return BG_OK;}
bg_status projection_download(void*,void*,double*f,uint64_t n,uint32_t*s,uint64_t count,
    uint32_t*u,uint64_t,uint32_t*e,uint64_t,uint32_t*flags,uint64_t,bg_diagnostic*d){
    if(mode==19){if(n)f[0]=456;return error(d,"modeled fatal partial projection download",true,BG_STAGE_DOWNLOAD);}
    for(uint64_t i=0;i<n;++i)f[i]=0;
    for(uint64_t i=0;i<count;++i){s[i]=BG_PROJ_RANK_UNRESOLVED;u[i]=0;e[i]=1;flags[i]=BG_PROJ_CANDIDATE_VALID|BG_PROJ_RANK_DEFICIENT;}
    return BG_OK;
}

const bg_backend_v1 table={BG_BACKEND_ABI_VERSION,sizeof(bg_backend_v1),BG_BACKEND_CAP_FP64,
    context_create,context_close,patch_create,patch_close,query_create,query_close,evaluate,result_close,download};
const bg_backend_v1 other_base=table;
const bg_backend_v2 extension={BG_PROJECTION_BACKEND_ABI_VERSION,sizeof(bg_backend_v2),BG_BACKEND_CAP_PROJECTION,&table,
    projection_query_create,projection_query_close,project,projection_result_close,projection_download};
const bg_backend_v2 wrong_base={BG_PROJECTION_BACKEND_ABI_VERSION,sizeof(bg_backend_v2),BG_BACKEND_CAP_PROJECTION,&other_base,
    projection_query_create,projection_query_close,project,projection_result_close,projection_download};
const bg_backend_v2 wrong_size={BG_PROJECTION_BACKEND_ABI_VERSION,sizeof(bg_backend_v2)-8,BG_BACKEND_CAP_PROJECTION,&table,
    projection_query_create,projection_query_close,project,projection_result_close,projection_download};

}
extern "C" bg_status brepgpu_cuda_backend_v1(uint32_t version,const bg_backend_v1**out,bg_diagnostic*){
    *out=nullptr;if(version!=BG_BACKEND_ABI_VERSION)return BG_BACKEND_UNAVAILABLE;*out=&table;return BG_OK;
}
extern "C" bg_status brepgpu_cuda_backend_v2(uint32_t version,const bg_backend_v2**out,bg_diagnostic*d){
    ++factory_calls;*out=nullptr;
    if(version!=BG_PROJECTION_BACKEND_ABI_VERSION || mode==10)return BG_UNSUPPORTED;
    if(mode==12){error(d,"modeled transient factory allocation failure",false,BG_STAGE_PLUGIN);d->code=BG_OUT_OF_MEMORY;return BG_OUT_OF_MEMORY;}
    if(mode==13)return error(d,"modeled fatal extension factory",true,BG_STAGE_PLUGIN);
    if(mode==14){error(d,"modeled fatal plus OK extension factory",true,BG_STAGE_PLUGIN);return BG_OK;}
    if(mode==15){error(d,"modeled fatal plus unsupported extension factory",true,BG_STAGE_PLUGIN);d->code=BG_UNSUPPORTED;return BG_UNSUPPORTED;}
    *out=mode==11?&wrong_base:mode==16?&wrong_size:&extension;return BG_OK;
}
extern "C" void bg_fault_mode(int value){mode=value;}
extern "C" int bg_fault_factory_calls(){return factory_calls.load();}
extern "C" int bg_fault_live(int kind){return kind==0?contexts.load():kind==1?patches.load():kind==2?queries.load():kind==3?results.load():kind==4?projection_queries.load():projection_results.load();}
