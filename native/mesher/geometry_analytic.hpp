#ifndef BREPGPU_MESHER_GEOMETRY_ANALYTIC_HPP
#define BREPGPU_MESHER_GEOMETRY_ANALYTIC_HPP

#include "geometry_store.hpp"
#include <deque>
#include <optional>

namespace bgmesh {
// Owner-local source enclosures. Endpoints are exact rational bounds, not
// rounded source replacements. The Budget must outlive every returned value.
struct ExactEnclosure final {
  Rational lower, upper;
  ExactEnclosure(Rational, Rational);
  explicit ExactEnclosure(const Rational &value);
  ExactEnclosure(const ExactEnclosure &) = default;
  ExactEnclosure(ExactEnclosure &&) noexcept = default;
  ExactEnclosure &operator=(const ExactEnclosure &) = delete;
  ExactEnclosure &operator=(ExactEnclosure &&) = delete;
};
struct SinCosEnclosure final {
  ExactEnclosure sine, cosine;
};
struct EnclosedPoint final {
  uint32_t dimension;
  std::array<ExactEnclosure, 3> coordinates;
};
struct CanonicalPoint final {
  uint32_t dimension;
  Vec3 xyz;
};
struct GeometryPrecision final {
  // Target point stage, and total transcendental working cap respectively.
  // Pi range reduction reserves guard+argument-magnitude bits within the cap.
  uint32_t initial_bits = 96, maximum_bits = 4096;
};
class GeometryContext final {
  bgfd::Budget &budget_;
  GeometryPrecision policy_;
  uint32_t bits_, guard_bits_ = 33;
  struct PiEntry final {
    uint32_t bits;
    ExactEnclosure enclosure;
  };
  std::pmr::deque<PiEntry> pi_cache_;
  friend SinCosEnclosure enclose_sin_cos(GeometryContext &, const Rational &);

public:
  explicit GeometryContext(bgfd::Budget &, GeometryPrecision = {});
  GeometryContext(const GeometryContext &) = delete;
  GeometryContext &operator=(const GeometryContext &) = delete;
  [[nodiscard]] bgfd::Budget &budget() const noexcept { return budget_; }
  [[nodiscard]] uint32_t bits() const noexcept { return bits_; }
  [[nodiscard]] uint32_t maximum_bits() const noexcept { return policy_.maximum_bits; }
  [[nodiscard]] const ExactEnclosure &pi(uint32_t required_bits);
  // Deterministic doubling schedule; unresolved if no further stage is allowed.
  void refine();
};

[[nodiscard]] SinCosEnclosure enclose_sin_cos(GeometryContext &, const Rational &);
// The finite original analytic formulas, evaluated synchronously from immutable
// source rows. Analytic dimensions2/3 retain only their declared components.
[[nodiscard]] EnclosedPoint evaluate_analytic_curve(GeometryContext &, const Curve &,
                                                    const Rational &);
[[nodiscard]] EnclosedPoint evaluate_analytic_surface(GeometryContext &, const Carrier &,
                                                      const Rational &, const Rational &);
[[nodiscard]] CanonicalPoint canonical_analytic_curve(GeometryContext &, const Curve &,
                                                      const Rational &);
[[nodiscard]] CanonicalPoint canonical_analytic_surface(GeometryContext &, const Carrier &,
                                                        const Rational &, const Rational &);
// General original-source dispatch. Rational roots remain exact and canonical
// output is rounded once; analytic source rows use the same enclosure routines.
[[nodiscard]] EnclosedPoint enclose_original_curve(GeometryContext &, const GeometryStore &,
                                                   Slot<CurveTag>, const Rational &);
[[nodiscard]] EnclosedPoint enclose_original_surface(GeometryContext &, const GeometryStore &,
                                                     Slot<CarrierTag>, const Rational &,
                                                     const Rational &);
[[nodiscard]] CanonicalPoint canonical_original_curve(GeometryContext &, const GeometryStore &,
                                                      Slot<CurveTag>, const Rational &);
[[nodiscard]] CanonicalPoint canonical_original_surface(GeometryContext &, const GeometryStore &,
                                                        Slot<CarrierTag>, const Rational &,
                                                        const Rational &);
} // namespace bgmesh
#endif
