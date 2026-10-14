#ifndef BREPGPU_DISCOVERY_H
#define BREPGPU_DISCOVERY_H
#include "brepgpu.h"
#ifdef __cplusplus
extern "C" {
#endif
#define BG_DISCOVERY_ABI_VERSION 3u
#define BG_DISCOVERY_FIELD_COUNT 8u
#define BG_DISCOVERY_INTEGER_SLAB_COUNT 6u
#define BG_HIERARCHY_DEFAULT_DEPTH 8u
#define BG_HIERARCHY_MAX_DEPTH 12u
#define BG_HIERARCHY_MAX_NODES 1048576ull
#define BG_DISCOVERY_DEFAULT_VISITS 8191u
#define BG_DISCOVERY_MAX_VISITS 1048576u
#define BG_DISCOVERY_DEFAULT_REFINEMENTS 8u
#define BG_DISCOVERY_MAX_REFINEMENTS 64u
#define BG_DISCOVERY_DEFAULT_TOLERANCE_SQUARED 0x1p-24
#define BG_DISCOVERY_PROJECTOR_ITERATIONS 32u
#define BG_DISCOVERY_PROJECTOR_BACKTRACKS 24u
#define BG_DISCOVERY_PROJECTOR_TOLERANCE 0x1p-24
/* Build flags hold exactly one enum value, not combinable bits. */
enum { BG_BOUND_AABB=0, BG_BOUND_DOP26=1, BG_BOUND_BERNSTEIN=2 };
enum { BG_HIERARCHY_LEAF=1u };
enum { BG_DISC_GAP_SATISFIED=0, BG_DISC_UNRESOLVED=1, BG_DISC_BAD_PATCH=2,
       BG_DISC_BAD_TARGET=3, BG_DISC_ARITHMETIC=4 };
enum { BG_DISC_CANDIDATE_VALID=1u, BG_DISC_GAP=2u, BG_DISC_COVERAGE_COMPLETE=4u,
       BG_DISC_VISIT_LIMIT=8u, BG_DISC_REFINEMENT_LIMIT=16u, BG_DISC_ARITHMETIC_REASON=32u };
enum { BG_DISC_S=0, BG_DISC_T=1, BG_DISC_X=2, BG_DISC_Y=3, BG_DISC_Z=4,
       BG_DISC_APPROXIMATE_DISTANCE_SQUARED=5, BG_DISC_LOWER_SQUARED=6,
       BG_DISC_UPPER_SQUARED=7 };
typedef struct bg_hierarchy_options {
    uint32_t abi_version, struct_size, depth, reserved;
    uint64_t max_nodes;
    uint32_t flags, reserved2;
} bg_hierarchy_options;
typedef struct bg_hierarchy_node {
    double lower[3], upper[3];
    uint32_t left, right, patch_slot, u_index, v_index, u_level, v_level, flags;
} bg_hierarchy_node;
typedef struct bg_hierarchy_info {
    uint32_t abi_version, struct_size;
    uint64_t patch_count, node_count;
    uint32_t depth, backend, flags, node_size;
} bg_hierarchy_info;
typedef struct bg_discovery_options {
    uint32_t abi_version, struct_size, max_visits, max_refinements;
    double tolerance_squared;
    uint32_t flags, reserved;
} bg_discovery_options;
typedef struct bg_discovery_info {
    uint32_t abi_version, struct_size;
    uint64_t count, patch_count;
    uint32_t backend, flags, field_count, reserved;
    double kernel_ms;
} bg_discovery_info;
typedef struct bg_discovery_abi_info {
    uint32_t abi_version, struct_size, hierarchy_options_size, node_size;
    uint32_t hierarchy_info_size, options_size, info_size, diagnostic_size;
    uint32_t field_count, integer_slab_count, double_size, integer_size;
    uint32_t handle_size, pointer_size;
} bg_discovery_abi_info;
/* New versioned records use ABI3; diagnostic and patch records retain ABI1.
   Canonical record sizes are exact, byte capacities can be larger. All numeric
   payload extents are exact; nonempty pointers are aligned, valid and stable
   for the call. Written regions are mutually disjoint and separate from the
   diagnostic and all borrowed creator inputs. Alias rejection writes nothing,
   including handle/diagnostic storage. Other creator failures zero a valid
   disjoint handle output. bg1_close closes all new resource kinds. */
bg_status bg3_discovery_abi_info(bg_discovery_abi_info* out,uint64_t out_bytes);
bg_status bg3_create_hierarchy(bg_handle patches,const bg_hierarchy_options* options,
    uint64_t options_bytes,bg_handle* out,bg_diagnostic* diag,uint64_t diag_bytes);
bg_status bg3_hierarchy_info(bg_handle hierarchy,bg_hierarchy_info* out,
    uint64_t out_bytes,bg_diagnostic* diag,uint64_t diag_bytes);
/* Actual resident storage for qualification. roots has exactly patch_count
   indices; nodes has exactly node_count records, node_bytes=node_count*80.
   Leaves have UINT32_MAX children. Lower/upper enclose original exact carrier. */
bg_status bg3_hierarchy_download(bg_handle hierarchy,bg_hierarchy_node* nodes,
    uint64_t node_count,uint64_t node_bytes,uint32_t* roots,uint64_t root_count,
    bg_diagnostic* diag,uint64_t diag_bytes);
/* Actual bound cache: exact node_count * [0,20,490] doubles for mode0/1/2.
   Node-major DOP index=node*20+2*direction+endpoint (0 lower,1 upper).
   Bernstein index=node*490+polynomial*98+2*coefficient+endpoint, polynomial
   [sum(A^2),AxW,AyW,AzW,W^2], coefficient=a*(2q+1)+b; active a=0..2p,b=0..2q.
   Remaining pairs are exact zero padding and never participate in query bounds. */
bg_status bg3_hierarchy_bound_download(bg_handle hierarchy,double* bounds,
    uint64_t bound_doubles,bg_diagnostic* diag,uint64_t diag_bytes);
bg_status bg3_create_discovery_query_set(bg_handle hierarchy,const uint32_t* slots,
    uint64_t count,const double* targets,uint64_t target_doubles,
    bg_handle* out,bg_diagnostic* diag,uint64_t diag_bytes);
bg_status bg3_discover(bg_handle queries,const bg_discovery_options* options,
    uint64_t options_bytes,bg_handle* out,bg_diagnostic* diag,uint64_t diag_bytes);
bg_status bg3_discovery_info(bg_handle result,bg_discovery_info* out,
    uint64_t out_bytes,bg_diagnostic* diag,uint64_t diag_bytes);
/* Fields[field*count+i], six distinct u32 slabs in the named order.
   GAP is only the stated squared-distance bracket criterion, not nearest UV,
   minimum enumeration, stationarity or trimmed-face proximity. Preflight
   rejection preserves every output; backend copy failure invalidates all host
   destinations. A fatal diagnostic poisons all CUDA operation families. */
bg_status bg3_discovery_download(bg_handle result,double* fields,uint64_t field_doubles,
    uint32_t* statuses,uint64_t status_count,uint32_t* flags,uint64_t flag_count,
    uint32_t* visits,uint64_t visit_count,uint32_t* pruned,uint64_t pruned_count,
    uint32_t* refinements,uint64_t refinement_count,uint32_t* evaluations,
    uint64_t evaluation_count,bg_diagnostic* diag,uint64_t diag_bytes);
/* Owned host source snapshots survive public patch/hierarchy/query close.
   Both option destinations are initialized ABI3 records and count as outputs. */
bg_status bg3_discovery_sources(bg_handle result,bg_patch_desc* patches,
    uint64_t patch_count,uint64_t patch_bytes,uint32_t* slots,uint64_t count,
    double* targets,uint64_t target_doubles,bg_hierarchy_options* hierarchy_options,
    uint64_t hierarchy_options_bytes,bg_discovery_options* options,
    uint64_t options_bytes,bg_diagnostic* diag,uint64_t diag_bytes);
#ifdef __cplusplus
}
#endif
#endif
