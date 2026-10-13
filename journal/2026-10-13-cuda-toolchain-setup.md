# Isolated CUDA toolchain setup, 2026-10-13

The operator explicitly authorized installing the CUDA compiler while design work continued. Acquired NVIDIA's pinned CUDA 12.8.1 manifest and four official Linux x86-64 component archives, checked their published SHA-256 digests and retained original archives and component licenses in a dedicated user-owned toolchain directory. Assembled a compiler/runtime prefix with nvcc 12.8.93, cudart 12.8.90, SDK C++ headers 12.8.90 and sanitizer component 12.8.93. No driver or system packages changed.

The first link failed because nvcc's Linux profile searches `lib64` but the component archive runtime is under `lib`. Studied the compiler profile and actual libraries, then added a relative `lib64` to `lib` symlink within the isolated prefix. Rebuilt successfully; no unsupported-host-compiler override was needed for GCC 13.3.

Compiled C++17 native `sm_60` cubin and shared-runtime executable from an owned setup probe stored in ignored context. Both P100 devices executed 257 FP64 polynomial inputs and matched the integer-derived analytic expectation exactly, with maximum absolute error zero. The loaded kernel binary version was 60, runtime API version 12080 and driver API version 13000. Driver version remained 580.178.04 on both cards.

Compute Sanitizer 2025.1.0.0 memcheck with full leak checking passed on both devices with zero reported errors and zero leaked bytes/allocations. CMake 3.28.3 identified the isolated compiler/toolkit and configured/built the C++17 probe with architecture `60-real`.

Wrote public setup commands and the archive acquisition catalog, updated the runtime study to mark the initial compiler blocker historical and resolved, and reported qualification to the integrated design owner. The setup probe is not a CAD product or performance result. No Git mutation or third-party CAD/binding adoption occurred.

At the design owner's request, published exact measured probe and CMake files under `scripts/toolchain-probe/`, with SHA-256 digests matching the execution-host source. Updated setup instructions to compile those checkout-relative files into ignored context outputs. This removes the unpublished-source dependency from public setup reproduction; probe behavior and measured outcomes are unchanged.
