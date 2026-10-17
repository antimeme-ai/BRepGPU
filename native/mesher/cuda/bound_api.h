#ifndef BREPGPU_MESHER_CUDA_BOUND_API_H
#define BREPGPU_MESHER_CUDA_BOUND_API_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#define BGMESH_BOUND_ABI 1u
typedef enum bgmb_status {
  BGMB_OK = 0,
  BGMB_INVALID = 1,
  BGMB_RESOURCE = 2,
  BGMB_DEVICE = 3,
  BGMB_INTERNAL = 4
} bgmb_status;
typedef struct bgmb_interval {
  double lower, upper;
} bgmb_interval;
typedef struct bgmb_key {
  uint64_t owner, cell, generation, ordinal;
} bgmb_key;
// kind0 surface,1 curve. Surface U-major/V-fast homogeneous stride4; curve stride4.
typedef struct bgmb_root {
  uint32_t kind, p, q, reserved;
  uint64_t offset, count;
  bgmb_interval bounds[4];
} bgmb_root;
typedef struct bgmb_task {
  bgmb_key key;
  uint32_t root, reserved;
  bgmb_interval parameter[6], linear_xyz[9], tolerance;
} bgmb_task;
// decision0 accept,1 bound_fail,2 unresolved. Errors are API failures, never geometry.
typedef struct bgmb_answer {
  bgmb_key key;
  uint32_t decision, reserved;
} bgmb_answer;
typedef struct bgmb_config {
  uint32_t abi, size;
  int32_t device;
  uint32_t task_capacity;
  uint64_t device_bytes, host_bytes, work_limit, reserved;
} bgmb_config;
typedef struct bgmb_error {
  int32_t status;
  char message[188];
} bgmb_error;
typedef struct bgmb_session bgmb_session;
typedef struct bgmb_api {
  uint32_t abi, size;
  // All allocations/copies are owned. Root/coeff input need only survive create.
  // NONZERO status may still return a nonnull owned poisoned session if rollback/
  // drain/restore failed. The native owner must register/retain ANY nonnull output
  // even on failure, attempt destroy/drain, and keep its reservations meanwhile.
  int32_t (*create)(const bgmb_config *, const bgmb_root *, uint32_t, const bgmb_interval *,
                    uint64_t, bgmb_session **, bgmb_error *);
  // One outstanding batch; submit copies into owned pinned staging before return.
  int32_t (*submit)(bgmb_session *, const bgmb_task *, uint32_t, uint64_t *, bgmb_error *);
  // ready0 leaves output untouched; ready1 consumes ticket and returns every answer.
  int32_t (*poll)(bgmb_session *, uint64_t, bgmb_answer *, uint32_t, uint32_t *, bgmb_error *);
  // Drain before release. If draining fails, the poisoned session remains owned
  // and must not be discarded/reused: its reservation persists until recovery
  // or process exit. This prevents freeing staging still used by DMA.
  int32_t (*destroy)(bgmb_session *, bgmb_error *);
} bgmb_api;
// Session API calls are serialized by their native owner. No hidden CPU fallback.
const bgmb_api *brepgpu_mesher_bound_cuda_v1(void);
#ifdef __cplusplus
}
#endif
#endif
