#ifndef BREPGPU_MESHER_CUDA_ADAPTIVE_CURVE_API_H
#define BREPGPU_MESHER_CUDA_ADAPTIVE_CURVE_API_H
#include "../adaptive_curve_types.h"
#include "adaptive_api.h"

namespace bgmesh::adaptive {
inline constexpr uint32_t curve_cuda_abi = 1;
struct CurveDeviceSample {
  uint32_t destination;
  CurveSampleInput input;
  CurveSampleResult supplied_result;
  uint32_t supplied;
};
struct CurveDeviceRestriction {
  CurveRootSlot root;
  ScalarRange parent;
  uint32_t left_destination, right_destination;
};
struct CurveDeviceNode {
  uint32_t slot;
  CurveNodeInput input;
  uint32_t points[2];
  double tolerance_down;
  uint32_t allow_image;
  CurveNodeBounds cached_bounds;
  uint32_t cached;
};
struct CurveDeviceNodeAnswer {
  CurveNodeBounds bounds;
  uint32_t action; // 0 accept, 1 split, 2 numerical unresolved
  uint32_t split_prefix;
  CurveCorrespondence selected;
};
// Curve and surface factories share the owned allocation/ticket/drain core.
// Tokens are mode qualified; using a surface operation on a curve token (or
// the reverse) refuses before reading any differently typed descriptors.
struct CurveCudaAPI {
  uint32_t abi, size;
  int32_t (*create)(const DeviceConfig *, const CurveRootDescriptor *, uint32_t,
                    const CurveRootDescriptor *, uint32_t, const Interval *, uint32_t,
                    DeviceSession **, DeviceError *);
  int32_t (*samples)(DeviceSession *, const CurveDeviceSample *, uint32_t, uint64_t *,
                     DeviceError *);
  // Counted initial source restrictions and demanded promoted tiers only.
  // Destinations must be uninitialized; retained source/base nets are
  // immutable.
  int32_t (*install_controls)(DeviceSession *, uint32_t destination, const Interval *, uint32_t,
                              uint64_t *, DeviceError *);
  int32_t (*paired_children)(DeviceSession *, const CurveDeviceRestriction *, uint32_t, uint64_t *,
                             DeviceError *);
  int32_t (*nodes)(DeviceSession *, const CurveDeviceNode *, uint32_t, uint64_t *, DeviceError *);
  // Dynamic original-carrier trace restrictions. Each task's input.controls
  // range addresses this batch's flat net pool; source leases stay native.
  int32_t (*trace_bounds)(DeviceSession *, const AffineTraceBoundInput *, uint32_t,
                          const Interval *, uint32_t, uint64_t *, DeviceError *);
  // capacity is a RESULT RECORD COUNT; paired_children uses nullptr/0.
  int32_t (*poll)(DeviceSession *, uint64_t, uint32_t *ready, void *output, uint32_t capacity,
                  DeviceError *);
  int32_t (*stats)(DeviceSession *, DeviceStats *, DeviceError *);
  int32_t (*destroy)(DeviceSession *, DeviceError *);
};
} // namespace bgmesh::adaptive
extern "C" const bgmesh::adaptive::CurveCudaAPI *brepgpu_adaptive_curve_cuda_v1();
#endif
