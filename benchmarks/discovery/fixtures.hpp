#ifndef BREPGPU_DISCOVERY_FIXTURES_HPP
#define BREPGPU_DISCOVERY_FIXTURES_HPP
#include "brepgpu_discovery.h"
#include <array>
#include <string>
#include <vector>
#include <stdexcept>

namespace discovery_benchmark {
// All controls and targets are dyadic. Each admitted true minimum below is
// dyadic with fewer than 53 significant bits; no epsilon qualifies a bracket.
struct Fixture {
    std::string corpus;
    bg_patch_desc patch{};
    std::vector<double> controls,targets,truth;
    std::vector<uint32_t> slots;
    explicit Fixture(uint64_t n,const std::string& name):corpus(name),targets(3*n),truth(n),slots(n,0) {
        patch.abi_version=BG_ABI_VERSION;patch.struct_size=sizeof patch;
        patch.flags=BG_PATCH_LINEAGE|BG_PATCH_FRAME;patch.body_id=1;
        patch.entity_id=1;patch.revision=1;patch.chart_id=1;patch.scale=1;
        patch.uv[1]=patch.uv[3]=1;
        const bool cylinder=name=="cylinder"||name=="axis"||name=="axis_midpoint"||name=="capped";
        const bool parabola=name=="parabolic"||name=="twins";
        if(!cylinder&&!parabola&&name!="plane"&&name!="diagonal"&&name!="diagonal3")
            throw std::runtime_error("unknown discovery corpus");
        patch.degree_u=cylinder||parabola?2:1;patch.degree_v=1;
        patch.control_count=(patch.degree_u+1)*2;
        for(unsigned i=0;i<=patch.degree_u;++i)for(unsigned j=0;j<2;++j) {
            double x=i,y=j,z=0,w=1;
            if(cylinder){const double a[]={1,1,0},b[]={0,1,2},c[]={1,1,2};x=a[i];y=b[i];w=c[i];z=j*w;}
            else if(parabola){x=double(i)-1;z=i==1?-1:1;}
            else if(name=="diagonal"){y=i;z=j;}
            else if(name=="diagonal3"){x=double(i)-.5;y=double(j)-.5;z=1.-i-j;}
            controls.insert(controls.end(),{x,y,z,w});
        }
        for(uint64_t i=0;i<n;++i) {
            const double s=(1.+i%15)/16,t=(1.+(i*7)%15)/16;
            double x=s,y=t,z=.5,d=.25;
            if(cylinder) {
                if(name=="axis"||name=="axis_midpoint"){x=y=0;z=name=="axis_midpoint"?.5:t;d=1;}
                else {const double lambda=1.+(i%4)/8.;x=lambda*(1-s*s);y=2*lambda*s;z=t;
                    // ||X_xy||=lambda*(1+s²) exactly, and its angle is on
                    // the admitted quarter-circle. True radial gap is r-1.
                    const double r=lambda*(1+s*s);d=(r-1)*(r-1);}
            } else if(parabola) {
                if(name=="twins"){x=0;y=t;z=.75;d=.5;}
                else {const double a[]={-.5,-.25,.25,.5},v=a[i%4],lambda=1./16;
                    x=v-2*v*lambda;y=t;z=v*v+lambda;d=lambda*lambda*(4*v*v+1);}
            } else if(name=="diagonal"){x=s+.25;y=s-.25;z=t;d=.125;}
            else if(name=="diagonal3"){x=y=z=.25;d=.1875;}
            else {z=i%2?.5:-.5;}
            targets[3*i]=x;targets[3*i+1]=y;targets[3*i+2]=z;truth[i]=d;
        }
    }
};
}
#endif
