cmake_minimum_required(VERSION 3.28)
project(BRepGPU VERSION 0.1.0 LANGUAGES CXX)

option(BREPGPU_ENABLE_CUDA "Build the optional CUDA backend" OFF)
option(BREPGPU_ENABLE_SANITIZERS "Enable native host ASan/UBSan" OFF)
option(BREPGPU_ENABLE_TEST_HOOKS "Build guarded runtime fault/admission tooling" OFF)
find_package(Threads REQUIRED)

add_library(brepgpu SHARED native/runtime.cpp native/evaluator_cpu.cpp native/projection_cpu.cpp native/discovery_cpu.cpp)
target_include_directories(brepgpu PUBLIC include PRIVATE native)
target_compile_features(brepgpu PUBLIC cxx_std_17)
target_compile_options(brepgpu PRIVATE -Wall -Wextra -Wpedantic -fno-fast-math -ffp-contract=off)
target_link_libraries(brepgpu PRIVATE Threads::Threads ${CMAKE_DL_LIBS})
set_target_properties(brepgpu PROPERTIES BUILD_RPATH "$ORIGIN")
if(BREPGPU_ENABLE_SANITIZERS)
  target_compile_options(brepgpu PUBLIC -fsanitize=address,undefined -fno-omit-frame-pointer)
  target_link_options(brepgpu PUBLIC -fsanitize=address,undefined)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/benchmarks/projection/benchmark.cpp")
  add_executable(brepgpu_projection_benchmark benchmarks/projection/benchmark.cpp)
  target_link_libraries(brepgpu_projection_benchmark PRIVATE brepgpu)
  target_compile_options(brepgpu_projection_benchmark PRIVATE -fno-fast-math -ffp-contract=off)
  set_target_properties(brepgpu_projection_benchmark PROPERTIES BUILD_RPATH "$ORIGIN")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/benchmarks/discovery/benchmark.cpp")
  add_executable(brepgpu_discovery_benchmark benchmarks/discovery/benchmark.cpp)
  target_link_libraries(brepgpu_discovery_benchmark PRIVATE brepgpu)
  target_compile_options(brepgpu_discovery_benchmark PRIVATE -fno-fast-math -ffp-contract=off)
  set_target_properties(brepgpu_discovery_benchmark PROPERTIES BUILD_RPATH "$ORIGIN")
endif()

if(BREPGPU_ENABLE_CUDA)
  enable_language(CUDA)
  find_package(CUDAToolkit REQUIRED)
  add_library(brepgpu_cuda SHARED native/cuda_backend.cu)
  target_include_directories(brepgpu_cuda PRIVATE include native)
  target_link_libraries(brepgpu_cuda PRIVATE CUDA::cudart)
  set_target_properties(brepgpu_cuda PROPERTIES CUDA_STANDARD 17 CUDA_STANDARD_REQUIRED ON
    CUDA_ARCHITECTURES 60-real CUDA_RUNTIME_LIBRARY Shared
    BUILD_RPATH "$<TARGET_FILE_DIR:CUDA::cudart>")
  target_compile_options(brepgpu_cuda PRIVATE
    $<$<COMPILE_LANGUAGE:CUDA>:--fmad=false;--ftz=false;--prec-div=true;--prec-sqrt=true;-Xcompiler=-fno-fast-math,-ffp-contract=off>)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/benchmarks/iteration1/benchmark.cpp")
  add_executable(brepgpu_benchmark benchmarks/iteration1/benchmark.cpp)
  target_link_libraries(brepgpu_benchmark PRIVATE brepgpu)
  set_target_properties(brepgpu_benchmark PROPERTIES BUILD_RPATH "$ORIGIN")
endif()

if(BREPGPU_ENABLE_TEST_HOOKS)
  target_compile_definitions(brepgpu PRIVATE BG_ENABLE_TEST_HOOKS=1)
  add_library(brepgpu_fault_backend SHARED tests/runtime_fault_backend.cpp)
  target_include_directories(brepgpu_fault_backend PRIVATE include native)
  target_compile_features(brepgpu_fault_backend PRIVATE cxx_std_17)
  add_executable(brepgpu_runtime_faults tests/native_runtime_faults.cpp)
  target_include_directories(brepgpu_runtime_faults PRIVATE native)
  target_link_libraries(brepgpu_runtime_faults PRIVATE brepgpu brepgpu_fault_backend Threads::Threads)
  set_target_properties(brepgpu_runtime_faults PROPERTIES BUILD_RPATH "$ORIGIN")
  if(BREPGPU_ENABLE_SANITIZERS)
    target_compile_options(brepgpu_fault_backend PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(brepgpu_fault_backend PRIVATE -fsanitize=address,undefined)
  endif()
  enable_testing()
  add_executable(brepgpu_projection_mathcheck native/projection_check.cpp)
  target_include_directories(brepgpu_projection_mathcheck PRIVATE native)
  target_link_libraries(brepgpu_projection_mathcheck PRIVATE brepgpu)
  target_compile_options(brepgpu_projection_mathcheck PRIVATE -fno-fast-math -ffp-contract=off)
  add_test(NAME projection-native-math COMMAND brepgpu_projection_mathcheck)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/native/discovery_check.cpp")
    add_executable(brepgpu_discovery_mathcheck native/discovery_check.cpp)
    target_include_directories(brepgpu_discovery_mathcheck PRIVATE native)
    target_link_libraries(brepgpu_discovery_mathcheck PRIVATE brepgpu)
    target_compile_options(brepgpu_discovery_mathcheck PRIVATE -fno-fast-math -ffp-contract=off)
    add_test(NAME discovery-native-math COMMAND brepgpu_discovery_mathcheck)
  endif()
  foreach(scenario admission-close publication-oom publication-oom-fatal-cleanup cleanup-fatal download-fatal restore-failure
      projection-admission-close projection-parent-close projection-publication-oom-fatal-cleanup
      projection-download-fatal projection-diagnostic-alias evaluation-poison-projection projection-restore-failure projection-extension-refusal
      projection-extension-base projection-extension-size projection-extension-transient
      projection-extension-fatal projection-extension-fatal-ok projection-extension-fatal-unsupported
      discovery-parent-close discovery-admission-close discovery-publication-oom-fatal-cleanup
      hierarchy-publication-oom-fatal-cleanup discovery-query-publication-oom-fatal-cleanup
      discovery-download-fatal hierarchy-download-fatal hierarchy-bound-download-fatal
      discovery-diagnostic-alias discovery-restore-failure hierarchy-upload-failure
      discovery-query-allocation-failure discovery-extension-refusal discovery-extension-base
      discovery-extension-size discovery-extension-transient discovery-extension-fatal
      discovery-extension-fatal-ok discovery-extension-fatal-unsupported discovery-environment
      pair-parent-close pair-admission-close pair-index-publication-oom-fatal-cleanup
      pair-page-publication-oom-fatal-cleanup pair-mask-download-fatal pair-index-download-fatal
      evaluation-poison-pairs pair-upload-failure pair-restore-failure pair-extension-refusal
      pair-extension-base pair-extension-size pair-extension-transient pair-extension-fatal
      pair-extension-fatal-ok pair-extension-fatal-unsupported pair-alias-extents pair-environment)
    add_test(NAME runtime-${scenario} COMMAND brepgpu_runtime_faults ${scenario} $<TARGET_FILE:brepgpu_fault_backend>)
    set_tests_properties(runtime-${scenario} PROPERTIES TIMEOUT 30)
  endforeach()
endif()
