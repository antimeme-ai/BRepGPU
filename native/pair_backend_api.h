#ifndef BREPGPU_PAIR_BACKEND_API_H
#define BREPGPU_PAIR_BACKEND_API_H
#include "backend_api.h"
#include "brepgpu_pairs.h"
#define BG_PAIR_BACKEND_ABI_VERSION 4u
#define BG_PAIR_BACKEND_FACTORY_SYMBOL "brepgpu_cuda_backend_v4"
#define BG_BACKEND_CAP_PAIRS 8ull
/* Pure optional factory, exact retained v1 base identity. Existing tables and
   device/context domain unchanged. Every callback completes/restores before
   publication; close consumes once and reports fatal secondary cleanup. */
typedef struct bg_backend_v4 {
    uint32_t abi_version, struct_size;
    uint64_t capabilities;
    const bg_backend_v1* base;
    bg_status (*index_upload)(void*,const bg_pair_box*,uint64_t,void**,bg_diagnostic*);
    bg_status (*index_close)(void*,void*,bg_diagnostic*);
    bg_status (*index_download)(void*,void*,bg_pair_box*,uint64_t,bg_diagnostic*);
    bg_status (*discover)(void*,void*,void*,const bg_pair_options*,void**,bg_backend_profile*,bg_diagnostic*);
    bg_status (*page_close)(void*,void*,bg_diagnostic*);
    bg_status (*page_download)(void*,void*,uint32_t*,uint64_t,bg_diagnostic*);
} bg_backend_v4;
typedef bg_status (*bg_pair_backend_factory_fn)(uint32_t,const bg_backend_v4**,bg_diagnostic*);
#ifdef __cplusplus
extern "C" bg_status brepgpu_cuda_backend_v4(uint32_t,const bg_backend_v4**,bg_diagnostic*);
#endif
#endif
