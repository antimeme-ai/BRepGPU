#include "projection_solver.hpp"

namespace brepgpu {
namespace {
struct ScalarCPU {
    const bg_patch_desc* patch;
    const double* controls;
    bool evaluate(double s,double t,uint32_t order,double* out) noexcept {
        const double uv[2]={s,t};const uint32_t slot=0;uint32_t status=BG_QUERY_ARITHMETIC;
        evaluate_cpu_range(patch,1,controls,&slot,uv,1,order,0,1,out,&status);
        return status==BG_QUERY_OK;
    }
    bool full(double s,double t,double* out) noexcept {return evaluate(s,t,2,out);}
    bool position(double s,double t,double* out) noexcept {return evaluate(s,t,0,out);}
};
}
void project_cpu_range(const bg_patch_desc* patches,uint64_t patch_count,
    const double* controls,const uint32_t* slots,const double* targets,
    const double* seeds,uint64_t query_count,const bg_projection_options* options,
    uint64_t begin,uint64_t end,double* fields,uint32_t* statuses,
    uint32_t* updates,uint32_t* evaluations,uint32_t* flags) noexcept {
    for(uint64_t i=begin;i<end;++i){
        ProjectionCandidate answer;
        if(slots[i]>=patch_count)answer=projection_detail::failed(BG_PROJ_BAD_PATCH);
        else if(!projection_detail::parameters_valid(targets+3*i,seeds+2*i))
            answer=projection_detail::failed(BG_PROJ_BAD_PARAMETER);
        else{
            const auto& patch=patches[slots[i]];
            if(patch.degree_u>3 || patch.degree_v>3)answer=projection_detail::failed(BG_PROJ_UNSUPPORTED);
            else{ScalarCPU evaluator{&patch,controls};answer=project_local(evaluator,targets+3*i,seeds+2*i,*options);}
        }
        for(unsigned k=0;k<BG_PROJECTION_FIELD_COUNT;++k)fields[k*query_count+i]=answer.fields[k];
        statuses[i]=answer.status;updates[i]=answer.updates;evaluations[i]=answer.evaluations;flags[i]=answer.flags;
    }
}
}
