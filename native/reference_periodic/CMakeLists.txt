cmake_minimum_required(VERSION 3.28)
project(brepgpu_reference_periodic LANGUAGES CXX)
include(../../tooling/native_cpu_rigor.cmake)
find_package(OpenCASCADE REQUIRED)

add_library(brepgpu-periodic-exact STATIC ../mesher/exact.cpp)
target_include_directories(brepgpu-periodic-exact PUBLIC ../mesher)
brepgpu_native_cpu_rigor(brepgpu-periodic-exact)

# Reuse the adopted reference bridge without turning this unit into unrelated
# legacy warning cleanup. New preparation and exact code use the native profile.
add_library(brepgpu-periodic-source-uv STATIC ../source_uv/source_uv.cpp)
target_compile_features(brepgpu-periodic-source-uv PRIVATE cxx_std_20)
target_include_directories(brepgpu-periodic-source-uv SYSTEM PRIVATE ${OpenCASCADE_INCLUDE_DIR})
target_compile_options(brepgpu-periodic-source-uv PRIVATE -Wall -Wextra -Wpedantic -fno-fast-math -ffp-contract=off)

add_library(brepgpu-periodic-preparation STATIC prepare.cpp)
target_include_directories(brepgpu-periodic-preparation SYSTEM PUBLIC ${OpenCASCADE_INCLUDE_DIR})
target_link_directories(brepgpu-periodic-preparation PUBLIC ${OpenCASCADE_LIBRARY_DIR})
target_link_libraries(brepgpu-periodic-preparation PUBLIC brepgpu-periodic-source-uv brepgpu-periodic-exact
  TKShHealing TKTopAlgo TKGeomAlgo TKBRep TKGeomBase TKG3d TKG2d TKMath TKernel)
brepgpu_native_cpu_rigor(brepgpu-periodic-preparation)

add_executable(brepgpu-reference-periodic main.cpp)
add_executable(brepgpu-reference-periodic-fixture EXCLUDE_FROM_ALL ../../tests/reference_periodic_fixture.cpp)
foreach(target brepgpu-reference-periodic brepgpu-reference-periodic-fixture)
  target_link_libraries(${target} PRIVATE brepgpu-periodic-preparation TKPrim)
  brepgpu_native_cpu_rigor(${target})
  target_link_options(${target} PRIVATE -Wl,--disable-new-dtags)
  set_target_properties(${target} PROPERTIES BUILD_RPATH "${OpenCASCADE_LIBRARY_DIR}")
endforeach()
