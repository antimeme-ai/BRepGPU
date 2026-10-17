# Owned host-only settings for the native face-domain unit.
include_guard(GLOBAL)
set(BREPGPU_NATIVE_PROFILE "linux-clang18" CACHE STRING "Qualified native CPU profile")
set(BREPGPU_NATIVE_INSTRUMENTATION "none" CACHE STRING "none,address,thread,fuzz")
if(NOT BREPGPU_NATIVE_PROFILE STREQUAL "linux-clang18" OR
   NOT CMAKE_SYSTEM_NAME STREQUAL "Linux" OR
   NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR
   NOT CMAKE_CXX_COMPILER_VERSION VERSION_EQUAL "18.1.3")
  message(FATAL_ERROR "Native CPU profile requires Linux Clang 18.1.3; refusing fallback")
endif()
if(NOT BREPGPU_NATIVE_INSTRUMENTATION MATCHES "^(none|address|thread|fuzz)$")
  message(FATAL_ERROR "Unknown native CPU instrumentation; refusing fallback")
endif()
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
function(brepgpu_native_cpu_rigor target)
  target_compile_features(${target} PUBLIC cxx_std_20)
  set_target_properties(${target} PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
  target_compile_options(${target} PRIVATE
    -include "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/native_cpu_profile.hpp"
    -stdlib=libstdc++ -Wall -Wextra -Wpedantic -Werror -Wconversion
    -Wsign-conversion -Wshadow -Wnon-virtual-dtor -Wold-style-cast
    -Woverloaded-virtual -Wformat=2 -Wundef -Wimplicit-fallthrough
    -Werror=unknown-warning-option -fno-fast-math -ffp-contract=off)
  target_link_options(${target} PRIVATE -stdlib=libstdc++)
  if(BREPGPU_NATIVE_INSTRUMENTATION STREQUAL "address")
    target_compile_options(${target} PRIVATE -fsanitize=address,undefined
      -fno-sanitize-recover=all -fno-omit-frame-pointer)
    target_link_options(${target} PRIVATE -fsanitize=address,undefined)
  elseif(BREPGPU_NATIVE_INSTRUMENTATION STREQUAL "thread")
    target_compile_options(${target} PRIVATE -fsanitize=thread -fno-omit-frame-pointer)
    target_link_options(${target} PRIVATE -fsanitize=thread)
  elseif(BREPGPU_NATIVE_INSTRUMENTATION STREQUAL "fuzz")
    target_compile_options(${target} PRIVATE -fsanitize=fuzzer-no-link,address,undefined
      -fno-sanitize-recover=all -fno-omit-frame-pointer)
    target_link_options(${target} PRIVATE -fsanitize=address,undefined)
  endif()
endfunction()
