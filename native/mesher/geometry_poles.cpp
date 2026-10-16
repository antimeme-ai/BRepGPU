#include "geometry_poles.hpp"
#include "geometry_enclosure.hpp"
#include <algorithm>
#include <cmath>
namespace bgmesh {
namespace {
using namespace geometry_detail;
using Axes=std::array<ExactEnclosure,3>;
[[noreturn]] void invalid(const char *m){throw Error(ErrorCode::invalid_source,m);}
[[noreturn]] void unresolved(const char *m){throw Error(ErrorCode::unresolved,m);}
ExactEnclosure divide(const ExactEnclosure &a,const ExactEnclosure &b) {
 if(b.lower.sign()>0)return quotient(a,b);
 if(b.upper.sign()<0)return quotient(negate(a),negate(b));
 unresolved("analytic pole denominator is not separated from zero");
}
Rational absolute(const Rational &x){return x.sign()<0?x.negative():Rational(x);}
Rational maximum(const Rational &a,const Rational &b){return a.compare(b)>0?Rational(a):Rational(b);}
Rational minimum(const Rational &a,const Rational &b){return a.compare(b)<0?Rational(a):Rational(b);}
std::array<Rational,3> axis(bgfd::Budget &b,const Frame &frame,std::size_t offset) {
 return {Rational::from_double(b,frame[offset]),Rational::from_double(b,frame[offset+1]),Rational::from_double(b,frame[offset+2])};
}
void independent(bgfd::Budget &b,const Frame &frame) {
 const auto x=axis(b,frame,3),y=axis(b,frame,6),z=axis(b,frame,9);
 const auto det=x[0].multiply(y[1].multiply(z[2]).subtract(y[2].multiply(z[1])))
 .subtract(x[1].multiply(y[0].multiply(z[2]).subtract(y[2].multiply(z[0]))))
 .add(x[2].multiply(y[0].multiply(z[1]).subtract(y[1].multiply(z[0]))));
 if(det.sign()==0)throw Error(ErrorCode::unsupported,"analytic pole requires independent original frame axes");
}
void nonzero_facet(bgfd::Budget &b,const std::array<double,9> &xyz) {
 std::array<Rational,3> x{Rational(b),Rational(b),Rational(b)},y{Rational(b),Rational(b),Rational(b)};
 for(std::size_t a=0;a<3;++a) {
  const auto origin=Rational::from_double(b,xyz[a]);
  x[a]=Rational::from_double(b,xyz[3+a]).subtract(origin);y[a]=Rational::from_double(b,xyz[6+a]).subtract(origin);
 }
 bool nonzero=false;
 for(std::size_t a=0;a<3;++a) {
  const auto j=(a+1)%3,k=(a+2)%3;
  if(x[j].multiply(y[k]).subtract(x[k].multiply(y[j])).sign()!=0)nonzero=true;
 }
 if(!nonzero)invalid("analytic pole coverage requires a nonzero actual physical facet");
}
Rational departure(bgfd::Budget &b,const Axes &point,std::span<const double,3> actual,uint32_t bits) {
 const Axes residual{difference(point[0],ExactEnclosure(Rational::from_double(b,actual[0]))),
 difference(point[1],ExactEnclosure(Rational::from_double(b,actual[1]))),
 difference(point[2],ExactEnclosure(Rational::from_double(b,actual[2])))};
 return enclosure_norm_upper(b,residual,bits);
}
}
PoleFanPlan bound_analytic_pole_fan(GeometryContext &ctx,const Carrier &source,const PoleFanRequest &request) {
 auto &b=ctx.budget();
 if((request.kind==AnalyticPoleKind::cone_apex?source.kind!=3:source.kind!=4)||
  (request.face_orientation!=1&&request.face_orientation!=-1))invalid("analytic pole kind/source orientation mismatch");
 for(const auto value:source.frame)if(!std::isfinite(value))invalid("nonfinite original pole frame");
 for(const auto value:source.radii)if(!std::isfinite(value))invalid("nonfinite original pole radii");
 for(const auto value:request.actual_xyz)if(!std::isfinite(value))invalid("nonfinite actual pole facet");
 for(const auto &value:request.cut_uv)if(&value.budget()!=&b)invalid("pole cut owner mismatch");
 for(const auto &value:request.original_pole_v)if(&value.budget()!=&b)invalid("pole band owner mismatch");
 if(request.precision.transcendental_bits<32||request.precision.transcendental_bits>ctx.maximum_bits()||
  request.precision.norm_fractional_bits>ctx.maximum_bits())invalid("pole precision profile invalid");
 independent(b,source.frame);nonzero_facet(b,request.actual_xyz);
 const auto &cut=request.cut_uv;const auto du=cut[2].subtract(cut[0]),dg=cut[3].subtract(cut[1]);
 if(du.sign()<=0||request.original_pole_v[0].compare(request.original_pole_v[1])>0)invalid("pole source intervals reversed or empty");
 const auto &pi=ctx.pi(request.precision.transcendental_bits+64);
 const auto half=Rational(b,1).divide(Rational(b,2));
 if(du.compare(pi.lower.multiply(half))>0)throw Error(ErrorCode::unsupported,"pole angular piece exceeds pi/2");
 const auto radius=Rational::from_double(b,source.radii[0]);
 const auto pole=[&]() -> ExactEnclosure {
  if(request.kind==AnalyticPoleKind::sphere_north)return times(pi,half);
  if(request.kind==AnalyticPoleKind::sphere_south)return times(pi,half.negative());
  const auto angle=Rational::from_double(b,source.radii[1]);const auto trig=trig_range(ctx,angle,angle,request.precision.transcendental_bits);
  if(trig.sine.lower.sign()==0&&trig.sine.upper.sign()==0)
   throw Error(ErrorCode::unsupported,"zero-angle cone has no isolated apex");
  return divide(ExactEnclosure(radius.negative()),trig.sine);
 }();
 const int side=cut[1].compare(pole.upper)>0&&cut[3].compare(pole.upper)>0?1:
  cut[1].compare(pole.lower)<0&&cut[3].compare(pole.lower)<0?-1:0;
 if(side==0)unresolved("pole cut does not stay strictly on one regular side");
 if(source.kind==4) {
  if(radius.sign()==0)throw Error(ErrorCode::unsupported,"zero sphere radius has no regular pole cap");
  const auto width=maximum(maximum(absolute(cut[1].subtract(pole.lower)),absolute(cut[1].subtract(pole.upper))),
   maximum(absolute(cut[3].subtract(pole.lower)),absolute(cut[3].subtract(pole.upper))));
  if(width.compare(pi.lower.multiply(half))>=0)throw Error(ErrorCode::unsupported,"sphere pole cut requires a single regular hemispherical side");
 }
 const auto origin=axis(b,source.frame,0),z=axis(b,source.frame,9);
 const auto ideal=[&]() -> Axes {
  if(source.kind==4) {
   const auto sign=Rational(b,request.kind==AnalyticPoleKind::sphere_north?1:-1);
   return {ExactEnclosure(origin[0].add(z[0].multiply(radius).multiply(sign))),
    ExactEnclosure(origin[1].add(z[1].multiply(radius).multiply(sign))),
    ExactEnclosure(origin[2].add(z[2].multiply(radius).multiply(sign)))};
  }
  const auto angle=Rational::from_double(b,source.radii[1]);const auto trig=trig_range(ctx,angle,angle,request.precision.transcendental_bits);
  const auto height=product(pole,trig.cosine);
  return {sum(ExactEnclosure(origin[0]),times(height,z[0])),sum(ExactEnclosure(origin[1]),times(height,z[1])),sum(ExactEnclosure(origin[2]),times(height,z[2]))};
 }();
 const auto low=minimum(minimum(cut[1],cut[3]),pole.lower),high=maximum(maximum(cut[1],cut[3]),pole.upper);
 const auto jet=enclose_analytic_surface_jet(ctx,source,{cut[0],cut[2],low,high},request.precision.transcendental_bits);
 const auto rho_dg=ExactEnclosure(minimum(Rational(b,0),dg),maximum(Rational(b,0),dg));
 const auto height=difference(ExactEnclosure(minimum(cut[1],cut[3]),maximum(cut[1],cut[3])),pole);
 const auto du2=du.multiply(du),two_du=du.multiply(Rational(b,2));
 const auto xx=[&](std::size_t a){return sum(sum(times(jet.duu[a],du2),times(product(jet.duv[a],rho_dg),two_du)),product(jet.dvv[a],squared(rho_dg)));};
 const Axes dxx{xx(0),xx(1),xx(2)},drr{product(jet.dvv[0],squared(height)),product(jet.dvv[1],squared(height)),product(jet.dvv[2],squared(height))};
 const auto bits=request.precision.norm_fractional_bits;
 auto corners=departure(b,ideal,std::span<const double,3>(request.actual_xyz.data(),3),bits);
 for(std::size_t i=0;i<2;++i) {
  const auto point=enclose_analytic_surface_jet(ctx,source,{cut[2*i],cut[2*i],cut[2*i+1],cut[2*i+1]},request.precision.transcendental_bits);
  corners=maximum(corners,departure(b,point.position,std::span<const double,3>(request.actual_xyz.data()+3+3*i,3),bits));
 }
 const auto position=enclosure_norm_upper(b,dxx,bits).add(enclosure_norm_upper(b,drr,bits)).divide(Rational(b,8)).add(corners);
 const auto &v=request.original_pole_v;
 const auto distance=maximum(maximum(absolute(v[0].subtract(pole.lower)),absolute(v[0].subtract(pole.upper))),
  maximum(absolute(v[1].subtract(pole.lower)),absolute(v[1].subtract(pole.upper))));
 const auto band_jet=enclose_analytic_surface_jet(ctx,source,{cut[0],cut[2],minimum(v[0],pole.lower),maximum(v[1],pole.upper)},request.precision.transcendental_bits);
 const auto band=enclosure_norm_upper(b,band_jet.dv,bits).multiply(distance);
 const auto roles=-request.face_orientation*side==1?std::array<uint32_t,3>{0,1,2}:std::array<uint32_t,3>{0,2,1};
 return {pole,position,band,roles};
}
} // namespace bgmesh
