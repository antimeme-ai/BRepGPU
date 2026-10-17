#ifndef BREPGPU_MESHER_ADAPTIVE_CURVE_GEOMETRY_HPP
#define BREPGPU_MESHER_ADAPTIVE_CURVE_GEOMETRY_HPP
#include "adaptive_geometry.hpp"
#include "curve_source_cache.hpp"
#include "geometry.hpp"
#include <optional>
namespace bgmesh::adaptive {
struct OriginalLineImage final {
  std::array<Rational, 4> endpoints;
  std::array<Rational, 2> covector, scalars;
};
[[nodiscard]] std::optional<OriginalLineImage>
original_curve_line_image(const CurveSourceCache &, Slot<CurveTag>,
                           std::span<const Rational, 2> domain, bgfd::Budget &);
namespace curve_detail {
BezierCurve original_net_exact(const CurveSourceCache &, OriginalCurveRootSlot, bgfd::Budget &);
}
struct PreciseCurveBoundResult {
  CurveNodeBounds bounds;
  bool parameter_certified = false, image_certified = false;
};
CurveSampleResult evaluate_curve_sample(const CurveSourceCache &, const CurveSampleInput &);
CurveNodeBounds bound_curve_node(const CurveSourceCache &, const CurveNodeInput &,
                                 std::span<const Interval> node_controls = {});
std::pmr::vector<Interval> prepare_curve_interval_controls(const CurveSourceCache &, CurveRootSlot,
                                                           std::span<const Rational, 2> exact_t,
                                                           bgfd::Budget &);
void restrict_curve_midpoint_pair(const CurveRootDescriptor &, std::span<const Interval> parent,
                                  std::span<Interval> left, std::span<Interval> right,
                                  std::span<Interval> workspace);
CurveSampleResult evaluate_curve_sample_precise(const CurveSourceCache &, OriginalCurveRootSlot,
                                                const Rational &, ArithmeticStage, bgfd::Budget &,
                                                std::optional<Vec3> fixed_actual = {});
PreciseCurveBoundResult bound_curve_node_precise(const CurveSourceCache &, CurveRootSlot,
                                                 std::span<const Rational, 2> original_t,
                                                 std::span<const double, 6> actual,
                                                 const Rational &allowance, bgfd::Budget &);
// Original rational/line coordinates only. Analytic unknown identities return
// nullopt; callers must not replace source equality by interval overlap.
std::optional<std::array<Rational, 3>> exact_curve_value(const CurveSourceCache &, Slot<CurveTag>,
                                                         const Rational &, bgfd::Budget &);

// Original positive homogeneous control hull, restricted exactly on ambiguity.
std::array<Interval, 3> enclose_curve_hull_precise(const CurveSourceCache &, Slot<CurveTag>,
                                                   std::span<const Rational, 2>, bgfd::Budget &);
// Owner-local compilation values; all Rational/PMR storage belongs to Budget.
// The caller seals the pieces/control slabs before crossing an owner boundary.
struct AffineTraceRequest {
  Slot<CarrierTag> carrier;
  std::array<Rational, 4> uv;
  std::array<Rational, 6> actual_endpoints;
};
struct AffineTracePiece {
  OriginalRootSlot source_root;
  std::array<Rational, 2> normalized_interval;
  AffineTraceBoundInput bound;
};
struct AffineTracePlan {
  SourceCache source;
  std::pmr::vector<AffineTracePiece> pieces;
  std::pmr::vector<Interval> controls;
  explicit AffineTracePlan(SourceCache roots, bgfd::Budget &budget)
      : source(std::move(roots)), pieces(&budget), controls(&budget) {}
};
AffineTracePlan prepare_affine_trace(const SourceCache &, const AffineTraceRequest &,
                                     bgfd::Budget &);
CurveNodeBounds bound_affine_trace_piece(const AffineTracePlan &, size_t piece);
// Shared original carrier point and finite first/mixed/second derivative bounds
// use the same source intervals/constants; no alternate source evaluator.
struct CarrierDerivativeBounds {
  double du = 0, dv = 0, duu = 0, duv = 0, dvv = 0;
};
CarrierDerivativeBounds
bound_original_carrier_derivatives(const SourceCache &, OriginalRootSlot,
                                   std::span<const Interval, 4> original_box, bgfd::Budget &);
OriginalRootSlot find_original_carrier_root(const SourceCache &, Slot<CarrierTag>,
                                            std::span<const Rational, 2>);
std::optional<std::array<Rational, 3>> exact_carrier_value(const SourceCache &, Slot<CarrierTag>,
                                                           std::span<const Rational, 2>,
                                                           bgfd::Budget &);
struct CurveChordPiece {
  std::array<Rational, 2> normalized_interval;
  AffineTraceBoundInput bound; // same interval-L bound packet as carrier traces
};
struct CurveChordPlan {
  CurveSourceCache source;
  std::pmr::vector<CurveChordPiece> pieces;
  std::pmr::vector<Interval> controls;
  explicit CurveChordPlan(CurveSourceCache roots, bgfd::Budget &b)
      : source(std::move(roots)), pieces(&b), controls(&b) {}
};
struct CurveChordRequest {
  Slot<CurveTag> curve;
  std::array<Rational, 2> original_t;
  std::array<Rational, 6> actual_endpoints;
  std::array<Interval, 3> source_translation{}; // validated original symbolic lift enclosure
};
CurveChordPlan prepare_curve_chord(const CurveSourceCache &, const CurveChordRequest &,
                                   bgfd::Budget &);
} // namespace bgmesh::adaptive
#endif
