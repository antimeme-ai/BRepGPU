#ifndef BREPGPU_DISCOVERY_MATH_HPP
#define BREPGPU_DISCOVERY_MATH_HPP
#include "discovery_backend_api.h"
#include "projection_solver.hpp"
#include <cmath>
#include <limits>
#if defined(__CUDACC__)
#define BG_DISC_INLINE __host__ __device__ __forceinline__
#else
#define BG_DISC_INLINE inline
#endif
namespace brepgpu {
struct DiscoveryCandidate {
    double fields[8];
    uint32_t status,flags,visits,pruned,refinements,evaluations;
};
namespace discovery_detail {
struct Interval { double lo,hi; };
BG_DISC_INLINE double finite_maximum() noexcept {return 0x1.fffffffffffffp1023;}
BG_DISC_INLINE double down_add(double a,double b) noexcept {
#if defined(__CUDA_ARCH__)
    return __dadd_rd(a,b);
#else
    return std::nextafter(a+b,-std::numeric_limits<double>::infinity());
#endif
}
BG_DISC_INLINE double up_add(double a,double b) noexcept {
#if defined(__CUDA_ARCH__)
    return __dadd_ru(a,b);
#else
    return std::nextafter(a+b,std::numeric_limits<double>::infinity());
#endif
}
BG_DISC_INLINE double down_sub(double a,double b) noexcept {
#if defined(__CUDA_ARCH__)
    return __dsub_rd(a,b);
#else
    return std::nextafter(a-b,-std::numeric_limits<double>::infinity());
#endif
}
BG_DISC_INLINE double up_sub(double a,double b) noexcept {
#if defined(__CUDA_ARCH__)
    return __dsub_ru(a,b);
#else
    return std::nextafter(a-b,std::numeric_limits<double>::infinity());
#endif
}
BG_DISC_INLINE double down_mul(double a,double b) noexcept {
#if defined(__CUDA_ARCH__)
    return __dmul_rd(a,b);
#else
    return std::nextafter(a*b,-std::numeric_limits<double>::infinity());
#endif
}
BG_DISC_INLINE double up_mul(double a,double b) noexcept {
#if defined(__CUDA_ARCH__)
    return __dmul_ru(a,b);
#else
    return std::nextafter(a*b,std::numeric_limits<double>::infinity());
#endif
}
BG_DISC_INLINE double down_div(double a,double b) noexcept {
#if defined(__CUDA_ARCH__)
    return __ddiv_rd(a,b);
#else
    return std::nextafter(a/b,-std::numeric_limits<double>::infinity());
#endif
}
BG_DISC_INLINE double up_div(double a,double b) noexcept {
#if defined(__CUDA_ARCH__)
    return __ddiv_ru(a,b);
#else
    return std::nextafter(a/b,std::numeric_limits<double>::infinity());
#endif
}
BG_DISC_INLINE double minimum(double a,double b) noexcept {return a<b?a:b;}
BG_DISC_INLINE double maximum(double a,double b) noexcept {return a>b?a:b;}
BG_DISC_INLINE bool valid(Interval a) noexcept {
    return numeric_finite(a.lo)&&numeric_finite(a.hi)&&a.lo<=a.hi;
}
BG_DISC_INLINE Interval add(Interval a,Interval b) noexcept {
    return {down_add(a.lo,b.lo),up_add(a.hi,b.hi)};
}
BG_DISC_INLINE Interval sub(Interval a,Interval b) noexcept {
    return {down_sub(a.lo,b.hi),up_sub(a.hi,b.lo)};
}
BG_DISC_INLINE Interval negate(Interval a) noexcept {return {-a.hi,-a.lo};}
BG_DISC_INLINE Interval multiply(Interval a,Interval b) noexcept {
    const double l0=down_mul(a.lo,b.lo),l1=down_mul(a.lo,b.hi),
        l2=down_mul(a.hi,b.lo),l3=down_mul(a.hi,b.hi);
    const double h0=up_mul(a.lo,b.lo),h1=up_mul(a.lo,b.hi),
        h2=up_mul(a.hi,b.lo),h3=up_mul(a.hi,b.hi);
    if(!numeric_finite(l0)||!numeric_finite(l1)||!numeric_finite(l2)||!numeric_finite(l3)||
       !numeric_finite(h0)||!numeric_finite(h1)||!numeric_finite(h2)||!numeric_finite(h3))
        return {projection_detail::invalid_number(),projection_detail::invalid_number()};
    return {minimum(minimum(l0,l1),minimum(l2,l3)),maximum(maximum(h0,h1),maximum(h2,h3))};
}
BG_DISC_INLINE Interval divide_positive(Interval a,Interval b) noexcept {
    if(!valid(a)||!valid(b)||b.lo<=0)
        return {projection_detail::invalid_number(),projection_detail::invalid_number()};
    const double l0=down_div(a.lo,b.lo),l1=down_div(a.lo,b.hi),
        l2=down_div(a.hi,b.lo),l3=down_div(a.hi,b.hi);
    const double h0=up_div(a.lo,b.lo),h1=up_div(a.lo,b.hi),
        h2=up_div(a.hi,b.lo),h3=up_div(a.hi,b.hi);
    if(!numeric_finite(l0)||!numeric_finite(l1)||!numeric_finite(l2)||!numeric_finite(l3)||
       !numeric_finite(h0)||!numeric_finite(h1)||!numeric_finite(h2)||!numeric_finite(h3))
        return {projection_detail::invalid_number(),projection_detail::invalid_number()};
    return {minimum(minimum(l0,l1),minimum(l2,l3)),maximum(maximum(h0,h1),maximum(h2,h3))};
}
BG_DISC_INLINE bool intersect(Interval& a,double lo,double hi) noexcept {
    if(!valid(a))return false;
    a.lo=maximum(a.lo,lo);a.hi=minimum(a.hi,hi);return valid(a);
}
BG_DISC_INLINE Interval midpoint(Interval a,Interval b) noexcept {
    return {down_mul(down_add(a.lo,b.lo),.5),up_mul(up_add(a.hi,b.hi),.5)};
}
BG_DISC_INLINE int direction(unsigned n,unsigned k) noexcept {
    /* Fixed non-coordinate normals; no dynamically initialized device data. */
    switch(n){
    case 0:return k==2?0:1;
    case 1:return k==0?1:(k==1?-1:0);
    case 2:return k==1?0:1;
    case 3:return k==0?1:(k==2?-1:0);
    case 4:return k==0?0:1;
    case 5:return k==1?1:(k==2?-1:0);
    case 6:return 1;
    case 7:return k==2?-1:1;
    case 8:return k==1?-1:1;
    default:return k==0?1:-1;
    }
}
BG_DISC_INLINE Interval projection(const Interval* values,unsigned n) noexcept {
    Interval out{};bool started=false;
    for(unsigned k=0;k<3;++k){const int sign=direction(n,k);if(!sign)continue;
        const Interval term=sign>0?values[k]:negate(values[k]);
        out=started?add(out,term):term;started=true;}
    return out;
}
BG_DISC_INLINE Interval curve(Interval* c,unsigned degree,double t) noexcept {
    if(t==0)return c[0];
    if(t==1)return c[degree];
    const Interval parameter{t,t},complement{down_sub(1,t),up_sub(1,t)};
    for(unsigned r=degree;r>0;--r)
        for(unsigned i=0;i<r;++i)c[i]=add(multiply(complement,c[i]),multiply(parameter,c[i+1]));
    return c[0];
}
BG_DISC_INLINE Interval channel(const double* net,unsigned p,unsigned q,unsigned a,
                                double s,double t) noexcept {
    Interval row[4],c[4];
    for(unsigned j=0;j<=q;++j){
        for(unsigned i=0;i<=p;++i){const double v=net[(i*(q+1)+j)*4+a];c[i]={v,v};}
        row[j]=curve(c,p,s);}
    return curve(row,q,t);
}
BG_DISC_INLINE bool witness_upper(const double* net,unsigned p,unsigned q,
    double s,double t,const double* target,double& upper) noexcept {
    Interval w=channel(net,p,q,3,s,t);if(!intersect(w,1,256))return false;
    double squares[3];
    for(unsigned a=0;a<3;++a){Interval numerator=channel(net,p,q,a,s,t);
        if(!intersect(numerator,-256,256))return false;
        Interval point=divide_positive(numerator,w);if(!intersect(point,-1,1))return false;
        const Interval residual=sub(point,{target[a],target[a]});if(!valid(residual))return false;
        const double magnitude=maximum(projection_detail::magnitude(residual.lo),
                                       projection_detail::magnitude(residual.hi));
        squares[a]=up_mul(magnitude,magnitude);
        if(!numeric_finite(squares[a])||squares[a]<0)return false;}
    upper=up_add(up_add(squares[0],squares[1]),squares[2]);
    return numeric_finite(upper)&&upper>=0;
}
struct TargetBounds { Interval dots[10];Interval norm; };
BG_DISC_INLINE bool prepare_target(const double* target,uint32_t mode,TargetBounds& out) noexcept {
    const Interval values[3]={{target[0],target[0]},{target[1],target[1]},{target[2],target[2]}};
    if(mode==BG_BOUND_DOP26)for(unsigned n=0;n<10;++n){out.dots[n]=projection(values,n);
        if(!valid(out.dots[n]))return false;}
    if(mode==BG_BOUND_BERNSTEIN){out.norm=add(add(multiply(values[0],values[0]),
        multiply(values[1],values[1])),multiply(values[2],values[2]));
        if(!intersect(out.norm,0,48))return false;}
    return mode<=BG_BOUND_BERNSTEIN;
}
BG_DISC_INLINE bool node_lower(const bg_hierarchy_node& node,uint32_t id,
    const double* bounds,uint32_t mode,unsigned p,unsigned q,const double* target,
    const TargetBounds& prepared,double& lower) noexcept {
    double squares[3];
    for(unsigned a=0;a<3;++a){if(!numeric_finite(node.lower[a])||!numeric_finite(node.upper[a])||
        node.lower[a]>node.upper[a])return false;
        double gap=0;
        if(target[a]<node.lower[a])gap=maximum(0,down_sub(node.lower[a],target[a]));
        else if(target[a]>node.upper[a])gap=maximum(0,down_sub(target[a],node.upper[a]));
        squares[a]=maximum(0,down_mul(gap,gap));}
    lower=maximum(0,down_add(down_add(squares[0],squares[1]),squares[2]));
    if(!numeric_finite(lower))return false;
    if(mode==BG_BOUND_DOP26){
        for(unsigned n=0;n<10;++n){const uint64_t offset=uint64_t(id)*20+2*n;
            const Interval support{bounds[offset],bounds[offset+1]};if(!valid(support))return false;
            const Interval x=prepared.dots[n];
            const double gap=maximum(0,maximum(down_sub(support.lo,x.hi),down_sub(x.lo,support.hi)));
            const double slab=maximum(0,down_div(down_mul(gap,gap),n<6?2:3));
            if(!numeric_finite(slab))return false;
            lower=maximum(lower,slab);}
    }else if(mode==BG_BOUND_BERNSTEIN){
        const double* cache=bounds+uint64_t(id)*490;
        double least=finite_maximum();
        const unsigned count=(2*p+1)*(2*q+1);
        for(unsigned i=0;i<count;++i){Interval c[5];
            for(unsigned k=0;k<5;++k){c[k]={cache[k*98+2*i],cache[k*98+2*i+1]};if(!valid(c[k]))return false;}
            if(c[4].lo<=0)return false;
            Interval sum=multiply({target[0],target[0]},c[1]);
            sum=add(sum,multiply({target[1],target[1]},c[2]));
            sum=add(sum,multiply({target[2],target[2]},c[3]));
            const Interval numerator=add(sub(c[0],multiply({2,2},sum)),multiply(prepared.norm,c[4]));
            if(!valid(numerator))return false;
            const double ratio=down_div(numerator.lo,numerator.lo<0?c[4].lo:c[4].hi);
            if(!numeric_finite(ratio))return false;
            least=minimum(least,ratio);}
        lower=maximum(lower,maximum(0,least));
    }
    return numeric_finite(lower)&&lower>=0;
}
BG_DISC_INLINE DiscoveryCandidate failed(uint32_t status,const DiscoveryCandidate* prior=nullptr) noexcept {
    DiscoveryCandidate a{};for(double& v:a.fields)v=projection_detail::invalid_number();
    a.status=status;a.flags=status==BG_DISC_ARITHMETIC?BG_DISC_ARITHMETIC_REASON:0;
    if(prior){a.visits=prior->visits;a.pruned=prior->pruned;a.refinements=prior->refinements;
        a.evaluations=prior->evaluations;}return a;
}
template<class Evaluator>
BG_DISC_INLINE bool refine(Evaluator& evaluator,const double* net,unsigned p,unsigned q,
    const double* target,const double* seed,DiscoveryCandidate& answer) noexcept {
    const bg_projection_options options{BG_PROJECTION_ABI_VERSION,sizeof(bg_projection_options),
        BG_DISCOVERY_PROJECTOR_ITERATIONS,BG_DISCOVERY_PROJECTOR_BACKTRACKS,
        BG_DISCOVERY_PROJECTOR_TOLERANCE,0,0};
    ++answer.refinements;const ProjectionCandidate candidate=project_local(evaluator,target,seed,options);
    answer.evaluations+=candidate.evaluations;
    if(candidate.status==BG_PROJ_ARITHMETIC || !(candidate.flags&BG_PROJ_CANDIDATE_VALID))return false;
    for(double v:candidate.fields)if(!numeric_finite(v))return false;
    if(candidate.fields[0]<0||candidate.fields[0]>1||candidate.fields[1]<0||candidate.fields[1]>1)return false;
    double upper;
    if(!witness_upper(net,p,q,candidate.fields[0],candidate.fields[1],target,upper))return false;
    if(upper<answer.fields[7]){for(unsigned k=0;k<6;++k)answer.fields[k]=candidate.fields[k];answer.fields[7]=upper;}
    return true;
}
BG_DISC_INLINE double center(uint32_t index,uint32_t level) noexcept {
#if defined(__CUDA_ARCH__)
    return ldexp(double(index)*2+1,-int(level)-1);
#else
    return std::ldexp(double(index)*2+1,-int(level)-1);
#endif
}
}

template<class Evaluator>
BG_DISC_INLINE DiscoveryCandidate discover_local(Evaluator& evaluator,const double* original_net,
    unsigned p,unsigned q,const bg_hierarchy_node* nodes,uint64_t node_count,uint32_t root,
    const double* bounds,uint32_t mode,const double* target,const bg_discovery_options& options) noexcept {
    using namespace discovery_detail;
    DiscoveryCandidate answer{};
    const double initial[2]={.5,.5};
    if(!projection_detail::parameters_valid(target,initial))return failed(BG_DISC_BAD_TARGET);
    if(p>3||q>3||root>=node_count||mode>BG_BOUND_BERNSTEIN)return failed(BG_DISC_ARITHMETIC);
    answer.fields[0]=.5;answer.fields[1]=.5;++answer.evaluations;
    if(!evaluator.position(.5,.5,answer.fields+2))return failed(BG_DISC_ARITHMETIC,&answer);
    double residual[3];for(unsigned k=0;k<3;++k){if(!numeric_finite(answer.fields[2+k]))return failed(BG_DISC_ARITHMETIC,&answer);
        residual[k]=answer.fields[2+k]-target[k];}
    answer.fields[5]=projection_detail::dot3(residual,residual);
    if(!numeric_finite(answer.fields[5])||answer.fields[5]<0||
       !witness_upper(original_net,p,q,.5,.5,target,answer.fields[7]))return failed(BG_DISC_ARITHMETIC,&answer);
    if(options.max_refinements){if(!refine(evaluator,original_net,p,q,target,initial,answer))return failed(BG_DISC_ARITHMETIC,&answer);}
    else answer.flags|=BG_DISC_REFINEMENT_LIMIT;
    TargetBounds prepared{};if(!prepare_target(target,mode,prepared))return failed(BG_DISC_ARITHMETIC,&answer);
    double root_lower;++answer.visits;
    if(!node_lower(nodes[root],root,bounds,mode,p,q,target,prepared,root_lower))return failed(BG_DISC_ARITHMETIC,&answer);
    uint32_t ids[13];double lowers[13];unsigned size=1;ids[0]=root;lowers[0]=root_lower;
    double least=finite_maximum();bool retained=false,capped=false;
    const uint32_t patch_slot=nodes[root].patch_slot;
    while(size){--size;const uint32_t id=ids[size];const double lower=lowers[size];
        if(id>=node_count)return failed(BG_DISC_ARITHMETIC,&answer);
        const bg_hierarchy_node node=nodes[id];
        if(node.patch_slot!=patch_slot||node.u_level>12||node.v_level>12||
           node.u_level+node.v_level>12 || node.u_index>=(1u<<node.u_level)||
           node.v_index>=(1u<<node.v_level)||node.flags>BG_HIERARCHY_LEAF)
            return failed(BG_DISC_ARITHMETIC,&answer);
        if(lower>answer.fields[7]){++answer.pruned;continue;}
        if(node.flags&BG_HIERARCHY_LEAF){
            if(node.left!=UINT32_MAX||node.right!=UINT32_MAX)return failed(BG_DISC_ARITHMETIC,&answer);
            retained=true;least=minimum(least,lower);
            if(id!=root){if(answer.refinements<options.max_refinements){
                const double seed[2]={center(node.u_index,node.u_level),center(node.v_index,node.v_level)};
                if(!refine(evaluator,original_net,p,q,target,seed,answer))return failed(BG_DISC_ARITHMETIC,&answer);
            }else answer.flags|=BG_DISC_REFINEMENT_LIMIT;}
        }else{
            if(node.left>=node_count||node.right>=node_count||node.left==node.right||
               node.left<=id||node.right<=id)return failed(BG_DISC_ARITHMETIC,&answer);
            if(options.max_visits-answer.visits<2){capped=true;answer.flags|=BG_DISC_VISIT_LIMIT;break;}
            if(size+2>13)return failed(BG_DISC_ARITHMETIC,&answer);
            double left,right;++answer.visits;
            if(!node_lower(nodes[node.left],node.left,bounds,mode,p,q,target,prepared,left))return failed(BG_DISC_ARITHMETIC,&answer);
            ++answer.visits;
            if(!node_lower(nodes[node.right],node.right,bounds,mode,p,q,target,prepared,right))return failed(BG_DISC_ARITHMETIC,&answer);
            const bool left_first=left<=right;
            ids[size]=left_first?node.right:node.left;lowers[size++]=left_first?right:left;
            ids[size]=left_first?node.left:node.right;lowers[size++]=left_first?left:right;
        }
    }
    if(!capped&&!retained)return failed(BG_DISC_ARITHMETIC,&answer);
    answer.fields[6]=capped?root_lower:minimum(answer.fields[7],least);
    if(!numeric_finite(answer.fields[6])||answer.fields[6]<0||answer.fields[6]>answer.fields[7])
        return failed(BG_DISC_ARITHMETIC,&answer);
    answer.flags|=BG_DISC_CANDIDATE_VALID;if(!capped)answer.flags|=BG_DISC_COVERAGE_COMPLETE;
    const double gap=up_sub(answer.fields[7],answer.fields[6]);
    if(!numeric_finite(gap)||gap<0)return failed(BG_DISC_ARITHMETIC,&answer);
    if(gap<=options.tolerance_squared){answer.flags|=BG_DISC_GAP;answer.status=BG_DISC_GAP_SATISFIED;}
    else answer.status=BG_DISC_UNRESOLVED;
    return answer;
}
}
#undef BG_DISC_INLINE
#endif
