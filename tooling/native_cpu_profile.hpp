#ifndef BREPGPU_NATIVE_CPU_PROFILE_HPP
#define BREPGPU_NATIVE_CPU_PROFILE_HPP
#include <version>
#if __cplusplus != 202002L
#error "Native CPU profile requires C++20 in the actual compilation unit"
#endif
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13
#error "Native CPU profile requires libstdc++13 in the actual compilation unit"
#endif
#endif
