#ifndef BREPGPU_MESHER_ADAPTIVE_GEOMETRY_TYPES_H
#define BREPGPU_MESHER_ADAPTIVE_GEOMETRY_TYPES_H
#include <stdint.h>

namespace bgmesh::adaptive {
inline constexpr uint32_t invalid_slot = UINT32_MAX;
struct RootSlot {
  uint32_t value = invalid_slot;
};
struct OriginalRootSlot {
  uint32_t value = invalid_slot;
};
struct Interval {
  double lower = 0, upper = 0;
};
struct ScalarRange {
  uint32_t first = 0, count = 0;
};
enum class GeometryFamily : uint32_t { plane, cylinder, cone, sphere, torus, rational_tensor };
enum class ArithmeticStage : uint32_t { rn64, p128, p192, p256, precise };
enum class NumericStatus : uint32_t { ready, needs_wider, unsupported, invalid };
enum class SplitAxis : uint32_t { u, v };
struct RootDescriptor {
  GeometryFamily family = GeometryFamily::plane;
  OriginalRootSlot original;
  uint32_t source_slot = invalid_slot;
  uint64_t source_id = 0;
  uint32_t span_u = 0, span_v = 0;
  uint32_t degree_u = 0, degree_v = 0;
  // U-major, V-minor, channel (Nx,Ny,Nz,W) fastest. Original roots
  // reference full source span controls; prepared roots reference restricted
  // controls. Samples ALWAYS use the original root and its controls.
  ScalarRange controls;
  // Source/query endpoints are exact original binary64 values (degenerate
  // intervals). Device node inputs may enclose finer exact dyadic parameters.
  Interval original_box[4]{};
  Interval prepared_box[4]{};
  double frame[12]{}, radii[2]{};
  Interval frame_a{}, frame_f{}, cone_sin{}, cone_cos{};
  OriginalRootSlot sample_owners[9]{}; // U low/interior/high + 3*V case
  uint32_t affine_separable_axes = 0;
};
struct SampleInput {
  OriginalRootSlot original;
  Interval uv[2]{};
  ArithmeticStage stage = ArithmeticStage::rn64;
};
struct SampleResult {
  NumericStatus status = NumericStatus::needs_wider;
  Interval original_xyz[3]{};
  double actual_xyz[3]{};
  double departure_upper = 0;
};
struct NodePredicate {
  RootSlot root;
  Interval box[4]{};
  ScalarRange controls;
  double actual_corners[12]{}; // 00,10,01,11; xyz fastest
  double corner_departures[4]{};
  ArithmeticStage stage = ArithmeticStage::rn64;
};
struct NodeBounds {
  NumericStatus status = NumericStatus::needs_wider;
  double source_to_bilinear_upper = 0, twist_upper = 0, packet_upper = 0;
  double score_u = 0, score_v = 0, numerical_upper = 0;
  uint32_t regularity_proved = 0; // local immersion, not global embedding
};
struct ActualFacetInput {
  double actual_corners[12]{};
  Interval uv[6]{};         // original piece vertices, normalized to SAME leaf
  Interval affine_xyz[9]{}; // SAME actual emitted affine facet, restricted
  double source_to_bilinear_upper = 0;
};
struct ActualFacetBounds {
  NumericStatus status = NumericStatus::needs_wider;
  double bilinear_to_facet_upper = 0, total_upper = 0;
};
} // namespace bgmesh::adaptive
#endif
