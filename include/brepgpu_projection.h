#ifndef BREPGPU_PROJECTION_H
#define BREPGPU_PROJECTION_H
#include "brepgpu.h"
#ifdef __cplusplus
extern "C" {
#endif
#define BG_PROJECTION_ABI_VERSION 2u
#define BG_PROJECTION_FIELD_COUNT 7u
#define BG_PROJECTION_MAX_ITERATIONS 128u
#define BG_PROJECTION_MAX_BACKTRACKS 24u
#define BG_PROJECTION_MAX_EVALUATIONS 3329u
#define BG_PROJECTION_DEFAULT_ITERATIONS 64u
#define BG_PROJECTION_DEFAULT_BACKTRACKS 24u
#define BG_PROJECTION_DEFAULT_TOLERANCE 0x1p-24
#define BG_PROJECTION_MIN_TOLERANCE 0x1p-32
#define BG_PROJECTION_MAX_TOLERANCE 0x1p-12
/* A separate per-request status domain. STATIONARY is first-order numerical
   box stationarity, never a local/global minimum or trimmed-face claim. */
enum { BG_PROJ_STATIONARY=0, BG_PROJ_BAD_PATCH=1, BG_PROJ_BAD_PARAMETER=2,
       BG_PROJ_ARITHMETIC=3, BG_PROJ_UNSUPPORTED=4, BG_PROJ_ITERATION_LIMIT=5,
       BG_PROJ_LINESEARCH_LIMIT=6, BG_PROJ_STAGNATION=7, BG_PROJ_RANK_UNRESOLVED=8 };
enum { BG_PROJ_CANDIDATE_VALID=1u, BG_PROJ_BOUNDARY=2u,
       BG_PROJ_RANK_DEFICIENT=4u, BG_PROJ_ILL_CONDITIONED=8u,
       BG_PROJ_CURVATURE_POSITIVE=16u, BG_PROJ_CURVATURE_NEGATIVE=32u,
       BG_PROJ_CURVATURE_AMBIGUOUS=64u };
enum { BG_PROJ_S=0, BG_PROJ_T=1, BG_PROJ_X=2, BG_PROJ_Y=3, BG_PROJ_Z=4,
       BG_PROJ_DISTANCE_SQUARED=5, BG_PROJ_RESIDUAL=6 };
typedef struct bg_projection_options {
    uint32_t abi_version, struct_size, max_iterations, max_backtracks;
    double gradient_tolerance;
    uint32_t flags, reserved; /* must be zero */
} bg_projection_options;
typedef struct bg_projection_info {
    uint32_t abi_version, struct_size;
    uint64_t count, patch_count;
    uint32_t backend, flags, field_count, reserved;
    double kernel_ms; /* NaN unless BG_RESULT_PROFILED */
} bg_projection_info;
typedef struct bg_projection_abi_info {
    uint32_t abi_version, struct_size, options_size, info_size, diagnostic_size;
    uint32_t field_count, double_size, integer_size, handle_size, pointer_size;
} bg_projection_abi_info;
/* New POD descriptors use version 2; shared bg_diagnostic remains version 1.
   Canonical struct_size is exact; descriptor byte capacity may be larger.
   All payload element extents are exact. Nonempty pointers are aligned, valid
   and stable for the call. Descriptor/diagnostic/handle storage is disjoint
   from payloads. Creators zero *out before any other work. */
bg_status bg2_projection_abi_info(bg_projection_abi_info* out,uint64_t out_bytes);
bg_status bg2_create_projection_query_set(bg_handle patches,const uint32_t* slots,
    uint64_t count,const double* targets,uint64_t target_doubles,
    const double* seeds,uint64_t seed_doubles,bg_handle* out,
    bg_diagnostic* diag,uint64_t diag_bytes);
bg_status bg2_project(bg_handle queries,const bg_projection_options* options,
    uint64_t options_bytes,bg_handle* out,bg_diagnostic* diag,uint64_t diag_bytes);
bg_status bg2_projection_info(bg_handle result,bg_projection_info* out,
    uint64_t out_bytes,bg_diagnostic* diag,uint64_t diag_bytes);
/* Fields are SoA fields[field*count+i] in enum order. Integer arrays are
   status, accepted updates, actual evaluator invocations, diagnostic flags.
   All ten output pairs must be disjoint. Preflight rejection writes nothing;
   backend copy failure makes all five destinations unusable. A diagnostic
   overlapping any output is rejected without writing it or the outputs. */
bg_status bg2_projection_download(bg_handle result,double* fields,uint64_t field_doubles,
    uint32_t* statuses,uint64_t status_count,uint32_t* updates,uint64_t update_count,
    uint32_t* evaluations,uint64_t evaluation_count,uint32_t* flags,uint64_t flag_count,
    bg_diagnostic* diag,uint64_t diag_bytes);
/* Retained host source snapshots, available after public patch/query close.
   Exact payload extents; all output regions (including options) disjoint.
   Options destination must be initialized with version 2/canonical size.
   Diagnostic overlap is rejected without writing any output or diagnostic. */
bg_status bg2_projection_sources(bg_handle result,bg_patch_desc* patches,
    uint64_t patch_count,uint64_t patch_bytes,uint32_t* slots,uint64_t count,
    double* targets,uint64_t target_doubles,double* seeds,uint64_t seed_doubles,
    bg_projection_options* options,uint64_t options_bytes,
    bg_diagnostic* diag,uint64_t diag_bytes);
#ifdef __cplusplus
}
#endif
#endif
