#include "evaluator.hpp"

#include <limits>

namespace brepgpu {
namespace {

void basis_values(unsigned degree, double t, double* out) noexcept {
    const double s = 1.0-t;
    switch (degree) {
    case 0: out[0] = 1.0; break;
    case 1: out[0] = s; out[1] = t; break;
    case 2:
        out[0] = s*s; out[1] = 2.0*s*t; out[2] = t*t;
        break;
    case 3:
        out[0] = s*s*s; out[1] = 3.0*s*s*t;
        out[2] = 3.0*s*t*t; out[3] = t*t*t;
        break;
    }
}

double basis_entry(const double* basis, int i, unsigned degree) noexcept {
    return i < 0 || static_cast<unsigned>(i) > degree ? 0.0 : basis[i];
}

void basis_jet(unsigned degree, double t, uint32_t order, double out[3][4]) noexcept {
    basis_values(degree, t, out[0]);
    if (order >= 1 && degree >= 1) {
        double lower[4]{};
        basis_values(degree-1, t, lower);
        for (unsigned i = 0; i <= degree; ++i) {
            out[1][i] = degree*(basis_entry(lower, static_cast<int>(i)-1, degree-1)
                                    - basis_entry(lower, static_cast<int>(i), degree-1));
        }
    }
    if (order == 2 && degree >= 2) {
        double lower[4]{};
        basis_values(degree-2, t, lower);
        for (unsigned i = 0; i <= degree; ++i) {
            out[2][i] = degree*(degree-1)*(
                basis_entry(lower, static_cast<int>(i)-2, degree-2)
                - 2.0*basis_entry(lower, static_cast<int>(i)-1, degree-2)
                + basis_entry(lower, static_cast<int>(i), degree-2));
        }
    }
}

void fail_query(uint64_t request, uint64_t count, uint32_t order,
                uint32_t status, double* jets, uint32_t* statuses) noexcept {
    const double invalid = std::numeric_limits<double>::quiet_NaN();
    for (unsigned k = 0; k < jet_terms(order)*3; ++k) jets[k*count+request] = invalid;
    statuses[request] = status;
}

}

void evaluate_cpu_range(const bg_patch_desc* patches, uint64_t patch_count,
                        const double* controls, const uint32_t* slots,
                        const double* uv, uint64_t query_count, uint32_t order,
                        uint64_t begin, uint64_t end, double* jets,
                        uint32_t* statuses) noexcept {
    /* The runtime validates immutable descriptors, sizes and order before
       invoking this helper. Each worker retains the full SoA stride. */
    for (uint64_t request = begin; request < end; ++request) {
        if (slots[request] >= patch_count) {
            fail_query(request, query_count, order, BG_QUERY_BAD_PATCH, jets, statuses);
            continue;
        }
        const double s = uv[2*request], t = uv[2*request+1];
        if (!numeric_finite(s) || !numeric_finite(t) || s < 0.0 || s > 1.0
                || t < 0.0 || t > 1.0) {
            fail_query(request, query_count, order, BG_QUERY_BAD_PARAMETER, jets, statuses);
            continue;
        }
        const bg_patch_desc& patch = patches[slots[request]];
        const unsigned p = patch.degree_u, q = patch.degree_v;
        if (p > 3 || q > 3) {
            fail_query(request, query_count, order, BG_QUERY_UNSUPPORTED, jets, statuses);
            continue;
        }
        double bs[3][4]{}, bt[3][4]{}, h[6][4]{};
        basis_jet(p, s, order, bs);
        basis_jet(q, t, order, bt);
        const double* net = controls + patch.control_offset*4;
        for (unsigned i = 0; i <= p; ++i) {
            for (unsigned j = 0; j <= q; ++j) {
                double coefficient[6]{};
                coefficient[0] = bs[0][i]*bt[0][j];
                if (order >= 1) {
                    coefficient[1] = bs[1][i]*bt[0][j];
                    coefficient[2] = bs[0][i]*bt[1][j];
                }
                if (order == 2) {
                    coefficient[3] = bs[2][i]*bt[0][j];
                    coefficient[4] = bs[1][i]*bt[1][j];
                    coefficient[5] = bs[0][i]*bt[2][j];
                }
                const double* control = net + (i*(q+1)+j)*4;
                for (unsigned k = 0; k < jet_terms(order); ++k) {
                    for (unsigned axis = 0; axis < 4; ++axis)
                        h[k][axis] += coefficient[k]*control[axis];
                }
            }
        }
        double weights[6]{};
        for (unsigned k = 0; k < jet_terms(order); ++k) weights[k] = h[k][3];
        bool success = true;
        for (unsigned axis = 0; axis < 3; ++axis) {
            double numerator[6]{}, result[6]{};
            for (unsigned k = 0; k < jet_terms(order); ++k) numerator[k] = h[k][axis];
            if (!recover_channel(numerator, weights, order, result)) {
                success = false;
                break;
            }
            for (unsigned k = 0; k < jet_terms(order); ++k)
                jets[(k*3+axis)*query_count+request] = result[k];
        }
        if (success) statuses[request] = BG_QUERY_OK;
        else fail_query(request, query_count, order, BG_QUERY_ARITHMETIC, jets, statuses);
    }
}
}
