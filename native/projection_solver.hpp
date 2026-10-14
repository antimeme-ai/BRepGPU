#ifndef BREPGPU_PROJECTION_SOLVER_HPP
#define BREPGPU_PROJECTION_SOLVER_HPP

#include "projection_backend_api.h"
#include "evaluator.hpp"
#include <cmath>
#include <limits>

#if defined(__CUDACC__)
#define BG_PROJECTION_INLINE __host__ __device__ __forceinline__
#else
#define BG_PROJECTION_INLINE inline
#endif

namespace brepgpu {
struct ProjectionCandidate {
    double fields[BG_PROJECTION_FIELD_COUNT];
    uint32_t status, updates, evaluations, flags;
};

namespace projection_detail {
BG_PROJECTION_INLINE double invalid_number() noexcept {
#if defined(__CUDA_ARCH__)
    return __longlong_as_double(0x7ff8000000000000LL);
#else
    return std::numeric_limits<double>::quiet_NaN();
#endif
}
BG_PROJECTION_INLINE double magnitude(double v) noexcept { return v < 0 ? -v : v; }
BG_PROJECTION_INLINE double maximum(double a,double b) noexcept { return a > b ? a : b; }
BG_PROJECTION_INLINE double clip(double v) noexcept { return v < 0 ? 0 : (v > 1 ? 1 : v); }
BG_PROJECTION_INLINE double square_root(double v) noexcept {
#if defined(__CUDA_ARCH__)
    return sqrt(v);
#else
    return std::sqrt(v);
#endif
}
BG_PROJECTION_INLINE double hypotenuse(double a,double b) noexcept {
#if defined(__CUDA_ARCH__)
    return hypot(a,b);
#else
    return std::hypot(a,b);
#endif
}
BG_PROJECTION_INLINE double dot3(const double* a,const double* b) noexcept {
    double value=a[0]*b[0];
    value+=a[1]*b[1]; value+=a[2]*b[2];
    return value;
}
BG_PROJECTION_INLINE double dot2(const double* a,const double* b) noexcept {
    return a[0]*b[0]+a[1]*b[1];
}
BG_PROJECTION_INLINE ProjectionCandidate failed(uint32_t status,uint32_t updates=0,
                                                 uint32_t evaluations=0) noexcept {
    ProjectionCandidate a{};
    for(double& v:a.fields)v=invalid_number();
    a.status=status; a.updates=updates; a.evaluations=evaluations;
    return a;
}
BG_PROJECTION_INLINE bool parameters_valid(const double* target,const double* seed) noexcept {
    for(unsigned k=0;k<3;++k)
        if(!numeric_finite(target[k]) || target[k] < -4 || target[k] > 4)return false;
    for(unsigned k=0;k<2;++k)
        if(!numeric_finite(seed[k]) || seed[k] < 0 || seed[k] > 1)return false;
    return true;
}

struct Matrix { double a,b,c; };
BG_PROJECTION_INLINE double matrix_norm(Matrix m,unsigned free=3) noexcept {
    if(free==1)return magnitude(m.a);
    if(free==2)return magnitude(m.c);
    if(!free)return 0;
    return maximum(magnitude(m.a)+magnitude(m.b),magnitude(m.b)+magnitude(m.c));
}
enum class Pivot { Pass, Nonpositive, Small, Invalid };

/* Screen normalized diagonal/Schur values before sqrt. RHS has the same
   normalization; a null output performs the identical metric screen only. */
BG_PROJECTION_INLINE Pivot positive_solve(Matrix m,unsigned free,const double* g,
                                          double* out) noexcept {
    if(out){out[0]=0;out[1]=0;}
    if(!free)return Pivot::Pass;
    const double scale=maximum(1,matrix_norm(m,free));
    if(!numeric_finite(scale))return Pivot::Invalid;
    const double a=(free==2?m.c:m.a)/scale;
    if(!numeric_finite(a))return Pivot::Invalid;
    if(a<=0)return Pivot::Nonpositive;
    if(a<=0x1p-40)return Pivot::Small;
    if(free!=3){
        if(out){const unsigned i=free==1?0:1;out[i]=(-g[i]/scale)/a;
            if(!numeric_finite(out[i]))return Pivot::Invalid;}
        return Pivot::Pass;
    }
    const double b=m.b/scale,c=m.c/scale;
    const double schur=c-b*b/a;
    if(!numeric_finite(schur))return Pivot::Invalid;
    if(schur<=0)return Pivot::Nonpositive;
    if(schur<=0x1p-40)return Pivot::Small;
    if(out){
        const double l00=square_root(a),l10=b/l00,l11=square_root(schur);
        const double y0=(-g[0]/scale)/l00;
        const double y1=((-g[1]/scale)-l10*y0)/l11;
        out[1]=y1/l11;
        out[0]=(y0-l10*out[1])/l00;
        if(!numeric_finite(out[0]) || !numeric_finite(out[1]))return Pivot::Invalid;
    }
    return Pivot::Pass;
}

BG_PROJECTION_INLINE bool state(const double* jet,const double* target,const double* uv,
                                double* residual,double* g,Matrix& metric,Matrix& hessian,
                                double& distance,double& rho) noexcept {
    for(unsigned k=0;k<18;++k)if(!numeric_finite(jet[k]))return false;
    for(unsigned k=0;k<3;++k)residual[k]=jet[k]-target[k];
    g[0]=dot3(residual,jet+3);g[1]=dot3(residual,jet+6);
    metric={dot3(jet+3,jet+3),dot3(jet+3,jet+6),dot3(jet+6,jet+6)};
    hessian={metric.a+dot3(residual,jet+9),metric.b+dot3(residual,jet+12),
        metric.c+dot3(residual,jet+15)};
    distance=dot3(residual,residual);
    rho=maximum(magnitude(uv[0]-clip(uv[0]-g[0])),magnitude(uv[1]-clip(uv[1]-g[1])));
    return numeric_finite(g[0]) && numeric_finite(g[1]) && numeric_finite(metric.a) &&
        numeric_finite(metric.b) && numeric_finite(metric.c) && numeric_finite(hessian.a) &&
        numeric_finite(hessian.b) && numeric_finite(hessian.c) && numeric_finite(distance) &&
        numeric_finite(rho);
}

BG_PROJECTION_INLINE uint32_t boundary_flags(const double* uv) noexcept {
    return uv[0]==0 || uv[0]==1 || uv[1]==0 || uv[1]==1 ? BG_PROJ_BOUNDARY : 0;
}
BG_PROJECTION_INLINE uint32_t curvature(Matrix h,const double* g,const double* uv,
                                        double tolerance) noexcept {
    unsigned free=3; bool weak=false;
    for(unsigned i=0;i<2;++i){
        if(uv[i]==0 || uv[i]==1){
            const double multiplier=uv[i]==0?g[i]:-g[i];
            if(multiplier>tolerance)free&=~(1u<<i);else weak=true;
        }
    }
    if(!free)return BG_PROJ_CURVATURE_POSITIVE;
    const double threshold=0x1p-20*maximum(1,matrix_norm(h));
    const double least=free==1?h.a:(free==2?h.c:
        .5*(h.a+h.c)-hypotenuse(.5*(h.a-h.c),h.b));
    if(least>threshold)return BG_PROJ_CURVATURE_POSITIVE;
    if(!weak && least < -threshold)return BG_PROJ_CURVATURE_NEGATIVE;
    if(weak && (((free&1) && h.a < -threshold) || ((free&2) && h.c < -threshold)))
        return BG_PROJ_CURVATURE_NEGATIVE;
    return BG_PROJ_CURVATURE_AMBIGUOUS;
}

BG_PROJECTION_INLINE void cap(double* p) noexcept {
    const double norm=maximum(magnitude(p[0]),magnitude(p[1]));
    if(norm>.25){const double scale=.25/norm;p[0]*=scale;p[1]*=scale;}
}
BG_PROJECTION_INLINE bool endpoint(const double* uv,const double* g,double* p,
                                   double* displacement) noexcept {
    if(!numeric_finite(p[0]) || !numeric_finite(p[1]))return false;
    cap(p);
    displacement[0]=clip(uv[0]+p[0])-uv[0];
    displacement[1]=clip(uv[1]+p[1])-uv[1];
    return (displacement[0]!=0 || displacement[1]!=0) && dot2(g,displacement)<0;
}
}

/* Evaluator.full(s,t,jet18) and .position(s,t,xyz3) are allocation-free and
   return false for an unusable evaluation. Options are validated by the core. */
template<class Evaluator>
BG_PROJECTION_INLINE ProjectionCandidate project_local(Evaluator& evaluator,const double* target,
                                                         const double* seed,const bg_projection_options& options) noexcept {
    using namespace projection_detail;
    if(!parameters_valid(target,seed))return failed(BG_PROJ_BAD_PARAMETER);
    double uv[2]={seed[0],seed[1]};
    uint32_t updates=0,evaluations=0;
    for(;;){
        double jet[18],residual[3],g[2],distance,rho;
        Matrix metric{},hessian{};
        ++evaluations;
        if(!evaluator.full(uv[0],uv[1],jet) ||
            !state(jet,target,uv,residual,g,metric,hessian,distance,rho))
            return failed(BG_PROJ_ARITHMETIC,updates,evaluations);
        ProjectionCandidate answer{{uv[0],uv[1],jet[0],jet[1],jet[2],distance,rho},
            BG_PROJ_STATIONARY,updates,evaluations,BG_PROJ_CANDIDATE_VALID|boundary_flags(uv)};
        const Pivot rank=positive_solve(metric,3,g,nullptr);
        if(rank==Pivot::Invalid)return failed(BG_PROJ_ARITHMETIC,updates,evaluations);
        if(rank!=Pivot::Pass){answer.status=BG_PROJ_RANK_UNRESOLVED;
            answer.flags|=rank==Pivot::Nonpositive?BG_PROJ_RANK_DEFICIENT:BG_PROJ_ILL_CONDITIONED;
            return answer;}
        if(rho<=options.gradient_tolerance){
            answer.flags|=curvature(hessian,g,uv,options.gradient_tolerance);return answer;}
        if(updates>=options.max_iterations){answer.status=BG_PROJ_ITERATION_LIMIT;return answer;}
        unsigned free=3;
        for(unsigned i=0;i<2;++i)
            if((uv[i]==0 && g[i]>=0) || (uv[i]==1 && g[i]<=0))free&=~(1u<<i);
        double p[2]{},displacement[2]{};
        if(positive_solve(hessian,free,g,p)!=Pivot::Pass){
            const double mu=0x1p-20*maximum(1,matrix_norm(metric,free));
            Matrix damped={metric.a+mu,metric.b,metric.c+mu};
            if(positive_solve(damped,free,g,p)!=Pivot::Pass){p[0]=0;p[1]=0;}
        }
        if(!endpoint(uv,g,p,displacement)){
            const double gamma=1/maximum(1,maximum(matrix_norm(metric),matrix_norm(hessian)));
            p[0]=-gamma*g[0];p[1]=-gamma*g[1];
            if(!endpoint(uv,g,p,displacement)){answer.status=BG_PROJ_STAGNATION;return answer;}
        }
        bool accepted=false,nonzero=false;
        double alpha=1;
        for(uint32_t trial=0;trial<=options.max_backtracks;++trial,alpha*=.5){
            const double next[2]={clip(uv[0]+alpha*displacement[0]),clip(uv[1]+alpha*displacement[1])};
            const double dx[2]={next[0]-uv[0],next[1]-uv[1]};
            if(dx[0]==0 && dx[1]==0)continue;
            nonzero=true;
            const double slope=dot2(g,dx);
            if(!(slope<0))continue;
            double point[3];++evaluations;
            if(!evaluator.position(next[0],next[1],point))return failed(BG_PROJ_ARITHMETIC,updates,evaluations);
            double change[3],sum[3];
            for(unsigned k=0;k<3;++k){
                if(!numeric_finite(point[k]))return failed(BG_PROJ_ARITHMETIC,updates,evaluations);
                change[k]=point[k]-jet[k];sum[k]=(point[k]-target[k])+residual[k];
            }
            const double delta=.5*dot3(change,sum);
            if(!numeric_finite(delta))return failed(BG_PROJ_ARITHMETIC,updates,evaluations);
            if(delta<0 && delta<=0x1p-12*slope){uv[0]=next[0];uv[1]=next[1];++updates;accepted=true;break;}
        }
        if(!accepted){answer.evaluations=evaluations;
            answer.status=nonzero?BG_PROJ_LINESEARCH_LIMIT:BG_PROJ_STAGNATION;return answer;}
    }
}
}
#undef BG_PROJECTION_INLINE
#endif
