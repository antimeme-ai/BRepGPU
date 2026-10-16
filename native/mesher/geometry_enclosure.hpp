#ifndef BREPGPU_MESHER_GEOMETRY_ENCLOSURE_HPP
#define BREPGPU_MESHER_GEOMETRY_ENCLOSURE_HPP
#include "geometry_analytic.hpp"

// Internal geometry-owner operations on exact interval endpoints. The same
// scalar mechanism serves analytic points, interval jets and coupled bounds;
// these do not define a second source evaluator or backend decision policy.
namespace bgmesh::geometry_detail {
[[nodiscard]] Rational dyadic_outward(const Rational &, uint32_t bits, bool upward);
[[nodiscard]] ExactEnclosure sum(const ExactEnclosure &, const ExactEnclosure &);
[[nodiscard]] ExactEnclosure negate(const ExactEnclosure &);
[[nodiscard]] ExactEnclosure difference(const ExactEnclosure &, const ExactEnclosure &);
[[nodiscard]] ExactEnclosure product(const ExactEnclosure &, const ExactEnclosure &);
[[nodiscard]] ExactEnclosure times(const ExactEnclosure &, const Rational &);
[[nodiscard]] ExactEnclosure quotient(const ExactEnclosure &, const ExactEnclosure &positive);
[[nodiscard]] ExactEnclosure squared(const ExactEnclosure &);
[[nodiscard]] Rational enclosure_norm_upper(bgfd::Budget &, std::span<const ExactEnclosure>,
                                            uint32_t fractional_bits);
[[nodiscard]] SinCosEnclosure trig_range(GeometryContext &, const Rational &begin,
                                         const Rational &end, uint32_t precision_bits);
} // namespace bgmesh::geometry_detail
#endif
