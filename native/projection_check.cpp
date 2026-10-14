#include "projection_backend_api.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace {
void require(bool ok, const char* message) {
    if (!ok) { std::fprintf(stderr, "%s\n", message); std::exit(1); }
}
bg_projection_options options() {
    return {BG_PROJECTION_ABI_VERSION, sizeof(bg_projection_options),
        BG_PROJECTION_DEFAULT_ITERATIONS, BG_PROJECTION_DEFAULT_BACKTRACKS,
        BG_PROJECTION_DEFAULT_TOLERANCE, 0, 0};
}
bg_patch_desc patch(unsigned p, unsigned q) {
    bg_patch_desc value{};
    value.abi_version = BG_ABI_VERSION; value.struct_size = sizeof(value);
    value.degree_u = p; value.degree_v = q; value.control_count = (p+1)*(q+1);
    return value;
}
struct Answer {
    double fields[7];
    uint32_t status = 99, updates = 99, evaluations = 99, flags = 99;
};
Answer solve(const bg_patch_desc& p, const double* controls, const double* target,
             const double* seed, const bg_projection_options& opt) {
    Answer a; const uint32_t slot = 0;
    brepgpu::project_cpu_range(&p,1,controls,&slot,target,seed,1,&opt,0,1,
        a.fields,&a.status,&a.updates,&a.evaluations,&a.flags);
    return a;
}
void stationary(const Answer& a, uint32_t flags, const double* uv, double distance) {
    require(a.status == BG_PROJ_STATIONARY, "analytic candidate is not stationary");
    require(a.flags == flags, "analytic rank/curvature flags differ");
    require(a.fields[0] == uv[0] && a.fields[1] == uv[1], "analytic UV differs");
    require(a.fields[5] == distance && a.fields[6] == 0, "analytic distance/residual differs");
    require(a.evaluations <= BG_PROJECTION_MAX_EVALUATIONS, "evaluator cap exceeded");
}
}

int main() {
    auto opt = options();
    auto p = patch(1,1);
    const double plane[] = {0,0,0,1, 0,1,0,1, 1,0,0,1, 1,1,0,1};
    const double target[] = {.25,.75,1}, seed[] = {.125,.25};
    auto a = solve(p,plane,target,seed,opt);
    const double expected[] = {.25,.75};
    stationary(a,BG_PROJ_CANDIDATE_VALID|BG_PROJ_CURVATURE_POSITIVE,expected,1);
    require(a.updates>0, "nontrivial plane did not update");
    const double offset_plane[]={0,0,0,1,0,0,0,1,0,1,0,1,1,0,0,1,1,1,0,1};
    p.control_offset=1;
    a=solve(p,offset_plane,target,seed,opt);
    stationary(a,BG_PROJ_CANDIDATE_VALID|BG_PROJ_CURVATURE_POSITIVE,expected,1);
    p.control_offset=0;
    const double edge_target[] = {-1,.75,1}, edge[] = {0,.75};
    a = solve(p,plane,edge_target,seed,opt);
    stationary(a,BG_PROJ_CANDIDATE_VALID|BG_PROJ_BOUNDARY|BG_PROJ_CURVATURE_POSITIVE,edge,2);

    opt.max_iterations=0;
    a=solve(p,plane,target,seed,opt);
    require(a.status==BG_PROJ_ITERATION_LIMIT && a.updates==0 && a.evaluations==1,
        "zero-update request did not report its evaluated seed");
    a=solve(p,plane,target,expected,opt);
    stationary(a,BG_PROJ_CANDIDATE_VALID|BG_PROJ_CURVATURE_POSITIVE,expected,1);
    require(a.updates==0 && a.evaluations==1, "initial solution incurred fictitious work");
    opt=options();

    p=patch(2,1);
    const double parabola[]={-1,0,1,1,-1,1,1,1, 0,0,-1,1,0,1,-1,1, 1,0,1,1,1,1,1,1};
    const double center[]={.5,.5}, saddle_target[]={0,.5,.75};
    a=solve(p,parabola,saddle_target,center,opt);
    stationary(a,BG_PROJ_CANDIDATE_VALID|BG_PROJ_CURVATURE_NEGATIVE,center,9./16);
    const double flat_target[]={0,.5,.5};
    a=solve(p,parabola,flat_target,center,opt);
    stationary(a,BG_PROJ_CANDIDATE_VALID|BG_PROJ_CURVATURE_AMBIGUOUS,center,.25);

    p=patch(1,1);
    const double corner_net[]={0,0,0,1,0,1,0,1,1,0,0,1,1,1,1,1};
    const double corner_target[]={0,0,-2}, corner[]={0,0};
    a=solve(p,corner_net,corner_target,corner,opt);
    stationary(a,BG_PROJ_CANDIDATE_VALID|BG_PROJ_BOUNDARY|BG_PROJ_CURVATURE_AMBIGUOUS,corner,4);

    const double tiny=0x1p-10;
    const double threshold_net[]={-1,0,0,1,0,-tiny,0,1,0,tiny,0,1,1,0,0,1};
    const double origin[]={0,0,0};
    a=solve(p,threshold_net,origin,center,opt);
    stationary(a,BG_PROJ_CANDIDATE_VALID|BG_PROJ_CURVATURE_AMBIGUOUS,center,0);

    const double epsilon=0x1p-20;
    const double rank_floor_net[]={0,0,0,1,0,epsilon,0,1,1,0,0,1,1,epsilon,0,1};
    const double rank_floor_target[]={.5,.5*epsilon,1};
    a=solve(p,rank_floor_net,rank_floor_target,center,opt);
    require(a.status==BG_PROJ_RANK_UNRESOLVED &&
        a.flags==(BG_PROJ_CANDIDATE_VALID|BG_PROJ_ILL_CONDITIONED) && a.evaluations==1,
        "metric equality at strict pivot floor was admitted");

    p=patch(0,0); const double constant[]={0,0,0,1};
    a=solve(p,constant,target,center,opt);
    require(a.status==BG_PROJ_RANK_UNRESOLVED &&
        a.flags==(BG_PROJ_CANDIDATE_VALID|BG_PROJ_RANK_DEFICIENT) && a.evaluations==1,
        "rank failure was hidden by vanishing gradient");

    p=patch(3,1);
    const double cubic[]={-1,0,0,1,-1,1,0,1,1,0,0,1,1,1,0,1,
        -1,0,0,1,-1,1,0,1,1,0,0,1,1,1,0,1};
    const double cubic_target[]={0x1p-10,.5,0}, cubic_seed[]={33./64,.5};
    opt.max_backtracks=0;
    a=solve(p,cubic,cubic_target,cubic_seed,opt);
    require(a.status==BG_PROJ_LINESEARCH_LIMIT && a.updates==0 && a.evaluations==2 &&
        a.fields[0]==cubic_seed[0] && a.fields[1]==cubic_seed[1],
        "exhausted full-trial cubic replaced seed or mislabeled convergence");

    p=patch(1,1); opt=options();
    const uint32_t slots[]={0,0,0};
    const double targets[]={.25,.75,1,.25,.75,1,.25,.75,1};
    const double seeds[]={.125,.25,.125,.25,.125,.25};
    double fields[21]; for(double& v:fields)v=-999;
    uint32_t statuses[]={99,99,99}, updates[]={99,99,99}, evaluations[]={99,99,99}, flags[]={99,99,99};
    brepgpu::project_cpu_range(&p,1,plane,slots,targets,seeds,3,&opt,1,2,
        fields,statuses,updates,evaluations,flags);
    require(statuses[1]==BG_PROJ_STATIONARY && fields[1]==.25 && fields[4]==.75,
        "worker chunk reinterpreted full SoA stride");
    for(unsigned k=0;k<7;++k)require(fields[k*3]==-999 && fields[k*3+2]==-999,
        "worker wrote another range's numeric output");
    require(statuses[0]==99 && statuses[2]==99 && updates[0]==99 && evaluations[2]==99 && flags[0]==99,
        "worker wrote another range's integer output");
    const uint32_t invalid_slot=1;
    brepgpu::project_cpu_range(&p,1,plane,&invalid_slot,target,seed,1,&opt,0,1,
        a.fields,&a.status,&a.updates,&a.evaluations,&a.flags);
    require(a.status==BG_PROJ_BAD_PATCH && a.updates==0 && a.evaluations==0 && a.flags==0,
        "bad patch was evaluated or retained flags");
    for(double v:a.fields)require(std::isnan(v),"invalid request retained candidate numbers");
    brepgpu::project_cpu_range(nullptr,0,nullptr,nullptr,nullptr,nullptr,0,nullptr,0,0,
        nullptr,nullptr,nullptr,nullptr,nullptr);
    std::puts("native projection analytic/rank/curvature/cap/range checks passed");
}
