#ifndef BREPGPU_MESHER_CUDA_ADAPTIVE_CAP_API_H
#define BREPGPU_MESHER_CUDA_ADAPTIVE_CAP_API_H
#include "../adaptive_cap_types.h"
#include "adaptive_api.h"

namespace bgmesh::adaptive {
inline constexpr uint32_t adaptive_cap_cuda_abi = 1;
struct DeviceCapFacet {
  CapSourceDescriptor source;
  CapFacetInput input;
};
// Optional extension on the SAME surface bank/session/ticket. Version1 also
// requires actual-box analytic regularity and full-frame cone frame_f
// semantics. Source descriptors are valid original-source enclosures produced
// by the host; physical nonzero and source identity/quality are checked by its
// native owner.
struct CapCudaAPI {
  uint32_t abi, size;
  int32_t (*facets)(DeviceSession *, const DeviceCapFacet *, uint32_t, uint64_t *, DeviceError *);
};
} // namespace bgmesh::adaptive
extern "C" const bgmesh::adaptive::CapCudaAPI *brepgpu_adaptive_cap_cuda_v1();
#endif
