#ifndef BREPGPU_EVALUATOR_HPP
#define BREPGPU_EVALUATOR_HPP

#include "backend_api.h"
#include <cmath>
#include <cstdint>

#if defined(__CUDACC__)
#define BG_NUMERIC_INLINE __host__ __device__ __forceinline__
#else
#define BG_NUMERIC_INLINE inline
#endif

namespace brepgpu {

BG_NUMERIC_INLINE unsigned jet_terms(uint32_t order) noexcept {
    return order == 0 ? 1u : (order == 1 ? 3u : 6u);
}

BG_NUMERIC_INLINE bool numeric_finite(double value) noexcept {
#if defined(__CUDA_ARCH__)
    return isfinite(value);
#else
    return std::isfinite(value);
#endif
}

/* One numerator channel and the denominator jet, ordinary derivatives.
   Division remains direct; no implicit reciprocal or geometry tolerance. */
BG_NUMERIC_INLINE bool recover_channel(const double* a, const double* w,
                                      uint32_t order, double* out) noexcept {
    if (!numeric_finite(w[0]) || w[0] <= 0.0) return false;
    out[0] = a[0] / w[0];
    if (order >= 1) {
        out[1] = (a[1] - w[1]*out[0]) / w[0];
        out[2] = (a[2] - w[2]*out[0]) / w[0];
    }
    if (order == 2) {
        out[3] = (a[3] - w[3]*out[0] - 2.0*w[1]*out[1]) / w[0];
        out[4] = (a[4] - w[4]*out[0] - w[1]*out[2] - w[2]*out[1]) / w[0];
        out[5] = (a[5] - w[5]*out[0] - 2.0*w[2]*out[2]) / w[0];
    }
    for (unsigned k = 0; k < jet_terms(order); ++k) {
        if (!numeric_finite(w[k]) || !numeric_finite(out[k])) return false;
    }
    return true;
}

}
#undef BG_NUMERIC_INLINE
#endif
