#ifndef BREPGPU_MESHER_CUDA_ADAPTIVE_TRIM_API_H
#define BREPGPU_MESHER_CUDA_ADAPTIVE_TRIM_API_H
#include "../adaptive_trimmed_types.h"
#include "adaptive_api.h"

namespace bgmesh::adaptive {
inline constexpr uint32_t adaptive_trim_cuda_abi = 1;
// Optional operation extension on the SAME surface session, resident original
// bank and one outstanding ticket. Base poll/stats/destroy retain ownership.
// Existing patch/curve API layouts and factories remain unchanged.
struct TrimCudaAPI {
  uint32_t abi, size;
  int32_t (*source_triangles)(DeviceSession *, const SourceTriangleInput *, uint32_t,
                              uint64_t *, DeviceError *);
};
} // namespace bgmesh::adaptive
extern "C" const bgmesh::adaptive::TrimCudaAPI *brepgpu_adaptive_trim_cuda_v1();
#endif
