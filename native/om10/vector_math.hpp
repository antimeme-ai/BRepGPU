#ifndef BG_OM10_VECTOR_MATH_HPP
#define BG_OM10_VECTOR_MATH_HPP
#include "types.hpp"
#include <cfloat>
#include <cmath>
#if defined(__CUDACC__)
#define BGOM_HD __host__ __device__ inline
#else
#define BGOM_HD inline
#endif
namespace bgom10 {
BGOM_HD bool finite(double x) {
#if defined(__CUDA_ARCH__)
  return isfinite(x);
#else
  return std::isfinite(x);
#endif
}
BGOM_HD Vec vec(const double *p) { return {p[0], p[1], p[2]}; }
BGOM_HD Vec add(Vec a, Vec b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
BGOM_HD Vec sub(Vec a, Vec b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
BGOM_HD Vec mul(Vec a, double b) { return {a.x * b, a.y * b, a.z * b}; }
BGOM_HD double dot(Vec a, Vec b) { return ((a.x * b.x + a.y * b.y) + a.z * b.z); }
BGOM_HD Vec cross(Vec a, Vec b) {
  return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
BGOM_HD bool good(Vec a) { return finite(a.x) && finite(a.y) && finite(a.z); }
BGOM_HD double maxd(double a, double b) { return a > b ? a : b; }
BGOM_HD double mind(double a, double b) { return a < b ? a : b; }
BGOM_HD bool unit(Vec a, Vec &out) {
  double m = maxd(fabs(a.x), maxd(fabs(a.y), fabs(a.z)));
  if (!finite(m) || m == 0)
    return false;
  Vec b = {a.x / m, a.y / m, a.z / m};
  double n = sqrt(dot(b, b));
  out = mul(b, 1 / n);
  return good(out);
}
BGOM_HD double maximum_component(Vec a) { return maxd(fabs(a.x), maxd(fabs(a.y), fabs(a.z))); }
BGOM_HD Vec divided(Vec a, double scale) { return {a.x / scale, a.y / scale, a.z / scale}; }
// Normalization removes the common positive square scale. Keep the actual
// relative source axes and affine matrix entries; never orthonormalize them.
BGOM_HD Vec cofactor_normal(const double *matrix, Vec n) {
  Vec a = vec(matrix), b = vec(matrix + 4), c = vec(matrix + 8);
  const double scale = maxd(maximum_component(a), maxd(maximum_component(b), maximum_component(c)));
  if (!finite(scale) || scale == 0)
    return {};
  a = divided(a, scale);
  b = divided(b, scale);
  c = divided(c, scale);
  return {dot(cross(b, c), n), dot(cross(c, a), n), dot(cross(a, b), n)};
}
BGOM_HD bool source_vertex_flags(const Carrier &c, uint32_t flags) {
  if ((flags & ~63u) != 0 ||
      (flags & (STORED_XYZ | COFACTOR_NORMAL)) != (STORED_XYZ | COFACTOR_NORMAL))
    return false;
  if ((flags & (NEGATIVE_RADIAL_SHEET | SPHERE_SOUTH)) && !(flags & ANALYTIC_NORMAL_LIMIT))
    return false;
  if ((flags & ANALYTIC_NORMAL_LIMIT) && c.tag != 3 && c.tag != 4)
    return false;
  return !(flags & SPHERE_SOUTH) || c.tag == 4;
}
} // namespace bgom10
#endif
