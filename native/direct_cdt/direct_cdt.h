#ifndef BREPGPU_DIRECT_CDT_H
#define BREPGPU_DIRECT_CDT_H
#include "../face_domain/face_domain.h"
#ifdef __cplusplus
extern "C" {
#endif
#define BGCDT_ABI 1u

typedef struct bgcdt_input {
  uint32_t abi, size;
  uint64_t seed, reserved;
  bgfd_input domain;
} bgcdt_input;

typedef struct bgcdt_stats {
  uint32_t abi, size;
  uint64_t token, seed, node_count, triangles;
  uint64_t validation_work, work, memory_peak, max_limbs;
  uint64_t validation_ns, prepare_ns, construct_ns, copy_ns;
  uint64_t merge_steps, crossed_edges, cavity_occurrences, cavity_insertions;
  uint64_t secondary_deletions, max_cavity, quadedge_peak, triangle_peak, constraints;
  uint64_t location_steps, hash_probes, secondary_calls, initial_triangles, void_triangles;
  uint64_t order_ns, initial_ns, conversion_ns, recovery_ns, selection_ns;
  uint64_t axis_compares, orientations, incircles, line_distance_compares;
  uint64_t scalar_signs, interval_signs, exact_signs, exact_zeros;
  uint64_t floating_disabled, enclosure_values, exact_lifts;
} bgcdt_stats;

typedef struct bgcdt_context bgcdt_context;
uint32_t bgcdt_abi_size(uint32_t record);
bgfd_status bgcdt_create(const bgcdt_input *input, bgcdt_context **output, bgfd_error *error);
bgfd_status bgcdt_construct(bgcdt_context *context, bgfd_error *error);
bgfd_status bgcdt_copy_triangles(bgcdt_context *context, uint64_t *ids, uint64_t capacity,
                                 uint64_t *required, bgfd_error *error);
bgfd_status bgcdt_get_stats(const bgcdt_context *context, bgcdt_stats *output, bgfd_error *error);
void bgcdt_destroy(bgcdt_context *context);
#ifdef __cplusplus
}
#endif
#endif
