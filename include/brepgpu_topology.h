#ifndef BREPGPU_TOPOLOGY_H
#define BREPGPU_TOPOLOGY_H
#include "brepgpu.h"
#ifdef __cplusplus
extern "C" {
#endif
#define BG_TOPOLOGY_ABI_VERSION 5u
#define BG_TOPOLOGY_MAX_COUNT 4194304ull
#define BG_TOPOLOGY_INVALID_SLOT UINT32_MAX
enum { BG_TOPOLOGY_UNUSED=0, BG_TOPOLOGY_BOUNDARY=1, BG_TOPOLOGY_PAIRED=2,
       BG_TOPOLOGY_SAME_FACE_PAIR=3, BG_TOPOLOGY_ORIENTATION_CONFLICT=4,
       BG_TOPOLOGY_MULTIPLE_USES=5 };
typedef struct bg5_desc {
    uint32_t abi_version, struct_size, backend;
    int32_t device;
    uint32_t threads, reserved;
    uint64_t edge_count, face_count, coedge_count;
} bg5_desc;
typedef struct bg5_info {
    uint32_t abi_version, struct_size, desc_size, timings_size,
             diagnostic_size, cuda_compiled;
} bg5_info;
typedef struct bg5_timings {
    uint32_t abi_version, struct_size;
    double admission_ms, context_ms, allocate_ms, upload_ms, count_ms,
           scan_ms, fill_ms, reader_ms, download_ms, cleanup_ms, total_ms;
} bg5_timings;
/* Standalone synchronous ABI5. Struct byte capacities are minimum capacities;
   descriptors carry canonical struct_size. Information/timings/diagnostics are
   output-only. Diagnostic is optional only as (NULL,0). CPU device must be zero.
   No storage is retained beyond the call. Pointers must name valid aligned
   stable storage. All numeric extents are EXACT element counts. Empty arrays
   may have NULL pointers. Descriptor, output arrays, timings and diagnostic
   must be separate from each other and all input arrays. Input arrays may
   overlap one another. Supplied struct capacities participate in alias checks.
   Unsafe structural destinations receive no writes. Preflight rejection does
   not write payload outputs; runtime failure may partially write destinations,
   which must all be discarded. Timings publish only on complete success.
   Each row contains original coedge slots with no within-row order guarantee.
   Mate/neighbor are UINT32_MAX unless degree two and opposed orientations.
   Same-face pairing preserves both coedges; it does not certify geometric seams.
   Times are synchronous host phase spans, not CUDA kernel-only measurements. */
bg_status bg5_abi_info(bg5_info* info, uint64_t info_bytes);
bg_status bg5_adjacency(const bg5_desc* desc, uint64_t desc_bytes,
    const uint32_t* edge_slots, const uint32_t* face_slots, const int32_t* signs,
    uint64_t input_count, uint32_t* offsets, uint64_t offset_count,
    uint32_t* members, uint64_t member_count, uint32_t* kinds, uint64_t kind_count,
    uint32_t* mates, uint32_t* neighbors, uint64_t reader_count,
    bg5_timings* timings, uint64_t timings_bytes,
    bg_diagnostic* diagnostic, uint64_t diagnostic_bytes);
#ifdef __cplusplus
}
#endif
#endif
