#ifndef BREPGPU_PROJECTION_BACKEND_API_H
#define BREPGPU_PROJECTION_BACKEND_API_H
#include "backend_api.h"
#include "brepgpu_projection.h"
#define BG_PROJECTION_BACKEND_ABI_VERSION 2u
#define BG_PROJECTION_BACKEND_FACTORY_SYMBOL "brepgpu_cuda_backend_v2"
#define BG_BACKEND_CAP_PROJECTION 2ull
/* Separate pure extension, not an appended/reinterpreted v1 layout. Factory
   returns a static table, without CUDA calls/allocation/device changes. Base
   must equal the exact v1 table that created context/patch tokens. All callback
   diagnostics remain ABI1. The v1 completion/device/once-only close contract
   applies, and projection tokens never enter v1 query/result callbacks. */
typedef struct bg_backend_v2 {
    uint32_t abi_version, struct_size;
    uint64_t capabilities;
    const bg_backend_v1* base;
    bg_status (*projection_query_create)(void*,void*,const uint32_t*,uint64_t,
        const double*,const double*,void**,bg_diagnostic*);
    bg_status (*projection_query_close)(void*,void*,bg_diagnostic*);
    bg_status (*project)(void*,void*,const bg_projection_options*,void**,
        bg_backend_profile*,bg_diagnostic*);
    bg_status (*projection_result_close)(void*,void*,bg_diagnostic*);
    bg_status (*projection_download)(void*,void*,double*,uint64_t,uint32_t*,uint64_t,
        uint32_t*,uint64_t,uint32_t*,uint64_t,uint32_t*,uint64_t,bg_diagnostic*);
} bg_backend_v2;
typedef bg_status (*bg_projection_backend_factory_fn)(uint32_t,const bg_backend_v2**,bg_diagnostic*);
#ifdef __cplusplus
extern "C" bg_status brepgpu_cuda_backend_v2(uint32_t requested_abi,
    const bg_backend_v2** out,bg_diagnostic* diag);
namespace brepgpu {
/* Full N is every output field's stride. [begin,end) is only the request range.
   Targets are XYZ triples; seeds are ST pairs; options is one owned snapshot.
   No allocation/throwing. Empty ranges never dereference payload pointers. */
void project_cpu_range(const bg_patch_desc* patches,uint64_t patch_count,
    const double* controls,const uint32_t* slots,const double* targets,
    const double* seeds,uint64_t query_count,const bg_projection_options* options,
    uint64_t begin,uint64_t end,double* fields,uint32_t* statuses,
    uint32_t* updates,uint32_t* evaluations,uint32_t* flags) noexcept;
}
#endif
#endif
