#ifndef BREPGPU_MESHER_GEOMETRY_PROFILE_HPP
#define BREPGPU_MESHER_GEOMETRY_PROFILE_HPP
#include "geometry.hpp"
namespace bgmesh::geometry_detail {
struct CoefficientChoice final {uint32_t degree,index;};
inline uint64_t coefficient_choose(CoefficientChoice value) {
  if(value.index>value.degree)return 0;
  if(value.degree>38)throw Error(ErrorCode::unsupported,"finite Bernstein coefficient degree exceeded");
  uint64_t result=1;
  // Divide each step before the next multiplication; our callers use degree<=38.
  for(uint32_t i=1;i<=value.index;++i)result=result*(value.degree-value.index+i)/i;
  return result;
}
inline bool ratio_increasing(std::span<const Rational> a,std::span<const Rational> w) {
  if(a.size()<2||a.size()!=w.size()||a.size()>33)return false;
  auto &budget=a[0].budget();
  for(std::size_t i=0;i<a.size();++i)
    if(&a[i].budget()!=&budget||&w[i].budget()!=&budget||w[i].sign()<=0)
      throw Error(ErrorCode::invalid_source,"invalid monotone rational Bernstein profile");
  const auto n=static_cast<uint32_t>(a.size()-1);
  bool positive=false;
  for(uint32_t k=0;k<2*n;++k) {
    Rational coefficient(budget,0);
    for(uint32_t i=0;i<n;++i) {
      if(k<i||k-i>n)continue;
      const auto j=k-i;
      const auto term=a[i+1].subtract(a[i]).multiply(w[j])
          .subtract(w[i+1].subtract(w[i]).multiply(a[j]));
      const auto factor=coefficient_choose({n-1,i})*coefficient_choose({n,j});
      coefficient=coefficient.add(term.multiply(Rational(budget,static_cast<int64_t>(factor))));
    }
    if(coefficient.sign()<0)return false;
    positive=positive||coefficient.sign()>0;
  }
  return positive;
}
} // namespace bgmesh::geometry_detail
#endif
