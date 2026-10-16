#ifndef BREPGPU_MESHER_CUDA_ADAPTIVE_API_H
#define BREPGPU_MESHER_CUDA_ADAPTIVE_API_H
#include "../adaptive_geometry_types.h"
#include <stdint.h>

namespace bgmesh::adaptive {
inline constexpr uint32_t adaptive_cuda_abi = 1;
struct DeviceConfig {
  uint32_t abi, size;
  int32_t device;
  uint32_t points, controls, nodes, frontier;
  uint64_t device_bytes, host_bytes, work;
  // Callbacks never throw. Reserve an actual external allocation before CUDA
  // acquires it; release its token only after successful free/drain.
  void *reservation_context;
  void *(*reserve)(void *, uint64_t bytes);
  void (*release)(void *, void *lease);
};
struct DeviceError {
  int32_t status;
  char message[188];
};
struct DeviceRestriction {
  RootSlot root;
  ScalarRange parent;
  uint32_t destination;
  SplitAxis axis;
  uint32_t child;
};
struct DeviceSample {
  uint32_t destination;
  SampleInput input;
  SampleResult supplied_result;
  uint32_t supplied; // rare promoted or reused demanded-tier canonical result
};
struct DeviceNode {
  uint32_t slot;
  NodePredicate input;
  uint32_t points[4];
  double tolerance_down;
  uint32_t fair_axis; // 0 U,1 V,2 heuristic
  NodeBounds cached_bounds;
  uint32_t cached; // same root/path/point/stage/profile demanded by this query
};
struct DeviceNodeAnswer {
  NodeBounds bounds;
  uint32_t action; // 0 accept,1 split,2 numerical unresolved
  SplitAxis axis;
  uint32_t split_prefix;
};
struct DeviceSession;
struct DeviceStats {
  uint64_t host_reserved_bytes, host_peak_bytes, device_reserved_bytes, device_peak_bytes;
  uint64_t uploaded_bytes, downloaded_bytes;
};
struct AdaptiveCudaAPI {
  uint32_t abi, size;
  // Any nonnull output is owned, even on failed create/rollback. The enclosing
  // native owner preregisters it and retains reservations through failed drain.
  int32_t (*create)(const DeviceConfig *, const RootDescriptor *, uint32_t, const RootDescriptor *,
                    uint32_t, const Interval *, uint32_t, DeviceSession **, DeviceError *);
  // One outstanding operation. Inputs are copied into owned staging. Root,
  // point and restricted-control pools persist until destroy. Capacities are
  // upper limits, not a requirement to allocate their entire size at create.
  int32_t (*samples)(DeviceSession *, const DeviceSample *, uint32_t, uint64_t *, DeviceError *);
  int32_t (*restrict_nodes)(DeviceSession *, const DeviceRestriction *, uint32_t, uint64_t *,
                            DeviceError *);
  int32_t (*nodes)(DeviceSession *, const DeviceNode *, uint32_t, uint64_t *, DeviceError *);
  int32_t (*facets)(DeviceSession *, const ActualFacetInput *, uint32_t, uint64_t *, DeviceError *);
  // ready0 leaves output untouched; ready1 consumes ticket. Output is the
  // operation's SampleResult/DeviceNodeAnswer/ActualFacetBounds array. Restricted
  // controls remain resident; that operation has no result payload.
  // capacity is a RESULT RECORD COUNT, not bytes; restriction uses nullptr/0.
  int32_t (*poll)(DeviceSession *, uint64_t, uint32_t *ready, void *output, uint32_t capacity,
                  DeviceError *);
  int32_t (*stats)(DeviceSession *, DeviceStats *, DeviceError *);
  int32_t (*destroy)(DeviceSession *, DeviceError *);
};
} // namespace bgmesh::adaptive
extern "C" const bgmesh::adaptive::AdaptiveCudaAPI *brepgpu_adaptive_cuda_v1();
#endif
