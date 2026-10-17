#ifndef BREPGPU_MESHER_ADAPTIVE_TRIMMED_TYPES_H
#define BREPGPU_MESHER_ADAPTIVE_TRIMMED_TYPES_H
#include "adaptive_geometry_types.h"

namespace bgmesh::adaptive {
// Bound-only restriction. All slots are qualified by the retained source bank.
// The three parameters are normalized to the ORIGINAL evaluator root, never a
// prepared tile or a selected leaf. affine_xyz is the SAME emitted L.
struct SourceTriangleInput {
  OriginalRootSlot original;
  Interval normalized_uv[6]{};
  Interval affine_xyz[9]{};
  double allowance_down = 0.;
};
struct SourceTriangleBounds {
  NumericStatus status = NumericStatus::needs_wider;
  double position_upper = 0.;
  // Arithmetic routing diagnostic, never an actual geometric point floor.
  double uncertainty_upper = 0.;
};
// Exponents are (degree-i-j,i,j), matching the owned exact triangle net.
struct TriangleCoefficient {
  uint32_t degree, i, j;
};
} // namespace bgmesh::adaptive
#endif
