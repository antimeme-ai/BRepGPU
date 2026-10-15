#ifndef BG_REFERENCE_GEOMETRY_CLOSEST_HPP
#define BG_REFERENCE_GEOMETRY_CLOSEST_HPP
#include <array>
#include <cmath>
#include <limits>
#include <stdexcept>
namespace refgeom {
using Point=std::array<long double,3>;
inline Point sub(Point a,Point b){return {a[0]-b[0],a[1]-b[1],a[2]-b[2]};}
inline long double dot(Point a,Point b){return (a[0]*b[0]+a[1]*b[1])+a[2]*b[2];}
inline Point cross(Point a,Point b){return {a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]};}
inline bool finite(Point p){return std::isfinite(p[0])&&std::isfinite(p[1])&&std::isfinite(p[2]);}
inline void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
struct Closest {long double squared=std::numeric_limits<long double>::infinity();Point bary{},point{};};
struct Triangle {
  std::array<Point,3> p;
  Point e,f,n;
  long double denominator;
  explicit Triangle(std::array<Point,3> points):p(points),e(sub(p[1],p[0])),f(sub(p[2],p[0])),n(cross(e,f)),denominator(dot(n,n)){
    check(finite(e)&&finite(f)&&finite(n)&&std::isfinite(denominator)&&denominator>0,"reverse: invalid triangle arithmetic");
  }
  Closest nearest(Point query) const {
    Closest best;
    auto candidate=[&](Point bary){
      check(finite(bary)&&bary[0]>=0&&bary[1]>=0&&bary[2]>=0,"reverse: infeasible barycentric result");
      Point value{};
      for(int k=0;k<3;++k)value[k]=(bary[0]*p[0][k]+bary[1]*p[1][k])+bary[2]*p[2][k];
      Point delta=sub(query,value);long double squared=dot(delta,delta);
      check(finite(value)&&finite(delta)&&std::isfinite(squared)&&squared>=0,"reverse: nonfinite closest point/distance");
      if(squared<best.squared)best={squared,bary,value};
    };
    Point w=sub(query,p[0]);
    long double u=dot(cross(w,f),n)/denominator,v=dot(cross(e,w),n)/denominator;
    check(finite(w)&&std::isfinite(u)&&std::isfinite(v),"reverse: nonfinite plane projection");
    if(u>=0&&v>=0&&u+v<=1)candidate({1-(u+v),u,v});
    for(int i=0;i<3;++i){
      int j=(i+1)%3;Point edge=sub(p[j],p[i]);long double length=dot(edge,edge);
      check(std::isfinite(length)&&length>0,"reverse: invalid segment arithmetic");
      long double t=dot(sub(query,p[i]),edge)/length;
      check(std::isfinite(t),"reverse: nonfinite segment projection");
      t=t<0?0:(t>1?1:t);Point bary{};bary[i]=1-t;bary[j]=t;candidate(bary);
    }
    return best;
  }
};
} // namespace refgeom
#endif
