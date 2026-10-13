#include "backend_api.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace {
void require(bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "%s\n", message);
        std::exit(1);
    }
}
}

int main() {
    bg_patch_desc patch{};
    patch.abi_version = BG_ABI_VERSION;
    patch.struct_size = sizeof(patch);
    patch.degree_u = patch.degree_v = 1;
    patch.control_count = 4;
    const double controls[] = {0,0,0,1, 0,2,0,2, 2,0,0,2, 3,3,3,3};
    const uint32_t slots[] = {0,0,0};
    const double uv[] = {0,0, .5,.5, 1,1};
    double jets[54];
    for (double& x : jets) x = -1000;
    uint32_t statuses[] = {999,999,999};
    brepgpu::evaluate_cpu_range(&patch, 1, controls, slots, uv, 3, 2, 1, 2,
                                jets, statuses);
    const double expected[] = {
        5./8,5./8,3./8, 15./16,-1./16,9./16, -1./16,15./16,9./16,
        -15./16,1./16,-9./16, 1./16,1./16,15./16, 1./16,-15./16,-9./16
    };
    require(statuses[1] == BG_QUERY_OK, "dyadic rational jet was refused");
    require(statuses[0] == 999 && statuses[2] == 999, "worker wrote another range's status");
    for (unsigned k = 0; k < 18; ++k) {
        require(jets[k*3+1] == expected[k], "dyadic rational jet differs from direct analytic value");
        require(jets[k*3] == -1000 && jets[k*3+2] == -1000,
                "worker did not preserve full result stride and disjoint ranges");
    }
    const uint32_t bad_slot = 1;
    double failed[18];
    uint32_t failed_status = 999;
    brepgpu::evaluate_cpu_range(&patch, 1, controls, &bad_slot, uv, 1, 2, 0, 1,
                                failed, &failed_status);
    require(failed_status == BG_QUERY_BAD_PATCH, "invalid slot was not refused");
    for (double x : failed) require(std::isnan(x), "failed query retained a numeric component");
    patch.degree_u = patch.degree_v = 0;
    patch.control_count = 1;
    const double constant[] = {1,2,3,4};
    const uint32_t constant_slot = 0;
    brepgpu::evaluate_cpu_range(&patch, 1, constant, &constant_slot, uv, 1, 2, 0, 1,
                                failed, &failed_status);
    require(failed_status == BG_QUERY_OK, "constant patch was refused");
    require(failed[0] == .25 && failed[1] == .5 && failed[2] == .75,
            "constant rational position is incorrect");
    for (unsigned k = 3; k < 18; ++k) require(failed[k] == 0, "constant patch derivative is nonzero");
    brepgpu::evaluate_cpu_range(nullptr, 0, nullptr, nullptr, nullptr, 0, 2,
                                0, 0, nullptr, nullptr);
    std::puts("native rational jet/range/failure checks passed");
}
