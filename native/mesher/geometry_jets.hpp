#ifndef BREPGPU_MESHER_GEOMETRY_JETS_HPP
#define BREPGPU_MESHER_GEOMETRY_JETS_HPP
#include "geometry_analytic.hpp"

namespace bgmesh {
struct CurveJetEnclosure final {
  uint32_t dimension;
  std::array<ExactEnclosure, 3> position, first, second;
};
struct SurfaceJetEnclosure final {
  std::array<ExactEnclosure, 3> position, du, dv, duu, duv, dvv;
};
struct GeometryBoundPrecision final {
  uint32_t transcendental_bits = 96, norm_fractional_bits = 96;
};
// Position-only bounds may span multiple C0 pieces; no derivative continuity
// claim is made. Rational natural-domain exit remains unsupported.
[[nodiscard]] EnclosedPoint enclose_original_curve_range(
    GeometryContext &,const GeometryStore &,Slot<CurveTag>,const std::array<Rational,2> &,
    uint32_t precision_bits=96);
// Whole-interval bounds in ORIGINAL parameter units. Rational ranges are
// positive and lie within one original root, so C0 breaks are never treated C2.
// Analytic precision is a request/profile value, independent of preceding
// canonical-point refinement and cache history in the same context.
[[nodiscard]] CurveJetEnclosure enclose_rational_curve_jet(const BezierCurve &, const Rational &,
                                                           const Rational &);
[[nodiscard]] SurfaceJetEnclosure enclose_rational_surface_jet(const BezierSurface &,
                                                               const std::array<Rational, 4> &);
[[nodiscard]] CurveJetEnclosure enclose_analytic_curve_jet(GeometryContext &, const Curve &,
                                                           const Rational &, const Rational &,
                                                           uint32_t precision_bits = 96);
[[nodiscard]] SurfaceJetEnclosure enclose_analytic_surface_jet(GeometryContext &, const Carrier &,
                                                               const std::array<Rational, 4> &,
                                                               uint32_t precision_bits = 96);
[[nodiscard]] CurveJetEnclosure enclose_original_curve_jet(GeometryContext &, const GeometryStore &,
                                                           Slot<CurveTag>, const Rational &,
                                                           const Rational &,
                                                           uint32_t precision_bits = 96);
[[nodiscard]] SurfaceJetEnclosure
enclose_original_surface_jet(GeometryContext &, const GeometryStore &, Slot<CarrierTag>,
                             const std::array<Rational, 4> &, uint32_t precision_bits = 96);
// A point jet selects the stable lower original span on an exact knot tie.
// Rational jets are exact collapsed enclosures; analytic precision is explicit.
[[nodiscard]] SurfaceJetEnclosure
enclose_original_surface_point_jet(GeometryContext &, const GeometryStore &, Slot<CarrierTag>,
                                   const Rational &u, const Rational &v,
                                   uint32_t precision_bits = 96);
// Actual endpoint words enter once. Rational original curves use the runtime's
// exact residual tasks; this host comparison is for original analytic kinds.
[[nodiscard]] Rational analytic_curve_chord_upper(GeometryContext &, const Curve &,
                                                  const Rational &begin, const Rational &end,
                                                  std::span<const Rational, 6> actual_endpoint_xyz,
                                                  GeometryBoundPrecision = {});
} // namespace bgmesh
#endif
