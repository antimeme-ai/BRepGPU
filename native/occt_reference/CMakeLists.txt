cmake_minimum_required(VERSION 3.20)
project(BRepGPUOCCTReference LANGUAGES CXX)
find_package(OpenCASCADE REQUIRED)
add_library(brepgpu-occt-reference-uv STATIC ../source_uv/source_uv.cpp)
target_compile_features(brepgpu-occt-reference-uv PRIVATE cxx_std_17)
target_include_directories(brepgpu-occt-reference-uv SYSTEM PRIVATE ${OpenCASCADE_INCLUDE_DIR})
target_compile_options(brepgpu-occt-reference-uv PRIVATE -Wall -Wextra -Wpedantic
  -fno-fast-math -ffp-contract=off)
add_executable(brepgpu-occt-reference main.cpp)
target_compile_features(brepgpu-occt-reference PRIVATE cxx_std_17)
target_include_directories(brepgpu-occt-reference SYSTEM PRIVATE ${OpenCASCADE_INCLUDE_DIR})
target_link_directories(brepgpu-occt-reference PRIVATE ${OpenCASCADE_LIBRARY_DIR})
target_link_libraries(brepgpu-occt-reference PRIVATE brepgpu-occt-reference-uv
  TKMesh TKShHealing TKTopAlgo TKGeomAlgo TKBRep
  TKGeomBase TKG3d TKG2d TKMath TKernel)
target_compile_options(brepgpu-occt-reference PRIVATE -Wall -Wextra -Wpedantic -Werror
  -fno-fast-math -ffp-contract=off)
if(UNIX AND NOT APPLE)
  target_link_options(brepgpu-occt-reference PRIVATE -Wl,--disable-new-dtags)
endif()
set_target_properties(brepgpu-occt-reference PROPERTIES BUILD_RPATH "${OpenCASCADE_LIBRARY_DIR}")
