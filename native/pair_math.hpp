#ifndef BREPGPU_PAIR_MATH_HPP
#define BREPGPU_PAIR_MATH_HPP
#include "brepgpu_pairs.h"
#include <cstring>
#ifdef __CUDACC__
#define BG_PAIR_HD __host__ __device__
#else
#define BG_PAIR_HD
#endif
namespace brepgpu { namespace pair_detail {
BG_PAIR_HD inline uint64_t bits(double value) noexcept {
#ifdef __CUDA_ARCH__
    return static_cast<uint64_t>(__double_as_longlong(value));
#else
    uint64_t result;std::memcpy(&result,&value,sizeof(result));return result;
#endif
}
BG_PAIR_HD inline bool finite(double value) noexcept {
    return (bits(value)&0x7ff0000000000000ull)!=0x7ff0000000000000ull;
}
BG_PAIR_HD inline uint64_t key(double value) noexcept {
    uint64_t b=bits(value);
    if((b&0x7fffffffffffffffull)==0)b=0;
    return b&0x8000000000000000ull ? ~b : b^0x8000000000000000ull;
}
BG_PAIR_HD inline bool valid(const bg_pair_box& box) noexcept {
    for(unsigned a=0;a<3;++a)
        if(!finite(box.lower[a])||!finite(box.upper[a])||key(box.lower[a])>key(box.upper[a]))return false;
    return true;
}
BG_PAIR_HD inline bool overlap(const bg_pair_box& l,const bg_pair_box& r) noexcept {
    for(unsigned a=0;a<3;++a)
        if(key(l.lower[a])>key(r.upper[a])||key(r.lower[a])>key(l.upper[a]))return false;
    return true;
}
BG_PAIR_HD inline uint32_t word(const bg_pair_box* left,const bg_pair_box* right,
    uint64_t right_count,uint64_t start,uint64_t count,uint64_t word_index) noexcept {
    uint32_t mask=0;const uint64_t first=word_index*32;
    for(unsigned bit=0;bit<32 && first+bit<count;++bit){
        const uint64_t q=start+first+bit;
        if(overlap(left[q/right_count],right[q%right_count]))mask|=uint32_t(1)<<bit;
    }
    return mask;
}
}}
#undef BG_PAIR_HD
#endif
