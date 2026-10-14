#ifndef BREPGPU_PROJECTION_BENCHMARK_FIXTURES_HPP
#define BREPGPU_PROJECTION_BENCHMARK_FIXTURES_HPP
#include "brepgpu.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

// Owned analytic carriers. Truth uses expanded maps, not Bernstein, de Casteljau
// or an iterative reference optimizer. All controls/targets/seeds are dyadic.
namespace projection_benchmark {
enum Shape { Plane, Tilted, Cylinder, Parabolic, Constant, Cubic, WeakCorner };
struct Geometry { std::array<long double,3> point{}, ds{}, dt{}; };
inline Geometry geometry(unsigned shape,long double s,long double t) {
    Geometry g; const long double a=2*s-1,d=1+s*s;
    switch(shape) {
    case Plane:g.point={s,t,0};g.ds={1,0,0};g.dt={0,1,0};break;
    case Tilted:g.point={s,t,(s+2*t)/4};g.ds={1,0,.25L};g.dt={0,1,.5L};break;
    case Cylinder:g.point={(1-s*s)/d,2*s/d,t};g.ds={-4*s/(d*d),2*(1-s*s)/(d*d),0};g.dt={0,0,1};break;
    case Parabolic:g.point={a,t,a*a};g.ds={2,0,4*a};g.dt={0,1,0};break;
    case Constant:g.point={.25L,-.5L,.75L};break;
    case Cubic:g.point={a*a*a,t,0};g.ds={6*a*a,0,0};g.dt={0,1,0};break;
    case WeakCorner:g.point={s,t,s*t};g.ds={1,0,t};g.dt={0,1,s};break;
    default:throw std::runtime_error("unknown analytic carrier");
    }
    return g;
}
struct Expected {
    unsigned shape=Plane;
    bool known_uv=true;
    double s=0,t=0;
    // Canonical public statuses and flags are assigned by the benchmark after
    // header handoff; these labels describe analytic cases, not ABI values.
    enum Outcome { Solved, BadSlot, BadParameter, Rank, Budget, Search } outcome=Solved;
    enum Curvature { Positive, Negative, Ambiguous, Unclassified } curvature=Positive;
};
struct Fixture {
    uint64_t n;
    std::string corpus;
    std::vector<bg_patch_desc> patches;
    std::vector<double> controls,targets,seeds;
    std::vector<uint32_t> slots;
    std::vector<Expected> expected;
    Fixture(uint64_t count,std::string name):n(count),corpus(std::move(name)),targets(3*count),seeds(2*count),slots(count),expected(count) {
        auto patch=[&](unsigned p,unsigned q,const std::vector<double>& c) {
            bg_patch_desc desc{};desc.abi_version=BG_ABI_VERSION;desc.struct_size=sizeof desc;
            desc.degree_u=p;desc.degree_v=q;desc.control_offset=controls.size()/4;desc.control_count=c.size()/4;
            desc.flags=BG_PATCH_FRAME;desc.scale=1;desc.uv[1]=desc.uv[3]=1;patches.push_back(desc);
            controls.insert(controls.end(),c.begin(),c.end());
        };
        for(unsigned shape=Plane;shape<=WeakCorner;++shape) {
            std::vector<double> c;
            if(shape==Constant)c={.25,-.5,.75,1};
            else {
                unsigned p=(shape==Cylinder||shape==Parabolic)?2:shape==Cubic?3:1;
                for(unsigned i=0;i<=p;++i)for(unsigned j=0;j<2;++j) {
                    double x=0,y=0,z=0,w=1;
                    if(shape==Cylinder) {
                        const double cx[]={1,1,0},cy[]={0,1,2},cw[]={1,1,2};
                        x=cx[i];y=cy[i];w=cw[i];z=j*w;
                    } else if(shape==Parabolic) {x=static_cast<double>(i)-1;y=j;z=i==1?-1:1;}
                    else if(shape==Cubic) {x=i%2?1:-1;y=j;}
                    else {x=i;y=j;if(shape==Tilted)z=(i+2.*j)/4;else if(shape==WeakCorner)z=i*j;}
                    c.insert(c.end(),{x,y,z,w});
                }
                patch(p,1,c);continue;
            }
            patch(0,0,c);
        }
        if(corpus!="plane"&&corpus!="tilted"&&corpus!="cylinder"&&corpus!="parabolic"&&corpus!="mixed"&&corpus!="adverse")
            throw std::runtime_error("unknown projection corpus");
        for(uint64_t i=0;i<n;++i) {
            auto& e=expected[i];
            unsigned shape=corpus=="mixed"?i%4:corpus=="tilted"?Tilted:corpus=="cylinder"?Cylinder:corpus=="parabolic"?Parabolic:Plane;
            double s=(1.+i%15)/16,t=(1.+(i*7)%15)/16;
            double x=0,y=0,z=0,ss=(i%2?1:0),st=(i%3?0:1);
            if(corpus=="adverse") {
                e.known_uv=false;e.curvature=Expected::Unclassified;
                switch(i%8) {
                case 0:shape=Plane;e.outcome=Expected::Budget;x=.25;y=.75;z=.5;ss=.875;st=.125;break;
                case 1:shape=Cubic;e.outcome=Expected::Search;x=std::ldexp(1.,-10);y=.5;ss=33./64;st=.5;break;
                case 2:shape=Constant;e.outcome=Expected::Rank;x=.5;y=.25;z=.25;ss=.5;st=.5;break;
                case 3:shape=Plane;e.outcome=Expected::BadSlot;slots[i]=999;x=std::numeric_limits<double>::quiet_NaN();break;
                case 4:shape=Plane;e.outcome=Expected::BadParameter;x=5;break;
                case 5:shape=Parabolic;e.outcome=Expected::Solved;e.curvature=Expected::Negative;x=0;y=.5;z=.75;ss=.5;st=.5;break;
                case 6:shape=Cylinder;e.outcome=Expected::Solved;e.curvature=Expected::Ambiguous;x=0;y=0;z=.5;ss=.5;st=.5;break;
                case 7:shape=WeakCorner;e.outcome=Expected::Solved;e.curvature=Expected::Ambiguous;x=0;y=0;z=-2;ss=0;st=0;break;
                }
            } else if(shape==Plane) {x=s;y=t;z=(i%2?.5:-.5);}
            else if(shape==Tilted) {s=0;t=(1.+i%7)/8;x=-.25;y=t;z=t/2;}
            else if(shape==Cylinder) {const double lambda=1.+(i%4)/8.;x=lambda*(1-s*s);y=2*lambda*s;z=t;}
            else {const double aa[]={-.5,-.25,.25,.5},a=aa[i%4],lambda=1./16;s=(a+1)/2;x=a-2*a*lambda;y=t;z=a*a+lambda;}
            e.shape=shape;e.s=s;e.t=t;
            if(!(corpus=="adverse"&&i%8==3))slots[i]=shape;
            targets[3*i]=x;targets[3*i+1]=y;targets[3*i+2]=z;seeds[2*i]=ss;seeds[2*i+1]=st;
        }
    }
};
}
#endif
