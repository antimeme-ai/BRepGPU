#ifndef BREPGPU_MESHER_INTERVAL_HPP
#define BREPGPU_MESHER_INTERVAL_HPP
#include "cuda/bound_api.h"
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#if defined(__CUDACC__)
#define BGMB_HD __host__ __device__
#else
#define BGMB_HD
#endif
namespace bgmesh_interval {
using Interval = bgmb_interval;
// Internal numerical primitive. Host callers must first verify nearest binary64,
// FLT_EVAL_METHOD==0 and FTZ/DAZ disabled (filter_bound_cpu does so per call).
// CUDA uses explicit directed intrinsics under the strict sm60 build profile.
BGMB_HD inline bool finite(double value) {
#if defined(__CUDA_ARCH__)
  return isfinite(value);
#else
  return std::isfinite(value);
#endif
}
BGMB_HD inline bool valid(Interval value) {
  return finite(value.lower) && finite(value.upper) && value.lower <= value.upper;
}
BGMB_HD inline Interval invalid() { return {1, -1}; }
BGMB_HD inline bool zero(Interval value) { return value.lower == 0 && value.upper == 0; }
BGMB_HD inline uint64_t bits(double value) {
#if defined(__CUDA_ARCH__)
  return static_cast<uint64_t>(__double_as_longlong(value));
#else
  uint64_t result = 0;
  std::memcpy(&result, &value, sizeof(result));
  return result;
#endif
}
BGMB_HD inline double value(uint64_t word) {
#if defined(__CUDA_ARCH__)
  return __longlong_as_double(static_cast<long long>(word));
#else
  double result = 0;
  std::memcpy(&result, &word, sizeof(result));
  return result;
#endif
}
BGMB_HD inline double down(double x) {
  if (!finite(x))
    return x;
  if (x == 0)
    return value(uint64_t{0x8000000000000001});
  const auto word = bits(x);
  return value((word >> 63) != 0 ? word + 1 : word - 1);
}
BGMB_HD inline double up(double x) {
  if (!finite(x))
    return x;
  if (x == 0)
    return value(1);
  const auto word = bits(x);
  return value((word >> 63) != 0 ? word - 1 : word + 1);
}
BGMB_HD inline double add_down(double a, double b) {
#if defined(__CUDA_ARCH__)
  return __dadd_rd(a, b);
#else
  return down(a + b);
#endif
}
BGMB_HD inline double add_up(double a, double b) {
#if defined(__CUDA_ARCH__)
  return __dadd_ru(a, b);
#else
  return up(a + b);
#endif
}
BGMB_HD inline double mul_down(double a, double b) {
#if defined(__CUDA_ARCH__)
  return __dmul_rd(a, b);
#else
  return down(a * b);
#endif
}
BGMB_HD inline double mul_up(double a, double b) {
#if defined(__CUDA_ARCH__)
  return __dmul_ru(a, b);
#else
  return up(a * b);
#endif
}
BGMB_HD inline double div_down(double a, double b) {
#if defined(__CUDA_ARCH__)
  return __ddiv_rd(a, b);
#else
  return down(a / b);
#endif
}
BGMB_HD inline double div_up(double a, double b) {
#if defined(__CUDA_ARCH__)
  return __ddiv_ru(a, b);
#else
  return up(a / b);
#endif
}
BGMB_HD inline double minimum(double a, double b) { return a < b ? a : b; }
BGMB_HD inline double maximum(double a, double b) { return a > b ? a : b; }
BGMB_HD inline Interval add(Interval a, Interval b) {
  if (!valid(a) || !valid(b))
    return invalid();
  if (zero(a))
    return b;
  if (zero(b))
    return a;
  return {add_down(a.lower, b.lower), add_up(a.upper, b.upper)};
}
BGMB_HD inline Interval negative(Interval a) { return {-a.upper, -a.lower}; }
BGMB_HD inline Interval subtract(Interval a, Interval b) { return add(a, negative(b)); }
BGMB_HD inline Interval multiply(Interval a, Interval b) {
  if (!valid(a) || !valid(b))
    return invalid();
  if (zero(a) || zero(b))
    return {0, 0};
  const auto a0 = mul_down(a.lower, b.lower), a1 = mul_down(a.lower, b.upper);
  const auto a2 = mul_down(a.upper, b.lower), a3 = mul_down(a.upper, b.upper);
  const auto b0 = mul_up(a.lower, b.lower), b1 = mul_up(a.lower, b.upper);
  const auto b2 = mul_up(a.upper, b.lower), b3 = mul_up(a.upper, b.upper);
  return {minimum(minimum(a0, a1), minimum(a2, a3)), maximum(maximum(b0, b1), maximum(b2, b3))};
}
BGMB_HD inline Interval square(Interval a) {
  if (!valid(a))
    return invalid();
  if (zero(a))
    return {0, 0};
  const auto lo = a.lower <= 0 && a.upper >= 0
                      ? 0
                      : maximum(0, minimum(mul_down(a.lower, a.lower), mul_down(a.upper, a.upper)));
  return {lo, maximum(mul_up(a.lower, a.lower), mul_up(a.upper, a.upper))};
}
BGMB_HD inline Interval divide_positive(Interval a, Interval b) {
  if (!valid(a) || !valid(b) || b.lower <= 0)
    return invalid();
  if (zero(a))
    return {0, 0};
  const auto a0 = div_down(a.lower, b.lower), a1 = div_down(a.lower, b.upper);
  const auto a2 = div_down(a.upper, b.lower), a3 = div_down(a.upper, b.upper);
  const auto b0 = div_up(a.lower, b.lower), b1 = div_up(a.lower, b.upper);
  const auto b2 = div_up(a.upper, b.lower), b3 = div_up(a.upper, b.upper);
  return {minimum(minimum(a0, a1), minimum(a2, a3)), maximum(maximum(b0, b1), maximum(b2, b3))};
}
BGMB_HD inline Interval ratio(uint32_t numerator, uint32_t denominator) {
  if (numerator == 0)
    return {0, 0};
  if (numerator == denominator)
    return {1, 1};
  return divide_positive({static_cast<double>(numerator), static_cast<double>(numerator)},
                         {static_cast<double>(denominator), static_cast<double>(denominator)});
}
BGMB_HD inline Interval lerp(Interval a, Interval b, Interval t) {
  return add(multiply(subtract({1, 1}, t), a), multiply(t, b));
}
} // namespace bgmesh_interval
#undef BGMB_HD
#endif
