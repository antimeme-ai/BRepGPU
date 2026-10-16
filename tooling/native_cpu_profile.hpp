#ifndef BREPGPU_NATIVE_CPU_PROFILE_HPP
#define BREPGPU_NATIVE_CPU_PROFILE_HPP
#include <version>
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13
#error "Native CPU profile requires libstdc++13 in the actual compilation unit"
#endif
#endif
