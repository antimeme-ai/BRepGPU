#ifndef BREPGPU_MESHER_CUDA_ADAPTIVE_ASSIGNMENT_API_H
#define BREPGPU_MESHER_CUDA_ADAPTIVE_ASSIGNMENT_API_H
#include "adaptive_api.h"

namespace bgmesh::adaptive {
inline constexpr uint32_t adaptive_assignment_cuda_abi = 1;
// Optional control extension on the SAME surface bank/stream/reservations.
// Call only after all submitted work has been consumed through base poll.
struct AssignmentCudaAPI {
  uint32_t abi, size;
  int32_t (*retire)(DeviceSession *, uint64_t expected_assignment, uint64_t next_assignment,
                    DeviceError *);
};
} // namespace bgmesh::adaptive
extern "C" const bgmesh::adaptive::AssignmentCudaAPI *brepgpu_adaptive_assignment_cuda_v1();
#endif
