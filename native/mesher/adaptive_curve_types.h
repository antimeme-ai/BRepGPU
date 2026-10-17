#ifndef BREPGPU_MESHER_ADAPTIVE_CURVE_TYPES_H
#define BREPGPU_MESHER_ADAPTIVE_CURVE_TYPES_H
#include "adaptive_geometry_types.h"
namespace bgmesh::adaptive {
struct CurveRootSlot {
  uint32_t value = invalid_slot;
};
struct OriginalCurveRootSlot {
  uint32_t value = invalid_slot;
};
enum class CurveFamily : uint32_t { line, circle, ellipse, rational };
enum class CurveCorrespondence : uint32_t {
  affine_parameter,
  monotone_line_image,
  cylinder_affine_identity
};
struct CurveRootDescriptor {
  CurveFamily family = CurveFamily::line;
  uint32_t dimension = 3, degree = 0;
  OriginalCurveRootSlot original;
  uint32_t source_slot = invalid_slot, original_span = 0;
  uint64_t source_id = 0;
  ScalarRange controls;
  Interval original_t[2]{}, prepared_t[2]{};
  double frame[12]{}, radii[2]{};
  Interval second_derivative_upper{};
  uint32_t line_axis = 0, line_profile = 0;
};
struct CurveSampleInput {
  OriginalCurveRootSlot original;
  Interval t{};
  ArithmeticStage stage = ArithmeticStage::rn64;
  uint32_t fixed_actual = 0;
  double actual_xyz[3]{};
};
struct CurveSampleResult {
  NumericStatus status = NumericStatus::needs_wider;
  Interval original_xyz[3]{};
  double actual_xyz[3]{};
  double departure_upper = 0;
};
struct CurveNodeInput {
  CurveRootSlot root;
  Interval t[2]{};
  ScalarRange controls;
  double actual_endpoints[6]{}, endpoint_departures[2]{};
  ArithmeticStage stage = ArithmeticStage::rn64;
};
struct CurveNodeBounds {
  NumericStatus status = NumericStatus::needs_wider;
  double parameter_upper = 0, image_upper = 0, numerical_upper = 0;
  uint32_t image_available = 0;
  CurveCorrespondence correspondence = CurveCorrespondence::affine_parameter;
};
struct CurveResidualCoefficient {
  uint32_t index, axis;
};
enum class AffineTraceBoundKind : uint32_t { rational, direct_derivative };
struct AffineTraceBoundInput {
  AffineTraceBoundKind kind = AffineTraceBoundKind::rational;
  // Bound-only restriction descriptor; never registered as an original curve
  // evaluator. Analytic S(gamma) is not mislabeled as an analytic source line.
  CurveRootDescriptor rational_curve;
  CurveNodeInput input;
  double second_derivative_upper = 0;
  Interval affine_endpoints[6]{}; // SAME actual L at exact virtual piece ends
};
} // namespace bgmesh::adaptive
#endif
