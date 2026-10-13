#ifndef BREPGPU_H
#define BREPGPU_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

#define BG_ABI_VERSION 1u
typedef uint64_t bg_handle;
typedef uint32_t bg_status;
enum { BG_OK=0, BG_INVALID_ARGUMENT=1, BG_UNSUPPORTED=2, BG_INVALID_HANDLE=3,
       BG_OUT_OF_MEMORY=4, BG_BACKEND_UNAVAILABLE=5, BG_DEVICE_FAILURE=6,
       BG_CONTEXT_UNUSABLE=7, BG_INTERNAL_ERROR=8 };
enum { BG_QUERY_OK=0, BG_QUERY_BAD_PATCH=1, BG_QUERY_BAD_PARAMETER=2,
       BG_QUERY_ARITHMETIC=3, BG_QUERY_UNSUPPORTED=4 };
enum { BG_BACKEND_CPU=0, BG_BACKEND_CUDA=1 };
enum { BG_STAGE_ARGUMENT=1, BG_STAGE_CONTEXT=2, BG_STAGE_UPLOAD=3,
       BG_STAGE_EVALUATE=4, BG_STAGE_DOWNLOAD=5, BG_STAGE_CLOSE=6,
       BG_STAGE_CONVERT=7, BG_STAGE_PLUGIN=8 };
enum { BG_CONTEXT_PROFILE=1u, BG_DIAG_FATAL=1u, BG_PATCH_LINEAGE=1u,
       BG_PATCH_FRAME=2u, BG_RESULT_PROFILED=1u };

typedef struct bg_diagnostic {
    uint32_t abi_version, struct_size, code, stage;
    int64_t query_index;
    int32_t backend_code;
    uint32_t flags;
    char message[256];
} bg_diagnostic;

typedef struct bg_abi_info {
    uint32_t abi_version, struct_size;
    uint32_t context_desc_size, patch_desc_size, result_info_size, diagnostic_size;
    uint32_t double_size, handle_size, slot_size, pointer_size;
} bg_abi_info;

typedef struct bg_context_desc {
    uint32_t abi_version, struct_size, backend;
    int32_t device;
    uint32_t threads, flags;
    const char* plugin_path;
    uint64_t plugin_path_bytes;
} bg_context_desc;

/* Offsets/counts address homogeneous control RECORDS, each four doubles;
   control_doubles must therefore be divisible by four, including unused tails.
   uv = (u_min,u_max,v_min,v_max); frame X=origin+scale*S.
   BG_PATCH_LINEAGE distinguishes absent lineage from a populated tuple. */
typedef struct bg_patch_desc {
    uint32_t abi_version, struct_size, degree_u, degree_v;
    uint64_t control_offset, control_count;
    uint64_t body_id, entity_id, revision, chart_id;
    uint32_t entity_kind, flags;
    double uv[4], origin[3], scale;
} bg_patch_desc;

typedef struct bg_result_info {
    uint32_t abi_version, struct_size;
    uint64_t count, patch_count;
    uint32_t order, terms, backend, flags;
    double kernel_ms; /* NaN unless BG_RESULT_PROFILED is set. */
} bg_result_info;

/* Descriptor/output byte capacities are explicit. Numeric capacities count
   elements. Nonempty pointers must be valid, aligned, and stable for the call.
   Diagnostic storage is optional; a supplied short/unaligned buffer is rejected
   before any operation or diagnostic write. All creators zero *out first. */
bg_status bg1_abi_info(bg_abi_info* out, uint64_t out_bytes);
bg_status bg1_create_context(const bg_context_desc* desc, uint64_t desc_bytes,
                            bg_handle* out, bg_diagnostic* diag, uint64_t diag_bytes);
bg_status bg1_create_patch_set(bg_handle context, const bg_patch_desc* patches,
                              uint64_t patch_count, uint64_t patch_bytes,
                              const double* controls, uint64_t control_doubles,
                              bg_handle* out, bg_diagnostic* diag, uint64_t diag_bytes);
bg_status bg1_create_query_set(bg_handle patches, const uint32_t* slots,
                              uint64_t count, const double* uv, uint64_t uv_doubles,
                              bg_handle* out, bg_diagnostic* diag, uint64_t diag_bytes);
bg_status bg1_evaluate(bg_handle queries, uint32_t order, bg_handle* out,
                       bg_diagnostic* diag, uint64_t diag_bytes);
bg_status bg1_result_info(bg_handle result, bg_result_info* out, uint64_t out_bytes,
                          bg_diagnostic* diag, uint64_t diag_bytes);
/* Numeric output extents are EXACT, not minimum capacities. Diagnostic/output
   descriptor byte capacities can be larger, with canonical struct_size.
   Descriptor, diagnostic and handle output storage must be separate from all
   payload arrays. Preflight-rejected downloads do not write payload outputs.
   A backend download failure may partially write: discard both destinations. */
bg_status bg1_download(bg_handle result, double* jets, uint64_t jet_doubles,
                       uint32_t* statuses, uint64_t status_count,
                       bg_diagnostic* diag, uint64_t diag_bytes);
/* Read retained host provenance. Exact capacities are required. This operation
   names a live result, not public patch/query handles that may be closed. */
bg_status bg1_result_sources(bg_handle result, bg_patch_desc* patches,
                             uint64_t patch_count, uint64_t patch_bytes,
                             uint32_t* slots, uint64_t count, double* uv,
                             uint64_t uv_doubles, bg_diagnostic* diag, uint64_t diag_bytes);
bg_status bg1_close(bg_handle handle, bg_diagnostic* diag, uint64_t diag_bytes);
/* Host-only conversion; output and input arrays must not overlap. Status and
   source identity are preserved. A failed conversion query has all NaN fields. */
bg_status bg1_convert(const bg_patch_desc* patches, uint64_t patch_count,
                      uint64_t patch_bytes, const uint32_t* slots, uint64_t count,
                      uint32_t order, const double* input, uint64_t input_doubles,
                      const uint32_t* input_statuses, uint64_t input_status_count,
                      double* output, uint64_t output_doubles,
                      uint32_t* output_statuses, uint64_t output_status_count,
                      bg_diagnostic* diag, uint64_t diag_bytes);

#ifdef __cplusplus
}
#endif
#endif
