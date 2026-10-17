#ifndef BREPGPU_MESHER_EXACT_METRIC_HPP
#define BREPGPU_MESHER_EXACT_METRIC_HPP
#include "exact.hpp"
namespace bgmesh {
[[nodiscard]] Rational rounded_dyadic53(const Rational &);
[[nodiscard]] Rational rounded_sqrt53(const Rational &);
} // namespace bgmesh
#endif
