#include "discovery_cpu.h"
#include "discovery_math.hpp"
#include <cfloat>
#include <cfenv>
#include <limits>
#if defined(__linux__) && defined(__x86_64__)
#include <xmmintrin.h>
#endif
namespace brepgpu {
bool discovery_environment_supported() noexcept {
#if defined(__linux__) && defined(__x86_64__) && FLT_EVAL_METHOD == 0 && !defined(__FAST_MATH__)
    if(sizeof(double)!=8 || std::numeric_limits<double>::radix!=2 ||
       std::numeric_limits<double>::digits!=53 || !std::numeric_limits<double>::is_iec559 ||
       std::fegetround()!=FE_TONEAREST)return false;
    const uint32_t mxcsr=_mm_getcsr();
    if((mxcsr&0x6000u)!=0 || (mxcsr&0x8040u)!=0 || (mxcsr&0x1f80u)!=0x1f80u)return false;
    unsigned short control=0;
    __asm__ __volatile__("fnstcw %0" : "=m"(control));
    return (control&0x0c00u)==0 && (control&0x003fu)==0x003fu;
#else
    return false;
#endif
}
namespace {
using namespace discovery_detail;
struct Net { Interval c[16][4]; };
unsigned choose(unsigned n,unsigned k) noexcept {
    if(k>n)return 0;
    unsigned value=1;
    for(unsigned i=1;i<=k;++i)value=value*(n+1-i)/i;
    return value;
}
bool clipped_control(Interval* c) noexcept {
    for(unsigned a=0;a<3;++a)if(!intersect(c[a],-256,256))return false;
    return intersect(c[3],1,256);
}
bool split_net(const Net& original,unsigned p,unsigned q,unsigned axis,Net& left,Net& right) noexcept {
    left=original;right=original;
    const unsigned degree=axis==0?p:q,other=axis==0?q:p;
    for(unsigned j=0;j<=other;++j){Interval line[4][4];
        for(unsigned i=0;i<=degree;++i){const unsigned index=axis==0?i*(q+1)+j:j*(q+1)+i;
            for(unsigned a=0;a<4;++a)line[i][a]=original.c[index][a];}
        for(unsigned r=1;r<=degree;++r){
            for(unsigned i=0;i<=degree-r;++i){
                for(unsigned a=0;a<4;++a)line[i][a]=midpoint(line[i][a],line[i+1][a]);
                if(!clipped_control(line[i]))return false;}
            const unsigned li=axis==0?r*(q+1)+j:j*(q+1)+r;
            const unsigned ri=axis==0?(degree-r)*(q+1)+j:j*(q+1)+degree-r;
            for(unsigned a=0;a<4;++a){left.c[li][a]=line[0][a];right.c[ri][a]=line[degree-r][a];}
        }
    }
    return true;
}
struct Builder {
    const bg_hierarchy_options& options;
    std::vector<bg_hierarchy_node>& nodes;
    std::vector<double>& bounds;
    unsigned p,q;uint32_t slot;
    bool box(const Net& net,bg_hierarchy_node& node,uint32_t parent) noexcept {
        for(unsigned a=0;a<3;++a){Interval range{1,-1};
            for(unsigned i=0;i<(p+1)*(q+1);++i){Interval point=divide_positive(net.c[i][a],net.c[i][3]);
                if(!intersect(point,-1,1))return false;
                if(i==0)range=point;else{range.lo=minimum(range.lo,point.lo);range.hi=maximum(range.hi,point.hi);}}
            if(parent!=UINT32_MAX&&!intersect(range,nodes[parent].lower[a],nodes[parent].upper[a]))return false;
            node.lower[a]=range.lo;node.upper[a]=range.hi;}
        return true;
    }
    bool dop(const Net& net,uint32_t id,uint32_t parent) noexcept {
        for(unsigned n=0;n<10;++n){Interval support{};
            for(unsigned i=0;i<(p+1)*(q+1);++i){Interval numerator=projection(net.c[i],n);
                Interval projected=divide_positive(numerator,net.c[i][3]);
                const double limit=n<6?2:3;
                if(!intersect(projected,-limit,limit))return false;
                if(i==0)support=projected;
                else{support.lo=minimum(support.lo,projected.lo);support.hi=maximum(support.hi,projected.hi);}}
            if(parent!=UINT32_MAX){const uint64_t at=uint64_t(parent)*20+2*n;
                if(!intersect(support,bounds[at],bounds[at+1]))return false;}
            const uint64_t at=uint64_t(id)*20+2*n;bounds[at]=support.lo;bounds[at+1]=support.hi;}
        return true;
    }
    bool bernstein(const Net& net,uint32_t id) noexcept {
        for(unsigned r=0;r<=2*p;++r)for(unsigned s=0;s<=2*q;++s){
            Interval coefficient[5]{};bool started=false;
            for(unsigned i=0;i<=p;++i){if(r<i||r-i>p)continue;const unsigned k=r-i;
                for(unsigned j=0;j<=q;++j){if(s<j||s-j>q)continue;const unsigned l=s-j;
                    const unsigned numerator=choose(p,i)*choose(p,k)*choose(q,j)*choose(q,l);
                    const unsigned denominator=choose(2*p,r)*choose(2*q,s);
                    const Interval factor{down_div(numerator,denominator),up_div(numerator,denominator)};
                    if(!valid(factor)||factor.lo<=0)return false;
                    const Interval* a=net.c[i*(q+1)+j];const Interval* b=net.c[k*(q+1)+l];
                    Interval term[5];
                    term[0]=add(add(multiply(a[0],b[0]),multiply(a[1],b[1])),multiply(a[2],b[2]));
                    for(unsigned c=0;c<3;++c)term[c+1]=multiply(a[c],b[3]);
                    term[4]=multiply(a[3],b[3]);
                    for(unsigned c=0;c<5;++c){term[c]=multiply(factor,term[c]);
                        if(!valid(term[c]))return false;
                        coefficient[c]=started?add(coefficient[c],term[c]):term[c];}
                    started=true;
                }
            }
            if(!started)return false;
            for(unsigned c=0;c<5;++c){const double limit=c==0?196608:65536;
                if(!intersect(coefficient[c],c==4?1:-limit,limit))return false;
                const uint64_t at=uint64_t(id)*490+c*98+2*(r*(2*q+1)+s);
                bounds[at]=coefficient[c].lo;bounds[at+1]=coefficient[c].hi;}
        }
        return true;
    }
    bool append(const Net& net,unsigned depth,uint32_t u,uint32_t v,uint32_t ul,uint32_t vl,
                uint32_t parent,uint32_t& id) {
        if(nodes.size()>=options.max_nodes || nodes.size()>=UINT32_MAX)return false;
        id=static_cast<uint32_t>(nodes.size());
        bg_hierarchy_node node{};node.left=UINT32_MAX;node.right=UINT32_MAX;node.patch_slot=slot;
        node.u_index=u;node.v_index=v;node.u_level=ul;node.v_level=vl;
        const bool leaf=depth==options.depth || (p==0&&q==0);node.flags=leaf?BG_HIERARCHY_LEAF:0;
        if(!box(net,node,parent))return false;
        nodes.push_back(node);
        const unsigned stride=options.flags==BG_BOUND_AABB?0:(options.flags==BG_BOUND_DOP26?20:490);
        if(stride){
            if(nodes.size()>bounds.max_size()/stride)return false;
            bounds.resize(nodes.size()*stride,0);
            if(options.flags==BG_BOUND_DOP26){if(!dop(net,id,parent))return false;}
            else if(!bernstein(net,id))return false;
        }
        if(leaf)return true;
        const unsigned axis=p==0?1:(q==0?0:depth%2);
        Net left{},right{};if(!split_net(net,p,q,axis,left,right))return false;
        uint32_t left_id,right_id;
        if(!append(left,depth+1,axis==0?2*u:u,axis==1?2*v:v,ul+(axis==0),vl+(axis==1),id,left_id))return false;
        if(!append(right,depth+1,axis==0?2*u+1:u,axis==1?2*v+1:v,ul+(axis==0),vl+(axis==1),id,right_id))return false;
        nodes[id].left=left_id;nodes[id].right=right_id;return true;
    }
};
struct ScalarCPU {
    const bg_patch_desc* patch;const double* controls;
    bool evaluate(double s,double t,uint32_t order,double* out) noexcept {
        const double uv[2]={s,t};const uint32_t slot=0;uint32_t status=BG_QUERY_ARITHMETIC;
        evaluate_cpu_range(patch,1,controls,&slot,uv,1,order,0,1,out,&status);
        return status==BG_QUERY_OK;
    }
    bool full(double s,double t,double* out) noexcept {return evaluate(s,t,2,out);}
    bool position(double s,double t,double* out) noexcept {return evaluate(s,t,0,out);}
};
}
bool build_discovery_hierarchy(const bg_patch_desc* patches,uint64_t patch_count,
    const double* controls,const bg_hierarchy_options& options,
    std::vector<bg_hierarchy_node>& nodes,std::vector<uint32_t>& roots,std::vector<double>& bounds) {
    nodes.clear();roots.clear();bounds.clear();
    if(options.depth>BG_HIERARCHY_MAX_DEPTH||options.flags>BG_BOUND_BERNSTEIN||
       options.max_nodes>BG_HIERARCHY_MAX_NODES||options.max_nodes==0||patch_count>options.max_nodes)return false;
    if(patch_count>roots.max_size())return false;
    roots.resize(static_cast<size_t>(patch_count));
    for(uint64_t slot=0;slot<patch_count;++slot){const bg_patch_desc& patch=patches[slot];
        if(patch.degree_u>3||patch.degree_v>3||patch.control_count!=(patch.degree_u+1)*(patch.degree_v+1))return false;
        Net net{};const double* input=controls+patch.control_offset*4;
        for(unsigned i=0;i<patch.control_count;++i){
            const double w=input[4*i+3];if(!numeric_finite(w)||w<1||w>256)return false;
            for(unsigned a=0;a<4;++a){const double value=input[4*i+a];
                if(!numeric_finite(value)||(a<3&&projection_detail::magnitude(value)>w))return false;
                net.c[i][a]={value,value};}
        }
        Builder builder{options,nodes,bounds,patch.degree_u,patch.degree_v,static_cast<uint32_t>(slot)};
        if(!builder.append(net,0,0,0,0,0,UINT32_MAX,roots[slot]))return false;
    }
    return true;
}
void discover_cpu_range(const bg_patch_desc* patches,uint64_t patch_count,
    const double* controls,const bg_hierarchy_node* nodes,uint64_t node_count,
    const uint32_t* roots,const double* bounds,uint32_t mode,const uint32_t* slots,
    const double* targets,uint64_t count,const bg_discovery_options* options,
    uint64_t begin,uint64_t end,double* fields,uint32_t* statuses,uint32_t* flags,
    uint32_t* visits,uint32_t* pruned,uint32_t* refinements,uint32_t* evaluations) noexcept {
    for(uint64_t i=begin;i<end;++i){DiscoveryCandidate answer;
        if(slots[i]>=patch_count)answer=discovery_detail::failed(BG_DISC_BAD_PATCH);
        else{const bg_patch_desc& patch=patches[slots[i]];const uint32_t root=roots[slots[i]];
            if(root>=node_count||nodes[root].patch_slot!=slots[i])answer=discovery_detail::failed(BG_DISC_ARITHMETIC);
            else{ScalarCPU evaluator{&patch,controls};answer=discover_local(evaluator,
                controls+patch.control_offset*4,patch.degree_u,patch.degree_v,nodes,node_count,
                root,bounds,mode,targets+3*i,*options);}}
        for(unsigned k=0;k<8;++k)fields[k*count+i]=answer.fields[k];
        statuses[i]=answer.status;flags[i]=answer.flags;visits[i]=answer.visits;
        pruned[i]=answer.pruned;refinements[i]=answer.refinements;evaluations[i]=answer.evaluations;
    }
}
}
