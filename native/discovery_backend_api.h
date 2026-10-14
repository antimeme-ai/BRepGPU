#ifndef BREPGPU_DISCOVERY_BACKEND_API_H
#define BREPGPU_DISCOVERY_BACKEND_API_H
#include "backend_api.h"
#include "brepgpu_discovery.h"
#define BG_DISCOVERY_BACKEND_ABI_VERSION 3u
#define BG_DISCOVERY_BACKEND_FACTORY_SYMBOL "brepgpu_cuda_backend_v3"
#define BG_BACKEND_CAP_DISCOVERY 4ull
/* Pure static extension factory: no CUDA/device calls or allocation. Exact base
   identity matches the v1 table used by retained context/patch tokens. v1/v2
   layouts unchanged; all completion/restoration/once-only cleanup/ABI1 diagnostic
   rules apply. Cache mode is enum0/1/2, never silently downgraded. */
typedef struct bg_backend_v3 {
    uint32_t abi_version, struct_size;
    uint64_t capabilities;
    const bg_backend_v1* base;
    bg_status (*hierarchy_upload)(void*,void*,const bg_hierarchy_node*,uint64_t,
        const uint32_t*,uint64_t,const double*,uint64_t,uint32_t,void**,bg_diagnostic*);
    bg_status (*hierarchy_close)(void*,void*,bg_diagnostic*);
    bg_status (*hierarchy_download)(void*,void*,bg_hierarchy_node*,uint64_t,
        uint32_t*,uint64_t,bg_diagnostic*);
    bg_status (*hierarchy_bound_download)(void*,void*,double*,uint64_t,bg_diagnostic*);
    bg_status (*discovery_query_create)(void*,void*,const uint32_t*,uint64_t,
        const double*,void**,bg_diagnostic*);
    bg_status (*discovery_query_close)(void*,void*,bg_diagnostic*);
    bg_status (*discover)(void*,void*,const bg_discovery_options*,void**,
        bg_backend_profile*,bg_diagnostic*);
    bg_status (*discovery_result_close)(void*,void*,bg_diagnostic*);
    bg_status (*discovery_download)(void*,void*,double*,uint64_t,uint32_t*,uint64_t,
        uint32_t*,uint64_t,uint32_t*,uint64_t,uint32_t*,uint64_t,uint32_t*,uint64_t,
        uint32_t*,uint64_t,bg_diagnostic*);
} bg_backend_v3;
typedef bg_status (*bg_discovery_backend_factory_fn)(uint32_t,const bg_backend_v3**,bg_diagnostic*);
#ifdef __cplusplus
extern "C" bg_status brepgpu_cuda_backend_v3(uint32_t requested_abi,
    const bg_backend_v3** out,bg_diagnostic* diag);
#endif
#endif
