#ifndef BREPGPU_FACE_DOMAIN_H
#define BREPGPU_FACE_DOMAIN_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#define BGFD_ABI 1u

typedef enum bgfd_status {
  BGFD_OK = 0,
  BGFD_INVALID_INPUT = 1,
  BGFD_CROSSING = 2,
  BGFD_INVALID_DOMAIN = 3,
  BGFD_NO_BRIDGE = 4,
  BGFD_NO_POSITIVE_EAR = 5,
  BGFD_RESOURCE_EXHAUSTED = 6,
  BGFD_INTERNAL_ERROR = 7
} bgfd_status;

typedef struct bgfd_integer {
  uint64_t offset;
  uint32_t count;
  int32_t sign;
} bgfd_integer;
typedef struct bgfd_node {
  uint64_t id;
  bgfd_integer x, y;
} bgfd_node;
typedef struct bgfd_loop {
  uint64_t offset, count;
} bgfd_loop;
typedef struct bgfd_input {
  uint32_t abi, size;
  uint64_t reserved, token, memory_limit, work_limit;
  const bgfd_node *nodes;
  uint64_t node_count;
  const bgfd_loop *loops;
  uint64_t loop_count;
  const uint32_t *slots;
  uint64_t slot_count;
  const uint32_t *limbs;
  uint64_t limb_count;
  bgfd_integer x_weight, y_weight;
} bgfd_input;
typedef struct bgfd_error {
  uint32_t abi, size;
  int32_t status;
  uint32_t reserved;
  char message[192];
} bgfd_error;
typedef struct bgfd_crossing {
  uint32_t abi, size, found, reserved;
  uint64_t segment_i, segment_j, ids[4];
} bgfd_crossing;
typedef struct bgfd_stats {
  uint32_t abi, size;
  uint64_t token, memory_peak, work, node_count, segment_count;
  uint64_t bvh_visits, leaf_box_tests, crossing_bvh_visits, crossing_box_tests;
  uint64_t segment_candidates, segment_predicates, orientations;
  uint64_t ear_attempts, ear_blocker_queries, ear_candidates, dead_skips;
  uint64_t bridge_candidates, triangles, max_limbs, strict_convex;
  uint64_t decode_ns, crossing_ns, bridge_ns, ear_ns;
} bgfd_stats;
typedef struct bgfd_context bgfd_context;
uint32_t bgfd_abi_size(uint32_t record);
bgfd_status bgfd_create(const bgfd_input *input, bgfd_context **output, bgfd_error *error);
bgfd_status bgfd_first_crossing(bgfd_context *context, bgfd_crossing *output, bgfd_error *error);
bgfd_status bgfd_validate_domain(bgfd_context *context, bgfd_error *error);
bgfd_status bgfd_triangulate(bgfd_context *context, bgfd_error *error);
bgfd_status bgfd_copy_triangles(bgfd_context *context, uint64_t *ids, uint64_t capacity,
                                uint64_t *required, bgfd_error *error);
bgfd_status bgfd_get_stats(const bgfd_context *context, bgfd_stats *output, bgfd_error *error);
void bgfd_destroy(bgfd_context *context);
#ifdef __cplusplus
}
#endif
#endif
