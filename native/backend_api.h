#ifndef BREPGPU_BACKEND_API_H
#define BREPGPU_BACKEND_API_H
#include "brepgpu.h"

#define BG_BACKEND_ABI_VERSION 1u
#define BG_BACKEND_FACTORY_SYMBOL "brepgpu_cuda_backend_v1"
#define BG_BACKEND_CAP_FP64 1ull

typedef struct bg_backend_profile { uint32_t flags, reserved; double kernel_ms; } bg_backend_profile;

/* Private ABI: all diagnostics are full-sized initialized bg_diagnostic records.
   Every callback catches exceptions, selects/restores the calling thread's
   device, and returns only after its owned-stream work completes. Create/eval
   tokens are published only after restoration succeeds; failure leaves *out=0.
   Close callbacks record cleanup errors and consume their token exactly once.
   The core retains the plugin/context until child cleanup has finished. */
typedef struct bg_backend_v1 {
    uint32_t abi_version, struct_size;
    uint64_t capabilities;
    bg_status (*context_create)(const bg_context_desc*, void**, bg_diagnostic*);
    bg_status (*context_close)(void*, bg_diagnostic*);
    bg_status (*patch_create)(void*, const bg_patch_desc*, uint64_t,
                             const double*, uint64_t, void**, bg_diagnostic*);
    bg_status (*patch_close)(void*, void*, bg_diagnostic*);
    bg_status (*query_create)(void*, void*, const uint32_t*, uint64_t,
                             const double*, void**, bg_diagnostic*);
    bg_status (*query_close)(void*, void*, bg_diagnostic*);
    bg_status (*evaluate)(void*, void*, uint32_t, void**, bg_backend_profile*, bg_diagnostic*);
    bg_status (*result_close)(void*, void*, bg_diagnostic*);
    bg_status (*download)(void*, void*, double*, uint64_t, uint32_t*, uint64_t, bg_diagnostic*);
} bg_backend_v1;

typedef bg_status (*bg_backend_factory_fn)(uint32_t, const bg_backend_v1**, bg_diagnostic*);
#ifdef __cplusplus
extern "C" bg_status brepgpu_cuda_backend_v1(uint32_t requested_abi,
                                            const bg_backend_v1** out,
                                            bg_diagnostic* diag);
namespace brepgpu {
/* Full count is the SoA stride, even for a worker's [begin,end) range.
   Empty ranges do not dereference payload pointers. No allocation/throwing. */
void evaluate_cpu_range(const bg_patch_desc* patches, uint64_t patch_count,
                        const double* controls, const uint32_t* slots,
                        const double* uv, uint64_t query_count, uint32_t order,
                        uint64_t begin, uint64_t end, double* jets,
                        uint32_t* statuses) noexcept;
}
#endif
#endif
