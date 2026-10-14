#ifndef BREPGPU_PAIRS_H
#define BREPGPU_PAIRS_H
#include "brepgpu.h"
#ifdef __cplusplus
extern "C" {
#endif
#define BG_PAIR_ABI_VERSION 4u
#define BG_PAIR_MAX_INDEX_COUNT 65536ull
#define BG_PAIR_MAX_PAIRS 1048576u
enum { BG_PAIR_REACHED_END=1u, BG_PAIR_FULL_DOMAIN=2u, BG_PAIR_PROFILED=4u };
typedef struct bg_pair_box { double lower[3], upper[3]; } bg_pair_box;
typedef struct bg_pair_options {
    uint32_t abi_version, struct_size;
    uint64_t start;
    uint32_t max_pairs, flags;
} bg_pair_options;
typedef struct bg_pair_index_info {
    uint32_t abi_version, struct_size;
    uint64_t count;
    uint32_t backend, reserved;
} bg_pair_index_info;
typedef struct bg_pair_page_info {
    uint32_t abi_version, struct_size;
    uint64_t total_pairs, start, count, next_start, left_count, right_count, word_count;
    uint32_t backend, flags;
    double kernel_ms;
} bg_pair_page_info;
typedef struct bg_pair_abi_info {
    uint32_t abi_version, struct_size, box_size, options_size, index_info_size,
             page_info_size, diagnostic_size, double_size, integer_size, handle_size;
} bg_pair_abi_info;
/* ABI4 versioned records, ABI1 diagnostics and bg1_close. Canonical record
   sizes are exact; byte capacities may be larger. Numeric payload extents are
   exact. Nonempty pointers are aligned and stable for each synchronous call.
   Alias/unrepresentable-span preflight writes nothing, including diagnostics
   and handle storage; other creator failures zero a valid disjoint handle.
   Index inputs are finite ordered closed boxes; signed zeros/subnormals admit.
   A bit denotes box survival, never contact/intersection/trim overlap. */
bg_status bg4_pair_abi_info(bg_pair_abi_info*,uint64_t);
bg_status bg4_create_pair_index(bg_handle,const bg_pair_box*,uint64_t,uint64_t,
    bg_handle*,bg_diagnostic*,uint64_t);
bg_status bg4_pair_index_info(bg_handle,bg_pair_index_info*,uint64_t,bg_diagnostic*,uint64_t);
/* Downloads actual copied CPU/resident CUDA storage. Failed backend copies
   invalidate all destinations; rejected preflight preserves every byte. */
bg_status bg4_pair_index_download(bg_handle,bg_pair_box*,uint64_t,uint64_t,bg_diagnostic*,uint64_t);
bg_status bg4_discover_pairs(bg_handle,bg_handle,const bg_pair_options*,uint64_t,
    bg_handle*,bg_diagnostic*,uint64_t);
bg_status bg4_pair_page_info(bg_handle,bg_pair_page_info*,uint64_t,bg_diagnostic*,uint64_t);
/* word k stores local bits32*k..32*k+31; padding is zero. Flat Cartesian order
   is left*nR+right. Cursors count examined pairs, not survivors. REACHED_END
   is next==total; FULL_DOMAIN is start==0&&count==total. CPU/empty/unprofiled
   kernel_ms is NaN. Source index objects remain retained after public close. */
bg_status bg4_pair_page_download(bg_handle,uint32_t*,uint64_t,uint64_t,bg_diagnostic*,uint64_t);
#ifdef __cplusplus
}
#endif
#endif
